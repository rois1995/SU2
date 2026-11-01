#!/usr/bin/env python3
"""Prepare RecordStream inputs from the complete, hash-pinned build202 failure."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import math
import struct
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
A = HERE / "m6_runs/euler_build202_real_cfd50k_inspection"
EXPECTED_ROWS = 810_163
FACES = ((1, 2, 3), (0, 3, 2), (0, 1, 3), (0, 2, 1))
POINT = struct.Struct("<QdddQQQdd")
SURFACE = struct.Struct("<QQQQ")
METRIC = struct.Struct("<Qdd" + "QQd" * 6)
U64 = struct.Struct("<Q")

spec = importlib.util.spec_from_file_location("inspect_native_failure3d", HERE / "inspect_native_failure3d.py")
reader = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = reader
spec.loader.exec_module(reader)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def input_hashes() -> dict[str, str]:
    command = json.loads((A / "inspection_assembly_command.json").read_text())
    expected = command["input_sha256"]
    assert expected and len(expected) >= 30
    actual = {name: sha256(Path(name)) for name in sorted(expected)}
    assert actual == expected, "build202 input hash mismatch"
    return actual


def exact_positive(nodes: list[dict]) -> bool:
    columns = []
    for axis in range(3):
        ratios = [node["xyz"][axis].as_integer_ratio() for node in nodes]
        denominator = max(den for _, den in ratios)
        columns.append([num * (denominator // den) for num, den in ratios])
    p = [[columns[axis][k] for axis in range(3)] for k in range(4)]
    a, b, c = ([p[k][i] - p[0][i] for i in range(3)] for k in (1, 2, 3))
    determinant = (a[0] * (b[1] * c[2] - b[2] * c[1])
                   - a[1] * (b[0] * c[2] - b[2] * c[0])
                   + a[2] * (b[0] * c[1] - b[1] * c[0]))
    return determinant > 0


def encode(path: Path, records: list[bytes], record: struct.Struct) -> None:
    assert not path.exists(), f"refusing to overwrite {path}"
    assert all(len(row) == record.size for row in records)
    with path.open("xb") as stream:
        stream.write(U64.pack(len(records)))
        for row in records:
            stream.write(row)
    assert path.stat().st_size == 8 + len(records) * record.size
    data = path.read_bytes()
    assert U64.unpack_from(data)[0] == len(records)
    if records:
        assert data[8:8 + record.size] == records[0]
        assert data[-record.size:] == records[-1]
        assert record.unpack(data[8:8 + record.size])
        assert record.unpack(data[-record.size:])


def main() -> None:
    started = time.monotonic()
    before = input_hashes()
    manifests, missing = reader._load_manifests([str(A / "*_inspection_rank*_manifest.json")])
    assert not missing and len(manifests) == 6
    assert all(m["complete_shard"] and m["owned_complete"] and m["metrics_complete"] for m in manifests)
    assert sum(m["written_rows"] for m in manifests) == EXPECTED_ROWS

    points: dict[int, tuple[tuple[float, float, float], tuple]] = {}
    physical: dict[tuple[int, int, int], tuple[tuple[int, int, int], int]] = {}
    target_state = json.loads((A / "witness_143424775_state.json").read_text())
    selected = {cell for cells in target_state["stars"].values() for cell in cells}
    assert len(selected) == target_state["complete_vertex_star_cells"] == 52
    metrics: dict[int, bytes] = {}
    rows = 0

    for manifest in manifests:
        files, cap = manifest["_resolved_files"], manifest["_caps"]
        owned = reader._read_owned(files["owned"], manifest["written_rows"], cap["owned"])
        metric_rows = reader._jsonl(files["metrics"], cap["metrics"], "metrics")
        for cell in owned:
            row = next(metric_rows, None)
            assert row is not None, f"short paired metric stream on rank {manifest['rank']}"
            m = reader._validate_metric(row, cell, files["metrics"])
            rows += 1
            assert exact_positive(cell["nodes"]), f"cell {cell['id']} is not exactly positive"
            for node, binding in zip(cell["nodes"], m["bindings"]):
                if binding is None:
                    continue
                assert binding["id"] == node["id"]
                key = (tuple(node["xyz"]), binding["feature"], binding["version"],
                       binding["dimension"], binding["u"], binding["v"])
                prior = points.setdefault(node["id"], (tuple(node["xyz"]), key[1:]))
                assert prior == (tuple(node["xyz"]), key[1:]), f"binding disagreement for node {node['id']}"
            for slot, marker in enumerate(cell["physical"]):
                if not marker["present"]:
                    continue
                ordered = tuple(cell["nodes"][i]["id"] for i in FACES[slot])
                face_key = tuple(sorted(ordered))
                assert face_key not in physical, f"physical face has multiple owners: {face_key}"
                physical[face_key] = (ordered, marker["facet"])
            if cell["id"] in selected:
                assert cell["id"] not in metrics
                assert m["q"] is not None and m["J"] is not None
                values = [cell["id"], m["q"], m["J"]]
                for a, b, length in m["edges"]:
                    assert length is not None and math.isfinite(length)
                    values.extend((a, b, length))
                metrics[cell["id"]] = METRIC.pack(*values)
        assert next(metric_rows, None) is None, f"extra paired metric rows on rank {manifest['rank']}"

    assert rows == EXPECTED_ROWS
    surface_vertices = {node for key in physical for node in key}
    assert surface_vertices == set(points), "physical surface vertices and bound node IDs differ"
    assert len(points) > 0 and len(physical) > 0 and set(metrics) == selected
    point_records = [POINT.pack(node_id, *xyz, *binding)
                     for node_id, (xyz, binding) in sorted(points.items())]
    surface_records = [SURFACE.pack(*ordered, facet)
                       for _, (ordered, facet) in sorted(physical.items())]
    metric_records = [metrics[cell] for cell in sorted(selected)]

    encode(A / "witness_boundary_points.bin", point_records, POINT)
    encode(A / "witness_boundary_surface.bin", surface_records, SURFACE)
    encode(A / "witness_expected_metrics.bin", metric_records, METRIC)
    after = input_hashes()
    assert before == after, "build202 inputs changed during preparation"

    receipt = {
        "schema": "build202_boundary_preparation_v1", "input_hashes_before": before,
        "input_hashes_after": after, "paired_rows": rows, "non_null_binding_points": len(points),
        "physical_faces": len(physical), "physical_vertices": len(surface_vertices),
        "selected_metric_cells": len(metrics), "record_bytes": {
            "BoundaryPoint": POINT.size, "SurfaceFace": SURFACE.size, "expected_metric": METRIC.size},
        "outputs": {name: {"bytes": (A / name).stat().st_size, "sha256": sha256(A / name)} for name in (
            "witness_boundary_points.bin", "witness_boundary_surface.bin", "witness_expected_metrics.bin")},
        "elapsed_seconds": time.monotonic() - started,
        "scope": "offline serialization only; current boundary coordinates/associations and faces, not historical trace authority or mesh acceptance",
    }
    receipt_path = A / "witness_boundary_preparation.json"
    assert not receipt_path.exists()
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print(json.dumps({key: receipt[key] for key in (
        "paired_rows", "non_null_binding_points", "physical_faces", "selected_metric_cells",
        "record_bytes", "elapsed_seconds", "outputs")}, indent=2))


if __name__ == "__main__":
    main()
