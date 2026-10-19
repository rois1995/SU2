#!/usr/bin/env python3
"""Seal the connected v3 checks and reuse independent auditors without changing native artifacts."""
import importlib.util,json,os,re,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main():
 source=module('joint_source_audit',HERE/'audit.py');sha=source.sha;source.selfcheck()
 build=json.loads((HERE/'build_process.json').read_text())
 assert build['status']=='PASS' and build['source_unchanged'] and build['exit_code']==0
 assert sha(ROOT/'integration_evidence/build-integrated-v2/UnitTests/test_driver')==build['binary_sha256']
 assert all(sha(ROOT/p)==h for p,h in build['source_sha256'].items())
 out=HERE/'audits';out.mkdir(exist_ok=True);runs=[];initial=[];signatures=[]
 for mode,ranks in [('source',n) for n in (1,2,4)]+[('affected',n) for n in (1,2,4)]+[('compatibility',1)]:
  folder=HERE/(mode+'_v3_n'+str(ranks));result=json.loads((folder/'result.json').read_text());proof=json.loads((folder/'provenance.json').read_text())
  assert result['exit_code']==0 and result['provenance_sha256']==sha(folder/'provenance.json') and result['log_sha256']==sha(folder/'run.log')
  assert proof['ranks']==ranks and proof['pins']['integration_evidence/build-integrated-v2/UnitTests/test_driver']==build['binary_sha256']
  assert all(sha(ROOT/p)==h for p,h in proof['pins'].items())
  assert all(sha(folder/p)==h for p,h in result['artifacts'].items())
  cases=re.findall(r'Passed (?:all )?(\d+) test cases? with (\d+) assertions?',(folder/'run.log').read_text())
  assert len(cases)==ranks and all(int(n)>0 and int(a)>0 for n,a in cases)
  runs.append(dict(folder=folder.name,ranks=ranks,result_sha256=sha(folder/'result.json'),elapsed_seconds=result['elapsed_seconds'],cases_assertions_per_rank=cases))
  if mode=='source':
   report=out/(folder.name+'.json');data=json.loads(report.read_text())
   assert data['status']=='PASS_INDEPENDENT_EXACT_JOINT_SOURCE_AUDIT' and data['binary_sha256']==build['binary_sha256'] and data['native_result_sha256']==sha(folder/'result.json')
   assert all(sha(ROOT/p)==h for p,h in data['helpers_sha256'].items())
   for c in data['cases']:
    signatures.append((c['original_field_signature'],c['complete_source_field_signature']))
    stem=c['case'];assert sha(folder/(stem+'_source.json'))==c['source_sha256'] and sha(folder/(stem+'.json'))==c['base_sha256'] and sha(folder/(stem+'.native_ref'))==c['reference_sha256']
  if mode=='affected':
   cases=[]
   for components in (1,2):
    pair=[source.refs.audit(folder,'affine_initial_'+physics+'_c'+str(components),components) for physics in ('euler','rans')]
    assert pair[0]['source_signature_sha256']==pair[1]['source_signature_sha256'];cases.extend(pair)
   initial.append(dict(folder=folder.name,cases=cases,narrow_gap=source.refs.narrow_gap(folder)))
 assert len(signatures)==12 and len(set(signatures))==1
 for component in (1,2):
  assert len({c['source_signature_sha256'] for row in initial for c in row['cases'] if c['components']==component})==1
 # Run the existing complete lifecycle audit in a closed relative-link view, keeping native artifact hashes immutable.
 folder=HERE/'compatibility_v3_n1';view=out/'compatibility_v3_n1';view.mkdir(exist_ok=True)
 artifacts=json.loads((folder/'result.json').read_text())['artifacts']
 for name in artifacts:
  target=view/name
  if target.is_symlink():assert target.resolve()==(folder/name).resolve()
  else:assert not target.exists();target.symlink_to(os.path.relpath(folder/name,view))
 auditor=HERE.with_name('planar_symmetry_reference_v1')/'audit_annulus.py';annulus=module('annular_lifecycle_audit',auditor)
 sys.argv=[str(auditor),str(view)];annulus.main()
 report=json.loads((view/'independent_annulus.json').read_text());assert report['status']=='INDEPENDENT_ANNULAR_SYMMETRY_LIFECYCLE_PASS'
 helpers=(Path(__file__),HERE/'audit.py',source.GEOMETRY,source.REFS,auditor,HERE.with_name('production_policy_v1')/'audit.py',ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py',ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py',ROOT/'TestCases/adaptation/capability/capcheck.py')
 summary=dict(status='PASS_SCOPED_JOINT_SOURCE_AND_WALL_COMPATIBILITY',binary_sha256=build['binary_sha256'],build_receipt_sha256=sha(HERE/'build_process.json'),runs=runs,source_domains=12,all_sensor_field_signatures_equal=True,initial_geometry=initial,lifecycle=report,helpers_sha256={str(p.relative_to(ROOT)):sha(p) for p in helpers},limitations=['Numerical-band source geometry; no mapped-body, affine BL-wall or mixed analytic support.','Current complete lifecycle is serial Cartesian annulus; no new oblique/exterior/MPI full lifecycle, physical CFD advance, viscous accuracy or affordability qualification.','Cold contended timings and admitted byte models are not runtime peak RSS, wire volume or speedup measurements.'])
 (HERE/'validation_summary_v3.json').write_text(json.dumps(summary,indent=2)+'\n')
 print(summary['status'],flush=True)
if __name__=='__main__':main()
