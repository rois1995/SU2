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
from scipy.optimize import linprog,minimize
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
edges=set()
for key in surface:
 if name(key)=='cylinder' and error(key)>.040000001:
  edges.update(tuple(sorted((key[i],key[j]))) for i in range(3) for j in range(i+1,3))
J_ID=I_ID+1
records=[];best=None;started=time.monotonic();LIMIT_SECONDS=480
for edge in sorted(edges):
 raw=(nodes[edge[0]]+nodes[edge[1]])/2;h=np.dot(raw-origin,axis);r=radial(raw);nodes[P_ID]=origin+h*axis+r/np.linalg.norm(r)
 patch={c for c,ids in cells.items() if all(n in ids for n in edge)}
 for growth in range(8):
  if time.monotonic()-started>LIMIT_SECONDS:break
  before,skin,tags,bex,aex=skin_for(patch,edge)
  if len(patch)>64 or len(skin)>120 or not aex<bex*(1-1e-12):break
  points=np.array([[nodes[n] for n in f] for f in skin]);normal=np.cross(points[:,1]-points[:,0],points[:,2]-points[:,0]);normal/=np.linalg.norm(normal,axis=1)[:,None]
  lo,hi=points.reshape(-1,3).min(axis=0),points.reshape(-1,3).max(axis=0)
  oldpoints=np.array([[nodes[n] for n in cells[c]] for c in patch]);volume=np.linalg.det(oldpoints[:,1:]-oldpoints[:,:1]);center=(oldpoints.mean(axis=1)*volume[:,None]).sum(axis=0)/volume.sum()
  det,q,j,length=measures(center,skin)
  anchors=list(np.argsort(np.minimum(q/.2,j/.05)))[:4]
  edge_faces={}
  for i,f in enumerate(skin):
   for k in range(3):edge_faces.setdefault(tuple(sorted((f[k],f[(k+1)%3]))),[]).append(i)
  groups=[]
  for i in anchors:
   groups.append({int(i)})
   adjacent={k for f in edge_faces.values() if i in f for k in f if k!=i}
   groups.extend({int(i),k} for k in sorted(adjacent))
  for group in groups:
   if time.monotonic()-started>LIMIT_SECONDS:break
   frontier=[]
   for i in group:
    f=skin[i]
    for k in range(3):
     a,b=f[k],f[(k+1)%3];neighbors=edge_faces[tuple(sorted((a,b)))]
     assert len(neighbors)==2
     if any(j not in group for j in neighbors):frontier.append((a,b))
   connectivity=[(I_ID if i in group else J_ID,*f) for i,f in enumerate(skin)]+[(I_ID,J_ID,b,a) for a,b in frontier]
   if len(connectivity)>128:continue
   anchor=points[min(group)];S=sum(np.linalg.norm(anchor[i]-anchor[j])**2 for i in range(3) for j in range(i+1,3));a=anchor.mean(axis=0)-normal[min(group)]*math.sqrt(2*S/9);initial=np.r_[a,center]
   def measured(x):
    mapping=dict(nodes);mapping[I_ID]=x[:3];mapping[J_ID]=x[3:];tet=np.array([[mapping[n] for n in c] for c in connectivity]);e=tet[:,1:]-tet[:,:1];det=np.einsum('ij,ij->i',e[:,0],np.cross(e[:,1],e[:,2]));ls=np.array([np.linalg.norm(tet[:,i]-tet[:,j],axis=1) for i in range(4) for j in range(i+1,4)]).T;q=12*np.maximum(det/2,1e-30)**(2/3)/(ls**2).sum(axis=1);index={(i,j):ls[:,k] for k,(i,j) in enumerate(( (i,j) for i in range(4) for j in range(i+1,4)))};j=np.min(np.array([math.sqrt(2)*det/np.prod([index[tuple(sorted((i,k)))] for k in range(4) if i!=k],axis=0) for i in range(4)]),axis=0);return det,q,j,ls.max()*math.sqrt(.5)
   def objective(x):
    det,q,j,_=measured(x);return np.sum((np.maximum(.22-q,0)/.22)**2+(np.maximum(.06-j,0)/.06)**2+100*np.minimum(det,0)**2)
   fit=minimize(objective,initial,method='SLSQP',bounds=[*zip(lo,hi),*zip(lo,hi)],constraints=[{'type':'ineq','fun':lambda x:measured(x)[0]-1e-12}],options={'maxiter':70,'ftol':1e-12})
   det,q,j,length=measured(fit.x);record=dict(edge=edge,growth=growth,old_cells=len(patch),new_cells=len(connectivity),group=sorted(group),minimum_q=float(q.min()),minimum_J=float(j.min()),minimum_det=float(det.min()),maximum_metric_edge=float(length),geometry_before=float(bex),geometry_after=float(aex));records.append(record)
   if det.min()>0 and q.min()>=.20 and j.min()>=.05 and length<=1.8:
    best=dict(record=record,old_cell_ids=sorted(patch),boundary_point=[P_ID,*nodes[P_ID]],interiors=[[I_ID,*fit.x[:3]],[J_ID,*fit.x[3:]]],cells=connectivity,surface=[dict(nodes=list(f),facet=tags[tuple(sorted(f))]) for f in skin if tuple(sorted(f)) in tags]);break
  if best:break
  priorities=[]
  for i,f in enumerate(skin):
   key=tuple(sorted(f))
   if P_ID in f or key in surface:continue
   outside=[c for c in owner[key] if c not in patch]
   if outside:priorities.append((min(q[i]/.2,j[i]/.05) if i<len(q) else 0,outside[0]))
  if not priorities:break
  patch.add(min(priorities)[1])
 if best or time.monotonic()-started>LIMIT_SECONDS:break
report=dict(cpu=selected_cpu,status='LOCAL_TWO_APEX_PROPOSAL_FOUND' if best else 'NO_PROPOSAL_IN_BOUNDED_SEARCH',input_sha256=hashlib.sha256(INPUT.read_bytes()).hexdigest(),script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),records=records,best=best,scope='Bounded floating-point coupled physical-split/two-interior-point cavity reconstruction; constant0.5I only, q0.20/J0.05/length1.8/geometry0.04 unchanged. Not production/MPI/exact-source/clearance/conservation/full-goal proof. Maximum64old/128new/8growth/70SLSQP iterations per proposal, wall ceiling480s checked between proposals. OneCPU/nice19/threads1.')
(HERE/'probe.json').write_text(json.dumps(report,indent=2,default=lambda x:x.item() if hasattr(x,'item') else list(x))+'\n');print(json.dumps({'status':report['status'],'trials':len(records),'best':best['record'] if best else None},indent=2))
