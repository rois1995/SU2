"""Combine independent recovered sensors; native gradation is deliberately excluded."""
import json
import numpy as np
import metric_floor_diagnostic as diagnostic


def intersection(A, B):
    # Zero is an absent sensor demand; physical SPD bounds are imposed later.
    az = np.max(abs(A),axis=(1,2)) == 0
    bz = np.max(abs(B),axis=(1,2)) == 0
    if np.any(az | bz):
        result = A+B
        active = ~(az|bz)
        if active.any(): result[active] = intersection(A[active],B[active])
        return result
    scale = np.maximum(np.max(abs(A),axis=(1,2)),np.max(abs(B),axis=(1,2)))
    assert (scale > 0).all()
    A = A/scale[:,None,None]; B = B/scale[:,None,None]
    val,Q = np.linalg.eigh(A+B)
    assert (val>0).all()
    sqrt = np.einsum('nik,nk,njk->nij',Q,np.sqrt(val),Q)
    inv = np.einsum('nik,nk,njk->nij',Q,1/np.sqrt(val),Q)
    val,Q = np.linalg.eigh(inv@A@inv)
    common = np.einsum('nik,nk,njk->nij',Q,np.maximum(val,1-val),Q)
    result = sqrt@common@sqrt
    return .5*(result+result.transpose(0,2,1))*scale[:,None,None]


def build(H, dual, local, norm):
    tensors = []
    for sensor in range(2):
        if not np.any(H[:,sensor]): continue
        eig,Q = diagnostic.sensor_spectrum(H[:,sensor],dual,local,norm=norm)
        tensors.append(np.einsum('nik,nk,njk->nij',Q,eig,Q))
    if not tensors:
        return np.broadcast_to(np.eye(3)*(50000/dual.sum())**(2/3),H[:,0].shape).copy()
    return tensors[0] if len(tensors)==1 else intersection(*tensors)


def tests():
    R = np.random.default_rng(123)
    A = R.normal(size=(12,3,3)); A = A@A.transpose(0,2,1)+np.eye(3)
    B = R.normal(size=(12,3,3)); B = B@B.transpose(0,2,1)+np.eye(3)
    C = intersection(A,B)
    assert np.min(np.linalg.eigvalsh(C-A)) >= -1e-12
    assert np.min(np.linalg.eigvalsh(C-B)) >= -1e-12
    assert np.allclose(C,intersection(B,A),rtol=1e-12,atol=1e-12)
    assert np.allclose(intersection(A,A),A,rtol=1e-12,atol=1e-12)
    assert np.array_equal(intersection(A,np.zeros_like(A)),A)
    assert not np.any(intersection(np.zeros_like(A),np.zeros_like(A)))
    h=np.stack([A,np.zeros_like(A)],axis=1)
    dual=np.ones(len(A))
    vals,Q=diagnostic.sensor_spectrum(A,dual,True)
    expected=np.einsum('nik,nk,njk->nij',Q,vals,Q)
    assert np.allclose(build(h,dual,True,1),expected)
    return dict(sensor_dominance=True,commutation=True,idempotence=True,absent_and_flat_sensor=True)


cache = np.load(diagnostic.OUT/'independent_recovery.npz')
H,dual = cache['hessians'],cache['dual']
mesh = diagnostic.capcheck.read_su2(diagnostic.RUN/'m6.su2')
P,f,_ = diagnostic.capcheck.read_restart(diagnostic.RUN/'restart_flow_adap_00000.dat')
original = np.zeros((len(P),3,3))
for a,b,k in [(0,0,'XX'),(0,1,'XY'),(0,2,'XZ'),(1,1,'YY'),(1,2,'YZ'),(2,2,'ZZ')]:
    original[:,a,b] = original[:,b,a] = f['Metric_'+k]
original_eig = np.linalg.eigvalsh(original)
floor = original_eig.min()
plateau = np.max(abs(original_eig-floor),axis=1)<floor*1e-12
T=mesh.M['WING3D']; W=P[np.unique(T)]
root=W[W[:,1]<1e-6]; root=root[np.argmax(root[:,0])]
tip=W[np.argmax(W[:,0])]
root_mask=np.linalg.norm(P-root,axis=1)<.05
tip_mask=np.linalg.norm(P-tip,axis=1)<.05
V=P[T];C=V.mean(axis=1);u=V[:,1]-V[:,0];v=V[:,2]-V[:,0]
G=np.stack([u,v],axis=1);s=f['Mach'][T]
gradient=np.einsum('ni,nij->nj',np.linalg.solve(G@G.transpose(0,2,1),(s[:,1:]-s[:,0,None])[...,None])[...,0],G)
gradient/=np.maximum(np.linalg.norm(gradient,axis=1)[:,None],1e-300)
sections=[]
for yc,shock in [(.55,.55),(.85,.45)]:
    strip=W[np.abs(W[:,1]-yc)<.025];xi=strip[:,0]-strip[:,1]/np.sqrt(3)
    chord=np.ptp(xi);xc=(C[:,0]-C[:,1]/np.sqrt(3)-xi.min())/chord
    mask=(np.abs(C[:,1]-yc)<.025)&(C[:,2]>.002)&(abs(xc-shock)<.05)
    sections.append((yc,chord,mask))


def assess(M):
    eig=np.linalg.eigvalsh(M)
    assert (eig>0).all() and np.isfinite(eig).all()
    budget=dual*np.sqrt(np.prod(eig,axis=1));total=budget.sum()
    A=M[T].mean(axis=1)
    h=1/np.sqrt(np.einsum('ni,nij,nj->n',gradient,A,gradient))
    return dict(complexity=float(total),old_plateau_fraction=float(budget[plateau].sum()/total),
                root_corner_fraction=float(budget[root_mask].sum()/total),tip_corner_fraction=float(budget[tip_mask].sum()/total),
                shock_spacing_fraction_chord={str(y):float(np.median(h[mask])/chord) for y,chord,mask in sections})


report=dict(scope='Independent full-grid two-sensor diagnostic; no native gradation or production correction qualification',self_tests=tests(),recorded_production_metric=assess(original),variants={})
for local,norm in [(False,1),(True,1),(True,2),(True,4)]:
    M=build(H,dual,local,norm)
    eig,Q=np.linalg.eigh(M)
    # Check the old construction at the scalar fixed by the observed plateau.
    if not local:
        scale=floor/eig[plateau].min()
        report['old_before_gradation_at_observed_floor']=assess(M*scale)
        report['old_raw_plateau_eigenvalue_relative_spread']=float(np.ptp(eig[plateau])/eig[plateau].min())
    bounded=diagnostic.normalize_bounded(eig,dual)
    M=np.einsum('nik,nk,njk->nij',Q,bounded,Q)
    key=('local' if local else 'global')+'_floor_p'+str(norm)
    report['variants'][key]=assess(M)
    if local and norm==1:
        np.savez(diagnostic.OUT/'local_floor_ungraded_candidate.npz',metric=M,points=P)
assert abs(report['old_before_gradation_at_observed_floor']['complexity']-40237.3)<.1
(diagnostic.OUT/'combined_results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
