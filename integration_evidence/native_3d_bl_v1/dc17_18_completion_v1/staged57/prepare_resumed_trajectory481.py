"""Derive continuation identities and complete edge stars from the actual published move."""
import importlib.util,json,hashlib
from pathlib import Path
R=Path(__file__).resolve().parent
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
a=json.loads((R/'saved_surface_corrections481_assessment.json').read_text());case=Path(a['case'])
owned=case/'trajectory_working_owned.bin';parameters=case/'trajectory_working_parameters.bin'
assert sha(owned)==a['working_owned_sha256'] and sha(parameters)==a['working_parameters_sha256']
trials=[json.loads(l) for l in (case/'actual_bl_cavity_planning.jsonl').read_text().splitlines()]
ids=lambda side:{r['vertex'] for r in trials if r['stage']=='saved_correction_target' and r['slot']==a['winner']['slot'] and r['side']==side}
retired=ids('old')-ids('fresh');replacement=ids('fresh')-ids('old');assert retired=={a['winner']['vertex']} and len(replacement)==1
old=next(iter(retired));new=next(iter(replacement));seeds=[dict(label='current_rank4_guidance',edge=[new,24439],cells=[]),dict(label='current_rank5_guidance',edge=[67,68],cells=[])]
spec=importlib.util.spec_from_file_location('native_inspection',R/'inspect_native_failure3d.py');reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
new_star=0
for row in reader._read_owned(owned,a['working_cells'],2_000_000_000):
    vertices={n['id'] for n in row['nodes']};assert old not in vertices
    new_star+=new in vertices
    for seed in seeds:
        if set(seed['edge'])<=vertices:seed['cells'].append(sorted(vertices))
assert new_star==a['winner']['new_cells']
for seed in seeds:seed['cells'].sort();assert 0<len(seed['cells'])<=64
output=R/'current_saved_trajectory481_resumed_contexts.dat'
output.write_text(str(len(seeds))+'\n'+''.join(f"{s['label']} {s['edge'][0]} {s['edge'][1]} {len(s['cells'])}\n"+'\n'.join(' '.join(map(str,c)) for c in s['cells'])+'\n' for s in seeds))
receipt=json.loads((R/'current_saved_trajectory481_inputs.json').read_text());receipt['contexts']=dict(path=str(output),sha256=sha(output));receipt['seeds']=seeds;receipt['identity_correction']=dict(retired=old,replacement=new,complete_replacement_star_cells=new_star,source='Published target witnesses plus full saved Owned scan; native Directory rechecks support before mutation.',preserved_failed_input='current_saved_trajectory481_inputs.json')
(R/'current_saved_trajectory481_resumed_inputs.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt['identity_correction']))
