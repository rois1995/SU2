"""Locate sealed geometry residuals; never infer complete stars from partial shards."""
from pathlib import Path
import json
import time

import numpy as np
from prepare_boundary202 import FACES, reader, sha256

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k"
OUT = RUN / "geometry_residual_location.json"


def main():
    started = time.monotonic()
    assert not OUT.exists()
    assert json.loads((RUN / "terminal_handles.json").read_text())["all_owned_handles_terminal"]
    ready = json.loads((RUN / "capture_ready.json").read_text())
    before = {p: sha256(Path(p)) for p in ready["files"]}
    assert all(before[p] == v["sha256"] for p, v in ready["files"].items())
    manifests, missing = reader._load_manifests([str(RUN / "adapted_mesh_inspection_rank*_manifest.json")])
    assert not missing and len(manifests) == 6
    bad = []
    for m in manifests:
        paths, caps = m["_resolved_files"], m["_caps"]
        metrics = reader._jsonl(paths["metrics"], caps["metrics"], "metrics")
        found = 0
        for cell in reader._read_owned(paths["owned"], m["written_rows"], caps["owned"]):
            metric = reader._validate_metric(next(metrics), cell, paths["metrics"])
            # FailureCapture writes every bad row before clean rows, even when capped.
            is_bad = any(metric[k] for k in ("q_bad", "j_bad", "geometry_bad", "size_bad", "height_bad", "height_unresolved"))
            if not is_bad:
                break
            assert metric["geometry_bad"] and metric["geometry_measured"]
            bad.append({"rank": m["rank"], "cell": cell, "metric": metric})
            found += 1
        assert found == m["progress"]["geometry_bad"]
    assert len(bad) == sum(m["progress"]["geometry_bad"] for m in manifests) == 78
    assert len({r["cell"]["id"] for r in bad}) == len(bad)

    # Reuse the exact 416-byte record layout already checked by the inspector.
    node = np.dtype([("id", "<u8"), ("xyz", "<f8", (3,))])
    dtype = np.dtype([("format", "<u8"), ("id", "<u8"), ("nodes", node, (4,)),
                      ("version", "<u8"), ("physical", "<u8", (4, 2)),
                      ("sensor_present", "<u8"), ("sensor", "<f8", (4, 6))])
    assert dtype.itemsize == reader.OWNED_RECORD_BYTES == 416
    wanted = sorted({n["id"] for r in bad for n in r["cell"]["nodes"]})
    available = {}
    for m in manifests:
        if not m["written_rows"]:
            continue
        data = np.memmap(m["_resolved_files"]["owned"], dtype=dtype, offset=8,
                         shape=(m["written_rows"],), mode="r")
        assert np.all(data["format"] == reader.OWNED_FORMAT)
        mask = np.isin(data["nodes"]["id"], wanted).any(axis=1)
        for row in data[mask]:
            cid = int(row["id"])
            assert cid not in available
            available[cid] = {"nodes": list(map(int, row["nodes"]["id"])),
                              "physical": row["physical"].tolist(), "rank": m["rank"]}
    incidence = {n: set() for n in wanted}
    for cid, row in available.items():
        for n in row["nodes"]:
            if n in incidence:
                incidence[n].add(cid)
    facet_ids = {f["facet"] for r in bad for f in r["cell"]["physical"] if f["present"]}
    facets = {}
    refpath = manifests[0]["_resolved_files"]["reference"]
    for row in reader._jsonl(refpath, manifests[0]["_caps"]["reference"], "reference"):
        if row.get("kind") == "facet" and row["id"] in facet_ids:
            facets[row["id"]] = {k: row.get(k) for k in ("marker", "body", "sheet", "mapping_error")}
    assert set(facets) == facet_ids
    result = []
    for r in bad:
        cell, metric = r["cell"], r["metric"]
        nodes = [n["id"] for n in cell["nodes"]]
        assert available[cell["id"]]["nodes"] == nodes
        faces = []
        for slot, f in enumerate(cell["physical"]):
            if f["present"]:
                xyz = np.array([cell["nodes"][k]["xyz"] for k in FACES[slot]])
                faces.append({"reference_facet": f["facet"], "reference": facets[f["facet"]],
                              "node_ids": [nodes[k] for k in FACES[slot]],
                              "center": xyz.mean(axis=0).tolist(), "xyz": xyz.tolist()})
        union = set().union(*(incidence[n] for n in nodes))
        result.append({"id": cell["id"], "rank": r["rank"], "nodes": nodes,
                       "physical_faces": faces, "geometry_excess_l2": metric["geometry_excess_l2"],
                       "q": metric["q"], "J": metric["J"], "near_feature": metric["near_feature"],
                       "feature_known": metric["feature_known"], "bindings": metric["bindings"],
                       "production_metric_edges": metric["edges"], "repairs": metric["repairs"],
                       "available_four_star_rows": len(union), "available_context_cell_ids": sorted(union),
                       "available_vertex_star_sizes": [len(incidence[n]) for n in nodes],
                       "global_support_complete": False})
    after = {p: sha256(Path(p)) for p in before}
    assert before == after
    report = {"scope": "All78cached geometry-bad seeds paired against owned records. Final q/J remain unknown where null. Available incidence is NOT globally complete support and must not authorize replacement or final gate claims.",
              "geometry_bad_seeds": len(result), "seeds_with_unknown_shape": sum(r["q"] is None or r["J"] is None for r in result),
              "near_feature": sum(r["near_feature"] for r in result),
              "available_neighbor_rows": len(available), "geometry_bad_cells": result,
              "input_hashes_before": before, "input_hashes_after": after,
              "helper_sha256": sha256(Path(__file__)), "seconds": time.monotonic() - started}
    OUT.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: report[k] for k in ("geometry_bad_seeds", "seeds_with_unknown_shape", "near_feature", "available_neighbor_rows", "seconds")}))


if __name__ == "__main__":
    main()
