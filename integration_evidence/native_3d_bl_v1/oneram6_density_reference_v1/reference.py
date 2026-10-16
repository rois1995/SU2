#!/usr/bin/env python3
"""Independent positive Duffy quadrature using the native frozen point law, without native quadrature/CFD."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import numpy as np

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BUILD=ROOT/'integration_evidence/build-integrated-v2'
OLD=HERE.parent/'oneram6_quadrature_v1'
CAPTURE=HERE.parent/'oneram6_frozen_field_v1'
BINARY=BUILD/'UnitTests/test_driver'

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda:stream.read(1024*1024),b''):h.update(part)
    return h.hexdigest()

def identity(fixture):
    paths=[Path(__file__).resolve(),ROOT/'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp',
           ROOT/'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',ROOT/'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',
           ROOT/'SU2_CFD/src/solvers/CSolver.cpp',ROOT/'Common/include/linear_algebra/blas_structure.hpp',
           BUILD/'build.ninja',BUILD/'meson-info/intro-buildoptions.json',
           BUILD/'SU2_CFD/src/libSU2core.a.p/adaptation_CBoundaryLayerMetric.cpp.o',BINARY,fixture]
    return {str(p.relative_to(ROOT)):sha(p) for p in paths}

def invoke(folder,fixture,bary,expected_success=True):
    folder.mkdir(exist_ok=True)
    inputs=folder/'queries.f64';output=folder/'point_values.f64'
    np.asarray(bary,dtype='<f8').tofile(inputs)
    env=dict(os.environ,SU2_NATIVE_BL_REPLAY=str(fixture),SU2_NATIVE_BL_QUERY_POINTS=str(inputs),
             OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(BUILD/'compile-tmp'))
    command=['taskset','-c','0','nice','-n','19','timeout','--kill-after=5s','120s',str(BINARY),
             '[NativeFacetedVolumeReplay3D]','--abort','--reporter','compact']
    start=time.monotonic()
    with (folder/'run.log').open('w') as log:
        result=subprocess.run(command,cwd=str(folder),env=env,stdout=log,stderr=subprocess.STDOUT)
    if expected_success:
        if result.returncode!=0:raise RuntimeError('Point replay failed: %s exit%d'%(folder,result.returncode))
        values=np.fromfile(output,dtype='<f8').reshape(-1,6)
        assert values.shape==(len(bary),6) and np.all(np.isfinite(values)) and np.all(values[:,4]>0)
        assert np.max(abs(values[:,:4].sum(axis=1)-1))<2e-12 and np.all(values[:,:4]>=0)
    else:
        assert result.returncode not in (0,124,137),result.returncode
        values=None
    report={'exit_code':result.returncode,'seconds':time.monotonic()-start,'command':command,
            'query_sha256':sha(inputs),'values_sha256':sha(output) if output.exists() else None,
            'log_sha256':sha(folder/'run.log')}
    return values,report

def check():
    folder=HERE/'query_checks';folder.mkdir(exist_ok=True)
    fixtures=[('full_unit22',OLD/'cell22_probe/native_wall_unit.dat',OLD/'cell22_probe/replay_rule_150.csv'),
              ('normal13',OLD/'wing_cell13.dat',OLD/'global_ray_v1/replay_rule_150.csv')]
    report={'scope':'Test-only point-query interface, not an independent full-density integral or mesh acceptance.',
            'valid':{},'invalid':{}}
    for label,fixture,rule in fixtures:
        table=np.genfromtxt(rule,delimiter=',',names=True)
        pick=np.unique(np.linspace(0,len(table)-1,64,dtype=int));table=table[pick]
        bary=np.column_stack([table['b'+str(k)] for k in range(4)])
        values,data=invoke(folder/label,fixture,bary)
        relative=float(np.max(abs(values[:,4]/table['target_density']-1)))
        bary_error=float(np.max(abs(values[:,:4]-bary)))
        assert relative<2e-8 and bary_error<2e-10,(label,relative,bary_error)
        data.update(maximum_density_relative_difference=relative,maximum_barycentric_roundtrip=bary_error,
                    rule_sha256=sha(rule),identity=identity(fixture));report['valid'][label]=data
    for label,points in [('bad_size',np.array([.25])),('bad_sum',np.array([[.5,.5,.5,.5]])),
                         ('nonfinite',np.array([[np.nan,.25,.25,.25]]))]:
        _,data=invoke(folder/label,fixtures[0][1],points,False);report['invalid'][label]=data
    report['status']='PASS_POINT_INTERFACE_CHECKS'
    (folder/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print('POINT_INTERFACE_PASS',json.dumps({k:v['maximum_density_relative_difference'] for k,v in report['valid'].items()}),flush=True)

def integrate(cell,order,apex):
    assert cell in (88,157,329) and 3<=order<=256 and 0<=apex<4
    fixture=OLD/'captured_cells_v1'/('cell%d'%cell)/'replay.dat'
    captured=json.loads((OLD/'captured_cells_v1'/('cell%d'%cell)/'input.json').read_text())
    proof=json.loads((CAPTURE/'result.json').read_text())
    assert any(c['cell']==cell and c['raw9_coefficients_match_previous_capture_exactly'] for c in proof['previous_cells'])
    folder=HERE/('cell%d'%cell)/('order%d_apex%d'%(order,apex));folder.mkdir(parents=True,exist_ok=True)
    lock=(folder/'.lock').open('w');fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    pins=identity(fixture);pins[str((CAPTURE/'result.json').relative_to(ROOT))]=sha(CAPTURE/'result.json')
    parameters={'cell':cell,'order':order,'apex':apex,'numpy':np.__version__}
    provenance=folder/'provenance.json'
    if provenance.exists():assert json.loads(provenance.read_text())=={'identity':pins,'parameters':parameters}
    else:provenance.write_text(json.dumps({'identity':pins,'parameters':parameters},indent=2)+'\n')
    if (folder/'result.json').exists():
        result=json.loads((folder/'result.json').read_text());assert result['complete'];print('REUSE',folder,result['integral'],flush=True);return
    nodes,weights=np.polynomial.legendre.leggauss(order);nodes=.5*(nodes+1);weights=.5*weights
    v,w=np.meshgrid(nodes,nodes,indexing='ij');wv,ww=np.meshgrid(weights,weights,indexing='ij')
    order_ids=[apex]+[k for k in range(4) if k!=apex]
    source=np.array(captured['coordinates'],dtype=np.longdouble)
    volume=abs(np.dot(source[1]-source[0],np.cross(source[2]-source[0],source[3]-source[0])))/6
    integral=np.longdouble(0);mass=np.longdouble(0);first=np.zeros(4,dtype=np.longdouble);second=np.zeros((4,4),dtype=np.longdouble)
    minimum=np.inf;maximum=0;minimum_active=np.inf;max_roundtrip=0;seconds=0;queries=0
    scratch=folder/'work';scratch.mkdir(exist_ok=True)
    for i in range(0,order,4):
        checkpoint=folder/('block%03d.json'%i)
        if checkpoint.exists():
            part=json.loads(checkpoint.read_text());assert part['provenance_sha256']==sha(provenance)
        else:
            requested=[];weight_parts=[]
            for u,wu in zip(nodes[i:i+4],weights[i:i+4]):
                canonical=np.column_stack([np.full(v.size,1-u),u*(1-v.ravel()),u*v.ravel()*(1-w.ravel()),u*v.ravel()*w.ravel()])
                beta=np.empty_like(canonical);beta[:,order_ids]=canonical;requested.append(beta)
                weight_parts.append((6*u*u*v*wu*wv*ww).ravel()*volume)
            bary=np.concatenate(requested);qweights=np.concatenate(weight_parts)
            values,call=invoke(scratch,fixture,bary)
            actual=values[:,:4];density=values[:,4];active=values[:,5]>=0
            part={'complete':True,'provenance_sha256':sha(provenance),'index':i,'queries':len(values),
                  'integral':str(np.sum(qweights*density,dtype=np.longdouble)),
                  'mass':str(np.sum(qweights,dtype=np.longdouble)),
                  'first':[str(np.sum(qweights*actual[:,k],dtype=np.longdouble)) for k in range(4)],
                  'second':[[str(np.sum(qweights*actual[:,k]*actual[:,j],dtype=np.longdouble)) for j in range(4)] for k in range(4)],
                  'minimum_density':float(density.min()),'maximum_density':float(density.max()),
                  'minimum_active_distance':float(values[active,5].min()) if np.any(active) else None,
                  'maximum_barycentric_roundtrip':float(np.max(abs(actual-bary))),'call':call}
            checkpoint.write_text(json.dumps(part,indent=2)+'\n')
            # Completed scalar/moment receipts keep identities; grids/values are deterministic regenerable scratch.
            for name in ('queries.f64','point_values.f64'):(scratch/name).unlink()
        integral+=np.longdouble(part['integral']);mass+=np.longdouble(part['mass'])
        first+=np.array(part['first'],dtype=np.longdouble);second+=np.array(part['second'],dtype=np.longdouble)
        minimum=min(minimum,part['minimum_density']);maximum=max(maximum,part['maximum_density'])
        if part['minimum_active_distance'] is not None:minimum_active=min(minimum_active,part['minimum_active_distance'])
        max_roundtrip=max(max_roundtrip,part['maximum_barycentric_roundtrip']);queries+=part['queries'];seconds+=part['call']['seconds']
        if i%max(4,4*(order//32))==0:print('BLOCK',cell,order,apex,i,'/',order,'integral',float(integral),flush=True)
    moments=max(float(abs(mass/volume-1)),float(np.max(abs(first/volume-.25))),
                float(np.max(abs(second/volume-(np.eye(4)*.05+.05)))))
    assert moments<2e-10 and max_roundtrip<2e-10 and queries==order**3
    result={'complete':True,'parameters':parameters,'provenance_sha256':sha(provenance),'integral':float(integral),
            'volume':float(volume),'positive_points':queries,'maximum_normalized_moment_error':moments,
            'maximum_barycentric_roundtrip':max_roundtrip,'minimum_density':minimum,'maximum_density':maximum,
            'minimum_active_distance':minimum_active if np.isfinite(minimum_active) else None,'native_query_wall_seconds':seconds,
            'scope':'Independent fixed positive tensor-Gauss Duffy integration using the unchanged native pointwise composition. No native EdgeBreakpoints/adaptive integration/controller reused. Numerical reference only: order/chart convergence and hidden-mass adequacy not yet established. Not an independent implementation of tensor/geometry laws, rigorous error enclosure, accepted mesh or performance proof.'}
    (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print('REFERENCE_COMPLETE',json.dumps(result),flush=True)

if __name__=='__main__':
    if sys.argv[1:]==['check']:check()
    elif len(sys.argv)==4:integrate(*map(int,sys.argv[1:]))
    else:raise SystemExit('usage: reference.py check | CELL ORDER APEX')
