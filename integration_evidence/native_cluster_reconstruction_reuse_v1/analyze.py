"""Compare unchanged reconstruction work, metric sampling and full remesh costs."""
import importlib.util
from pathlib import Path

SHARED = Path(__file__).resolve().parents[2] / 'integration_evidence/native_cluster_balance_profile_v1'
spec = importlib.util.spec_from_file_location('reuse_analysis', SHARED / 'analyze.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
profile = base.profile
ACTIONS = base.ACTIONS


def compare(rows):
    pairs = base.compare(rows)
    by_name = {row['case']:row for row in rows}
    for pair in pairs:
        old, new = by_name[pair['control']], by_name[pair['profile']]
        assert len(old['balance']) == len(new['balance']) == old['workers']
        unchanged = all(a[key] == b[key]
                        for left, right in zip(old['balance'], new['balance'])
                        for a, b in zip(left['operations'], right['operations'])
                        for key in ('rank','workers','action','selected','attempts','reconstructed','committed','cells','max_cells','selection_scans'))
        assert unchanged, 'Candidate changed reconstruction work: '+pair['profile']
        pair['operation_counts_identical'] = unchanged
        pair['remesh_change_percent'] = pair.pop('instrumentation_change_percent')
        def totals(row):
            result = {key:sum(op[key] for rank in row['balance'] for op in rank['operations'])
                      for key in ('requests','evaluations','evictions','private_wall_seconds','private_cpu_seconds')}
            result['private_wall_max_seconds'] = max(rank['private_wall_seconds'] for rank in row['balance'])
            result['longest_private_seconds'] = max(rank['longest_private_seconds'] for rank in row['balance'])
            return result
        pair['control_costs'], pair['candidate_costs'] = totals(old), totals(new)
        pair['cost_change_percent'] = {key:100*(pair['candidate_costs'][key]/value-1) if value else None
                                       for key,value in pair['control_costs'].items()}
    return pairs
