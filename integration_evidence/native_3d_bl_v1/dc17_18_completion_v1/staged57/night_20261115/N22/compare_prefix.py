"""Compare ordered N22 G2/GQ evidence with the inherited exact gate parser."""

from dataclasses import replace
from pathlib import Path
import runpy

NIGHT = Path(__file__).resolve().parent.parent
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


def compare_timer_lists(before, after, allowance):
    if len(before) != len(after):
        return {'passed': False, 'first_difference': {'timer_snapshot_counts': [len(before), len(after)]}}
    results = []
    for old, new in zip(before, after, strict=True):
        left, old_calls = split_communication(old, allowance)
        right, new_calls = split_communication(new, allowance)
        result = DELTA['compare_snapshot'](left, right)
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
    return dict(result, passed=not first, integer_rows=integer, timer_rows=timers,
                modeled_storage_equal=not storage, storage_differences=storage, first_difference=first)


def self_check():
    from tempfile import TemporaryDirectory
    with TemporaryDirectory() as directory:
        root = Path(directory)
        old = root / 'old'
        new = root / 'new'
        for rank in range(6):
            name = f'rank_output/1/rank.{rank}/stdout'
            original = [
                f'NATIVE3D_STAGE rank={rank} stage=length_collapse_exit collective_calls=13 atlas_bytes=25',
                f'NATIVE3D_PHASE_OWNERSHIP rank={rank} quality=0 evaluate=1 migrate=1',
                f'NATIVE3D_TIMER root=remesh boundary=entry rank={rank} phase=construction operation=communication calls=20 inclusive_call_seconds=1.0 exclusive_seconds=0.9',
                f'NATIVE3D_TIMER root=remesh boundary=entry rank={rank} phase=construction operation=import calls=6 inclusive_call_seconds=2.0 exclusive_seconds=1.9',
                f'NATIVE3D_TIMER_SUMMARY root=remesh boundary=entry rank={rank} ranks=6 fixedstorage=4120',
                f'NATIVE3D_STORAGE rank={rank} stage=quality_entry native_peak_bytes=42',
                *(f'NATIVE3D_QUALITY rank={rank} stage=window census={i} commits=0' for i in range(3)),
            ]
            for side, lines in (
                    (old, original),
                    (new, [line.replace('calls=20 ', 'calls=2 ').replace('collective_calls=13', 'collective_calls=3')
                           if not line.startswith('NATIVE3D_STAGE') else line for line in original])):
                path = side / name
                path.parent.mkdir(parents=True)
                path.write_text('\n'.join(lines) + '\n')
        assert compare_cases('M', 'GQ', old, new)['passed']
        assert compare_cases('C', 'GQ', old, new)['passed']
        sample = new / 'rank_output/1/rank.0/stdout'
        sealed = sample.read_text()
        sample.write_text(sealed.replace('operation=import calls=6', 'operation=import calls=7'))
        assert not compare_cases('C', 'GQ', old, new)['passed']
        sample.write_text(sealed.replace('native_peak_bytes=42', 'native_peak_bytes=43'))
        assert not compare_cases('C', 'GQ', old, new)['passed']
        sample.write_text(sealed.replace('migrate=1', 'migrate=0'))
        assert compare_cases('M', 'GQ', old, new)['decisions_changed']
        assert not compare_cases('C', 'GQ', old, new)['passed']
    print('N22 prefix comparison self-check passed.')


if __name__ == '__main__':
    self_check()
