"""Qualify the unchanged-binary n4 wedge practical envelope and phase-cost evidence."""
from collections import defaultdict
from pathlib import Path
import csv,json,statistics
from analyze_insertion import HERE,qualify,sha,parse,closes,ENGINE

def main():
    matrix=json.loads((HERE/'LARGER_WEDGE_MATRIX.json').read_text())
    reg=qualify('restart_regression_v1',('source_drift','object_drift','archive_drift'))
    binary=qualify('restart_control_box_n1_v1',('changed_during_run','archive_changes','object_changes'))
    cases=[];rows=[]
    for entry in matrix['cases']:
        name=entry['folder'];receipt=qualify(name,('changed_during_run','archive_changes','object_changes'))
        assert receipt['binary_sha256']==binary['binary_sha256'] and receipt['actual_test_object_sha256']==binary['actual_test_object_sha256']
        assert receipt['source_sha256']==binary['source_sha256'] and receipt['library_sha256']==binary['library_sha256'] and receipt['thin_archive_member_sha256']==binary['thin_archive_member_sha256']
        assert all(reg['source_sha256'][k]==v for k,v in receipt['source_sha256'].items() if k in reg['source_sha256'])
        assert all(reg['library_sha256'].get(k)==v for k,v in receipt['library_sha256'].items())
        assert reg['thin_archive_member_sha256']==receipt['thin_archive_member_sha256']
        assert all(reg['actual_objects_sha256'].get(k)==v for k,v in receipt['actual_test_object_sha256'].items())
        a=json.loads((HERE/name/'independent_audit.json').read_text());assert a['status']=='PASS'
        p=a['parameters'];assert all(p[k]==entry[k] for k in ('ranks','workers'))
        assert (p['size'],p['anisotropy'],p['varying'],p['direction'],p['height_power'],p['diagnostic_rounds'],p['aligned_markers'],p['wedge'],p['restart_format'])==(4,1,1,1,0,512,True,True,'none')
        assert a['adversaries_rejected']==6 and a['restart'] is None
        details=[];groups=defaultdict(list)
        for line in (HERE/name/'case.stdout').read_text().splitlines():
            if not line.startswith('NATIVE3D_DETAIL '):continue
            row=parse(line)
            if row['stage'] in ('refine','coarsen'):closes(row,ENGINE)
            elif row['stage'] in ('import','weights','return'):closes(row,('points','cells','faces','source','query','reader'))
            elif row['stage']=='partition':closes(row,('graph','parmetis','migration'))
            else:assert row['stage']=='source'
            details.append(row);groups[row['stage']].append(row)
        rows.extend(dict(case=name,record='exclusive',**r) for r in a['rank_stages'])
        rows.extend(dict(case=name,record='nested',**r) for r in details)
        cycles=[]
        for cycle in range(3):
            parents=[r for r in a['rank_rows'] if r['cycle']==cycle];wall=max(r['parent_wall'] for r in parents)
            native=[[r for r in a['native_rows'] if r['rank']==rank][cycle] for rank in range(p['ranks'])]
            edits=sum(r['refine_accepted']+r['coarsen_accepted'] for r in native)
            cycles.append(dict(cycle=cycle,parent_max_seconds=wall,remesh_max_seconds=max(r['total'] for r in native),
                parent_max_over_mean=wall/statistics.mean(r['parent_wall'] for r in parents),accepted_edits=edits,accepted_edits_per_parent_second=edits/wall,
                native_bound=max(r['native_bound'] for r in native),lifetime_rss=max(r['process_peak_rss'] for r in native),packed_record_bytes=sum(r['tracked_bytes'] for r in native)))
        costs={}
        for stage in ('refine','coarsen'):
            totals={k:sum(r[k] for r in groups[stage]) for k in ENGINE}
            active=[r for r in groups[stage] if r['rank']<p['workers']]
            costs[stage]=dict(rank_event_work_seconds=totals,
                measured_private_cpu_seconds={k:sum(r[k] for r in active) for k in ('private_reconstruction_cpu','private_metric_cpu','private_measurement_cpu')},
                query_detail_work_seconds={k:sum(r[k] for r in active) for k in ('nested_field_build','nested_field_query','nested_cache','nested_search','nested_interpolation','nested_edge','nested_edge_trace','nested_edge_integral')},
                counters={k:sum(r[k] for r in active) for k in ('rounds','attempted','accepted','rejected','requests','hits','edge_evaluations','edge_reused','private_query_retries','query_retries')})
        frames=[r for r in a['frames'] if r['file'] in ('envelope_s0.json','envelope_s1.json','envelope_s2.json')]
        assert len(frames)==3 and len(a['states'])==3 and len(a['reload_metrics'])==2
        cases.append(dict(folder=name,parameters=p,cells=[r['cells'] for r in frames],points=[r['points'] for r in frames],
            minimum_q=min(r['minimum_mean_ratio'] for r in frames),minimum_J=min(r['minimum_scaled_jacobian'] for r in frames),maximum_L=max(r['maximum_length'] for r in frames),
            state_error=max(r['maximum_state_relative_error'] for r in a['states']),tensor_error=max(r['maximum_transported_tensor_absolute_error'] for r in a['states']+a['reload_metrics']),
            cycles=cycles,costs=costs,validation_sha256=sha(HERE/name/'validation.json'),audit_sha256=sha(HERE/name/'independent_audit.json')))
    keys=['case','record','stage','rank','ranks']+sorted({k for r in rows for k in r}-{'case','record','stage','rank','ranks'})
    with (HERE/'larger_wedge_rank_phases.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=keys,lineterminator='\n');writer.writeheader();writer.writerows(rows)
    out=dict(status='PASS',analyzer_sha256=sha(Path(__file__)),dependencies_sha256={n:sha(HERE/n) for n in ('analyze_insertion.py','audit_lifecycle.py')},
        matrix_sha256=sha(HERE/'LARGER_WEDGE_MATRIX.json'),cases=cases,accounting_rows=len(rows),csv_sha256=sha(HERE/'larger_wedge_rank_phases.csv'),
        unchanged_binary_sha256=binary['binary_sha256'],regression_validation_sha256=sha(HERE/'restart_regression_v1/validation.json'),
        limits=dict(original_reference_faces=256,original_reference_file_bytes=1048576,private_cavity_cells=64,private_donor_cells=256,default_rounds=512),
        scope='Four actual n4 wedge three-event MPI1/2/4 M=N/M<N cases, unchanged qualified binary/production archives/source and retained full configured regression. Independent exact geometry/q/J/L/originalP1/BDF/output/readback checks. Per-event elapsed maxima and accepted-work rates are separate from summed rank-event phase work; packed records include self traffic and are not MPI network bytes. Single contended changed-work pilots, not measured speedup/scaling or whole-goal completion. Global reference facet bound is explicit and not an operator cavity bound; complete communication/private-wait/memory calibration and SGE repetitions remain open.')
    (HERE/'LARGER_WEDGE_VALIDATION.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS',len(cases),'larger cases;',len(rows),'closed phase rows; unchanged qualified binary/source/production archives')
if __name__=='__main__':main()
