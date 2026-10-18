#!/usr/bin/env python3
"""Exact positive-child witness for joint construction versus sequential wall-point insertion."""
import gzip,hashlib,importlib.util,itertools,json,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'));import audit_coupled as exact
V=[(0.,0.,0.),(1.,1.5075567228888185,-.7196119662144681),(0.,1.809068067466582,1.7893054295062427),(0.,0.,1.0696934632917758)]
def det(t):return exact.determinant([tuple(F(float(x)) for x in p) for p in t])
def faces(t):
 a,b,c,d=t;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def split(t,p):return [(p,)+f for f in faces(t)]
def main():
 assert det(V)>0;points={}
 for i in (1,2):
  a,b,c=[np.array(q,dtype=np.longdouble) for q in faces(tuple(V))[i]];n=np.cross(b-a,c-a);n/=np.sqrt(sum(n*n));center=np.asarray((a+b+c)/3,dtype=float)
  points[i]=tuple(float(np.longdouble(center[k])-np.longdouble(.1)*n[k]) for k in range(3))
 background=tuple(float(sum(np.longdouble(q[k])/4 for q in V)) for k in range(3));reports=[]
 for bg in (None,background):
  for order in ((1,2),(2,1)):
   cells=[tuple(V)] if bg is None else split(tuple(V),bg);steps=[]
   for i in order:
    face=set(faces(tuple(V))[i]);at=next(k for k,t in enumerate(cells) if face.issubset(set(t)));children=split(cells[at],points[i]);volumes=[det(t) for t in children];valid=all(v>0 for v in volumes)
    steps.append(dict(wall=i,positive=valid,minimum_exact_determinant=str(min(volumes))))
    if not valid:break
    cells[at:at+1]=children
   if bg is None:assert [s['positive'] for s in steps]==[True,False]
   else:
    assert all(s['positive'] for s in steps) and len(cells)==10 and sum(det(t) for t in cells)==det(V)
    for a,b in itertools.combinations(cells,2):assert not exact.overlap([tuple(map(F,p)) for p in a],[tuple(map(F,p)) for p in b])
   reports.append(dict(common_background=bg,order=order,steps=steps,final_cells=len(cells)))
 capsules=[HERE/'annulus_v1_n1/executed_source.json.gz',HERE/'controls_v2_n1/executed_source.json.gz'];sources=[json.loads(gzip.decompress(p.read_bytes()))['Common/src/adaptation/CNativeBoundary3D.cpp'] for p in capsules]
 assert 'Cone(old.cells,background,first_cell' in sources[0] and 'Patch proposal;proposal.cells=old.cells;' in sources[1]
 result=dict(status='EXACT_COUPLED_WALL_COUNTEREXAMPLE_CONFIRMED',points=V,first_height=.1,wall_opposite_vertices=[1,2],normal_points=points,background=background,construction=reports,
  scope='Geometric fixed-interface/prescribed point construction only. Common fan positive/nonoverlapping/volume closing, direct sequence blocked in both orders. Not full metric/quality acceptance or a native runtime replay.',
  consequence='Retain qualified common-background joint construction. Restrict the direct operator to one wall face in one tetrahedron; do not replace the generic coupled constructor.',
  hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in capsules+[Path(__file__).resolve()]})
 (HERE/'coupled_wall_counterexample.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
if __name__=='__main__':main()
