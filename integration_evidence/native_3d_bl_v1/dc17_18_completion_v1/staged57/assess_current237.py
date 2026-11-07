"""Consolidate existing operators on five complete terminal237 contexts."""
from pathlib import Path
import importlib.util
import json

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build237_signed_bounds_real_cfd50k"
ATTEMPTS = [ROOT / "cases" / name for name in (
    "current_compound_20261107T073703_3082067",
    "current_residual_20261107T074059_3095033")]
spec = importlib.util.spec_from_file_location("prior_assessment", ROOT / "assess_current235.py")
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)


def main():
    paths = [Path(__file__), ROOT / "assess_current235.py",
             RUN / "support237_coverage_assessment.json",
             RUN / "current_witnesses_v1/witness_preparation.json",
             RUN / "current_controls_v1/witness_preparation.json",
             RUN / "current_controls_actual_input_pins.json"]
    statuses, rows = [], []
    for attempt in ATTEMPTS:
        status = json.loads((attempt / "status.json").read_text())
        assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
        assert json.loads((attempt / "terminal_handles.json").read_text())["all_absent"]
        statuses.append(status)
        paths += [attempt / name for name in ("status.json", "pins.json", "command.json",
                                             "terminal_handles.json", "native_current_residual_result.jsonl")]
        rows.extend(json.loads(line) for line in (attempt / "native_current_residual_result.jsonl").read_text().splitlines())
    supplemental = json.loads((RUN / "current_controls_actual_input_pins.json").read_text())
    assert supplemental["state"] == "terminal" and supplemental["pins_verified"]
    assert all(prior.sha(Path(p)) == h for p, h in supplemental["pins_from_prelaunch_preparation"].items())
    before = {str(p): prior.sha(p) for p in paths}
    matches = [row for row in rows if row["stage"] == "current_metric_match"]
    assert len(matches) == 5 and len({row["case"] for row in matches}) == 5
    assert all(row["q_error"] == row["J_error"] == 0 for row in matches)
    candidates = [row for row in rows if row["stage"] == "bounded_star_result"]
    for row in candidates:
        for stem in ("q", "J", "geometry"):
            assert row[stem + "_retired"] - row[stem + "_introduced"] == row["old_" + stem + "_bad"] - row["new_" + stem + "_bad"]
        assert not row["accepted_mesh"] and not row["nonlocal_contact_and_publication_checked"]
    cases = {}
    for match in matches:
        label = match["case"]
        trials = [row for row in candidates if row["case"] == label]
        original = [row for row in trials if row["kind"] == "original_star"]
        assert len(original) == 4
        alternatives = [row for row in trials if row["kind"] != "original_star"]
        cases[label] = {"original_star_trials": 4, "useful_original_stars": sum(map(prior.useful, original)),
                        "alternative_candidates": len(alternatives),
                        "useful_alternatives": sum(map(prior.useful, alternatives)),
                        "old_context_geometry_bad": original[0]["old_geometry_bad"]}
    assert before == {str(p): prior.sha(p) for p in paths}
    result = {"source_build": "build237", "production_changed": False,
              "inputs_sha256": before, "statuses": statuses, "cases": cases,
              "candidate_results": candidates, "raw_operator_results": [r for r in rows if "reason" in r],
              "limits": "Alternatives overlap: do not add their gains. These final residuals are not historical replay of the earlier blocked44q/17J tail. Fresh associations/current context-only nodal tensors; old q/J match exactly, captured edge equality disabled and candidate edges freshly checked. No nonlocal publication, nonzero requested-height, strict mesh, transfer or restart proof. Geometry-clean labels describe cores, not all neighbors. Counted work is distinct from overlapping timers and cold geometry setup."}
    (RUN / "current_residual_assessment.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(cases, indent=2))


if __name__ == "__main__":
    main()
