#!/usr/bin/env python3
"""stats3d.py <multizone mesh.su2> <izone (1-based)> <restart.csv>  limiter statistics on PER* marker nodes and elsewhere."""
import sys, csv
mesh, iz, rst = sys.argv[1], int(sys.argv[2]), sys.argv[3]
marks = {}; cur = 0; npoin = 0
with open(mesh) as f:
    it = iter(f)
    for l in it:
        if l.startswith("IZONE"): cur = int(l.split("=")[1]); continue
        if cur != iz: continue
        if l.startswith("NPOIN"): npoin = int(l.split("=")[1].split()[0])
        if l.startswith("MARKER_TAG"):
            tag = l.split("=")[1].strip(); n = int(next(it).split("=")[1]); s = set()
            for _ in range(n): s.update(int(x) for x in next(it).split()[1:])
            marks[tag] = s
rows = list(csv.reader(open(rst))); head = [h.strip().strip('"') for h in rows[0]]
d = {int(float(r[0])): [float(x) for x in r] for r in rows[1:] if r}
per = {t: s for t, s in marks.items() if t.startswith("PER")}
allper = set().union(*per.values()); per["rest"] = set(d) - allper
print(f"{rst}: zone {iz}, {len(d)} points, markers {list(marks)}")
for tag, s in per.items():
    for h in head:
        if not h.startswith("Limiter"): continue
        j = head.index(h); v = [d[p][j] for p in s if p in d]
        if not v or max(abs(x) for x in v) == 0: continue
        print(f"  {tag:5s} {h:22s} min {min(v):+.5f} max {max(v):.5f} mean {sum(v)/len(v):+.5f}  <0: {sum(x<0 for x in v):5d}  <-0.5: {sum(x<-0.5 for x in v):5d} /{len(v)}")
