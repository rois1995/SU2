#!/usr/bin/env python3
"""Tiny local SST/EPM restart and extraction regression; no external mesh needed."""

import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import signal
import time


def read_csv(path):
    with path.open() as stream:
        return [{key.strip(): value.strip() for key, value in row.items()}
                for row in csv.DictReader(stream, skipinitialspace=True)]


def read_config(path):
    config = {}
    for line in path.read_text().splitlines():
        line = line.split("%", 1)[0].strip()
        if line:
            key, value = line.split("=", 1)
            config[key.strip()] = value.strip()
    return config


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path, help="SU2_CFD_AD executable")
    parser.add_argument("workdir", type=Path, help="new directory for all test outputs")
    parser.add_argument("--dimension", type=int, choices=(2, 3), default=2)
    parser.add_argument("--ranks", type=int, choices=(1, 2), default=1)
    parser.add_argument("--reference", type=Path, help="serial test directory: reuse its primal and compare sensitivities")
    args = parser.parse_args()
    if args.ranks > 1 and args.reference is None:
        parser.error("MPI checks require --reference pointing to the matching serial test")
    binary = args.binary.resolve(strict=True)
    root = args.workdir.resolve()
    root.mkdir(parents=True, exist_ok=False)
    config = read_config(Path(__file__).with_name("smoke.cfg"))
    if args.dimension == 3:
        config.update(MESH_FORMAT="BOX", MESH_BOX_SIZE="(9, 7, 5)", AOA="7", CFL_NUMBER="20",
                      MARKER_FAR="(x_minus, x_plus, y_plus, z_minus, z_plus)", REF_AREA="0.1")
        config["CONV_FIELD"] = config["CONV_FIELD"].replace("RMS_ENERGY", "RMS_MOMENTUM-Z, RMS_ENERGY")
    command = [str(binary), "run.cfg"]
    if args.ranks > 1:
        command = ["mpirun", "--host", "localhost", "--bind-to", "none", "-np", str(args.ranks)] + command
    timings = {}

    def run(name, overrides):
        directory = root / name
        directory.mkdir()
        options = dict(config, **overrides)
        (directory / "run.cfg").write_text("".join(f"{k}= {v}\n" for k, v in options.items()))
        start = time.monotonic()
        with (directory / "run.log").open("w") as log:
            process = subprocess.Popen(command, cwd=directory, stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            try:
                returncode = process.wait(timeout=300)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                raise
        timings[name] = time.monotonic() - start
        if returncode:
            raise RuntimeError(f"{name} failed: {(directory / 'run.log').read_text()[-6000:]}")
        assert "All convergence criteria satisfied." in (directory / "run.log").read_text()
        history = read_csv(directory / "history.csv")
        residuals = {k: float(v) for k, v in history[-1].items() if k.startswith("rms[")}
        assert residuals and all(math.isfinite(v) and v <= -10 for v in residuals.values()), residuals
        print(f"{name}: converged in {len(history)} history rows, {timings[name]:.2f}s", flush=True)
        return directory, history

    if args.reference:
        baseline = args.reference.resolve(strict=True) / "baseline"
        assert read_config(baseline / "run.cfg") == config, "Reference case differs from the requested case"
        primal_history = read_csv(baseline / "history.csv")
        assert all(float(v) <= -10 for k, v in primal_history[-1].items() if k.startswith("rms["))
    else:
        baseline, primal_history = run("baseline", {})
    primal_restart = baseline / "restart_flow.dat"
    original_hash = hashlib.sha256(primal_restart.read_bytes()).hexdigest()
    adjoint = {
        "MATH_PROBLEM": "DISCRETE_ADJOINT",
        "SOLUTION_FILENAME": str(baseline / "restart_flow"),
        "CONV_FIELD": "(RMS_ADJ_DENSITY, RMS_ADJ_MOMENTUM-X, RMS_ADJ_MOMENTUM-Y, "
                      "RMS_ADJ_ENERGY, RMS_ADJ_TKE, RMS_ADJ_DISSIPATION)",
        "SCREEN_OUTPUT": "(INNER_ITER, RMS_ADJ_DENSITY, RMS_ADJ_TKE, RMS_ADJ_DISSIPATION)",
    }
    if args.dimension == 3:
        adjoint["CONV_FIELD"] = adjoint["CONV_FIELD"].replace("RMS_ADJ_ENERGY", "RMS_ADJ_MOMENTUM-Z, RMS_ADJ_ENERGY")
    off, off_history = run("disabled", adjoint)
    on, on_history = run("enabled", dict(adjoint, SST_EPM_POSTPROCESS="YES"))
    assert not (off / "sst_epm_sensitivities.csv").exists()
    assert len(off_history) == len(on_history)
    # Enabling postprocessing must not change the coupled adjoint iteration.
    for off_row, on_row in zip(off_history, on_history):
        for key in off_row:
            if key.startswith("rms["):
                assert off_row[key] == on_row[key], (key, off_row[key], on_row[key])

    rows = read_csv(on / "sst_epm_sensitivities.csv")
    canonical = {r["case"]: r for r in rows if r["probe_pair"] == "3"}
    assert set(canonical) == {"1C_KEEP", "1C_SWAP", "2C_KEEP", "2C_SWAP", "3C"}
    assert len({(r["case"], r["probe_pair"]) for r in rows}) == len(rows)
    assert len(rows) <= 17
    sizes = [int(v.strip()) for v in config["MESH_BOX_SIZE"].strip("()").split(",")][:args.dimension]
    edges = sum((n-1) * math.prod(sizes[:i] + sizes[i+1:]) for i, n in enumerate(sizes))
    for row in rows:
        assert int(row["dimension"]) == args.dimension and int(row["mpi_ranks"]) == args.ranks
        assert int(row["stress_evaluations"]) >= edges, "Interior viscous edges were not all perturbed"
        assert int(row["source_evaluations"]) >= math.prod(sizes) - math.prod(sizes[:1] + sizes[2:])
        assert int(row["total_evaluations"]) == int(row["stress_evaluations"]) + int(row["source_evaluations"])
        base, gradient, prediction = (float(row[k]) for k in
                                      ("J_baseline", "dJ_da_at_zero", "J_linear_at_one"))
        assert all(math.isfinite(v) for v in (base, gradient, prediction))
        assert math.isclose(base, float(primal_history[-1]["CD"]), rel_tol=1e-8, abs_tol=1e-9)
        assert math.isclose(prediction, base + gradient, rel_tol=1e-12, abs_tol=1e-14)
        assert row["status"] == "frozen_forcing_unverified" and row["adjoint_converged"] == "true"
    assert all(abs(float(r["dJ_da_at_zero"])) > 1e-12 for r in canonical.values())
    restarted, _ = run("restarted", dict(adjoint, SST_EPM_POSTPROCESS="YES", RESTART_SOL="YES",
                                         SOLUTION_ADJ_FILENAME=str(on / "restart_adj")))
    resumed_rows = read_csv(restarted / "sst_epm_sensitivities.csv")
    assert len(rows) == len(resumed_rows)
    for first, resumed in zip(rows, resumed_rows):
        assert first["case"] == resumed["case"] and first["probe_pair"] == resumed["probe_pair"]
        assert math.isclose(float(first["dJ_da_at_zero"]), float(resumed["dJ_da_at_zero"]),
                            rel_tol=1e-5, abs_tol=1e-9), (first, resumed)
    assert hashlib.sha256(primal_restart.read_bytes()).hexdigest() == original_hash
    if args.reference:
        reference = read_csv(args.reference / "enabled" / "sst_epm_sensitivities.csv")
        assert len(rows) == len(reference)
        for row, ref in zip(rows, reference):
            assert (row["case"], row["probe_pair"]) == (ref["case"], ref["probe_pair"])
            assert math.isclose(float(row["dJ_da_at_zero"]), float(ref["dJ_da_at_zero"]),
                                rel_tol=1e-5, abs_tol=1e-9), (row, ref)
    summary = {"binary": str(binary), "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
               "dimension": args.dimension, "ranks": args.ranks,
               "primal_restart_sha256": original_hash,
               "timings_seconds": timings, "primal_final_residuals": {
                   k: v for k, v in primal_history[-1].items() if k.startswith("rms[")},
               "canonical_gradients": {k: float(v["dJ_da_at_zero"]) for k, v in canonical.items()},
               "stress_evaluations": int(rows[0]["stress_evaluations"]),
               "source_evaluations": int(rows[0]["source_evaluations"]),
               "J_baseline": float(rows[0]["J_baseline"]),
               "max_restart_gradient_abs_error": max(abs(float(a["dJ_da_at_zero"])-float(b["dJ_da_at_zero"]))
                                                     for a, b in zip(rows, resumed_rows)),
               "adjoint_final_residuals": {k: v for k, v in on_history[-1].items() if k.startswith("rms[")},
               "rows": len(rows), "status": "passed_infrastructure_smoke_test"}
    (root / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
