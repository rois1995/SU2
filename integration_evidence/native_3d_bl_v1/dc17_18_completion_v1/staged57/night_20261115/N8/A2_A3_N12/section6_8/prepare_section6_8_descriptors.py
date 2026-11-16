#!/usr/bin/env python3
"""Clone only the accepted fixture descriptors, changing case names for build534."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
A2 = HERE.parent
N8 = A2.parent
NIGHT = N8.parent
N5 = NIGHT / "N5"
OUTPUT = HERE / "descriptors"

SOURCES = {
    "n5_d9_a2_requirements_534": N8 / "A2_A3_N12/descriptors/requirements_unit.json",
    "n5_d9_n12_lane_census_534": N8 / "A2_A3_N12/descriptors/lane_census_unit.json",
    "n5_d9_a1_wall_law_534": N8 / "A1/descriptors/wall_geometry_law_A1.json",
    "n5_d9_a3_m6b_projection_534": N8 / "A2_A3_N12/descriptors/m6b_projection.json",
    "n5_d9_a1_geometry_euler293_534": N8 / "A1/descriptors/geometry_euler293_A1.json",
    "n5_d9_a1_geometry_saved492_534": N8 / "A1/descriptors/geometry_saved492_A1.json",
    "n5_d9_a1_geometry_euler_input_decode12_534": N8 / "A1/descriptors/geometry_euler_input_decode12_A1.json",
}
DIAGNOSTICS = "n5_d9_a1_wall_diagnostics68_534"


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_descriptor(case, source):
    data = json.loads(Path(source).read_text())
    data["case_name"] = case
    for raw, digest in data["input_file_sha256"].items():
        assert sha(raw) == digest, f"fixture input changed: {raw}"
    path = OUTPUT / f"{case}.json"
    payload = json.dumps(data, indent=2) + "\n"
    if path.exists():
        assert path.read_text() == payload, f"refusing changed descriptor: {path}"
    else:
        path.write_text(payload)
    return path


def write_diagnostic_descriptor():
    fixture = Path("UnitTests/Common/adaptation/fixtures/native_wall_diagnostics68_tests.inc")
    source = N8.parents[5] / fixture
    text = source.read_text()
    assert '[NativeWallDiagnostics68]' in text
    assert '[.][NativeWallDiagnostics68]' not in text
    case = DIAGNOSTICS
    data = {"schema": "n5_d9_measurement_v1", "action": "counter_check", "ranks": 1,
            "case_name": case, "test_filter": "[NativeWallDiagnostics68]",
            "environment": {}, "input_file_sha256": {}}
    path = OUTPUT / f"{case}.json"
    payload = json.dumps(data, indent=2) + "\n"
    if path.exists():
        assert path.read_text() == payload, f"refusing changed descriptor: {path}"
    else:
        path.write_text(payload)
    return path


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    paths = [write_descriptor(case, source) for case, source in SOURCES.items()]
    paths.append(write_diagnostic_descriptor())
    print(json.dumps({"passed": True, "launched": False,
                      "descriptors": {path.name: sha(path) for path in paths}}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
