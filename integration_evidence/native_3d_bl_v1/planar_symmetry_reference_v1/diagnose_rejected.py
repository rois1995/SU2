#!/usr/bin/env python3
"""Read-only structural diagnosis of the saved annular failure, not an infeasibility certificate."""
import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
folder=HERE/'annulus_v4_n1';stem='symmetry_annulus_bl_budget_20_m1';prefix=stem+'_rejected_rank0'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
working=json.loads((folder/(prefix+'.json')).read_text());trial=json.loads((folder/(prefix+'_trial_after.json')).read_text())
nodes={v[0]:v[1:] for v in working['nodes']};fresh={v[0]:v[1:] for v in trial['nodes']}
info=json.loads((folder/(prefix+'_trial.json')).read_text());edge=info['vertices'][:2]
star=[c for c in working['cells'] if set(edge).issubset(c['nodes'])]
weakest=next(c for c in trial['cells'] if c['id']==info['weakest_cell'])
witness=json.loads((HERE/'feasibility_witness_v1/report.json').read_text())
assert sha(folder/(stem+'_initial.json'))==witness['original_source_sha256']
assert sha(folder/(stem+'_frozen_profile.json'))==witness['original_profile_sha256']
report={'status':'SAVED_NATIVE_REJECTION_WITH_INDEPENDENT_FEASIBLE_TARGET','edge':edge,
 'edge_endpoints':[nodes[n] for n in edge],'terminal_edge_star_cells':[c['id'] for c in star],
 'last_trial':info,'weakest_trial_cell':{'id':weakest['id'],'vertices':[[n]+fresh[n] for n in weakest['nodes']]},
 'same_original_field_and_profile_as_independent_witness':True,
 'diagnosis':'Resource ceiling reached during local construction; observed transition split fails shape/sliver gates. A feasible same-field mesh exists. This is not invalid geometry or an infeasibility proof.',
 'next_capability':'Coordinated finite-surface point distribution/reconnection together with first-row/volume transition reconstruction. Preserve fixed junctions and validate the full 3D replacement. An independent surface remesh alone proves no volume feasibility.',
 'limits':['Last observed trial is a sample, not proof that every proposal is blocked','No claim that larger work caps, more ranks, a rotated plane or another local fallback fixes construction'],
 'inputs':{p.name:sha(p) for p in [folder/(prefix+'.json'),folder/(prefix+'_trial_after.json'),folder/(prefix+'_trial.json'),HERE/'feasibility_witness_v1/report.json',Path(__file__)]}}
(folder/'structural_diagnosis.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
