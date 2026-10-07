#!/usr/bin/env python3
"""Statistics quoted in the test comment, computed from the ASCII restart files (restart.csv) that the runs write.

    python3 stats.py

Run it in this folder after the runs. It writes
  pipe_axis/<run>/vs_full_pipe.txt, pipe_axis/axis_profile.csv, pipe_axis/axis_summary.txt
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


def pipe_axis():
    d = "pipe_axis/"
    one = [("develop_w30_central", "w30", "full_field12_central"), ("pr_w30_central", "w30", "full_field12_central"),
           ("develop_s90_central", "s90", "full_field4_central"), ("pr_s90_central", "s90", "full_field4_central"),
           ("pr_w30_limiter", "w30", "full_field12_limiter"), ("pr_s90_limiter", "s90", "full_field4_limiter"),
           ("pr_w30_least_squares", "w30", "full_field12_least_squares"), ("pr_s90_least_squares", "s90", "full_field4_least_squares")]
    for run, mesh, ref in one:
        res = subprocess.run([sys.executable, os.path.join(HERE, d, "pipecmp.py"), os.path.join(HERE, d, run, "restart.csv"),
                              os.path.join(HERE, d, "pipe_%s.su2.map" % mesh), os.path.join(HERE, d, ref, "restart.csv"),
                              os.path.join(HERE, d, "pipe_full.su2.map"), "Residual_", "Limiter_Velocity", "Limiter_Pressure"],
                             capture_output=True, text=True, check=True)
        open(os.path.join(HERE, d, run, "vs_full_pipe.txt"), "w").write(
            "%s against %s: same nodes, same field, one iteration\n" % (run, ref) + res.stdout)
    # converged runs: values on the axis and along the radius at half length
    sol = {}
    for run in ("develop_w30_converged", "pr_w30_converged", "pr_s90_converged", "full_converged"):
        h, r = restart(d + run + "/restart.csv")
        sol[run] = (h, list(r.values()))
    summary = []
    with open(os.path.join(HERE, d, "axis_profile.csv"), "w") as f:
        f.write("line,coordinate,run,Velocity_z,Pressure,Density\n")
        for run, (h, rows) in sol.items():
            x, y, z = h.index("x"), h.index("y"), h.index("z")
            # theta = 0 line of nodes only (all meshes have it)
            axis = sorted((r for r in rows if math.hypot(r[x], r[y]) < 1e-9), key=lambda r: r[z])
            radial = sorted((r for r in rows if abs(r[z] - 1.0) < 1e-9 and abs(r[y]) < 1e-9 and r[x] > -1e-9), key=lambda r: r[x])
            for name, line, c in (("axis", axis, z), ("radius", radial, x)):
                for r in line:
                    f.write("%s,%.6f,%s,%.8f,%.4f,%.8f\n" % (name, r[c], run, r[h.index("Velocity_z")], r[h.index("Pressure")], r[h.index("Density")]))
            w = [r[h.index("Velocity_z")] for r in axis]
            vp = max(math.hypot(r[h.index("Velocity_x")], r[h.index("Velocity_y")]) for r in axis)
            summary.append("%-22s axis nodes: axial velocity %.4f .. %.4f m/s, pressure %.1f .. %.1f Pa, largest velocity normal to the axis %.1e m/s" %
                           (run, min(w), max(w), min(r[h.index("Pressure")] for r in axis), max(r[h.index("Pressure")] for r in axis), vp))
    open(os.path.join(HERE, d, "axis_summary.txt"), "w").write("\n".join(summary) + "\n")


if __name__ == "__main__":
    for job in (pipe_axis,):
        try:
            job()
            print("done:", job.__name__)
        except FileNotFoundError as err:
            print("skipped %s (missing %s)" % (job.__name__, err.filename))
