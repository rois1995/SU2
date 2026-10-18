#!/usr/bin/env python3
"""Resumable exact reuse controls and existing independently audited complete workflows."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT;mode=sys.argv[1];assert mode in ('serial','mpi4','runtime','disabled_runtime')
sources=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
sources += [ROOT/p for p in ('UnitTests/Common/adaptation/CNativeField3D_tests.cpp','UnitTests/Common/adaptation/CNativeMetric3D_tests.cpp',
 'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc','SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
 'SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp')]+[Path(__file__).resolve(),HERE.parent/'run.py',
 HERE.parent/'budget_bl_policy_v1/audit_enabled.py',HERE.parent/'frozen_normalization_stages_v1/campaign.py']
base=run.pins;run.pins=lambda:dict(base(),**{str(p.resolve().relative_to(ROOT)):run.sha(p) for p in sources})
folder=HERE/mode;folder.mkdir(parents=True,exist_ok=True)
capsule=json.dumps({str(p.resolve().relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode()
archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
filter='[NativeCampaignQuery3D],[NativeCampaignEdge3D],[NativeMetricReuse3D],[NativeRepresentableSplit3D],[NativeBalancedSplit3D],[NativeMidpointQuery3D],[NativeSegmentCoverage3D],[NativeComposite3D],[NativeLayeredField3D]'
if mode=='mpi4':filter+=',[NativeOriginalSupport3D],[NativeProgressiveMeasurement3D],[NativeFirstLayerBatch3D]'
if mode=='runtime':filter='[NativeBLBudgetRuntime3D]'
if mode=='disabled_runtime':filter='[NativeNormalizationRuntime3D]'
result=run.invoke(folder,{'SU2_NATIVE_NORMALIZATION_WORKERS':'2'} if mode=='disabled_runtime' else {},
 run.OLD/'cell329/replay.dat',1 if mode=='serial' else 4,test_filter=filter)
if mode in ('runtime','disabled_runtime'):
 path=HERE.parent/('budget_bl_policy_v1/audit_enabled.py' if mode=='runtime' else 'frozen_normalization_stages_v1/campaign.py')
 spec=importlib.util.spec_from_file_location('existing_independent_audit',path)
 audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
 if mode=='runtime':audit.check(folder)
 else:(folder/'independent_runtime.json').write_text(json.dumps(audit.runtime(folder),indent=2,allow_nan=False)+'\n')
print('QUALIFIED',mode,result['exit_code'],flush=True)
