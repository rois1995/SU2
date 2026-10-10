"""Independent embedding/geometry/P1 checks of actual MPI publication and subset-return records."""
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sys
from audit_coupled import frame

log, destination = map(lambda x: Path(x).resolve(), sys.argv[1:])
print('Case working and retained folder:', destination, flush=True)
destination.mkdir(exist_ok=False)
groups = defaultdict(list)
for line in log.read_text().splitlines():
    if not line.startswith('MESH '): continue
    parts = line.split()
    assert len(parts) == 28, 'Partial MPI mesh record'
    _, label, id, version = parts[:4]
    vertices = [[int(parts[4+4*i])]+list(map(float, parts[5+4*i:8+4*i])) for i in range(4)]
    physical = [[int(parts[20+2*i]), int(parts[21+2*i])] for i in range(4)]
    groups[label].append(dict(id=int(id), version=int(version), vertices=vertices, physical=physical))
assert {'coupled_initial', 'coupled_refined', 'coupled_coarsened'} <= groups.keys()
facets = {}
def faces(v):
    a,b,c,d = v
    return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
for row in groups['coupled_initial']:
    for face, (present, facet) in zip(faces(row['vertices']), row['physical']):
        if present:
            assert facet not in facets
            facets[facet] = dict(id=facet, marker=10+facet, vertices=face)
assert len(facets) == 12
records = []
for label, rows in sorted(groups.items()):
    assert len(rows) == (6 if label in ('coupled_initial', 'coupled_coarsened') else 12)
    nodes = {}; cells = []; physical = []
    for row in rows:
        cells.append(dict(id=row['id'], nodes=[v[0] for v in row['vertices']]))
        assert row['version'] == 17+row['id']
        for vertex in row['vertices']:
            if vertex[0] in nodes: assert nodes[vertex[0]] == vertex
            nodes[vertex[0]] = vertex
        for face, (present, facet) in zip(faces(row['vertices']), row['physical']):
            if present: physical.append(dict(facet=facet, nodes=[v[0] for v in face]))
    coarse = label == 'coupled_coarsened'
    original = groups['coupled_refined' if coarse else 'coupled_initial']
    tensor = [1,0,0,1,0,1] if coarse else [4,0,0,4,0,4]
    data = dict(nodes=[nodes[id] for id in sorted(nodes)], cells=cells, surface=physical,
                facets=[facets[id] for id in sorted(facets)],
                original=[dict(vertices=[v+[tensor] for v in row['vertices']]) for row in original])
    path = destination/(label+'.json')
    path.write_text(json.dumps(data, indent=2)+'\n')
    measured, _ = frame(path, coarse)
    assert measured['volume'] == '1'
    if label.startswith('returned_m'): assert rows == groups['coupled_refined'], 'Subset migration changed accepted cell records'
    records.append(measured)
result = dict(status='PASS', frames=records, source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
    scope='Exact positive volumes, global no-overlap, finite reference facet coverage/marker/embedding/manifold checks and original-P1 metric shape/edge measurement of actual MPI records. Coarsened grid meets strict length gate; one-step refined/returned grids are private progress with oversized original-field edges. No complete adaptation sweep, CFD state, CGNS or scaling qualification.')
result['artifacts_sha256'] = {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
(destination/'independent_audit.json').write_text(json.dumps(result, indent=2)+'\n')
print('PASS', len(records), 'independently audited MPI mesh frames; SU2 grids retained', flush=True)
