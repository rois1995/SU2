"""Audit fixed actual-cavity coverage, native gates and the saved publication state."""
import collections,json,math,re
from pathlib import Path
from assess_surface_transaction470 import sha,owned_rows,terminal_receipt
R=Path(__file__).resolve().parent

def main():
    case=max((R/'cases').glob('curved478_saved_surface_corrections_serial_*'),key=lambda p:p.stat().st_mtime)
    status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['pins_verified']
    pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
    handles=terminal_receipt(case);(case/'terminal_handles_actual.json').write_text(json.dumps(handles,indent=2)+'\n')
    records=[json.loads(l) for l in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
    inputs=json.loads((R/'surface_comparison478_inputs.json').read_text());spec=inputs['contexts']['pairs']
    capture=[v for v in records if v['stage']=='saved_correction_capture'];assert len(capture)==1
    contexts=[v for v in records if v['stage']=='saved_correction_context'];trials=[v for v in records if v['stage']=='saved_correction_trial']
    result=[v for v in records if v['stage']=='saved_correction_result'];pub=[v for v in records if v['stage']=='saved_correction_publication']
    if status['exit_code']==0:
        assert len(contexts)==3 and len(trials)==24 and len(result)==1
        assert result[0]['contexts_completed']==3 and result[0]['slots']==24 and result[0]['constructor_calls']<=24
    summary=[]
    for i,entry in enumerate(spec):
        rows=[v for v in trials if v['context']==entry['label']]
        if status['exit_code']==0:
            assert len(rows)==8 and [v['slot'] for v in rows]==list(range(i*8+1,i*8+9))
            assert [v['local_slot'] for v in rows]==list(range(8))
            assert [v['operator'] for v in rows]==['MoveVertex']*6+['CollapseVertex','Reconnect']
        for row in rows:
            assert row['old_cells']<=64 and row['new_cells']<=128 and row['reserved']
            if row['private_feasible']:
                assert all(row[k] for k in ('constructed','complete_removed_stars','measured','geometry_complete','geometry_pass','metric_pass','parameter_pass','contact_protocol','contact_pass','passing_heights_preserved','provisional_height'))
            if row['operator']=='MoveVertex':assert row['vertex']==entry['vertices'][row['local_slot']//3]
            for family in ('q','J','geometry'):
                if row['measured']:assert row['halo_'+family+'_delta']==row['new_'+family+'_bad']-row['old_'+family+'_bad']
        summary.append(dict(context=entry['label'],slots=len(rows),constructed=sum(v['constructed'] for v in rows),private_feasible=sum(v['private_feasible'] for v in rows),geometry_progress_feasible=sum(v['private_feasible'] and v['active_geometry_progress'] for v in rows),refusals=[dict(operator=v['operator'],slot=v['slot'],constructor=v['constructor_reason'],certification=v['certification_reason'],support=v['support_reason'],geometry_pass=v['geometry_pass'],metric=v['metric_reason'],parameters=v['parameter_reason'],contact=v['contact_reason'],preserve=v['preserve_reason'],height=v['height_reason']) for v in rows if not v['private_feasible']]))
    source=Path(inputs['owned']['path']);assert sha(case/'correction_unchanged_owned.bin')==inputs['owned']['sha256']
    original=dict(owned_rows(source));working=dict(owned_rows(case/'trajectory_working_owned.bin'))
    support=[v for v in records if v['stage']=='saved_correction_publication_support']
    if pub:
        assert len(pub)==len(support)==1 and pub[0]['published']
        removed=set(support[0]['removed_cell_ids']);added=set(support[0]['added_cell_ids']);assert not removed&added
        assert original.keys()-working.keys()==removed and working.keys()-original.keys()==added
        assert all(working[k]==digest for k,digest in original.items() if k not in removed)
        unchanged=len(original)-len(removed)
        if pub[0]['active_context']:assert any(v['context']==pub[0]['context'] and v['operator']==pub[0]['operator'] and v['private_feasible'] and v['active_geometry_progress'] for v in trials)
    else:
        assert original==working;assert sha(case/'trajectory_working_parameters.bin')==inputs['parameters']['sha256'];unchanged=len(original)
    edges=[v for v in records if v['stage']=='saved_correction_edge'];targets=[v for v in records if v['stage']=='saved_correction_target']
    distributions=[]
    for slot in sorted({v['slot'] for v in edges}):
        for side in ('old','fresh'):
            e=[v for v in edges if v['slot']==slot and v['side']==side];lengths=sorted(v['length'] for v in e)
            assert all(math.isfinite(x) and x>0 for x in lengths)
            distributions.append(dict(slot=slot,side=side,edges=len(e),below_half=sum(x<.5 for x in lengths),below_nine_tenths=sum(x<.9 for x in lengths),overlong=sum(x>1.8 for x in lengths),minimum=min(lengths) if lengths else None,maximum=max(lengths) if lengths else None,planning_only=True))
    report=dict(case=str(case),status=status,pins_verified=len(pins),all_actual_handles_terminal=True,contexts=contexts,summary=summary,trials=trials,result=result,publication=pub,untouched_cells_byte_identical=unchanged,original_cells=len(original),working_cells=len(working),working_owned_sha256=sha(case/'trajectory_working_owned.bin'),working_parameters_sha256=sha(case/'trajectory_working_parameters.bin'),edge_distributions=distributions,target_samples=len(targets),production_changed=False,goal_complete=False,limitations=['Serial capability comparison; distributed integration and final BL/RANS acceptance remain unqualified.','q/J are centroid construction observations; native full-target final quality/size gates remain mandatory.','Lengths are production SurfacePlanning endpoint-tangent guidance, not full-composed-volume quadrature certificates.','Untouched halo is reused only through complete removed stars, unchanged existing-node coordinates/tensors and independently byte-identical unchanged cells.','No alternative midpoint, additional relocation starts/steps, larger cavities or surface flips were tested.'])
    (R/'surface_comparison478_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('case','status','summary','publication','working_cells','untouched_cells_byte_identical')},indent=2))
if __name__=='__main__':main()
