"""All original M6 wall-face normal caps; actual source input, no changed target."""
import hashlib,json,struct,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];CASE=HERE.parent/'first_row_shape_ranking_v1'
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
mesh=read_su2(CASE/'Coarse.su2');data=np.load(HERE/'wall_geometry.npz');rows=data['measurements'];wall=data['faces']
sensor=CASE/'sensor_original.bin';n=struct.unpack('<Q',sensor.open('rb').read(8))[0]
dtype=np.dtype([('id','<u8'),('point','<f8',(3,)),('metric','<f8',(3,3))]);original=np.memmap(sensor,mode='r',dtype=dtype,offset=8,shape=(n,))
assert np.array_equal(mesh.P[original['id']],original['point'])
record=np.dtype([('parent','<u8'),('tet','<f8',(4,3))]);queries=np.empty(len(rows),dtype=record)
for i,row in enumerate(rows):queries['parent'][i]=int(row[1]);queries['tet'][i,:3]=mesh.P[wall[int(row[0])]];queries['tet'][i,3]=row[6:9]
path=HERE/'normal_caps.bin';payload=struct.pack('<QQ',0x4e41544c50524f42,len(rows))+queries.tobytes();path.write_bytes(payload)
(HERE/'caps_input.json').write_text(json.dumps(dict(rows=len(rows),input_sha256=hashlib.sha256(payload).hexdigest(),geometry_sha256=hashlib.sha256((HERE/'wall_geometry.npz').read_bytes()).hexdigest(),helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),source_rule='Native original owner-UPPER6 sensor with full geometric BL at every actual centroid; field unchanged.'),indent=2)+'\n')
