import importlib.util
import itertools
import json
import math
import struct
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


SCRIPT = Path(__file__).with_name("inspect_native_failure3d.py")
SPEC = importlib.util.spec_from_file_location("inspect_native_failure3d", SCRIPT)
INSPECT = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(INSPECT)


def _owned_record(cell_id, version, nodes, physical=None):
    data = bytearray()
    data += struct.pack("<QQ", INSPECT.OWNED_FORMAT, cell_id)
    for node_id, xyz in nodes:
        data += struct.pack("<Q3d", node_id, *xyz)
    data += struct.pack("<Q", version)
    for present, facet_id in (physical or [(False, 0)] * 4):
        data += struct.pack("<BQ", int(present), facet_id)
    data += struct.pack("<B", 0)
    data += struct.pack("<24d", *([0.0] * 24))
    assert len(data) == INSPECT.OWNED_RECORD_BYTES
    return data


def _metric(cell_id, version, nodes, *, bad=False, valid=True):
    edges = [[a, b, 1.0] for (a, _), (b, _) in itertools.combinations(nodes, 2)]
    return {
        "id": cell_id, "version": version, "cache_valid": valid,
        "q": 0.1 if valid else None, "J": 0.01 if valid else None,
        "q_bad": bad, "j_bad": False, "geometry_bad": False, "size_bad": False,
        "height_bad": False, "height_unresolved": False, "geometry_excess_l2": 0.0 if valid else None,
        "surface": False, "near_feature": False, "feature_known": True,
        "edges": edges, "repairs": [], "bindings": [None] * 4,
    }


def _reference():
    return {"kind": "facet", "id": 50, "marker": 2,
            "nodes": [[1, 0.0, 0.0, 0.0], [2, 1.0, 0.0, 0.0], [3, 0.0, 1.0, 0.0]],
            "body": 3, "sheet": 4, "mapping_error": 1e-12,
            "controls": [[0.0, 0.0, 0.0]] * 10, "analytic": None, "affine": None}


def _write_shard(directory, rank, ranks, cells, *, event=7, written_rows=None, working_rows=None,
                 complete=True, reference=None, rejections=None, physical_slots=None, marker_names=None):
    prefix = f"event{event}_rank{rank}"
    owned_name, metrics_name = prefix + ".owned", prefix + ".metrics.jsonl"
    reference_name, rejections_name = prefix + ".reference.jsonl", prefix + ".rejections.jsonl"
    with (directory / owned_name).open("wb") as stream:
        stream.write(struct.pack("<Q", len(cells)))
        for cell_id, version, nodes, metric in cells:
            stream.write(_owned_record(cell_id, version, nodes, (physical_slots or {}).get(cell_id)))
    with (directory / metrics_name).open("w", encoding="utf-8") as stream:
        for cell_id, version, nodes, metric in cells:
            stream.write(json.dumps(metric) + "\n")
    refs = [_reference()] if reference is None else reference
    (directory / reference_name).write_text("".join(json.dumps(row) + "\n" for row in refs), encoding="utf-8")
    rejects = rejections or []
    (directory / rejections_name).write_text("".join(json.dumps(row) + "\n" for row in rejects), encoding="utf-8")
    manifest = {
        "schema": INSPECT.SCHEMA, "rank": rank, "ranks": ranks, "event": event,
        "complete_shard": complete,
        "owned_complete": complete, "metrics_complete": complete,
        "reference_complete": complete,
        **({"marker_names": marker_names} if marker_names is not None else {}),
        "working_rows": len(cells) if working_rows is None else working_rows,
        "written_rows": len(cells) if written_rows is None else written_rows,
        "owned_record_bytes": INSPECT.OWNED_RECORD_BYTES, "byte_order": "little",
        "files": {"owned": owned_name, "metrics": metrics_name, "reference": reference_name,
                  "rejections": rejections_name},
        "progress": {"q_bad_cells": sum(m["q_bad"] for *_, m in cells)},
        "policies": {"q_min": 0.2}, "cap_bytes": INSPECT.FILE_LIMIT,
    }
    path = directory / f"event{event}_rank{rank}_manifest.json"
    path.write_text(json.dumps(manifest), encoding="utf-8")
    return path


def _fixture(directory):
    xyz = {
        1: (0.0, 0.0, 0.0), 2: (1.0, 0.0, 0.0), 3: (0.0, 1.0, 0.0),
        4: (0.0, 0.0, 1.0), 5: (0.0, 0.0, -1.0), 6: (-1.0, 0.0, 0.0),
        7: (0.0, -1.0, 0.0),
    }
    nodes_a = [(i, xyz[i]) for i in (1, 2, 3, 4)]
    nodes_b = [(i, xyz[i]) for i in (1, 2, 3, 5)]
    bad = (10, 1, nodes_a, _metric(10, 1, nodes_a, bad=True))
    neighbor = (20, 1, nodes_b, _metric(20, 1, nodes_b))
    nodes_far = [(8, (10.0, 0.0, 0.0)), (9, (11.0, 0.0, 0.0)),
                 (10, (10.0, 1.0, 0.0)), (11, (10.0, 0.0, 1.0))]
    distant = (30, 1, nodes_far, _metric(30, 1, nodes_far))
    reject = {"feature": [1, 2], "old_cells": [10], "old_cells_truncated": False,
              "reason": "rejected trial", "shape_measured": True, "q": 0.1, "J": 0.01, "failure": 2}
    manifests = [
        _write_shard(directory, 0, 3, [bad], rejections=[reject]),
        _write_shard(directory, 1, 3, [neighbor]),
        _write_shard(directory, 2, 3, [distant]),
    ]
    return manifests


class InspectNativeFailureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir="/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/build-integrated-v2/compile-tmp")
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_cross_rank_vertex_star_and_empty_rank(self):
        manifests = _fixture(self.root)
        result = INSPECT.inspect([str(self.root)], str(self.root / "out"))
        self.assertFalse(result["partial"])
        self.assertEqual(result["present_ranks"], [0, 1, 2])
        self.assertEqual(result["bad_cells_retained"], 1)
        self.assertEqual(result["cavity_cells_retained"], 2)
        self.assertEqual(result["actual_full_mesh_cells"], 3)
        self.assertEqual(result["actual_full_mesh_points"], 9)
        self.assertTrue(result["final_mesh_complete"])
        self.assertFalse(result["final_mesh_accepted"])
        self.assertTrue(result["final_su2_complete"])
        su2_header = (self.root / "out/rejected_mesh.su2").read_text().splitlines()
        self.assertEqual(su2_header[:3], ["% Unapproved rejected diagnostic mesh; not an accepted restart.",
                                          "NDIME= 3", "NELEM= 3"])
        self.assertIn("NPOIN= 9", su2_header)
        self.assertEqual(result["historical_rejections"], 1)
        self.assertTrue(json.loads((self.root / "out/rejections_historical.jsonl").read_text().splitlines()[0])["historical"])
        for filename, expected in (("bad_cells.vtu", 1), ("cavities.vtu", 2)):
            doc = ET.parse(self.root / "out" / filename).getroot()
            piece = doc.find("./UnstructuredGrid/Piece")
            self.assertEqual(int(piece.attrib["NumberOfCells"]), expected)
            ids = piece.findall("./CellData/DataArray[@Name='global_cell_id']")[0].text.split()
            self.assertEqual([int(x) for x in ids], [10] if expected == 1 else [10, 20])
        reference = ET.parse(self.root / "out/reference.vtu").getroot()
        self.assertEqual(reference.find("./UnstructuredGrid/Piece").attrib["NumberOfCells"], "1")
        full = ET.parse(self.root / "out/rejected_mesh.vtu").getroot()
        full_piece = full.find("./UnstructuredGrid/Piece")
        self.assertEqual(full_piece.attrib["NumberOfCells"], "3")
        full_ids = [int(x) for x in full_piece.find("./CellData/DataArray[@Name='global_cell_id']").text.split()]
        self.assertEqual(full_ids, [10, 20, 30])
        full_node_ids = [int(x) for x in full_piece.find("./PointData/DataArray[@Name='global_node_id']").text.split()]
        full_conn = [int(x) for x in full_piece.find("./Cells/DataArray[@Name='connectivity']").text.split()]
        self.assertEqual([full_node_ids[i] for i in full_conn[:8]], [1, 2, 3, 4, 1, 2, 3, 5])
        self.assertEqual(set(full_ids) - {10, 20}, {30})
        records = (self.root / "out/reference_records.jsonl").read_text()
        self.assertIn('"controls"', records)
        self.assertEqual(len(manifests), 3)

    def test_unknown_metric_is_explicit_nan_and_partial(self):
        xyz = {i: (float(i), 0.0, 0.0) for i in range(1, 5)}
        nodes = list(xyz.items())
        unknown = _metric(5, 2, nodes, valid=False)
        unknown["geometry_bad"] = True
        row = (5, 2, nodes, unknown)
        _write_shard(self.root, 0, 1, [row])
        result = INSPECT.inspect([str(self.root)], str(self.root / "out"))
        self.assertTrue(result["partial"])
        self.assertEqual(result["unknown_metric_rows"], 1)
        self.assertTrue(result["final_mesh_complete"])
        self.assertFalse(result["full_mesh_measurements_complete"])
        text = (self.root / "out/bad_cells.vtu").read_text()
        self.assertIn("nan", text)

    def test_explicit_partial_and_missing_rank_never_claim_complete(self):
        xyz = {i: (float(i), 0.0, 0.0) for i in range(1, 5)}
        nodes = list(xyz.items())
        _write_shard(self.root, 0, 2, [(1, 1, nodes, _metric(1, 1, nodes))], complete=False,
                     working_rows=2)
        result = INSPECT.inspect([str(self.root)], str(self.root / "out"))
        self.assertTrue(result["partial"])
        self.assertIn(1, result["missing_ranks"])
        self.assertEqual(result["bad_cells_omitted"], 0)
        self.assertFalse(result["final_mesh_complete"])

    def test_rejects_bad_schema_endian_count_and_path_traversal(self):
        for mutation in ("schema", "endian", "count", "path"):
            with self.subTest(mutation=mutation):
                root = self.root / mutation
                root.mkdir()
                manifest = _write_shard(root, 0, 1, [])
                value = json.loads(manifest.read_text())
                if mutation == "schema":
                    value["schema"] = "other"
                elif mutation == "endian":
                    value["byte_order"] = "big"
                elif mutation == "count":
                    value["written_rows"] = 1
                else:
                    value["files"]["owned"] = "../escape"
                manifest.write_text(json.dumps(value))
                with self.assertRaises(INSPECT.InspectionError):
                    INSPECT.inspect([str(manifest)], str(root / "out"))

    def test_rejects_duplicate_owner_coordinate_and_metric_identity(self):
        xyz = {i: (float(i), 0.0, 0.0) for i in range(1, 5)}
        nodes = list(xyz.items())
        duplicate = (1, 1, nodes, _metric(1, 1, nodes))
        _write_shard(self.root, 0, 2, [duplicate])
        _write_shard(self.root, 1, 2, [duplicate])
        with self.assertRaisesRegex(INSPECT.InspectionError, "duplicate cell"):
            INSPECT.inspect([str(self.root)], str(self.root / "out"))

        root = self.root / "coords"
        root.mkdir()
        first = (1, 1, nodes, _metric(1, 1, nodes))
        changed = [(n, (p[0] + (1 if n == 1 else 0), *p[1:])) for n, p in nodes]
        second = (2, 1, changed, _metric(2, 1, changed))
        _write_shard(root, 0, 2, [first])
        _write_shard(root, 1, 2, [second])
        with self.assertRaisesRegex(INSPECT.InspectionError, "conflicting coordinates"):
            INSPECT.inspect([str(root)], str(root / "out"))

        root = self.root / "identity"
        root.mkdir()
        manifest = _write_shard(root, 0, 1, [first])
        metric_file = root / "event7_rank0.metrics.jsonl"
        metric = json.loads(metric_file.read_text())
        metric["version"] = 9
        metric_file.write_text(json.dumps(metric) + "\n")
        with self.assertRaisesRegex(INSPECT.InspectionError, "metric id/version mismatch"):
            INSPECT.inspect([str(manifest)], str(root / "out"))

    def test_core_and_cavity_caps_are_reported_as_partial(self):
        original_core, original_cavity = INSPECT.MAX_CORE_CELLS, INSPECT.MAX_CAVITY_CELLS
        INSPECT.MAX_CORE_CELLS = 1
        INSPECT.MAX_CAVITY_CELLS = 1
        try:
            xyz = {i: (float(i), 0.0, 0.0) for i in range(1, 9)}
            cells = []
            for cell_id, ns in ((1, (1, 2, 3, 4)), (2, (1, 5, 6, 7)), (3, (8, 2, 3, 4))):
                nodes = [(i, xyz[i]) for i in ns]
                cells.append((cell_id, 1, nodes, _metric(cell_id, 1, nodes, bad=True)))
            _write_shard(self.root, 0, 1, cells)
            result = INSPECT.inspect([str(self.root)], str(self.root / "out"))
            self.assertTrue(result["partial"])
            self.assertEqual(result["bad_rows_total"], 3)
            self.assertEqual(result["bad_cells_retained"], 1)
            self.assertEqual(result["bad_cells_omitted"], 2)
            self.assertGreater(result["cavity_cells_omitted"], 0)
        finally:
            INSPECT.MAX_CORE_CELLS, INSPECT.MAX_CAVITY_CELLS = original_core, original_cavity

    def test_output_is_never_overwritten(self):
        _fixture(self.root)
        output = self.root / "out"
        output.mkdir()
        (output / "keep").write_text("x")
        with self.assertRaisesRegex(INSPECT.InspectionError, "already exists"):
            INSPECT.inspect([str(self.root)], str(output))
        self.assertEqual((output / "keep").read_text(), "x")

    def test_su2_contains_full_tetrahedra_and_physical_reference_marker(self):
        nodes = [(1, (0.0, 0.0, 0.0)), (2, (1.0, 0.0, 0.0)),
                 (3, (0.0, 1.0, 0.0)), (4, (0.0, 0.0, 1.0))]
        cell = (42, 3, nodes, _metric(42, 3, nodes))
        facet = {"kind": "facet", "id": 50, "marker": 2,
                 "nodes": [[102, 10.0, 0.0, 0.0], [103, 0.0, 10.0, 0.0], [104, 0.0, 0.0, 10.0]],
                 "body": 0, "sheet": 0, "mapping_error": 0.0}
        _write_shard(self.root, 0, 1, [cell], reference=[facet], marker_names=["zero", "one", "wall"],
                     physical_slots={42: [(True, 50), (False, 0), (False, 0), (False, 0)]})
        result = INSPECT.inspect([str(self.root)], str(self.root / "out"))
        self.assertTrue(result["final_su2_complete"])
        self.assertEqual(result["marker_name_fallbacks"], [])
        lines = (self.root / "out/rejected_mesh.su2").read_text().splitlines()
        self.assertIn("NELEM= 1", lines)
        self.assertIn("NPOIN= 4", lines)
        self.assertIn("NMARK= 1", lines)
        self.assertIn("MARKER_TAG= wall", lines)
        marker_pos = lines.index("MARKER_ELEMS= 1")
        self.assertEqual(lines[marker_pos + 1], "5 1 2 3")
        node_map = [json.loads(line) for line in (self.root / "out/rejected_mesh_nodes.jsonl").read_text().splitlines()]
        self.assertEqual(node_map, [{"su2_index": 0, "global_node_id": 1},
                                    {"su2_index": 1, "global_node_id": 2},
                                    {"su2_index": 2, "global_node_id": 3},
                                    {"su2_index": 3, "global_node_id": 4}])
        cell_map = [json.loads(line) for line in (self.root / "out/rejected_mesh_cells.jsonl").read_text().splitlines()]
        self.assertEqual(cell_map, [{"su2_element_index": 0, "global_cell_id": 42}])

    def test_su2_keeps_negative_and_degenerate_tets_and_reports_orientation(self):
        negative_nodes = [(1, (0.0, 0.0, 0.0)), (2, (0.0, 1.0, 0.0)),
                          (3, (1.0, 0.0, 0.0)), (4, (0.0, 0.0, 1.0))]
        degenerate_nodes = [(11, (0.0, 0.0, 0.0)), (12, (1.0, 0.0, 0.0)),
                            (13, (2.0, 0.0, 0.0)), (14, (3.0, 0.0, 0.0))]
        negative = (60, 1, negative_nodes, _metric(60, 1, negative_nodes, bad=True))
        degenerate = (61, 1, degenerate_nodes, _metric(61, 1, degenerate_nodes, bad=True))
        refs = [
            {"kind": "facet", "id": 70, "marker": 0,
             "nodes": [[102, 0, 0, 0], [103, 0, 0, 0], [104, 0, 0, 0]], "mapping_error": 0},
            {"kind": "facet", "id": 71, "marker": 0,
             "nodes": [[112, 0, 0, 0], [113, 0, 0, 0], [114, 0, 0, 0]], "mapping_error": 0},
        ]
        _write_shard(self.root, 0, 1, [negative, degenerate], reference=refs,
                     physical_slots={60: [(True, 70), (False, 0), (False, 0), (False, 0)],
                                     61: [(True, 71), (False, 0), (False, 0), (False, 0)]})
        result = INSPECT.inspect([str(self.root)], str(self.root / "out"))
        self.assertTrue(result["final_su2_complete"])
        self.assertEqual(result["full_su2_signed_orientation_counts"],
                         {"positive": 0, "negative": 1, "degenerate": 1,
                          "boundary_orientation_unknown": 1})
        lines = (self.root / "out/rejected_mesh.su2").read_text().splitlines()
        marker_lines = lines[lines.index("MARKER_ELEMS= 2") + 1:]
        self.assertEqual(marker_lines, ["5 1 3 2", "5 5 6 7"])


if __name__ == "__main__":
    unittest.main()
