#!/usr/bin/env python3
"""Verify unchanged M6 profile estimates and bounded reused-background qualification."""
import hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];hashes={}
def sha(p):
 if p not in hashes:
  h=hashlib.sha256()
  with p.open('rb') as f:
   for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
  hashes[p]=h.hexdigest()
 return hashes[p]
def load(p):return json.loads(p.read_text())
def receipt(folder,conflict=False):
 proof=folder/'provenance.json';identity=load(proof);r=load(folder/'result.json')
 assert (0<r['exit_code']<124) if conflict else r['exit_code']==0
 assert r['provenance_sha256']==sha(proof) and r['log_sha256']==sha(folder/'run.log')
 for name,digest in identity['pins'].items():assert sha(ROOT/name)==digest,name
 return r
controls={}
for mode in ('primitives','uncertainty_mpi4','enabled_mpi4','disabled_runtime'):
 folder=HERE/'qualification_v2'/mode;controls[mode]=receipt(folder)
 if mode in ('enabled_mpi4','disabled_runtime'):
  name='independent_enabled.json' if mode=='enabled_mpi4' else 'independent_runtime.json';audits=load(folder/name)
  assert len(audits)==2 and all(a['SU2_and_CGNS_reload'] and a['SA_restart_fields'] for a in audits)
  controls[mode]['independent_audit_sha256']=sha(folder/name)
  controls[mode]['audits']=audits
baseline=HERE.parent/'budget_bl_policy_v1'
def trials(folder):
 return [dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in (folder/'run.log').read_text().splitlines()
         if line.startswith('Native3D BL budget trial:')]
def stats(folder):
 rows=[dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in (folder/'run.log').read_text().splitlines()
       if line.startswith('Native3D support correction:')]
 assert len(rows)==1
 return rows[0]
comparisons=[]
for budget,ranks,expected in ((300000,4,'selected'),(300000,2,'selected'),(100000,4,'conflict')):
 folder=HERE/('m6_growth2_budget%d_np%d'%(budget,ranks));r=receipt(folder,expected=='conflict')
 old=baseline/('m6_growth2_budget%d_np%d%s'%(budget,4,'_v5' if budget==300000 else ''))
 old_result=load(old/'result.json');assert old_result['log_sha256']==sha(old/'run.log')
 for name in ('Coarse.su2','Coarse.su2.native_ref','donor_flow.dat'):assert sha(folder/name)==sha(old/name),name
 def physical_config(path):return [line for line in path.read_text().splitlines() if not line.startswith('ADAP_NATIVE_RANKS=')]
 assert physical_config(folder/'run.cfg')==physical_config(old/'run.cfg')
 new_rows,old_rows=trials(folder),trials(old);assert len(new_rows)==len(old_rows)
 drift=0
 for a,b in zip(new_rows,old_rows):
  for key in ('parameter','profile_version','coarsest_estimate','uncertainty_indicator','mandatory_estimate','mandatory_indicator'):
   x,y=float(a[key]),float(b[key]);drift=max(drift,abs(x-y)/max(1,abs(y)));assert abs(x-y)<=3e-12*max(1,abs(y)),key
 comparison={'budget':budget,'ranks':ranks,'outcome':expected,'trial_rows':len(new_rows),'maximum_relative_drift':drift,
             'launcher_seconds':r['elapsed_seconds'],'provenance_sha256':r['provenance_sha256'],'baseline_log_sha256':sha(old/'run.log')}
 if expected=='selected':
  saved=load(folder/'saved_metric_budget.json');assert saved['frozen'] and saved['CFD_steps']==0 and not saved['mesh_construction_attempted']
  assert saved['flow_and_turbulence_unchanged'] and saved['reference_unchanged'];comparison['selected']=saved
  row=stats(folder);previous=stats(old);assert int(row['background_rank_reuses'])>0
  comparison['statistics']=row;comparison['baseline_statistics']=previous
  comparison['background_determinant_reduction']=1-int(row['background_determinants'])/int(previous['background_determinants'])
 comparisons.append(comparison)
result={'controls_pass':True,'full_goal_complete':False,'same_M6_profile_estimates':True,'comparisons':comparisons,
 'controls':controls,'report_helper_sha256':sha(Path(__file__)),
 'limits':['No new accepted M6 construction','No calibrated global absolute-error allocator','No complete-event speedup or converged physical accuracy claim']}
(HERE/'summary.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='controls'},indent=2))
