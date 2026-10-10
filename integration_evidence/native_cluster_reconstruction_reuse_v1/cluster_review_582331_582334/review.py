"""Review saved cluster evidence only; no solver/build or numerical audit rerun."""
import csv
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import sys

repo=Path(sys.argv[1]).resolve()
records={}
verified={}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def checked(p,digest):
    p.resolve().relative_to(repo)
    assert p.is_file() and not p.is_symlink() and sha(p)==digest,p
    verified[str(p.relative_to(repo))]=digest

def finite(x):
    if isinstance(x,float):assert math.isfinite(x)
    elif isinstance(x,dict):
        for v in x.values():finite(v)
    elif isinstance(x,list):
        for v in x:finite(v)

def read(p):
    r=json.loads(p.read_text());finite(r);return r

def module(name,p):
    spec=importlib.util.spec_from_file_location(name,p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

correct=repo/'ClusterResults/predict_correctness_582331'
v=read(correct/'validation.json');cp=read(correct/'checkpoint.json')
assert v['status']=='PASS' and len(v['stages'])==20
checked(correct/'checkpoint.json',v['checkpoint_sha256'])
source_count=0
for p,h in cp['files_sha256'].items():
    if p.startswith(('Common/','SU2_CFD/','UnitTests/','integration_evidence/','TestCases/')):
        checked(repo/p,h);source_count+=1
for p in (correct/'tools').rglob('*'):
    if p.is_file():checked(p,cp['files_sha256'][str(p.relative_to(correct/'tools'))])
assert cp['binaries']['SU2_CFD']['sha256']==v['binary_sha256']
assert cp['binaries']['test_driver']['sha256']==v['test_binary_sha256']
negative={'native-fixed-point-guard':'FIXED_POINT','invalid_count_low':'ADAP_PREDICT_SNAPSHOTS must','invalid_count_high':'ADAP_PREDICT_SNAPSHOTS must','invalid_cadence_span':'ADAP_PREDICT_SEPARATION *','invalid_negative_filter':'ADAP_PREDICT_TEMPORAL_FILTER must'}
for stage in v['stages']:
    assert stage['status']=='PASS'
    wd=repo/stage['working_directory'];p=wd/(stage['label']+'.log');checked(p,stage['log_sha256'])
    text=p.read_text()
    if wd.name in negative:assert stage['exit_code']==1 and negative[wd.name] in text
    else:
        assert stage['exit_code']==0
        if stage['label'] in ('native','metric'):
            n=int(stage['command'][stage['command'].index('-np')+1]);assert text.count('All tests passed')==n
        else:assert 'Exit Success' in text
extrema=dict(min_quality=1,max_length=0,max_history_defect=0,max_height_error=0)
windows=0;audits=[]
for row in v['cases']:
    case=repo/row['case'];p=case/'independent_unsteady_audit.json';checked(p,row['audit_sha256'])
    a=read(p);assert a['status']=='PASS' and len(a['windows'])==row['adaptations']
    checked(correct/'tools/integration_evidence/audit_native_unsteady.py',a['checker_sha256'])
    for name,h in a['input_sha256'].items():checked(case/name,h)
    ev=read(case/'run_evidence.json');assert ev['status']=='PASS' and ev['solver_exit']==0 and ev['binary_sha256']==v['binary_sha256'] and ev['audit_sha256']==row['audit_sha256']
    actual=list(map(int,re.findall(r'Metric snapshot of time step (\d+)',(case/'solver.log').read_text())));assert actual==row['snapshots']
    for w in a['windows']:
        windows+=1;assert w['min_quality']>=.18-1e-6 and w['max_simpson_length']<=1.8+1e-6
        extrema['min_quality']=min(extrema['min_quality'],w['min_quality']);extrema['max_length']=max(extrema['max_length'],w['max_simpson_length'])
        for h in w['history']:
            assert h['relative_integral_defect']<1e-10 and h['min_density']>0 and h['min_pressure']>0
            extrema['max_history_defect']=max(extrema['max_history_defect'],h['relative_integral_defect'])
        if 'relative_first_height_error' in w and w['relative_first_height_error'] is not None:
            assert w['relative_first_height_error']<1e-8;extrema['max_height_error']=max(extrema['max_height_error'],w['relative_first_height_error'])
    assert a['final_admissibility']['min_density']>0 and a['final_admissibility']['min_pressure']>0
    audits.append(dict(case=row['name'],windows=len(a['windows']),report_sha256=sha(p)))
for start in (7,8,9):
    case=correct/f'partial_restart_{start}_n1m1';a=read(case/'independent_partial_restart_audit.json')
    assert a['status']=='PASS' and a['samples']==list(range(start,10)) and a['min_metric_eigenvalue']>0
    for name,h in a['input_sha256'].items():checked(case/name,h)
    assert a['final_admissibility']['min_density']>0 and a['final_admissibility']['min_pressure']>0
records['correctness']=dict(status='PASS',job=582331,stages=20,replacements=windows,audits=audits,extrema=extrema,binaries=cp['binaries'],source_and_suite_pins_checked=source_count)

cost=module('saved_cost',repo/'integration_evidence/native_cluster_reconstruction_reuse_v1/analyze.py')
account=module('saved_account',repo/'integration_evidence/audit_native_profile_accounting.py')
for job in (582333,582334):
    root=repo/f'ClusterResults/reconstruction_reuse_{job}';v=read(root/'validation.json');cp=read(root/'checkpoint.json')
    assert v['status']=='FAIL' and v['error']=='Candidate changed frozen numerical outputs; campaign stopped' and len(v['cases'])==2
    checked(root/'checkpoint.json',v['checkpoint_sha256'])
    assert cp['binaries']['control']['sha256']=='1a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f'
    assert cp['binaries']['profile']['sha256']==records['correctness']['binaries']['test_driver']['sha256']
    for p,h in cp['files_sha256'].items():checked(repo/p,h)
    for p in root.glob('*.py'):checked(p,cp['files_sha256']['integration_evidence/native_cluster_reconstruction_reuse_v1/'+p.name])
    for p in (root/'tools').rglob('*'):
        if p.is_file():checked(p,cp['files_sha256'][str(p.relative_to(root/'tools'))])
    for p in (root/'inputs').rglob('*'):
        if p.is_file():checked(p,cp['files_sha256']['integration_evidence/native_cluster_campaign_v1/inputs/'+str(p.relative_to(root/'inputs'))])
    for u in v['unit_stages']:
        assert u['status']=='PASS' and u['exit_code']==0
        p=root/f'unit_n{u["ranks"]}'/'solver.log';checked(p,u['log_sha256'])
        assert p.read_text().count('All tests passed')==u['ranks']
    case_records=[]
    for row in v['cases']:
        assert row['status']=='PASS' and row['outcome']['exit_code']==0
        case=root/'cases'/row['case'];manifest=read(case/'collection_manifest.json');assert manifest['status']=='VERIFIED_COMPACT_EXPORT'
        assert row['output_sha256']=={k:info['sha256'] for k,info in manifest['files'].items()}
        for name,info in manifest['files'].items():
            assert Path(name).name==name and (case/name).stat().st_size==info['bytes'];checked(case/name,info['sha256'])
        ev=read(case/'run_evidence.json');assert ev['binary_sha256']==cp['binaries'][row['role']]['sha256'] and ev['command']==row['outcome']['command'] and ev['exit_code']==0
        for name,h in ev['inputs_sha256'].items():checked(case/name,h)
        audit=read(case/'independent_frozen_metric_audit.json');assert audit==row['audit'] and audit['status']=='PASS'
        assert not audit['bad_quality_cells'] and not audit['bad_length_edges'] and audit['transported_metric']['status']=='PASS'
        assert audit['min_quality']>=.18 and audit['max_simpson_length']<=1.8+1e-6 and audit['max_relative_first_height_error']<1e-8
        assert all(x[0]=='PASS' for x in audit['validity'].values())
        b=cost.profile(case,row['workers']);assert b==row['balance']
        timing=account.inspect((case/'solver.log').read_text());assert timing==row['timing_scopes']==read(case/'independent_profile_accounting.json')
        commits=list(map(int,re.search(r'^Native operations .*?:([^;]+);',(case/'solver.log').read_text(),re.M)[1].split()))
        assert commits==[sum(int(p['operations'][i]['committed']) for p in b) for i in range(8)]
        assert all(p['cpu_samples']==p['attempts'] for p in b)
        times=[]
        for rank in range(4):
            t=list(csv.DictReader((case/f'native_frozen_timing_rank_{rank}.csv').open()));assert len(t)==1 and int(t[0]['accepted'])==1;times.append(float(t[0]['remesh_max_seconds']))
        assert max(times)==row['remesh_seconds'] and len(set(times))==1
        priv=[p['private_wall_seconds'] for p in b]
        for k,value in zip(('min','mean','max'),(min(priv),sum(priv)/len(priv),max(priv))):assert abs(timing['worker_profiles'][0]['private reconstruction'][k]-value)<max(1e-6,1e-5*value)
        hot=next(h for p in b for h in p['hotspots'] if int(h['round'])==31436)
        case_records.append(dict(role=row['role'],case=row['case'],audit=audit,remesh_seconds=row['remesh_seconds'],private_max_mean=max(priv)/(sum(priv)/len(priv)),hotspot31436=hot,timing_scopes=timing,machine_samples=row['outcome']['machine_samples']))
    pairs=cost.compare(v['cases']);assert pairs==v['pairs'] and not pairs[0]['numerical_outputs_identical'] and pairs[0]['operation_counts_identical']
    old,new=[root/'cases'/r['case'] for r in v['cases']]
    a=(old/'native_frozen_adapted.su2').read_text().splitlines();b=(new/'native_frozen_adapted.su2').read_text().splitlines();i=next(i for i,s in enumerate(a) if s.startswith('NPOIN'));count=int(a[i].split('=')[1]);assert len(a)==len(b)
    assert a[:i+1]==b[:i+1] and a[i+1+count:]==b[i+1+count:]
    points=[]
    for j,(x,y) in enumerate(zip(a[i+1:i+1+count],b[i+1:i+1+count])):
        xx=list(map(float,x.split()[:2]));yy=list(map(float,y.split()[:2]));assert x.split()[2]==y.split()[2]
        if xx!=yy:points.append(dict(point=j,control=xx,candidate=yy,displacement=math.dist(xx,yy)))
    tensors=[];unchanged=[];csv_diff=0
    for rank in range(4):
        aa=list(csv.DictReader((old/f'native_frozen_target_rank_{rank}.csv').open()));bb=list(csv.DictReader((new/f'native_frozen_target_rank_{rank}.csv').open()));assert len(aa)==len(bb)
        for x,y in zip(aa,bb):
            assert x['point']==y['point']
            csv_diff+=x!=y
            if any(x[k]!=y[k] for k in ('xx','xy','yy')):
                scale=max(abs(float(x[k])) for k in ('xx','xy','yy'));delta=max(abs(float(x[k])-float(y[k])) for k in ('xx','xy','yy'))
                tensors.append(dict(point=int(x['point']),max_entry_difference=delta,tensor_scale=scale,relative_to_tensor_scale=delta/scale))
                if x['x']==y['x'] and x['y']==y['y']:unchanged.append(int(x['point']))
    assert len(points)==41 and len(tensors)==43 and csv_diff==45
    records[str(job)]=dict(status='COMPARISON_FAIL_NUMERICAL_AUDITS_PASS',cases=case_records,pairs=pairs,points_changed=points,tensors_changed=tensors,unchanged_coordinate_tensor_changes=unchanged,noncoordinate_mesh_records_identical=True,max_coordinate_displacement=max(p['displacement'] for p in points),max_tensor_entry_difference_over_scale=max(t['relative_to_tensor_scale'] for t in tensors),mesh_hashes={r['role']:r['output_sha256']['native_frozen_adapted.su2'] for r in v['cases']})
assert records['582333']['mesh_hashes']==records['582334']['mesh_hashes']
assert records['582333']['points_changed']==records['582334']['points_changed']
assert records['582333']['tensors_changed']==records['582334']['tensors_changed']
for job,kind,exitcode,wrapper in [(582331,'predict',0,'integration_evidence/native_cluster_metric_comparison_v1/RunPredictCorrectnessSGE.sh'),(582333,'reconstruction_reuse',1,'integration_evidence/native_cluster_reconstruction_reuse_v1/RunReconstructionReuseSGE.sh'),(582334,'reconstruction_reuse',1,'integration_evidence/native_cluster_reconstruction_reuse_v1/RunReconstructionReuseSGE.sh')]:
    launcher=repo/f'ClusterResults/jobs/{job}_{kind}_launcher';assert int((launcher/'exit_code.txt').read_text())==exitcode
    checked(launcher/Path(wrapper).name,sha(repo/wrapper))
records.update(status='REVIEW_COMPLETE_CORRECTNESS_PASS_COMPARISON_FAIL',files_sha256=verified,scope='Saved-file hashes, logs, audit reports and bounded profile/accounting recomputed; coordinate/tensor differences measured. No local solver/MPI/build or numerical mesh-audit rerun. Exact cause of roundoff remains unproven.')
if len(sys.argv)>2:
    out=Path(sys.argv[2]);assert not out.exists();out.write_text(json.dumps(records,indent=2)+'\n')
print('REVIEW COMPLETE:',len(verified),'unique files verified;20 correctness stages PASS;2 repeatable first-pair byte-identity failures;no local solver runs.')
print('Correctness extrema:',records['correctness']['extrema'])
