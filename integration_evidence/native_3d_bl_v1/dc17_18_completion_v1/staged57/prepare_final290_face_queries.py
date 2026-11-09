#!/usr/bin/env python3
"""Prepare one bounded binary geometry query for every captured physical face in A289."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import struct
import sys
import time


R = Path(__file__).resolve().parent
RUN = R / "m6_runs/euler_build289_prepared_borders_mach_only_real_cfd50k"
OUT = RUN / "final_face_queries_v1"
READER = R / "inspect_native_failure3d.py"
NATIVE_REF = RUN / "m6.su2.native_ref"
FILE_LIMIT = 128 << 20
OWNED_ROWS_EXPECTED = 736323
PHYSICAL_FACES_EXPECTED = 69544
WING_ENTITY = (0, 10154, 2)
WING_FACES_EXPECTED = 53512
MAGIC = 0x4D36515545525931
FORMAT_VERSION = 2
TOLERANCE = 1e-4
HOST_RANK_AVAILABLE_BYTES = 715827882
HEADER = struct.Struct("<QQQQdQ")
FACE = struct.Struct("<QQQQ")
POINT = struct.Struct("<QdddQQQdd")
SUPPORT_COUNT = struct.Struct("<Q")
# Oriented tetrahedron face order. Each face is then rotated, never reversed,
# so its smallest node id comes first; this is the established FaceOrder rule.
FACE_ORDER = ((1, 2, 3), (0, 3, 2), (0, 1, 3), (0, 2, 1))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def receipt(path: Path) -> dict[str, object]:
    path = Path(path)
    return {"bytes": path.stat().st_size, "sha256": sha256(path)}


def load_reader():
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location("inspect_native_failure3d_a289", READER)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load existing reader: {READER}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def input_receipts(capture: dict[str, object]) -> dict[str, dict[str, object]]:
    files = capture["files"]
    assert isinstance(files, dict) and len(files) == 30
    result: dict[str, dict[str, object]] = {}
    for raw_path, expected in files.items():
        path = Path(raw_path).resolve(strict=True)
        actual = receipt(path)
        assert isinstance(expected, dict)
        assert actual == {"bytes": expected["bytes"], "sha256": expected["sha256"]}, path
        result[str(path)] = actual
    return result


def binding_tuple(binding: dict[str, object]) -> tuple[object, ...]:
    required = ("id", "feature", "version", "dimension", "u", "v", "mobility_dimension")
    assert all(key in binding and binding[key] is not None for key in required)
    assert all(isinstance(binding[key], int) and not isinstance(binding[key], bool)
               for key in ("id", "feature", "version", "dimension", "mobility_dimension"))
    assert all(isinstance(binding[key], (int, float)) and math.isfinite(float(binding[key]))
               for key in ("u", "v"))
    return (binding["id"], binding["feature"], binding["version"], binding["dimension"],
            struct.pack("<d", float(binding["u"])), struct.pack("<d", float(binding["v"])),
            binding["mobility_dimension"])


def main() -> None:
    started = time.monotonic()
    assert not OUT.exists(), f"refusing to overwrite {OUT}"
    capture_path = RUN / "capture_ready.json"
    terminal_path = RUN / "terminal_handles.json"
    capture = json.loads(capture_path.read_text())
    terminal = json.loads(terminal_path.read_text())
    assert capture["raw_capture_complete"] and capture["owned_rows"] == OWNED_ROWS_EXPECTED
    assert capture["missing_owned_rows"] == 0
    assert terminal["all_owned_handles_terminal"]
    assert all(row["state"] == "absent" for row in terminal["owned_identities"])

    sealed_before = input_receipts(capture)
    assert len(sealed_before) == 30
    reference_hashes = {path: receipt for path, receipt in sealed_before.items()
                        if Path(path).name.endswith("_reference.jsonl")}
    assert len(reference_hashes) == 6 and len({r["sha256"] for r in reference_hashes.values()}) == 1

    # Preserve hashes for the authoritative reference, reader, helper, capture seal,
    # terminal receipt and the complete exit census in addition to all 30 sealed inputs.
    supplemental_paths = [NATIVE_REF, READER, Path(__file__).resolve(), capture_path,
                          terminal_path, RUN / "construction_progress_assessment.json", RUN / "run.cfg"]
    supplemental_before = {str(path.resolve(strict=True)): receipt(path) for path in supplemental_paths}
    with NATIVE_REF.open("r", encoding="ascii") as reference_stream:
        assert reference_stream.readline().strip() == "SU2_NATIVE_REFERENCE_3D 7"
        archive_header = [int(value) for value in reference_stream.readline().split()]
        marker_names = [json.loads(reference_stream.readline()) for _ in range(archive_header[0])]
    assert marker_names == ["FARFIELD", "SYMMETRY", "WING3D"]
    assert archive_header[1] == 48954 and archive_header[9] == 48954
    curves_count = archive_header[9]
    per_curve_cache_bytes = 1 << 16
    while per_curve_cache_bytes and curves_count * per_curve_cache_bytes > HOST_RANK_AVAILABLE_BYTES:
        per_curve_cache_bytes //= 2
    assert per_curve_cache_bytes == 8192
    ATLAS_QUOTA = curves_count * per_curve_cache_bytes
    assert ATLAS_QUOTA == 401031168

    cfg = (RUN / "run.cfg").read_text()
    def config_value(key: str) -> str:
        import re
        match = re.search(r"(?m)^\s*" + re.escape(key) + r"\s*=\s*(.*?)\s*$", cfg)
        assert match, f"missing {key} in source config"
        return match.group(1)
    assert float(config_value("ADAP_ANGLE")) == 60
    assert int(config_value("ADAP_COMPLEXITY")) == 50000
    assert int(config_value("ADAP_NATIVE_RANKS")) == 6
    assert int(config_value("ADAP_NATIVE_MEMORY_LIMIT")) == 12288

    reader = load_reader()
    manifests, missing = reader._load_manifests([str(RUN)])
    assert len(manifests) == 6 and not missing
    assert [m["rank"] for m in manifests] == list(range(6))
    assert all(m["ranks"] == 6 and m["complete_shard"] and m["owned_complete"] and
               m["metrics_complete"] and m["reference_complete"] and m["rejections_complete"]
               for m in manifests)
    assert sum(m["written_rows"] for m in manifests) == OWNED_ROWS_EXPECTED

    reference_facets: dict[int, dict[str, object]] = {}
    reference_path = manifests[0]["_resolved_files"]["reference"]
    for row in reader._jsonl(reference_path, manifests[0]["_caps"]["reference"], "reference"):
        key, clean = reader._validate_reference(row)
        if key[0] != "facet":
            continue
        facet_id = clean["id"]
        assert facet_id not in reference_facets
        reference_facets[facet_id] = clean
    assert reference_facets
    reference_facets_by_marker: dict[int, int] = {}
    reference_facets_by_entity: dict[tuple[int, int, int], int] = {}
    for facet in reference_facets.values():
        marker = facet["marker"]
        reference_facets_by_marker[marker] = reference_facets_by_marker.get(marker, 0) + 1
        entity = tuple(facet[key] for key in ("body", "sheet", "marker"))
        reference_facets_by_entity[entity] = reference_facets_by_entity.get(entity, 0) + 1

    face_records: dict[tuple[int, int, int], dict[str, object]] = {}
    all_node_fields: dict[int, tuple[bytes, ...]] = {}
    all_faces: set[tuple[int, int, int]] = set()
    faces_by_marker: dict[int, int] = {}
    faces_by_entity: dict[tuple[int, int, int], int] = {}
    cached_geometry_bad = 0
    cached_geometry_unknown = 0
    cached_qj_unknown = 0
    max_geometry_excess = 0.0
    scanned = 0
    physical_observations = 0

    for manifest in manifests:
        owned_path = manifest["_resolved_files"]["owned"]
        metric_path = manifest["_resolved_files"]["metrics"]
        metrics = reader._jsonl(metric_path, manifest["_caps"]["metrics"], "metrics")
        for cell_index, cell in enumerate(reader._read_owned(
                owned_path, manifest["written_rows"], manifest["_caps"]["owned"])):
            try:
                metric_row = next(metrics)
            except StopIteration as error:
                raise RuntimeError(f"metrics ended before owned row {cell_index} on rank {manifest['rank']}") from error
            metric = reader._validate_metric(metric_row, cell, metric_path)
            scanned += 1
            if not metric["cache_valid"] or not metric["geometry_measured"] or metric["geometry_excess_l2"] is None:
                cached_geometry_unknown += 1
            else:
                max_geometry_excess = max(max_geometry_excess, metric["geometry_excess_l2"])
            if metric["geometry_bad"]:
                cached_geometry_bad += 1
            if not metric["q_measured"] or not metric["j_measured"]:
                cached_qj_unknown += 1

            for slot, physical in enumerate(cell["physical"]):
                if not physical["present"]:
                    continue
                physical_observations += 1
                facet_id = physical["facet"]
                assert facet_id in reference_facets, (manifest["rank"], cell["id"], slot, facet_id)
                reference = reference_facets[facet_id]
                marker = reference["marker"]
                entity = tuple(reference[key] for key in ("body", "sheet", "marker"))
                indices = FACE_ORDER[slot]
                nodes = [cell["nodes"][index] for index in indices]
                bindings = [metric["bindings"][index] for index in indices]
                node_ids = [node["id"] for node in nodes]
                assert len(set(node_ids)) == 3
                face_key = tuple(sorted(node_ids))
                assert face_key not in all_faces, ("duplicate physical face", face_key)
                all_faces.add(face_key)
                faces_by_marker[marker] = faces_by_marker.get(marker, 0) + 1
                faces_by_entity[entity] = faces_by_entity.get(entity, 0) + 1

                rotated_by = node_ids.index(min(node_ids))
                nodes = nodes[rotated_by:] + nodes[:rotated_by]
                bindings = bindings[rotated_by:] + bindings[:rotated_by]
                point_records = []
                for node, binding in zip(nodes, bindings):
                    assert binding is not None, ("missing physical-vertex binding", node["id"], face_key)
                    assert binding["id"] == node["id"]
                    binding_fields = binding_tuple(binding)
                    xyz = tuple(float(value) for value in node["xyz"])
                    assert len(xyz) == 3 and all(math.isfinite(value) for value in xyz)
                    fields = (struct.pack("<d", value) for value in xyz)
                    shared = tuple(fields) + binding_fields
                    previous = all_node_fields.setdefault(node["id"], shared)
                    assert previous == shared, ("shared node coordinate/binding disagreement", node["id"])
                    point_records.append((node["id"], *xyz, binding["feature"], binding["version"],
                                          binding["dimension"], float(binding["u"]), float(binding["v"])))

                version_set = {binding["version"] for binding in bindings}
                assert len(version_set) == 1
                face_records[face_key] = {
                    "node_ids": tuple(node["id"] for node in nodes),
                    "facet_id": facet_id,
                    "marker": marker,
                    "points": tuple(point_records),
                    "geometry_version": next(iter(version_set)),
                }

        try:
            next(metrics)
        except StopIteration:
            pass
        else:
            raise RuntimeError(f"metrics contain extra rows after rank {manifest['rank']} owned data")

    assert scanned == OWNED_ROWS_EXPECTED
    assert len(all_faces) == PHYSICAL_FACES_EXPECTED
    assert len(face_records) == PHYSICAL_FACES_EXPECTED
    wing_faces = faces_by_entity.get(WING_ENTITY, 0)
    assert wing_faces == WING_FACES_EXPECTED
    assert reference_facets_by_entity.get(WING_ENTITY) == 37202
    assert cached_geometry_unknown == 0 and cached_geometry_bad == 0
    assert cached_qj_unknown == 0

    assessment = json.loads((RUN / "construction_progress_assessment.json").read_text())
    exit_rows = [event for event in assessment["events"]
                 if event.get("stage") == "exit" and event.get("round") == 25210]
    assert len(exit_rows) == 1
    exit_event = exit_rows[0]
    assert all(int(row["working_faces"]) == PHYSICAL_FACES_EXPECTED
               for row in exit_event["rank_rows"].values())
    prepared_limits = {int(row["prepared_limit"]) for row in exit_event["rank_rows"].values()}
    assert prepared_limits == {396836624}
    assert all(int(row[k]) == 0 for row in exit_event["rank_rows"].values()
               for k in ("q_bad", "j_bad", "geometry_bad", "size_bad"))
    assert all(row["cache_consistent"] == "1" and row["census_complete"] == "1" and
               row["shape_complete"] == "1" and row["geometry_complete"] == "1"
               for row in exit_event["rank_rows"].values())

    versions = {record["geometry_version"] for record in face_records.values()}
    assert len(versions) == 1, f"physical-face bindings have multiple geometry versions: {versions}"
    geometry_version = next(iter(versions))

    payload = bytearray(HEADER.pack(MAGIC, FORMAT_VERSION, geometry_version,
                                    PHYSICAL_FACES_EXPECTED, TOLERANCE, ATLAS_QUOTA))
    for face_key in sorted(face_records):
        record = face_records[face_key]
        payload.extend(FACE.pack(*record["node_ids"], record["facet_id"]))
        for point in record["points"]:
            payload.extend(POINT.pack(*point))
        payload.extend(SUPPORT_COUNT.pack(0))
    expected_bytes = HEADER.size + PHYSICAL_FACES_EXPECTED * (FACE.size + 3 * POINT.size + SUPPORT_COUNT.size)
    assert len(payload) == expected_bytes
    assert len(payload) <= FILE_LIMIT

    # Recheck all 30 sealed inputs and supplemental provenance immediately before output.
    sealed_after = input_receipts(capture)
    supplemental_after = {str(path.resolve(strict=True)): receipt(path) for path in supplemental_paths}
    assert sealed_before == sealed_after
    assert supplemental_before == supplemental_after

    OUT.mkdir()
    faces_path = OUT / "faces.bin"
    with faces_path.open("xb") as stream:
        stream.write(payload)
        stream.flush()
        os.fsync(stream.fileno())
    native_ref_out = OUT / "m6.su2.native_ref"
    native_ref_out.symlink_to(NATIVE_REF.resolve(strict=True))

    # Binary self-check: exact size/header/count and byte-for-byte record roundtrip.
    assert faces_path.stat().st_size == expected_bytes
    with faces_path.open("rb") as stream:
        header = stream.read(HEADER.size)
        assert HEADER.unpack(header) == (MAGIC, FORMAT_VERSION, geometry_version,
                                        PHYSICAL_FACES_EXPECTED, TOLERANCE, ATLAS_QUOTA)
        for face_key in sorted(face_records):
            record = face_records[face_key]
            expected_record = bytearray(FACE.pack(*record["node_ids"], record["facet_id"]))
            for point in record["points"]:
                expected_record.extend(POINT.pack(*point))
            expected_record.extend(SUPPORT_COUNT.pack(0))
            actual_record = stream.read(len(expected_record))
            assert actual_record == expected_record, ("binary record mismatch", face_key)
        assert not stream.read(1), "unexpected trailing bytes"

    outputs = {
        "faces.bin": receipt(faces_path),
        "m6.su2.native_ref": {**receipt(native_ref_out), "symlink_target": str(native_ref_out.resolve())},
    }
    after_outputs = input_receipts(capture)
    supplemental_after_outputs = {str(path.resolve(strict=True)): receipt(path) for path in supplemental_paths}
    assert sealed_before == after_outputs
    assert supplemental_before == supplemental_after_outputs

    result = {
        "schema": "native_current_geometry_query_preparation_v1",
        "scope": "All unique physical faces from the sealed current A289 snapshot; this prepares input only and is not a geometry certification or acceptance result.",
        "source_event": 1,
        "source_round": 25210,
        "source_exit_elapsed": exit_event["maximum_native_elapsed"],
        "archive_format_version": 7,
        "query_format": {
            "magic": MAGIC,
            "version": FORMAT_VERSION,
            "geometry_definition_version": geometry_version,
            "tolerance": TOLERANCE,
            "atlas_quota": ATLAS_QUOTA,
            "host_rank_available_bytes": HOST_RANK_AVAILABLE_BYTES,
            "prepared_limit_after_face_memo_bytes": next(iter(prepared_limits)),
            "face_memo_reservation_bytes": ATLAS_QUOTA - next(iter(prepared_limits)),
            "curves_count_from_native_reference_header": curves_count,
            "cache_bytes_per_curve": per_curve_cache_bytes,
            "native_config": {
                "ADAP_ANGLE": float(config_value("ADAP_ANGLE")),
                "ADAP_COMPLEXITY": int(config_value("ADAP_COMPLEXITY")),
                "ADAP_NATIVE_RANKS": int(config_value("ADAP_NATIVE_RANKS")),
                "ADAP_NATIVE_MEMORY_LIMIT": int(config_value("ADAP_NATIVE_MEMORY_LIMIT")),
            },
            "face_struct": FACE.format,
            "point_struct": POINT.format,
            "support_count_struct": SUPPORT_COUNT.format,
            "face_order": [list(order) for order in FACE_ORDER],
            "rotation": "rotate oriented FaceOrder so the minimum global node id is first; never reverse orientation",
            "support_count": 0,
        },
        "counts": {
            "sealed_input_files": len(sealed_before),
            "manifests": len(manifests),
            "owned_rows_scanned": scanned,
            "physical_face_observations": physical_observations,
            "unique_physical_faces": len(face_records),
            "wing_entity": list(WING_ENTITY),
            "wing_physical_faces": wing_faces,
            "faces_by_reference_marker": {str(key): value for key, value in sorted(faces_by_marker.items())},
            "faces_by_reference_entity": [
                {"entity": list(entity), "physical_faces": count,
                 "reference_facets": reference_facets_by_entity.get(entity, 0)}
                for entity, count in sorted(faces_by_entity.items())
            ],
            "unique_physical_vertices": len(all_node_fields),
        },
        "reference_source": {
            "shard_reference_files": len(reference_hashes),
            "identical_reference_sha256": next(iter(reference_hashes.values()))["sha256"],
            "marker_names_in_archive_order": marker_names,
            "reference_facets_by_marker": {str(key): value for key, value in sorted(reference_facets_by_marker.items())},
            "reference_facets_by_entity": [
                {"entity": list(entity), "reference_facets": count}
                for entity, count in sorted(reference_facets_by_entity.items())
            ],
        },
        "cached_geometry_diagnostic": {
            "all_geometry_values_known": cached_geometry_unknown == 0,
            "unknown_geometry_rows": cached_geometry_unknown,
            "geometry_bad_rows": cached_geometry_bad,
            "unknown_q_or_j_rows": cached_qj_unknown,
            "maximum_cached_geometry_excess_l2": max_geometry_excess,
            "interpretation": "Whole-field cached census only; it is not proof of native geometry certification or acceptance.",
        },
        "terminal_receipt": {
            "all_owned_handles_terminal": terminal["all_owned_handles_terminal"],
            "owned_identities_absent": all(row["state"] == "absent" for row in terminal["owned_identities"]),
            "sha256": sha256(terminal_path),
        },
        "failed_preparation_attempts": [{
            "attempt": 1,
            "failure": "Marker-id-only Wing assertion failed; marker 2 spans multiple (body,sheet,marker) entities.",
            "corrected_identity": list(WING_ENTITY),
            "sealed_inputs_verified_before_scan": True,
            "after_hash_verification_reached": False,
            "output_directory_created": False,
            "binary_payload_written": False,
        }],
        "execution_receipt": {
            "pid": os.getpid(),
            "nice": os.getpriority(os.PRIO_PROCESS, 0),
            "OMP_NUM_THREADS": os.environ.get("OMP_NUM_THREADS"),
            "OPENBLAS_NUM_THREADS": os.environ.get("OPENBLAS_NUM_THREADS"),
            "MKL_NUM_THREADS": os.environ.get("MKL_NUM_THREADS"),
            "seconds": time.monotonic() - started,
        },
        "input_hashes_before": {**sealed_before, **supplemental_before},
        "input_hashes_after": {**sealed_after, **supplemental_after_outputs},
        "outputs": outputs,
    }
    prep_path = OUT / "geometry_query_preparation.json"
    with prep_path.open("x", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())
    print(json.dumps({"output": str(OUT), "faces": len(face_records), "wing_faces": wing_faces,
                      "geometry_definition_version": geometry_version, "bytes": expected_bytes,
                      "cached_geometry_bad": cached_geometry_bad,
                      "cached_geometry_unknown": cached_geometry_unknown,
                      "seconds": result["execution_receipt"]["seconds"]}))


if __name__ == "__main__":
    main()
