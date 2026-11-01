"""Bounded offline inspection of the selected build202 mesh; no repair decisions."""
import collections
import hashlib
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET

import inspect_native_failure3d as reader

ROOT = Path(__file__).resolve().parent
ATTEMPT = ROOT / "m6_runs/euler_build202_real_cfd50k_inspection"
OUTPUT = ATTEMPT / "inspection_analysis.json"


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(4 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def vtu(path):
    piece = ET.parse(path).getroot().find("./UnstructuredGrid/Piece")
    data = {a.attrib["Name"]: a.text.split() for a in piece.find("CellData")}
    return piece, data


def main():
    assert not OUTPUT.exists(), "Never overwrite an inspection receipt"
    manifest_path = ATTEMPT / "inspection/inspect_manifest.json"
    manifest = json.loads(manifest_path.read_text())
    assert not manifest["partial"] and not manifest["final_mesh_accepted"]
    assert all(manifest[k] for k in ("final_mesh_complete", "final_su2_complete",
                                   "full_mesh_measurements_complete", "cavity_complete", "bad_set_complete"))
    assert manifest["actual_full_mesh_cells"] == 810163
    assert manifest["actual_full_mesh_points"] == 164526
    with (ATTEMPT / "m6.su2.native_ref").open() as stream:
        assert stream.readline().strip() == "SU2_NATIVE_REFERENCE_3D 7"
        assert stream.readline().split()[0] == "3"
        assert [json.loads(stream.readline()) for _ in range(3)] == ["FARFIELD", "SYMMETRY", "WING3D"]

    # Parse the standalone SU2 file independently of the raw-record decoder.
    mesh = ATTEMPT / "inspection/rejected_mesh.su2"
    with mesh.open() as stream:
        lines = (line.strip() for line in stream if line.strip() and not line.startswith("%"))
        assert next(lines) == "NDIME= 3"
        assert next(lines) == "NELEM= 810163"
        for _ in range(810163):
            row = list(map(int, next(lines).split()))
            assert len(row) == 5 and row[0] == 10 and len(set(row[1:])) == 4
            assert all(0 <= n < 164526 for n in row[1:])
        assert next(lines) == "NPOIN= 164526"
        for i in range(164526):
            row = next(lines).split()
            assert len(row) == 4 and int(row[3]) == i
            assert all(math.isfinite(float(v)) for v in row[:3])
        assert next(lines) == "NMARK= 3"
        markers = {}
        for _ in range(3):
            tag = next(lines).split("=", 1)[1].strip()
            count = int(next(lines).split("=", 1)[1])
            assert tag not in markers
            markers[tag] = count
            for _ in range(count):
                row = list(map(int, next(lines).split()))
                assert len(row) == 4 and row[0] == 5 and len(set(row[1:])) == 3
                assert all(0 <= n < 164526 for n in row[1:])
        assert next(lines, None) is None

    # Producer writes bad rows first. Stop at the first clean row in each shard.
    manifests, missing = reader._load_manifests([str(ATTEMPT / "*inspection_rank*_manifest.json")])
    assert not missing
    bad = []
    for rank in manifests:
        metrics = reader._jsonl(rank["_resolved_files"]["metrics"], rank["_caps"]["metrics"], "metrics")
        owned = reader._read_owned(rank["_resolved_files"]["owned"], rank["written_rows"], rank["_caps"]["owned"])
        for cell, metric in zip(owned, metrics):
            metric = reader._validate_metric(metric, cell, rank["_resolved_files"]["metrics"])
            if not metric["is_bad"]:
                break
            bad.append({"cell": cell, "metric": metric, "rank": rank["rank"]})
    assert len(bad) == manifest["bad_rows_total"] == 2476
    assert len({row["cell"]["id"] for row in bad}) == len(bad)
    totals = {key: sum(row["metric"][key] for row in bad)
              for key in ("q_bad", "j_bad", "geometry_bad", "size_bad")}
    assert totals == {"q_bad": 295, "j_bad": 294, "geometry_bad": 2110, "size_bad": 0}

    location = collections.Counter()
    union = []
    unique_short = set()
    for row in bad:
        m = row["metric"]
        if not (m["q_bad"] or m["j_bad"]):
            continue
        union.append(row)
        kind = "surface" if m["surface"] else "constrained_interior" if any(m["bindings"]) else "free_interior"
        location[kind] += 1
        for a, b, length in m["edges"]:
            if length < .5:
                unique_short.add(tuple(sorted((a, b))))
    assert len(union) == 450

    # Nearby controls share the exported vertex-star region, not matched anisotropy.
    _, cavity = vtu(ATTEMPT / "inspection/cavities.vtu")
    controls = collections.defaultdict(lambda: {"cells": 0, "short_below_05": 0, "short_below_09": 0})
    for i in range(len(cavity["q"])):
        shape_bad = float(cavity["q"][i]) < .2 or float(cavity["J"][i]) < .05
        surface = int(cavity["surface"][i])
        key = ("shape_bad" if shape_bad else "shape_clean") + ("_surface" if surface else "_no_physical_face")
        row = controls[key]
        row["cells"] += 1
        row["short_below_05"] += float(cavity["metric_edge_min"][i]) < .5
        row["short_below_09"] += float(cavity["metric_edge_min"][i]) < .9
    assert sum(v["cells"] for v in controls.values()) == manifest["cavity_cells_retained"]
    assert sum(v["cells"] for k, v in controls.items() if k.startswith("shape_bad")) == 450
    selected = sorted(union, key=lambda row: min(row["metric"]["q"] / .2, row["metric"]["J"] / .05))[:12]
    paths = [Path(__file__), ROOT / "inspect_native_failure3d.py", manifest_path, mesh,
             ATTEMPT / "inspection/bad_cells.vtu", ATTEMPT / "inspection/cavities.vtu",
             ATTEMPT / "inspection/rejected_mesh.vtu", ATTEMPT / "m6.su2.native_ref"]
    result = {
        "scope": "Actual final build202 inspection; not accepted mesh or historical trial replay",
        "artifact_sha256": {str(p): sha(p) for p in paths},
        "points": 164526, "cells": 810163, "marker_triangles": markers,
        "marker_names_from_original_sidecar": {"Marker_0": "FARFIELD", "Marker_1": "SYMMETRY", "Marker_2": "WING3D"},
        "terminal_bad_counts": totals, "shape_bad_union": len(union), "shape_bad_location": dict(location),
        "unique_shape_bad_edges_below_05": len(unique_short),
        "nearby_cavity_controls": dict(controls), "selected_witnesses": selected,
        "limits": ["Controls are local neighbors, not matched metric anisotropy/variation.",
                   "Unbound means no captured boundary association; not a global freedom proof.",
                   "Geometry excess is a conservative admission bound, not proved distance violation.",
                   "SU2 labels use explicit fallback names; original marker ordering retained.",
                   "No topology, geometry, quality, transfer or solver acceptance follows from parsing."],
    }
    OUTPUT.write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: result[k] for k in ("points", "cells", "terminal_bad_counts", "shape_bad_location", "nearby_cavity_controls")}, indent=2))


if __name__ == "__main__":
    main()
