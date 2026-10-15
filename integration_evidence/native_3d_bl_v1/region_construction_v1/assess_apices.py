#!/usr/bin/env python3
"""Assess band-template feasibility using captured tensors; these proxies never replace native field acceptance."""
import csv,hashlib,json,math,time
from pathlib import Path
import numpy as np
from scipy.optimize import minimize
HERE=Path(__file__).resolve().parent

def shape(points,metrics):
 edges=points[:,None,:,:]-points[:,:,None,:]
 squared=np.einsum('...i,nij,...j->...',edges,metrics,edges) if False else np.einsum('nabi,nij,nabj->nab',edges,metrics,edges)
 det=np.linalg.det(points[:,1:]-points[:,:1]);weighted=det*np.sqrt(np.linalg.det(metrics));upper=np.triu_indices(4,1)
 q=12*np.maximum(weighted/2,0)**(2/3)/squared[:,upper[0],upper[1]].sum(axis=1)
 products=np.array([np.prod(np.delete(squared[:,i,:],i,axis=1),axis=1) for i in range(4)]).T
 j=np.min(np.sqrt(2)*weighted[:,None]/np.sqrt(np.maximum(products,1e-300)),axis=1)
 return q,j,det

def main():
 start=time.monotonic();case=HERE/'affine_band';data=json.loads((case/'candidate.json').read_text());nodes={n[0]:np.array(n[1:]) for n in data['nodes']}
 samples={int(r['cell']):r for r in csv.DictReader((case/'native_field/grid_samples.csv').open())};groups={f['apex']:f for f in data['first_rows']}
 by_apex={n:[] for n in groups}
 for c in data['cells']:
  if c['region']=='wall_band':by_apex[c['nodes'][0]].append(c)
 result=[]
 for apex,face in groups.items():
  cells=by_apex[apex];assert len(cells)==8
  points=np.array([[nodes[n] for n in c['nodes']] for c in cells]);metrics=[]
  for c in cells:
   s=samples[c['id']];xx,xy,xz,yy,yz,zz=[float(s[k]) for k in ('xx','xy','xz','yy','yz','zz')];metrics.append([[xx,xy,xz],[xy,yy,yz],[xz,yz,zz]])
  metrics=np.array(metrics);base=np.array([nodes[n] for n in face['face']]);center=base.mean(axis=0)
  e=base[1]-base[0];u=e/np.linalg.norm(e);n=np.cross(e,base[2]-base[0]);n/=np.linalg.norm(n);v=np.cross(n,u);basis=np.stack([u,v],axis=1);origin=nodes[apex]
  local=(base-center)@basis;matrix=np.vstack([local.T,np.ones(3)]);inverse=np.linalg.inv(matrix)
  def values(x):
   p=points.copy();p[:,0,:]=origin+basis@x[:2];return shape(p,metrics)
  def constraints(x):
   q,j,det=values(x);bary=inverse@np.r_[x[:2],1]
   return np.r_[q-.2*x[2],j-.05*x[2],det,bary]
  q0,j0,_=values(np.zeros(3));t0=min(q0.min()/.2,j0.min()/.05)
  fit=minimize(lambda x:-x[2],np.array([0.,0.,t0]),method='SLSQP',constraints={'type':'ineq','fun':constraints},bounds=[(None,None),(None,None),(0,3)],options={'maxiter':100,'ftol':1e-10})
  q,j,det=values(fit.x);t=min(q.min()/.2,j.min()/.05);assert det.min()>0
  result.append(dict(apex=apex,before=t0,proxy_margin=float(t),p=(origin+basis@fit.x[:2]).tolist(),optimizer_success=bool(fit.success),iterations=int(fit.nit)))
 report=dict(status='FIXED_TENSOR_DESIGN_ASSESSMENT_ONLY',prisms=len(result),proxy_infeasible=sum(r['proxy_margin']<1 for r in result),minimum_proxy_margin=min(r['proxy_margin'] for r in result),results=result,helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),elapsed_seconds=time.monotonic()-start,scope='Actual complete candidate geometry; each8-tet prism apex stays at prescribed altitude and inside its face footprint. Tensors remain fixed at the previous native centroids only to assess the construction template. These are proposal proxies, not changed metric targets or native acceptance. Any new candidate requires fresh native actual-query q/J/full-edge and independent geometry checks.')
 (case/'apex_design.json').write_text(json.dumps(report,indent=2)+'\n');print({k:v for k,v in report.items() if k!='results'})
if __name__=='__main__':main()
