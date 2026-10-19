#!/usr/bin/env python3
"""Seal scoped geometry evidence; no recompilation, native rerun or broadened qualification."""
import gzip,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  while b:=f.read(4*1024**2):h.update(b)
 return h.hexdigest()
def main():
 reports={name:json.loads((HERE/path).read_text()) for name,path in [('core_v1','core_v1_n1/result.json'),('core_v2','core_v2_n1/result.json'),('oracle','oracle_v1/result.json'),('compatibility','coordinate_compatibility_v1_n1/result.json')]}
 assert reports['core_v1']['status']==reports['core_v2']['status']=='PASS'
 assert reports['oracle']['status']=='EXACT_RATIONAL_PARAMETRIC_ORACLE_PASS' and reports['compatibility']['status']=='PASS_UNCHANGED_COORDINATE_COMPATIBILITY'
 for version in ('v1','v2'):
  d=reports['core_'+version];folder=HERE/('core_'+version+'_n1');capsule=json.loads(gzip.decompress((folder/'source.json.gz').read_bytes()))
  assert sha(folder/'geometry_driver')==d['binary_sha256']
  for p,text in capsule.items():assert hashlib.sha256(text.encode()).hexdigest()==d['source_sha256'][p]
  for step in d['steps']:assert step['exit_code']==0 and sha(folder/(step['name']+'.log'))==step['log_sha256']
  for p,h in d['support_archive_sha256'].items():assert sha(Path(p))==h,('Support archive changed',p)
 assert sha(HERE/'core_v1_n1/executed_focused.py')==reports['core_v1']['source_sha256'][str((HERE/'focused.py').relative_to(ROOT))]
 assert sha(HERE/'focused.py')==reports['core_v2']['source_sha256'][str((HERE/'focused.py').relative_to(ROOT))]
 compiled=json.loads(gzip.decompress((HERE/'core_v2_n1/source.json.gz').read_bytes()))
 old='/*! Explicit numerical mapping contract on immutable finite triangles. Every original edge is\n * protected in this first representation. No near-coplanar merging, inferred body identity,'
 new='/*! Explicit numerical mapping contract on immutable finite triangles.\n * shortcut: original edges remain protected; extend after feature/accuracy classification.\n * No near-coplanar merging, inferred body identity,'
 current={p:sha(ROOT/p) for p in compiled}
 for p,text in compiled.items():
  expected=text.replace(old,new) if p.endswith('CNativeAffineReference.hpp') else text
  assert (ROOT/p).read_text()==expected,('Current code changed beyond documented comment',p)
 # Reuse exact arithmetic evidence only where the defining sources are byte-identical.
 arithmetic=('Common/include/adaptation/CNativePointConstruction3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp')
 assert all(reports['core_v1']['source_sha256'][p]==reports['core_v2']['source_sha256'][p]==current[p] for p in arithmetic)
 oracle=reports['oracle'];assert sha(HERE/'probe.cpp')==oracle['probe_sha256'] and sha(HERE/'oracle.py')==oracle['oracle_sha256'] and sha(HERE/'oracle_v1/probe.log')==oracle['data_sha256']
 assert sha(HERE/'coordinate_compatibility.py')==reports['compatibility']['script_sha256']
 for p,h in reports['compatibility']['reused_objects_and_sources'].items():assert sha(Path(p) if Path(p).is_absolute() else ROOT/p)==h
 review=json.loads((HERE/'abstract_review_receipt.json').read_text());assert review['exit_code']==0 and review['models_reported']==['claude-opus-5-5'] and review['effort']=='high' and not review['is_error']
 representation=json.loads((HERE/'representation_replay.json').read_text());assert sha(HERE/'representation_replay.py')==representation['script_sha256']
 summary=dict(status='PARAMETER_GEOMETRY_CORE_PASS_PRODUCTION_LIFECYCLE_NOT_IMPLEMENTED',current_source_sha256=current,compiled_header_comment_only_annotation=True,arithmetic_evidence_reused_by_identical_sources=list(arithmetic),reports=reports,
  review=review,representation=representation,seal_sha256=sha(Path(__file__).resolve()),
  unsupported=['No runtime option activates mapped bodies.','Boundary parameter store/private proposals/atomic MPI publication and N-M return are not connected.','Finite symmetry-plane/line queries do not yet consume latent bindings.','No mapped parameter SU2/CGNS node-remap persistence or real restart.','All original facet edges remain protected; no general triangulation-seam removal.','No new complete accepted oblique/exterior/multiple-body mesh, 2D lifecycle, viscous or affordability result.'],
  resource_scope='One nice19 compiler at a time; host available memory~40--41GiB. No whole-job peak RSS or complete-event timing measured. Support archive pinning and geometry preparation costs not separately timed.')
 (HERE/'validation_summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(summary['status'])
if __name__=='__main__':main()
