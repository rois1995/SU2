#!/usr/bin/env python3
"""Verify sealed current qualification and scoped historical construction without simulations."""
import gzip,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def binary(folder):
 p=folder/'reassess';h=hashlib.sha256()
 with (p.open('rb') if p.exists() else gzip.open(folder/'reassess.gz','rb')) as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def native(folder,current=False):
 r=json.loads((folder/'receipt.json').read_text());assert r['compile']['exit']==r['link']['exit']==r['run']['exit']==0
 assert sha(folder/'run.log')==r['run']['log_sha256'] and binary(folder)==r['binary_sha256']
 capsule=json.loads(gzip.decompress((folder/'executed_source.json.gz').read_bytes()))
 for name,expected in r['hashes'].items():
  if name in capsule:assert hashlib.sha256(capsule[name].encode()).hexdigest()==expected,name
  else:assert sha(ROOT/name)==expected,name
 if current:
  for name,text in capsule.items():assert (ROOT/name).read_text()==text,name
  for name,expected in r['linked_archives'].items():assert sha(Path(name))==expected,name
 return capsule

def main():
 seal=json.loads((HERE/'seal.json').read_text())
 for name,expected in seal['artifacts'].items():assert sha(HERE/name)==expected,name
 for case in [HERE,HERE/'budget_6144_v1',HERE/'geodesic_v1',HERE/'release_v1']:native(case)
 native(HERE/'release_regional_v1',True)
 construction=json.loads(gzip.decompress((HERE/'geodesic_v1/executed_source.json.gz').read_bytes()))
 assert construction['Common/src/adaptation/CNativeRegion3D.cpp']==(ROOT/'Common/src/adaptation/CNativeRegion3D.cpp').read_text()
 for case in ['geodesic_v1','release_regional_v1']:
  report=json.loads((HERE/case/'independent.json').read_text());assert report['status'].startswith('PASS_NATIVE_ACCEPTED_')
  for name,expected in report['identity'].items():
   actual=sha(ROOT/name)
   if actual!=expected and name.endswith('/audit_sphere.py'):actual=sha(HERE/'audit_sphere_construct_executed.py')
   assert actual==expected,name
 for mode in ['serial','mpi4']:
  folder=HERE/'qualification_v2'/mode;r=json.loads((folder/'result.json').read_text());p=json.loads((folder/'provenance.json').read_text());assert r['exit_code']==0
  assert sha(folder/'provenance.json')==r['provenance_sha256'] and sha(folder/'run.log')==r['log_sha256']
  for name,expected in p['pins'].items():assert sha(ROOT/name)==expected,name
 independent=json.loads((HERE/'primitive_independent_v2.json').read_text());assert len(independent['cases'])==15
 for name,expected in independent['input_hashes'].items():assert sha(HERE/name)==expected,name
 assert sha(HERE/'audit_primitive_lifecycles_v2.py')==independent['helper_sha256']
 summary=json.loads((HERE/'summary.json').read_text());assert summary['total_native_work']==6087 and summary['total_native_work']<=summary['shared_native_work_limit']
 assert summary['total_native_seconds']<=summary['shared_native_seconds_limit'] and summary['construction']['cells']==5440 and summary['release']['cells']==320
 assert json.loads((HERE/'geodesic_v1/source.json').read_text())==json.loads((HERE/'release_regional_v1/source.json').read_text())
 print('PASS: sealed executed construction/current release, original source/authority, exact geometry/height/shape and release sensor field, current serial/MPI4 primitive N/M lifecycles, shared native budget. No MPI BL/viscous/affordability claim.')
if __name__=='__main__':main()
