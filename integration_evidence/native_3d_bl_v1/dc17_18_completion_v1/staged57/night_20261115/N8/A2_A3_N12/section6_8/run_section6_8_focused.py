#!/usr/bin/env python3
"""Run one section 6.8 focused descriptor with the current build534 source pins."""
import argparse
import hashlib
import importlib.util
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
A2 = HERE.parent
N8 = A2.parent
NIGHT = N8.parent
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
N5 = NIGHT / "N5"
BASE_RUNNER = A2 / "run_a2_a3_n12_checks.py"
BUILD_ROOT = HERE / "build534"
READY = HERE / "primary_run_ready.json"
SOURCE_CHECK = HERE / "source_check.md"
GRAPH_RECEIPT = BUILD_ROOT / "generated_build_graph_after.json"
UNIT_PATH = N5 / "run_d9_units.py"
COMPARATOR = HERE / "validation/compare_a1_edge_rows.py"
DESCRIPTORS = HERE / "descriptors"

CASES = {
    "n5_d9_a2_requirements_534": ("counter_check", "[NativeRequirements3D]"),
    "n5_d9_n12_lane_census_534": ("counter_check", "[NativeLaneCensus3D]"),
    "n5_d9_a1_wall_law_534": ("wall_law", "[NativeWallGeometryLaw3D]"),
    "n5_d9_a1_wall_diagnostics68_534": ("counter_check", "[NativeWallDiagnostics68]"),
    "n5_d9_a3_m6b_projection_534": ("actual_operators", "[.][NativeM6bProjection]"),
    "n5_d9_a1_geometry_euler293_534": ("geometry", "[.][NativeD9Geometry]"),
    "n5_d9_a1_geometry_saved492_534": ("geometry", "[.][NativeD9Geometry]"),
    "n5_d9_a1_geometry_euler_input_decode12_534": ("geometry", "[.][NativeD9Geometry]"),
}
GEOMETRY = {
    "n5_d9_a1_geometry_euler293_534": ("euler293", 7521),
    "n5_d9_a1_geometry_saved492_534": ("saved492", 6759),
    "n5_d9_a1_geometry_euler_input_decode12_534": ("euler_input_decode12", 4368),
}
REF_CASES = {
    "euler293": "n5_d9_geometry_euler293_R1_repeat",
    "saved492": "n5_d9_geometry_saved492_R1_repeat",
    "euler_input_decode12": "n5_d9_geometry_euler_input_decode12_R1_repeat",
}


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def expected_tag(case):
    return CASES[case][1]


def patch_descriptor_validator(parent, unit):
    original = unit.validate_descriptor
    def validate(path):
        data = unit.strict_json(Path(path))
        case = data.get("case_name")
        assert case in CASES and CASES[case][0] == data.get("action")
        assert data.get("test_filter") == expected_tag(case)
        unit.ACTION_TAGS[data["action"]] = expected_tag(case)
        return original(path)
    unit.validate_descriptor = validate


def pin_baseline_files(pinned, case):
    if case not in GEOMETRY:
        return
    fixture, _ = GEOMETRY[case]
    receipt_path = N5 / "unit_measurement_receipts" / f"{REF_CASES[fixture]}.json"
    receipt = json.loads(receipt_path.read_text())
    attempt = Path(receipt["attempt"])
    paths = [receipt_path, Path(receipt["descriptor"]), attempt / "status.json",
             attempt / "d9_geometry_edges.csv", attempt / "d9_geometry_summary.json"]
    for path in paths:
        pinned[str(path.resolve())] = sha(path)


def patch_pin_descriptor(parent, unit):
    parent.patch_pin_descriptor(unit)
    original = unit.pin_descriptor
    def pin(runtime, descriptor_path, descriptor):
        original(runtime, descriptor_path, descriptor)
        old_save = runtime.save
        extras = [Path(__file__).resolve(), COMPARATOR, SOURCE_CHECK, GRAPH_RECEIPT,
                  READY, BUILD_ROOT / "build_status.json", BUILD_ROOT / "source_pins.json",
                  BUILD_ROOT / "terminal_handles.json", BUILD_ROOT / "build.py", *DESCRIPTORS.glob("*.json")]
        def save_with_group_inputs(path, value):
            if Path(path).name == "pins.json":
                for source in extras:
                    value[str(source.resolve())] = sha(source)
                pin_baseline_files(value, descriptor["case_name"])
            old_save(path, value)
        runtime.save = save_with_group_inputs
    unit.pin_descriptor = pin


def patch_artifact_checker(parent, unit):
    original = unit.produced_artifacts
    def check(action, attempt, ranks, resource_path):
        if action == "actual_operators":
            artifacts = parent.inspect_m6b_artifacts(unit, attempt, original, ranks, resource_path)
        else:
            artifacts = original(action, attempt, ranks, resource_path)
        case = next((name for name in CASES if name in attempt.name), None)
        assert case, f"unrecognized section 6.8 case directory: {attempt.name}"
        log = (attempt / "run.log").read_text(errors="replace")
        assert expected_tag(case).split("[")[-1].rstrip("]") in log
        if case in GEOMETRY:
            from compare_a1_edge_rows import compare_rows
            fixture, expected = GEOMETRY[case]
            reference_receipt = N5 / "unit_measurement_receipts" / f"{REF_CASES[fixture]}.json"
            reference = json.loads(reference_receipt.read_text())
            reference_dir = Path(reference["attempt"])
            comparison = compare_rows(reference_dir / "d9_geometry_edges.csv",
                                      attempt / "d9_geometry_edges.csv",
                                      reference_dir / "d9_geometry_summary.json",
                                      attempt / "d9_geometry_summary.json", expected)
            artifact_path = attempt / "a1_rowwise_comparison.json"
            artifact_path.write_text(json.dumps(comparison, indent=2, sort_keys=True) + "\n")
            artifacts[artifact_path.name] = sha(artifact_path)
            assert comparison["pass"], f"A1 rowwise result failed: {comparison['first_difference']}"
        return artifacts
    unit.produced_artifacts = check


def patch_unit_timeout(parent, unit):
    original_run = unit.run
    def run_descriptor(path):
        descriptor = unit.validate_descriptor(path)
        if descriptor["case_name"] == "n5_d9_a3_m6b_projection_534":
            sys.path.insert(0, str(STAGED))
            import run_cases as runtime
            source = Path(runtime.__file__).read_text()
            body = parent.m6b_timeout_main(source, descriptor["case_name"])
            exec(compile(body, runtime.__file__, "exec"), runtime.__dict__)
        return original_run(path)
    unit.run = run_descriptor


def load_unit():
    os.environ.setdefault("OMPI_MCA_osc", "pt2pt")
    parent = load_module("section6_8_parent_runner", BASE_RUNNER)
    parent.__file__ = str(Path(__file__).resolve())
    parent.A2 = HERE
    parent.NIGHT = NIGHT
    parent.STAGED = STAGED
    parent.REPO = REPO
    parent.N5 = N5
    parent.UNIT_PATH = UNIT_PATH
    parent.BUILD_ROOT = BUILD_ROOT
    parent.READY = READY
    parent.SOURCE_CHECK = SOURCE_CHECK
    parent.GRAPH_RECEIPT = GRAPH_RECEIPT
    sys.path.insert(0, str(COMPARATOR.parent))
    parent.TAG_BY_ACTION = {
        "counter_check": {tag for action, tag in CASES.values() if action == "counter_check"},
        "actual_operators": {"[.][NativeM6bProjection]"},
        "geometry": {"[.][NativeD9Geometry]"},
        "wall_law": {"[NativeWallGeometryLaw3D]"},
    }
    parent.EXPECTED_TAGS = {case: tag for case, (_, tag) in CASES.items()}
    sys.path.insert(0, str(N5))
    spec = importlib.util.spec_from_file_location("section6_8_n5_unit_runner", UNIT_PATH)
    unit = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = unit
    spec.loader.exec_module(unit)
    unit.N5 = HERE
    unit.STAGED = STAGED
    core = unit.core
    core.N5 = HERE
    core.BUILD521 = BUILD_ROOT
    core.ALIAS_STATUS = HERE / "build534_status.json"
    core.ALIAS_PINS = HERE / "build534_source_pins.json"
    core.ALIAS_HANDLES = HERE / "build534_terminal_handles.json"
    core.RUN_READY = READY
    core.no_heavy_processes = parent.no_owned_jobs_active
    patch_descriptor_validator(parent, unit)
    patch_pin_descriptor(parent, unit)
    patch_artifact_checker(parent, unit)
    patch_unit_timeout(parent, unit)
    unit.ACTION_RANKS["wall_law"] = {1}
    unit.ACTION_TAGS["wall_law"] = "[NativeWallGeometryLaw3D]"
    return parent, unit


def descriptor_checks(unit):
    paths = sorted(DESCRIPTORS.glob("*.json"))
    records = [unit.validate_descriptor(path) for path in paths]
    assert {record["case_name"] for record in records} == set(CASES)
    bases = {
        "n5_d9_a2_requirements_534": N8 / "A2_A3_N12/descriptors/requirements_unit.json",
        "n5_d9_n12_lane_census_534": N8 / "A2_A3_N12/descriptors/lane_census_unit.json",
        "n5_d9_a1_wall_law_534": N8 / "A1/descriptors/wall_geometry_law_A1.json",
        "n5_d9_a1_wall_diagnostics68_534": None,
        "n5_d9_a3_m6b_projection_534": N8 / "A2_A3_N12/descriptors/m6b_projection.json",
        "n5_d9_a1_geometry_euler293_534": N8 / "A1/descriptors/geometry_euler293_A1.json",
        "n5_d9_a1_geometry_saved492_534": N8 / "A1/descriptors/geometry_saved492_A1.json",
        "n5_d9_a1_geometry_euler_input_decode12_534": N8 / "A1/descriptors/geometry_euler_input_decode12_A1.json",
    }
    for record in records:
        base_path = bases[record["case_name"]]
        if base_path:
            base = json.loads(base_path.read_text())
            assert record == dict(base, case_name=record["case_name"])
    for case, (action, tag) in CASES.items():
        assert next(row for row in records if row["case_name"] == case)["test_filter"] == tag
    return {"count": len(records), "case_names": sorted(CASES), "exact_source_descriptors_reused": True}


def self_check():
    parent, unit = load_unit()
    inherited = unit.self_check()
    descriptors = descriptor_checks(unit)
    assert inherited["passed"] and parent.m6b_schema_self_check()
    assert all(name in CASES for name in EXPECTED_CASES)
    return {"passed": True, "launched": False, "build": str(BUILD_ROOT),
            "focused_cases": descriptors, "n5_harness_self_check": inherited,
            "M6b_schema_self_check": True, "geometry_comparator": str(COMPARATOR),
            "all_m6b_artifact_checks_reused": True}


EXPECTED_CASES = tuple(CASES)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--sync-build-alias", action="store_true")
    parser.add_argument("--descriptor", type=Path)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2, sort_keys=True))
        return
    parent, unit = load_unit()
    if args.sync_build_alias:
        parent.verify_generated_graph()
        print(json.dumps(unit.core.sync_build_alias(), indent=2))
        return
    if args.descriptor is None:
        parser.error("provide --descriptor or --self-check")
    parent.verify_generated_graph()
    result = unit.run(args.descriptor.resolve(strict=True))
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
