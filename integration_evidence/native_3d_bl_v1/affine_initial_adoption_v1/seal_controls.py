#!/usr/bin/env python3
"""Seal affected serial/MPI controls and cross-rank original-field consistency; no full-mesh claim."""
import functools,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
@functools.lru_cache(maxsize=None)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def main():
 receipt=json.loads((HERE/'build_provenance.json').read_text());binary=ROOT/'integration_evidence/build-integrated-v2/UnitTests/test_driver'
 assert receipt['status']=='PASS' and sha(binary)==receipt['binary_sha256']
 assert all(sha(ROOT/p)==v for p,v in receipt['source'].items())
 cases=[];signatures={}
 for ranks in (1,2,4):
  folder=HERE/('controls_v2_n%d'%ranks);result=json.loads((folder/'result.json').read_text());proof=json.loads((folder/'provenance.json').read_text());audit=json.loads((folder/'independent_controls.json').read_text())
  assert result['exit_code']==0 and sha(folder/'provenance.json')==result['provenance_sha256'] and sha(folder/'run.log')==result['log_sha256']
  assert all(sha(ROOT/p)==v for p,v in proof['pins'].items())
  assert all(sha(folder/p)==v for p,v in result['artifacts'].items())
  assert audit['status']=='INDEPENDENT_AFFINE_INITIAL_CONTROLS_PASS' and audit['auditor_sha256']==sha(HERE/'audit_controls.py')
  assert all(sha(ROOT/p)==v for p,v in audit['helpers'].items())
  summary=re.findall(r'Passed all (\d+) test cases with (\d+) assertions\.',(folder/'run.log').read_text());assert len(summary)==ranks and all(int(n)==23 for n,_ in summary)
  for name,case in audit['cases'].items():
   signature=case['source_signature_sha256'];assert name not in signatures or signatures[name]==signature;signatures[name]=signature
  cases.append(dict(ranks=ranks,test_cases_per_rank=23,assertions_per_reported_rank=[int(a) for _,a in summary],elapsed_seconds=result['elapsed_seconds'],independent_initial_source_geometry_collision='PASS',
   hashes={p.name:sha(p) for p in (folder/'result.json',folder/'provenance.json',folder/'independent_controls.json',folder/'executed_source.json.gz')}))
 resources=[json.loads(line) for line in (HERE/'build_resources_v2.jsonl').read_text().splitlines()]
 output=dict(status='AFFECTED_CONTROLS_ONLY_PASS',cases=cases,original_source_signatures_identical_across_MPI=signatures,binary_sha256=receipt['binary_sha256'],build=dict(created_utc=receipt['created_utc'],finished_utc=receipt['finished_utc'],
  maximum_sampled_owned_RSS_sum_KiB=max(r['sampled_rss_sum_KiB'] for r in resources),minimum_sampled_available_KiB=min(int(r['available'].split()[0]) for r in resources),memory_scope='Sampled owned RSS sum; shared pages may be counted repeatedly, not PSS or guaranteed peak'),
  hashes={p.name:sha(p) for p in (Path(__file__).resolve(),HERE/'build_provenance.json',HERE/'build_resources_v2.jsonl',HERE/'audit_controls.py',HERE/'annulus_v1_n1/exact_feature_obstruction.json')},
  limits=['Initial adoption/original P1/codec and affected first-row/height/reconstruction/transaction controls; no accepted oblique mesh',
   'Native assertions cover state/coordinate atomicity and worker-subset engine paths; not a new independently audited complete N/M transfer/restart lifecycle',
   'Exact literal-body obstruction prevents an unchanged positive oblique target; geometry/source redesign remains required',
   'Contended correctness durations are not scalability, speedup or CFD affordability evidence'])
 (HERE/'controls_summary_v2.json').write_text(json.dumps(output,indent=2)+'\n');print(output['status'],'serial/MPI2/MPI4, source traces bitwise identical')
if __name__=='__main__':main()
