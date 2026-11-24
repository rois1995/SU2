"""Accumulate the FP2-M2 proof anatomy without retaining individual trace rows."""

from collections import Counter
from dataclasses import dataclass, field
import math
import struct


LENGTH_EDGES = (0, .002, .005, .01, .02, None)
CLASSES = ('incomplete', 'direct_only', 'hit', 'miss', 'bypass')
CACHES = ('hit', 'miss', 'bypass')
PHASES = ('collapse_report', 'handover_strict', 'proof')
RATIOS = ('<=1', '(1,2]', '(2,10]', '>10', 'not_finite', 'missing')
PLANES_FIELDS = ('triangles', 'groups', 'accepted', 'invalid', 'storage_refused', 'boundary_trimmed',
                 'trimmed_triangles')
MISC_KINDS = ('PLANE_REJECT', 'BOUND', 'REFINE', 'UNRESOLVED')
SURFACE = b'NATIVE_SURFACE_'
CLASSIFIED_ROWS = (b'NATIVE_GEOMETRY_WORK', SURFACE + b'QUERY', SURFACE + b'QUERY_RESULT',
                   SURFACE + b'DOMAIN', SURFACE + b'MODEL', SURFACE + b'PLANES', SURFACE + b'GRAPH')
PHASE_ROWS = CLASSIFIED_ROWS[:3]


def divide(numerator, denominator):
    return numerator / denominator if denominator else None


def histogram(values):
    n = sum(values.values())
    if not n:
        return {'n': 0, 'mean': None, 'median': None, 'p90': None, 'maximum': None}
    median = p90 = None
    seen = 0
    for value, count in sorted(values.items()):
        seen += count
        if median is None and seen >= (n + 1) // 2:
            median = value
        if seen >= (9 * n + 9) // 10:
            p90 = value
            break
    return {'n': n, 'mean': sum(value * count for value, count in values.items()) / n,
            'median': median, 'p90': p90, 'maximum': max(values)}


def fields(tail):
    return dict(part.split(b'=', 1) for part in tail.split())


def outer_key(row):
    return row[b'authority'], row[b'budget'], int(row[b'start'])


def outer_tolerance(bits):
    value, = struct.unpack('>d', bytes.fromhex(bits.decode('ascii')))
    if not math.isfinite(value) or value <= 0:
        raise ValueError('invalid outer tolerance')
    return value


@dataclass
class TraceRows:
    counts: Counter = field(default_factory=Counter)
    exclusive: Counter = field(default_factory=Counter)
    pending: Counter = field(default_factory=Counter)
    pairs: Counter = field(default_factory=Counter)
    unmatched_results: int = 0
    orphaned_queries: int = 0
    segments: int = 0
    ended_segments: int = 0

    def add(self, kind, row):
        self.counts[kind] += 1
        if kind == 'NATIVE_SURFACE_QUERY_OUTER':
            self.pending[outer_key(row)] += 1
        elif kind == 'NATIVE_SURFACE_QUERY_RESULT':
            key = outer_key(row)
            cache = row[b'cache'].decode('ascii')
            int(row[b'work'])
            if cache not in CACHES:
                raise ValueError('invalid cache result')
            if not self.pending[key]:
                self.unmatched_results += 1
                return
            self.pairs[cache] += 1
            self.pending[key] -= 1
            if not self.pending[key]:
                del self.pending[key]
        elif kind == 'NATIVE_GEOMETRY_WORK':
            self.exclusive[row[b'operation'].decode('ascii')] += int(row[b'exclusive'])

    def close_pending(self):
        self.orphaned_queries += sum(self.pending.values())
        self.pending.clear()

    def report(self):
        return {'row_counts': dict(self.counts), 'paired_by_cache': {k: self.pairs[k] for k in CACHES},
                'outer_without_result': self.orphaned_queries + sum(self.pending.values()),
                'result_without_outer': self.unmatched_results, 'exclusive_work': dict(self.exclusive),
                'segments': self.segments, 'ended_segments': self.ended_segments}


class Forward:
    def __init__(self, ordinal, length_bin, direct, sample):
        self.ordinal = ordinal
        self.length_bin = length_bin
        self.direct = direct
        self.sample = sample
        self.tolerance = None
        self.root_ratio = None
        self.depth = None
        self.totals = Counter()
        self.planes = Counter()
        self.rows = TraceRows()
        self.malformed = False

    def add(self, kind, row):
        self.rows.add(kind, row)
        totals = self.totals
        if kind == 'NATIVE_SURFACE_QUERY_OUTER':
            self.tolerance = outer_tolerance(row[b'tolerance'])
            totals['support_ids'] += row[b'support'].count(b',')
        elif kind == 'NATIVE_SURFACE_QUERY_RESULT':
            totals['result_work'] += int(row[b'work'])
        elif kind == 'NATIVE_SURFACE_MODEL':
            totals['model_rows'] += 1
            totals['pieces'] += int(row[b'pieces'])
            totals['witness'] += int(row[b'witness'])
        elif kind == 'NATIVE_SURFACE_PLANES':
            totals['planes_rows'] += 1
            for key in PLANES_FIELDS:
                self.planes[key] += int(row[key.encode('ascii')])
        elif kind == 'NATIVE_SURFACE_GRAPH':
            totals['graph_attempts'] += 1
            upper = float(row[b'bound'])
            totals['graph_within_tolerance'] += self.within(upper)
            totals['coverage_work'] += int(row[b'coverage_work'])
            totals['lift_work'] += int(row[b'lift_work'])
        elif kind == 'NATIVE_SURFACE_DOMAIN' and row[b'model'] == b'0':
            upper = float(row[b'upper'])
            depth = int(row[b'depth'])
            totals['prepared_physical_nodes'] += 1
            totals['prepared_within_tolerance'] += self.within(upper)
            self.depth = max(depth, self.depth if self.depth is not None else depth)
            if depth == 0 and self.root_ratio is None and self.tolerance is not None:
                self.root_ratio = upper / self.tolerance
            place = 'root' if depth == 0 else 'other'
            totals[f'{place}_upper_work'] += int(row[b'upper_work'])
            totals[f'{place}_distance_work'] += int(row[b'distance_work'])

    def within(self, bound):
        return self.tolerance is not None and math.isfinite(bound) and bound <= self.tolerance

    def classify(self, answer):
        outer = self.rows.counts['NATIVE_SURFACE_QUERY_OUTER']
        if answer in (None, b'threw') or self.malformed or self.rows.pending or self.rows.unmatched_results or outer > 1:
            return 'incomplete'
        if not outer:
            return 'direct_only'
        if sum(self.rows.pairs.values()) != 1:
            return 'incomplete'
        return next(iter(self.rows.pairs))


class Group:
    def __init__(self):
        self.calls = Counter()
        self.totals = Counter()
        self.nodes = Counter()
        self.depths = Counter()
        self.ratios = Counter()
        self.planes = Counter()
        self.exclusive = Counter()
        self.trace_rows = Counter()
        self.seconds = Counter()

    def add(self, forward, classification, seconds):
        self.calls[classification] += 1
        self.trace_rows.update(forward.rows.counts)
        if classification == 'incomplete':
            return
        self.seconds[classification] += seconds
        if classification not in ('miss', 'bypass'):
            return
        total = forward.totals
        nodes = total['prepared_physical_nodes']
        self.nodes[nodes] += 1
        if forward.depth is not None:
            self.depths[forward.depth] += 1
        self.ratios[ratio_bin(forward.root_ratio)] += 1
        self.totals.update(total)
        self.totals['root_enough'] += nodes == 1 and total['prepared_within_tolerance'] == 1
        self.totals['forwards_with_planes'] += total['planes_rows'] > 0
        self.totals['forwards_with_model'] += total['model_rows'] > 0
        self.planes.update(forward.planes)
        self.exclusive.update(forward.rows.exclusive)

    def report(self):
        total = self.totals
        n = self.calls['miss'] + self.calls['bypass']
        upper = total['root_upper_work'] + total['other_upper_work']
        distance = total['root_distance_work'] + total['other_distance_work']
        work_parts = {'coverage': total['coverage_work'], 'lift': total['lift_work'],
                      'other_upper': upper - total['coverage_work'] - total['lift_work'],
                      'distance': distance, 'other_result': total['result_work'] - upper - distance}
        work = total['result_work']
        operations = sum(self.exclusive.values())
        by_operation = sorted(self.exclusive.items(), key=lambda item: (-item[1], item[0]))
        nodes = total['prepared_physical_nodes']
        return {
            'calls_by_class': {k: self.calls[k] for k in CLASSES}, 'calls': sum(self.calls.values()),
            'seconds_by_class': {k: self.seconds[k] for k in CLASSES if k != 'incomplete'},
            'miss_bypass_denominators': {'forwards': n, 'prepared_nodes': nodes,
                                         'root_ratio_forwards': n, 'root_ratio_present': n - self.ratios['missing'],
                                         'planes_rows': total['planes_rows'], 'model_rows': total['model_rows'],
                                         'graph_attempts': total['graph_attempts'], 'result_work': work,
                                         'exclusive_work': operations},
            'prepared_physical_nodes': histogram(self.nodes),
            'prepared_within_tolerance': total['prepared_within_tolerance'],
            'root_enough': total['root_enough'],
            'root_enough_share': divide(total['root_enough'], n),
            'root_ratio': {k: {'count': self.ratios[k], 'share': divide(self.ratios[k], n)} for k in RATIOS},
            'largest_depth': histogram(self.depths),
            'means_per_forward': {k: divide(total[k], n) for k in ('support_ids', 'witness', 'pieces', 'graph_attempts')},
            'forwards_with_model': total['forwards_with_model'],
            'forwards_with_planes': total['forwards_with_planes'],
            'forwards_with_planes_share': divide(total['forwards_with_planes'], n),
            'means_per_planes_row': {k: divide(self.planes[k], total['planes_rows']) for k in PLANES_FIELDS},
            'graph_within_tolerance': total['graph_within_tolerance'],
            'graph_within_tolerance_share': divide(total['graph_within_tolerance'], total['graph_attempts']),
            'work_per_forward': divide(work, n),
            'work_per_prepared_physical_node': divide(work, nodes),
            'result_work_parts': {k: {'work': count, 'share': divide(count, work)} for k, count in work_parts.items()},
            'root_and_other_work': {k: total[k] for k in
                                    ('root_upper_work', 'other_upper_work', 'root_distance_work', 'other_distance_work')},
            'top_operations': [{'operation': k, 'exclusive': v, 'share': divide(v, operations)}
                               for k, v in by_operation[:10]],
            'exclusive_by_operation': dict(self.exclusive),
            'trace_row_counts': {**{f'NATIVE_SURFACE_{kind}': 0 for kind in MISC_KINDS}, **self.trace_rows},
        }


def ratio_bin(ratio):
    if ratio is None:
        return 'missing'
    if not math.isfinite(ratio):
        return 'not_finite'
    if ratio <= 1:
        return '<=1'
    if ratio <= 2:
        return '(1,2]'
    return '(2,10]' if ratio <= 10 else '>10'


class TraceAnalyzer:
    def __init__(self, sample, unresolved, sample_limit=300):
        self.sample = sample
        self.unresolved = unresolved
        self.sample_limit = sample_limit
        self.forwards_by_bin = Counter()
        self.groups = {**{str(i): Group() for i in range(5)}, 'direct_eligible_1': Group()}
        self.phases = {name: TraceRows() for name in (*PHASES, 'outside')}
        self.forward = None
        self.phase = 'outside'
        self.started = 0
        self.ended = 0
        self.last_ordinal = None
        self.sample_lines = 0
        self.unresolved_lines = 0
        self.trace_lines = 0
        self.errors = []
        self.error_count = 0
        self.partial = False
        self.finished = False

    def error(self, error, line=None):
        self.error_count += 1
        if len(self.errors) < 8:
            self.errors.append({'message': str(error), 'trace_line': self.trace_lines,
                                'text': line[:512].decode('utf-8', 'replace').rstrip() if line else None})
        if self.forward:
            self.forward.malformed = True

    def feed(self, line):
        self.trace_lines += 1
        if line.startswith(SURFACE + b'UNRESOLVED '):
            self.unresolved.write(line)
            self.unresolved_lines += 1
        if not line.endswith(b'\n'):
            self.partial = True
            if self.forward and self.forward.sample:
                self.write_sample(line)
            return
        try:
            if line.startswith(b'FP2_M2 Q '):
                self.mark_forward(line)
            elif line.startswith(b'FP2_M2 PHASE '):
                self.mark_phase(line)
            else:
                self.add_row(line)
        except (KeyError, OverflowError, ValueError, struct.error) as error:
            self.error(error, line)

    def add_row(self, line):
        if self.forward and self.forward.sample:
            self.write_sample(line)
        kind, _, tail = line.partition(b' ')
        kind = kind.rstrip()
        row = fields(tail) if kind in (CLASSIFIED_ROWS if self.forward else PHASE_ROWS) else {}
        if kind == SURFACE + b'QUERY':
            kind += b'_OUTER' if b'authority' in row else b'_INNER'
        name = kind.decode('ascii')
        if self.forward:
            self.forward.add(name, row)
        else:
            self.phases[self.phase].add(name, row)

    def write_sample(self, line):
        self.sample.write(line)
        self.sample_lines += 1

    def mark_forward(self, line):
        direction, _, tail = line[len(b'FP2_M2 Q '):].partition(b' ')
        row = fields(tail)
        ordinal = int(row[b'ordinal'])
        if direction == b'begin':
            length_bin = int(row[b'bin'])
            direct = int(row[b'direct_eligible'])
            if length_bin not in range(5) or direct not in (0, 1):
                raise ValueError('invalid forward bin or direct flag')
            if self.phase != 'outside':
                self.error('forward starts inside a marked phase', line)
            if self.forward:
                self.close_forward(None)
            if self.last_ordinal is not None and ordinal <= self.last_ordinal:
                self.error('forward ordinals do not increase', line)
            self.phases['outside'].close_pending()
            self.last_ordinal = ordinal
            self.forwards_by_bin[length_bin] += 1
            self.started += 1
            sample = self.forwards_by_bin[length_bin] <= self.sample_limit
            self.forward = Forward(ordinal, length_bin, direct, sample)
            if sample:
                self.write_sample(line)
        elif direction == b'end':
            if not self.forward or self.forward.ordinal != ordinal:
                self.error('end marker without matching forward', line)
                return
            if self.forward.sample:
                self.write_sample(line)
            seconds = float(row[b'seconds'])
            if not math.isfinite(seconds) or seconds < 0:
                raise ValueError('invalid forward seconds')
            self.ended += 1
            self.close_forward(row[b'answer'], seconds)
        else:
            raise ValueError('invalid forward marker')

    def close_forward(self, answer, seconds=0):
        forward, self.forward = self.forward, None
        self.partial |= answer is None
        group = 'direct_eligible_1' if forward.direct else str(forward.length_bin)
        self.groups[group].add(forward, forward.classify(answer), seconds)

    def mark_phase(self, line):
        _, name, marker = line[len(b'FP2_M2 '):].rstrip().split()
        prefix, sep, name = name.partition(b'=')
        name = name.decode('ascii')
        if prefix != b'name' or not sep or name not in PHASES:
            raise ValueError('unexpected phase')
        if self.forward:
            self.error('phase marker inside a forward', line)
            self.close_forward(None)
        self.phases[self.phase].close_pending()
        if marker == b'begin' and self.phase == 'outside':
            self.phase = name
            self.phases[name].segments += 1
        elif marker == b'end' and self.phase == name:
            self.phases[name].ended_segments += 1
            self.phase = 'outside'
        else:
            self.error('phase marker without matching boundary', line)

    def finish(self):
        if not self.finished:
            self.partial |= self.forward is not None or self.phase != 'outside'
            if self.forward:
                self.close_forward(None)
            self.finished = True
        stream = {'trace_lines': self.trace_lines, 'forwards_started': self.started, 'forwards_ended': self.ended,
                  'forwards_by_length_bin': {str(i): self.forwards_by_bin[i] for i in range(5)},
                  'sample_forwards_by_length_bin': {str(i): min(self.forwards_by_bin[i], self.sample_limit)
                                                    for i in range(5)},
                  'sample_lines': self.sample_lines, 'unresolved_lines': self.unresolved_lines,
                  'error_count': self.error_count, 'errors': list(self.errors)}
        completion = 'failed' if self.error_count else 'partial' if self.partial else 'complete'
        groups = {key: group.report() for key, group in self.groups.items()}
        for i in range(5):
            groups[str(i)].update(direct_eligible=0, lower_m=LENGTH_EDGES[i], upper_m=LENGTH_EDGES[i + 1])
        groups['direct_eligible_1']['direct_eligible'] = 1
        return {'completion': completion, 'groups': groups, 'phases': {k: v.report() for k, v in self.phases.items()},
                'stream': stream}
