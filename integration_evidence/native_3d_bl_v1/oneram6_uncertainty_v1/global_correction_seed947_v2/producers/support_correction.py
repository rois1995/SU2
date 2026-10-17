#!/usr/bin/env python3
"""Exact algebra sensor integral + global geometric-support correction; experimental uncertainty only."""
import csv,json,struct,sys,time
from pathlib import Path
import numpy as np
from run import HERE,ROOT,OLD,FROZEN,BUILD,sha,invoke
from correction_support import Proposal,selfcheck,geometry_selfcheck
from support_bound import support
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
sys.path.insert(0,str(FROZEN))
from audit_sensor_mass import duffy_rule,logdet
DOMAIN=np.dtype([('id','<u8'),('nodes','<u8',(4,)),('coordinates','<f8',(4,3)),('sensor','<f8',(4,3,3))])
SHARED=HERE/'correction_domain_v1'


def domain():
    SHARED.mkdir(exist_ok=True)
    if (SHARED/'manifest.json').exists():raise RuntimeError('Original domain already exported; verify its manifest before reuse.')
    start=time.monotonic();capture=json.loads((FROZEN/'result.json').read_text());assert sha(FROZEN/'frozen_trial.npz')==capture['sha256']['frozen_trial.npz']
    data=np.load(FROZEN/'frozen_trial.npz',allow_pickle=False);mesh=read_su2(FROZEN/'Coarse.su2');cells=np.sort(mesh.E,axis=1)
    maps=[]
    for rank in range(4):
        m=np.full(len(mesh.P),-1,dtype=np.int64);ids=data['rank%d_global'%rank];m[ids]=np.arange(len(ids));maps.append(m)
    assert DOMAIN.itemsize==424;path=SHARED/'original.bin'
    with path.open('xb') as out:
        out.write(struct.pack('<QQd',0x4e41545352433344,len(cells),float(data['core'])))
        for begin in range(0,len(cells),4096):
            end=min(len(cells),begin+4096);nodes=cells[begin:end];rows=np.empty(end-begin,dtype=DOMAIN)
            rows['id']=np.arange(begin,end);rows['nodes']=nodes;rows['coordinates']=mesh.P[nodes]
            for rank in range(4):
                mask=data['cell_owner'][begin:end]==rank;local=maps[rank][nodes[mask]];assert np.all(local>=0)
                rows['sensor'][mask]=data['rank%d_sensor'%rank][local]
            assert np.all(np.isfinite(rows['sensor']));rows.tofile(out)
    inputs=[Path(__file__).resolve(),FROZEN/'Coarse.su2',FROZEN/'frozen_trial.npz',FROZEN/'result.json']
    manifest=dict(cells=len(cells),core=float(data['core']),original_sha256=sha(path),preparation_seconds=time.monotonic()-start,
        bytes=path.stat().st_size,representation='Original hosting-rank RAW9; only geometric locator identity tensors are canonicalized.',
        sha256={str(p.relative_to(ROOT)):sha(p) for p in inputs})
    (SHARED/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest),flush=True)


def prepare(label,seed,count,cell=None):
    folder=HERE/label
    if folder.exists():raise RuntimeError('Use a fresh proposal label; preserve existing evidence.')
    folder.mkdir();start=time.monotonic();selfcheck();geometry_selfcheck(folder/'geometry_selfcheck');manifest=json.loads((SHARED/'manifest.json').read_text())
    source=SHARED/'original.bin';assert sha(source)==manifest['original_sha256']
    for name,digest in manifest['sha256'].items():assert sha(ROOT/name)==digest,name
    fixture=OLD/'cell329/replay.dat';proposal=Proposal(fixture);selected=None;sensor=json.loads((FROZEN/'sensor_mass_audit.json').read_text())
    reference=None
    if cell is not None:
        assert 0<=cell<manifest['cells']
        with source.open('rb') as f:f.seek(24+cell*DOMAIN.itemsize);record=np.fromfile(f,dtype=DOMAIN,count=1)
        assert int(record['id'][0])==cell
        coordinates=record['coordinates'][0];selected=support(proposal.g,coordinates.astype(np.longdouble))
        proposal=Proposal(fixture,selected)
        source=folder/'original_cell.bin'
        with source.open('xb') as f:f.write(struct.pack('<QQd',0x4e41545352433344,1,manifest['core']));record.tofile(f)
        matrix=np.tril(record['sensor'][0])+np.swapaxes(np.tril(record['sensor'][0],-1),-1,-2)
        volume=abs(np.linalg.det(coordinates[1:]-coordinates[0]))/6
        beta,weights=duffy_rule();det=[];rho=[]
        for b in beta:
            log=logdet(np.einsum('k,kij->ij',b,matrix));det.append(float(np.exp(log)));rho.append(float(np.exp(.5*log)))
        sensor={'sensor_q18':volume*float(weights@rho),'sensor_lower':volume*float(np.mean(np.exp(logdet(matrix)/3))**1.5),
            'sensor_upper':volume*float(np.sqrt(weights@det))}
        if cell in (88,157,329):
            order=72 if cell==157 else 128
            paths=[HERE.parent/'oneram6_density_reference_v1'/('cell%d'%cell)/('order%d_apex%d'%(order,a))/'result.json' for a in (0,3)]
            reference={'value':sum(json.loads(p.read_text())['integral'] for p in paths)/2,'sha256':{str(p.relative_to(ROOT)):sha(p) for p in paths}}
    points,family,index=proposal.draw(count,seed);pdf=proposal.pdf(points)
    values=np.column_stack([points,1/pdf]);assert np.all(np.isfinite(values))
    path=folder/'queries.bin'
    with path.open('xb') as f:f.write(struct.pack('<QQ',0x4e41545155455259,count));values.astype('<f8').tofile(f)
    np.savez_compressed(folder/'proposal_samples.npz',family=family,index=index,points=points,pdf=pdf)
    metadata=dict(scope='Faceted triangular wall support with face/edge/vertex mixture; actual target is evaluated independently in native code.',
        seed=seed,samples=count,cell=cell,domain=str(source.relative_to(ROOT)),domain_sha256=sha(source),queries_sha256=sha(path),
        proposal_sha256=sha(folder/'proposal_samples.npz'),preparation_seconds=time.monotonic()-start,proposal=proposal.metadata,
        pdf_checks=proposal.pdf_checks,pdf_seconds=proposal.pdf_seconds,maximum_selected_component_pdf_excess=proposal.maximum_selected_pdf_excess,sensor=sensor,reference=reference,
        uncertainty='Empirical sample variance is a heuristic, not a proven error/confidence bound. Ideal continuous proposal coverage does not certify floating membership/PDF.',
        sha256={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),HERE/'correction_support.py',HERE/'support_bound.py',FROZEN/'audit_sensor_mass.py',fixture,SHARED/'manifest.json')})
    (folder/'preparation.json').write_text(json.dumps(metadata,indent=2)+'\n');print(json.dumps(metadata),flush=True)


def run(label,ranks):
    assert ranks in (1,2,4);folder=HERE/label;prep=json.loads((folder/'preparation.json').read_text());source=ROOT/prep['domain']
    assert sha(source)==prep['domain_sha256'] and sha(folder/'queries.bin')==prep['queries_sha256']
    for name,digest in prep['sha256'].items():assert sha(ROOT/name)==digest,name
    dest=folder/('np%d'%ranks);dest.mkdir(exist_ok=True)
    files=['Common/include/adaptation/CNativeDonor3D.hpp','Common/src/adaptation/CNativeDonor3D.cpp',
        'Common/include/adaptation/CNativeDistributed2D.hpp','UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp']
    identity=dict(preparation_sha256=sha(folder/'preparation.json'),sha256={p:sha(ROOT/p) for p in files})
    proof=dest/'extra_provenance.json'
    if proof.exists():assert json.loads(proof.read_text())==identity
    else:proof.write_text(json.dumps(identity,indent=2)+'\n')
    invoke(dest,{'SU2_NATIVE_BL_CORRECTION_DOMAIN':str(source),'SU2_NATIVE_BL_CORRECTION_QUERIES':str(folder/'queries.bin')},OLD/'cell329/replay.dat',ranks)
    native=json.loads((dest/'correction_global.json').read_text());assert native['samples']==prep['samples']
    rows=[]
    for rank in range(ranks):rows.extend(csv.DictReader((dest/('correction_rank%d.csv'%rank)).open()))
    rows.sort(key=lambda row:int(row['row']));assert [int(row['row']) for row in rows]==list(range(prep['samples']))
    weighted=np.array([float(row['weighted_correction']) for row in rows]);assert np.all(np.isfinite(weighted))
    mean=float(weighted.mean());standard=float(weighted.std(ddof=1)/np.sqrt(len(weighted)))
    assert abs(mean-native['correction'])<1e-11*max(1.,abs(mean))
    assert abs(standard-native['empirical_standard_error'])<1e-11*max(1.,standard)
    combined=prep['sensor']['sensor_q18']+mean
    report=dict(native=native,composed_estimate=combined,sensor_Q18=prep['sensor']['sensor_q18'],
        heuristic_three_SE=3*standard,original_sensor_analytic_lower=prep['sensor']['sensor_lower'],original_sensor_analytic_upper=prep['sensor']['sensor_upper'],
        proven_error_bound=False,sampling_sparsity_warning=native['nonzero']<32,CFD_steps=0,production_normalization_changed=False,
        preparation_sha256=sha(folder/'preparation.json'),native_result_sha256=sha(dest/'correction_global.json'),
        rows_sha256={p.name:sha(p) for p in dest.glob('correction_rank*.csv')})
    if prep['reference']:
        ref=prep['reference']['value'];report.update(reference=ref,observed_error=abs(combined-ref),observed_relative_error=abs(combined/ref-1),
            heuristic_three_SE_covers_observed_error=abs(combined-ref)<=3*standard)
    serial=folder/'np1/report.json'
    if ranks>1 and serial.exists():
        old=list(csv.DictReader((folder/'np1/correction_rank0.csv').open()));assert rows==old,'MPI point/source/density outputs differ.'
        report['all_rows_identical_to_serial']=True
    (dest/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)


if __name__=='__main__':
    if sys.argv[1]=='domain':domain()
    elif sys.argv[1]=='prepare':prepare(sys.argv[2],int(sys.argv[3]),int(sys.argv[4]),None if len(sys.argv)<6 else int(sys.argv[5]))
    elif sys.argv[1]=='run':run(sys.argv[2],int(sys.argv[3]))
    else:raise ValueError('domain | prepare LABEL SEED COUNT [CELL] | run LABEL RANKS')
