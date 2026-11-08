"""Seal the connected qualification before the immediate full M6 attempt."""
from pathlib import Path
import hashlib
import json
import re
import sys
import time

ROOT = Path(__file__).resolve().parent
assert len(sys.argv) == 3
cases = [Path(p).resolve() for p in sys.argv[1:]]
assert all(p.parent == (ROOT / "cases").resolve() for p in cases)
output = ROOT / "geometry_priority260_qualification.json"
assert not output.exists()
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
pins, stages = {}, []
for index, case in enumerate(cases):
    status = json.loads((case / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert json.loads((case / "terminal_handles.json").read_text())["all_owned_handles_terminal"]
    command = json.loads((case / "command.json").read_text())
    assert command["build"] == "build260" and command["ranks"] == (1 if index == 0 else 6)
    clean_log = re.sub(r"\x1b\[[0-9;]*m", "", (case / "run.log").read_text())
    summaries = [list(map(int, row)) for row in re.findall(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", clean_log)]
    assert len(summaries) == (1 if index == 0 else 6)
    assert all(assertions > 0 and count == (58 if index == 0 else 99) for assertions, count in summaries)
    for path, digest in json.loads((case / "pins.json").read_text()).items():
        assert pins.setdefault(path, digest) == digest
    for name in ("status.json", "terminal_handles.json", "command.json", "pins.json", "run.log"):
        path = case / name
        pins[str(path)] = sha(path)
    stages.append({"case": str(case), "status": status, "summaries": summaries})
rows_path = cases[0] / "native_current_residual_result.jsonl"
rows = [json.loads(line) for line in rows_path.read_text().splitlines()]
results = [r for r in rows if r["stage"] == "bounded_star_result"]
assert len([r for r in rows if r["stage"] == "current_metric_match"]) == 6
for row in results:
    for prefix in ("old", "new"):
        strict = row[prefix + "_face_excess_squared"]
        guidance = row[prefix + "_geometry_excess"]
        assert abs(strict - guidance) <= 1e-10 * max(strict, 1e-30)
witnesses = [r for r in results if r["case"] == "geometry_clean_4"
    and r["kind"] == "collapse_before_retained_star_optimizer" and r["metric_valid"]
    and r["replacement"] and r["interface_valid"]
    and r["new_q_bad"] < r["old_q_bad"] and r["new_J_bad"] < r["old_J_bad"]
    and r["new_bad_physical_faces"] < r["old_bad_physical_faces"]
    and r["new_face_excess_squared"] < r["old_face_excess_squared"]
    and r["new_maximum_face_error"] <= r["old_maximum_face_error"]
    and r["new_context_vertices"] < r["old_context_vertices"] and r["new_maximum_edge"] <= 1.8]
assert witnesses
predictions = [r for r in rows if r["stage"] == "complete_boundary_edge_split"]
assert len(predictions) == 20 and all(r["prediction_complete"] and r["prediction_faces"] == 6 for r in predictions)
oracle = {(r["case"], "complete_edge_split_" + "_".join(map(str, r["edge"]))): r for r in predictions}
matches = []
for row in results:
    if not row["kind"].startswith("complete_edge_split_"):
        continue
    predicted = oracle[(row["case"], row["kind"])]
    actual_delta = row["new_geometry_excess"] - row["old_geometry_excess"]
    predicted_delta = predicted["predicted_after"] - predicted["predicted_before"]
    scale = max(row["old_geometry_excess"], row["new_geometry_excess"],
                predicted["predicted_before"], predicted["predicted_after"], 1e-24)
    assert abs(actual_delta - predicted_delta) <= 1e-10 * scale
    matches.append({"case": row["case"], "kind": row["kind"], "actual_delta": actual_delta,
                    "predicted_delta": predicted_delta, "relative_error": abs(actual_delta - predicted_delta) / scale})
assert len(matches) == 14
useful = [r for r in results if r["kind"] == "complete_edge_split_18795_202090700"]
assert len(useful) == 1
control = useful[0]
assert control["replacement"] and control["interface_valid"] and control["geometry_refine_valid"]
assert control["new_bad_physical_faces"] < control["old_bad_physical_faces"]
assert control["new_face_excess_squared"] < control["old_face_excess_squared"]
assert all(control[k] == 0 for k in ("q_retired", "q_introduced", "J_retired", "J_introduced"))
for path in (rows_path, Path(__file__).resolve(), ROOT / "geometry_priority260_validate.py", ROOT / "geometry_priority259_plan.json",
             ROOT / "geometry_priority259_review.json", ROOT / "launch_m6260.py"):
    pins[str(path)] = sha(path)
changed = [path for path, digest in pins.items() if sha(Path(path)) != digest]
assert not changed, changed
result = {"time": time.time(), "all_required_checks_passed": True, "mpi_ranks": 6,
          "source_pins_sha256": sha(ROOT / "build260_source_pins.json"), "stages": stages,
          "actual_complete_context_raw_witnesses": witnesses, "qualification_pins": pins,
          "paired_face_predictions": predictions, "complete_context_prediction_matches": matches,
          "useful_complete_edge_control": control, "runtime_priority_m6_verified": False,
          "scope": "Connected paired-face geometry-priority serial/MPI qualification with complete captured-context prediction oracles and preserved canonical guidance. Not actual M6 phase-use/acceptance, original-field equality at trial points, conservative transfer or adapted disk-restart proof."}
output.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({"all_required_checks_passed": True, "stages": stages, "actual_raw_witnesses": len(witnesses)}, indent=2))
