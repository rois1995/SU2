"""Independent small-mesh audit using exact binary64 rational geometry; no native predicate imports."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib
import itertools
import json
import math
from pathlib import Path
import sys


def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def determinant(p):return dot(sub(p[1],p[0]),cross(sub(p[2],p[0]),sub(p[3],p[0])))


def overlap(a,b):
    """Exact separating-axis test for positive-volume intersection of two full-dimensional tetrahedra."""
    def axes(t):
        return [cross(sub(t[j],t[i]),sub(t[k],t[i])) for i,j,k in itertools.combinations(range(4),3)]
    ea=[sub(a[j],a[i]) for i,j in itertools.combinations(range(4),2)]
    eb=[sub(b[j],b[i]) for i,j in itertools.combinations(range(4),2)]
    for axis in axes(a)+axes(b)+[cross(x,y) for x in ea for y in eb]:
        if not any(axis):continue
        pa=[dot(axis,p) for p in a];pb=[dot(axis,p) for p in b]
        if max(pa)<=min(pb) or max(pb)<=min(pa):return False
    return True


def read(path):
    data=json.loads(path.read_text())
    ids=[n[0] for n in data['nodes']]
    assert len(ids)==len(set(ids))
    nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']}
    assert len(set(nodes.values()))==len(nodes)
    cells=[c['nodes'] for c in data['cells']]
    assert len({c['id'] for c in data['cells']})==len(cells)
    assert len({tuple(sorted(c)) for c in cells})==len(cells)
    geometry=[[nodes[i] for i in c] for c in cells]
    volumes=[determinant(t)/6 for t in geometry]
    assert all(v>0 for v in volumes)
    for a,b in itertools.combinations(geometry,2):assert not overlap(a,b),'positive-volume overlap'
    uses=defaultdict(list)
    for a,b,c,d in cells:
        for face in [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]:
            parity=(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3))
            uses[tuple(sorted(face))].append(parity)
    assert all(len(x)==1 or len(x)==2 and sum(x)==0 for x in uses.values())
    boundary={key:value[0] for key,value in uses.items() if len(value)==1}
    m=list(map(F,data['metric']))
    xx,xy,xz,yy,yz,zz=m
    det=xx*(yy*zz-yz*yz)-xy*(xy*zz-yz*xz)+xz*(xy*yz-yy*xz)
    assert xx>0 and xx*yy-xy*xy>0 and det>0
    def quadratic(e):
        x,y,z=e
        return xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)
    measures=[]
    for t,v,record in zip(geometry,volumes,data['cells']):
        lengths={(i,j):math.sqrt(float(quadratic(sub(t[j],t[i])))) for i,j in itertools.combinations(range(4),2)}
        dv=6*float(v)*math.sqrt(float(det))
        sum2=sum(x*x for x in lengths.values())
        q=12*(dv/2)**(2/3)/sum2
        jac=min(1.,min(math.sqrt(2)*dv/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if j!=i) for i in range(4)))
        current=[q,jac,math.sqrt(sum2/6),max(lengths.values())]
        assert all(abs(a-b)<=1e-8*max(1,abs(a),abs(b)) for a,b in zip(current,record['measures']))
        assert q>=.20 and jac>=.05 and current[3]<=1.8
        measures.append(current)
    return data,nodes,boundary,sum(volumes),measures


def export(path,data,boundary):
    ids=[n[0] for n in data['nodes']];local={v:i for i,v in enumerate(ids)}
    lines=['NDIME= 3','NELEM= '+str(len(data['cells']))]
    lines.extend('10 '+' '.join(str(local[i]) for i in c['nodes'])+' '+str(j) for j,c in enumerate(data['cells']))
    lines.append('NPOIN= '+str(len(ids)))
    lines.extend(' '.join(format(float(x),'.17g') for x in n[1:])+' '+str(j) for j,n in enumerate(data['nodes']))
    lines.extend(['NMARK= 1','MARKER_TAG= private_interface','MARKER_ELEMS= '+str(len(boundary))])
    for face,sign in sorted(boundary.items()):
        if sign<0:face=(face[0],face[2],face[1])
        lines.append('5 '+' '.join(str(local[i]) for i in face))
    path.write_text('\n'.join(lines)+'\n')


def main(folder):
    summary=[]
    for before in sorted(folder.glob('*_initial.json')):
        name=before.name[:-len('_initial.json')]
        after=folder/(name+'_candidate.json')
        a,pa,sa,va,_=read(before);b,pb,sb,vb,measures=read(after)
        assert a['metric']==b['metric'] and sa==sb and va==vb
        assert all(pa[i]==pb[i] for face in sa for i in face)
        for path,data,skin in [(before,a,sa),(after,b,sb)]:export(path.with_suffix('.su2'),data,skin)
        summary.append(dict(case=name,initial_cells=len(a['cells']),candidate_cells=len(b['cells']),
            exact_volume=str(va),skin_faces=len(sa),qmin=min(m[0] for m in measures),
            minimum_scaled_jacobian=min(m[1] for m in measures),maximum_metric_edge=max(m[3] for m in measures)))
    assert len(summary)==5
    # Negative controls prove that geometry and skin checks are exercised.
    p=[(F(0),F(0),F(0)),(F(1),F(0),F(0)),(F(0),F(1),F(0)),(F(0),F(0),F(1))]
    assert overlap(p,p)
    assert overlap(p,[tuple(x+F(1,16) for x in row) for row in p])
    assert not overlap(p,[(x+2,y,z) for x,y,z in p])
    assert not overlap(p,[p[0],p[2],p[1],(F(0),F(0),F(-1))])
    result=dict(status='PASS',cases=summary,scope='Five private fixed-interface geometric candidates; exact positive volumes, pairwise SAT interior-disjointness, oriented skin/coordinate and exact volume equality. Constant tensor checks only; no full driver, physical surface adaptation, spatial metrics, donor transfer or MPI qualification.')
    result['mesh_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(folder.glob('*.su2'))}
    (folder/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':main(Path(sys.argv[1]).resolve())
