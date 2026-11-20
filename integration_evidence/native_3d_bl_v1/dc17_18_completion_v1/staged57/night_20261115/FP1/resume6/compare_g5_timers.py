"""Report Part S coverage and construction scheduling rows from existing G5 logs."""

from argparse import ArgumentParser
from decimal import Decimal
from pathlib import Path
import json
import runpy

HERE = Path(__file__).resolve().parent
FP1 = HERE.parent
TIMER = runpy.run_path(str(FP1.parents[1] / 'f3b_20261115/check_timer_tree.py'))
BOUNDARIES = ('length_collapse_exit', 'construction_entry', 'final')
OPERATIONS = ('unattributed', 'scheduling')


def construction_rows(snapshot):
    summary = snapshot['summary']
    rows = {row['operation']: row for row in snapshot['rows'] if row['phase'] == 'construction'}
    out = {operation: {field: rows[operation][field] for field in ('calls', 'exclusive_seconds')}
           for operation in OPERATIONS if operation in rows}
    if summary['boundary'] == 'final':
        assessed = TIMER['assess'](snapshot)
        root = Decimal(summary['root_seconds'])
        named = sum(Decimal(row['exclusive_seconds']) for row in snapshot['rows'] if row['operation'] != 'unattributed')
        assert root > 0 and abs(named / root - Decimal(summary['coverage'])) < Decimal('1e-8')
        assert summary['fixedstorage'] == '4120'
        out.update(coverage=summary['coverage'], root_seconds=summary['root_seconds'],
                   unattributed_seconds=summary['unattributed'], coverage_from_rows=str(named / root),
                   root_residual_seconds=assessed['root_residual_seconds'], gates=assessed['gates'])
    return out


def read_rank(path, rank):
    """Use the first occurrence of each boundary in each event's cumulative ledger."""
    events = {}
    event, previous = 1, None
    for snapshot in TIMER['snapshots'](path.read_text(errors='replace')):
        summary = snapshot['summary']
        boundary = summary['boundary']
        if (summary['root'], summary['rank']) != ('remesh', str(rank)) or boundary not in BOUNDARIES:
            continue
        if snapshot == previous:
            continue
        previous = snapshot
        rows = events.setdefault(event, {})
        if boundary not in rows:
            rows[boundary] = construction_rows(snapshot)
        if boundary == 'final':
            event += 1
    return events


def read_case(case):
    if not case.is_dir():
        return {}
    return {rank: read_rank(path, rank) for rank in range(6)
            if (path := case / f'rank_output/1/rank.{rank}/stdout').is_file()}


def comparison():
    result = []
    for mode in ('ON1', 'ON2'):
        prior = FP1 / f'resume5/G5/{mode}/full615_run1'
        current = HERE / f'G5/{mode}/full616_run1'
        before, after = map(read_case, (prior, current))
        accepted = (current.parent / 'run_receipt.json').is_file()
        for event in (1, 2):
            for rank in range(6):
                old, new = (case.get(rank, {}).get(event, {}) for case in (before, after))
                if accepted:
                    assert all(boundary in new and new[boundary].keys() >= set(OPERATIONS) for boundary in BOUNDARIES)
                    assert all(new['final']['gates'].values())
                result.append({'mode': mode, 'event': event, 'rank': rank, 'build615': old, 'build616': new})
    return result


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    report = comparison()
    by_key = {(row['mode'], row['event'], row['rank']): row for row in report}
    failing = by_key['ON2', 2, 0]['build615']['final']
    assert failing['coverage'] == '0.9484167186729866'
    assert not failing['gates']['named_coverage_at_least_95_percent']
    assert by_key['ON1', 2, 0]['build615']['final']['coverage'] == '0.95110102993614232'
    assert len(report) == 24
    if args.self_check:
        print('Archived build615 ON1 and ON2 snapshots reproduced both rank-0 event-2 coverage values.')
        return
    output = HERE / 'G5/timer_comparison.json'
    with output.open('x') as stream:
        json.dump({'phase': 'construction', 'seconds': 'exclusive', 'rows': report}, stream, indent=2)
        stream.write('\n')
    print(output)


if __name__ == '__main__':
    main()
