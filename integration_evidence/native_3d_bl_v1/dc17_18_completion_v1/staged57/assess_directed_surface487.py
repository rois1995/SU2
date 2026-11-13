"""Assess exact directed candidate coverage, native refusals and any private publication."""
import json,collections
from pathlib import Path
from assess_surface_transaction470 import sha,owned_rows
from inspect_native_failure3d import _read_owned
R=Path(__file__).resolve().parent
case=max((R/'cases').glob('curved487_directed_surface_corrections_serial_*'),key=lambda p:p.stat().st_mtime)
status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
rows=[json.loads(s) for s in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
trials=[x for x in rows if x['stage']=='saved_correction_trial'];contexts=[x for x in rows if x['stage']=='saved_correction_context']
assert len(trials)==3 and [x['slot'] for x in trials]==[1,2,3] and len(contexts)==3
entry=next(x for x in rows if x['stage']=='saved_correction_input');assert entry['directed_collapse'] and entry['total_slots']==3
assert [(x['vertex'],x['retained']) for x in trials]==[(7594,4280),(17814,64065077),(6070,6072)]
assert all(x['local_slot']==6 and 0<=x['production_repair_apex_rank']<=15 for x in trials)
counts=collections.Counter(x['context'] for x in trials);assert set(counts.values())=={1}
result=next(x for x in rows if x['stage']=='saved_correction_result');assert result['contexts_completed']==3 and result['constructor_calls']==3
inputs=json.loads((R/'reverse_surface486_inputs.json').read_text());source=Path(inputs['owned']['path']);assert sha(source)==inputs['owned']['sha256']
assert sha(case/'correction_unchanged_owned.bin')==sha(source)
before=dict(owned_rows(source));after=dict(owned_rows(case/'trajectory_working_owned.bin'))
removed=before.keys()-after.keys();added=after.keys()-before.keys();shared=before.keys()&after.keys();assert all(before[k]==after[k] for k in shared)
def nodes(path,count):
    answer={}
    for cell in _read_owned(path,count,2_000_000_000):
        assert cell['sensor_present']
        for i,node in enumerate(cell['nodes']):
            value=(tuple(node['xyz']),tuple(cell['sensor'][6*i:6*i+6]));assert answer.setdefault(node['id'],value)==value
    return answer
oldnodes=nodes(source,len(before));newnodes=nodes(case/'trajectory_working_owned.bin',len(after))
assert all(oldnodes[k]==newnodes[k] for k in oldnodes.keys()&newnodes.keys())
active_labels={x['context'] for x in contexts[:2]};feasible=[x for x in trials if x['context'] in active_labels and x['private_feasible']]
assert result['active_feasible_candidates']==len(feasible)
publications=[x for x in rows if x['stage']=='saved_correction_publication'];assert len(publications)==int(result['published'])
winner=None;halo_after=None
if result['published']:
    publication=publications[0];assert publication['context'] in active_labels
    support=next(x for x in rows if x['stage']=='saved_correction_publication_support')
    assert removed==set(support['removed_cell_ids']) and added==set(support['added_cell_ids'])
    new_ids=newnodes.keys()-oldnodes.keys();gone=oldnodes.keys()-newnodes.keys()
    eligible=[x for x in feasible if x['context']==publication['context'] and x['operator']==publication['operator']]
    for trial in eligible:
        target={x['vertex'] for x in rows if x['stage']=='saved_correction_target' and x['slot']==trial['slot'] and x['side']=='fresh'}
        if (new_ids and new_ids<=target) or (not new_ids and trial['vertex'] in gone):winner=trial;break
    assert winner is not None and len(removed)==winner['old_cells'] and len(added)==winner['new_cells']
    assert winner['new_geometry_excess']<=winner['old_geometry_excess']
    assert publication['geometry_bad']==winner['new_geometry_bad']
    context=next(x for x in contexts if x['context']==winner['context'])
    halo_after=dict(cells=context['halo_cells']-winner['old_cells']+winner['new_cells'],
        q_bad=context['halo_q_bad']+winner['new_q_bad']-winner['old_q_bad'],
        J_bad=context['halo_J_bad']+winner['new_J_bad']-winner['old_J_bad'],
        geometry_bad=context['halo_geometry_bad']+winner['new_geometry_bad']-winner['old_geometry_bad'])
report=dict(case=str(case),status=status,pins_verified=len(pins),all_actual_handles_terminal=True,
    planned_context_slots=dict(counts),constructor_calls=result['constructor_calls'],active_admissible_candidates=len(feasible),
    trials=trials,contexts=contexts,result=result,winner=winner,publication=publications[0] if publications else None,
    post_publication_halo_from_complete_delta=halo_after,working_cells=len(after),working_nodes=len(newnodes),
    removed_cells=len(removed),added_cells=len(added),untouched_cells=len(shared),untouched_rows_bitidentical=True,
    retired_nodes=sorted(oldnodes.keys()-newnodes.keys()),new_nodes=sorted(newnodes.keys()-oldnodes.keys()),
    all_retained_coordinates_and_sensors_unchanged=True,input_mesh_unchanged=True,
    working_owned_sha256=sha(case/'trajectory_working_owned.bin'),working_parameters_sha256=sha(case/'trajectory_working_parameters.bin'),
    assessment_script_sha256=sha(__file__),geometry_bad_retirement=winner['old_geometry_bad']-winner['new_geometry_bad'] if winner else None,
    geometry_excess_reduction_fraction=(1-winner['new_geometry_excess']/winner['old_geometry_excess']) if winner and winner['old_geometry_excess']>0 else None,
    production_source_changed=False,production_integration_justified=False,first_row_ready=False,goal_complete=False,
    limitations=['Exact3directed candidates on complete currentstars, not live selector/reachability or generalized cavity equivalence.',
        'Upper-bound screen/excess is not proof of physical violation or final bidirectional certification.',
        'No wholefield closure; q/J diagnostic only, strictfirstheight not achieved.',
        'Controlprimitive observational; no corrected-coarsening or MPIcandidate usefulness proved.',
        'Matched patchdelta plus complete stars/unchanged outside rows gives halo effect; no new global certification.'])
if not result['published']:
    assert sha(case/'trajectory_working_owned.bin')==inputs['owned']['sha256']
    assert sha(case/'trajectory_working_parameters.bin')==inputs['parameters']['sha256']
edge_rows=[x for x in rows if x['stage']=='saved_correction_edge']
report['surface_planning_edge_distributions']=[]
for trial in trials:
    for side in ('old','fresh'):
        lengths=sorted(x['length'] for x in edge_rows if x['slot']==trial['slot'] and x['side']==side)
        if lengths:
            report['surface_planning_edge_distributions'].append(dict(slot=trial['slot'],side=side,count=len(lengths),
                minimum=lengths[0],maximum=lengths[-1],median=lengths[len(lengths)//2],
                below_point5=sum(x<.5 for x in lengths),below_point9=sum(x<.9 for x in lengths),above1point8=sum(x>1.8 for x in lengths)))
report['edge_measurement_scope']='Exact production SurfacePlanning lengths; not full composed-volume edge certification or a standalone directional conformity certificate.'
(R/'directed_surface487_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ('active_admissible_candidates','working_cells','working_nodes','removed_cells','added_cells','retired_nodes','new_nodes','geometry_bad_retirement','geometry_excess_reduction_fraction','post_publication_halo_from_complete_delta')},indent=2))
