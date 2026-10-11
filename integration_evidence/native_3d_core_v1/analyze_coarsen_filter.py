"""Bind actual driver/filter evidence and closed phase accounting; no adaptation rerun."""
from pathlib import Path
import csv,json,statistics
from analyze_insertion import qualify,sha,HERE,parse,closes,ENGINE
CASES=('coarsen_filter_aligned_n3_n1_v1','coarsen_filter_driver_n1_v1',
       'coarsen_filter_aligned_n3_n4_m2_v1','coarsen_filter_aligned_n3_n4_m4_v1')
def main():
    regression=qualify('coarsen_filter_regression_v2',('source_drift','object_drift','archive_drift'))
    assert len(regression['actual_objects_sha256'])==68
    samples=[];cases=[];baseline=None
    for name in CASES:
        receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        assert all(regression['source_sha256'][k]==v for k,v in receipt['source_sha256'].items() if k in regression['source_sha256']), 'Compiled source differs across receipts'
        production=(receipt['library_sha256'],receipt['thin_archive_member_sha256'])
        if baseline is None:baseline=production
        else:assert production==baseline
        assert all(regression['library_sha256'].get(k)==v for k,v in production[0].items())
        assert all(regression['thin_archive_member_sha256'].get(k)==v for k,v in production[1].items())
        folder=HERE/name;audit=json.loads((folder/'independent_audit.json').read_text());assert audit['status']=='PASS'
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
        cycles=[]
        for cycle in range(3):
            parents=[r for r in audit['rank_rows'] if r['cycle']==cycle]
            native=[next(r for i,r in enumerate(x for x in audit['native_rows'] if x['rank']==rank) if i==cycle)
                    for rank in range(audit['parameters']['ranks'])]
            coarsen=[next(r for i,r in enumerate(x for x in details if x['stage']=='coarsen' and x['rank']==rank) if i==cycle)
                     for rank in range(audit['parameters']['ranks'])]
            wall=max(r['parent_wall'] for r in parents);ops=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            cycles.append(dict(cycle=cycle,parent_max_seconds=wall,remesh_max_seconds=max(r['total'] for r in native),
                parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in parents),accepted_operations=ops,
                accepted_operations_per_parent_second=ops/wall,coarsen_rounds_max=max(r['rounds'] for r in coarsen),
                coarsen_accepted=sum(r['accepted'] for r in coarsen),coarsen_rejected=sum(r['rejected'] for r in coarsen),
                geometry_predicates=sum(r['collapse_geometry_checks'] for r in coarsen),
                queued_directions_pruned=sum(r['collapse_filtered'] for r in coarsen),
                maximum_geometry_nested_seconds=max(r['nested_collapse_geometry'] for r in coarsen),
                native_bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native)))
        frames=[r for r in audit['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        assert len(frames)==3 and len(audit['states'])==3
        cases.append(dict(folder=name,parameters=audit['parameters'],cells=[r['cells'] for r in frames],
            minimum_q=min(r['minimum_mean_ratio'] for r in frames),minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),
            maximum_L=max(r['maximum_length'] for r in frames),state_error=max(r['maximum_state_relative_error'] for r in audit['states']),
            transported_metric_error=max(r['maximum_transported_tensor_absolute_error'] for r in audit['states']),
            validation_sha256=sha(folder/'validation.json'),cycles=cycles))
    auto=qualify('coarsen_filter_auto_audit_v1',());assert auto['source_folder']==str(HERE/'coarsen_filter_regression_v2')
    independent_frames=0
    for ranks in (1,2,4):
        audit=json.loads((HERE/f'coarsen_filter_auto_audit_v1/mpi{ranks}/independent_audit.json').read_text())
        assert audit['status']=='PASS';independent_frames+=len(audit['frames'])
    assert independent_frames==324
    first=('envelope_input.su2','envelope_s0_initial.json','envelope_s0.json','envelope_s0_states.json',
           'envelope_s1_initial.json','envelope_mesh_00001.su2','envelope_mesh_00001.su2.native_ref')
    baseline_case=HERE/'insertion_aligned_n3_n1_v1';updated=HERE/CASES[0]
    identity={p:sha(updated/p) for p in first if (updated/p).read_bytes()==(baseline_case/p).read_bytes()}
    assert len(identity)==len(first),'Starting first event or second donor changed before coarsening'
    keys=['case','record','stage','rank','ranks']+sorted({k for r in samples for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'coarsen_filter_rank_phases.csv').open('w',newline='') as f:
        out=csv.DictWriter(f,fieldnames=keys);out.writeheader();out.writerows(samples)
    result=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','audit_lifecycle.py')},matrix_sha256=sha(HERE/'COARSEN_FILTER_MATRIX.json'),cases=cases,
        regression_validation_sha256=sha(HERE/'coarsen_filter_regression_v2/validation.json'),
        independent_automatic_frames=independent_frames,auto_validation_sha256=sha(HERE/'coarsen_filter_auto_audit_v1/validation.json'),
        unchanged_first_event_and_second_input=identity,accounting_rows=len(samples),csv_sha256=sha(HERE/'coarsen_filter_rank_phases.csv'),
        scope='Four default512 actual manufactured lifecycle cases; MPI1/2/4 full configured filter/coplanar/insertion/invalid-field repair/atomic/budget/default/2D/MMG checks and324 independently audited automatic frames. Exact local facet counterexamples prune queued directions only after global admissibility. Counts do not include every future enqueue suppressed. Single contended pilots, changed mesh trajectories: no timing speedup/scaling/CFD claim. FullGoal1 active.')
    (HERE/'COARSEN_FILTER_CHECKPOINT_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(cases),'driver cases;',independent_frames,'automatic frames;',len(samples),'phase rows')
if __name__=='__main__':main()
