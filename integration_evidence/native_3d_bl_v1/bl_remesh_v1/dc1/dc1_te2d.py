#!/usr/bin/env python3
"""DC-1 2D part: the accepted v8 airfoil TE metric under the shared D1 law (2D: feature vertex, circumferential
direction receives hn_f(rho)). Proxy: the recorded frozen P1 metric (metric.csv) already contains today's 2D BL
corner limit, which D1 replaces; D1 is applied ON TOP of it, so dC2D here is an upper estimate of the increase.
Reports: TE turn and fold excess E, R_f, the recorded circumferential size vs hn_f(rho) at sample radii, and
dC2D = int (sqrt det(M ∩ F) - sqrt det M) dA over the fluid part of rho <= R_f, d <= T (polar quadrature)."""
import hashlib, json, math, os, sys
import numpy as np
EV = '/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence'
sys.path.insert(0, EV)
from audit_native_bl import mesh            # noqa: E402
from frozen_field_audit import FrozenField  # noqa: E402

H0, G, T = 2e-4, 1.25, 0.02
case = os.path.join(EV, 'integrated_airfoil_baseline_v8', 'audit_np1', 'native_airfoil_cycle_0')
files = [case + s for s in ('_donor.su2', '_metric.csv')]
pts, cells, mk = mesh(files[0]); tens = [tuple(map(float, r.split(','))) for r in open(files[1]).read().splitlines()[1:]]
F = FrozenField(pts, cells, tens, mk, 1e-6)
P = np.array(pts); wall = np.array(mk['airfoil'])
te = int(np.argmin(np.linalg.norm(P - [1.0, 0.0], axis=1))); TE = P[te]
nb = [b if a == te else a for a, b in wall.tolist() if te in (a, b)]
u1 = (P[nb[0]] - TE) / np.linalg.norm(P[nb[0]] - TE); u2 = (P[nb[1]] - TE) / np.linalg.norm(P[nb[1]] - TE)
wedge = math.degrees(math.acos(float(u1 @ u2))); turn = 180 - wedge
def hn(d): return np.maximum(H0, 2 * (H0 + (G - 1) * d) / (G + 1))
def Gc(d):
    return d / H0 if d <= H0 / 2 else 0.5 + (G + 1) / (2 * (G - 1)) * math.log1p(2 * (G - 1) * (d - H0 / 2) / (H0 * (G + 1)))
phi = math.radians(turn) / 2; E = Gc(T / math.cos(phi)) - Gc(T)
# recorded tangential sizes along the wall near TE bound h_max: largest metric size in the region
A = P[wall[:, 0]]; B = P[wall[:, 1]]
def wdist(X):
    AB = B - A; t = np.clip(((X[:, None, :] - A[None]) * AB[None]).sum(-1) / (AB * AB).sum(-1)[None], 0, 1)
    Q = A[None] + t[..., None] * AB[None]; return np.linalg.norm(X[:, None, :] - Q, axis=-1).min(1)
def Mat(x):
    xx, xy, yy = F(tuple(map(float, x))); return np.array([[xx, xy], [xy, yy]], dtype=float)
# polygon test (airfoil interior) by winding
loop = wall
def inside(x):
    s = 0.0
    for a, b in loop.tolist():
        p, q = P[a] - x, P[b] - x; s += math.atan2(p[0] * q[1] - p[1] * q[0], p @ q)
    return abs(s) > math.pi
# R_f from the largest recorded size within T of the wall near TE (h_max^-2 = min eigenvalue)
probe = [TE + r * np.array([math.cos(a), math.sin(a)]) for r in np.geomspace(H0, 0.2, 30) for a in np.linspace(0, 2 * math.pi, 24, endpoint=False)]
probe = [x for x in probe if not inside(x) and wdist(x[None])[0] < T]
hmax = max(1 / math.sqrt(np.linalg.eigvalsh(Mat(x))[0]) for x in probe)
R_f = ((G + 1) * hmax / 2 - H0) / (G - 1)
# circumferential sizes: recorded vs hn_f
cmp_ = []
for r in (H0, 3e-4, 1e-3, 3e-3, 1e-2, 2e-2):
    for a in (0.0,):
        x = TE + r * np.array([1.0, 0.0]); et = np.array([0.0, 1.0])
        cmp_.append({'rho': r, 'recorded_circumferential_size': float(1 / math.sqrt(et @ Mat(x) @ et)), 'hn_f': float(hn(r))})
# dC2D by polar quadrature (log-spaced radii, uniform angles)
nr, na = 160, 180; lr = np.linspace(math.log(H0 / 100), math.log(R_f), nr + 1); dC = 0.0; Cold = 0.0
for i in range(nr):
    r = math.exp(0.5 * (lr[i] + lr[i + 1])); dr = math.exp(lr[i + 1]) - math.exp(lr[i])
    for j in range(na):
        a = 2 * math.pi * (j + 0.5) / na; e_r = np.array([math.cos(a), math.sin(a)]); x = TE + r * e_r
        if inside(x) or wdist(x[None])[0] > T: continue
        try: M = Mat(x)
        except Exception: continue
        et = np.array([-e_r[1], e_r[0]]); lam = 1 / hn(r) ** 2
        Fm = lam * np.outer(et, et)
        S = M + Fm; w, V = np.linalg.eigh(S); Sh = (V * np.sqrt(w)) @ V.T; Si = (V / np.sqrt(w)) @ V.T
        l, Q = np.linalg.eigh(Si @ M @ Si); X = Sh @ ((Q * np.maximum(l, 1 - l)) @ Q.T) @ Sh
        dA = r * dr * 2 * math.pi / na
        dC += (math.sqrt(max(np.linalg.det(X), 0)) - math.sqrt(max(np.linalg.det(M), 0))) * dA
        Cold += math.sqrt(max(np.linalg.det(M), 0)) * dA
res = {'case': 'integrated_airfoil_baseline_v8 cycle 0', 'files': {f: hashlib.sha256(open(f, 'rb').read()).hexdigest() for f in files},
       'te_wedge_deg': wedge, 'te_turn_deg': turn, 'fold_excess_E': E, 'feature': E > 0.2, 'h_max_near_te': hmax, 'R_f': R_f,
       'circumferential_sizes': cmp_, 'complexity_in_region_recorded': Cold, 'dC2D_upper_estimate': dC,
       'note': 'D1 applied on top of the recorded metric (corner limit not removable from the P1 field); polar quadrature 160x180',
       'script_sha256': hashlib.sha256(open(__file__, 'rb').read()).hexdigest()}
json.dump(res, open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'results', 'te2d_v8.json'), 'w'), indent=1)
print(json.dumps(res, indent=1))
