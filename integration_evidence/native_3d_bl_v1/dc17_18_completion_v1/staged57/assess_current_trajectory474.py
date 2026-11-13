"""Verify bounded current473 patch trajectories; a passing test need not close a patch."""
import json,sys
from pathlib import Path
from assess_surface_transaction470 import sha, owned_rows, terminal_receipt
R=Path(__file__).resolve().parent

def main():
    build=sys.argv[1] if len(sys.argv)>1 else '474';assert build in ('474','475','476')
    case=max((R/'cases').glob('curved'+build+'_current_trajectory_serial_*'),key=lambda p:p.stat().st_mtime)
    status=json.loads((case/'status.json').read_text())
    assert status['state']=='terminal' and status['pins_verified']
    pins=json.loads((case/'pins.json').read_text())
    assert all(sha(p)==h for p,h in pins.items())
    handles=terminal_receipt(case)
    (case/'terminal_handles_actual.json').write_text(json.dumps(handles,indent=2)+'\n')
    records=[json.loads(s) for s in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
    assert not (case/'trajectory_replay.dat').exists()
    assert not any(v['stage']=='split_continuation' for v in records)
    controls=[v for v in records if v['stage']=='current_trajectory_control']
    assert len(controls)==1 and controls[0]['geometry_excess']==0
    contexts=[];current=None
    for v in records:
        if v['stage']=='current_trajectory_context':
            current=dict(input=v,operations=[],states=[],results=[],budget=[]);contexts.append(current)
            assert not v['legacy_precursor']
        elif current is not None:
            key={'transaction_operation':'operations','transaction_state':'states','transaction_result':'results','trajectory_budget':'budget'}.get(v['stage'])
            if key:current[key].append(v)
    assert 1<=len(contexts)<=2
    used=0
    for ctx in contexts:
        ops=ctx['operations'];results=ctx['results'];budgets=ctx['budget']
        assert ctx['input']['remaining_shared_calls']==24-used
        assert len(ops)<=24-used
        for n,v in enumerate(ops,1):
            assert v['attempt']==n and v['old_cells']<=64 and v['new_cells']<=128
            if build in ('475','476'):assert v['production_geometry_guard_required']
            if v['published']:
                if build in ('475','476'):assert v['geometry_progress']
                assert all(v[k] for k in ('constructed','native_input_topology','metric','parameters','contact_protocol','contact','preserved_heights','provisional_height','reserved'))
        used+=len(ops)
        if results:
            assert len(results)==len(budgets)==1
            result=results[0];b=budgets[0]
            assert result['attempts']==len(ops)==b['geometry_constructor_calls']
            assert result['commits']==sum(v['published'] for v in ops)
            assert not result['surface_geometry_certified'] and not result['original_mesh_accepted']
            used+=b['first_row_work'];assert b['remaining_shared_calls']==24-used
            for name,old,new in [('q','old_q_bad','new_q_bad'),('J','old_J_bad','new_J_bad'),('geometry','old_geometry_bad','new_geometry_bad')]:
                assert result['sum_local_'+name+'_delta']==sum(v[new]-v[old] for v in ops if v['published'])
        assert used<=24
    capture=[v for v in records if v['stage']=='current_trajectory_capture'];assert len(capture)==1
    if status['exit_code']==0:assert len(contexts)==2 and all(c['results'] for c in contexts)
    source=Path(json.loads((R/'current_trajectory474_contexts_provenance.json').read_text())['case'])
    expected={}
    for rank in range(6):
        for identity,digest in owned_rows(source/f'stage_surface_rank{rank}_owned.bin'):
            assert identity not in expected;expected[identity]=digest
    count=len(expected)
    for identity,digest in owned_rows(case/'correction_unchanged_owned.bin'):assert expected.pop(identity)==digest
    assert not expected and count==304508
    working=list(owned_rows(case/'trajectory_working_owned.bin'))
    assert len(working)==capture[0]['cells'] and len({r[0] for r in working})==len(working)
    report=dict(case=str(case),status=status,pins_verified=len(pins),actual_handles_terminal=True,
        contexts=contexts,control=controls[0],capture=capture[0],unchanged_original_records=count,
        working_cells=len(working),working_mesh_sha256=sha(case/'trajectory_working_owned.bin'),
        working_parameters_sha256=sha(case/'trajectory_working_parameters.bin'),
        production_changed=False,strict_surface_ready=False,goal_complete=False,
        limitations=['Serial bounded current-input trajectory; no new distributed capability claimed.',
        'q/J are construction observations; local transaction deltas are not whole-field churn.',
        'Geometry splitting/reconstruction prefix does not exhaust tangential collapse/relocation/reconnection capabilities.',
        'Guidance closure is not final bidirectional certification; directional readiness and first-row coverage require separate proof.'])
    (R/('current_trajectory'+build+'_assessment.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('case','status','working_cells','unchanged_original_records')},indent=2))
    for ctx in contexts:print(json.dumps(ctx['results'] or {'incomplete':ctx['input']},indent=2))
if __name__=='__main__':main()
