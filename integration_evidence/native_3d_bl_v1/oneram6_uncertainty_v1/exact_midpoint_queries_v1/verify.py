"""Verify pinned connected checks and preserve the two substantive construction failures."""
import hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
def sha(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  for part in iter(lambda:f.read(1048576),b''):h.update(part)
 return h.hexdigest()
def main():
 results={}
 for mode in ('pilot_serial','mpi2','mpi4','runtime','coupled'):
  d=HERE/mode;proof=d/'provenance.json';p=json.loads(proof.read_text());r=json.loads((d/'result.json').read_text())
  for name,digest in p['pins'].items():assert sha(ROOT/name)==digest,name
  assert sha(proof)==r['provenance_sha256'] and sha(d/'run.log')==r['log_sha256']
  assert r['exit_code']==(1 if mode=='coupled' else 0)
  results[mode]={'exit_code':r['exit_code'],'seconds':r['elapsed_seconds'],'provenance_sha256':sha(proof)}
 runtime=json.loads((HERE/'runtime/independent_runtime.json').read_text())
 assert len(runtime)==2 and all(x['one_final_transfer'] and x['SU2_and_CGNS_reload'] and x['transport_error']==0 and x['first_height_error']==0 for x in runtime)
 m6=HERE.parent/'construction_progressive_midpoint_v1';r=json.loads((m6/'result.json').read_text())
 assert not r['construction']['accepted'] and r['CFD_steps']==0 and not r['solution_transfer_performed']
 for name,digest in json.loads((m6/'provenance.json').read_text()).items():assert sha(ROOT/name)==digest,name
 assert sha(m6/'run.log')==r['log_sha256'] and sha(m6/'construction_result.json')==r['construction_sha256']
 assert 'Uncovered native 3D source segment endpoint.' in (m6/'run.log').read_text()
 assert 'joint strip exhausted the global work budget' in (HERE/'coupled/run.log').read_text()
 results.update(M6={'accepted':False,'backend_seconds':r['construction']['seconds'],'reason':'Uncovered native 3D source segment endpoint.','receipt_sha256':sha(m6/'result.json')},runtime=runtime,
  status='SCOPED_CHECKS_PASS_WITH_PRESERVED_CONSTRUCTION_FAILURES',proven_error_bound=False,full_goal_complete=False,helper_sha256=sha(Path(__file__)))
 (HERE/'summary.json').write_text(json.dumps(results,indent=2)+'\n');print(results['status'])
if __name__=='__main__':main()
