#!/usr/bin/env python3
"""Tiny SST_EPM_DIRECT regression on the converged primal of a check_extraction.py run.

The EPM direct mode runs plain SST with the residual F0 + A (Fm - F0), S0 + A (clip(Pm) - clip(P0)), the path the
extraction linearizes. This script checks, from the serial reference directory of check_extraction.py:
  1. A = 0 reproduces the plain SST restart run bit for bit (history and restart file);
  2. the option guards reject unsupported configurations;
  3. for every case, the central difference (J(h) - J(-h)) / 2h of converged direct runs equals the extracted
     dJ_da_at_zero of the reference CSV (SWAP included);
  4. optionally (--endpoints), A = 1 against the classical endpoint (SST_OPTIONS UQ, UQ_URLX = 1).
If the reference extraction was faded (fade_gap column of its CSV, check_extraction.py --fade-gap), the direct runs
use the same UQ_FADE_GAP: residual F0 + A f (Fm - F0), f the fade factor of each site. Stages 1-3 hold unchanged. The
A = 1 endpoint is then not the classical one (which keeps (1-f) 2/3 rho k in the stress and clips after the blend), so
stage 4 is recorded as a diagnostic only.
"""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_extraction import (fade_gap_of_rows, mpi_command, read_config,  # noqa: E402  (sits next to this script)
                              read_csv)

MODES = {"1C_KEEP": (1, "NO"), "1C_SWAP": (1, "YES"),
         "2C_KEEP": (2, "NO"), "2C_SWAP": (2, "YES"), "3C": (3, "NO")}
# Central-difference step and acceptance. On the smoke meshes dJ/da is 1e-6..3e-5 and CD 6e-3..8e-3 (cluster job
# 664831), so J(h) - J(-h) at h = 0.01 is 2e-8..6e-7, three or more decades above the objective noise of a
# 1e-10 residual. Truncation is h^2 J'''/6; 2e-3 leaves room for J'''/J' up to ~100 and still fails the factor
# ~1.8 of the clip-after-blend (classical solver) path by far.
DEFAULT_STEP = 0.01
TAYLOR_REL_TOL = 2e-3
TAYLOR_ABS_TOL = 1e-11
# A = 1 direct vs classical UQ_URLX = 1: the same residual up to round-off, so the same converged objective.
ENDPOINT_REL_TOL = 1e-8
IDENTITY_ITER = 5


def canonical_gradients(rows):
    """dJ_da_at_zero of the five canonical cases (probe_pair 3) of an sst_epm_sensitivities.csv."""
    gradients = {}
    for row in rows:
        if row["probe_pair"] != "3":
            continue
        if row["case"] in gradients:
            raise ValueError(f"Duplicate canonical row for {row['case']}")
        gradients[row["case"]] = float(row["dJ_da_at_zero"])
    if set(gradients) != set(MODES):
        raise ValueError(f"Canonical rows {sorted(gradients)} differ from {sorted(MODES)}")
    return gradients


def _timing_column(name):
    lowered = name.lower()
    return "wall" in lowered or ("time" in lowered and lowered != "time_iter")


def history_mismatches(a, b):
    """(row, column, a, b) of every differing non-timing value; one entry for a length or column-set mismatch."""
    if not a or len(a) != len(b):
        return [("rows", None, len(a), len(b))]
    columns = sorted(key for key in a[0] if not _timing_column(key))
    if columns != sorted(key for key in b[0] if not _timing_column(key)):
        return [("columns", None, columns, sorted(b[0]))]
    return [(i, key, ra[key], rb[key]) for i, (ra, rb) in enumerate(zip(a, b)) for key in columns if ra[key] != rb[key]]


def taylor(j_plus, j_minus, j0, step, gradient, rel_tol=TAYLOR_REL_TOL, abs_tol=TAYLOR_ABS_TOL):
    """Central difference of J at A = +-step against the extracted gradient, with the one-sided secants."""
    central = (j_plus - j_minus) / (2 * step)
    error = abs(central - gradient)
    values = (j_plus, j_minus, j0, central)
    passed = all(math.isfinite(v) for v in values) and error <= rel_tol * abs(gradient) + abs_tol
    return {"J_plus": j_plus, "J_minus": j_minus, "J0": j0, "step": step, "dJ_da_at_zero": gradient,
            "central": central, "forward": (j_plus - j0) / step, "backward": (j0 - j_minus) / step,
            "rel_error": error / abs(gradient) if gradient else math.inf, "passed": bool(passed)}


def endpoint_comparison(j_direct, j_classical, direct_converged, classical_converged, rel_tol=ENDPOINT_REL_TOL):
    """A = 1 direct objective against the classical endpoint; compared only if both runs converged."""
    result = {"J_direct": j_direct, "J_classical": j_classical,
              "direct_converged": direct_converged, "classical_converged": classical_converged, "passed": None}
    if direct_converged and classical_converged:
        difference = abs(j_direct - j_classical)
        result["rel_difference"] = difference / max(abs(j_classical), 1e-300)
        result["passed"] = difference <= rel_tol * max(abs(j_direct), abs(j_classical))
    return result


def direct_overrides(mode, amplitude, fade_gap=0.0):
    component, permute = MODES[mode]
    options = {"SST_OPTIONS": "V2003m", "UQ_COMPONENT": str(component), "UQ_PERMUTE": permute,
               "UQ_DELTA_B": "1", "UQ_URLX": "1", "SST_EPM_DIRECT": "YES", "SST_EPM_DIRECT_AMPLITUDE": repr(amplitude)}
    if fade_gap:
        options["UQ_FADE_GAP"] = repr(float(fade_gap))
    return options


def classical_overrides(mode, fade_gap=0.0):
    component, permute = MODES[mode]
    options = {"SST_OPTIONS": "(V2003m, UQ)", "UQ_COMPONENT": str(component), "UQ_PERMUTE": permute,
               "UQ_DELTA_B": "1", "UQ_URLX": "1"}
    if fade_gap:
        options["UQ_FADE_GAP"] = repr(float(fade_gap))
    return options


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("binary", type=Path, help="SU2_CFD or SU2_CFD_AD (runs MATH_PROBLEM= DIRECT), without OpenMP")
    parser.add_argument("workdir", type=Path, help="new directory for all outputs")
    parser.add_argument("--reference", type=Path, required=True,
                        help="serial check_extraction.py directory (baseline/ restart, enabled/ sensitivities)")
    parser.add_argument("--ranks", type=int, choices=(1, 2), default=1)
    parser.add_argument("--step", type=float, default=DEFAULT_STEP, help="central-difference amplitude step h")
    parser.add_argument("--rel-tol", type=float, default=TAYLOR_REL_TOL)
    parser.add_argument("--endpoints", action="store_true",
                        help="also run A = 1 against the classical UQ_URLX = 1 endpoint (diagnostic if unconverged)")
    args = parser.parse_args()
    if not 0 < args.step <= 1:
        parser.error("--step must be in (0, 1]")
    binary = args.binary.resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    root = args.workdir.resolve()
    baseline = reference / "baseline"
    config = read_config(baseline / "run.cfg")
    baseline_history = read_csv(baseline / "history.csv")
    assert all(float(v) <= -10 for k, v in baseline_history[-1].items() if k.startswith("rms[")), \
        "The reference primal is not converged"
    objective = {"DRAG": "CD", "LIFT": "CL"}[config["OBJECTIVE_FUNCTION"]]
    j0 = float(baseline_history[-1][objective])
    reference_rows = read_csv(reference / "enabled" / "sst_epm_sensitivities.csv")
    gradients = canonical_gradients(reference_rows)
    fade_gap = fade_gap_of_rows(reference_rows)
    root.mkdir(parents=True, exist_ok=False)
    command = [str(binary), "run.cfg"]
    if args.ranks > 1:
        command = mpi_command(args.ranks) + command
    restart = {"MATH_PROBLEM": "DIRECT", "RESTART_SOL": "YES", "SOLUTION_FILENAME": str(baseline / "restart_flow"),
               "OUTPUT_FILES": "(RESTART)"}
    timings = {}

    def launch(name, overrides):
        directory = root / name
        directory.mkdir(parents=True)
        options = dict(config, **restart)
        options.update(overrides)
        (directory / "run.cfg").write_text("".join(f"{k}= {v}\n" for k, v in options.items() if v is not None))
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

    def run(name, overrides, require_converged=True):
        directory, returncode = launch(name, overrides)
        log = (directory / "run.log").read_text()
        if returncode:
            raise RuntimeError(f"{name} failed: {log[-6000:]}")
        history = read_csv(directory / "history.csv")
        converged = "All convergence criteria satisfied." in log and \
            all(float(v) <= -10 for k, v in history[-1].items() if k.startswith("rms["))
        if require_converged and not converged:
            raise RuntimeError(f"{name} did not converge: {history[-1]}")
        print(f"{name}: {len(history)} history rows, converged={converged}, {timings[name]:.2f}s", flush=True)
        return directory, history, converged

    def run_expect_failure(name, overrides, substrings):
        directory, returncode = launch(name, overrides)
        output = (directory / "run.log").read_text()
        if returncode == 0 or not any(s in output for s in substrings):
            raise RuntimeError(f"{name}: expected a failure with one of {substrings!r}, got exit code {returncode}: "
                               f"{output[-6000:]}")
        print(f"{name}: rejected as expected, {timings[name]:.2f}s", flush=True)

    # 1. A = 0 is the plain SST residual: the same iterations from the same restart, bit for bit.
    identity = {"ITER": str(IDENTITY_ITER), "CONV_STARTITER": str(10 * IDENTITY_ITER)}
    plain, plain_history, _ = run("identity/plain", identity, require_converged=False)
    zero, zero_history, _ = run("identity/direct_zero", dict(identity, **direct_overrides("1C_SWAP", 0.0, fade_gap)),
                                require_converged=False)
    assert plain_history, "identity/plain wrote no history"
    mismatches = history_mismatches(plain_history, zero_history)
    assert not mismatches, mismatches[:10]
    plain_hash = hashlib.sha256((plain / "restart_flow.dat").read_bytes()).hexdigest()
    zero_hash = hashlib.sha256((zero / "restart_flow.dat").read_bytes()).hexdigest()
    assert plain_hash == zero_hash, (plain_hash, zero_hash)
    assert "EPM direct mode: case 1C_SWAP at amplitude 0" in (zero / "run.log").read_text()

    # 2. Guards (SU2_MPI::Error at configuration time).
    direct = direct_overrides("1C_KEEP", args.step, fade_gap)
    run_expect_failure("reject/classical_uq", dict(direct, SST_OPTIONS="(V2003m, UQ)"),
                       ["SST_EPM_DIRECT requires MATH_PROBLEM= DIRECT"])
    run_expect_failure("reject/adjoint", dict(direct, MATH_PROBLEM="DISCRETE_ADJOINT"),
                       ["SST_EPM_DIRECT requires MATH_PROBLEM= DIRECT", "requires AD support"])
    run_expect_failure("reject/extraction", dict(direct, SST_EPM_POSTPROCESS="YES"), ["are exclusive"])
    run_expect_failure("reject/amplitude_range", dict(direct, SST_EPM_DIRECT_AMPLITUDE="1.5"),
                       ["SST_EPM_DIRECT_AMPLITUDE must be"])
    run_expect_failure("reject/amplitude_missing", dict(direct, SST_EPM_DIRECT_AMPLITUDE=None),
                       ["requires SST_EPM_DIRECT_AMPLITUDE"])
    run_expect_failure("reject/urlx", dict(direct, UQ_URLX="0.5"), ["UQ_URLX= 1 or unset"])
    run_expect_failure("reject/3c_swap", dict(direct_overrides("3C", args.step, fade_gap), UQ_PERMUTE="YES"),
                       ["UQ_COMPONENT"])

    # 3. Central differences against the extraction, every case.
    results = {}
    for mode in MODES:
        values = {}
        for sign, label in ((1, "plus"), (-1, "minus")):
            directory, history, _ = run(f"taylor/{mode}/{label}", direct_overrides(mode, sign * args.step, fade_gap))
            values[label] = float(history[-1][objective])
        results[mode] = taylor(values["plus"], values["minus"], j0, args.step, gradients[mode], rel_tol=args.rel_tol)
        print(f"{mode}: central {results[mode]['central']:.10e} vs dJ_da_at_zero {gradients[mode]:.10e}, "
              f"rel error {results[mode]['rel_error']:.2e}", flush=True)

    # 4. Optional A = 1 against the classical endpoint.
    endpoints = {}
    if args.endpoints:
        for mode in MODES:
            _, direct_history, direct_converged = run(f"endpoint/{mode}/direct", direct_overrides(mode, 1.0, fade_gap),
                                                      require_converged=False)
            _, classical_history, classical_converged = run(f"endpoint/{mode}/classical",
                                                            classical_overrides(mode, fade_gap),
                                                            require_converged=False)
            endpoints[mode] = endpoint_comparison(float(direct_history[-1][objective]),
                                                  float(classical_history[-1][objective]),
                                                  direct_converged, classical_converged)
            if fade_gap:
                # Faded: the two A = 1 residuals differ by design (module docstring); diagnostic only.
                endpoints[mode]["passed"] = None
                endpoints[mode]["note"] = "faded: not the same residual, diagnostic only"

    failed = [mode for mode, result in results.items() if not result["passed"]]
    failed += [f"endpoint/{mode}" for mode, result in endpoints.items() if result["passed"] is False]
    summary = {"binary": str(binary), "reference": str(reference), "ranks": args.ranks, "objective": objective,
               "J0": j0, "fade_gap": fade_gap, "identity": {"iterations": IDENTITY_ITER, "restart_sha256": plain_hash},
               "taylor": results, "taylor_rel_tol": args.rel_tol, "endpoints": endpoints, "timings_s": timings,
               "status": "failed: " + ", ".join(failed) if failed else "passed"}
    (root / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
