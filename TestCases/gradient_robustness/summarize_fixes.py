#!/usr/bin/env python3
"""Archive matched RANS evidence and plot residual histories; never infer accuracy from forces."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re

import numpy as np


def history(path):
    with path.open() as stream:
        return [{key.strip().strip('"'): float(value) for key, value in row.items()}
                for row in csv.DictReader(stream)]


def options(text):
    return dict(re.findall(r"^\s*([A-Z][A-Z0-9_]*)\s*=\s*([^%\n]*)", text, re.MULTILINE))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaign", type=Path)
    parser.add_argument("output", type=Path, help="JSON result; figures use the same stem")
    parser.add_argument("--expected-runs", type=int, default=20)
    args = parser.parse_args()
    root = args.campaign.resolve()
    runs = json.loads((root / "results.json").read_text())
    assert len(runs) == args.expected_runs, "Wait for all campaign variants before archiving"
    report = {"manifest": json.loads((root / "manifest.json").read_text()), "runs": [], "pairs": [],
              "limits": ["MGLEVEL=0; serial nice=19 on a contended host; timings are not benchmarks.",
                         "Earlier native/adapted stress grids are excluded; bl_* identifies the selected RANS grids.",
                         "Residual thresholds are case-specific monitors, not an accuracy proof.",
                         "Force changes on nonconverged states do not establish better accuracy."]}
    histories, configs = {}, {}
    for run in runs:
        name = run["case"] + "_" + run["variant"]
        directory = root / name
        text = (directory / "case.cfg").read_text()
        assert hashlib.sha256(text.encode()).hexdigest() == run["config_sha256"]
        config = options(text)
        assert config["MGLEVEL"].strip() == "0"
        rows = history(directory / "history.csv")
        assert len(rows) == run["history_rows"]
        monitors = ["rms[Rho]"] + (["rms[k]", "rms[w]"] if "sst" in name else ["rms[nu]"])
        assert all(key in rows[-1] for key in monitors)
        finite = all(np.isfinite(row[key]) for row in rows for key in row if key.startswith("rms["))
        record = dict(run, config=text, residual_monitors=monitors, residuals_finite=bool(finite),
                      convergence_assessment_eligible=run["case"].startswith("bl_"),
                      residual_target_met=bool(finite and all(rows[-1][key] <= -8 for key in monitors)))
        tail = rows[-100:]
        record["last_100_iterations"] = {
            key: {"min": min(row[key] for row in tail), "max": max(row[key] for row in tail),
                  "median": float(np.median([row[key] for row in tail]))}
            for key in monitors + ["CL", "CD", "Avg CFL"]}
        fields = directory / "fields.csv"
        if fields.exists():
            with fields.open() as stream:
                columns = next(csv.reader(stream))
            values = np.loadtxt(fields, delimiter=",", skiprows=1)
            get = lambda column: values[:, columns.index(column)]
            density = get("Density")
            internal = get("Energy") - (get("Momentum_x") ** 2 + get("Momentum_y") ** 2) / (2 * density)
            record["state"] = {"finite": bool(np.isfinite(values).all()), "points": len(values),
                               "min_density": float(density.min()), "min_internal_energy_density": float(internal.min()),
                               "sha256": hashlib.sha256(fields.read_bytes()).hexdigest()}
            if "Nu_Tilde" in columns:
                record["state"]["nu_at_floor_count"] = int((get("Nu_Tilde") <= 1e-15).sum())
        report["runs"].append(record)
        histories[name], configs[name] = rows, config
    for case in dict.fromkeys(run["case"] for run in runs):
        control = case + "_current"
        for run in [item for item in runs if item["case"] == case and item["variant"] != "current"]:
            name = case + "_" + run["variant"]
            a, b = dict(configs[control]), dict(configs[name])
            a.pop("LIMITER_LOCAL_LENGTH", None); b.pop("LIMITER_LOCAL_LENGTH", None)
            assert a == b, "Unmatched solver settings: " + name
            by_iter = {row["Inner_Iter"]: row for row in histories[name]}
            common = [row for row in histories[control] if row["Inner_Iter"] in by_iter]
            assert common
            before = common[-1]; after = by_iter[before["Inner_Iter"]]
            pair = {"case": case, "reference": "current", "variant": run["variant"],
                    "convergence_assessment_eligible": case.startswith("bl_"),
                    "common_iteration": int(before["Inner_Iter"]),
                    "residual_log10_change": {key: before[key] - after[key] for key in before if key.startswith("rms[")},
                    "force_difference": {key: after[key] - before[key] for key in ("CL", "CD")}}
            report["pairs"].append(pair)
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    colors = {"main": "0.6", "current": "#bd3f32", "fixed": "#205b9e", "fixed_local": "#29854a"}
    for field, suffix in [("rms[Rho]", "density"), (None, "turbulence")]:
        fig, axes = plt.subplots(1, 2, figsize=(11, 4), sharex=False)
        cases = [case for case in dict.fromkeys(run["case"] for run in runs) if case.startswith("bl_")]
        for axis, case in zip(axes.flat, cases):
            for run in [item for item in runs if item["case"] == case]:
                rows = histories[case + "_" + run["variant"]]
                keys = [field] if field else (["rms[k]", "rms[w]"] if "sst" in case else ["rms[nu]"])
                for key in keys:
                    axis.plot([row["Inner_Iter"] for row in rows], [row[key] for row in rows],
                              color=colors[run["variant"]], linestyle="--" if key == "rms[w]" else "-",
                              label=run["variant"] + (" " + key if len(keys) > 1 else ""))
            axis.axhline(-8, color="0.4", linestyle=":", linewidth=.8)
            axis.set_title(case.replace("_", " ")); axis.set_xlabel("Iteration")
            axis.set_ylabel("log10 RMS residual"); axis.grid(alpha=.2); axis.legend(fontsize=8)
        fig.suptitle("RAE2822 boundary-layer mesh: " + suffix + " residuals, MGLEVEL=0")
        fig.tight_layout()
        for extension in ("png", "svg"):
            fig.savefig(args.output.with_name(args.output.stem + "_" + suffix + "." + extension), dpi=150)
        plt.close(fig)


if __name__ == "__main__":
    main()
