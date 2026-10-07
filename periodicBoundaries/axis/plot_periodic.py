"""Figure of the fix for the points on the rotation axis, develop against the PR.

    python3 plot_periodic.py

Reads the statistics file written by stats.py (pipe_axis/axis_profile.csv) and writes
  axis_nodes.png  pipe sector with nodes on the rotation axis, against the full pipe
"""
import csv
import os

import matplotlib
import numpy as np

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
SURFACE, INK, INK2, MUTED, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
DEV, FIX, REF = "#eb6834", "#2a78d6", "#52514e"
plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6, "axes.spines.top": False,
    "axes.spines.right": False, "font.size": 9.5, "axes.titlesize": 9.5, "axes.titlecolor": INK,
    "legend.frameon": False, "legend.labelcolor": INK2, "legend.fontsize": 8,
})


def hist(run):
    with open(os.path.join(HERE, run, "history.csv")) as f:
        rows = list(csv.reader(f))
    head = [h.strip().strip('"') for h in rows[0]]
    data = np.array([[float(v) for v in r] for r in rows[1:]])
    return {h: data[:, i] for i, h in enumerate(head)}


def axis():
    rows = list(csv.DictReader(open(os.path.join(HERE, "pipe_axis", "axis_profile.csv"))))
    fig, ax = plt.subplots(1, 2, figsize=(11, 4.0), sharey=True)
    runs = (("full_converged", "full pipe (no periodic boundary)", REF, dict(lw=4.8, alpha=0.45, zorder=2)),
            ("develop_w30_converged", "30 degree sector, develop", DEV, dict(lw=1.4, marker="o", ms=3, zorder=3)),
            ("pr_w30_converged", "30 degree sector, this PR", FIX, dict(lw=1.4, marker="o", ms=3, zorder=4)))
    for a, line, xlabel, title in ((ax[0], "axis", "z along the axis", "(a) points on the axis"),
                                   (ax[1], "radius", "radius at half length (z = 1)", "(b) from the axis to the wall")):
        for run, label, color, style in runs:
            sel = [r for r in rows if r["line"] == line and r["run"] == run]
            a.plot([float(r["coordinate"]) for r in sel], [float(r["Velocity_z"]) for r in sel], color=color, label=label, **style)
        a.set_xlabel(xlabel)
        a.set_title(title)
    ax[0].set_ylabel("axial velocity [m/s]")
    ax[1].legend(loc="upper right")
    fig.suptitle("Laminar pipe flow, converged (rms[Rho] < -12): sector with nodes on the rotation axis", color=INK, fontsize=10.5)
    fig.tight_layout()
    fig.savefig(os.path.join(HERE, "axis_nodes.png"), dpi=150)
    plt.close(fig)


if __name__ == "__main__":
    axis()
