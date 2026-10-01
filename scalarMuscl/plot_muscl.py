"""Validation plots of the turbulence/species MUSCL fixes (branch fix_scalar_muscl), develop against the PR.

    python3 plot_muscl.py

Reads the history.csv of the runs in this folder and writes muscl_fixes.png:
(a) SST NACA0012 10 deg, MUSCL_TURB from free stream: face values outside the variable bounds (fix: first order there)
(b) incompressible SA flat plate with BOUNDED_SCALAR in water units (rho = 998.2) and non-dimensional (rho = 1)
(c) SA NACA0012 10 deg restart, LIMITER_ITER = 50: distance of CL from the always-limited run
(d) SST flat plate: MUSCL_TURB limiters, the omega residual
"""
import csv
import os

import matplotlib
import numpy as np

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
SURFACE, INK, INK2, MUTED, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
DEV, FIX, REF, EXTRA = "#eb6834", "#2a78d6", "#52514e", "#1a9e77"
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


def main():
    fig, ax = plt.subplots(2, 2, figsize=(11, 7.6))

    a = ax[0, 0]
    for run, label, color, ls in (("sst_naca/develop_first_order", "develop, first order", REF, ":"),
                                  ("sst_naca/develop_muscl_wang", "develop, MUSCL + Wang", DEV, "-"),
                                  ("sst_naca/develop_muscl_none", "develop, MUSCL, no limiter (stopped)", DEV, "--"),
                                  ("sst_naca/pr_muscl_wang", "fix, MUSCL + Wang", FIX, "-"),
                                  ("sst_naca/pr_muscl_none", "fix, MUSCL, no limiter", FIX, "--")):
        h = hist(run)
        a.plot(h["Inner_Iter"], h["CD"], color=color, ls=ls, lw=1.4, label=f"{label}: CD {h['CD'][-1]:.4f}")
    a.set_yscale("log")
    a.set_xlabel("iteration")
    a.set_ylabel("CD")
    a.set_title("(a) SST NACA0012, 10 deg, MUSCL_TURB from free stream")
    a.legend(loc="upper right")

    b = ax[0, 1]
    for run, label, color, ls in (("bounded_sa/develop_upwind_rho998", "develop, SCALAR_UPWIND, rho 998.2", REF, ":"),
                                  ("bounded_sa/develop_bounded_rho1", "develop, BOUNDED_SCALAR, rho 1", DEV, "--"),
                                  ("bounded_sa/develop_bounded_rho998", "develop, BOUNDED_SCALAR, rho 998.2", DEV, "-"),
                                  ("bounded_sa/pr_bounded_rho998", "fix, BOUNDED_SCALAR, rho 998.2", FIX, "-")):
        h = hist(run)
        b.plot(h["Inner_Iter"], h["CD"], color=color, ls=ls, lw=1.4, label=f"{label}: CD {h['CD'][-1]:.7f}")
    b.set_ylim(0.0027, 0.0032)
    b.set_xlabel("iteration")
    b.set_ylabel("CD")
    b.set_title("(b) incompressible SA flat plate, BOUNDED_SCALAR, water units")
    b.legend(loc="upper right")

    c = ax[1, 0]
    ref = hist("sa_naca/develop_wang_always")
    for run, label, color, ls in (("sa_naca/develop_no_limiter", "develop, no limiter", REF, ":"),
                                  ("sa_naca/develop_wang_iter50", "develop, Wang, LIMITER_ITER = 50", DEV, "-"),
                                  ("sa_naca/pr_wang_iter50", "fix, Wang, LIMITER_ITER = 50", FIX, "-")):
        h = hist(run)
        n = min(len(h["CL"]), len(ref["CL"]))
        d = np.abs(h["CL"][:n] - ref["CL"][:n]) + 1e-12
        c.plot(h["Inner_Iter"][:n], d, color=color, ls=ls, lw=1.4, label=label)
    c.axvline(50, color=MUTED, lw=0.8)
    c.set_yscale("log")
    c.set_xlabel("iteration (restart)")
    c.set_ylabel("|CL - CL of the always-limited run|")
    c.set_title("(c) SA NACA0012, 10 deg: limiter after LIMITER_ITER")
    c.legend(loc="lower right")

    d = ax[1, 1]
    for run, label, color, ls in (("sst_flatplate/develop_first_order", "develop, first order", REF, ":"),
                                  ("sst_flatplate/develop_muscl_none", "develop, MUSCL, no limiter", DEV, "-"),
                                  ("sst_flatplate/pr_muscl_none", "fix, MUSCL, no limiter", FIX, "-"),
                                  ("sst_flatplate/pr_muscl_vanalbada", "fix, MUSCL, VAN_ALBADA_EDGE (develop: error)", EXTRA, "-")):
        h = hist(run)
        d.plot(h["Inner_Iter"], h["rms[w]"], color=color, ls=ls, lw=1.4, label=label)
    d.set_xlabel("iteration")
    d.set_ylabel("log10 rms[omega]")
    d.set_title("(d) SST flat plate (TMR 137x97): omega residual")
    d.legend(loc="upper right")

    fig.tight_layout()
    fig.savefig(os.path.join(HERE, "muscl_fixes.png"), dpi=150)
    print("wrote muscl_fixes.png")


if __name__ == "__main__":
    main()
