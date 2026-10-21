#!/usr/bin/env python3
"""WP-0(a): independent Python replay of the saved oblique-annulus joint candidate (ROBUST_JUNCTION_PLAN rev 4).

Approximate, NOT native evidence: the composed metric is re-implemented from the documented BL rule
(CBoundaryLayerMetric.hpp:47-70) for this case's uniform original sensor (core 5.0609044591818613 = original
P1 tensor), and is validated by reproducing the recorded last-sample q/J of cell 16825. Local optimisation gives a
feasible witness or the best found, never a certified optimum or an impossibility proof.
Variants: V0 saved topology (interior vertices movable as the native repair allows; wall apex keeps z=h0 and
its foot), V1 + symmetry vertex 13663 movable in its plane, V2 + surface edge 1-5 swapped to 0-13663, V3 = V1+V2.
"""
import hashlib, json, math, sys, os, platform
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CASE = os.path.normpath(os.path.join(HERE, '..', '..', 'mapped_component_geometry_v1', 'annulus_n1'))
FILES = {k: os.path.join(CASE, 'affine_annulus_bl_budget_20_m1_' + v) for k, v in {
    'after': 'rejected_rank0_trial_after.json', 'before': 'rejected_rank0_trial_before.json',
    'state': 'rejected_rank0.json', 'profile': 'frozen_profile.json', 'cfg': '.cfg'.lstrip('_')}.items()}
FILES['cfg'] = os.path.join(CASE, 'affine_annulus_bl_budget_20_m1.cfg')
H0, G, T = 0.1, 1.2, 0.3
CORE = json.load(open(FILES['profile']))['core']
QMIN, JMIN, LMAX = 0.20, 0.05, 1.8
RECORDED = {'cell': 16825, 'q': 0.19884476892564093, 'J': 0.06959269083420723}


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


after = json.load(open(FILES['after'])); before = json.load(open(FILES['before']))
names = after['marker_names']; facet_marker = {f['id']: names[f['marker']] for f in after['facets']}
WALL = [np.array([v[1:4] for v in f['vertices']], float) for f in after['facets'] if facet_marker[f['id']] == 'wall']


def closest_on_triangle(p, a, b, c):  # Ericson, real-time collision detection 5.1.5
    ab, ac, ap = b - a, c - a, p - a
    d1, d2 = ab @ ap, ac @ ap
    if d1 <= 0 and d2 <= 0: return a
    bp = p - b; d3, d4 = ab @ bp, ac @ bp
    if d3 >= 0 and d4 <= d3: return b
    vc = d1 * d4 - d3 * d2
    if vc <= 0 and d1 >= 0 and d3 <= 0: return a + d1 / (d1 - d3) * ab
    cp = p - c; d5, d6 = ab @ cp, ac @ cp
    if d6 >= 0 and d5 <= d6: return c
    vb = d5 * d2 - d1 * d6
    if vb <= 0 and d2 >= 0 and d6 <= 0: return a + d2 / (d2 - d6) * ac
    va = d3 * d6 - d5 * d4
    if va <= 0 and (d4 - d3) >= 0 and (d5 - d6) >= 0: return b + (d4 - d3) / ((d4 - d3) + (d5 - d6)) * (c - b)
    den = 1 / (va + vb + vc); return a + ab * vb * den + ac * vc * den


def metric(p):
    best = None
    for t in WALL:
        q = closest_on_triangle(p, *t); d = np.linalg.norm(p - q)
        if best is None or d < best[0]: best = (d, q)
    d, q = best
    n = (p - q) / d if d > 0 else np.array([0, 0, 1.])
    hn = max(H0, 2 * (H0 + (G - 1) * d) / (G + 1))
    full = max(H0, 0.9 * T)
    if d <= full: w = 1.0
    elif d >= T: w = 0.0
    else:
        x = (d - full) / (T - full); w = 1 - (3 * x * x - 2 * x ** 3)
    lam = math.exp(w * math.log(1 / hn ** 2) + (1 - w) * math.log(CORE)) if w > 0 else CORE
    lam = max(lam, CORE)  # intersection with the isotropic sensor; wall tangential sizes are weaker than core here
    return CORE * np.eye(3) + (lam - CORE) * np.outer(n, n)


def measure(X):
    c = X.mean(0); L = np.linalg.cholesky(metric(c)).T
    Y = (X - X[0]) @ L.T
    det = np.linalg.det(np.array([Y[1], Y[2], Y[3]]))
    e = [Y[j] - Y[i] for i in range(4) for j in range(i + 1, 4)]
    sq = sum(v @ v for v in e)
    q = math.copysign(12 * (abs(det) / 2) ** (2 / 3) / sq, det)
    jac = 1.0
    for k in range(4):
        den = 1.0
        for j in range(4):
            if j != k: den *= np.linalg.norm(Y[j] - Y[k])
        jac = min(jac, math.sqrt(2) * abs(det) / den)
    return q, math.copysign(jac, det)


GL = np.polynomial.legendre.leggauss(16)


def edge_length(a, b):
    s = 0.0; e = b - a
    for x, w in zip(*GL):
        p = a + (x + 1) / 2 * e; s += w / 2 * math.sqrt(e @ metric(p) @ e)
    return s


class Mesh:
    def __init__(self, data):
        self.P = {n[0]: np.array(n[1:4], float) for n in data['nodes']}
        self.cells = [list(c['nodes']) for c in data['cells']]
        self.surface = [(tuple(s['nodes']), facet_marker[s['facet']]) for s in data['surface']]
        for c in self.cells:  # native orientation convention: positive det
            if self.det(c) < 0: c[0], c[1] = c[1], c[0]

    def det(self, c):
        X = [self.P[i] for i in c]; return np.linalg.det(np.array([X[1] - X[0], X[2] - X[0], X[3] - X[0]]))

    def evaluate(self):
        out = []
        for c in self.cells:
            if self.det(c) <= 0: return None
            q, j = measure(np.array([self.P[i] for i in c])); out.append((q, j, c))
        return out


def loss(ev):
    return sum(max(0, 1 - q / QMIN) ** 2 + max(0, 1 - j / JMIN) ** 2 for q, j, _ in ev)


def worst(ev):
    return min(min(q / QMIN, j / JMIN) for q, j, _ in ev)


SYM_N = np.array([-0.37, 1.0, -0.61]); SYM_N /= np.linalg.norm(SYM_N)
SYM_U = np.array([1, 0.37, 0.0]); SYM_U /= np.linalg.norm(SYM_U); SYM_V = np.cross(SYM_N, SYM_U)
WALL_FACE = (0, 1, 9)


def foot_inside(p, mesh):
    a, b, c = (mesh.P[i][:2] for i in WALL_FACE); x = p[:2]
    def cr(u, v, w): return (v[0] - u[0]) * (w[1] - u[1]) - (v[1] - u[1]) * (w[0] - u[0])
    s = [cr(a, b, x), cr(b, c, x), cr(c, a, x)]; return all(v >= 0 for v in s) or all(v <= 0 for v in s)


def sym_faces_ok(mesh, vid):
    # In-plane orientation of every symmetry face around vid must keep its sign (no surface fold).
    for f, m in mesh.surface:
        if m != 'symmetry' or vid not in f: continue
        a, b, c = (mesh.P[i] for i in f)
        if np.cross(b - a, c - a) @ SYM_N * mesh.sym_sign[f] <= 0: return False
    return True


def optimise(mesh, free, seeds=6, iterations=4000, rng_seed=1):
    """free: list of (vid, kind) with kind 'xyz', 'wallapex' (x,y at z=h0, foot inside), 'sym' (in-plane)."""
    base = {v: mesh.P[v].copy() for v, _ in free}
    dims = [3 if k == 'xyz' else 2 for _, k in free]
    def apply(x):
        o = 0
        for (v, k), d in zip(free, dims):
            y = x[o:o + d]; o += d
            if k == 'xyz': mesh.P[v] = base[v] + y
            elif k == 'wallapex': mesh.P[v] = base[v] + np.array([y[0], y[1], 0.0])
            else: mesh.P[v] = base[v] + y[0] * SYM_U + y[1] * SYM_V
    def objective(x):
        apply(x)
        for (v, k) in free:
            if k == 'wallapex' and not foot_inside(mesh.P[v], mesh): return 1e9, None
            if k == 'sym' and not sym_faces_ok(mesh, v): return 1e9, None
        ev = mesh.evaluate()
        if ev is None: return 1e9, None
        return loss(ev) - 1e-3 * worst(ev), ev
    n = sum(dims); rng = np.random.default_rng(rng_seed)
    best_x, (best_f, best_ev) = np.zeros(n), objective(np.zeros(n))
    for s in range(seeds):
        x = best_x.copy() if s == 0 else best_x + rng.normal(0, 0.03, n)
        f, ev = objective(x); step = 0.05
        if f >= 1e9: continue
        for it in range(iterations):
            y = x + rng.normal(0, step, n); g, evy = objective(y)
            if g < f: x, f, ev = y, g, evy; step = min(step * 1.5, 0.1)
            else: step *= 0.997
            if step < 1e-7: break
        if f < best_f: best_x, best_f, best_ev = x, f, ev
    objective(best_x)
    return best_f, best_ev


def swap_edge(mesh, a, b, apex_pair):
    """Boundary 2-2 swap of surface edge (a,b) to apex_pair=(r0,rm) with a fan re-coning of its volume shell."""
    shell = [c for c in mesh.cells if a in c and b in c]
    ring_edges = [tuple(sorted(set(c) - {a, b})) for c in shell]
    r0, rm = apex_pair; order = [r0]
    while order[-1] != rm:
        nxt = [e for e in ring_edges if order[-1] in e and (len(order) < 2 or order[-2] not in e)]
        if len(nxt) != 1: raise RuntimeError('non-manifold shell')
        order.append(nxt[0][0] if nxt[0][1] == order[-1] else nxt[0][1])
    new = []
    for i in range(1, len(order) - 1):
        for apex in (a, b):
            c = [apex, order[0], order[i], order[i + 1]]
            if mesh.det(c) < 0: c[0], c[1] = c[1], c[0]
            new.append(c)
    mesh.cells = [c for c in mesh.cells if c not in shell] + new
    surf = [(f, m) for f, m in mesh.surface if not (a in f and b in f)]
    for f in [(a, r0, rm), (b, r0, rm)]:
        surf.append((f, 'symmetry'))
    mesh.surface = surf
    for f, mk in mesh.surface:
        if mk == 'symmetry' and f not in mesh.sym_sign:
            x, y, z = (mesh.P[i] for i in f); mesh.sym_sign[f] = math.copysign(1, np.cross(y - x, z - x) @ SYM_N)
    return len(shell), len(new), order


def report(tag, ev, mesh):
    q = min(e[0] for e in ev); j = min(e[1] for e in ev)
    edges = set()
    for c in mesh.cells:
        for i in range(4):
            for k in range(i + 1, 4): edges.add(tuple(sorted((c[i], c[k]))))
    lmax = max(edge_length(mesh.P[a], mesh.P[b]) for a, b in edges)
    bad = sorted([(round(e[0], 4), round(e[1], 4), e[2]) for e in ev if e[0] < QMIN or e[1] < JMIN])
    # REFINE progress (CNativeMetric3D.cpp:319-337): an edge is shared only with identical ids AND coordinates;
    # moved vertices receive fresh identities, so their edges count as new.
    old = Mesh(before); oe = {}; ne = {}
    for c in old.cells:
        for i in range(4):
            for k in range(i + 1, 4):
                a, b = sorted((c[i], c[k])); oe[(a, b)] = (old.P[a], old.P[b])
    base = Mesh(after)
    for a, b in edges:
        ne[(a, b)] = (mesh.P[a], mesh.P[b])
    def shared(key, ends, other):
        o = other.get(key); return o is not None and all(np.array_equal(x, y) for x, y in zip(ends, o))
    ex = lambda L: max(0.0, L - LMAX) ** 2
    removed = sum(ex(edge_length(*v)) for k, v in oe.items() if not shared(k, v, ne))
    added = sum(ex(edge_length(*v)) for k, v in ne.items() if not shared(k, v, oe))
    refine = len(mesh.cells) > len(old.cells) and removed > 0 and added < removed * (1 - 1e-12)
    return {'variant': tag, 'q_min': q, 'J_min': j, 'shape_gate_pass': q >= QMIN and j >= JMIN,
            'failing_cells': bad[:8], 'n_failing': len(bad),
            'refine_progress': {'removed_excess': removed, 'added_excess': added, 'cells_old': len(old.cells),
                                'cells_new': len(mesh.cells), 'passes': refine}, 'max_edge_length_full_integral_approx': lmax,
            'edges_over_1.8': sum(edge_length(mesh.P[a], mesh.P[b]) > LMAX for a, b in edges),
            'coords': {str(v): mesh.P[v].tolist() for v in sorted(mesh.P) if v >= 13663}}


def fresh():
    m = Mesh(after); m.sym_sign = {}
    for f, mk in m.surface:
        if mk == 'symmetry':
            a, b, c = (m.P[i] for i in f); m.sym_sign[f] = math.copysign(1, np.cross(b - a, c - a) @ SYM_N)
    return m


def main():
    out = {'provenance': {k: {'path': os.path.relpath(v, HERE), 'sha256': sha(v)} for k, v in FILES.items()},
           'script_sha256': sha(os.path.abspath(__file__)), 'python': sys.version, 'numpy': np.__version__,
           'host': platform.node(), 'model': 'documented BL rule + uniform core; approximate, not native',
           'gates': {'q': QMIN, 'J': JMIN, 'L': LMAX}}
    m = fresh(); ev = m.evaluate(); rec = [e for e in ev if set(e[2]) == {16809, 1, 13663, 5}][0]
    out['model_validation'] = {'cell_16825_q': rec[0], 'cell_16825_J': rec[1], 'recorded': RECORDED,
                               'abs_err_q': abs(rec[0] - RECORDED['q']), 'abs_err_J': abs(rec[1] - RECORDED['J'])}
    out['saved'] = report('saved candidate (no optimisation)', ev, m)
    interior = [(16808, 'wallapex'), (16809, 'xyz'), (16810, 'xyz'), (16811, 'xyz')]
    results = []
    for tag, extra, swap in [('V0 interior only', [], False), ('V1 +13663 in-plane', [(13663, 'sym')], False),
                             ('V2 +swap 1-5 -> 0-13663', [], True), ('V3 = V1 + V2', [(13663, 'sym')], True)]:
        m = fresh(); info = None
        if swap: info = swap_edge(m, 1, 5, (0, 13663))
        f, ev = optimise(m, interior + extra)
        if ev is None: results.append({'variant': tag, 'status': 'no valid state found', 'swap': info}); continue
        r = report(tag, ev, m); r['loss'] = f; r['swap_shell_old_new_ring'] = info; results.append(r)
        print(tag, 'q_min %.4f J_min %.4f pass %s Lmax %.3f refine %s' % (r['q_min'], r['J_min'], r['shape_gate_pass'],
              r['max_edge_length_full_integral_approx'], r['refine_progress']), flush=True)
    out['variants'] = results
    json.dump(out, open(os.path.join(HERE, 'result.json'), 'w'), indent=1, default=str)
    print(json.dumps(out['model_validation']))


if __name__ == '__main__':
    main()
