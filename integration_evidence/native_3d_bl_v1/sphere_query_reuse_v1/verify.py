#!/usr/bin/env python3
"""Verify current source/build/binary and sealed scientific records without rerunning CFD."""
import importlib.util,json,os,runpy
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.with_name('sphere_rans_lifecycle_v1')
spec=importlib.util.spec_from_file_location('lifecycle_proof',BASE/'verify.py');proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)
def main():
 seal=json.loads((HERE/'seal.json').read_text())
 for name,wanted in seal['files'].items():assert proof.sha(ROOT/name)==wanted,name
 for name,target in seal['links'].items():
  p=ROOT/name;assert p.is_symlink() and os.readlink(str(p))==target and p.exists(),name
  assert os.path.commonpath([str(ROOT),str(p.resolve())])==str(ROOT),name
 for name,wanted in json.loads((HERE/'build_source.json').read_text()).items():assert proof.sha(ROOT/name)==wanted,name
 results={mode:proof.run(HERE/'controls'/mode,True,0) for mode in ['serial','mpi2','mpi4','rans_mpi4']}
 results['actual_update']=proof.run(BASE/'n4_m2_reuse_v1/update',True,1)
 results['previous_update']=proof.run(BASE/'n4_m2_query_v4/update',False,1)
 runpy.run_path(str(HERE/'report.py'),run_name='__main__')
 out={'status':'VERIFIED_PARTIAL_QUERY_REUSE_CHECKPOINT','goal':'ACTIVE_INCOMPLETE','verified_runs':results,
  'scope':'Current controls and atomically rejected actual N4/M2 repeat; historical comparison binary retained as identity record. Cold accepted producer is reused, not recomputed. No accepted large-BL update or affordability claim.'}
 (HERE/'verification.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'],flush=True)
if __name__=='__main__':main()
