"""Higher-order frozen-field complexity replay; no CFD or remeshing.

Uses independent NumPy composition and Duffy Gauss cubature (production uses a
three-point triangle rule). Geometric bands are tighter. Reports continuous-
field quadrature sensitivity, not a new budget or a changed metric target.
"""
import hashlib,json,math,sys,time
from pathlib import Path
import numpy as np
from audit_native_metric_integration import load,tensor,original_wall,batch_wall,GeometricWall,capcheck

def rules(mesh,wall,ratio):
 cache={}
 def distance(p):
  key=tuple(p)
  if key not in cache:
   t=np.clip(np.sum((p-wall.a)*wall.delta,axis=1)/wall.length**2,0,1)
   cache[key]=float(np.linalg.norm(p-wall.a-t[:,None]*wall.delta,axis=1).min())
  return cache[key]
 levels=[0.];level=wall.h
 while level<.9*wall.thickness:levels.append(level);level*=ratio
 levels.extend(np.linspace(.9*wall.thickness,wall.thickness,25));levels=np.unique(levels)
 gauss,weights=np.polynomial.legendre.leggauss(4);gauss=(gauss+1)/2;weights=weights/2
 def split(x,bary,depth):
  # Wall proximity cannot be inferred from a sample's weight alone.
  low=x.min(0);high=x.max(0)
  if (high<wall.lo).any() or (low>wall.hi).any():active=False
  else:active=True
  d=np.array([distance(p) for p in x]) if active else np.full(3,math.inf)
  mids=(x+np.roll(x,-1,axis=0))/2;md=np.array([distance(p) for p in mids]) if active else np.full(3,math.inf)
  refine=active and np.any(np.abs(md-(d+np.roll(d,-1))/2)>.075*max(wall.h,d.min()))
  if active and d.min()>=wall.thickness:
   # Conservative segment/triangle proximity, including bands missed by vertices.
   candidates=np.flatnonzero(((np.maximum(wall.a,wall.b)+wall.thickness>=low)&(np.minimum(wall.a,wall.b)-wall.thickness<=high)).all(1))
   def cross(a,b,c):return np.cross(b-a,c-a)
   def point_segment(p,a,b):
    v=b-a;t=np.clip(np.dot(p-a,v)/np.dot(v,v),0,1);return np.linalg.norm(p-a-t*v)
   def inside(p):
    c=np.array([cross(x[k],x[(k+1)%3],p) for k in range(3)]);return (c>=0).all() or (c<=0).all()
   for face in candidates:
    a,b=wall.a[face],wall.b[face];gap=0 if inside(a) or inside(b) else math.inf
    for k in range(3):
     u,v=x[k],x[(k+1)%3];gap=min(gap,point_segment(u,a,b),point_segment(a,u,v),point_segment(b,u,v))
     if cross(a,b,u)*cross(a,b,v)<0 and cross(u,v,a)*cross(u,v,b)<0:gap=0
    refine=refine or gap<wall.thickness*(1-1e-12)
  if refine:
   assert depth<16,'Independent quadrature geometry limit reached'
   xx=np.vstack([x,mids]);bb=np.vstack([bary,(bary+np.roll(bary,-1,axis=0))/2])
   for ids in ((0,3,5),(3,1,4),(5,4,2),(3,4,5)):yield from split(xx[list(ids)],bb[list(ids)],depth+1)
   return
  pieces=[np.eye(3)]
  for level in levels:
   if not d.min()<level<d.max():continue
   nxt=[]
   for poly in pieces:
    dd=poly@d;lo=[];hi=[]
    for k in range(len(poly)):
     a=poly[k];b=poly[(k+1)%len(poly)];da=dd[k];db=dd[(k+1)%len(poly)]
     (lo if da<=level else hi).append(a)
     if (da<level<db) or (db<level<da):
      cut=a+(level-da)/(db-da)*(b-a);lo.append(cut);hi.append(cut)
    for side in (lo,hi):
     if len(side)>=3:nxt.append(np.array(side))
   pieces=nxt
  for poly in pieces:
   for k in range(1,len(poly)-1):
    bb=poly[[0,k,k+1]];xx=bb@x;area=.5*abs(np.cross(xx[1]-xx[0],xx[2]-xx[0]))
    if area<=0:continue
    for i,u in enumerate(gauss):
     for j,v in enumerate(gauss):
      local=np.array([1-u-(1-u)*v,u,(1-u)*v]);yield local@bb@bary,2*area*(1-u)*weights[i]*weights[j]
 for ids in mesh.E:
  for bary,weight in split(mesh.P[ids],np.eye(3),0):yield ids,bary,weight

def audit(folder,ratio=1.25):
 start=time.monotonic();out={'scope':'Independent original-P1 sensor plus geometric BL intersection/fade; tighter distance bands and 4x4 Duffy Gaussian cubature. Does not change the frozen target.','band_ratio':ratio,'cases':{}}
 for seed in ('fine','coarse'):
  inputs=folder/'frozen'/seed/'inputs';mesh=capcheck.read_su2(inputs/'input.su2');p,e,_=original_wall(inputs/'input.su2');metrics={};walls={}
  for method in ('weighted_least_squares','quadratic_least_squares'):
   a=tensor(load(folder/'frozen'/seed/(method+'_np1')/'fields.csv'));val=np.linalg.eigvalsh(a);core=float(np.min((a[:,0,0].astype(np.longdouble)*a[:,1,1]-a[:,0,1].astype(np.longdouble)**2)/val[:,1]));metrics[method]=a;walls[method]=GeometricWall(p,e,1e-5,1.2,.02,45,core)
  total={key:0. for key in metrics};mass=0.;count=0;ids=[];bary=[];weights=[]
  def integrate():
   nonlocal mass,count
   ii=np.array(ids);bb=np.array(bary);w=np.array(weights);x=np.einsum('nk,nkd->nd',bb,mesh.P[ii]);mass+=float(w.sum());count+=len(w)
   for key,m in metrics.items():
    sensor=np.einsum('nk,nkab->nab',bb,m[ii]);combined,_,_=batch_wall(walls[key],x,sensor);total[key]+=float(np.dot(w,np.sqrt(np.linalg.det(combined))))
   ids.clear();bary.clear();weights.clear()
  for i,b,w in rules(mesh,next(iter(walls.values())),ratio):
   ids.append(i);bary.append(b);weights.append(w)
   if len(weights)>=8192:integrate()
  if weights:integrate()
  expected=float(np.sum(.5*np.abs(np.cross(mesh.P[mesh.E[:,1]]-mesh.P[mesh.E[:,0]],mesh.P[mesh.E[:,2]]-mesh.P[mesh.E[:,0]]))));assert abs(mass/expected-1)<1e-11
  out['cases'][seed]={'quadrature_points':count,'integrated_area':mass,'mesh_area':expected,'complexities':total,'relative_to_reported_target':{k:v/60000-1 for k,v in total.items()}}
  print(seed,out['cases'][seed],flush=True)
 out['elapsed_seconds']=time.monotonic()-start;out['auditor_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();path=folder/('quadrature_audit_'+str(ratio)+'.json');assert not path.exists();path.write_text(json.dumps(out,indent=2)+'\n')
def self_check():
 from types import SimpleNamespace
 mesh=SimpleNamespace(P=np.array([[.2,0.],[.8,0.],[.2,.4]]),E=np.array([[0,1,2]]))
 wall=GeometricWall([[0,0],[1,0],[1,1],[0,1]],[[0,1],[1,2],[2,3],[3,0]],1e-4,1.2,.01,45,4.)
 totals=[]
 for ratio in (1.5,1.25):
  rows=list(rules(mesh,wall,ratio));b=np.array([r[1] for r in rows]);w=np.array([r[2] for r in rows])
  assert (w>0).all() and b.min()>-1e-12 and np.allclose(b.sum(1),1)
  assert abs(w.sum()/.12-1)<1e-12
  p=b@mesh.P;sensor=np.broadcast_to(np.diag([4.,9.]),(len(p),2,2));m,_,_=batch_wall(wall,p,sensor)
  totals.append(float(np.dot(w,np.sqrt(np.linalg.det(m)))))
 assert abs(totals[0]/totals[1]-1)<1e-5,totals
 print('Independent geometric cubature self-check PASS',totals)
if __name__=='__main__':
 if sys.argv[1:] == ['--self-check']:self_check()
 else:audit(Path(sys.argv[1]).resolve(),float(sys.argv[2]) if len(sys.argv)>2 else 1.25)
