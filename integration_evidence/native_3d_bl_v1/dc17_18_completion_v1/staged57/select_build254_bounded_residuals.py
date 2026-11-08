#!/usr/bin/env python3
"""Select <=8 complete actual build254 residual contexts; no serialization."""
from __future__ import annotations

import contextlib
import hashlib
import importlib.util
import json
import math
import struct
import time
from pathlib import Path

import meshio
import numpy as np

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build254_conditioned_mach_only_real_cfd50k"
OUT = RUN / "bounded_residual_selection.json"
SEED_SU2 = ROOT / "m6_inputs/euler_original_force_seed_v1/m6.su2"
FACES = ((1, 2, 3), (0, 3, 2), (0, 1, 3), (0, 2, 1))
U64 = struct.Struct("<Q")
RECORD = 416
MAGIC = 0x4E334443454C4C02

spec = importlib.util.spec_from_file_location("inspect_native_failure3d", ROOT / "inspect_native_failure3d.py")
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def source_marker_names(path: Path) -> list[str]:
    lines = path.read_text().splitlines()
    return [line.split("=", 1)[1].strip() for line in lines if line.startswith("MARKER_TAG")]


def main() -> None:
    started = time.monotonic()
    assert not OUT.exists(), f"refusing to overwrite {OUT}"
    receipt = json.loads((RUN / "inspection_reader_status.json").read_text())
    ready = json.loads((RUN / "capture_ready.json").read_text())
    manifest_json = json.loads((RUN / "inspection/inspect_manifest.json").read_text())
    assert receipt["all_reader_checks_passed"] and receipt["exit_code"] == 0
    assert manifest_json["bad_set_complete"] and manifest_json["cavity_complete"]
    manifests, missing = reader._load_manifests([str(RUN / "adapted_mesh_inspection_rank*_manifest.json")])
    assert not missing and len(manifests) == 6 and sum(m["written_rows"] for m in manifests) == receipt["actual_cells"]
    assert all(m["complete_shard"] and m["owned_complete"] and m["metrics_complete"] for m in manifests)

    paths = [RUN / "inspection" / n for n in ("rejected_mesh.vtu", "rejected_mesh.su2", "bad_cells.vtu", "cavities.vtu", "reference.vtu", "reference_records.jsonl", "inspect_manifest.json")]
    paths += [RUN / n for n in ("inspection_reader_status.json", "capture_ready.json")]
    paths += [m["_path"] for m in manifests] + [m["_resolved_files"]["owned"] for m in manifests]
    paths += [SEED_SU2, ROOT / "inspect_native_failure3d.py", Path(__file__)]
    before = {str(p.resolve()): sha(p) for p in paths}
    expected = {str(Path(p).resolve()): h for p, h in receipt["input_hashes_after"].items()}
    for p in paths[:7]:
        if str(p.resolve()) in expected:
            assert before[str(p.resolve())] == expected[str(p.resolve())]
    for manifest in manifests:
        owned = manifest["_resolved_files"]["owned"]
        rec = ready["files"][str(owned.resolve())]
        assert before[str(owned.resolve())] == rec["sha256"] and owned.stat().st_size == rec["bytes"]

    full = meshio.read(paths[0])
    su2 = meshio.read(paths[1])
    bad = meshio.read(paths[2])
    ref = meshio.read(paths[4])
    cells = full.cells_dict["tetra"]
    ids = full.cell_data_dict["global_cell_id"]["tetra"].astype(np.uint64)
    node_ids = full.point_data["global_node_id"].astype(np.uint64)
    bdata = {k: v["tetra"] for k, v in bad.cell_data_dict.items()}
    fdata = {k: v["tetra"] for k, v in full.cell_data_dict.items()}
    assert len(cells) == receipt["actual_cells"] and len(full.points) == receipt["actual_points"]
    assert len(bad.cells_dict["tetra"]) == receipt["bad_cells"] == 260
    assert int(np.count_nonzero(fdata["q"] < .2)) == receipt["measured_bad_counts"]["q_bad"]
    assert int(np.count_nonzero(fdata["J"] < .05)) == receipt["measured_bad_counts"]["j_bad"]
    assert int(np.count_nonzero(fdata["geometry_bad"])) == receipt["measured_bad_counts"]["geometry_bad"]
    assert np.array_equal(full.points, su2.points)
    assert np.array_equal(cells, su2.cells_dict["tetra"])

    marker_names = source_marker_names(SEED_SU2)
    assert marker_names == ["FARFIELD", "SYMMETRY", "WING3D"]
    ref_ids = ref.cell_data_dict["global_facet_id"]["triangle"].astype(np.uint64)
    ref_marker = ref.cell_data_dict["marker"]["triangle"].astype(int)
    ref_point_ids = ref.point_data["global_node_id"].astype(np.uint64)
    ref_tris = ref.cells_dict["triangle"]
    facet = {}
    wing_vertices = set()
    for fi, tri in enumerate(ref_tris):
        fid, mark = int(ref_ids[fi]), int(ref_marker[fi])
        nids = tuple(int(ref_point_ids[p]) for p in tri)
        assert fid not in facet
        facet[fid] = {"marker_id": mark, "marker_name": marker_names[mark], "node_ids": nids,
                      "mapping_error": float(ref.cell_data_dict["mapping_error"]["triangle"][fi])}
    assert len(facet) == manifest_json["reference_facets"]

    # Rank shards are written in the same global row order as rejected_mesh.vtu.
    rank_ends = np.cumsum([m["written_rows"] for m in manifests])
    rank_starts = np.r_[0, rank_ends[:-1]]
    by_id = {int(cid): i for i, cid in enumerate(ids)}
    assert len(by_id) == len(ids)
    bad_ids = bdata["global_cell_id"].astype(np.uint64)
    bad_idx = np.array([by_id[int(cid)] for cid in bad_ids], dtype=np.int64)
    assert np.array_equal(ids[bad_idx], bad_ids)
    assert int(np.count_nonzero(bdata["q_bad"])) + int(np.count_nonzero(bdata["j_bad"])) >= int(np.count_nonzero((bdata["q_bad"] != 0) | (bdata["j_bad"] != 0)))

    # Vertex incidence for complete four-star contexts.
    incidence = np.argsort(cells.ravel(), kind="stable")
    offsets = np.r_[0, np.cumsum(np.bincount(cells.ravel(), minlength=len(full.points)))]
    vertex_ids = node_ids
    coords_by_id = {int(n): full.points[i] for i, n in enumerate(node_ids)}

    def stars(index: int) -> list[np.ndarray]:
        return [incidence[offsets[v]:offsets[v + 1]] // 4 for v in cells[index]]

    # Read every current owned row sequentially to build adapted-volume physical
    # faces and current WING3D vertices. Reference node IDs are CAD IDs and must
    # not be confused with the adapted volume's global node IDs.
    owned_streams = [m["_resolved_files"]["owned"].open("rb") for m in manifests]

    def read_owned(index: int) -> tuple[dict, int]:
        rank = int(np.searchsorted(rank_ends, index, side="right"))
        local = int(index - rank_starts[rank])
        stream = owned_streams[rank]
        stream.seek(8 + RECORD * local)
        raw = stream.read(RECORD)
        assert len(raw) == RECORD and U64.unpack_from(raw, 0)[0] == MAGIC
        cid = U64.unpack_from(raw, 8)[0]
        nodes = []
        for k in range(4):
            off = 16 + 32 * k
            nodes.append((U64.unpack_from(raw, off)[0], struct.unpack_from("<ddd", raw, off + 8)))
        phys = []
        for slot in range(4):
            off = 152 + 16 * slot
            present, fid = struct.unpack_from("<QQ", raw, off)
            assert present in (0, 1)
            phys.append((bool(present), fid))
        assert cid == int(ids[index])
        assert [n[0] for n in nodes] == [int(node_ids[v]) for v in cells[index]]
        for k, (_, xyz) in enumerate(nodes):
            assert np.array_equal(np.asarray(xyz), full.points[cells[index, k]])
        physical = []
        for slot, (present, fid) in enumerate(phys):
            if not present:
                continue
            assert fid in facet, f"facet {fid} missing reference"
            face_ids = tuple(nodes[k][0] for k in FACES[slot])
            # The facet ID denotes a CAD/reference triangle association. Its three
            # reference node IDs are not the output mesh's volume node IDs.
            physical.append({"reference_facet_id": int(fid), "marker_id": facet[fid]["marker_id"],
                             "marker_name": facet[fid]["marker_name"], "mapping_error": facet[fid]["mapping_error"],
                             "volume_face_node_ids": list(face_ids),
                             "reference_facet_node_ids": list(facet[fid]["node_ids"])})
        return {"cell_id": int(cid), "nodes": [n[0] for n in nodes], "physical_faces": physical}, rank

    owned_boundary_faces = {name: set() for name in marker_names}
    for rank, manifest in enumerate(manifests):
        stream = owned_streams[rank]
        stream.seek(0)
        count = U64.unpack(stream.read(8))[0]
        assert count == manifest["written_rows"]
        global_start, global_end = int(rank_starts[rank]), int(rank_ends[rank])
        for index in range(global_start, global_end):
            raw = stream.read(RECORD)
            assert len(raw) == RECORD and U64.unpack_from(raw, 0)[0] == MAGIC
            cid = U64.unpack_from(raw, 8)[0]
            assert cid == int(ids[index]), f"manifest rank order mismatch at row {index}"
            node_row = [U64.unpack_from(raw, 16 + 32 * k)[0] for k in range(4)]
            assert node_row == [int(node_ids[v]) for v in cells[index]], f"owned/full VTU connectivity mismatch at {cid}"
            for slot in range(4):
                present, fid = struct.unpack_from("<QQ", raw, 152 + 16 * slot)
                assert present in (0, 1)
                if not present:
                    continue
                assert fid in facet
                marker_name = facet[fid]["marker_name"]
                face = tuple(node_row[k] for k in FACES[slot])
                key = tuple(sorted(face))
                assert key not in owned_boundary_faces[marker_name], f"duplicate owned physical face {key}"
                owned_boundary_faces[marker_name].add(key)
                if marker_name == "WING3D":
                    wing_vertices.update(face)
        assert stream.read(1) == b""
    # Reconcile current output SU2 boundary triangles by tag and node IDs with
    # those physical owned faces; source marker order supplies the tag names.
    su2_triangles = su2.cells_dict["triangle"]
    su2_tags = su2.cell_data_dict["su2:tag"]["triangle"].astype(int)
    assert len(su2_triangles) == sum(len(v) for v in owned_boundary_faces.values())
    seen_su2_faces = {name: set() for name in marker_names}
    for tri, tag in zip(su2_triangles, su2_tags):
        assert 1 <= tag <= len(marker_names)
        marker_name = marker_names[tag - 1]
        key = tuple(sorted(int(node_ids[v]) for v in tri))
        assert key not in seen_su2_faces[marker_name]
        seen_su2_faces[marker_name].add(key)
    assert seen_su2_faces == owned_boundary_faces, "SU2 marker interpretation/physical face incidence mismatch"

    # Derive bad-core physical categories from actual physical facet IDs, not the VTU surface proxy.
    classifications = {"physical_wing_face": 0, "interior_touching_wing": 0, "interior_free_or_other": 0}
    flag_groups = {}
    location_flag_groups = {}
    bad_physical = {}
    for j, idx in enumerate(bad_idx):
        row, _ = read_owned(int(idx))
        is_wing_face = any(f["marker_name"] == "WING3D" for f in row["physical_faces"])
        touches_wing = any(int(node_ids[v]) in wing_vertices for v in cells[idx])
        cls = "physical_wing_face" if is_wing_face else ("interior_touching_wing" if touches_wing else "interior_free_or_other")
        classifications[cls] += 1
        qbad, jbad, gbad = (bool(bdata[k][j]) for k in ("q_bad", "j_bad", "geometry_bad"))
        combo = f"q{int(qbad)}_J{int(jbad)}_geometry{int(gbad)}"
        flag_groups[combo] = flag_groups.get(combo, 0) + 1
        location_flag_groups.setdefault(cls, {})[combo] = location_flag_groups.setdefault(cls, {}).get(combo, 0) + 1
        bad_physical[int(bdata["global_cell_id"][j])] = row["physical_faces"]
    assert sum(classifications.values()) == 260

    chosen = []
    used = set()
    omissions = []
    centers = full.points[cells].mean(axis=1)
    bad_centers = {int(cid): centers[idx] for cid, idx in zip(bad_ids, bad_idx)}

    def context(index: int, role: str) -> dict | None:
        cellid = int(ids[index])
        if cellid in used:
            return None
        vertex_stars = stars(index)
        union = np.unique(np.concatenate(vertex_stars))
        if len(union) > 64:
            return None
        owned, rank = read_owned(index)
        row = {"id": cellid, "rank": rank, "role": role,
               "nodes": owned["nodes"], "vertex_star_sizes": [int(len(s)) for s in vertex_stars],
               "four_star_union_size": int(len(union)), "within_existing64": True,
               "context_cell_ids": [int(ids[i]) for i in union],
               "physical_faces": len(owned["physical_faces"]),
               "physical_reference_facets": owned["physical_faces"],
               "center_xyz": centers[index].tolist(),
               "location_class": "physical_wing_face" if any(f["marker_name"] == "WING3D" for f in owned["physical_faces"]) else
                    ("interior_touching_wing" if any(int(node_ids[v]) in wing_vertices for v in cells[index]) else "interior_free_or_other")}
        pos = np.where(bad_ids == cellid)[0]
        if len(pos):
            j = int(pos[0])
            row.update({"q": float(bdata["q"][j]), "J": float(bdata["J"][j]),
                        "q_bad": bool(bdata["q_bad"][j]), "j_bad": bool(bdata["j_bad"][j]),
                        "geometry_bad": bool(bdata["geometry_bad"][j]), "size_bad": bool(bdata["size_bad"][j]),
                        "geometry_excess_l2": float(bdata["geometry_excess_l2"][j]),
                        "near_feature": bool(bdata["near_feature"][j]), "feature_known": bool(bdata["feature_known"][j]),
                        "normalized_shape_ratio_min": min(float(bdata["q"][j]) / .2, float(bdata["J"][j]) / .05),
                        "normalized_shape_miss": max(0., 1. - min(float(bdata["q"][j]) / .2, float(bdata["J"][j]) / .05)),
                        })
        else:
            row.update({"q": float(fdata["q"][index]), "J": float(fdata["J"][index]),
                        "q_bad": bool(fdata["q"][index] < .2), "j_bad": bool(fdata["J"][index] < .05),
                        "geometry_bad": bool(fdata["geometry_bad"][index])})
        return row

    def select(role: str, candidates: np.ndarray, spatial: bool = False) -> None:
        candidates = [int(i) for i in candidates if int(ids[i]) not in used]
        if spatial and chosen:
            candidates.sort(key=lambda i: min(float(np.linalg.norm(centers[i] - np.asarray(c["center_xyz"]))) for c in chosen), reverse=True)
        for index in candidates:
            row = context(index, role)
            if row is not None:
                chosen.append(row); used.add(row["id"]); return
        omissions.append({"role": role, "candidate_count": len(candidates), "reason": "no unused complete four-star union within existing64"})

    surface = bdata["surface"].astype(bool)
    qbad = bdata["q_bad"].astype(bool)
    jbad = bdata["j_bad"].astype(bool)
    gbad = bdata["geometry_bad"].astype(bool)
    shape = qbad | jbad
    ratio = np.minimum(bdata["q"] / .2, bdata["J"] / .05)
    ids_bad = np.asarray([int(ids[i]) for i in bad_idx])
    wingface = np.asarray([any(f["marker_name"] == "WING3D" for f in bad_physical[int(cid)]) for cid in ids_bad])
    touchwing = np.asarray([any(int(node_ids[v]) in wing_vertices for v in cells[idx]) for idx in bad_idx])
    # Strictly choose named contrasts; sort worst normalized miss first, then geometric excess.
    def ordered(mask: np.ndarray) -> np.ndarray:
        candidates = np.flatnonzero(mask)
        order = np.lexsort((-bdata["geometry_excess_l2"][candidates], ratio[candidates]))
        return bad_idx[candidates[order]]

    select("surface_shape_only_worst", ordered(surface & wingface & shape & ~gbad))
    select("surface_shape_only_spatial", ordered(surface & wingface & shape & ~gbad), spatial=True)
    select("surface_shape_and_geometry_worst", ordered(surface & wingface & shape & gbad))
    select("surface_geometry_only_worst", ordered(surface & wingface & ~shape & gbad))
    select("adjacent_volume_shape_only", ordered(~surface & touchwing & shape & ~gbad))
    select("adjacent_volume_shape_and_geometry", ordered(~surface & touchwing & shape & gbad))
    select("interior_free_shape_only", ordered(~surface & ~touchwing & shape & ~gbad))

    # Nearest clean physical wing face in the complete vertex star of selected wing boundary core.
    boundary = next((r for r in chosen if r["role"] == "surface_shape_only_worst"), None)
    if boundary is not None:
        core_index = by_id[boundary["id"]]
        near = np.unique(np.concatenate(stars(core_index)))
        clean = [int(i) for i in near if fdata["q"][i] >= .2 and fdata["J"][i] >= .05 and not fdata["geometry_bad"][i]]
        clean.sort(key=lambda i: float(np.linalg.norm(centers[i] - centers[core_index])))
        for index in clean:
            row = context(index, "nearby_clean_surface_control")
            if row is not None and any(f["marker_name"] == "WING3D" for f in row["physical_reference_facets"]):
                row["control_for_cell"] = boundary["id"]
                row["q"] = float(fdata["q"][index]); row["J"] = float(fdata["J"][index])
                row["geometry_bad"] = bool(fdata["geometry_bad"][index])
                chosen.append(row); used.add(row["id"]); break
        else:
            omissions.append({"role": "nearby_clean_surface_control", "reason": "no adjacent clean physical wing core within existing64"})
    else:
        omissions.append({"role": "nearby_clean_surface_control", "reason": "no selected surface shape-only core"})

    assert len(chosen) <= 8 and len({r["id"] for r in chosen}) == len(chosen)
    assert all(r["within_existing64"] and r["four_star_union_size"] <= 64 for r in chosen)
    # Each direct physical facet is tied back to a unique actual reference record above.
    for row in chosen:
        for f in row["physical_reference_facets"]:
            assert f["reference_facet_id"] in facet and facet[f["reference_facet_id"]]["marker_name"] == f["marker_name"]
    after = {str(p.resolve()): sha(p) for p in paths}
    assert before == after, "source/input changed during bounded selection"
    outdata = {
        "schema": "build254_bounded_actual_residual_selection_v2",
        "source_build": "build254", "source_event": 1,
        "mesh": {"points": len(full.points), "tetrahedra": len(cells), "bad_union": len(bad_idx), "complete_neighbor_cells": int(len(meshio.read(paths[3]).cells_dict["tetra"]))},
        "reader_receipt": {k: receipt[k] for k in ("all_reader_checks_passed", "actual_points", "actual_cells", "bad_cells", "cavity_cells", "measured_bad_counts", "minimum_metric_eigenvalue", "coordinates_tetrahedra_equal_su2")},
        "classification": {"physical_location": classifications, "q_J_geometry_boolean_counts": flag_groups,
                           "location_by_q_J_geometry": location_flag_groups,
                           "q_bad": int(qbad.sum()), "j_bad": int(jbad.sum()), "shape_bad_union": int(shape.sum()),
                           "geometry_bad": int(gbad.sum()), "geometry_only": int((gbad & ~shape).sum()),
                           "shape_and_geometry": int((gbad & shape).sum()), "shape_only": int((shape & ~gbad).sum()),
                           "near_feature_bad": int(np.count_nonzero(bdata["near_feature"]))},
        "marker_id_to_name": {str(i): name for i, name in enumerate(marker_names)},
        "expected_bounded_contexts": len(chosen), "bounded_selected_contexts": chosen, "omissions": omissions,
        "supersedes_provisional": {"path": str((RUN / "bounded_residual_selection_provisional_original_id_classification.json").resolve()),
                                   "reason": "Provisional classification incorrectly compared adapted volume node IDs with CAD/reference facet node IDs. Corrected selection derives current WING3D vertices from complete owned physical faces and reconciles every output SU2 boundary triangle/tag exactly with owned face node IDs and reference-facet marker names."},
        "input_hashes_before": before, "input_hashes_after": after,
        "elapsed_seconds": time.monotonic() - started,
        "wing_vertex_source": "Current WING3D vertices come from complete owned physical faces; every owned row ID/connectivity is checked against full rejected_mesh.vtu, and all SU2 boundary triangles/tags are reconciled exactly to owned physical faces using original marker order. Reference facet IDs identify CAD/reference triangles, not output node IDs.",
        "scope": "Corrected actual terminal cores and complete GLOBAL four-star unions from full VTU connectivity; random-seek owned physical facet associations checked against reference facet records. Contexts may overlap; no serialization or historical replay; <=existing64 only."
    }
    OUT.write_text(json.dumps(outdata, indent=2) + "\n")
    print(json.dumps({"elapsed_seconds": outdata["elapsed_seconds"], "classification": outdata["classification"],
                      "selected": [{k: r.get(k) for k in ("id", "role", "location_class", "four_star_union_size", "physical_faces", "normalized_shape_miss", "near_feature")} for r in chosen], "omissions": omissions}, indent=2))
    for s in owned_streams: s.close()


if __name__ == "__main__":
    main()
