#!/usr/bin/env python3
"""Complete fixed-work frozen-field audit; no thin-BL accuracy or normalization claim."""
import csv,json,sys,time
from pathlib import Path
import numpy as np
from run import HERE,ROOT,OLD,FROZEN,sha,invoke
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
DTYPE=np.dtype([('id','<u8'),('coordinates','<f8',(4,3)),('sensor','<f8',(4,3,3))])
FOLDER=HERE/'complete_volume_seed_v1'
WATCH=(88,157,329,622,2331)


def prepare():
    if (FOLDER/'inputs.json').exists():raise RuntimeError('Inputs already prepared; preserve or verify evidence before reuse.')
    start=time.monotonic();proof=json.loads((FROZEN/'result.json').read_text())
    assert sha(FROZEN/'frozen_trial.npz')==proof['sha256']['frozen_trial.npz']
    data=np.load(FROZEN/'frozen_trial.npz',allow_pickle=False);mesh=read_su2(FROZEN/'Coarse.su2')
    cells=np.sort(mesh.E,axis=1);host=data['cell_owner'];maps=[]
    for rank in range(4):
        a=np.full(len(mesh.P),-1,dtype=np.int64);ids=data['rank%d_global'%rank];a[ids]=np.arange(len(ids));maps.append(a)
    assert DTYPE.itemsize==392 and len(host)==len(cells)
    full=FOLDER/'full_original.bin';control=FOLDER/'controls_original.bin';controls=[]
    with full.open('xb') as out:
        np.array([len(cells)],dtype='<u8').tofile(out)
        for begin in range(0,len(cells),4096):
            end=min(len(cells),begin+4096);chunk=cells[begin:end];rows=np.empty(end-begin,dtype=DTYPE)
            rows['id']=np.arange(begin,end);rows['coordinates']=mesh.P[chunk]
            for rank in range(4):
                mask=host[begin:end]==rank
                indices=maps[rank][chunk[mask]];assert np.all(indices>=0)
                rows['sensor'][mask]=data['rank%d_sensor'%rank][indices]
            assert np.all(np.isfinite(rows['sensor']));rows.tofile(out)
            controls.extend(row.copy() for row in rows if int(row['id']) in WATCH)
    assert [int(row['id']) for row in controls]==list(WATCH)
    with control.open('xb') as out:
        np.array([len(controls)],dtype='<u8').tofile(out);np.array(controls,dtype=DTYPE).tofile(out)
    paths=[Path(__file__).resolve(),FROZEN/'result.json',FROZEN/'frozen_trial.npz',FROZEN/'Coarse.su2',
        FROZEN/'sensor_mass_audit.json',FROZEN/'audit_sensor_mass.py',ROOT/'TestCases/adaptation/capability/capcheck.py']
    identity=dict(scope='All original303359 tetrahedra; hosting-rank RAW9/P1, independent coarsest-sensor comparison. Fixed Q4/Q18, not thin-BL convergence.',
        cells=len(cells),watch=list(WATCH),coarse_sensor_eigenvalue=1e-4,preparation_seconds=time.monotonic()-start,
        files={p.name:sha(p) for p in (full,control)},sha256={str(p.relative_to(ROOT)):sha(p) for p in paths})
    (FOLDER/'inputs.json').write_text(json.dumps(identity,indent=2)+'\n');print(json.dumps(identity),flush=True)


def run(kind,ranks):
    assert kind in ('controls','full') and ranks in (1,4)
    proof=json.loads((FOLDER/'inputs.json').read_text());path=FOLDER/('full_original.bin' if kind=='full' else 'controls_original.bin')
    assert sha(path)==proof['files'][path.name]
    for name,digest in proof['sha256'].items():assert sha(ROOT/name)==digest,name
    folder=FOLDER/('%s_np%d'%(kind,ranks))
    invoke(folder,{'SU2_NATIVE_BL_FIELD':str(path),'SU2_NATIVE_BL_VOLUME_SEED':str(proof['coarse_sensor_eigenvalue']),
        'SU2_NATIVE_BL_SEED_WATCH':','.join(map(str,proof['watch']))},OLD/'cell329/replay.dat',ranks)
    output=folder/'volume_seed_global.json';result=json.loads(output.read_text())
    assert result['cells']==(proof['cells'] if kind=='full' else len(WATCH)) and result['query_points']==22*result['cells']
    for key in ('sensor_q4','sensor_q18','composed_q4','composed_q18','minimum_q4','minimum_q18'):assert np.isfinite(result[key]) and result[key]>0
    assert result['composed_q18']>=result['sensor_q18']*(1-1e-12)
    rows=[]
    for rank in range(ranks):rows.extend(csv.DictReader((folder/('seed_watch_rank%d.csv'%rank)).open()))
    rows.sort(key=lambda row:int(row['cell']));assert [int(row['cell']) for row in rows]==list(WATCH)
    refs=[]
    for row in rows:
        cell=int(row['cell'])
        if cell not in (88,157,329):continue
        order=72 if cell==157 else 128
        paths=[HERE.parent/'oneram6_density_reference_v1'/('cell%d'%cell)/('order%d_apex%d'%(order,apex))/'result.json' for apex in (0,3)]
        reference=sum(json.loads(p.read_text())['integral'] for p in paths)/2
        actual=abs(float(row['composed_q18'])-reference);indicator=float(row['absolute_rule_difference'])
        refs.append(dict(cell=cell,reference=reference,Q4=float(row['composed_q4']),Q18=float(row['composed_q18']),
            observed_absolute_error=actual,raw_indicator=indicator,indicator_covers_observed_error=indicator>=actual,
            references={str(p.relative_to(ROOT)):sha(p) for p in paths}))
    verification=dict(complete_input_rows=True,query_count_matches=True,reference_cells=refs,CFD_steps=0,
        global_result_sha256=sha(output),input_manifest_sha256=sha(FOLDER/'inputs.json'),
        rank_detail_sha256={p.name:sha(p) for p in folder.glob('seed_watch_rank*.csv')},thin_BL_accuracy_certified=False)
    serial=FOLDER/'controls_np1/volume_seed_global.json'
    if kind=='controls' and ranks>1 and serial.exists():
        baseline=json.loads(serial.read_text());maximum=0
        for key,value in result.items():
            if key not in ('ranks','seconds') and type(value) in (int,float):
                maximum=max(maximum,abs(value-baseline[key])/max(1.,abs(baseline[key])))
        assert maximum<1e-13;verification['maximum_MPI_relative_difference']=maximum
        old=list(csv.DictReader((FOLDER/'controls_np1/seed_watch_rank0.csv').open()));assert rows==old
        verification['all_watched_queries_identical_to_serial']=True
    if kind=='full':
        audit=json.loads((FROZEN/'sensor_mass_audit.json').read_text())
        verification['independent_sensor_Q18_relative_difference']=abs(result['sensor_q18']/audit['sensor_q18']-1)
        assert verification['independent_sensor_Q18_relative_difference']<3e-12
        lower=json.loads((HERE/'wall_prism_budget_summary_v1.json').read_text())
        bound=min(lower['subset_statistical_lower_estimates'])
        verification.update(conditional_BL_subset_lower=bound,
            minimum_missing_mass_Q4=max(0.,bound-result['composed_q4']),minimum_missing_mass_Q18=max(0.,bound-result['composed_q18']),
            minimum_missing_mass_coarsest_Q4=max(0.,bound-result['minimum_q4']),
            minimum_missing_mass_coarsest_Q18=max(0.,bound-result['minimum_q18']),
            global_sum_rule_difference=result['sum_absolute_cell_rule_difference'],
            statistical_lower_evidence_sha256=sha(HERE/'wall_prism_budget_summary_v1.json'))
    (folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n');print(json.dumps(dict(result=result,verification=verification)),flush=True)


if __name__=='__main__':
    if sys.argv[1]=='prepare':prepare()
    elif sys.argv[1]=='run':run(sys.argv[2],int(sys.argv[3]))
    else:raise ValueError('prepare | run controls|full 1|4')
