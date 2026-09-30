"""Plots for the split mean-flow PRs: each PR against the PR below it (develop for the first one).

    python3 plot_split.py

Reads <pr>/<case>/<run>/history.csv and writes <pr>_<case>.png. Missing runs are skipped.
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
    f = os.path.join(HERE, *path, "history.csv")
    if not os.path.exists(f):
        return None
    rows = list(csv.reader(open(f)))
    head = [h.strip().strip('"') for h in rows[0]]
    cols = {h: [] for h in head}
    for r in rows[1:]:
        if len(r) != len(head):
            continue
        try:
            vals = [float(x) for x in r]
        except ValueError:
            continue
        for h, v in zip(head, vals):
            cols[h].append(v)
    return cols if cols[head[0]] else None


def figure(name, title, panels, note=None, xlim=None):
    """panels: list of (ylabel, [(style, label, x, y)], options)."""
    if any(not s for _, s, _ in panels):
        print(f"skip {name}: history missing")
        return
    fig, axes = plt.subplots(1, len(panels), figsize=(4.4 * len(panels), 3.4), squeeze=False)
    for ax, (ylabel, series, opts) in zip(axes[0], panels):
        for style, label, x, y in series:
            ax.plot(x, y, label=label, **style)
        ax.set_ylabel(ylabel)
        ax.set_xlabel(opts.get("xlabel", "iteration"))
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
    if note:
        fig.text(0.5, 0.005, note, ha="center", va="bottom", color=MUTED, fontsize=8.5)
    fig.tight_layout(rect=(0, 0.04 if note else 0, 1, 0.86))
    out = os.path.join(HERE, name + ".png")
    fig.savefig(out, dpi=150)
    plt.close(fig)
    print("wrote", out)


def s(style, label, h, key, x="Inner_Iter"):
    return [(style, label, h[x], h[key])] if h and key in h else []


def last_per_step(h, key):
    t = h["Time_Iter"]
    idx = [i for i in range(len(t)) if i == len(t) - 1 or t[i + 1] != t[i]]
    return [t[i] for i in idx], [h[key][i] for i in idx]


def main():
    # PR 1: under-relaxation factor (develop vs this PR).
    d, f = history("pr1", "nozzle_restart", "develop"), history("pr1", "nozzle_restart", "pr1")
    figure("pr1_nozzle_restart", "Axisymmetric air nozzle (SST): restart from a converged solution",
           [("rms[Rho]", s(BEFORE, "develop", d, "rms[Rho]") + s(AFTER, "this PR", f, "rms[Rho]"), {}),
            ("rms[RhoE]", s(BEFORE, "develop", d, "rms[RhoE]") + s(AFTER, "this PR", f, "rms[RhoE]"), {})])
    d, f = history("pr1", "nozzle_from_rest", "develop"), history("pr1", "nozzle_from_rest", "pr1")
    figure("pr1_nozzle_from_rest", "Axisymmetric air nozzle (SA, dimensional), from rest, CFL adaptation",
           [("rms[Rho]", s(BEFORE, "develop", d, "rms[Rho]") + s(AFTER, "this PR", f, "rms[Rho]"), {}),
            ("outlet mean pressure [Pa]", s(BEFORE, "develop", d, "Avg_Press") + s(AFTER, "this PR", f, "Avg_Press"), {}),
            ("outlet mass flow [kg/s]", s(BEFORE, "develop", d, "Avg_Massflow") + s(AFTER, "this PR", f, "Avg_Massflow"), {})])

    # PR 2: HLLC on moving grids (PR 1 vs this PR, fixed grid as reference).
    d, f = history("pr2", "hllc_translating", "pr1"), history("pr2", "hllc_translating", "pr2")
    r = history("pr2", "hllc_translating", "fixed")
    lab = "before this PR (develop + under-relaxation fix)"
    figure("pr2_hllc_translating", "HLLC, NACA0012 M0.8: still air + translating grid vs fixed grid",
           [("rms[Rho]", s(BEFORE, lab, d, "rms[Rho]") + s(AFTER, "this PR", f, "rms[Rho]") + s(REF, "fixed grid", r, "rms[Rho]"), {}),
            ("CL", s(BEFORE, lab, d, "CL") + s(AFTER, "this PR", f, "CL") + s(REF, "fixed grid", r, "CL"), {}),
            ("CD", s(BEFORE, lab, d, "CD") + s(AFTER, "this PR", f, "CD") + s(REF, "fixed grid", r, "CD"), {"ylim": (0.015, 0.03)})],
           xlim=(0, 6000))

    # PR 4: incompressible moving-grid Jacobian (PR 3 vs this PR).
    ser_res, ser_cl = [], []
    for style, label, run in ((BEFORE, "before this PR", "pr3"), (AFTER, "this PR", "pr4")):
        h = history("pr4", "inc_pitching", run)
        if h:
            ser_res.append((style, label, list(range(len(h["rms[P]"]))), h["rms[P]"]))
            ser_cl.append((style, label, *last_per_step(h, "CL")))
    figure("pr4_inc_pitching", "Incompressible pitching NACA0012, dual time, 60 inner iterations per time step",
           [("rms[P] (all inner iterations)", ser_res, {"xlabel": "inner iteration (all time steps)"}),
            ("CL at the end of each time step", ser_cl, {"xlabel": "time step"})])


if __name__ == "__main__":
    main()
