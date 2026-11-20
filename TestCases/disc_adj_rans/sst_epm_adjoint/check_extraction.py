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
import sys
import time

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import read_epm_field as epm_field  # noqa: E402  (sits next to this script)

FIELD_RELATIVE_TOLERANCE = 1e-10
FIELD_KINDS = {"EDGE": 0, "BOUNDARY": 1, "NODE": 2}


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


def mpi_command(ranks):
    """mpirun prefix. Under an SGE parallel environment (PE_HOSTFILE set) OpenMPI takes the allocated hosts itself,
    so --host localhost must not be passed."""
    if os.environ.get("PE_HOSTFILE"):
        return ["mpirun", "--bind-to", "none", "-np", str(ranks)]
    return ["mpirun", "--host", "localhost", "--bind-to", "none", "-np", str(ranks)]


def check_field_files(directory, canonical, edges, source_minimum):
    """Check the EPM field files written in `directory` against the CSV rows `canonical` (probe_pair == 3).

    Returns (merged fields, {"bytes": total .bin size, "sites": {kind: merged site count with uses > 0}})."""
    objectives = {r["objective"] for r in canonical.values()}
    assert len(objectives) == 1, objectives
    objective = objectives.pop()
    header_path = directory / f"sst_epm_field_{objective}.json"
    assert header_path.is_file(), f"{header_path} was not written"
    csv_path = directory / "sst_epm_sensitivities.csv"
    header = json.loads(header_path.read_text())
    kinds = header.get("kinds", FIELD_KINDS)
    assert kinds == FIELD_KINDS, kinds
    columns = header["columns"]
    assert header["dtype"] == "float64" and header["record_bytes"] == 8 * len(columns), header["record_bytes"]
    assert header["objective"] == objective, (header["objective"], objective)
    assert len(header["ranks"]) == header["mpi_ranks"], header["ranks"]
    total_bytes = 0
    for entry in header["ranks"]:
        size = (directory / entry["file"]).stat().st_size
        assert size == entry["count"] * header["record_bytes"], (entry, size, header["record_bytes"])
        total_bytes += size
    # The reader recomputes the five classical gradients from the files and compares them with the CSV.
    reader = subprocess.run([sys.executable, str(Path(epm_field.__file__).resolve()), str(header_path),
                             "--csv", str(csv_path)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            universal_newlines=True)
    assert reader.returncode == 0, f"read_epm_field.py failed on {header_path}:\n{reader.stdout}"
    fields = epm_field.load(header_path)
    kind = fields["kind"]
    uses = fields["uses"]
    # A site stands for its repeated uses: unmerged (all ranks, halo copies included) the uses add up to the
    # evaluations the CSV counts.
    stress_uses = math.fsum(uses[(kind == kinds["EDGE"]) | (kind == kinds["BOUNDARY"])].tolist())
    source_uses = math.fsum(uses[kind == kinds["NODE"]].tolist())
    for row in canonical.values():
        assert stress_uses == int(row["stress_evaluations"]), (stress_uses, row["stress_evaluations"])
        assert source_uses == int(row["source_evaluations"]), (source_uses, row["source_evaluations"])
    merged = epm_field.merge(fields)
    sites = {name: int(np.count_nonzero((merged["kind"] == value) & (merged["uses"] > 0)))
             for name, value in kinds.items()}
    assert sites["EDGE"] >= edges, (sites, edges)
    assert sites["NODE"] >= source_minimum, (sites, source_minimum)
    return merged, {"bytes": total_bytes, "sites": sites}


def assert_no_field_files(directory):
    found = sorted(p.name for p in directory.glob("sst_epm_field_*"))
    assert not found, f"{directory.name} must not write field files, found {found}"


def compare_fields(reference, other, used_only=False):
    """Compare the merged S_* columns of two fields site by site; returns the largest |a-b| / max|S of the column|.

    Tolerance per value: max(1e-10 * max(|a|, |b|), 1e-10 * max|S| over the column in both fields).
    With used_only, only the sites with uses > 0 are kept in each field before comparing: different partitions
    register different unused (halo) sites, so only the evaluated sites are comparable, and their key sets must
    agree."""
    if used_only:
        def keep_used(fields):
            mask = fields["uses"] > 0
            return {name: values[mask] for name, values in fields.items()}
        reference, other = keep_used(reference), keep_used(other)

    def order(fields):
        keys = [fields[name] for name in epm_field.KEY_COLUMNS]
        return np.lexsort(keys[::-1]), np.column_stack(keys)

    first, first_keys = order(reference)
    second, second_keys = order(other)
    assert first_keys.shape == second_keys.shape and np.array_equal(first_keys[first], second_keys[second]), \
        "The two fields do not hold the same set of " + ("used " if used_only else "") + "sites"
    names = [n for n in reference if n.startswith("S_")]
    assert len([n for n in names if n.startswith("S_tau_")]) > 0 and "S_pk" in names and "S_pw" in names, names
    worst = 0.0
    for name in names:
        a, b = reference[name][first], other[name][second]
        assert np.all(np.isfinite(a)) and np.all(np.isfinite(b)), f"{name} has nonfinite values"
        scale = max(float(np.max(np.abs(a), initial=0.0)), float(np.max(np.abs(b), initial=0.0)))
        difference = np.abs(a - b)
        allowed = np.maximum(FIELD_RELATIVE_TOLERANCE * np.maximum(np.abs(a), np.abs(b)),
                             FIELD_RELATIVE_TOLERANCE * scale)
        bad = np.flatnonzero(difference > allowed)
        assert bad.size == 0, (name, first_keys[first][bad[0]].tolist(), float(a[bad[0]]), float(b[bad[0]]), scale)
        if scale > 0:
            worst = max(worst, float(np.max(difference)) / scale)
    return worst


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
        command = mpi_command(args.ranks) + command
    timings = {}

    def launch(name, overrides):
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
        return directory, returncode

    def run_expect_failure(name, overrides, substring):
        directory, returncode = launch(name, overrides)
        output = (directory / "run.log").read_text()
        if returncode == 0 or substring not in output:
            raise RuntimeError(f"{name}: expected a failure with {substring!r}, got exit code {returncode}: "
                               f"{output[-6000:]}")
        print(f"{name}: rejected as expected, {timings[name]:.2f}s", flush=True)

    def run(name, overrides):
        directory, returncode = launch(name, overrides)
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
    scalar, scalar_history = run("scalar", dict(adjoint, SST_EPM_POSTPROCESS="YES",
                                                SST_EPM_EXTRACTION="SCALAR_REPLAY"))
    assert not (off / "sst_epm_sensitivities.csv").exists()
    # Enabling postprocessing (either extraction) must not change the coupled adjoint iteration.
    for history in (on_history, scalar_history):
        assert len(off_history) == len(history)
        for off_row, on_row in zip(off_history, history):
            for key in off_row:
                if key.startswith("rms["):
                    assert off_row[key] == on_row[key], (key, off_row[key], on_row[key])

    objective_suffix = {"DRAG": "_cd", "LIFT": "_cl"}[config["OBJECTIVE_FUNCTION"]]
    adjoint_restart = on / f"restart_adj{objective_suffix}.dat"
    adjoint_hash = hashlib.sha256(adjoint_restart.read_bytes()).hexdigest()
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
        assert row["extraction"] == "field", row["extraction"]
    assert all(abs(float(r["dJ_da_at_zero"])) > 1e-12 for r in canonical.values())
    assert {r["objective"] for r in canonical.values()} == {config["OBJECTIVE_FUNCTION"]}
    source_minimum = math.prod(sizes) - math.prod(sizes[:1] + sizes[2:])
    enabled_field, enabled_info = check_field_files(on, canonical, edges, source_minimum)
    # The field replay must reproduce the scalar-amplitude replay row by row.
    scalar_rows = {(r["case"], r["probe_pair"]): r for r in read_csv(scalar / "sst_epm_sensitivities.csv")}
    assert set(scalar_rows) == {(r["case"], r["probe_pair"]) for r in rows}, (sorted(scalar_rows), rows)
    assert_no_field_files(scalar)
    assert_no_field_files(off)
    max_field_scalar_error = 0.0
    for row in rows:
        reference = scalar_rows[(row["case"], row["probe_pair"])]
        assert reference["extraction"] == "scalar_replay", reference["extraction"]
        for key in ("dJ_da_at_zero", "J_linear_at_one"):
            assert math.isclose(float(row[key]), float(reference[key]), rel_tol=1e-10, abs_tol=1e-14), \
                (key, row, reference)
        for key in ("ambiguous_evaluations", "total_evaluations", "stress_evaluations", "source_evaluations"):
            assert int(row[key]) == int(reference[key]), (key, row, reference)
        max_field_scalar_error = max(max_field_scalar_error,
                                     abs(float(row["dJ_da_at_zero"]) - float(reference["dJ_da_at_zero"])))
    restarted, _ = run("restarted", dict(adjoint, SST_EPM_POSTPROCESS="YES", RESTART_SOL="YES",
                                         SOLUTION_ADJ_FILENAME=str(on / "restart_adj")))
    resumed_rows = read_csv(restarted / "sst_epm_sensitivities.csv")
    assert len(rows) == len(resumed_rows)
    assert all(r["extraction"] == "field" for r in resumed_rows)
    for first, resumed in zip(rows, resumed_rows):
        assert first["case"] == resumed["case"] and first["probe_pair"] == resumed["probe_pair"]
        assert math.isclose(float(first["dJ_da_at_zero"]), float(resumed["dJ_da_at_zero"]),
                            rel_tol=1e-5, abs_tol=1e-9), (first, resumed)
    # Extract-only: load the primal and the adjoint saved by "enabled", measure the residual with one iteration
    # whose update is discarded, and replay from the loaded adjoint; it must reproduce "enabled" to round-off.
    extract_only_options = dict(adjoint, SST_EPM_POSTPROCESS="YES", SST_EPM_EXTRACT_ONLY="YES",
                                RESTART_SOL="YES", ITER="1")
    extracted, extracted_history = run("extract_only", dict(extract_only_options,
                                                            SOLUTION_ADJ_FILENAME=str(on / "restart_adj"),
                                                            RESTART_ADJ_FILENAME="restart_adj_out"))
    assert len(extracted_history) == 1, len(extracted_history)
    extracted_log = (extracted / "run.log").read_text()
    assert "EPM extract-only: residual of one adjoint iteration" in extracted_log and "): passed" in extracted_log
    extracted_rows = read_csv(extracted / "sst_epm_sensitivities.csv")
    assert len(rows) == len(extracted_rows)
    for first, again in zip(rows, extracted_rows):
        assert (first["case"], first["probe_pair"]) == (again["case"], again["probe_pair"]), (first, again)
        assert again["extraction"] == "field", again["extraction"]
        assert math.isclose(float(first["dJ_da_at_zero"]), float(again["dJ_da_at_zero"]),
                            rel_tol=1e-10, abs_tol=1e-14), (first, again)
    extracted_canonical = {r["case"]: r for r in extracted_rows if r["probe_pair"] == "3"}
    assert set(extracted_canonical) == set(canonical), sorted(extracted_canonical)
    extracted_field, extracted_info = check_field_files(extracted, extracted_canonical, edges, source_minimum)
    max_extract_only_field_error = compare_fields(enabled_field, extracted_field)
    assert hashlib.sha256(adjoint_restart.read_bytes()).hexdigest() == adjoint_hash
    written_adjoint = extracted / f"restart_adj_out{objective_suffix}.dat"
    assert written_adjoint.exists(), written_adjoint
    # Config rejections abort during parsing; none of these reaches the solver.
    run_expect_failure("reject_no_restart", dict(extract_only_options, RESTART_SOL="NO",
                                                 SOLUTION_ADJ_FILENAME="restart_adj_in"),
                       "SST_EPM_EXTRACT_ONLY= YES requires RESTART_SOL= YES")
    run_expect_failure("reject_iter", dict(extract_only_options, ITER="5", SOLUTION_ADJ_FILENAME="restart_adj_in"),
                       "SST_EPM_EXTRACT_ONLY= YES requires RESTART_SOL= YES")
    # Same effective name: the .dat extension is stripped before the objective suffix is added.
    run_expect_failure("reject_same_adjoint_file", dict(extract_only_options, SOLUTION_ADJ_FILENAME="restart_adj",
                                                        RESTART_ADJ_FILENAME="restart_adj.dat"),
                       "requires SOLUTION_ADJ_FILENAME to differ from RESTART_ADJ_FILENAME")
    assert hashlib.sha256(primal_restart.read_bytes()).hexdigest() == original_hash
    mpi_errors = {}
    if args.reference:
        serial = args.reference.resolve(strict=True)
        reference = read_csv(serial / "enabled" / "sst_epm_sensitivities.csv")
        assert len(rows) == len(reference)
        for row, ref in zip(rows, reference):
            assert (row["case"], row["probe_pair"]) == (ref["case"], ref["probe_pair"])
            assert math.isclose(float(row["dJ_da_at_zero"]), float(ref["dJ_da_at_zero"]),
                                rel_tol=1e-5, abs_tol=1e-9), (row, ref)
    if args.reference and args.ranks > 1:
        # Two ranks against serial, extract-only from the SERIAL primal and adjoint: any difference is then due to
        # the partitioning of the extraction alone, so the tolerance is round-off.
        serial_adjoint = serial / "enabled" / f"restart_adj{objective_suffix}.dat"
        serial_adjoint_hash = hashlib.sha256(serial_adjoint.read_bytes()).hexdigest()
        serial_canonical = {r["case"]: r for r in reference if r["probe_pair"] == "3"}
        assert set(serial_canonical) == set(canonical), sorted(serial_canonical)
        assert all(int(r["mpi_ranks"]) == 1 for r in reference), "The reference must be a serial run"
        serial_field, _ = check_field_files(serial / "enabled", serial_canonical, edges, source_minimum)
        serial_scalar = read_csv(serial / "scalar" / "sst_epm_sensitivities.csv")
        mpi_options = dict(extract_only_options, SOLUTION_ADJ_FILENAME=str(serial / "enabled" / "restart_adj"),
                           RESTART_ADJ_FILENAME="restart_adj_out")

        def compare_rows(name, mpi_rows, serial_rows, extraction):
            """Same (case, probe_pair) rows and dJ_da_at_zero to round-off; returns the largest relative error."""
            assert len(mpi_rows) == len(serial_rows), (name, len(mpi_rows), len(serial_rows))
            serial_by_key = {(r["case"], r["probe_pair"]): r for r in serial_rows}
            assert len(serial_by_key) == len(serial_rows)
            assert [(r["case"], r["probe_pair"]) for r in mpi_rows] == [(r["case"], r["probe_pair"]) for r in serial_rows], name
            worst = 0.0
            for row in mpi_rows:
                ref = serial_by_key[(row["case"], row["probe_pair"])]
                assert int(row["mpi_ranks"]) == args.ranks and row["extraction"] == extraction, (name, row)
                a, b = float(row["dJ_da_at_zero"]), float(ref["dJ_da_at_zero"])
                assert math.isclose(a, b, rel_tol=1e-10, abs_tol=1e-14), (name, row, ref)
                if max(abs(a), abs(b)) > 0:
                    worst = max(worst, abs(a - b) / max(abs(a), abs(b)))
            return worst

        mpi_field, mpi_field_history = run("mpi_extract_field", mpi_options)
        assert len(mpi_field_history) == 1, len(mpi_field_history)
        mpi_field_rows = read_csv(mpi_field / "sst_epm_sensitivities.csv")
        mpi_errors["field"] = compare_rows("mpi_extract_field", mpi_field_rows, reference, "field")
        mpi_canonical = {r["case"]: r for r in mpi_field_rows if r["probe_pair"] == "3"}
        assert set(mpi_canonical) == set(canonical), sorted(mpi_canonical)
        mpi_merged, mpi_info = check_field_files(mpi_field, mpi_canonical, edges, source_minimum)
        mpi_errors["field_scaled"] = compare_fields(serial_field, mpi_merged, used_only=True)
        mpi_scalar, mpi_scalar_history = run("mpi_extract_scalar", dict(mpi_options,
                                                                        SST_EPM_EXTRACTION="SCALAR_REPLAY"))
        assert len(mpi_scalar_history) == 1, len(mpi_scalar_history)
        mpi_errors["scalar"] = compare_rows("mpi_extract_scalar", read_csv(mpi_scalar / "sst_epm_sensitivities.csv"),
                                            serial_scalar, "scalar_replay")
        assert_no_field_files(mpi_scalar)
        for directory in (mpi_field, mpi_scalar):
            log = (directory / "run.log").read_text()
            assert "EPM extract-only: residual of one adjoint iteration" in log and "): passed" in log, directory
        # The serial inputs were only read.
        assert hashlib.sha256(serial_adjoint.read_bytes()).hexdigest() == serial_adjoint_hash
        assert hashlib.sha256(primal_restart.read_bytes()).hexdigest() == original_hash
    summary = {"binary": str(binary), "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
               "dimension": args.dimension, "ranks": args.ranks,
               "primal_restart_sha256": original_hash,
               "timings_seconds": timings, "primal_final_residuals": {
                   k: v for k, v in primal_history[-1].items() if k.startswith("rms[")},
               "canonical_gradients": {k: float(v["dJ_da_at_zero"]) for k, v in canonical.items()},
               "stress_evaluations": int(rows[0]["stress_evaluations"]),
               "source_evaluations": int(rows[0]["source_evaluations"]),
               "J_baseline": float(rows[0]["J_baseline"]),
               "max_field_scalar_gradient_abs_error": max_field_scalar_error,
               "max_restart_gradient_abs_error": max(abs(float(a["dJ_da_at_zero"])-float(b["dJ_da_at_zero"]))
                                                     for a, b in zip(rows, resumed_rows)),
               "adjoint_final_residuals": {k: v for k, v in on_history[-1].items() if k.startswith("rms[")},
               "adjoint_restart_sha256": adjoint_hash,
               "extract_only_residuals": {k: v for k, v in extracted_history[-1].items() if k.startswith("rms[")},
               "max_extract_only_gradient_abs_error": max(abs(float(a["dJ_da_at_zero"])-float(b["dJ_da_at_zero"]))
                                                          for a, b in zip(rows, extracted_rows)),
               "extract_only_restart_identical_to_input": written_adjoint.read_bytes() == adjoint_restart.read_bytes(),
               "field_file_bytes": {"enabled": enabled_info["bytes"], "extract_only": extracted_info["bytes"]},
               "field_sites": {"enabled": enabled_info["sites"], "extract_only": extracted_info["sites"]},
               "max_extract_only_field_scaled_error": max_extract_only_field_error,
               "rows": len(rows), "status": "passed_infrastructure_smoke_test"}
    if mpi_errors:
        summary.update(mpi_max_rel_error_field=mpi_errors["field"], mpi_max_rel_error_scalar=mpi_errors["scalar"],
                       mpi_max_field_scaled_error=mpi_errors["field_scaled"])
        summary["field_file_bytes"]["mpi_extract_field"] = mpi_info["bytes"]
        summary["field_sites"]["mpi_extract_field"] = mpi_info["sites"]
    (root / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
