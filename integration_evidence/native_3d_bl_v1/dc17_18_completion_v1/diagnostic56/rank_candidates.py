"""Independent full-patch centroid quality for geometrically constructed captured candidates."""
import importlib.util,json,sys
from pathlib import Path
import numpy as np
from analyze import Reader,read
from run import ROOT,save
sys.path.insert(0,str(ROOT.parent/'diagnostic55'))
spec=importlib.util.spec_from_file_location('witness55',ROOT.parent/'diagnostic55/witness_analysis.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
folder=ROOT/'capture';r=Reader(read(folder/'transaction_rank2_donors.bin'));nodes={};cells=[]
for _ in range(r.count(65536)):
    cid,vertices=r.cell();metrics=[r.take('dddddd') for _ in range(4)];extension,parent=r.take('QQ')
    cells.append([n[0] for n in vertices])
    for n,m in zip(vertices,metrics):
        xx,xy,xz,yy,yz,zz=m;value=(n[1:],(xx,xy,xz,xy,yy,yz,xz,yz,zz))
        assert n[0] not in nodes or nodes[n[0]]==value
        nodes[n[0]]=value
r.end();ids=sorted(nodes);index={v:i for i,v in enumerate(ids)}
field=object.__new__(module.Original3D)
field.points=np.asarray([nodes[n][0] for n in ids],dtype=np.longdouble)
field.tensors=np.asarray([nodes[n][1] for n in ids],dtype=np.longdouble)
field.cells=np.asarray([[index[n] for n in c] for c in cells]);field.cache={};field.candidates=lambda p:range(len(cells))
report=json.loads((folder/'transaction_rank2.json').read_text());rows=[]
for i,event in enumerate(report['events']):
    if event['stage'] not in ('SplitEdge','ReconstructSplitPatch') or not event['passed']:continue
    reader=Reader(read(folder/('transaction_rank2_event%d_fresh.bin'%i)));shapes=[]
    for _ in range(reader.count(128)):
        cid,vertices=reader.cell();p=np.array([n[1:] for n in vertices],dtype=np.longdouble)
        center=tuple(np.asarray(np.mean(p,axis=0),dtype=float));shapes.append(module.shape(p,field(center)))
    q=float(min(x[0] for x in shapes));j=float(min(x[1] for x in shapes))
    rows.append(dict(event=i,stage=event['stage'],cells=len(shapes),q=q,J=j,score=min(q/.2,j/.05)))
rows.sort(key=lambda r:-r['score']);save(ROOT/'candidate_ranking.json',rows)
for row in rows:print(row)

# A necessary condition only: every fixed skin triangle needs some admissible tetrahedron apex.
# Enumerating all existing vertices even allows exterior/overlapping cells, so failure is informative;
# success would not establish a conforming filling, contact, geometry or edge acceptance.
from itertools import combinations
from collections import Counter
selected=json.loads((ROOT/'selected_trials.json').read_text())['events'][1]['fresh']
vertices={n[0]:n[1:] for _,tet in selected['cells'] for n in tet}
incidence=Counter(tuple(sorted(n[0] for n in face)) for _,tet in selected['cells'] for face in combinations(tet,3))
assert all(count in (1,2) for count in incidence.values())
skin=sorted(face for face,count in incidence.items() if count==1)
assert len(vertices)==6
candidates=[]
for ids in combinations(sorted(vertices),4):
    xyz=np.asarray([vertices[n] for n in ids],dtype=np.longdouble)
    orientation=module.exact_volume(xyz)
    if orientation==0:continue
    if orientation<0:xyz[[0,1]]=xyz[[1,0]]
    row=dict(vertices=ids)
    try:
        q,j=module.shape(xyz,field(tuple(np.asarray(np.mean(xyz,axis=0),dtype=float))))
        row.update(q=float(q),J=float(j),shape_pass=bool(q>=.2 and j>=.05),source_covered=True)
    except ValueError as error:row.update(source_covered=False,reason=str(error))
    candidates.append(row)
faces=[]
for face in skin:
    attached=[c for c in candidates if set(face).issubset(c['vertices'])]
    admissible=[c['vertices'] for c in attached if c.get('shape_pass')]
    unknown=[c['vertices'] for c in attached if not c['source_covered']]
    faces.append(dict(face=face,admissible_existing_vertex_tets=admissible,unknown_source_tets=unknown,
                      no_admissible_existing_vertex_tet=not admissible and not unknown))
result=dict(seed_event=1,vertices=len(vertices),nondegenerate_candidate_tets=candidates,skin_faces=faces,
            blocked_faces=sum(f['no_admissible_existing_vertex_tet'] for f in faces),
            permits_added_vertices=False,proves_general_infeasibility=False,
            source_pins={name:__import__('hashlib').sha256((ROOT/name).read_bytes()).hexdigest()
                         for name in ['selected_trials.json','selected_donors.json','rank_candidates.py']})
save(ROOT/'fixed_vertex_skin_check.json',result)
print('Fixed-vertex necessary condition:',result['blocked_faces'],'of',len(faces),'skin faces lack any shape-admissible apex.')
