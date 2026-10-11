"""Bind donor/trace fast-path evidence to unchanged fixed-partition mesh/state data."""
from pathlib import Path
import csv,json,statistics
from analyze_insertion import qualify,qualify_failure,sha,HERE,parse,closes,ENGINE
CASES=('donor_reuse_aligned_n3_n1_v2','donor_reuse_driver_n1_v1',
       'donor_reuse_aligned_n3_n4_m2_v1','donor_reuse_aligned_n3_n4_m4_v1')
FILES=('envelope_s0_initial.json','envelope_s0.json','envelope_s1_initial.json','envelope_s1.json',
       'envelope_s2_initial.json','envelope_s2.json','envelope_s2_written_SU2_reload.json',
       'envelope_s2_written_CGNS_reload.json','envelope_s0_states.json','envelope_s1_states.json','envelope_s2_states.json')
def main():
    regression=qualify('donor_reuse_regression_v4',('source_drift','object_drift','archive_drift'))
    assert len(regression['actual_objects_sha256'])==68
    assert any(s['stage']=='mmg_serial' for s in regression['stages'])
    for name in ('mmg_serial','mpi1','mpi2','mpi4'):
        assert 'No test cases matched' not in (HERE/'donor_reuse_regression_v4'/(name+'.stdout')).read_text()
    samples=[];cases=[];production=None
    for name in CASES+('donor_reuse_replay_original_n3_v1',):
        receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        assert all(regression['source_sha256'][k]==v for k,v in receipt['source_sha256'].items() if k in regression['source_sha256'])
        pin=(receipt['library_sha256'],receipt['thin_archive_member_sha256'])
        if production is None:production=pin
        else:assert production==pin
        assert all(regression['library_sha256'].get(k)==v for k,v in pin[0].items())
        assert all(regression['thin_archive_member_sha256'].get(k)==v for k,v in pin[1].items())
        folder=HERE/name;audit=json.loads((folder/'independent_audit.json').read_text());assert audit['status']=='PASS'
        if 'replay' in name:
            assert not audit['replay_result']['complete'] and audit['replay_result']['accepted_state_preserved']
            baseline=HERE/'insertion_replay_original_n3_v1'
            equal={p:sha(folder/p) for p in ('replay_initial.json','replay_rejected.json','replay_initial.su2','replay_rejected.su2')
                   if (folder/p).read_bytes()==(baseline/p).read_bytes()}
            assert len(equal)==4
            replay=dict(result=audit['replay_result'],unchanged=equal,validation_sha256=sha(folder/'validation.json'))
            continue
        baseline=HERE/('coarsen_filter_aligned_n3_n1_v1' if name.endswith('n3_n1_v2') else name.replace('donor_reuse_','coarsen_filter_'))
        unchanged={p:sha(folder/p) for p in FILES if (folder/p).read_bytes()==(baseline/p).read_bytes()}
        assert len(unchanged)==len(FILES),(name,'fixed-work data changed')
        assert audit['parameters']['diagnostic_rounds']==512
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
        operations=[r for r in details if r['stage'] in ('refine','coarsen')]
        assert sum(r['donor_reused'] for r in operations)>0
        assert sum(r['edge_separated'] for r in operations)>0
        cycles=[]
        for cycle in range(3):
            parents=[r for r in audit['rank_rows'] if r['cycle']==cycle]
            native=[next(r for i,r in enumerate(x for x in audit['native_rows'] if x['rank']==rank) if i==cycle)
                    for rank in range(audit['parameters']['ranks'])]
            wall=max(r['parent_wall'] for r in parents);ops=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            cycles.append(dict(cycle=cycle,parent_max_seconds=wall,remesh_max_seconds=max(r['total'] for r in native),
                parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in parents),accepted_operations=ops,
                accepted_operations_per_parent_second=ops/wall,native_bound=max(r['native_bound'] for r in native),
                lifetime_rss=max(r['process_peak_rss'] for r in native)))
        frames=[r for r in audit['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        assert len(frames)==3 and len(audit['states'])==3
        cases.append(dict(folder=name,unchanged=unchanged,cells=[r['cells'] for r in frames],
            minimum_q=min(r['minimum_mean_ratio'] for r in frames),minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),
            maximum_L=max(r['maximum_length'] for r in frames),state_error=max(r['maximum_state_relative_error'] for r in audit['states']),
            transported_metric_error=max(r['maximum_transported_tensor_absolute_error'] for r in audit['states']),
            donor_reused=sum(r['donor_reused'] for r in operations),edge_separated=sum(r['edge_separated'] for r in operations),
            validation_sha256=sha(folder/'validation.json'),cycles=cycles))
    failed_mmg=qualify_failure('donor_reuse_regression_v3','mpi2')
    assert 'MMG2D_Set_tensorSols' in (HERE/'donor_reuse_regression_v3/mpi2.stderr').read_text()
    assert failed_mmg['library_sha256']==regression['library_sha256']
    historical_probe=qualify('segment_prefilter_probe_v2',())
    probe=qualify('segment_prefilter_probe_v3',());
    assert probe['pins']['Common/src/adaptation/CNativePredicates3D.cpp']==regression['source_sha256']['Common/src/adaptation/CNativePredicates3D.cpp']
    auto=qualify('donor_reuse_auto_audit_v1',());assert auto['source_folder']==str(HERE/'donor_reuse_regression_v4')
    count=0
    for ranks in (1,2,4):
        audit=json.loads((HERE/f'donor_reuse_auto_audit_v1/mpi{ranks}/independent_audit.json').read_text())
        assert audit['status']=='PASS';count+=len(audit['frames'])
    assert count==324
    keys=['case','record','stage','rank','ranks']+sorted({k for r in samples for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'donor_reuse_rank_phases.csv').open('w',newline='') as f:
        out=csv.DictWriter(f,fieldnames=keys);out.writeheader();out.writerows(samples)
    result=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={p:sha(HERE/p) for p in ('analyze_insertion.py','audit_lifecycle.py')},
        matrix_sha256=sha(HERE/'DONOR_REUSE_MATRIX.json'),cases=cases,replay=replay,automatic_frames=count,probe_validation_sha256=sha(HERE/'segment_prefilter_probe_v3/validation.json'),
        regression_validation_sha256=sha(HERE/'donor_reuse_regression_v4/validation.json'),
        retained_mmg_fixture_failure_sha256=sha(HERE/'donor_reuse_regression_v3/validation.json'),
        superseded_exact_sign_probe_sha256=sha(HERE/'segment_prefilter_probe_v2/validation.json'),
        auto_validation_sha256=sha(HERE/'donor_reuse_auto_audit_v1/validation.json'),accounting_rows=len(samples),
        csv_sha256=sha(HERE/'donor_reuse_rank_phases.csv'),
        scope='Four fixed-partition actual default512 manufactured lifecycles,44 byte-identical accepted/source/state/readback files,4 unchanged rejected-replay files, configuredMPI1/2/4 source/edge/gap/overlap/thin/invalid/filter/insertion/repair/budget/default/native2D bridge controls plus serial MMG2D/3D round trips and324 independent automatic frames. Counts prove skipped work; single contended timings do not prove speedup/scaling/CFD affordability. FullGoal1 active.')
    (HERE/'DONOR_REUSE_CHECKPOINT_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(cases),'fixed-work driver cases;',count,'automatic frames;',len(samples),'phase rows')
if __name__=='__main__':main()
