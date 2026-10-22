#!/usr/bin/env python3
"""DC-7 test-only M6 neighbourhood cuts (root TE, tip/TE corner, LE) with the real selected 500k sensor.

Adoption (as wp0b/prepare_m6.py): SYMMETRY vertices with |y| <= 1e-10 are snapped to the declared Y = 0 plane; every
affected cut tetrahedron must keep a positive signed volume. The cut is the face-connected set of original cells with
all vertices within a radius of the centre, the largest radius keeping <= 1200 cells and a closed manifold skin;
the real selected sensor of the centre node is frozen uniform on covering donors (FrozenField bound 256 donors). The cut skin is an artificial interface; physical faces
keep their FARFIELD/SYMMETRY/WING3D markers. Inputs: Coarse.su2, the 2-rank export selected_rank*.bin and field.json
of the [NativeFirstRowDC7Field] run (ADAP_ANGLE 60, ADAP_COMPLEXITY 500000)."""
import hashlib, json, struct, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
FIELD = HERE / 'm6_field'
MESH = (FIELD / 'Coarse.su2').resolve()
MAX_CELLS = 1200  # cut size (the sensor is frozen uniform at the centre: no 256-donor bound)

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def read_su2(path):
    lines = Path(path).read_text().splitlines()
    at, cells, points, markers = 0, [], [], {}
    while at < len(lines):
        s = lines[at].split('%')[0].strip()
        if s.startswith('NELEM='):
            n = int(s.split('=')[1]); at += 1
            for k in range(n):
                t = lines[at + k].split(); assert t[0] == '10'; cells.append(tuple(map(int, t[1:5])))
            at += n; continue
        if s.startswith('NPOIN='):
            n = int(s.split('=')[1].split()[0]); at += 1
            for k in range(n):
                t = lines[at + k].split(); points.append([float(x) for x in t[:3]])
            at += n; continue
        if s.startswith('MARKER_TAG='):
            name = s.split('=')[1].strip(); n = int(lines[at + 1].split('=')[1]); at += 2
            markers[name] = [tuple(map(int, lines[at + k].split()[1:4])) for k in range(n)]; at += n; continue
        at += 1
    return cells, points, markers

def det(a, b, c, d):
    p = [[b[t] - a[t] for t in range(3)], [c[t] - a[t] for t in range(3)], [d[t] - a[t] for t in range(3)]]
    return (p[0][0] * (p[1][1] * p[2][2] - p[1][2] * p[2][1]) - p[0][1] * (p[1][0] * p[2][2] - p[1][2] * p[2][0])
            + p[0][2] * (p[1][0] * p[2][1] - p[1][1] * p[2][0]))

def main():
    cells, P, markers = read_su2(MESH)
    field = json.loads((FIELD / 'field.json').read_text())
    sensor = {}
    for f in sorted(FIELD.glob('selected_rank*.bin')):
        raw = f.read_bytes(); n = struct.unpack_from('<Q', raw, 0)[0]; off = 8
        for _ in range(n):
            gid = struct.unpack_from('<Q', raw, off)[0]; off += 8
            x = struct.unpack_from('<3d', raw, off); off += 24
            m = struct.unpack_from('<9d', raw, off); off += 72
            assert list(x) == P[gid], ('coordinate mismatch', gid)
            sensor[gid] = m
    assert len(sensor) == len(P), (len(sensor), len(P))
    names = ['FARFIELD', 'SYMMETRY', 'WING3D']
    face_marker = {}
    for k, name in enumerate(names):
        for f in markers[name]:
            face_marker[tuple(sorted(f))] = k
    before = [det(*[P[v] for v in c]) for c in cells]
    sym = {v for f in markers['SYMMETRY'] for v in f}
    snapped = 0
    for v in sym:
        if abs(P[v][1]) <= 1e-10 and P[v][1] != 0.0:
            P[v][1] = 0.0; snapped += 1
    flipped = sum(1 for c, d0 in zip(cells, before) if (det(*[P[v] for v in c]) > 0) != (d0 > 0) or det(*[P[v] for v in c]) == 0)
    assert flipped == 0, flipped
    wall = {v for f in markers['WING3D'] for v in f}
    ymax = max(P[v][1] for v in wall)
    centres = {
        'm6_root_te': max((v for v in wall if P[v][1] == 0.0), key=lambda v: P[v][0]),
        'm6_tip_te': max(wall, key=lambda v: P[v][0]),  # most aft wall point = TE at the tip (swept TE)
        'm6_le': min((v for v in wall if abs(P[v][1] - 0.5 * ymax) < 0.02 * ymax), key=lambda v: P[v][0]),
    }
    vertex_cells = {}
    for i, c in enumerate(cells):
        for v in c:
            vertex_cells.setdefault(v, []).append(i)
    def faces_of(c):
        return [tuple(sorted((c[1], c[2], c[3]))), tuple(sorted((c[0], c[2], c[3]))),
                tuple(sorted((c[0], c[1], c[3]))), tuple(sorted((c[0], c[1], c[2])))]
    def manifold(comp):
        # closed 2-manifold skin: every skin edge in exactly two skin faces, every vertex link one cycle
        cnt = {}
        for i in comp:
            for f in faces_of(cells[i]):
                cnt[f] = cnt.get(f, 0) + 1
        skin = [f for f, n in cnt.items() if n == 1]
        edges = {}
        for f in skin:
            for a, b in ((f[0], f[1]), (f[1], f[2]), (f[0], f[2])):
                edges[(a, b)] = edges.get((a, b), 0) + 1
        if any(n != 2 for n in edges.values()):
            return False
        link = {}
        for f in skin:
            for k in range(3):
                v, a, b = f[k], f[(k + 1) % 3], f[(k + 2) % 3]
                link.setdefault(v, {}).setdefault(a, []).append(b); link[v].setdefault(b, []).append(a)
        for v, g in link.items():
            start = next(iter(g)); seen, stack = {start}, [start]
            while stack:
                x = stack.pop()
                for y in g[x]:
                    if y not in seen:
                        seen.add(y); stack.append(y)
            if len(seen) != len(g):
                return False
        return True
    out = {'mesh_sha256': sha(MESH), 'snapped_symmetry_vertices': snapped, 'flipped_cells': flipped, 'field': field, 'cuts': {}}
    for name, centre in centres.items():
        c0 = P[centre]
        def dist(v):
            return sum((P[v][t] - c0[t]) ** 2 for t in range(3)) ** 0.5
        near = sorted(range(len(cells)), key=lambda i: max(dist(v) for v in cells[i]))
        radii = [max(dist(v) for v in cells[i]) for i in near]
        best = None
        # grow by radius order, keep the face-connected component of the centre; largest prefix with <= MAX_CELLS
        chosen = []
        k = 0
        while k < len(near):
            r = radii[k]
            j = k
            while j < len(near) and radii[j] <= r:
                j += 1
            trial = set(near[:j])
            # component
            seeds = [i for i in vertex_cells[centre] if i in trial]
            if not seeds:
                k = j; continue
            adj = {}
            for i in trial:
                for f in faces_of(cells[i]):
                    adj.setdefault(f, []).append(i)
            comp, stack = set(seeds), list(seeds)
            while stack:
                i = stack.pop()
                for f in faces_of(cells[i]):
                    for o in adj[f]:
                        if o not in comp:
                            comp.add(o); stack.append(o)
            if len(comp) > MAX_CELLS:
                break
            if manifold(comp):
                chosen = sorted(comp); best = r
            k = j
        assert chosen, name
        nodes = sorted({v for i in chosen for v in cells[i]})
        count = {}
        for i in chosen:
            for f in faces_of(cells[i]):
                count[f] = count.get(f, 0) + 1
        # the artificial cut skin is declared as marker CUT (closed authority required by the mapped reference);
        # wall vertices on CUT faces are the fixed rim of the neighbourhood (never lifted, faces reported separately)
        facets = [(f, face_marker.get(f, 3)) for f, n in count.items() if n == 1]
        nmark = {(names + ['CUT'])[k]: sum(1 for _, m in facets if m == k) for k in range(4)}
        txt = [f'SU2_DC7_M6 1', f'name {name}', f'cfg {(FIELD / "run_serial.cfg").resolve()}', f'ref {(FIELD / "Coarse.su2.native_ref").resolve()}',
               f'parameter {field["parameter"]!r}', f'centre {centre}', f'core {field["sensor_core_min_eigenvalue"]!r}', 'markers 4 ' + ' '.join(names) + ' CUT', 'walls 1 2',
               f'nodes {len(nodes)}']
        for v in nodes:
            m = sensor[v]
            txt.append(f'{v} {P[v][0]!r} {P[v][1]!r} {P[v][2]!r} {m[0]!r} {m[1]!r} {m[2]!r} {m[4]!r} {m[5]!r} {m[8]!r}')
        txt.append(f'cells {len(chosen)}')
        for i in chosen:
            txt.append(f'{i} ' + ' '.join(map(str, cells[i])))
        txt.append(f'facets {len(facets)}')
        for k, (f, m) in enumerate(sorted(facets)):
            txt.append(f'{k} {m} {f[0]} {f[1]} {f[2]}')
        txt.append('end')
        path = HERE / 'm6_cuts' / f'{name}.txt'; path.parent.mkdir(exist_ok=True)
        path.write_text('\n'.join(txt) + '\n')
        out['cuts'][name] = {'centre': centre, 'centre_xyz': c0, 'radius': best, 'cells': len(chosen), 'nodes': len(nodes),
                             'physical_faces': nmark, 'sha256': sha(path)}
        print(name, out['cuts'][name])
    out['script_sha256'] = sha(__file__)
    (HERE / 'm6_cuts' / 'prepare.json').write_text(json.dumps(out, indent=1))

if __name__ == '__main__':
    main()
