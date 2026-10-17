"""Diagnose the sealed64-parent private mesh; approximate shape audit, never acceptance."""
from pathlib import Path
import hashlib,json
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
BASE=HERE.parent/'deferred_first_row_lengths_v1/frozen'
traces=[BASE/('query_trace_rank%d.csv'%r) for r in range(4)]
field={}
for path in traces:
    for row in np.loadtxt(path,delimiter=','):
        key=tuple(row[:3]);m=row[9:];matrix=np.array([[m[0],m[1],m[2]],[m[1],m[3],m[4]],[m[2],m[4],m[5]]],dtype=np.longdouble)
        if key in field:assert np.array_equal(field[key],matrix)
        field[key]=matrix
rows=(BASE/'first_row_rejected.dat').read_text().splitlines();assert int(rows[0])==len(rows)-1
results=[];unobserved=0
for line in rows[1:]:
    row=line.split();assert len(row)==25
    vertices=np.array([[float(row[2+4*k+j]) for j in range(3)] for k in range(4)],dtype=np.longdouble)
    center=np.zeros(3,dtype=np.longdouble)
    for vertex in vertices:center+=vertex/4
    matrix=field.get(tuple(center.astype(np.float64)))
    if matrix is None:unobserved+=1;continue
    def det(m):return m[0,0]*(m[1,1]*m[2,2]-m[1,2]*m[2,1])-m[0,1]*(m[1,0]*m[2,2]-m[1,2]*m[2,0])+m[0,2]*(m[1,0]*m[2,1]-m[1,1]*m[2,0])
    volume=det(vertices[1:]-vertices[0])*np.sqrt(det(matrix))
    assert volume>0
    squared={(i,j):np.dot(vertices[j]-vertices[i],matrix.dot(vertices[j]-vertices[i])) for i in range(4) for j in range(i+1,4)}
    q=12*(volume/2)**(np.longdouble(2)/3)/sum(squared.values())
    jac=min(np.sqrt(np.longdouble(2))*volume/np.prod([np.sqrt(squared[tuple(sorted((i,j)))]) for j in range(4) if i!=j]) for i in range(4))
    results.append(dict(cell=int(row[0]),q=float(q),J=float(min(1,jac))))
summary=dict(observed_cells=len(results),unobserved_cells=unobserved,
    shape_failures=sum(r['q']<.2 or r['J']<.05 for r in results),
    minimum_q=min(r['q'] for r in results),minimum_J=min(r['J'] for r in results),
    scope='Frozen64-parent PRIVATE mesh; long-double shape diagnostic with sealed actual-query target tensors. No full edge/mesh acceptance or full-wing extrapolation.',
    sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in traces+[BASE/'first_row_rejected.dat',Path(__file__).resolve()]},cells=results)
(HERE/'seed_shape_audit.json').write_text(json.dumps(summary,indent=2,allow_nan=False)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k not in ('cells','sha256')},indent=2))
