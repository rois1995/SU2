"""Nominate current free-side/root and actual failed-vertex stars without changing source or target."""
import json,math
from pathlib import Path
from assess_surface_transaction470 import sha
from inspect_native_failure3d import _read_owned
R=Path(__file__).resolve().parent
assessment=R/'saved_surface_corrections485_assessment.json';a=json.loads(assessment.read_text());assert a['result']['published'] and a['retired_nodes']==[67] and len(a['new_nodes'])==1
case=Path(a['case']);root=a['new_nodes'][0];owned=case/'trajectory_working_owned.bin';parameters=case/'trajectory_working_parameters.bin'
assert sha(owned)==a['working_owned_sha256'] and sha(parameters)==a['working_parameters_sha256']
assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
selected={7594,17814,root,6070,6072};stars={key:[] for key in selected};physical=set();positions={};retired_seen=False
layout=((1,2,3),(0,3,2),(0,1,3),(0,2,1))
for row in _read_owned(owned,a['working_cells'],2_000_000_000):
    v=row['nodes'];ids=tuple(sorted(n['id'] for n in v));retired_seen=retired_seen or 67 in ids
    for n in v:positions.setdefault(n['id'],tuple(n['xyz']))
    for key in set(ids)&selected:stars[key].append((row['id'],ids))
    if 7594 in ids:
        for binding,face in zip(row['physical'],layout):
            members={v[k]['id'] for k in face}
            if binding['present'] and 7594 in members:physical.update(members-{7594})
assert not retired_seen and physical
second=min(physical,key=lambda key:(math.dist(positions[7594],positions[key]),key))
# Complete the second nominated star in another bounded read, without any geometry query or constructor.
if second not in stars:
    stars[second]=[]
    for row in _read_owned(owned,a['working_cells'],2_000_000_000):
        ids=tuple(sorted(n['id'] for n in row['nodes']))
        if second in ids:stars[second].append((row['id'],ids))
pairs=[('actual_tip_7594',7594,second),('free_side_to_root',17814,root),('clean_control',6070,6072)]
blocks=['3'];seeds=[]
for label,first,last in pairs:
    union=dict(stars[first]+stars[last]);assert 0<len(union)<=64 and stars[first] and stars[last]
    keys=sorted(union.values());assert len(keys)==len(set(keys))
    blocks.append(f'{label} {first} {last} {len(keys)}');blocks.extend(' '.join(map(str,key)) for key in keys)
    seeds.append(dict(label=label,vertices=[first,last],union_cells=len(keys),edge_cells=sum(first in ids and last in ids for ids in keys),cell_ids=sorted(union)))
context=R/'reverse_surface486_contexts.dat';context.write_text('\n'.join(blocks)+'\n')
adapter=R/'surface486_current_state';adapter.mkdir()
for key,path in [('owned',owned),('parameters',parameters)]:(adapter/f'trajectory_working_{key}.bin').symlink_to(path.resolve())
(adapter/'terminal_handles_actual.json').symlink_to((case/'terminal_handles_actual.json').resolve())
receipt=json.loads((R/'surface_residual485_inputs.json').read_text());receipt.update(
    owned=dict(path=str(owned.resolve()),sha256=sha(owned)),parameters=dict(path=str(parameters.resolve()),sha256=sha(parameters)),
    contexts=dict(path=str(context.resolve()),sha256=sha(context)),capture_adapter=str(adapter.resolve()),cells=a['working_cells'],nodes=a['working_nodes'],
    source_assessment=dict(path=str(assessment.resolve()),sha256=sha(assessment)),seeds=seeds,
    source_qualification=dict(path=str(assessment.resolve()),sha256=sha(assessment)),
    hypothesis='Test removal of free-side point rather than constrained root; preserve485published root move, also sample actual failed7594vertex and cleancontrol.',
    tip_second_selection='Closest current physical neighbor by geometry only, not claim exact last failed edge or production metric ranking.',
    limitations=['Shortestincident proposal may choose a different retained point; inspect actual chosen direction before drawing a conclusion.',
        'No contraction allowedness or replacement feasibility inferred from positions; native guards decide.',
        'Same fixed24slots/300geometry/600outer; no source/build/target or physical constraint change.'])
receipt['previous_inventory']=receipt.pop('source_inventory')
receipt['source_checkpoint']=dict(path=str((R/'saved_surface485_checkpoint.json').resolve()),sha256=sha(R/'saved_surface485_checkpoint.json'))
(R/'reverse_surface486_inputs.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(root_replacement=root,seeds=seeds),indent=2))
