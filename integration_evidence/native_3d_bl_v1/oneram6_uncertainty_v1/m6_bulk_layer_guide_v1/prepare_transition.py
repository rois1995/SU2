"""Build direct-cone transition/unique-wall-edge queries, preserving original parents."""
import json,struct,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];CASE=HERE.parent/'first_row_shape_ranking_v1'
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2
m=read_su2(CASE/'Coarse.su2');data=np.load(HERE/'wall_geometry.npz');faces=data['faces'];rows=data['measurements']
dtype=np.dtype([('parent','<u8'),('tet','<f8',(4,3))]);out=np.empty(3*len(rows),dtype=dtype);edges={}
for i,row in enumerate(rows):
 f=faces[int(row[0])];parent=int(row[1]);e=int(row[2]);q=row[6:9]
 for j in range(3):
  out['parent'][3*i+j]=parent;out['tet'][3*i+j]=[q,m.P[f[(j+1)%3]],m.P[f[(j+2)%3]],m.P[e]]
  edge=tuple(sorted((int(f[j]),int(f[(j+1)%3]))));edges[edge]=min(parent,edges.get(edge,parent))
(HERE/'transition_children.bin').write_bytes(struct.pack('<QQ',0x4e41544c50524f42,len(out))+out.tobytes())
values=np.array([(v,*k) for k,v in sorted(edges.items())],dtype='<u8')
assert len(np.unique(out['parent']))==len(rows) and len(values)==len(edges)
(HERE/'wall_edges.bin').write_bytes(struct.pack('<QQ',0x4e41544550524f42,len(values))+values.tobytes())
print(json.dumps({'transition_children':len(out),'unique_wall_edges':len(values),'accepted_mesh':False}))
