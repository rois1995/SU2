#!/usr/bin/env python3
"""Apply the existing independent strict-policy oracle to this current N4/M2 receipt."""
import hashlib,importlib.util,json,os,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(2**20),b''):h.update(block)
 return h.hexdigest()
folder=HERE/'controls_v3/rans_mpi4';record=json.loads((folder/'result.json').read_text())
assert record['exit_code']==0 and record['log_sha256']==sha(folder/'run.log')
assert record['provenance_sha256']==sha(folder/'provenance.json')
proof=json.loads((folder/'provenance.json').read_text())
for name,wanted in proof['pins'].items():assert sha(ROOT/name)==wanted,name
adapter=HERE/'strict_independent_adapter';adapter.mkdir(exist_ok=True)
link=adapter/'mpi4_m2'
if not link.is_symlink():link.symlink_to(os.path.relpath(folder,adapter),target_is_directory=True)
assert link.resolve()==folder.resolve()
receipt={'status':'PASS','contract':{'selected_workers':2},'artifacts':{p.name:sha(p) for p in folder.iterdir() if p.is_file()},'producer_result_sha256':sha(folder/'result.json'),'adapter_sha256':sha(Path(__file__))}
(adapter/'mpi4_m2.json').write_text(json.dumps(receipt,indent=2)+'\n')
helper=HERE.with_name('production_policy_v1')/'audit.py'
spec=importlib.util.spec_from_file_location('strict_policy_oracle',str(helper));audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
audit.HERE=adapter;sys.argv=[str(helper),'4','2'];audit.main()
report=json.loads((adapter/'independent_mpi4_m2.json').read_text());assert report['status']=='PASS_LIMITED_SCOPE' and len(report['frames'])==2
print('PASS_CURRENT_STRICT_POLICY_N4_M2',flush=True)
