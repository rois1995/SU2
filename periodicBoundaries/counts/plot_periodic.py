"""Figure of the fix for the flow with the weakly coupled heat equation, develop against the PR.

    python3 plot_periodic.py

Reads the history.csv of the runs and writes
  weak_heat.png  periodic pin array with and without the weakly coupled heat equation
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


def heat():
    fig, a = plt.subplots(1, 1, figsize=(6.4, 4.0))
    p = "pin_array_heat/"
    for run, label, color, style in (("develop_heat_no", "heat equation off (develop and this PR)", REF, dict(lw=4.8, alpha=0.45, zorder=2)),
                                     ("develop_heat_yes", "heat equation on, develop", DEV, dict(lw=1.4, zorder=3)),
                                     ("pr_heat_yes", "heat equation on, this PR", FIX, dict(lw=1.4, ls="--", zorder=4))):
        h = hist(p + run)
        a.plot(h["Inner_Iter"], h["rms[P]"], color=color, label="%s: %d iterations" % (label, len(h["rms[P]"])), **style)
    a.set_xlabel("iteration")
    a.set_ylabel("rms[P] (log10)")
    a.set_title("Periodic pin array, flow with the weakly coupled heat equation")
    a.legend(loc="upper right")
    fig.tight_layout()
    fig.savefig(os.path.join(HERE, "weak_heat.png"), dpi=150)
    plt.close(fig)


if __name__ == "__main__":
    heat()
