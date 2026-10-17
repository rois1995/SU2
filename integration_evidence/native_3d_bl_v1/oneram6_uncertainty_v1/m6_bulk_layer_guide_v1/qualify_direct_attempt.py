"""Affected native3D initialization/atomicity/runtime checks, never advance CFD."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
import run
ROOT=run.ROOT
spec=importlib.util.spec_from_file_location('stages',HERE.parent/'frozen_normalization_stages_v1/campaign.py')
stages=importlib.util.module_from_spec(spec);spec.loader.exec_module(stages)
old_pins=run.pins
sources=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
sources += [ROOT/p for p in ('UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_bulk_layer_probe.inc')]+[Path(__file__).resolve()]
def pins():return dict(old_pins(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources})
run.pins=pins
mode=sys.argv[1];assert mode in ('serial','mpi','runtime','coupled')
ranks=1 if mode=='serial' else 4
filters={'serial':'[NativeFirstLayer3D],[NativeFirstLayerEngine3D],[NativeFirstLayerBatch3D],[NativePolicy3D]',
 'mpi':'[NativeFirstLayerEngine3D],[NativeFirstLayerBatch3D],[NativePolicy3D],[NativeCurvedRuntime3D],[NativeBridge3D]',
 'coupled':'[NativeCoupledBL3D],[NativeCoupledBLGrouped3D],[NativeBLColdEngine3D],[NativeFinerSensorBL3D]',
 'runtime':'[NativeNormalizationRuntime3D]'}
folder=HERE/('qualified_'+mode);folder.mkdir(exist_ok=True)
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
report=run.invoke(folder,{'SU2_NATIVE_NORMALIZATION_WORKERS':'2'} if mode=='runtime' else {},run.OLD/'cell329/replay.dat',ranks,test_filter=filters[mode])
if mode=='runtime':
 result=stages.runtime(folder);(folder/'independent_runtime.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
print('QUALIFIED',mode,report['exit_code'],flush=True)
