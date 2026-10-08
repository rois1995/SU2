#!/usr/bin/env python3
"""Summarize fixed-mesh RANS experiments, including CFL bursts and interior SA floors."""
import argparse
import csv
import json
from pathlib import Path

import numpy as np

from run_convergence import read_options
from summarize_fixes import history


def wall_points(mesh, tags):
    lines = mesh.read_text().splitlines()
    wall = set()
    for index, line in enumerate(lines):
        if line.strip().startswith("MARKER_TAG=") and line.split("=", 1)[1].strip() in tags:
            count = int(lines[index + 1].split("=")[1])
            for element in lines[index + 2:index + 2 + count]:
                wall.update(map(int, element.split()[1:]))
    return wall


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaigns", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = {"runs": [], "limits": ["MGLEVEL=0, serial nice=19; host contention precludes timing claims.",
              "A burst is a density log-residual rise exceeding 0.2 in one iteration; this is descriptive.",
              "All results concern fixed RANS boundary-layer meshes; forces alone do not establish accuracy."]}
    series, controls = [], {}
    report["comparisons"] = []
    for campaign in args.campaigns:
        for result in json.loads((campaign / "results.json").read_text()):
            directory = campaign / result["name"]
            rows = history(directory / "history.csv")
            if not rows or not result["finite_residuals"]:
                report["runs"].append(result)
                continue
            opts = {key: value.strip() for key, value in read_options(result["config"]).items()}
            assert opts["MGLEVEL"] == "0"
            record = dict(result)
            tail = rows[-100:]
            keys = [key for key in rows[0] if key.startswith("rms[")]
            record["tail100"] = {key: {"median": float(np.median([row[key] for row in tail])),
                                     "min": min(row[key] for row in tail), "max": max(row[key] for row in tail)}
                                 for key in keys + ["Avg CFL", "CL", "CD"]}
            density = np.array([row["rms[Rho]"] for row in rows])
            cfl = np.array([row["Avg CFL"] for row in rows])
            bursts = np.flatnonzero(np.diff(density) > 0.2) + 1
            record["density_bursts"] = [{"iteration": int(rows[i]["Inner_Iter"]),
                                         "rise": float(density[i] - density[i-1]), "cfl": float(cfl[i])}
                                        for i in bursts]
            record["cfl_drops_over_30_percent"] = int(np.sum(cfl[1:] < 0.7 * cfl[:-1]))
            monitors = ["rms[Rho]", "rms[nu]"] if "rms[nu]" in rows[0] else ["rms[Rho]", "rms[k]", "rms[w]"]
            record["monitored_target_met"] = all(rows[-1][key] <= -8 for key in monitors)
            fields = directory / "fields.csv"
            if fields.exists():
                with fields.open() as stream:
                    columns = next(csv.reader(stream))
                values = np.loadtxt(fields, delimiter=",", skiprows=1)
                get = lambda key: values[:, columns.index(key)]
                rho = get("Density")
                momentum2 = sum(get(key)**2 for key in ("Momentum_x", "Momentum_y", "Momentum_z") if key in columns)
                internal = get("Energy") - momentum2 / (2 * rho)
                record["state"] = {"finite": bool(np.isfinite(values).all()), "min_density": float(rho.min()),
                                   "min_internal_energy_density": float(internal.min())}
                if "Nu_Tilde" in columns:
                    tags = opts["MARKER_HEATFLUX"].strip("() ").split(",")[::2]
                    wall = wall_points(Path(opts["MESH_FILENAME"]), {tag.strip() for tag in tags})
                    assert wall, "Boundary vertices must be identified before counting interior floors"
                    ids = get("PointID").astype(int)
                    interior = ~np.isin(ids, list(wall))
                    clipped = (get("Nu_Tilde") <= 1e-15) & interior
                    record["state"]["interior_sa_floor_ids"] = ids[clipped].tolist()
                    record["state"]["interior_sa_floor_count"] = int(clipped.sum())
            report["runs"].append(record)
            series.append((result["name"], rows))
            inputs = result["inputs"]
            group = (inputs["mesh"]["sha256"], inputs.get("restart", {}).get("sha256"), opts["KIND_TURB_MODEL"])
            if group not in controls:
                controls[group] = (result["name"], rows, opts)
            else:
                name, reference, settings = controls[group]
                by_iteration = {row["Inner_Iter"]: row for row in rows}
                common = [row for row in reference if row["Inner_Iter"] in by_iteration]
                assert common, "Matched inputs require at least one common history iteration"
                before = common[-1]
                after = by_iteration[before["Inner_Iter"]]
                report["comparisons"].append({"reference": name, "candidate": result["name"],
                    "common_iteration": int(before["Inner_Iter"]),
                    "residual_log10_reduction": {key: before[key] - after[key] for key in keys},
                    "changed_options": {key: [settings.get(key), opts.get(key)]
                                        for key in set(settings) | set(opts) if settings.get(key) != opts.get(key)}})
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    for prefix in ("sa_", "sst_", "adapted_"):
        selected = [(name, rows) for name, rows in series if name.startswith(prefix)]
        if not selected:
            continue
        fig, axes = plt.subplots(3, 1, figsize=(10, 9), sharex=True)
        for name, rows in selected:
            iteration = [row["Inner_Iter"] for row in rows]
            for axis, key in zip(axes, ["rms[Rho]", "rms[nu]" if prefix != "sst_" else "rms[w]", "Avg CFL"]):
                axis.plot(iteration, [row[key] for row in rows], label=name, linewidth=1)
                axis.set_ylabel(key)
                axis.grid(alpha=0.2)
        axes[0].legend(fontsize=7)
        axes[-1].set_xlabel("Continuation iteration")
        fig.suptitle("RAE2822 RANS: fixed mesh, MGLEVEL=0")
        fig.tight_layout()
        for extension in ("png", "svg"):
            fig.savefig(args.output.with_name(args.output.stem + "_" + prefix.rstrip("_") + "." + extension), dpi=150)
        plt.close(fig)
    for record in report["runs"]:
        print(record["name"], record["status"], record.get("tail100", {}), record.get("state", {}))


if __name__ == "__main__":
    main()
