#!/usr/bin/env python3
"""DC-5 input: export the DC-1 D1 feature set (decided profile) of each fixture for the test-local C++ law."""
import importlib.util, os, sys, numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('dc1', os.path.join(HERE, '..', 'dc1', 'dc1_d1law.py'))
dc1 = importlib.util.module_from_spec(spec); spec.loader.exec_module(dc1)
for case in sys.argv[1:]:
    path, P, cells, mk, walls, (h0, g, T), hs, hgrad = dc1.load(case)
    prof = dc1.Profile(h0, g, T, hgrad); f = dc1.Features(P, cells, mk, walls, prof)
    law = dc1.D1Law(P, f.W, prof, 1 / hs ** 2, f)
    # marker of each feature: a wall face of the given marker containing the edge (original segments), images inherit
    wm = sum(([n] * len(mk[n]) for n in walls), [])
    Wraw = np.concatenate([mk[n] for n in walls])
    owner = {}
    for i, tri in enumerate(Wraw.tolist()):
        for k in range(3): owner.setdefault(tuple(sorted((tri[k], tri[(k + 1) % 3]))), wm[i])
    markers = [owner[tuple(sorted(e))] for e in f.seg.tolist()]
    vmark = {}
    for e, m in zip(f.seg.tolist(), markers): vmark[e[0]] = m; vmark[e[1]] = m
    img_markers = []
    for j in range(f.nimg):
        A = f.A[len(f.seg) + j]
        k = [i for i, (a, b) in enumerate(f.seg.tolist()) if np.array_equal(P[a], A) or np.array_equal(P[b], A)]
        img_markers.append(markers[k[0]] if k else walls[0])
    with open(os.path.join(HERE, f'd1_{case}.txt'), 'w') as out:
        out.write(f'SU2_D1_FEATURES\n{h0!r} {g!r} {T!r} {hgrad!r} {law.R_f!r}\n{len(f.A)}\n')
        for A, B, m in zip(f.A, f.Bp, markers + img_markers):
            out.write(' '.join(repr(float(x)) for x in (*A, *B)) + f' {m}\n')
        out.write(f'{len(f.corners)}\n')
        for v in f.corners.tolist(): out.write(' '.join(repr(float(x)) for x in P[v]) + f' {vmark.get(v, walls[0])}\n')
    print(case, 'segments', len(f.A), 'corners', len(f.corners), 'R_f', law.R_f)
