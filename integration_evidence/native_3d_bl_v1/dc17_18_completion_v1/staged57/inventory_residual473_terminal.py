#!/usr/bin/env python3
"""Stream saved surface473 Owned shards and inventory requested edge/vertex stars."""
from __future__ import annotations
import hashlib, importlib.util, json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ASSESS = HERE / "surface_stage473_assessment.json"
TARGETS = ((24152, 24423), (67, 68))
CONTROL_VERTEX = 6070

spec = importlib.util.spec_from_file_location("inspect_native_failure3d", HERE / "inspect_native_failure3d.py")
mod = importlib.util.module_from_spec(spec)
assert spec.loader
spec.loader.exec_module(mod)
assessment = json.loads(ASSESS.read_text())
case = Path(assessment["case"])
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
rows_by_rank = {}
relevant = {}
node_stars = {n: [] for e in TARGETS for n in e}
node_stars[CONTROL_VERTEX] = []
rank_inputs = []
all_param_bytes = []
expected_total = 0
for rank in range(6):
    manifest_path = case / f"stage_surface_rank{rank}_manifest.json"
    manifest = json.loads(manifest_path.read_text())
    assert manifest["rank"] == rank and manifest["complete_shard"]
    owned_path = case / manifest["files"]["owned"]
    parameter_path = case / manifest["files"]["parameters"]
    count = int(manifest["working_rows"])
    expected_total += count
    rows = 0
    for row in mod._read_owned(owned_path, count, 2_000_000_000):
        rows += 1
        ids = {node["id"] for node in row["nodes"]}
        if any(set(edge).issubset(ids) for edge in TARGETS) or ids.intersection(node_stars):
            row = dict(row)
            row["owner_rank"] = rank
            for i, node in enumerate(row["nodes"]):
                node["raw_nodal_tensor6"] = row["sensor"][6*i:6*i+6] if row["sensor_present"] else None
            # Physical slots follow the Owned record's local tetra face order; preserve slot and facet ID verbatim.
            standard_faces = ((1, 2, 3), (0, 3, 2), (0, 1, 3), (0, 2, 1))
            row["physical_faces"] = [dict(local_face_slot=i, present=x["present"], facet_id=x["facet"],
                node_ids=[row["nodes"][j]["id"] for j in standard_faces[i]] if x["present"] else None)
                for i,x in enumerate(row["physical"])]
            relevant[row["id"]] = row
            for n in ids.intersection(node_stars):
                node_stars[n].append(row["id"])
    assert rows == count
    params = parameter_path.read_bytes()
    all_param_bytes.append(params)
    rank_inputs.append({
        "rank": rank, "owned_path": str(owned_path), "owned_sha256": sha(owned_path),
        "owned_rows": count, "manifest_path": str(manifest_path), "manifest_sha256": sha(manifest_path),
        "parameters_path": str(parameter_path), "parameters_sha256": hashlib.sha256(params).hexdigest(),
        "parameters_bytes": len(params),
    })
assert expected_total == sum(int(x["local_tets"]) for x in assessment["ranks"])
# Full star membership derives from the streamed complete shard scan, while relevant rows retain exact source fields.
edge_stars = {}
for edge in TARGETS:
    a,b=edge
    ids = sorted(cid for cid,row in relevant.items() if {a,b}.issubset({n["id"] for n in row["nodes"]}))
    edge_stars[f"{a}-{b}"] = {"edge_nodes": list(edge), "cell_count": len(ids), "cells": [relevant[c] for c in ids]}
vertex_result = {str(n): {"present": bool(cids), "cell_ids": sorted(cids), "cell_count": len(cids),
    "cells": [relevant[c] for c in sorted(cids)]} for n,cids in node_stars.items()}
param_hashes=[x["parameters_sha256"] for x in rank_inputs]
output = {
    "scope": "Saved Owned-shard source inventory only; no native query/operator, geometry association, cleanliness, or phenotype inference.",
    "assessment_path": str(ASSESS), "assessment_sha256": sha(ASSESS),
    "case": str(case), "expected_total_owned_rows": expected_total,
    "parameter_payloads_bit_identical": all(x == all_param_bytes[0] for x in all_param_bytes[1:]),
    "parameter_sha256_by_rank": param_hashes,
    "rank_inputs": rank_inputs,
    "edge_stars": edge_stars,
    "endpoint_vertex_stars": vertex_result,
    "control_vertex": CONTROL_VERTEX,
    "limits": ["Physical entries preserve the Owned record's slot and facet IDs; no external reference association is reconstructed.", "Raw nodal tensor6 values are the six serialized values per node in the Owned sensor field when present."],
}
out=HERE/"residual473_terminal_inventory.json"
out.write_text(json.dumps(output, indent=2, sort_keys=True)+"\n")
print(json.dumps({"output":str(out),"rows":expected_total,"edge_counts":{k:v["cell_count"] for k,v in edge_stars.items()},"vertex_counts":{k:v["cell_count"] for k,v in vertex_result.items()},"parameters_bit_identical":output["parameter_payloads_bit_identical"]},indent=2))
