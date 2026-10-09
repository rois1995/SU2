"""Check bounded balance records and paired frozen remeshing results."""
import csv
import math
from pathlib import Path

ACTIONS = ('height', 'split', 'remove', 'redistribute', 'bulk_remove', 'bulk_split', 'bulk_flip', 'bulk_move')


def profile(case, workers):
    result = []
    for rank in range(workers):
        prefix = case / ('native_balance_1_rank_' + str(rank))
        with Path(str(prefix) + '_operations.csv').open() as handle:
            rows = list(csv.DictReader(handle))
        with Path(str(prefix) + '_hotspots.csv').open() as handle:
            hotspots = list(csv.DictReader(handle))
        assert len(rows) == 8 and len(hotspots) <= 32
        for i, row in enumerate(rows):
            assert int(row['rank']) == rank and int(row['workers']) == workers and int(row['action']) == i
            assert all(math.isfinite(float(v)) and float(v) >= 0 for v in row.values())
            assert int(row['committed']) <= int(row['reconstructed']) <= int(row['attempts']) <= int(row['selected'])
            assert int(row['cpu_samples']) <= int(row['attempts'])
            assert int(row['max_cells']) <= int(row['cells'])
        for row in hotspots:
            assert int(row['rank']) == rank and int(row['workers']) == workers and 0 <= int(row['action']) < 8
            assert all(math.isfinite(float(v)) for v in row.values())
            assert float(row['private_wall_seconds']) >= 0
            assert float(row['private_cpu_seconds']) == -1 or float(row['private_cpu_seconds']) >= 0
            assert all(int(row[k])>=0 for k in ('round','seed_a','seed_b','cells','requests','evaluations','evictions'))
            assert int(row['committed']) in (0,1) and int(row['reconstructed']) in (0,1) and int(row['coordinated']) in (0,1)
            assert int(row['committed']) <= int(row['reconstructed'])
            assert int(row['boundary_cells']) <= int(row['cells']) and int(row['boundary_cells'])>=0
            assert float(row['xmin']) <= float(row['xmax']) and float(row['ymin']) <= float(row['ymax'])
        operations = [dict(name=ACTIONS[i], **{k: float(v) for k, v in row.items()}) for i,row in enumerate(rows)]
        attempts=sum(int(row['attempts']) for row in rows)
        assert len(hotspots)==min(32,attempts), 'Incomplete bounded hotspot records'
        total = sum(row['private_wall_seconds'] for row in operations)
        longest = max(row['longest_seconds'] for row in operations)
        hotspot_wall = sum(float(row['private_wall_seconds']) for row in hotspots)
        assert hotspot_wall <= total + 1e-9 * max(1, total)
        assert len({row['round'] for row in hotspots}) == len(hotspots)
        if hotspots:
            assert abs(max(float(row['private_wall_seconds']) for row in hotspots) - longest) <= 1e-12 * max(1, longest)
        result.append(dict(rank=rank, operations=operations, hotspots=hotspots,
                           private_wall_seconds=total,
                           private_process_cpu_seconds=sum(row['private_cpu_seconds'] for row in operations),
                           cpu_samples=sum(int(row['cpu_samples']) for row in operations),
                           attempts=sum(int(row['attempts']) for row in operations),
                           longest_private_seconds=longest, hotspot_wall_fraction=hotspot_wall/total if total else 0))
    return result


def compare(rows):
    pairs = []
    for new in (r for r in rows if r['role'] == 'profile'):
        old = next(r for r in rows if r['role'] == 'control' and
                   (r['kind'], r['workers'], r['repartition'], r['repeat']) ==
                   (new['kind'], new['workers'], new['repartition'], new['repeat']))
        keys = ('native_frozen_adapted.su2',) + tuple('native_frozen_target_rank_' + str(i) + '.csv' for i in range(4))
        identical = all(new['output_sha256'].get(k) == old['output_sha256'].get(k) and k in old['output_sha256'] for k in keys)
        assert new['status']==old['status']=='PASS'
        assert math.isfinite(old['remesh_seconds']) and old['remesh_seconds']>0
        assert math.isfinite(new['remesh_seconds']) and new['remesh_seconds']>0
        pairs.append(dict(control=old['case'], profile=new['case'], numerical_outputs_identical=identical,
                          control_remesh_seconds=old['remesh_seconds'], profile_remesh_seconds=new['remesh_seconds'],
                          instrumentation_change_percent=100*(new['remesh_seconds']/old['remesh_seconds']-1)))
    assert len(pairs) * 2 == len(rows)
    return pairs
