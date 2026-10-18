#!/usr/bin/env python3
"""Verify scoped point-construction evidence; never promote a rejected M6 mesh to success."""
import gzip,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];cache={}
def sha(p):
 if p not in cache:
  h=hashlib.sha256()
  with p.open('rb') as f:
   for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
  cache[p]=h.hexdigest()
 return cache[p]
def load(p):return json.loads(p.read_text())
checks={}
for mode in ('serial','mpi4','runtime','disabled_runtime'):
 folder=HERE/mode if mode=='disabled_runtime' else HERE/'qualification_v1'/mode
 proof=folder/'provenance.json';identity=load(proof);result=load(folder/'result.json')
 assert result['exit_code']==0 and result['provenance_sha256']==sha(proof) and result['log_sha256']==sha(folder/'run.log')
 for name,digest in identity['pins'].items():assert sha(ROOT/name)==digest,name
 if mode in ('serial','mpi4'):
  bounds=load(folder/'independent_affine_bounds.json');assert bounds['passed']
  assert bounds['helper_sha256']==sha(HERE/'audit_bounds.py')
  assert len(bounds['files'])==(1 if mode=='serial' else 4)
  for row in bounds['files']:assert row['sha256']==sha(folder/row['file']) and row['rows']==42
  assert len({r['sha256'] for r in bounds['files']})==1
 elif mode=='disabled_runtime':
  audits=load(folder/'independent_runtime.json');assert len(audits)==2
  assert all(a['SU2_and_CGNS_reload'] and a['SA_restart_fields'] and a['restart_flow_defect']<1e-10 for a in audits)
 else:
  audits=load(folder/'independent_enabled.json');assert len(audits)==2
  assert all(a['accepted'] and a['frozen'] and a['SU2_and_CGNS_reload'] and a['SA_restart_fields'] for a in audits)
 checks[mode]=result
hard=HERE.parent/'construction_source_point_v1';receipt=load(hard/'result.json');proof=hard/'provenance.json'
assert receipt['exit_code']==0 and receipt['provenance_sha256']==sha(proof) and receipt['log_sha256']==sha(hard/'run.log')
for name,digest in load(proof).items():assert sha(ROOT/name)==digest,name
assert receipt['construction_sha256']==sha(hard/'construction_result.json')
construction=receipt['construction'];assert construction['CFD_steps']==0 and not construction['normalization_accuracy_established']
result={'controls_pass':True,'hard_construction_accepted':construction['accepted'],'full_goal_complete':False,
 'controls':checks,'construction':receipt,'report_helper_sha256':sha(Path(__file__)),
 'hard_phase_extract_sha256':sha(HERE/'hard_phase_extract.json'),
 'hard_worker_stack_sha256':sha(HERE/'hard_worker3_stack.txt'),
 'scope':'Source-contained bounded affine point realization. Ordinary point/edge queries, geometry, height, q/J/full metric and transfer gates unchanged. No exact M6 normalization or physical CFD accuracy claim.'}
(HERE/'summary.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('controls','construction')},indent=2))
