#!/usr/bin/env python3
"""Verify sealed hypothesis evidence without CFD, native work or repeated full-mesh screens."""
import gzip,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def main():
 seal=json.loads((HERE/'seal.json').read_text())
 for name,value in seal['artifacts'].items():assert sha(HERE/name)==value,name
 for folder in (HERE,HERE/'wall_reference_v1'):
  r=json.loads((folder/'receipt.json').read_text());assert r['compile']['exit']==r['link']['exit']==r['run']['exit']==0
  assert sha(folder/'run.log')==r['run']['log_sha256']
  for name,value in r['hashes'].items():assert sha(ROOT/name)==value,name
  for name,value in r['linked_archives'].items():assert sha(Path(name))==value,name
  for name,value in r['stage_hashes'].items():assert sha(folder/name)==value,name
  binary=folder/'reassess';h=hashlib.sha256()
  with (binary.open('rb') if binary.exists() else gzip.open(folder/'reassess.gz','rb')) as f:
   for b in iter(lambda:f.read(2**20),b''):h.update(b)
  assert h.hexdigest()==r['binary_sha256']
 for row in json.loads((HERE/'inputs.json').read_text())['pieces']:
  assert sha(ROOT/row['compressed_source'])==row['compressed_sha256']
 summary=json.loads((HERE/'assessment_comparison.json').read_text())
 assert summary['domain_screen']['finer_nonpositive']==0 and summary['domain_screen']['all_open_faces_physically_bound']
 assert all(x['inside_sum_of_heuristic_indicators'] for x in summary['stages'].values())
 assert 0<summary['indicator_ratio_fine_over_coarse']<1 and summary['reassessment_time_ratio']>1
 print('PASS: exact executed sources/builds/binaries/inputs, MPI4 stages, independent same-field proposal, sealed domain screens. Partial variance evidence only; no accepted M6/accuracy-bound/speedup claim.')
if __name__=='__main__':main()
