"""Qualify real persisted SU2/CGNS BDF2 restart cases and unchanged production regressions."""
from pathlib import Path
import csv,json,statistics
from analyze_insertion import HERE,qualify,sha,parse,closes,ENGINE
from analyze_donor_reuse import FILES

def main():
    matrix=json.loads((HERE/'RESTART_MATRIX.json').read_text())
    reg=qualify('restart_regression_v1',('source_drift','object_drift','archive_drift'))
    old=qualify('wedge_regression_v1',('source_drift','object_drift','archive_drift'))
    assert len(reg['actual_objects_sha256'])==68
    assert reg['library_sha256']==old['library_sha256'] and reg['thin_archive_member_sha256']==old['thin_archive_member_sha256']
    regression_counts={}
    for rank in (1,2,4):
        text=(HERE/'restart_regression_v1'/f'mpi{rank}.stdout').read_text()
        assert text.count('All tests passed')==rank and text.count('65 test cases')==rank and 'No test cases matched' not in text
        regression_counts[str(rank)]=65
    text=(HERE/'restart_regression_v1/mmg_serial.stdout').read_text()
    assert '48 assertions in 2 test cases' in text
    cases=[];rows=[];identity=None;unchanged={}
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
        p=a['parameters'];assert all(p[k]==entry[k] for k in ('wedge','ranks','workers','restart_format'))
        assert (p['size'],p['anisotropy'],p['varying'],p['direction'],p['height_power'],p['diagnostic_rounds'],p['aligned_markers'])==(2,1,1,1,0,512,True)
        if p['restart_format']=='none':
            assert a['restart'] is None
            for f in list(FILES)+['envelope_s2_written_SU2_reload_target.json','envelope_s2_written_CGNS_reload_target.json']:
                assert (HERE/name/f).read_bytes()==(HERE/'wedge_control_box_n1_v1'/f).read_bytes(),f
                unchanged[f]=sha(HERE/name/f)
        else:
            assert a['restart']['adversaries_rejected']==4 and a['adversaries_rejected']==6
            assert all(r['maximum_state_relative_error']==0. for r in a['restart']['files'])
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
        restart=[r for r in a['rank_stages'] if r['stage']=='envelope_restart']
        frames=[r for r in a['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        assert len(frames)==3 and len(a['states'])==3 and len(a['reload_metrics'])==2
        cases.append(dict(folder=name,parameters=p,cells=[r['cells'] for r in frames],
            minimum_q=min(r['minimum_mean_ratio'] for r in frames),minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),maximum_L=max(r['maximum_length'] for r in frames),
            state_error=max(r['maximum_state_relative_error'] for r in a['states']),tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in a['states']+a['reload_metrics']),
            restart=a['restart'],restart_rank_phases=restart,restart_parent_max_seconds=max((r['total'] for r in restart),default=0.),
            cycles=cycles,validation_sha256=sha(HERE/name/'validation.json'),audit_sha256=sha(HERE/name/'independent_audit.json')))
    keys=['case','record','stage','rank','ranks']+sorted({k for r in rows for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'restart_rank_phases.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=keys,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    out=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','analyze_donor_reuse.py','audit_lifecycle.py','audit_geometry_bridge.py','audit_envelope.py','audit_restart.py')},
        binary_reader_sha256=sha(HERE.parents[1]/'TestCases/adaptation/capability/capcheck.py'),matrix_sha256=sha(HERE/'RESTART_MATRIX.json'),
        cases=cases,accounting_rows=len(rows),csv_sha256=sha(HERE/'restart_rank_phases.csv'),regression_case_counts=regression_counts,
        regression_validation_sha256=sha(HERE/'restart_regression_v1/validation.json'),unchanged_control_files=unchanged,production_unchanged_from_wedge=True,
        scope='Eight actual default512 wedge three-event driver cases: persisted SU2/CGNS original authority and two binary BDF2 states, real driver destruction/construction/Preprocess then resumed coarsening/refinement, N1M1/N2M2/N4M2/N4M4. Exact geometry/metric/currentBDF2 audits and readback plus13 unchanged control files; actual configured68-object regression65native/reference/bridges cases per rank and serialMMG48assertions. Restart phases separate from event parents; fixture write contains bounded state checks and teardown contains restart-config serialization. One contended sample per changed trajectory: no speedup/scaling/private-wait claim, fresh event sensor sources rather than persistent full tensor donor field, no PDE iteration, curved/3DBL/fullunsteady/affordability or fullGoal1 claim.')
    (HERE/'RESTART_CHECKPOINT_VALIDATION.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS',len(cases),'cases;',len(rows),'closed rows;',len(unchanged),'byte-identical files')
if __name__=='__main__':main()
