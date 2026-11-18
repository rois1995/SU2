"""Read the sealed quality-round outputs and compute the specified purpose gate."""
from collections import defaultdict
import csv
from math import fsum
from pathlib import Path
import sys

from event_prefix import marked_rows
from run_scoped import N16, cut

OPERATIONS = ('join_wait', 'communication', 'clearance', 'census', 'scheduling')
COUNTERS = ('collision_builds', 'collision_checks', 'conflicts', 'proposal_duplicates', 'batch_candidates',
            'batch_reserved')
CERTIFICATION = ('cells', 'measured', 'q_measured', 'j_measured', 'q_bad', 'j_bad', 'size_bad', 'geometry_bad')


def private_ledgers(log, rank):
    """Group records by the two adaptation partition events for this rank."""
    events = []
    for tag, fields, _ in marked_rows(log, rank):
        if tag == 'NATIVE3D_PARTITION':
            events.append(defaultdict(list))
        if events:
            events[-1][tag].append(fields)
    assert len(events) == 2, (log, rank, len(events))
    return events


def rank_ledgers(case, name, require_choices=True):
    receipt = cut.read(case / 'full_workflow_receipt.json')
    assert receipt['workflow_accepted'] and not receipt['errors']
    assert [row['event'] for row in receipt['events']] == [1, 2]
    rank_events = [private_ledgers(case / f'rank_output/1/rank.{rank}/stdout', rank) for rank in range(6)]
    records = []
    for event in receipt['events']:
        number = event['event']
        timer = cut.read(case / f'f3b_event{number}_timer_gate.json')
        assert timer['passed']
        for rank in range(6):
            tags = rank_events[rank][number - 1]
            detail, = (item for item in event['refine_details'] if int(item['rank']) == rank)
            native, = (item for item in event['native_summaries'] if int(item['rank']) == rank)
            ledger, = (row['ledger'] for row in timer['ranks'] if int(row['rank']) == rank)
            private = tags['NATIVE3D_PRIVATE_WORK']
            assert private and len({row['round'] for row in private}) == len(private)
            quality = tags['NATIVE3D_QUALITY']
            quality_exit = [row for row in quality if row.get('stage') == 'exit'][-1]
            end_rows = [row for row in tags['NATIVE3D_STAGE'] if row.get('stage') == 'exit' and
                        all(key in row for key in CERTIFICATION)]
            assert end_rows
            storage = [int(row['native_peak_bytes']) for row in tags['NATIVE3D_STORAGE']
                       if 'native_peak_bytes' in row]
            row = {'run': name, 'event': number, 'rank': rank, 'root_seconds': float(ledger['summary']['root_seconds']),
                   'quality_rounds': len(private), 'quality_commits': int(quality_exit['commits_total']),
                   'quality_trials': sum(int(item['trials']) for item in tags['NATIVE3D_QUALITY_LADDER']),
                   'private_work_used': sum(int(item['private_work_used']) for item in private),
                   'private_busy_seconds': fsum(float(item['busy_seconds']) for item in private)}
            if require_choices:
                row['largest_round_choices'] = max(int(item['choices']) for item in private)
            for op in OPERATIONS:
                row[f'quality_{op}_seconds'] = fsum(float(item['exclusive_seconds']) for item in ledger['rows']
                    if item['phase'] == 'quality' and item['operation'] == op)
            row['quality_purpose_seconds'] = fsum(row[f'quality_{op}_seconds'] for op in OPERATIONS[:3])
            row.update({key: int(detail[key]) for key in COUNTERS})
            row['quality_windows'] = sum(item.get('stage') == 'window' for item in quality)
            row['quality_ownership_migrations'] = sum(int(item['ownership_changes']) for item in quality)
            row['process_peak_bytes'] = int(native['process_peak_rss'])
            row['modeled_peak_bytes'] = int(native['native_bound'])
            row['engine_modeled_peak_bytes'] = max(storage)
            row.update({f'final_{key}': int(end_rows[-1][key]) for key in CERTIFICATION})
            row.update(final_mesh_points='', final_mesh_cells='')
            records.append(row)
        these_ranks = records[-6:]
        sums = {key: fsum(row[key] for row in these_ranks) if isinstance(value, float)
                else sum(row[key] for row in these_ranks) for key, value in these_ranks[0].items()
                if key not in {'run', 'event', 'rank', 'final_mesh_points', 'final_mesh_cells', 'largest_round_choices'}}
        sums.update(run=name, event=number, rank='sum', final_mesh_points=event['actual_mesh_counts']['points'],
                    final_mesh_cells=event['actual_mesh_counts']['cells'])
        if require_choices:
            sums['largest_round_choices'] = max(row['largest_round_choices'] for row in these_ranks)
        for key, expected in event['certification_six_rank_sums'].items():
            assert sums[f'final_{key}'] == expected, (name, number, key, sums[f'final_{key}'], expected)
        records.append(sums)
    return records


def purpose(reference, candidate):
    results = []
    for ref, run in zip((r for r in reference if r['rank'] == 'sum'),
                        (r for r in candidate if r['rank'] == 'sum'), strict=True):
        first, after = ref['quality_purpose_seconds'], run['quality_purpose_seconds']
        assert ref['event'] == run['event'] and first > 0
        results.append({'event': ref['event'], 'T0_seconds': first, 'run1_seconds': after,
                        'limit_seconds': 0.80 * first, 'ratio': after / first, 'passed': after <= 0.80 * first})
    assert len(results) == 2
    return {'passed': all(row['passed'] for row in results), 'events': results}


def write_csv(path, rows):
    with path.open('x', newline='') as file:
        writer = csv.DictWriter(file, rows[0], lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)


if __name__ == '__main__':
    case = Path(sys.argv[1])
    rows = rank_ledgers(case, 'baseline self-check', require_choices=False)
    for row in rows:
        if row['rank'] == 'sum':
            print(row)
