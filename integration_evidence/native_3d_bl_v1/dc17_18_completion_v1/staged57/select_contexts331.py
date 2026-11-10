"""Select bounded complete M6 edge/vertex neighborhoods from the preserved input; no remeshing."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
R=Path(__file__).resolve().parent;repo=R.parents[3]
sys.path.insert(0,str(repo/'TestCases/adaptation/capability'));import capcheck
mesh_path=R/'m6_bl_inputs/rans_curved303_v1/m6.su2'
m=capcheck.read_su2(mesh_path);faces=m.M['WING3D'];P=m.P
edges={}
for i,f in enumerate(faces):
    for j in range(3):edges.setdefault(tuple(sorted((int(f[j]),int(f[(j+1)%3])))),[]).append(i)
centres=P[faces].mean(axis=1)
raw=np.cross(P[faces[:,1]]-P[faces[:,0]],P[faces[:,2]]-P[faces[:,0]])
normals=raw/np.linalg.norm(raw,axis=1)[:,None]
w=json.loads((R/'m6_bl_inputs/chord331_from330_v1/witness.json').read_text())
ids=[int(np.argmin(np.linalg.norm(P-np.array(x),axis=1))) for x in (w['a'],w['b'])]
assert all(np.linalg.norm(P[i]-x)<1e-13 for i,x in zip(ids,(w['a'],w['b'])))
requests=[('actual_failed_chord',tuple(sorted(ids)),None),('smooth_upper',None,(.65,.6,.03)),('leading_edge',None,(.18,.6,0)),('symmetry_wall_interface',None,(.55,0,.02)),('sharp_edge_candidate',None,(.95,.6,0))]
result=[]
for label,prescribed,target in requests:
    candidates=[prescribed] if prescribed else sorted(edges,key=lambda e:np.linalg.norm(P[list(e)].mean(axis=0)-target))
    for e in candidates:
        if e not in edges:continue
        p=P[list(e)].mean(axis=0);adj=edges[e]
        sharp=len(adj)==2 and float(normals[adj[0]]@normals[adj[1]])<.5
        if label=='sharp_edge_candidate' and not sharp:continue
        if label=='symmetry_wall_interface' and max(abs(P[i,1]) for i in e)>1e-10:continue
        if label in ('smooth_upper','leading_edge') and (sharp or p[1]<.2 or p[1]>1.05):continue
        if label=='smooth_upper' and p[2]<=0:continue
        star=np.flatnonzero(np.any(np.isin(m.E,e),axis=1))
        if len(star)>64:continue
        edge_star=np.flatnonzero(np.sum(np.isin(m.E,e),axis=1)==2)
        nodes=np.unique(m.E[star]);near=np.flatnonzero(np.any(np.isin(faces,e),axis=1))
        result.append(dict(label=label,edge=list(e),coordinates=P[list(e)].tolist(),complete_endpoint_star_cell_ordinals=star.tolist(),complete_edge_star_cell_ordinals=edge_star.tolist(),surface_face_ordinals=near.tolist(),node_count=len(nodes),cell_count=len(star),edge_star_cells=len(edge_star),reference_support='Complete immutable original native_ref; do not truncate support to this patch.',feature_status='Sharp-angle screening only; native feature/association policy must classify before a test.' if sharp else 'Native association/feature classification still required.',source_cell_ordinals_are_not_native_cell_ids=True))
        break
    else:raise RuntimeError('No bounded complete candidate for '+label)
assert len(result)==len(requests)
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
out=dict(scope='Original actual M6 geometry/connectivity contexts only; no generated BL or quality/metric qualification.',pins={str(p):sha(p) for p in (mesh_path,Path(__file__),repo/'TestCases/adaptation/capability/capcheck.py')},contexts=result)
(R/'m6_bl_inputs/chord331_from330_v1/actual_contexts.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps([dict(label=x['label'],edge=x['edge'],cells=x['cell_count'],edge_cells=x['edge_star_cells']) for x in result],indent=2))
