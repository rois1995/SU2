"""Canonical owner point queries: focused controls, frozen M6 and complete initialized runtime. No CFD solve."""
import csv,json,os,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import run
ROOT=run.ROOT;BASE=Path(__file__).resolve().parent;PILOT="--pilot" in sys.argv
original_pins=run.pins
SOURCES=['Common/src/adaptation/CNativeImport3D.cpp','Common/include/adaptation/CNativeImport3D.hpp',
 'Common/src/adaptation/CNativeRemesher3D.cpp','Common/src/adaptation/CNativeDonor3D.cpp',
 'Common/include/adaptation/CNativeDonor3D.hpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',
 'Common/src/adaptation/CNativePartition3D.cpp','Common/include/adaptation/CNativePartition3D.hpp',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
 'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py','integration_evidence/native_3d_core_v1/audit_coupled.py']
def pins():return dict(original_pins(),**{p:run.sha(ROOT/p) for p in SOURCES})
run.pins=pins
fixture=run.OLD/'cell329/replay.dat'
def invoke(name,ranks,filter,env={}):
 folder=BASE/name;folder.mkdir(exist_ok=True)
 return run.invoke(folder,env,fixture,ranks,test_filter=filter)
source=run.HERE/'construction_first_trial_v1'
for ranks in (4,):
 folder=BASE/('m6_4096_np%d'%ranks);folder.mkdir(exist_ok=True)
 inputs={}
 for name in ('run.cfg','Coarse.su2','Coarse.su2.native_ref','donor_flow.dat','sensor_original.bin'):
  origin=source/name;dest=folder/name
  if name=='run.cfg':
   text=re.sub(r'(?m)^ADAP_NATIVE_RANKS\s*=.*$', 'ADAP_NATIVE_RANKS= '+str(ranks),origin.read_text())
   if dest.exists():assert dest.read_text()==text
   else:dest.write_text(text)
  else:
   if not dest.exists():dest.symlink_to(os.path.relpath(origin.resolve(strict=True),folder))
   dest.resolve(strict=True).relative_to(ROOT)
   assert run.sha(dest)==run.sha(origin)
  inputs[str(dest.relative_to(ROOT))]=run.sha(dest)
 proof=folder/'input_provenance.json'
 if proof.exists():assert json.loads(proof.read_text())==inputs
 else:proof.write_text(json.dumps(inputs,indent=2)+'\n')
 invoke(folder.name,ranks,'[NativeFrozenVolume3D]',{'SU2_NATIVE_FROZEN_CONFIG':str(folder/'run.cfg'),
     'SU2_NATIVE_FROZEN_SENSOR3D':str(folder/'sensor_original.bin'),'SU2_NATIVE_PREPARATION_LIMIT':'4096'})
 rows={}
 reports=[]
 for rank in range(ranks):
  f=folder/('preparation_rank%d.json'%rank);r=json.loads(f.read_text());reports.append(r)
  assert r['exact_weight_match'] and r['exact_query_tensors_match'] and r['requests']==r['compositions']==5*r['cells']
  assert r['field_builds']==0 and r['source_point_queries']<=r['requests']
  with (folder/('preparation_weights_rank%d.csv'%rank)).open() as data:
   for row in csv.DictReader(data):
    cell=tuple(int(row['n%d'%j]) for j in range(4));weight=int(row['retained'])
    assert weight==int(row['cold']) and cell not in rows
    rows[cell]=weight
 archived=run.HERE/'point_preparation_reuse_v1/m6_np1/preparation_weights_rank0.csv'
 with archived.open() as f:reference={tuple(int(row['n%d'%j]) for j in range(4)):int(row['cold']) for row in csv.DictReader(f)}
 assert all(k in rows and rows[k]==v for k,v in reference.items())
 assert any(r['source_point_queries']>2048 for r in reports),'Eviction coverage was not exercised.'
 (folder/'summary.json').write_text(json.dumps({'reports':reports,'source_pins':pins(),
     'minimum_evictions_required':sum(max(0,r['source_point_queries']-2048) for r in reports),
     'eviction_proof':'One memo insertion per successful source miss and fixed2048 capacity; no other deletion in a passing call.',
     'matches_saved_serial_cold_reference':len(set(reference)&set(rows)),
     'serial_reference_sha256':run.sha(archived),'exact_weight_match':True,
     'limits':['Selected original cells, not complete M6 preparation/adaptation.',
               'Cold reference omits production admission bookkeeping; timing is not an old-binary comparison.',
               'No CFD, normalization change, construction or transfer in M6 replay.']},indent=2)+'\n')
print('PASS_LARGER_M6_OWNER_POINT_REPLAY',flush=True)
