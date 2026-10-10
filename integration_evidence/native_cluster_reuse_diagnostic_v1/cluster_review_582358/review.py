"""Review saved strict matrix582358 evidence; no solver, build or numerical mesh-audit rerun."""
import copy
import csv
import datetime
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import subprocess
import sys

repo = Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).resolve().parents[3]).resolve()
root = repo / 'ClusterResults/reuse_matrix_582358'
revision = 'd44cc986c2880ac3bf7e975342efc257f55cd8a9'
expected_binary = '5827ffbe8b7ac9dd0336d000a33a8d649975579f1cd57f623a440a00fb87019e'
verified = {}
historical = []

def read(path):
    return json.loads(path.read_text(), parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))

def check_bytes(data, expected):
    actual = hashlib.sha256(data).hexdigest()
    assert actual == expected, 'Hash mismatch'
    return actual

def checked(path, expected):
    path = path.resolve()
    name = str(path.relative_to(repo))
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest()!=expected and name.startswith(
            ('Common/','SU2_CFD/','UnitTests/','integration_evidence/','meson.build')):
        data = subprocess.check_output(['git','show',revision+':'+name],cwd=repo)
        historical.append(name)
    verified[name] = check_bytes(data,expected)
    return path

def module(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

v = read(root/'validation.json')
cp = read(root/'checkpoint.json')
assert v['status']=='PASS' and len(v['cases'])==16 and len(v['pairs'])==8
checked(root/'checkpoint.json',v['checkpoint_sha256'])
assert cp['binaries']['profile']['sha256']==expected_binary
for name,digest in cp['files_sha256'].items():
    checked(repo/name,digest)
for path in (root/'tools').rglob('*'):
    if path.is_file():
        checked(path,cp['files_sha256'][str(path.relative_to(root/'tools'))])
package = 'integration_evidence/native_cluster_reuse_diagnostic_v1/'
for name in ('matrix_prepare.py','matrix_run.py','prepare.py','run.py','analyze.py','check_package.py'):
    checked(root/name,cp['files_sha256'][package+name])
# Imported comparison helpers must match the receipt, even if later source is available.
for name in ('integration_evidence/native_cluster_reconstruction_reuse_v1/analyze.py',
             'integration_evidence/native_cluster_balance_profile_v1/analyze.py'):
    check_bytes((repo/name).read_bytes(),cp['files_sha256'][name])
strict = module('matrix_review_strict',repo/'integration_evidence/native_cluster_reconstruction_reuse_v1/analyze.py')
diag = module('matrix_review_diagnostic',root/'analyze.py')
account = module('matrix_review_accounting',root/'tools/integration_evidence/audit_native_profile_accounting.py')
modes = {(kind,workers,partition,role) for kind in ('frozen_euler_to_bl','frozen_bl_to_euler')
         for workers,partition in ((4,'NO'),(4,'YES'),(3,'YES'),(2,'YES'))
         for role in ('control','profile')}
assert {(r['kind'],r['workers'],r['repartition'],r['role']) for r in v['cases']}==modes
assert all(r['repeat']==1 for r in v['cases'])
last_end = None
def chronology(evidence):
    global last_end
    start = datetime.datetime.fromisoformat(evidence['utc_started'])
    end = datetime.datetime.fromisoformat(evidence['utc_finished'])
    assert start.tzinfo and end.tzinfo and start<end
    assert last_end is None or last_end<=start
    last_end = end

units = []
assert [u['ranks'] for u in v['unit_stages']]==[1,2,4]
for unit in v['unit_stages']:
    assert unit['status']=='PASS' and unit['exit_code']==0
    text = checked(root/('unit_n'+str(unit['ranks']))/'solver.log',unit['log_sha256']).read_text()
    summaries = re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',text)
    assert len(summaries)==unit['ranks'] and all(int(n)==63 for _,n in summaries)
    chronology(unit)
    units.append(dict(ranks=unit['ranks'],summaries=summaries))

cases = []
for row in v['cases']:
    case = root/'cases'/row['case']
    manifest = read(case/'collection_manifest.json')
    assert manifest['status']=='VERIFIED_COMPACT_EXPORT'
    assert {name:info['sha256'] for name,info in manifest['files'].items()}==row['output_sha256']
    for name,info in manifest['files'].items():
        assert Path(name).name==name
        checked(case/name,info['sha256'])
        assert (case/name).stat().st_size==info['bytes']
    evidence = read(case/'run_evidence.json')
    assert evidence['binary_sha256']==expected_binary and evidence['exit_code']==0 and row['status']=='PASS'
    assert evidence['command'][2]=='4' and evidence['command'][-3]=='[NativeFrozenAirfoil2D]'
    assert evidence['command'][-4].endswith('/'+cp['binaries']['profile']['path'])
    mode = 'OFF' if row['role']=='control' else 'BOTH'
    assert evidence['diagnostic_environment']==dict(SU2_NATIVE_REUSE=mode,SU2_NATIVE_REUSE_AUDIT='NO')
    for name,digest in evidence['inputs_sha256'].items():
        checked(case/name,digest)
    text = (case/'solver.log').read_text()
    assert text.count('All tests passed')==4
    audit = read(case/'independent_frozen_metric_audit.json')
    assert audit==row['audit'] and audit['status']=='PASS'
    checked(root/'tools/integration_evidence/audit_native_frozen_case.py',audit['checker_sha256'])
    for name,digest in audit['input_sha256'].items():
        path = root/'tools'/name.split('/tools/',1)[1] if '/tools/' in name else case/Path(name).name
        checked(path,digest)
    assert all(check[0]=='PASS' for check in audit['validity'].values())
    assert not audit['bad_quality_cells'] and not audit['bad_length_edges']
    assert audit['min_quality']>=.18-1e-8 and audit['max_simpson_length']<=1.8+1e-8
    if row['kind']=='frozen_euler_to_bl':
        assert audit['max_relative_first_height_error']<=1e-8
    else:
        assert audit['max_relative_first_height_error'] is None
    assert audit['transported_metric']['status']=='PASS'
    assert audit['transported_metric']['max_relative_directional_tensor_defect']<=1e-7
    profile = strict.profile(case,row['workers'])
    assert profile==row['balance']
    timing = account.inspect(text)
    assert timing==row['timing_scopes']==read(case/'independent_profile_accounting.json')
    assert len(timing['worker_profiles'])==len(timing['accounting_closure'])==1
    commits = list(map(int,re.search(r'^Native operations .*?:([^;]+);',text,re.M)[1].split()))
    assert commits==[sum(int(rank['operations'][i]['committed']) for rank in profile) for i in range(8)]
    times = [list(csv.DictReader((case/('native_frozen_timing_rank_'+str(i)+'.csv')).open())) for i in range(4)]
    assert all(len(t)==1 and int(t[0]['accepted'])==1 for t in times)
    assert math.isclose(max(float(t[0]['remesh_max_seconds']) for t in times),row['remesh_seconds'],rel_tol=1e-14)
    diagnostic = diag.details(case,dict(row,role=mode))
    assert diagnostic==row['diagnostic']
    chronology(evidence)
    costs = {key:sum(op[key] for rank in profile for op in rank['operations'])
             for key in ('requests','evaluations','evictions','private_wall_seconds','private_cpu_seconds')}
    imbalance = max(rank['private_wall_seconds'] for rank in profile)/(costs['private_wall_seconds']/row['workers'])
    assert math.isclose(imbalance,row['balance_summary']['private_wall_max_mean'],rel_tol=1e-14)
    cases.append(dict(case=row['case'],kind=row['kind'],mode=mode,workers=row['workers'],repartition=row['repartition'],
                      remesh_seconds=row['remesh_seconds'],costs=costs,private_max_mean=imbalance,
                      peak_rss_mib=max(int(t[0]['rss_hwm_kib']) for t in times)/1024,
                      phase_scopes=timing,diagnostic=diagnostic,
                      utc_started=evidence['utc_started'],utc_finished=evidence['utc_finished'],
                      other_visible_compute_max=max((s['other_compute_processes'] for s in evidence['machine_samples']),default=None),
                      audit_extrema={key:audit[key] for key in ('points','triangles','min_quality','max_simpson_length','max_relative_first_height_error')},
                      transported_metric_defect=audit['transported_metric']['max_relative_directional_tensor_defect'],
                      mesh_sha256=row['output_sha256']['native_frozen_adapted.su2']))

def validate_pairs(rows):
    pairs = strict.compare(rows)
    assert len(pairs)==8 and all(p['numerical_outputs_identical'] and p['operation_counts_identical'] for p in pairs)
    by_name = {r['case']:r for r in rows}
    for pair in pairs:
        left,right = (root/'cases'/pair[key] for key in ('control','profile'))
        assert read(left/'run_evidence.json')['inputs_sha256']==read(right/'run_evidence.json')['inputs_sha256']
    return pairs

pairs = validate_pairs(v['cases'])
assert pairs==v['pairs']
# Small negative checks exercise identity, count and hash rejection without changing saved files.
for change in ('hash','count'):
    bad = copy.deepcopy(v['cases'])
    row = next(r for r in bad if r['role']=='profile')
    if change=='hash':
        row['output_sha256']['native_frozen_adapted.su2']='0'*64
    else:
        row['balance'][0]['operations'][0]['selected']+=1
    try:
        validate_pairs(bad)
    except AssertionError:
        pass
    else:
        raise AssertionError('Review accepted changed '+change)
try:
    check_bytes(b'corrupted','0'*64)
except AssertionError:
    pass
else:
    raise AssertionError('Review accepted corrupt bytes')

launcher = repo/'ClusterResults/jobs/582358_reuse_matrix_launcher'
assert int((launcher/'exit_code.txt').read_text())==0
checked(launcher/'RunReuseMatrixSGE.sh',cp['files_sha256'][package+'RunReuseMatrixSGE.sh'])
for path in (root/'validation.json',launcher/'launcher.out',launcher/'exit_code.txt'):
    verified[str(path.relative_to(repo))]=hashlib.sha256(path.read_bytes()).hexdigest()
result = dict(status='REVIEW_COMPLETE_STRICT_MATRIX_PASS',job=582358,source_checkpoint=revision,
              binary_sha256=expected_binary,units=units,cases=cases,pairs=pairs,files_sha256=verified,
              historical_source_files=historical,selfchecks='Changed output hash, changed operation count and corrupt bytes all rejected.',
              retained_storage={k:v[k] for k in ('retained_logical_bytes','retained_unique_file_bytes','export_storage_counts')},
              scope='Saved data/report/source/tool hashes, numerical-report extrema, bounded profiles, mode/binary receipts and chronology verified. No local solver/build/MPI or heavy numerical audit rerun. No CFD/time-history/combined-gradation/3D validation. One ordered repetition under shared-node contention; no general speedup/scaling certificate. Binary receipt is not independent compilation provenance.')
if len(sys.argv)>2:
    out = Path(sys.argv[2])
    with out.open('x') as stream:
        stream.write(json.dumps(result,indent=2)+'\n')
print(result['status'],len(verified),'unique files verified;',len(cases),'cases;',len(pairs),'strict pairs; negative selfchecks PASS')
