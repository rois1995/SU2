#!/usr/bin/env python3
"""Independent check of DC-5 exported meshes: positive orientation, face closure (interior faces twice, skin faces
exactly the exported physical faces), first-child height/foot of every wall face against T = 0.10."""
import json, sys, numpy as np
def load(p):
    L = open(p).read().split('\n'); i = 0; P = {}; C = []; F = []; h0 = None
    while i < len(L):
        t = L[i].split()
        if not t: i += 1; continue
        if t[0] == 'nodes':
            for k in range(int(t[1])): r = L[i + 1 + k].split(); P[int(r[0])] = np.array(list(map(float, r[1:4])))
            i += int(t[1]) + 1; continue
        if t[0] == 'cells':
            for k in range(int(t[1])): r = list(map(int, L[i + 1 + k].split())); C.append(r[1:5])
            i += int(t[1]) + 1; continue
        if t[0] == 'faces':
            for k in range(int(t[1])): r = list(map(int, L[i + 1 + k].split())); F.append(r)
            i += int(t[1]) + 1; continue
        if t[0] == 'h0': h0 = float(t[1])
        i += 1
    return P, C, F, h0
out = {}
for p in sys.argv[1:]:
    P, C, F, h0 = load(p)
    vol = [np.dot(np.cross(P[c[1]] - P[c[0]], P[c[2]] - P[c[0]]), P[c[3]] - P[c[0]]) for c in C]
    faces = {}
    for ci, c in enumerate(C):
        for k in range(4): faces.setdefault(tuple(sorted(c[:k] + c[k + 1:])), []).append(ci)
    skin = {f for f, v in faces.items() if len(v) == 1}; bad = [f for f, v in faces.items() if len(v) > 2]
    phys = {tuple(sorted(f[1:4])) for f in F}
    hmax = 0.0; foot_out = 0; walls = 0
    for f in F:
        if not f[5]: continue
        walls += 1; c = C[faces[tuple(sorted(f[1:4]))][0]]; apex = [v for v in c if v not in f[1:4]][0]
        a, b, cc = (P[v] for v in f[1:4]); n = np.cross(b - a, cc - a); A = np.linalg.norm(n); n /= A
        alt = abs(np.dot(P[apex] - a, n)); hmax = max(hmax, abs(alt / h0 - 1))
        q = P[apex] - np.dot(P[apex] - a, n) * n
        w = [np.dot(np.cross(y - q, z - q), n) / A for y, z in ((b, cc), (cc, a), (a, b))]
        foot_out += min(w) < -1e-9
    out[p] = {'cells': len(C), 'nonpositive': int(sum(v <= 0 for v in vol)), 'nonmanifold_faces': len(bad), 'skin_faces': len(skin),
              'skin_equals_physical': skin == phys, 'wall_faces': walls, 'max_rel_height_error': float(hmax), 'height_within_T': bool(hmax <= 0.10),
              'feet_outside': int(foot_out)}
json.dump(out, open('dc5_export_audit.json', 'w'), indent=1); print(json.dumps(out, indent=1))
