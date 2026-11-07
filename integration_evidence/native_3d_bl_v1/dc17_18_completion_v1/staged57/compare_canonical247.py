"""Compare the same bounded actual-context protocol before/after canonical queries."""
from pathlib import Path
import hashlib
import json

from analyze_mach245_queries import analyze, parse, self_check
from assess_current235 import useful

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build244_mach_only_real_cfd50k"


def main():
    self_check()
    inputs, reports, candidates = {}, {}, {}
    for build in (245, 247):
        launch = RUN / f"current_query{build}_attempt.json"
        attempt = Path(json.loads(launch.read_text())["case"])
        status = json.loads((attempt / "status.json").read_text())
        assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
        assert json.loads((attempt / "terminal_handles.json").read_text())["all_absent"]
        command = json.loads((attempt / "command.json").read_text())
        assert command["environment"]["SU2_NATIVE_GEOMETRY_WORK_TRACE"] == "1"
        assert command["environment"]["SU2_NATIVE_FROZEN_CONFIG"] == str(RUN / "compound_probe_v1/run.cfg")
        log = attempt / "run.log"
        assert log.stat().st_size <= 256 << 20
        digest = hashlib.sha256(log.read_bytes()).hexdigest()
        with log.open() as stream:
            reports[build] = analyze(parse(stream))
        assert reports[build]["counts"]["unresolved_queries"] == 0
        assert reports[build]["counts"]["identical_answer_changes"] == 0
        assert hashlib.sha256(log.read_bytes()).hexdigest() == digest
        rows = [json.loads(line) for line in (attempt / "native_current_residual_result.jsonl").read_text().splitlines()]
        metrics = [row for row in rows if row["stage"] == "current_metric_match"]
        assert len(metrics) == 3 and all(row["q_error"] == row["J_error"] == 0 for row in metrics)
        selected = [row for row in rows if row["stage"] == "bounded_star_result"]
        candidates[build] = {(row["case"], row["kind"], row["vertex"]): row for row in selected}
        assert len(candidates[build]) == len(selected)
        inputs[str(log)] = digest
        for path in (launch, attempt / "status.json", attempt / "pins.json", attempt / "command.json",
                     attempt / "terminal_handles.json", attempt / "native_current_residual_result.jsonl"):
            inputs[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    assert candidates[245].keys() == candidates[247].keys()
    fields = ("old_q_bad", "new_q_bad", "old_J_bad", "new_J_bad", "old_geometry_bad", "new_geometry_bad")
    changed_counts, lost, gained = [], [], []
    complete_useful = lambda row: useful(row) and row["new_geometry_bad"] <= row["old_geometry_bad"]
    for key, old in candidates[245].items():
        new = candidates[247][key]
        if any(old[name] != new[name] for name in fields):
            changed_counts.append({"candidate": key, "before": {k: old[k] for k in fields}, "after": {k: new[k] for k in fields}})
        if complete_useful(old) and not complete_useful(new):
            lost.append(key)
        if complete_useful(new) and not complete_useful(old):
            gained.append(key)
    totals = {build: sum(reports[build]["counted_work"].get(k, 0) for k in ("hit", "miss", "bypass")) for build in reports}
    result = {"input_sha256": inputs, "analysis_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "query_reports": reports, "candidate_inventory_equal": True, "changed_bad_counts": changed_counts,
              "lost_useful_candidates": lost, "gained_useful_candidates": gained,
              "query_work_before": totals[245], "query_work_after": totals[247],
              "query_work_reduction_fraction": 1 - totals[247] / totals[245],
              "scope": "Same three actual current contexts and raw-control protocol. Work includes canonical scratch/key processing; verbose timing is not a performance measure. Overlapping candidate gains must not be added. No whole-field mesh acceptance or M6 speedup established."}
    (RUN / "canonical247_actual_comparison.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ("input_sha256", "query_reports")}, indent=2))
    assert totals[247] < totals[245] and not lost


if __name__ == "__main__":
    main()
