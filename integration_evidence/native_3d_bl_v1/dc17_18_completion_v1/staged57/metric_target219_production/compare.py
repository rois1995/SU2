"""Audit actual production L1/L2 targets on identical original-grid CFD fields."""
import hashlib,json,sys,re
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;REPO=HERE.parents[4]
sys.path.insert(0,str(REPO/'TestCases/adaptation/capability'))
import capcheck
OLD=HERE.parent/'m6_runs/euler_build217_real_cfd50k_inspection_metric'
mesh=capcheck.read_su2(HERE/'L1/m6.su2')
P,old,_=capcheck.read_restart(OLD/'restart_flow_adap_00000.dat')
E=mesh.E;N=len(P);T=mesh.M['WING3D'];W=P[np.unique(T)]
tet=P[E];volume=np.abs(np.einsum('ni,ni->n',np.cross(tet[:,1]-tet[:,0],tet[:,2]-tet[:,0]),tet[:,3]-tet[:,0]))/6
assert (volume>0).all();dual=np.bincount(E.ravel(),weights=np.repeat(volume/4,4),minlength=N)

def metric(f):
    M=np.zeros((N,3,3))
    for a,b,key in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:M[:,a,b]=M[:,b,a]=f['Metric_'+key]
    return M
original=metric(old);eig=np.linalg.eigvalsh(original);floor=eig.min();plateau=np.max(abs(eig-floor),axis=1)<floor*1e-12
root=W[W[:,1]<1e-6];root=root[np.argmax(root[:,0])];tip=W[np.argmax(W[:,0])]
V=P[T];C=V.mean(axis=1);u=V[:,1]-V[:,0];v=V[:,2]-V[:,0];G=np.stack([u,v],axis=1)
normal=np.cross(u,v);normal/=np.linalg.norm(normal,axis=1)[:,None]
basis1=u/np.linalg.norm(u,axis=1)[:,None];basis=np.stack([basis1,np.cross(normal,basis1)],axis=1)

def gradient(values):
    s=values[T]
    return np.einsum('ni,nij->nj',np.linalg.solve(G@G.transpose(0,2,1),(s[:,1:]-s[:,0,None])[...,None])[...,0],G)
grad=gradient(old['Mach']);mag=np.linalg.norm(grad,axis=1);direction=grad/np.maximum(mag[:,None],1e-300)
sections=[]
for yc in np.linspace(.15,1.15,11):
    strip=W[np.abs(W[:,1]-yc)<.025];xi=strip[:,0]-strip[:,1]/np.sqrt(3);chord=np.ptp(xi)
    xc=(C[:,0]-C[:,1]/np.sqrt(3)-xi.min())/chord
    eligible=(np.abs(C[:,1]-yc)<.025)&(C[:,2]>.002)&(xc>.15)&(xc<.8)&(grad[:,0]<0)
    assert eligible.any();cut=np.quantile(-grad[eligible,0],.8);mask=eligible&(-grad[:,0]>=cut)
    sections.append((float(yc),float(chord),mask))
pairs=np.concatenate([E[:,[a,b]] for a in range(4) for b in range(4) if a!=b]);keys=np.unique(pairs[:,0]*N+pairs[:,1]);sources=keys//N;dest=keys%N

def summary(a):return {'min':float(np.min(a)),'p10':float(np.quantile(a,.1)),'median':float(np.median(a)),'p90':float(np.quantile(a,.9)),'max':float(np.max(a))}

def assess(M,audit=False):
    eigen,Q=np.linalg.eigh(M);assert np.isfinite(M).all() and (eigen>0).all()
    budget=dual*np.sqrt(np.prod(eigen,axis=1));total=float(budget.sum());A=M[T].mean(axis=1)
    h=1/np.sqrt(np.einsum('ni,nij,nj->n',direction,A,direction));hn=1/np.sqrt(np.einsum('ni,nij,nj->n',normal,A,normal));tan=np.linalg.eigvalsh(basis@A@basis.transpose(0,2,1));htmin=1/np.sqrt(tan[:,1]);htmax=1/np.sqrt(tan[:,0])
    r={'complexity':total,'relative_complexity_error':abs(total/50000-1),'old_plateau_fraction':float(budget[plateau].sum()/total),'farfield_radius_gt5_fraction':float(budget[np.linalg.norm(P,axis=1)>5].sum()/total),'farfield_radius_gt20_fraction':float(budget[np.linalg.norm(P,axis=1)>20].sum()/total),'minimum_eigenvalue':float(eigen.min()),'maximum_eigenvalue':float(eigen.max()),'maximum_aspect_ratio':float(np.sqrt(eigen[:,-1]/eigen[:,0]).max()),'shock_sections':[],'corners':{}}
    for y,chord,mask in sections:r['shock_sections'].append({'span':y,'triangles':int(mask.sum()),'spacing_fraction_chord':summary(h[mask]/chord)})
    for label,center in [('root_trailing',root),('tip_trailing',tip)]:
        data={}
        for radius in (.005,.02,.05):
            nodes=np.linalg.norm(P-center,axis=1)<radius;faces=np.linalg.norm(C-center,axis=1)<radius
            data[str(radius)]={'points':int(nodes.sum()),'triangles':int(faces.sum()),'budget_fraction':float(budget[nodes].sum()/total)}
            if faces.any():data[str(radius)].update(normal_spacing=summary(hn[faces]),tangent_min_spacing=summary(htmin[faces]),tangent_max_spacing=summary(htmax[faces]))
        r['corners'][label]={'center':center.tolist(),'radii':data}
    if audit:
        assert abs(total/50000-1)<1e-6
        assert eigen.min()>=1e-4*(1-1e-8) and eigen.max()<=1e12*(1+1e-8)
        assert r['maximum_aspect_ratio']<=1e6*(1+1e-8)
        inverse=(Q*(1/np.sqrt(eigen))[:,None,:])@Q.transpose(0,2,1);bad=0;maximum=1.
        for begin in range(0,len(keys),32768):
            i=sources[begin:begin+32768];j=dest[begin:begin+32768];dx=P[i]-P[j];scale=1/(1+np.log(1.3)*np.sqrt(np.maximum(0,np.einsum('ni,nij,nj->n',dx,M[j],dx))))**2
            transported=inverse[i]@M[j]@inverse[i];ratio=np.linalg.eigvalsh(transported)[:,-1]*scale;bad+=int(np.count_nonzero(ratio>1+1e-5));maximum=max(maximum,float(ratio.max()))
        r['gradation']={'directed_edges':len(keys),'violations':bad,'maximum_ratio':maximum,'threshold':1+1e-5};assert bad==0
    return r
report={'scope':'Actual production ComputeMetric from same saved CFD seed, one CFD continuation, original grid; no remeshing or acceptance claim','original':assess(original),'norms':{},'flow_comparison':{}}
previous=None
for norm in (1,2):
    folder=HERE/f'L{norm}';status=json.loads((folder/'status.json').read_text());assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
    xyz,f,_=capcheck.read_restart(folder/'restart_flow.dat');assert np.array_equal(xyz,P)
    text=(folder/'run.log').read_text();assert 'Metric computed in ' in text
    report['norms'][str(norm)]=assess(metric(f),True)
    report['norms'][str(norm)]['restart_sha256']=hashlib.sha256((folder/'restart_flow.dat').read_bytes()).hexdigest()
    report['norms'][str(norm)]['metric_log_lines']=[line for line in text.splitlines() if any(key in line.lower() for key in ('gradation','complexity','metric computed','sensor metric'))]
    for name in ('Density','Momentum_x','Momentum_y','Momentum_z','Energy','Mach','Pressure'):
        if name not in f:continue
        report['flow_comparison'][f'L{norm}_versus_build217_'+name]=float(np.max(np.abs(f[name]-old[name])))
        if previous is not None:assert np.array_equal(f[name],previous[name]),name
    previous=f
(HERE/'comparison.json').write_text(json.dumps(report,indent=2)+'\n')
for norm,data in report['norms'].items():print(norm,'complexity',data['complexity'],'plateau',data['old_plateau_fraction'],'shock',[(s['span'],s['spacing_fraction_chord']['median']) for s in data['shock_sections']], 'corners .05',[(k,v['radii']['0.05']['budget_fraction']) for k,v in data['corners'].items()],'gradation',data['gradation'])
