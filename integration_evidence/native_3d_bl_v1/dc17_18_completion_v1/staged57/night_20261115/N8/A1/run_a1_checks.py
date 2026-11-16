#!/usr/bin/env python3
"""A1 focused runner retargeting the accepted N5 descriptor harness to build532."""
import argparse
import importlib.util
import json
import os
import sys
from pathlib import Path

A1 = Path(__file__).resolve().parent
N5 = A1.parents[1] / "N5"
STAGED = A1.parents[2]
REPO = A1.parents[6]
UNIT_PATH = N5 / "run_d9_units.py"
GEOMETRY = {
    "n5_d9_a1_geometry_euler293": (7521, "e19ab8eef41204c1b668f443dc3376218d9c5e437463689f00a5c4a3b2a7fd09"),
    "n5_d9_a1_geometry_saved492": (6759, "934bcc20ac45b1d4d451430f603d477c7001a10aa3299582aeed55de99b91e6c"),
    "n5_d9_a1_geometry_euler_input_decode12": (4368, "9897699675dca2c8208f6d588ac0d1aac0f9cde18bd6e20ba58a559a5dcfcfb7"),
}


def load_unit():
    os.environ.setdefault("OMPI_MCA_osc", "pt2pt")
    sys.path.insert(0, str(N5))
    spec = importlib.util.spec_from_file_location("a1_n5_d9_unit_runner", UNIT_PATH)
    unit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(unit)
    core = unit.core
    unit.N5 = A1
    core.N5 = A1
    core.BUILD521 = A1 / "build532"
    core.ALIAS_STATUS = A1 / "build532_status.json"
    core.ALIAS_PINS = A1 / "build532_source_pins.json"
    core.ALIAS_HANDLES = A1 / "build532_terminal_handles.json"
    core.RUN_READY = A1 / "primary_run_ready.json"
    unit.ACTION_RANKS["wall_law"] = {1}
    unit.ACTION_TAGS["wall_law"] = "[NativeWallGeometryLaw3D]"
    return unit


def checked_artifacts(unit):
    original = unit.produced_artifacts
    def validate(action, attempt, ranks, resource_path):
        artifacts = original(action, attempt, ranks, resource_path)
        case = next((name for name in GEOMETRY if name in attempt.name), None)
        if action == "geometry":
            assert case, f"unexpected A1 geometry attempt name: {attempt.name}"
            summary = json.loads((attempt / "d9_geometry_summary.json").read_text())
            expected_count, expected_sha = GEOMETRY[case]
            assert summary["M1"]["geometry_need_edges"] == expected_count
            assert unit.core.sha(attempt / "d9_geometry_edges.csv") == expected_sha
        elif action == "wall_law":
            assert "NativeWallGeometryLaw3D" in (attempt / "run.log").read_text(errors="replace")
        return artifacts
    unit.produced_artifacts = validate
    return original


def self_check():
    assert A1.name == "A1" and REPO.name == "SU2_NativeIntegrated"
    assert STAGED.name == "staged57" and len(GEOMETRY) == 3
    unit = load_unit()
    sources = {
        "geometry_euler293_A1.json": "geometry_euler293_R1_repeat.json",
        "geometry_saved492_A1.json": "geometry_saved492_R1_repeat.json",
        "geometry_euler_input_decode12_A1.json": "geometry_euler_input_decode12_R1_repeat.json",
    }
    for filename, source_name in sources.items():
        path = A1 / "descriptors" / filename
        candidate = unit.validate_descriptor(path)
        source_path = N5 / "descriptors" / source_name
        source = json.loads(source_path.read_text())
        expected = dict(source, case_name=candidate["case_name"])
        assert candidate == expected, f"unexpected descriptor changes: {path}"
        receipt_path = N5 / "unit_measurement_receipts" / (source["case_name"] + ".json")
        receipt = json.loads(receipt_path.read_text())
        case = candidate["case_name"]
        count, edge_sha = GEOMETRY[case]
        assert receipt["descriptor_sha256"] == unit.core.sha(source_path)
        assert receipt["artifacts_sha256"]["d9_geometry_edges.csv"] == edge_sha
        attempt = Path(receipt["attempt"])
        summary = json.loads((attempt / "d9_geometry_summary.json").read_text())
        assert summary["M1"]["geometry_need_edges"] == count
        assert unit.core.sha(attempt / "d9_geometry_edges.csv") == edge_sha
    law_path = A1 / "descriptors/wall_geometry_law_A1.json"
    law = unit.validate_descriptor(law_path)
    assert law["test_filter"] == "[NativeWallGeometryLaw3D]"
    assert law["ranks"] == 1 and not law["environment"] and not law["input_file_sha256"]
    return {"passed": True, "launched": False, "geometry_cases": len(sources),
            "descriptor_inputs_validated": True, "baseline_csv_provenance_validated": True,
            "law_descriptor_validated": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--sync-build-alias", action="store_true")
    parser.add_argument("--descriptor", type=Path)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    unit = load_unit()
    checked_artifacts(unit)
    if args.sync_build_alias:
        print(json.dumps(unit.core.sync_build_alias(), indent=2))
        return
    if args.descriptor is None:
        parser.error("provide --descriptor or --sync-build-alias")
    result = unit.run(args.descriptor.resolve(strict=True))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
