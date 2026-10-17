"""Frozen full-target edge reuse: identity controls, original M6 wall-cell replay and scoped complete runtime."""
import csv,json,os,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import run
ROOT=run.ROOT;BASE=Path(__file__).resolve().parent;PILOT="--pilot" in sys.argv
original_pins=run.pins
SOURCES=['Common/include/adaptation/CNativeEngine3D.hpp','Common/src/adaptation/CNativeEngine3D.cpp',
 'Common/include/adaptation/CNativeMetric3D.hpp','Common/src/adaptation/CNativeMetric3D.cpp',
 'UnitTests/Common/adaptation/CNativeMetric3D_tests.cpp','Common/src/adaptation/CNativeImport3D.cpp','Common/include/adaptation/CNativeImport3D.hpp',
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
invoke('serial_controls',1,'[NativeMetricReuse3D],[NativeMesh3D],[NativeComposite3D],[NativeLayeredField3D],[NativeFirstLayerEngine3D]')
source=run.HERE/'construction_first_trial_v1'
allrows={}
for ranks in ((1,) if PILOT else (1,4)):
 folder=BASE/('m6_wall_np%d'%ranks);folder.mkdir(exist_ok=True)
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
     'SU2_NATIVE_FROZEN_SENSOR3D':str(folder/'sensor_original.bin'),'SU2_NATIVE_EDGE_REUSE_LIMIT':'32'})
 rows={};reports=[]
 for rank in range(ranks):
  report=json.loads((folder/('edge_reuse_rank%d.json'%rank)).read_text());reports.append(report)
  assert report['exact_metric_match'] and len(report['passes'])==3
  cold,first,repeat=report['passes'];n=report['cells']
  assert cold['evaluations']==6*n and cold['reused']==0
  assert first['evaluations']+first['reused']==6*n
  assert repeat['evaluations']==0 and repeat['reused']==6*n and repeat['compositions']==n
  with (folder/('edges_rank%d.csv'%rank)).open() as data:
   for row in csv.DictReader(data):
    assert row['cold']==row['reused']==row['repeat']
    key=tuple(int(row[x]) for x in ('n0','n1','n2','n3','a','b'))
    assert key not in rows;rows[key]=tuple(row[x] for x in ('cold','q','J','max_length'))
 assert len(rows)==32*6 and sum(r['passes'][1]['reused'] for r in reports)>0
 allrows[ranks]=rows
 if ranks>1:assert allrows[1]==rows,'MPI changed exact metric answers.'
 (folder/'summary.json').write_text(json.dumps({'reports':reports,'source_pins':pins(),
   'exact_cold_reused_repeat':True,'selected_wall_cells':32,'full_goal_complete':False,
   'scope':'Original M6 wall-cell measurements; no construction, normalization or CFD.'},indent=2)+'\n')
if PILOT:
 print('PASS_SERIAL_EDGE_REUSE_PILOT',flush=True);raise SystemExit(0)
invoke('mpi_controls',4,'[NativeMetricReuse3D],[NativeFirstLayerEngine3D],[NativeFirstLayerConcurrency3D],[NativeCoupledBL3D]')
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
(BASE/'summary.json').write_text(json.dumps({'status':'PASS_FROZEN_COMPOSED_EDGE_REUSE',
 'source_pins':pins(),'verification_helper_sha256':run.sha(Path(__file__)),'full_goal_complete':False,
 'accepted_M6_mesh':False,'limits':['M6 replay covers32 original wall cells, not complete construction.',
 'No full adaptation speedup, uncertainty tolerance or refined-field correction stage.']},indent=2)+'\n')
print('PASS_COMPOSED_EDGE_REUSE_CONNECTED_BATCH',flush=True)
