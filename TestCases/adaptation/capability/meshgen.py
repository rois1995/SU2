#!/usr/bin/env python3
"""Deterministic small SU2 meshes for the adaptation capability cases (no external mesher, no data files).

rectangle: 2D triangles on [x0, x1] x [y0, y1], markers lower, right, upper, left.
plate:     2D triangles on [-0.25, 1] x [0, 0.5], wall-clustered rows; markers symmetry (y = 0, x < 0), wall
           (y = 0, x >= 0), outlet (x = 1), farfield (y = 0.5), inlet (x = -0.25).
bump2d:    2D triangles in a channel [-1.5, 1.5] x [0, 1] over the bump y = BUMP2D_HEIGHT cos^2(pi x / 2), |x| < 1,
           wall-clustered rows; markers lower (bump wall), outlet, upper, inlet.
bump3d:    3D tetrahedra (6 per hexahedron) in a channel [-1.5, 1.5] x [0, 1] x [0, 1] whose lower wall is the bump
           y = BUMP_HEIGHT cos^2(pi x / 2) sin^2(pi z) for |x| < 1 (zero on the side walls, so every seam is straight);
           markers lower (bump), upper (y = 1), side0 (z = 0), side1 (z = 1), inlet (x = -1.5), outlet (x = 1.5).
"""

import math
from pathlib import Path

BUMP_HEIGHT = 0.1
BUMP2D_HEIGHT = 0.05


def bump2d_wall(x):
    """Height of the 2D bump wall (the analytic reference of the 'lower' marker)."""
    return BUMP2D_HEIGHT * math.cos(0.5 * math.pi * x) ** 2 if abs(x) < 1.0 else 0.0


def _rows(n, first, height):
    """n geometric rows from 0 to height with first row height 'first'."""
    ratio = 1.2
    for _ in range(200):  # growth ratio with first * (r^n - 1) / (r - 1) = height
        ratio = (height * (ratio - 1.0) / first + 1.0) ** (1.0 / n)
    return [0.0] + [first * (ratio ** j - 1.0) / (ratio - 1.0) for j in range(1, n)] + [height]


def bump(x, z):
    """Height of the 3D bump wall (the analytic reference of the 'lower' marker)."""
    if abs(x) >= 1.0:
        return 0.0
    return BUMP_HEIGHT * math.cos(0.5 * math.pi * x) ** 2 * math.sin(math.pi * z) ** 2


def _write(path, ndim, elems, points, markers):
    """elems: (vtk type, nodes); points: coordinates; markers: {name: [(vtk type, nodes)]} (dict order kept)."""
    with Path(path).open("w") as stream:
        stream.write(f"NDIME= {ndim}\nNELEM= {len(elems)}\n")
        for i, (kind, nodes) in enumerate(elems):
            stream.write(f"{kind} " + " ".join(map(str, nodes)) + f" {i}\n")
        stream.write(f"NPOIN= {len(points)}\n")
        for i, x in enumerate(points):
            stream.write(" ".join(f"{c:.17g}" for c in x) + f" {i}\n")
        stream.write(f"NMARK= {len(markers)}\n")
        for name, faces in markers.items():
            stream.write(f"MARKER_TAG= {name}\nMARKER_ELEMS= {len(faces)}\n")
            for kind, nodes in faces:
                stream.write(f"{kind} " + " ".join(map(str, nodes)) + "\n")


def _grid2d(xs, ys):
    nx, ny = len(xs) - 1, len(ys) - 1
    point = lambda i, j: i + (nx + 1) * j
    elems = []
    for j in range(ny):
        for i in range(nx):
            a, b, c, d = point(i, j), point(i + 1, j), point(i + 1, j + 1), point(i, j + 1)
            elems += [(5, (a, b, c)), (5, (a, c, d))]
    points = [(x, y) for y in ys for x in xs]
    return nx, ny, point, elems, points


def rectangle(path, nx=24, ny=24, x0=-5.0, x1=5.0, y0=-5.0, y1=5.0):
    xs = [x0 + (x1 - x0) * i / nx for i in range(nx + 1)]
    ys = [y0 + (y1 - y0) * j / ny for j in range(ny + 1)]
    nx, ny, point, elems, points = _grid2d(xs, ys)
    markers = {
        "lower": [(3, (point(i + 1, 0), point(i, 0))) for i in range(nx)],
        "right": [(3, (point(nx, j + 1), point(nx, j))) for j in range(ny)],
        "upper": [(3, (point(i, ny), point(i + 1, ny))) for i in range(nx)],
        "left": [(3, (point(0, j), point(0, j + 1))) for j in range(ny)],
    }
    _write(path, 2, elems, points, markers)


def plate(path, n_front=6, n_wall=30, ny=20, first=2e-3, height=0.5):
    """Flat plate starting at x = 0; geometric rows from the wall (first row height 'first')."""
    xs = [-0.25 + 0.25 * i / n_front for i in range(n_front)] + [i / n_wall for i in range(n_wall + 1)]
    ys = _rows(ny, first, height)
    nx, ny, point, elems, points = _grid2d(xs, ys)
    markers = {
        "symmetry": [(3, (point(i + 1, 0), point(i, 0))) for i in range(n_front)],
        "wall": [(3, (point(i + 1, 0), point(i, 0))) for i in range(n_front, nx)],
        "outlet": [(3, (point(nx, j + 1), point(nx, j))) for j in range(ny)],
        "farfield": [(3, (point(i, ny), point(i + 1, ny))) for i in range(nx)],
        "inlet": [(3, (point(0, j), point(0, j + 1))) for j in range(ny)],
    }
    _write(path, 2, elems, points, markers)


def bump2d(path, nx=60, ny=20, first=2e-3):
    xs = [-1.5 + 3.0 * i / nx for i in range(nx + 1)]
    eta = _rows(ny, first, 1.0)
    nx, ny, point, elems, _ = _grid2d(xs, eta)
    points = [(x, bump2d_wall(x) + e * (1.0 - bump2d_wall(x))) for e in eta for x in xs]
    markers = {
        "lower": [(3, (point(i + 1, 0), point(i, 0))) for i in range(nx)],
        "outlet": [(3, (point(nx, j + 1), point(nx, j))) for j in range(ny)],
        "upper": [(3, (point(i, ny), point(i + 1, ny))) for i in range(nx)],
        "inlet": [(3, (point(0, j), point(0, j + 1))) for j in range(ny)],
    }
    _write(path, 2, elems, points, markers)


def _tet_volume(p, t):
    a, b, c, d = (p[i] for i in t)
    u = [b[k] - a[k] for k in range(3)]
    v = [c[k] - a[k] for k in range(3)]
    w = [d[k] - a[k] for k in range(3)]
    return (u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0]) + u[2] * (v[0] * w[1] - v[1] * w[0]))


def bump3d(path, nx=24, ny=8, nz=8):
    """Kuhn split of each hexahedron (6 tetrahedra along its main diagonal): conforming across hexahedra."""
    idx = lambda i, j, k: i + (nx + 1) * (j + (ny + 1) * k)
    points = []
    for k in range(nz + 1):
        for j in range(ny + 1):
            for i in range(nx + 1):
                x, z, eta = -1.5 + 3.0 * i / nx, k / nz, j / ny
                b = bump(x, z)
                points.append((x, b + eta * (1.0 - b), z))
    # the 6 monotone paths from corner (0,0,0) to (1,1,1)
    paths = [(0, 1, 2), (0, 2, 1), (1, 0, 2), (1, 2, 0), (2, 0, 1), (2, 1, 0)]
    elems = []
    for k in range(nz):
        for j in range(ny):
            for i in range(nx):
                for path_ in paths:
                    c = [i, j, k]
                    nodes = [idx(*c)]
                    for axis in path_:
                        c[axis] += 1
                        nodes.append(idx(*c))
                    if _tet_volume(points, nodes) < 0.0:
                        nodes[2], nodes[3] = nodes[3], nodes[2]
                    elems.append((10, tuple(nodes)))
    # boundary faces: faces of one tetrahedron, oriented outwards
    count = {}
    for _, t in elems:
        for f in ((t[1], t[2], t[3]), (t[0], t[3], t[2]), (t[0], t[1], t[3]), (t[0], t[2], t[1])):
            key = tuple(sorted(f))
            count[key] = None if key in count else f
    ijk = lambda p: (p % (nx + 1), (p // (nx + 1)) % (ny + 1), p // ((nx + 1) * (ny + 1)))
    names = ["lower", "upper", "side0", "side1", "inlet", "outlet"]
    markers = {name: [] for name in names}
    for key, face in count.items():
        if face is None:
            continue
        c = [ijk(p) for p in face]
        for name, axis, value in (("lower", 1, 0), ("upper", 1, ny), ("side0", 2, 0), ("side1", 2, nz),
                                  ("inlet", 0, 0), ("outlet", 0, nx)):
            if all(q[axis] == value for q in c):
                markers[name].append((5, face))
                break
        else:
            raise RuntimeError("unclassified boundary face")
    for name in names:
        markers[name].sort()
    _write(path, 3, elems, points, markers)


if __name__ == "__main__":
    import sys
    kind, out = sys.argv[1], sys.argv[2]
    {"rectangle": rectangle, "plate": plate, "bump2d": bump2d, "bump3d": bump3d}[kind](out)
