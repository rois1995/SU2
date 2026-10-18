#!/usr/bin/env python3
"""Verify connected concurrency controls, complete workflows and the exact frozen M6 outcome."""
import gzip,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];hashes={}
def sha(p):
 p=p.resolve()
 if p not in hashes:
  h=hashlib.sha256()
  with p.open('rb') as f:
   for b in iter(lambda:f.read(1048576),b''):h.update(b)
  hashes[p]=h.hexdigest()
 return hashes[p]
def load(p):return json.loads(p.read_text())
def receipt(f):
 r=load(f/'result.json');d=load(f/'provenance.json');assert r['exit_code']==0
 assert r['provenance_sha256']==sha(f/'provenance.json') and r['log_sha256']==sha(f/'run.log')
 for p,h in d['pins'].items():assert sha(ROOT/p)==h,p
 for p,text in json.loads(gzip.decompress((f/'executed_source.json.gz').read_bytes())).items():assert (ROOT/p).read_text()==text,p
 return r,d
def rows(f,prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',s)) for s in (f/'run.log').read_text().splitlines() if s.startswith(prefix)]
controls={}
for mode in ('serial','mpi4','enabled_mpi4','disabled_runtime'):
 f=HERE/'qualification_v3'/mode;r,d=receipt(f);entry={'receipt':r}
 if mode in ('enabled_mpi4','disabled_runtime'):
  name='independent_enabled.json' if mode=='enabled_mpi4' else 'independent_runtime.json';audits=load(f/name)
  assert len(audits)==2 and all(a['SU2_and_CGNS_reload'] and a['SA_restart_fields'] for a in audits)
  entry.update(audits=audits,independent_audit_sha256=sha(f/name))
 else:
  epoch=[x for x in rows(f,'ENGINE ') if x.get('case')=='concurrent_composed_first_epoch']
  count=1 if mode=='serial' else 4;assert len(epoch)==count and sum(int(x['accepted']) for x in epoch)==count
  if count>1:assert all(int(x['maximum_parallel_commits'])==count and int(x['final_reservations'])==1 for x in epoch)
  entry['first_epoch']=epoch
 controls[mode]=entry
f=HERE/'frozen_m6_np4';r,d=receipt(f);manifest=d['selected_profile']
assert sha(f/'selected_sensor.bin')==manifest['selected_sensor_sha256']
assert sha(f/'Coarse.su2')==manifest['source_mesh_sha256'] and sha(f/'run.cfg')==manifest['config_sha256']
profiles=[load(f/('frozen_profile_rank%d.json'%i)) for i in range(4)]
assert all(p==profiles[0] for p in profiles) and profiles[0]['frozen']
assert profiles[0]['parameter']==manifest['profile_parameter'] and profiles[0]['original_version']==manifest['profile_version']
assert profiles[0]['runtime_version']>0 and abs(profiles[0]['core']-manifest['core'])<=1e-10*manifest['core']
assert not r['metric_initialization_repeated'] and r['CFD_steps']==0 and not d['original_family_correction_available']
log=(f/'run.log').read_text();assert 'Metric computed in ' not in log and 'Native3D BL budget trial:' not in log
parent=rows(f,'NATIVE3D rank=');engine=rows(f,'NATIVE3D_DETAIL stage=refine ')
assert len(parent)==len(engine)==4 and len(re.findall(r'Passed .*test case',log))==4
assert all(int(x['initial'])==303359 for x in parent)
assert all(int(x['native_bound'])<=2<<30 for x in parent)
assert all(int(x['global_work'])<=int(x['work_limit']) for x in engine)
assert max(int(x['concurrent_rounds']) for x in engine)>0
counts={k:sum(int(x[k]) for x in engine) for k in ('accepted','rejected','proposal_deferred','layer_published','remaining_edges','cached_invalid_cells')}
maximum_parallel=max(int(x['maximum_parallel_commits']) for x in engine)
baseline=load(HERE.parent/'selected_profile_construction_v1/summary.json')
comparison={'same_frozen_source':True,'same_config':True,'same_physical_profile':True,'same_native_event_ceiling':300,
 'baseline_private_edits':baseline['global_engine_counts']['accepted'],'new_private_edits':counts['accepted'],
 'baseline_kernel_rounds':int(baseline['engine_rank_diagnostics'][0]['rounds']),'new_kernel_rounds':int(engine[0]['rounds']),
 'baseline_kernel_tracked_calls':int(baseline['engine_rank_diagnostics'][0]['tracked_calls']),'new_kernel_tracked_calls':int(engine[0]['tracked_calls']),
 'baseline_event_seconds':baseline['construction']['remesh_seconds'],'new_event_seconds':r['construction']['seconds'],
 'causal_speedup_established':False,'qualification':'Different adaptation order, contended local observation, frozen replay omits retained raw-family/correction infrastructure.'}
result={'scoped_validation_pass':True,'full_goal_complete':False,'controls':controls,'M6_receipt':r,
 'frozen_profiles':profiles,'M6_parent_rank_diagnostics':parent,'M6_engine_rank_diagnostics':engine,'global_engine_counts':counts,
 'maximum_parallel_commits':maximum_parallel,'comparison':comparison,'report_helper_sha256':sha(Path(__file__)),
 'limits':['Faceted composed REFINE concurrency only; projected curved/repair/coarsen ordering retained',
 'No full-goal or clean scaling/affordability claim','Frozen M6 replay cannot correct a budget using clipped donors',
 'A native geometry result is not an actual full RANS transfer/restart result; qualify that separately if reached']}
witness=load(HERE/'original_edge_witness/witness.json');metric=load(HERE/'original_edge_witness/metric_diagnosis.json')
assert witness['log_sha256']==sha(f/'run.log') and witness['mesh_sha256']==sha(f/'Coarse.su2')
assert witness['helper_sha256']==sha(HERE/'trace_original_edge.py')
assert witness['witness_mesh_sha256']==sha(HERE/'original_edge_witness/original_star.su2')
assert metric['witness_sha256']==sha(HERE/'original_edge_witness/witness.json') and metric['sensor_sha256']==sha(f/'selected_sensor.bin')
assert metric['helper_sha256']==sha(HERE/'original_edge_metric.py')
assert not witness['late_cavity_equivalence_established'] and not witness['native_reconstruction_attempted']
result['original_edge_witness']=witness;result['original_edge_metric_diagnosis']=metric
(HERE/'summary.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('controls','M6_engine_rank_diagnostics','M6_parent_rank_diagnostics','M6_receipt')},indent=2))
