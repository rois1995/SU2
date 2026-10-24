#!/usr/bin/env python3
"""DC-15 evidence for the LE jaggedness: wall facet size vs. curvature radius and the facet (chord-height) deviation of
the flat WING3D facets of a mesh, by region and span station. Read-only. Usage: wall_curvature.py <mesh.su2> [h0]
Per smooth wall edge (each face keeps its steepest smooth edge) (dihedral <= ADAP_ANGLE 60 deg; creases excluded): turn angle theta between the two face normals,
across-edge curvature kappa = theta / |c1 - c2| (centroid distance), radius R = 1/kappa; w = across-edge spacing |c1 - c2|, chord-height deviation dev = kappa w^2 / 8 = theta w / 8 (L = max face edge, spanwise at the LE); size for a chord-height tolerance eps: L_eps = sqrt(8 eps R). ONERA M6 planform only for the region labels
(LE sweep 30 deg, root chord 0.8059, TE x = 0.8059 + 0.28297 y, span 1.1963)."""
import math, sys
import numpy as np
mesh = sys.argv[1]; h0 = float(sys.argv[2]) if len(sys.argv) > 2 else 2e-6
lines = open(mesh).read().splitlines(); at = 0; P = []; markers = {}
while at < len(lines):
    s = lines[at].split('%')[0].strip()
    if s.startswith('NPOIN='):
        n = int(s.split('=')[1].split()[0]); at += 1
        P = np.array([[float(x) for x in lines[at + k].split()[:3]] for k in range(n)]); at += n; continue
    if s.startswith('MARKER_TAG='):
        name = s.split('=')[1].strip(); n = int(lines[at + 1].split('=')[1]); at += 2
        markers[name] = [tuple(map(int, lines[at + k].split()[1:4])) for k in range(n)]; at += n; continue
    at += 1
F = np.array(markers['WING3D']); A, B, C = P[F[:, 0]], P[F[:, 1]], P[F[:, 2]]
N = np.cross(B - A, C - A); N /= np.linalg.norm(N, axis=1)[:, None]; G = (A + B + C) / 3
L = np.max(np.stack([np.linalg.norm(B - A, axis=1), np.linalg.norm(C - B, axis=1), np.linalg.norm(A - C, axis=1)]), axis=0)
edges = {}
for f, (a, b, c) in enumerate(F):
    for u, v in ((a, b), (b, c), (c, a)): edges.setdefault((min(u, v), max(u, v)), []).append(f)
kap = np.zeros(len(F)); theta_max = np.zeros(len(F)); W = np.zeros(len(F)); devE = np.zeros(len(F))
for e, fs in edges.items():
    if len(fs) != 2: continue
    i, j = fs; th = math.acos(max(-1.0, min(1.0, float(N[i] @ N[j]))))
    if th > math.radians(60): continue  # crease
    w = float(np.linalg.norm(G[i] - G[j])); k = th / w if w > 0 else 0
    for f in fs:
        if math.degrees(th) >= theta_max[f]: theta_max[f] = math.degrees(th); kap[f] = k; W[f] = w; devE[f] = th * w / 8
y = G[:, 1]; xle = 0.57735 * y; xte = 0.8059 + 0.28297 * y; frac = (G[:, 0] - xle) / (xte - xle); eta = y / 1.1963
dev = devE  # chord-height across the steepest smooth edge: kappa w^2 / 8 = theta w / 8
def row(tag, m):
    if not m.any(): print(f'{tag:24s} none'); return
    R = np.where(kap[m] > 0, 1 / np.maximum(kap[m], 1e-300), np.inf)
    q = lambda a, p: float(np.percentile(a[np.isfinite(a)], p)) if np.isfinite(a).any() else float('nan')
    print(f'{tag:24s} faces {m.sum():6d}  w med/max {q(W[m],50)*1e3:6.2f}/{W[m].max()*1e3:6.2f} mm (L max {q(L[m],50)*1e3:5.1f})  R med/min {q(R,50)*1e3:8.3f}/{np.nanmin(R)*1e3:7.3f} mm'
          f'  w/R med {q(W[m]*kap[m],50):5.3f}  turn med/max {q(theta_max[m],50):5.1f}/{theta_max[m].max():5.1f} deg'
          f'  dev med/max {q(dev[m],50)*1e6:8.2f}/{dev[m].max()*1e6:8.2f} um = {dev[m].max()/h0:7.1f} h0  L_eps(1e-4 m) med {q(np.sqrt(8e-4*R),50)*1e3:6.2f} mm')
print(f'mesh {mesh}: {len(F)} WING3D faces, h0 {h0:g} m')
for lo, hi in ((0.15, 0.25), (0.45, 0.55), (0.85, 0.95), (0.0, 1.0)):
    s = (eta >= lo) & (eta < hi)
    row(f'eta {lo:.2f}-{hi:.2f} LE<5%', s & (frac < .05)); row(f'eta {lo:.2f}-{hi:.2f} 5-20%', s & (frac >= .05) & (frac < .2))
    row(f'eta {lo:.2f}-{hi:.2f} mid', s & (frac >= .2) & (frac < .95))
