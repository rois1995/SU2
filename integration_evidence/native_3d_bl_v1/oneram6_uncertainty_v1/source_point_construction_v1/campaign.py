#!/usr/bin/env python3
"""Focused source-construction controls and one existing complete N/M workflow; no CFD advance."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT;mode=sys.argv[1];assert mode in ('serial','mpi4','runtime')
sources=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
sources += [ROOT/p for p in ('UnitTests/Common/adaptation/CNativeField3D_tests.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc',
 'SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp')]+[Path(__file__)]
base=run.pins;run.pins=lambda:dict(base(),**{str(p.resolve().relative_to(ROOT)):run.sha(p) for p in sources})
folder=HERE/'qualification_v1'/mode;folder.mkdir(parents=True,exist_ok=True)
capsule=json.dumps({str(p.resolve().relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode()
archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
filter='[NativeRepresentableSplit3D],[NativeBalancedSplit3D],[NativeMidpointQuery3D],[NativeSegmentCoverage3D],[NativeComposite3D]'
if mode=='mpi4':filter+=',[NativeOriginalSupport3D],[NativeProgressiveMeasurement3D],[NativeFirstLayerBatch3D]'
if mode=='runtime':filter='[NativeBLBudgetRuntime3D]'
result=run.invoke(folder,{},run.OLD/'cell329/replay.dat',1 if mode=='serial' else 4,test_filter=filter)
if mode=='runtime':
 spec=importlib.util.spec_from_file_location('existing_budget_audit',HERE.parent/'budget_bl_policy_v1/audit_enabled.py')
 audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit);audit.check(folder)
print('QUALIFIED',mode,result['exit_code'],flush=True)
