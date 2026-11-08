"""Compare bounded actual254 results against the complete captured field."""
from pathlib import Path
import hashlib
import json
import re
import meshio
import numpy as np

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build254_conditioned_mach_only_real_cfd50k"
CASE = ROOT / "cases/current_compound_20261108T004404_1638191"
status = json.loads((CASE / "status.json").read_text())
assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
paths = [CASE / n for n in ("status.json", "pins.json", "run.log", "native_current_residual_result.jsonl")]
paths += [RUN / "bounded_residual_selection.json", RUN / "inspection/rejected_mesh.vtu", Path(__file__).resolve()]
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
before = {str(p): sha(p) for p in paths}
selection = json.loads(paths[-3].read_text())
assert selection["schema"] == "build254_bounded_actual_residual_selection_v2"
mesh = meshio.read(paths[-2])
data = {k: v["tetra"] for k, v in mesh.cell_data_dict.items()}
by_id = {int(cid): i for i, cid in enumerate(data["global_cell_id"])}
rows = [json.loads(line) for line in paths[3].read_text().splitlines()]
results = [r for r in rows if r["stage"] == "bounded_star_result"]
assert results
reports = []
for i, context in enumerate(selection["bounded_selected_contexts"]):
    label = ("geometry_bad_" if context["geometry_bad"] else "geometry_clean_") + str(i)
    indices = [by_id[c] for c in context["context_cell_ids"]]
    baseline = {"q_bad": int((data["q"][indices] < .2).sum()),
                "J_bad": int((data["J"][indices] < .05).sum()),
                "geometry_bad": int(data["geometry_bad"][indices].sum())}
    local = [r for r in results if r["case"] == label]
    assert local
    for row in local:
        assert row["context_cells"] == context["four_star_union_size"]
        assert row["old_q_bad"] == baseline["q_bad"] and row["old_J_bad"] == baseline["J_bad"]
        assert row["q_retired"] - row["q_introduced"] == row["old_q_bad"] - row["new_q_bad"]
        assert row["J_retired"] - row["J_introduced"] == row["old_J_bad"] - row["new_J_bad"]
    useful = [r for r in local if r["metric_valid"] and r["replacement"] and r["interface_valid"]
              and r["new_q_bad"] <= r["old_q_bad"] and r["new_J_bad"] <= r["old_J_bad"]
              and (r["new_q_bad"] < r["old_q_bad"] or r["new_J_bad"] < r["old_J_bad"])
              and r["new_geometry_bad"] <= r["old_geometry_bad"]
              and r["new_geometry_excess"] <= r["old_geometry_excess"]]
    churn = [r for r in local if r["metric_valid"] and
             (r["new_q_bad"] > r["old_q_bad"] or r["new_J_bad"] > r["old_J_bad"])]
    reports.append({"core": context["id"], "role": context["role"], "label": label,
                    "captured_complete_context_bad_counts": baseline,
                    "recomputed_geometry_bad_counts": sorted({r["old_geometry_bad"] for r in local}),
                    "geometry_count_matches_capture": all(r["old_geometry_bad"] == baseline["geometry_bad"] for r in local),
                    "result_rows": len(local), "useful_rows": useful, "metric_admitted_churn_rows": churn,
                    "directed_collapse_trials": [r for r in rows if r["stage"] == "current_directed_collapse" and r["case"] == label]})
after = {str(p): sha(p) for p in paths}
assert before == after
assertions = re.findall(r"All tests passed \((\d+) assertions", paths[2].read_text())
assert len(assertions) == 1
out = {"schema": "actual254_cavity_assessment_v1", "status": status, "case": str(CASE),
       "assertions": int(assertions[0]), "contexts": reports, "rows": rows,
       "input_hashes_before": before, "input_hashes_after": after,
       "scope": "Existing bounded operator controls only; overlapping contexts must not be summed. Current nodal donor reconstruction matches captured q/J at unchanged cells; it does not prove original frozen-field equality at trial points. Geometry count reproduction is checked explicitly; area-weighted guidance is distinct from terminal excess. GeometryRepair metric kind, nonlocal contact/publication and requested BL heights are not exercised. No accepted mesh or production correction established."}
(RUN / "bounded_cavity_assessment.json").write_text(json.dumps(out, indent=2) + "\n")
print(json.dumps({"assertions": out["assertions"], "contexts": [
    {k: r[k] for k in ("core", "role", "captured_complete_context_bad_counts", "recomputed_geometry_bad_counts", "geometry_count_matches_capture", "result_rows")}
    | {"useful_rows": len(r["useful_rows"]), "admitted_churn_rows": len(r["metric_admitted_churn_rows"])}
    for r in reports]}, indent=2))
