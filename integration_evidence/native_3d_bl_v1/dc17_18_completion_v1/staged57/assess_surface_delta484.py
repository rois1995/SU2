"""Compare preserved physical surface and untouched cells; no new certification or constructors."""
import json
from pathlib import Path
from assess_surface_transaction470 import sha,owned_rows
from inspect_native_failure3d import _read_owned
R=Path(__file__).resolve().parent
a=json.loads((R/'saved_production_surface484_serial_assessment.json').read_text())
i=json.loads((R/'saved_production_surface484_inputs.json').read_text())
old=Path(i['owned']['path']);new=Path(a['shards'][0]['files']['owned']['path'])
assert sha(old)==i['owned']['sha256'] and sha(new)==a['shards'][0]['files']['owned']['sha256']
before=dict(owned_rows(old));after=dict(owned_rows(new))
surviving=before.keys()&after.keys();changed=[key for key in surviving if before[key]!=after[key]]
assert not changed,'Unexpected modification outside newly allocated replacements'
vertices=(28651505,24439,67,68,6070,6072,7594)
def inspect(path,count):
    nodes={};faces=set();stars={key:[] for key in vertices}
    for row in _read_owned(path,count,2_000_000_000):
        v=row['nodes'];layout=((1,2,3),(0,3,2),(0,1,3),(0,2,1))
        for k,node in enumerate(v):
            value=(tuple(node['xyz']),tuple(row['sensor'][6*k:6*k+6]));assert nodes.setdefault(node['id'],value)==value
            if node['id'] in stars:stars[node['id']].append(row['id'])
        for binding,slots in zip(row['physical'],layout):
            if not binding['present']:continue
            face=tuple(v[k]['id'] for k in slots);index=face.index(min(face));face=face[index:]+face[:index]
            key=(face,binding['facet']);assert key not in faces;faces.add(key)
    return nodes,faces,stars
oldnodes,oldfaces,oldstars=inspect(old,i['cells']);newnodes,newfaces,newstars=inspect(new,a['output_cells'])
coordinate_changes=[];tensor_changes=0;max_tensor_relative=0.
for key in oldnodes.keys()&newnodes.keys():
    if oldnodes[key][0]!=newnodes[key][0]:coordinate_changes.append(key)
    if oldnodes[key][1]!=newnodes[key][1]:
        tensor_changes+=1;baseline=max(abs(x) for x in oldnodes[key][1]);assert baseline>0
        max_tensor_relative=max(max_tensor_relative,max(abs(x-y) for x,y in zip(oldnodes[key][1],newnodes[key][1]))/baseline)
result=a['shards'][0]['result'];assert result['splits']==len(newnodes.keys()-oldnodes.keys())==86
assert not coordinate_changes and not (oldnodes.keys()-newnodes.keys())
removed=oldfaces-newfaces;added=newfaces-oldfaces;assert removed and added
report=dict(input_sha256=sha(old),output_sha256=sha(new),assessment_script_sha256=sha(__file__),
 unchanged_cells_byte_identical=len(surviving),removed_cells=len(before.keys()-after.keys()),new_cells=len(after.keys()-before.keys()),
 input_nodes=len(oldnodes),output_nodes=len(newnodes),new_nodes=len(newnodes.keys()-oldnodes.keys()),retired_nodes=0,
 changed_existing_coordinates=coordinate_changes,changed_existing_nodal_tensors=tensor_changes,maximum_relative_tensor_change=max_tensor_relative,
 input_physical_faces=len(oldfaces),output_physical_faces=len(newfaces),physical_faces_removed=len(removed),physical_faces_added=len(added),
 physical_surface_changed=True,skin_preserving_volume_only=False,
 representative_face_changes=dict(removed=sorted(removed)[:16],added=sorted(added)[:16]),
 legacy_vertex_star_inventory={key:dict(input_cells=len(oldstars[key]),output_cells=len(newstars[key]),cell_ids_unchanged=set(oldstars[key])==set(newstars[key]),point=newnodes[key][0] if key in newnodes else None) for key in vertices},
 limits=['Physical surface change does not prove geometry closure or net bad-cell removal.',
 'Unknown/unmeasured rows prevent whole-field q/J/geometry comparison; native positive-cell checks and full target preservation are separate evidence.',
 'Legacy vertex inventory is support discovery only; it does not certify current control cleanliness or replacement feasibility.'],
 final_mesh_accepted=False,goal_complete=False)
(R/'surface_delta484_assessment.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ('unchanged_cells_byte_identical','removed_cells','new_cells','new_nodes','physical_faces_removed','physical_faces_added','changed_existing_nodal_tensors')},indent=2))
