#!/usr/bin/env python3
"""Resumable complete-proposal controls and existing actual N/M workflows; no CFD advance."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT;mode=sys.argv[1];assert mode in ('serial','mpi4','enabled_mpi4','disabled_runtime')
sources=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
sources += [ROOT/p for p in ('UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc','UnitTests/SU2_CFD/adaptation/fixtures/native_saved_metric_probe.inc',
 'SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp',
 'SU2_CFD/include/adaptation/CNativeCandidateComplexity3D.hpp','SU2_CFD/include/adaptation/CNativeMetricFamily3D.hpp',
 'integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_policy_v1/audit_enabled.py',
 'integration_evidence/native_3d_bl_v1/production_policy_v1/audit.py','integration_evidence/native_3d_core_v1/audit_coupled.py',
 'integration_evidence/native_3d_core_v1/audit_cavities.py','TestCases/adaptation/capability/capcheck.py')]+[Path(__file__).resolve()]
base=run.pins;run.pins=lambda:dict(base(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources})
folder=HERE/'qualification_v1'/mode;folder.mkdir(parents=True,exist_ok=True)
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
filters='[NativeConcurrentComposed3D],[NativeProgressiveMeasurement3D],[NativeBatchEngine3D],[NativeCoupledBLBudget3D],[NativeFirstLayerBatch3D],[NativeMidpointQuery3D]'
if mode=='enabled_mpi4':filters='[NativeBLBudgetRuntime3D]'
if mode=='disabled_runtime':filters='[NativeNormalizationRuntime3D]'
env={'SU2_NATIVE_NORMALIZATION_WORKERS':'2'} if mode=='disabled_runtime' else {}
r=run.invoke(folder,env,run.OLD/'cell329/replay.dat',1 if mode=='serial' else 4,test_filter=filters)
if mode=='enabled_mpi4':
 spec=importlib.util.spec_from_file_location('enabled_audit',HERE.parent/'budget_bl_policy_v1/audit_enabled.py')
 audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit);audit.check(folder)
if mode=='disabled_runtime':
 spec=importlib.util.spec_from_file_location('stages',HERE.parent/'frozen_normalization_stages_v1/campaign.py')
 stages=importlib.util.module_from_spec(spec);spec.loader.exec_module(stages)
 (folder/'independent_runtime.json').write_text(json.dumps(stages.runtime(folder),indent=2,allow_nan=False)+'\n')
print('QUALIFIED',mode,r['exit_code'],flush=True)
