from pathlib import Path
import os
os.sched_setaffinity(0,{min(os.sched_getaffinity(0))});os.nice(19)
for name in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS'):os.environ[name]='1'
import json,hashlib,math,time
import numpy as np
from scipy.optimize import linprog,minimize
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];INPUT=HERE.parent/'joint_runtime_v3/mpi1/cylinder_axis1_m1_s0_rejected.json'
row=json.loads(INPUT.read_text());nodes={n[0]:np.array(n[1:]) for n in row['nodes']};cells={c['id']:tuple(c['nodes']) for c in row['cells']};surface={tuple(sorted(f['nodes'])):f['facet'] for f in row['surface']};facets={f['id']:f for f in row['facets']}
axis=np.ones(3)/math.sqrt(3);origin=np.array([2.,-1.,.5]);P_ID=max(nodes)+1;I_ID=P_ID+1
faces=lambda c:((c[1],c[2],c[3]),(c[0],c[3],c[2]),(c[0],c[1],c[3]),(c[0],c[2],c[1]))
owner={}
for c,ids in cells.items():
 for f in faces(ids):owner.setdefault(tuple(sorted(f)),[]).append(c)
assert all(len(v)<=2 for v in owner.values())
name=lambda key:row['marker_names'][facets[surface[key]]['marker']]
def radial(p):
 d=p-origin;return d-np.dot(d,axis)*axis
def error(f):
 if name(tuple(sorted(f)))!='cylinder':return 0.
 return radial_error(np.array([nodes[n] for n in f]))
def radial_error(p):
 r=np.array([radial(v) for v in p]);norm=np.linalg.norm(r,axis=1);cos=np.clip((r@r.T)/norm[:,None]/norm[None,:],-1.,1.)
 return 1-norm.min()*math.cos(math.acos(cos.min())/2)
def measures(apex,skin):
 p=np.array([[apex,*[nodes[n] for n in f]] for f in skin]);e=p[:,1:]-p[:,:1];det=np.einsum('ij,ij->i',e[:,0],np.cross(e[:,1],e[:,2]))
 lengths=np.array([np.linalg.norm(p[:,i]-p[:,j],axis=1) for i in range(4) for j in range(i+1,4)]).T
 q=12*np.maximum(det/2,1e-30)**(2/3)/(lengths**2).sum(axis=1)
 edge={(i,j):lengths[:,k] for k,(i,j) in enumerate(( (i,j) for i in range(4) for j in range(i+1,4)))}
 j=np.min(np.array([math.sqrt(2)*det/np.prod([edge[tuple(sorted((i,k)))] for k in range(4) if i!=k],axis=0) for i in range(4)]),axis=0)
 return det,q,j,lengths.max()*math.sqrt(.5)
def skin_for(patch,edge):
 before=[]
 for c in patch:
  before.extend(f for f in faces(cells[c]) if sum(n in patch for n in owner[tuple(sorted(f))])==1)
 skin=[];tags={};before_excess=after_excess=0.
 for f in before:
  key=tuple(sorted(f));physical=key in surface;new=[f]
  if physical and all(n in f for n in edge):
   new=[]
   for i in range(3):
    a,b,c=f[i],f[(i+1)%3],f[(i+2)%3]
    if {a,b}==set(edge):new=[(a,P_ID,c),(P_ID,b,c)];break
  skin+=new
  if physical:
   p=np.array([nodes[n] for n in f]);area=np.linalg.norm(np.cross(p[1]-p[0],p[2]-p[0]));before_excess+=max(error(f)-.04,0)**2*area
   for child in new:
    p=np.array([nodes[n] for n in child]);area=np.linalg.norm(np.cross(p[1]-p[0],p[2]-p[0]));err=radial_error(p) if name(key)=='cylinder' else 0
    after_excess+=max(err-.04,0)**2*area;tags[tuple(sorted(child))]=surface[key]
 return before,skin,tags,before_excess,after_excess
edges=set()
for key in surface:
 if name(key)=='cylinder' and error(key)>.040000001:
  edges.update(tuple(sorted((key[i],key[j]))) for i in range(3) for j in range(i+1,3))
records=[];best=None;started=time.monotonic()
for edge in sorted(edges):
 raw=(nodes[edge[0]]+nodes[edge[1]])/2;h=np.dot(raw-origin,axis);r=radial(raw);nodes[P_ID]=origin+h*axis+r/np.linalg.norm(r)
 patch={c for c,ids in cells.items() if all(n in ids for n in edge)}
 for growth in range(12):
  if len(patch)>64:break
  before,skin,tags,bex,aex=skin_for(patch,edge)
  if len(skin)>128:break
  points=np.array([[nodes[n] for n in f] for f in skin]);normal=np.cross(points[:,1]-points[:,0],points[:,2]-points[:,0]);normal/=np.linalg.norm(normal,axis=1)[:,None]
  bounds=np.einsum('ij,ij->i',normal,points[:,0]);lo,hi=points.reshape(-1,3).min(axis=0),points.reshape(-1,3).max(axis=0)
  lp=linprog([0,0,0,-1],A_ub=np.c_[normal,np.ones(len(skin))],b_ub=bounds,bounds=[*zip(lo,hi),(None,None)],method='highs')
  start=lp.x[:3] if lp.success else points.reshape(-1,3).mean(axis=0)
  def objective(x):
   det,q,j,_=measures(x,skin)
   return np.sum(np.maximum(.22-q,0)**2+np.maximum(.06-j,0)**2+10*np.minimum(det,0)**2)
  oldpoints=np.array([[nodes[n] for n in cells[c]] for c in patch]);vol=np.linalg.det(oldpoints[:,1:]-oldpoints[:,:1]);start=(oldpoints.mean(axis=1)*vol[:,None]).sum(axis=0)/vol.sum()
  chosen=start;cost=objective(chosen)
  rms=np.mean([math.sqrt(sum(np.linalg.norm(t[i]-t[j])**2 for i in range(4) for j in range(i+1,4))/6) for t in oldpoints])
  directions=[np.array([i,j,k],float)/math.sqrt(i*i+j*j+k*k) for i in (-1,0,1) for j in (-1,0,1) for k in (-1,0,1) if i or j or k]
  for level in range(9):
   for sweep in range(3):
    center=chosen.copy();previous=cost
    for direction in directions:
     candidate=center+direction*rms*.5/(2**level)
     value=objective(candidate)
     if value<cost:cost=value;chosen=candidate
    if cost<1e-12 or cost>=previous:break
   if cost<1e-12:break
  class Fit:pass
  fit=Fit();fit.x=chosen
  apex=fit.x;det,q,j,length=measures(apex,skin);record=dict(edge=edge,old_cells=len(patch),new_cells=len(skin),growth=growth,minimum_q=float(q.min()),minimum_J=float(j.min()),minimum_det=float(det.min()),maximum_metric_edge=float(length),geometry_before=float(bex),geometry_after=float(aex));records.append(record)
  if det.min()>0 and q.min()>=.20 and j.min()>=.05 and length<=1.8 and aex<bex*(1-1e-12):
   nodes[I_ID]=apex;best=dict(record=record,old_cell_ids=sorted(patch),point=nodes[P_ID].tolist(),interior=apex.tolist(),cells=[[I_ID,*f] for f in skin],surface=[dict(nodes=list(f),facet=tags[tuple(sorted(f))]) for f in skin if tuple(sorted(f)) in tags]);break
  priorities=[]
  for i,f in enumerate(skin):
   key=tuple(sorted(f))
   if P_ID in f or key in surface:continue
   outside=[c for c in owner[key] if c not in patch]
   if outside:priorities.append((min(q[i]/.2,j[i]/.05),outside[0]))
  if not priorities:break
  patch.add(min(priorities)[1])
 if best:break
assert time.monotonic()-started<600
report=dict(status='LOCAL_PROPOSAL_FOUND' if best else 'NO_ADMISSIBLE_ONE_INTERIOR_POINT_PROPOSAL',input_sha256=hashlib.sha256(INPUT.read_bytes()).hexdigest(),script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),records=records,best=best,scope='Floating-point, constant0.5I bounded combined surface/volume reconstruction probe only. Not production/MPI/source-coverage/conservation/independent exact proof or full Goal2. q>=0.20/J>=0.05/length<=1.8/error0.04 unchanged, max64 old/128 new/12 growth steps/80 optimization iterations.')
(HERE/'pattern_probe.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'status':report['status'],'trials':len(records),'best':best['record'] if best else None},indent=2))
