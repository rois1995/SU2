#!/usr/bin/env python3
"""DC-4 (BL_REMESH_PLAN rev 3): target-layout quality of the coordinated first row (D.3) on the FULL wall.

Proxy only: native-law transcription + D1 (bl_remesh_v1/dc1/dc1_d1law.py), isotropic core proxy; native gates and
connectivity are NOT evaluated (DC-5 decides realizability). The layout is the D.3 target layout:
  face cells     (a, b, c, p_f)                      p_f: foot/height inside T (CheckFirstHeight semantics)
  edge cells     (a, b, p_f, p_g)                    interior non-feature edges
  crease fans    (a, b, x_i, x_i+1), i = 0..m        feature edges, m = ceil(turn*h0/hn_f(h0)) - 1 apices at
                                                     radius ~h0 from the crease, angles interpolated n_f -> n_g
  vertex cells   (v, q_i, q_i+1, c_s)                ring of first-row points around v (p_f and fan apices),
                                                     one c_s per sector (sectors split at feature edges)
  rim cells      (a, b, p_f, r_ab)                   wall edges on a reflective carrier; r_ab on the plane
Free variables from deterministic finite candidate sets, chosen by coordinate ascent of the minimum q of the cells
each variable touches (p_f, then c_s, then p_f again, then r_ab). Every target cell must be positive; q/J by the
native mean-ratio/scaled-Jacobian convention at the actual centroid. Output: per class q/J distribution, cells below
q .20 / J .05 at the best candidates (refusal classes), localisation (TE/tip/LE/root/smooth by feature proximity).
Usage: dc4_layout.py <case> [...]   (m6, tube_root_z, tube_root_x, two_tube_y)
"""
import hashlib, importlib.util, json, math, os, resource, sys, time
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('dc1', os.path.join(HERE, '..', 'dc1', 'dc1_d1law.py'))
dc1 = importlib.util.module_from_spec(spec); spec.loader.exec_module(dc1)
ic = dc1.ic
QMIN, JMIN = 0.20, 0.05
HEIGHTS = (0.92, 1.0, 1.08)                        # |h/h0 - 1| <= 0.08 < T = 0.10
FOOT = ((1/3, 1/3, 1/3), (.5, .25, .25), (.25, .5, .25), (.25, .25, .5))
CV_STEPS = (0.8, 1.0, 1.25, 1.6, 2.0, 2.6)          # c_s = v + beta*h0*n_s
FAN_H = (1.0, 1.5, 2.0)                             # geometric fan apex above a convex edge: mid + eta*h0*bisector


def sha(p):
    return hashlib.sha256(open(p, 'rb').read()).hexdigest()


def quality(law, X, chunk=40000):
    q = np.empty(len(X)); J = np.empty(len(X))
    for s in range(0, len(X), chunk):
        Y = X[s:s + chunk]; C = Y.mean(1)
        M0, d = law.pre_fade(C); M = law.fade(law.compose(M0, C), d)
        q[s:s + chunk], J[s:s + chunk] = ic.mean_ratio_jac(Y, M)
    return q, J


def run(case):
    t0 = time.time(); log = lambda *a: print(f'[{time.time()-t0:7.1f}s]', case, *a, flush=True)
    path, P, cells, mk, walls, (h0, g, T), hs, hgrad = dc1.load(case)
    prof = dc1.Profile(h0, g, T, hgrad); f = dc1.Features(P, cells, mk, walls, prof)
    law = dc1.D1Law(P, f.W, prof, 1 / hs ** 2, f); W = f.W; nF = len(W)
    n = np.cross(P[W[:, 1]] - P[W[:, 0]], P[W[:, 2]] - P[W[:, 0]]); area = np.linalg.norm(n, axis=1); n /= area[:, None]
    log('faces', nF, 'features', len(f.seg), 'corners', len(f.corners), 'R_f', law.R_f)
    featset = set(map(tuple, np.sort(f.seg, 1).tolist())) if len(f.seg) else set()
    # adjacency
    edge_faces = {}
    for i, tri in enumerate(W.tolist()):
        for k in range(3): edge_faces.setdefault(tuple(sorted((tri[k], tri[(k + 1) % 3]))), []).append(i)
    # reflective carrier planes and wall edges on them
    planes = [(nrm, p0) for nrm, p0, _ in f.halfspaces]
    def on_plane(v):
        return [j for j, (nrm, p0) in enumerate(planes) if abs((P[v] - p0) @ nrm) <= 1e-9 * max(1, np.abs(P[v]).max())]
    # ---- p_f candidates -------------------------------------------------------------------------------------------
    cand = []
    for fb in FOOT:
        foot = fb[0] * P[W[:, 0]] + fb[1] * P[W[:, 1]] + fb[2] * P[W[:, 2]]
        for hh in HEIGHTS: cand.append(foot + hh * h0 * n)
    cand = np.stack(cand, 1)                                                  # (nF, nc, 3)
    nc = cand.shape[1]
    Xf = np.concatenate([np.repeat(P[W][:, None], nc, 1), cand[:, :, None]], 2).reshape(-1, 4, 3)
    qf, Jf = quality(law, Xf); qf = qf.reshape(nF, nc); Jf = Jf.reshape(nF, nc)
    choice = np.argmax(np.minimum(qf / QMIN, Jf / JMIN), 1); pf = cand[np.arange(nF), choice]
    log('face cells scored')
    # ---- crease fans and edge cells -------------------------------------------------------------------------------
    edge_cells, edge_kind, fan_pts, plain_edges = [], [], {}, []
    hnf_h0 = float(prof.hnf(h0))
    rims = []
    def directed(a, b, fi):
        tri = W[fi].tolist(); k = tri.index(a); return tri[(k + 1) % 3] == b
    # orientation convention: (a, b) directed as in the left face f1, so (a, b, left apex, right apex) is positive
    for e, fs in edge_faces.items():
        if len(fs) == 2:
            a, b = e; f1, f2 = fs
            if not directed(a, b, f1): f1, f2 = f2, f1
            if not directed(a, b, f1): a, b = b, a
            if e in featset:
                th = math.acos(max(-1, min(1, float(n[f1] @ n[f2]))))
                m = max(0, math.ceil(th * h0 / hnf_h0) - 1)
                mid = 0.5 * (P[a] + P[b]); pts = [pf[f1]]
                for i in range(1, m + 1):
                    s = i / (m + 1); v = (1 - s) * n[f1] + s * n[f2]; v /= np.linalg.norm(v)
                    pts.append(mid + h0 * v)
                pts.append(pf[f2]); fan_pts[e] = (f1, f2, pts[1:-1])
                for i in range(len(pts) - 1): edge_cells.append((P[a], P[b], pts[i], pts[i + 1])); edge_kind.append(1)
            else:
                plain_edges.append((a, b, f1, f2))
        elif len(fs) == 1:
            a, b = e; pa, pb = on_plane(a), on_plane(b); common = set(pa) & set(pb)
            if not directed(a, b, fs[0]): a, b = b, a
            if common:
                nrm, p0 = planes[sorted(common)[0]]; f1 = fs[0]
                d = n[f1] - (n[f1] @ nrm) * nrm; d /= max(np.linalg.norm(d), 1e-300)
                rims.append((a, b, f1, 0.5 * (P[a] + P[b]) + h0 * d))
    # Non-feature interior edges: plain cell (a,b,p_f,p_g) or, at convex edges, a geometric fan through apex
    # candidates above the edge (bisector, heights FAN_H*h0); the better min score wins (finite candidate set).
    if plain_edges:
        A_ = np.array([P[a] for a, b, f1, f2 in plain_edges]); B_ = np.array([P[b] for a, b, f1, f2 in plain_edges])
        F1 = np.array([f1 for a, b, f1, f2 in plain_edges]); F2 = np.array([f2 for a, b, f1, f2 in plain_edges])
        q0, J0 = quality(law, np.stack([A_, B_, pf[F1], pf[F2]], 1)); s0 = np.minimum(q0 / QMIN, J0 / JMIN)
        opp = np.array([[v for v in W[f2] if v not in (a, b)][0] for a, b, f1, f2 in plain_edges])
        convex = ((P[opp] - A_) * n[F1]).sum(1) < 0
        bis = n[F1] + n[F2]; bis /= np.linalg.norm(bis, axis=1)[:, None]; mid = 0.5 * (A_ + B_)
        best_s = s0.copy(); best_x = np.full((len(A_), 3), np.nan)
        for hh in FAN_H:
            x = mid + hh * h0 * bis
            q1, J1 = quality(law, np.stack([A_, B_, pf[F1], x], 1)); q2, J2 = quality(law, np.stack([A_, B_, x, pf[F2]], 1))
            sc = np.minimum(np.minimum(q1, q2) / QMIN, np.minimum(J1, J2) / JMIN)
            better = convex & (sc > best_s); best_s[better] = sc[better]; best_x[better] = x[better]
        geometric_fans = 0
        for k, (a, b, f1, f2) in enumerate(plain_edges):
            if np.isfinite(best_x[k, 0]):
                geometric_fans += 1; fan_pts[(a, b) if a < b else (b, a)] = (f1, f2, [best_x[k]])
                edge_cells.append((P[a], P[b], pf[f1], best_x[k])); edge_kind.append(2)
                edge_cells.append((P[a], P[b], best_x[k], pf[f2])); edge_kind.append(2)
            else:
                edge_cells.append((P[a], P[b], pf[f1], pf[f2])); edge_kind.append(0)
        fold_stats = {'interior_nonfeature_edges': len(plain_edges), 'convex': int(convex.sum()),
                      'plain_cell_nonpositive': int((q0 <= 0).sum()), 'plain_cell_nonpositive_convex': int(((q0 <= 0) & convex).sum()),
                      'geometric_fans_chosen': geometric_fans}
    else: fold_stats = {}
    E = np.array([np.stack(c) for c in edge_cells]) if edge_cells else np.zeros((0, 4, 3))
    qe, Je = quality(law, E) if len(E) else (np.zeros(0), np.zeros(0))
    edge_kind = np.array(edge_kind)
    log('edge/fan cells', len(E))
    R = np.array([np.stack([P[a], P[b], pf[f1], r]) for a, b, f1, r in rims]) if rims else np.zeros((0, 4, 3))
    qr, Jr = quality(law, R) if len(R) else (np.zeros(0), np.zeros(0))
    # ---- vertex rings, sectors, c_s -------------------------------------------------------------------------------
    vfaces = {}
    for i, tri in enumerate(W.tolist()):
        for v in tri: vfaces.setdefault(v, []).append(i)
    ring_cells, ring_owner, ring_kind = [], [], []                          # (v, q_i, q_i+1, sector-id)
    sector_dir, sector_v = [], []
    skipped = {'open_ring_non_carrier': 0}
    for v, fl in vfaces.items():
        # order faces around v by shared edges
        nbr = {}
        for i in fl:
            tri = W[i].tolist(); k = tri.index(v); a, b = tri[(k + 1) % 3], tri[(k + 2) % 3]   # oriented: v->a->b
            nbr[i] = (a, b)
        by_a = {}; [by_a.setdefault(ab[0], i) for i, ab in nbr.items()]
        start = fl[0]
        prev_of = {nbr[i][1]: i for i in fl}
        open_ring = any(nbr[i][0] not in prev_of for i in fl)
        if open_ring:
            starts = [i for i in fl if nbr[i][0] not in prev_of]
            if not on_plane(v): skipped['open_ring_non_carrier'] += 1; continue
            start = starts[0]
        order, i = [], start
        while i is not None and i not in order:
            order.append(i); i = by_a.get(nbr[i][1])
        # sectors split at feature edges
        sec = [0] * len(order); s = 0
        for j in range(1, len(order)):
            e = tuple(sorted((v, nbr[order[j]][0])))
            if e in featset: s += 1
            sec[j] = s
        closing = (not open_ring) and tuple(sorted((v, nbr[order[0]][0]))) in featset
        if not open_ring and not closing and s > 0:
            sec = [x if x > 0 else s for x in sec]                         # wrap: first run joins the last sector
        if not open_ring and s == 0: pass
        nsec_ids = sorted(set(sec)); base = len(sector_dir)
        for sid in nsec_ids:
            N = n[[order[j] for j in range(len(order)) if sec[j] == sid]]
            w = ic.maxmin_direction(N)[0] if len(N) > 1 else N[0]
            if w is None: w = N.sum(0); skipped['sector_without_open_hemisphere'] = skipped.get('sector_without_open_hemisphere', 0) + 1
            w = np.asarray(w, float); w /= np.linalg.norm(w)
            if open_ring:                                                  # junction c_s on the carrier
                nrm, p0 = planes[on_plane(v)[0]]; w = w - (w @ nrm) * nrm; w /= np.linalg.norm(w)
            sector_dir.append(w); sector_v.append(v)
        sid_map = {sid: base + k for k, sid in enumerate(nsec_ids)}
        # ring of first-row points with fan apices across feature edges
        L = len(order) if not open_ring else len(order) - 1
        for j in range(L):
            fi, fj = order[j], order[(j + 1) % len(order)]
            e = tuple(sorted((v, nbr[fi][1])))
            si, sj = sid_map[sec[j]], sid_map[sec[(j + 1) % len(order)]]
            if e in fan_pts:
                f1, f2, apex = fan_pts[e]
                seq = [pf[fi]] + (apex if f1 == fi else apex[::-1]) + [pf[fj]]
                half = len(seq) // 2
                for k in range(len(seq) - 1):
                    ring_cells.append((v, seq[k], seq[k + 1])); ring_owner.append(si if k < half else sj); ring_kind.append(1)
                if si != sj:   # closure between two sector points only where the fan separates sectors
                    ring_cells.append((v, seq[half], None)); ring_owner.append((si, sj)); ring_kind.append(2)
            else:
                ring_cells.append((v, pf[fi], pf[fj])); ring_owner.append(si); ring_kind.append(0)
        if open_ring:
            # junction ends: rim points of the two carrier edges at v
            for first, fi in ((True, order[0]), (False, order[-1])):
                rr = [r for a, b, f1, r in rims if f1 == fi and v in (a, b)]
                if rr:   # CCW ring: the rim precedes the first face and follows the last one
                    ring_cells.append((v, rr[0], pf[fi]) if first else (v, pf[fi], rr[0]))
                    ring_owner.append(sid_map[sec[0] if first else sec[-1]]); ring_kind.append(3)
    nS = len(sector_dir); sector_dir = np.array(sector_dir); sector_v = np.array(sector_v)
    log('vertex ring cells', len(ring_cells), 'sectors', nS, skipped)
    # c_s candidates: score each candidate on all ring cells it owns, keep the best (coordinate ascent step)
    best_c = np.zeros((nS, 3)); best_score = np.full(nS, -np.inf)
    simple = [k for k, o in enumerate(ring_owner) if not isinstance(o, tuple)]
    own = np.array([ring_owner[k] for k in simple]); Vv = np.array([P[ring_cells[k][0]] for k in simple])
    Q1 = np.array([ring_cells[k][1] for k in simple]); Q2 = np.array([ring_cells[k][2] for k in simple])
    for beta in CV_STEPS:
        C = P[sector_v] + beta * h0 * sector_dir
        X = np.stack([Vv, Q1, Q2, C[own]], 1)
        q, J = quality(law, X); sc = np.minimum(q / QMIN, J / JMIN)
        per = np.full(nS, np.inf); np.minimum.at(per, own, sc)
        better = per > best_score; best_score[better] = per[better]; best_c[better] = C[better]
    X = np.stack([Vv, Q1, Q2, best_c[own]], 1); qv, Jv = quality(law, X)
    clos = [k for k, o in enumerate(ring_owner) if isinstance(o, tuple)]
    if clos:
        Xc = np.array([np.stack([P[ring_cells[k][0]], ring_cells[k][1], best_c[ring_owner[k][1]], best_c[ring_owner[k][0]]]) for k in clos])
        qc, Jc = quality(law, Xc)
    else: qc = Jc = np.zeros(0)
    log('vertex cells scored')
    # ---- report --------------------------------------------------------------------------------------------------
    def summary(q, J):
        if not len(q): return {'n': 0}
        bad = (q < QMIN) | (J < JMIN)
        return {'n': int(len(q)), 'nonpositive': int((q <= 0).sum()), 'q_min': float(q.min()), 'q_p01': float(np.percentile(q, 1)),
                'q_median': float(np.median(q)), 'J_min': float(J.min()), 'below_gate': int(bad.sum()), 'below_gate_fraction': float(bad.mean())}
    qbest = qf[np.arange(nF), choice]; Jbest = Jf[np.arange(nF), choice]
    rk = np.array(ring_kind)[simple]
    res = {'case': case, 'input_sha256': sha(path), 'faces': nF, 'features': int(len(f.seg)), 'corners': int(len(f.corners)),
           'R_f': law.R_f, 'skipped': skipped,
           'face_cells': summary(qbest, Jbest), 'edge_cells': summary(qe[edge_kind == 0], Je[edge_kind == 0]),
           'crease_fan_cells': summary(qe[edge_kind == 1], Je[edge_kind == 1]), 'geometric_fan_cells': summary(qe[edge_kind == 2], Je[edge_kind == 2]),
           'fold_stats': fold_stats, 'rim_cells': summary(qr, Jr),
           'vertex_cells_plain': summary(qv[rk == 0], Jv[rk == 0]), 'vertex_cells_fan': summary(qv[rk == 1], Jv[rk == 1]),
           'vertex_cells_junction': summary(qv[rk == 3], Jv[rk == 3]), 'fan_closure_cells': summary(qc, Jc)}
    # localisation of failures by distance to features / carrier
    def where(points):
        if not len(points): return {}
        out = {}
        dfeat = np.min(np.stack([law.rho_seg(points, k) for k in range(len(f.seg))], 1), 1) if len(f.seg) else np.full(len(points), np.inf)
        dpl = np.min(np.stack([np.abs((points - p0) @ nrm) for nrm, p0 in planes], 1), 1) if planes else np.full(len(points), np.inf)
        out['near_feature_(<10h0)'] = int((dfeat < 10 * h0).sum()); out['near_carrier_(<10h0)'] = int(((dpl < 10 * h0) & (dfeat >= 10 * h0)).sum())
        out['smooth'] = int(((dfeat >= 10 * h0) & (dpl >= 10 * h0)).sum())
        return out
    fails = []
    for X_, q_, J_ in ((Xf.reshape(nF, nc, 4, 3)[np.arange(nF), choice], qbest, Jbest), (E, qe, Je), (R, qr, Jr), (X, qv, Jv)):
        if len(q_): fails.append(X_[(q_ < QMIN) | (J_ < JMIN)].mean(1))
    fails = np.concatenate(fails) if fails else np.zeros((0, 3))
    res['below_gate_localisation'] = where(fails)
    res['total_cells'] = int(nF + len(E) + len(R) + len(qv) + len(qc))
    res['total_below_gate'] = int(len(fails) + int(((qc < QMIN) | (Jc < JMIN)).sum()))
    res['seconds'] = time.time() - t0; res['max_rss_MiB'] = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024
    res['script_sha256'] = sha(__file__); res['dc1_law_sha256'] = sha(os.path.join(HERE, '..', 'dc1', 'dc1_d1law.py'))
    return res


def main():
    out = os.path.join(HERE, 'results'); os.makedirs(out, exist_ok=True)
    for case in sys.argv[1:]:
        dest = os.path.join(out, case + '.json')
        if os.path.exists(dest): print('resume: kept', dest); continue
        res = run(case); json.dump(res, open(dest + '.tmp', 'w'), indent=1); os.replace(dest + '.tmp', dest)
        print(json.dumps(res, indent=1), flush=True)


if __name__ == '__main__':
    main()
