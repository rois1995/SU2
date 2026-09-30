"""NASA TMR zero-pressure-gradient flat plate (M 0.2, Re 5e6), SST, 137x97 grid: SU2 against the TMR results.

    python3 plot_tmr.py <run> [<run> ...]

Each <run> folder holds flow.vtu (with VOLUME_OUTPUT= (COORDINATES, SOLUTION, PRIMITIVE)). The TMR data (CFL3D and FUN3D, SST-Vm) are in tmr/.
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

LABELS = {"sstA_Vm": "before (V1994m, VORTICITY)", "sstB_Vm": "this PR, V1994m, VORTICITY",
          "sstB_V1994_V": "this PR, V1994, VORTICITY", "sstB_V1994": "this PR, V1994"}


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
    """Cf along the wall (y = 0, x >= 0) from the volume solution."""
    import vtk
    from vtk.util.numpy_support import vtk_to_numpy
    r = vtk.vtkXMLUnstructuredGridReader()
    r.SetFileName(os.path.join(HERE, run, "flow.vtu"))
    r.Update()
    g = r.GetOutput()
    p = vtk_to_numpy(g.GetPoints().GetData())
    cf = vtk_to_numpy(g.GetPointData().GetArray("Skin_Friction_Coefficient"))[:, 0]
    wall = np.where((abs(p[:, 1]) < 1e-10) & (p[:, 0] >= 0.0))[0]
    order = wall[np.argsort(p[wall, 0])]
    return p[order, 0], cf[order]


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
    cf_tmr = tmr_zones("cf_plate_sstv.dat")
    up_tmr = tmr_zones("flatplate_u+y+_sstv.dat")
    conv = tmr_zones("cf_convergence_sstv.dat")
    drag = tmr_zones("drag_convergence_sstv.dat")

    fig, axes = plt.subplots(1, 2, figsize=(10.6, 3.8))
    for name, style in (("CFL3D", "--"), ("FUN3D", ":")):
        d = cf_tmr[name]
        axes[0].plot(d[:, 0], d[:, 1], color=INK2, lw=1.2, ls=style, label=f"TMR {name} (545x385)")
    zone = [k for k in up_tmr if "0.97" in k][0]
    axes[1].plot(up_tmr[zone][:, 0], up_tmr[zone][:, 1], color=INK2, lw=1.2, ls="--", label="TMR (545x385)")
    for run, color in zip(runs, COLORS):
        x, cf = surface(run)
        axes[0].plot(x, cf, color=color, lw=1.5, label=LABELS.get(run, run))
        ly, up = profile(run)
        axes[1].plot(ly, up, color=color, lw=1.5, label=LABELS.get(run, run))
    axes[0].set_xlim(0, 2)
    axes[0].set_ylim(0.002, 0.006)
    axes[0].set_xlabel("x")
    axes[0].set_ylabel("Cf")
    axes[1].set_xlim(-1, 4)
    axes[1].set_xlabel("log10(y+)")
    axes[1].set_ylabel("u+ at x = 0.97")
    handles, labels = [], []
    for ax in axes:
        for h, l in zip(*ax.get_legend_handles_labels()):
            if l not in labels:
                handles.append(h)
                labels.append(l)
    fig.legend(handles, labels, loc="upper center", ncol=3, bbox_to_anchor=(0.5, 0.97), fontsize=8.5)
    fig.suptitle("NASA TMR flat plate, SST, SU2 on the 137x97 grid", color=INK, fontsize=11, y=0.995)
    fig.tight_layout(rect=(0, 0, 1, 0.82))
    fig.savefig(os.path.join(HERE, "sstB_tmr_flatplate.png"), dpi=150)
    print("wrote sstB_tmr_flatplate.png")

    n = 13056.0  # 137x97 grid in the TMR tables
    ref = {k: v[v[:, 0] == n][0, 3] for k, v in conv.items()}
    refd = {k: v[v[:, 0] == n][0, 3] for k, v in drag.items()}
    print(f"TMR 137x97: Cf(0.97) CFL3D {ref['CFL3D']:.6f} FUN3D {ref['FUN3D']:.6f}; CD CFL3D {refd['CFL3D']:.6f} FUN3D {refd['FUN3D']:.6f}")
    for run in runs:
        x, cf = surface(run)
        print(f"{LABELS.get(run, run):32s} Cf(0.97) {np.interp(X_PROFILE, x, cf):.6f}")


if __name__ == "__main__":
    main(sys.argv[1:] or ["sstA_Vm", "sstB_Vm", "sstB_V1994_V", "sstB_V1994"])
