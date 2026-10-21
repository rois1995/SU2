#!/usr/bin/env python3
"""WP-E test-only M6 adopted-reference preparation for WP-0(b) (no CFD, no field, no change to the original mesh).

Adoption: SYMMETRY vertices are snapped to the declared Y=0 plane (|y| <= 1e-10, as the planar reference audit);
every affected tetrahedron must keep a positive signed volume. This is a fresh initial reference prepared before any
field exists (the archived off-plane v1 reference/field is not reused). Two root neighbourhoods (root trailing edge
and root leading edge) are extracted: all cells with every vertex within the radius, grown until at least the
requested cell count. Their cut skin is an artificial interface; physical faces keep WING3D/SYMMETRY/FARFIELD markers.
Two isotropic proxy sensors bracket h_t/h0 = 1e3..1e4 for h0 = 2e-6 (sizes 2e-3 and 2e-2)."""
import hashlib, json, math, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
MESH = (HERE / '../../oneram6_coarse_v1/Coarse.su2').resolve()

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def read_su2(path):
    lines = Path(path).read_text().splitlines()
    at = 0
    cells, points, markers = [], [], {}
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
                t = lines[at + k].split(); points.append(tuple(map(float, t[:3])))
            at += n; continue
        if s.startswith('MARKER_TAG='):
            name = s.split('=')[1].strip(); n = int(lines[at + 1].split('=')[1]); at += 2
            faces = []
            for k in range(n):
                t = lines[at + k].split(); assert t[0] == '5'; faces.append(tuple(map(int, t[1:4])))
            markers[name] = faces; at += n; continue
        at += 1
    return cells, points, markers

def det(a, b, c, d):
    p = [[b[t] - a[t] for t in range(3)], [c[t] - a[t] for t in range(3)], [d[t] - a[t] for t in range(3)]]
    return (p[0][0] * (p[1][1] * p[2][2] - p[1][2] * p[2][1]) - p[0][1] * (p[1][0] * p[2][2] - p[1][2] * p[2][0])
            + p[0][2] * (p[1][0] * p[2][1] - p[1][1] * p[2][0]))

def main():
    cells, points, markers = read_su2(MESH)
    sym = {n for f in markers['SYMMETRY'] for n in f}
    deviation = max(abs(points[n][1]) for n in sym)
    assert deviation <= 1e-10, deviation
    adopted = list(points)
    for n in sym:
        adopted[n] = (points[n][0], 0.0, points[n][2])
    flipped, ratio_lo, ratio_hi, affected = 0, math.inf, -math.inf, 0
    for c in cells:
        if any(n in sym for n in c):
            a = det(*[points[n] for n in c]); b = det(*[adopted[n] for n in c]); affected += 1
            if a * b <= 0: flipped += 1
            else: ratio_lo = min(ratio_lo, b / a); ratio_hi = max(ratio_hi, b / a)
    assert flipped == 0
    wing = {n for f in markers['WING3D'] for n in f}
    root = sorted(wing & sym, key=lambda n: adopted[n][0])
    centers = {'root_te': root[-1], 'root_le': root[0]}
    face_marker = {}
    names = ['FARFIELD', 'SYMMETRY', 'WING3D']
    for k, name in enumerate(names):
        for f in markers[name]:
            face_marker[tuple(sorted(f))] = (k, f)
    summary = {'mesh': str(MESH), 'mesh_sha256': sha(MESH), 'symmetry_vertices': len(sym), 'maximum_snap': deviation,
               'affected_cells': affected, 'orientation_flips': flipped, 'volume_ratio': [ratio_lo, ratio_hi],
               'root_vertices': len(root), 'cases': {}}
    for label, center in centers.items():
        cx = adopted[center]
        radius = .002
        while True:
            near = {n for n, p in enumerate(adopted) if math.dist(p, cx) <= radius}
            chosen = [i for i, c in enumerate(cells) if all(n in near for n in c)]
            if len(chosen) >= 1500 or radius > .2:
                break
            radius *= 1.25
        used = sorted({n for i in chosen for n in cells[i]})
        faces = {}
        for i in chosen:
            c = cells[i]
            for f in [(c[1], c[2], c[3]), (c[0], c[3], c[2]), (c[0], c[1], c[3]), (c[0], c[2], c[1])]:
                faces.setdefault(tuple(sorted(f)), []).append(f)
        facets = [(face_marker[k][0], face_marker[k][1]) for k, uses in faces.items() if len(uses) == 1 and k in face_marker]
        wall_edges = [math.dist(adopted[f[a]], adopted[f[(a + 1) % 3]]) for mk, f in facets if mk == 2 for a in range(3)]
        wall_edges.sort()
        for size in (2e-3, 2e-2):
            m = 1 / size ** 2
            name = f'm6_{label}_h{size:g}'
            lines = ['SU2_JUNCTION_CASE 1', f'name {name}', f'markers {len(names)}'] + names
            lines += [f'policy {2e-6!r} {1.2!r} {.01!r}', 'walls 1 2', f'core {m!r}', f'sensor {m!r} 0 0 {m!r} 0 {m!r}']
            lines.append(f'nodes {len(used)}')
            lines += [f'{n} {adopted[n][0]!r} {adopted[n][1]!r} {adopted[n][2]!r}' for n in used]
            lines.append(f'cells {len(chosen)}')
            lines += [f'{i} ' + ' '.join(map(str, cells[i])) for i in chosen]
            lines.append(f'facets {len(facets)}')
            lines += [f'{k} {mk} ' + ' '.join(map(str, f)) for k, (mk, f) in enumerate(facets)]
            lines.append(f'center {center}')
            lines.append('end')
            out = HERE / f'{name}.txt'
            out.write_text('\n'.join(lines) + '\n')
            summary['cases'][name] = {'center_vertex': center, 'center': cx, 'radius': radius, 'cells': len(chosen),
                                      'nodes': len(used), 'facets': {nm: sum(1 for mk, f in facets if mk == k) for k, nm in enumerate(names)},
                                      'wall_edge_median': wall_edges[len(wall_edges) // 2] if wall_edges else None,
                                      'proxy_sensor_size': size, 'sha256': sha(out)}
    summary['script_sha256'] = sha(__file__)
    (HERE / 'prepare_m6.json').write_text(json.dumps(summary, indent=1) + '\n')
    print(json.dumps(summary, indent=1))

if __name__ == '__main__':
    main()
