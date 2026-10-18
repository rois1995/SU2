#!/usr/bin/env python3
"""Default-policy original-family correction and current/BDF restart, with the existing independent audit."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT
sources=[p for name in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/name).glob('CNative*3D.*')]
sources += [ROOT/p for p in ('SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc')]+[Path(__file__).resolve()]
base=run.pins;run.pins=lambda:dict(base(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources})
folder=HERE/'disabled_runtime';folder.mkdir(exist_ok=True);archive=folder/'executed_source.json.gz'
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode()
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
result=run.invoke(folder,{'SU2_NATIVE_NORMALIZATION_WORKERS':'2'},run.OLD/'cell329/replay.dat',4,test_filter='[NativeNormalizationRuntime3D]')
spec=importlib.util.spec_from_file_location('existing_stages_audit',HERE.parent/'frozen_normalization_stages_v1/campaign.py')
stages=importlib.util.module_from_spec(spec);spec.loader.exec_module(stages);checks=stages.runtime(folder)
(folder/'independent_runtime.json').write_text(json.dumps(checks,indent=2,allow_nan=False)+'\n');print('QUALIFIED disabled_runtime',result['exit_code'],flush=True)
