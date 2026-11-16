#!/usr/bin/env python3
"""Run focused A2/A3/N12 units and the bounded accepted-Euler M6b measurement."""
import argparse
import ast
import csv
import hashlib
import importlib.util
import json
import os
import sys
from collections import Counter
from pathlib import Path

A2 = Path(__file__).resolve().parent
STAGED = A2.parents[2]
NIGHT = A2.parent.parent
REPO = STAGED.parents[3]
N5 = NIGHT / "N5"
UNIT_PATH = N5 / "run_d9_units.py"
BUILD_ROOT = A2 / "build533"
READY = A2 / "primary_run_ready.json"
SOURCE_CHECK = A2 / "source_check.md"
GRAPH_RECEIPT = BUILD_ROOT / "generated_build_graph_after.json"
GRAPH_NAMES = ("build.ninja", "compile_commands.json")
TAG_BY_ACTION = {"counter_check": {"[NativeRequirements3D]", "[NativeLaneCensus3D]"},
                 "actual_operators": {"[.][NativeM6bProjection]"}}
EXPECTED_TAGS = {"n5_d9_a2_requirements_533": "[NativeRequirements3D]",
                 "n5_d9_n12_lane_census_533": "[NativeLaneCensus3D]",
                 "n5_d9_a3_m6b_projection_533": "[.][NativeM6bProjection]"}


def sha(path):
    """Hash one pinned helper, build artifact, or measurement input."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def no_owned_jobs_active():
    """Check this build and its cases while leaving unrelated host processes alone."""
    handles_path = BUILD_ROOT / "terminal_handles.json"
    handles = json.loads(handles_path.read_text())
    assert handles.get("all_owned_handles_terminal") is True
    assert not any(row.get("alive") for row in handles.get("identities", []))
    active = []
    for status_path in (A2 / "cases").glob("**/status.json"):
        status = json.loads(status_path.read_text())
        if status.get("state") == "running":
            active.append(str(status_path))
    assert not active, f"Section N8 owns active case statuses: {active}"
    return []


def load_unit():
    """Load the N5 runner and retarget it to the A2/A3/N12 build and evidence root."""
    os.environ.setdefault("OMPI_MCA_osc", "pt2pt")
    sys.path.insert(0, str(N5))
    spec = importlib.util.spec_from_file_location("n8_a2_a3_n12_n5_runner", UNIT_PATH)
    unit = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = unit
    spec.loader.exec_module(unit)
    unit.N5 = A2
    unit.STAGED = STAGED
    core = unit.core
    core.N5 = A2
    core.BUILD521 = BUILD_ROOT
    core.ALIAS_STATUS = A2 / "build533_status.json"
    core.ALIAS_PINS = A2 / "build533_source_pins.json"
    core.ALIAS_HANDLES = A2 / "build533_terminal_handles.json"
    core.RUN_READY = READY
    core.no_heavy_processes = no_owned_jobs_active
    patch_descriptor_validator(unit)
    patch_pin_descriptor(unit)
    patch_artifact_checker(unit)
    patch_unit_timeout(unit)
    return unit


def patch_descriptor_validator(unit):
    """Accept only the three descriptors and exact focused test tags for this group."""
    original = unit.validate_descriptor
    def validate(path):
        data = unit.strict_json(Path(path))
        expected = EXPECTED_TAGS.get(data.get("case_name"))
        assert expected and expected in TAG_BY_ACTION[data["action"]]
        assert data.get("test_filter") == expected
        unit.ACTION_TAGS[data["action"]] = expected
        return original(path)
    unit.validate_descriptor = validate


def patch_pin_descriptor(unit):
    """Add this wrapper, builder, source check, and graph receipt to case provenance."""
    original = unit.pin_descriptor
    def pin(runtime, descriptor_path, descriptor):
        original(runtime, descriptor_path, descriptor)
        old_save = runtime.save
        helpers = {str(Path(__file__).resolve()): sha(__file__),
                   str((BUILD_ROOT / "build.py").resolve()): sha(BUILD_ROOT / "build.py"),
                   str(SOURCE_CHECK.resolve()): sha(SOURCE_CHECK),
                   str(GRAPH_RECEIPT.resolve()): sha(GRAPH_RECEIPT),
                   str(UNIT_PATH.resolve()): sha(UNIT_PATH),
                   str((N5 / "run_measurements.py").resolve()): sha(N5 / "run_measurements.py"),
                   str((STAGED / "run_cases.py").resolve()): sha(STAGED / "run_cases.py"),
                   str((BUILD_ROOT / "build_status.json").resolve()): sha(BUILD_ROOT / "build_status.json"),
                   str((BUILD_ROOT / "source_pins.json").resolve()): sha(BUILD_ROOT / "source_pins.json"),
                   str((BUILD_ROOT / "terminal_handles.json").resolve()): sha(BUILD_ROOT / "terminal_handles.json")}
        def save_with_helpers(path, value):
            if Path(path).name == "pins.json":
                for name, digest in helpers.items():
                    assert name not in value or value[name] == digest
                    value[name] = digest
            old_save(path, value)
        runtime.save = save_with_helpers
    unit.pin_descriptor = pin


def row_counts(rows, selected_columns):
    """Count actual CSV values in selected columns without manufacturing zeroes."""
    result = {}
    for column in selected_columns:
        counts = Counter(row[column] for row in rows if row.get(column, "") != "")
        if counts:
            result[column] = dict(sorted(counts.items()))
    return result


def validate_m6b_rows(data, fields, rows):
    """Reconcile actual per-edge sign counts and refusal blanks with the summary."""
    sign_fields = [f"{marker}_{sign}" for marker in ("wing", "other")
                   for sign in ("positive", "zero", "negative")]
    face_fields = ["wing_shell_faces", "other_shell_faces"]
    assert set(sign_fields + face_fields + ["edge_a", "edge_b", "shell_measured",
               "projection_attempted", "projection_succeeded", "projection_reason"]) <= set(fields)
    assert data["wall_edges"] == 82559 and len(rows) == data["attempted"] <= 82559
    assert data["unvisited"] == 82559 - len(rows)
    assert data["all_edges_attempted"] == (len(rows) == 82559)
    edges, sums = set(), Counter()
    for row in rows:
        edge = (int(row["edge_a"]), int(row["edge_b"]))
        assert edge[0] < edge[1] and edge not in edges
        edges.add(edge)
        assert row["projection_attempted"] in ("true", "1")
        assert row["projection_succeeded"] in ("0", "1")
        success = row["projection_succeeded"] == "1"
        assert row["shell_measured"] == str(int(success))
        sums["projection_succeeded" if success else "projection_refusals"] += 1
        if not success:
            assert all(row[field] == "" for field in sign_fields + face_fields)
            continue
        for marker in ("wing", "other"):
            count = int(row[f"{marker}_shell_faces"])
            values = {sign: int(row[f"{marker}_{sign}"]) for sign in ("positive", "zero", "negative")}
            assert count >= 0 and all(value >= 0 for value in values.values())
            assert sum(values.values()) == count
            sums[f"{marker}_faces"] += count
            for sign, value in values.items():
                sums[f"{marker}_{sign}"] += value
    for field in ["projection_succeeded", "projection_refusals", "wing_faces", "other_faces", *sign_fields]:
        assert sums[field] == data[field], f"M6b summary/CSV disagreement for {field}"
    assert sums["projection_succeeded"] + sums["projection_refusals"] == len(rows)
    assert data["shell_faces"] == sums["wing_faces"] + sums["other_faces"]
    assert data["all_attempted_shells_measured"] == (not sums["projection_refusals"] and not data["unvisited"])
    return dict(sums)


def inspect_m6b_artifacts(unit, attempt, original, ranks, resource_path):
    """Validate the M6b outputs while retaining partial rows and explicit refusals."""
    artifacts = original("counter_check", attempt, ranks, resource_path)
    log_path = attempt / "run.log"
    log = log_path.read_text(errors="replace")
    assert "NativeM6bProjection" in log
    json_path = attempt / "native_m6b_projection.json"
    csv_path = attempt / "native_m6b_projection.csv"
    assert json_path.is_file() and json_path.stat().st_size > 0
    assert csv_path.is_file() and csv_path.stat().st_size > 0
    data = unit.strict_json(json_path)
    assert isinstance(data, dict), "M6b summary JSON must be an object"
    with csv_path.open(newline="", errors="replace") as stream:
        reader = csv.DictReader(stream)
        fields = list(reader.fieldnames or [])
        rows = list(reader)
    assert fields, "M6b CSV header is missing"
    totals = validate_m6b_rows(data, fields, rows)
    sign_fields = [field for field in fields if any(
        token in field for token in ("positive", "zero", "negative"))]
    status_fields = [field for field in fields if any(
        token in field.lower() for token in ("status", "refusal", "result", "reason", "succeeded", "measured"))]
    receipt = {"schema": "n8_a3_m6b_artifact_capture_v1", "expected_edges": 82559,
               "csv_data_rows": len(rows), "population_complete_if_one_row_per_edge": len(rows) == 82559,
               "csv_header": fields, "csv_reconciled_totals": totals,
               "raw_sign_value_counts": row_counts(rows, sign_fields),
               "status_and_refusal_value_counts": row_counts(rows, status_fields),
               "summary_json": data, "json_sha256": sha(json_path), "csv_sha256": sha(csv_path),
               "partial_counts_remain_observed": True}
    capture_path = attempt / "native_m6b_artifact_capture.json"
    capture_path.write_text(json.dumps(receipt, indent=2) + "\n")
    artifacts.update({path.name: sha(path) for path in (json_path, csv_path, capture_path)})
    return artifacts


def patch_artifact_checker(unit):
    """Reuse N5's generic test/resource verification and add the requested artifact checks."""
    original = unit.produced_artifacts
    def check(action, attempt, ranks, resource_path):
        if action == "actual_operators":
            return inspect_m6b_artifacts(unit, attempt, original, ranks, resource_path)
        artifacts = original(action, attempt, ranks, resource_path)
        expected = "NativeRequirements3D" if "requirements" in attempt.name else "NativeLaneCensus3D"
        assert expected in (attempt / "run.log").read_text(errors="replace")
        return artifacts
    unit.produced_artifacts = check


def patch_unit_timeout(unit):
    """Give only the M6b measurement its fixture-local maximum 90-minute timeout."""
    original_run = unit.run
    def run_descriptor(path):
        descriptor = unit.validate_descriptor(path)
        if descriptor["case_name"] == "n5_d9_a3_m6b_projection_533":
            sys.path.insert(0, str(STAGED))
            import run_cases as runtime
            source = Path(runtime.__file__).read_text()
            body = m6b_timeout_main(source, descriptor["case_name"])
            exec(compile(body, runtime.__file__, "exec"), runtime.__dict__)
        return original_run(path)
    unit.run = run_descriptor


def m6b_timeout_main(source, case_name):
    """Return run_cases.main with only the M6b action capped at 5400 seconds."""
    tree = ast.parse(source)
    main = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "main")
    body = ast.get_source_segment(source, main)
    old = 'timeout_seconds = 2400 if case == "m6_stage" else 600'
    assert body.count(old) == 1
    new = (f'timeout_seconds = 5400 if case == {case_name!r} else '
           '2400 if case == "m6_stage" else 600')
    adapted = body.replace(old, new, 1)
    compile(adapted, "run_cases.py", "exec")
    return adapted


def verify_generated_graph():
    """Reject stale or modified build.ninja and compile_commands before any check."""
    receipt = json.loads(GRAPH_RECEIPT.read_text())
    status_path = BUILD_ROOT / "build_status.json"
    pins_path = BUILD_ROOT / "source_pins.json"
    pins = json.loads(pins_path.read_text())
    generated = {str((REPO / "integration_evidence/build-integrated-v3-p5-20261114" / name).resolve())
                 for name in GRAPH_NAMES}
    assert not generated.intersection(pins), "generated build products must not appear as source pins"
    assert receipt["build_status_sha256"] == sha(status_path)
    assert receipt["source_pins_sha256"] == sha(pins_path)
    assert receipt["after"] and all(Path(path).is_file() and sha(path) == digest
                                     for path, digest in receipt["after"].items())
    status = json.loads(status_path.read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert status["binary_sha256"] == receipt["binary_sha256"]
    assert status["test_binary_sha256"] == receipt["test_binary_sha256"]
    return receipt


def descriptor_checks(unit):
    """Check exact descriptor inputs, action mappings, tags, and unchanged source hashes."""
    paths = [A2 / "descriptors/requirements_unit.json", A2 / "descriptors/lane_census_unit.json",
             A2 / "descriptors/m6b_projection.json"]
    records = [unit.validate_descriptor(path) for path in paths]
    mesh = Path(records[2]["environment"]["SU2_NATIVE_D9_MESH"]).resolve(strict=True)
    sidecar = Path(str(mesh) + ".native_ref").resolve(strict=True)
    assert sha(mesh) == records[2]["input_file_sha256"][str(mesh)]
    assert sha(sidecar) == records[2]["input_file_sha256"][str(sidecar)]
    assert records[2]["case_name"] == "n5_d9_a3_m6b_projection_533"
    assert records[2]["test_filter"] == "[.][NativeM6bProjection]"
    assert [item["case_name"] for item in records[:2]] == [
        "n5_d9_a2_requirements_533", "n5_d9_n12_lane_census_533"]
    return {"descriptors": [str(path) for path in paths], "accepted_euler_mesh_sha256": sha(mesh),
            "native_reference_sha256": sha(sidecar), "m6b_expected_edge_count": 82559}


def synthetic_artifact_check():
    """Check real CSV counting behavior without creating a numerical case."""
    rows = [{"edge": "1", "projection_status": "refused", "orientation_sign": "-1"},
            {"edge": "2", "projection_status": "accepted", "orientation_sign": "1"}]
    assert row_counts(rows, ["orientation_sign"]) == {"orientation_sign": {"-1": 1, "1": 1}}
    assert row_counts(rows, ["projection_status"]) == {"projection_status": {"accepted": 1, "refused": 1}}
    assert row_counts([], ["projection_status"]) == {}
    return True


def m6b_schema_self_check():
    """Exercise the real emitted schema, including refused edges and bad totals."""
    signs = [f"{marker}_{sign}" for marker in ("wing", "other")
             for sign in ("positive", "zero", "negative")]
    faces = ["wing_shell_faces", "other_shell_faces"]
    first = dict(edge_a="1", edge_b="2", projection_attempted="true", projection_succeeded="1",
                 shell_measured="1", projection_reason="", wing_shell_faces="2", other_shell_faces="1",
                 wing_positive="1", wing_zero="0", wing_negative="1",
                 other_positive="0", other_zero="1", other_negative="0")
    second = dict(first, edge_a="2", edge_b="3", projection_succeeded="0", shell_measured="0",
                  projection_reason="refused", **{key: "" for key in signs + faces})
    data = dict(wall_edges=82559, attempted=2, unvisited=82557, all_edges_attempted=False,
                projection_succeeded=1, projection_refusals=1, all_attempted_shells_measured=False,
                shell_faces=3, wing_faces=2, other_faces=1,
                **{key: int(first[key]) for key in signs})
    assert validate_m6b_rows(data, list(first), [first, second])["projection_refusals"] == 1
    for bad_rows, bad_data in (([first, first], data),
                                ([first, second], dict(data, wing_positive=2))):
        try:
            validate_m6b_rows(bad_data, list(first), bad_rows)
        except AssertionError:
            pass
        else:
            raise AssertionError("M6b schema check accepted duplicate edge or wrong sign sum")
    return True


def self_check():
    """Validate runner wiring and descriptors without launching a build or test."""
    unit = load_unit()
    inherited = unit.self_check()
    descriptors = descriptor_checks(unit)
    assert inherited["passed"] and synthetic_artifact_check()
    fixture_root = REPO / "UnitTests/Common/adaptation/fixtures"
    for name, tag in (("native_requirements3d_tests.inc", "[NativeRequirements3D]"),
                      ("native_lane_census3d_tests.inc", "[NativeLaneCensus3D]")):
        assert f'"{tag}"' in (fixture_root / name).read_text()
    assert m6b_schema_self_check()
    source = (STAGED / "run_cases.py").read_text()
    assert 'timeout_seconds = 2400 if case == "m6_stage" else 600' in source
    adapted = m6b_timeout_main(source, "n5_d9_a3_m6b_projection_533")
    assert "5400 if case == 'n5_d9_a3_m6b_projection_533'" in adapted
    assert 'else 2400 if case == "m6_stage" else 600' in adapted
    return {"passed": True, "launched": False, "n5_harness": inherited,
            "descriptor_inputs": descriptors, "generic_counter_check_reused": True,
            "m6b_artifacts": ["native_m6b_projection.json", "native_m6b_projection.csv"],
            "m6b_fixture_timeout_seconds": 5400, "production_timeout_source_unchanged": True}


def main():
    """Run readiness, write aliases, self-check, or exactly one descriptor-backed case."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--sync-build-alias", action="store_true")
    parser.add_argument("--descriptor", type=Path)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    unit = load_unit()
    if args.sync_build_alias:
        verify_generated_graph()
        print(json.dumps(unit.core.sync_build_alias(), indent=2))
        return
    if args.descriptor is None:
        parser.error("provide --descriptor or --self-check")
    verify_generated_graph()
    result = unit.run(args.descriptor.resolve(strict=True))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
