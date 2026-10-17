"""Find declared midpoint witnesses and check ideal versus represented points exactly."""
from pathlib import Path
from fractions import Fraction as F
import hashlib,json,re,sys
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];CASE=HERE.parent/'construction_progressive_diagnostic_v1'
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'));from audit_coupled import determinant
base=json.loads((HERE/'missing_query_audit.json').read_text());point=np.array(base['point'])
line=next(s for s in (CASE/'run.log').read_text().splitlines() if 'Native3D rejected event:' in s and 'point=(' in s)
packet=re.search(r'admitted=([^;]+)',line).group(1)
keys={tuple(sorted(int(v) for v in ids.split(',') if v)) for _,ids in re.findall(r'(\d+)\[([\d,]+)\]',packet)}
ids=sorted({v for k in keys for v in k});assert len(ids)<=1024
mesh=read_su2(CASE/'Coarse.su2');pairs=[]
for at,a in enumerate(ids):
    others=ids[at+1:]
    midpoint=((mesh.P[others].astype(np.longdouble)+mesh.P[a].astype(np.longdouble))/2).astype(float)
    for j in np.flatnonzero(np.all(midpoint==point,axis=1)):pairs.append((a,others[j]))
record=[]
for a,b in pairs:
    ideal=tuple((F(float(x))+F(float(y)))/2 for x,y in zip(mesh.P[a],mesh.P[b]))
    candidates=np.flatnonzero(np.any(mesh.E==a,axis=1)&np.any(mesh.E==b,axis=1))
    witnesses=[]
    for i in candidates:
        nodes=mesh.E[i];tet=[tuple(F(float(x)) for x in mesh.P[n]) for n in nodes];den=determinant(tet);assert den
        def weights(p):
            out=[]
            for k in range(4):
                trial=list(tet);trial[k]=p;out.append(determinant(trial)/den)
            return out
        w=weights(ideal);rounded=weights(tuple(F(float(x)) for x in point))
        if min(w)>=0:
            key=tuple(sorted(map(int,nodes)))
            witnesses.append(dict(original_cell=int(i),nodes=key,ideal_weights=[str(x) for x in w],
                represented_weights=[str(x) for x in rounded],in_admitted_packet=key in keys))
    record.append(dict(endpoints=(a,b),coordinates=mesh.P[[a,b]].tolist(),ideal=[str(x) for x in ideal],
        coordinate_roundoff=[float(F(float(x))-q) for x,q in zip(point,ideal)],containing_original_edge_cells=witnesses))
assert base['classification']=='OUTSIDE_ORIGINAL_PHYSICAL_DOMAIN'
result=dict(point=point.tolist(),midpoint_pairs=record,classification='ROUNDED_MIDPOINT_LEAVES_ORIGINAL_DOMAIN' if any(r['containing_original_edge_cells'] for r in record) else 'NO_ORIGINAL_VERTEX_MIDPOINT_WITNESS',
    strict_predicates_unchanged=True,scope='Exact rational physical original mesh and ideal midpoint. This diagnoses the represented guide coordinate; it does not authorize a point-query fallback.',
    sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (Path(__file__).resolve(),HERE/'missing_query_audit.json',CASE/'run.log',CASE/'Coarse.su2',ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py')})
(HERE/'rounding_witness.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(classification=result['classification'],pairs=len(record),witness_cells=[len(r['containing_original_edge_cells']) for r in record],roundoff=[r['coordinate_roundoff'] for r in record]),indent=2))
