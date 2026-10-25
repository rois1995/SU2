#!/usr/bin/env python3
"""DC-16: where the growth layers are missing. Vertices of a *_final.mesh by wall-distance band (in h0) and region of
their nearest wall point (ONERA M6 planform labels: root eta < 5 %, tip eta > 95 %, LE chord < 5 %, TE chord > 95 %,
else mid), normalised by the wall vertices of the region (one full layer = 1.0 per band slot), plus the bad-cell and
long-edge-free shares from the exported gates. Usage: layer_census.py <final.mesh> <h0>"""
import os, sys
import numpy as np
from scipy.spatial import cKDTree
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../viz'))
import make_slices as ms
m = ms.read_mesh(sys.argv[1]); h0 = float(sys.argv[2])
X, F = m["X"], m["F"]; W = m["Ftri"][F[:, 5] == 1]
A, B, C = X[W[:, 0]], X[W[:, 1]], X[W[:, 2]]; G = (A + B + C) / 3; tree = cKDTree(G)
def ptri(P, a, b, c):  # exact point-triangle distance (vectorised over candidate triangles)
    ab, ac, ap = b - a, c - a, P - a
    n = np.cross(ab, ac); nn = np.linalg.norm(n, axis=1); u = n / nn[:, None]
    q = P - np.sum(ap * u, axis=1)[:, None] * u
    def inside(x, y): return np.sum(np.cross(y - x, q - x) * n, axis=1) >= 0
    ins = inside(a, b) & inside(b, c) & inside(c, a)
    def seg(x, y):
        d = y - x; t = np.clip(np.sum((P - x) * d, axis=1) / np.maximum(np.sum(d * d, axis=1), 1e-300), 0, 1)
        return np.linalg.norm(P - (x + t[:, None] * d), axis=1)
    return np.where(ins, np.abs(np.sum(ap * u, axis=1)), np.minimum(np.minimum(seg(a, b), seg(b, c)), seg(c, a)))
used = np.unique(m["T"]); d = np.zeros(len(X)); foot = np.zeros((len(X), 3))
_, idx = tree.query(X[used], k=12)
for i, v in enumerate(used):
    k = idx[i]; dd = ptri(X[v][None, :].repeat(len(k), 0), A[k], B[k], C[k]); j = np.argmin(dd); d[v] = dd[j]; foot[v] = G[k[j]]
def region(p):
    y = p[:, 1]; xle = 0.57735 * y; xte = 0.8059 + 0.28297 * y; fr = (p[:, 0] - xle) / (xte - xle); eta = y / 1.1963
    r = np.full(len(p), 'mid', object); r[fr < .05] = 'LE'; r[fr > .95] = 'TE'; r[eta < .05] = 'root'; r[eta > .95] = 'tip'; return r
reg = region(foot[used]); wallv = np.unique(W); wreg = region(X[wallv])
bands = [(0.5, 1.5), (1.5, 3), (3, 6), (6, 12), (12, 25), (25, 50), (50, 100), (100, 300)]
print(f"{sys.argv[1]}: vertices per wall vertex by band (h0 = {h0:g} m); 1.0 = one full layer in the band")
print("region  wall_v " + " ".join(f"{a:g}-{b:g}".rjust(8) for a, b in bands))
for r in ('LE', 'mid', 'TE', 'root', 'tip'):
    nw = max(1, int(np.sum(wreg == r))); dr = d[used][reg == r] / h0
    print(f"{r:6s} {nw:7d} " + " ".join(f"{np.sum((dr >= a) & (dr < b)) / nw:8.2f}" for a, b in bands))
# DC-16: bad cells and long edges by wall-distance band (mean vertex distance of the cell / edge) and region
dv = np.full(len(X), np.nan); dv[used] = d[used] / h0; rv = np.full(len(X), '', object); rv[used] = reg
cd = np.nanmean(dv[m["T"]], axis=1); cr = rv[m["T"][:, 0]]
ed = np.nanmean(dv[m["E"]], axis=1) if len(m["E"]) else np.zeros(0); er = rv[m["E"][:, 0]] if len(m["E"]) else np.zeros(0, object)
bands2 = [(0, 3), (3, 10), (10, 30), (30, 100), (100, 300), (300, 1e9)]
for name, mask, dist, rr in (("bad cells", m["bad"], cd, cr), ("long edges", np.ones(len(ed), bool), ed, er)):
    print(f"{name} by band (h0) and region; total {int(np.sum(mask))}, in 10-300 h0: {int(np.sum(mask & (dist >= 10) & (dist < 300)))}")
    print("region  " + " ".join(f"{a:g}-{b:g}".rjust(9) for a, b in bands2))
    for r in ('LE', 'mid', 'TE', 'root', 'tip'):
        print(f"{r:6s}  " + " ".join(f"{int(np.sum(mask & (rr == r) & (dist >= a) & (dist < b))):9d}" for a, b in bands2))
