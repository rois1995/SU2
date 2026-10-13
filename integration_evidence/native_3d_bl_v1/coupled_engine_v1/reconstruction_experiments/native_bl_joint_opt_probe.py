"""Bounded joint placement probe; no native acceptance claim."""
import json,math,itertools,time
from pathlib import Path
import numpy as np
from scipy.optimize import minimize
path=Path('integration_evidence/native_3d_bl_v1/coupled_engine_v1/independent_coupled_BL_rejected_n1.json');data=json.loads(path.read_text())
ids=[n[0] for n in data['nodes']]+[200000];indices={n:i for i,n in enumerate(ids)};original=np.array([n[1:] for n in data['nodes']]+[[0,0,.16873350527066577]])
cells=[]
for row in data['cells']:
 c=row['nodes']
 if 0 in c and 4 in c:cells.extend([[200000 if x==0 else x for x in c],[200000 if x==4 else x for x in c]])
 else:cells.append(c)
cells=np.array([[indices[n] for n in c] for c in cells]);pairs=list(itertools.combinations(range(4),2));pairidx={p:i for i,p in enumerate(pairs)}
def mapped(v,grouped):
 p=original.copy()
 if grouped:a,z,b,h,s=v;p[indices[31]]=[a,0,z];p[indices[47]]=[0,a,z]
 else:a,b,h,s=v;p[indices[31]]=[a,0,a];p[indices[47]]=[0,a,a]
 p[indices[15]]=[b,b,h];p[-1]=[0,0,s];return p

def constraints(v,grouped):
 p=mapped(v,grouped);t=p[cells];e=t[:,1:,:]-t[:,:1,:];det=np.einsum('ij,ij->i',e[:,0,:],np.cross(e[:,1,:],e[:,2,:]));z=t[:,:,2].mean(axis=1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u);tan=1.5**w;normal=np.maximum(.1,2*(.1+.2*z)/2.2)**(-2*w)
 d=np.array([t[:,j,:]-t[:,i,:] for i,j in pairs]);sq=(1+tan)/2*(d[:,:,0]**2+d[:,:,1]**2)+(1-tan)*d[:,:,0]*d[:,:,1]+normal*d[:,:,2]**2
 vol=det*np.sqrt(tan*normal);q=12*(np.maximum(vol,1e-30)/2)**(2/3)/sq.sum(axis=0);jac=np.min(np.array([np.sqrt(2)*vol/np.sqrt(np.prod(sq[[pairidx[tuple(sorted((i,k)))] for k in range(4) if k!=i]],axis=0)) for i in range(4)]),axis=0)
 return np.concatenate((q/.2-1,jac/.05-1,det*100))
reports=[]
for grouped in [False,True]:
 x0=[.186656102409401,.186656102409401,.20056516217177078,.20056516217177078,.16873350527066577] if grouped else [.186656102409401,.20056516217177078,.20056516217177078,.16873350527066577]
 def penalty(x):c=constraints(x,grouped);return np.dot(np.minimum(c,0),np.minimum(c,0))
 start=time.monotonic();r=minimize(penalty,x0,method='SLSQP',bounds=[(.05,.7)]*len(x0),options={'maxiter':120,'ftol':1e-12});c=constraints(r.x,grouped)
 report={'grouped':grouped,'method':'SLSQP','time':time.monotonic()-start,'iterations':int(r.nit),'evaluations':int(r.nfev),'success':bool(r.success),'parameters':r.x.tolist(),'shape_constraint_minimum':float(c.min()),'penalty':float(penalty(r.x)),'scope':'heuristic joint inherited-connectivity shape search; no integrated lengths/native/geometry certificate'};reports.append(report);print(report,flush=True)
Path('integration_evidence/native_3d_bl_v1/coupled_engine_v1/reconstruction_experiments/native_bl_joint_opt_probe.json').write_text(json.dumps(reports,indent=2)+'\n')
