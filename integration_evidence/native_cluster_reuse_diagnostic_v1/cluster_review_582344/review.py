"""Review saved job582344 evidence only; no solver, build, MPI or heavy mesh audit."""
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
root = repo / 'ClusterResults/reuse_diagnostic_582344'
verified = {};historical = []
revision = '0bb829910ad646496e9e87ab9a5983487e69efc7'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def checked(p, expected):
    p = p.resolve();name = str(p.relative_to(repo))
    actual = sha(p)
    if actual!=expected and name.startswith(('Common/','SU2_CFD/','UnitTests/','integration_evidence/','meson.build')):
        data = subprocess.check_output(['git','show',revision+':'+name],cwd=repo)
        actual = hashlib.sha256(data).hexdigest();historical.append(name)
    assert actual==expected, 'Hash mismatch: '+name
    verified[name] = actual
    return p

def read(p): return json.loads(p.read_text(),parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))
def module(name,p):
    spec = importlib.util.spec_from_file_location(name,p);m = importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

v = read(root/'validation.json');cp = read(root/'checkpoint.json')
assert v['status']=='DIAGNOSTIC_COMPLETE' and len(v['cases'])==7 and len(v['unit_stages'])==3
checked(root/'checkpoint.json',v['checkpoint_sha256'])
for name,digest in cp['files_sha256'].items(): checked(repo/name,digest)
for p in (root/'tools').rglob('*'):
    if p.is_file(): checked(p,cp['files_sha256'][str(p.relative_to(root/'tools'))])
expected = dict(control='1a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f',
                archived='13751201942dfcf9731bc377e45a69b0793ce82f0e2abe0b72edfbcee0902626',
                profile='5827ffbe8b7ac9dd0336d000a33a8d649975579f1cd57f623a440a00fb87019e')
assert {k:r['sha256'] for k,r in cp['binaries'].items()}==expected
units = []
for u in v['unit_stages']:
    assert u['status']=='PASS' and u['exit_code']==0
    text = checked(root/('unit_n'+str(u['ranks']))/'solver.log',u['log_sha256']).read_text()
    summaries = re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',text)
    assert len(summaries)==u['ranks'] and all(int(c)==63 for a,c in summaries)
    units.append(dict(ranks=u['ranks'],summaries=summaries))
analysis = module('saved_diagnostic_analysis',repo/'integration_evidence/native_cluster_reuse_diagnostic_v1/analyze.py')
account = module('saved_diagnostic_accounting',root/'tools/integration_evidence/audit_native_profile_accounting.py')
keys = ('native_frozen_adapted.su2',)+tuple('native_frozen_target_rank_'+str(i)+'.csv' for i in range(4))
cases = [];last_end = None
for row in v['cases']:
    case = root/'cases'/row['case'];manifest = read(case/'collection_manifest.json')
    assert manifest['status']=='VERIFIED_COMPACT_EXPORT'
    assert {k:i['sha256'] for k,i in manifest['files'].items()}==row['output_sha256']
    for name,info in manifest['files'].items():
        assert Path(name).name==name
        checked(case/name,info['sha256']);assert (case/name).stat().st_size==info['bytes']
    evidence = read(case/'run_evidence.json');role = row['role'] if row['role'] in ('control','archived') else 'profile'
    assert evidence['binary_sha256']==expected[role] and evidence['exit_code']==0 and row['status']=='PASS'
    assert evidence['command'][-4].endswith('/'+cp['binaries'][role]['path'])
    assert evidence['command'][2]=='4' and evidence['command'][-3]=='[NativeFrozenAirfoil2D]'
    for name,digest in evidence['inputs_sha256'].items(): checked(case/name,digest)
    text = (case/'solver.log').read_text();assert text.count('All tests passed')==4
    audit = read(case/'independent_frozen_metric_audit.json');assert audit==row['audit'] and audit['status']=='PASS'
    checked(root/'tools/integration_evidence/audit_native_frozen_case.py',audit['checker_sha256'])
    for name,digest in audit['input_sha256'].items():
        path = root/'tools'/name.split('/tools/',1)[1] if '/tools/' in name else case/Path(name).name
        checked(path,digest)
    assert all(value[0]=='PASS' for value in audit['validity'].values())
    assert not audit['bad_quality_cells'] and not audit['bad_length_edges']
    assert audit['min_quality']>=.18-1e-6 and audit['max_simpson_length']<=1.8+1e-6
    assert audit['max_relative_first_height_error']<=1e-8 and audit['transported_metric']['status']=='PASS'
    assert audit['transported_metric']['max_relative_directional_tensor_defect']<=1e-7
    profile = analysis.profile(case,4);assert profile==row['balance']
    timing = account.inspect(text);assert timing==row['timing_scopes']==read(case/'independent_profile_accounting.json')
    commits = list(map(int,re.search(r'^Native operations .*?:([^;]+);',text,re.M)[1].split()))
    assert commits==[sum(int(r['operations'][i]['committed']) for r in profile) for i in range(8)]
    times = [list(csv.DictReader((case/('native_frozen_timing_rank_'+str(i)+'.csv')).open())) for i in range(4)]
    assert all(len(t)==1 and int(t[0]['accepted'])==1 for t in times)
    assert max(float(t[0]['remesh_max_seconds']) for t in times)==row['remesh_seconds']
    assert analysis.details(case,row)==row['diagnostic']
    start = datetime.datetime.fromisoformat(evidence['utc_started']);end = datetime.datetime.fromisoformat(evidence['utc_finished'])
    assert start<end and (last_end is None or last_end<=start);last_end=end
    costs = {k:sum(op[k] for r in profile for op in r['operations'])
             for k in ('requests','evaluations','evictions','private_wall_seconds','private_cpu_seconds')}
    cases.append(dict(role=row['role'],case=row['case'],remesh_seconds=row['remesh_seconds'],costs=costs,
                      private_max_mean=row['balance_summary']['private_wall_max_mean'],diagnostic=row['diagnostic'],
                      mesh_sha256=row['output_sha256'][keys[0]],utc_started=evidence['utc_started'],utc_finished=evidence['utc_finished'],
                      other_visible_compute_max=max((s['other_compute_processes'] for s in evidence['machine_samples']),default=None),
                      audit_extrema={k:audit[k] for k in ('points','triangles','min_quality','max_simpson_length','max_relative_first_height_error')},
                      transported_metric_defect=audit['transported_metric']['max_relative_directional_tensor_defect']))
pairs = analysis.compare(v['cases']);assert pairs==v['pairs'] and all(p['operation_counts_identical'] for p in pairs)
assert not pairs[0]['numerical_outputs_identical'] and all(p['numerical_outputs_identical'] for p in pairs[1:])
old,new = [root/'cases'/row['case'] for row in (v['cases'][0],v['cases'][2])]
a,b = [(p/'native_frozen_adapted.su2').read_text().splitlines() for p in (old,new)]
i = next(i for i,line in enumerate(a) if line.startswith('NPOIN'));n = int(a[i].split('=')[1])
assert a[:i+1]==b[:i+1] and a[i+1+n:]==b[i+1+n:]
points = []
for x,y in zip(a[i+1:i+1+n],b[i+1:i+1+n]):
    assert x.split()[2]==y.split()[2]
    xx,yy = [list(map(float,z.split()[:2])) for z in (x,y)]
    if xx!=yy:points.append(dict(point=int(x.split()[2]),control=xx,rebuilt=yy,displacement=math.dist(xx,yy)))
tensors = [];unchanged = [];csv_different = 0
for rank in range(4):
    aa,bb = [list(csv.DictReader((p/keys[rank+1]).open())) for p in (old,new)]
    assert len(aa)==len(bb)
    for x,y in zip(aa,bb):
        assert x['point']==y['point'];csv_different+=x!=y
        if any(x[k]!=y[k] for k in ('xx','xy','yy')):
            scale = max(abs(float(x[k])) for k in ('xx','xy','yy'))
            delta = max(abs(float(x[k])-float(y[k])) for k in ('xx','xy','yy'))
            tensors.append(dict(point=int(x['point']),max_relative_entry_change=delta/scale))
            if x['x']==y['x'] and x['y']==y['y']:unchanged.append(int(x['point']))
launcher = repo/'ClusterResults/jobs/582344_reuse_diagnostic_launcher'
assert int((launcher/'exit_code.txt').read_text())==0
checked(launcher/'RunReuseDiagnosticSGE.sh',cp['files_sha256']['integration_evidence/native_cluster_reuse_diagnostic_v1/RunReuseDiagnosticSGE.sh'])
result = dict(status='REVIEW_COMPLETE_SAME_EXECUTABLE_IDENTITY_PASS_HISTORICAL_IDENTITY_FAIL',job=582344,
              source_revision=revision,units=units,cases=cases,pairs=pairs,files_sha256=verified,historical_source_files=historical,
              historical_difference=dict(noncoordinate_records_identical=True,points_changed=points,tensors_changed=tensors,
              unchanged_coordinate_tensor_changes=unchanged,csv_rows_changed=csv_different,
              maximum_displacement=max(p['displacement'] for p in points),maximum_relative_tensor_entry_change=max(t['max_relative_entry_change'] for t in tensors)),
              scope='Saved reports/data/source hashes and bounded profiles checked; no local build/SU2/MPI/heavy numerical audit. Receipt hashes are not independent compilation provenance. No exhaustive hit or compiler-causality proof.')
if len(sys.argv)>2:
    out = Path(sys.argv[2]);assert not out.exists();out.write_text(json.dumps(result,indent=2)+'\n')
print(result['status'],len(verified),'unique files verified;',len(points),'changed historical coordinate rows;',len(tensors),'tensor rows')
