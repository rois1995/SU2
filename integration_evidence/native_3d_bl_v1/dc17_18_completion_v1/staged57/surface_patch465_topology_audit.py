"""Reconstruct saved split topology independently; no geometry or acceptance inference."""
from pathlib import Path
import collections,itertools,json,struct
R=Path(__file__).resolve().parent
prior=json.loads((R/'surface_strip464_assessment.json').read_text())
case=R/'cases/curved451_strip_reuse_saved_target_surface_mpi6_20261112T185700_120424'
cells=[]
for rank in range(6):
    with (case/f'stage_surface_rank{rank}_owned.bin').open('rb') as f:
        n=struct.unpack('<Q',f.read(8))[0]
        for _ in range(n):
            row=f.read(416);assert len(row)==416
            cells.append(tuple(struct.unpack_from('<Q',row,16+32*k)[0] for k in range(4)))
        assert not f.read(1)
assert len(cells)==303622
first=max(v for c in cells for v in c)+1
for k,(u,v) in enumerate(((4218,8070),(4218,18565),(4219,8070),(4219,18565))):
    rebuilt=[]
    for c in cells:
        if u in c and v in c:
            rebuilt.extend(tuple(first+k if x==a else x for x in c) for a in (u,v))
        else:rebuilt.append(c)
    cells=rebuilt
edges={tuple(sorted((f['vertices'][k][0],f['vertices'][(k+1)%3][0]))) for f in prior['faces'] for k in range(3)}
support=[c for c in cells if any(u in c and v in c for u,v in edges)]
def surface(faces):
    edgeowners=collections.defaultdict(list)
    for k,f in enumerate(faces):
        for e in itertools.combinations(f,2):edgeowners[tuple(sorted(e))].append(k)
    graph=[set() for _ in faces]
    for owners in edgeowners.values():
        for a,b in itertools.combinations(owners,2):graph[a].add(b);graph[b].add(a)
    todo=set(range(len(faces)));components=0
    while todo:
        stack=[todo.pop()];components+=1
        while stack:
            for q in graph[stack.pop()]:
                if q in todo:todo.remove(q);stack.append(q)
    vertices={v for f in faces for v in f}
    return dict(vertices=len(vertices),edges=len(edgeowners),faces=len(faces),euler=len(vertices)-len(edgeowners)+len(faces),
                face_components=components,edge_multiplicities=dict(collections.Counter(map(len,edgeowners.values()))))
def ball(cs):
    faces=collections.Counter(tuple(sorted(f)) for c in cs for f in itertools.combinations(c,3))
    boundary=[f for f,n in faces.items() if n==1];bv={v for f in boundary for v in f};badlinks=[]
    for v in {v for c in cs for v in c}:
        link=surface([tuple(x for x in c if x!=v) for c in cs if v in c]);expected=1 if v in bv else 2
        if link['euler']!=expected or link['face_components']!=1:badlinks.append(dict(vertex=v,expected_euler=expected,**link))
    return dict(cells=len(cs),boundary=surface(boundary),bad_vertex_links=badlinks)
operations=json.loads((R/'surface_patch465_assessment.json').read_text())['operations']
local=[]
for row in operations:
    u,v=row['edge'];local.append(dict(edge=[u,v],**ball([c for c in support if u in c and v in c])))
report=dict(whole_original_support=ball(support),local_original_edge_stars=local,
    limits=['Combinatorial replay only, no replacement geometry or native topology-gate bypass.',
            'Passing Euler/connectivity tests alone does not prove a manifold ball.'])
(R/'surface_patch465_topology_audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
