#!/usr/bin/env python3
"""replicate.py <sector.su2> <ncopies> <out.su2> <inlet_out.dat> [theta0_deg]
Copies the 45 deg sector ncopies times (rotation about z), merges the coincident nodes of per2(copy k) and
per1(copy k+1). ncopies = 8 gives the full annulus (no periodic markers). theta0 rotates the whole mesh first.
Also writes the inlet profile (T 300, P 1e5, flow direction = radial + 75 deg) for the inlet nodes."""
import sys, math
src, ncop, out, inl = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
th0 = math.radians(float(sys.argv[5])) if len(sys.argv) > 5 else 0.0
L = open(src).read().split("\n")
i = 0; elems = []; pts = []; marks = {}
while i < len(L):
    l = L[i]
    if l.startswith("NELEM"):
        n = int(l.split("=")[1]); elems = [list(map(int, x.split()))[:5] for x in L[i+1:i+1+n]]; i += n
    elif l.startswith("NPOIN"):
        n = int(l.split("=")[1].split()[0]); pts = [tuple(map(float, x.split()[:2])) for x in L[i+1:i+1+n]]; i += n
    elif l.startswith("MARKER_TAG"):
        tag = l.split("=")[1].strip(); n = int(L[i+1].split("=")[1])
        marks[tag] = [list(map(int, x.split()))[1:3] for x in L[i+2:i+2+n]]; i += 1+n
    i += 1
np_ = len(pts); full = (ncop == 8)
def key(x, y): return (round(x*1e9), round(y*1e9))
newpts = []; idmap = {}; lookup = {}
per1 = set(a for e in marks["per1"] for a in e); per2 = set(a for e in marks["per2"] for a in e)
for k in range(ncop):
    a = th0 + k*math.pi/4; c, s = math.cos(a), math.sin(a)
    for p, (x, y) in enumerate(pts):
        X, Y = c*x - s*y, s*x + c*y
        kk = key(X, Y)
        shared = (p in per1 and k > 0) or (p in per2 and full and k == ncop-1)
        if shared and kk in lookup:
            idmap[(k, p)] = lookup[kk]; continue
        assert not (p in per1 and k > 0), "per1 node of a later copy not matched"
        idmap[(k, p)] = len(newpts); newpts.append((X, Y))
        if p in per2 or (p in per1 and k == 0): lookup[kk] = idmap[(k, p)]
if full:
    assert len(newpts) == ncop*(np_-40), len(newpts)
else:
    assert len(newpts) == ncop*np_ - (ncop-1)*40, len(newpts)
o = ["NDIME= 2", "NELEM= %d" % (ncop*len(elems))]; n = 0
for k in range(ncop):
    for e in elems:
        o.append("9 " + " ".join(str(idmap[(k, v)]) for v in e[1:5]) + " %d" % n); n += 1
o.append("NPOIN= %d" % len(newpts))
for j, (x, y) in enumerate(newpts): o.append("%.17g %.17g %d" % (x, y, j))
mk = {"inlet": [(k, e) for k in range(ncop) for e in marks["inlet"]], "outlet": [(k, e) for k in range(ncop) for e in marks["outlet"]]}
if not full:
    mk["per1"] = [(0, e) for e in marks["per1"]]; mk["per2"] = [(ncop-1, e) for e in marks["per2"]]
o.append("NMARK= %d" % len(mk))
for tag, es in mk.items():
    o.append("MARKER_TAG= " + tag); o.append("MARKER_ELEMS= %d" % len(es))
    for k, e in es: o.append("3 %d %d" % (idmap[(k, e[0])], idmap[(k, e[1])]))
open(out, "w").write("\n".join(o) + "\n")
src_of = {}
for (k, p), j in idmap.items(): src_of.setdefault(j, (k, p))
open(out + ".map", "w").write("\n".join("%d %d %d" % (j, src_of[j][0], src_of[j][1]) for j in range(len(newpts))) + "\n")
ip = sorted(set(idmap[(k, v)] for k, e in mk["inlet"] for v in e))
w = ["NMARK= 1", "MARKER_TAG= inlet", "NROW=%d" % len(ip), "NCOL=6", "# COORD-X       COORD-Y         TEMPERATURE PRESSURE    NORMAL-X    NORMAL-Y   "]
for j in ip:
    x, y = newpts[j]; t = math.atan2(y, x) + math.radians(75.0)
    w.append("%.15E\t%.15E\t3.00E+02\t1.00E+05\t%.15f\t%.15f" % (x, y, math.cos(t), math.sin(t)))
open(inl, "w").write("\n".join(w) + "\n")
print(out, "points", len(newpts), "elems", n, "inlet pts", len(ip), "markers", list(mk))
