"""Reuse complete-census counters for the frozen260 attempt and same-target256 comparison."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parent
source = ROOT / "analyze_m6254_frozen.py"
code = source.read_text()
changes = [
    ("a=r/'m6_runs/euler_build254_conditioned_mach_only_real_cfd50k'", "a=r/'m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k'"),
    ("m6_runs/euler_build253_witness_cutoff_mach_only_real_cfd50k/quality_progress.json", "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k/quality_progress.json"),
    ("Replace BOTH literal244 attempt paths with254", "Replace BOTH literal244 attempt paths with260"),
    ("live_quality254_analysis_provenance.json", "live_quality260_analysis_provenance.json"),
    ("round254", "round260"), ("round253", "round256"),
    ("quality_seconds254", "quality_seconds260"), ("quality_seconds253", "quality_seconds256"),
    ("gap254_minus253", "gap260_minus256"),
    ("current254", "current260"), ("previous253", "previous256"),
    ("quality_comparison253.json", "quality_comparison256.json"),
    ("Corrected target changes workload;no same-query or isolated-speedup claim.",
     "Initial CFD fields,target tensors,configuration and host cache quota match256;late geometry-priority decisions change workload. No same-query or isolated-speedup claim under host contention."),
]
for old, new in changes:
    assert code.count(old) == 1, old
    code = code.replace(old, new)
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
before = {str(p): sha(p) for p in (source, Path(__file__).resolve())}
exec(compile(code, str(source), "exec"), {"__file__": str(source.resolve()), "__name__": "frozen260_counter_analysis"})
assert before == {p: sha(Path(p)) for p in before}
attempt = ROOT / "m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k"
receipt = {"input_hashes_before": before, "input_hashes_after": before,
           "transformed_sha256": hashlib.sha256(code.encode()).hexdigest(), "changes": changes,
           "scope": "Read-only existing first-event counters;whole-field counts require complete flags. Nearest complete quality times are measured,not interpolated;later phase transitions remain separate. No acceptance or timing causality claim."}
(attempt / "frozen260_counter_reuse_provenance.json").write_text(json.dumps(receipt, indent=2) + "\n")

# Isolate advisory scoring between an unchanged complete quality exit and construction entry.
progress = json.loads((attempt / "construction_progress_assessment.json").read_text())
exits = [e for e in progress["events"] if e["stage"] == "quality_exit"]
if exits:
    exit_row = exits[0]
    entries = [e for e in progress["events"] if e["stage"] == "construction_entry"
               and e["round"] == exit_row["round"] and e["phase"] == "construction"]
    if entries:
        entry_row = entries[0]
        assert exit_row["totals"] == entry_row["totals"]
        assert exit_row["complete_flags"] == entry_row["complete_flags"]
        assert all(exit_row["complete_flags"].values())
        assert exit_row["replicated_global_work"] == entry_row["replicated_global_work"]
        baseline = ROOT / "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k/construction_progress_assessment.json"
        old = json.loads(baseline.read_text())
        old_index = {(e["stage"], e["round"], e["phase"]): e for e in old["events"]}
        matched = []
        for event in progress["events"]:
            key = (event["stage"], event["round"], event["phase"])
            control = old_index[key]
            assert event["totals"] == control["totals"]
            assert event["complete_flags"] == control["complete_flags"]
            for rank, row in event["rank_rows"].items():
                assert all(value == control["rank_rows"][rank][name] for name, value in row.items()
                           if not any(word in name for word in ("seconds", "elapsed", "bytes")))
            matched.append(key)
            if event is exit_row:
                break
        keys = ("guidance_trials", "guidance_faces", "guidance_seconds", "upper_calls", "upper_work",
                "upper_seconds", "scope_build_work", "scope_build_seconds", "facet_build_work",
                "facet_build_seconds", "support_calls", "support_seconds", "traversal_work",
                "traversal_seconds", "memo_union_hits", "memo_union_misses")
        deltas = {}
        for rank, row in entry_row["rank_rows"].items():
            previous = exit_row["rank_rows"][rank]
            assert all(row[k] == previous[k] for k in ("accepted", "length_splits", "length_collapses"))
            deltas[rank] = {k: (float(row[k]) - float(previous[k]) if "seconds" in k
                               else int(row[k]) - int(previous[k])) for k in keys}
            assert all(v >= 0 for v in deltas[rank].values())
        trials = sum(d["guidance_trials"] for d in deltas.values())
        faces = sum(d["guidance_faces"] for d in deltas.values())
        assert trials > 0 and faces > 0
        result = {"scope": "Existing counters isolate advisory scoring before any construction publication. Timers overlap; no speedup, positive-score count, acceptance or final closure claim.",
                  "scoring_path_measured": True, "positive_scored_edges": None,
                  "matched_pre_priority_censuses": matched, "exit": exit_row, "entry": entry_row,
                  "rank_scoring_deltas": deltas, "scoring_trials": trials, "evaluated_faces": faces,
                  "source_prefixes": progress["source_prefixes"], "baseline_source_prefixes": old["source_prefixes"],
                  "analysis_sha256": sha(Path(__file__).resolve())}
        (attempt / "geometry_priority_phase_use.json").write_text(json.dumps(result, indent=2) + "\n")

# First geometry return comparison uses complete fresh shape/geometry censuses.
if exits:
    next_entries = [e for e in progress["events"] if e["stage"] == "quality_entry" and e["round"] > exits[0]["round"]]
    if next_entries:
        baseline = ROOT / "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k/construction_progress_assessment.json"
        old = json.loads(baseline.read_text())
        old_exit = next(e for e in old["events"] if e["stage"] == "quality_exit")
        old_entry = next(e for e in old["events"] if e["stage"] == "quality_entry" and e["round"] > old_exit["round"])
        rows = {}
        for label, before, after in (("build260", exits[0], next_entries[0]), ("build256", old_exit, old_entry)):
            assert all(before["complete_flags"].values()) and all(after["complete_flags"].values())
            work_keys = ("guidance_trials", "guidance_faces", "upper_calls", "upper_work", "traversal_work",
                         "scope_build_work", "facet_build_work", "length_splits", "accepted")
            delta = {k: sum(int(row[k]) for row in after["rank_rows"].values()) -
                        sum(int(row[k]) for row in before["rank_rows"].values()) for k in work_keys}
            rows[label] = {"exit_round": before["round"], "entry_round": after["round"],
                           "before": before["totals"], "after": after["totals"],
                           "native_seconds": after["maximum_native_elapsed"] - before["maximum_native_elapsed"],
                           "counted_work": delta,
                           "net_change": {k: after["totals"][k] - before["totals"][k] for k in ("cells", "q_bad", "j_bad", "geometry_bad", "size_bad")}}
        comparison = {"scope": "Matched first geometry-return stage from identical pre-priority whole-field state. Counted work is measured; walltime is not isolated speedup evidence. Net shape increases are not gross introduction counts. Subsequent reclosure and final acceptance remain unproved.",
                      "stages": rows, "source_prefixes260": progress["source_prefixes"],
                      "source_prefixes256": old["source_prefixes"], "analysis_sha256": sha(Path(__file__).resolve())}
        (attempt / "first_geometry_return_comparison256.json").write_text(json.dumps(comparison, indent=2) + "\n")
