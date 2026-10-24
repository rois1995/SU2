#!/usr/bin/env python3
"""Classify the bad cells of a DC-13 *_final.mesh: wall/cut corner (a wall vertex and a vertex on the artificial cut),
cut-touching volume (cut vertex, no wall vertex), real wall/BL (wall vertex, no cut vertex), volume (neither); with the
distance of the cell centroid from the nearest wall face (multiples of h0). Usage: classify_bad.py mesh cut_marker"""
import sys, collections, numpy as np
path, cut = sys.argv[1], int(sys.argv[2])
L = open(path).read().split("\n"); i = 0; X = {}; C = {}; F = []; G = {}; h0 = 1
while i < len(L):
    t = L[i].split()
    if not t: i += 1; continue
    n = int(t[1]) if len(t) > 1 and t[1].isdigit() else 0
    if t[0] == "nodes":
        for k in range(n): a = L[i+1+k].split(); X[int(a[0])] = np.array(list(map(float, a[1:4])))
    elif t[0] == "cells":
        for k in range(n): a = L[i+1+k].split(); C[int(a[0])] = list(map(int, a[1:5]))
    elif t[0] == "faces":
        for k in range(n): a = L[i+1+k].split(); F.append((list(map(int, a[1:4])), int(a[4]), a[5] == "1"))
    elif t[0] == "cell_gates":
        for k in range(n): a = L[i+1+k].split(); G[int(a[0])] = (float(a[1]), float(a[2]), int(a[3]))
    elif t[0] == "h0": h0 = float(t[1])
    i += n + 1
wallv = set(); cutv = set(); tris = []
for v, m, w in F:
    if w: wallv |= set(v); tris.append([X[x] for x in v])
    elif m == cut: cutv |= set(v)
def dist(p):
    best = 1e300
    for a, b, c in tris:
        n = np.cross(b-a, c-a); nn = np.linalg.norm(n)
        if nn == 0: continue
        n /= nn; q = p - np.dot(p-a, n)*n
        inside = all(np.dot(np.cross(y-x, q-x), n) >= 0 for x, y in ((a, b), (b, c), (c, a)))
        d = abs(np.dot(p-a, n)) if inside else min(np.linalg.norm(p-x) for x in (a, b, c))
        best = min(best, d)
    return best / h0
bins = [0, 1, 2, 5, 10, 30, 100, 300, 1e9]; cls = collections.Counter(); hist = collections.defaultdict(collections.Counter)
for cid, g in G.items():
    if not g[2]: continue
    v = C[cid]; w = any(x in wallv for x in v); c = any(x in cutv for x in v)
    k = "wall/cut corner" if w and c else "cut-touching volume" if c else "real wall/BL" if w else "volume"
    cls[k] += 1; d = dist(sum(X[x] for x in v)/4)
    for j in range(len(bins)-1):
        if bins[j] <= d < bins[j+1]: hist[k][f"{bins[j]:g}-{bins[j+1]:g}"] += 1; break
print("bad cells", sum(cls.values()), dict(cls))
for k in hist: print(f"  {k}: by wall distance [h0] ", dict(sorted(hist[k].items(), key=lambda x: float(x[0].split('-')[0]))))
