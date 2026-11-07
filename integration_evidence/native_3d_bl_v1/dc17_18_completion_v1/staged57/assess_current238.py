"""Assess the bounded one-face collapse hypothesis against preserved237 controls."""
from pathlib import Path
import importlib.util
import json

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build237_signed_bounds_real_cfd50k"
spec = importlib.util.spec_from_file_location("prior_assessment", ROOT / "assess_current235.py")
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)


def main():
    launch = json.loads((ROOT / "current238_launch.json").read_text())
    attempt = Path(launch["attempt"])
    status = json.loads((attempt / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert json.loads((attempt / "terminal_handles.json").read_text())["all_absent"]
    paths = [Path(__file__), ROOT / "assess_current235.py", RUN / "current_residual_assessment.json"]
    paths += [attempt / name for name in ("status.json", "pins.json", "command.json",
                                         "terminal_handles.json", "native_current_residual_result.jsonl")]
    before = {str(p): prior.sha(p) for p in paths}
    baseline = json.loads((RUN / "current_residual_assessment.json").read_text())
    rows = [json.loads(line) for line in (attempt / "native_current_residual_result.jsonl").read_text().splitlines()]
    matches = [r for r in rows if r["stage"] == "current_metric_match"]
    assert len(matches) == 5 and len({r["case"] for r in matches}) == 5
    assert all(r["q_error"] == r["J_error"] == 0 for r in matches)
    candidates = [r for r in rows if r["stage"] == "bounded_star_result"]
    for row in candidates:
        for stem in ("q", "J", "geometry"):
            assert row[stem + "_retired"] - row[stem + "_introduced"] == row["old_" + stem + "_bad"] - row["new_" + stem + "_bad"]
        assert not row["accepted_mesh"] and not row["nonlocal_contact_and_publication_checked"]
    stable = ("changed", "trials", "old_q_bad", "new_q_bad", "old_J_bad", "new_J_bad",
              "old_geometry_bad", "new_geometry_bad", "old_geometry_excess", "new_geometry_excess",
              "old_maximum_edge", "new_maximum_edge", "metric_valid", "replacement", "interface_valid")
    comparisons, cases = [], {}
    for match in matches:
        label = match["case"]
        current = [r for r in candidates if r["case"] == label]
        old = [r for r in baseline["candidate_results"] if r["case"] == label]
        for kind in {r["kind"] for r in old}:
            a, b = [r for r in old if r["kind"] == kind], [r for r in current if r["kind"] == kind]
            assert len(a) == len(b)
            assert all(all(x[k] == y[k] for k in stable) for x, y in zip(a, b)), (label, kind)
            comparisons.append({"case": label, "kind": kind, "unchanged_results": len(a)})
        original = [r for r in current if r["kind"] == "original_star"]
        assert len(original) == 4
        alternatives = [r for r in current if r["kind"] != "original_star"]
        cases[label] = {"useful_original_stars": sum(map(prior.useful, original)),
                        "alternative_candidates": len(alternatives),
                        "useful_alternatives": sum(map(prior.useful, alternatives))}
    assert cases["geometry_bad_1"]["alternative_candidates"] == 1
    assert cases["geometry_bad_1"]["useful_alternatives"] == 0
    assert before == {str(p): prior.sha(p) for p in paths}
    result = {"source_build": "build238", "production_changed": False, "inputs_sha256": before,
              "status": status, "cases": cases, "baseline237_comparisons": comparisons,
              "candidate_results": candidates, "raw_operator_results": [r for r in rows if "reason" in r],
              "decision": "Reject production integration of the tested one-face collapse/retained-star hypothesis: its sole constructed alternative removes one J failure but raises geometry excess. Do not broaden production support or add another local variant from this result. Next distinguish actual geometric displacement from conservative certification on the refused changed faces, using existing reference/query facilities and this captured context.",
              "limits": baseline["limits"]}
    (RUN / "current_residual238_assessment.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"cases": cases, "preserved_control_results": sum(r["unchanged_results"] for r in comparisons),
                      "decision": result["decision"]}, indent=2))


if __name__ == "__main__":
    main()
