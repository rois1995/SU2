"""Qualify retained envelope receipts and nested accounting; report trajectory-aware pilot costs."""
from pathlib import Path
from collections import Counter
import csv,hashlib,json,statistics
from audit_lifecycle import parse,closes,ENGINE
from audit_geometry_bridge import reject,states
from audit_coupled import frame

HERE=Path(__file__).resolve().parent
CASES=('envelope_pilot_v1','envelope_varying_v1','envelope_mpi2_m1_v1','envelope_mpi2_m2_v1',
       'envelope_mpi4_m1_v1','envelope_mpi4_m2_v1','envelope_mpi4_m4_v1','envelope_size3_v1',
       'envelope_size4_v1','envelope_anisotropy2_v1')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    results=[];failures=[];samples=[];baseline=None
    for name in CASES:
        folder=HERE/name
        if not (folder/'validation.json').exists():continue
        receipt=json.loads((folder/'validation.json').read_text())
        for path,digest in receipt['artifact_sha256'].items():assert sha(folder/path)==digest,(name,path)
        assert not receipt['changed_during_run'] and not receipt['archive_changes'] and not receipt['object_changes']
        production=(receipt['library_sha256'],receipt['thin_archive_member_sha256'])
        if baseline is None:baseline=production
        else:assert production==baseline,'Production libraries changed between cases'
        if receipt['status']!='PASS' and not (folder/'AUDIT_RECOVERY.json').exists():
            assert name=='envelope_size3_v1'
            assert [(s['stage'],s['exit_code']) for s in receipt['stages'] if s['exit_code']] == [('case',1)]
            log=(folder/'case.stdout').read_text()
            assert 'round limit reached before strict metric completion' in log
            row=next(parse(line) for line in log.splitlines() if line.startswith('NATIVE3D_DETAIL stage=refine '))
            closes(row,ENGINE);assert row['rounds']==row['attempted']==512
            assert row['accepted']+row['rejected']==row['attempted']
            native=next(parse(line) for line in log.splitlines() if line.startswith('NATIVE3D '))
            assert native['complete']==0 and native['initial']==162 and native['final']==0
            initial,_=frame(folder/'envelope_s0_initial.json',False)
            failure=dict(folder=name,status='RETAINED_REJECTION',validation_sha256=sha(folder/'validation.json'),
                         initial=initial,refinement=row,remesher=native,
                         diagnosis='First size increase hits the fixed 512-round budget after 59 private accepted edits and 453 rejected attempts. This does not establish that repeated feature attempts caused the cap or that a larger budget would complete.',
                         scope='Default-backend rejection; Catch checks accepted mesh/state/reference preservation before the deliberate completion assertion fails. Initial grid and logs retained; private rejected mesh not yet exported. No accepted numerical output or useful accepted-work rate.')
            failures.append(failure);continue
        if receipt['status']!='PASS':
            recovery=json.loads((folder/'AUDIT_RECOVERY.json').read_text());assert recovery['status']=='PASS'
            assert recovery['original_validation_sha256']==sha(folder/'validation.json')
            assert recovery['original_failure_stderr_sha256']==sha(folder/'independent_audit.stderr')
            assert recovery['independent_audit_sha256']==sha(folder/'independent_audit.json')
            assert recovery['vtu_export_sha256']==sha(folder/'vtu_export.json')
        assert not receipt['changed_during_run'] and not receipt['archive_changes'] and not receipt['object_changes']
        assert all(s['exit_code']==0 for s in receipt['stages'] if s['stage']!='independent_audit')
        audit=json.loads((folder/'independent_audit.json').read_text());assert audit['status']=='PASS'
        p=audit['parameters'];n=p['ranks'];details=[]
        for line in (folder/'case.stdout').read_text().splitlines():
            if not line.startswith('NATIVE3D_DETAIL '):continue
            row=parse(line);assert row['ranks']==n and 0<=row['rank']<n and row['complete']==1
            if row['stage'] in ('refine','coarsen'):closes(row,ENGINE);assert row['hits']<=row['requests']
            elif row['stage'] in ('import','weights','return'):
                closes(row,('points','cells','faces','source','query','reader'));assert row['hits']<=row['requests']
            elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
            else:assert row['stage']=='source'
            details.append(row)
        assert Counter(r['stage'] for r in details)==Counter({k:3*n for k in ('import','weights','return','source','partition','refine','coarsen')})
        if details:
            bad=dict(next(r for r in details if r['stage']=='refine'));bad['metric']+=1;reject(lambda:closes(bad,ENGINE))
        for r in audit['rank_stages']:samples.append(dict(case=name,record='exclusive_stage',**r))
        for r in details:samples.append(dict(case=name,record='nested_detail',**r))
        cycles=[]
        for cycle in range(3):
            outer=[r for r in audit['rank_rows'] if r['cycle']==cycle];assert len(outer)==n
            engine_rows=[];native=[]
            for rank in range(n):
                rank_native=[r for r in audit['native_rows'] if r['rank']==rank];assert len(rank_native)==3
                native.append(rank_native[cycle])
                for label in ('refine','coarsen'):
                    rows=[r for r in details if r['stage']==label and r['rank']==rank];assert len(rows)==3
                    engine_rows.append(rows[cycle])
            accepted=sum(r['accepted'] for r in engine_rows)
            wall=max(r['parent_wall'] for r in outer)
            cycles.append(dict(cycle=cycle,accepted_edits=int(accepted),parent_seconds=wall,accepted_edits_per_parent_second=accepted/wall,
                               parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in outer),
                               remesh_max_seconds=max(r['total'] for r in native),
                               maximum_native_bound=max(r['native_bound'] for r in native),
                               maximum_process_lifetime_rss=max(r['process_peak_rss'] for r in native)))
        frames=[r for r in audit['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')];assert len(frames)==3
        results.append(dict(folder=name,parameters=p,original_status=receipt['status'],qualified_status='PASS',
                            validation_sha256=sha(folder/'validation.json'),independent_audit_sha256=sha(folder/'independent_audit.json'),
                            audit_frames=len(audit['frames']),state_frames=len(audit['states']),rank_stage_count=len(audit['rank_stages']),nested_detail_count=len(details),
                            initial_cells=audit['frames'][0]['cells'],accepted_cells=[r['cells'] for r in frames],
                            minimum_mean_ratio=min(r['minimum_mean_ratio'] for r in frames),minimum_scaled_jacobian=min(r['minimum_scaled_jacobian'] for r in frames),
                            maximum_metric_edge=max(r['maximum_length'] for r in frames),
                            maximum_state_relative_error=max(r['maximum_state_relative_error'] for r in audit['states']),
                            maximum_transported_tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in audit['states']),cycles=cycles))
    assert results,'No qualified evidence'
    legacy=HERE/'reader_runtime_v2';legacy_count=0;legacy_error=0.
    for folder in (legacy/f'mpi{n}' for n in (1,2,4)):
        for path in folder.glob('runtime_*_states.json'):
            data=json.loads(path.with_name(path.name.replace('_states.json','.json')).read_text())
            check=states(data,json.loads(path.read_text()));legacy_count+=1
            legacy_error=max(legacy_error,check['maximum_state_relative_error'])
    assert legacy_count==84,'Missing published legacy default-sensor evidence'
    control=HERE/'envelope_legacy_control_v1';control_receipt=json.loads((control/'validation.json').read_text())
    assert control_receipt['status']=='PASS' and not control_receipt['source_changes'] and not control_receipt['production_changes']
    assert control_receipt['production_equal_to_published_reader']
    assert all(s['exit_code']==0 for s in control_receipt['stages'])
    for path,digest in control_receipt['artifact_sha256'].items():assert sha(control/path)==digest
    published=json.loads((HERE/'reader_skew_v1/validation.json').read_text())
    assert baseline==(published['library_sha256'],published['thin_archive_member_sha256'])
    keys=['case','record','stage','rank','ranks']+sorted({k for r in samples for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'envelope_rank_phases.csv').open('w',newline='') as f:
        out=csv.DictWriter(f,fieldnames=keys);out.writeheader();out.writerows(samples)
    result=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),matrix_sha256=sha(HERE/'ENVELOPE_MATRIX.json'),
                qualified_cases=results,retained_rejections=failures,exclusive_and_nested_csv_rows=len(samples),legacy_default_sensor_state_frames=legacy_count,
                legacy_maximum_state_error=legacy_error,legacy_control_validation_sha256=sha(control/'validation.json'),production_equal_to_published_reader=True,remaining_initial_cases=[x for x in CASES if x not in {r['folder'] for r in results+failures}],
                scope='Retained small manufactured actual-driver evidence, identical production archive/member identities, receipt artifact integrity, closed exclusive/nested accounting and default-sensor re-audit. Single contended pilots with different trajectories are not fixed-work scaling or instrumentation-overhead measurements. Parent includes diagnostic observation/reporting; requested output is separate. Lifetime RSS and modeled native bounds differ. Full Goal1 remains incomplete.')
    (HERE/'ENVELOPE_CHECKPOINT_VALIDATION.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(results),'qualified cases;',len(failures),'retained rejections;',len(samples),'exclusive/nested rows;',legacy_count,'legacy state frames')
if __name__=='__main__':main()
