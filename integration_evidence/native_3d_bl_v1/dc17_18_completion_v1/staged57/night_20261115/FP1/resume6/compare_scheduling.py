"""Copy of the N22 comparator with the Resume 6 scheduling call exception."""

from dataclasses import replace
from pathlib import Path
import runpy

NIGHT = Path(__file__).resolve().parents[2]
DELTA = runpy.run_path(str(NIGHT / 'N11/section6_8_preparation/drafts/compare_timer_census_delta_69.py'))
PARSER = DELTA['PARSER']
N4 = PARSER.N4
M_FIELDS = {'collective_calls', 'collective_seconds', 'tracked_calls'}
C_FIELDS = M_FIELDS | {'election_calls', 'election_seconds', 'tracked_collective_seconds', 'tracked_worker_calls',
                      'nested_tracked_worker', 'nested_tracked_collectives', 'tracked_collective_call_seconds'}


def scan(path, rank, cutoff):
    """Get ownership evaluations and modeled storage through the same physical cutoff."""
    ownership, storage = [], []
    with path.open(errors='replace') as stream:
        for number, raw in enumerate(stream, 1):
            if number > cutoff:
                break
            line = N4.strip_mpi_prefixes(raw)
            if line.startswith('NATIVE3D_PHASE_OWNERSHIP '):
                fields = PARSER.fields_after(line, 'NATIVE3D_PHASE_OWNERSHIP ')
                if fields.get('rank') == str(rank) and fields.get('evaluate') == '1':
                    assert fields.get('migrate') in ('0', '1'), (path, number, fields)
                    assert fields.get('quality') in ('0', '1'), (path, number, fields)
                    ownership.append({'line': number, **fields})
            if line.startswith('NATIVE3D_STORAGE '):
                fields = PARSER.fields_after(line, 'NATIVE3D_STORAGE ')
                if fields.get('rank') == str(rank):
                    storage.append(fields)
    return ownership, storage


def parse_case(case, gate):
    logs = PARSER.source_files(case)
    rows = N4.streams(case)
    assert logs.keys() == rows.keys()
    assert len(logs) == 6, case
    results = {}
    for name, path in logs.items():
        rank = PARSER.stream_rank(logs, name)
        cutoff = PARSER.cutoff_lines(rows[name], gate)
        assert str(rank) in cutoff, (path, gate, cutoff)
        ownership, storage = scan(path, rank, cutoff[str(rank)])
        PARSER.verify_law_ranks(path, rank, cutoff)
        timers = PARSER.parse_timer_stream(path, cutoff)
        assert timers and all(row['summary'].get('fixedstorage') == '4120' for row in timers), path
        results[name] = {'plain': PARSER.plain_prefix(rows[name], cutoff, name, logs, gate),
                         'timers': timers, 'ownership': ownership, 'storage': storage}
    assert {PARSER.stream_rank(logs, name) for name in logs} == set(range(6))
    return results


def outcomes(before, after):
    """Compare the ordered quality/evaluation pairs and record each changed outcome."""
    old = [(row['quality'], row['migrate']) for row in before]
    new = [(row['quality'], row['migrate']) for row in after]
    changes = []
    for i, (a, b) in enumerate(zip(old, new), 1):
        if a[0] != b[0]:
            break
        if a[1] != b[1]:
            changes.append(i)
    return {'equal': old == new, 'changed_decisions': changes, 'reference': before, 'candidate': after}


def split_communication(snapshot, allowance):
    if not allowance:
        return snapshot, {}
    own = [row for row in snapshot['rows'] if row['fields']['operation'] == 'communication']
    communication = {}
    for row in own:
        fields = row['fields']
        phase = fields['phase']
        assert phase not in communication, (snapshot['path'], snapshot['line'], phase)
        communication[phase] = {'calls': PARSER.parse_calls(fields, row['path'], row['line']),
                                **{key: fields[key] for key in PARSER.TIMER_MEASUREMENTS}}
    rest = [row for row in snapshot['rows'] if row['fields']['operation'] != 'communication']
    return dict(snapshot, rows=rest), communication


SCHEDULING_BOUNDARIES = {"length_collapse_exit": 1, "construction_entry": 74}


def compare_snapshot(left, right):
    root, boundary, rank = left['key']
    if root != 'remesh' or boundary not in SCHEDULING_BOUNDARIES:
        return DELTA['compare_snapshot'](left, right)

    def scheduling_rows(snapshot):
        return [row for row in snapshot['rows'] if
                (row['fields']['phase'], row['fields']['operation']) == ('construction', 'scheduling')]

    old, new = scheduling_rows(left), scheduling_rows(right)
    if len(old) != 1 or len(new) != 1:
        return {'passed': False, 'key': left['key'], 'difference': 'expected one construction/scheduling row',
                'reference_rows': len(old), 'candidate_rows': len(new)}
    old, new = old[0], new[0]
    before = PARSER.parse_calls(old['fields'], old['path'], old['line'])
    after = PARSER.parse_calls(new['fields'], new['path'], new['line'])
    normalized = dict(new, fields=dict(new['fields'], calls=old['fields']['calls']))
    comparable = dict(right, rows=[normalized if row is new else row for row in right['rows']])
    result = DELTA['compare_snapshot'](left, comparable)
    return dict(result, scheduling_calls={'snapshot': boundary, 'rank': int(rank), 'reference_calls': before,
                                         'calls': after, 'delta': after - before,
                                         'expected_delta': SCHEDULING_BOUNDARIES[boundary]})


def scheduling_differences(timers, gate):
    """Require one allowed row per rank and cutoff, and equal per-rank deltas."""
    rows = [dict(row['scheduling_calls'], case=gate) for stream in timers.values()
            for row in stream['snapshots'] if 'scheduling_calls' in row]
    boundaries = ('length_collapse_exit',) if gate == 'G2' else tuple(SCHEDULING_BOUNDARIES)
    if len(rows) != 6 * len(boundaries):
        return rows, {'scheduling_row_count': len(rows), 'expected': 6 * len(boundaries)}
    for boundary in boundaries:
        selected = [row for row in rows if row['snapshot'] == boundary]
        ranks = sorted(row['rank'] for row in selected)
        if ranks != list(range(6)) or len({row['delta'] for row in selected}) != 1:
            return rows, {'inconsistent_scheduling_deltas': boundary, 'observed': selected}
    return rows, None


def require_shared_collapse_delta(g2, gq):
    before, after = (next(row['delta'] for row in result['scheduling_calls']
                          if row['snapshot'] == 'length_collapse_exit') for result in (g2, gq))
    return None if before == after else {'collapse_scheduling_delta': {'G2': before, 'GQ': after}}


def compare_timer_lists(before, after, allowance):
    if len(before) != len(after):
        return {'passed': False, 'first_difference': {'timer_snapshot_counts': [len(before), len(after)]}}
    results = []
    for old, new in zip(before, after, strict=True):
        left, old_calls = split_communication(old, allowance)
        right, new_calls = split_communication(new, allowance)
        result = compare_snapshot(left, right)
        result['communication'] = {'reference': old_calls, 'candidate': new_calls}
        results.append(result)
        if not result['passed']:
            return {'passed': False, 'first_difference': result, 'snapshots': results}
    return {'passed': True, 'snapshots': results}


def compare_cases(part, gate, reference, candidate):
    before, after = parse_case(reference, gate), parse_case(candidate, gate)
    assert before.keys() == after.keys()
    decisions = {name: outcomes(before[name]['ownership'], after[name]['ownership']) for name in before}
    changed = any(row['changed_decisions'] for row in decisions.values())
    same_decisions = all(row['equal'] for row in decisions.values())
    result = {'part': part, 'gate': gate, 'reference': str(reference), 'candidate': str(candidate),
              'decisions_changed': changed, 'same_decisions': same_decisions, 'decisions': decisions,
              'fixedstorage': 4120}
    if part == 'M' and gate == 'GQ' and changed:
        # The physical prefix has already passed. Rows after the changed decision
        # are not an identity gate in the two-full-run branch.
        return dict(result, passed=True, identity_check='changed-decision branch')
    if not same_decisions:
        return dict(result, passed=False, first_difference='ownership evaluation count, phase, or outcome')
    allowance = part == 'C' or (part == 'M' and (gate == 'GQ' or any(
        row['reference'] for row in decisions.values())))
    ignored = (C_FIELDS if part == 'C' else M_FIELDS) if allowance else set()

    def filtered(streams):
        return {name: [replace(row, fields={k: v for k, v in row.fields.items() if k not in ignored})
                       for row in entry['plain']] for name, entry in streams.items()}

    integer = N4.compare_streams(filtered(before), filtered(after), gate)
    timers, storage = {}, {}
    for name in before:
        timers[name] = compare_timer_lists(before[name]['timers'], after[name]['timers'], allowance)
        if before[name]['storage'] != after[name]['storage']:
            storage[name] = {'reference': before[name]['storage'], 'candidate': after[name]['storage']}
    first = integer.get('first_difference') or next(
        (row.get('first_difference') for row in timers.values() if not row['passed']), None)
    if not first and storage:
        stream, difference = next(iter(storage.items()))
        first = {'storage_stream': stream, **difference}
    scheduling = []
    if not first:
        scheduling, first = scheduling_differences(timers, gate)
    return dict(result, passed=not first, integer_rows=integer, timer_rows=timers, scheduling_calls=scheduling,
                modeled_storage_equal=not storage, storage_differences=storage, first_difference=first)


def self_check():
    from tempfile import TemporaryDirectory
    with TemporaryDirectory(prefix='fp1-scheduling-') as directory:
        root = Path(directory)
        for side in ('old', 'new'):
            for rank in range(6):
                prefix = f'root=remesh boundary=length_collapse_exit rank={rank}'
                suffix = 'inclusive_call_seconds=1.0 exclusive_seconds=0.9'
                calls = 8136 + (side == 'new')
                lines = [
                    f'NATIVE3D_TIMER {prefix} phase=construction operation=communication calls=20 {suffix}',
                    f'NATIVE3D_TIMER {prefix} phase=construction operation=import calls=6 {suffix}',
                    f'NATIVE3D_TIMER {prefix} phase=construction operation=scheduling calls={calls} {suffix}',
                    f'NATIVE3D_TIMER_SUMMARY {prefix} ranks=6 fixedstorage=4120',
                    f'NATIVE3D_STAGE rank={rank} stage=length_collapse_exit collective_calls=13 atlas_bytes=25',
                ]
                path = root / side / f'rank_output/1/rank.{rank}/stdout'
                path.parent.mkdir(parents=True)
                path.write_text('\n'.join(lines) + '\n')
        old, new = root / 'old', root / 'new'
        accepted = compare_cases('C', 'G2', old, new)
        assert accepted['passed'] and len(accepted['scheduling_calls']) == 6, accepted
        assert all(row['delta'] == 1 for row in accepted['scheduling_calls'])
        assert require_shared_collapse_delta(accepted, accepted) is None
        bad = {'scheduling_calls': [dict(row, delta=2) for row in accepted['scheduling_calls']]}
        assert require_shared_collapse_delta(accepted, bad)
        sample = new / 'rank_output/1/rank.0/stdout'
        sealed = sample.read_text()
        sample.write_text(sealed.replace('operation=import calls=6', 'operation=import calls=7'))
        changed = compare_cases('C', 'G2', old, new)
        assert not changed['passed'] and 'calls' in changed['first_difference']['difference'], changed
        sample.write_text('\n'.join(line for line in sealed.splitlines() if 'operation=scheduling' not in line) + '\n')
        missing = compare_cases('C', 'G2', old, new)
        assert not missing['passed'] and missing['first_difference']['candidate_rows'] == 0, missing
        sample.write_text(sealed.replace('operation=scheduling calls=8137', 'operation=scheduling calls=8138'))
        uneven = compare_cases('C', 'G2', old, new)
        assert not uneven['passed'] and uneven['first_difference']['inconsistent_scheduling_deltas'] == 'length_collapse_exit'
    print('Resume 6 scheduling comparator self-check passed.')


if __name__ == '__main__':
    self_check()
