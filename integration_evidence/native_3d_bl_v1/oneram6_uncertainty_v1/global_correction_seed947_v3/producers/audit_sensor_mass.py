#!/usr/bin/env python3
"""Independent sensor-only mass and conservative wall-box support audit, not full BL quadrature."""
import hashlib
import itertools
import json
import math
from pathlib import Path
import sys
import numpy as np

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def duffy_rule():
    rows=[];weights=[]
    axes=[np.polynomial.legendre.leggauss(n) for n in (3,3,2)]
    for i,j,k in itertools.product(range(3),range(3),range(2)):
        u,v,w=[.5*(axes[d][0][index]+1) for d,index in enumerate((i,j,k))]
        rows.append([1-u,u*(1-v),u*v*(1-w),u*v*w])
        weights.append(6*u*u*v*np.prod([.5*axes[d][1][index] for d,index in enumerate((i,j,k))]))
    bary=np.array(rows);weight=np.array(weights)
    assert np.all(bary>0) and np.all(weight>0)
    for power in itertools.product(range(4),repeat=4):
        if sum(power)>3:continue
        exact=6*math.prod(math.factorial(p) for p in power)/math.factorial(3+sum(power))
        measured=weight@np.prod(bary**np.array(power),axis=1)
        assert abs(measured-exact)<2e-14,(power,measured,exact)
    return bary,weight

def logdet(matrix):
    scale=np.max(abs(matrix),axis=(-2,-1))
    sign,log=np.linalg.slogdet(matrix/scale[...,None,None])
    assert np.all(scale>0) and np.all(sign>0) and np.all(np.isfinite(log))
    return log+3*np.log(scale)

def main():
    proof=json.loads((HERE/'result.json').read_text());assert proof['status']=='PASS_EXACT_COMPLETE_FIRST_TRIAL_FIELD_CAPTURE'
    assert sha(HERE/'frozen_trial.npz')==proof['sha256']['frozen_trial.npz']
    mesh=read_su2(HERE/'Coarse.su2');data=np.load(HERE/'frozen_trial.npz',allow_pickle=False)
    cells=np.sort(mesh.E,axis=1);n=len(cells);host=data['cell_owner']
    local=[];rank_values=[]
    for rank in range(4):
        ids=data['rank%d_global'%rank];mapping=np.full(len(mesh.P),-1,dtype=np.int64);mapping[ids]=np.arange(len(ids));local.append(mapping)
        rank_values.append(data['rank%d_sensor'%rank])
    x=mesh.P[cells].astype(np.longdouble);a=x[:,1]-x[:,0];b=x[:,2]-x[:,0];c=x[:,3]-x[:,0]
    volume=np.abs(np.einsum('ij,ij->i',a,np.cross(b,c)))/6
    assert np.all(volume>0)
    lo=x.min(axis=1);hi=x.max(axis=1);del x,a,b,c
    wall=mesh.P[np.unique(mesh.M['WING3D'])].astype(np.longdouble)
    pad=256*np.finfo(float).eps*max(float(abs(mesh.P).max()),mesh.size)
    gap=np.maximum(np.maximum(wall.min(axis=0)-hi,lo-wall.max(axis=0))-pad,0)
    possible=np.sum(gap*gap,axis=1)<=(.01+pad)**2
    bary,w=duffy_rule();a=(5+3*np.sqrt(5))/20;b=(5-np.sqrt(5))/20
    q4=np.eye(4)*(a-b)+b
    assert np.max(abs(q4.sum(axis=1)-1))<1e-14
    lower=np.empty(n);upper=np.empty(n);q18mass=np.empty(n);q4mass=np.empty(n);constant=np.empty(n,dtype=bool)
    for begin in range(0,n,4096):
        end=min(n,begin+4096);ids=cells[begin:end];raw=np.empty((end-begin,4,3,3))
        for rank in range(4):
            mask=host[begin:end]==rank
            indexes=local[rank][ids[mask]];assert np.all(indexes>=0)
            raw[mask]=rank_values[rank][indexes]
        constant[begin:end]=np.all(raw==raw[:,:1],axis=(1,2,3))
        # tred2 reads the lower triangle; diagnostic copies match its symmetric matrix, raw archive is unchanged.
        matrix=np.tril(raw)+np.swapaxes(np.tril(raw,-1),-2,-1)
        nodal=np.exp(logdet(matrix)/3)
        low_density=nodal.mean(axis=1)**1.5
        mean_det=np.zeros(end-begin,dtype=np.longdouble);fine=np.zeros_like(mean_det);coarse=np.zeros_like(mean_det)
        for beta,weight in zip(bary,w):
            sample=np.einsum('k,nkij->nij',beta,matrix);log=logdet(sample)
            mean_det+=weight*np.exp(log);fine+=weight*np.exp(.5*log)
        for beta in q4:coarse+=.25*np.exp(.5*logdet(np.einsum('k,nkij->nij',beta,matrix)))
        lower[begin:end]=volume[begin:end]*low_density;upper[begin:end]=volume[begin:end]*np.sqrt(mean_det)
        q18mass[begin:end]=volume[begin:end]*fine;q4mass[begin:end]=volume[begin:end]*coarse
    assert np.all(lower<=q18mass*(1+2e-12)) and np.all(upper>=q18mass*(1-2e-12))
    total=lambda values:float(np.sum(values,dtype=np.longdouble))
    rank_summary=[]
    for rank in range(4):
        mask=host==rank
        rank_summary.append({'rank':rank,'cells':int(mask.sum()),'possible_wall_box_cells':int(np.count_nonzero(mask & possible)),
                             'sensor_q18':total(q18mass[mask]),'sensor_lower':total(lower[mask]),'sensor_upper':total(upper[mask])})
    failed=[]
    for cell in (88,157,329):
        assert possible[cell]
        failed.append({'cell':cell,'rank':int(host[cell]),'volume':float(volume[cell]),'sensor_lower':float(lower[cell]),
                       'sensor_upper':float(upper[cell]),'sensor_q18':float(q18mass[cell]),'sensor_q4':float(q4mass[cell]),
                       'scope':'Sensor part only; not an upper bound on full sensor+BL cell contribution.'})
    np.savez_compressed(HERE/'sensor_mass_audit.npz',volume=volume.astype(float),wall_box_possible=possible,
                        lower=lower,upper=upper,q18=q18mass,q4=q4mass,raw_constant=constant)
    report={'status':'PASS_SENSOR_ONLY_DIAGNOSTIC_AUDIT','cells':n,'volume':total(volume),
            'sensor_lower':total(lower),'sensor_upper':total(upper),'sensor_q18':total(q18mass),'sensor_q4':total(q4mass),
            'relative_q4_q18_change':abs(total(q4mass)/total(q18mass)-1),
            'raw_constant_cells':int(constant.sum()),'raw_constant_volume_fraction':total(volume[constant])/total(volume),
            'raw_constant_sensor_q18':total(q18mass[constant]),'possible_wall_box_cells':int(possible.sum()),
            'wall_box_excluded_cells':int((~possible).sum()),'wall_box_possible_volume':total(volume[possible]),
            'wall_box_possible_sensor_q18':total(q18mass[possible]),'ranks':rank_summary,'failed_cells':failed,
            'bound_derivation':'For the effective symmetric P1 sensor A, det(A)^(1/3) is concave. Jensen on t^(3/2) gives integral sqrt(det(A)) >= V*(mean nodal det(A)^(1/3))^(3/2). Concavity of sqrt gives integral sqrt(det(A)) <= V*sqrt(mean det(A)). det(A) is degree3 in barycentrics, whose mean the18-point positive Duffy rule integrates exactly in exact arithmetic. Checked all barycentric moments through degree3. Floating outputs are diagnostic analytic bounds, not outward-rounded machine certificates. These bounds do not include BL composition.',
            'geometry_derivation':'A tetrahedron/whole-wing AABB separation exceeding thickness excludes every faceted wall triangle. Padding is conservative. Overlap only marks possible support; it does not prove an actual band intersection. Scope is this faceted wing, not analytic CAD proxies.',
            'scope':'Original captured trial1, exact owning-rank raw9 matrices and original sorted cells. Sensor-only quadrature/bounds and conservative wall-box exclusion. No full BL integral, global full-density accuracy, new stopping policy, accepted adaptation or performance qualification.',
            'sha256':{'checkpoint':sha(HERE/'frozen_trial.npz'),'capture_proof':sha(HERE/'result.json'),
                      'helper':sha(Path(__file__)),'audit_arrays':sha(HERE/'sensor_mass_audit.npz'),
                      'mesh':sha(HERE/'Coarse.su2'),'eigensolver_source':sha(ROOT/'Common/include/linear_algebra/blas_structure.hpp')},
            'full_goal3_complete':False}
    (HERE/'sensor_mass_audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print('SENSOR_ONLY_AUDIT_PASS lower',report['sensor_lower'],'q18',report['sensor_q18'],'upper',report['sensor_upper'],
          'q4_relative_change',report['relative_q4_q18_change'],'possible_BL_box',report['possible_wall_box_cells'],
          'raw_constant_cells',report['raw_constant_cells'],flush=True)

if __name__=='__main__':main()
