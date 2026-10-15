#!/usr/bin/env python3
"""Run the correctness regressions used by the Newton-Krylov baseline audit."""

import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import sys
import subprocess
import time

TESTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TESTS))
from TestCase import TestCase

# Use the registered reference values, rather than maintaining a second copy.
SELECTION = {
    "serial_regression.py": {"channel_identity"},
    "parallel_regression.py": {
        "flatplate_wall_limiter", "axi_rans_air_nozzle_density_velocity", "channel_identity"
    },
    "hybrid_regression.py": {"lam_naca0012_ilu_levels"},
}


BOX = ("SOLVER= EULER\nMESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= 5,5,5\n"
       "MARKER_EULER= (x_minus,x_plus,y_minus,y_plus)\nMUSCL_FLOW= YES\nCONV_NUM_METHOD_FLOW= ROE\n")
SST_BOX = BOX.replace("SOLVER= EULER", "SOLVER= RANS\nKIND_TURB_MODEL= SST\nREYNOLDS_NUMBER= 1e6")
GUARDS = {
    "fgcrodr_deflation": (BOX + "LINEAR_SOLVER= FGCRODR\nLINEAR_SOLVER_RESTART_FREQUENCY= 10\n"
                         "LINEAR_SOLVER_RESTART_DEFLATION= 9\n", "FGCRODR requires"),
    "zero_limiter_width": (BOX + "SLOPE_LIMITER_FLOW= WALL_DISTANCE\nVENKAT_LIMITER_COEFF= 0\n",
                           "slope limiters need VENKAT_LIMITER_COEFF"),
    "wall_limiter_without_walls": (BOX + "SLOPE_LIMITER_FLOW= WALL_DISTANCE\n",
                                   "WALL_DISTANCE slope limiter needs viscous walls"),
    "conflicting_sources": (BOX + "BODY_FORCE= YES\nGRAVITY_FORCE= YES\n", "Only one of BODY_FORCE"),
    "sst_engine": (SST_BOX + "MARKER_ENGINE_INFLOW= (x_minus, 0.5)\n", "not supported with the SST model"),
    "sst_freestream": (SST_BOX + "FREESTREAM_TURBULENCEINTENSITY= 0\n",
                       "free-stream k and omega must be positive and finite"),
}


def selected_cases(script, names):
    namespace = {"TestCase": TestCase, "test_list": []}
    tree = ast.parse((TESTS / script).read_text())
    main = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "main")
    for node in main.body:
        if names & {item.id for item in ast.walk(node) if isinstance(item, ast.Name)}:
            exec(compile(ast.Module(body=[node], type_ignores=[]), script, "exec"), namespace)
    cases = namespace["test_list"]
    assert {case.tag for case in cases} == names, "The selected regression definitions changed"
    return cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--data", type=Path, required=True, help="SU2 TestCases mesh repository")
    parser.add_argument("--output", type=Path, required=True, help="New evidence directory")
    args = parser.parse_args()
    binary = args.binary.resolve(strict=True)
    data = args.data.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    os.environ["OPENBLAS_NUM_THREADS"] = "1"
    results = []
    for script, names in SELECTION.items():
        for original in selected_cases(script, names):
            layouts = [(2, 2)] if script == "hybrid_regression.py" else [(2, 1)]
            if script == "serial_regression.py":
                layouts = [(1, 1)]
            if original.tag == "lam_naca0012_ilu_levels":
                layouts.insert(0, (2, 1))
            for ranks, threads in layouts:
                # Reload because run_test updates the configuration filename.
                case = next(case for case in selected_cases(script, names) if case.tag == original.tag)
                destination = output / (case.tag + "_%dr_%dt" % (ranks, threads))
                destination.mkdir()
                fixture = data / case.cfg_dir
                config = TESTS / case.cfg_dir / case.cfg_file
                for item in fixture.iterdir():
                    if item.is_file():
                        shutil.copy2(item, destination / item.name)
                shutil.copy2(config, destination / case.cfg_file)
                case.cfg_dir = str(destination)
                case.command = TestCase.Command("mpirun --bind-to none -n %d" % ranks,
                                                shlex.quote(str(binary)), "-t %d" % threads)
                case.timeout = 600
                if case.tol == 0:
                    case.tol = 1e-5
                os.environ["OMP_NUM_THREADS"] = str(threads)
                start = time.monotonic()
                passed = case.run_test()
                results.append({"case": case.tag, "ranks": ranks, "threads": threads,
                                "passed": passed, "wall_seconds": time.monotonic() - start,
                                "config_sha256": digest(config), "suite_sha256": digest(TESTS / script),
                                "run_config_sha256": digest(destination / case.cfg_file),
                                "binary_sha256": digest(binary), "reference": case.test_vals,
                                "tolerance": case.tol, "command": case.command.assemble(),
                                "inputs": {p.name: digest(p) for p in destination.iterdir()
                                           if p.suffix in (".su2", ".cfg")}})
                (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    for name, (options, message) in GUARDS.items():
        destination = output / name
        destination.mkdir()
        config = destination / "run.cfg"
        config.write_text(options)
        with (destination / "log.txt").open("w") as log:
            process = subprocess.run([str(binary), "run.cfg"], cwd=destination, stdout=log,
                                     stderr=subprocess.STDOUT, timeout=30)
        passed = process.returncode != 0 and message in (destination / "log.txt").read_text()
        results.append({"case": name, "kind": "configuration_guard", "passed": passed,
                        "exit_code": process.returncode, "expected_message": message,
                        "config_sha256": digest(config), "binary_sha256": digest(binary)})
        print(name, "PASSED" if passed else "FAILED", flush=True)
        (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return 0 if all(result["passed"] for result in results) else 1


if __name__ == "__main__":
    sys.exit(main())
