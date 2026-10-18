#!/usr/bin/env python3
"""Joint placement of four new height apices on an exact positive star-split topology."""
import hashlib,itertools,json,time
from pathlib import Path
import numpy as np
from scipy.optimize import least_squares
import probe
HERE=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    seed=HERE/'candidate_0.075.json';data=json.loads(seed.read_text());profile=probe.BASE/(probe.STEM+'_frozen_profile.json');theta=json.loads(profile.read_text())['core']
    ids=[n[0] for n in data['nodes']];index={n:i for i,n in enumerate(ids)};origin=np.array([n[1:] for n in data['nodes']]);cells=np.array([[index[n] for n in c['nodes']] for c in data['cells']])
    pairs=np.array(list(itertools.combinations(range(4),2)));edges=np.unique(np.sort(cells[:,pairs].reshape(-1,2),axis=1),axis=0);incident=np.array([[k for k,e in enumerate(pairs) if i in e] for i in range(4)])
    def normal(z):
        h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u)
        return np.exp(np.maximum(np.log(theta),w*(-2*np.log(h))+(1-w)*np.log(theta)))
    n=normal(origin[cells,:,2].mean(axis=1)) if False else normal(origin[cells][:,:,2].mean(axis=1))
    gx,gw=np.polynomial.legendre.leggauss(32);ew=[];en=[];ei=[]
    for k,(a,b) in enumerate(edges):
        z=origin[a,2];dz=origin[b,2]-z;cuts=[0.,1.]
        if dz:cuts += [(v-z)/dz for v in (.05,.27,.3) if 0<(v-z)/dz<1]
        cuts.sort()
        for low,high in zip(cuts,cuts[1:]):
            t=(low+high)/2+(high-low)*gx/2;ew.extend((high-low)/2*gw);en.extend(normal(z+t*dz)*dz*dz);ei.extend([k]*len(t))
    ew=np.array(ew);en=np.array(en);ei=np.array(ei);start=time.monotonic();calls=0
    def unpack(v):
        x=origin.copy();a,b,c,d=v
        x[index[900002],:2]=(.5-a,.5-a)
        y=.5+b;x[index[900003],:2]=(1-y,y)
        q=.5+c;x[index[900004],:2]=(q,1-q)
        x[index[900005],:2]=(.5+d,.5+d)
        return x
    def quantities(x):
        p=x[cells];v=p[:,1:]-p[:,:1];det=np.einsum('ij,ij->i',v[:,0],np.cross(v[:,1],v[:,2]));e=p[:,pairs[:,1]]-p[:,pairs[:,0]];sq=theta*(e[:,:,0]**2+e[:,:,1]**2)+n[:,None]*e[:,:,2]**2
        dv=det*theta*np.sqrt(n);q=np.sign(dv)*12*np.abs(dv/2)**(2/3)/sq.sum(axis=1);j=np.min(np.sqrt(2)*dv[:,None]/np.sqrt(np.prod(sq[:,incident],axis=2)),axis=1)
        delta=x[edges[:,1],:2]-x[edges[:,0],:2];r=theta*np.sum(delta*delta,axis=1);length=np.bincount(ei,weights=ew*np.sqrt(r[ei]+en),minlength=len(edges))
        return det,q,j,length
    def objective(v):
        nonlocal calls
        calls+=1
        if time.monotonic()-start>30:raise RuntimeError('Explicit30s placement ceiling')
        det,q,j,length=quantities(unpack(v))
        return np.r_[np.maximum(0,1-q/.202),np.maximum(0,1-j/.0505),10*np.maximum(0,-det)/.001,np.maximum(0,length/1.799-1)]
    result=least_squares(objective,[.075]*4,bounds=([.025]*4,[.14]*4),max_nfev=256,ftol=1e-11,xtol=1e-11,gtol=1e-11)
    x=unpack(result.x);data['nodes']=[[id]+list(x[i]) for i,id in enumerate(ids)];path=HERE/'optimized.json';path.write_text(json.dumps(data,separators=(',',':'))+'\n')
    m=probe.measures(data,theta);report=dict(status='FOUR_HEIGHT_APEX_JOINT_PLACEMENT',delta=result.x.tolist(),evaluations=calls,solver_evaluations=result.nfev,cost=result.cost,seconds=time.monotonic()-start,**m,source_sha256=sha(seed),profile_sha256=sha(profile),script_sha256=sha(Path(__file__)),candidate_sha256=sha(path),limits=['Floating joint placement on fixed positive combinatorial topology','Full exact geometry/first-height/no-overlap and native validation still required','No whole native adaptation/transfer/lifecycle claim'])
    (HERE/'optimization.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
