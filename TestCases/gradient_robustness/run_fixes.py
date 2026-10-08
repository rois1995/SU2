#!/usr/bin/env python3
"""Matched MGLEVEL=0 RANS comparisons; input configs/meshes are explicit and output is fresh."""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import time


def configure(text, options):
    for key, value in options.items():
        text = re.sub(r"^\s*" + re.escape(key) + r"\s*=.*$", "", text, flags=re.MULTILINE)
        text += "\n{}= {}\n".format(key, value)
    return text


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("native_config", type=Path)
    parser.add_argument("bl_mesh", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--adapted-config", type=Path, required=True,
                        help="Snapshot config with absolute paths to an adapted RAE mesh and its restart")
    parser.add_argument("--main-baseline", type=Path, required=True,
                        help="Original native main binary, additionally compared on the adapted mesh")
    parser.add_argument("--iterations", type=int, default=2000)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--adapted-only", action="store_true",
                        help="Test a user-identified adapted RANS grid: existing turbulence settings and MUSCL")
    args = parser.parse_args()
    if args.iterations <= 0 or args.timeout <= 0:
        parser.error("Iteration and timeout limits must be positive")
    binaries = {name: path.resolve(strict=True) for name, path in
                [("current", args.baseline), ("fixed", args.candidate), ("main", args.main_baseline)]}
    if not all(path.is_file() and os.access(path, os.X_OK) for path in binaries.values()):
        parser.error("Both binaries must be executable files")
    bl_mesh = args.bl_mesh.resolve(strict=True)
    native_config = args.native_config.resolve(strict=True)
    native = native_config.read_text()
    adapted_config = args.adapted_config.resolve(strict=True)
    adapted = adapted_config.read_text()
    root = Path(__file__).resolve().parents[2]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    # Exercise this helper independently before any solver work.
    assert configure("ITER= 1\nITER= 2\n% ITER= 3\n", {"ITER": "4"}).count("\nITER=") == 1
    common = {"MGLEVEL": "0", "ITER": str(args.iterations), "COMPUTE_METRIC": "NO", "ADAP_LOOP": "NO",
              "OUTPUT_FILES": "(RESTART_ASCII)", "OUTPUT_WRT_FREQ": "1000000", "SCREEN_WRT_FREQ_INNER": "100",
              "HISTORY_OUTPUT": "(ITER, RMS_RES, AERO_COEFF, CFL_NUMBER, LINSOL)", "TABULAR_FORMAT": "CSV",
              "CONV_FILENAME": "history", "RESTART_FILENAME": "fields", "CONV_RESIDUAL_MINVAL": "-8",
              "NUM_METHOD_GRAD": "WEIGHTED_LEAST_SQUARES", "NUM_METHOD_GRAD_RECON": "WEIGHTED_LEAST_SQUARES",
              "WRT_ADAP_MESH": "NO", "VOLUME_OUTPUT": "(COORDINATES, SOLUTION, PRIMITIVE)"}
    cases = [("native_sa_current_settings", native, {"MUSCL_TURB": "NO"}),
             ("native_sa_muscl", native, {"MUSCL_TURB": "YES", "SLOPE_LIMITER_TURB": "VENKATAKRISHNAN",
                                           "VENKAT_LIMITER_COEFF": "0.05", "LIMITER_ITER": "999999"})]
    cases += [("adapted_sa_current_settings", adapted, {"MUSCL_TURB": "NO"}),
              ("adapted_sa_muscl", adapted, {"MUSCL_TURB": "YES", "SLOPE_LIMITER_TURB": "VENKATAKRISHNAN",
                                            "VENKAT_LIMITER_COEFF": "0.05", "LIMITER_ITER": "999999"})]
    for model in ["SA", "SST"]:
        template = (root / "TestCases/rans/rae2822/turb_{}_RAE2822.cfg".format(model)).read_text()
        cases.append(("bl_{}_muscl".format(model.lower()), template,
                      {"MESH_FILENAME": str(bl_mesh), "RESTART_SOL": "NO", "CONV_NUM_METHOD_FLOW": "ROE",
                       "MUSCL_FLOW": "YES", "SLOPE_LIMITER_FLOW": "VENKATAKRISHNAN", "MUSCL_TURB": "YES",
                       "SLOPE_LIMITER_TURB": "VENKATAKRISHNAN", "VENKAT_LIMITER_COEFF": "0.05", "LIMITER_ITER": "999999",
                       "CFL_NUMBER": "3", "CFL_ADAPT": "YES", "CFL_ADAPT_PARAM": "(0.5, 1.2, 1, 30, 0.05, 10)",
                       "LINEAR_SOLVER_PREC": "ILU", "LINEAR_SOLVER_ITER": "25", "LINEAR_SOLVER_ERROR": "0.05",
                       "CONV_FIELD": "(RMS_DENSITY, {})".format("RMS_NU_TILDE" if model == "SA" else "RMS_TKE, RMS_DISSIPATION"),
                       "SCREEN_OUTPUT": "(INNER_ITER, RMS_DENSITY, LIFT, DRAG, AVG_CFL)"}))
    cases.append(("frozen_sa_limiter", adapted,
                  {"MUSCL_TURB": "YES", "SLOPE_LIMITER_TURB": "VENKATAKRISHNAN",
                   "VENKAT_LIMITER_COEFF": "0.05", "LIMITER_ITER": "50", "ITER": str(min(args.iterations, 200)),
                   "CONV_STARTITER": "300"}))
    if args.adapted_only:
        common["INNER_ITER"] = str(args.iterations)
        cases = [("bl_adapted_sa_current_settings", adapted, {"MUSCL_TURB": "NO"}),
                 ("bl_adapted_sa_muscl", adapted, {"MUSCL_TURB": "YES", "SLOPE_LIMITER_TURB": "VENKATAKRISHNAN",
                                                  "VENKAT_LIMITER_COEFF": "0.05", "LIMITER_ITER": "999999"})]
    manifest = {"binaries": {name: {"path": str(path), "sha256": digest(path)} for name, path in binaries.items()},
                "native_config": {"path": str(native_config), "sha256": digest(native_config)},
                "adapted_config": {"path": str(adapted_config), "sha256": digest(adapted_config)},
                "bl_mesh": {"path": str(bl_mesh), "sha256": digest(bl_mesh)},
                "threads": 1, "MGLEVEL": 0, "iterations": args.iterations, "timeout_seconds": args.timeout,
                "note": "Wall time is informational on a contended machine, not a performance benchmark."}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    results = []
    env = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    for name, template, options in cases:
        variants = [("current", "current", False), ("fixed", "fixed", False)]
        if name.startswith("adapted_") or args.adapted_only:
            variants.insert(0, ("main", "main", False))
        if "muscl" in name:
            variants.append(("fixed_local", "fixed", True))
        for label, binary_name, local in variants:
            case = output / (name + "_" + label)
            case.mkdir()
            settings = dict(common, **options)
            if binary_name == "fixed":
                settings["LIMITER_LOCAL_LENGTH"] = "YES" if local else "NO"
            # A fixed-mesh continuation must not retain remeshing prerequisites from its source config.
            template = re.sub(r"^\s*ADAP_[A-Z0-9_]+\s*=.*$", "", template, flags=re.MULTILINE)
            config = configure(template, settings)
            (case / "case.cfg").write_text(config)
            started = time.monotonic()
            with (case / "run.log").open("w") as log:
                try:
                    run = subprocess.run(["nice", "-n", "19", str(binaries[binary_name]), "case.cfg"], cwd=case,
                                         env=env, stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout)
                    status = "finished" if run.returncode == 0 else "failed"
                    exit_code = run.returncode
                except subprocess.TimeoutExpired:
                    status, exit_code = "timeout", None
            rows = []
            if (case / "history.csv").exists():
                with (case / "history.csv").open() as history:
                    rows = [{key.strip().strip('"'): value.strip() for key, value in row.items()}
                            for row in csv.DictReader(history)]
            numeric = [float(v) for row in rows for k, v in row.items() if k.startswith("rms[")]
            if status == "finished" and not numeric:
                status = "missing_history"
            if numeric and not all(math.isfinite(v) for v in numeric):
                status = "nonfinite"
            result = {"case": name, "variant": label, "status": status, "exit_code": exit_code,
                      "wall_seconds": time.monotonic() - started, "history_rows": len(rows),
                      "last_history": rows[-1] if rows else {}, "config_sha256": digest(case / "case.cfg")}
            results.append(result)
            (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
            print(name, label, status, result["history_rows"], result["last_history"], flush=True)


if __name__ == "__main__":
    main()
