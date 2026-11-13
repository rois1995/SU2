"""Assess the bounded connected trajectory without treating excess descent as closure."""
import json,collections
from pathlib import Path
from assess_surface_transaction470 import sha,terminal_receipt,owned_rows
R=Path(__file__).resolve().parent
case=max((R/'cases').glob('curved481_saved_connected_trajectory_resumed_serial_*'),key=lambda p:p.stat().st_mtime)
s=json.loads((case/'status.json').read_text());assert s['state']=='terminal' and s['exit_code']==0 and s['pins_verified']
pins=json.loads((case/'pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
t=terminal_receipt(case);(case/'terminal_handles_actual.json').write_text(json.dumps(t,indent=2)+'\n')
rows=[json.loads(l) for l in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()];contexts=[]
for row in rows:
    if row['stage']=='current_trajectory_context':contexts.append(dict(context=row,states=[],operations=[],results=[]))
    elif contexts and row['stage']=='transaction_state':contexts[-1]['states'].append(row)
    elif contexts and row['stage']=='transaction_operation':contexts[-1]['operations'].append(row)
    elif contexts and row['stage']=='transaction_result':contexts[-1]['results'].append(row)
assert len(contexts)==2 and sum(len(x['operations']) for x in contexts)==24
prefix=json.loads((R/'saved_surface_corrections481_assessment.json').read_text());source=Path(prefix['case']);old=dict(owned_rows(source/'trajectory_working_owned.bin'));new=dict(owned_rows(case/'trajectory_working_owned.bin'))
assert sha(case/'correction_unchanged_owned.bin')==prefix['working_owned_sha256']
assert sha(source/'trajectory_working_owned.bin')==prefix['working_owned_sha256'] and sha(source/'trajectory_working_parameters.bin')==prefix['working_parameters_sha256']
assert all(old[k]==new[k] for k in set(old)&set(new))
assert len(new)==304585 and len(new)-len(old)==64
operations=[v for c in contexts for v in c['operations']];assert all(v['published'] for v in operations)
result=dict(case=str(case),status=s,pins_verified=len(pins),actual_terminal_handles_verified=len(t['identities']),contexts=contexts,constructor_calls=24,published=24,operator_counts=dict(collections.Counter(v['operator'] for v in operations)),working_cells=len(new),working_owned_sha256=sha(case/'trajectory_working_owned.bin'),working_parameters_sha256=sha(case/'trajectory_working_parameters.bin'),untouched_cells=len(set(old)&set(new)),untouched_rows_bitidentical=True,prefix_input_unchanged=True,geometry_work=rows[-1]['geometry_work'],goal_complete=False,production_qualified=False,diagnosis='Bounded split-first continuation does not close geometry: all24publish, R4 local geometrybad delta+4 and q/J+64each despite~89percent excess decline; R5 receives0calls. Do not infer operator impossibility or global whole-field totals.',limitations=['Raw observed counts27to121include30newlyobservedoldcells; use local deltas, not raw counts as churn.','Belowhalf SurfacePlanning wall edges2of23to72of128; geometry may require short edges, no blanket minimum gate justified.','Firstrow/strictheightcoverage and final geometry/q/J/size acceptance unreached.','R5had8fixed comparison slots and3admitted moves, but0continuation constructors.','Fixed comparison24calls and corrected trajectory24calls are separate bounded experiments; failed stale-descriptor entry used0constructors.','No new MPI qualification; focused distant-body resource failure remains.'])
(R/'saved_connected_trajectory481_assessment.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ['pins_verified','actual_terminal_handles_verified','constructor_calls','published','operator_counts','working_cells','untouched_cells','geometry_work','diagnosis']}))
