"""Qualify the frozen affine-thin matrix and its retained control failures; no remeshing reruns."""
from pathlib import Path
import csv,json,statistics
from analyze_insertion import HERE,qualify,qualify_failure,sha,parse,closes,ENGINE
from analyze_donor_reuse import FILES

def main():
    matrix=json.loads((HERE/'THIN_MATRIX.json').read_text())
    reg=qualify('thin_regression_v1',('source_drift','object_drift','archive_drift'))
    previous=qualify('direction_regression_v1',('source_drift','object_drift','archive_drift'))
    assert len(reg['actual_objects_sha256'])==68
    assert reg['library_sha256']==previous['library_sha256']
    assert reg['thin_archive_member_sha256']==previous['thin_archive_member_sha256']
    rows=[];cases=[];identity=None;unchanged={}
    for entry in matrix['cases']:
        name=entry['folder'];receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        assert all(reg['source_sha256'][k]==v for k,v in receipt['source_sha256'].items() if k in reg['source_sha256'])
        assert all(reg['library_sha256'].get(k)==v for k,v in receipt['library_sha256'].items())
        assert reg['thin_archive_member_sha256']==receipt['thin_archive_member_sha256']
        assert all(reg['actual_objects_sha256'].get(k)==v for k,v in receipt['actual_test_object_sha256'].items())
        key=(receipt['binary_sha256'],receipt['actual_test_object_sha256'])
        if identity is None:identity=key
        else:assert identity==key
        a=json.loads((HERE/name/'independent_audit.json').read_text());assert a['status']=='PASS'
        p=a['parameters']
        assert (p['size'],p['anisotropy'],p['varying'],p['direction'],p['diagnostic_rounds'],p['aligned_markers'])==(2,1,1,1,512,True)
        assert all(p[k]==entry[k] for k in ('height_power','ranks','workers'))
        assert a['adversaries_rejected']==(7 if p['height_power'] else 4)
        assert {r['format'] for r in a['reload_metrics']}=={'SU2','CGNS'}
        if not p['height_power']:
            for f in list(FILES)+['envelope_s2_written_SU2_reload_target.json','envelope_s2_written_CGNS_reload_target.json']:
                assert (HERE/name/f).read_bytes()==(HERE/'direction_a15_n2_n1_v2'/f).read_bytes(),f
                unchanged[f]=sha(HERE/name/f)
        details=[]
        for line in (HERE/name/'case.stdout').read_text().splitlines():
            if not line.startswith('NATIVE3D_DETAIL '):continue
            row=parse(line)
            if row['stage'] in ('refine','coarsen'):closes(row,ENGINE)
            elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'))
            elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
            else:assert row['stage']=='source'
            details.append(row)
        rows.extend(dict(case=name,record='exclusive',**r) for r in a['rank_stages'])
        rows.extend(dict(case=name,record='nested',**r) for r in details)
        cycles=[]
        for cycle in range(3):
            parents=[r for r in a['rank_rows'] if r['cycle']==cycle];wall=max(r['parent_wall'] for r in parents)
            native=[[r for r in a['native_rows'] if r['rank']==rank][cycle] for rank in range(p['ranks'])]
            edits=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            cycles.append(dict(cycle=cycle,parent_max_seconds=wall,remesh_max_seconds=max(r['total'] for r in native),
                parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in parents),accepted_edits=edits,accepted_edits_per_parent_second=edits/wall,
                native_bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native)))
        frames=[r for r in a['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        assert len(frames)==3 and len(a['states'])==3
        residuals=a['states']+a['reload_metrics']
        cases.append(dict(folder=name,parameters=p,cells=[r['cells'] for r in frames],minimum_q=min(r['minimum_mean_ratio'] for r in frames),
            minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),maximum_L=max(r['maximum_length'] for r in frames),
            state_error=max(r['maximum_state_relative_error'] for r in a['states']),
            reference_tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in residuals),
            physical_tensor_error=max(r.get('maximum_transported_tensor_physical_error',r['maximum_transported_tensor_absolute_error']) for r in residuals),
            cycles=cycles,validation_sha256=sha(HERE/name/'validation.json'),audit_sha256=sha(HERE/name/'independent_audit.json')))
    old=qualify('thin_control_h0_n1_v1',('changed_during_run','archive_changes','object_changes'))
    assert (HERE/'thin_control_h0_n1_v1/envelope_s0_initial.json').read_bytes()!=(HERE/'direction_a15_n2_n1_v2/envelope_s0_initial.json').read_bytes()
    failed=qualify_failure('thin_control_h0_n1_v2','compile_actual_meson_tests')
    assert 'no match for' in (HERE/'thin_control_h0_n1_v2/compile_actual_meson_tests.stdout').read_text()
    keys=['case','record','stage','rank','ranks']+sorted({k for r in rows for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'thin_rank_phases.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=keys);writer.writeheader();writer.writerows(rows)
    out=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','analyze_donor_reuse.py','audit_lifecycle.py')},
        matrix_sha256=sha(HERE/'THIN_MATRIX.json'),cases=cases,accounting_rows=len(rows),csv_sha256=sha(HERE/'thin_rank_phases.csv'),
        regression_validation_sha256=sha(HERE/'thin_regression_v1/validation.json'),unchanged_control_files=unchanged,
        retained_control_v1_validation_sha256=sha(HERE/'thin_control_h0_n1_v1/validation.json'),
        retained_control_v2_validation_sha256=sha(HERE/'thin_control_h0_n1_v2/validation.json'),
        production_unchanged_from_direction=True,
        scope='Frozen affine thin-domain matrix: h=1/1024 on N1M1,N2M2,N4M2,N4M4 and h=1/1048576 on N1M1, plus13 byte-identical zero-height-change control files. Three actual default512 refine/coarsen/refine events per case; exact physical geometry, physical q/J/L, frozen original-P1 sensor transport and current/BDF2, SU2CGNS nodal and retained-target readback checks. Tensor residuals independently reported in physical and reference units; no clipping/guard relaxation/BL generation. Complete68-object native/bridge MPI1/2/4 and serialMMG regression. One contended pilot per changed trajectory; no repeated timing, scaling, fullrestart, larger-domain or fullGoal1 qualification.')
    (HERE/'THIN_CHECKPOINT_VALIDATION.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS',len(cases),'cases;',len(rows),'closed accounting rows;',len(unchanged),'byte-identical control files')
if __name__=='__main__':main()
