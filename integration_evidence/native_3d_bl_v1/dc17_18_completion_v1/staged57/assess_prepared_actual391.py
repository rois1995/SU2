"""Preserve original-chord outcome, including refusals; no mesh capability claim."""
import json,re
from pathlib import Path
R=Path(__file__).resolve().parent
case=sorted((R/'cases').glob('curved391_target_geometry_*'),key=lambda p:p.stat().st_mtime)[-1]
status=json.loads((case/'status.json').read_text());assert status['state']=='terminal' and status['pins_verified']
terminal=json.loads((case/'terminal_handles.json').read_text());assert terminal['all_owned_handles_terminal']
records=[];unparsed=[]
for line in (case/'actual_bl_front.jsonl').read_text().splitlines():
 try:records.append(json.loads(line))
 except json.JSONDecodeError:unparsed.append(line)
row=lambda tag: next((x for x in records if x.get('stage')==tag),{})
counters=row('existing_counters');geometry=row('existing_geometry_counters')
assert counters and geometry
full=row('saved_target_geometry_complete')
length=next((x.get('composed_edge_length') for x in records if 'composed_edge_length' in x),None)
prior=json.loads((R/'geometry_target388_assessment.json').read_text())['comparison']
current=dict(preparation_seconds=counters['edge_geometry_seconds'],integrand_evaluations=counters['field']['edge_compositions'],
             composition_seconds=counters['edge_composition_seconds'])
log=(case/'run.log').read_text();times={}
for label in ['User time (seconds)','System time (seconds)']:
 match=re.search(re.escape(label)+r':\s*([\d.]+)',log)
 if match:times[label]=float(match.group(1))
result=dict(case=str(case),status=status,terminal=terminal,records=records,unparsed_refusal_lines=unparsed,
            comparison=dict(**prior,current391=current),chord_completed=length is not None,chord_length=length,
            observed_process_cpu=times,source_mesh=dict(points=61330,tetrahedra=303359),adapted_mesh=None,
            limitation='Same saved seed/core/scales/constraints/limits.391reuses competitive prepared supports for bounded certified upper witnesses; integrand/cut trajectory can change. Nested timers not additive. No constructed patch/full target checkpoint/accepted BL.',
            numerical_reference='Existing production numerical integration unchanged; no fixed10 substitution or classification estimate.')
(R/'geometry_target391_assessment.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(status=status,comparison=result['comparison'],chord_completed=result['chord_completed'],chord_length=length,cpu=times),indent=2))
