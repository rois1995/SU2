#!/usr/bin/env python3
"""Run one descriptor-pinned hidden D9 unit measurement after primary readiness."""

import argparse
import json
import math
import os
import sys
from pathlib import Path

import run_measurements as core

N5 = core.N5
STAGED = core.STAGED


def strict_json(path):
    def reject_constant(value):
        raise ValueError(f"nonstandard JSON number: {value}")
    data = json.loads(path.read_text(), parse_constant=reject_constant)
    def check(value):
        if isinstance(value, float):
            assert math.isfinite(value), f"non-finite JSON number in {path}"
        elif isinstance(value, dict):
            for item in value.values():
                check(item)
        elif isinstance(value, list):
            for item in value:
                check(item)
    check(data)
    return data


def validate_descriptor(path):
    descriptor = strict_json(path)
    assert descriptor.get("schema") == "n5_d9_measurement_v1"
    action, env, hashes = descriptor.get("action"), descriptor.get("environment"), descriptor.get("input_file_sha256")
    assert action in ("serial_stack", "geometry", "counter_check")
    assert isinstance(env, dict) and isinstance(hashes, dict) and descriptor.get("ranks") == 1
    tags = {"serial_stack": "[.][NativeD9SerialStack]", "geometry": "[.][NativeD9Geometry]",
            "counter_check": "[.][NativeD9CounterCheck]"}
    assert descriptor.get("test_filter") == tags[action]
    for raw, expected in hashes.items():
        path_input = Path(raw).resolve(strict=True)
        assert core.sha(path_input) == expected, f"descriptor input hash mismatch: {path_input}"
    if action == "serial_stack":
        assert set(env) == {"SU2_D9_STACK_MODE", "SU2_D9_STACK_JSON"}
        assert env["SU2_D9_STACK_MODE"] in {"M12_control", "M12_meet", "M10_n1", "M10_n2", "M10_n4"}
        assert env["SU2_D9_STACK_JSON"] == "d9_stack_measurement.json"
    elif action == "geometry":
        assert set(env) <= {"SU2_D9_REFERENCE", "SU2_D9_PARAMETERS", "SU2_D9_WALL_MARKERS"}
        ref = str(Path(env["SU2_D9_REFERENCE"]).resolve(strict=True))
        inputs = {str(Path(p).resolve(strict=True)) for p in hashes}
        assert ref in inputs
        if env.get("SU2_D9_PARAMETERS"):
            assert str(Path(env["SU2_D9_PARAMETERS"]).resolve(strict=True)) in inputs
    else:
        assert not env and not hashes
    assert descriptor.get("case_name", "").startswith("n5_d9_")
    return descriptor


def pin_descriptor(runtime, descriptor_path, descriptor):
    old_save = runtime.save
    pinned = {str(descriptor_path): core.sha(descriptor_path)}
    pinned.update({str(Path(p).resolve()): digest for p, digest in descriptor["input_file_sha256"].items()})
    def save_with_descriptor(path, value):
        if Path(path).name == "pins.json":
            value.update(pinned)
        old_save(path, value)
    runtime.save = save_with_descriptor


def produced_artifacts(action, attempt):
    if action == "serial_stack":
        paths = [attempt / "d9_stack_measurement.json"]
    elif action == "geometry":
        paths = [attempt / "d9_geometry_summary.json"]
    else:
        paths = []
    for path in paths:
        assert path.is_file() and path.stat().st_size > 0, f"required measurement artifact missing: {path}"
    log = attempt / "run.log"
    assert log.is_file()
    names = {"serial_stack": "D9 serial stack measurement on synthetic boxes",
             "geometry": "D9 geometry need and exact wall proof measurements",
             "counter_check": "D9 observations preserve timer calls and separate candidate qJ work"}
    assert names[action] in log.read_text(errors="replace"), "requested hidden test is absent from run.log"
    artifacts = {path.name: core.sha(path) for path in [*paths, log]}
    if action in ("serial_stack", "geometry"):
        data = strict_json(paths[0])
        schema = "D9_stack_v1" if action == "serial_stack" else "d9_geometry_measurement_v1"
        assert data.get("schema") == schema
        if action == "serial_stack":
            assert data.get("mode")
            assert all(k in data for k in ("timing_seconds", "phases", "room", "positive_validity_and_topology"))
        else:
            assert data.get("status") in {"observed", "unsupported_reference_has_no_geometry_authority",
                                          "unsupported_missing_parameter_state"}
            if data["status"] == "observed":
                for name in ("d9_geometry_edges.csv", "d9_geometry_faces.csv", "d9_geometry_projections.csv"):
                    path = paths[0].parent / name
                    assert path.is_file() and path.stat().st_size > 0, f"geometry output missing: {path}"
                    artifacts[name] = core.sha(path)
    return artifacts


def run(descriptor_path):
    core.verify_run_ready()
    descriptor_path = descriptor_path.resolve(strict=True)
    descriptor = validate_descriptor(descriptor_path)
    receipt = N5 / "unit_measurement_receipts" / (descriptor["case_name"] + ".json")
    assert not receipt.exists(), f"refusing to repeat existing measurement action: {receipt}"
    sys.path.insert(0, str(STAGED))
    import run_cases as runtime
    status, _, _, _, test_binary = core.verify_build()
    runtime.ROOT = N5
    runtime.BINARY = test_binary
    runtime.CASES[descriptor["case_name"]] = (1, descriptor["test_filter"])
    pin_descriptor(runtime, descriptor_path, descriptor)
    for key in tuple(os.environ):
        if key.startswith("SU2_D9_"):
            os.environ.pop(key)
    os.environ.update(descriptor["environment"])
    sys.argv = [str(STAGED / "run_cases.py"), "build512", descriptor["case_name"]]
    try:
        runtime.main(STAGED / "runtime_20261113_474/loaded_runtime_pins.json")
    except SystemExit as result:
        if result.code not in (None, 0):
            raise
    attempts = sorted((N5 / "cases").glob(descriptor["case_name"] + "_*"), key=lambda p: p.stat().st_mtime)
    assert attempts
    attempt = attempts[-1]
    result = strict_json(attempt / "status.json")
    assert result.get("state") == "terminal" and result.get("exit_code") == 0 and result.get("pins_verified") is True
    artifacts = produced_artifacts(descriptor["action"], attempt)
    receipt.parent.mkdir(parents=True, exist_ok=True)
    assert not receipt.exists(), f"refusing to overwrite {receipt}"
    core.dump(receipt, {"schema": "n5_d9_unit_action_v1", "case": descriptor["case_name"],
                        "descriptor": str(descriptor_path), "descriptor_sha256": core.sha(descriptor_path),
                        "attempt": str(attempt), "attempt_status_sha256": core.sha(attempt / "status.json"),
                        "artifacts_sha256": artifacts, "source_pins_sha256": core.sha(N5 / "build512_source_pins.json"),
                        "test_binary_sha256": status["test_binary_sha256"], "passed": True})
    return strict_json(receipt)


def self_check():
    assert "NativeD9CounterCheck" in (core.REPO / "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp").read_text()
    assert "NativeD9SerialStack" in (core.REPO / "UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc12.inc").read_text()
    assert "NativeD9Geometry" in (core.REPO / "UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp").read_text()
    return {"passed": True, "launched": False, "counter_check_artifact": "run.log",
            "measurement_json_is_required_for": ["serial_stack", "geometry"], "unit_timeout_seconds": 600}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--descriptor", type=Path)
    args = parser.parse_args()
    if args.self_check:
        assert args.descriptor is None
        print(json.dumps(self_check(), indent=2))
        return
    if args.descriptor is None:
        parser.error("provide --descriptor FILE or --self-check")
    print(json.dumps(run(args.descriptor), indent=2))


if __name__ == "__main__":
    main()
