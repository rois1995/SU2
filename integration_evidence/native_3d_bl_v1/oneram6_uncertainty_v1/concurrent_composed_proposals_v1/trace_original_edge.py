#!/usr/bin/env python3
"""Recover an original-mesh witness for the reported edge; never claim it is the late private cavity."""
import collections,hashlib,json,re,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
source=HERE/'frozen_m6_np4';summary=json.loads((HERE/'summary.json').read_text())
row=next(x for x in summary['M6_engine_rank_diagnostics'] if x['rank']=='0')
assert row['sample_feature']=='2';edge=tuple(sorted((int(row['sample_v0']),int(row['sample_v1']))))
reason=next(l for l in (source/'run.log').read_text().splitlines() if l.startswith('Native3D rejected event:'))
match=re.search(r'interface face (\d+),(\d+),(\d+)',reason);assert match;obstruction=tuple(sorted(map(int,match.groups())))
mesh=read_su2(source/'Coarse.su2');assert max(edge)<len(mesh.P)
indices=np.flatnonzero(np.isin(mesh.E,edge).sum(axis=1)==2);assert 0<len(indices)<=128
cells=mesh.E[indices];ids=sorted(set(cells.ravel().tolist()));local={n:i for i,n in enumerate(ids)}
faces=collections.Counter(tuple(sorted(np.delete(c,k))) for c in cells for k in range(4));boundary=sorted(f for f,count in faces.items() if count==1)
assert all(v<=2 for v in faces.values())
centre=np.asarray(mesh.P[cells],dtype=np.longdouble).mean(axis=1).astype(float)
reported=np.array([float(row['sample_'+k]) for k in ('x','y','z')]);dist=np.max(np.abs(centre-reported),axis=1);nearest=int(dist.argmin())
markers={}
for name,values in mesh.M.items():
 hits=[tuple(map(int,f)) for f in values if tuple(sorted(f)) in boundary]
 if hits:markers[name]=hits
physical={tuple(sorted(f)) for rows in markers.values() for f in rows};markers['ARTIFICIAL_PATCH_INTERFACE']=[f for f in boundary if f not in physical]
folder=HERE/'original_edge_witness';folder.mkdir(exist_ok=True)
p=folder/'original_star.su2'
with p.open('w') as f:
 f.write('%% Original input edge-star witness; NOT the late rejected working mesh.\nNDIME= 3\nNELEM= %d\n'%len(cells))
 for i,c in enumerate(cells):f.write('10 '+' '.join(str(local[int(n)]) for n in c)+' '+str(i)+'\n')
 f.write('NPOIN= %d\n'%len(ids))
 for i,n in enumerate(ids):f.write(' '.join(format(float(x),'.17g') for x in mesh.P[n])+' '+str(i)+'\n')
 f.write('NMARK= %d\n'%len(markers))
 for name,rows in markers.items():
  f.write('MARKER_TAG= '+name+'\nMARKER_ELEMS= '+str(len(rows))+'\n')
  for face in rows:f.write('5 '+' '.join(str(local[int(n)]) for n in face)+'\n')
reload=read_su2(p);assert np.array_equal(reload.P,mesh.P[ids]) and len(reload.E)==len(cells)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
result={'scope':'Original input edge-star witness, not a captured late private cavity or rejected candidate',
 'edge':edge,'obstruction_face':obstruction,'original_cells':indices.tolist(),'original_nodes':ids,
 'boundary_has_reported_obstruction':obstruction in boundary,'reported_centroid':reported.tolist(),
 'closest_original_cell':int(indices[nearest]),'closest_original_centroid':centre[nearest].tolist(),
 'centroid_maximum_absolute_difference':float(dist[nearest]),'centroid_matches_binary64_tolerance':bool(float(dist[nearest])<=8*np.finfo(float).eps*max(1,float(np.max(np.abs(reported))))),
 'physical_markers':{k:len(v) for k,v in markers.items()},'mesh_sha256':sha(source/'Coarse.su2'),
 'log_sha256':sha(source/'run.log'),'helper_sha256':sha(Path(__file__)),
 'witness_mesh_sha256':sha(p),'native_reconstruction_attempted':False,'late_cavity_equivalence_established':False}
(folder/'witness.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
