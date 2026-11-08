"""Extend the same bounded assessment with face-level geometry accounting."""
from pathlib import Path
import hashlib
import json
import sys
import numpy as np

ROOT = Path(__file__).resolve().parent
case = Path(sys.argv[1]).resolve()
assert case.parent == (ROOT / "cases").resolve()
assert json.loads((case / "command.json").read_text())["build"] == "build255"
assert json.loads((case / "terminal_handles.json").read_text())["all_owned_handles_terminal"]
source = ROOT / "assess_current254_cavities.py"
code = source.read_text()
changes = [
    ('CASE = ROOT / "cases/current_compound_20261108T004404_1638191"', f"CASE = Path({str(case)!r})"),
    ('bounded_cavity_assessment.json', 'bounded_cavity255_assessment.json'),
    ('GeometryRepair metric kind, nonlocal contact/publication and requested BL heights are not exercised.',
     'GeometryRepair metric-kind admission is reported; nonlocal contact/publication and requested BL heights are not exercised.'),
]
for old, new in changes:
    assert code.count(old) == 1
    code = code.replace(old, new)
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
before = {str(p): sha(p) for p in (source, Path(__file__).resolve())}
namespace = {"__file__": str(source.resolve()), "__name__": "actual255_cavity_assessment"}
exec(compile(code, str(source), "exec"), namespace)
report = namespace["out"]
data, by_id = namespace["data"], namespace["by_id"]
selection = namespace["selection"]["bounded_selected_contexts"]
for context, result in zip(selection, report["contexts"]):
    indices = [by_id[c] for c in context["context_cell_ids"]]
    captured_excess = float(np.square(data["geometry_excess_l2"][indices]).sum())
    local = [r for r in report["rows"] if r.get("case") == result["label"] and r["stage"] == "bounded_star_result"]
    assert all(np.isfinite(r[k]) and r[k] >= 0 for r in local for k in (
        "old_face_excess_squared", "new_face_excess_squared", "old_maximum_face_error", "new_maximum_face_error"))
    result["captured_face_excess_squared"] = captured_excess
    result["recomputed_face_excess_squared"] = sorted({r["old_face_excess_squared"] for r in local})
    result["face_error_progress_rows"] = [r for r in local if r["metric_valid"] and r["replacement"] and r["interface_valid"]
        and r["new_q_bad"] <= r["old_q_bad"] and r["new_J_bad"] <= r["old_J_bad"]
        and (r["new_q_bad"] < r["old_q_bad"] or r["new_J_bad"] < r["old_J_bad"])
        and r["new_bad_physical_faces"] <= r["old_bad_physical_faces"]
        and r["new_face_excess_squared"] <= r["old_face_excess_squared"]
        and r["new_maximum_face_error"] <= r["old_maximum_face_error"]]
assert before == {str(Path(p)): sha(Path(p)) for p in before}
report["schema"] = "actual255_face_cavity_assessment_v1"
report["reused_assessment"] = {"input_hashes_before": before, "input_hashes_after": before,
                              "transformed_sha256": hashlib.sha256(code.encode()).hexdigest(), "changes": changes}
output = namespace["RUN"] / "bounded_cavity255_assessment.json"
output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps({r["role"]: {"captured_excess": r["captured_face_excess_squared"],
                             "recomputed_excess": r["recomputed_face_excess_squared"],
                             "face_error_progress_rows": len(r["face_error_progress_rows"])}
                  for r in report["contexts"]}, indent=2))
