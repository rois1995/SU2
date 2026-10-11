"""Verify retained insertion receipts and publish trajectory-aware accounting, without remeshing reruns."""
from pathlib import Path
import csv,hashlib,json,statistics
from audit_lifecycle import parse,closes,ENGINE
HERE=Path(__file__).resolve().parent
CASES=('insertion_driver_n1_v1','insertion_driver_n2_m1_v1','insertion_driver_n2_m2_v1',
       'insertion_driver_n4_m1_v1','insertion_driver_n4_m2_v1','insertion_driver_n4_m4_v1','insertion_aligned_n3_n1_r1024_v1','insertion_aligned_n3_n4_m4_v1','insertion_aligned_n3_n4_m2_v1')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def qualify(name,drift_keys):
    folder=HERE/name;receipt=json.loads((folder/'validation.json').read_text())
    assert receipt['status']=='PASS' and all(s['exit_code']==0 for s in receipt['stages']),name
    assert all(not receipt[k] for k in drift_keys),name
    for path,digest in receipt['artifact_sha256'].items():assert sha(folder/path)==digest,(name,path)
    return receipt

def qualify_failure(name,stage):
    folder=HERE/name;receipt=json.loads((folder/'validation.json').read_text());assert receipt['status']=='FAIL'
    assert [(s['stage'],s['exit_code']) for s in receipt['stages'] if s['exit_code']]==[(stage,1)]
    for key in ('changed_during_run','archive_changes','object_changes','source_drift','object_drift','archive_drift'):
        assert not receipt.get(key,[]),name
    for path,digest in receipt['artifact_sha256'].items():assert sha(folder/path)==digest,(name,path)
    return receipt

def main():
    cases=[];samples=[];replays=[];baseline=None
    for name in CASES+('insertion_replay_original_n3_v1',):
        receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        production=(receipt['library_sha256'],receipt['thin_archive_member_sha256'])
        if baseline is None:baseline=production
        else:assert baseline==production,'Production changed between cases'
        folder=HERE/name;audit=json.loads((folder/'independent_audit.json').read_text());assert audit['status']=='PASS'
        if name.startswith('insertion_replay'):
            assert not audit['production_equal_to_rejection'] and not audit['replay_result']['complete']
            assert audit['replay_result']['inserted']>0
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
    failed=qualify_failure('insertion_aligned_n3_n1_v1','case')
    qualified=HERE/'insertion_aligned_n3_n1_r1024_v1';partial=HERE/'insertion_aligned_n3_n1_v1'
    common=('envelope_input.su2','envelope_s0_initial.json','envelope_s0.json','envelope_s0_states.json',
            'envelope_s1_initial.json','envelope_mesh_00001.su2','envelope_mesh_00001.su2.native_ref')
    equal={name:sha(partial/name) for name in common if (partial/name).read_bytes()==(qualified/name).read_bytes()}
    equal_common=equal
    assert len(equal)==len(common),'Common512/1024 n3 input, first accepted mesh/state or second input differs'
    assert (failed['library_sha256'],failed['thin_archive_member_sha256'])==baseline
    compile_failure=qualify_failure('insertion_regression_v1','compile_full_configured_driver')
    assert 'operator|| is not supported' in (HERE/'insertion_regression_v1/compile_full_configured_driver.stdout').read_text()
    auto=qualify('insertion_auto_audit_v1',())
    auto_source=json.loads((HERE/'insertion_regression_v2/validation.json').read_text())
    assert auto_source['status']=='PASS'
    independent_frames=0
    for ranks in (1,2,4):
        record=json.loads((HERE/f'insertion_auto_audit_v1/mpi{ranks}/independent_audit.json').read_text())
        assert record['status']=='PASS';independent_frames+=len(record['frames'])
    assert independent_frames==324
    regression=qualify('insertion_regression_v3',('source_drift','object_drift','archive_drift'))
    assert len(regression['actual_objects_sha256'])==68
    assert all(regression['library_sha256'].get(p)==h for p,h in baseline[0].items())
    assert all(regression['thin_archive_member_sha256'].get(p)==h for p,h in baseline[1].items())
    assert all(regression['library_sha256'].get(p)==h for p,h in auto_source['library_sha256'].items())
    assert regression['thin_archive_member_sha256']==auto_source['thin_archive_member_sha256']
    legacy_files=('envelope_s0_initial.json','envelope_s0.json','envelope_s1_initial.json','envelope_s1.json',
                  'envelope_s2_initial.json','envelope_s2.json','envelope_s2_written_SU2_reload.json',
                  'envelope_s2_written_CGNS_reload.json','envelope_s0_states.json','envelope_s1_states.json','envelope_s2_states.json')
    unchanged={}
    for name in CASES[:6]:
        old=HERE/name.replace('insertion_driver_','coordination_driver_')
        equal=[file for file in legacy_files if (HERE/name/file).read_bytes()==(old/file).read_bytes()]
        assert len(equal)==len(legacy_files),(name,equal)
        unchanged[name]=equal
    keys=['case','record','stage','rank','ranks']+sorted({k for r in samples for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'insertion_rank_phases.csv').open('w',newline='') as f:
        out=csv.DictWriter(f,fieldnames=keys);out.writeheader();out.writerows(samples)
    result=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),matrix_sha256=sha(HERE/'INSERTION_MATRIX.json'),
                cases=cases,rejected_private_replays=replays,regression_validation_sha256=sha(HERE/'insertion_regression_v3/validation.json'),
                retained_compile_failure_sha256=sha(HERE/'insertion_regression_v1/validation.json'),
                retained_default_n3_failure_sha256=sha(partial/'validation.json'),
                qualified_default_first_event_equal_to_1024=equal_common,legacy_unchanged_data_files=unchanged,
                automatic_independent_frames=independent_frames,automatic_audit_validation_sha256=sha(HERE/'insertion_auto_audit_v1/validation.json'),
                accounting_rows=len(samples),csv_sha256=sha(HERE/'insertion_rank_phases.csv'),
                scope='Six original n2 default-budget N/M manufactured lifecycle controls with66 byte-identical geometry/state data files; three larger aligned n3 lifecycle controls: default N4M4/N4M2, diagnostic explicit-adapter1024 N1. Actual return/replacement/current+BDF2/SU2+CGNS, frozen geometry/residual checks; configured MPI1/2/4 insertion/thin/junction/rollback/repair/budget/default/native2D/MMG and324 independently audited automatic repair frames. Same production identities. Original n3 rejection preserved; default aligned N1 fails second coarsening at512, first accepted event independently qualified by byte identity with the passed1024 case. Single contended pilots/different trajectories are not scaling or speedup evidence. Full Goal1 remains active.')
    (HERE/'INSERTION_CHECKPOINT_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(cases),'actual-driver cases;',len(replays),'qualified rejections;',len(samples),'accounting rows')
if __name__=='__main__':main()
