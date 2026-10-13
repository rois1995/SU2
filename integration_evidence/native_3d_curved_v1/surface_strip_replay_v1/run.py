"""Bounded joint wall-strip/volume proposal on the saved native blockage.
Scientific placement experiment only; native exact/field/publication checks remain mandatory.
"""
from pathlib import Path
import os,time,json,hashlib,math
HERE=Path(__file__).resolve().parent
INPUT=HERE.parent/'recovery_collapse_v1/mpi1/cylinder_axis1_m1_s0_rejected.json'
def ticks():
 return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for c in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(c)],b['cpu'+str(c)])];idle[c]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);os.sched_setaffinity(0,{cpu});os.nice(19)
for key in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS'):os.environ[key]='1'
import numpy as np
from scipy.optimize import minimize
raw=json.loads(INPUT.read_text());base={n[0]:np.array(n[1:]) for n in raw['nodes']}
cells={c['id']:tuple(c['nodes']) for c in raw['cells']}
wall={tuple(sorted(f['nodes'])):f for f in raw['surface']};facet={f['id']:f for f in raw['facets']}
faces=lambda c:((c[1],c[2],c[3]),(c[0],c[3],c[2]),(c[0],c[1],c[3]),(c[0],c[2],c[1]))
owner={};star={};incident={};edges={}
for cid,c in cells.items():
 for n in c:star.setdefault(n,set()).add(cid)
 for f in faces(c):owner.setdefault(tuple(sorted(f)),[]).append(cid)
for key,f in wall.items():
 marker=raw['marker_names'][facet[f['facet']]['marker']]
 for n in f['nodes']:incident.setdefault(n,set()).add(marker)
 for i in range(3):edges.setdefault(tuple(sorted((f['nodes'][i],f['nodes'][(i+1)%3]))),[]).append(f)
assert all(len(v)<=2 for v in owner.values())
axis=np.ones(3)/math.sqrt(3);origin=np.array([2.,-1.,.5]);u=np.array([1.,-1.,0.])/math.sqrt(2);v=np.cross(axis,u)
radial=lambda p:p-origin-np.dot(p-origin,axis)*axis
marker=lambda f:raw['marker_names'][facet[f['facet']]['marker']]
def geometry(p,physical):
 excess=0.;direction=[]
 for f in physical:
  tri=np.array([p[n] for n in f['nodes']]);cross=np.cross(tri[1]-tri[0],tri[2]-tri[0]);m=marker(f)
  if m=='cylinder':
   r=np.array([radial(q) for q in tri]);norm=np.linalg.norm(r,axis=1)
   cos=np.clip((r@r.T)/(norm[:,None]*norm[None,:]),-1,1)
   error=1-norm.min()*math.cos(math.acos(cos.min())/2)
   excess+=np.linalg.norm(cross)*max(error-.04,0)**2
   direction.append(np.dot(cross,r[0]))
  else:direction.append(np.dot(cross,axis)*(1 if m=='cap1' else -1))
 return excess,np.array(direction)
def measures(p,tets):
 t=np.array([[p[n] for n in c] for c in tets]);e=t[:,1:]-t[:,:1]
 det=np.einsum('ij,ij->i',e[:,0],np.cross(e[:,1],e[:,2]));pairs=[(i,j) for i in range(4) for j in range(i+1,4)]
 lengths=np.array([np.linalg.norm(t[:,i]-t[:,j],axis=1) for i,j in pairs]).T
 with np.errstate(divide='ignore',invalid='ignore'):
  q=12*np.maximum(det/2,1e-30)**(2/3)/np.maximum((lengths**2).sum(axis=1),1e-30)
  lookup={ij:lengths[:,k] for k,ij in enumerate(pairs)}
  jac=np.min([math.sqrt(2)*det/np.maximum(np.prod([lookup[tuple(sorted((i,j)))] for j in range(4) if i!=j],axis=0),1e-30) for i in range(4)],axis=0)
 return det,q,jac,lengths.max()*math.sqrt(.5)
new_id=max(base)+1;records=[];best=None;started=time.monotonic()
# Rank by geometric deficit; no geometry-changing prefilter excludes a joint repair.
ranked=sorted(edges,key=lambda e:-sum(geometry(base,[f])[0] for f in edges[e]))
for edge in ranked:
 if time.monotonic()-started>240:break
 if len(edges[edge])!=2 or not any(marker(f)=='cylinder' for f in edges[edge]):continue
 neighborhood=sorted({n for f in edges[edge] for n in f['nodes']})
 for count in (2,4):
  moving=list(edge) if count==2 else neighborhood
  patch=set().union(*(star[n] for n in moving))
  if len(patch)>64:continue
  skin=[f for cid in sorted(patch) for f in faces(cells[cid]) if sum(c in patch for c in owner[tuple(sorted(f))])==1]
  interface={n for f in skin if tuple(sorted(f)) not in wall for n in f}
  assert not interface.intersection(moving),'Complete admitted stars must exclude moving nodes from artificial interface'
  # Admit existing free interior vertices as a coordinated transition, bounded to four.
  interior=sorted({n for cid in patch for n in cells[cid]}-interface-set(incident))[:4]
  p={n:q.copy() for n,q in base.items()};rawmid=(p[edge[0]]+p[edge[1]])/2
  height=np.dot(rawmid-origin,axis);r=radial(rawmid);p[new_id]=origin+height*axis+r/np.linalg.norm(r)
  tets=[]
  for cid in sorted(patch):
   c=cells[cid]
   if all(n in c for n in edge):
    l=list(c);rr=list(c);l[l.index(edge[1])]=new_id;rr[rr.index(edge[0])]=new_id;tets.extend((tuple(l),tuple(rr)))
   else:tets.append(c)
  physical=[]
  for f in raw['surface']:
   if not star[f['nodes'][0]].intersection(patch) or tuple(sorted(f['nodes'])) not in {tuple(sorted(s)) for s in skin}:continue
   c=f['nodes']
   if all(n in c for n in edge):
    for i in range(3):
     a,b,c0=c[i],c[(i+1)%3],c[(i+2)%3]
     if {a,b}==set(edge):physical.extend((dict(nodes=[a,new_id,c0],facet=f['facet']),dict(nodes=[new_id,b,c0],facet=f['facet'])));break
   else:physical.append(f)
  oldphysical=[wall[tuple(sorted(f))] for f in skin if tuple(sorted(f)) in wall]
  before=geometry(base,oldphysical)[0]
  if before==0:continue
  # Angular/axial coordinates preserve each cap intersection exactly; the split point shares edge features.
  features={n:incident[n] for n in moving};features[new_id]=incident[edge[0]].intersection(incident[edge[1]])
  variables=[];initial=[];bounds=[]
  lo=np.array([base[n] for cid in patch for n in cells[cid]]).min(axis=0);hi=np.array([base[n] for cid in patch for n in cells[cid]]).max(axis=0)
  for n in moving+[new_id]:
   if 'cylinder' not in features[n]:continue
   r=radial(p[n]);theta=math.atan2(np.dot(r,v),np.dot(r,u));z=np.dot(p[n]-origin,axis)
   variables.append((n,'theta'));initial.append(theta);bounds.append((theta-.3,theta+.3))
   if not features[n].intersection({'cap0','cap1'}):variables.append((n,'z'));initial.append(z);bounds.append((max(0,z-.3),min(1,z+.3)))
  for n in interior:
   for k in range(3):variables.append((n,k));initial.append(p[n][k]);bounds.append((lo[k],hi[k]))
  fixed={n:p[n].copy() for n in p}
  def unpack(x):
   polar={n:[math.atan2(np.dot(radial(fixed[n]),v),np.dot(radial(fixed[n]),u)),np.dot(fixed[n]-origin,axis)] for n in moving+[new_id] if 'cylinder' in features[n]}
   for (n,k),value in zip(variables,x):
    if k=='theta':polar[n][0]=value
    elif k=='z':polar[n][1]=value
    else:p[n][k]=value
   for n,(theta,z) in polar.items():p[n]=origin+z*axis+math.cos(theta)*u+math.sin(theta)*v
  def constraints(x):
   unpack(x);det,q,j,length=measures(p,tets);after,direction=geometry(p,physical)
   return np.r_[det-1e-12,q-.2001,j-.0501,direction-1e-12,(before*.99-after)/max(before,1e-20),1.8-length]
  def objective(x):
   unpack(x);det,q,j,_=measures(p,tets);after,_=geometry(p,physical)
   return ((np.maximum(.22-q,0)/.22)**2+(np.maximum(.06-j,0)/.06)**2).sum()+after/max(before,1e-20)
  fit=minimize(objective,initial,method='SLSQP',bounds=bounds,constraints=[{'type':'ineq','fun':constraints}],options={'maxiter':100,'ftol':1e-10})
  unpack(fit.x);det,q,j,length=measures(p,tets);after,direction=geometry(p,physical)
  record=dict(edge=edge,moving=moving,free_interior=interior,old_cells=len(patch),new_cells=len(tets),variables=len(variables),minimum_q=float(q.min()),minimum_J=float(j.min()),minimum_det=float(det.min()),maximum_metric_edge=float(length),geometry_before=float(before),geometry_after=float(after),iterations=int(fit.nit),optimizer_success=bool(fit.success))
  records.append(record);print(json.dumps(record),flush=True)
  if np.isfinite(q).all() and np.isfinite(j).all() and det.min()>0 and q.min()>=.20 and j.min()>=.05 and length<=1.8 and direction.min()>0 and after<=before*.99:
   best=dict(record=record,old_cell_ids=sorted(patch),nodes=[[n,*p[n]] for n in sorted({n for c in tets for n in c})],cells=tets,surface=physical);break
 if best:break
report=dict(status='JOINT_STRIP_PROPOSAL_FOUND' if best else 'NO_PROPOSAL_IN_BOUNDED_SEARCH',cpu=cpu,input_sha256=hashlib.sha256(INPUT.read_bytes()).hexdigest(),script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),records=records,best=best,scope='Split plus two/four complete boundary vertex stars and up to four existing interior vertices; preserved inherited topology, fixed artificial interface, cylinder/disk intersections, 0.5I/q0.20/J0.05/edge1.8/error0.04, >=1% geometry deficit reduction. Scientific floating placement only; native/exact original-P1/clearance/conservation/MPI qualification NOTRUN. Max64 old/128 new,100SLSQP iterations,240s ceiling between trials.')
(HERE/'probe.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],flush=True)
