"""Qualify the frozen oblique-wedge matrix and default box marker compatibility."""
from pathlib import Path
from collections import Counter
import csv,json,statistics
from analyze_insertion import HERE,qualify,sha,parse,closes,ENGINE
from analyze_donor_reuse import FILES
from audit_geometry_bridge import signature,su2_signature,cgns_signature,reject
from audit_envelope import WEDGE_MARKERS

def main():
    matrix=json.loads((HERE/'WEDGE_MATRIX.json').read_text())
    reg=qualify('wedge_regression_v1',('source_drift','object_drift','archive_drift'))
    old=qualify('thin_regression_v1',('source_drift','object_drift','archive_drift'))
    assert len(reg['actual_objects_sha256'])==68
    assert reg['library_sha256']==old['library_sha256'] and reg['thin_archive_member_sha256']==old['thin_archive_member_sha256']
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
        p=a['parameters'];assert all(p[k]==entry[k] for k in ('wedge','ranks','workers'))
        assert (p['size'],p['anisotropy'],p['varying'],p['direction'],p['height_power'],p['diagnostic_rounds'],p['aligned_markers'])==(2,1,1,1,0,512,True)
        assert a['adversaries_rejected']==(6 if p['wedge'] else 4)
        surfaces=[]
        for stem in ('envelope_s0_initial','envelope_s0','envelope_s1','envelope_s2'):
            data=json.loads((HERE/name/(stem+'.json')).read_text())
            names={f['id']:data['marker_names'][f['marker']] for f in data['facets']}
            surfaces.append(dict(sorted(Counter(names[f['facet']] for f in data['surface']).items())))
        if p['wedge']:
            assert all(r['volume']=='1/2' for r in a['frames'])
            assert surfaces[0]['wedge_oblique']==8 and surfaces[1]['wedge_oblique']>8 and surfaces[2]['wedge_oblique']<surfaces[1]['wedge_oblique']
            reject(lambda:signature(data))  # The legacy default must still require all seven box markers.
        else:
            for f in list(FILES)+['envelope_s2_written_SU2_reload_target.json','envelope_s2_written_CGNS_reload_target.json']:
                assert (HERE/name/f).read_bytes()==(HERE/'thin_control_h0_n1_v3'/f).read_bytes(),f
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
        assert len(frames)==3 and len(a['states'])==3 and len(a['reload_metrics'])==2
        cases.append(dict(folder=name,parameters=p,cells=[r['cells'] for r in frames],surface_triangles=surfaces,
            minimum_q=min(r['minimum_mean_ratio'] for r in frames),minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),maximum_L=max(r['maximum_length'] for r in frames),
            state_error=max(r['maximum_state_relative_error'] for r in a['states']),tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in a['states']+a['reload_metrics']),
            cycles=cycles,validation_sha256=sha(HERE/name/'validation.json'),audit_sha256=sha(HERE/name/'independent_audit.json')))
    legacy=[]
    thin=json.loads((HERE/'THIN_CHECKPOINT_VALIDATION.json').read_text())
    for case in thin['cases']:
        name=case['folder'];qualify(name,('changed_during_run','archive_changes','object_changes'))
        data=json.loads((HERE/name/'envelope_s2.json').read_text());expected=signature(data)
        su2=HERE/name/'envelope_s2_written_SU2.su2';cgns=HERE/name/'envelope_s2_written_CGNS.cgns'
        assert su2_signature(su2)==cgns_signature(cgns)==expected
        reject(lambda:signature(data,WEDGE_MARKERS))
        legacy.append(dict(folder=name,su2_sha256=sha(su2),cgns_sha256=sha(cgns)))
    keys=['case','record','stage','rank','ranks']+sorted({k for r in rows for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'wedge_rank_phases.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=keys,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    out=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','analyze_donor_reuse.py','audit_lifecycle.py','audit_geometry_bridge.py','audit_envelope.py')},
        matrix_sha256=sha(HERE/'WEDGE_MATRIX.json'),cases=cases,accounting_rows=len(rows),csv_sha256=sha(HERE/'wedge_rank_phases.csv'),
        regression_validation_sha256=sha(HERE/'wedge_regression_v1/validation.json'),unchanged_control_files=unchanged,legacy_default_signature_checks=legacy,
        production_unchanged_from_thin=True,
        scope='Four oblique triangular-prism lifecycles N1M1,N2M2,N4M2,N4M4 plus13 byte-identical box-control files. Exact domain/oblique plane/marker/positive volume/conformity and physical q/J/L, originalP1/currentBDF2 and SU2CGNS nodal/frozen target readback. Adaptable oblique triangles and two-sided marker junction; default512. Actual configured68-object regression native/bridges MPI1/2/4 and serialMMG. Legacy default seven-marker signature validation retained on six published thin/box outputs. Closed accounting and useful rates from single contended changed-work pilots only; no physicalCFD, restart, 3DBL, larger-domain, repeated scaling or fullGoal1 claim.')
    (HERE/'WEDGE_CHECKPOINT_VALIDATION.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS',len(cases),'cases;',len(rows),'closed rows;',len(unchanged),'byte-identical files;',len(legacy),'default-marker compatibility checks')
if __name__=='__main__':main()
