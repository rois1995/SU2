"""Collect FP3-E stall, exchange, suppression and timing evidence from the rank logs."""

import argparse
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import datetime, timezone
from decimal import Decimal, InvalidOperation
import hashlib
import json
import math
from pathlib import Path
import re
from tempfile import TemporaryDirectory

from collect_stall import HANDOFF, STALL, STALL_KINDS, inspect_stall

PRIVATE = 'NATIVE3D_PRIVATE_WORK'
QUALITY = 'NATIVE3D_QUALITY'


STAMP = r'[A-Z][a-z]{2} [A-Z][a-z]{2} [ \d]\d \d{2}:\d{2}:\d{2} \d{4}'
PREFIX = re.compile(fr'(?P<stamp>{STAMP})?\s*(?:\[\d+,\d+\])?<(?:stdout|stderr)>:')
KINDS = re.compile(
    r'\bNATIVE3D(?:_(?:PARTITION|SEED_SHORT|LENGTH|STAGE|DETAIL|LOCAL_CORE|LOCAL_SURFACE_STATE|'
    r'QUALITY_STALL(?:_HANDOFF)?|QUALITY(?:_LOCATION)?|PRIVATE_WORK|TIMER(?:_SUMMARY)?|OWNERSHIP_MEMORY|PHASE_OWNERSHIP|FP3_ENGINE_COLLAPSE|FP3_HANDOVER_SURFACE))?(?=\s|$)'
    r'|Native3D frozen stages:|\bD9_QUERY(?=\s|$)'
)
FIELDS = re.compile(r'(?:^|\s)(\w+)=([^\s]*)')
STAGES = {
    'length_collapse_exit', 'length_split_exit', 'split_surface_exit', 'split_interior_entry',
    'construction_entry', 'quality_entry', 'quality_exit', 'exit', 'metric_cleanup_entry', 'metric_cleanup_exit',
    'window', 'ownership_staged', 'ownership_committed', 'split_interior_exit', 'split_surface_resume',
}
OWNERSHIP = 'NATIVE3D_PHASE_OWNERSHIP'
OWNERSHIP_MEMORY = 'NATIVE3D_OWNERSHIP_MEMORY'
TIMER = 'NATIVE3D_TIMER'
SNAPSHOT = 'NATIVE3D_TIMER_SUMMARY'
PROBE_KINDS = ('NATIVE3D_FP3_ENGINE_COLLAPSE', 'NATIVE3D_FP3_HANDOVER_SURFACE')
RAW_INPUTS = ('predicted_imbalance', 'predicted_gain', 'predicted_cost')
OWNERSHIP_TIMES = ('total_seconds', 'partition_seconds', 'directory_build_seconds')
OWNERSHIP_MEMORY_FIELDS = ('current_rss', 'process_peak_rss', 'max_exchange_work_bytes', 'bytes_sent',
                           'pending_bytes', 'outgoing_bytes', 'record_bytes')
EXCHANGE_AGREEMENT = ('quality', 'site', 'evaluate', 'committed', 'policy', 'migrate', 'global_cells',
                      'max_owned_cells', 'moved_cells')
SPLIT_STAGES = {'split_surface_exit', 'split_interior_entry', 'split_interior_exit', 'split_surface_resume',
                'length_split_exit'}
ENTRY_STAGES = {'construction_entry', 'quality_entry', 'surface_repair_entry', 'surface_required_entry',
                'size_repair_entry', 'geometry_repair_entry', 'metric_cleanup_entry', 'exit'}
OWNERSHIP_BOUNDARIES = SPLIT_STAGES | ENTRY_STAGES
PHASES = ('construction', 'quality', 'final_proof', 'preparation')
STAMP_FORMAT = '%a %b %d %H:%M:%S %Y'
PROOF = re.compile(r'\bNATIVE3D_FINAL_PROOF(?=\s|$)')
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


def scan(path, rank, check_ownership):
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
                fields = FIELDS.findall(fragment)
                rec = Record(match.group(), dict(fields), number, stamp, not raw.endswith(b'\n'))
                if rec.kind in (OWNERSHIP, *STALL_KINDS) and (len(fields) != len(rec.fields)
                        or any('=' not in word for word in fragment.split())):
                    rec.cut = True
                fields = rec.fields
                if rec.kind == 'Native3D frozen stages:':
                    if rank == 0 and events:
                        events[-1].records.append(rec)
                    continue
                target_rank = rec.optional('rank')
                if rec.kind in STALL_KINDS:
                    if not events:
                        issues.append(RankEvent(rank, 0, path).issue('stall row before partition', rec))
                    else:
                        if 'rank' in fields and target_rank != rank:
                            rec.cut = True
                        events[-1].records.append(rec)
                    continue
                if rank == 0 and events and target_rank in range(6) and (
                        rec.kind == 'NATIVE3D' or
                        rec.kind == 'NATIVE3D_DETAIL' and fields.get('stage') == 'refine'):
                    # The house runner serializes the parent report on rank 0.
                    events[-1].records.append(rec)
                    continue
                if not events and rec.kind not in ('NATIVE3D_PARTITION', 'NATIVE3D_SEED_SHORT', ENGINE_PROBE, CORE_PROBE):
                    continue
                important = (rec.kind in {'NATIVE3D_PARTITION', 'NATIVE3D_SEED_SHORT', 'NATIVE3D_LENGTH',
                                         'NATIVE3D_STAGE', CORE_PROBE, ENGINE_PROBE} or check_ownership and rec.kind == OWNERSHIP)
                if target_rank is None and important:
                    event = events[-1] if events else RankEvent(rank, 0, path)
                    first = event.issue('rank is missing or malformed', rec, row=rec.kind)
                    if rec.kind in (CORE_PROBE, ENGINE_PROBE):
                        probe_issues.append(first)
                    else:
                        issues.append(first)
                        if events:
                            kind = OWNERSHIP if rec.kind == OWNERSHIP else 'gap'
                            event.records.append(Record(kind, {}, number, stamp, True))
                    continue
                if target_rank != rank:
                    if events and check_ownership and rec.kind == OWNERSHIP:
                        issues.append(events[-1].issue('ownership row rank differs from its stream', rec,
                                                       expected=rank, actual=target_rank))
                        events[-1].records.append(Record(OWNERSHIP, {}, number, stamp, True))
                    continue
                if rec.kind == 'NATIVE3D_PARTITION':
                    events.append(RankEvent(rank, len(events) + 1, path, [rec]))
                elif not events:
                    if important:
                        issues.append(RankEvent(rank, 0, path).issue('row precedes first partition', rec, row=rec.kind))
                elif rec.kind == 'NATIVE3D_STAGE':
                    stage = fields.get('stage')
                    if stage in STAGES | OWNERSHIP_BOUNDARIES or not stage:
                        events[-1].records.append(rec)
                elif rec.kind == 'NATIVE3D_DETAIL':
                    if fields.get('stage') == 'refine':
                        events[-1].records.append(rec)
                elif rec.kind == SNAPSHOT:
                    if fields.get('root') == 'remesh':
                        events[-1].records.append(rec)
                elif rec.kind == TIMER:
                    if fields.get('root') == 'remesh' and (
                            fields.get('boundary') == 'final' and fields.get('phase') in PHASES
                            or fields.get('phase') == 'construction' and fields.get('operation') == 'census'):
                        events[-1].records.append(rec)
                elif rec.kind == 'D9_QUERY':
                    if fields.get('phase') == 'final_proof' and fields.get('site') == 'g':
                        events[-1].records.append(rec)
                else:
                    events[-1].records.append(rec)
    return events, issues, probe_issues, digest.hexdigest()


def elapsed_between(start, end):
    return end - start if start is not None and end is not None and end >= start else None


def unix_stamp(stamp):
    try:
        return datetime.strptime(stamp, STAMP_FORMAT).timestamp() if stamp else None
    except ValueError:
        return None


def timestamp_between(start, end):
    return elapsed_between(unix_stamp(start.stamp) if start else None, unix_stamp(end.stamp) if end else None)


def projection(rec, *keys):
    if rec is None:
        return None
    row = {key: rec.optional(key) for key in keys}
    return {**row, 'line': rec.line, 'elapsed_seconds': rec.seconds()}


def observed_attempts(rec):
    # The global attempt count is emitted through a double reduction and can use scientific notation.
    try:
        value = Decimal(rec.fields.get('attempts', ''))
    except InvalidOperation:
        return None
    if value.is_finite() and 0 <= value <= 2**64 and value == value.to_integral_value():
        return int(value)
    return None


def construction_census(packet):
    """Choose the first remesh snapshot after entry and subtract its construction census totals."""
    entry, census = None, []
    for rec in packet:
        if rec.kind == TIMER:
            census.append(rec)
        elif rec.kind == SNAPSHOT:
            boundary = rec.fields.get('boundary')
            selected = [row for row in census if row.fields.get('boundary') == boundary]
            totals = {'boundary': boundary, 'line': rec.line}
            for field in ('calls', 'inclusive_call_seconds', 'exclusive_seconds'):
                values = [row.optional(field) if field == 'calls' else row.seconds(field) for row in selected]
                totals[field] = sum(values) if all(value is not None for value in values) else None
            if entry is not None:
                return {'entry': entry, 'next': totals,
                        **{field: totals[field] - entry[field]
                           if totals[field] is not None and entry[field] is not None else None
                           for field in ('calls', 'inclusive_call_seconds', 'exclusive_seconds')}}
            if boundary == 'construction_entry' and not rec.cut:
                entry = totals
            census.clear()
    return {'entry': entry, 'next': None, 'calls': None, 'inclusive_call_seconds': None,
            'exclusive_seconds': None}


def inspect_ownership(event, index, packet, initial, final, exchange, baseline, workflow_passed):
    rows, evaluations, measurements, results, missing_commits = [], [], [], [], []
    invalid_row = False
    exit_position = next((i for i, row in enumerate(packet) if row is initial), None)
    after_exit = range(exit_position + 1, len(packet)) if exit_position is not None else ()
    first_split = next((i for i in after_exit if packet[i].kind == 'NATIVE3D_STAGE'
                        and packet[i].fields.get('stage') in SPLIT_STAGES), None)
    boundary = first_split if first_split is not None else next(
        (i for i in after_exit if packet[i].kind == 'NATIVE3D_STAGE'
         and packet[i].fields.get('stage') in ENTRY_STAGES), None)
    policy = exchange
    previous_eval, previous_commit, previous_stage, construction = None, None, None, None
    memory = {}
    quality_started = False

    def measure(rec, field, **details):
        if exchange:
            measurements.append(event.issue(field, rec, index, **details))

    def reject(rec, field, expected, actual):
        if exchange:
            results.append(event.issue(field, rec, index, expected=expected, actual=actual))

    def finish_evaluation():
        if previous_eval is not None and previous_eval['migrate'] == 1 and previous_commit is None:
            missing_commits.append(previous_eval['line'])

    for position, rec in enumerate(packet):
        if rec.kind == 'NATIVE3D_STAGE':
            stage = rec.fields.get('stage')
            previous_stage = rec
            if stage == 'metric_cleanup_entry':
                policy = 'raw'
                quality_started = False
            elif stage == 'construction_entry':
                policy = exchange
                construction = rec
                quality_started = False
            elif stage == 'quality_entry':
                quality_started = True
            continue
        if rec.kind == OWNERSHIP_MEMORY:
            memory[rec.fields.get('boundary')] = projection(rec, *OWNERSHIP_MEMORY_FIELDS)
            if rec.fields.get('boundary') == 'committed' and previous_commit is not None:
                previous_commit['memory'] = memory.copy()
                before, after = (memory.get(key, {}).get('bytes_sent') for key in ('before', 'committed'))
                previous_commit['bytes_sent'] = (after - before
                                                 if before is not None and after is not None and after >= before
                                                 else None)
            continue
        if rec.kind != OWNERSHIP:
            continue
        row = {'event': event.event, 'stage': index, 'rank': event.rank, 'line': rec.line,
               'stamp': rec.stamp, 'raw_fields': rec.fields.copy(),
               **{key: rec.optional(key) for key in ('quality', 'evaluate', 'committed', 'migrate', 'moved_cells',
                                                     'global_cells', 'max_owned_cells', 'cells',
                                                     'directory_peak_bytes', 'transition_peak_bytes')},
               'attempts': observed_attempts(rec) if not rec.cut else None,
               'policy': rec.fields.get('policy'), 'owned_ratio': rec.seconds('owned_ratio'),
               **{key: rec.seconds(key) for key in (*RAW_INPUTS, *OWNERSHIP_TIMES)}}
        if row['policy'] is None and isinstance(baseline, str):
            row['policy'] = baseline
        rows.append(row)
        quality, evaluate, committed = (row[key] for key in ('quality', 'evaluate', 'committed'))
        if quality not in (0, 1) or (evaluate, committed) not in ((1, None), (None, 1)):
            invalid_row = True
            measure(rec, 'ownership row discriminator missing, cut or malformed')
            continue
        if committed == 1:
            valid = previous_eval is not None and previous_commit is None
            if previous_eval is not None:
                row.update(site=previous_eval.get('site'), policy=previous_eval['policy'],
                           evaluation_line=previous_eval['line'])
            if row['moved_cells'] is None:
                invalid_row = True
                measure(rec, 'ownership commit is cut or malformed')
            elif not invalid_row and not (
                    valid and previous_eval['quality'] == quality and previous_eval['migrate'] == 1):
                reject(rec, 'ownership commit pairing', 'one preceding positive evaluation of same quality', {
                    'previous_evaluation': previous_eval['line'] if previous_eval else None,
                    'previous_quality': previous_eval['quality'] if previous_eval else None,
                    'previous_migrate': previous_eval['migrate'] if previous_eval else None,
                    'quality': quality, 'already_committed': previous_commit is not None,
                })
            previous_commit = row
            continue

        finish_evaluation()
        previous_eval, previous_commit, memory = row, None, {}
        stage = previous_stage.fields.get('stage') if previous_stage else None
        row['site'] = 'length_collapse_exit' if quality == 0 else 'quality_window' if quality_started else 'quality_entry'
        row['preceding_stage'] = stage
        if construction:
            row['construction_entry_line'] = construction.line
        row['construction_entry_to_evaluation_seconds'] = timestamp_between(construction, rec)
        if row['owned_ratio'] is not None:
            row['owned_ratio_basis'] = 'reported max_owned_cells / global_cells times ranks'
        row['local_cells_before'] = previous_stage.optional('cells') if previous_stage else None
        row['local_cells_before_basis'] = stage
        # The staging row prints the old ownership; it is closer to a positive evaluation than a prior snapshot.
        # For a guarded or negative quality entry, the next quality_entry row reflects the same owned cells.
        for later in packet[position + 1:]:
            if later.kind == OWNERSHIP and later.fields.get('evaluate') == '1':
                break
            if later.kind == 'NATIVE3D_STAGE':
                later_stage = later.fields.get('stage')
                if later_stage == 'ownership_staged' or (
                        row['site'] == 'quality_entry' and row['migrate'] == 0 and later_stage == 'quality_entry'):
                    row['local_cells_before'] = later.optional('cells')
                    row['local_cells_before_basis'] = later_stage
                break
        migrate, attempts, global_cells = (row[key] for key in ('migrate', 'attempts', 'global_cells'))
        if not exchange:
            continue
        actual_policy = row['policy']
        if (migrate not in (0, 1) or attempts is None or not global_cells or global_cells >= 2**64
                or any(row[key] is None for key in RAW_INPUTS) or actual_policy not in ('cells', 'raw', 'fp3b')
                or actual_policy == 'cells' and (row['max_owned_cells'] is None or row['owned_ratio'] is None)):
            invalid_row = True
            measure(rec, 'ownership evaluation missing, cut or malformed', actual=row['raw_fields'])
            continue
        if quality == 0:
            evaluations.append(rec)
            if exit_position is not None and (
                    position <= exit_position or (boundary is not None and position >= boundary)):
                reject(rec, 'construction ownership evaluation order',
                       'after initial collapse exit and before next split-stage or fallback boundary', rec.line)
            if attempts != 0:
                reject(rec, 'construction ownership attempts', 0, attempts)
        if actual_policy != policy:
            reject(rec, 'ownership policy', policy, actual_policy)
            continue
        raw_pays = row['predicted_imbalance'] > 1.25 and row['predicted_gain'] > 2 * float(global_cells)
        if policy == 'cells':
            max_owned = row['max_owned_cells']
            ranks = event.records[0].optional('parts')
            if ranks != 6:
                measure(rec, 'ownership evaluation rank count missing or malformed', actual=ranks)
                continue
            expected = ranks * max_owned > 2 * global_cells
        else:
            expected = raw_pays and (policy == 'raw' or quality == 1)
        if migrate != int(expected):
            reject(rec, 'ownership migrate', int(expected), migrate)

    finish_evaluation()
    if exchange:
        if len(evaluations) > 1:
            reject(evaluations[1], 'construction ownership evaluation count', 1, len(evaluations))
        elif final is not None and not evaluations and not invalid_row:
            measure(initial or final, 'construction ownership evaluation missing', expected=1, actual=0)
    return rows, measurements, results, missing_commits if exchange and workflow_passed and final and not invalid_row else []


def inspect_packet(event, index, records, core, detail, suppress, exchange, baseline, workflow_passed):
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
        if rec.kind in (ENGINE_PROBE, OWNERSHIP, OWNERSHIP_MEMORY, TIMER, SNAPSHOT, PRIVATE, QUALITY, *STALL_KINDS):
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
                                  'direction': 'geometry_split' if lane == 1 else 'collapse',
                                  'transition_seconds': elapsed_between(quality_exit.seconds(), rec.seconds())})
                if lane == 1:
                    split_clock_at = rec.seconds()
        elif stage == 'quality_entry':
            if reentries and 'construction_seconds' not in reentries[-1]:
                reentries[-1]['construction_seconds'] = elapsed_between(
                    reentries[-1]['construction_entry']['elapsed_seconds'], rec.seconds())
                reentries[-1]['construction_end'] = 'quality_entry'
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
    if reentries and final and 'construction_seconds' not in reentries[-1]:
        reentries[-1]['construction_seconds'] = elapsed_between(
            reentries[-1]['construction_entry']['elapsed_seconds'], final.seconds())
        reentries[-1]['construction_end'] = 'exit'
    if cleanup_entry is not None:
        measure(cleanup_entry, 'metric cleanup entry without exit')
    if length is not None:
        measure(length, 'collapse LENGTH without exit stage')
    if final is None:
        measure(seed, 'worker stage has no complete engine exit')
    if suppress and initial is None and not results:
        measure(seed, 'worker stage has no initial collapse pair')

    ownership, ownership_measurements, ownership_results, missing_commits = inspect_ownership(
        event, index, records, initial, final, exchange, baseline, workflow_passed)
    measurements.extend(ownership_measurements)
    results.extend(ownership_results)
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
        'construction_exits': collapse_rows, 'construction_reentries': reentries, 'ownership_rows': ownership,
        'metric_cleanups': cleanups, 'split_generations': splits, 'quality_spans': quality_spans,
        'quality_entry': projection(next((rec for rec in body if rec.kind == 'NATIVE3D_STAGE'
                                          and rec.fields.get('stage') == 'quality_entry'), None), 'cells'),
        'construction_census_delta': construction_census(records),
        'exchange_evidence_incomplete': bool(ownership_measurements),
        'missing_exchange_commits': missing_commits,
        'file': str(event.path),
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


def inspect_event(event, worker_count, suppress, probe, detail, exchange, baseline, workflow_passed):
    packets, stage_rows, measurements, failures, probe_rows, probe_issues = [], [], [], [], [], []
    core, pending = defaultdict(list), []
    meta, phase_seconds, proof_union = {}, defaultdict(list), []
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
        elif rec.kind == SNAPSHOT:
            if rec.fields.get('boundary') == 'final':
                meta[rec.kind] = rec
            if pending:
                pending.append(rec)
        elif rec.kind in METADATA:
            meta[rec.kind] = rec
        elif rec.kind == TIMER:
            if rec.fields.get('boundary') == 'final':
                phase_seconds[rec.fields['phase']].append(rec.seconds('exclusive_seconds'))
            if pending and rec.fields.get('phase') == 'construction' and rec.fields.get('operation') == 'census':
                pending.append(rec)
        elif rec.kind == 'D9_QUERY':
            proof_union.append(rec)
        elif pending and rec.kind in ('NATIVE3D_STAGE', 'NATIVE3D_LENGTH', 'NATIVE3D_QUALITY_LOCATION',
                                      OWNERSHIP, OWNERSHIP_MEMORY, ENGINE_PROBE, PRIVATE, QUALITY, *STALL_KINDS, 'gap'):
            pending.append(rec)
            if rec.kind == 'NATIVE3D_STAGE' and rec.fields.get('stage') == 'exit':
                packets.append(pending)
                pending = []
        elif (rec.kind in ('NATIVE3D_LENGTH', ENGINE_PROBE, *STALL_KINDS) or exchange and rec.kind == OWNERSHIP
              or rec.kind == 'NATIVE3D_STAGE' and rec.fields.get('stage') in STAGES):
            issue = event.issue('engine row outside a seed packet', rec, row=rec.kind)
            (probe_issues if rec.kind == ENGINE_PROBE else measurements).append(issue)
    if pending:
        packets.append(pending)
    if worker_count is None or len(packets) != worker_count:
        measurements.append(event.issue('seed packet count', expected=worker_count, actual=len(packets)))
    for index, packet in enumerate(packets):
        final_detail = detail if worker_count is not None and index == worker_count - 1 else None
        stage, problems, wrong = inspect_packet(event, index, packet, core[index], final_detail, suppress,
                                                exchange, baseline, workflow_passed)
        stage_rows.append(stage)
        measurements.extend(problems)
        failures.extend(wrong)
        observed = [rec for rec in (*core[index], *packet) if rec.kind in (CORE_PROBE, ENGINE_PROBE)]
        if not baseline:
            expected = int(probe in (1, 2))
            if len(observed) != expected:
                issue = event.issue('probe row count', packet[0], index, expected=expected, actual=len(observed))
                (probe_issues if probe else measurements).append(issue)
            for rec in observed:
                if probe in (1, 2):
                    row, error = inspect_probe(event, rec, index, probe, stage)
                    probe_rows.append(row)
                    if error:
                        probe_issues.append(event.issue(error, rec, index, row=rec.kind))
                elif probe in (3, 4):
                    probe_rows.append({'event': event.event, 'stage': index, 'rank': event.rank, 'line': rec.line,
                                       'status': 'unexpected', 'kind': rec.kind})
    for index in set(core) - set(range(len(packets))):
        for rec in core[index]:
            if rec.kind == CORE_PROBE:
                probe_issues.append(event.issue('probe without corresponding seed packet', rec, index))
    meta['phase_seconds'] = {key: sum(values) if all(value is not None for value in values) else None
                             for key, values in phase_seconds.items()}
    if len(proof_union) == 1:
        rec, = proof_union
        meta['proof_union'] = {'rank': event.rank, 'union_log': str(event.path), 'union_line': rec.line,
                               'union_work': rec.optional('upper_work'), 'union_calls': rec.optional('upper_calls'),
                               'union_memo_hits': rec.optional('memo_union_hits')}
    return stage_rows, meta, measurements, failures, probe_rows, probe_issues, packets


def extrema(values, pick=max):
    return pick(values) if values and all(value is not None for value in values) else None


def table_order(row):
    return row['event'], row['stage'], row['rank']


def summarize_exchanges(rows, exchange):
    """Count each agreed collective migration once and keep rank-local observations."""
    groups = defaultdict(list)
    summaries, measurements, results = [], [], []
    for row in rows:
        groups[row['stage']].append(row)
    for index, ranks in sorted(groups.items()):
        timelines = [row['ownership_rows'] for row in ranks]
        if len(ranks) != 6:
            continue  # Packet-count evidence is diagnosed by the enclosing collector.
        unequal = len({len(timeline) for timeline in timelines}) != 1
        incomplete = any(row['exchange_evidence_incomplete'] for row in ranks)
        if exchange and unequal and not incomplete:
            measurements.append({'event': ranks[0]['event'], 'worker_stage': index,
                                 'field': 'ownership sequence row count by rank',
                                 'actual': {row['rank']: len(row['ownership_rows']) for row in ranks}})
        for ordinal, peers in enumerate(zip(*timelines)):
            reference = peers[0]
            if exchange and not unequal and not incomplete:
                for other in peers[1:]:
                    mismatch = next((key for key in EXCHANGE_AGREEMENT if other[key] != reference[key]), None)
                    if mismatch:
                        results.append({'event': other['event'], 'worker_stage': index, 'rank': other['rank'],
                                        'line': other['line'], 'field': f'ownership collective {mismatch}',
                                        'expected': reference[mismatch], 'actual': other[mismatch]})
                        break
            if len({(row['evaluate'], row['committed'], row['quality'], row.get('site')) for row in peers}) != 1:
                continue
            if reference['evaluate'] == 1 and not reference['owned_ratio']:
                cells = [row.get('local_cells_before') for row in peers]
                total = reference['global_cells']
                if total and all(value is not None for value in cells) and sum(cells) == total:
                    ratio = len(ranks) * max(cells) / total
                    for row in peers:
                        row.update(owned_ratio=ratio, owned_ratio_basis='rank-local observations sum to global_cells')
            if reference['committed'] == 1 and reference['moved_cells'] is not None:
                seconds = [row['total_seconds'] for row in peers]
                sent = [row.get('bytes_sent') for row in peers]
                summaries.append({'stage': index, 'ordinal': ordinal, 'quality': reference['quality'],
                                  'site': reference.get('site'), 'policy': reference['policy'],
                                  'moved_cells': reference['moved_cells'],
                                  'seconds_max': extrema(seconds),
                                  'bytes_sent_sum': sum(sent) if all(value is not None for value in sent) else None})
        if exchange and not unequal and not incomplete:
            missing = [row['missing_exchange_commits'] for row in ranks]
            if all(missing):
                results.append({'event': ranks[0]['event'], 'worker_stage': index, 'rank': ranks[0]['rank'],
                                'line': missing[0][0], 'field': 'matching commit after positive ownership evaluation',
                                'expected': 1, 'actual': 0})
            elif any(missing):
                measurements.append({'event': ranks[0]['event'], 'worker_stage': index,
                                     'field': 'incomplete commit evidence after evaluation',
                                     'actual': {row['rank']: lines for row, lines in zip(ranks, missing)}})
    return summaries, measurements, results


def summarize_event(event, count, rows, meta, native, receipt):
    summary = {
        'event': event, 'worker_stage_count': count,
        'native_seconds': extrema([rec.seconds('total') for rec in native]),
        'final_cells': extrema([rec.optional('final') for rec in native]),
        'final_points': None, 'final_surface_faces': extrema([rec.optional('final_proof_surface_faces') for rec in native]),
        'job_peak_rss_bytes': extrema([rec.optional('job_peak_rss') for rec in native]),
        'minimum_timer_coverage': None, 'worker_stages': [],
        'ownership_rows': [entry for row in rows for entry in row['ownership_rows']],
        'final_proof_union_by_rank': [row['proof_union'] for row in meta if 'proof_union' in row],
        'construction_reentry_counts_by_rank': {rank: sum(len(row['construction_reentries']) for row in rows
                                                           if row['rank'] == rank)
                                                for rank in sorted({row['rank'] for row in rows})},
        'phase_seconds_max': {phase: extrema([rank['phase_seconds'].get(phase) for rank in meta])
                              if len(meta) == 6 else None for phase in PHASES},
    }
    if receipt:
        summary.update(native_seconds=receipt.get('native_seconds'),
                       final_cells=receipt.get('actual_mesh_counts', {}).get('cells'),
                       final_points=receipt.get('actual_mesh_counts', {}).get('points'),
                       final_surface_faces=receipt.get('proof_faces'), job_peak_rss_bytes=receipt.get('job_peak_rss_bytes'),
                       minimum_timer_coverage=receipt.get('minimum_timer_coverage'))
        for phase, row in receipt.get('phase_exclusive_clocks', {}).items():
            if phase in PHASES:
                summary['phase_seconds_max'][phase] = row.get('seconds_max')
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


def enrich_observations(case, paths, intervals, events):
    resources = case / 'host_resources.jsonl'
    samples, digests = [], {}
    if resources.is_file():
        digest = hashlib.sha256()
        with resources.open('rb') as source:
            for raw in source:
                digest.update(raw)
                try:
                    host = json.loads(raw)['host']
                    sampled = host['unix']
                    load = host['loadavg']
                    if math.isfinite(sampled) and isinstance(load, str):
                        samples.append((sampled, load))
                except (ValueError, TypeError, KeyError):
                    continue  # An unfinished observation can end on a partial JSON line.
        digests[str(resources)] = digest.hexdigest()

    def utc(unix):
        return datetime.fromtimestamp(unix, timezone.utc).isoformat() if unix is not None else None

    def nearest(unix):
        if unix is None or not samples:
            return None
        sampled, load = min(samples, key=lambda row: abs(row[0] - unix))
        return {'sample_utc': utc(sampled), 'sample_offset_seconds': sampled - unix, 'loadavg': load}

    proof_by_event = defaultdict(list)
    for rank, stdout in sorted(paths.items()):
        if rank not in range(6) or len(stdout) != 1:
            continue
        stderr = stdout[0].with_name('stderr')
        if not stderr.is_file():
            continue
        digest = hashlib.sha256()
        with stderr.open('rb') as source:
            for number, raw in enumerate(source, 1):
                digest.update(raw)
                text = raw.decode('utf-8', errors='replace')
                prefix = PREFIX.search(text)
                line = PREFIX.sub('', text)
                stamp = prefix.group('stamp') if prefix else None
                unix = unix_stamp(stamp)
                for proof in PROOF.finditer(line):
                    row = Record(proof.group(), dict(FIELDS.findall(line[proof.end():])),
                                 number, stamp, not raw.endswith(b'\n'))
                    if row.optional('rank') != rank or unix is None:
                        continue
                    matches = [event for event, (_, end, partition) in enumerate(intervals, 1)
                               if partition is not None and partition <= unix and (end is None or unix <= end)]
                    if len(matches) == 1:
                        proof_by_event[matches[0]].append({
                            'rank': rank, 'file': str(stderr), 'line': number, 'stamp_utc': utc(unix),
                            'reverse_seconds': row.seconds('reverse_seconds'), 'share_seconds': row.seconds('share_seconds'),
                        })
        digests[str(stderr)] = digest.hexdigest()

    for event, (start, end, partition) in zip(events, intervals, strict=True):
        proof = proof_by_event[event['event']]
        complete = (len(proof) == 6 and {row['rank'] for row in proof} == set(range(6))
                    and all(row[key] is not None for row in proof for key in ('reverse_seconds', 'share_seconds')))
        union = event['final_proof_union_by_rank']
        union_by_rank = {row['rank']: row for row in union}
        complete_union = len(union) == 6 and set(union_by_rank) == set(range(6)) and all(
            row[key] is not None for row in union for key in ('union_work', 'union_calls', 'union_memo_hits'))
        union_work, union_calls, union_hits = (sum(row[field] for row in union) if complete_union else None
                                             for field in ('union_work', 'union_calls', 'union_memo_hits'))
        for row in proof:
            row.update(union_by_rank.get(row['rank'], {}))
        event.update(native_start_utc=utc(start), native_end_utc=utc(end), partition_utc=utc(partition),
                     native_start_basis='remesh report timestamp minus native maximum',
                     host_load_start=nearest(start), host_load_end=nearest(end),
                     final_proof_by_rank=proof, final_proof_complete=complete,
                     final_proof_union_work=union_work, final_proof_union_calls=union_calls,
                     final_proof_union_memo_hits=union_hits,
                     final_proof_union_memo_hit_fraction=union_hits / union_calls if union_calls else None,
                     final_proof_reverse_seconds_max=extrema([row['reverse_seconds'] for row in proof]) if complete else None,
                     final_proof_share_seconds_max=extrema([row['share_seconds'] for row in proof]) if complete else None)
    return digests


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
    exchange = {0: 'cells', 3: 'fp3b', 4: 'cells'}.get(probe) if not baseline else None
    suppress = not baseline and probe != 2
    check_ownership = exchange is not None
    for rank in sorted(set(paths) | set(range(6))):
        if rank not in range(6) or len(paths[rank]) != 1:
            measurements.append({'field': 'rank stdout files', 'rank': rank, 'expected': 1 if rank in range(6) else 0,
                                 'actual': [str(path) for path in paths[rank]]})
            continue
        path, = paths[rank]
        try:
            found, problems, probe_problems, digest = scan(path, rank, check_ownership)
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
    stages, probes, events, intervals = [], [], [], []
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
        accumulated, meta, packets = [], [], {}
        for rank_event in present:
            rows, event_meta, problems, wrong, observations, obs_problems, rank_packets = inspect_event(
                rank_event, count, suppress, probe, details.get(rank_event.rank), exchange, baseline,
                not partial and event in receipts)
            packets[rank_event.rank] = rank_packets
            accumulated.extend(rows)
            meta.append(event_meta)
            measurements.extend(problems)
            results.extend(wrong)
            probes.extend(observations)
            probe_issues.extend(obs_problems)
        stages.extend(accumulated)
        summary = summarize_event(event, count, accumulated, meta, native, receipts.get(event))
        exchanges, issues, wrong = summarize_exchanges(accumulated, exchange)
        measurements.extend(issues)
        results.extend(wrong)
        stall, issues, wrong = inspect_stall(ranks, packets, enabled=probe == 0 and not baseline,
                                             check=not baseline, event_passed=not partial and (
                                                 event in receipts or event_receipts is None))
        measurements.extend(issues)
        results.extend(wrong)
        summary['stall'] = stall
        summary['exchanges'] = exchanges
        summary['moved_cells'] = sum(row['moved_cells'] for row in exchanges)
        end = extrema([unix_stamp(rec.stamp) for rec in native])
        duration = summary['native_seconds']
        start = end - duration if end is not None and duration is not None else None
        partition = unix_stamp(ranks[0].records[0].stamp) if 0 in ranks else None
        intervals.append((start, end, partition))
        events.append(summary)

    observation_hashes = enrich_observations(case, paths, intervals, events)

    observations = defaultdict(list)
    for row in probes:
        observations[row['event'], row['stage']].append(row)
    totals = []
    if probe in (1, 2):
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
    if probe in (0, 3, 4) and not baseline:
        measurements.extend(probe_issues)
    problems = results or measurements
    return {
        'status': 'FAILED' if results or (measurements and not partial) else 'incomplete' if measurements else 'passed',
        'failure_kind': 'result' if results else 'measurement' if measurements else None,
        'first_difference': problems[0] if problems else None, 'measurement_issues': measurements,
        'result_issues': results, 'events': events, 'worker_stages': sorted(stages, key=table_order), 'log_sha256': logs,
        'observation_sha256': observation_hashes, 'probe': probe, 'probe_rows': sorted(probes, key=table_order), 'probe_totals': totals, 'probe_issues': probe_issues,
        'probe_status': ('disabled' if not probe else 'incomplete'
                         if probe_issues or measurements or probe in (1, 2) and (
                             not totals or any(row['status'] != 'complete' for row in totals))
                         else 'complete'),
    }


def self_check():
    with TemporaryDirectory(prefix='fp3-collector-') as scratch:
        case = Path(scratch)

        def fixture(rank, control=False):
            def stage(name, rounds=0, collapses=0, lane=0):
                return (f'NATIVE3D_STAGE rank={rank} stage={name} rounds={rounds} length_collapses={collapses} '
                        f'split_lane={lane} cells=5 elapsed=3')

            def length(collapses=0):
                return f'NATIVE3D_LENGTH rank={rank} pass=collapse collapse_accepted={collapses} elapsed=2'

            def evaluate(quality, migrate, policy, max_owned=None, gain=80):
                row = (f'{OWNERSHIP} rank={rank} quality={quality} evaluate=1 attempts=0 migrate={migrate} '
                       f'predicted_imbalance=1.5 predicted_gain={gain} predicted_cost=30 global_cells=30 policy={policy}')
                return row if max_owned is None else row + f' max_owned_cells={max_owned} owned_ratio={max_owned / 5}'

            def commit(quality):
                return f'{OWNERSHIP} rank={rank} quality={quality} committed=1 moved_cells=19 total_seconds=2'

            def snapshot(boundary, seconds):
                return [
                    f'{TIMER} root=remesh boundary={boundary} rank={rank} phase=construction operation=census '
                    f'calls={seconds} inclusive_call_seconds={seconds} exclusive_seconds={seconds}',
                    f'{SNAPSHOT} root=remesh boundary={boundary} rank={rank} root_seconds={seconds}',
                ]

            seed = f'NATIVE3D_SEED_SHORT rank={rank} cells=5'
            policy = 'fp3b' if control else 'cells'
            initial = evaluate(0, 0, policy, gain=0) if control else evaluate(0, 1, policy, 11, gain=0)
            quality = evaluate(1, 1, policy) if control else evaluate(1, 0, policy, 10)
            event1 = [
                seed, length(), stage('length_collapse_exit'), initial,
                *([] if control else [commit(0)]),
                *snapshot('construction_entry', 1), stage('construction_entry'),
                quality, *([commit(1)] if control else []), stage('quality_entry'),
                stage('metric_cleanup_entry'), stage('metric_cleanup_exit', 2, 1),
                evaluate(1, 1, 'raw'), commit(1), stage('quality_entry', 2, 1),
                stage('quality_exit', 3, 1), stage('construction_entry', 3, 1),
                length(1), stage('length_collapse_exit', 3, 1), quality,
                *([commit(1)] if control else []), stage('quality_entry', 3, 1),
                stage('quality_exit', 4, 1), stage('construction_entry', 4, 1, 1),
                stage('length_split_exit', 5, 1), *snapshot('exit', 3), stage('exit', 6, 1),
            ]
            event1 += [seed, length(), stage('length_collapse_exit'), initial,
                       *([] if control else [commit(0)]), stage('exit')]
            partition = f'NATIVE3D_PARTITION rank={rank} parts=6'
            count1, count2 = ('Native3D frozen stages: count=2', 'Native3D frozen stages: count=1') if rank == 0 else ('', '')
            detail = f'NATIVE3D_DETAIL stage=refine rank={rank} initialization=0.5 seed=0.5'
            return '\n'.join([partition, *event1, detail, count1, partition, seed, length(),
                              stage('length_collapse_exit'), initial, *([] if control else [commit(0)]),
                              stage('exit'), count2, ''])

        def rank_path(rank):
            return case / f'rank_output/1/rank.{rank}/stdout'

        def trial(change, *, kind, field=None):
            path = rank_path(0)
            path.write_text(change(fixture(0)))
            row = collect(case)
            assert row['failure_kind'] == kind, row['first_difference']
            if field is not None:
                assert row['first_difference']['field'] == field, row['first_difference']
            path.write_text(fixture(0))

        for rank in range(6):
            path = rank_path(rank)
            path.parent.mkdir(parents=True)
            path.write_text(fixture(rank))
        accepted = collect(case)
        assert accepted['status'] == 'passed' and len(accepted['worker_stages']) == 18, accepted['first_difference']
        assert accepted['worker_stages'][0]['collapse_pass_time_status'] == 'pass time not available'
        assert accepted['worker_stages'][6]['collapse_pass_seconds'] == 1
        assert accepted['events'][0]['moved_cells'] == 57
        assert accepted['worker_stages'][0]['construction_census_delta']['exclusive_seconds'] == 2

        initial = next(line for line in fixture(0).splitlines() if 'quality=0 evaluate=1' in line)
        commit = next(line for line in fixture(0).splitlines() if 'quality=0 committed=1' in line)
        trial(lambda t: t.replace('stage=length_collapse_exit rounds=0',
                                  'stage=length_collapse_exit rounds=1', 1),
              kind='result', field='initial rounds')
        trial(lambda t: t.replace(initial + '\n' + commit + '\n', '', 1),
              kind='measurement', field='construction ownership evaluation missing')
        trial(lambda t: t.replace(initial, initial.replace('migrate=1', 'migrate=invalid'), 1),
              kind='measurement', field='ownership evaluation missing, cut or malformed')
        trial(lambda t: t.replace(initial, initial + ' global_cells=30', 1), kind='measurement')
        trial(lambda t: t.replace(initial, initial.replace('migrate=1', 'migrate=0'), 1),
              kind='result', field='ownership migrate')
        trial(lambda t: t.replace(initial, initial.replace('attempts=0', 'attempts=1e+6'), 1),
              kind='result', field='construction ownership attempts')
        trial(lambda t: t.replace(initial, initial + '\n' + initial, 1),
              kind='result', field='construction ownership evaluation count')
        trial(lambda t: t.replace('committed=1 moved_cells=19', 'committed=1 moved_cells=18', 1),
              kind='result', field='ownership collective moved_cells')
        trial(lambda t: t.replace(commit, commit.replace('quality=0', 'quality=1'), 1),
              kind='result', field='ownership commit pairing')
        trial(lambda t: t.replace(commit + '\n', '', 1),
              kind='measurement', field='ownership sequence row count by rank')
        trial(lambda t: t.replace('policy=raw', 'policy=fp3b', 1), kind='result', field='ownership policy')

        for rank in range(6):
            row = f'{OWNERSHIP} rank={rank} quality=0 committed=1 moved_cells=19 total_seconds=2\n'
            rank_path(rank).write_text(fixture(rank).replace(row, '', 1))
        validated = collect(case, event_receipts=[{'event': 1}])
        assert validated['failure_kind'] == 'result'
        assert validated['first_difference']['field'] == 'matching commit after positive ownership evaluation'
        partial = collect(case, partial=True)
        assert partial['failure_kind'] != 'result'

        for rank in range(6):
            rank_path(rank).write_text(fixture(rank, control=True))
        observation = collect(case, 3)
        assert observation['status'] == 'passed' and observation['probe_status'] == 'complete', observation
        assert observation['probe_rows'] == [] and observation['probe_totals'] == []
        rank_path(0).write_text(fixture(0, control=True).replace('NATIVE3D_SEED_SHORT rank=0 cells=5\n',
            'NATIVE3D_SEED_SHORT rank=0 cells=5\nNATIVE3D_FP3_ENGINE_COLLAPSE rank=0 accepted=0\n', 1))
        unexpected = collect(case, 3)
        assert unexpected['status'] == 'FAILED' and unexpected['probe_status'] == 'incomplete'


        def stall_fixture(rank, *, emit=True):
            def stage(name, rounded, *, covered=1, debt=2, phase='improvement', lane=0):
                return (f'NATIVE3D_STAGE rank={rank} stage={name} phase={phase} rounds={rounded} '
                        f'length_collapses=0 split_lane={lane} cells=5 elapsed={rounded} '
                        f'cache_consistent=1 census_complete=1 shape_complete={covered} '
                        f'q_bad=1 j_bad=1 q_debt={debt} j_debt=0 length_splits=0')

            seed = f'NATIVE3D_SEED_SHORT rank={rank} cells=5'
            length = f'NATIVE3D_LENGTH rank={rank} pass=collapse collapse_accepted=0'
            partition = f'NATIVE3D_PARTITION rank={rank} parts=6'
            census = [seed, length, stage('length_collapse_exit', 0, phase='construction'),
                      (f'{OWNERSHIP} rank={rank} quality=0 evaluate=1 attempts=0 migrate=0 '
                       'predicted_imbalance=1.5 predicted_gain=0 predicted_cost=30 global_cells=30 '
                       'policy=cells max_owned_cells=10 owned_ratio=2'),
                      stage('construction_entry', 0, phase='construction'), stage('quality_entry', 0)]
            rows, count = [], 0
            for rounded in (8, 16, 24, 32, 40, 48):
                observed = int(rounded != 16)
                rows.append(stage('window', rounded, covered=int(observed or rank != 4),
                                  debt=3 if rounded == 32 else 2))
                if rounded == 32:
                    rows += [
                        (f'{OWNERSHIP} rank={rank} quality=1 evaluate=1 attempts=0 migrate=1 '
                         'predicted_imbalance=1.5 predicted_gain=0 predicted_cost=30 global_cells=30 '
                         'policy=cells max_owned_cells=11 owned_ratio=2.2'),
                        stage('ownership_staged', rounded, debt=3),
                        f'{OWNERSHIP} rank={rank} quality=1 committed=1 moved_cells=19 total_seconds=2',
                        stage('ownership_committed', rounded),
                    ]
                count += observed
                if emit:
                    rows.append(f'{STALL} round={rounded} observed={observed} debt=12 bad=1.2e1 best=12 '
                                f'best_before={12 if count > 4 else -1} count={count} stalled={int(count > 4)}')
            if emit:
                rows += [f'{HANDOFF} round=48 taken=0 by=none failed=demand',
                         f'{HANDOFF} round=49 taken=1 by=stall failed=none']
            rows += [stage('quality_exit', 49), stage('construction_entry', 49, phase='construction', lane=1),
                     stage('length_split_exit', 52, phase='construction'), stage('quality_entry', 52),
                     stage('window', 56), stage('exit', 57)]
            count_row = ['Native3D frozen stages: count=1'] if rank == 0 else []
            return '\n'.join([partition, *census, *rows, *count_row,
                               partition, *census, stage('exit', 0), *count_row, ''])

        def stall_trial(change=lambda text: text, *, probe=0, results=None, altered_ranks=range(6)):
            for rank in range(6):
                text = stall_fixture(rank, emit=probe == 0)
                rank_path(rank).write_text(change(text) if rank in altered_ranks else text)
            row = collect(case, probe, **(results or {}))
            return row

        accepted = stall_trial()
        assert accepted['status'] == 'passed', accepted['first_difference']
        event1 = accepted['events'][0]['stall']
        assert event1['observed_debt_comparisons'] == 5 and event1['maximum_absolute_debt_difference'] == 0
        packet, = event1['packets']
        first, later = packet['quality_steps']
        assert (first['window_count'], later['window_count']) == (6, 0)
        assert first['fired_window']['round'] == 48 and packet['request_episodes'][0]['exit_round'] == 49
        assert stall_trial(probe=4)['status'] == 'passed'
        flip = stall_trial(lambda s: s.replace('round=16 observed=0', 'round=16 observed=1'))
        assert flip['failure_kind'] == 'result' and flip['first_difference']['field'] == 'independent stall observed'
        mismatch = stall_trial(lambda s: s.replace('round=32 observed=1 debt=12', 'round=32 observed=1 debt=18'))
        assert mismatch['failure_kind'] == 'result'
        assert mismatch['first_difference']['field'] == 'independent stall debt'
        disparity = stall_trial(lambda s: s.replace('round=16 observed=0 debt=12', 'round=16 observed=0 debt=14'),
                                altered_ranks=(2,))
        assert disparity['first_difference']['field'] == f'collective {STALL} debt'
        malformed = stall_trial(lambda s: s.replace('round=48 observed=1', 'round=48 observed=invalid'),
                                 altered_ranks=(0,))
        assert malformed['failure_kind'] == 'measurement', malformed['first_difference']
        missing = stall_trial(lambda s: '\n'.join(line for line in s.split('\n')
                              if not line.startswith(f'{STALL} round=24')))
        assert missing['failure_kind'] == 'measurement', missing['first_difference']
        extra = stall_trial(lambda s: s.replace(f'{HANDOFF} round=48',
                            f'{STALL} round=56 observed=1 debt=12 bad=12 best=12 best_before=-1 count=1 stalled=0\n'
                            f'{HANDOFF} round=48'))
        assert extra['failure_kind'] == 'result' and extra['first_difference']['field'] == 'stall row on inactive path'
        wrong_exit = stall_trial(lambda s: s.replace(f'{HANDOFF} round=49', f'{HANDOFF} round=48', 1)
                                .replace(f'{HANDOFF} round=48 taken=0 by=none failed=demand\n', '', 1))
        assert wrong_exit['first_difference']['field'] == 'taken handoff exit round'
        absent_exit = stall_trial(lambda s: s.replace('stage=quality_exit ', 'stage=geometry_repair_entry ', 1))
        assert absent_exit['failure_kind'] == 'result'
        assert absent_exit['first_difference']['field'] == 'taken handoff has no matching exit'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path, nargs='?')
    parser.add_argument('--probe', type=int, choices=(0, 1, 2, 3, 4), default=0)
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
