#!/usr/bin/env python3
"""mkrestart.py <sector restart.csv> <replicated mesh.su2.map> <out solution.csv>
Replicates a sector solution on a replicated mesh (velocity/momentum rotated by k*45 deg)."""
import sys, csv, math
rows = list(csv.reader(open(sys.argv[1]))); head = [h.strip().strip('"') for h in rows[0]]
d = {int(float(r[0])): [float(x) for x in r] for r in rows[1:] if r}
cols = ["x", "y", "Density", "Momentum_x", "Momentum_y", "Energy"]; ix = [head.index(c) for c in cols]
out = ['"PointID","x","y","Density","Momentum_x","Momentum_y","Energy"']
for l in open(sys.argv[2]):
    j, k, p = map(int, l.split()); a = k*math.pi/4; c, s = math.cos(a), math.sin(a)
    x, y, rho, mx, my, e = (d[p][i] for i in ix)
    out.append("%d, %.17e, %.17e, %.17e, %.17e, %.17e, %.17e" % (j, c*x - s*y, s*x + c*y, rho, c*mx - s*my, s*mx + c*my, e))
open(sys.argv[3], "w").write("\n".join(out) + "\n")
