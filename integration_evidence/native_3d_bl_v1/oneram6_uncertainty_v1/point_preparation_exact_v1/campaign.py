"""Sequential retained-support controls, saved-field M6 replay and complete runtime checks. No CFD solve."""
import csv,json,os,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import run
ROOT=run.ROOT;BASE=Path(__file__).resolve().parent;PILOT="--pilot" in sys.argv
original_pins=run.pins
SOURCES=['Common/src/adaptation/CNativeImport3D.cpp','Common/include/adaptation/CNativeImport3D.hpp',
 'Common/src/adaptation/CNativeRemesher3D.cpp','Common/src/adaptation/CNativeDonor3D.cpp',
 'Common/src/adaptation/CNativePartition3D.cpp','Common/include/adaptation/CNativePartition3D.hpp',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
 'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py','integration_evidence/native_3d_core_v1/audit_coupled.py']
def pins():return dict(original_pins(),**{p:run.sha(ROOT/p) for p in SOURCES})
run.pins=pins
fixture=run.OLD/'cell329/replay.dat'
def invoke(name,ranks,filter,env={}):
 folder=BASE/name;folder.mkdir(exist_ok=True)
 return run.invoke(folder,env,fixture,ranks,test_filter=filter)
invoke('retained_controls_np1',1,'[NativePreparationReuse3D],[NativeBridge3D]')
if not PILOT:invoke('retained_controls_np4',4,'[NativePreparationReuse3D],[NativeBridge3D]')
source=run.HERE/'construction_first_trial_v1'
for ranks in ((1,) if PILOT else (1,2,4)):
 folder=BASE/('m6_np%d'%ranks);folder.mkdir(exist_ok=True)
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
     'SU2_NATIVE_FROZEN_SENSOR3D':str(folder/'sensor_original.bin'),'SU2_NATIVE_PREPARATION_LIMIT':'512'})
 rows={}
 reports=[]
 for rank in range(ranks):
  f=folder/('preparation_rank%d.json'%rank);r=json.loads(f.read_text());reports.append(r)
  assert r['exact_weight_match'] and r['exact_query_tensors_match'] and r['requests']==r['compositions']==5*r['cells']
  with (folder/('preparation_weights_rank%d.csv'%rank)).open() as data:
   for row in csv.DictReader(data):
    cell=tuple(int(row['n%d'%j]) for j in range(4));weight=int(row['retained'])
    assert weight==int(row['cold']) and cell not in rows
    rows[cell]=weight
 if ranks==1:reference=rows
 else:
  assert reference==rows
  shared=set(reference)&set(rows)
 (folder/'summary.json').write_text(json.dumps({'reports':reports,'source_pins':pins(),
     'shared_with_serial':len(set(reference)&set(rows)),'exact_weight_match':True,
     'limits':['Selected original cells, not complete M6 preparation/adaptation.',
               'Cold reference omits production admission bookkeeping; timing is not an old-binary comparison.',
               'No CFD, normalization change, construction or transfer in M6 replay.']},indent=2)+'\n')
if PILOT:
 print('PILOT_COMPLETE_NO_WHOLE_CAMPAIGN_CLAIM',flush=True);raise SystemExit(0)
invoke('curved_runtime_np4',4,'[NativeCurvedRuntime3D]')
invoke('mpi4_m2',4,'[NativeBLPolicyRuntime3D]',{'SU2_NATIVE_POLICY_WORKERS':'2'})
folder=BASE/'mpi4_m2'
receipt={'status':'PASS','contract':{'selected_workers':2},
         'artifacts':{p.name:run.sha(p) for p in folder.iterdir() if p.is_file()}}
(BASE/'mpi4_m2.json').write_text(json.dumps(receipt,indent=2)+'\n')
import importlib.util
spec=importlib.util.spec_from_file_location('policy_audit',ROOT/'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
audit.HERE=BASE;sys.argv=['audit.py','4','2'];audit.main()
(BASE/'summary.json').write_text(json.dumps({'status':'PASS_RETAINED_POINT_SUPPORT_AND_AFFECTED_RUNTIME_CHECKS',
 'source_pins':pins(),'verification_helper_sha256':run.sha(Path(__file__)),'full_goal_complete':False,
 'accepted_M6_mesh':False,'limits':['No construction campaign donor retention or shared-edge/transaction batching.',
 'No complete M6 lifecycle speedup claim; original-field reassessment/correction epochs remain open.']},indent=2)+'\n')
print('PASS_RETAINED_PREPARATION_CONNECTED_BATCH',flush=True)
