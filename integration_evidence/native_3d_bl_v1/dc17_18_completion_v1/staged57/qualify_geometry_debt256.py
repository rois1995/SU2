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
output = ROOT / "geometry_debt256_qualification.json"
assert not output.exists()
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
pins, stages = {}, []
for index, case in enumerate(cases):
    status = json.loads((case / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert json.loads((case / "terminal_handles.json").read_text())["all_owned_handles_terminal"]
    command = json.loads((case / "command.json").read_text())
    assert command["build"] == "build256" and command["ranks"] == (1 if index == 0 else 6)
    clean_log = re.sub(r"\x1b\[[0-9;]*m", "", (case / "run.log").read_text())
    summaries = [list(map(int, row)) for row in re.findall(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", clean_log)]
    assert len(summaries) == (1 if index == 0 else 6)
    assert all(assertions > 0 and count >= (38 if index == 0 else 90) for assertions, count in summaries)
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
for path in (rows_path, Path(__file__).resolve(), ROOT / "geometry_debt256_validate.py", ROOT / "geometry_debt256_plan.json"):
    pins[str(path)] = sha(path)
changed = [path for path, digest in pins.items() if sha(Path(path)) != digest]
assert not changed, changed
result = {"time": time.time(), "all_required_checks_passed": True, "mpi_ranks": 6,
          "source_pins_sha256": sha(ROOT / "build256_source_pins.json"), "stages": stages,
          "actual_complete_context_raw_witnesses": witnesses, "qualification_pins": pins,
          "scope": "Connected canonical geometry-debt serial/MPI qualification including actual complete captured contexts; not an accepted M6 mesh, original-field equality at trial points, conservative CFD transfer or adapted disk-restart proof."}
output.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({"all_required_checks_passed": True, "stages": stages, "actual_raw_witnesses": len(witnesses)}, indent=2))
