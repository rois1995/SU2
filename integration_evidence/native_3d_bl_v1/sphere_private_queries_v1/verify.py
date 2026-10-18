#!/usr/bin/env python3
"""Verify this partial checkpoint's scientific bytes, provenance and retained refusals; no CFD."""
import importlib.util,json,os,runpy
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OLD=HERE.with_name('sphere_rans_lifecycle_v1')
spec=importlib.util.spec_from_file_location('lifecycle_provenance',str(OLD/'verify.py'));proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)
sha=proof.sha

def main():
 seal=json.loads((HERE/'seal.json').read_text())
 for name,wanted in seal['files'].items():assert sha(ROOT/name)==wanted,name
 for name,target in seal['links'].items():
  p=ROOT/name;assert p.is_symlink() and os.readlink(str(p))==target and p.exists(),name
  assert os.path.commonpath([str(ROOT),str(p.resolve())])==str(ROOT),name
 verified={}
 for batch in ['controls','controls_v2','controls_v3']:
  for mode in ['serial','mpi2','mpi4','rans_mpi4']:
   folder=HERE/batch/mode;verified[batch+'/'+mode]=proof.run(folder,batch=='controls_v3',0)
 for case in ['n4_m2_query_v2','n4_m2_query_v3','n4_m2_query_v4']:
  verified[case]=proof.run(OLD/case/'update',case=='n4_m2_query_v4',1)
 endpoint=HERE/'endpoint_filter';generated=[]
 for name in ['before','after']:
  receipt=json.loads((endpoint/(name+'_receipt.json')).read_text());assert receipt['exit']==0
  assert sha(endpoint/(name+'.json'))==receipt['stdout_sha256'] and sha(endpoint/(name+'.log'))==receipt['stderr_sha256']
  hashes=receipt['hashes'];source_name='Common/src/adaptation/CNativePredicates3D.cpp'
  assert sha(endpoint/'before_predicate.cpp' if name=='before' else ROOT/source_name)==hashes[source_name]
  assert sha(endpoint/'probe.cpp')==hashes[str((endpoint/'probe.cpp').relative_to(ROOT))]
  witness=OLD/'donor_witness_v1/witness.dat';assert sha(witness)==hashes[str(witness.relative_to(ROOT))]
  binary=endpoint/name
  if binary.exists():assert sha(binary)==hashes[str(binary.relative_to(ROOT))]
  generated.append({'file':name,'historical_binary_sha256':hashes[str(binary.relative_to(ROOT))],'local_binary_checked':binary.exists()})
 runpy.run_path(str(HERE/'report.py'),run_name='__main__')
 result={'status':'VERIFIED_PARTIAL_QUERY_ADMISSION_CHECKPOINT','goal':'ACTIVE_INCOMPLETE','verified_runs':verified,'generated_probe_binaries':generated,'scope':'Scientific records and executed capsules sealed; current build/binary pins verified. Historical binaries are identity records; small local probes checked when present. No accepted large repeated-BL workflow or CFD/scaling/affordability claim.'}
 (HERE/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
if __name__=='__main__':main()
