from pathlib import Path
import csv
import json
from collections import defaultdict
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

root = Path(__file__).resolve().parent

def read_field(case):
    with (root / case / "restart_flow.csv").open() as stream:
        return [{k.strip().strip('"'): float(v) for k, v in row.items()} for row in csv.DictReader(stream)]

def average_sections(field):
    sections = defaultdict(list)
    for row in field:
        sections[row["x"]].append(row)
    result = []
    for x, rows in sorted(sections.items()):
        rows.sort(key=lambda row: row["y"])
        height = rows[-1]["y"] - rows[0]["y"]
        assert height > 0
        values = {key: sum(.5 * (left[key] + right[key]) * (right["y"] - left["y"])
                          for left, right in zip(rows, rows[1:])) / height
                  for key in ["Mach", "Pressure"]}
        assert all(math.isfinite(value) for value in values.values())
        result.append((x, values))
    return result

before = read_field("air_nozzle_before")
after = read_field("air_nozzle_after")
control = read_field("air_nozzle_regular")
assert len(after) == len(control)
comparison = {}
for key in ["Density", "Pressure", "Momentum_x", "Momentum_y", "Energy", "Mach"]:
    comparison[key] = max(abs(a[key] - b[key]) for a, b in zip(after, control)) / max(abs(a[key]) for a in after)
(root / "air_control_field_difference.json").write_text(json.dumps(comparison, indent=2) + "\n")
assert max(comparison.values()) < 1e-10

p0 = 101325 * 1.162**3.5
reference = []
for i in range(101):
    x = i / 100
    area_ratio = (.1 - .02 * math.sin(math.pi * x)**2) / .08
    lo, hi = (1e-6, 1.) if x < .5 else (1., 3.)
    for _ in range(80):
        mach = .5 * (lo + hi)
        area = ((2 / 2.4) * (1 + .2 * mach**2))**3 / mach
        if (area > area_ratio) == (x < .5):
            lo = mach
        else:
            hi = mach
    mach = .5 * (lo + hi)
    assert abs(((2 / 2.4) * (1 + .2 * mach**2))**3 / mach - area_ratio) < 1e-12
    reference.append((x, {"Mach": mach, "Pressure": p0 / (1 + .2 * mach**2)**3.5}))

fig, axes = plt.subplots(1, 2, figsize=(11, 5), constrained_layout=False)
fig.subplots_adjust(left=.08, right=.98, bottom=.25, top=.87, wspace=.28)
for ax, key, scale, ylabel in zip(axes, ["Mach", "Pressure"], [1, 1e-3], ["Cross-section average Mach", "Cross-section average pressure [kPa]"]):
    for field, label, color in [(before, "Old reconstruction (lookup corrected)", "#c76b22"),
                                (after, "Corrected TurboRiemann / regular Riemann", "#176ba0")]:
        data = average_sections(field)
        ax.plot([x for x, _ in data], [values[key] * scale for _, values in data], label=label, color=color, lw=2.5)
    ax.plot([x for x, _ in reference], [values[key] * scale for _, values in reference], color="#555555", ls="--", lw=1.3, label="Quasi-1D isentropic reference")
    ax.axvline(.5, color="#aaaaaa", ls=":", lw=1)
    ax.set(xlabel="Axial coordinate x [m]", ylabel=ylabel, xlim=(0, 1.025))
    ax.grid(alpha=.2)
axes[0].axhline(1, color="#aaaaaa", ls=":", lw=1)
axes[0].legend(fontsize=8, loc="upper left")
fig.suptitle("Pressure-outlet repair removes an artificial jump at the nozzle exit", fontsize=12)
fig.text(.5, .035, "Fixed walls; 100×8 cells; first-order HLLC. Both old/new runs reach density residual −10 in 352 iterations.\nRequested outlet pressure: 1000 Pa; the corrected supersonic outlet extrapolates the interior state.", ha="center", fontsize=9)
for suffix in ["png", "svg"]:
    fig.savefig(root / ("air_nozzle_outlet_comparison." + suffix), dpi=180, bbox_inches="tight")
