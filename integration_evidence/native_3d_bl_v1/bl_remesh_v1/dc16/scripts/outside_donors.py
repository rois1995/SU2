#!/usr/bin/env python3
"""DC-16 diagnosis: mesh vertices of an exported *_final.mesh that no original donor tetrahedron contains (closed test,
float64 with a 1e-12 relative barycentric slack), split by skin/interior. Usage: outside_donors.py <final.mesh> <donors.bin>"""
import os, sys, struct
import numpy as np
from scipy.spatial import cKDTree
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../viz'))
import make_slices as ms
m = ms.read_mesh(sys.argv[1]); X = m["X"]
raw = open(sys.argv[2], 'rb').read(); n = struct.unpack_from('<Q', raw, 0)[0]
rec = np.dtype([('id', '<u8'), ('x', '<f8', 3), ('m', '<f8', 6)])
D = np.frombuffer(raw, dtype=rec, count=4 * n, offset=8)['x'].reshape(n, 4, 3)
cen = D.mean(axis=1); rad = np.max(np.linalg.norm(D - cen[:, None, :], axis=2), axis=1); tree = cKDTree(cen)
used = np.unique(m["T"]); skin = np.zeros(len(X), bool); skin[np.unique(m["Ftri"])] = True
wall = np.zeros(len(X), bool); wall[np.unique(m["Ftri"][m["F"][:, 5] == 1])] = True
def inside(p, ks):
    for k in ks:
        a, b, c, d = D[k]; M = np.column_stack([b - a, c - a, d - a])
        try: w = np.linalg.solve(M, p - a)
        except np.linalg.LinAlgError: continue
        l = np.array([1 - w.sum(), *w])
        if np.all(l >= -1e-12): return True
    return False
out = []
rmax = rad.max()
for v in used:
    p = X[v]; ks = tree.query_ball_point(p, rmax)
    ks = [k for k in ks if np.linalg.norm(p - cen[k]) <= rad[k] * (1 + 1e-9) + 1e-15]
    if not inside(p, ks): out.append(v)
out = np.array(out, int)
print(f"vertices {len(used)}, outside every donor {len(out)}: skin {int(np.sum(skin[out]))} (wall {int(np.sum(wall[out]))}), interior {int(np.sum(~skin[out]))}")
for v in out[:20]: print(v, X[v], "wall" if wall[v] else "skin" if skin[v] else "interior")
