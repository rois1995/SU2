"""Check cumulative per-rank F3 snapshots; never add snapshots or inclusive call times."""
import json
import re
import sys
from pathlib import Path

STAGED = next(parent for parent in Path(__file__).resolve().parents if parent.name == "staged57")
sys.path.insert(0, str(STAGED / "nobl_clean_check_20261114"))
from extract_markers import strip_mpi_prefixes


def snapshots(text):
    """Pair local operation rows with their following root summary, retaining repeated boundaries."""
    pending = {}
    result = []
    for line in text.splitlines(keepends=True):
        if not line.endswith(('\n', '\r')):
            continue  # A live MPI stream may end partway through a record.
        match = re.search(r'\b(NATIVE3D_TIMER(?:_SUMMARY)?)\s+(.*)', strip_mpi_prefixes(line))
        if not match:
            continue
        fields = dict(re.findall(r'(\w+)=([^\s]+)', match[2]))
        key = tuple(fields[name] for name in ('root', 'boundary', 'rank'))
        if match[1] == 'NATIVE3D_TIMER':
            pending.setdefault(key, []).append(fields)
        else:
            result.append({'summary': fields, 'rows': pending.pop(key, [])})
    return result


def assess(snapshot):
    """Require telescoping nonnegative rows and coverage from named exclusive time only."""
    summary = snapshot['summary']
    root = float(summary['root_seconds'])
    epsilon = 1e-8 * max(1., root)
    rows = snapshot['rows']
    named = sum(float(row['exclusive_seconds']) for row in rows if row['operation'] != 'unattributed')
    all_rows = sum(float(row['exclusive_seconds']) for row in rows)
    keys = [(row['phase'], row['operation']) for row in rows]
    gates = {
        'positive_root': root > 0.,
        'unique_operation_rows': len(keys) == len(set(keys)),
        'exclusive_total_telescopes': abs(float(summary['sumexclusive']) - root) <= epsilon,
        'nonnegative_root_residual': root - all_rows >= -epsilon,
        'named_rows_match_summary': abs(named + float(summary['unattributed']) - root) <= epsilon,
        'coverage_matches_rows': abs(named / root - float(summary['coverage'])) <= 1e-8 if root > 0 else False,
        'valid_operation_records': all(int(row['calls']) > 0 and float(row['exclusive_seconds']) >= 0. and
            float(row['inclusive_call_seconds']) + epsilon >= float(row['exclusive_seconds']) for row in rows),
        'named_coverage_at_least_95_percent': root > 0 and named / root >= .95,
    }
    return dict(snapshot, gates=gates, passed=all(gates.values()), named_seconds=named,
                root_residual_seconds=root-all_rows)


def self_check():
    """A correct small snapshot passes; absent live rows and double-counted totals fail."""
    text = ('NATIVE3D_TIMER root=n boundary=final rank=0 phase=metric operation=census calls=1 '
            'inclusive_call_seconds=10 exclusive_seconds=9.6\n'
            'NATIVE3D_TIMER_SUMMARY root=n boundary=final rank=0 ranks=1 root_seconds=10 '
            'sumexclusive=10 unattributed=0.4 coverage=0.96 fixedstorage=1\n')
    parsed = snapshots(text)
    assert len(parsed) == 1 and assess(parsed[0])['passed']
    assert not assess(dict(parsed[0], rows=[]))['passed']
    bad = dict(parsed[0], summary=dict(parsed[0]['summary'], sumexclusive='11'))
    assert not assess(bad)['passed']
    assert len(snapshots(text + text)) == 2
    assert snapshots(text.rstrip('\n')) == []
    assert len(snapshots(text + 'NATIVE3D_TIMER_SUMMARY root=remesh boundary=fi')) == 1
    decorated = text.replace('exclusive_seconds=9.6',
                             'exclusive_seconds=9.Sun Nov 15 15:00:00 2026<stdout>:6')
    assert assess(snapshots(decorated)[0])['passed']


def main(paths, boundary):
    """Save every requested snapshot and require all six ranks when MPI output is supplied."""
    records = [dict(assess(item), source=str(path)) for path in paths
               for item in snapshots(path.read_text(errors='replace')) if item['summary']['boundary'] == boundary]
    assert records, 'No requested timer snapshots; a missing report fails.'
    counts = {}
    for item in records:
        row = item['summary']
        key = (row['root'], int(row['ranks']))
        ranks = counts.setdefault(key, {})
        rank = int(row['rank'])
        ranks[rank] = ranks.get(rank, 0) + 1
    rank_coverage = [{'root': name, 'ranks': ranks, 'counts': found,
                     'passed': set(found) == set(range(ranks)) and len(set(found.values())) == 1}
                    for (name, ranks), found in counts.items()]
    result = {'boundary': boundary, 'records': records, 'rank_coverage': rank_coverage,
              'passed': all(row['passed'] for row in records) and all(row['passed'] for row in rank_coverage),
              'scope': 'Per-rank cumulative wall time. Inclusive totals and repeated snapshots are nonadditive.'}
    print(json.dumps(result, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    self_check()
    if sys.argv[1:] != ['--self-check']:
        raise SystemExit(main([Path(p) for p in sys.argv[2:]], sys.argv[1]))
