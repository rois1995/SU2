"""Compare the existing exact-query corpus before/after bounded witness pruning."""
import hashlib
import json
import sys
from pathlib import Path
from analyze_mach245_queries import parse, analyze

ROOT = Path(__file__).resolve().parent
baseline = ROOT / "cases/current_compound_20261107T175201_454502"
current = Path(sys.argv[1]).absolute()
assert current != baseline

def read_case(folder):
    status = json.loads((folder / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    log = folder / "run.log"
    queries = parse(log.read_text().splitlines())
    assert len(queries) == 5241 and all("bound_bits" in q for q in queries)
    results = [json.loads(line) for line in (folder / "native_current_residual_result.jsonl").read_text().splitlines()]
    assert len(results) == 166
    return status, log, queries, results

old_status, old_log, old_queries, old_rows = read_case(baseline)
new_status, new_log, new_queries, new_rows = read_case(current)
keys = ("tolerance_bits", "support", "triangles", "cache", "bound_bits")
query_changes = [i for i, (a, b) in enumerate(zip(old_queries, new_queries)) if any(a[k] != b[k] for k in keys)]
# Timing and counted work are the measured outputs; every other result must remain exact.
normalize = lambda row: {k: v for k, v in row.items() if "seconds" not in k and "work" not in k}
result_changes = [i for i, (a, b) in enumerate(zip(old_rows, new_rows)) if normalize(a) != normalize(b)]
old_work = sum(q["work"] for q in old_queries)
new_work = sum(q["work"] for q in new_queries)
output = {"baseline": str(baseline), "current": str(current), "status": new_status,
          "baseline_log_sha256": hashlib.sha256(old_log.read_bytes()).hexdigest(),
          "current_log_sha256": hashlib.sha256(new_log.read_bytes()).hexdigest(),
          "ordered_queries": len(new_queries), "result_rows": len(new_rows),
          "changed_query_results": query_changes, "changed_candidate_results": result_changes,
          "baseline_work": old_work, "current_work": new_work,
          "work_reduction_fraction": 1 - new_work / old_work,
          "current_query_report": analyze(new_queries),
          "scope": "Exact local geometry/candidate comparison; diagnostic timing is not whole-M6 performance, global acceptance or publication proof."}
(ROOT / "witness253_actual_comparison.json").write_text(json.dumps(output, indent=2) + "\n")
print(json.dumps({k: v for k, v in output.items() if k != "current_query_report"}, indent=2))
assert not query_changes and not result_changes
