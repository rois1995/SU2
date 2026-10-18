#!/usr/bin/env python3
"""Verify current scoped evidence without running CFD or native reconstruction."""
import gzip,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def main():
 for mode in ('serial','mpi4','enabled_mpi4','disabled_runtime'):
  f=HERE/'qualification_v2'/mode;r=json.loads((f/'result.json').read_text());p=json.loads((f/'provenance.json').read_text())
  assert r['exit_code']==0 and r['provenance_sha256']==sha(f/'provenance.json') and r['log_sha256']==sha(f/'run.log')
  for name,value in p['pins'].items():assert sha(ROOT/name)==value,name
  capsule=json.loads(gzip.decompress((f/'executed_source.json.gz').read_bytes()))
  for name,text in capsule.items():assert (ROOT/name).read_text()==text,name
 build=json.loads((HERE/'probe_build.json').read_text());assert build['compile']['exit']==build['link']['exit']==0
 for name,value in build['hashes'].items():assert sha(ROOT/name)==value,name
 for name,value in build['linked_archives'].items():assert sha(Path(name))==value,name
 executable=HERE/'probe';h=hashlib.sha256()
 with (executable.open('rb') if executable.exists() else gzip.open(HERE/'probe.gz','rb')) as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 assert h.hexdigest()==build['binary_sha256']
 witnesses=[]
 for n in (1,2,4):
  f=HERE/f'probe_np{n}';r=json.loads((f/'result.json').read_text())
  assert r['exit']==0 and r['build_sha256']==sha(HERE/'probe_build.json') and r['log_sha256']==sha(f/'run.log')
  rows=[json.loads(p.read_text()) for p in sorted(f.glob('result_rank*.json'))]
  assert rows==r['ranks'] and len(rows)==n
  for row in rows:
   assert row['immutable_input'] and row['stop']=='failure'
   witness=row['reason'].split(' Last local rejection:')[0]
   assert witness.startswith('Native3D exact-facet resolution conflict on crease edge 211,3481:')
   witnesses.append(witness)
 assert len(set(witnesses))==1
 print('PASS: current source/build/binary, four scoped qualifications, exact admitted-feature MPI1/2/4 witness and immutable input. No accepted whole-M6 or performance claim.')
if __name__=='__main__':main()
