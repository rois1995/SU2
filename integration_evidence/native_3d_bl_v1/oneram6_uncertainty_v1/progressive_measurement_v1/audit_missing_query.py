"""Exact binary64 original-tet containment for the captured failed actual point; no CFD."""
from pathlib import Path
from fractions import Fraction as F
import hashlib,json,re,sys
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
CASE=HERE.parent/'construction_progressive_diagnostic_v1'
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
text=(CASE/'run.log').read_text()
line=next(s for s in text.splitlines() if 'Native3D rejected event:' in s and 'point=(' in s)
m=re.search(r'point=\(([^)]+)\) admitted=([^;]+)',line);assert m,line
point=tuple(float(v) for v in m.group(1).split(','));assert len(point)==3 and all(np.isfinite(point))
admitted={tuple(sorted(int(v) for v in ids.split(',') if v)) for _,ids in re.findall(r'(\d+)\[([\d,]+)\]',m.group(2))}
assert admitted
mesh=read_su2(CASE/'Coarse.su2');assert mesh.dim==3
bounds=mesh.P[mesh.E];lo=bounds.min(axis=1);hi=bounds.max(axis=1)
candidates=np.flatnonzero(np.all((lo<=point)&(hi>=point),axis=1))
query=tuple(F(v) for v in point)
def determinant(tet):
    a,b,c,d=tet;x,y,z=[tuple(v[j]-a[j] for j in range(3)) for v in (b,c,d)]
    return x[0]*(y[1]*z[2]-y[2]*z[1])-x[1]*(y[0]*z[2]-y[2]*z[0])+x[2]*(y[0]*z[1]-y[1]*z[0])
contained=[]
for i in candidates:
    ids=mesh.E[i];tet=[tuple(F(float(x)) for x in mesh.P[n]) for n in ids];denominator=determinant(tet);assert denominator
    weights=[]
    for j in range(4):
        replaced=list(tet);replaced[j]=query;weights.append(determinant(replaced)/denominator)
    if min(weights)>=0:
        assert sum(weights)==1
        key=tuple(sorted(map(int,ids)))
        contained.append(dict(original_file_cell=int(i),nodes=key,weights=[str(w) for w in weights],in_admitted_packet=key in admitted))
classification='INSIDE_ORIGINAL_BUT_MISSING_IMPORTED_SUPPORT' if contained and not any(r['in_admitted_packet'] for r in contained) else 'INSIDE_ADMITTED_SOURCE_REQUIRES_INDEX_OR_QUERY_AUDIT' if contained else 'OUTSIDE_ORIGINAL_PHYSICAL_DOMAIN'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
result=dict(point=point,admitted_cell_keys=len(admitted),AABB_candidates=len(candidates),exact_containing_cells=contained,
    classification=classification,scope='Exact rational orientation/weights of actual represented query against the ORIGINAL tetrahedral file. No metric/geometry extrapolation. M6 has no analytic extension authority.',
    sha256={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__).resolve(),CASE/'run.log',CASE/'Coarse.su2',CASE/'provenance.json',ROOT/'TestCases/adaptation/capability/capcheck.py')})
(HERE/'missing_query_audit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k not in ('sha256','exact_containing_cells')},indent=2))
