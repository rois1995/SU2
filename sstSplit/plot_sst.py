"""Plot of the #1851 flat plate for the SST bug-fix PR: before the PR (mean-flow PRs) and with it.

    python3 plot_sst.py

Reads case1851/<run>/history.csv and writes sstA_1851.png.
"""
import csv
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))

SURFACE, INK, INK2, MUTED, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
BEFORE = dict(color="#eb6834", lw=1.6, ls="-")
AFTER = dict(color="#2a78d6", lw=1.6, ls="-")
REF = dict(color=INK2, lw=1.2, ls="--")

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6, "axes.spines.top": False,
    "axes.spines.right": False, "font.size": 10, "axes.titlesize": 10, "axes.titlecolor": INK,
    "legend.frameon": False, "legend.labelcolor": INK2,
})


def history(*path):
    rows = list(csv.reader(open(os.path.join(HERE, *path, "history.csv"))))
    head = [h.strip().strip('"') for h in rows[0]]
    cols = {h: [] for h in head}
    for r in rows[1:]:
        if len(r) == len(head):
            for h, v in zip(head, r):
                cols[h].append(float(v))
    return cols


def main():
    b, a = history("sstA", "flatplate_1851", "before"), history("sstA", "flatplate_1851", "after")
    it_max = max(b["Inner_Iter"][-1], a["Inner_Iter"][-1])
    panels = [("inlet mean temperature [K]", "Avg_Temp", 800.0, "imposed (800 K)"),
              ("inlet mean Mach number", "Avg_Mach", 6.1, "imposed (6.1)")]
    fig, axes = plt.subplots(1, 3, figsize=(13.2, 3.4))
    for ax, (ylabel, key, target, tlabel) in zip(axes[:2], panels):
        ax.plot(b["Inner_Iter"], b[key], label="before this PR", **BEFORE)
        ax.plot(a["Inner_Iter"], a[key], label="this PR", **AFTER)
        ax.plot([0, it_max], [target, target], label="imposed value", **REF)
        ax.set_ylabel(ylabel)
        ax.set_xlabel("iteration")
    axes[0].set_ylim(780, 860)
    axes[1].set_ylim(5.8, 6.2)
    axes[2].plot(b["Inner_Iter"], b["rms[Rho]"], label="before this PR", **BEFORE)
    axes[2].plot(a["Inner_Iter"], a["rms[Rho]"], label="this PR", **AFTER)
    axes[2].set_ylabel("rms[Rho]")
    axes[2].set_xlabel("iteration")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=3, bbox_to_anchor=(0.5, 0.93))
    fig.suptitle("Flat plate of #1851: Mach 6.1, 800 K, turbulence intensity 5 %, SST", color=INK, fontsize=11, y=0.995)
    fig.tight_layout(rect=(0, 0, 1, 0.86))
    out = os.path.join(HERE, "sstA_1851.png")
    fig.savefig(out, dpi=150)
    print("wrote", out)


if __name__ == "__main__":
    main()
