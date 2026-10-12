from pathlib import Path
import os,time
eligible=os.sched_getaffinity(0)
def cpu_ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=cpu_ticks();time.sleep(1);b=cpu_ticks();idle={}
for c in eligible:
 d=[y-x for x,y in zip(a['cpu'+str(c)],b['cpu'+str(c)])];idle[c]=(d[3]+d[4])/sum(d) if sum(d) else 0
selected_cpu=max(idle,key=idle.get);os.sched_setaffinity(0,{selected_cpu});os.nice(19)
for name in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS'):os.environ[name]='1'
import json,hashlib,math,time
import numpy as np
from scipy.optimize import minimize
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];INPUT=HERE.parent/'joint_runtime_v4/mpi1/cylinder_axis1_m1_s0_rejected.json'
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
surface_edges={};oriented={tuple(sorted(f['nodes'])):tuple(f['nodes']) for f in row['surface']}
for f in oriented.values():
 for i in range(3):surface_edges.setdefault(tuple(sorted((f[i],f[(i+1)%3]))),[]).append(f)
records=[];best=None;started=time.monotonic();ceiling=240
for edge,wall in sorted(surface_edges.items()):
 if time.monotonic()-started>ceiling:break
 if len(wall)!=2 or any(name(tuple(sorted(f)))!='cylinder' for f in wall):continue
 if surface[tuple(sorted(wall[0]))]!=surface[tuple(sorted(wall[1]))]:continue
 first,second=wall;a,b=edge
 for i in range(3):
  if {first[i],first[(i+1)%3]}==set(edge):a,b,c=first[i],first[(i+1)%3],first[(i+2)%3];break
 d=next(n for n in second if n not in edge)
 if len({a,b,c,d})!=4:continue
 children=[(c,a,d),(c,d,b)]
 def defect(face):
  p=np.array([nodes[n] for n in face]);return max(radial_error(p)-.04,0)**2*np.linalg.norm(np.cross(p[1]-p[0],p[2]-p[0]))
 old_error=sum(defect(f) for f in wall);new_error=sum(defect(f) for f in children)
 if old_error==0 or not new_error<old_error*(1-1e-12):continue
 # Outward curved orientation, not just nodal containment.
 if any(np.dot(np.cross(nodes[f[1]]-nodes[f[0]],nodes[f[2]]-nodes[f[0]]),radial(nodes[f[0]]))<=0 for f in children):continue
 patch={cid for cid,ids in cells.items() if all(n in ids for n in edge)}
 for growth in range(10):
  if time.monotonic()-started>ceiling or len(patch)>64:break
  skin=[];tags={}
  for cid in patch:
   for f in faces(cells[cid]):
    if sum(n in patch for n in owner[tuple(sorted(f))])!=1:continue
    key=tuple(sorted(f))
    if key in {tuple(sorted(w)) for w in wall}:continue
    skin.append(f)
    if key in surface:tags[key]=surface[key]
  skin+=children
  for f in children:tags[tuple(sorted(f))]=surface[tuple(sorted(wall[0]))]
  if len(skin)>128:break
  ed={}
  for f in skin:
   for i in range(3):ed.setdefault(tuple(sorted((f[i],f[(i+1)%3]))),0);ed[tuple(sorted((f[i],f[(i+1)%3])))]+=1
  topology=all(n==2 for n in ed.values())
  points=np.array([[nodes[n] for n in f] for f in skin]);lo,hi=points.reshape(-1,3).min(axis=0),points.reshape(-1,3).max(axis=0)
  oldpoints=np.array([[nodes[n] for n in cells[cid]] for cid in patch]);vol=np.linalg.det(oldpoints[:,1:]-oldpoints[:,:1]);start=(oldpoints.mean(axis=1)*vol[:,None]).sum(axis=0)/vol.sum()
  def objective(x):
   det,q,j,_=measures(x,skin);return np.sum((np.maximum(.22-q,0)/.22)**2+(np.maximum(.06-j,0)/.06)**2+100*np.minimum(det,0)**2)
  if topology:
   fit=minimize(objective,start,method='SLSQP',bounds=list(zip(lo,hi)),constraints=[{'type':'ineq','fun':lambda x:measures(x,skin)[0]-1e-12}],options={'maxiter':70,'ftol':1e-12});apex=fit.x
  else:apex=start
  det,q,j,length=measures(apex,skin);record=dict(edge=edge,diagonal=[c,d],growth=growth,old_cells=len(patch),new_cells=len(skin),minimum_q=float(q.min()),minimum_J=float(j.min()),minimum_det=float(det.min()),maximum_metric_edge=float(length),geometry_before=float(old_error),geometry_after=float(new_error),closed_edge_topology=topology);records.append(record)
  if topology and np.isfinite(q).all() and np.isfinite(j).all() and det.min()>0 and q.min()>=.20 and j.min()>=.05 and length<=1.8:
   best=dict(record=record,old_cell_ids=sorted(patch),interior=[I_ID,*apex],cells=[[I_ID,*f] for f in skin],surface=[dict(nodes=list(f),facet=tags[tuple(sorted(f))]) for f in skin if tuple(sorted(f)) in tags]);break
  priorities=[]
  for i,f in enumerate(skin):
   key=tuple(sorted(f))
   if key in tags:continue
   outside=[cid for cid in owner[key] if cid not in patch]
   if outside:priorities.append((min(q[i]/.2,j[i]/.05),outside[0]))
  if not priorities:break
  patch.add(min(priorities)[1])
 if best:break
report=dict(status='LOCAL_SURFACE_SWAP_PROPOSAL_FOUND' if best else 'NO_PROPOSAL_IN_BOUNDED_SEARCH',cpu=selected_cpu,input_sha256=hashlib.sha256(INPUT.read_bytes()).hexdigest(),script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),records=records,best=best,scope='Same-marker/chart physical diagonal swap plus complete volume reconstruction; constant0.5I/q0.20/J0.05/edge1.8/error0.04 unchanged. Floating-point proposal only; not native/MPI/exact conformity/clearance/source coverage/conservation/full Goal2. Max64old/128new/10growth/70optimizer iterations, wall240s between proposals plus hard process limit.')
(HERE/'probe.json').write_text(json.dumps(report,indent=2,default=lambda x:x.item() if hasattr(x,'item') else list(x))+'\n');print(json.dumps({'status':report['status'],'trials':len(records),'best':best['record'] if best else None},indent=2))
