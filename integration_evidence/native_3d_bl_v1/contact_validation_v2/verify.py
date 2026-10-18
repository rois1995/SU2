#!/usr/bin/env python3
"""Recheck provenance and exact-output reuse; no CFD or geometric reconstruction."""
import gzip,hashlib,json,runpy
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
OLD=HERE.with_name('closed_sphere_construction_v1');BASE=HERE.with_name('contact_validation_v1')
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def binary(folder,name):
 p=folder/name;h=hashlib.sha256()
 with (p.open('rb') if p.exists() else gzip.open(folder/(name+'.gz'),'rb')) as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def contact(folder,current):
 r=json.loads((folder/'receipt.json').read_text());assert r['compile']['exit']==r['link']['exit']==0
 assert binary(folder,'measure')==r['binary_sha256']
 assert sha(folder/'predicate_executed.cpp')==r['hashes']['Common/src/adaptation/CNativePredicates3D.cpp']
 for name,expected in r['hashes'].items():
  if name=='Common/src/adaptation/CNativePredicates3D.cpp' and not current:continue
  assert sha(ROOT/name)==expected,name
 if current:
  for name,expected in r['archives'].items():assert sha(Path(name))==expected,name
 for case in r['cases']:
  assert sha(ROOT/case['input'])==case['input_sha256']
  assert sha(folder/(case['name']+'.json'))==case['stdout_sha256']
  assert sha(folder/(case['name']+'.log'))==case['stderr_sha256']
 assert r['cases'][0]['exit']==0
 if current:
  assert r['cases'][1]['exit']==1
  assert 'exhausted90seconds' in (folder/'BL_measure.log').read_text()
 else:assert r['cases'][1]['exit']==0
 return json.loads((folder/'released_measure.json').read_text())
def main():
 seal=HERE/'seal.json'
 if seal.exists():
  for name,expected in json.loads(seal.read_text())['artifacts'].items():assert sha(ROOT/name)==expected,name
 before,after=contact(BASE,False),contact(HERE,True)
 assert before['cells']==after['cells']==320 and before['predicate_calls']==after['predicate_calls']==51040
 native=runpy.run_path(str(OLD/'verify.py'))['native']
 stages=[]
 for new,old in [('construction_v1','geodesic_v1'),('release_v1','release_regional_v1')]:
  folder=HERE/new;original=OLD/old;native(folder,True)
  a=json.loads((folder/'result.json').read_text());b=json.loads((original/'result.json').read_text())
  assert a['valid'] and a['stop']=='complete' and a['immutable_input'] and a['unchanged_authority'] and a['CFD_steps']==0
  assert (a['cells'],a['work'])==(b['cells'],b['work'])
  compared=['accepted.dat','accepted.su2','accepted.su2.native_ref','samples.csv','edges.csv','source.json','run.cfg','sphere.su2','sphere.su2.native_ref']
  for name in compared:assert sha(folder/name)==sha(original/name),name
  oracle=json.loads((original/'independent.json').read_text());assert oracle['status'].startswith('PASS_NATIVE_ACCEPTED_')
  for name,expected in oracle['identity'].items():
   p=ROOT/name
   if sha(p)!=expected and name.endswith('/audit_sphere.py'):p=OLD/'audit_sphere_construct_executed.py'
   assert sha(p)==expected,name
  stages.append({'stage':new,'native_seconds':a['seconds'],'baseline_native_seconds':b['seconds'],'cells':a['cells'],'work':a['work'],'identical_oracle_inputs':{name:sha(folder/name) for name in compared},'retained_oracle_sha256':sha(original/'independent.json')})
 for mode in ['serial','mpi4']:
  folder=HERE/'qualification'/mode;r=json.loads((folder/'result.json').read_text());p=json.loads((folder/'provenance.json').read_text());assert r['exit_code']==0
  assert sha(folder/'provenance.json')==r['provenance_sha256'] and sha(folder/'run.log')==r['log_sha256']
  for name,expected in p['pins'].items():assert sha(ROOT/name)==expected,name
 audit=json.loads((HERE/'primitive_independent_v2.json').read_text());assert len(audit['cases'])==15
 assert audit['helper_sha256']==sha(HERE/'audit_primitive_lifecycles.py')
 assert audit['reference_oracle_sha256']==sha(ROOT/'integration_evidence/native_3d_curved_v1/primitive_audit_v1/audit_primitives_final.py')
 for name,expected in audit['input_hashes'].items():assert sha(HERE/name)==expected,name
 summary={'status':'PASS_EXACT_CONTACT_AND_IDENTICAL_ACCEPTED_SPHERE_OUTPUTS','contact_before':before,'contact_after':after,'stages':stages,'total_native_seconds':sum(s['native_seconds'] for s in stages),'baseline_native_seconds':sum(s['baseline_native_seconds'] for s in stages),'total_work':sum(s['work'] for s in stages),'work_limit':6144,'time_limit':300,'scope':'Identical frozen-field serial mesh construction/removal; reused exact geometry/height/shape and release-field oracle on byte-identical inputs. Current actual primitive MPI N/M Euler lifecycles. No MPI large-BL lifecycle, independent spherical BL field oracle, converged viscous accuracy or affordability claim.'}
 assert summary['total_work']<=6144 and summary['total_native_seconds']<=300
 (HERE/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(summary['status'])
if __name__=='__main__':main()
