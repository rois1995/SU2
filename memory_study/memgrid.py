#!/usr/bin/env python3
"""Bump-in-channel meshes for the SU2 memory study (SU2 native ASCII format).

Same points for every element type; only the connectivity changes.
  2D types: quad | tri | mixed (quads in the bottom third of the rows, triangles above)
  3D types: hexa | tet | mixed (prisms extruded wall-normal in the bottom third, tets above)
Tets: Freudenthal 6-tet split of every hexa (conforming). Prism bases are split along the
same x-z diagonal as the tet faces, so the prism/tet interface is conforming.
Markers: inlet (x=0), outlet (x=L), wall (y=wall), top (y=H), 3D also side (z=0 and z=W).
"""
import argparse, itertools, sys
import numpy as np

L, H, W, BETA = 3.0, 1.0, 1.0, 5.0


def wall_height(x, z=None):
    h = np.where((x > 1.0) & (x < 2.0), 0.05 * np.sin(np.pi * (x - 1.0)) ** 2, 0.0)
    if z is not None:
        h = h * np.sin(np.pi * z / W) ** 2
    return h


def stretch(n):
    eta = np.linspace(0.0, 1.0, n)
    return 1.0 - np.tanh(BETA * (1.0 - eta)) / np.tanh(BETA)


def coords(dim, ni, nj, nk):
    s = stretch(nj)
    x1 = np.linspace(0.0, L, ni)
    if dim == 2:
        X, S = np.meshgrid(x1, s, indexing="xy")          # shape (nj, ni): node = i + ni*j
        yw = wall_height(X)
        return np.column_stack([X.ravel(), (yw + (H - yw) * S).ravel()])
    z1 = np.linspace(0.0, W, nk)
    Z, S, X = np.meshgrid(z1, s, x1, indexing="ij")       # shape (nk, nj, ni): node = i + ni*(j + nj*k)
    yw = wall_height(X, Z)
    return np.column_stack([X.ravel(), (yw + (H - yw) * S).ravel(), Z.ravel()])


def write_section(f, header, rows, fmt):
    f.write(header)
    if len(rows):
        np.savetxt(f, rows, fmt=fmt)


def elems_2d(ni, nj, etype, jb):
    I, J = np.meshgrid(np.arange(ni - 1), np.arange(nj - 1), indexing="xy")
    I, J = I.ravel(), J.ravel()
    n00 = I + ni * J; n10 = n00 + 1; n01 = n00 + ni; n11 = n01 + 1
    quadmask = np.zeros(I.shape, bool) if etype == "tri" else (np.ones(I.shape, bool) if etype == "quad" else J < jb)
    quads = np.column_stack([n00, n10, n11, n01])[quadmask]
    t = ~quadmask
    tris = np.vstack([np.column_stack([n00, n10, n11])[t], np.column_stack([n00, n11, n01])[t]])
    return [(9, quads), (5, tris)]


def bnd_2d(ni, nj):
    i = np.arange(ni - 1); j = np.arange(nj - 1)
    return {
        "wall": np.column_stack([i, i + 1]),
        "top": np.column_stack([i + ni * (nj - 1), i + 1 + ni * (nj - 1)]),
        "inlet": np.column_stack([ni * j, ni * (j + 1)]),
        "outlet": np.column_stack([ni - 1 + ni * j, ni - 1 + ni * (j + 1)]),
    }


# Unit-cube corner bits (dx, dy, dz).
HEX_BITS = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (0, 0, 1), (1, 0, 1), (1, 1, 1), (0, 1, 1)]


def tet_bits():
    tets = []
    for p in itertools.permutations(range(3)):
        v = [0, 0, 0]; path = [tuple(v)]
        for ax in p:
            v[ax] = 1; path.append(tuple(v))
        a, b, c, d = (np.array(q, float) for q in path)
        if np.dot(np.cross(b - a, c - a), d - a) < 0:
            path[2], path[3] = path[3], path[2]
        tets.append(path)
    return tets


def prism_bits():
    # Base triangles in the x-z plane (dy=0), split along diagonal 000-101; extruded to dy=1.
    out = []
    for tri in [[(0, 0, 0), (1, 0, 1), (1, 0, 0)], [(0, 0, 0), (0, 0, 1), (1, 0, 1)]]:
        a, b, c = (np.array(q, float) for q in tri)
        top = np.array([0, 1, 0], float)
        # SU2 rule: tet(0,2,1,3) positive, i.e. base normal (0,1,2) points away from the top.
        if np.dot(np.cross(c - a, b - a), top) <= 0:
            tri = [tri[0], tri[2], tri[1]]
        out.append(tri + [(q[0], 1, q[2]) for q in tri])
    return out


def elems_3d(ni, nj, nk, etype, jb):
    K, J, I = np.meshgrid(np.arange(nk - 1), np.arange(nj - 1), np.arange(ni - 1), indexing="ij")
    I, J, K = I.ravel(), J.ravel(), K.ravel()
    base = I + ni * (J + nj * K)
    off = lambda b: b[0] + ni * (b[1] + nj * b[2])
    conn = lambda bits, m: np.column_stack([base[m] + off(b) for b in bits])
    allm = np.ones(I.shape, bool)
    if etype == "hexa":
        return [(12, conn(HEX_BITS, allm))]
    tetm = allm if etype == "tet" else J >= jb
    out = [(10, np.vstack([conn(t, tetm) for t in tet_bits()]))]
    if etype == "mixed":
        out.append((13, np.vstack([conn(p, ~tetm) for p in prism_bits()])))
    return out


def face_conn(n0, da, db, tri_mask):
    """Boundary face on a structured patch: corner n0, steps da, db; split along n0 -> n0+da+db where tri_mask."""
    q = np.column_stack([n0, n0 + da, n0 + da + db, n0 + db])
    return q[~tri_mask], np.vstack([q[tri_mask][:, [0, 1, 2]], q[tri_mask][:, [0, 2, 3]]])


def bnd_3d(ni, nj, nk, etype, jb):
    sj, sk = ni, ni * nj
    simplex = lambda jj: np.ones(jj.shape, bool) if etype == "tet" else (np.zeros(jj.shape, bool) if etype == "hexa" else jj >= jb)
    out = {}
    # y faces (wall j=0, top j=nj-1): x-z patches, diagonal (i,k)-(i+1,k+1). Mixed: prisms at the wall have tri bases too.
    K, I = np.meshgrid(np.arange(nk - 1), np.arange(ni - 1), indexing="ij"); I, K = I.ravel(), K.ravel()
    wall_tri = np.zeros(I.shape, bool) if etype == "hexa" else np.ones(I.shape, bool)
    out["wall"] = face_conn(I + sk * K, 1, sk, wall_tri)
    top_tri = np.zeros(I.shape, bool) if etype == "hexa" else np.ones(I.shape, bool)
    out["top"] = face_conn(I + sj * (nj - 1) + sk * K, 1, sk, top_tri)
    # x faces: y-z patches, diagonal (j,k)-(j+1,k+1).
    K, J = np.meshgrid(np.arange(nk - 1), np.arange(nj - 1), indexing="ij"); J, K = J.ravel(), K.ravel()
    out["inlet"] = face_conn(sj * J + sk * K, sj, sk, simplex(J))
    out["outlet"] = face_conn(ni - 1 + sj * J + sk * K, sj, sk, simplex(J))
    # z faces: x-y patches, diagonal (i,j)-(i+1,j+1).
    J, I = np.meshgrid(np.arange(nj - 1), np.arange(ni - 1), indexing="ij"); I, J = I.ravel(), J.ravel()
    s0 = face_conn(I + sj * J, 1, sj, simplex(J))
    s1 = face_conn(I + sj * J + sk * (nk - 1), 1, sj, simplex(J))
    out["side"] = (np.vstack([s0[0], s1[0]]), np.vstack([s0[1], s1[1]]))
    return out


def check_orientation(xyz, sections):
    """Same tests as CPhysicalGeometry::Check_IntElem_Orientation. Returns number of elements SU2 would flip."""
    def tet_neg(c, a, b, d, e):
        p = [xyz[c[:, k]] for k in (a, b, d, e)]
        return np.einsum("ij,ij->i", np.cross(p[1] - p[0], p[2] - p[0]), p[3] - p[0]) < 0
    def tri_neg(c, a, b, d):
        p = [xyz[c[:, k]] for k in (a, b, d)]
        u, v = p[1] - p[0], p[2] - p[0]
        return u[:, 0] * v[:, 1] - u[:, 1] * v[:, 0] < 0
    bad = 0
    for vtk, c in sections:
        if not len(c):
            continue
        if vtk == 5: bad += tri_neg(c, 0, 1, 2).sum()
        if vtk == 9: bad += (tri_neg(c, 0, 1, 2) | tri_neg(c, 0, 2, 3)).sum()
        if vtk == 10: bad += tet_neg(c, 0, 1, 2, 3).sum()
        if vtk == 13: bad += (tet_neg(c, 0, 2, 1, 3) | tet_neg(c, 3, 4, 5, 2)).sum()
        if vtk == 12:
            bad += (tet_neg(c, 0, 1, 2, 5) | tet_neg(c, 0, 2, 3, 7) | tet_neg(c, 4, 6, 5, 1) | tet_neg(c, 4, 7, 6, 3)).sum()
    return int(bad)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dim", type=int, choices=[2, 3], required=True)
    ap.add_argument("--type", required=True)
    ap.add_argument("--ni", type=int, required=True)
    ap.add_argument("--nj", type=int, required=True)
    ap.add_argument("--nk", type=int, default=1)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    ok = {2: ("quad", "tri", "mixed"), 3: ("hexa", "tet", "mixed")}[a.dim]
    if a.type not in ok:
        sys.exit(f"type must be one of {ok}")
    jb = (a.nj - 1) // 3
    xyz = coords(a.dim, a.ni, a.nj, a.nk)
    if a.dim == 2:
        secs = elems_2d(a.ni, a.nj, a.type, jb)
        bnd = {k: [(3, v)] for k, v in bnd_2d(a.ni, a.nj).items()}
    else:
        secs = elems_3d(a.ni, a.nj, a.nk, a.type, jb)
        bnd = {k: [(9, q), (5, t)] for k, (q, t) in bnd_3d(a.ni, a.nj, a.nk, a.type, jb).items()}
    bad = check_orientation(xyz, secs)
    if bad:
        sys.exit(f"orientation check failed for {bad} elements")
    nelem = sum(len(c) for _, c in secs)
    with open(a.out, "w") as f:
        f.write(f"NDIME= {a.dim}\nNELEM= {nelem}\n")
        eid = 0
        for vtk, c in secs:
            if len(c):
                rows = np.column_stack([np.full(len(c), vtk), c, np.arange(eid, eid + len(c))])
                np.savetxt(f, rows, fmt="%d")
                eid += len(c)
        f.write(f"NPOIN= {len(xyz)}\n")
        np.savetxt(f, np.column_stack([xyz, np.arange(len(xyz))]), fmt=["%.15e"] * a.dim + ["%d"])
        f.write(f"NMARK= {len(bnd)}\n")
        for name, lst in bnd.items():
            n = sum(len(c) for _, c in lst)
            f.write(f"MARKER_TAG= {name}\nMARKER_ELEMS= {n}\n")
            for vtk, c in lst:
                if len(c):
                    np.savetxt(f, np.column_stack([np.full(len(c), vtk), c]), fmt="%d")
    counts = {vtk: len(c) for vtk, c in secs}
    print(f"MESH {a.out} dim={a.dim} type={a.type} npoin={len(xyz)} nelem={nelem} by_vtk={counts} jb={jb}")


if __name__ == "__main__":
    main()
