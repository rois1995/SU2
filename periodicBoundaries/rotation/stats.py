#!/usr/bin/env python3
"""Statistics quoted in the test comment, computed from the ASCII restart files (restart.csv) that the runs write.

    python3 stats.py

Run it in this folder after the runs. It writes
  limiter_annulus/limiter_boundary_45.csv, limiter_annulus/sector_vs_annulus.txt
"""
import csv
import math
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def restart(path):
    rows = list(csv.reader(open(os.path.join(HERE, path))))
    head = [h.strip().strip('"') for h in rows[0]]
    return head, {int(float(r[0])): [float(x) for x in r] for r in rows[1:] if r}


def markers(mesh):
    lines = open(os.path.join(HERE, mesh)).read().split("\n")
    marks, i = {}, 0
    while i < len(lines):
        if lines[i].startswith("MARKER_TAG"):
            tag = lines[i].split("=")[1].strip()
            n = int(lines[i + 1].split("=")[1])
            pts = set()
            for l in lines[i + 2:i + 2 + n]:
                pts.update(int(x) for x in l.split()[1:])
            marks[tag] = pts
            i += 2 + n
        else:
            i += 1
    return marks


def limiter_annulus():
    """Velocity limiters on the periodic nodes: sector (develop, pr) against the full annulus, same field, one iteration."""
    d = "limiter_annulus/"
    ha, annulus = restart(d + "annulus_develop/restart.csv")
    out = []
    for ang in (45, 90):
        marks = markers(d + "sector%d.su2" % ang)
        corner = marks["inlet"] | marks["outlet"]
        runs = {}
        for tag in ("develop", "pr"):
            runs[tag] = restart(d + "%s_sector%d/restart.csv" % (tag, ang))
        names = ["Limiter_Velocity_x", "Limiter_Velocity_y"]
        if ang == 45:
            with open(os.path.join(HERE, d, "limiter_boundary_45.csv"), "w") as f:
                f.write("marker,point,radius," + ",".join("%s_%s" % (n, t) for n in names for t in ("develop", "pr", "annulus")) + "\n")
                for m in ("per1", "per2"):
                    for p in sorted(marks[m], key=lambda q: math.hypot(annulus[q][1], annulus[q][2])):
                        v = [m, str(p), "%.6f" % math.hypot(annulus[p][1], annulus[p][2])]
                        for n in names:
                            v += ["%.8f" % runs["develop"][1][p][runs["develop"][0].index(n)],
                                  "%.8f" % runs["pr"][1][p][runs["pr"][0].index(n)], "%.8f" % annulus[p][ha.index(n)]]
                        f.write(",".join(v) + "\n")
        out.append("%d degree sector against the full annulus (same field, one iteration, VENKATAKRISHNAN)" % ang)
        for tag in ("develop", "pr"):
            h, r = runs[tag]
            for m in ("per1", "per2"):
                for n in names:
                    v = [r[p][h.index(n)] for p in marks[m]]
                    e = [abs(r[p][h.index(n)] - annulus[p][ha.index(n)]) for p in marks[m]]
                    out.append("  %-7s %s %s: min %+.4f max %+.4f mean %+.4f | error to the annulus: mean %.1e max %.1e" %
                               (tag, m, n, min(v), max(v), sum(v) / len(v), sum(e) / len(e), max(e)))
            inner = [p for p in r if p not in marks["per1"] and p not in marks["per2"] and p not in corner]
            for n in ("Residual_Density", "Residual_Momentum_x"):
                e = max(abs(r[p][h.index(n)] - annulus[p][ha.index(n)]) for p in inner)
                ref = max(abs(annulus[p][ha.index(n)]) for p in inner)
                out.append("  %-7s interior points %s: max difference to the annulus %.1e (max value %.1e)" % (tag, n, e, ref))
    open(os.path.join(HERE, d, "sector_vs_annulus.txt"), "w").write("\n".join(out) + "\n")


if __name__ == "__main__":
    for job in (limiter_annulus,):
        try:
            job()
            print("done:", job.__name__)
        except FileNotFoundError as err:
            print("skipped %s (missing %s)" % (job.__name__, err.filename))
