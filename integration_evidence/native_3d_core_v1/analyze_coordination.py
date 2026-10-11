"""Verify retained coordination receipts and publish trajectory-aware accounting, without remeshing reruns."""
from pathlib import Path
import csv,hashlib,json,statistics
from audit_lifecycle import parse,closes,ENGINE
HERE=Path(__file__).resolve().parent
CASES=('coordination_driver_n1_v1','coordination_driver_n2_m1_v1','coordination_driver_n2_m2_v1',
       'coordination_driver_n4_m1_v1','coordination_driver_n4_m2_v1','coordination_driver_n4_m4_v1')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def qualify(name,drift_keys):
    folder=HERE/name;receipt=json.loads((folder/'validation.json').read_text())
    assert receipt['status']=='PASS' and all(s['exit_code']==0 for s in receipt['stages']),name
    assert all(not receipt[k] for k in drift_keys),name
    for path,digest in receipt['artifact_sha256'].items():assert sha(folder/path)==digest,(name,path)
    return receipt

def main():
    cases=[];samples=[];replays=[];baseline=None
    for name in CASES+('coordination_replay_r512_v1','coordination_replay_r1024_v1'):
        receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        production=(receipt['library_sha256'],receipt['thin_archive_member_sha256'])
        if baseline is None:baseline=production
        else:assert baseline==production,'Production changed between cases'
        folder=HERE/name;audit=json.loads((folder/'independent_audit.json').read_text());assert audit['status']=='PASS'
        if name.startswith('coordination_replay'):
            assert not audit['production_equal_to_rejection'] and not audit['replay_result']['complete']
            replays.append(dict(folder=name,result=audit['replay_result'],private_progress=audit['private_progress'],
                                oversized_edges=len(audit['oversized_edges']),oversized_cells=audit['oversized_cells'],
                                validation_sha256=sha(folder/'validation.json'),audit_sha256=sha(folder/'independent_audit.json')))
            continue
        details=[]
        for line in (folder/'case.stdout').read_text().splitlines():
            if not line.startswith('NATIVE3D_DETAIL '):continue
            row=parse(line)
            if row['stage'] in ('refine','coarsen'):closes(row,ENGINE)
            elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'))
            elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
            else:assert row['stage']=='source'
            details.append(row)
        samples.extend(dict(case=name,record='exclusive',**r) for r in audit['rank_stages'])
        samples.extend(dict(case=name,record='nested',**r) for r in details)
        cycles=[]
        for cycle in range(3):
            parents=[r for r in audit['rank_rows'] if r['cycle']==cycle]
            # MPI output may interleave; native_rows is rank-local event order, not global order.
            native=[next(r for i,r in enumerate(x for x in audit['native_rows'] if x['rank']==rank) if i==cycle)
                    for rank in range(audit['parameters']['ranks'])]
            wall=max(r['parent_wall'] for r in parents);operations=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            cycles.append(dict(cycle=cycle,parent_max_seconds=wall,remesh_max_seconds=max(r['total'] for r in native),
                               parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in parents),
                               useful_accepted_operations=operations,useful_operations_per_parent_second=operations/wall,
                               maximum_native_bound=max(r['native_bound'] for r in native),
                               maximum_lifetime_rss=max(r['process_peak_rss'] for r in native)))
        frames=[r for r in audit['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        assert len(frames)==3 and len(audit['states'])==3
        cases.append(dict(folder=name,parameters=audit['parameters'],accepted_cells=[r['cells'] for r in frames],
                          minimum_mean_ratio=min(r['minimum_mean_ratio'] for r in frames),
                          minimum_scaled_jacobian=min(r['minimum_scaled_jacobian'] for r in frames),
                          maximum_metric_edge=max(r['maximum_length'] for r in frames),
                          maximum_state_error=max(r['maximum_state_relative_error'] for r in audit['states']),
                          maximum_transported_tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in audit['states']),
                          validation_sha256=sha(folder/'validation.json'),audit_sha256=sha(folder/'independent_audit.json'),cycles=cycles))
    assert (HERE/'coordination_replay_r512_v1/replay_rejected.json').read_bytes()==(HERE/'coordination_replay_r1024_v1/replay_rejected.json').read_bytes()
    regression=qualify('coordination_regression_v1',('source_drift','object_drift','archive_drift'))
    assert len(regression['actual_objects_sha256'])==68
    assert all(regression['library_sha256'].get(p)==h for p,h in baseline[0].items())
    assert all(regression['thin_archive_member_sha256'].get(p)==h for p,h in baseline[1].items())
    placement=HERE/'placement_certificate_v2';cert=json.loads((placement/'placement_certificates.json').read_text())
    assert cert['status']=='PASS' and cert['source_audit_sha256']==sha(HERE/'coordination_replay_r1024_v1/independent_audit.json')
    assert cert['source_initial_mesh_sha256']==sha(HERE/'coordination_replay_r1024_v1/replay_initial.json')
    assert cert['source_private_mesh_sha256']==sha(HERE/'coordination_replay_r1024_v1/replay_rejected.json')
    assert cert['auditor_sha256']==sha(HERE/'audit_placement.py')==sha(placement/'audit_placement.py')
    keys=['case','record','stage','rank','ranks']+sorted({k for r in samples for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'coordination_rank_phases.csv').open('w',newline='') as f:
        out=csv.DictWriter(f,fieldnames=keys);out.writeheader();out.writerows(samples)
    result=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),matrix_sha256=sha(HERE/'COORDINATION_MATRIX.json'),
                cases=cases,rejected_private_replays=replays,regression_validation_sha256=sha(HERE/'coordination_regression_v1/validation.json'),
                placement_certificate_sha256=sha(placement/'placement_certificates.json'),
                accounting_rows=len(samples),csv_sha256=sha(HERE/'coordination_rank_phases.csv'),
                scope='Six actual N/M manufactured backend/return/replace/current+BDF2/SU2+CGNS three-event controls, frozen geometry/residual audits and configured MPI1/2/4 repair/coarsening/budget/default/2D/MMG assertions. Same production identities; varied partitions can change trajectories. Two rejected n3 private replays and exact binary64 placement/marker-junction diagnosis. Single contended pilots, parent includes diagnostics; output separate. No scaling, overhead, private-wait/complete-traffic or CFD affordability certificate. Full Goal1 remains active.')
    (HERE/'COORDINATION_CHECKPOINT_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(cases),'actual-driver cases;',len(replays),'qualified rejections;',len(samples),'accounting rows')
if __name__=='__main__':main()
