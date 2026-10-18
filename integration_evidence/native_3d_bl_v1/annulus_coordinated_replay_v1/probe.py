#!/usr/bin/env python3
"""Bounded placement experiment on an exact saved patch; no native acceptance claim."""
import hashlib,itertools,json,os,time
from pathlib import Path
import numpy as np
from scipy.optimize import least_squares
HERE=Path(__file__).resolve().parent;BASE=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v4_n1'
STEM='symmetry_annulus_bl_budget_20_m1';SOURCE=BASE/(STEM+'_rejected_rank0_trial_before.json')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    old=json.loads(SOURCE.read_text());profile=json.loads((BASE/(STEM+'_frozen_profile.json')).read_text());theta=profile['core']
    ids=[n[0] for n in old['nodes']];nodes=np.array([n[1:] for n in old['nodes']],dtype=float);index={n:i for i,n in enumerate(ids)}
    new_id=max(ids)+100;index[new_id]=len(ids);ids.append(new_id)
    origin=np.vstack([nodes,[.8295189704274959,0,.17048102957250408]])
    cells=[]
    for c in old['cells']:
        v=[index[n] for n in c['nodes']]
        if 1 in c['nodes'] and 5 in c['nodes']:
            a=v.copy();b=v.copy();a[c['nodes'].index(5)]=index[new_id];b[c['nodes'].index(1)]=index[new_id];cells.extend([a,b])
        else:cells.append(v)
    cells=np.array(cells);pairs=np.array(list(itertools.combinations(range(4),2)));edges=np.unique(np.sort(cells[:,pairs].reshape(-1,2),axis=1),axis=0)
    incident=np.array([[k for k,(a,b) in enumerate(pairs) if i in (a,b)] for i in range(4)])
    def normal(z):
        h=np.maximum(.1,(.1+.2*z)/1.1);u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u)
        return np.exp(np.maximum(np.log(theta),w*(-2*np.log(h))+(1-w)*np.log(theta)))
    def measures(x):
        p=x[cells];v=p[:,1:]-p[:,:1];det=np.einsum('ij,ij->i',v[:,0],np.cross(v[:,1],v[:,2]));n=normal(p[:,:,2].mean(axis=1))
        d=p[:,pairs[:,1]]-p[:,pairs[:,0]];sq=theta*(d[:,:,0]**2+d[:,:,1]**2)+n[:,None]*d[:,:,2]**2
        volume=det*theta*np.sqrt(n);q=np.sign(volume)*12*np.abs(volume/2)**(2/3)/sq.sum(axis=1)
        j=np.min(np.sqrt(2)*volume[:,None]/np.sqrt(np.prod(sq[:,incident],axis=2)),axis=1)
        de=x[edges[:,1]]-x[edges[:,0]];z=(x[edges[:,1],2]+x[edges[:,0],2])/2
        length=np.sqrt(theta*(de[:,0]**2+de[:,1]**2)+normal(z)*de[:,2]**2)
        return det,q,j,length
    reports={};start=time.monotonic()
    for mode in ['interior_only','joint_surface_volume']:
        dofs=[(index[n],a) for n in [5263,13650] for a in range(3)]+[(index[new_id],a) for a in [0,2]]
        if mode=='joint_surface_volume':dofs += [(index[n],a) for n in [11551,34607] for a in [0,2]]
        def unpack(v):
            p=origin.copy()
            for (i,a),value in zip(dofs,v):p[i,a]=value
            return p
        calls=0;seed=np.array([origin[i,a] for i,a in dofs])
        def objective(v):
            nonlocal calls
            calls+=1
            if time.monotonic()-start>30:raise RuntimeError('Explicit30s replay work ceiling')
            det,q,j,length=measures(unpack(v))
            return np.r_[np.maximum(0,1-q/.202),np.maximum(0,1-j/.0505),10*np.maximum(0,-det)/.001,.1*np.maximum(0,length/1.8-1)]
        before=measures(origin);sol=least_squares(objective,seed,bounds=(np.zeros(len(seed))+1e-8,np.ones(len(seed))-1e-8),max_nfev=160,ftol=1e-10,xtol=1e-10,gtol=1e-10)
        x=unpack(sol.x);det,q,j,length=measures(x)
        # Full straight-edge integrals at geometric breakpoints; proxy values do not establish size progress.
        gx,gw=np.polynomial.legendre.leggauss(48);full=[]
        for a,b in edges:
            p=x[a];d=x[b]-p;cuts=[0.,1.]
            if d[2]:cuts += [(z-p[2])/d[2] for z in (.05,.27,.3) if 0<(z-p[2])/d[2]<1]
            cuts=sorted(cuts);value=0.
            for low,high in zip(cuts,cuts[1:]):
                t=(low+high)/2+(high-low)*gx/2
                value+=(high-low)/2*float(gw@np.sqrt(theta*(d[0]**2+d[1]**2)+normal(p[2]+t*d[2])*d[2]**2))
            full.append(value)
        reports[mode]=dict(evaluations=calls,solver_evaluations=sol.nfev,cost=sol.cost,minimum_determinant=float(min(det)),minimum_q=float(min(q)),minimum_J=float(min(j)),
            maximum_full_edge=max(full),full_excess=float(sum(max(0,l-1.8) for l in full)),bad_shape_cells=int(sum((q<.2)|(j<.05))),
            before_q=float(min(before[1])),before_J=float(min(before[2])),shape_thresholds_unchanged=True,
            limits=['Floating placement search; exact topology/geometry/source/height/full native change gates still required','No whole-mesh or adaptation acceptance claim'])
        result=dict(old);result['nodes']=[[n]+list(x[i]) for i,n in enumerate(ids)];result['cells']=[dict(id=i,nodes=[ids[k] for k in c]) for i,c in enumerate(cells)]
        surface=[]
        for f in old['surface']:
            v=f['nodes']
            if 1 in v and 5 in v:
                a=v.copy();b=v.copy();a[v.index(5)]=new_id;b[v.index(1)]=new_id;surface.extend([dict(facet=f['facet'],nodes=a),dict(facet=f['facet'],nodes=b)])
            else:surface.append(f)
        result['surface']=surface
        (HERE/(mode+'.json')).write_text(json.dumps(result,separators=(',',':'))+'\n');print(mode,json.dumps(reports[mode]),flush=True)
    summary=dict(status='EXACT_PATCH_PLACEMENT_EXPERIMENT',source_sha256=sha(SOURCE),profile_sha256=sha(BASE/(STEM+'_frozen_profile.json')),script_sha256=sha(Path(__file__)),seconds=time.monotonic()-start,results=reports)
    (HERE/'report.json').write_text(json.dumps(summary,indent=2)+'\n')
if __name__=='__main__':main()
