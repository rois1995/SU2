#!/usr/bin/env python3
"""Read-only whole-candidate field oracle from the pinned study binary; no remeshing or CFD iterations."""
import hashlib,json,os,re,shutil,subprocess,time,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 case=HERE/(sys.argv[1] if len(sys.argv)>1 else 'affine_band');folder=case/'native_field';folder.mkdir(exist_ok=False)
 template=ROOT/'integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/curved_boundary_residual.json'
 receipt=json.loads(template.read_text());assert receipt['status']=='PASS'
 binary=ROOT/'integration_evidence/build-integrated-v2/UnitTests/test_driver'
 assert sha(binary)==receipt['contract']['pins']['test_binary'],'Oracle binary changed: qualify/rebuild the field oracle explicitly'
 candidate=case/'candidate_rejected.su2';construction=case/'construction.json'
 c=json.loads(construction.read_text());assert c['nonpositive']==0 and sha(candidate)==c['candidate_sha256']
 shutil.copyfile(candidate,folder/'candidate.su2')
 sources=ROOT/'integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/curved_boundary_residual/source_snapshots'
 # This exact binary's captured sources, rather than current restored production sources, establish its provenance.
 for name,h in receipt['contract']['pins'].items():
  p=sources/name
  if p.is_file():assert sha(p)==h,name
 env={k:v for k,v in os.environ.items() if not k.startswith('SU2_NATIVE_CURVED_')}
 env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',SU2_NATIVE_CURVED_EVIDENCE='1',SU2_NATIVE_CURVED_GRID_AUDIT='candidate.su2')
 command=['taskset','-c','5','nice','-n','19',str(binary),'[NativeCurvedBLConstruction3D]','--reporter','compact']
 start=time.monotonic()
 contract=dict(command=command,binary_sha256=sha(binary),oracle_commit='39a1f487a628b72b896a0df1bd4069e7b7a610fa',template_receipt_sha256=sha(template),construction_sha256=sha(construction),candidate_sha256=sha(candidate),helper_sha256=sha(Path(__file__)),environment={k:v for k,v in env.items() if k.startswith('SU2_NATIVE_CURVED_') or k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS')})
 (case/'native_field.json').write_text(json.dumps(dict(status='RUNNING',collector_pid=os.getpid(),contract=contract),indent=2)+'\n')
 with (folder/'native.log').open('w') as out:
  try:code=subprocess.run(command,cwd=folder,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=180).returncode
  except subprocess.TimeoutExpired:code=124
 assert sha(binary)==contract['binary_sha256'] and sha(candidate)==contract['candidate_sha256']
 report=dict(status='REJECTED_NATIVE_AUDIT_EXECUTION',returncode=code,contract=contract,elapsed_seconds=time.monotonic()-start)
 if code==0:
  counts=re.findall(r'Passed (?:(?:all|both) )?(\d+) test cases? with ',(folder/'native.log').read_text());assert counts==['1']
  stats=json.loads((folder/'grid_metric_summary.json').read_text());exported=json.loads((folder/'audited_grid.json').read_text())
  original=json.loads((HERE.parent/'wall_reconstruction_v1/curved_metric_complete/audited_grid.json').read_text())
  assert exported['original']==original['original'] and all(exported[k]==original[k] for k in ('facets','facet_groups','marker_names'))
  report.update(status='PASS_NATIVE_FIELD_ONLY' if stats['shape_failures']==0 and stats['oversized_edges']==0 else 'REJECTED_NATIVE_METRIC',metrics=stats,original_donors_and_reference_unchanged=True)
 report.update(artifacts={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()},scope='Read-only native actual-query centroid q/J and full geometry-resolved composed edge measurements of complete candidate, same original-P1 sensors/BL/fade/reference. Native test export success is not mesh acceptance. Separate independent embedding/height/geometry and actual native integration remain required; no CFD/MPI/transfer/restart or speedup claim.')
 (case/'native_field.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status']);print(report.get('metrics',{}))
if __name__=='__main__':main()
