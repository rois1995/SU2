"""Figures of the fixes for rotational periodic boundaries, develop against the PR.

    python3 plot_periodic.py

Reads the history.csv of the runs and the statistics files written by stats.py, and writes
  rotational_convergence.png  rotational periodic sector (periodic2d): convergence histories
  rotational_limiter.png      velocity limiters along the periodic boundary, against the full annulus
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


def end(h, key="rms[Rho]"):
    """Short text for the legend: how the run ended."""
    r = h[key]
    if not np.isfinite(r[-1]) or r[-1] > 0:
        return "diverged after %d iterations" % len(r)
    return "%d iterations, %.1f" % (len(r), r[-1])


def convergence():
    fig, ax = plt.subplots(2, 2, figsize=(11, 7.6), sharey=True)
    p = "periodic2d/"

    a = ax[0, 0]
    for tag, label, color in (("develop", "develop", DEV), ("pr", "this PR", FIX)):
        h = hist(p + tag + "_no_limiter_cfl100")
        a.plot(h["Inner_Iter"], h["rms[Rho]"], color=color, lw=1.4, label="%s: %s" % (label, end(h)))
    a.set_title("(a) no limiter, CFL 100, no multigrid")
    a.legend(loc="upper right", bbox_to_anchor=(1.0, 0.72))

    for b, tag, label, color in ((ax[0, 1], "develop", "develop", DEV), (ax[1, 0], "pr", "this PR", FIX)):
        for mg, ls in ((0, "-"), (1, "--"), (2, ":")):
            h = hist(p + "%s_no_limiter_cfl20_mg%d" % (tag, mg))
            b.plot(h["Inner_Iter"], h["rms[Rho]"], color=color, ls=ls, lw=1.4, label="MGLEVEL= %d: %s" % (mg, end(h)))
        b.set_title("(%s) %s, multigrid, no limiter, CFL 20" % ("b" if tag == "develop" else "c", label))
        b.legend(loc="upper right")

    d = ax[1, 1]
    for tag, label, color in (("develop", "develop", DEV), ("pr", "this PR", FIX)):
        h = hist(p + tag + "_limiter_cfl100")
        d.plot(h["Inner_Iter"], h["rms[Rho]"], color=color, lw=1.4, label="%s: %s" % (label, end(h)))
    d.set_title("(d) VENKATAKRISHNAN_WANG limiter, CFL 100, no multigrid")
    d.legend(loc="upper right")

    for b in ax.flat:
        b.set_xlabel("iteration")
    for b in ax[:, 0]:
        b.set_ylabel("rms[Rho] (log10)")
    fig.suptitle("45 degree rotational periodic sector (navierstokes/periodic2D), 2 ranks", color=INK, fontsize=10.5)
    fig.tight_layout()
    fig.savefig(os.path.join(HERE, "rotational_convergence.png"), dpi=150)
    plt.close(fig)


def limiter():
    rows = list(csv.DictReader(open(os.path.join(HERE, "limiter_annulus", "limiter_boundary_45.csv"))))
    rows = [r for r in rows if r["marker"] == "per2"]
    r = np.array([float(x["radius"]) for x in rows])
    fig, ax = plt.subplots(1, 2, figsize=(11, 4.0), sharey=True)
    for a, comp, name in ((ax[0], "Limiter_Velocity_x", "x velocity"), (ax[1], "Limiter_Velocity_y", "y velocity")):
        ref = np.array([float(x[comp + "_annulus"]) for x in rows])
        a.plot(r, ref, color=REF, lw=3.2, alpha=0.35, label="full annulus (no periodic boundary)")
        a.plot(r, [float(x[comp + "_develop"]) for x in rows], color=DEV, lw=1.4, marker="o", ms=3, label="sector, develop")
        a.plot(r, [float(x[comp + "_pr"]) for x in rows], color=FIX, lw=1.4, marker="o", ms=3, label="sector, this PR")
        a.set_xlabel("radius of the point on the periodic boundary")
        a.set_title("limiter of the %s" % name)
    ax[0].set_ylabel("limiter")
    ax[0].legend(loc="center right")
    fig.suptitle("Velocity limiters on the second periodic boundary of the 45 degree sector, same field as the full annulus",
                 color=INK, fontsize=10.5)
    fig.tight_layout()
    fig.savefig(os.path.join(HERE, "rotational_limiter.png"), dpi=150)
    plt.close(fig)


if __name__ == "__main__":
    convergence()
    limiter()
