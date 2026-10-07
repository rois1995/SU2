#!/usr/bin/env python3
"""pipecmp.py <sector restart.csv> <sector mesh.su2.map> <full restart.csv> <full mesh.su2.map> [name filter ...]
Compares the fields of a sector run with the full pipe run at the same nodes (same j, i, k in the .map files).
Prints, per group of nodes (axis, first ring, periodic faces, rest), the largest absolute difference of every column
whose name starts with one of the filters (default: Residual_ Limiter_), and the largest absolute value in the full pipe."""
import sys, csv
sec, secmap, ful, fulmap = sys.argv[1:5]; filt = sys.argv[5:] or ["Residual_", "Limiter_"]
def rd(f):
    rows = list(csv.reader(open(f))); head = [h.strip().strip('"') for h in rows[0]]
    return head, {int(float(r[0])): [float(x) for x in r] for r in rows[1:] if r}
def rmap(f): return {int(a): (int(j), int(i), int(k)) for a, j, i, k in (l.split() for l in open(f) if l.strip())}
hs, ds = rd(sec); hf, df = rd(ful); ms = rmap(secmap); mf = {v: p for p, v in rmap(fulmap).items()}
jmax = max(j for j, i, k in ms.values())
def group(t):
    j, i, k = t
    if i == 0: return "axis"
    if j in (0, jmax): return "periodic, first ring" if i == 1 else "periodic"
    return "first ring" if i == 1 else "rest"
cols = [c for c in hs if any(c.startswith(f) for f in filt) and c in hf]
res = {}; passive = 0
rcols = [c for c in hs if c.startswith("Residual_")]
for p, t in ms.items():
    q = mf[t]; g = group(t)
    # the passive side of a periodic pair has a zero residual in implicit runs (its equation is removed): skip
    if rcols and g.startswith("periodic") and all(ds[p][hs.index(c)] == 0.0 for c in rcols): passive += 1; continue
    for c in cols:
        a, b = ds[p][hs.index(c)], df[q][hf.index(c)]
        e = res.setdefault(g, {}).setdefault(c, [0.0, 0.0]); e[0] = max(e[0], abs(a-b)); e[1] = max(e[1], abs(b))
for g in ("axis", "periodic, first ring", "periodic", "first ring", "rest"):
    if g not in res: continue
    print("%s:" % g)
    for c in cols: print("  %-24s max|sector - full| %.3e   max|full| %.3e" % (c, res[g][c][0], res[g][c][1]))
if passive: print("skipped %d periodic nodes with zero residual (passive side)" % passive)
