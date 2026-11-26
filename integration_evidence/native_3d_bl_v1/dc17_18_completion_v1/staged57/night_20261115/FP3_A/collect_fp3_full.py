"""Collect FP3-A suppression and probe evidence from the separate rank stdouts."""

import argparse
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path
import re
from tempfile import TemporaryDirectory


STAMP = r'[A-Z][a-z]{2} [A-Z][a-z]{2} [ \d]\d \d{2}:\d{2}:\d{2} \d{4}'
PREFIX = re.compile(fr'(?P<stamp>{STAMP})?\s*(?:\[\d+,\d+\])?<(?:stdout|stderr)>:')
KINDS = re.compile(
    r'\bNATIVE3D(?:_(?:PARTITION|SEED_SHORT|LENGTH|STAGE|DETAIL|LOCAL_CORE|LOCAL_SURFACE_STATE|'
    r'QUALITY_LOCATION|TIMER_SUMMARY|FP3_ENGINE_COLLAPSE|FP3_HANDOVER_SURFACE))?(?=\s|$)'
    r'|Native3D frozen stages:'
)
FIELDS = re.compile(r'(?:^|\s)(\w+)=([^\s]*)')
STAGES = {
    'length_collapse_exit', 'length_split_exit', 'split_surface_exit', 'split_interior_entry',
    'construction_entry', 'quality_entry', 'quality_exit', 'exit', 'metric_cleanup_entry', 'metric_cleanup_exit',
}
ENGINE_PROBE = 'NATIVE3D_FP3_ENGINE_COLLAPSE'
CORE_PROBE = 'NATIVE3D_FP3_HANDOVER_SURFACE'
REMOVALS = ('removed_volume', 'removed_surface_2', 'removed_surface_1', 'removed_surface_0', 'unclassified')
ENGINE_COUNTS = ('accepted', *REMOVALS, 'shared', 'local', 'old_cells', 'old_bad', 'old_unmeasured',
                 'fresh_cells', 'fresh_bad', 'fresh_unmeasured')
REFUSALS = ('None', 'Protected', 'Border', 'Ineligible', 'BorderKeeper', 'FeatureChord', 'Link', 'Near',
            'Outward', 'Reverse', 'Topology', 'Size', 'Shape', 'Validity', 'Distance', 'Entity', 'Bound',
            'ContactRemote', 'Contact')
CORE_COUNTS = ('short_surface_edges', 'prepare_pass', *REFUSALS, 'protected_same_curve', 'unclassified')
METADATA = {'NATIVE3D', 'NATIVE3D_DETAIL', 'NATIVE3D_TIMER_SUMMARY'}


@dataclass
class Record:
    kind: str
    fields: dict
    line: int
    stamp: str | None
    cut: bool = False

    def number(self, key):
        if self.cut:
            raise ValueError('cut row')
        raw = self.fields.get(key, '')
        if not raw.isdecimal():
            raise ValueError(f'{key} is missing or malformed')
        return int(raw)

    def seconds(self, key='elapsed'):
        try:
            value = float(self.fields.get(key, ''))
        except ValueError:
            return None
        return value if math.isfinite(value) and value >= 0 and not self.cut else None

    def optional(self, key):
        try:
            return self.number(key)
        except ValueError:
            return None


@dataclass
class RankEvent:
    rank: int
    event: int
    path: Path
    records: list = field(default_factory=list)

    def issue(self, problem, rec=None, stage=None, **details):
        return {'event': self.event, 'rank': self.rank, 'worker_stage': stage, 'file': str(self.path),
                'line': rec.line if rec else None, 'field': problem, **details}


def scan(path, rank):
    """Break each rank's own stdout at its partition rows, keeping a digest of the original bytes."""
    digest = hashlib.sha256()
    events, issues, probe_issues = [], [], []
    with path.open('rb') as stream:
        for number, raw in enumerate(stream, 1):
            text = raw.decode('utf-8', errors='replace')
            prefix = PREFIX.search(text)
            stamp = prefix.group('stamp') if prefix else None
            line = PREFIX.sub('', text)
            matches = list(KINDS.finditer(line))
            digest.update(raw)
            for i, match in enumerate(matches):
                fragment = line[match.end():matches[i + 1].start() if i + 1 < len(matches) else None]
                rec = Record(match.group(), dict(FIELDS.findall(fragment)), number, stamp,
                             not raw.endswith(b'\n'))
                fields = rec.fields
                if rec.kind == 'Native3D frozen stages:':
                    if rank == 0 and events:
                        events[-1].records.append(rec)
                    continue
                target_rank = rec.optional('rank')
                if rank == 0 and events and target_rank in range(6) and (
                        rec.kind == 'NATIVE3D' or
                        rec.kind == 'NATIVE3D_DETAIL' and fields.get('stage') == 'refine'):
                    # The house runner serializes the parent report on rank 0.
                    events[-1].records.append(rec)
                    continue
                if not events and rec.kind not in ('NATIVE3D_PARTITION', 'NATIVE3D_SEED_SHORT', ENGINE_PROBE, CORE_PROBE):
                    continue
                important = (rec.kind in {'NATIVE3D_PARTITION', 'NATIVE3D_SEED_SHORT', 'NATIVE3D_LENGTH',
                                         'NATIVE3D_STAGE', CORE_PROBE, ENGINE_PROBE})
                if target_rank is None and important:
                    event = events[-1] if events else RankEvent(rank, 0, path)
                    first = event.issue('rank is missing or malformed', rec, row=rec.kind)
                    if rec.kind in (CORE_PROBE, ENGINE_PROBE):
                        probe_issues.append(first)
                    else:
                        issues.append(first)
                        if events:
                            event.records.append(Record('gap', {}, number, stamp, True))
                    continue
                if target_rank != rank:
                    continue
                if rec.kind == 'NATIVE3D_PARTITION':
                    events.append(RankEvent(rank, len(events) + 1, path, [rec]))
                elif not events:
                    if important:
                        issues.append(RankEvent(rank, 0, path).issue('row precedes first partition', rec, row=rec.kind))
                elif rec.kind == 'NATIVE3D_STAGE':
                    stage = fields.get('stage')
                    if stage in STAGES or not stage:
                        events[-1].records.append(rec)
                elif rec.kind == 'NATIVE3D_DETAIL':
                    if fields.get('stage') == 'refine':
                        events[-1].records.append(rec)
                elif rec.kind == 'NATIVE3D_TIMER_SUMMARY':
                    if fields.get('root') == 'remesh' and fields.get('boundary') == 'final':
                        events[-1].records.append(rec)
                else:
                    events[-1].records.append(rec)
    return events, issues, probe_issues, digest.hexdigest()


def elapsed_between(start, end):
    return end - start if start is not None and end is not None and end >= start else None


def timestamp_between(start, end):
    if start and end and start.stamp and end.stamp:
        structure = '%a %b %d %H:%M:%S %Y'
        t1, t2 = (datetime.strptime(rec.stamp, structure) for rec in (start, end))
        return elapsed_between(t1.timestamp(), t2.timestamp())
    return None


def projection(rec, *keys):
    if rec is None:
        return None
    row = {key: rec.optional(key) for key in keys}
    return {**row, 'line': rec.line, 'elapsed_seconds': rec.seconds()}


def inspect_packet(event, index, records, core, detail, suppress):
    """Validate one engine invocation without carrying counts across workers."""
    measurements, results = [], []
    seed, *body = records
    length = None
    collapse_rows, reentries, cleanups, splits, quality_spans = [], [], [], [], []
    handover = next((rec for rec in reversed(core) if rec.kind == 'NATIVE3D_LOCAL_SURFACE_STATE'), None)
    last_pass = next((rec for rec in reversed(core) if rec.kind == 'NATIVE3D_LOCAL_CORE'), None)
    initial, initial_length, final, quality_location = None, None, None, None
    initial_effect_unknown = seed.cut
    initial_phase_closed = False
    quality_begin, quality_exit, required_reentry, cleanup_entry = None, None, None, None
    reentry_evidence_gap = False
    reentry_direction_seen = False
    cleanup_total, cleanup_known = 0, True
    split_clock_at = None

    def measure(rec, problem, **details):
        measurements.append(event.issue(problem, rec, index, **details))

    def result(rec, problem, expected, actual):
        if suppress:
            results.append(event.issue(problem, rec, index, expected=expected, actual=actual))

    def compare(rec, field_name, expected, seen):
        if expected is not None and seen is not None and expected != seen:
            result(rec, field_name, expected, seen)

    def number(rec, key):
        try:
            return rec.number(key)
        except ValueError as error:
            measure(rec, key, error=str(error), row=rec.kind)
            return None

    for rec in body:
        if rec.kind == ENGINE_PROBE:
            continue
        if rec.kind == 'gap' or rec.cut:
            initial_effect_unknown |= not initial_phase_closed
            reentry_evidence_gap |= required_reentry is not None
            cleanup_known = False
            measure(rec, 'cut or malformed worker-stage evidence', row=rec.kind)
            continue
        if rec.kind == 'NATIVE3D_QUALITY_LOCATION':
            if rec.optional('census') == 0 and quality_location is None:
                quality_location = rec
            continue
        stage = rec.fields.get('stage')
        if rec.kind == 'NATIVE3D_LENGTH':
            if rec.fields.get('pass') == 'collapse':
                if length is not None:
                    measure(rec, 'unpaired collapse LENGTH', previous_line=length.line)
                length = rec
                if not initial_phase_closed and suppress:
                    compare(rec, 'initial collapse_accepted', 0, number(rec, 'collapse_accepted'))
                continue
            if rec.fields.get('pass') != 'split':
                measure(rec, 'length direction missing or malformed', value=rec.fields.get('pass'))
                initial_effect_unknown |= not initial_phase_closed
                continue
            stage = 'split_LENGTH'
        if stage in {'split_LENGTH', 'length_split_exit', 'split_surface_exit', 'split_interior_entry',
                     'quality_entry', 'quality_exit', 'exit'}:
            if not initial_phase_closed:
                if length is not None or initial_effect_unknown:
                    measure(rec, 'initial collapse stage/LENGTH pair incomplete', boundary=stage)
                else:
                    result(rec, 'initial collapse boundary before later phase', 'length_collapse_exit', stage)
                initial_phase_closed = True
                length = None
            if required_reentry is not None:
                if length is not None or reentry_evidence_gap:
                    measure(rec, 'construction reentry collapse stage row missing', boundary=stage)
                else:
                    result(rec, 'construction reentry collapse boundary', 'length_collapse_exit', stage)
                required_reentry = None
                length = None
            if quality_exit is not None and not reentry_direction_seen:
                measure(rec, 'quality exit not followed by a construction entry', boundary=stage)
                quality_exit = None
        if stage == 'split_LENGTH':
            end = rec.seconds()
            row = projection(rec, 'split_accepted', 'short_local_edges', 'long_local_edges', 'cells')
            splits.append({**row, 'seconds_since_previous_length_boundary': elapsed_between(split_clock_at, end)})
            split_clock_at = end
        elif stage == 'metric_cleanup_entry':
            if cleanup_entry is not None:
                measure(rec, 'nested metric cleanup entry', previous_line=cleanup_entry.line)
                cleanup_known = False
            cleanup_entry = rec
        elif stage == 'metric_cleanup_exit':
            if cleanup_entry is None:
                measure(rec, 'metric cleanup exit without entry')
                cleanup_known = False
                continue
            opening = number(cleanup_entry, 'length_collapses')
            closing = number(rec, 'length_collapses')
            delta = closing - opening if opening is not None and closing is not None else None
            if delta is None:
                cleanup_known = False
            elif delta < 0:
                result(rec, 'metric cleanup collapse increase', 'nonnegative', delta)
                cleanup_known = False
            else:
                cleanup_total += delta
            cleanups.append({'entry': projection(cleanup_entry, 'rounds', 'length_collapses'),
                             'exit': projection(rec, 'rounds', 'length_collapses'), 'accepted': delta})
            cleanup_entry = None
        elif stage == 'length_collapse_exit':
            rounds = number(rec, 'rounds')
            accepted = number(rec, 'length_collapses')
            if length is None:
                measure(rec, 'collapse stage without preceding unpaired LENGTH')
                length_accepted = None
            else:
                length_accepted = number(length, 'collapse_accepted')
            if not initial_phase_closed:
                initial, initial_length = rec, length
                initial_phase_closed = True
                compare(rec, 'initial rounds', 0, rounds)
                compare(rec, 'initial length_collapses', 0, accepted)
                compare(rec, 'initial collapse_accepted', 0, length_accepted)
            elif quality_exit is None:
                measure(rec, 'later collapse exit without preceding quality exit')
            else:
                compare(rec, 'construction reentry rounds', number(quality_exit, 'rounds'), rounds)
            compare(rec, 'stated collapse count', accepted, length_accepted)
            if cleanup_known and cleanup_entry is None:
                compare(rec, 'cumulative collapses at construction exit', cleanup_total, accepted)
            if reentries and quality_exit is not None:
                reentries[-1]['collapse_exit_line'] = rec.line
            collapse_rows.append({'stage': projection(rec, 'rounds', 'length_collapses'),
                                  'length': projection(length, 'collapse_accepted', 'short_local_edges',
                                                       'long_local_edges', 'cells'),
                                  'completed_cleanup_increase': cleanup_total if cleanup_known else None})
            length, quality_exit, required_reentry = None, None, None
            reentry_evidence_gap = False
            split_clock_at = rec.seconds()
        elif stage == 'construction_entry' and quality_exit is not None:
            lane = number(rec, 'split_lane')
            if lane not in (0, 1):
                measure(rec, 'construction reentry direction unknown', split_lane=lane)
            else:
                required_reentry = rec if lane == 0 else None
                reentry_evidence_gap = False
                reentry_direction_seen = True
                reentries.append({'quality_exit': projection(quality_exit, 'rounds', 'length_collapses'),
                                  'construction_entry': projection(rec, 'split_lane', 'rounds', 'length_collapses'),
                                  'direction': 'geometry_split' if lane == 1 else 'collapse'})
                if lane == 1:
                    split_clock_at = rec.seconds()
        elif stage == 'quality_entry':
            quality_exit = None
            if quality_begin is None:
                quality_begin = rec
        elif stage in ('quality_exit', 'exit'):
            if quality_begin is not None:
                quality_spans.append({'entry_line': quality_begin.line, 'exit_line': rec.line,
                                      'seconds': elapsed_between(quality_begin.seconds(), rec.seconds())})
                quality_begin = None
            if stage == 'quality_exit':
                quality_exit = rec
                reentry_direction_seen = False
            else:
                final = rec
                accepted = number(rec, 'length_collapses')
                if cleanup_known and cleanup_entry is None:
                    compare(rec, 'cumulative collapses at final engine exit', cleanup_total, accepted)
        elif rec.kind == 'NATIVE3D_STAGE' and not stage:
            measure(rec, 'stage name is missing or malformed')
            initial_effect_unknown |= not initial_phase_closed
            reentry_evidence_gap |= required_reentry is not None
            cleanup_known = False
    if cleanup_entry is not None:
        measure(cleanup_entry, 'metric cleanup entry without exit')
    if length is not None:
        measure(length, 'collapse LENGTH without exit stage')
    if final is None:
        measure(seed, 'worker stage has no complete engine exit')
    if suppress and initial is None and not results:
        measure(seed, 'worker stage has no initial collapse pair')

    pass_seconds = None
    if initial_length is not None and detail is not None:
        clock = [initial_length.seconds(), detail.seconds('initialization'), detail.seconds('seed')]
        if all(value is not None for value in clock):
            pass_seconds = clock[0] - clock[1] - clock[2]
    stage = {
        'event': event.event, 'stage': index, 'rank': event.rank, 'seed_line': seed.line,
        'seed': projection(seed, 'cells', 'edges', 'lt_1e_2', 'lt_0_1'),
        'handover': projection(handover, 'cells', 'faces', 'points'),
        'handover_to_first_collapse_exit_seconds': timestamp_between(handover, initial),
        'collapse_length_elapsed_seconds': initial_length.seconds() if initial_length else None,
        'collapse_pass_seconds': pass_seconds,
        'collapse_pass_time_status': 'measured' if pass_seconds is not None else 'pass time not available',
        'construction_exits': collapse_rows, 'construction_reentries': reentries,
        'metric_cleanups': cleanups, 'split_generations': splits, 'quality_spans': quality_spans,
        'quality_seconds': (sum(row['seconds'] for row in quality_spans)
                            if quality_spans and all(row['seconds'] is not None for row in quality_spans) else None),
        'core_last_pass_bad_out': last_pass.optional('bad_out') if last_pass else None,
        'core_last_pass_short_out': last_pass.optional('short_out') if last_pass else None,
        'quality_entry_bad_union': quality_location.optional('bad_union') if quality_location else None,
        'final_engine': projection(final, 'rounds', 'length_collapses', 'length_splits',
                                   'cells', 'associations', 'working_faces', 'job_peak_rss_lower'),
    }
    return stage, measurements, results


def inspect_probe(event, rec, index, probe, stage):
    expected_kind = CORE_PROBE if probe == 1 else ENGINE_PROBE
    row = {'event': event.event, 'stage': index, 'rank': event.rank, 'line': rec.line,
           'status': 'incomplete', 'kind': rec.kind}
    why = None
    if rec.kind != expected_kind:
        return row, 'unexpected probe row kind'
    if probe == 1:
        row.update(checkpoint=rec.fields.get('checkpoint'), proof=rec.fields.get('proof'))
        if rec.optional('stage') != index:
            why = 'core probe stage does not match the associated seed'
        elif row['checkpoint'] != 'last_pass_pre_shift' or row['proof'] != 'not_evaluated':
            why = 'wrong core probe checkpoint or proof status'
    keys = CORE_COUNTS if probe == 1 else ENGINE_COUNTS
    row.update({key: rec.optional(key) for key in keys})
    if rec.fields.get('status') not in ('ok', 'complete', 'passed'):
        why = why or 'probe status is absent or incomplete'
    if rec.cut or next(reversed(rec.fields), None) != 'status' or any(row[key] is None for key in keys):
        why = why or 'probe row is cut or malformed'
    if why is None and probe == 1:
        edges = row['short_surface_edges']
        if (row['None'] or row['prepare_pass'] + sum(row[key] for key in REFUSALS) != edges or
                row['protected_same_curve'] > edges or row['unclassified'] > edges):
            why = 'core probe classification counts do not close'
    elif why is None:
        exits = stage['construction_exits']
        initial = exits[0]['stage']['length_collapses'] if exits else None
        if (initial is None or row['accepted'] != initial or
                row['accepted'] != sum(row[key] for key in REMOVALS) or
                row['accepted'] != row['shared'] + row['local'] or
                any(row[f'{age}_bad'] + row[f'{age}_unmeasured'] > row[f'{age}_cells']
                    for age in ('old', 'fresh'))):
            why = 'engine probe counts do not close against committed initial collapses'
    row['status'] = 'incomplete' if why else 'complete'
    return row, why


def inspect_event(event, worker_count, suppress, probe, detail):
    packets, stage_rows, measurements, failures, probe_rows, probe_issues = [], [], [], [], [], []
    core, pending = defaultdict(list), []
    meta = {}
    for rec in event.records:
        if rec.kind == 'NATIVE3D_SEED_SHORT':
            if pending:
                packets.append(pending)
            pending = [rec]
        elif rec.kind in ('NATIVE3D_LOCAL_CORE', 'NATIVE3D_LOCAL_SURFACE_STATE', CORE_PROBE):
            stage = rec.optional('stage')
            if stage is None:
                if rec.kind == CORE_PROBE:
                    probe_issues.append(event.issue('core probe worker stage missing', rec))
            else:
                core[stage].append(rec)
        elif rec.kind in METADATA:
            meta[rec.kind] = rec
        elif pending and rec.kind in ('NATIVE3D_STAGE', 'NATIVE3D_LENGTH', 'NATIVE3D_QUALITY_LOCATION', ENGINE_PROBE, 'gap'):
            pending.append(rec)
            if rec.kind == 'NATIVE3D_STAGE' and rec.fields.get('stage') == 'exit':
                packets.append(pending)
                pending = []
        elif rec.kind in ('NATIVE3D_LENGTH', ENGINE_PROBE) or (rec.kind == 'NATIVE3D_STAGE' and rec.fields.get('stage') in STAGES):
            issue = event.issue('engine row outside a seed packet', rec, row=rec.kind)
            (probe_issues if rec.kind == ENGINE_PROBE else measurements).append(issue)
    if pending:
        packets.append(pending)
    if worker_count is None or len(packets) != worker_count:
        measurements.append(event.issue('seed packet count', expected=worker_count, actual=len(packets)))
    for index, packet in enumerate(packets):
        final_detail = detail if worker_count is not None and index == worker_count - 1 else None
        stage, problems, wrong = inspect_packet(event, index, packet, core[index], final_detail, suppress)
        stage_rows.append(stage)
        measurements.extend(problems)
        failures.extend(wrong)
        observed = [rec for rec in (*core[index], *packet) if rec.kind in (CORE_PROBE, ENGINE_PROBE)]
        if len(observed) != bool(probe):
            issue = event.issue('probe row count', packet[0], index, expected=int(bool(probe)), actual=len(observed))
            (probe_issues if probe else measurements).append(issue)
        for rec in observed:
            if probe:
                row, error = inspect_probe(event, rec, index, probe, stage)
                probe_rows.append(row)
                if error:
                    probe_issues.append(event.issue(error, rec, index, row=rec.kind))
    for index in set(core) - set(range(len(packets))):
        for rec in core[index]:
            if rec.kind == CORE_PROBE:
                probe_issues.append(event.issue('probe without corresponding seed packet', rec, index))
    return stage_rows, meta, measurements, failures, probe_rows, probe_issues


def extrema(values, pick=max):
    return pick(values) if values and all(value is not None for value in values) else None


def table_order(row):
    return row['event'], row['stage'], row['rank']


def summarize_event(event, count, rows, meta, native, receipt):
    summary = {
        'event': event, 'worker_stage_count': count,
        'native_seconds': extrema([rec.seconds('total') for rec in native]),
        'final_cells': extrema([rec.optional('final') for rec in native]),
        'final_points': None, 'final_surface_faces': extrema([rec.optional('final_proof_surface_faces') for rec in native]),
        'job_peak_rss_bytes': extrema([rec.optional('job_peak_rss') for rec in native]),
        'minimum_timer_coverage': None, 'worker_stages': [],
    }
    if receipt:
        summary.update(native_seconds=receipt.get('native_seconds'),
                       final_cells=receipt.get('actual_mesh_counts', {}).get('cells'),
                       final_points=receipt.get('actual_mesh_counts', {}).get('points'),
                       final_surface_faces=receipt.get('proof_faces'), job_peak_rss_bytes=receipt.get('job_peak_rss_bytes'),
                       minimum_timer_coverage=receipt.get('minimum_timer_coverage'))
    elif len(meta) == 6:
        coverage = []
        for rank in meta:
            timer = rank.get('NATIVE3D_TIMER_SUMMARY')
            root = timer.seconds('root_seconds') if timer else None
            other = timer.seconds('unattributed') if timer else None
            coverage.append(1 - other / root if root and other is not None else None)
        summary['minimum_timer_coverage'] = extrema(coverage, min)
    for index in range(count or 0):
        stage = [row for row in rows if row['stage'] == index]
        initial = [row['construction_exits'][0]['length'] if row['construction_exits'] else None for row in stage]
        last = [row['split_generations'][-1] if row['split_generations'] else start
                for row, start in zip(stage, initial, strict=True)]
        def sum_field(collection, key):
            values = [row.get(key) if row else None for row in collection]
            return sum(values) if len(values) == 6 and all(value is not None for value in values) else None

        summary['worker_stages'].append({
            'stage': index, 'ranks': len(stage),
            'handover_to_first_collapse_exit_seconds_max': extrema(
                [row['handover_to_first_collapse_exit_seconds'] for row in stage]),
            'collapse_length_elapsed_seconds_max': extrema([row['collapse_length_elapsed_seconds'] for row in stage]),
            'collapse_pass_seconds_max': extrema([row['collapse_pass_seconds'] for row in stage]),
            'core_last_pass_bad_out_sum': sum_field(stage, 'core_last_pass_bad_out'),
            'quality_entry_bad_union_sum': sum_field(stage, 'quality_entry_bad_union'),
            'quality_seconds_max': extrema([row['quality_seconds'] for row in stage]),
            'core_last_pass_short_edges_sum': sum_field(stage, 'core_last_pass_short_out'),
            'short_edges_at_initial_collapse_exit_sum': sum_field(initial, 'short_local_edges'),
            'short_edges_at_last_length_boundary_sum': sum_field(last, 'short_local_edges'),
            'split_generation_count_by_rank': {row['rank']: len(row['split_generations']) for row in stage},
        })
    return summary


def collect(case, probe=0, *, partial=False, baseline=False, event_receipts=None):
    """The full-workflow adapter supplies its validated event receipts when available."""
    case = Path(case)
    paths, measurements = defaultdict(list), []
    for path in case.glob('rank_output/*/rank.*/stdout'):
        label = path.parent.name.removeprefix('rank.')
        if label.isdecimal():
            paths[int(label)].append(path)
        else:
            measurements.append({'field': 'malformed rank stdout path', 'file': str(path)})
    raw, logs, results, probe_issues = {}, {}, [], []
    for rank in sorted(set(paths) | set(range(6))):
        if rank not in range(6) or len(paths[rank]) != 1:
            measurements.append({'field': 'rank stdout files', 'rank': rank, 'expected': 1 if rank in range(6) else 0,
                                 'actual': [str(path) for path in paths[rank]]})
            continue
        path, = paths[rank]
        try:
            found, problems, probe_problems, digest = scan(path, rank)
        except OSError as error:
            measurements.append({'field': 'rank stdout read', 'file': str(path), 'rank': rank, 'error': str(error)})
            continue
        raw[rank] = found
        measurements.extend(problems)
        probe_issues.extend(probe_problems)
        logs[str(path)] = digest
        if len(found) != 2:
            measurements.append({'field': 'partition anchors', 'file': str(path), 'rank': rank, 'expected': 2,
                                 'actual': len(found)})
        for event in found:
            parts = event.records[0].optional('parts')
            if parts != 6:
                measurements.append(event.issue('partition participants', event.records[0], expected=6, actual=parts))

    receipts = {row['event']: row for row in event_receipts or []}
    stages, probes, events = [], [], []
    for event in (1, 2):
        present = [values[event - 1] for values in raw.values() if len(values) >= event]
        ranks = {value.rank: value for value in present}
        frozen = [rec for rec in ranks[0].records if rec.kind == 'Native3D frozen stages:'] if 0 in ranks else []
        gathered = ranks[0].records if 0 in ranks else []
        details = {rec.optional('rank'): rec for rec in gathered if rec.kind == 'NATIVE3D_DETAIL'}
        native = [rec for rec in gathered if rec.kind == 'NATIVE3D']
        count = frozen[0].optional('count') if len(frozen) == 1 else None
        if not count:
            measurements.append({'field': 'frozen stage count receipt', 'event': event, 'rank': 0,
                                 'expected': 'one positive count', 'actual': [rec.fields for rec in frozen]})
        accumulated, meta = [], []
        for rank_event in present:
            rows, event_meta, problems, wrong, observations, obs_problems = inspect_event(
                rank_event, count, not baseline and probe != 2, probe, details.get(rank_event.rank))
            accumulated.extend(rows)
            meta.append(event_meta)
            measurements.extend(problems)
            results.extend(wrong)
            probes.extend(observations)
            probe_issues.extend(obs_problems)
        stages.extend(accumulated)
        events.append(summarize_event(event, count, accumulated, meta, native, receipts.get(event)))

    observations = defaultdict(list)
    for row in probes:
        observations[row['event'], row['stage']].append(row)
    totals = []
    if probe:
        for event in events:
            for index in range(event['worker_stage_count'] or 0):
                key = event['event'], index
                rows = observations[key]
                missing = set(range(6)) - {row['rank'] for row in rows}
                for rank in sorted(missing):
                    placeholder = {'event': key[0], 'stage': key[1], 'rank': rank, 'line': None, 'status': 'incomplete',
                                   'kind': CORE_PROBE if probe == 1 else ENGINE_PROBE}
                    probes.append(placeholder)
                complete = len(rows) == 6 and {row['rank'] for row in rows} == set(range(6)) and all(
                    row['status'] == 'complete' for row in rows)
                totals.append({'event': key[0], 'stage': key[1], 'status': 'complete' if complete else 'incomplete',
                               'ranks_reported': len(rows), 'scope': 'sum of rank-local edge incidences' if probe == 1
                               else 'sum over proposing ranks',
                               'counts': {field: sum(row[field] for row in rows)
                                          for field in (CORE_COUNTS if probe == 1 else ENGINE_COUNTS)} if complete else None})
    problems = results or measurements
    return {
        'status': 'FAILED' if results or (measurements and not partial) else 'incomplete' if measurements else 'passed',
        'failure_kind': 'result' if results else 'measurement' if measurements else None,
        'first_difference': problems[0] if problems else None, 'measurement_issues': measurements,
        'result_issues': results, 'events': events, 'worker_stages': sorted(stages, key=table_order), 'log_sha256': logs,
        'probe': probe, 'probe_rows': sorted(probes, key=table_order), 'probe_totals': totals, 'probe_issues': probe_issues,
        'probe_status': ('disabled' if not probe else 'incomplete'
                         if probe_issues or measurements or not totals or any(row['status'] != 'complete' for row in totals)
                         else 'complete'),
    }


def time_comparison(current_events, prior_event_sets):
    """Use the faster full-precision P3 time in each event. OPEN flags are reports."""
    rows = []
    for event in current_events:
        number = event['event']
        faster = min(row['native_seconds'] for group in prior_event_sets for row in group if row['event'] == number)
        upper = 0.9 * faster
        actual = event['native_seconds']
        rows.append({'event': number, 'p3_faster_seconds': faster, 'ten_percent_below_seconds': upper,
                     'native_seconds': actual, 'flag': 'time OPEN for the user' if actual > upper else 'threshold met'})
    return rows


def self_check():
    with TemporaryDirectory(prefix='fp3-collector-') as scratch:
        case = Path(scratch)
        def fixture(rank):
            def stage(name, rounds=0, collapses=0, lane=0):
                return (f'NATIVE3D_STAGE rank={rank} stage={name} rounds={rounds} length_collapses={collapses} '
                        f'split_lane={lane} elapsed=3')
            def length(collapses=0):
                return f'NATIVE3D_LENGTH rank={rank} pass=collapse collapse_accepted={collapses} elapsed=2'
            seed = f'NATIVE3D_SEED_SHORT rank={rank} cells=10'
            event1 = [seed, length(), stage('length_collapse_exit'), stage('quality_entry'),
                      stage('metric_cleanup_entry'), stage('metric_cleanup_exit', 2, 1),
                      stage('quality_entry', 2, 1), stage('quality_exit', 3, 1), stage('construction_entry', 3, 1),
                      length(1), stage('length_collapse_exit', 3, 1), stage('quality_entry', 3, 1),
                      stage('quality_exit', 4, 1), stage('construction_entry', 4, 1, 1),
                      stage('length_split_exit', 5, 1), stage('exit', 6, 1)]
            event1 += [seed, length(), stage('length_collapse_exit'), stage('exit')]
            partition = f'NATIVE3D_PARTITION rank={rank} parts=6'
            count1, count2 = ('Native3D frozen stages: count=2', 'Native3D frozen stages: count=1') if rank == 0 else ('', '')
            detail = f'NATIVE3D_DETAIL stage=refine rank={rank} initialization=0.5 seed=0.5'
            return '\n'.join([partition, *event1, detail, count1, partition, seed, length(),
                              stage('length_collapse_exit'), stage('exit'), count2, ''])

        for rank in range(6):
            path = case / f'rank_output/1/rank.{rank}/stdout'
            path.parent.mkdir(parents=True)
            path.write_text(fixture(rank))
        path = case / 'rank_output/1/rank.0/stdout'
        normal = fixture(0)
        accepted = collect(case)
        assert accepted['status'] == 'passed' and len(accepted['worker_stages']) == 18, accepted['first_difference']
        assert accepted['worker_stages'][0]['collapse_pass_time_status'] == 'pass time not available'
        assert accepted['worker_stages'][6]['collapse_pass_seconds'] == 1
        # Wrong rounds, an entirely skipped boundary, and a cut pair are different outcomes.
        path.write_text(normal.replace('stage=length_collapse_exit rounds=0',
                                       'stage=length_collapse_exit rounds=1', 1))
        wrong = collect(case)
        assert wrong['failure_kind'] == 'result' and wrong['first_difference']['field'] == 'initial rounds'
        lines = normal.splitlines(keepends=True)
        path.write_text(''.join(lines[:2] + lines[4:]))
        assert collect(case)['failure_kind'] == 'result'
        path.write_text(''.join(lines[:3] + lines[4:]))
        assert collect(case)['failure_kind'] == 'measurement'
        path.write_text(''.join(lines[:3]))
        assert collect(case, partial=True)['status'] == 'incomplete'
        # A cut probe only affects the observation.
        path.write_text(normal.replace(lines[2], 'NATIVE3D_FP3_ENGINE_COLLAPSE rank=0 accepted=0\n' + lines[2], 1))
        obs = collect(case, 2)
        assert obs['status'] == 'passed' and obs['probe_status'] == 'incomplete'
        assert time_comparison([{'event': 1, 'native_seconds': 9}], [[{'event': 1, 'native_seconds': 10}]])[0][
            'flag'] == 'threshold met'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path, nargs='?')
    parser.add_argument('--probe', type=int, choices=(0, 1, 2), default=0)
    parser.add_argument('--partial', action='store_true')
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--event-receipt', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return 0
    if not args.case:
        parser.error('a case path is required')
    events = json.loads(args.event_receipt.read_text())['events'] if args.event_receipt else None
    row = collect(args.case, args.probe, partial=args.partial, baseline=args.baseline, event_receipts=events)
    payload = json.dumps(row, indent=2) + '\n'
    if args.output:
        args.output.write_text(payload)
    else:
        print(payload, end='')
    return 0 if row['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
