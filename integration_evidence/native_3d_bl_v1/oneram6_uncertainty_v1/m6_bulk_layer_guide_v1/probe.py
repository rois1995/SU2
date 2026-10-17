"""One bounded whole-M6 cap/guide diagnostic on the original owner-UPPER6 field."""
import gzip,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT;CASE=HERE.parent/'first_row_shape_ranking_v1';old_pins=run.pins
source=['UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_bulk_layer_probe.inc','Common/src/adaptation/CNativeMetric3D.cpp','Common/src/adaptation/CNativePredicates3D.cpp','Common/src/adaptation/CNativeField3D.cpp','Common/src/adaptation/CNativeReferenceIO3D.cpp']
inputs=[CASE/p for p in ('Coarse.su2','Coarse.su2.native_ref','run.cfg','sensor_original.bin','working_cells.dat')]+[HERE/'normal_caps.bin',HERE/'wall_geometry.npz',Path(__file__).resolve()]
def pins():return dict(old_pins(),**{p:run.sha(ROOT/p) for p in source},**{str(p.relative_to(ROOT)):run.sha(p) for p in inputs})
run.pins=pins
folder=HERE/'normal_caps_np2';folder.mkdir(exist_ok=True)
capsule=json.dumps({p:(ROOT/p).read_text() for p in source},sort_keys=True).encode();path=folder/'executed_source.json.gz'
if path.exists():assert gzip.decompress(path.read_bytes())==capsule
else:path.write_bytes(gzip.compress(capsule,mtime=0))
run.invoke(folder,{'SU2_NATIVE_FIRST_ROW_ROOT':str(CASE),'SU2_NATIVE_FIRST_ROW_PROBE_TETS':str(HERE/'normal_caps.bin')},CASE/'working_cells.dat',2,test_filter='[NativeFirstRowReplay3D]')
