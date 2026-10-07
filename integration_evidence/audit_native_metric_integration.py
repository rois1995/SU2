"""Frozen native integration audit: sensors, actual geometric queries, and MPI.

Retains incoming historical checker unchanged. Metric_* are sensor-only here;
composed transport is audited separately, including quarter-edge P1 queries.
Residuals are measurements, never silently accepted as a gradation certificate.
"""
import csv,hashlib,json,re,sys,time
from pathlib import Path
import numpy as np
from audit_native_composite_rae import GeometricWall,check_math
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'TestCases/adaptation/capability'))
import capcheck

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):
 with p.open() as f:names=next(csv.reader(f))
 a=np.loadtxt(p,delimiter=',',skiprows=1);a=a[np.argsort(a[:,0])]
 assert np.array_equal(a[:,0],np.arange(len(a)))
 return {n:a[:,i] for i,n in enumerate(names)}
def tensor(d,p='Metric'):
 a=np.zeros((len(d['x']),2,2));a[:,0,0]=d[p+'_XX'];a[:,0,1]=a[:,1,0]=d[p+'_XY'];a[:,1,1]=d[p+'_YY'];return a

def original_wall(path):
 sidecar=Path(str(path)+'.native_ref')
 if not sidecar.exists():
  mesh=capcheck.read_su2(path);return mesh.P,mesh.M['AIRFOIL'],[path]
 rows=sidecar.read_text().splitlines();assert rows[0]=='SU2_NATIVE_REFERENCE 1'
 markers,faces,bindings=map(int,rows[1].split());names=[json.loads(s) for s in rows[2:2+markers]]
 points={};edges=[]
 for line in rows[2+markers:2+markers+faces]:
  s=line.split();marker=int(s[0])
  if names[marker]!='AIRFOIL':continue
  ids=[]
  for i in (1,5):
   gid=int(s[i]);xy=tuple(map(float,s[i+1:i+3]));assert gid not in points or points[gid]==xy
   points[gid]=xy;ids.append(gid)
  edges.append(ids)
 ids=sorted(points);mapping={gid:i for i,gid in enumerate(ids)}
 return np.array([points[i] for i in ids]),np.array([[mapping[a],mapping[b]] for a,b in edges]),[path,sidecar]

def batch_wall(wall,p,sensor):
 """NumPy reference composition, with independent symmetric eigensolves."""
 out=sensor.copy();dist=np.full(len(p),np.inf);wall_tensors=np.zeros_like(sensor)
 use=np.flatnonzero(((p>=wall.lo)&(p<=wall.hi)).all(1))
 for begin in range(0,len(use),512):
  ids=use[begin:begin+512];x=p[ids];delta=x[:,None,:]-wall.a
  t=np.clip(np.einsum('bsi,si->bs',delta,wall.delta)/wall.length**2,0,1)
  r=delta-t[:,:,None]*wall.delta;ds=np.linalg.norm(r,axis=2);idx=ds.argmin(1);row=np.arange(len(x))
  for k in np.flatnonzero((t[row,idx]==0)|(t[row,idx]==1)):
   i=idx[k];node=wall.edges[i,int(t[k,i])]
   for other in wall.incident[node]:
    tie=abs(ds[k,other]-ds[k,i])<=1e-12*ds[k,i]
    nearer=np.sum((x[k]-(wall.a[other]+wall.b[other])/2)**2)<np.sum((x[k]-(wall.a[i]+wall.b[i])/2)**2)
    if (ds[k,other]<ds[k,i] and not tie) or (tie and nearer):i=other
   idx[k]=i
  distance=ds[row,idx];dist[ids]=distance;along=t[row,idx]
  n=np.column_stack([-wall.delta[idx,1],wall.delta[idx,0]])/wall.length[idx,None]
  endpoint=(along==0)|(along==1);radial=endpoint&(distance>1e-12*wall.length[idx]);n[radial]=r[row,idx][radial]/distance[radial,None]
  for k in np.flatnonzero(endpoint&~radial):n[k]=wall.normals[wall.edges[idx[k],int(along[k])]]
  tangent=np.column_stack([-n[:,1],n[:,0]]);ht=wall.length[idx].copy()
  for corner,turn in wall.corners:
   radius=np.linalg.norm(x-corner,axis=1);active=radius<=.5*wall.length[idx]
   ht[active]=np.minimum(ht[active],np.maximum(wall.h,2*radius[active]/turn))
  hn=np.maximum(wall.h,2*(wall.h+(wall.g-1)*distance)/(wall.g+1));full=max(wall.h,.9*wall.thickness);width=wall.thickness-full
  fade=np.clip((distance-full)/width,0,1) if width else (distance>full).astype(float);weight=1-fade**2*(3-2*fade)
  lt=np.exp(weight*np.log(1/ht**2)+(1-weight)*np.log(wall.core));ln=np.exp(weight*np.log(1/hn**2)+(1-weight)*np.log(wall.core))
  b=lt[:,None,None]*tangent[:,:,None]*tangent[:,None,:]+ln[:,None,None]*n[:,:,None]*n[:,None,:]
  wall_tensors[ids]=b
  active=weight>0
  if not active.any():continue
  chosen=ids[active];a=sensor[chosen];b=b[active]
  val,q=np.linalg.eigh(a);assert val.min()>0
  sqrt=np.einsum('nai,ni,nbi->nab',q,np.sqrt(val),q);inv=np.einsum('nai,ni,nbi->nab',q,1/np.sqrt(val),q)
  white=inv@b@inv;val,q=np.linalg.eigh(white)
  c=sqrt@np.einsum('nai,ni,nbi->nab',q,np.maximum(val,1),q)@sqrt
  out[chosen]=.5*(c+np.swapaxes(c,1,2))
 return out,dist,wall_tensors

def transport(dst,src,delta):
 distance=np.sqrt(np.maximum(0,np.einsum('ni,nij,nj->n',delta,src,delta)))
 val,q=np.linalg.eigh(dst);inv=q/np.sqrt(val)[:,None,:]
 white=np.einsum('nai,nab,nbj->nij',inv,src/(1+np.log(1.3)*distance)[:,None,None]**2,inv)
 return np.linalg.eigvalsh(white)[:,-1]
def stats(r):return {'maximum_ratio':float(r.max()),'directed_samples_above_1_plus_1e_5':int((r>1+1e-5).sum())}

def audit(folder):
 check_math();start=time.monotonic();prepared=json.loads((folder/'prepared_cases.json').read_text())
 result={'scope':'Frozen restart, zero CFD iterations, sensor-only donor field. Original geometric BL at nodes and P1 quarter-edge queries; transported residuals measured separately. No composed-gradation certificate or CFD-accuracy claim.','noise':0,'cases':{},'inputs':{}}
 for seed in ('fine','coarse'):
  inputs=folder/'frozen'/seed/'inputs';mesh=capcheck.read_su2(inputs/'input.su2');points,edges,paths=original_wall(inputs/'input.su2')
  _,original,_=capcheck.read_restart(inputs/'solution.dat');paths.append(inputs/'solution.dat');result['inputs'][seed]={str(p):sha(p) for p in paths}
  e=np.unique(np.sort(np.vstack([mesh.E[:,[0,1]],mesh.E[:,[1,2]],mesh.E[:,[2,0]]]),axis=1),axis=0)
  for method in ('weighted_least_squares','quadratic_least_squares'):
   key=seed+'/'+method;wd=folder/'frozen'/seed/(method+'_np1');d=load(wd/'fields.csv');a=tensor(d);val=np.linalg.eigvalsh(a);assert np.isfinite(a).all() and val.min()>0
   assert np.allclose(np.column_stack([d['x'],d['y']]),mesh.P,rtol=0,atol=1e-12)
   core=float(np.min((a[:,0,0].astype(np.longdouble)*a[:,1,1]-a[:,0,1].astype(np.longdouble)**2)/val[:,1]))
   wall=GeometricWall(points,edges,1e-5,1.2,.02,45,core);composed,distance,b=batch_wall(wall,mesh.P,a)
   # Scalar composition cross-check includes vertices, full band and fade queries.
   ix=np.unique(np.concatenate([np.linspace(0,len(a)-1,80,dtype=int),np.flatnonzero((distance>.018)&(distance<.02))[:80]]))
   for i in ix:
    target=np.array(wall(mesh.P[i],(a[i,0,0],a[i,0,1],a[i,1,1])));ref=np.array([composed[i,0,0],composed[i,0,1],composed[i,1,1]])
    assert np.linalg.norm(target-ref)<=2e-10*max(1,np.linalg.norm(ref))
   sensor_gap=float(np.linalg.eigvalsh(composed-a).min());scale=float(np.linalg.eigvalsh(composed).max());assert sensor_gap>=-1e-10*scale
   tri=mesh.P[mesh.E];area=.5*np.abs(np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]));mass=np.zeros(len(mesh.P));np.add.at(mass,mesh.E.ravel(),np.repeat(area/3,3))
   sensor_nodal_complexity=float(np.sum(np.sqrt(np.linalg.det(a))*mass));composed_nodal_complexity=float(np.sum(np.sqrt(np.linalg.det(composed))*mass))
   srat=[];crat=[]
   for dst,src in (e.T,e[:,::-1].T):
    dx=mesh.P[dst]-mesh.P[src];srat.append(transport(a[dst],a[src],dx));crat.append(transport(composed[dst],composed[src],dx))
   sr=np.concatenate(srat);cr=np.concatenate(crat)
   directed=np.concatenate([e,e[:,::-1]])
   def region(distance):
    return 'wall' if distance<=1e-11 else ('full_band' if distance<=.018 else ('fade' if distance<.02 else 'outer'))
   def nodal_locations(r):
    records=[]
    for i in np.argsort(r)[-20:][::-1]:
     dst,src=directed[i];records.append({'ratio':float(r[i]),'destination_gid':int(dst),'source_gid':int(src),'destination':mesh.P[dst].tolist(),'source':mesh.P[src].tolist(),'destination_region':region(distance[dst]),'source_region':region(distance[src])})
    return records
   groups={}
   for name in ('wall','full_band','fade','outer'):
    mask=np.array([region(distance[i])==name for i in directed[:,0]])
    groups[name]=stats(cr[mask]) if mask.any() else None
   vals,vectors=np.linalg.eigh(a);inverse=np.einsum('nai,ni,nbi->nab',vectors,1/np.sqrt(vals),vectors)
   minimum_domination=float(np.linalg.eigvalsh(inverse@composed@inverse).min())
   assert minimum_domination>=1-1e-6,minimum_domination

   entry={'points':len(a),'triangles':len(mesh.E),'original_wall_segments':len(edges),'core_eigenvalue':core,'minimum_sensor_eigenvalue':float(val.min()),'sensor_nodal_complexity':sensor_nodal_complexity,'composed_nodal_quadrature_counterexample':composed_nodal_complexity,'sensor_nodal':stats(sr),'composed_nodal':stats(cr),'sensor_domination_minimum_absolute_eigenvalue':sensor_gap,'minimum_generalized_sensor_domination_ratio':minimum_domination,'domination_numerical_tolerance':1e-6,'composed_residual_by_destination_region':groups,'worst_composed_nodal_locations':nodal_locations(cr),'mpi':{},'quarter_edge':{}}
   # Audits actual interior query locations; no P1 interpolation of wall tensors.
   qrat=[];sqrat=[];worst_quarter=[]
   for begin in range(0,len(e),4096):
    ee=e[begin:begin+4096];t=np.linspace(0,1,5);p=(1-t)[None,:,None]*mesh.P[ee[:,0],None,:]+t[None,:,None]*mesh.P[ee[:,1],None,:]
    s=(1-t)[None,:,None,None]*a[ee[:,0],None,:,:]+t[None,:,None,None]*a[ee[:,1],None,:,:]
    c,dd,_=batch_wall(wall,p.reshape(-1,2),s.reshape(-1,2,2));c=c.reshape(-1,5,2,2);dd=dd.reshape(-1,5)
    for dst,src in ((slice(1,None),slice(None,-1)),(slice(None,-1),slice(1,None))):
     dx=(p[:,dst]-p[:,src]).reshape(-1,2);ratios=transport(c[:,dst].reshape(-1,2,2),c[:,src].reshape(-1,2,2),dx);qrat.append(ratios);sqrat.append(transport(s[:,dst].reshape(-1,2,2),s[:,src].reshape(-1,2,2),dx))
     for i in np.argsort(ratios)[-20:]:
      local_edge=int(i)//4;k=int(i)%4;dstxy=p[:,dst,:].reshape(-1,2)[i];srcxy=p[:,src,:].reshape(-1,2)[i]
      worst_quarter.append({'ratio':float(ratios[i]),'donor_edge_gids':ee[local_edge].tolist(),'destination':dstxy.tolist(),'source':srcxy.tolist(),'destination_region':region(dd[:,dst].reshape(-1)[i]),'source_region':region(dd[:,src].reshape(-1)[i])})
     worst_quarter=sorted(worst_quarter,key=lambda row:row['ratio'],reverse=True)[:20]
   entry['quarter_edge']={'sensor':stats(np.concatenate(sqrat)),'composed':stats(np.concatenate(qrat)),'locations_per_edge':5,'worst_composed_locations':worst_quarter}
   for rank in (1,2,4):
    case=folder/'frozen'/seed/(method+'_np'+str(rank));dd=load(case/'fields.csv');bb=tensor(dd);assert np.isfinite(bb).all() and np.linalg.eigvalsh(bb).min()>0
    for field in ('Density','Momentum_x','Momentum_y','Energy','Nu_Tilde'):assert np.allclose(dd[field],original[field],rtol=1e-14,atol=0),field
    err=float(np.max(np.linalg.norm(bb-a,axis=(1,2))/np.linalg.norm(a,axis=(1,2))));assert err<1e-9,(key,rank,err)
    herror={}
    for sensor in ('MACH','PRESSURE'):
     h=tensor(d,'Hessian_'+sensor);hh=tensor(dd,'Hessian_'+sensor);norm=np.linalg.norm(h,axis=(1,2));herror[sensor]=float(np.max(np.linalg.norm(hh-h,axis=(1,2))/np.maximum(norm,1e-14*norm.max())))
    log=(case/'solver.log').read_text();complexity=float(re.search(r'Mesh complexity after native constraints: ([\d.e+\-]+)',log)[1]);assert abs(complexity/60000-1)<1e-5
    row={'maximum_relative_sensor_difference':err,'hessian_relative_with_global_floor':herror,'reported_composed_complexity':complexity,'elapsed_seconds':json.loads((case/'run_evidence.json').read_text())['elapsed_seconds'],'fields_sha256':sha(case/'fields.csv'),'log_sha256':sha(case/'solver.log')}
    for label,pattern in [('hessian','Hessian recovery: (.*)'),('integration','Native geometric integration: (.*)'),('work','Native metric work: (.*)'),('sensor_gradation','Native sensor gradation: (.*)'),('composed_audit','Native composed nodal audit: (.*)')]:row[label]=re.findall(pattern,log)[-1]
    entry['mpi'][str(rank)]=row
   result['cases'][key]=entry
   print(key,'sensor',entry['sensor_nodal'],'composed',entry['composed_nodal'],'quarter',entry['quarter_edge'],flush=True)
 result['elapsed_seconds']=time.monotonic()-start;result['auditor_sha256']=sha(__file__)
 out=folder/'frozen_field_audit.json';assert not out.exists();out.write_text(json.dumps(result,indent=2)+'\n')
 return result
if __name__=='__main__':audit(Path(sys.argv[1]).resolve())
