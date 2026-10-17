#!/usr/bin/env python3
"""Independent planar/fade integral checks the same generic correction backend used for M6."""
import csv,json,struct,sys
from pathlib import Path
import numpy as np
from run import HERE,ROOT,sha,invoke
from correction_support import Proposal,selfcheck
from support_correction import DOMAIN


def prepare(label,count=8192):
    folder=HERE/label
    if folder.exists():raise RuntimeError('Use a fresh validation label.')
    folder.mkdir();selfcheck()
    points=np.array([[0,0,0],[1,0,0],[1,1,0],[0,1,0],[0,0,.5],[1,0,.5],[1,1,.5],[0,1,.5]],float)
    ring=[1,2,3,7,4,5];rows=np.empty(6,dtype=DOMAIN)
    for i in range(6):
        nodes=np.array([0,6,ring[i],ring[(i+1)%6]])
        x=points[nodes]
        if np.linalg.det(x[1:]-x[0])<0:nodes[[0,1]]=nodes[[1,0]]
        rows[i]['id']=i;rows[i]['nodes']=nodes;rows[i]['coordinates']=points[nodes];rows[i]['sensor']=2*np.eye(3)
    assert np.isclose(sum(abs(np.linalg.det(row['coordinates'][1:]-row['coordinates'][0]))/6 for row in rows),.5)
    domain=folder/'original.bin'
    with domain.open('xb') as f:f.write(struct.pack('<QQd',0x4e41545352433344,6,2.));rows.tofile(f)
    fixture=folder/'wall.dat';body=['NATIVE_BL_REPLAY_3D 2','4 2','0.1 1 0.2']
    body+=[' '.join(format(v,'.17g') for v in p) for p in points[:4]]+['0 1 2','0 2 3']
    body+=[' '.join(format(v,'.17g') for v in p) for p in rows[0]['coordinates']]+['2']
    body+=[' '.join(format(v,'.17g') for v in matrix.ravel()) for matrix in rows[0]['sensor']]
    fixture.write_text('\n'.join(body)+'\n');proposal=Proposal(fixture)
    sample,family,index=proposal.draw(count,449);pdf=proposal.pdf(sample)
    query=folder/'queries.bin'
    with query.open('xb') as f:f.write(struct.pack('<QQ',0x4e41545155455259,count));np.column_stack([sample,1/pdf]).astype('<f8').tofile(f)
    np.savez_compressed(folder/'samples.npz',points=sample,pdf=pdf,family=family,index=index)
    reference=[]
    for order in (32,64):
        x,w=np.polynomial.legendre.leggauss(order);t=(x+1)/2;fade=1-3*t*t+2*t*t*t
        reference.append(.18*(20-2*np.sqrt(2))+.01*float(w@(2*np.sqrt(2)*np.expm1(.5*np.log(50)*fade))))
    assert abs(reference[0]-reference[1])<1e-13
    proof=dict(scope='Unit-square planar wall, h0=.1/g1/T.2, fluid box height.5, constant original sensor2I/core2.',
        samples=count,seed=449,correction_reference=reference[1],independent_reference_order32_64_difference=abs(reference[0]-reference[1]),
        sensor_integral=np.sqrt(2),source_volume=.5,proposal=proposal.metadata,pdf_checks=proposal.pdf_checks,
        maximum_selected_component_pdf_excess=proposal.maximum_selected_pdf_excess,
        sha256={p.name:sha(p) for p in (domain,fixture,query,folder/'samples.npz')},
        producer_sha256={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),HERE/'correction_support.py')})
    (folder/'preparation.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof),flush=True)


def run(label,ranks):
    folder=HERE/label;prep=json.loads((folder/'preparation.json').read_text())
    for name,digest in prep['sha256'].items():assert sha(folder/name)==digest
    for name,digest in prep['producer_sha256'].items():assert sha(ROOT/name)==digest
    dest=folder/('np%d'%ranks);dest.mkdir(exist_ok=True)
    paths=[Path(__file__).resolve(),ROOT/'Common/src/adaptation/CNativeDonor3D.cpp',ROOT/'Common/include/adaptation/CNativeDonor3D.hpp',
        ROOT/'UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp']
    identity=dict(preparation_sha256=sha(folder/'preparation.json'),sha256={str(p.relative_to(ROOT)):sha(p) for p in paths})
    extra=dest/'extra_provenance.json'
    if extra.exists():assert json.loads(extra.read_text())==identity
    else:extra.write_text(json.dumps(identity,indent=2)+'\n')
    invoke(dest,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(folder/'original.bin'),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(folder/'queries.bin')},folder/'wall.dat',ranks)
    native=json.loads((dest/'correction_global.json').read_text());sample=np.load(folder/'samples.npz',allow_pickle=False)['points']
    data=[]
    for rank in range(ranks):data.extend(csv.DictReader((dest/('correction_rank%d.csv'%rank)).open()))
    data.sort(key=lambda row:int(row['row']));assert [int(row['row']) for row in data]==list(range(len(sample)))
    inside=np.all((sample>=[0,0,0])&(sample<=[1,1,.5]),axis=1);z=sample[:,2]
    t=np.clip((z-.18)/.02,0,1);fade=1-3*t*t+2*t*t*t
    expected=2*np.sqrt(2)*np.exp(.5*np.log(50)*fade)
    maximum=0
    for i,row in enumerate(data):
        present=int(row['donor'])!=2**64-1;assert present==bool(inside[i])
        if present:
            error=max(abs(float(row['sensor'])/(2*np.sqrt(2))-1),abs(float(row['composed'])/expected[i]-1));maximum=max(maximum,error)
            assert error<1e-11,'Owner must interpolate actual2I, never the canonical locatorI.'
        else:assert float(row['sensor'])==float(row['composed'])==float(row['correction'])==0
    error=abs(native['correction']-prep['correction_reference']);sem=native['empirical_standard_error']
    assert error<=3*sem
    result=dict(status='PASS',native=native,independent_reference=prep['correction_reference'],observed_error=error,
        error_over_empirical_SE=error/sem,maximum_independent_point_density_relative_difference=maximum,
        exact_box_membership_agrees=True,locator_identity_not_used_as_sensor=True,three_SE_is_proven_bound=False,
        preparation_sha256=sha(folder/'preparation.json'),native_result_sha256=sha(dest/'correction_global.json'),
        rows_sha256={p.name:sha(p) for p in dest.glob('correction_rank*.csv')})
    if ranks>1 and (folder/'np1/report.json').exists():
        old=list(csv.DictReader((folder/'np1/correction_rank0.csv').open()));assert data==old;result['all_rows_identical_to_serial']=True
    (dest/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)


if __name__=='__main__':
    if sys.argv[1]=='prepare':prepare(sys.argv[2])
    elif sys.argv[1]=='run':run(sys.argv[2],int(sys.argv[3]))
    else:raise ValueError('prepare LABEL | run LABEL RANKS')
