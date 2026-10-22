#!/usr/bin/env python3
"""DC-1 (BL_REMESH_PLAN rev 3): D1 law specification checks with the Python native-law transcription.

Proxy only (lib/native_law.py transcription of CBoundaryLayerMetric; faceted walls, isotropic core/sensor proxy);
native gates are not evaluated here. Per case:
  1. D1 feature set from the ORIGINAL facets: BL-BL and BL-nonBL creases, exact convexity (fluid-oriented normals),
     finite-profile fold excess E(phi) = Gc(T/cos phi) - Gc(T) (closed form, log1p; Gc(d) = d/h0 for g = 1),
     features E > 0.2, chaining, corners (>= 3 feature edges, line turn >= ADAP_ANGLE, end off a carrier), carrier
     ends with reflection image segments (corner if segment+image turn >= ADAP_ANGLE or meeting another feature).
  2. Support radius R_f: smallest rho with hn_f(rho) >= h_max, h_max = max(largest tangential wall size, hn(T)).
  3. Law X = M_BL(pre-fade) ∩ F_1 ∩ ... in canonical feature order over ALL active features (rho <= R_f; exactly
     redundant F skipped: A >= F => A∩F = A), then the production fade and the isotropic core.
  4. Continuity: support boundary (rho = R_f -/+ delta), segment switches (all-active vs the superseded k-nearest
     proxy), carrier reflection symmetry at line ends on a symmetry plane.
  5. Gradation: directional size slope along rays from features vs 2(g-1)/(g+1) (O1 beyond T).
  6. Complexity added by D1 in the BL support, dC = int (sqrt det X_faded - sqrt det M_BL_faded) dV, importance-
     sampled in tubes rho <= R_f around every feature segment (fluid, d < T), with its Monte-Carlo standard error.
  7. O1 variant g = 1 (hn_f = h0 for rho <= T, then slope 2(hgrad-1)/(hgrad+1)) on the same geometry.
Usage: dc1_d1law.py <case> [<case> ...]; cases: m6, tube_root_z, tube_root_x, two_tube_y, annulus_faceted
"""
import hashlib, json, math, os, resource, sys, time
import numpy as np
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
EV = os.path.normpath(os.path.join(HERE, '..', '..'))                 # integration_evidence/native_3d_bl_v1
CS = os.path.join(EV, 'construction_strategy_v1')
sys.path.insert(0, os.path.join(CS, 'lib'))
import inflate_common as ic, native_law as nl, swbi_build3 as sb   # noqa: E402

EPS_F, ANGLE = 0.2, 45.0
REFLECTIVE = ('SYMMETRY', 'symmetry')


def sha(p):
    return hashlib.sha256(open(p, 'rb').read()).hexdigest()


def load(case):
    if case == 'm6':
        path = os.path.join(EV, 'oneram6_coarse_v1', 'Coarse.su2')
        cells, P, mk = ic.read_su2(path); sym = np.unique(mk['SYMMETRY']); P[sym, 1] = 0.0
        return path, P, cells, mk, ['WING3D'], (2e-6, 1.2, 0.01), 2e-2, 1.3
    if case == 'annulus_faceted':
        path = os.path.join(EV, 'mapped_component_geometry_v1', 'annulus_n1', 'affine_annulus_bl_budget_20_m1.su2')
        cells, P, mk = ic.read_su2(path)
        return path, P, cells, mk, ['wall'], (0.1, 1.2, 0.3), 0.25, 1.3
    path = os.path.join(EV, 'robust_junction_v1', 'wp0b', case + '.txt')
    P, cells, mk, meta = sb.read_fixture(path)
    return path, P, cells, mk, meta['walls'], meta['policy'], meta['hs'], 1.3


class Profile:
    """Face normal-size law hn and the decided D1 law hn_f (O1)."""

    def __init__(self, h0, g, T, hgrad):
        self.h0, self.g, self.T, self.hgrad = h0, g, T, hgrad

    def hn(self, d):
        return np.maximum(self.h0, 2 * (self.h0 + (self.g - 1) * d) / (self.g + 1))

    def hnf(self, rho):
        if self.g > 1: return self.hn(rho)
        s = 2 * (self.hgrad - 1) / (self.hgrad + 1)
        return self.h0 + s * np.maximum(rho - self.T, 0)

    def hnf_inverse(self, h):
        if h <= self.h0: return 0.0
        if self.g > 1: return ((self.g + 1) * h / 2 - self.h0) / (self.g - 1)
        return self.T + (h - self.h0) / (2 * (self.hgrad - 1) / (self.hgrad + 1))

    def Gc(self, d):
        h0, g = self.h0, self.g
        if g == 1: return d / h0
        if d <= h0 / 2: return d / h0
        return 0.5 + (g + 1) / (2 * (g - 1)) * math.log1p(2 * (g - 1) * (d - h0 / 2) / (h0 * (g + 1)))

    def excess(self, phi):
        c = math.cos(phi)
        if c <= 0: return math.inf
        return self.Gc(self.T / c) - self.Gc(self.T)


class Features:
    def __init__(self, P, cells, markers, walls, prof, angle=ANGLE, eps=EPS_F):
        self.P, self.prof = P, prof
        W = np.concatenate([markers[n] for n in walls]); W = ic.orient_into_fluid(W, cells, P)
        others = [n for n in markers if n not in walls]
        B = [ic.orient_into_fluid(markers[n], cells, P) for n in others]
        Bm = sum(([n] * len(markers[n]) for n in others), [])
        B = np.concatenate(B) if B else np.zeros((0, 3), np.int64)
        self.W, self.B = W, B
        allf = np.concatenate([W, B]); kind = np.array([1] * len(W) + [0] * len(B))
        fmark = sum(([n] * len(markers[n]) for n in walls), []) + Bm
        X = P[allf]; n = np.cross(X[:, 1] - X[:, 0], X[:, 2] - X[:, 0]); n /= np.linalg.norm(n, axis=1)[:, None]
        self.fn = n
        em = {}
        for i, f in enumerate(allf.tolist()):
            for j in range(3): em.setdefault(tuple(sorted((f[j], f[(j + 1) % 3]))), []).append(i)
        feats, turns, stats = [], [], {'bl_bl_edges': 0, 'bl_nonbl_edges': 0, 'convex': 0, 'concave_or_flat': 0}
        for e, fs in sorted(em.items()):
            if len(fs) != 2 or (kind[fs[0]] == 0 and kind[fs[1]] == 0): continue
            f1, f2 = fs
            stats['bl_bl_edges' if kind[f1] and kind[f2] else 'bl_nonbl_edges'] += 1
            o2 = [v for v in allf[f2] if v not in e][0]; o1 = [v for v in allf[f1] if v not in e][0]
            # exact-sign convexity: opposite vertex of each face strictly on the solid side of the other's plane
            convex = (n[f1] @ (P[o2] - P[e[0]]) < 0) and (n[f2] @ (P[o1] - P[e[0]]) < 0)
            th = math.acos(max(-1.0, min(1.0, float(n[f1] @ n[f2]))))
            if not convex: stats['concave_or_flat'] += 1; continue
            stats['convex'] += 1
            E = prof.excess(th / 2)
            if E > eps:
                feats.append(e); turns.append(math.degrees(th))
        self.stats = stats
        self.seg = np.array(feats, np.int64).reshape(-1, 2); self.turn = np.array(turns)
        # carrier planes: reflective boundary facets
        carrier = {}
        for i, f in enumerate(allf.tolist()):
            if kind[i] == 0 and str(fmark[i]) in REFLECTIVE:
                for v in f: carrier.setdefault(v, set()).add(i)
        nonrefl = set(v for i, f in enumerate(allf.tolist()) if kind[i] == 0 and str(fmark[i]) not in REFLECTIVE for v in f)
        deg = {}
        for k, (a, b) in enumerate(self.seg.tolist()):
            deg.setdefault(a, []).append((b, k)); deg.setdefault(b, []).append((a, k))
        corners, images, ends = [], [], {'carrier_ends': 0, 'carrier_end_corners': 0, 'free_end_corners': 0, 'partner_end_corners': 0}
        for v, nb in sorted(deg.items()):
            if len(nb) >= 3: corners.append(v); continue
            if len(nb) == 2:
                d1 = P[nb[0][0]] - P[v]; d2 = P[nb[1][0]] - P[v]
                turn = 180 - math.degrees(math.acos(max(-1, min(1, d1 @ d2 / np.linalg.norm(d1) / np.linalg.norm(d2)))))
                if turn >= angle: corners.append(v)
                continue
            other, k = nb[0]
            if v in carrier and v not in nonrefl:
                ends['carrier_ends'] += 1
                fi = sorted(carrier[v])[0]; nrm = n[fi]
                u = (P[v] - P[other]); u /= np.linalg.norm(u)
                turn = math.degrees(math.acos(max(-1, min(1, 2 * (u @ nrm) ** 2 - 1))))
                img = P[other] - 2 * ((P[other] - P[v]) @ nrm) * nrm
                images.append((v, img, k))
                if turn >= angle: corners.append(v); ends['carrier_end_corners'] += 1
            else:
                corners.append(v); ends['partner_end_corners' if v in nonrefl else 'free_end_corners'] += 1
        self.corners = np.array(sorted(set(corners)), np.int64); self.ends = ends
        # segment list incl. image segments: (A, B, id); canonical order: original by (min id, max id), images after
        A = [P[a] for a, b in self.seg.tolist()] + [P[v] for v, img, k in images]
        Bp = [P[b] for a, b in self.seg.tolist()] + [img for v, img, k in images]
        self.A = np.array(A).reshape(-1, 3); self.Bp = np.array(Bp).reshape(-1, 3)
        self.nimg = len(images)
        self.lines = self._chain()
        self.line_of = self._line_labels()
        # reflective carrier half-spaces (fluid side), for the domain test of sampled points
        self.halfspaces = []
        for i, f in enumerate(allf.tolist()):
            if kind[i] == 0 and str(fmark[i]) in REFLECTIVE:
                key = (round(float(n[i] @ P[f[0]]), 9), tuple(np.round(n[i], 9)))
                if key not in [h[2] for h in self.halfspaces]: self.halfspaces.append((n[i].copy(), P[f[0]].copy(), key))

    def _line_labels(self):
        adj = {}
        for k, (a, b) in enumerate(self.seg.tolist()): adj.setdefault(a, []).append(k); adj.setdefault(b, []).append(k)
        lab = -np.ones(len(self.seg), np.int64); cur = 0
        for k0 in range(len(self.seg)):
            if lab[k0] >= 0: continue
            stack = [k0]
            while stack:
                k = stack.pop()
                if lab[k] >= 0: continue
                lab[k] = cur
                for v in self.seg[k]: stack += [j for j in adj[v] if lab[j] < 0]
            cur += 1
        return lab

    def _chain(self):
        adj = {}
        for a, b in self.seg.tolist(): adj.setdefault(a, []).append(b); adj.setdefault(b, []).append(a)
        seen, lines = set(), 0
        for v in adj:
            if v in seen: continue
            lines += 1; stack = [v]
            while stack:
                x = stack.pop()
                if x in seen: continue
                seen.add(x); stack += adj[x]
        return lines


class D1Law(nl.NativeLaw):
    def __init__(self, P, faces, prof, core, feats, kv=6, kc=16):
        super().__init__(P, faces, prof.h0, prof.g, prof.T, core, kv=kv, kc=kc)
        self.prof, self.f = prof, feats
        tmax = float(np.exp(np.linalg.eigvalsh(self.vlog[np.unique(self.F)])[:, -1].max() / 2))
        self.h_tmax = tmax
        self.h_max = max(tmax, float(prof.hn(prof.T)))
        self.R_f = prof.hnf_inverse(self.h_max)
        self.L = np.linalg.norm(feats.Bp - feats.A, axis=1) if len(feats.A) else np.zeros(0)
        self.t = (feats.Bp - feats.A) / np.maximum(self.L, 1e-300)[:, None] if len(feats.A) else np.zeros((0, 3))
        self.mid = 0.5 * (feats.A + feats.Bp) if len(feats.A) else np.zeros((0, 3))
        self.stree = cKDTree(self.mid) if len(self.mid) else None
        self.Lmax = float(self.L.max()) if len(self.L) else 0.0
        self.cP = P[feats.corners] if len(feats.corners) else np.zeros((0, 3))
        self.ctree = cKDTree(self.cP) if len(self.cP) else None

    def pre_fade(self, X):
        d, n, S, face = self.frame(X)
        ax = np.argmin(np.abs(n), 1); e = np.zeros_like(n); e[np.arange(len(n)), ax] = 1
        u = np.cross(n, e); u /= np.linalg.norm(u, axis=1)[:, None]; v = np.cross(n, u)
        Bm = np.stack([u, v], 2); S2 = np.swapaxes(Bm, 1, 2) @ S @ Bm
        wt, Vt = np.linalg.eigh(S2); lt = 1 / wt; ln = 1 / self.hn(d) ** 2
        Q = Bm @ Vt
        M = (Q * lt[:, None, :]) @ np.swapaxes(Q, 1, 2) + ln[:, None, None] * n[:, :, None] * n[:, None, :]
        return M, d

    def rho_seg(self, X, k):
        A = self.f.A[k]; t = self.t[k]
        s = np.clip(((X - A) * t).sum(1), 0, self.L[k])
        return np.linalg.norm(X - (A + s[:, None] * t), axis=1)

    def compose(self, M, X, mode='all', kk=4, images=True):
        """mode 'all': every feature with rho <= R_f (canonical order); 'knn': superseded k-nearest-midpoint proxy.
        A feature with lam <= lambda_min(M) is exactly redundant (F <= lam I <= M => M∩F = M) and skipped."""
        M = M.copy(); I = np.eye(3)[None]; lmin = np.linalg.eigvalsh(M)[:, 0]
        nseg_orig = len(self.f.seg)
        if self.stree is not None:
            if mode == 'all':
                cand = self.stree.query_ball_point(X, self.R_f + 0.5 * self.Lmax)
                ids = sorted(set(i for c in cand for i in c))
            else:
                _, idx = self.stree.query(X, min(kk, len(self.mid))); idx = np.atleast_2d(idx.reshape(len(X), -1))
                ids = sorted(set(idx.ravel().tolist()))
            for k in ids:
                if not images and k >= nseg_orig: continue
                rho = self.rho_seg(X, k); lam = 1 / self.prof.hnf(rho) ** 2
                act = (rho <= self.R_f) & (lam > lmin)
                if mode == 'knn': act &= (idx == k).any(1)
                if not act.any(): continue
                t = self.t[k]
                F = lam[act][:, None, None] * (I - t[None, :, None] * t[None, None, :])
                need = np.linalg.eigvalsh(M[act] - F)[:, 0] < 0
                if need.any():
                    sub = np.where(act)[0][need]; M[sub] = nl.intersect(M[sub], F[need]); lmin[sub] = np.linalg.eigvalsh(M[sub])[:, 0]
        if self.ctree is not None:
            cand = self.ctree.query_ball_point(X, self.R_f)
            for c in sorted(set(i for cc in cand for i in cc)):
                rho = np.linalg.norm(X - self.cP[c], axis=1); lam = 1 / self.prof.hnf(rho) ** 2
                act = (rho <= self.R_f) & (lam > lmin)
                if not act.any(): continue
                F = lam[act][:, None, None] * I
                need = np.linalg.eigvalsh(M[act] - F)[:, 0] < 0
                if need.any():
                    sub = np.where(act)[0][need]; M[sub] = nl.intersect(M[sub], F[need]); lmin[sub] = np.linalg.eigvalsh(M[sub])[:, 0]
        return M

    def fade(self, M, d):
        s_ = np.clip((d - self.full) / self.width, 0, 1); wf = 1 - s_ * s_ * (3 - 2 * s_)
        w, V = np.linalg.eigh(M); lc = math.log(self.c)
        w = np.maximum(np.exp(wf[:, None] * np.log(w) + (1 - wf[:, None]) * lc), self.c)
        return (V * w[:, None, :]) @ np.swapaxes(V, 1, 2)

    def both(self, X, mode='all'):
        M, d = self.pre_fade(X)
        return self.fade(M, d), self.fade(self.compose(M, X, mode), d), d


def fluid_mask(law, X):
    d, foot, face = law.wq.query(X)
    side = ((X - foot) * law.fn[face]).sum(1)
    ok = (side > 0) | (d <= law.tol)
    for nrm, p0, _ in law.f.halfspaces: ok &= (X - p0) @ nrm >= 0
    return ok, d


def complexity_increase(law, rng, per_seg=600, chunk=20000):
    """Importance-sampled dC over tubes rho <= R_f around every segment (ownership: minimal rho over segments)."""
    f = law.f; nseg = len(f.A)
    if nseg == 0: return {'dC': 0.0, 'stderr': 0.0, 'samples': 0}
    rmin = law.prof.h0 / 100; R = law.R_f
    vals = []; norig = len(f.seg)
    def rho_all(Y):
        out = np.empty((len(Y), norig))
        for c in range(norig):
            out[:, c] = law.rho_seg(Y, c)
        return out
    for k in range(norig):
        t = law.t[k]; a = np.cross(t, [1, 0, 0] if abs(t[0]) < .9 else [0, 1, 0]); a /= np.linalg.norm(a); b = np.cross(t, a)
        s = rng.random(per_seg) * law.L[k]; lr = math.log(rmin) + rng.random(per_seg) * math.log(R / rmin)
        rho = np.exp(lr); psi = rng.random(per_seg) * 2 * math.pi
        X = f.A[k] + s[:, None] * t + rho[:, None] * (np.cos(psi)[:, None] * a + np.sin(psi)[:, None] * b)
        wvol = rho ** 2 * math.log(R / rmin) * 2 * math.pi * law.L[k]       # rho drho dpsi ds = rho^2 dln(rho) dpsi ds
        ok, d = fluid_mask(law, X); ok &= d < law.prof.T
        # ownership: this segment has the minimal rho (ties to lower id)
        if ok.any():
            idx = np.where(ok)[0]; ra = rho_all(X[idx]); own = ra.argmin(1) == k
            ok[idx[~own]] = False
        contrib = np.zeros(per_seg)
        if ok.any():
            Mb, Md, _ = law.both(X[ok])
            contrib[ok] = (np.sqrt(np.maximum(np.linalg.det(Md), 0)) - np.sqrt(np.maximum(np.linalg.det(Mb), 0))) * wvol[ok]
        vals.append(contrib)
    v = np.concatenate(vals) if vals else np.zeros(0)
    # each segment's estimate = mean over its samples; total = sum over segments
    seg_means = np.array([c.mean() for c in vals]); seg_var = np.array([c.var(ddof=1) / len(c) for c in vals])
    per_line = {}
    for k in range(norig):
        lid = int(f.line_of[k]); r = per_line.setdefault(lid, {'segments': 0, 'length': 0.0, 'dC': 0.0, 'turn_mean_deg': 0.0, 'centroid': np.zeros(3)})
        r['segments'] += 1; r['length'] += float(law.L[k]); r['dC'] += float(seg_means[k]); r['turn_mean_deg'] += float(f.turn[k])
        r['centroid'] = r['centroid'] + 0.5 * (f.A[k] + f.Bp[k])
    for r in per_line.values():
        r['turn_mean_deg'] /= r['segments']; r['centroid'] = (r['centroid'] / r['segments']).round(4).tolist()
    return {'dC': float(seg_means.sum()), 'per_line': per_line, 'stderr': float(math.sqrt(seg_var.sum())), 'samples': int(len(v)),
            'rho_min': rmin, 'R_f': R, 'note': 'region rho < h0/100 ignored (bounded by pi*(h0/100)^2*L/h0^3 per unit length)'}


def continuity(law, rng, n=400):
    out = {}
    f = law.f
    if len(f.A) == 0: return {'features': 0}
    k = rng.integers(0, len(f.A), n)
    t = law.t[k]; a = np.cross(t, np.where(np.abs(t[:, :1]) < .9, [[1, 0, 0]], [[0, 1, 0]])); a /= np.linalg.norm(a, axis=1)[:, None]
    s = rng.random(n)[:, None] * law.L[k][:, None]
    # 1. support boundary rho = R_f -/+ delta (wall distance may exceed T; then the fade hides it: report both)
    rel = []
    for sign in (-1, 1):
        rel.append(f.A[k] + s * t + (law.R_f * (1 + sign * 1e-9)) * a)
    # D1 part isolated: the same base tensor at both points (the BL transcription's own nearest-face switches are
    # not part of this check); base = the BL tensor at the inner point and the isotropic floor h_max^-2 I
    base, dd = law.pre_fade(rel[0]); inside = dd < law.prof.T
    Mm = law.compose(base, rel[0]); Mp = law.compose(base, rel[1])
    jump = np.linalg.norm(Mm - Mp, axis=(1, 2)) / np.linalg.norm(Mm, axis=(1, 2))
    Fm, Fp = law.fade(Mm, dd), law.fade(Mp, dd)
    out['support_boundary_points_d_lt_T'] = int(inside.sum())
    out['support_boundary_max_rel_jump_prefade_d_lt_T'] = float(jump[inside].max()) if inside.any() else None
    out['support_boundary_max_rel_jump_faded_all'] = float((np.linalg.norm(Fm - Fp, axis=(1, 2)) / np.linalg.norm(Fm, axis=(1, 2))).max())
    iso = np.broadcast_to(np.eye(3) / law.h_max ** 2, (n, 3, 3)).copy()
    Im = law.compose(iso, rel[0]); Ip = law.compose(iso, rel[1])
    out['support_boundary_max_rel_jump_isotropic_floor'] = float((np.linalg.norm(Im - Ip, axis=(1, 2)) / np.linalg.norm(Im, axis=(1, 2))).max())
    # 2. segment switches: points at feature-line joints displaced across the bisector, all-active vs knn
    joints = []
    adj = {}
    for i, (a_, b_) in enumerate(f.seg.tolist()): adj.setdefault(a_, []).append(i); adj.setdefault(b_, []).append(i)
    for v, segs in adj.items():
        if len(segs) == 2: joints.append((v, segs))
    jumps_all, jumps_knn = [], []
    for v, (s1, s2) in joints[:200]:
        p = law.P[v]; tt = law.t[s1] + law.t[s2]; tt /= max(np.linalg.norm(tt), 1e-300)
        off = np.cross(tt, [0, 0, 1.0] if abs(tt[2]) < .9 else [1.0, 0, 0]); off /= np.linalg.norm(off)
        for r in (law.prof.h0 * 10, law.prof.T / 3):
            # move along the line direction across the joint at fixed offset
            x = np.array([p + r * off - 1e-9 * r * tt, p + r * off + 1e-9 * r * tt])
            M0, _ = law.pre_fade(x); M0[1] = M0[0]          # same base: isolates the D1 part
            Ma = law.compose(M0, x, 'all'); Mk = law.compose(M0, x, 'knn')
            jumps_all.append(float(np.linalg.norm(Ma[0] - Ma[1]) / np.linalg.norm(Ma[0])))
            jumps_knn.append(float(np.linalg.norm(Mk[0] - Mk[1]) / np.linalg.norm(Mk[0])))
    out['segment_switch_max_rel_jump_all_active'] = max(jumps_all) if jumps_all else None
    out['segment_switch_max_rel_jump_knn_proxy'] = max(jumps_knn) if jumps_knn else None
    # random pairs anywhere near features: all-active vs knn discontinuity rate (|jump| > 1e-3 for 1e-9 moves)
    X = f.A[k] + s * t + (law.prof.T * rng.random(n))[:, None] * a
    dx = rng.normal(size=(n, 3)); dx /= np.linalg.norm(dx, axis=1)[:, None]; X2 = X + 1e-9 * law.prof.T * dx
    M1, _ = law.pre_fade(X); M2 = M1.copy()                # same base: isolates the D1 part
    ja = np.linalg.norm(law.compose(M1, X) - law.compose(M2, X2), axis=(1, 2)) / np.linalg.norm(M1, axis=(1, 2))
    jk = np.linalg.norm(law.compose(M1, X, 'knn') - law.compose(M2, X2, 'knn'), axis=(1, 2)) / np.linalg.norm(M1, axis=(1, 2))
    out['random_pairs'] = n
    out['random_pairs_jump_gt_1e-3_all_active'] = int((ja > 1e-3).sum()); out['random_pairs_jump_gt_1e-3_knn'] = int((jk > 1e-3).sum())
    # 3. carrier ends: on-plane points near each line end; reflection symmetry of the composed tensor
    #    (X = R X R  <=>  the plane normal is an eigenvector) with and without the image segment
    if f.nimg:
        base = len(f.seg); with_img, without_img = [], []
        for j in range(f.nimg):
            kk = base + j; A = f.A[kk]; img = f.Bp[kk]; other = 2 * A - img - 2 * ((A - img) - ((A - img) @ law.t[kk]) * law.t[kk]) * 0
            # plane normal: the end segment and its image are mirror images => nrm along (orig_other - img_other)
            orig_k = [i for i, (a_, b_) in enumerate(f.seg.tolist()) if np.array_equal(law.P[a_], A) or np.array_equal(law.P[b_], A)]
            if not orig_k: continue
            a_, b_ = f.seg[orig_k[0]]; oth = law.P[b_] if np.array_equal(law.P[a_], A) else law.P[a_]
            nrm = oth - img; nrm /= np.linalg.norm(nrm); Rm = np.eye(3) - 2 * np.outer(nrm, nrm)
            tang = np.cross(nrm, [1.0, 0, 0] if abs(nrm[0]) < .9 else [0, 1.0, 0]); tang /= np.linalg.norm(tang); tang2 = np.cross(nrm, tang)
            ang = rng.random(60) * 2 * math.pi; r = law.prof.T * rng.random(60) ** 2
            pts = A + r[:, None] * (np.cos(ang)[:, None] * tang + np.sin(ang)[:, None] * tang2)
            ok, d = fluid_mask(law, pts + 1e-12 * nrm); pts = pts[ok & (d < law.prof.T)]
            if not len(pts): continue
            M0 = np.broadcast_to(np.eye(3) / law.h_max ** 2, (len(pts), 3, 3)).copy()   # symmetric base isolates D1
            for imgs, acc in ((True, with_img), (False, without_img)):
                Xc = law.compose(M0, pts, images=imgs)
                acc.append(float((np.linalg.norm(Xc - Rm @ Xc @ Rm, axis=(1, 2)) / np.linalg.norm(Xc, axis=(1, 2))).max()))
        out['carrier_images'] = f.nimg
        out['carrier_end_max_rel_asymmetry_with_image'] = max(with_img) if with_img else None
        out['carrier_end_max_rel_asymmetry_without_image'] = max(without_img) if without_img else None
        out['carrier_note'] = 'D1 part on the carrier plane over the symmetric base h_max^-2 I: 0 means reflection-symmetric'
    return out


def gradation(law, rng, n=200):
    f = law.f
    if len(f.A) == 0: return {}
    k = rng.integers(0, len(f.A), n); t = law.t[k]
    a = np.cross(t, np.where(np.abs(t[:, :1]) < .9, [[1, 0, 0]], [[0, 1, 0]])); a /= np.linalg.norm(a, axis=1)[:, None]
    s = rng.random(n)[:, None] * law.L[k][:, None]
    slopes = []
    for r0, r1 in ((law.prof.h0, 2 * law.prof.h0), (0.1 * law.prof.T, 0.11 * law.prof.T), (0.5 * law.prof.T, 0.55 * law.prof.T)):
        x0 = f.A[k] + s * t + r0 * a; x1 = f.A[k] + s * t + r1 * a
        ok0, _ = fluid_mask(law, x0); ok1, _ = fluid_mask(law, x1); ok = ok0 & ok1
        if not ok.any(): slopes.append(None); continue
        x0, x1, a_ = x0[ok], x1[ok], a[ok]
        M0, _ = law.pre_fade(x0); M1, _ = law.pre_fade(x1)
        X0 = law.compose(M0, x0); X1 = law.compose(M1, x1)
        h0_ = 1 / np.sqrt(np.einsum('ni,nij,nj->n', a_, X0, a_)); h1_ = 1 / np.sqrt(np.einsum('ni,nij,nj->n', a_, X1, a_))
        slopes.append(float(np.max(np.abs(h1_ - h0_) / (r1 - r0))))
    g = law.prof.g
    return {'max_radial_size_slope_at_rho_h0_.1T_.5T': slopes,
            'hn_f_slope': 2 * (g - 1) / (g + 1) if g > 1 else 2 * (law.prof.hgrad - 1) / (law.prof.hgrad + 1)}


def run_case(case, seed=20261022):
    t0 = time.time(); rng = np.random.default_rng(seed)
    path, P, cells, mk, walls, (h0, g, T), hs, hgrad = load(case)
    res = {'case': case, 'input': path, 'input_sha256': sha(path), 'policy': {'h0': h0, 'g': g, 'T': T, 'hs': hs, 'hgrad': hgrad}}
    for label, gg in (('decided_profile', g), ('O1_g1', 1.0)):
        prof = Profile(h0, gg, T, hgrad)
        feats = Features(P, cells, mk, walls, prof)
        r = {'g': gg, 'edge_stats': feats.stats, 'features': int(len(feats.seg)), 'lines': feats.lines,
             'corners': int(len(feats.corners)), 'carrier_images': feats.nimg, 'ends': feats.ends,
             'turn_deg_min_max': [float(feats.turn.min()), float(feats.turn.max())] if len(feats.turn) else None,
             'E_threshold_turn_deg': None}
        # smallest turn that is a feature under this profile (bisection on E(phi) = eps)
        lo, hi = 0.0, math.pi / 2 - 1e-12
        for _ in range(200):
            mid = 0.5 * (lo + hi)
            lo, hi = (mid, hi) if prof.excess(mid) <= EPS_F else (lo, mid)
        r['E_threshold_turn_deg'] = math.degrees(2 * hi)
        law = D1Law(P, feats.W, prof, 1 / hs ** 2, feats)
        r.update({'h_tangential_max': law.h_tmax, 'h_max': law.h_max, 'R_f': law.R_f, 'R_f_over_T': law.R_f / T})
        if label == 'decided_profile' or len(feats.seg) <= 400:
            r['continuity'] = continuity(law, rng)
            r['gradation'] = gradation(law, rng)
            per = 600 if len(feats.A) <= 300 else max(60, int(180000 / max(len(feats.A), 1)))
            r['complexity_increase'] = complexity_increase(law, rng, per_seg=per)
        else:
            r['complexity_increase'] = {'skipped': f'{len(feats.seg)} features; per-feature sampling bounded instead',
                                        'lower_bound_dC': lower_bound_dC(law, feats, rng)}
        res[label] = r
        print(f'[{time.time()-t0:7.1f}s] {case} {label} features={len(feats.seg)} corners={len(feats.corners)} '
              f'R_f={law.R_f:.4g} dC={r["complexity_increase"].get("dC", r["complexity_increase"].get("lower_bound_dC"))}', flush=True)
    res['seconds'] = time.time() - t0
    res['max_rss_MiB'] = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024
    return res


def lower_bound_dC(law, feats, rng, nsamp=40):
    """Cheap lower bound for very large feature sets: sample nsamp segments, extrapolate the mean (estimate only)."""
    k = rng.choice(len(feats.seg), min(nsamp, len(feats.seg)), replace=False)
    sub = Features.__new__(Features); sub.__dict__.update(feats.__dict__)
    sub.A, sub.Bp, sub.nimg, sub.seg = feats.A[k], feats.Bp[k], 0, feats.seg[k]
    sl = D1Law.__new__(D1Law); sl.__dict__.update(law.__dict__); sl.f = sub
    sl.L = np.linalg.norm(sub.Bp - sub.A, axis=1); sl.t = (sub.Bp - sub.A) / sl.L[:, None]; sl.mid = 0.5 * (sub.A + sub.Bp)
    sl.stree = cKDTree(sl.mid); sl.Lmax = float(sl.L.max())
    est = complexity_increase(sl, rng, per_seg=200)
    return {'sampled_segments': int(len(k)), 'sampled_dC': est['dC'], 'stderr': est['stderr'],
            'extrapolated_dC': est['dC'] * len(feats.seg) / len(k),
            'note': 'ownership among sampled segments only; extrapolation is an order-of-magnitude estimate'}


def main():
    out = os.path.join(HERE, 'results'); os.makedirs(out, exist_ok=True)
    for case in sys.argv[1:]:
        dest = os.path.join(out, case + '.json')
        if os.path.exists(dest):
            print('resume: kept', dest); continue
        res = run_case(case)
        res['script_sha256'] = sha(__file__); res['native_law_sha256'] = sha(os.path.join(CS, 'lib', 'native_law.py'))
        json.dump(res, open(dest + '.tmp', 'w'), indent=1); os.replace(dest + '.tmp', dest)


if __name__ == '__main__':
    main()
