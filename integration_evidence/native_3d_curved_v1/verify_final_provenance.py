from pathlib import Path
import os,json,hashlib,subprocess
r=Path.cwd();b=r/'integration_evidence/native_3d_curved_v1';build=r/'integration_evidence/build-integrated-v2'
os.nice(19);os.sched_setaffinity(0,{min(os.sched_getaffinity(0))});cache={}
def sha(p):
 p=p.resolve(strict=True)
 if p not in cache:
  h=hashlib.sha256()
  with p.open('rb') as f:
   for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
  cache[p]=h.hexdigest()
 return cache[p]
def pins(values,base):
 for n,h in values.items():assert sha(base/n)==h,str(base/n)
 return len(values)
checks=[]
for name in ['cylinder_cap_coverage_v2','cylinder_cap_regression_v2','curved_transaction_v1','resolved_vortex_v3','cylinder_cap_audit_v1']:
 p=b/name/'validation.json';d=json.loads(p.read_text());assert d['status']=='PASS';count=pins(d['artifact_sha256'],b/name)
 checks.append({'receipt':str(p.relative_to(r)),'sha256':sha(p),'status':d['status'],'artifact_count':count})
latest=json.loads((b/'resolved_vortex_v3/validation.json').read_text());baseline=json.loads((b/'cylinder_cap_regression_v2/validation.json').read_text());primitive=json.loads((b/'cylinder_cap_coverage_v2/validation.json').read_text())
source_count=pins(latest['source_sha256'],r);production={p:h for p,h in baseline['source_sha256'].items() if p.startswith(('Common/','SU2_CFD/'))};production_count=pins(production,r)
objects=pins(latest['actual_objects_sha256'],build);libraries=pins(latest['library_sha256'],r);members=pins(latest['thin_archive_member_sha256'],r)
assert sha(build/'UnitTests/test_driver')==latest['binary_sha256'];assert baseline['library_sha256']==latest['library_sha256'] and baseline['thin_archive_member_sha256']==latest['thin_archive_member_sha256']
changed=[p for p,h in primitive['source_sha256'].items() if p.startswith(('Common/','SU2_CFD/')) and sha(r/p)!=h];assert changed==['Common/src/adaptation/CNativeBoundary3D.cpp']
old=subprocess.check_output(['git','show','240a27bd2576ff1a2f0d725127df86bb39d5d0c2^:Common/src/adaptation/CNativeBoundary3D.cpp']).decode();assert hashlib.sha256(old.encode()).hexdigest()==primitive['source_sha256'][changed[0]]
current=(r/changed[0]).read_text().replace('"Native 3D incomplete movement dependency at interface node "+std::to_string(vertex)+"."','"Native3D movement has an unadmitted interface vertex."').replace('"Native 3D movement leaves incident reference facet "+std::to_string(marked->second.facet)+"."','"Native3D movement leaves an incident reference feature."');assert current==old
for name,base in [('cylinder_cap_audit_v1/fresh_collar_audit.json',r),('cylinder_cap_audit_v1/cylinder_cap_coverage_v2_audit.json',b),('cylinder_cap_regression_v2/independent_lifecycle_validation.json',b/'cylinder_cap_regression_v2'),('resolved_vortex_audit_v1/physics_audit.json',b/'resolved_vortex_v3')]:
 p=b/name;d=json.loads(p.read_text());assert d['status'].startswith('PASS');count=pins(d.get('inputs_sha256',d.get('input_sha256',{})),base)
 if 'helper_sha256' in d:pins(d['helper_sha256'],r/'integration_evidence/native_3d_core_v1')
 checks.append({'receipt':str(p.relative_to(r)),'sha256':sha(p),'status':d['status'],'input_count':count,'input_base':str(base.relative_to(r)) or '.'})
for rel,script,key in [('cylinder_cap_audit_v1/fresh_collar_audit.json','cylinder_cap_audit_v1/audit.py','auditor_sha256'),('cylinder_cap_regression_v2/independent_lifecycle_validation.json','cylinder_cap_regression_v2/audit_lifecycle.py','script_sha256'),('resolved_vortex_audit_v1/physics_audit.json','resolved_vortex_audit_v1/audit_physics.py','script_sha256')]:assert json.loads((b/rel).read_text())[key]==sha(b/script)
a=b/'resolved_vortex_audit_v2';controller=json.loads((a/'validation.json').read_text());assert controller['status']=='PASS';pins(controller['artifact_sha256'],a)
plan=[(n,m,e) for n in (1,2,4) for m in (1,2,4) if m<=n for e in range(2)];assert [tuple(c) for c in controller['planned_events']]==plan
for c in plan:
 p=a/('event_n%d_m%d_s%d.json'%c);d=json.loads(p.read_text());assert d['status']=='PASS_RESOLVED_VORTEX_EVENT' and tuple(d['selection'])==c;assert d['script_sha256']==sha(a/'audit_lifecycle.py');pins(d['inputs_sha256'],r);pins(d['helper_sha256'],r/'integration_evidence/native_3d_core_v1')
agg=json.loads((a/'independent_lifecycle_validation.json').read_text());assert agg['status']=='PASS_RESOLVED_VORTEX_LIFECYCLES';pins(agg['inputs_sha256'],r);rows=[x for cases in agg['results'].values() for es in cases.values() for x in es];assert len(rows)==12
links=[]
for folder in ['resolved_vortex_v2','resolved_vortex_v3']:
 for p in (b/folder).rglob('*'):
  if p.is_symlink():links.append({'path':str(p.relative_to(r)),'target':str(p.resolve(strict=True).relative_to(r))})
subprocess.run(['git','merge-base','--is-ancestor','23197eba1715f44d61c9ad3a070f61441dfb0560','HEAD'],check=True)
report={'status':'PASS_FINAL_SOURCE_BUILD_AND_EVIDENCE','native_receipts':checks,'current_source_files':source_count,'unchanged_production_files':production_count,'actual_test_objects':objects,'libraries':libraries,'thin_archive_members':members,'actual_binary_sha256':latest['binary_sha256'],'primitive_source_difference':'Exactly two reviewed error-message expressions; accepted geometry/source algorithms byte-identical after those two replacements. Current diagnostics tested by 107-case/rank regression.','resolved_event_count':12,'resolved_controller_sha256':sha(a/'validation.json'),'resolved_aggregate_sha256':sha(a/'independent_lifecycle_validation.json'),'verifier_sha256':sha(Path(__file__)),'metric_summary':{'minimum_q':min(x['minimum_metric_mean_ratio'] for x in rows),'minimum_j':min(x['minimum_metric_scaled_jacobian'] for x in rows),'maximum_length':max(x['maximum_exact_source_metric_edge'] for x in rows),'maximum_transported_metric_error':max(x['transported_metric_relative_error'] for x in rows)},'closed_repository_links':links,'goal1_ancestor':True,'scope':'Current source/build/binary and completed immutable input/certificate verification; no new solver run or performance assessment. Original-scope requirement review and final publication are separate.'}
(b/'final_source_and_evidence_validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ['native_receipts','closed_repository_links']},indent=2))
