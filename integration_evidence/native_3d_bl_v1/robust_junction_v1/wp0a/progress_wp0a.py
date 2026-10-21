#!/usr/bin/env python3
"""WP-0(a) follow-up: does any local witness pass shape AND REFINE progress (added < removed changed-edge excess)?
Same approximate model as replay_wp0a.py (imported unchanged). Objective = shape loss + 10*max(0, added-removed).
Optimisation uses 8-point Gauss edge integrals; the reported state is re-measured with the 16-point rule.
Best found only; not a certified optimum or impossibility proof."""
import importlib.util, json, os, sys, hashlib
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('r', os.path.join(HERE, 'replay_wp0a.py'))
r = importlib.util.module_from_spec(spec); spec.loader.exec_module(r)

GL8 = np.polynomial.legendre.leggauss(8)
old = r.Mesh(r.before); OE = {}
for c in old.cells:
    for i in range(4):
        for k in range(i + 1, 4):
            a, b = sorted((c[i], c[k])); OE[(a, b)] = (old.P[a].copy(), old.P[b].copy())


def length8(a, b):
    e = b - a; return sum(w / 2 * np.sqrt(e @ r.metric(a + (x + 1) / 2 * e) @ e) for x, w in zip(*GL8))


OLD8 = {k: length8(*v) for k, v in OE.items()}


def progress(mesh, L=length8):
    ne = {}
    for c in mesh.cells:
        for i in range(4):
            for k in range(i + 1, 4):
                a, b = sorted((c[i], c[k])); ne[(a, b)] = (mesh.P[a], mesh.P[b])
    sh = lambda k, v, o: k in o and all(np.array_equal(x, y) for x, y in zip(v, o[k]))
    ex = lambda x: max(0.0, x - r.LMAX) ** 2
    removed = sum(ex(OLD8[k] if L is length8 else L(*v)) for k, v in OE.items() if not sh(k, v, ne))
    added = sum(ex(L(*v)) for k, v in ne.items() if not sh(k, v, OE))
    return removed, added


orig_loss = r.loss
def combined_objective_factory(mesh):
    def f(ev):
        rem, add = progress(mesh); return orig_loss(ev) + 10 * max(0.0, add - rem * (1 - 1e-12))
    return f


out = {'script_sha256': hashlib.sha256(open(os.path.abspath(__file__), 'rb').read()).hexdigest(),
       'replay_sha256': hashlib.sha256(open(os.path.join(HERE, 'replay_wp0a.py'), 'rb').read()).hexdigest(),
       'variants': []}
interior = [(16808, 'wallapex'), (16809, 'xyz'), (16810, 'xyz'), (16811, 'xyz')]
for tag, extra, swap in [('P0 interior only', [], False), ('P3 interior + 13663 in-plane + swap 1-5', [(13663, 'sym')], True)]:
    m = r.fresh()
    if swap: r.swap_edge(m, 1, 5, (0, 13663))
    r.loss = combined_objective_factory(m)
    f, ev = r.optimise(m, interior + extra, seeds=3, iterations=1500)
    r.loss = orig_loss
    if ev is None:
        out['variants'].append({'variant': tag, 'status': 'no valid state'}); continue
    rem, add = progress(m, L=r.edge_length)
    q = min(e[0] for e in ev); j = min(e[1] for e in ev)
    row = {'variant': tag, 'q_min': q, 'J_min': j, 'shape_pass': bool(q >= r.QMIN and j >= r.JMIN),
           'removed_excess_16pt': rem, 'added_excess_16pt': add, 'refine_pass': bool(add < rem * (1 - 1e-12)),
           'coords': {str(v): m.P[v].tolist() for v, _ in interior + extra}}
    out['variants'].append(row); print(json.dumps(row)[:400], flush=True)
json.dump(out, open(os.path.join(HERE, 'progress_result.json'), 'w'), indent=1)
