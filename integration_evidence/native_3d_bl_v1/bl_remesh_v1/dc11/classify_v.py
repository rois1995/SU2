#!/usr/bin/env python3
"""DC-11 V diagnosis (Q1): classify the long edges (> 1.8) and bad cells of a dc11_<case>_final.mesh by location.
usage: classify_v.py <final.mesh> <input fixture .txt> [wall_marker ...]
Location classes (an edge):
  on_cut      the edge lies on a face of the artificial cut/far marker (M6: CUT=3, tubes: far=0)
  on_sym      on a SYMMETRY face (M6: 1, tubes: 1)
  touch_cut   an endpoint is a vertex of the cut/far surface (not on it)
  touch_sym   an endpoint on the symmetry surface
  interior    no endpoint on any non-wall physical surface
plus: inside BL columns (harness flag), endpoints original fixture vertices, wall distance of the midpoint in h0.
Bad cells: cell class (harness cell_gates) and the same surface-touch classes.
"""
import sys, numpy as np, collections

def read(path):
    L = open(path).read().split("\n"); i = 0; X = {}; C = {}; F = []; h0 = 1.0; G = {}; E = []
    while i < len(L):
        t = L[i].split(); i += 1
        if not t: continue
        k = t[0]
        if k in ("nodes", "cells", "faces", "cell_gates", "long_edges"):
            n = int(t[1])
            for _ in range(n):
                r = L[i].split(); i += 1
                if k == "nodes": X[int(r[0])] = [float(v) for v in r[1:4]]
                elif k == "cells": C[int(r[0])] = [int(v) for v in r[1:5]]
                elif k == "faces": F.append([int(v) for v in r])
                elif k == "cell_gates": G[int(r[0])] = (float(r[1]), float(r[2]), int(r[3]), int(r[4]), int(r[5]), int(r[6]))
                else: E.append((int(r[0]), int(r[1]), float(r[2]), int(r[3]), int(r[4])))
        elif k == "h0": h0 = float(t[1])
    return X, C, F, h0, G, E

def fixture(path):
    """input node ids and the marker names of a dc7 M6 cut or a wp0b junction case"""
    L = open(path).read().split("\n"); ids = set(); names = []
    for j, l in enumerate(L):
        t = l.split()
        if not t: continue
        if t[0] == "markers":
            n = int(t[1]); names = t[2:2 + n] if len(t) > 2 else [L[j + 1 + m].strip() for m in range(n)]
        if t[0] == "nodes":
            for m in range(int(t[1])): ids.add(int(L[j + 1 + m].split()[0]))
            break
    return ids, names

def tri_dist(P, A, B, C):
    """distance of points P (n,3) to triangles (m,3) -> (n,) minimum (Ericson closest point)"""
    best = np.full(len(P), np.inf)
    for a, b, c in zip(A, B, C):
        ab, ac = b - a, c - a; ap = P - a
        d1 = ap @ ab; d2 = ap @ ac
        bp = P - b; d3 = bp @ ab; d4 = bp @ ac
        cp = P - c; d5 = cp @ ab; d6 = cp @ ac
        va = d3 * d6 - d5 * d4; vb = d5 * d2 - d1 * d6; vc = d1 * d4 - d3 * d2
        den = va + vb + vc; den = np.where(np.abs(den) < 1e-300, 1e-300, den)
        v = vb / den; w = vc / den
        q = a + np.outer(v, ab) + np.outer(w, ac)
        m = (d1 <= 0) & (d2 <= 0); q[m] = a
        m2 = (d3 >= 0) & (d4 <= d3); q[m2] = b
        m3 = (d6 >= 0) & (d5 <= d6); q[m3] = c
        m4 = (vc <= 0) & (d1 >= 0) & (d3 <= 0) & ~m & ~m2; t = d1 / np.where(d1 - d3 == 0, 1, d1 - d3); q[m4] = a + np.outer(t, ab)[m4]
        m5 = (vb <= 0) & (d2 >= 0) & (d6 <= 0) & ~m & ~m3; t = d2 / np.where(d2 - d6 == 0, 1, d2 - d6); q[m5] = a + np.outer(t, ac)[m5]
        m6 = (va <= 0) & ((d4 - d3) >= 0) & ((d5 - d6) >= 0) & ~m2 & ~m3
        t = (d4 - d3) / np.where((d4 - d3) + (d5 - d6) == 0, 1, (d4 - d3) + (d5 - d6)); q[m6] = b + np.outer(t, c - b)[m6]
        best = np.minimum(best, np.linalg.norm(P - q, axis=1))
    return best

def pct(a, p): return float(np.percentile(a, p)) if len(a) else float("nan")

def main():
    mesh, fix = sys.argv[1], sys.argv[2]
    X, C, F, h0, G, E = read(mesh); orig, names = fixture(fix)
    cutm = names.index("CUT") if "CUT" in names else names.index("far")
    symm = [k for k, n in enumerate(names) if n.lower().startswith("sym")][0]
    wall = [f for f in F if f[5] == 1]
    surf = collections.defaultdict(set); fedges = collections.defaultdict(set)
    for f in F:
        if f[5] == 1: surf["wall"].update(f[1:4]); continue
        k = "cut" if f[4] == cutm else "sym" if f[4] == symm else "other"
        surf[k].update(f[1:4])
        for a, b in ((f[1], f[2]), (f[2], f[3]), (f[1], f[3])): fedges[k].add((min(a, b), max(a, b)))
    W = np.array([[X[v] for v in f[1:4]] for f in wall])
    print(f"mesh {mesh}\n nodes {len(X)} cells {len(C)} faces cut={sum(1 for f in F if f[5]==0 and f[4]==cutm)} sym={sum(1 for f in F if f[5]==0 and f[4]==symm)} wall={len(wall)} h0={h0:g}")
    print(f" fixture nodes {len(orig)}; cut-surface vertices {len(surf['cut'])} ({len(surf['cut'] & orig)} original); sym-surface vertices {len(surf['sym'])}")
    # ---- long edges
    loc = []; ln = []; col = []; org = []; mids = []
    for a, b, l, cls, incol in E:
        e = (min(a, b), max(a, b))
        if e in fedges["cut"]: k = "on_cut"
        elif e in fedges["sym"]: k = "on_sym"
        elif cls in (1, 2): k = "wall/seam"
        elif a in surf["cut"] or b in surf["cut"]: k = "touch_cut"
        elif a in surf["sym"] or b in surf["sym"]: k = "touch_sym"
        elif a in surf["wall"] or b in surf["wall"]: k = "touch_wall"
        else: k = "interior"
        loc.append(k); ln.append(l); col.append(incol); org.append((a in orig) + (b in orig))
        mids.append((np.array(X[a]) + np.array(X[b])) / 2)
    ln = np.array(ln); col = np.array(col); org = np.array(org); loc = np.array(loc)
    d = tri_dist(np.array(mids), W[:, 0], W[:, 1], W[:, 2]) / h0 if len(mids) else np.array([])
    eu = np.array([np.linalg.norm(np.array(X[a]) - np.array(X[b])) for a, b, *_ in E]) / h0
    print(f" LONG EDGES {len(E)}: metric length median {pct(ln,50):.2f} p90 {pct(ln,90):.2f} max {ln.max() if len(ln) else 0:.2f}; midpoint wall distance/h0 median {pct(d,50):.0f} (p10 {pct(d,10):.0f}, p90 {pct(d,90):.0f})")
    print(f"   in columns {int(col.sum())}; endpoints original: 2={int((org==2).sum())} 1={int((org==1).sum())} 0={int((org==0).sum())}")
    print(f"   {'class':11s} {'n':>5s} {'share':>6s} {'Lmed':>5s} {'Lmax':>6s} {'d/h0 med':>8s} {'d p10':>6s} {'d p90':>6s} {'eucl/h0 med':>11s} {'orig2':>5s}")
    for k in ("on_cut", "on_sym", "wall/seam", "touch_cut", "touch_sym", "touch_wall", "interior"):
        m = loc == k
        if not m.any(): continue
        print(f"   {k:11s} {int(m.sum()):5d} {m.mean():6.1%} {pct(ln[m],50):5.2f} {ln[m].max():6.2f} {pct(d[m],50):8.0f} {pct(d[m],10):6.0f} {pct(d[m],90):6.0f} {pct(eu[m],50):11.0f} {int((org[m]==2).sum()):5d}")
    bins = [0, 10, 30, 100, 300, 1000, 1e9]
    h = np.histogram(d, bins)[0]
    print("   wall-distance bins (h0) " + " ".join(f"[{bins[i]:g},{bins[i+1]:g}):{h[i]}" for i in range(len(h))))
    for lo, hi in ((1.8, 2.5), (2.5, 4), (4, 8), (8, 1e9)):
        m = (ln > lo) & (ln <= hi); print(f"   L in ({lo},{hi:g}]: {int(m.sum())}  share touching cut/sym surface {np.isin(loc[m],['on_cut','on_sym','touch_cut','touch_sym']).mean() if m.any() else 0:.1%}")
    # ---- bad cells
    bad = [c for c, g in G.items() if g[2]]
    cls = collections.Counter(); tq = []; bl = []
    for c in bad:
        v = C[c]; g = G[c]
        k = "first_child" if g[5] == 1 else "column_other" if g[5] == 2 else ("touch_cut" if any(x in surf["cut"] for x in v) else "touch_sym" if any(x in surf["sym"] for x in v) else "touch_wall" if any(x in surf["wall"] for x in v) else "interior")
        cls[k] += 1; tq.append(g[0]); bl.append(np.mean([X[x] for x in v], axis=0))
    db = tri_dist(np.array(bl), W[:, 0], W[:, 1], W[:, 2]) / h0 if bl else np.array([])
    tq = np.array(tq)
    print(f" BAD CELLS {len(bad)}: q median {pct(tq,50):.3f} min {tq.min() if len(tq) else 0:.4f}; centroid wall distance/h0 median {pct(db,50):.0f} (p10 {pct(db,10):.0f}, p90 {pct(db,90):.0f})")
    print("   " + " ".join(f"{k}:{n}" for k, n in sorted(cls.items())))
    # cells touching a long edge
    le = set((min(a, b), max(a, b)) for a, b, *_ in E); badset = set(bad); nb = 0
    for c in bad:
        v = C[c]
        if any((min(v[i], v[j]), max(v[i], v[j])) in le for i in range(4) for j in range(i + 1, 4)): nb += 1
    print(f"   bad cells having a long edge: {nb}")

if __name__ == "__main__": main()
