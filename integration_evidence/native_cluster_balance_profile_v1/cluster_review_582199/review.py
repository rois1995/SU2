"""Verify downloaded job582199 evidence and summarize recorded costs; no solver or mesh audit rerun."""
import csv
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import sys

job=Path(sys.argv[1]).resolve()
repo=job.parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def finite(value):
    if isinstance(value,float):assert math.isfinite(value),value
    elif isinstance(value,dict):
        for v in value.values():finite(v)
    elif isinstance(value,list):
        for v in value:finite(v)

def saved_module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module

analysis=saved_module('saved_balance_analyze',job/'analyze.py')
account=saved_module('saved_profile_accounting',job/'tools/integration_evidence/audit_native_profile_accounting.py')
v=json.loads((job/'validation.json').read_text());finite(v)
assert v['status']=='PASS' and len(v['cases'])==16 and len(v['pairs'])==8
checkpoint=json.loads((job/'checkpoint.json').read_text())
assert sha(job/'checkpoint.json')==v['checkpoint_sha256']
assert checkpoint['binaries']['control']['sha256']=='b8bb7bc0bf3ba701232513d0c4d0e812e3483cac605da6ecd13e1a4845b7971e'
source_count=0
for name,digest in checkpoint['files_sha256'].items():
    if name.startswith(('Common/','SU2_CFD/','UnitTests/')) or name in ('meson.build',):
        assert sha(repo/name)==digest,name
        source_count+=1
hashes={}
for path in job.glob('*.py'):
    name='integration_evidence/native_cluster_balance_profile_v1/'+path.name
    assert sha(path)==checkpoint['files_sha256'][name],name
    hashes[str(path.relative_to(job))]=sha(path)
for path in (job/'tools').rglob('*'):
    if not path.is_file():continue
    name=str(path.relative_to(job/'tools'))
    assert sha(path)==checkpoint['files_sha256'][name],name
    hashes[str(path.relative_to(job))]=sha(path)
for path in (job/'inputs').rglob('*'):
    if not path.is_file():continue
    suffix=path.relative_to(job/'inputs')
    name='integration_evidence/native_cluster_campaign_v1/inputs/'+str(suffix)
    assert sha(path)==checkpoint['files_sha256'][name],name
    hashes[str(path.relative_to(job))]=sha(path)
launcher=repo/'ClusterResults/jobs/582199_balance_profile_launcher'
assert (launcher/'exit_code.txt').read_text().strip()=='0'
assert not (launcher/'launcher.err').read_text().strip()
wrapper='integration_evidence/native_cluster_balance_profile_v1/RunBalanceProfileSGE.sh'
assert sha(launcher/'RunBalanceProfileSGE.sh')==checkpoint['files_sha256'][wrapper]
units=[]
for u in v['unit_stages']:
    assert u['status']=='PASS' and u['exit_code']==0
    path=job/('unit_n'+str(u['ranks']))/'solver.log'
    assert sha(path)==u['log_sha256']
    counts=re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',path.read_text())
    assert len(counts)==u['ranks'] and all(int(t)==39 for _,t in counts)
    units.append(dict(ranks=u['ranks'],tests_per_rank=39,assertions_per_rank=[int(a) for a,_ in counts]))
    hashes[str(path.relative_to(job))]=sha(path)
assert [u['ranks'] for u in units]==[1,2,4]
rows=[];selected_count=0
extrema=dict(min_quality=1.,max_length=0.,max_relative_height=0.,max_transported_directional_defect=0.)
for c in v['cases']:
    assert c['status']=='PASS' and c['outcome']['exit_code']==0
    case=job/'cases'/c['case']
    manifest=json.loads((case/'collection_manifest.json').read_text())
    assert manifest['status']=='VERIFIED_COMPACT_EXPORT'
    assert c['output_sha256']=={name:info['sha256'] for name,info in manifest['files'].items()}
    for name,info in manifest['files'].items():
        assert Path(name).name==name
        path=case/name;path.resolve().relative_to(job)
        assert not path.is_symlink() and path.stat().st_size==info['bytes'] and sha(path)==info['sha256'],path
        assert info['storage']=='copy'
        hashes[str(path.relative_to(job))]=info['sha256'];selected_count+=1
    evidence=json.loads((case/'run_evidence.json').read_text())
    assert evidence['phase']=='terminal' and evidence['exit_code']==0
    assert evidence['binary_sha256']==checkpoint['binaries'][c['role']]['sha256']
    assert evidence['command']==c['outcome']['command']
    for name,digest in evidence['inputs_sha256'].items():assert sha(case/name)==digest,name
    audit=json.loads((case/'independent_frozen_metric_audit.json').read_text());finite(audit)
    assert audit==c['audit'] and audit['status']=='PASS'
    assert not audit['bad_quality_cells'] and not audit['bad_length_edges']
    assert all(group[0]=='PASS' for group in audit['validity'].values())
    assert all(ref['all_declared_features_exactly_retained'] and abs(ref['covered_arc_over_period']-1)<1e-10 and ref['max_corresponding_parameter_deviation']<=1e-6 for ref in audit['original_reference'])
    assert audit['transported_metric']['status']=='PASS'
    assert audit['min_quality']>=.18-1e-12 and audit['max_simpson_length']<=1.8+1e-6
    extrema['min_quality']=min(extrema['min_quality'],audit['min_quality'])
    extrema['max_length']=max(extrema['max_length'],audit['max_simpson_length'])
    extrema['max_transported_directional_defect']=max(extrema['max_transported_directional_defect'],audit['transported_metric']['max_relative_directional_tensor_defect'])
    height=audit['max_relative_first_height_error']
    if height is not None:assert height<=1e-8;extrema['max_relative_height']=max(extrema['max_relative_height'],height)
    log=(case/'solver.log').read_text()
    timing=account.inspect(log)
    assert timing==c['timing_scopes']==json.loads((case/'independent_profile_accounting.json').read_text())
    times=[]
    for rank in range(4):
        timing_rows=list(csv.DictReader((case/f'native_frozen_timing_rank_{rank}.csv').read_text().splitlines()))
        assert len(timing_rows)==1
        tr=timing_rows[0];assert int(tr['rank'])==rank and int(tr['ranks'])==4 and int(tr['accepted'])==1
        times.append(float(tr['remesh_max_seconds']))
    assert max(times)==c['remesh_seconds'] and all(t==times[0] for t in times)
    row=dict(case=c['case'],kind=c['kind'],workers=c['workers'],partition=c['repartition'],role=c['role'],remesh_seconds=c['remesh_seconds'],
             points=audit['points'],triangles=audit['triangles'],timing_scopes=timing)
    if c['role']=='profile':
        b=analysis.profile(case,c['workers']);assert b==c['balance']
        printed=list(map(int,re.search(r'^Native operations .*?:([^;]+);',log,re.M)[1].split()))
        assert printed==[sum(int(p['operations'][i]['committed']) for p in b) for i in range(8)]
        priv=[p['private_wall_seconds'] for p in b]
        for key,value in zip(('min','mean','max'),(min(priv),sum(priv)/len(priv),max(priv))):
            assert abs(timing['worker_profiles'][0]['private reconstruction'][key]-value)<=max(1e-6,1e-5*value)
        cpu=sum(p['private_process_cpu_seconds'] for p in b)
        attempts=sum(p['attempts'] for p in b)
        assert sum(p['cpu_samples'] for p in b)==attempts
        hot=[h for p in b for h in p['hotspots']]
        joint=[h for h in hot if int(h['coordinated'])]
        query_totals=list(map(int,re.search(r'^Native private target requests/evaluations/dynamic evictions: (.*)$',log,re.M)[1].split()))
        private_queries=[sum(int(op[key]) for p in b for op in p['operations']) for key in ('requests','evaluations','evictions')]
        assert all(part<=whole for part,whole in zip(private_queries,query_totals))
        row.update(private_wall_max_mean=max(priv)/(sum(priv)/len(priv)),private_wall_seconds_by_rank=priv,
                   private_cpu_seconds_by_rank=[p['private_process_cpu_seconds'] for p in b],private_cpu_wall_ratio=cpu/sum(priv),
                   private_attempts=attempts,private_queries=private_queries,all_round_query_totals=query_totals,
                   bulk_split_private_fraction=sum(p['operations'][5]['private_wall_seconds'] for p in b)/sum(priv),
                   rank_operation_totals=[dict(rank=p['rank'],operations=p['operations'],top32_wall_fraction=p['hotspot_wall_fraction']) for p in b],
                   bounded_hotspot_count=len(hot),retained_joint_count=len(joint),retained_joint_wall_seconds=sum(float(h['private_wall_seconds']) for h in joint),
                   retained_joint_committed=sum(int(h['committed']) for h in joint),
                   largest_hotspots=sorted(hot,key=lambda h:float(h['private_wall_seconds']),reverse=True)[:5],
                   profile_output_seconds=float(re.search(r'^Native balance profile output seconds .*?: ([^;]+);',log,re.M)[1]))
        coordinated=re.search(r'^Native coordinated repair: (\d+) commits, ([^ ]+) seconds',log,re.M)
        assert coordinated is not None
        row['coordinated_commits']=int(coordinated[1]);row['coordinated_phase_max_seconds']=float(coordinated[2])
        assert row['retained_joint_committed']<=row['coordinated_commits']
        for h in row['largest_hotspots']:
            h['private_wall_fraction_of_remesh']=float(h['private_wall_seconds'])/c['remesh_seconds']
    else:assert not list(case.glob('native_balance_*.csv'))
    samples=c['outcome']['machine_samples']
    row['observed_hosts']=sorted({s['host'] for s in samples})
    row['other_compute_process_counts']=sorted({s['other_compute_processes'] for s in samples})
    row['affinities']=sorted({tuple(s['owned_rank_affinity']) for s in samples})
    rows.append(row)
pairs=analysis.compare(v['cases']);assert pairs==v['pairs'] and all(p['numerical_outputs_identical'] for p in pairs)
assert all(p.is_file() or p.is_dir() for p in job.rglob('*'))
assert not any(p.is_symlink() for p in job.rglob('*'))
assert not (job/'objects').exists() and not v.get('pending_work_folder')
profile_rows=[r for r in rows if r['role']=='profile']
assert selected_count+8==v['export_storage_counts']['copy'] and v['export_storage_counts']['hardlink']==0
actual_bytes=sum(p.stat().st_size for p in job.rglob('*') if p.is_file())
result=dict(status='DOWNLOADED_EVIDENCE_REVIEW_PASS',job='582199',source=str(job),validation_sha256=sha(job/'validation.json'),
    checkpoint_sha256=sha(job/'checkpoint.json'),binaries=checkpoint['binaries'],local_published_source_pins_checked=source_count,
    selected_case_files_verified=selected_count,tool_files_verified=sum(p.is_file() for p in (job/'tools').rglob('*')),
    archived_package_python_files_verified=len(list(job.glob('*.py'))),
    pairs=pairs,unit_stages=units,audit_extrema=extrema,cases=rows,files_sha256=hashes,
    disk=dict(cluster_recorded_bytes=v['retained_unique_file_bytes'],downloaded_current_bytes=actual_bytes,storage=v['export_storage_counts']),
    scope='Verified downloaded hashes, original input/binary receipts, matching archived tools, recorded independent numerical audit reports and CSV/log timing/counter closure. No local mesh-audit recomputation, solver/MPI/build or scheduler runs. Four ranks, one shared-node repetition; no general speedup/scaling or composed-field-gradation certificate.')
finite(result)
print('PASS',selected_count,'case files;',source_count,'source pins;16 cases;8 identical pairs; bytes',actual_bytes)
for r in profile_rows:print(r['case'],round(r['private_wall_max_mean'],3),round(r['bulk_split_private_fraction']*100,1),'%',r['private_queries'])
if len(sys.argv)>2:
    output=Path(sys.argv[2]);assert not output.exists();output.write_text(json.dumps(result,indent=2)+'\n')
