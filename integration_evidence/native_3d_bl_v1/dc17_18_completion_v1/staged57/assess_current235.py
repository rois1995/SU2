"""Consolidate the completed bounded235 cavity and exact-query evidence."""
from collections import Counter
from fractions import Fraction
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build235_geometry_coupled_real_cfd50k"
CAVITY = ROOT / "cases/current_compound_20261107T052520_2804746"
REPLAY = ROOT / "cases/geometry_current_queries_20261107T053107_2817419"


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 << 20), b""):
            h.update(block)
    return h.hexdigest()


def useful(row):
    return (row["metric_valid"] and row["replacement"] and row["interface_valid"]
            and row["new_q_bad"] <= row["old_q_bad"] and row["new_J_bad"] <= row["old_J_bad"]
            and (row["new_q_bad"] < row["old_q_bad"] or row["new_J_bad"] < row["old_J_bad"])
            and row["new_geometry_excess"] <= row["old_geometry_excess"])


def main():
    paths = [Path(__file__), RUN / "query235_trace_assessment.json", RUN / "current_witnesses_v1/witness_preparation.json"]
    for attempt in (CAVITY, REPLAY):
        status = json.loads((attempt / "status.json").read_text())
        assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
        paths += [attempt / name for name in ("status.json", "pins.json", "run.log")]
    paths += [CAVITY / "native_current_residual_result.jsonl", REPLAY / "m6_surface_queries.json"]
    before = {str(p): sha(p) for p in paths}
    rows = [json.loads(line) for line in (CAVITY / "native_current_residual_result.jsonl").read_text().splitlines()]
    matches = [row for row in rows if row["stage"] == "current_metric_match"]
    assert len(matches) == 7 and all(row["q_error"] == row["J_error"] == 0 for row in matches)
    candidates = [row for row in rows if row["stage"] == "bounded_star_result"]
    for row in candidates:
        for stem in ("q", "J", "geometry"):
            assert row[stem + "_retired"] - row[stem + "_introduced"] == row["old_" + stem + "_bad"] - row["new_" + stem + "_bad"]
        assert not row["accepted_mesh"] and not row["nonlocal_contact_and_publication_checked"]
    cases = {}
    for label in sorted(row["case"] for row in matches):
        trials = [row for row in candidates if row["case"] == label]
        original = [row for row in trials if row["kind"] == "original_star"]
        assert len(original) == 4
        cases[label] = {"original_star_trials": 4, "useful_original_stars": sum(map(useful, original)),
                        "alternative_candidates": len(trials) - 4,
                        "useful_alternatives": sum(useful(row) for row in trials if row["kind"] != "original_star"),
                        "old_context_geometry_bad": original[0]["old_geometry_bad"]}
    assert all(cases[f"geometry_clean_{i}"]["useful_original_stars"] > 0 for i in range(5))
    assert all(cases[f"geometry_bad_{i}"]["useful_original_stars"] == 0 for i in (5, 6))
    trace = json.loads((RUN / "query235_trace_assessment.json").read_text())
    replay = json.loads((REPLAY / "m6_surface_queries.json").read_text())
    assert len(replay["queries"]) == len(trace["selected_queries"]) == 12
    pairs = []
    for i, pair in enumerate(trace["selected_replay_pairs"]):
        old, new = trace["selected_queries"][2*i:2*i+2]
        a, b = replay["queries"][2*i:2*i+2]
        assert old["facets"] == new["facets"] and old["tolerance"] == new["tolerance"] == replay["tolerance"]
        shared = set(map(tuple, old["points"])) & set(map(tuple, new["points"]))
        assert len(shared) == 2
        p, = set(map(tuple, old["points"])) - shared
        q, = set(map(tuple, new["points"])) - shared
        squared = sum((Fraction(x)-Fraction(y))**2 for x, y in zip(p, q))
        slack = Fraction(replay["tolerance"]) - Fraction(a["bound"])
        pairs.append(dict(pair, old_bound=a["bound"], new_bound=b["bound"], remaining_slack=float(slack),
                          exact_slack_proof=slack >= 0 and squared <= slack*slack,
                          warm_work=b["work"], warm_traversal_work=b["traversal_work"],
                          exact_repeat_work=b["repeat_work"], recomputed_work=b["recomputed_work"]))
    assert not any(p["exact_slack_proof"] for p in pairs)
    graph = Counter()
    for line in (CAVITY / "run.log").open():
        if line.startswith("NATIVE_SURFACE_GRAPH "):
            match = re.search(r"coverage=(\d+) bound=(\S+) coverage_work=(\d+) lift_work=(\d+)", line)
            assert match
            graph["calls"] += 1
            graph["covered"] += int(match[1]) > 0
            graph["coverage_work"] += int(match[3])
            graph["lift_work"] += int(match[4])
    result = {"source_build": "build235", "production_changed": False, "inputs_sha256": before,
              "cavity_status": json.loads((CAVITY / "status.json").read_text()),
              "query_replay_status": json.loads((REPLAY / "status.json").read_text()),
              "cases": cases, "candidate_results": candidates, "raw_operator_results": [r for r in rows if "reason" in r],
              "nearby_query_pairs": pairs, "graph_work_all_diagnostic_queries": dict(graph),
              "warm_selected_query_work": sum(p["warm_work"] for p in pairs),
              "warm_selected_traversal_work": sum(p["warm_traversal_work"] for p in pairs),
              "decision": "Do not broaden four-star preflight merely from eligibility: all five excluded clean cores already have useful original-star controls. No slack cache selected: zero of six expensive nearby pairs pass the conservative movement proof. Continue this same cycle with finite-footprint/traversal analysis using existing geometry structures, not a new operator, representation or construction policy.",
              "limits": "Alternatives overlap and cannot be summed as population gains. Fresh associations/current local nodal tensors, not historical execution replay. Geometry-clean labels describe the core, not neighbors. Trace timings include diagnostic overhead; graph scopes overlap. Local metric/interface/replacement checks do not establish nonlocal publication, requested nonzero heights, final acceptance, conservative transfer or disk restart."}
    assert before == {str(p): sha(p) for p in paths}
    (ROOT / "current235_residual_query_assessment.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"cases": cases, "warm_work": result["warm_selected_query_work"], "warm_traversal": result["warm_selected_traversal_work"], "decision": result["decision"]}))


if __name__ == "__main__":
    main()
