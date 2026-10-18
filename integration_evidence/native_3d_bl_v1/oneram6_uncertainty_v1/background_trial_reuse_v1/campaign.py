"""Resumable focused policy/compatibility/lifecycle checks, no CFD advancement."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT
spec=importlib.util.spec_from_file_location('stages',HERE.parent/'frozen_normalization_stages_v1/campaign.py')
stages=importlib.util.module_from_spec(spec);spec.loader.exec_module(stages)
base_pins=run.pins
sources=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
sources += [ROOT/p for p in ('Common/include/CConfig.hpp','Common/src/CConfig.cpp','config_template.cfg',
 'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',
 'SU2_CFD/src/adaptation/CNativeComplexity3D.cpp','SU2_CFD/include/adaptation/CNativeComplexity3D.hpp',
 'SU2_CFD/include/adaptation/CNativeCandidateComplexity3D.hpp','SU2_CFD/include/adaptation/CNativeMetricFamily3D.hpp',
 'SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
 'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_saved_metric_probe.inc')]+[Path(__file__).resolve()]
sources += [ROOT/p for p in ('integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_policy_v1/audit_enabled.py',
 'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py','integration_evidence/native_3d_core_v1/audit_coupled.py',
 'integration_evidence/native_3d_core_v1/audit_cavities.py','TestCases/adaptation/capability/capcheck.py')]
run.pins=lambda:dict(base_pins(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources})
mode=sys.argv[1];assert mode in ('primitives','disabled_serial','disabled_runtime','enabled_serial','enabled_mpi2','enabled_mpi4','uncertainty_serial','uncertainty_mpi4')
ranks=2 if mode=='enabled_mpi2' else 4 if mode in ('enabled_mpi4','disabled_runtime','uncertainty_mpi4') else 1
filters={'primitives':'[NativeBLBudgetPolicy],[NativeBLBudgetProfile],[NativeComplexityNormalization],[NativeDynamicComplexity3D],[NativeRuntimeComplexity3D],[NativeComposite2D],[NativeComposite3D],[NativeMetricFamily3D]',
 'disabled_serial':'[NativeComposite2D],[NativeComposite3D],[NativeMetricFamily3D],[NativeDynamicComplexity3D],[NativeRuntimeComplexity3D],Native3D complexity integrates the geometric BL with sensor-only donors',
 'uncertainty_serial':'[NativeBLBudgetUncertainty3D]','uncertainty_mpi4':'[NativeBLBudgetUncertainty3D],[NativeDynamicComplexity3D],[NativeBLBudgetProfile]',
 'disabled_runtime':'[NativeNormalizationRuntime3D]'}
filter=filters.get(mode,'[NativeBLBudgetRuntime3D]');folder=HERE/'qualification_v2'/mode;folder.mkdir(parents=True,exist_ok=True)
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
env={'SU2_NATIVE_NORMALIZATION_WORKERS':'2'} if mode=='disabled_runtime' else {}
report=run.invoke(folder,env,run.OLD/'cell329/replay.dat',ranks,test_filter=filter)
if mode=='disabled_runtime':
 checks=stages.runtime(folder);(folder/'independent_runtime.json').write_text(json.dumps(checks,indent=2,allow_nan=False)+'\n')
if mode.startswith('enabled'):
 profiles=[json.loads(p.read_text()) for p in folder.glob('bl_budget_*_profile.json')]
 assert len(profiles)==2 and {p['budget'] for p in profiles}=={3,20},'Both independently complete cases are required'
 assert all(p['accepted'] and p['frozen'] and p['trials']<=9 for p in profiles)
 assert next(p['parameter'] for p in profiles if p['budget']==3)>0
 (folder/'profiles.json').write_text(json.dumps(profiles,indent=2)+'\n')
 spec=importlib.util.spec_from_file_location('existing_enabled_audit',HERE.parent/'budget_bl_policy_v1/audit_enabled.py')
 auditor=importlib.util.module_from_spec(spec);spec.loader.exec_module(auditor);auditor.check(folder)
print('QUALIFIED',mode,report['exit_code'],flush=True)
