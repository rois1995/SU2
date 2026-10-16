#!/usr/bin/env python3
"""Independent geometry-only experiment: feature-plane partition, never an SU2 remesh."""
from pathlib import Path
import csv,json,sys,itertools
import numpy as np
from scipy.spatial import ConvexHull
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
m=read_su2(ROOT/'integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/Coarse.su2')
cell=m.P[np.sort(m.E[13])];allfaces=m.M['WING3D'];fulltri=m.P[allfaces]
radius=np.linalg.norm(cell[:,None,:]-cell[None,:,:],axis=2).max()
# A wall vertex bounds every source point's nearest distance by the cell diameter.
assert np.any(np.linalg.norm(fulltri[:,0,None,:]-cell[None,:,:],axis=2)==0)
keep=np.all(fulltri.max(1)>=cell.min(0)-radius,axis=1)&np.all(fulltri.min(1)<=cell.max(0)+radius,axis=1)
ids=np.flatnonzero(keep);tri=fulltri[keep];a=tri[:,0];ab=tri[:,1]-a;ac=tri[:,2]-a
normal=np.cross(ab,ac);normal/=np.linalg.norm(normal,axis=1)[:,None]
d00=np.einsum('ij,ij->i',ab,ab);d01=np.einsum('ij,ij->i',ab,ac);d11=np.einsum('ij,ij->i',ac,ac);det=d00*d11-d01*d01

def nearest(p):
 r=p-a;off=np.einsum('ij,ij->i',r,normal)
 q0=np.einsum('ij,ij->i',r,ab);q1=np.einsum('ij,ij->i',r,ac);u=(q0*d11-q1*d01)/det;v=(q1*d00-q0*d01)/det
 inside=(u>=0)&(v>=0)&(u+v<=1);d2=np.where(inside,off*off,np.inf)
 for k in range(3):
  start=tri[:,k];edge=tri[:,(k+1)%3]-start;t=np.clip(np.einsum('ij,ij->i',p-start,edge)/np.einsum('ij,ij->i',edge,edge),0,1)
  dr=p-start-t[:,None]*edge;d2=np.minimum(d2,np.einsum('ij,ij->i',dr,dr))
 f=int(np.argmin(d2));return float(np.sqrt(max(0,d2[f]))),f

def world(p):return cell[0]+p@(cell[1:]-cell[0])
def defect(t):
 nodal=np.array([nearest(p)[0] for p in world(t)]);worst=0
 for mask in range(1,16):
  selected=[k for k in range(4) if mask&(1<<k)];p=t[selected].mean(0)
  worst=max(worst,abs(nearest(world(p))[0]-nodal[selected].mean()))
 return worst

reference=json.loads((HERE/'independent_distance_probes.json').read_text())
for row in reference:
 d,f=nearest(np.array(row['point']));assert abs(d-row['distance'])<2e-15
active=set()
for row in reference:
 d,f=nearest(np.array(row['point']))
 if d>64*np.finfo(float).eps*np.max(abs(cell)):active.add(f)
planes=[]
def add_plane(n,origin):
 coeff=np.r_[(cell[1:]-cell[0])@n,(cell[0]-origin)@n]
 scale=np.linalg.norm(coeff[:3])
 if not scale:return
 coeff/=scale
 nz=np.flatnonzero(abs(coeff)>1e-13)
 if len(nz) and coeff[nz[0]]<0:coeff=-coeff
 if not any(np.max(abs(coeff-p))<1e-11 for p in planes):planes.append(coeff)
for f in sorted(active):
 add_plane(normal[f],a[f])
 for k in range(3):
  edge=tri[f,(k+1)%3]-tri[f,k];add_plane(np.cross(edge,normal[f]),tri[f,k])
for first,second in itertools.combinations(sorted(active),2):
 # Both factors of squared face-plane distance equality; feature validity is tested separately.
 for sign in (-1,1):
  n=normal[first]+sign*normal[second]
  coeff=np.r_[(cell[1:]-cell[0])@n,(cell[0]-a[first])@normal[first]+sign*(cell[0]-a[second])@normal[second]]
  norm=np.linalg.norm(coeff[:3])
  if norm:
   coeff/=norm;nz=np.flatnonzero(abs(coeff)>1e-13)
   if len(nz) and coeff[nz[0]]<0:coeff=-coeff
   if not any(np.max(abs(coeff-p))<1e-11 for p in planes):planes.append(coeff)

pieces=[np.array([[0.,0,0],[1.,0,0],[0,1.,0],[0,0,1.]])]
for coeff in planes:
 nextpieces=[]
 for poly in pieces:
  value=poly@coeff[:3]+coeff[3];tol=128*np.finfo(float).eps*max(1,np.max(abs(poly)))
  if not (value.min() < -tol and value.max() > tol):nextpieces.append(poly);continue
  hull=ConvexHull(poly);edges=set()
  for face in hull.simplices:
   for i,j in itertools.combinations(face,2):edges.add(tuple(sorted((i,j))))
  intersections=[]
  for i,j in edges:
   if value[i]*value[j]<0:intersections.append(poly[i]+value[i]/(value[i]-value[j])*(poly[j]-poly[i]))
  for sign in (-1,1):
   points=np.vstack([poly[sign*value>=-tol],intersections]);h=ConvexHull(points);part=points[h.vertices]
   if h.volume>0:nextpieces.append(part)
 pieces=nextpieces
 assert len(pieces)<=4096
cells=[]
for poly in pieces:
 h=ConvexHull(poly);center=poly.mean(0)
 for face in h.simplices:
  t=np.vstack([center,poly[face]])
  if abs(np.linalg.det(t[1:]-t[0]))>1e-18:cells.append(t)
volume=sum(abs(np.linalg.det(t[1:]-t[0]))/6 for t in cells)
assert abs(volume-1/6)<2e-12
errors=[defect(t) for t in cells]
result={'scope':'Independent feature-plane partition experiment, geometry only, no production change or mesh generation.',
        'conservative_candidate_faces':len(ids),'active_probe_faces':[int(ids[f]) for f in sorted(active)],
        'feature_planes':len(planes),'convex_pieces':len(pieces),'tetrahedra':len(cells),'reference_volume':volume,
        'baseline_distance_defect':defect(np.array([[0.,0,0],[1.,0,0],[0,1.,0],[0,0,1.]])),
        'maximum_child_distance_defect':max(errors),'children_above_existing_tolerance':int(sum(x>2e-7 for x in errors)),
        'notes':'Probe faces/feature planes are proposals; sample agreement does not prove nearest-feature coverage or integration accuracy.'}
(HERE/'feature_partition_experiment.json').write_text(json.dumps(result,indent=2)+'\n')
np.savez_compressed(HERE/'feature_partition_cells.npz',cells=np.array(cells),defects=errors)
print(json.dumps(result,indent=2))
