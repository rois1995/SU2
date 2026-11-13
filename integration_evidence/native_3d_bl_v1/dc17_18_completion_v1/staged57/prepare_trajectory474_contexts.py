#!/usr/bin/env python3
"""Prepare exact current473 edge-star input for native_actual_bl_surface_cavities."""
from __future__ import annotations
import hashlib, importlib.util, json
from pathlib import Path

HERE = Path(__file__).resolve().parent
INV = HERE / "residual473_terminal_inventory.json"
ASSESS = HERE / "surface_stage473_assessment.json"
OUT = HERE / "current_trajectory474_contexts.dat"
PROV = HERE / "current_trajectory474_contexts_provenance.json"
TARGETS = (("current_rank4_edge_24152_24423", (24152, 24423)),
           ("current_rank5_edge_67_68", (67, 68)))
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
inv = json.loads(INV.read_text())
assessment = json.loads(ASSESS.read_text())
assert inv["assessment_sha256"] == sha(ASSESS)
assert inv["case"] == assessment["case"]
case = Path(inv["case"])
spec = importlib.util.spec_from_file_location("inspect_native_failure3d", HERE / "inspect_native_failure3d.py")
mod = importlib.util.module_from_spec(spec); assert spec.loader; spec.loader.exec_module(mod)
rank_rows = {int(r["rank"]): r for r in inv["rank_inputs"]}
actual = {edge: {} for _, edge in TARGETS}
shard_records=[]
for rank in range(6):
    record=rank_rows[rank]; owned=Path(record["owned_path"]); manifest=Path(record["manifest_path"])
    assert sha(owned)==record["owned_sha256"]
    assert sha(manifest)==record["manifest_sha256"]
    md=json.loads(manifest.read_text()); assert md["rank"]==rank and md["complete_shard"]
    assert int(md["working_rows"])==record["owned_rows"]
    param=Path(record["parameters_path"]); assert sha(param)==record["parameters_sha256"]
    assert len(param.read_bytes())==record["parameters_bytes"]
    shard_records.append({"rank":rank,"owned_path":str(owned),"owned_sha256":sha(owned),
                          "manifest_path":str(manifest),"manifest_sha256":sha(manifest),
                          "parameters_path":str(param),"parameters_sha256":sha(param)})
    for row in mod._read_owned(owned, int(md["working_rows"]), 2_000_000_000):
        ids={n["id"] for n in row["nodes"]}
        for _, edge in TARGETS:
            if set(edge).issubset(ids):
                key=tuple(sorted(n["id"] for n in row["nodes"]))
                assert row["id"] not in actual[edge]
                actual[edge][row["id"]]=key

lines=[str(len(TARGETS))]
contexts=[]
for label, edge in TARGETS:
    key=f"{edge[0]}-{edge[1]}"
    saved=inv["edge_stars"][key]
    assert saved["edge_nodes"]==list(edge)
    saved_by_id={int(c["id"]):tuple(sorted(int(n["id"]) for n in c["nodes"])) for c in saved["cells"]}
    assert len(saved_by_id)==saved["cell_count"]
    assert actual[edge]==saved_by_id, f"owned shards disagree with inventoried complete edge star {edge}"
    assert all(edge[0] in q and edge[1] in q for q in actual[edge].values())
    lines.append(f"{label} {edge[0]} {edge[1]} {len(saved_by_id)}")
    for q in sorted(saved_by_id.values()):
        assert len(q)==4 and len(set(q))==4
        lines.append(" ".join(map(str,q)))
    contexts.append({"label":label,"edge_nodes":list(edge),"cell_count":len(saved_by_id),
                     "cells":[{"global_cell_id":cid,"sorted_vertex_ids":list(q)} for cid,q in sorted(saved_by_id.items())]})
OUT.write_text("\n".join(lines)+"\n")
provenance={"scope":"Diagnostic exact edge-star context input only; labels do not select production behavior.",
 "format_source":"UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_surface_cavities.inc:59-60,108-110",
 "inventory_path":str(INV),"inventory_sha256":sha(INV),
 "assessment_path":str(ASSESS),"assessment_sha256":sha(ASSESS),"case":str(case),
 "context_file":str(OUT),"context_file_sha256":sha(OUT),"saved_shard_inputs":shard_records,
 "contexts":contexts,"assertions":"All six Owned shards were streamed; exact complete edge-star cell IDs/quads were compared to inventory; duplicate/missing/extra cells fail; each cell contains both edge endpoints."}
PROV.write_text(json.dumps(provenance,indent=2,sort_keys=True)+"\n")
print(json.dumps({"contexts":contexts,"context_file":str(OUT),"provenance":str(PROV),"sha256":sha(OUT)},indent=2))
