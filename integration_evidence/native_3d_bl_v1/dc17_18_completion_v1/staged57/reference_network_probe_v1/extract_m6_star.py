"""Extract one complete faceted M6 wing star; no curved or adaptation claim."""
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
MESH = HERE.parent / 'cases/m6_stage_20261027T145416_2560816/m6.su2'
PARSER = ROOT / 'TestCases/adaptation/capability/capcheck.py'
def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''): h.update(chunk)
    return h.hexdigest()

before = {str(p): sha(p) for p in (MESH, PARSER, Path(__file__).resolve())}
spec = importlib.util.spec_from_file_location('capcheck', PARSER)
capcheck = importlib.util.module_from_spec(spec); spec.loader.exec_module(capcheck)
mesh = capcheck.read_su2(MESH); faces = mesh.M['WING3D']; points = mesh.P
edges = {}; incident = {}
for i, face in enumerate(faces):
    for v in face: incident.setdefault(int(v), []).append(i)
    for a, b in zip(face, np.roll(face, -1)):
        edges.setdefault(tuple(sorted((int(a), int(b)))), []).append(i)
normals = np.cross(points[faces[:, 1]] - points[faces[:, 0]], points[faces[:, 2]] - points[faces[:, 0]])
lengths = np.linalg.norm(normals, axis=1)
if not np.all(lengths > 0): raise ValueError('degenerate wing face')
normals /= lengths[:, None]
turn = {edge: math.degrees(math.acos(float(np.clip(np.dot(normals[rows[0]], normals[rows[1]]), -1, 1))))
        for edge, rows in edges.items() if len(rows) == 2}
wing = np.unique(faces); ymin, ymax = float(points[wing, 1].min()), float(points[wing, 1].max())
lo, hi = ymin + .3*(ymax-ymin), ymin + .7*(ymax-ymin)
mid = wing[(points[wing, 1] >= lo) & (points[wing, 1] <= hi)]
x20 = float(np.quantile(points[mid, 0], .2))
candidates = []
for v in mid:
    v = int(v)
    if points[v, 0] > x20: continue
    rows = incident[v]; successor = {}; radial = []
    for i in rows:
        face = list(map(int, faces[i])); k = face.index(v); a, b = face[(k+1)%3], face[(k+2)%3]
        if a in successor: break
        successor[a] = b
        radial.extend((tuple(sorted((v,a))), tuple(sorted((v,b)))))
    else:
        if len(successor) < 4 or len(set(successor.values())) != len(successor): continue
        ring = [min(successor)]
        while successor[ring[-1]] != ring[0] and len(ring) <= len(successor):
            nxt = successor[ring[-1]]
            if nxt not in successor or nxt in ring: break
            ring.append(nxt)
        if len(ring) != len(successor) or successor[ring[-1]] != ring[0]: continue
        radial = sorted(set(radial))
        if any(len(edges[e]) != 2 or turn[e] > 45 for e in radial): continue
        eligible = [e for e in radial if 10 <= turn[e] <= 35]
        if not eligible: continue
        # Prefer the front of the midspan distribution, then stronger eligible turn and ID.
        best = min(eligible, key=lambda e: (-turn[e], e))
        candidates.append((float(points[v,0]), -turn[best], v, ring, rows, best))
if not candidates: raise RuntimeError('No vertex meets the specified midspan, front-quantile, manifold and turn criteria')
_, neg_turn, center, ring, rows, edge = min(candidates)
lookup = {center: 0, **{v:i+1 for i,v in enumerate(ring)}}
ordered = [center] + ring
out = HERE / 'm6_faceted_star.txt'; receipt = HERE / 'm6_faceted_star.json'
text = f'{len(ordered)} {len(ring)}\n' + ''.join(' '.join(format(float(x), '.17g') for x in points[v])+'\n' for v in ordered)
text += ''.join(f'{lookup[ring[i]]} 0 {lookup[ring[(i+1)%len(ring)]]}\n' for i in range(len(ring)))
after = {str(p): sha(p) for p in (MESH, PARSER, Path(__file__).resolve())}
if before != after: raise RuntimeError('input, parser or extractor changed during extraction')
out.write_text(text)
receipt.write_text(json.dumps({'scope':'faceted M6 geometry patch only; no cubic, BL or accepted-mesh claim',
    'sha256':before, 'output_sha256':sha(out), 'original_center':center, 'original_ring':ring,
    'wing_facet_indices':rows, 'selected_radial_edge':edge, 'selected_turn_degrees':-neg_turn,
    'criteria':{'span_fraction':[.3,.7], 'span_y_bounds':[lo,hi], 'front_x_quantile':.2, 'front_x_threshold':x20,
                'eligible_turn_degrees':[10,35], 'maximum_radial_turn_degrees':45, 'minimum_ring_vertices':4},
    'counts':{'vertices':len(ordered),'faces':len(ring),'candidate_centers':len(candidates)}},indent=2)+'\n')
print(json.dumps({'center':center,'ring':len(ring),'turn':-neg_turn,'candidates':len(candidates)}))
