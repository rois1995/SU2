"""Plots for issue #2939: pressure-based solver with variable density, develop vs fix branch.

    python3 plot_issue.py

Reads <case>/<run>/history.csv and writes the PNGs next to this script.
"""
import csv
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))

SURFACE, INK, INK2, MUTED, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
DEV = dict(label="develop", color="#eb6834", lw=1.6, ls="-")
FIX = dict(label="fix branch", color="#2a78d6", lw=1.6, ls="-")
REF = dict(label="density-based solver", color=INK2, lw=1.2, ls="--")

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6, "axes.spines.top": False,
    "axes.spines.right": False, "font.size": 10, "axes.titlesize": 10, "axes.titlecolor": INK,
    "legend.frameon": False, "legend.labelcolor": INK2,
})


def history(case, run):
    rows = list(csv.reader(open(os.path.join(HERE, case, run, "history.csv"))))
    head = [h.strip().strip('"') for h in rows[0]]
    cols = {h: [] for h in head}
    for r in rows[1:]:
        if len(r) != len(head):
            continue
        for h, v in zip(head, r):
            cols[h].append(float(v))
    return cols


def figure(name, title, panels, xlim=None):
    """panels: list of (ylabel, [(style, x, y)], options)."""
    fig, axes = plt.subplots(1, len(panels), figsize=(4.4 * len(panels), 3.4), squeeze=False)
    for ax, (ylabel, series, opts) in zip(axes[0], panels):
        for style, x, y in series:
            ax.plot(x, y, color=style["color"], lw=style["lw"], ls=style["ls"], label=style["label"])
        ax.set_ylabel(ylabel)
        ax.set_xlabel("iteration")
        if "ylim" in opts:
            ax.set_ylim(*opts["ylim"])
        if xlim:
            ax.set_xlim(*xlim)
    handles, labels = [], []
    for ax in axes[0]:
        for h, l in zip(*ax.get_legend_handles_labels()):
            if l not in labels:
                handles.append(h)
                labels.append(l)
    fig.legend(handles, labels, loc="upper center", ncol=len(labels), bbox_to_anchor=(0.5, 0.93))
    fig.suptitle(title, color=INK, fontsize=11, y=0.995)
    fig.tight_layout(rect=(0, 0, 1, 0.86))
    out = os.path.join(HERE, name + ".png")
    fig.savefig(out, dpi=150)
    plt.close(fig)
    print("wrote", out)


def flat(x, value):
    return [x[0], x[-1]], [value, value]


def main():
    # pb_poly_cylinder run to convergence; the density-based values are the converged ones of poly_cylinder.cfg.
    d, f, r = history("poly_cylinder", "pb_dev"), history("poly_cylinder", "pb_all"), history("poly_cylinder", "db_dev")
    x = [0, max(d["Inner_Iter"][-1], f["Inner_Iter"][-1])]
    figure("pb_poly_cylinder", "pb_poly_cylinder (variable density, CFL 1000)",
           [("rms[h]", [(DEV, d["Inner_Iter"], d["rms[h]"]), (FIX, f["Inner_Iter"], f["rms[h]"])], {}),
            ("CD", [(DEV, d["Inner_Iter"], d["CD"]), (FIX, f["Inner_Iter"], f["CD"]), (REF, *flat(x, r["CD"][-1]))],
             {"ylim": (0, 8)}),
            ("total heat flux [W/m]", [(DEV, d["Inner_Iter"], d["HF"]), (FIX, f["Inner_Iter"], f["HF"]),
                                       (REF, *flat(x, r["HF"][-1]))], {"ylim": (-6000, 0)})])

    # Bend with a 400 K inlet (initial and wall-free-stream state 288.15 K, adiabatic walls).
    d, f = history("bend_inlet", "pb_dev"), history("bend_inlet", "pb_all")
    target = dict(label="inlet temperature (400 K)", color=INK2, lw=1.2, ls="--")
    x = [0, max(d["Inner_Iter"][-1], f["Inner_Iter"][-1])]
    figure("pb_bend_inlet", "3D bend (pb_lam_bend.cfg), variable density, inlet at 400 K, initial 288.15 K",
           [("inlet mean temperature [K]", [(DEV, d["Inner_Iter"], d["Avg_Temp(INLET)"]),
                                            (FIX, f["Inner_Iter"], f["Avg_Temp(INLET)"]), (target, *flat(x, 400.0))],
             {"ylim": (280, 410)}),
            ("outlet mean temperature [K]", [(DEV, d["Inner_Iter"], d["Avg_Temp(OUTLET)"]),
                                             (FIX, f["Inner_Iter"], f["Avg_Temp(OUTLET)"]), (target, *flat(x, 400.0))],
             {"ylim": (280, 410)})])


if __name__ == "__main__":
    main()
