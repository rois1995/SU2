"""#1851 flat plate (Mach 6.1, turbulence intensity 5 %): SST-2003m against the standard SST-2003 (2/3 rho k in the
stress tensor, exact production). Reads <run>/surface.vtu, writes sstB_1851_versions.png.

    python3 plot_versions.py
"""
import os

import matplotlib
import numpy as np
import vtk
from vtk.util.numpy_support import vtk_to_numpy

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
SURFACE, INK, INK2, MUTED, GRID, AXIS = "#fcfcfb", "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6, "axes.spines.top": False,
    "axes.spines.right": False, "font.size": 10, "axes.titlesize": 10, "axes.titlecolor": INK,
    "legend.frameon": False, "legend.labelcolor": INK2,
})
RUNS = (("sstB_V2003m", "V2003m (2/3 rho k ignored, P = mu_t S^2)", "#eb6834"),
        ("sstB_V2003", "V2003 (standard, this PR)", "#2a78d6"))


def surface(run):
    r = vtk.vtkXMLUnstructuredGridReader()
    r.SetFileName(os.path.join(HERE, run, "surface.vtu"))
    r.Update()
    g = r.GetOutput()
    p = vtk_to_numpy(g.GetPoints().GetData())
    pd = g.GetPointData()
    o = np.argsort(p[:, 0])
    return p[o, 0], vtk_to_numpy(pd.GetArray("Skin_Friction_Coefficient"))[o, 0], vtk_to_numpy(pd.GetArray("Heat_Flux"))[o]


fig, axes = plt.subplots(1, 2, figsize=(10.6, 3.6))
for run, label, color in RUNS:
    x, cf, q = surface(run)
    axes[0].plot(x, cf, color=color, lw=1.6, label=label)
    axes[1].plot(x, abs(q) / 1e6, color=color, lw=1.6, label=label)
axes[0].set_ylabel("Cf")
axes[1].set_ylabel("wall heat flux (magnitude) [MW/m^2]")
axes[1].set_ylim(3.0, 4.5)
for ax in axes:
    ax.set_xlabel("x [m]")
    ax.set_xlim(0, 2)
axes[0].set_ylim(0.004, 0.0055)
h, l = axes[0].get_legend_handles_labels()
fig.legend(h, l, loc="upper center", ncol=2, bbox_to_anchor=(0.5, 0.93))
fig.suptitle("Flat plate of #1851 (Mach 6.1, 800 K, turbulence intensity 5 %): SST-2003m and SST-2003", color=INK, fontsize=11, y=0.995)
fig.tight_layout(rect=(0, 0, 1, 0.84))
fig.savefig(os.path.join(HERE, "sstB_1851_versions.png"), dpi=150)
print("wrote sstB_1851_versions.png")
