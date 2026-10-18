#!/usr/bin/env python3
"""Verify reuse evidence and distinguish useful private progress from accepted construction."""
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
def receipt(folder,current=True):
 proof=folder/'provenance.json';identity=load(proof);result=load(folder/'result.json')
 assert result['exit_code']==0 and result['provenance_sha256']==sha(proof) and result['log_sha256']==sha(folder/'run.log')
 if current:
  for name,digest in identity.get('pins',identity).items():assert sha(ROOT/name)==digest,name
 if 'construction_sha256' in result:assert result['construction_sha256']==sha(folder/'construction_result.json')
 if 'resource_sha256' in result:assert result['resource_sha256']==sha(folder/'resources.txt')
 return result
checks={}
for mode in ('serial','mpi4','runtime','disabled_runtime'):
 folder=HERE/mode;checks[mode]=receipt(folder)
 if mode in ('runtime','disabled_runtime'):
  filename='independent_enabled.json' if mode=='runtime' else 'independent_runtime.json'
  audits=load(folder/filename);assert len(audits)==2
  assert all(a['SU2_and_CGNS_reload'] and a['SA_restart_fields'] and a['restart_flow_BDF_defect' if mode=='runtime' else 'restart_flow_defect']<1e-10 for a in audits)
  checks[mode]['independent_audit_sha256']=sha(folder/filename)
  checks[mode]['independent_audits']=audits
hard=HERE/'hard_replay';latest=receipt(hard)
assert latest['construction']['CFD_steps']==0 and not latest['construction']['normalization_accuracy_established']
baseline=HERE.parent/'construction_source_point_v1';previous=receipt(baseline,False)
for name in ('run.cfg','Coarse.su2','Coarse.su2.native_ref','donor_flow.dat','sensor_original.bin'):
 assert sha(hard/name)==sha(baseline/name),name
def phases(folder):
 rows=[]
 for line in (folder/'run.log').read_text().splitlines():
  if line.startswith('NATIVE3D_DETAIL stage=refine '):
   row=dict(re.findall(r'(\w+)=([^\s]+)',line));rows.append(row)
 assert len(rows)==4 and {r['rank'] for r in rows}=={'0','1','2','3'}
 return rows
old,new=phases(baseline),phases(hard)
comparison={'baseline_seconds':previous['construction']['seconds'],'candidate_seconds':latest['construction']['seconds'],
 'baseline_private_edits':sum(int(r['accepted']) for r in old),'candidate_private_edits':sum(int(r['accepted']) for r in new),
 'candidate_hint_hits':sum(int(r['donor_hint_hits']) for r in new),
 'candidate_private_edge_campaign_hits':sum(int(r['private_edge_campaign_hits']) for r in new),
 'baseline_phases':old,'candidate_phases':new,
 'interpretation':'Same original frozen field/config/constraints. Local contended observations; no accepted-mesh affordability or statistical speedup claim.'}
assert comparison['candidate_hint_hits']>0
summary={'controls_pass':True,'full_goal_complete':False,'construction_accepted':latest['construction']['accepted'],
 'controls':checks,'construction':latest,'comparison':comparison,'report_helper_sha256':sha(Path(__file__)),
 'scope':'Bounded exact physical-donor hypotheses and existing strict edge certificates. No broader support/reservation batching, physical accuracy or normalization claim.'}
(HERE/'summary.json').write_text(json.dumps(summary,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k not in ('controls','construction','comparison')},indent=2))
print(json.dumps({k:v for k,v in comparison.items() if not k.endswith('_phases')},indent=2))
