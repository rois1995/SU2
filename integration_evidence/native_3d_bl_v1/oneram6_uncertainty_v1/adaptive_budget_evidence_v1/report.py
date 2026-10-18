#!/usr/bin/env python3
"""Qualify bounded evidence stages from saved receipts; no native execution."""
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
def receipt(folder,conflict=False):
 p=folder/'provenance.json';identity=load(p);r=load(folder/'result.json')
 assert (0<r['exit_code']<124) if conflict else r['exit_code']==0
 assert r['provenance_sha256']==sha(p) and r['log_sha256']==sha(folder/'run.log')
 for name,digest in identity['pins'].items():assert sha(ROOT/name)==digest,name
 capsule=folder/'executed_source.json.gz'
 for name,source in json.loads(gzip.decompress(capsule.read_bytes())).items():
  assert source==(ROOT/name).read_text(),name
 r['capsule_sha256']=sha(capsule)
 return r

def rows(folder,prefix):
 return [dict(re.findall(r'(\w+)=([^\s]+)',s)) for s in (folder/'run.log').read_text().splitlines() if s.startswith(prefix)]
def trials(folder):return rows(folder,'Native3D BL budget trial:')
def drift(a,b,keys,tolerance):
 assert len(a)==len(b)
 maximum=0
 for x,y in zip(a,b):
  for k in keys:
   u,v=float(x[k]),float(y[k]);d=abs(u-v)/max(1,abs(v));maximum=max(maximum,d);assert d<=tolerance,(k,u,v,d)
 return maximum
controls={}
for mode in ('primitives','uncertainty_mpi4','enabled_mpi4','disabled_runtime'):
 f=HERE/'qualification_v2'/mode;r=receipt(f);entry={'receipt':r}
 if mode in ('enabled_mpi4','disabled_runtime'):
  name='independent_enabled.json' if mode=='enabled_mpi4' else 'independent_runtime.json';audits=load(f/name)
  assert len(audits)==2 and all(a['SU2_and_CGNS_reload'] and a['SA_restart_fields'] for a in audits)
  entry.update(audits=audits,audit_sha256=sha(f/name))
 controls[mode]=entry
serial=load(HERE/'qualification_v2/primitives/independent_calibration.json')
parallel=load(HERE/'qualification_v2/uncertainty_mpi4/independent_calibration.json')
assert {k:v for k,v in serial.items() if k!='input_sha256'}=={k:v for k,v in parallel.items() if k!='input_sha256'}
assert serial['all_observed_indicators_cover']
assert serial['helper_sha256']==sha(HERE/'calibrate.py')
for mode in ('primitives','uncertainty_mpi4'):
 assert (serial if mode=='primitives' else parallel)['input_sha256']==sha(HERE/'qualification_v2'/mode/'adaptive_complexity_control.json')
# MPI JSON includes its rank count; compare the independently audited trial values.
scalar_s=load(HERE/'qualification_v2/primitives/adaptive_complexity_control.json')
scalar_p=load(HERE/'qualification_v2/uncertainty_mpi4/adaptive_complexity_control.json')
assert scalar_s['trials']==scalar_p['trials']
comparisons=[];selected=[]
keys=('parameter','coarsest_estimate','uncertainty_indicator','mandatory_estimate','mandatory_indicator')
for budget,ranks in ((300000,4),(300000,2),(100000,4)):
 f=HERE/('m6_growth2_budget%d_np%d'%(budget,ranks));r=receipt(f,budget==100000);t=trials(f)
 old=HERE.parent/'background_trial_reuse_v1'/('m6_growth2_budget%d_np%d'%(budget,ranks if budget==300000 else 4))
 previous=load(old/'result.json');assert previous['log_sha256']==sha(old/'run.log')
 for name in ('Coarse.su2','Coarse.su2.native_ref','donor_flow.dat'):
  assert ROOT in (f/name).resolve().parents and sha(f/name)==sha(old/name)
 before=trials(old);initial=drift(t[:len(before)],before,keys,3e-12)
 entry={'budget':budget,'ranks':ranks,'receipt':r,'initial_trial_relative_drift':initial,'trial_rows':len(t)}
 if budget==100000:
  assert len(t)==9 and not rows(f,'Native3D BL evidence stage:')
  assert 'Estimated BL budget conflict' in (f/'run.log').read_text()
  assert float(t[-1]['coarsest_estimate'])-float(t[-1]['uncertainty_indicator'])>budget
  entry.update(status='estimated_sampled_profile_conflict',last_trial=t[-1],extensions=0)
 else:
  s=load(f/'saved_metric_budget.json');assert s['frozen'] and s['CFD_steps']==0 and not s['mesh_construction_attempted']
  assert s['flow_and_turbulence_unchanged'] and s['reference_unchanged'] and not s['support_changed']
  assert s['evidence_stages']==2 and s['trials']==len(t)==22 and s['parameter']==.875 and s['version']==22
  stages=rows(f,'Native3D BL evidence stage:');assert len(stages)==2 and [int(x['samples']) for x in stages]==[16384,32768]
  stat=rows(f,'Native3D support correction:');assert len(stat)==1;stat=stat[0]
  assert int(stat['samples'])==32768 and int(stat['donor_index_rank_builds'])==ranks
  assert int(stat['refinement_rank_stages'])==2*ranks and int(stat['retained_bytes_max'])<=2<<30
  assert int(stat['support_pdf_candidates'])+int(stat['support_build_work'])<=80000000
  last=t[-2:];assert [float(x['parameter']) for x in last]==[.75,.875]
  assert float(last[0]['coarsest_estimate'])-float(last[0]['uncertainty_indicator'])>budget
  assert float(last[1]['coarsest_estimate'])+float(last[1]['uncertainty_indicator'])<budget
  stage_rows=rows(f,'NATIVE3D_STAGE stage=metric ')
  entry.update(status='estimated_profile_attainable_normalization_integration_limited',selected=s,statistics=stat,evidence_stages=stages,
   max_single_process_peak_rss=max(int(x['process_peak_rss']) for x in stage_rows),last_profile_rows=last,
   normalization_relative_budget_discrepancy=s['estimate']/budget-1,normalization_relative_indicator=s['uncertainty']/budget)
  selected.append((f,t,s))
 comparisons.append(entry)
mpi_trial_drift=drift(selected[0][1],selected[1][1],keys,3e-12)
mpi_final_drift=drift([selected[0][2]],[selected[1][2]],('estimate','uncertainty'),1e-10)
# Independent saved seeds are noisy holdouts, not exact references for M6.
p=HERE.parent/'budget_bl_cost_v1/normal_profile_probe.json';holdout=load(p);holdout_rows=[]
background=2.08846181061555
for seed in holdout['results']:
 e=next(x for x in seed['experiments'] if x['protected_distance']==2e-6 and x['maximum_outer_growth']==2)
 for v in e['trials']:
  if v['parameter'] not in (.75,.875):continue
  mean=v['value']+background;indicator=v['heuristic_3SE'];status='incompatible' if mean-indicator>300000 else 'attainable' if mean+indicator<300000 else 'uncertain'
  assert status==('incompatible' if v['parameter']==.75 else 'attainable')
  holdout_rows.append(dict(seed=seed['seed_label'],parameter=v['parameter'],mean=mean,indicator=indicator,status=status))
result={'qualified':True,'full_goal_complete':False,'controls':controls,'scalar_calibration':serial,
 'comparisons':comparisons,'MPI_trial_maximum_relative_drift':mpi_trial_drift,'MPI_final_maximum_relative_drift':mpi_final_drift,
 'holdout_sha256':sha(p),'holdouts':holdout_rows,'holdout_background':background,
 'report_helper_sha256':sha(Path(__file__)),
 'limits':['No accepted M6 construction or complete-event speedup','Heuristic uncertainty; no general tail or sequential-stopping calibration',
 'Global component-level evidence allocation only, no per-cell allocator','Opt-in selection currently native3D; strict2D retained',
 'M6 final normalization remains provisional; composed-field gradation is audited, not certified',
 'No claim that every holdout mean lies inside the new narrower heuristic indicator',
 'Retained memory is higher; single-process RSS peaks are not an aggregate job peak']}
(HERE/'summary.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('controls','comparisons','scalar_calibration')},indent=2))
