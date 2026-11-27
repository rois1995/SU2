"""Check FP3-E rows against the independent stage and ownership evidence."""

from collections import defaultdict
from dataclasses import dataclass, field
from decimal import Decimal, InvalidOperation
import math


STALL = 'NATIVE3D_QUALITY_STALL'
HANDOFF = 'NATIVE3D_QUALITY_STALL_HANDOFF'
STALL_KINDS = (STALL, HANDOFF)
STAGE = 'NATIVE3D_STAGE'
OWNERSHIP = 'NATIVE3D_PHASE_OWNERSHIP'
PRIVATE = 'NATIVE3D_PRIVATE_WORK'
QUALITY = 'NATIVE3D_QUALITY'
QUALITY_ENDS = {'quality_exit', 'metric_cleanup_entry', 'construction_entry', 'exit'}
BY = {'ordinary', 'no_candidate', 'stall'}
FAILED = {'metric_cleanup', 'phase', 'improvement', 'demand'}
CONSTRUCTION_LANES = {'length_collapse_exit', 'split_surface_exit', 'split_interior_entry',
                      'split_interior_exit', 'split_surface_resume', 'length_split_exit'}
DEBTS = ('q_debt', 'j_debt', 'size_debt', 'geometry_debt')


@dataclass
class Window:
    row: object
    position: int
    step: int
    candidate: bool
    migration: bool = False
    committed: bool = False
    source: object = None
    source_position: int = 0


@dataclass
class Packet:
    event: object
    index: int
    records: list
    steps: list = field(default_factory=list)
    windows: dict = field(default_factory=dict)
    entries: list = field(default_factory=list)
    rows: dict = field(default_factory=lambda: {kind: {} for kind in STALL_KINDS})
    incomplete_stall: bool = False
    measurements: list = field(default_factory=list)
    failures: list = field(default_factory=list)

    def issue(self, target, problem, rec, **details):
        target.append(self.event.issue(problem, rec, self.index, **details))

    def measure(self, problem, rec, **details):
        self.issue(self.measurements, problem, rec, **details)

    def reject(self, problem, rec, expected, actual):
        self.issue(self.failures, problem, rec, expected=expected, actual=actual)


def round_count(row):
    return row.optional('rounds') if row else None


def count_as_double(row, key):
    value = Decimal(row.fields.get(key, ''))
    if not value.is_finite() or not 0 <= value < 2**53 or value != value.to_integral_value():
        raise ValueError(key)
    return int(value)


def decode(row):
    if row.cut:
        raise ValueError('cut row')
    rounded = row.number('round')
    expected = {'round', 'taken', 'by', 'failed'} if row.kind == HANDOFF else {
        'round', 'observed', 'debt', 'bad', 'best', 'best_before', 'count', 'stalled'}
    if set(row.fields) - {'rank'} != expected:
        raise ValueError('missing or unknown fields')
    if row.kind == HANDOFF:
        taken = row.number('taken')
        by, failed = (row.fields.get(key) for key in ('by', 'failed'))
        if taken not in (0, 1) or by not in BY | {'none'} or failed not in FAILED | {'none'}:
            raise ValueError('handoff fields')
        return {'round': rounded, 'taken': taken, 'by': by, 'failed': failed}
    debt, best, before = (float(row.fields.get(key, '')) for key in ('debt', 'best', 'best_before'))
    count, observed, stalled = (row.number(key) for key in ('count', 'observed', 'stalled'))
    if (not all(math.isfinite(value) for value in (debt, best, before))
            or observed not in (0, 1) or stalled not in (0, 1)):
        raise ValueError('window fields')
    return {'round': rounded, 'observed': observed, 'debt': debt, 'bad': count_as_double(row, 'bad'),
            'best': best, 'best_before': before, 'count': count, 'stalled': stalled}


def source_values(row):
    valid = [row.number(key) for key in ('cache_consistent', 'census_complete', 'shape_complete')]
    if any(value not in (0, 1) for value in valid):
        raise ValueError('local census validity flags')
    bad = row.number('q_bad') + row.number('j_bad')
    debt = sum(float(row.fields.get(key, '')) for key in ('q_debt', 'j_debt'))
    if not math.isfinite(debt):
        raise ValueError('local debt')
    return all(valid), bad, debt


def read_packet(event, index, records, check):
    packet = Packet(event, index, records)
    queue_closed, step, window = True, None, None
    for position, row in enumerate(records):
        if row.kind in STALL_KINDS:
            try:
                values = decode(row)
            except (ValueError, InvalidOperation) as error:
                if check:
                    packet.measure('malformed stall row', row, row=row.kind, error=str(error))
                    if row.kind == STALL:
                        packet.incomplete_stall = True
                continue
            rounded = values['round']
            if rounded in packet.rows[row.kind]:
                if check:
                    packet.reject('duplicate stall row', row, 'one row per completed round', rounded)
                continue
            packet.rows[row.kind][rounded] = (position, row, values)
        elif row.kind == OWNERSHIP and window is not None and row.optional('quality') == 1:
            if row.optional('evaluate') == 1:
                window.migration = row.optional('migrate') == 1
            elif row.optional('committed') == 1:
                window.committed = True
        elif row.kind == STAGE:
            stage = row.fields.get('stage')
            rounded = round_count(row)
            if stage == 'ownership_committed':
                if window and rounded == round_count(window.row):
                    window.source = row
                    window.source_position = position
                continue
            if stage == 'ownership_staged':
                continue
            window = None
            if stage in QUALITY_ENDS:
                if step is not None:
                    step['end'] = position
                    step = None
                if stage in ('construction_entry', 'metric_cleanup_entry'):
                    queue_closed = stage == 'construction_entry'
                packet.entries.append((position, stage, queue_closed))
            if stage == 'quality_entry':
                if step is not None:
                    step['end'] = position
                step = {'start': position, 'end': None}
                packet.entries.append((position, stage, queue_closed))
                packet.steps.append(step)
            elif stage == 'surface_repair_entry':
                packet.entries.append((position, stage, queue_closed))
            elif stage == 'window':
                if rounded is None:
                    if check:
                        packet.measure('malformed independent window round', row)
                elif rounded in packet.windows:
                    if check:
                        packet.reject('duplicate independent window', row, 'one window per completed round', rounded)
                else:
                    candidate = (queue_closed and step is not None and row.fields.get('phase') == 'improvement')
                    window = Window(row, position, len(packet.steps) - 1, candidate, source=row, source_position=position)
                    packet.windows[rounded] = window
    return packet


def same_rows(packets, reference, kind):
    expected = reference.rows[kind]
    for packet in packets.values():
        if packet is reference:
            continue
        actual = packet.rows[kind]
        if expected.keys() != actual.keys():
            if kind == STALL:
                packet.incomplete_stall |= bool(expected.keys() - actual.keys())
                reference.incomplete_stall |= bool(actual.keys() - expected.keys())
            packet.measure('stall row presence across ranks', packet.records[0], row=kind,
                           rank_reference=reference.event.rank, expected=list(expected), actual=list(actual))
        common = expected.keys() & actual.keys()
        if [key for key in expected if key in common] != [key for key in actual if key in common]:
            packet.reject('stall round order across ranks', packet.records[0], list(expected), list(actual))
        for rounded, (_, _, expected_values) in expected.items():
            if rounded not in actual:
                continue
            _, row, actual_values = actual[rounded]
            for key, value in expected_values.items():
                if actual_values[key] != value:
                    packet.reject(f'collective {kind} {key}', row, value, actual_values[key])


def check_windows(packets, reference, enabled, event_passed):
    """Rebuild the phase history and validate the rank-local source for each observation."""
    if enabled:
        expected_windows = [key for key, window in reference.windows.items() if window.candidate]
        for packet in packets.values():
            actual_windows = [key for key, window in packet.windows.items() if window.candidate]
            if expected_windows != actual_windows:
                packet.measure('independent active windows across ranks', packet.records[0],
                               rank_reference=reference.event.rank, expected=expected_windows, actual=actual_windows)
    active_step, best, history, fired, replayable = None, -1, [], False, True
    expected_rounds, debt_errors = set(), []
    for rounded, window in reference.windows.items():
        if not enabled or not window.candidate:
            continue
        if window.step != active_step:
            active_step, best, history, replayable = window.step, -1, [], True
        if fired:
            continue
        expected_rounds.add(rounded)
        source = []
        for packet in packets.values():
            local = packet.windows.get(rounded)
            if local is None:
                packet.measure('independent window evidence missing', packet.records[0], round=rounded)
                continue
            if not local.candidate:
                packet.reject('independent window eligibility across ranks', local.row, True, False)
                continue
            if local.migration or local.committed:
                if not local.committed or local.source is local.row:
                    if event_passed or rounded in packet.rows[STALL] or any(
                            key > rounded for key in packet.windows):
                        packet.measure('post-migration window evidence missing', local.row, round=rounded)
                    continue
            try:
                source.append(source_values(local.source))
            except (ValueError, InvalidOperation) as error:
                packet.measure('malformed independent stall input', local.source, error=str(error))
        parsed = reference.rows[STALL].get(rounded)
        if parsed is None:
            if event_passed or len(source) == 6 and any(
                    later.kind == PRIVATE and later.optional('round') is not None and later.optional('round') > rounded
                    for later in reference.records[window.position + 1:]):
                reference.measure('active stall row missing', window.row, round=rounded)
            if event_passed or any(rounded in packet.rows[STALL] for packet in packets.values()):
                reference.incomplete_stall = True
            replayable = False
            continue
        _, row, values = parsed
        for packet in packets.values():
            local = packet.windows.get(rounded)
            reported = packet.rows[STALL].get(rounded)
            if local is not None and reported is not None and reported[0] <= local.source_position:
                packet.reject('stall observation order', reported[1], 'after the independent source row', rounded)
        if len(source) == 6:
            expected_observed = int(all(item[0] for item in source))
            expected_bad = sum(item[1] for item in source)
            for key, expected in (('observed', expected_observed), ('bad', expected_bad)):
                if values[key] != expected:
                    reference.reject(f'independent stall {key}', row, expected, values[key])
            if expected_observed and values['observed']:
                debt = math.fsum(item[2] for item in source)
                difference = abs(values['debt'] - debt)
                relative = difference / abs(debt) if debt else 0.0 if difference == 0 else None
                debt_errors.append((difference, relative))
                if difference and (relative is None or relative > 1e-12):
                    reference.reject('independent stall debt', row, debt, values['debt'])
        if replayable:
            before, stalled = -1, 0
            if values['observed']:
                best = values['debt'] if best < 0 else min(best, values['debt'])
                before = history[-4] if len(history) >= 4 else -1
                stalled = int(len(history) >= 4 and values['bad'] > 0 and best > 0.95 * before)
                history.append(best)
            for key, expected in (('count', len(history)), ('best', best), ('best_before', before),
                                  ('stalled', stalled)):
                if values[key] != expected:
                    reference.reject(f'stall replay {key}', row, expected, values[key])
            fired = bool(stalled)
        elif values['stalled']:
            fired = True
    for packet in packets.values():
        for rounded, (position, row, _) in packet.rows[STALL].items():
            if not enabled:
                packet.reject('stall row on disabled path', row, 'no stall row', rounded)
            elif rounded not in expected_rounds:
                boundary = next((entry for entry in reversed(packet.entries) if entry[0] < position), None)
                phase_ended = boundary is None or boundary[1] != 'quality_entry' or not boundary[2]
                if phase_ended or rounded in reference.windows:
                    packet.reject('stall row on inactive path', row, 'no stall row', rounded)
                else:
                    packet.measure('stall row without independent window', row, round=rounded)
    return debt_errors


def check_handoffs(packet, enabled, event_passed):
    by_position = {position: (name, closed) for position, name, closed in packet.entries}
    fires = {position: values for position, _, values in packet.rows[STALL].values() if values['stalled']}
    hands = {position: values for position, _, values in packet.rows[HANDOFF].values()}
    if len(fires) > 1:
        first_extra = list(fires)[1]
        packet.reject('fires per worker packet', packet.records[first_extra], 'at most one', len(fires))
    fire, taken = None, None
    episodes = []
    for position, row in enumerate(packet.records):
        if position in fires:
            fire = {'fire': fires[position], 'handoffs': [], 'resolution': 'no hand-off call recorded'}
            episodes.append(fire)
        if position in hands:
            values = hands[position]
            if enabled and fire is None and packet.incomplete_stall:
                packet.measure('handoff pending request evidence incomplete', row, round=values['round'])
            elif not enabled or fire is None or fire.get('preempted_by'):
                packet.reject('handoff without pending closed-queue request', row,
                              'a prior fire with closed queue before phase entry', values['round'])
            else:
                fire['handoffs'].append(values)
                fire['resolution'] = 'hand-off call recorded'
            if values['taken']:
                correct = values['by'] in BY and values['failed'] == 'none'
                desc = 'named by and failed=none'
            else:
                correct = values['by'] == 'none' and values['failed'] in FAILED
                desc = 'by=none and named failed guard'
            if not correct:
                packet.reject('handoff disposition', row, desc, values)
            if values['taken']:
                if taken is not None:
                    packet.reject('handoff matched exit', row, 'one exit preceding the next taken row', taken['round'])
                taken = values
        if position not in by_position:
            continue
        stage, closed = by_position[position]
        if stage == 'quality_exit':
            rounded = round_count(row)
            if fire is not None and closed and taken is None:
                packet.measure('pending quality exit has no taken handoff row', row, round=rounded)
            if taken is not None and taken['round'] != rounded:
                packet.reject('taken handoff exit round', row, taken['round'], rounded)
            if fire is not None and taken is not None:
                fire.update(resolution='hand-off taken', exit_round=rounded)
            taken = None
            continue
        if taken is not None:
            if event_passed:
                packet.reject('taken handoff has no matching exit', row, taken['round'], stage)
            else:
                packet.measure('terminal taken handoff lacks matching exit evidence', row, round=taken['round'])
            taken = None
        if stage == 'metric_cleanup_entry':
            if fire is not None:
                fire['preempted_by'] = stage
        elif fire is not None:
            fire.update(next_boundary=stage, next_boundary_round=round_count(row))
            fire = None
    if taken is not None:
        if event_passed and packet.records[-1].fields.get('stage') == 'exit':
            packet.reject('taken handoff has no matching exit', packet.records[-1], taken['round'], None)
        else:
            packet.measure('terminal taken handoff lacks matching exit evidence', packet.records[-1], round=taken['round'])
    return episodes


def project_stage(row):
    if not row:
        return None
    return {'stage': row.fields.get('stage'), 'round': round_count(row), 'elapsed_seconds': row.seconds(),
            'length_splits': row.optional('length_splits'), 'line': row.line,
            'debt': {key: row.seconds(key) for key in DEBTS}}


def longest_single_rank(rounds):
    longest, current = [], []
    for rounded, ranks in sorted(rounds.items()):
        active = [(rank, row) for rank, row in ranks.items() if row['choices'] > 0]
        if len(ranks) != 6 or len(active) != 1:
            current = []
            continue
        rank, values = active[0]
        if current and (current[-1]['rank'] != rank or current[-1]['round'] != rounded - 1):
            current = []
        current.append({'round': rounded, 'rank': rank, **values})
        if len(current) > len(longest):
            longest = current.copy()
    return longest


def join_wait(censuses, first, last):
    # A census reports the delta since the prior census, even across a skipped window.
    intervals = []
    for before, row in zip(censuses, censuses[1:]):
        a, b = before.optional('round'), row.optional('round')
        ac, bc = before.optional('census'), row.optional('census')
        seconds = row.seconds('join_call_seconds')
        if None not in (a, b, ac, bc, seconds) and bc == ac + 1 and first <= a + 1 <= b <= last:
            intervals.append({'first_round': a + 1, 'last_round': b, 'join_wait_seconds': seconds,
                              'opening_census': before.line, 'closing_census': row.line})
    aligned = (bool(intervals) and intervals[0]['first_round'] == first and intervals[-1]['last_round'] == last
               and all(before['last_round'] + 1 == row['first_round'] for before, row in zip(intervals, intervals[1:])))
    total = math.fsum(row['join_wait_seconds'] for row in intervals)
    return {'covered_intervals': intervals, 'covered_join_wait_seconds': total,
            'exact_span_join_wait_seconds': total if aligned else None}


def report_quality(packets, reference):
    rows = []
    for i, phase in enumerate(reference.steps):
        entry, exit = (project_stage(reference.records[phase[key]]) if phase[key] is not None else None
                       for key in ('start', 'end'))
        work, censuses, boundary = defaultdict(dict), {}, {}
        for rank, packet in packets.items():
            if i >= len(packet.steps):
                continue
            step = packet.steps[i]
            start, end = step['start'], step['end']
            if end is None:
                end = len(packet.records) - 1
            # Complete census rows precede the matching STAGE rows.
            census = [row for row in packet.records[:end + 1] if row.kind == QUALITY]
            censuses[rank] = census
            boundary[rank] = {'entry': project_stage(packet.records[start]),
                              'exit': project_stage(packet.records[step['end']]) if step['end'] is not None else None}
            for row in packet.records[start + 1:end]:
                if row.kind == PRIVATE:
                    rounded, choices = (row.optional(key) for key in ('round', 'choices'))
                    busy = row.seconds('busy_seconds')
                    if None not in (rounded, choices, busy):
                        work[rounded][rank] = {'choices': choices, 'busy_seconds': busy}
        single = longest_single_rank(work)
        for rank in boundary:
            owned = [values[rank] for values in work.values() if rank in values]
            boundary[rank].update(private_round_rows=len(owned), rounds_with_choices=sum(
                row['choices'] > 0 for row in owned), total_choices=sum(row['choices'] for row in owned),
                private_busy_seconds=math.fsum(row['busy_seconds'] for row in owned))
        tail = None
        if single:
            rank, first, last = single[0]['rank'], single[0]['round'], single[-1]['round']
            tail = {'rank': rank, 'first_round': first, 'last_round': last, 'round_count': len(single),
                    'total_choices': sum(row['choices'] for row in single),
                    'private_busy_seconds': math.fsum(row['busy_seconds'] for row in single),
                    'per_round': single,
                    'other_ranks_join_wait': {r: join_wait(census, first, last) for r, census in censuses.items()
                                              if r != rank}}
        window_values = [values for position, _, values in reference.rows[STALL].values()
                         if phase['start'] < position < (phase['end'] or len(reference.records))]
        phase_seconds = [row['exit']['elapsed_seconds'] - row['entry']['elapsed_seconds']
                         for row in boundary.values() if row['exit'] is not None
                         and row['entry']['elapsed_seconds'] is not None and row['exit']['elapsed_seconds'] is not None]
        rows.append({'entry_round': entry['round'], 'exit_round': exit['round'] if exit else None,
                     'end_stage': exit['stage'] if exit else None,
                     'seconds_max': max(phase_seconds) if len(phase_seconds) == 6 else None,
                     'by_rank': boundary, 'longest_single_rank': tail, 'window_count': len(window_values),
                     'first_window': window_values[0] if window_values else None,
                     'last_window': window_values[-1] if window_values else None,
                     'fired_window': next((row for row in window_values if row['stalled']), None)})
    return rows


def construction_after_quality(packets, reference):
    """Report each construction span after a quality exit, including the old D hand-off."""
    by_rank = {}
    for rank, packet in packets.items():
        spans, active = [], None
        for row in packet.records:
            if row.kind != STAGE:
                continue
            name = row.fields.get('stage')
            if name == 'quality_exit':
                active = {'quality_exit': project_stage(row), 'construction_entry': None, 'stages': []}
                spans.append(active)
            elif active is not None and name == 'construction_entry':
                active['construction_entry'] = project_stage(row)
            elif active is not None and name in CONSTRUCTION_LANES:
                active['stages'].append(project_stage(row))
            elif active is not None and name in ('quality_entry', 'exit'):
                last = project_stage(row)
                active['end'] = last
                start = active['construction_entry']
                if start is not None:
                    for key, field_name in (('seconds', 'elapsed_seconds'), ('splits', 'length_splits'),
                                            ('rounds', 'round')):
                        a, b = start[field_name], last[field_name]
                        active[key] = b - a if a is not None and b is not None else None
                active = None
        by_rank[rank] = spans
    spans = []
    for index, span in enumerate(by_rank[reference.event.rank]):
        rows = {rank: values[index] for rank, values in by_rank.items() if index < len(values)}
        seconds = [row['seconds'] for row in rows.values() if row.get('seconds') is not None]
        splits = [row['splits'] for row in rows.values() if row.get('splits') is not None]
        spans.append({'quality_exit_round': span['quality_exit']['round'], 'by_rank': rows,
                      'seconds_max': max(seconds) if len(seconds) == 6 else None,
                      'splits_sum': sum(splits) if len(splits) == 6 else None})
    return spans


def phase_entries(packets):
    names = {'construction_entry', 'quality_entry', 'metric_cleanup_entry',
             'size_repair_entry', 'geometry_repair_entry', 'surface_repair_entry', 'surface_required_entry'}
    return {rank: [project_stage(row) for row in packet.records if row.kind == STAGE
                   and row.fields.get('stage') in names] for rank, packet in packets.items()}


def inspect_stall(ranks, packet_records, *, enabled, check, event_passed):
    """Pair seed packets by position and FP3-E evidence by completed round."""
    report, measurements, failures, errors = [], [], [], []
    for index in range(max(map(len, packet_records.values()), default=0)):
        packets = {rank: read_packet(ranks[rank], index, rows[index], check) for rank, rows in packet_records.items()
                   if index < len(rows)}
        reference = packets[min(packets)]
        episodes = []
        if check:
            for kind in STALL_KINDS:
                same_rows(packets, reference, kind)
            errors.extend(check_windows(packets, reference, enabled, event_passed))
            by_rank = {rank: check_handoffs(packet, enabled, event_passed) for rank, packet in packets.items()}
            episodes = by_rank[reference.event.rank]
        report.append({'worker_stage': index, 'quality_steps': report_quality(packets, reference),
                       'reference_rank': reference.event.rank,
                       'windows': [values for _, _, values in reference.rows[STALL].values()],
                       'phase_entries_by_rank': phase_entries(packets),
                       'handoffs': [values for _, _, values in reference.rows[HANDOFF].values()],
                       'request_episodes': episodes,
                       'construction_after_quality': construction_after_quality(packets, reference)})
        for packet in packets.values():
            measurements.extend(packet.measurements)
            failures.extend(packet.failures)
    relative = [error for _, error in errors if error is not None]
    return {'packets': report, 'observed_debt_comparisons': len(errors),
            'maximum_relative_debt_difference': max(relative) if relative else None,
            'maximum_absolute_debt_difference': max((error for error, _ in errors), default=None)}, measurements, failures
