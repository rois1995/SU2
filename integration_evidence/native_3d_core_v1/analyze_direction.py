"""Bind spatial-direction driver evidence to production identities and the frozen target."""
from pathlib import Path
import csv,json,statistics
from analyze_insertion import HERE,qualify,qualify_failure,sha,parse,closes,ENGINE
from analyze_donor_reuse import FILES
CASES=('direction_control_a2_n2_n1_v2','direction_a15_n2_n1_v2','direction_a15_n2_n2_m2_v2','direction_a15_n2_n4_m2_v1','direction_a15_n2_n4_m4_v1')
def main():
    reg=qualify('direction_regression_v1',('source_drift','object_drift','archive_drift'))
    assert len(reg['actual_objects_sha256'])==68
    cases=[];samples=[];binary=None
    for name in CASES:
        r=qualify(name,('changed_during_run','archive_changes','object_changes'))
        assert all(reg['source_sha256'][k]==v for k,v in r['source_sha256'].items() if k in reg['source_sha256'])
        assert all(reg['library_sha256'].get(k)==v for k,v in r['library_sha256'].items())
        assert reg['thin_archive_member_sha256']==r['thin_archive_member_sha256']
        if binary is None:binary=(r['binary_sha256'],r['actual_test_object_sha256'])
        else:assert binary==(r['binary_sha256'],r['actual_test_object_sha256'])
        a=json.loads((HERE/name/'independent_audit.json').read_text());assert a['status']=='PASS'
        p=a['parameters'];control=name.startswith('direction_control')
        assert (p['size'],p['anisotropy'],p['varying'],p['direction'],p['diagnostic_rounds'],p['aligned_markers'])==(2,2 if control else 1,1,0 if control else 1,512,True)
        equal={}
        if control:
            equal={n:sha(HERE/name/n) for n in FILES if (HERE/name/n).read_bytes()==(HERE/'robustness_a2_n2_n1_v1'/n).read_bytes()}
            assert len(equal)==11
        else:
            assert a['adversaries_rejected']==4 and {r['format'] for r in a['reload_metrics']}=={'SU2','CGNS'}
            assert max(r['maximum_transported_tensor_absolute_error'] for r in a['reload_metrics'])<1e-13
            data=json.loads((HERE/name/'envelope_s0_initial.json').read_text())
            vertices=[v for c in data['original'] for v in c['vertices']]
            assert any(abs(v[4][2])>.1 and abs(v[4][4])>.1 for v in vertices),'No out-of-plane coupling'
        details=[]
        for line in (HERE/name/'case.stdout').read_text().splitlines():
            if not line.startswith('NATIVE3D_DETAIL '):continue
            row=parse(line)
            if row['stage'] in ('refine','coarsen'):closes(row,ENGINE)
            elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'))
            elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
            else:assert row['stage']=='source'
            details.append(row)
        samples.extend(dict(case=name,record='exclusive',**r) for r in a['rank_stages'])
        samples.extend(dict(case=name,record='nested',**r) for r in details)
        cycles=[]
        for cycle in range(3):
            rows=[r for r in a['rank_rows'] if r['cycle']==cycle];wall=max(r['parent_wall'] for r in rows)
            native=[[r for r in a['native_rows'] if r['rank']==rank][cycle] for rank in range(p['ranks'])]
            edits=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            cycles.append(dict(cycle=cycle,parent_max_seconds=wall,remesh_max_seconds=max(r['total'] for r in native),
                parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in rows),accepted_edits=edits,accepted_edits_per_parent_second=edits/wall,
                native_bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native)))
        frames=[r for r in a['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        cases.append(dict(folder=name,unchanged=equal,cells=[r['cells'] for r in frames],minimum_q=min(r['minimum_mean_ratio'] for r in frames),
            minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),maximum_L=max(r['maximum_length'] for r in frames),
            state_error=max(r['maximum_state_relative_error'] for r in a['states']),tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in a['states']),
            reload_metric_error=max((r['maximum_transported_tensor_absolute_error'] for r in a['reload_metrics']),default=0),cycles=cycles,
            validation_sha256=sha(HERE/name/'validation.json')))
    failed=qualify_failure('direction_a15_n2_n2_m2_v1','independent_audit')
    assert failed['library_sha256']==r['library_sha256']
    assert 'Oversized final original-field edge' in (HERE/'direction_a15_n2_n2_m2_v1/independent_audit.stderr').read_text()
    common=[n for n in FILES if '_written_' not in n]
    before=HERE/'direction_a15_n2_n2_m2_v1';after=HERE/'direction_a15_n2_n2_m2_v2'
    unchanged={n:sha(after/n) for n in common if (before/n).read_bytes()==(after/n).read_bytes()}
    assert len(unchanged)==9,'Reload correction changed actual adaptation/state trajectory'
    keys=['case','record','stage','rank','ranks']+sorted({k for r in samples for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'direction_rank_phases.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=keys);writer.writeheader();writer.writerows(samples)
    out=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','analyze_donor_reuse.py','audit_lifecycle.py')},
        matrix_sha256=sha(HERE/'DIRECTION_MATRIX.json'),cases=cases,accounting_rows=len(samples),csv_sha256=sha(HERE/'direction_rank_phases.csv'),
        regression_validation_sha256=sha(HERE/'direction_regression_v1/validation.json'),unchanged_adaptation_data_before_reload_fix=unchanged,retained_reload_failure_sha256=sha(HERE/'direction_a15_n2_n2_m2_v1/validation.json'),
        scope='Four spatial 3D direction-changing box lifecycles MPI1/M1,MPI2/M2,MPI4/M2,MPI4/M4, plus eleven byte-identical legacy control datafiles. Frozen original-P1 metrics, actual geometry/marker/current+BDF2/SU2CGNS checks, loaded nodal transport checked separately from frozen-target geometry. Actual complete configured68-object regression. Test-only changes; no production change, analytic-target substitution, actual solver restart, new-source interior admissibility, scaling or fullGoal1 qualification.')
    (HERE/'DIRECTION_CHECKPOINT_VALIDATION.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS',len(cases),'driver cases;',len(samples),'closed accounting rows')
if __name__=='__main__':main()
