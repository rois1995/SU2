"""F3b complete-event coverage and JoinWait checks; read existing logs only."""
import importlib.util
import json
import math
import re
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('exclusive_timer_gate', HERE / 'check_timer_tree.py')
timer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timer)


def join_matches(measured, legacy):
    """Require finite nonnegative join time and the reviewer's one-percent agreement."""
    if not all(math.isfinite(value) and value >= 0. for value in (measured, legacy)):
        return False
    return abs(measured - legacy) <= .01 * legacy if legacy > 0. else measured == 0.


def rank_events(path):
    """Group legacy window deltas by the following terminal remesh root, never by wall-clock guesses."""
    text = path.read_text(errors='replace')
    rank = int(path.parent.name.split('.')[-1])
    roots = [item for item in timer.snapshots(text)
             if item['summary']['root'] == 'remesh' and item['summary']['boundary'] == 'final'
             and int(item['summary']['rank']) == rank]
    legacy = []
    current = {'join_seconds': 0., 'windows': 0, 'exit_rows': 0}
    for raw in text.splitlines(keepends=True):
        if not raw.endswith(('\n', '\r')):
            continue
        line = timer.strip_mpi_prefixes(raw)
        if re.search(r'\bNATIVE3D_QUALITY\s+', line):
            fields = dict(re.findall(r'(\w+)=([^\s]+)', line))
            if int(fields['rank']) == rank:
                current['join_seconds'] += float(fields['join_call_seconds'])
                current['windows'] += 1
                current['exit_rows'] += fields.get('stage') == 'exit'
        if re.search(r'\bNATIVE3D_TIMER_SUMMARY\s+', line):
            fields = dict(re.findall(r'(\w+)=([^\s]+)', line))
            if fields.get('root') == 'remesh' and fields.get('boundary') == 'final' and int(fields['rank']) == rank:
                legacy.append(current)
                current = {'join_seconds': 0., 'windows': 0, 'exit_rows': 0}
    assert len(roots) == len(legacy), (path, 'inconsistent complete-root parsing')
    return rank, roots, legacy


def assess_event(case, event):
    """Return None until all six terminal event roots exist; then apply each required rank gate."""
    results = []
    for path in sorted(Path(case).glob('rank_output/*/rank.*/stdout')):
        rank, roots, legacy = rank_events(path)
        if len(roots) < event:
            return None
        ledger = timer.assess(roots[event - 1])
        old = legacy[event - 1]
        join = sum(float(row['exclusive_seconds']) for row in ledger['rows'] if row['operation'] == 'join_wait')
        quality_gap = sum(float(row['exclusive_seconds']) for row in ledger['rows']
                          if row['phase'] == 'quality' and row['operation'] == 'unattributed')
        results.append({'rank': rank, 'ledger': ledger, 'legacy_join': old, 'join_wait_seconds': join,
                        'join_relative_difference': abs(join-old['join_seconds'])/old['join_seconds']
                        if old['join_seconds'] > 0. else None,
                        'join_within_one_percent': join_matches(join, old['join_seconds']),
                        'quality_unattributed_seconds': quality_gap,
                        'prediction_quality_gap_15_to_25': 15. <= quality_gap <= 25.,
                        'prediction_coverage_96p5_to_97': .965 <= float(ledger['summary']['coverage']) <= .97})
    if len(results) != 6:
        return None
    assert {row['rank'] for row in results} == set(range(6))
    passed = all(row['ledger']['passed'] and row['join_within_one_percent'] and
                 row['legacy_join']['windows'] > 0 and row['legacy_join']['exit_rows'] > 0 for row in results)
    return {'event': event, 'passed': passed, 'ranks': results,
            'scope': '95%coverage and1%join agreement are gates; reviewer ranges are predictions, reported without tuning.'}


def self_check():
    """The gate distinguishes exact, near, excessive, zero and nonfinite discrepancies."""
    timer.self_check()
    assert join_matches(100., 100.) and join_matches(100.5, 100.)
    assert not join_matches(102., 100.) and not join_matches(-1., 100.)
    assert join_matches(0., 0.) and not join_matches(1., 0.)
    assert not join_matches(float('nan'), 1.)
    with tempfile.TemporaryDirectory(prefix='f3b-event-parser-') as temporary:
        case = Path(temporary)
        for rank in range(6):
            path = case / f'rank_output/1/rank.{rank}/stdout'
            path.parent.mkdir(parents=True)
            events = []
            for join in (10., 20.):
                events.append(f'NATIVE3D_QUALITY rank={rank} stage=exit join_call_seconds={join}\n'
                              f'NATIVE3D_TIMER root=remesh boundary=final rank={rank} phase=quality '
                              f'operation=join_wait calls=1 inclusive_call_seconds={join} exclusive_seconds={join}\n'
                              f'NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank={rank} ranks=6 '
                              f'root_seconds={join} sumexclusive={join} unattributed=0 coverage=1 fixedstorage=4120\n')
            path.write_text(''.join(events))
        assert assess_event(case, 1)['passed'] and assess_event(case, 2)['passed']
        assert all(row['legacy_join']['join_seconds'] == 20. for row in assess_event(case, 2)['ranks'])
        missing = case / 'rank_output/1/rank.5/stdout'
        missing.write_text(missing.read_text().split('NATIVE3D_QUALITY', 2)[0])
        assert assess_event(case, 2) is None


if __name__ == '__main__':
    import sys
    self_check()
    if sys.argv[1:] != ['--self-check']:
        result = assess_event(Path(sys.argv[1]), int(sys.argv[2]))
        print(json.dumps(result, indent=2))
        raise SystemExit(2 if result is None else not result['passed'])
