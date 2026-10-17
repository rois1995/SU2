"""Whole saved-M6 geometric feasibility for a bulk layer-guide construction; no remeshing claim."""
import hashlib,json,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
CASE=HERE.parent/'first_row_shape_ranking_v1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
start=time.monotonic();m=read_su2(CASE/'Coarse.su2');h=2e-6
print('loaded',len(m.P),len(m.E),{k:len(v) for k,v in m.M.items()},flush=True)
wall=m.M['WING3D'];wall_nodes=np.unique(wall);onwall=np.zeros(len(m.P),dtype=bool);onwall[wall_nodes]=True
# Sorted boundary keys preserve source cell IDs independent of file face orientation.
lookup={tuple(sorted(map(int,f))):i for i,f in enumerate(wall)};owner=np.full(len(wall),-1,dtype=np.int64)
faces=np.array([[1,2,3],[0,3,2],[0,1,3],[0,2,1]])
wall_cells={}
for cid,ids in enumerate(m.E):
 if onwall[ids].sum()<3:continue
 for opp,local in enumerate(faces):
  index=lookup.get(tuple(sorted(map(int,ids[local]))))
  if index is not None:
   assert owner[index]<0;owner[index]=cid;wall_cells.setdefault(cid,[]).append((index,int(ids[opp])))
assert np.all(owner>=0)
fields=[];failures=[];max_ratio=[];vol=[]
for cid,rows in wall_cells.items():
 tet=m.P[m.E[cid]];center=tet.mean(axis=0);plane=[]
 for index,apex_id in rows:
  x=m.P[wall[index]];normal=np.cross(x[1]-x[0],x[2]-x[0]);normal/=np.linalg.norm(normal)
  if normal@(m.P[apex_id]-x[0])<0:normal=-normal
  height=normal@(m.P[apex_id]-x[0]);center_height=normal@(center-x[0])
  q=x.mean(axis=0)+h*normal
  bary=np.linalg.solve((tet[1:]-tet[0]).T,q-tet[0]);bary=np.r_[1-bary.sum(),bary]
  plane.append(center_height/h);fields.append((index,cid,apex_id,height,center_height,float(bary.min()),*q))
  if bary.min()<=0:failures.append(dict(face=index,cell=cid,wall_nodes=list(map(int,wall[index])),minimum_barycentric=float(bary.min()),apex=q.tolist(),height=float(height)))
 max_ratio.append(max(plane)/min(plane));vol.append(abs(np.linalg.det((tet[1:]-tet[0]).T))/6)
a=np.array(fields);np.savez_compressed(HERE/'wall_geometry.npz',faces=wall,owners=owner,measurements=a)
q=lambda x:{str(p):float(np.quantile(x,p)) for p in (0,.01,.1,.5,.9,.99,1)}
report=dict(scope='Original entire M6 geometry only. Normal apices are proposals, not a constructed/accepted mesh or proof of metric feasibility.',
 points=len(m.P),tetrahedra=len(m.E),markers={k:len(v) for k,v in m.M.items()},wall_nodes=len(wall_nodes),
 wall_cells=len(wall_cells),wall_faces_per_cell={str(i):sum(len(r)==i for r in wall_cells.values()) for i in (1,2,3,4)},
 original_wall_vertices_per_wall_cell={str(i):sum(int(onwall[m.E[c]].sum())==i for c in wall_cells) for i in (3,4)},
 first_height=h,wall_tet_altitude=q(a[:,3]),centroid_altitude_over_first_height=q(a[:,4]/h),
 multiple_wall_face_centroid_height_ratio=q(np.array(max_ratio)),normal_apex_minimum_barycentric=q(a[:,5]),
 normal_apex_outside_its_coarse_tet=len(failures),outside_examples=failures[:32],
 proposed_first_guide_level=1.5,wall_cones_without_room_at_centroid=int(np.sum(a[:,4]/h<=1.5)),
 input_sha256={str((CASE/p).relative_to(ROOT)):sha(CASE/p) for p in ('Coarse.su2','run.cfg','Coarse.su2.native_ref','sensor_original.bin')},
 helper_sha256=sha(Path(__file__)),seconds=time.monotonic()-start,CFD_steps=0)
(HERE/'geometry_summary.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');print(json.dumps(report,indent=2),flush=True)
