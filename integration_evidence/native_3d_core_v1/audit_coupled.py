"""Independent exact embedding/facet audit and original-P1 metric checks for bounded native3D whole-mesh probes."""
from collections import defaultdict
from decimal import Decimal as D,localcontext
from fractions import Fraction as F
import itertools
import json
import math
from pathlib import Path
import sys
from audit_cavities import cross,determinant,dot,overlap,sub

def connected(graph):
    seen=set();pending=[next(iter(graph))]
    while pending:
        node=pending.pop()
        if node in seen:continue
        seen.add(node);pending.extend(graph[node]-seen)
    return len(seen)==len(graph)

def surface(triangles,closed):
    edges=defaultdict(list);vertex_links=defaultdict(lambda:defaultdict(set));adjacency={i:set() for i in range(len(triangles))}
    for i,t in enumerate(triangles):
        for a,b,c in [(t[0],t[1],t[2]),(t[1],t[2],t[0]),(t[2],t[0],t[1])]:
            edges[tuple(sorted((a,b)))].append(i)
            vertex_links[a][b].add(c);vertex_links[a][c].add(b)
    border=defaultdict(set)
    for (a,b),uses in edges.items():
        if len(uses)==2:adjacency[uses[0]].add(uses[1]);adjacency[uses[1]].add(uses[0])
        else:assert not closed and len(uses)==1;border[a].add(b);border[b].add(a)
    assert connected(adjacency)
    assert len(vertex_links)-len(edges)+len(triangles)==(2 if closed else 1)
    if closed:assert not border
    else:assert all(len(v)==2 for v in border.values()) and connected(border)
    for node,graph in vertex_links.items():
        assert connected(graph)
        degrees=[len(v) for v in graph.values()]
        assert all(x in (1,2) for x in degrees)
        assert degrees.count(1)==(2 if node in border else 0)

def parity(face):return (-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3))
def quad(m,e):
    x,y,z=e;xx,xy,xz,yy,yz,zz=m
    return xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)
def spd(m):
    xx,xy,xz,yy,yz,zz=m;d=xx*(yy*zz-yz*yz)-xy*(xy*zz-yz*xz)+xz*(xy*yz-yy*xz)
    assert xx>0 and xx*yy-xy*xy>0 and d>0
    return d

def sources(data):
    result=[];nodes={}
    for original in data['original']:
        ids=[v[0] for v in original['vertices']];points=[tuple(F(float(x)) for x in v[1:4]) for v in original['vertices']]
        metrics=[tuple(F(float(x)) for x in v[4]) for v in original['vertices']]
        assert determinant(points)>0
        for node,p,m in zip(ids,points,metrics):
            spd(m)
            if node in nodes:assert nodes[node]==(p,m)
            nodes[node]=(p,m)
        result.append(dict(key=tuple(sorted(ids)),points=points,metrics=metrics))
    result.sort(key=lambda p:p['key'])
    return result,nodes

def weights(t,p):
    volume=determinant(t);result=[]
    for i in range(4):q=t[:];q[i]=p;result.append(determinant(q)/volume)
    return result

def metric(original,p):
    for cell in original:
        w=weights(cell['points'],p)
        if min(w)>=0:return [sum(w[i]*cell['metrics'][i][j] for i in range(4)) for j in range(6)]
    raise AssertionError('Metric sample outside original donor')

def length(original,a,b):
    chord=sub(b,a)
    nodal=[m for c in original for m in c['metrics']]
    if all(m==nodal[0] for m in nodal):return math.sqrt(float(quad(nodal[0],chord)))
    intervals=[]
    for c in original:
        va,vb=weights(c['points'],a),weights(c['points'],b);lo,hi=F(0),F(1)
        for x,y in zip(va,vb):
            if x<0 and y<0:hi=F(-1);break
            if x<0:lo=max(lo,-x/(y-x))
            if y<0:hi=min(hi,x/(x-y))
        if lo>=hi:continue
        w0=[x+lo*(y-x) for x,y in zip(va,vb)];w1=[x+hi*(y-x) for x,y in zip(va,vb)]
        q=[]
        for w in (w0,w1):q.append(quad([sum(w[i]*c['metrics'][i][j] for i in range(4)) for j in range(6)],chord))
        intervals.append((lo,hi,c['key'],q))
    intervals.sort(key=lambda x:x[:3]);unique=[];cursor=F(0)
    for lo,hi,key,q in intervals:
        if unique and (lo,hi)==unique[-1][:2]:assert q==unique[-1][3];continue
        assert lo==cursor,'Uncovered/overlapping original interval'
        unique.append((lo,hi,key,q));cursor=hi
    assert cursor==1
    with localcontext() as context:
        context.prec=100;total=D(0)
        def dec(x):return D(x.numerator)/D(x.denominator)
        for lo,hi,key,q in unique:
            q0,q1=map(dec,q)
            value=q0.sqrt() if q0==q1 else D(2)/3*(q1*q1.sqrt()-q0*q0.sqrt())/(q1-q0)
            total+=dec(hi-lo)*value
        return float(total)

def frame(path,strict):
    data=json.loads(path.read_text());nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']}
    assert len(nodes)==len(data['nodes']) and len(set(nodes.values()))==len(nodes)
    cells=[c['nodes'] for c in data['cells']];assert len({c['id'] for c in data['cells']})==len(cells)
    assert len({tuple(sorted(c)) for c in cells})==len(cells)
    geometries=[[nodes[i] for i in c] for c in cells]
    volumes=[determinant(t)/6 for t in geometries];assert min(volumes)>0
    for a,b in itertools.combinations(geometries,2):assert not overlap(a,b),'Positive-volume overlap'
    uses=defaultdict(list);links=defaultdict(list)
    for a,b,c,d in cells:
        for face in [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]:uses[tuple(sorted(face))].append(parity(face))
        for node,tri in [(a,(b,c,d)),(b,(a,c,d)),(c,(a,b,d)),(d,(a,b,c))]:links[node].append(tri)
    assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in uses.values())
    boundary={f:v[0] for f,v in uses.items() if len(v)==1}
    physical={tuple(sorted(f['nodes'])):f for f in data['surface']};assert len(physical)==len(data['surface'])
    assert physical.keys()==boundary.keys()
    surface(list(boundary),True)
    boundary_nodes={i for f in boundary for i in f}
    for node,triangles in links.items():surface(triangles,node not in boundary_nodes)
    facets={f['id']:f for f in data['facets']};areas=defaultdict(F)
    for f in facets.values():
        points=[tuple(F(float(x)) for x in v[1:]) for v in f['vertices']]
        normal=cross(sub(points[1],points[0]),sub(points[2],points[0]));assert any(normal)
        for p in nodes.values():assert dot(normal,sub(p,points[0]))<=0,'Vertex outside convex reference domain'
        f['points']=points;f['normal']=normal;f['axis']=next(i for i,x in enumerate(normal) if x)
    for key,f in physical.items():
        assert parity(f['nodes'])==boundary[key]
        ref=facets[f['facet']];a,b,c=ref['points'];normal=ref['normal'];axis=ref['axis']
        for id in f['nodes']:
            p=nodes[id];assert dot(normal,sub(p,a))==0
            for v,w in [(a,b),(b,c),(c,a)]:assert dot(cross(sub(w,v),sub(p,v)),normal)>=0
        a0,b0,c0=[nodes[i] for i in f['nodes']];child=cross(sub(b0,a0),sub(c0,a0))[axis]
        assert child*normal[axis]>0;areas[f['facet']]+=child
    assert set(areas)==set(facets)
    for id,f in facets.items():assert areas[id]==f['normal'][f['axis']]
    original,source_nodes=sources(data)
    minq=minj=1.;unique_edges=set()
    for t,c in zip(geometries,cells):
        # The probe uses a rounded binary64 centroid; evaluate the exact P1 reference there.
        center=tuple(F(float(sum(p[axis] for p in t)/4)) for axis in range(3));m=metric(original,center)
        detm=spd(m);v=float(determinant(t))*math.sqrt(float(detm))
        squared={(i,j):float(quad(m,sub(t[j],t[i]))) for i,j in itertools.combinations(range(4),2)}
        q=12*(v/2)**(2/3)/sum(squared.values())
        j=min(1.,min(math.sqrt(2)*v/math.prod(math.sqrt(squared[tuple(sorted((i,k)))]) for k in range(4) if k!=i) for i in range(4)))
        assert q>=.20 and j>=.05
        minq=min(minq,q);minj=min(minj,j)
        unique_edges.update(tuple(sorted(pair)) for pair in itertools.combinations(c,2))
    maxl=0.;longest=None
    for edge in sorted(unique_edges):
        value=length(original,nodes[edge[0]],nodes[edge[1]])
        if value>maxl:maxl=value;longest=edge
    if strict:assert maxl<=1.8+1e-12,'Oversized final original-field edge'
    marker_counts=defaultdict(int)
    for f in physical.values():marker_counts[facets[f['facet']]['marker']]+=1
    # Export with actual preserved physical markers; dependencies are all local regular files.
    ids=sorted(nodes);local={id:i for i,id in enumerate(ids)}
    lines=['NDIME= 3','NELEM= '+str(len(cells))]
    lines.extend('10 '+' '.join(str(local[i]) for i in c)+' '+str(j) for j,c in enumerate(cells))
    lines.append('NPOIN= '+str(len(ids)))
    lines.extend(' '.join(format(float(x),'.17g') for x in nodes[id])+' '+str(local[id]) for id in ids)
    lines.append('NMARK= '+str(len(marker_counts)))
    for marker in sorted(marker_counts):
        faces=[f['nodes'] for f in physical.values() if facets[f['facet']]['marker']==marker]
        lines.extend(['MARKER_TAG= side_'+str(marker),'MARKER_ELEMS= '+str(len(faces))])
        lines.extend('5 '+' '.join(str(local[id]) for id in f) for f in faces)
    path.with_suffix('.su2').write_text('\n'.join(lines)+'\n')
    return dict(file=path.name,cells=len(cells),points=len(nodes),physical_faces=len(physical),volume=str(sum(volumes)),
                minimum_mean_ratio=minq,minimum_scaled_jacobian=minj,maximum_length=maxl,longest_edge=longest,
                markers=dict(marker_counts),strict_metric_gate=strict),data

def main(folder):
    records=[]
    for case in json.loads((folder/'probe_summary.json').read_text()):
        name=case['case'];initial,a=frame(folder/(name+'_initial.json'),False)
        adapted,b=frame(folder/(name+'_adapted.json'),True);coarsened,c=frame(folder/(name+'_coarsened.json'),True)
        assert initial['volume']==adapted['volume']==coarsened['volume']
        assert initial['maximum_length']>1.8
        assert {tuple(sorted(x['nodes'])) for x in a['cells']}=={tuple(sorted(x['nodes'])) for x in c['cells']}
        assert {(tuple(f['nodes']),f['facet']) for f in a['surface']}=={(tuple(f['nodes']),f['facet']) for f in c['surface']}
        assert {tuple(sorted(v[0] for v in d['vertices'])) for d in c['original']}=={tuple(sorted(x['nodes'])) for x in b['cells']}
        assert adapted['cells']==case['adapted_cells'] and adapted['points']==case['adapted_points']
        assert abs(adapted['minimum_mean_ratio']-case['final_q'])<=1e-8
        assert abs(adapted['minimum_scaled_jacobian']-case['final_J'])<=1e-8
        assert abs(adapted['maximum_length']-case['final_L'])<=1e-10
        phases=[case[key] for key in ('field_build_seconds','selection_seconds','geometry_seconds','metric_seconds','commit_seconds','final_gate_seconds','output_seconds','other_seconds')]
        assert min(phases)>=0 and abs(sum(phases)-case['parent_seconds'])<=1e-10
        records.extend([initial,adapted,coarsened])
        print('AUDITED',name,'6 ->',adapted['cells'],'-> 6 tetrahedra; facets and six markers preserved',flush=True)
    (folder/'independent_audit.json').write_text(json.dumps(dict(status='PASS',frames=records,
      scope='Exact positive volumes, global tetrahedron SAT no-overlap, convex-reference containment, complete oriented physical skin, sphere/disk links, original facet coverage/markers, original-P1 metric shape and integrated edge gates, accepted refinement/coarsening reversal. Small serial manufactured fields, no CFD solution/history, MPI, CGNS or scalability certificate.'),indent=2)+'\n')
if __name__=='__main__':main(Path(sys.argv[1]).resolve())
