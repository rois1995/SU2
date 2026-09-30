"""NASA TMR 2D bump in channel (M 0.2, Re 3e6), SST, 353x161 grid: SU2 against the TMR results.

    python3 plot_tmr.py <run> [<run> ...]

Each <run> folder holds surface.vtu of the bump marker (VOLUME_OUTPUT with PRIMITIVE). The TMR data (CFL3D and FUN3D, SST-Vm) are in tmr/.
Writes sstB_tmr_flatplate.png and prints Cf at x = 0.97008 and CD against the TMR values on the same grid.
"""
import csv
import os
import sys

import matplotlib
import numpy as np

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
X_PROFILE = 0.97008

SURFACE, INK, INK2, MUTED, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
COLORS = ["#eb6834", "#2a78d6", "#1a9e77", "#8e5bd0"]
plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6, "axes.spines.top": False,
    "axes.spines.right": False, "font.size": 10, "axes.titlesize": 10, "axes.titlecolor": INK,
    "legend.frameon": False, "legend.labelcolor": INK2,
})

LABELS = {"sstA_V1994m": "before (V1994m)", "sstB_V1994m": "this PR, V1994m", "sstB_V1994": "this PR, V1994"}


def tmr_zones(name):
    """TMR Tecplot file -> {zone title: array}."""
    zones, title = {}, None
    for line in open(os.path.join(HERE, "tmr", name)):
        s = line.strip()
        if s.lower().startswith("zone"):
            title = s.split("=", 1)[1].strip().strip('"').strip()
            zones[title] = []
        elif title and s and not s.startswith(("#", "variables")):
            zones[title].append([float(v) for v in s.split()])
    return {k: np.array(v) for k, v in zones.items()}


def surface(run):
    """Cf (along the wall) and Cp on the bump marker, from the surface solution."""
    import vtk
    from vtk.util.numpy_support import vtk_to_numpy
    r = vtk.vtkXMLUnstructuredGridReader()
    r.SetFileName(os.path.join(HERE, run, "surface.vtu"))
    r.Update()
    g = r.GetOutput()
    p = vtk_to_numpy(g.GetPoints().GetData())
    cf = vtk_to_numpy(g.GetPointData().GetArray("Skin_Friction_Coefficient"))
    cp = vtk_to_numpy(g.GetPointData().GetArray("Pressure_Coefficient"))
    order = np.argsort(p[:, 0])
    t = np.gradient(p[order, :2], axis=0)
    t /= np.linalg.norm(t, axis=1)[:, None]
    return p[order, 0], np.sum(cf[order, :2] * t, axis=1), cp[order]


def profile(run, x0=X_PROFILE):
    """u+ against log10(y+) on the grid line closest to x0, from the volume solution."""
    import vtk
    from vtk.util.numpy_support import vtk_to_numpy
    r = vtk.vtkXMLUnstructuredGridReader()
    r.SetFileName(os.path.join(HERE, run, "flow.vtu"))
    r.Update()
    g = r.GetOutput()
    p = vtk_to_numpy(g.GetPoints().GetData())
    pd = g.GetPointData()
    u = vtk_to_numpy(pd.GetArray("Velocity"))[:, 0]
    rho = vtk_to_numpy(pd.GetArray("Density"))
    mu = vtk_to_numpy(pd.GetArray("Laminar_Viscosity"))
    xs = np.unique(np.round(p[:, 0], 8))
    xl = xs[np.argmin(abs(xs - x0))]
    line = np.where(abs(p[:, 0] - xl) < 1e-7)[0]
    line = line[np.argsort(p[line, 1])]
    y, ul, rl, ml = p[line, 1], u[line], rho[line], mu[line]
    tau_w = ml[0] * (ul[1] - ul[0]) / (y[1] - y[0])
    u_tau = np.sqrt(tau_w / rl[0])
    yp = y[1:] * u_tau * rl[0] / ml[0]
    return np.log10(yp), ul[1:] / u_tau


def main(runs):
    cf_tmr = tmr_zones("cf_bump_sst.dat")
    cp_tmr = tmr_zones("cp_bump_sst.dat")
    conv = tmr_zones("cf_convergence_sst.dat")
    force = tmr_zones("force_convergence_sst.dat")

    fig, axes = plt.subplots(1, 2, figsize=(10.6, 3.8))
    for name, style in (("CFL3D", "--"), ("FUN3D", ":")):
        axes[0].plot(cf_tmr[name][:, 0], cf_tmr[name][:, 1], color=INK2, lw=1.2, ls=style, label=f"TMR {name} (1409x641)")
        axes[1].plot(cp_tmr[name][:, 0], cp_tmr[name][:, 1], color=INK2, lw=1.2, ls=style, label=f"TMR {name} (1409x641)")
    for run, color in zip(runs, COLORS):
        x, cf, cp = surface(run)
        axes[0].plot(x, cf, color=color, lw=1.2, marker="." if len(x) < 100 else None, ms=3, label=LABELS.get(run, run))
        axes[1].plot(x, cp, color=color, lw=1.5, label=LABELS.get(run, run))
    axes[0].set_xlim(0, 1.5)
    axes[0].set_ylim(0.0, 0.008)
    axes[0].set_xlabel("x")
    axes[0].set_ylabel("Cf")
    axes[1].set_xlim(0, 1.5)
    axes[1].invert_yaxis()
    axes[1].set_xlabel("x")
    axes[1].set_ylabel("Cp")
    handles, labels = [], []
    for ax in axes:
        for h, l in zip(*ax.get_legend_handles_labels()):
            if l not in labels:
                handles.append(h)
                labels.append(l)
    fig.legend(handles, labels, loc="upper center", ncol=3, bbox_to_anchor=(0.5, 0.97), fontsize=8.5)
    fig.suptitle("NASA TMR 2D bump in channel, SST, SU2 on the 353x161 grid", color=INK, fontsize=11, y=0.995)
    fig.tight_layout(rect=(0, 0, 1, 0.82))
    fig.savefig(os.path.join(HERE, "sstB_tmr_bump.png"), dpi=150)
    print("wrote sstB_tmr_bump.png")

    n = 56320.0  # 353x161 grid in the TMR tables
    for xs in ("0.6321975", "0.75", "0.8678025"):
        ref = {k.split(",")[0]: v[v[:, 0] == n][0, 3] for k, v in conv.items() if k.endswith("x=" + xs)}
        line = f"x={xs}: TMR 353x161 CFL3D {ref['CFL3D']:.6f} FUN3D {ref['FUN3D']:.6f}"
        for run in runs:
            x, cf, _ = surface(run)
            line += f" | {LABELS.get(run, run)} {np.interp(float(xs), x, cf):.6f}"
        print(line)
    print("TMR 353x161 CD: CFL3D %.6f FUN3D %.6f" % tuple(force[k][force[k][:, 0] == n][0, 4] for k in ("CFL3D", "FUN3D")))


if __name__ == "__main__":
    main(sys.argv[1:] or ["sstA_V1994m", "sstB_V1994m", "sstB_V1994"])
