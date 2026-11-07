"""Select bounded actual Mach-only residuals for the existing complete-star diagnostic."""
from pathlib import Path
import contextlib
import json
import struct
import time

import meshio
import numpy as np

import prepare_current237_witnesses as preparation

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build244_mach_only_real_cfd50k"
SELECTION = RUN / "mach244_coverage_assessment.json"


def prepare(run, selection):
    RUN, SELECTION = Path(run), Path(selection)
    started = time.monotonic()
    assert not SELECTION.exists(), "Preserve and inspect an existing preparation before resuming"
    readback = json.loads((RUN / "inspection_reader_status.json").read_text())
    assert readback["all_reader_checks_passed"] and readback["exit_code"] == 0
    ready = json.loads((RUN / "capture_ready.json").read_text())
    manifests, missing = preparation.old.reader._load_manifests(
        [str(RUN / "adapted_mesh_inspection_rank*_manifest.json")])
    assert not missing and len(manifests) == 6 and ready["metric_census_complete"]
    paths = [RUN / "inspection" / name for name in ("rejected_mesh.vtu", "bad_cells.vtu")]
    paths += [RUN / "inspection_reader_status.json", RUN / "capture_ready.json", RUN / "command.json", Path(__file__),
              Path(preparation.__file__), ROOT / "prepare_boundary202.py", ROOT / "inspect_native_failure3d.py"]
    paths += [m["_resolved_files"]["owned"] for m in manifests]
    before = {str(p.resolve()): preparation.old.sha256(p) for p in paths}
    reader_hashes = {str(Path(p).resolve()): h for p, h in readback["input_hashes_after"].items()}
    for p in paths[:2]:
        assert before[str(p.resolve())] == reader_hashes[str(p.resolve())]
    for m in manifests:
        p = m["_resolved_files"]["owned"]
        assert before[str(p.resolve())] == ready["files"][str(p.resolve())]["sha256"]

    full, bad = (meshio.read(p) for p in paths[:2])
    cells = full.cells_dict["tetra"]
    ids = full.cell_data_dict["global_cell_id"]["tetra"]
    nodes = full.point_data["global_node_id"]
    data = {k: v["tetra"] for k, v in full.cell_data_dict.items()}
    defects = {k: v["tetra"] for k, v in bad.cell_data_dict.items()}
    assert len(cells) == ready["owned_rows"] == readback["actual_cells"]
    assert len(bad.cells_dict["tetra"]) == readback["bad_cells"]
    assert int(np.count_nonzero(data["q"] < .2)) == ready["totals"]["q_bad"] == readback["measured_bad_counts"]["q_bad"]
    assert int(np.count_nonzero(data["J"] < .05)) == ready["totals"]["j_bad"] == readback["measured_bad_counts"]["j_bad"]
    assert int(np.count_nonzero(data["geometry_bad"])) == ready["totals"]["geometry_bad"] == readback["measured_bad_counts"]["geometry_bad"]
    assert len(np.unique(ids)) == len(ids)
    sorted_ids = np.argsort(ids)
    bad_indices = sorted_ids[np.searchsorted(ids[sorted_ids], defects["global_cell_id"])]
    assert np.array_equal(ids[bad_indices], defects["global_cell_id"])

    # Complete global vertex incidence, including clean neighbors outside the bounded bad-cell export.
    incidence = np.argsort(cells.ravel(), kind="stable")
    offsets = np.r_[0, np.cumsum(np.bincount(cells.ravel(), minlength=len(nodes)))]
    rank_ends = np.cumsum([m["written_rows"] for m in manifests])
    rank_starts = np.r_[0, rank_ends[:-1]]
    selected, used, omissions = [], set(), []
    with contextlib.ExitStack() as stack:
        streams = [stack.enter_context(m["_resolved_files"]["owned"].open("rb")) for m in manifests]

        def context(index, role):
            stars = [incidence[offsets[v]:offsets[v + 1]] // 4 for v in cells[index]]
            union = np.unique(np.concatenate(stars))
            if len(union) > 64:
                return None
            rank = int(np.searchsorted(rank_ends, index, side="right"))
            stream = streams[rank]
            stream.seek(8 + 416 * (int(index) - int(rank_starts[rank])))
            raw = stream.read(416)
            assert len(raw) == 416 and struct.unpack_from("<Q", raw)[0] == preparation.old.reader.OWNED_FORMAT
            assert struct.unpack_from("<Q", raw, 8)[0] == int(ids[index])
            for k, v in enumerate(cells[index]):
                node, *xyz = struct.unpack_from("<Qddd", raw, 16 + 32 * k)
                assert node == nodes[v] and np.array_equal(xyz, full.points[v])
            flags = [struct.unpack_from("<Q", raw, 152 + 16 * k)[0] for k in range(4)]
            assert all(x in (0, 1) for x in flags)
            return {"id": int(ids[index]), "rank": rank, "role": role,
                    "nodes": [int(nodes[v]) for v in cells[index]],
                    "vertex_star_sizes": [len(star) for star in stars],
                    "four_star_union_size": len(union), "within_existing64": True,
                    "context_cell_ids": [int(ids[i]) for i in union],
                    "physical_faces": sum(flags), "q": float(data["q"][index]),
                    "J": float(data["J"][index]), "geometry_bad": bool(data["geometry_bad"][index]),
                    "geometry_excess_l2": float(data["geometry_excess_l2"][index])}

        surface = defects["surface"].astype(bool)
        shape = (defects["q"] < .2) | (defects["J"] < .05)
        geometry = defects["geometry_bad"].astype(bool)
        strength = np.minimum(defects["q"] / .2, defects["J"] / .05)
        groups = [("interior_shape_severe", ~surface & shape, strength),
                  ("interior_q_near_gate", ~surface & (defects["q"] < .2) & (defects["q"] > .19), -defects["q"]),
                  ("surface_shape_geometry_clean", surface & shape & ~geometry, strength),
                  ("surface_shape_and_geometry", surface & shape & geometry, strength),
                  ("surface_geometry_only", surface & ~shape & geometry, -defects["geometry_excess_l2"]),
                  ("feature_adjacent_shape", surface & shape & defects["near_feature"].astype(bool), strength)]
        for role, mask, order in groups:
            candidates = np.flatnonzero(mask)
            candidates = candidates[np.argsort(order[candidates], kind="stable")]
            for candidate in candidates:
                index = int(bad_indices[candidate])
                if int(ids[index]) in used:
                    continue
                row = context(index, role)
                if row is not None:
                    row["near_feature"] = bool(defects["near_feature"][candidate])
                    selected.append(row)
                    used.add(row["id"])
                    break
            else:
                omissions.append({"role": role, "candidate_cells": len(candidates), "reason": "No unused complete four-star union within existing64"})

        boundary = next(row for row in selected if row["role"] == "surface_shape_geometry_clean")
        core = next(int(i) for i in bad_indices if int(ids[i]) == boundary["id"])
        neighbors = np.unique(np.concatenate([incidence[offsets[v]:offsets[v + 1]] // 4 for v in cells[core]]))
        distance = np.linalg.norm(full.points[cells[neighbors]].mean(axis=1) - full.points[cells[core]].mean(axis=0), axis=1)
        for index in neighbors[np.argsort(distance, kind="stable")]:
            if int(ids[index]) in used or data["q"][index] < .2 or data["J"][index] < .05 or data["geometry_bad"][index]:
                continue
            row = context(int(index), "nearby_clean_surface_control")
            if row is not None and row["physical_faces"]:
                row["control_for_cell"] = boundary["id"]
                selected.append(row)
                break
        else:
            omissions.append({"role": "nearby_clean_surface_control", "reason": "No adjacent clean physical core within existing64"})

    assert 5 <= len(selected) <= 7 and len({row["id"] for row in selected}) == len(selected)
    assert any(not row["physical_faces"] for row in selected)
    assert any(row["physical_faces"] and row["geometry_bad"] for row in selected)
    assert any(row["role"] == "nearby_clean_surface_control" for row in selected)
    assert before == {str(p.resolve()): preparation.old.sha256(p) for p in paths}
    result = {"schema": "native_actual_residual_selection_v1", "source_build": json.loads((RUN / "command.json").read_text())["build"],
              "sensor": "MACH", "source_event": 1, "expected_bounded_contexts": len(selected),
              "bounded_selected_contexts": selected, "omissions": omissions,
              "input_hashes_before": before, "input_hashes_after": before,
              "selection_seconds": time.monotonic() - started,
              "scope": "Actual terminal cores and complete GLOBAL four-star neighbors within existing64; contexts overlap and gains must not be added. Current nodal tensors/fresh associations, not historical trial replay or acceptance."}
    SELECTION.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"selected": [{k: row[k] for k in ("id", "role", "four_star_union_size", "physical_faces")} for row in selected], "omissions": omissions}), flush=True)
    preparation.prepare(RUN, SELECTION)


def main():
    prepare(RUN, SELECTION)


if __name__ == "__main__":
    main()
