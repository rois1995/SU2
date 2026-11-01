#!/usr/bin/env python3
"""Bounded offline inspection of rank-local native 3D failure shards."""

from __future__ import annotations

import argparse
import glob
import json
import math
import os
import struct
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Iterator


SCHEMA = "native_failure3d_v1"
OWNED_FORMAT = 0x4E334443454C4C02
OWNED_RECORD_BYTES = 381
FILE_LIMIT = 128 << 20
MAX_CORE_CELLS = 32768
MAX_CAVITY_CELLS = 262144
MAX_TOTAL_ROWS = 2_200_000
MAX_GLOBAL_NODES = 1_000_000
MAX_FULL_VTU_BYTES = 512 << 20
MAX_PHYSICAL_FACES = 2_000_000
MAX_REFERENCE_RECORDS = 1_000_000
MAX_REJECTION_RECORDS = 100_000
U64 = struct.Struct("<Q")
F64 = struct.Struct("<d")


class InspectionError(ValueError):
    """Invalid or unsafe input shard."""


def _integer(value: Any, name: str, minimum: int = 0) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum:
        raise InspectionError(f"{name} must be an integer >= {minimum}")
    return value


def _finite(value: Any, name: str, *, nullable: bool = False) -> float | None:
    if value is None and nullable:
        return None
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise InspectionError(f"{name} must be numeric" + (" or null" if nullable else ""))
    result = float(value)
    if not math.isfinite(result):
        raise InspectionError(f"{name} must be finite")
    return result


def _boolean(value: Any, name: str) -> bool:
    if not isinstance(value, bool):
        raise InspectionError(f"{name} must be boolean")
    return value


def _jsonl(path: Path, cap: int, label: str) -> Iterator[dict[str, Any]]:
    if not path.is_file():
        raise InspectionError(f"missing {label} file: {path}")
    if path.stat().st_size > cap:
        raise InspectionError(f"{label} file exceeds cap: {path}")
    with path.open("r", encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            if not line.strip():
                continue
            try:
                value = json.loads(line)
            except (json.JSONDecodeError, UnicodeDecodeError) as error:
                raise InspectionError(f"invalid {label} JSONL at {path}:{line_number}") from error
            if not isinstance(value, dict):
                raise InspectionError(f"{label} row must be an object at {path}:{line_number}")
            yield value


def _safe_file(directory: Path, name: Any, cap: int, label: str) -> Path:
    if not isinstance(name, str) or not name or Path(name).name != name or name in (".", ".."):
        raise InspectionError(f"unsafe {label} basename")
    path = directory / name
    if not path.is_file():
        raise InspectionError(f"missing {label} file: {path}")
    if path.stat().st_size > cap:
        raise InspectionError(f"{label} file exceeds cap: {path}")
    return path


def _load_manifests(inputs: list[str]) -> tuple[list[dict[str, Any]], list[str]]:
    paths: set[Path] = set()
    for raw in inputs:
        expanded = glob.glob(raw)
        candidates = [Path(item) for item in expanded] if expanded else [Path(raw)]
        for candidate in candidates:
            if candidate.is_dir():
                paths.update(candidate.glob("*_manifest.json"))
            elif candidate.is_file():
                paths.add(candidate)
    if not paths:
        raise InspectionError("no manifest files found")

    manifests = []
    for path in sorted(paths):
        try:
            manifest = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            raise InspectionError(f"cannot read manifest {path}") from error
        if not isinstance(manifest, dict) or manifest.get("schema") != SCHEMA:
            raise InspectionError(f"unsupported manifest schema: {path}")
        if manifest.get("byte_order") != "little":
            raise InspectionError(f"unsupported byte_order in {path}")
        if manifest.get("owned_record_bytes") != OWNED_RECORD_BYTES:
            raise InspectionError(f"unsupported owned_record_bytes in {path}")
        rank = _integer(manifest.get("rank"), "rank")
        ranks = _integer(manifest.get("ranks"), "ranks", 1)
        if rank >= ranks:
            raise InspectionError(f"rank {rank} is outside ranks={ranks} in {path}")
        if not isinstance(manifest.get("event"), (int, str)) or isinstance(manifest.get("event"), bool):
            raise InspectionError(f"invalid event in {path}")
        _boolean(manifest.get("complete_shard"), "complete_shard")
        for flag in ("owned_complete", "metrics_complete", "reference_complete"):
            _boolean(manifest.get(flag), flag)
        _integer(manifest.get("working_rows"), "working_rows")
        written = _integer(manifest.get("written_rows"), "written_rows")
        if written > manifest["working_rows"]:
            raise InspectionError(f"written_rows exceeds working_rows in {path}")
        declared_caps = manifest.get("cap_bytes")
        if isinstance(declared_caps, dict):
            if set(declared_caps) != {"owned", "metrics", "reference", "rejections"}:
                raise InspectionError(f"cap_bytes must cover all four files in {path}")
            caps = {key: _integer(value, f"cap_bytes.{key}", 1) for key, value in declared_caps.items()}
            if any(value > FILE_LIMIT for value in caps.values()):
                raise InspectionError(f"declared cap_bytes exceeds {FILE_LIMIT} in {path}")
        else:
            cap = _integer(declared_caps, "cap_bytes", 1)
            if cap > FILE_LIMIT:
                raise InspectionError(f"declared cap_bytes exceeds {FILE_LIMIT} in {path}")
            caps = {key: cap for key in ("owned", "metrics", "reference", "rejections")}
        if not isinstance(manifest.get("files"), dict):
            raise InspectionError(f"files must be an object in {path}")
        files = {
            label: _safe_file(path.parent, manifest["files"].get(label), caps[label], label)
            for label in ("owned", "metrics", "reference", "rejections")
        }
        if len(set(files.values())) != len(files):
            raise InspectionError(f"manifest aliases input files in {path}")
        manifest["_path"] = path
        manifest["_resolved_files"] = files
        manifest["_caps"] = caps
        manifests.append(manifest)

    ranks = {m["ranks"] for m in manifests}
    events = {m["event"] for m in manifests}
    if len(ranks) != 1 or len(events) != 1:
        raise InspectionError("manifests disagree on event or rank count")
    rank_count = next(iter(ranks))
    by_rank: dict[int, dict[str, Any]] = {}
    for manifest in manifests:
        rank = manifest["rank"]
        if rank in by_rank:
            raise InspectionError(f"duplicate manifest for rank {rank}")
        by_rank[rank] = manifest
    missing = sorted(set(range(rank_count)) - set(by_rank))
    marker_names = [m.get("marker_names") for m in manifests]
    present_names = [x for x in marker_names if x is not None]
    if present_names and any(x != present_names[0] for x in present_names[1:]):
        raise InspectionError("manifests disagree on marker_names")
    if present_names and any(x is None for x in marker_names):
        raise InspectionError("marker_names missing from some rank manifests")
    if present_names and not isinstance(present_names[0], (dict, list)):
        raise InspectionError("marker_names must be a list or object")
    return [by_rank[r] for r in sorted(by_rank)], [str(x) for x in missing]


def _read_owned(path: Path, expected: int, cap: int) -> Iterator[dict[str, Any]]:
    size = path.stat().st_size
    if size > cap:
        raise InspectionError(f"owned file exceeds cap: {path}")
    expected_size = 8 + expected * OWNED_RECORD_BYTES
    if size != expected_size:
        raise InspectionError(f"owned file byte count mismatch: {path}")
    with path.open("rb") as stream:
        count_bytes = stream.read(8)
        if len(count_bytes) != 8 or U64.unpack(count_bytes)[0] != expected:
            raise InspectionError(f"owned row count mismatch: {path}")
        for row_index in range(expected):
            record = stream.read(OWNED_RECORD_BYTES)
            if len(record) != OWNED_RECORD_BYTES:
                raise InspectionError(f"truncated owned record {row_index} in {path}")
            offset = 0

            def u64() -> int:
                nonlocal offset
                value = U64.unpack_from(record, offset)[0]
                offset += 8
                return value

            def f64() -> float:
                nonlocal offset
                value = F64.unpack_from(record, offset)[0]
                offset += 8
                if not math.isfinite(value):
                    raise InspectionError(f"nonfinite owned value in {path}, row {row_index}")
                return value

            if u64() != OWNED_FORMAT:
                raise InspectionError(f"unsupported owned record format in {path}, row {row_index}")
            cell_id = u64()
            nodes = []
            for _ in range(4):
                nodes.append({"id": u64(), "xyz": [f64(), f64(), f64()]})
            if len({node["id"] for node in nodes}) != 4:
                raise InspectionError(f"duplicate node id in owned cell {cell_id} in {path}")
            version = u64()
            physical = []
            for _ in range(4):
                present = record[offset]
                offset += 1
                if present not in (0, 1):
                    raise InspectionError(f"invalid physical bool in {path}, row {row_index}")
                physical.append({"present": bool(present), "facet": u64()})
            sensor_present = record[offset]
            offset += 1
            if sensor_present not in (0, 1):
                raise InspectionError(f"invalid sensor bool in {path}, row {row_index}")
            sensor = [f64() for _ in range(24)]
            if offset != OWNED_RECORD_BYTES:
                raise InspectionError(f"internal record size mismatch in {path}")
            yield {"id": cell_id, "version": version, "nodes": nodes, "physical": physical,
                   "sensor_present": bool(sensor_present), "sensor": sensor}


def _validate_metric(row: dict[str, Any], cell: dict[str, Any], path: Path) -> dict[str, Any]:
    cell_id = _integer(row.get("id"), "metric id")
    version = _integer(row.get("version"), "metric version")
    if cell_id != cell["id"] or version != cell["version"]:
        raise InspectionError(f"metric id/version mismatch in {path}: {cell_id}/{version} != {cell['id']}/{cell['version']}")
    cache_valid = _boolean(row.get("cache_valid"), "cache_valid")
    for name in ("q_bad", "j_bad", "geometry_bad", "size_bad", "height_bad", "height_unresolved",
                 "surface", "near_feature", "feature_known"):
        _boolean(row.get(name), name)
    q = _finite(row.get("q"), "q", nullable=True)
    jac = _finite(row.get("J"), "J", nullable=True)
    geometry_excess = _finite(row.get("geometry_excess_l2"), "geometry_excess_l2", nullable=True)
    if geometry_excess is not None and geometry_excess < 0:
        raise InspectionError("geometry_excess_l2 must be nonnegative")
    edges = row.get("edges")
    if not isinstance(edges, list) or len(edges) != 6:
        raise InspectionError("edges must contain six metric lengths")
    cell_nodes = {n["id"] for n in cell["nodes"]}
    seen_edges = set()
    normalized_edges = []
    for edge in edges:
        if not isinstance(edge, list) or len(edge) != 3:
            raise InspectionError("each edge must be [a,b,length]")
        a, b = _integer(edge[0], "edge a"), _integer(edge[1], "edge b")
        length = _finite(edge[2], "edge length")
        key = tuple(sorted((a, b)))
        if a == b or a not in cell_nodes or b not in cell_nodes or key in seen_edges or length < 0:
            raise InspectionError("invalid or duplicate cell edge")
        seen_edges.add(key)
        normalized_edges.append([a, b, length])
    repairs = row.get("repairs")
    if not isinstance(repairs, list):
        raise InspectionError("repairs must be an array")
    for repair in repairs:
        if not isinstance(repair, dict) or not isinstance(repair.get("vertices"), list):
            raise InspectionError("invalid repair record")
        if any(_integer(v, "repair vertex") not in cell_nodes for v in repair["vertices"]):
            raise InspectionError("repair vertex is not in cell")
        _boolean(repair.get("blocked"), "repair blocked")
        _integer(repair.get("epoch"), "repair epoch")
    bindings = row.get("bindings")
    if not isinstance(bindings, list) or len(bindings) != 4:
        raise InspectionError("bindings must have four entries")
    for binding in bindings:
        if binding is None:
            continue
        if not isinstance(binding, dict):
            raise InspectionError("invalid surface binding")
        for name in ("id", "feature", "version", "dimension", "mobility_dimension"):
            if name in binding:
                _integer(binding[name], f"binding {name}")
        for name in ("u", "v"):
            if name in binding:
                _finite(binding[name], f"binding {name}")
    result = dict(row)
    result.update(q=q, J=jac, geometry_excess_l2=geometry_excess, edges=normalized_edges,
                  is_bad=bool(row["q_bad"] or row["j_bad"] or row["geometry_bad"] or row["size_bad"] or
                              row["height_bad"] or row["height_unresolved"]))
    return result


def _validate_reference(row: dict[str, Any]) -> tuple[str, Any]:
    kind = row.get("kind")
    if kind == "facet":
        facet_id = _integer(row.get("id"), "facet id")
        marker = _integer(row.get("marker"), "facet marker")
        nodes = row.get("nodes")
        if not isinstance(nodes, list) or len(nodes) != 3:
            raise InspectionError("facet nodes must contain three coordinates")
        clean_nodes = []
        for node in nodes:
            if not isinstance(node, list) or len(node) != 4:
                raise InspectionError("reference node must be [id,x,y,z]")
            clean_nodes.append([_integer(node[0], "reference node id")] +
                               [_finite(v, "reference coordinate") for v in node[1:]])
        mapping_error = _finite(row.get("mapping_error", 0), "mapping_error")
        if mapping_error < 0:
            raise InspectionError("mapping_error must be nonnegative")
        controls = row.get("controls")
        if controls is not None:
            if not isinstance(controls, list) or len(controls) != 10:
                raise InspectionError("controls must contain ten points")
            controls = [[_finite(v, "control coordinate") for v in p] for p in controls
                        if isinstance(p, list) and len(p) == 3]
            if len(controls) != 10:
                raise InspectionError("each control must have three coordinates")
        for key in ("body", "sheet"):
            if key in row and row[key] is not None:
                _integer(row[key], f"facet {key}")
        for key in ("analytic", "affine"):
            if key in row and row[key] is not None and not isinstance(row[key], dict):
                raise InspectionError(f"facet {key} must be an object or null")
        clean = dict(row, id=facet_id, marker=marker, nodes=clean_nodes, mapping_error=mapping_error, controls=controls)
        return (kind, facet_id), clean
    if kind == "feature":
        edge = row.get("edge")
        if not isinstance(edge, list) or len(edge) != 2:
            raise InspectionError("feature edge must have two node ids")
        clean = dict(row, edge=[_integer(v, "feature edge node") for v in edge],
                     feature=_integer(row.get("feature"), "feature id"))
        if clean["edge"][0] == clean["edge"][1]:
            raise InspectionError("feature edge repeats a node")
        return (kind, tuple(clean["edge"])), clean
    if kind == "corner":
        clean = dict(row, id=_integer(row.get("id"), "corner id"))
        return (kind, clean["id"]), clean
    raise InspectionError(f"unknown reference record kind: {kind!r}")


def _pair_rows(manifest: dict[str, Any], cell_ids: set[int], node_coords: dict[int, tuple[float, float, float]],
               core: dict[int, dict[str, Any]], counts: dict[str, int]) -> None:
    files = manifest["_resolved_files"]
    rank = manifest["rank"]
    cap = manifest["_caps"]
    written = manifest["written_rows"]
    owned_iter = _read_owned(files["owned"], written, cap["owned"])
    metrics_iter = _jsonl(files["metrics"], cap["metrics"], "metrics")
    metric_count = 0
    for cell in owned_iter:
        try:
            metric_row = next(metrics_iter)
        except StopIteration as error:
            raise InspectionError(f"metrics file has fewer rows than owned shard: {files['metrics']}") from error
        metric_count += 1
        metric = _validate_metric(metric_row, cell, files["metrics"])
        if cell["id"] in cell_ids:
            raise InspectionError(f"duplicate cell owner/id {cell['id']}")
        cell_ids.add(cell["id"])
        counts["input_rows"] += 1
        if counts["input_rows"] > MAX_TOTAL_ROWS:
            raise InspectionError("input row count exceeds internal safety cap")
        for node in cell["nodes"]:
            xyz = tuple(node["xyz"])
            previous = node_coords.setdefault(node["id"], xyz)
            if previous != xyz:
                raise InspectionError(f"conflicting coordinates for global node {node['id']}")
        if len(node_coords) > MAX_GLOBAL_NODES:
            raise InspectionError("unique node count exceeds internal safety cap")
        if not metric["cache_valid"] or metric["q"] is None or metric["J"] is None:
            counts["unknown_metric_rows"] += 1
        if metric["is_bad"]:
            counts["bad_rows_total"] += 1
            if len(core) < MAX_CORE_CELLS:
                core[cell["id"]] = {"cell": cell, "metric": metric, "rank": rank}
    try:
        next(metrics_iter)
    except StopIteration:
        pass
    else:
        raise InspectionError(f"metrics file has more rows than owned shard: {files['metrics']}")
    if metric_count != written:
        raise InspectionError(f"metric row count mismatch: {files['metrics']}")


def _collect_reference(manifests: list[dict[str, Any]]) -> dict[tuple[str, Any], dict[str, Any]]:
    result: dict[tuple[str, Any], dict[str, Any]] = {}
    total = 0
    for manifest in manifests:
        for row in _jsonl(manifest["_resolved_files"]["reference"], manifest["_caps"]["reference"], "reference"):
            total += 1
            if total > MAX_REFERENCE_RECORDS:
                raise InspectionError("reference record count exceeds internal safety cap")
            key, clean = _validate_reference(row)
            previous = result.get(key)
            if previous is not None and previous != clean:
                raise InspectionError(f"conflicting replicated reference record {key}")
            result[key] = clean
    return result


def _collect_rejections(manifests: list[dict[str, Any]]) -> list[dict[str, Any]]:
    result = []
    for manifest in manifests:
        for row in _jsonl(manifest["_resolved_files"]["rejections"], manifest["_caps"]["rejections"], "rejections"):
            if len(result) >= MAX_REJECTION_RECORDS:
                raise InspectionError("rejection record count exceeds internal safety cap")
            feature = row.get("feature")
            if not isinstance(feature, list) or not feature or any(_integer(x, "rejection feature") < 0 for x in feature):
                raise InspectionError("invalid rejection feature")
            old_cells = row.get("old_cells")
            if not isinstance(old_cells, list) or any(_integer(x, "old cell id") < 0 for x in old_cells):
                raise InspectionError("invalid rejection old_cells")
            _boolean(row.get("old_cells_truncated"), "old_cells_truncated")
            _boolean(row.get("shape_measured"), "shape_measured")
            _finite(row.get("q"), "rejection q", nullable=True)
            _finite(row.get("J"), "rejection J", nullable=True)
            _integer(row.get("failure"), "rejection failure")
            if not isinstance(row.get("reason"), str):
                raise InspectionError("rejection reason must be text")
            result.append(dict(row, rank=manifest["rank"], historical=True))
    return result


def _xml_array(parent: ET.Element, name: str, kind: str, values: list[Any], components: int = 1) -> None:
    array = ET.SubElement(parent, "DataArray", type=kind, Name=name, format="ascii")
    if components != 1:
        array.set("NumberOfComponents", str(components))
    array.text = " ".join(str(x) for x in values)


def _write_vtu(path: Path, cells: list[dict[str, Any]], *, reference: bool = False) -> None:
    points: list[tuple[int, tuple[float, float, float]]] = []
    point_index: dict[int, int] = {}
    connectivity: list[int] = []
    offsets: list[int] = []
    cell_types: list[int] = []
    for item in cells:
        nodes = item["nodes"]
        for node in nodes:
            node_id, xyz = (node[0], tuple(node[1:])) if reference else (node["id"], tuple(node["xyz"]))
            if node_id not in point_index:
                point_index[node_id] = len(points)
                points.append((node_id, xyz))
            elif points[point_index[node_id]][1] != xyz:
                raise InspectionError(f"conflicting output coordinates for node {node_id}")
            connectivity.append(point_index[node_id])
        offsets.append(len(connectivity))
        cell_types.append(5 if reference else 10)  # VTK_TRIANGLE / VTK_TETRA
    vtk = ET.Element("VTKFile", type="UnstructuredGrid", version="0.1", byte_order="LittleEndian")
    grid = ET.SubElement(vtk, "UnstructuredGrid")
    piece = ET.SubElement(grid, "Piece", NumberOfPoints=str(len(points)), NumberOfCells=str(len(cells)))
    field = ET.SubElement(piece, "FieldData")
    _xml_array(field, "visualization_only", "UInt8", [1])
    _xml_array(field, "global_node_ids", "UInt64", [p[0] for p in points])
    _xml_array(field, "global_cell_ids", "UInt64",
               [c["id"] if reference else c["cell"]["id"] for c in cells])
    point_data = ET.SubElement(piece, "PointData")
    _xml_array(point_data, "global_node_id", "UInt64", [p[0] for p in points])
    cell_data = ET.SubElement(piece, "CellData")
    if reference:
        for name, kind, getter in (
            ("global_facet_id", "UInt64", lambda c: c["id"]),
            ("marker", "UInt64", lambda c: c["marker"]),
            ("mapping_error", "Float64", lambda c: c["mapping_error"]),
        ):
            _xml_array(cell_data, name, kind, [getter(c) for c in cells])
        for name in ("body", "sheet"):
            if all(name in c for c in cells):
                _xml_array(cell_data, name, "UInt64", [c[name] for c in cells])
    else:
        _xml_array(cell_data, "global_cell_id", "UInt64", [c["cell"]["id"] for c in cells])
        _xml_array(cell_data, "is_bad_core", "UInt8", [int(c.get("is_bad_core", False)) for c in cells])
        for name, getter in (
            ("q", lambda x: x["metric"]["q"]), ("J", lambda x: x["metric"]["J"]),
            ("geometry_excess_l2", lambda x: x["metric"]["geometry_excess_l2"]),
        ):
            _xml_array(cell_data, name, "Float64", ["nan" if getter(c) is None else getter(c) for c in cells])
        for name in ("q_bad", "j_bad", "geometry_bad", "size_bad", "height_bad", "height_unresolved",
                     "surface", "near_feature", "feature_known", "cache_valid"):
            _xml_array(cell_data, name, "UInt8", [int(c["metric"][name]) for c in cells])
        for index in range(6):
            _xml_array(cell_data, f"edge_{index}_length", "Float64", [c["metric"]["edges"][index][2] for c in cells])
    point_section = ET.SubElement(piece, "Points")
    _xml_array(point_section, "coordinates", "Float64", [x for _, xyz in points for x in xyz], 3)
    cell_section = ET.SubElement(piece, "Cells")
    _xml_array(cell_section, "connectivity", "Int64", connectivity)
    _xml_array(cell_section, "offsets", "Int64", offsets)
    _xml_array(cell_section, "types", "UInt8", cell_types)
    ET.ElementTree(vtk).write(path, encoding="utf-8", xml_declaration=True)


def _write_lines(path: Path, rows: list[dict[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as stream:
        for row in rows:
            stream.write(json.dumps(row, separators=(",", ":"), allow_nan=False) + "\n")


class _CappedText:
    def __init__(self, path: Path, limit: int):
        self.stream = path.open("w", encoding="utf-8")
        self.limit = limit
        self.written = 0

    def write(self, text: str) -> None:
        size = len(text.encode("utf-8"))
        if self.written + size > self.limit:
            raise InspectionError(f"full mesh VTU exceeds output cap {self.limit}")
        self.stream.write(text)
        self.written += size

    def close(self) -> None:
        self.stream.close()


def _stream_data_array(stream: _CappedText, name: str, vtk_type: str,
                       values: Iterator[Any], components: int = 1) -> None:
    extra = f' NumberOfComponents="{components}"' if components != 1 else ""
    stream.write(f'<DataArray type="{vtk_type}" Name="{name}"{extra} format="ascii">')
    first = True
    for value in values:
        stream.write(("" if first else " ") + str(value))
        first = False
    stream.write("</DataArray>")


def _write_full_vtu(path: Path, manifests: list[dict[str, Any]],
                    node_coords: dict[int, tuple[float, float, float]]) -> int:
    node_ids = sorted(node_coords)
    point_index = {node_id: index for index, node_id in enumerate(node_ids)}
    total_cells = sum(m["written_rows"] for m in manifests)
    stream = _CappedText(path, MAX_FULL_VTU_BYTES)
    try:
        stream.write('<?xml version="1.0"?>\n<VTKFile type="UnstructuredGrid" version="0.1" byte_order="LittleEndian"><UnstructuredGrid>')
        stream.write(f'<Piece NumberOfPoints="{len(node_ids)}" NumberOfCells="{total_cells}"><FieldData>')
        _stream_data_array(stream, "visualization_only", "UInt8", iter([1]))
        _stream_data_array(stream, "global_node_ids", "UInt64", iter(node_ids))
        stream.write('</FieldData><PointData>')
        _stream_data_array(stream, "global_node_id", "UInt64", iter(node_ids))
        stream.write('</PointData><CellData>')

        def cell_values(name: str) -> Iterator[Any]:
            for manifest in manifests:
                metrics = iter(_jsonl(manifest["_resolved_files"]["metrics"],
                                      manifest["_caps"]["metrics"], "metrics"))
                for cell in _read_owned(manifest["_resolved_files"]["owned"],
                                        manifest["written_rows"], manifest["_caps"]["owned"]):
                    metric = _validate_metric(next(metrics), cell, manifest["_resolved_files"]["metrics"])
                    if name == "global_cell_id":
                        yield cell["id"]
                    elif name == "q":
                        yield "nan" if metric["q"] is None else metric["q"]
                    elif name == "J":
                        yield "nan" if metric["J"] is None else metric["J"]
                    elif name == "geometry_excess_l2":
                        yield "nan" if metric["geometry_excess_l2"] is None else metric["geometry_excess_l2"]
                    elif name == "geometry_bad":
                        yield int(metric["geometry_bad"])

        for name, kind in (("global_cell_id", "UInt64"), ("q", "Float64"), ("J", "Float64"),
                           ("geometry_excess_l2", "Float64"), ("geometry_bad", "UInt8")):
            _stream_data_array(stream, name, kind, cell_values(name))
        stream.write('</CellData><Points>')
        _stream_data_array(stream, "coordinates", "Float64",
                           (value for node_id in node_ids for value in node_coords[node_id]), 3)
        stream.write('</Points><Cells>')

        def connectivity() -> Iterator[int]:
            for manifest in manifests:
                for cell in _read_owned(manifest["_resolved_files"]["owned"],
                                        manifest["written_rows"], manifest["_caps"]["owned"]):
                    for node in cell["nodes"]:
                        yield point_index[node["id"]]

        _stream_data_array(stream, "connectivity", "Int64", connectivity())
        _stream_data_array(stream, "offsets", "Int64", (4 * (i + 1) for i in range(total_cells)))
        _stream_data_array(stream, "types", "UInt8", (10 for _ in range(total_cells)))
        stream.write('</Cells></Piece></UnstructuredGrid></VTKFile>')
    except Exception:
        stream.close()
        path.unlink(missing_ok=True)
        raise
    stream.close()
    return total_cells


def _marker_name(marker: int, names: Any) -> tuple[str, bool]:
    value = None
    if isinstance(names, list) and marker < len(names):
        value = names[marker]
    elif isinstance(names, dict):
        value = names.get(str(marker), names.get(marker))
    if isinstance(value, dict):
        value = value.get("name")
    if isinstance(value, str) and value:
        if "\n" in value or "\r" in value:
            raise InspectionError(f"marker {marker} name contains a line break")
        return value, False
    return f"Marker_{marker}", True


def _determinant(nodes: list[dict[str, Any]]) -> float:
    p0, p1, p2, p3 = [n["xyz"] for n in nodes]
    a = [p1[i] - p0[i] for i in range(3)]
    b = [p2[i] - p0[i] for i in range(3)]
    c = [p3[i] - p0[i] for i in range(3)]
    return (a[0] * (b[1] * c[2] - b[2] * c[1])
            - a[1] * (b[0] * c[2] - b[2] * c[0])
            + a[2] * (b[0] * c[1] - b[1] * c[0]))


def _write_full_su2(path: Path, manifests: list[dict[str, Any]],
                    node_coords: dict[int, tuple[float, float, float]],
                    references: dict[tuple[str, Any], dict[str, Any]]) -> tuple[int, list[str], dict[str, int]]:
    node_ids = sorted(node_coords)
    point_index = {node_id: index for index, node_id in enumerate(node_ids)}
    facet_reference = {row["id"]: row for (kind, _), row in references.items() if kind == "facet"}
    marker_names = manifests[0].get("marker_names")
    marker_counts: dict[int, int] = {}
    seen_faces: set[tuple[int, int, int]] = set()
    marker_name_rows: dict[int, tuple[str, bool]] = {}
    with tempfile.TemporaryDirectory(prefix="native_failure3d_su2_", dir=str(path.parent)) as temporary:
        spool_dir = Path(temporary)
        boundary_spools: dict[int, Any] = {}
        volume_count = 0
        orientation_counts = {"positive": 0, "negative": 0, "degenerate": 0,
                              "boundary_orientation_unknown": 0}
        standard_faces = ((1, 2, 3), (0, 3, 2), (0, 1, 3), (0, 2, 1))
        for manifest in manifests:
            for cell in _read_owned(manifest["_resolved_files"]["owned"],
                                    manifest["written_rows"], manifest["_caps"]["owned"]):
                volume_count += 1
                determinant = _determinant(cell["nodes"])
                if not math.isfinite(determinant):
                    raise InspectionError(f"nonfinite signed orientation for cell {cell['id']}")
                if determinant > 0:
                    orientation_counts["positive"] += 1
                    order_sign = 1
                elif determinant < 0:
                    orientation_counts["negative"] += 1
                    order_sign = -1
                else:
                    orientation_counts["degenerate"] += 1
                    orientation_counts["boundary_orientation_unknown"] += sum(
                        int(x["present"]) for x in cell["physical"])
                    order_sign = 1  # deterministic opposite-vertex ordering; orientation is unknown
                for slot, physical in enumerate(cell["physical"]):
                    if not physical["present"]:
                        continue
                    facet_id = physical["facet"]
                    reference = facet_reference.get(facet_id)
                    if reference is None:
                        raise InspectionError(f"physical facet {facet_id} has no reference triangle")
                    face = [cell["nodes"][i] for i in standard_faces[slot]]
                    if order_sign < 0:
                        face[1], face[2] = face[2], face[1]
                    face_ids = tuple(n["id"] for n in face)
                    key = tuple(sorted(face_ids))
                    if key in seen_faces:
                        raise InspectionError(f"duplicate physical boundary face with nodes {key}")
                    seen_faces.add(key)
                    if len(seen_faces) > MAX_PHYSICAL_FACES:
                        raise InspectionError("physical boundary face count exceeds internal cap")
                    marker = reference["marker"]
                    if marker not in marker_name_rows:
                        marker_name_rows[marker] = _marker_name(marker, marker_names)
                    if marker not in boundary_spools:
                        boundary_spools[marker] = (spool_dir / f"marker_{marker}.txt").open("w", encoding="utf-8")
                    mapped = [point_index[nid] for nid in face_ids]
                    boundary_spools[marker].write("5 " + " ".join(map(str, mapped)) + "\n")
                    marker_counts[marker] = marker_counts.get(marker, 0) + 1
        for spool in boundary_spools.values():
            spool.close()

        with path.open("w", encoding="utf-8") as stream:
            stream.write("% Unapproved rejected diagnostic mesh; not an accepted restart.\n")
            stream.write("NDIME= 3\n")
            stream.write(f"NELEM= {volume_count}\n")
            for manifest in manifests:
                for cell in _read_owned(manifest["_resolved_files"]["owned"],
                                        manifest["written_rows"], manifest["_caps"]["owned"]):
                    conn = [point_index[n["id"]] for n in cell["nodes"]]
                    stream.write("10 " + " ".join(map(str, conn)) + "\n")
            stream.write(f"NPOIN= {len(node_ids)}\n")
            for point_id, node_id in enumerate(node_ids):
                xyz = node_coords[node_id]
                stream.write(" ".join(format(v, ".17g") for v in xyz) + f" {point_id}\n")
            stream.write(f"NMARK= {len(marker_counts)}\n")
            for marker in sorted(marker_counts):
                name, _ = marker_name_rows[marker]
                stream.write(f"MARKER_TAG= {name}\nMARKER_ELEMS= {marker_counts[marker]}\n")
                with (spool_dir / f"marker_{marker}.txt").open("r", encoding="utf-8") as faces:
                    for line in faces:
                        stream.write(line)
    fallbacks = [f"{marker}:{marker_name_rows[marker][0]}" for marker in sorted(marker_counts)
                 if marker_name_rows[marker][1]]
    return volume_count, fallbacks, orientation_counts


def _write_cell_id_map(path: Path, manifests: list[dict[str, Any]]) -> None:
    with path.open("w", encoding="utf-8") as stream:
        index = 0
        for manifest in manifests:
            for cell in _read_owned(manifest["_resolved_files"]["owned"],
                                    manifest["written_rows"], manifest["_caps"]["owned"]):
                stream.write(json.dumps({"su2_element_index": index, "global_cell_id": cell["id"]},
                                        separators=(",", ":")) + "\n")
                index += 1


def _write_node_id_map(path: Path, node_ids: list[int]) -> None:
    with path.open("w", encoding="utf-8") as stream:
        for index, node_id in enumerate(node_ids):
            stream.write(json.dumps({"su2_index": index, "global_node_id": node_id},
                                    separators=(",", ":")) + "\n")


def inspect(inputs: list[str], output: str) -> dict[str, Any]:
    manifests, missing_ranks = _load_manifests(inputs)
    output_path = Path(output)
    if output_path.exists():
        if not output_path.is_dir() or any(output_path.iterdir()):
            raise InspectionError(f"output directory already exists and is not empty: {output_path}")
    else:
        output_path.mkdir(parents=True)

    cell_ids: set[int] = set()
    node_coords: dict[int, tuple[float, float, float]] = {}
    core: dict[int, dict[str, Any]] = {}
    counts = {"input_rows": 0, "bad_rows_total": 0, "unknown_metric_rows": 0}
    partial_reasons = []
    for manifest in manifests:
        if not manifest["complete_shard"] or manifest["written_rows"] != manifest["working_rows"]:
            partial_reasons.append(f"rank {manifest['rank']} shard is explicitly partial")
        if not manifest["owned_complete"]:
            partial_reasons.append(f"rank {manifest['rank']} owned source is incomplete")
        if not manifest["metrics_complete"]:
            partial_reasons.append(f"rank {manifest['rank']} metrics source is incomplete")
        if not manifest["reference_complete"]:
            partial_reasons.append(f"rank {manifest['rank']} reference source is incomplete")
        _pair_rows(manifest, cell_ids, node_coords, core, counts)
    if missing_ranks:
        partial_reasons.append("missing ranks: " + ",".join(missing_ranks))
    if counts["unknown_metric_rows"]:
        partial_reasons.append(f"{counts['unknown_metric_rows']} cells have unknown metric cache")
    omitted_cores = max(0, counts["bad_rows_total"] - len(core))
    if omitted_cores:
        partial_reasons.append(f"core cap omitted {omitted_cores} bad cells")

    # Keep bounded core seeds, then make a second pass for their complete local/cross-rank vertex stars.
    core_ids = set(core)
    core_nodes = {n["id"] for row in core.values() for n in row["cell"]["nodes"]}
    cavities = dict(core)
    omitted_cavity = 0
    for manifest in manifests:
        path = manifest["_resolved_files"]["owned"]
        metrics_path = manifest["_resolved_files"]["metrics"]
        metrics = iter(_jsonl(metrics_path, manifest["_caps"]["metrics"], "metrics"))
        for cell in _read_owned(path, manifest["written_rows"], manifest["_caps"]["owned"]):
            metric = _validate_metric(next(metrics), cell, metrics_path)
            if cell["id"] in core_ids or not any(n["id"] in core_nodes for n in cell["nodes"]):
                continue
            if len(cavities) < MAX_CAVITY_CELLS:
                cavities[cell["id"]] = {"cell": cell, "metric": metric, "rank": manifest["rank"]}
            else:
                omitted_cavity += 1
    if omitted_cavity:
        partial_reasons.append(f"cavity cap omitted {omitted_cavity} vertex-star cells")

    # Cross-check all cell coordinates against the first-pass identity map before writing selected rows.
    for item in cavities.values():
        for node in item["cell"]["nodes"]:
            if node_coords.get(node["id"]) != tuple(node["xyz"]):
                raise InspectionError(f"selected cell coordinate identity changed for node {node['id']}")
        item["nodes"] = item["cell"]["nodes"]
        item["is_bad_core"] = item["cell"]["id"] in core_ids
    bad_rows = [cavities[cell_id] for cell_id in sorted(core_ids)]
    cavity_rows = [cavities[cell_id] for cell_id in sorted(cavities)]
    reference = _collect_reference(manifests)
    facets = [r for (kind, _), r in sorted(reference.items(), key=lambda x: (x[0][0], x[0][1])) if kind == "facet"]
    feature_rows = [r for (kind, _), r in sorted(reference.items(), key=lambda x: (x[0][0], x[0][1])) if kind != "facet"]
    rejections = _collect_rejections(manifests)

    _write_vtu(output_path / "bad_cells.vtu", bad_rows)
    _write_vtu(output_path / "cavities.vtu", cavity_rows)
    _write_vtu(output_path / "reference.vtu", facets, reference=True)
    _write_lines(output_path / "reference_records.jsonl", [*facets, *feature_rows])
    _write_lines(output_path / "rejections_historical.jsonl", rejections)
    source_complete = not missing_ranks and all(
        m["complete_shard"] and m["owned_complete"] and m["written_rows"] == m["working_rows"]
        for m in manifests)
    full_mesh_exported_cells = 0
    full_mesh_complete = False
    full_mesh_error = None
    try:
        full_mesh_exported_cells = _write_full_vtu(output_path / "rejected_mesh.vtu", manifests, node_coords)
        full_mesh_complete = source_complete and full_mesh_exported_cells == counts["input_rows"]
        if not full_mesh_complete:
            partial_reasons.append("rejected_mesh.vtu contains only the available owned source rows")
    except InspectionError as error:
        if "full mesh VTU exceeds output cap" not in str(error):
            raise
        full_mesh_error = str(error)
        partial_reasons.append(full_mesh_error)
    full_su2_cells = 0
    full_su2_complete = False
    full_su2_error = None
    marker_name_fallbacks: list[str] = []
    orientation_counts: dict[str, int] = {"positive": 0, "negative": 0, "degenerate": 0,
                                           "boundary_orientation_unknown": 0}
    reference_complete = all(m["reference_complete"] for m in manifests)
    if source_complete and reference_complete:
        try:
            full_su2_cells, marker_name_fallbacks, orientation_counts = _write_full_su2(
                output_path / "rejected_mesh.su2", manifests, node_coords, reference)
            if (output_path / "rejected_mesh.su2").stat().st_size > MAX_FULL_VTU_BYTES:
                (output_path / "rejected_mesh.su2").unlink()
                raise InspectionError(f"full mesh SU2 exceeds output cap {MAX_FULL_VTU_BYTES}")
            full_su2_complete = full_su2_cells == counts["input_rows"]
            if full_su2_complete:
                _write_node_id_map(output_path / "rejected_mesh_nodes.jsonl", sorted(node_coords))
                _write_cell_id_map(output_path / "rejected_mesh_cells.jsonl", manifests)
        except InspectionError as error:
            full_su2_error = str(error)
            partial_reasons.append(full_su2_error)
            (output_path / "rejected_mesh.su2").unlink(missing_ok=True)
    elif not reference_complete:
        full_su2_error = "reference source is incomplete; SU2 boundary export omitted"
        partial_reasons.append(full_su2_error)
    inspect_manifest = {
        "schema": "native_failure3d_inspection_v1",
        "event": manifests[0]["event"], "expected_ranks": manifests[0]["ranks"],
        "present_ranks": [m["rank"] for m in manifests], "missing_ranks": [int(x) for x in missing_ranks],
        "input_rows": counts["input_rows"], "bad_rows_total": counts["bad_rows_total"],
        "bad_cells_retained": len(bad_rows), "bad_cells_omitted": omitted_cores,
        "cavity_cells_retained": len(cavity_rows), "cavity_cells_omitted": omitted_cavity,
        "unknown_metric_rows": counts["unknown_metric_rows"],
        "actual_full_mesh_points": len(node_coords), "actual_full_mesh_cells": counts["input_rows"],
        "full_mesh_exported_cells": full_mesh_exported_cells,
        "final_mesh_accepted": False, "final_mesh_complete": full_mesh_complete,
        "full_su2_exported_cells": full_su2_cells,
        "final_su2_complete": full_su2_complete, "full_su2_error": full_su2_error,
        "marker_name_fallbacks": marker_name_fallbacks,
        "full_su2_signed_orientation_counts": orientation_counts,
        "full_mesh_measurements_complete": all(m["metrics_complete"] for m in manifests)
                                                and counts["unknown_metric_rows"] == 0,
        "full_mesh_error": full_mesh_error,
        "reference_facets": len(facets), "reference_features_corners": len(feature_rows),
        "historical_rejections": len(rejections), "rejection_scope": "historical_not_final_state_reason",
        "star_definition": "cells sharing any global node id with a retained bad core cell",
        "partial": bool(partial_reasons), "partial_reasons": partial_reasons,
        "visualization_note": "VTU is a bounded visualization export; exact reference controls and metadata are in reference_records.jsonl.",
        "inputs": [str(m["_path"]) for m in manifests],
    }
    path = output_path / "inspect_manifest.json"
    path.write_text(json.dumps(inspect_manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return inspect_manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", help="manifest paths, glob patterns, or directories")
    parser.add_argument("--output", required=True, help="new or empty output directory")
    args = parser.parse_args(argv)
    try:
        result = inspect(args.inputs, args.output)
    except (InspectionError, OSError, struct.error, StopIteration) as error:
        print(f"inspect_native_failure3d: {error}", file=sys.stderr)
        return 2
    print(json.dumps({k: result[k] for k in ("input_rows", "bad_cells_retained", "cavity_cells_retained", "partial")}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
