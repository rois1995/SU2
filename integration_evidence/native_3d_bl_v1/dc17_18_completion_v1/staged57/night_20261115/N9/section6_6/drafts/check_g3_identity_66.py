#!/usr/bin/env python3
"""Draft G3 identity gate for section 6.6 rule 1; report other integers only."""
import argparse
import hashlib
import json
import re
import shlex
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[3]
NIGHT = STAGED / "night_20261115"
REFERENCE = NIGHT / "section6_1/cases/section6_1_bl_20261116T082025_3350201"
P6C = STAGED / "cases/p6c_bl_20261115T003316_3200535"
N9_CASE = NIGHT / "N9/cases/N9_bl_20261116T124507_4057925"
PHYSICAL_GATES = (
    "executed_successfully", "runtime_pins_verified", "serial_bounded_command",
    "exact_input_and_target", "unchanged_stage_limits", "at_least_300_splits",
    "all_private_cells_positive_with_sensors", "modeled_storage_within_12GiB",
    "same_frozen_input_contract", "same_frozen_configuration", "zero_upper_refinements",
    "at_least_336_splits", "first_336_edits_identical",
    "whole_job_peak_RSS_19073_MiB_serial")


def sha(path):
    """Hash a saved report input."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def key_values(line):
    """Parse space-delimited key/value fields while preserving quoted text."""
    fields = {}
    for token in shlex.split(line, posix=True):
        if "=" in token:
            key, value = token.split("=", 1)
            fields[key] = value
    return fields


def load_json(case, name):
    """Read a required case receipt."""
    return json.loads((case / name).read_text())


def records(case):
    """Read edit, observer, stage, and optional quoted wall-reason records."""
    edits, observers, stages, reasons, refusals = [], [], [], [], []
    log = case / "run.log"
    for line_no, line in enumerate(log.read_text(errors="replace").splitlines(), 1):
        if "NATIVE3D_STAGE " in line:
            fields = key_values(line[line.index("NATIVE3D_STAGE "):])
            stages.append({"line": line_no, **fields})
        if "NATIVE3D_WALL_REASON " in line:
            fields = key_values(line[line.index("NATIVE3D_WALL_REASON "):])
            reasons.append({"line": line_no, **fields})
        if "NATIVE3D_WALL_REFUSAL " in line:
            fields = key_values(line[line.index("NATIVE3D_WALL_REFUSAL "):])
            refusals.append({"line": line_no, **fields})
    for line in (case / "nose_target_observation.jsonl").read_text().splitlines():
        item = json.loads(line)
        if item.get("stage") == "accepted_split":
            edits.append({"edge": item["edge"], "point": item["point"]})
        if item.get("stage") == "fixed_summary" and item.get("side") == "before":
            observers.append(item)
    return edits, observers, stages, reasons, refusals


def stage_for(rows, stage, phase="construction"):
    """Select one rank-zero construction stage row by its exact label."""
    matches = [row for row in rows if row.get("rank") == "0" and
               row.get("stage") == stage and row.get("phase") == phase]
    assert len(matches) == 1, f"expected one rank-zero {stage} row, found {len(matches)}"
    return matches[0]


def accepted_count(case, stages):
    """Read the accepted split count from the saved row and cross-check stage output."""
    row = load_json(case, "actual_saved_surface_stage_rank0.json")
    exit_row = stage_for(stages, "exit")
    count = int(row["splits"])
    assert int(exit_row["accepted"]) == count
    return count, row


def physical_gates(case):
    """Reuse the existing cheap-BL physical gate without its all-integer identity test."""
    receipt = load_json(case, "cheap_bl_receipt.json")
    gates = receipt["gates"]
    missing = [name for name in PHYSICAL_GATES if gates.get(name) is not True]
    status = load_json(case, "status.json")
    terminal = status.get("state") == "terminal" and status.get("exit_code") == 0
    return {"passed": receipt.get("passed") is True and terminal and not missing,
            "terminal_exit_zero": terminal, "physical_receipt_passed": receipt.get("passed"),
            "required_gates": {name: gates.get(name) for name in PHYSICAL_GATES},
            "missing_or_failed": missing, "first336_p6c": gates.get("first_336_edits_identical"),
            "at_least_336_splits": gates.get("at_least_336_splits"),
            "receipt_sha256": sha(case / "cheap_bl_receipt.json")}


def p6c_prefix(candidate_edits):
    """Independently verify the existing physical first-336 P6c requirement."""
    reference_edits = []
    for line in (P6C / "nose_target_observation.jsonl").read_text().splitlines():
        row = json.loads(line)
        if row.get("stage") == "accepted_split":
            reference_edits.append({"edge": row["edge"], "point": row["point"]})
    count = min(336, len(candidate_edits), len(reference_edits))
    return {"passed": count == 336 and candidate_edits[:count] == reference_edits[:count],
            "compared_edits": count, "reference_count": len(reference_edits),
            "candidate_count": len(candidate_edits)}


def observer_adjusted_work(stages, observers):
    """Subtract the pre-PrepareSurface observer work from entry atlas_work."""
    entry = stage_for(stages, "surface_required_entry")
    assert len(observers) == 1, f"expected one pre-stage observer row, found {len(observers)}"
    observer = int(observers[0]["geometry_work"])
    atlas = int(entry["atlas_work"])
    return {"entry_atlas_work": atlas, "observer_geometry_work": observer,
            "observer_adjusted_entry_work": atlas - observer,
            "observer_source": "nose_target_observation.jsonl fixed_summary side=before geometry_work",
            "entry_source": "run.log NATIVE3D_STAGE surface_required_entry atlas_work"}


def integer_stage_rows(stages):
    """Return integer counters and vectors by stage; these are report-only under 6.6."""
    result = {}
    for row in stages:
        label = row.get("stage")
        if label not in ("surface_required_entry", "exit"):
            continue
        values = {}
        for key, value in row.items():
            if key in ("line", "stage", "phase"):
                continue
            if re.fullmatch(r"[+-]?\d+", str(value)):
                values[key] = int(value)
            elif "," in str(value) and all(re.fullmatch(r"[+-]?\d+", part)
                                           for part in str(value).split(",")):
                values[key] = [int(part) for part in str(value).split(",")]
        result[label] = values
    return result


def integer_report(reference, candidate):
    """Report every differing old integer stage field without gating on the result."""
    before, after = integer_stage_rows(reference), integer_stage_rows(candidate)
    differences = []
    for stage in sorted(before.keys() | after.keys()):
        left, right = before.get(stage, {}), after.get(stage, {})
        for key in sorted(left.keys() | right.keys()):
            if left.get(key) != right.get(key):
                differences.append({"stage": stage, "field": key,
                                   "reference": left.get(key), "candidate": right.get(key)})
    return {"gate": False, "meaning": "report only under 6.6 rule 1(c)",
            "reference_rows": before, "candidate_rows": after,
            "differences": differences}


def wall_reason_report(reasons, operator_refusals=None):
    """Preserve quoted reason text and reconcile counts only when source data exists."""
    if not reasons:
        return {"rows_present": False, "counts": None, "more": None,
                "sum": None, "operator_refusals": operator_refusals,
                "reconciled": None}
    counts = {}
    more = 0
    for row in reasons:
        count = int(row["count"])
        if row.get("text") in ("more", "<more>"):
            more += count
        else:
            counts[row["text"]] = counts.get(row["text"], 0) + count
    total = sum(counts.values()) + more
    return {"rows_present": True, "counts": counts, "more": more, "sum": total,
            "operator_refusals": operator_refusals,
            "reconciled": total == operator_refusals if operator_refusals is not None else None}


def wall_operator_refusal_total(rows):
    """Sum operator classes while excluding guidance and unwrapped trials."""
    if not rows:
        return None
    ignored = {"guidance_guard", "unwrapped_geometry_trial"}
    return sum(int(row["count"]) for row in rows if row.get("class") not in ignored)


def wall_excluded_refusal_counts(rows):
    """Report non-operator refusal classes excluded from the operator denominator."""
    ignored = {"guidance_guard", "unwrapped_geometry_trial"}
    return {name: sum(int(row["count"]) for row in rows if row.get("class") == name)
            for name in sorted(ignored) if any(row.get("class") == name for row in rows)}


def evaluate(reference, candidate):
    """Apply only the physical and 6.6 identity gates; retain other integers as reports."""
    reference_edits, reference_observer, reference_stages, reference_reasons, reference_refusals = records(reference)
    candidate_edits, candidate_observer, candidate_stages, candidate_reasons, candidate_refusals = records(candidate)
    reference_count, reference_row = accepted_count(reference, reference_stages)
    candidate_count, candidate_row = accepted_count(candidate, candidate_stages)
    common_count = min(reference_count, candidate_count)
    edits_same = (len(reference_edits) == reference_count and
                  len(candidate_edits) == candidate_count and
                  reference_edits[:common_count] == candidate_edits[:common_count])
    reference_work = observer_adjusted_work(reference_stages, reference_observer)
    candidate_work = observer_adjusted_work(candidate_stages, candidate_observer)
    adjusted_work_same = (reference_work["observer_adjusted_entry_work"] ==
                          candidate_work["observer_adjusted_entry_work"])
    physical = physical_gates(candidate)
    prefix = p6c_prefix(candidate_edits)
    report_only = integer_report(reference_stages, candidate_stages)
    reference_exit = stage_for(reference_stages, "exit")
    candidate_exit = stage_for(candidate_stages, "exit")
    prior = load_json(candidate, "g3_integer_comparison.json")
    return {"passed": physical["passed"] and prefix["passed"] and edits_same and adjusted_work_same,
            "reference": str(reference), "candidate": str(candidate),
            "physical_gates": physical, "first336_p6c_prefix": prefix,
            "g3_edit_identity": {"passed": edits_same, "reference_accepted": reference_count,
                                 "candidate_accepted": candidate_count,
                                 "compared_count": common_count,
                                 "reference_edits": len(reference_edits),
                                 "candidate_edits": len(candidate_edits)},
            "observer_adjusted_entry_work": {"passed": adjusted_work_same,
                                             "reference": reference_work,
                                             "candidate": candidate_work,
                                             "expected_method_value": 5113},
            "other_old_integer_rows": report_only,
            "prior_all_integer_comparison": {"path": str(candidate / "g3_integer_comparison.json"),
                "sha256": sha(candidate / "g3_integer_comparison.json"),
                "returncode": prior.get("returncode"),
                "first_difference": prior.get("comparison", {}).get("first_difference"),
                "gate_used": False},
            "wall_reason_diagnostics": wall_reason_report(
                candidate_reasons, wall_operator_refusal_total(candidate_refusals)),
            "reference_wall_reason_diagnostics": wall_reason_report(
                reference_reasons, wall_operator_refusal_total(reference_refusals)),
            "reported_counts": {"reference_splits": reference_count,
                                "candidate_splits": candidate_count,
                                "candidate_geometry_bad": int(candidate_exit["geometry_bad"]),
                                "candidate_size_bad": int(candidate_exit["size_bad"]),
                                "reference_geometry_bad": int(reference_exit["geometry_bad"]),
                                "reference_size_bad": int(reference_exit["size_bad"])},
            "excluded_wall_refusal_classes": {
                "candidate": wall_excluded_refusal_counts(candidate_refusals),
                "reference": wall_excluded_refusal_counts(reference_refusals)}}


def self_check():
    """Check the preserved N9 459-run against N1 and quoted-field parsing synthetically."""
    result = evaluate(REFERENCE, N9_CASE)
    assert result["physical_gates"]["passed"]
    assert result["first336_p6c_prefix"]["passed"]
    assert result["g3_edit_identity"]["passed"]
    assert result["g3_edit_identity"]["reference_accepted"] == 486
    assert result["g3_edit_identity"]["candidate_accepted"] == 459
    assert result["g3_edit_identity"]["compared_count"] == 459
    assert result["observer_adjusted_entry_work"]["passed"]
    assert result["observer_adjusted_entry_work"]["reference"]["observer_adjusted_entry_work"] == 5113
    assert result["observer_adjusted_entry_work"]["candidate"]["observer_adjusted_entry_work"] == 5113
    assert result["prior_all_integer_comparison"]["gate_used"] is False
    parsed = key_values('NATIVE3D_WALL_REASON text="orientation with \\"quoted\\" detail" count=7')
    assert parsed["text"] == 'orientation with "quoted" detail' and parsed["count"] == "7"
    reasons = wall_reason_report([{"text": parsed["text"], "count": parsed["count"]},
                                  {"text": "<more>", "count": "2"}], 9)
    assert reasons["sum"] == 9 and reasons["reconciled"] is True
    refusal_rows = [{"class": "guidance_guard", "count": "1190"},
                    {"class": "unwrapped_geometry_trial", "count": "0"},
                    {"class": "SplitEdge", "count": "912"}]
    assert wall_operator_refusal_total(refusal_rows) == 912
    assert wall_excluded_refusal_counts(refusal_rows) == {"guidance_guard": 1190,
                                                           "unwrapped_geometry_trial": 0}
    assert result["reported_counts"]["candidate_geometry_bad"] == 2456
    assert result["reported_counts"]["candidate_size_bad"] == 4293
    missing = wall_reason_report([], 9)
    assert missing["sum"] is None and missing["reconciled"] is None
    return {"passed": result["passed"], "launched": False,
            "reference_splits": 486, "candidate_splits": 459,
            "common_edits": 459, "observer_adjusted_work": 5113,
            "other_integer_differences_report_only": len(
                result["other_old_integer_rows"]["differences"])}


def main():
    """Write one read-only G3 identity report for an explicit candidate case."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, default=REFERENCE)
    parser.add_argument("--candidate", type=Path, default=N9_CASE)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    result = evaluate(args.reference.resolve(strict=True), args.candidate.resolve(strict=True))
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"passed": result["passed"], "common_edits": result["g3_edit_identity"]["compared_count"],
                      "observer_adjusted_entry_work_passed": result["observer_adjusted_entry_work"]["passed"],
                      "report_only_integer_differences": len(result["other_old_integer_rows"]["differences"])}))
    if not result["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
