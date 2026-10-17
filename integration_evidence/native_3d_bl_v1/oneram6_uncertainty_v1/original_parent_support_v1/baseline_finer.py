"""One unchanged-source baseline of the unresolved generic finer-sensor control."""
import gzip,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT;old_pins=run.pins
paths=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
paths += [ROOT/'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp',Path(__file__).resolve()]
def pins():return dict(old_pins(),**{str(p.relative_to(ROOT)):run.sha(p) for p in paths})
run.pins=pins
folder=HERE/'baseline_finer_np4';folder.mkdir(exist_ok=True)
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in paths},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
run.invoke(folder,{},run.OLD/'cell329/replay.dat',4,test_filter='[NativeFinerSensorBL3D]')
