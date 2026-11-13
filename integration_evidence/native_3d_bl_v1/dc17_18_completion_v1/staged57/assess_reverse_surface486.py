"""Verify complete coverage and unchanged publication state of the data-only486 comparison."""
import json
from pathlib import Path
from assess_surface_transaction470 import sha,owned_rows
R=Path(__file__).resolve().parent
case=R/'cases/curved485_reverse_surface_corrections_serial_20261113T123419_3117385'
status=json.loads((case/'status.json').read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
rows=[json.loads(x) for x in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
trials=[x for x in rows if x['stage']=='saved_correction_trial']
contexts=[x for x in rows if x['stage']=='saved_correction_context']
result=next(x for x in rows if x['stage']=='saved_correction_result')
assert len(contexts)==3 and len(trials)==24 and [x['slot'] for x in trials]==list(range(1,25))
assert all(sum(x['context']==c['context'] for x in trials)==8 for c in contexts)
assert result['contexts_completed']==3 and result['constructor_calls']==24 and not result['published']
assert result['active_feasible_candidates']==0 and not any(t['private_feasible'] for t in trials[:16])
control=trials[22];assert control['private_feasible'] and control['retained']==6072
inputs=json.loads((R/'reverse_surface486_inputs.json').read_text())
for name in ('owned','parameters','target','prepared_reference','contexts'):
 item=inputs[name];assert sha(item['path'])==item['sha256']
for filename,name in [('trajectory_working_owned.bin','owned'),('trajectory_working_parameters.bin','parameters'),('correction_unchanged_owned.bin','owned')]:
 assert sha(case/filename)==inputs[name]['sha256']
assert len(dict(owned_rows(case/'trajectory_working_owned.bin')))==303823
report=dict(case=str(case),status=status,pins_verified=len(pins),actual_handles_terminal=True,
 contexts=contexts,trials=trials,result=result,input_state_preserved=True,
 working_cells=303823,working_nodes=61550,working_owned_sha256=inputs['owned']['sha256'],
 working_parameters_sha256=inputs['parameters']['sha256'],assessment_sha256=sha(__file__),
 production_changed=False,first_row_ready=False,production_integration_justified=False,
 exact_nominated_root_collapse_tested=False,control_feasible=True,
 limitations=['Shortest-incident chooser did not test nominated17814→64065077.',
 'Geometry screens/excess are conservative upper estimates, not demonstrated physical violations.',
 'Mean movement and skin-preserving reconnect are narrower than generalized cavity methods.',
 'No smooth-midspan residual, distributed candidate capability, strict heights or final closure proved.'])
(R/'reverse_surface486_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ('pins_verified','input_state_preserved','exact_nominated_root_collapse_tested','control_feasible')},indent=2))
