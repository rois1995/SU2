"""Affected active-support geometry/field/runtime checks, never advance CFD."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
import run
ROOT=run.ROOT
spec=importlib.util.spec_from_file_location('stages',HERE.parent/'frozen_normalization_stages_v1/campaign.py')
stages=importlib.util.module_from_spec(spec);spec.loader.exec_module(stages)
old_pins=run.pins
sources=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
sources += [ROOT/p for p in ('SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','UnitTests/Common/adaptation/CNativeField3D_tests.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_bulk_layer_probe.inc')]+[Path(__file__).resolve()]
def pins():return dict(old_pins(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources})
run.pins=pins
mode=sys.argv[1]
edges=('first_layer_edge_band_reference_v1','normal_classification_first_edge_v1','normal_classification_surface_edge_v1','interior_first_edge_final_v1','interior_surface_edge_final_v1')
assert mode in ('serial','mpi','runtime','cell88','cell157','cell329') or mode in edges
ranks=4 if mode=='runtime' else 2 if mode=='mpi' else 1
filters={'serial':'[NativeChordEnvelope],[NativeVolumeBL3D],[NativeParallelBL3D],[NativeCurvedBL3D],[BoundaryLayerMetric],[NativeComposite3D]',
 'mpi':'[NativeChordEnvelope],[NativeVolumeBL3D]', 'runtime':'[NativeNormalizationRuntime3D]'}
folder=HERE/mode;folder.mkdir(exist_ok=True)
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
env={'SU2_NATIVE_NORMALIZATION_WORKERS':'2'} if mode=='runtime' else {}
fixture=run.OLD/'cell329/replay.dat'
if mode in edges:
 env.update(SU2_NATIVE_BL_EDGE=str(HERE.parent/mode/'edge.dat'),
            SU2_NATIVE_BL_EDGE_SENSOR=str(HERE.parent/'first_row_shape_ranking_v1/sensor_original.bin'))
if mode.startswith('cell'):
 fixture=run.OLD/mode/'replay.dat';env['SU2_NATIVE_BL_COMPLEXITY']='8192'
for path in [fixture]+[Path(v) for k,v in env.items() if k.startswith('SU2_NATIVE_BL_EDGE')]:
 original_pins=pins
 def pins(original_pins=original_pins,path=path):return dict(original_pins(),**{str(path.relative_to(ROOT)):run.sha(path)})
 run.pins=pins
report=run.invoke(folder,env,fixture,ranks,test_filter=filters.get(mode,'[NativeFacetedVolumeReplay3D]'))
if mode=='runtime':
 result=stages.runtime(folder);(folder/'independent_runtime.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
print('QUALIFIED',mode,report['exit_code'],flush=True)
