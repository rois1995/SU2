"""Plots of the test cases of PR #2941 (develop vs meanFlowFixes), from the histories written by run_cases.sh.

    python3 plot_cases.py

Writes one PNG per case in this folder. A case with a missing history is skipped. Needs matplotlib.
"""
import csv
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))

SURFACE, INK, INK2, MUTED, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
DEV = dict(label="develop", color="#eb6834", lw=1.6, ls="-")
FIX = dict(label="meanFlowFixes", color="#2a78d6", lw=1.6, ls="-")
REF = dict(label="fixed grid (reference)", color=INK2, lw=1.2, ls="--")

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6, "axes.spines.top": False,
    "axes.spines.right": False, "font.size": 10, "axes.titlesize": 10, "axes.titlecolor": INK,
    "legend.frameon": False, "legend.labelcolor": INK2,
})


def history(case, variant, cfg="case"):
    """<case>/<variant>/<cfg>/history.csv -> {column: [values]} (None if missing)."""
    f = os.path.join(HERE, case, variant, cfg, "history.csv")
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
    """panels: list of (ylabel, [(style, x, y)], options). Saves <name>.png if every panel has data."""
    panels = [p if len(p) == 3 else (*p, {}) for p in panels]
    if any(not series for _, series, _ in panels):
        print(f"skip {name}: history missing")
        return
    fig, axes = plt.subplots(1, len(panels), figsize=(4.4 * len(panels), 3.4), squeeze=False)
    for ax, (ylabel, series, opts) in zip(axes[0], panels):
        for style, x, y in series:
            ax.plot(x, y, color=style["color"], lw=style["lw"], ls=style["ls"], label=style["label"])
        ax.set_ylabel(ylabel)
        if "ylim" in opts:
            ax.set_ylim(*opts["ylim"])
        if xlim:
            ax.set_xlim(*xlim)
        ax.set_xlabel("iteration" if "time step" not in ylabel else "time step")
    handles, labels = axes[0][0].get_legend_handles_labels()
    for ax in axes[0][1:]:
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


def last_per_step(h, key):
    """Value at the last inner iteration of each time step (unsteady histories)."""
    t = h["Time_Iter"]
    idx = [i for i in range(len(t)) if i == len(t) - 1 or t[i + 1] != t[i]]
    return [t[i] for i in idx], [h[key][i] for i in idx]


def xy(h, key, x="Inner_Iter"):
    return (h[x], h[key]) if h and key in h else None


def series(*items):
    """Keep only the (style, x, y) whose data exist."""
    return [(s, *d) for s, d in items if d is not None]


def main():
    # Under-relaxation: restart of a converged solution.
    d, f = history("1a_nozzle_restart", "develop"), history("1a_nozzle_restart", "meanFlowFixes")
    figure("1a_underrelaxation_restart", "Axisymmetric air nozzle (SST): restart from a converged solution",
           [("rms[Rho]", series((DEV, xy(d, "rms[Rho]")), (FIX, xy(f, "rms[Rho]")))),
            ("rms[RhoE]", series((DEV, xy(d, "rms[RhoE]")), (FIX, xy(f, "rms[RhoE]"))))])

    # Under-relaxation: nozzle from rest and RAE2822.
    d, f = history("1b_nozzle_from_rest", "develop"), history("1b_nozzle_from_rest", "meanFlowFixes")
    figure("1b_underrelaxation_nozzle_from_rest", "Axisymmetric air nozzle (SA, dimensional), from rest, CFL adaptation",
           [("rms[Rho]", series((DEV, xy(d, "rms[Rho]")), (FIX, xy(f, "rms[Rho]")))),
            ("outlet mean pressure [Pa]", series((DEV, xy(d, "Avg_Press")), (FIX, xy(f, "Avg_Press")))),
            ("outlet mass flow [kg/s]", series((DEV, xy(d, "Avg_Massflow")), (FIX, xy(f, "Avg_Massflow"))))])
    d, f = history("1c_rae2822", "develop"), history("1c_rae2822", "meanFlowFixes")
    figure("1c_underrelaxation_rae2822", "RAE2822 (SA, dimensional)",
           [("rms[Rho]", series((DEV, xy(d, "rms[Rho]")), (FIX, xy(f, "rms[Rho]")))),
            ("CL", series((DEV, xy(d, "CL")), (FIX, xy(f, "CL")))),
            ("CD", series((DEV, xy(d, "CD")), (FIX, xy(f, "CD"))), {"ylim": (0.012, 0.016)})])

    # HLLC: still air + translating grid vs fixed grid.
    d = history("2_hllc_translating", "develop", "moving")
    f = history("2_hllc_translating", "meanFlowFixes", "moving")
    r = history("2_hllc_translating", "develop", "fixed")
    figure("2_hllc_translating_grid", "HLLC, NACA0012 M0.8: still air + translating grid vs fixed grid (Galilean invariance)",
           [("rms[Rho]", series((DEV, xy(d, "rms[Rho]")), (FIX, xy(f, "rms[Rho]")), (REF, xy(r, "rms[Rho]")))),
            ("CL", series((DEV, xy(d, "CL")), (FIX, xy(f, "CL")), (REF, xy(r, "CL")))),
            ("CD", series((DEV, xy(d, "CD")), (FIX, xy(f, "CD")), (REF, xy(r, "CD"))), {"ylim": (0.015, 0.03)})],
           xlim=(0, 6000))

    # Incompressible moving-grid Jacobian: pitching airfoil, dual time.
    ser_res, ser_cl = [], []
    for style, variant in ((DEV, "develop"), (FIX, "meanFlowFixes")):
        h = history("3_inc_pitching", variant)
        if h:
            ser_res.append((style, list(range(len(h["rms[P]"]))), h["rms[P]"]))
            ser_cl.append((style, *last_per_step(h, "CL")))
    figure("3_inc_pitching_airfoil", "Incompressible pitching NACA0012, dual time, 60 inner iterations per time step",
           [("rms[P] (all inner iterations)", ser_res), ("CL at the end of each time step", ser_cl)])


if __name__ == "__main__":
    main()
