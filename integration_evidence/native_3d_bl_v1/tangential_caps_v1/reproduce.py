"""Reproduce primary midpoint cap refinement and sorted-ID prism subdivision; diagnostic, not native acceptance."""
import hashlib,itertools,json,math,time
from pathlib import Path
import numpy as np
from scipy.integrate import quad
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
source=ROOT/'integration_evidence/native_3d_bl_v1/interacting_walls_v1/native_opposing_v3/opposing_finer_grouped_coupled_BL_independent.json'
data=json.loads(source.read_text());facets=[f for f in data['facets'] if f['marker']==1]
original={v[0]:np.array(v[1:3],float) for f in facets for v in f['vertices']};seed=[tuple(v[0] for v in f['vertices']) for f in facets]
assert len(seed)==2 and len(original)==4
h=.1;g=1.2;T=.3

def normal(z):
 hn=max(h,2*(h+.2*z)/2.2);t=min(1.,max(0.,(z-.27)/.03));w=1-t*t*(3-2*t)
 return max(4.,math.exp(w*math.log(1/(hn*hn))+(1-w)*math.log(4)))
def span(a,b):return quad(lambda z:math.sqrt(normal(z)),a,b,points=[x for x in (.05,.27,.3) if a<x<b],epsabs=1e-12,epsrel=1e-12)[0]
levels=[0.,h];total=span(h,1.);count=math.ceil(total/(.55*1.8))
for j in range(1,count):
 lo=levels[-1];hi=1.;want=total*j/count
 for k in range(24):
  mid=(lo+hi)/2;value=span(h,mid)
  if value<=want:lo=mid
  else:hi=mid
  if value<=want and want-value<=1e-7*total:break
 levels.append(lo)
levels.append(1.)

def measures(p,m):
 v=abs(float(np.linalg.det(p[1:]-p[0])))*math.sqrt(float(np.linalg.det(m)))
 edges={(i,j):float((p[j]-p[i])@m@(p[j]-p[i])) for i,j in itertools.combinations(range(4),2)}
 return 12*(v/2)**(2/3)/sum(edges.values()),min(1.,min(math.sqrt(2)*v/math.sqrt(math.prod(edges[tuple(sorted((i,k)))] for k in range(4) if k!=i)) for i in range(4)))
results=[];start=time.monotonic()
for high,angle in [(256,0),(256,15),(256,30),(256,45),(1024,30),(1024,45)]:
 c=math.cos(math.radians(angle));s=math.sin(math.radians(angle));a=np.array([[c,-s],[s,c]]);metric=a@np.diag([high,4.])@a.T
 nodes={i:p.copy() for i,p in original.items()};base=list(seed);new=1+max(v[0] for c in data['original'] for v in c['vertices']);trials=0
 def links():
  out={}
  for k,f in enumerate(base):
   for j in range(3):
    x,y=f[j],f[(j+1)%3];out.setdefault(tuple(sorted((x,y))),[]).append((k,x,y))
  return out
 while True:
  old=links();marked=[e for e in sorted(old) if math.sqrt(float((nodes[e[1]]-nodes[e[0]])@metric@(nodes[e[1]]-nodes[e[0]])))>.8*1.8]
  if not marked:break
  for edge in marked:
   uses=links()[edge];mid=new;new+=1;nodes[mid]=(nodes[edge[0]]+nodes[edge[1]])/2;fresh=[]
   for k,f in enumerate(base):
    hit=[row for row in uses if row[0]==k]
    if hit:
     _,x,y=hit[0];other=next(n for n in f if n not in edge);fresh.extend([(x,mid,other),(mid,y,other)])
    else:fresh.append(f)
   base=fresh;trials+=1
  assert len(base)<16384
 q=J=1.;bad=0;tetcount=0;capq=1.
 for f in base:
  pp=np.array([nodes[n] for n in f]);twice=abs(float(np.linalg.det(pp[1:]-pp[0])));ss=sum(float((pp[j]-pp[i])@metric@(pp[j]-pp[i])) for i,j in itertools.combinations(range(3),2));capq=min(capq,2*math.sqrt(3)*twice*math.sqrt(float(np.linalg.det(metric)))/ss)
  for lo,hi in zip(levels,levels[1:]):
   a,b,c=sorted(f);p={0:np.r_[nodes[a],lo],1:np.r_[nodes[b],lo],2:np.r_[nodes[c],lo],3:np.r_[nodes[a],hi],4:np.r_[nodes[b],hi],5:np.r_[nodes[c],hi]}
   for ids in [(0,1,2,5),(0,1,5,4),(0,4,5,3)]:
    t=np.array([p[i] for i in ids]);m=np.zeros((3,3));m[:2,:2]=metric;m[2,2]=normal(t[:,2].mean());v,j=measures(t,m);q=min(q,v);J=min(J,j);bad+=v<.2 or j<.05;tetcount+=1
 angle_bound=math.acos(min(1.,abs(metric[0,1])/math.sqrt(metric[0,0]*metric[1,1])));best_cap_corner=math.sqrt(3)*math.sin(angle_bound)/(2-math.cos(angle_bound))
 results.append({'eigenvalues':[high,4,4],'rotation_degrees':angle,'base_triangles':len(base),'normal_intervals':len(levels)-1,'tetrahedra':tetcount,'surface_splits':trials,'minimum_cap_quality':capq,'minimum_q3D':q,'minimum_J3D':J,'invalid_cells':bad,'best_cap_quality_bound_at_acute_transformed_cube_corner':best_cap_corner,'status':'RECONSTRUCTED_CANDIDATE_SHAPE_PASS' if not bad else 'RECONSTRUCTED_CANDIDATE_SHAPE_REJECTED'})
result={'status':'OFFLINE_PRIMARY_CAP_REFINEMENT_DIAGNOSIS_ONLY','source_geometry_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'levels':levels,'cases':results,'elapsed_seconds':time.monotonic()-start,'scope':'Independent reproduction of constant tangential original-P1 sensors, actual planar h.1/g1.2/T.3 normal/fade, midpoint longest-edge cap refinement and sorted-ID3-tet extrusion. Reuses finite unit-box geometry. Shape diagnostics only: no production/source-work/time/complete geometry/edge quadrature/MPI/CFD acceptance. Corner bound is the maximum2D triangle mean ratio at the smaller transformed angle, not global3D infeasibility.'}
(HERE/'reproduction.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(results,indent=2))
