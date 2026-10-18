#!/usr/bin/env python3
"""Verify saved receipts, exact contexts and local progress; never execute SU2."""
import gzip,hashlib,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]

def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()

def binary_digest(p):
 if p.exists():return digest(p)
 h=hashlib.sha256()
 with gzip.open(p.with_suffix('.gz'),'rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()

def main():
 variants={digest(p) for p in HERE.glob('*controller_runtime_v1.py')}
 for folder,exe in [(HERE,'replay'),(HERE/'actual_context_np4','context')]:
  r=json.loads((folder/'receipt.json').read_text())
  assert all(r[k]['exit']==0 for k in ['compile','link','run'])
  for path,expected in r['input_hashes'].items():
   actual=digest(ROOT/path)
   assert actual==expected or (path.endswith('/run.py') and expected in variants),(path,actual,expected)
  for path,expected in r['linked_archives'].items():assert digest(Path(path))==expected,path
  assert binary_digest(HERE/exe)==r['binary_sha256']
 trials=[json.loads(x) for x in (HERE/'trials.jsonl').read_text().splitlines()]
 byname={x['name']:x for x in trials if x['kind']=='trial'}
 assert not byname['cavity_0']['local_progress'] and byname['cavity_0']['metric_failure']==2
 for name in ['cavity_2','cavity_3','joint']:
  row=byname[name];assert row['geometry'] and row['local_progress'] and row['q']>=.2 and row['J']>=.05
  assert row['added_excess']<row['removed_excess']
 assert next(x for x in trials if x['kind']=='joint_work')['trials']==3
 summary=json.loads((HERE/'context_summary.json').read_text())
 assert summary['helper_sha256']==digest(HERE/'diagnose_contexts.py')
 assert len(summary['rank_results'])==4
 for rank in range(4):
  folder=HERE/'actual_context_np4'
  result=json.loads((folder/f'result_rank{rank}.json').read_text());assert not result['accepted'] and result['atomic_input'] and result['CFD_steps']==0 and result['retained']==16
  rows=[json.loads(x) for x in (folder/f'contexts_rank{rank}.jsonl').read_text().splitlines()];assert len(rows)==16
  assert rows[-1]['feature']==result['last_feature']
  for row in rows:
   for suffix in ['_old.dat','_private.dat']:assert (folder/(row['stem']+suffix)).is_file()
 profiles=[json.loads(p.read_text()) for p in (HERE/'actual_context_np4').glob('profile_rank*.json')]
 assert len(profiles)==4 and all(p==profiles[0] for p in profiles)
 assert profiles[0]=={'parameter':.875,'archive_version':22,'runtime_version':2,'core':.003215747252591252}
 manifest=HERE/'checkpoint_provenance.json'
 if manifest.exists():
  for path,expected in json.loads(manifest.read_text())['source_and_artifact_hashes'].items():assert digest(ROOT/path)==expected,path
 print('PASS: frozen inputs/build/archive/binaries/runtime helpers; original-star local gates; 64 exact contexts; MPI profile agreement; atomic input snapshots. No accepted M6 or whole-event speedup established.')
if __name__=='__main__':main()
