#!/usr/bin/env python3
"""Check an executed exact-C492 repeat; artifact equality alone is not execution evidence."""
import hashlib
import json
import math
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REFERENCE_CASE = HERE.parent / "cases/p5_bl_20261114T192833_2451182"
FILES = tuple(f"stage_resumed_surface_rank0_{name}.bin" for name in ("owned", "parameters", "donors"))


def conditions(status, command, row, identical):
    return {
        "executed_successfully": status.get("state") == "terminal" and status.get("exit_code") == 0,
        "runtime_pins_verified": status.get("pins_verified") is True and not status.get("changed"),
        "serial_bounded_command": command.get("ranks") == 1 and command.get("timeout_seconds") == 600,
        "exact_input_and_target": all(row.get(key) is True for key in (
            "same_target_policy", "input_parameters_unchanged", "exact_transported_input", "captured")),
        "unchanged_stage_limits": math.isclose(row.get("maximum_seconds", -1), 388.8, rel_tol=0, abs_tol=1e-9)
            and row.get("maximum_work") == 233220 and math.isclose(row.get("stage_height", -1), 6.4e-5, rel_tol=0, abs_tol=1e-18),
        "at_least_300_splits": row.get("splits", -1) >= 300,
        "all_private_cells_positive_with_sensors": row.get("output_cells", 0) > 0
            and row.get("output_cells") == row.get("positive_cells") == row.get("sensor_cells"),
        "modeled_storage_within_12GiB": 0 < row.get("maximum_work_bytes", 0) <= 12 * (1 << 30),
        "same_bounded_output": all(identical.get(name) is True for name in FILES),
    }


def self_check():
    status = dict(state="terminal", exit_code=0, pins_verified=True, changed=[])
    command = dict(ranks=1, timeout_seconds=600)
    row = dict(same_target_policy=True, input_parameters_unchanged=True, exact_transported_input=True,
               captured=True, maximum_seconds=388.8, maximum_work=233220, stage_height=6.4e-5,
               splits=300, output_cells=10, positive_cells=10, sensor_cells=10, maximum_work_bytes=1 << 30)
    hashes = dict.fromkeys(FILES, True)
    assert all(conditions(status, command, row, hashes).values())
    for key, value in (("splits", 299), ("maximum_seconds", 400), ("maximum_work_bytes", 13 << 30),
                       ("positive_cells", 9), ("same_target_policy", False)):
        assert not all(conditions(status, command, dict(row, **{key: value}), hashes).values())
    assert not all(conditions(dict(status, exit_code=1), command, row, hashes).values())
    assert not all(conditions(dict(status, pins_verified=False), command, row, hashes).values())
    assert not all(conditions(status, command, row, dict.fromkeys(FILES, False)).values())


def main(case):
    case = Path(case).resolve()
    status = json.loads((case / "status.json").read_text())
    command = json.loads((case / "command.json").read_text())
    row = json.loads((case / "actual_saved_surface_stage_rank0.json").read_text())
    baseline = json.loads((HERE / "bl_output_comparison.json").read_text())
    actual = {}
    for name in FILES:
        with (case / name).open("rb") as stream:
            actual[name] = hashlib.file_digest(stream, "sha256").hexdigest()
    equality = {name: actual[name] == baseline[name]["reference_sha256"] for name in FILES}
    gates = conditions(status, command, row, equality)
    reference = json.loads((REFERENCE_CASE / "command.json").read_text())
    environment = lambda c: {k: v for k, v in c["environment"].items()
                             if k.startswith("SU2_NATIVE_") and k != "SU2_NATIVE_FROZEN_CONFIG"}
    gates["same_frozen_input_contract"] = environment(command) == environment(reference)
    gates["same_frozen_configuration"] = (case / "run.cfg").read_bytes() == (REFERENCE_CASE / "run.cfg").read_bytes()
    result = {"case": str(case), "passed": all(gates.values()), "gates": gates,
              "actual_splits": row["splits"], "actual_output_sha256": actual,
              "reference": str(HERE / "bl_output_comparison.json"),
              "scope": "Executed bounded-state preservation only; no accepted final mesh or qualified joint-curvature target claimed.",
              "changed_output_rule": "A different bounded output needs explicit decision/prefix evidence before preservation is established; higher throughput alone is not a regression verdict."}
    (case / "bl_regression_receipt.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result))
    return not result["passed"]


if __name__ == "__main__":
    self_check()
    raise SystemExit(0 if sys.argv[1:] == ["--self-check"] else main(sys.argv[1]))
