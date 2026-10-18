#!/usr/bin/env python3
"""Resumable focused sphere control and existing actual primitive N/M lifecycle checks."""
import gzip,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;SHARED=HERE.parent/'oneram6_uncertainty_v1';sys.path.insert(0,str(SHARED));import run
ROOT=run.ROOT;mode=sys.argv[1];assert mode in ('serial','mpi4')
sources=[p for d in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/d).glob('CNative*3D.*')]
sources += [ROOT/p for p in ['UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp']]+[Path(__file__).resolve()]
base=run.pins;run.pins=lambda:dict(base(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources})
folder=HERE/'qualification'/mode;folder.mkdir(parents=True,exist_ok=True)
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
result=run.invoke(folder,{},run.OLD/'cell329/replay.dat',1 if mode=='serial' else 4,test_filter='[NativeContact3D],[NativeSphereFront3D],[NativePrimitiveRuntime3D]')
print('QUALIFIED',mode,result['exit_code'],flush=True)
