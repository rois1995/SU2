#!/usr/bin/env python3
"""WP-E test-only fixture generator: structured extruded tetrahedral domains with wall tubes (holes).

Chart (u,v,w): a (nu x nv) grid of cells in the plane w=0, extruded in nw layers along w with shear
(su,sv) per unit w. Removed grid cells are tubes (holes) through every layer; every hole in `walls` gets the
wall marker, other holes are far. w=0 is the partner plane ("symmetry"); outer sides and top are "far".
Each sheared hexahedron is split into 6 Kuhn tetrahedra, conforming across neighbours.
The chart axis maps w to the physical x, y or z axis, so the partner plane is exactly representable.
Output: SU2_JUNCTION_CASE text read by [NativeJunctionReplay3D] (uniform isotropic sensor donors = cells).
"""
import argparse, hashlib, itertools, json, math
from pathlib import Path

def build(args):
    nu, nv, nw = args.nu, args.nv, args.nw
    holes = {tuple(map(int, h.split(','))) for h in args.holes}
    walls = {tuple(map(int, h.split(','))) for h in args.walls}
    su, sv = math.tan(math.radians(args.lean_u)), math.tan(math.radians(args.lean_v))
    def position(i, j, k):
        w = k * args.dw
        u, v = i * args.du + su * w, j * args.dv + sv * w
        return {'z': (u, v, w), 'y': (v, w, u), 'x': (w, u, v)}[args.axis]
    ids = {}
    nodes = []
    def node(i, j, k):
        key = (i, j, k)
        if key not in ids:
            ids[key] = len(nodes)
            nodes.append(position(i, j, k))
        return ids[key]
    def det(a, b, c, d):
        p = [[b[t] - a[t] for t in range(3)], [c[t] - a[t] for t in range(3)], [d[t] - a[t] for t in range(3)]]
        return (p[0][0] * (p[1][1] * p[2][2] - p[1][2] * p[2][1]) - p[0][1] * (p[1][0] * p[2][2] - p[1][2] * p[2][0])
                + p[0][2] * (p[1][0] * p[2][1] - p[1][1] * p[2][0]))
    cells = []
    for i, j, k in itertools.product(range(nu), range(nv), range(nw)):
        if (i, j) in holes:
            continue
        for perm in itertools.permutations(range(3)):
            corner = [0, 0, 0]
            path = [node(i, j, k)]
            for axis in perm:
                corner[axis] += 1
                path.append(node(i + corner[0], j + corner[1], k + corner[2]))
            if det(*[nodes[n] for n in path]) < 0:
                path[0], path[1] = path[1], path[0]
            cells.append(path)
    faces = {}
    for c in cells:
        for f in [(c[1], c[2], c[3]), (c[0], c[3], c[2]), (c[0], c[1], c[3]), (c[0], c[2], c[1])]:
            faces.setdefault(tuple(sorted(f)), []).append(f)
    inverse = {v: k for k, v in ids.items()}
    markers = ['far', 'symmetry', 'wall']
    facets = []
    for key, uses in faces.items():
        if len(uses) != 1:
            continue
        grid = [inverse[n] for n in key]
        if all(g[2] == 0 for g in grid):
            marker = 1
        else:
            # A hole side face has all its chart (i,j) on the boundary of one hole cell.
            marker = 0
            for h in holes:
                if all(h[0] <= g[0] <= h[0] + 1 and h[1] <= g[1] <= h[1] + 1 for g in grid) and \
                   not all(g[2] == nw for g in grid):
                    marker = 2 if h in walls else 0
        facets.append((len(facets), marker, uses[0]))
    return nodes, cells, facets, markers

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--name', required=True)
    p.add_argument('--nu', type=int, default=10); p.add_argument('--nv', type=int, default=8); p.add_argument('--nw', type=int, default=4)
    p.add_argument('--du', type=float, default=.2); p.add_argument('--dv', type=float, default=.2); p.add_argument('--dw', type=float, default=.25)
    p.add_argument('--lean-u', type=float, default=0.); p.add_argument('--lean-v', type=float, default=0.)
    p.add_argument('--axis', choices=['x', 'y', 'z'], default='z')
    p.add_argument('--holes', nargs='*', default=[]); p.add_argument('--walls', nargs='*', default=[])
    p.add_argument('--h0', type=float, default=.05); p.add_argument('--growth', type=float, default=1.2); p.add_argument('--thickness', type=float, default=.3)
    p.add_argument('--sensor-size', type=float, default=.25)
    p.add_argument('--out', required=True)
    args = p.parse_args()
    nodes, cells, facets, markers = build(args)
    m = 1 / args.sensor_size ** 2
    lines = ['SU2_JUNCTION_CASE 1', f'name {args.name}', f'markers {len(markers)}'] + markers
    lines += [f'policy {args.h0!r} {args.growth!r} {args.thickness!r}', 'walls 1 2', f'core {m!r}', f'sensor {m!r} 0 0 {m!r} 0 {m!r}']
    lines.append(f'nodes {len(nodes)}')
    lines += [f'{i} {x!r} {y!r} {z!r}' for i, (x, y, z) in enumerate(nodes)]
    lines.append(f'cells {len(cells)}')
    lines += [f'{i} ' + ' '.join(map(str, c)) for i, c in enumerate(cells)]
    lines.append(f'facets {len(facets)}')
    lines += [f'{i} {mk} ' + ' '.join(map(str, f)) for i, mk, f in facets]
    lines.append('end')
    Path(args.out).write_text('\n'.join(lines) + '\n')
    counts = {name: sum(1 for f in facets if f[1] == k) for k, name in enumerate(markers)}
    meta = {'arguments': vars(args), 'nodes': len(nodes), 'cells': len(cells), 'facets': counts,
            'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'output_sha256': hashlib.sha256(Path(args.out).read_bytes()).hexdigest(),
            'junction_dihedrals_deg': sorted({90 + args.lean_u, 90 - args.lean_u, 90 + args.lean_v, 90 - args.lean_v})}
    Path(args.out + '.json').write_text(json.dumps(meta, indent=1) + '\n')

if __name__ == '__main__':
    main()
