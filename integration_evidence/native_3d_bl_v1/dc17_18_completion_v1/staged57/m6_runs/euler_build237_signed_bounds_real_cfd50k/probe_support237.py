"""Bounded, read-only support census of the sealed build237 rejected state."""
from pathlib import Path
import hashlib
import importlib.util
import json
import time

import meshio
import numpy as np

attempt = Path(__file__).resolve().parent
started = time.time()
inspector = attempt.parent.parent / "inspect_native_failure3d.py"
spec = importlib.util.spec_from_file_location("native_inspector", inspector)
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def sha(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


assert json.loads((attempt / "terminal_handles.json").read_text())["all_absent"]
assert json.loads((attempt / "inspection_export_terminal_handles.json").read_text())["all_absent"]
verified = json.loads((attempt / "inspection_reader_status.json").read_text())
bad_path = attempt / "inspection/bad_cells.vtu"
assert sha(bad_path) == verified["output_sha256"][str(bad_path)]
bad = meshio.read(bad_path)
fields = {name: value["tetra"] for name, value in bad.cell_data_dict.items()}
assert np.all(fields["cache_valid"] == 1)
bad_ids = fields["global_cell_id"]
assert len(np.unique(bad_ids)) == len(bad_ids) == verified["measured_bad_counts"]["bad_union"]
index = {int(key): i for i, key in enumerate(bad_ids)}
ready = json.loads((attempt / "capture_ready.json").read_text())
dtype = np.dtype({"names": ["format", "id", "nodes", "version", "physical"],
                  "formats": ["<u8", "<u8", ("<u8", (4, 4)), "<u8", ("<u8", (4, 2))],
                  "offsets": [0, 8, 16, 144, 152], "itemsize": reader.OWNED_RECORD_BYTES})
assert reader.OWNED_RECORD_BYTES == 416
all_ids, all_nodes, all_face_counts, candidates, seen = [], [], [], [], set()
histogram = {str(k): {"shape_bad": 0, "shape_and_geometry_bad": 0,
                      "geometry_clean_shape_bad": 0} for k in range(5)}
input_hashes = {str(bad_path): sha(bad_path), str(inspector): sha(inspector),
                str(Path(__file__)): sha(Path(__file__))}
for rank in range(6):
    manifest = json.loads((attempt / f"adapted_mesh_inspection_rank{rank}_manifest.json").read_text())
    path = attempt / manifest["files"]["owned"]
    assert sha(path) == ready["files"][str(path)]["sha256"]
    input_hashes[str(path)] = ready["files"][str(path)]["sha256"]
    rows = np.memmap(path, dtype=dtype, mode="r", offset=8, shape=(manifest["written_rows"],))
    first = next(reader._read_owned(path, manifest["written_rows"], manifest["cap_bytes"]))
    assert int(rows[0]["id"]) == first["id"] and int(rows[0]["version"]) == first["version"]
    assert rows[0]["nodes"][:, 0].tolist() == [n["id"] for n in first["nodes"]]
    assert rows[0]["physical"][:, 0].tolist() == [int(f["present"]) for f in first["physical"]]
    assert np.all(rows["format"] == reader.OWNED_FORMAT)
    assert np.all((rows["physical"][:, :, 0] == 0) | (rows["physical"][:, :, 0] == 1))
    all_face_counts.append(np.sum(rows["physical"][:, :, 0], axis=1))
    all_ids.append(np.array(rows["id"]))
    all_nodes.append(np.array(rows["nodes"][:, :, 0]))
    for j in np.flatnonzero(np.isin(rows["id"], bad_ids, kind="sort")):
        cell = int(rows[j]["id"])
        assert cell not in seen
        seen.add(cell)
        k = index[cell]
        count = int(rows[j]["physical"][:, 0].sum())
        assert bool(fields["surface"][k]) == (count > 0)
        shape_bad = bool(fields["q_bad"][k] or fields["j_bad"][k])
        geometry_bad = bool(fields["geometry_bad"][k])
        histogram[str(count)]["shape_bad"] += int(shape_bad)
        histogram[str(count)]["shape_and_geometry_bad"] += int(shape_bad and geometry_bad)
        histogram[str(count)]["geometry_clean_shape_bad"] += int(shape_bad and not geometry_bad)
        if shape_bad:
            candidates.append({"id": cell, "owner": rank, "version": int(rows[j]["version"]),
                               "nodes": rows[j]["nodes"][:, 0].tolist(),
                               "physical_faces": count, "geometry_bad": geometry_bad, "q": float(fields["q"][k]),
                               "J": float(fields["J"][k]), "near_feature": int(fields["near_feature"][k])})
assert seen == set(index)
assert sum(x["shape_bad"] for x in histogram.values()) == verified["measured_bad_counts"]["shape_bad_union"]
assert sum(x["shape_and_geometry_bad"] for x in histogram.values()) == int((((fields["q_bad"] != 0) | (fields["j_bad"] != 0)) & (fields["geometry_bad"] != 0)).sum())
ids, nodes = np.concatenate(all_ids), np.concatenate(all_nodes)
assert len(ids) == ready["owned_rows"]
# Compare short-edge prevalence against clean cells, rather than inferring causality from bad cells alone.
full_path = attempt / "inspection/rejected_mesh.vtu"
assert sha(full_path) == verified["output_sha256"][str(full_path)]
full = meshio.read(full_path)
all_fields = {name: values["tetra"] for name, values in full.cell_data_dict.items()}
assert np.array_equal(all_fields["global_cell_id"], ids)
face_counts = np.concatenate(all_face_counts)
shape = (all_fields["q"] < .2) | (all_fields["J"] < .05)
assert int(shape.sum()) == verified["measured_bad_counts"]["shape_bad_union"]
short_population = {}
for count in range(5):
    short_population[str(count)] = {}
    for is_bad in (False, True):
        members = (face_counts == count) & (shape == is_bad)
        short_population[str(count)]["shape_bad" if is_bad else "shape_clean"] = {
            "cells": int(members.sum()),
            "short09": int((members & (all_fields["metric_edge_min"] < .9)).sum()),
            "short05": int((members & (all_fields["metric_edge_min"] < .5)).sum())}
input_hashes[str(full_path)] = verified["output_sha256"][str(full_path)]
selected = []
for count in range(5):
    for geometry_bad in (False, True):
        subset = sorted((x for x in candidates if x["geometry_bad"] == geometry_bad
                         and x["physical_faces"] == count),
                        key=lambda x: (min(x["q"] / .2, x["J"] / .05), x["id"]))[:2]
        for core in subset:
            union = np.zeros(len(ids), dtype=bool)
            counts = []
            for vertex in core["nodes"]:
                member = np.any(nodes == vertex, axis=1)
                counts.append(int(member.sum()))
                union |= member
            selected.append(dict(core, vertex_star_sizes=counts, four_star_union_size=int(union.sum()),
                                 within_existing64=int(union.sum()) <= 64,
                                 context_cell_ids=sorted(ids[union].tolist())))
assert input_hashes == {name: sha(name) for name in input_hashes}
result = {"time": time.time(), "seconds": time.time() - started,
          "scope": "Exact terminal237 owned topology and measured bad flags; no repair trial, decision change, or missing-support inference.",
          "inputs_sha256": input_hashes, "physical_face_count_histogram": histogram,
          "short_edge_population_by_physical_face_count": short_population,
          "shape_bad_cores": len(candidates),
          "geometry_bad_shape_cores": sum(x["geometry_bad"] for x in candidates),
          "geometry_clean_shape_cores": sum(not x["geometry_bad"] for x in candidates),
          "bounded_selected_contexts": selected,
          "limits": "Eligibility is not usefulness. Selected unions are exact global four-star unions, not proof of a successful candidate or historical private-query replay."}
(attempt / "support237_coverage_assessment.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({key: result[key] for key in ("seconds", "physical_face_count_histogram",
      "shape_bad_cores", "geometry_bad_shape_cores",
      "geometry_clean_shape_cores")}))
print(json.dumps([{k: x[k] for k in ("id", "geometry_bad", "four_star_union_size", "within_existing64")}
                  for x in selected]))
