#!/usr/bin/env python3
"""Verify complete scoped receipts and summarize qualified policy evidence, without running SU2."""
import hashlib,json,math,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
cache={}
def digest(path):
 if path not in cache:
  h=hashlib.sha256()
  with path.open('rb') as stream:
   for chunk in iter(lambda:stream.read(1024*1024),b''):h.update(chunk)
  cache[path]=h.hexdigest()
 return cache[path]
def load(path):return json.loads(path.read_text())
def receipt(folder,current=True):
 proof=folder/'provenance.json';identity=load(proof);result=load(folder/'result.json')
 assert result['provenance_sha256']==digest(proof) and result['log_sha256']==digest(folder/'run.log')
 assert (0<result['exit_code']<124) if identity['expected_failure'] else result['exit_code']==0
 if current:
  for name,sha in identity['pins'].items():assert digest(ROOT/name)==sha,name
 return result

def trials(folder):
 rows=[]
 for line in (folder/'run.log').read_text().splitlines():
  if line.startswith('Native3D BL budget trial:'):
   row={k:float(v) for k,v in re.findall(r'(\w+)=([-+\d.eE]+)',line)}
   assert row['proven_bound']==0 and row['support_changed']==0;rows.append(row)
 assert rows and len(rows)<=9
 return rows

def main():
 result={'scope':'Opt-in native3D budget policy; full Goal3 remains incomplete','proven_error_bound':False,
  'report_helper_sha256':digest(Path(__file__)),'testcases':{},'M6':{}}
 for mode in ('primitives','disabled_serial','uncertainty_serial','uncertainty_mpi4','enabled_serial','enabled_mpi4'):
  folder=HERE/'qualification_v7'/mode;result['testcases'][mode]=receipt(folder)
 enabled=[]
 for mode in ('enabled_serial','enabled_mpi4'):
  rows=load(HERE/'qualification_v7'/mode/'independent_enabled.json');assert len(rows)==2
  for row in rows:
   assert row['accepted'] and row['frozen'] and row['SU2_and_CGNS_reload'] and row['SA_restart_fields']
   assert row['independent_helper_sha256']==digest(HERE/'audit_enabled.py')
   assert row['first_height_error']<1e-8 and row['transported_sensor_residual']<2e-12
   assert row['indicator_covers_observed_error']
  enabled.append({row['budget']:row for row in rows})
 for budget in (3,20):
  for key in ('parameter','version','trials','final_sensor_eigenvalue','independent_reference'):
   a,b=enabled[0][budget][key],enabled[1][budget][key]
   assert math.isclose(a,b,rel_tol=1e-11,abs_tol=1e-12),(budget,key,a,b)
 result['accepted_workflows']=enabled
 result['profile_clocks']={}
 for mode in ('enabled_serial','enabled_mpi4'):
  clocks=[]
  for line in (HERE/'qualification_v7'/mode/'run.log').read_text().splitlines():
   if line.startswith(('NATIVE3D_STAGE ','NATIVE3D_DETAIL ','NATIVE3D ')):
    row=dict(re.findall(r'(\w+)=([^ ]+)',line));row['record']=line.split()[0];clocks.append(row)
  assert clocks;result['profile_clocks'][mode]=clocks
 result['resource_scope']='Low-priority contended correctness pilots. Phase clocks can overlap; process_peak_rss and time -v RSS are process values, not an aggregate MPI peak.'
 for name in ('m6_growth2_budget300000_np4_v5','m6_growth2_budget100000_np2_v5','m6_growth2_budget1000_np4_v5'):
  folder=HERE/name;entry={'receipt':receipt(folder),'trials':trials(folder),'construction_attempted':False}
  if (folder/'saved_metric_budget.json').exists():
   saved=load(folder/'saved_metric_budget.json');assert saved['CFD_steps']==0 and saved['frozen']
   assert saved['normalization_status']==1 and saved['parameter']==.75
   entry['selected']=saved
  else:
   log=(folder/'run.log').read_text();assert 'Estimated BL budget conflict' in log
   entry['mandatory_exceeds_budget']='protected_minimum_exceeds_budget=1' in log
  result['M6'][name]=entry
 assert result['M6']['m6_growth2_budget1000_np4_v5']['mandatory_exceeds_budget']
 assert not result['M6']['m6_growth2_budget100000_np2_v5']['mandatory_exceeds_budget']
 previous=HERE/'m6_growth2_budget100000_np4';receipt(previous,False)
 first=trials(previous);second=result['M6']['m6_growth2_budget100000_np2_v5']['trials'];assert len(first)==len(second)==9
 differences=[]
 for a,b in zip(first,second):
  assert a['parameter']==b['parameter'] and a['profile_version']==b['profile_version']
  for key in ('coarsest_estimate','uncertainty_indicator','mandatory_estimate','mandatory_indicator'):
   delta=abs(a[key]-b[key])/max(1.,abs(a[key]));assert delta<1e-7,(key,delta)
   differences.append(delta)
 result['M6_rank2_vs_rank4_maximum_relative_difference']=max(differences)
 old=load(HERE/'m6_growth2_budget300000_np4/saved_metric_budget.json')
 new=result['M6']['m6_growth2_budget300000_np4_v5']['selected']
 probe_path=HERE.parent/'budget_bl_cost_v1/normal_profile_probe.json';probe=load(probe_path)
 assert probe['helper_sha256']==digest(probe_path.with_suffix('.py'))
 holdouts=[];background=2.0884618106155486
 for row in probe['results']:
  diagnosis=probe_path.parent/row['seed_label']/'np4/diagnosis.json'
  assert row['source_diagnosis_sha256']==digest(diagnosis)
  for name,sha in load(diagnosis)['rows_sha256'].items():assert digest(diagnosis.parent/name)==sha
  experiment=next(e for e in row['experiments'] if e['protected_distance']==2e-6 and e['maximum_outer_growth']==2)
  estimate=next(t for t in experiment['trials'] if t['parameter']==new['parameter'])
  holdouts.append({'seed':row['seed_label'],'composed_estimate':estimate['value']+background,
   'heuristic_3SE':estimate['heuristic_3SE'],'primary_estimate_difference':estimate['value']+background-new['estimate']})
 assert len(holdouts)==2
 result['M6_selected_coarsest_holdouts']={'estimates':holdouts,'probe_sha256':digest(probe_path),
  'scope':'Reused independent saved clouds and spectral density reconstruction at the same constant HMAX100 sensor endpoint. Native geometric samples reused; no rigorous continuum reference.',
  'warning':'Both holdout lower heuristic endpoints exceed300000. This is a provisional integration-limited selection, not demonstrated budget feasibility.'}
 result['M6_uncertainty_stage_comparison']={'previous_metric_seconds':old['seconds'],'current_metric_seconds':new['seconds'],
  'same_profile':old['parameter']==new['parameter'],'changed_provisional_scale':True,
  'scope':'Bounded original-coarsest-field stopping replaces unresolved finer-scale trials; not same-field speedup or lifecycle scaling.'}
 result['limitations']=['Heuristic uncalibrated uncertainty; no absolute-global-error allocator yet',
  'Profile scan resolution 1/8; no continuous optimum or infeasibility proof',
  'No accepted M6 construction; no complete curved/interacting policy workflow',
  'No converged viscous accuracy or complete-event affordability claim',
  'Automatic policy currently native3D only; strict2D remains unchanged']
 (HERE/'summary.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
 print('Verified complete scoped policy evidence; full Goal3 remains active.')
if __name__=='__main__':main()
