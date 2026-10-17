"""Connected production-estimator checks; sequential, bounded and provenance-aware. No CFD."""
import json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import run
from support_correction import SHARED,domain
ROOT,HERE=run.ROOT,run.HERE
BASE=Path(__file__).resolve().parent
old_pins=run.pins
SOURCES=['SU2_CFD/src/adaptation/CNativeComplexity3D.cpp','SU2_CFD/include/adaptation/CNativeComplexity3D.hpp',
 'SU2_CFD/src/meson.build','Common/include/adaptation/CNativeSupport3D.hpp',
 'Common/include/adaptation/CNativeDonor3D.hpp','Common/src/adaptation/CNativeDonor3D.cpp',
 'UnitTests/SU2_CFD/adaptation/NativeCorrectionReplay3D.hpp']
def pins():
 return dict(old_pins(),**{p:run.sha(ROOT/p) for p in SOURCES})
run.pins=pins
fixture=run.OLD/'cell329/replay.dat'
def invoke(name,env={},ranks=1,filter='[NativeFacetedVolumeReplay3D]',extra=()):
 folder=BASE/name;folder.mkdir(exist_ok=True)
 proof={str(p.relative_to(ROOT)):run.sha(p) for p in extra}
 target=folder/'input_provenance.json'
 if target.exists():assert json.loads(target.read_text())==proof
 else:target.write_text(json.dumps(proof,indent=2)+'\n')
 return run.invoke(folder,env,fixture,ranks,test_filter=filter)
invoke('controls_existing_np1',filter='[NativeComposite2D],[NativeComposite3D],[NativeComplexityNormalization]')
invoke('planar_runtime_np1',filter='Native3D complexity integrates the geometric BL with sensor-only donors')
invoke('dynamic_controls_np1',filter='[NativeDynamicComplexity3D],[NativeIntegrationPolicy3D]')
invoke('feasible_runtime_np1',filter='[NativeFeasibleComplexity3D]')
invoke('runtime_controls_np1',filter='[NativeRuntimeComplexity3D]')
invoke('controls_np4',ranks=4,filter='[NativeDynamicComplexity3D]')
domain()
planar=HERE/'planar_support_correction_v2'
for name,source,wall,ranks in [('planar_np1',planar/'original.bin',planar/'wall.dat',1),
 ('m6_np2',SHARED/'original.bin',fixture,2),('m6_np4',SHARED/'original.bin',fixture,4)]:
 folder=BASE/name;folder.mkdir(exist_ok=True)
 proof={str(p.relative_to(ROOT)):run.sha(p) for p in (source,wall)}
 p=folder/'input_provenance.json'
 if p.exists():assert json.loads(p.read_text())==proof
 else:p.write_text(json.dumps(proof,indent=2)+'\n')
 run.invoke(folder,{'SU2_NATIVE_BL_DYNAMIC_DOMAIN':str(source)},wall,ranks)
 saved=json.loads((folder/'dynamic_complexity.json').read_text())
 assert saved['original_RAW9'] and not saved['actual_CSolver_called']
 assert saved['trials'][0]==saved['trials'][2]
 assert saved['trials'][0]['samples']==8192
 assert saved['trials'][0]['value']!=saved['trials'][1]['value']
 for t in saved['trials']:
  assert t['inside']==t['direct_checks']>0
  assert t['sensor_lower']<=t['sensor_q18']*(1+1e-12)<=t['sensor_upper']*(1+2e-12)
 if name.startswith('m6'):
  audit=json.loads((run.FROZEN/'sensor_mass_audit.json').read_text())
  t=saved['trials'][0]
  for field,key in [('sensor_lower','sensor_lower'),('sensor_upper','sensor_upper')]:
   assert abs(t[field]/audit[key]-1)<2e-10,(field,t[field],audit[key])
  oriented=json.loads((HERE/'production_complexity_v1/oriented_background_reference.json').read_text())
  assert oriented['source_sha256']==run.sha(source)
  assert abs(t['sensor_q18']/oriented['sensor_q18']-1)<2e-10
a=json.loads((BASE/'m6_np2/dynamic_complexity.json').read_text())
b=json.loads((BASE/'m6_np4/dynamic_complexity.json').read_text())
for x,y in zip(a['trials'],b['trials']):
 for k in x:
  assert x[k]==y[k] or abs(x[k]/y[k]-1)<2e-12,(k,x[k],y[k])
summary={'status':'PASS_CURRENT_TRIAL_STENCILS_AND_RUNTIME_NORMALIZATION_CONTROLS',
 'actual_solver_control':True,'full_M6_CSolver_called':False,'M6_frozen_field':b,
 'oriented_background_reference_sha256':run.sha(HERE/'production_complexity_v1/oriented_background_reference.json'),
 'source_pins':pins(),'verification_helper_sha256':run.sha(Path(__file__)),'full_goal_complete':False,'accepted_M6_mesh':False,
 'limits':['Error indicators heuristic; no certified error bound.',
 'No refined-original-field reassessment or correction/remesh stages yet.',
 'Runtime normalization may be provisional at the unchanged requested tolerance.']}
runtime=BASE/'m6_runtime_metric/result.json'
if runtime.exists():
 r=json.loads(runtime.read_text());proof=runtime.parent/'provenance.json';identity=json.loads(proof.read_text())
 current=(r['exit_code']==0 and r['log_sha256']==run.sha(runtime.parent/'run.log') and r['provenance_sha256']==run.sha(proof)
          and all((ROOT/name).is_file() and run.sha(ROOT/name)==digest for name,digest in identity['pins'].items()))
 if current:
  summary.update(status='PASS_RUNTIME_INITIALIZATION_AND_FROZEN_FIELD_CHECKS_WITH_PROVISIONAL_BUDGET',
                 full_M6_CSolver_called=True,actual_M6_metric_runtime=r,runtime_result_sha256=run.sha(runtime))
(BASE/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print('PASS_CONNECTED_PRODUCTION_COMPLEXITY_BATCH',flush=True)
