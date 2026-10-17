"""Bounded real M6 finite-feature geometry replay, independent of the CFD executable."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
SOURCE=HERE.parent/'first_row_shape_ranking_v1/Coarse.su2'
mesh=read_su2(SOURCE);triangles=mesh.M['WING3D']
assert len(triangles)==38800
low=mesh.P[triangles].min(axis=1);high=mesh.P[triangles].max(axis=1)
# Combine the beginning of the real cell order, volume-spanning cells and actual wall edges.
indices=np.unique(np.r_[np.linspace(0,8191,128,dtype=int),np.linspace(0,len(mesh.E)-1,128,dtype=int)])
edges=np.unique(np.sort(np.r_[mesh.E[indices][:,[0,1]],mesh.E[indices][:,[2,3]],triangles[np.linspace(0,len(triangles)-1,128,dtype=int),:2]],axis=1),axis=0)
counts=[]
for a,b in mesh.P[edges]:
    lo=np.minimum(a,b)-.01;hi=np.maximum(a,b)+.01
    counts.append(np.count_nonzero(np.all((low<=hi)&(high>=lo),axis=1)))
counts=np.array(counts);order=np.argsort(counts,kind='stable')
positive=order[counts[order]>0];zero=order[counts[order]==0]
chosen=np.unique(np.r_[positive[-32:],positive[np.linspace(0,len(positive)-1,min(32,len(positive)),dtype=int)],zero[:8]])
selected=edges[chosen]
with (HERE/'geometry.bin').open('wb') as f:
    f.write(np.array([0x43484f52444d3655,len(mesh.P),len(triangles),len(selected)],dtype='<u8').tobytes())
    f.write(np.asarray(mesh.P,dtype='<f8').tobytes());f.write(np.asarray(triangles,dtype='<u8').tobytes());f.write(np.asarray(selected,dtype='<u8').tobytes())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
(HERE/'inputs.json').write_text(json.dumps(dict(mesh=str(SOURCE.relative_to(ROOT)),mesh_sha256=sha(SOURCE),
    preparation_helper_sha256=sha(Path(__file__)),geometry_sha256=sha(HERE/'geometry.bin'),
    points=len(mesh.P),wall_faces=len(triangles),edges=len(selected),candidates=counts[chosen].tolist(),
    selected_edges=selected.tolist(),limits='Bounded original-edge sample; not a seed-wide cost or CFD/whole-event timing.'),indent=2)+'\n')
print('PREPARED',len(selected),'original edges; face candidates',counts[chosen].min(),counts[chosen].max())
