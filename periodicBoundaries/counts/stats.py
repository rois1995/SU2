#!/usr/bin/env python3
"""Statistics quoted in the test comment, computed from the ASCII restart files (restart.csv) that the runs write.

    python3 stats.py

Run it in this folder after the runs. It writes
  periodic2d_jst/jst_rank_difference.txt
  pin_array_heat/flow_difference.txt
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


def jst_ranks():
    """Converged JST solution with 1 and with 2 ranks."""
    d = "periodic2d_jst/"
    out = ["converged solution (rms[Rho] < -13), JST: largest difference between the run with 1 rank and the run with 2 ranks"]
    for tag in ("develop", "pr"):
        h1, r1 = restart(d + tag + "_jst_1rank/restart.csv")
        h2, r2 = restart(d + tag + "_jst_2ranks/restart.csv")
        for n in ("Density", "Momentum_x", "Momentum_y", "Energy"):
            e = max(abs(r1[p][h1.index(n)] - r2[p][h2.index(n)]) for p in r1)
            ref = max(abs(r1[p][h1.index(n)]) for p in r1)
            out.append("  %-7s %-10s max difference %.2e (max value %.3e, relative %.1e)" % (tag, n, e, ref, e / ref))
    open(os.path.join(HERE, d, "jst_rank_difference.txt"), "w").write("\n".join(out) + "\n")


def pin_array_heat():
    d = "pin_array_heat/"
    out = ["flow field with WEAKLY_COUPLED_HEAT_EQUATION= YES minus the flow field with NO (largest difference over all points)"]
    for tag in ("develop", "pr"):
        h1, r1 = restart(d + tag + "_heat_no/restart.csv")
        h2, r2 = restart(d + tag + "_heat_yes/restart.csv")
        for n in ("Pressure", "Velocity_x", "Velocity_y"):
            e = max(abs(r1[p][h1.index(n)] - r2[p][h2.index(n)]) for p in r1)
            ref = max(r1[p][h1.index(n)] for p in r1) - min(r1[p][h1.index(n)] for p in r1)
            out.append("  %-7s %-10s max difference %.3e (range of the field %.3e)" % (tag, n, e, ref))
    open(os.path.join(HERE, d, "flow_difference.txt"), "w").write("\n".join(out) + "\n")


if __name__ == "__main__":
    for job in (jst_ranks, pin_array_heat):
        try:
            job()
            print("done:", job.__name__)
        except FileNotFoundError as err:
            print("skipped %s (missing %s)" % (job.__name__, err.filename))
