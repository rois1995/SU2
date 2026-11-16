#!/usr/bin/env python3
"""Postprocess the two N11 G3 runs; apply only the ratio and common-edit gates."""
import argparse
import importlib.util
import json
import re
import sys
from pathlib import Path

STAGED = Path(__file__).resolve().parents[4]
IDENTITY_HELPER = STAGED / "night_20261115/N9/section6_6/drafts/check_g3_identity_66.py"
GEOMETRY_CAUSES = ("incomplete", "nonfinite", "no_progress", "after_above", "exception", "other")
SOURCE_KINDS = ("primary", "RefitWallSplit", "ReconstructWallSplit", "ReconstructSplitPatch")


def load_identity():
    """Load the accepted G3 edit/stage parser without running its fixed-case checks."""
    spec = importlib.util.spec_from_file_location("n11_g3_identity_parser", IDENTITY_HELPER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fields(line, marker, identity):
    """Use the existing G3 helper's quoted key/value parser."""
    return identity.key_values(line[line.index(marker) + len(marker):])


def collect_source_row(row, rank, sources, issues, log, line_no):
    """Keep valid source counters and record schema problems as report-only."""
    source = row.get("source")
    if rank is None or source not in SOURCE_KINDS:
        issues.append({"line": line_no, "rank": rank, "source": source,
                       "problem": "missing rank or unknown source"})
        return
    bucket = sources.setdefault(rank, {})
    if source in bucket:
        issues.append({"line": line_no, "rank": rank, "source": source,
                       "problem": "duplicate source row"})
        return
    try:
        values = {key: int(row[key]) for key in ("validated", "guidance_rejected", "approved")}
    except (KeyError, ValueError):
        issues.append({"line": line_no, "rank": rank, "source": source,
                       "problem": "missing or malformed integer"})
        return
    if min(values.values()) < 0:
        issues.append({"line": line_no, "rank": rank, "source": source,
                       "problem": "negative integer"})
        return
    bucket[source] = values


def geometry_guidance(case, identity):
    """Collect strict geometry counters and report-only source rows per rank."""
    counts, rejects, sources, source_issues = {}, {}, {}, []
    log = case / "run.log"
    for line_no, line in enumerate(log.read_text(errors="replace").splitlines(), 1):
        if "NATIVE3D_WALL_COUNTS " in line:
            row = fields(line, "NATIVE3D_WALL_COUNTS ", identity)
            rank = row.get("rank")
            if rank is None or rank in counts:
                raise ValueError(f"{log}:{line_no}: missing or duplicate rank COUNTS row {rank}")
            try:
                counts[rank] = {key: int(row[key]) for key in
                                ("geometry_queue_guidance_trials", "guidance_trials",
                                 "guidance_rejected", "accepted")}
            except (KeyError, ValueError) as exc:
                raise ValueError(f"{log}:{line_no}: invalid NATIVE3D_WALL_COUNTS integer") from exc
            if min(counts[rank].values()) < 0:
                raise ValueError(f"{log}:{line_no}: negative NATIVE3D_WALL_COUNTS value")
        elif "NATIVE3D_WALL_GUIDANCE " in line:
            row = fields(line, "NATIVE3D_WALL_GUIDANCE ", identity)
            if row.get("origin") != "geometry":
                continue
            rank, cause = row.get("rank"), row.get("cause")
            if rank is None or cause not in GEOMETRY_CAUSES:
                raise ValueError(f"{log}:{line_no}: invalid geometry guidance row {row}")
            bucket = rejects.setdefault(rank, {})
            if cause in bucket:
                raise ValueError(f"{log}:{line_no}: duplicate geometry cause {cause} for rank {rank}")
            try:
                count = int(row["count"])
            except (KeyError, ValueError) as exc:
                raise ValueError(f"{log}:{line_no}: invalid rejection count") from exc
            if count < 0:
                raise ValueError(f"{log}:{line_no}: negative rejection count")
            bucket[cause] = count
        elif "NATIVE3D_WALL_SOURCE " in line:
            row = fields(line, "NATIVE3D_WALL_SOURCE ", identity)
            collect_source_row(row, row.get("rank"), sources, source_issues, log, line_no)
    if not counts:
        raise ValueError(f"{log}: no NATIVE3D_WALL_COUNTS rows")
    if set(counts) != set(rejects):
        raise ValueError(f"{log}: COUNTS/guidance rank mismatch {set(counts)} != {set(rejects)}")
    result = {}
    for rank, old in counts.items():
        if set(rejects[rank]) != set(GEOMETRY_CAUSES):
            raise ValueError(f"{log}: incomplete geometry causes for rank {rank}: {rejects[rank]}")
        missing_sources = set(SOURCE_KINDS) - set(sources.get(rank, {}))
        source_issues.extend({"rank": rank, "source": source, "problem": "missing source row"}
                             for source in sorted(missing_sources))
        rejected = sum(rejects[rank].values())
        trials = old["geometry_queue_guidance_trials"]
        result[rank] = {**old,
                        "geometry_origin_rejections": rejected,
                        "rejections_by_cause": rejects[rank],
                        "source_rows": sources.get(rank, {}),
                        "source_row_diagnostics": [item for item in source_issues
                                                    if item.get("rank") in (rank, None)],
                        "ratio": rejected / trials if trials else None,
                        "strictly_below_0_70": trials > 0 and rejected * 10 < trials * 7}
    for issue in source_issues:
        if issue.get("rank") not in counts:
            result[next(iter(counts))]["source_row_diagnostics"].append(issue)
    return result


def source_report(per_rank):
    """Compare source sums with existing totals as report-only differences."""
    sums = {source: {key: 0 for key in ("validated", "guidance_rejected", "approved")}
            for source in SOURCE_KINDS}
    old = {key: 0 for key in ("guidance_trials", "guidance_rejected", "accepted")}
    for row in per_rank.values():
        for source, values in row["source_rows"].items():
            for key, value in values.items():
                sums[source][key] += value
        for key in old:
            old[key] += row[key]
    totals = {key: sum(values[key] for values in sums.values())
              for key in ("validated", "guidance_rejected", "approved")}
    primary = sums["primary"]
    diagnostics = [issue for row in per_rank.values()
                   for issue in row["source_row_diagnostics"]]
    return {"by_source": sums, "source_sums": totals, "existing_totals": old,
            "differences_source_minus_existing": {
                "validated_minus_guidance_trials": totals["validated"] - old["guidance_trials"],
                "guidance_rejected_minus_existing": totals["guidance_rejected"] - old["guidance_rejected"],
                "approved_minus_accepted": totals["approved"] - old["accepted"]},
            "source_sum_difference_is_report_only": True,
            "source_row_diagnostics": diagnostics,
            "source_rows_complete": not diagnostics,
            "primary_guidance_rejection_ratio": (primary["guidance_rejected"] / primary["validated"]
                                                  if primary["validated"] else None)}


def run_summary(case, identity):
    """Read one run's edit stream and aggregate the strict geometry-origin ratio."""
    edits, _, stages, _, _ = identity.records(case)
    accepted, _ = identity.accepted_count(case, stages)
    if len(edits) != accepted:
        raise ValueError(f"{case}: edit stream has {len(edits)} rows, accepted count is {accepted}")
    per_rank = geometry_guidance(case, identity)
    trials = sum(row["geometry_queue_guidance_trials"] for row in per_rank.values())
    rejected = sum(row["geometry_origin_rejections"] for row in per_rank.values())
    return {"case": str(case), "accepted_edits": accepted, "geometry_queue_guidance_trials": trials,
            "geometry_origin_rejections": rejected,
            "geometry_origin_rejection_ratio": rejected / trials if trials else None,
            "ratio_strictly_below_0_70": trials > 0 and rejected * 10 < trials * 7,
            "by_rank": per_rank, "source_report": source_report(per_rank), "_edits": edits}


def compare_pair(first, second, identity=None):
    """Evaluate both strict purpose ratios and the common edge/point edit prefix."""
    identity = identity or load_identity()
    left, right = run_summary(Path(first), identity), run_summary(Path(second), identity)
    common = min(left["accepted_edits"], right["accepted_edits"])
    mismatch = next((i for i in range(common) if left["_edits"][i] != right["_edits"][i]), None)
    result = {"passed": left["ratio_strictly_below_0_70"] and right["ratio_strictly_below_0_70"] and mismatch is None,
              "purpose_gate": "in each run geometry-origin guidance rejections / geometry-queue guidance trials < 0.70",
              "first_run": {key: value for key, value in left.items() if key != "_edits"},
              "second_run": {key: value for key, value in right.items() if key != "_edits"},
              "common_edit_prefix": {"passed": mismatch is None, "compared_count": common,
                  "first_accepted": left["accepted_edits"], "second_accepted": right["accepted_edits"],
                  "first_mismatch_index": mismatch,
                  "first_mismatch": ({"first": left["_edits"][mismatch], "second": right["_edits"][mismatch]}
                                     if mismatch is not None else None)},
              "other_gates": "not evaluated by this postprocessor",
              "physical_gate_note": "N11/N10 validity requires private cells positive with sensors, zero upper refinements, modeled storage and job peak within limits, and no validity failure. At least 336 splits and first-336 P6c identity are not gates for N11/N10. This checker does not evaluate physical validity; N11 split count is report-only."}
    return result


def write_synthetic_case(path, edits, ratio_rejected, ratio_trials):
    """Write a minimal serial case with complete rank-zero diagnostic rows."""
    path.mkdir()
    rows = [f"NATIVE3D_STAGE rank=0 stage=exit phase=construction accepted={len(edits)}",
            f"NATIVE3D_WALL_COUNTS rank=0 geometry_queue_guidance_trials={ratio_trials} "
            f"guidance_trials={ratio_trials} guidance_rejected={ratio_rejected} accepted={len(edits)}"]
    causes = {cause: 0 for cause in GEOMETRY_CAUSES}
    if ratio_rejected:
        causes["incomplete"] = ratio_rejected
    rows.extend(f"NATIVE3D_WALL_GUIDANCE rank=0 origin=geometry cause={cause} count={count}"
                for cause, count in causes.items())
    rows.extend(f"NATIVE3D_WALL_GUIDANCE rank=0 origin=size cause={cause} count=0"
                for cause in GEOMETRY_CAUSES)
    source_counts = {"primary": (5, 3, 2), "RefitWallSplit": (2, 1, 1),
                     "ReconstructWallSplit": (1, 0, 1), "ReconstructSplitPatch": (2, 1, 1)}
    rows.extend(f"NATIVE3D_WALL_SOURCE rank=0 source={source} validated={values[0]} "
                f"guidance_rejected={values[1]} approved={values[2]}"
                for source, values in source_counts.items())
    (path / "run.log").write_text("\n".join(rows) + "\n")
    (path / "actual_saved_surface_stage_rank0.json").write_text(json.dumps({"splits": len(edits)}) + "\n")
    observations = [{"stage": "accepted_split", "edge": edge, "point": point} for edge, point in edits]
    (path / "nose_target_observation.jsonl").write_text("".join(json.dumps(row) + "\n" for row in observations))


def self_check():
    """Exercise strict result gates and report-only source-row diagnostics."""
    import tempfile
    identity = load_identity()
    common = [([1, 2], [0.1, 0.2, 0.3]), ([3, 4], [0.4, 0.5, 0.6])]
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_synthetic_case(root / "a", common + [([5, 6], [0.7, 0.8, 0.9])], 6, 10)
        write_synthetic_case(root / "b", common, 2, 10)
        passed = compare_pair(root / "a", root / "b", identity)
        assert passed["passed"] and passed["common_edit_prefix"]["compared_count"] == 2
        assert passed["first_run"]["geometry_origin_rejection_ratio"] == 0.6
        report = passed["first_run"]["source_report"]
        assert report["primary_guidance_rejection_ratio"] == 0.6
        assert report["source_sum_difference_is_report_only"]
        assert report["differences_source_minus_existing"]["approved_minus_accepted"] != 0
        write_synthetic_case(root / "boundary", common, 7, 10)
        assert not run_summary(root / "boundary", identity)["ratio_strictly_below_0_70"]
        write_synthetic_case(root / "mismatch", [([1, 2], [0.1, 0.2, 0.3]),
                                                  ([3, 5], [0.4, 0.5, 0.6])], 1, 10)
        assert not compare_pair(root / "b", root / "mismatch", identity)["common_edit_prefix"]["passed"]
        log = (root / "missing" / "run.log")
        write_synthetic_case(root / "missing", common, 1, 10)
        log.write_text("\n".join(line for line in log.read_text().splitlines()
                                   if "cause=other" not in line) + "\n")
        try:
            geometry_guidance(root / "missing", identity)
        except ValueError as exc:
            assert "incomplete geometry causes" in str(exc)
        else:
            raise AssertionError("missing cause row accepted")
        duplicate = root / "duplicate"
        write_synthetic_case(duplicate, common, 1, 10)
        with (duplicate / "run.log").open("a") as stream:
            stream.write("NATIVE3D_WALL_COUNTS rank=0 geometry_queue_guidance_trials=10\n")
        try:
            geometry_guidance(duplicate, identity)
        except ValueError as exc:
            assert "duplicate rank COUNTS" in str(exc)
        else:
            raise AssertionError("duplicate rank row accepted")
        missing_source = root / "missing_source"
        write_synthetic_case(missing_source, common, 1, 10)
        source_log = missing_source / "run.log"
        source_log.write_text("\n".join(line for line in source_log.read_text().splitlines()
                                           if "source=primary" not in line) + "\n")
        missing_result = compare_pair(root / "b", missing_source, identity)
        assert missing_result["passed"]
        missing_report = missing_result["second_run"]["source_report"]
        assert not missing_report["source_rows_complete"]
        assert missing_report["primary_guidance_rejection_ratio"] is None
        duplicate_source = root / "duplicate_source"
        write_synthetic_case(duplicate_source, common, 1, 10)
        with (duplicate_source / "run.log").open("a") as stream:
            stream.write("NATIVE3D_WALL_SOURCE rank=0 source=primary validated=1 guidance_rejected=0 approved=1\n")
        duplicate_result = compare_pair(root / "b", duplicate_source, identity)
        assert duplicate_result["passed"]
        assert any(issue["problem"] == "duplicate source row"
                   for issue in duplicate_result["second_run"]["source_report"]["source_row_diagnostics"])
    return {"passed": True, "launched": False, "strict_ratio_boundary_rejected": True,
            "common_prefix_identity_checked": True, "missing_and_duplicate_guidance_rejected": True,
            "source_rows_parsed_and_primary_ratio_reported": True,
            "source_sum_differences_report_only": True,
            "missing_and_duplicate_source_rows_report_only": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", nargs="?", type=Path)
    parser.add_argument("second", nargs="?", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if args.first is None or args.second is None:
        parser.error("two G3 case directories are required")
    result = compare_pair(args.first.resolve(strict=True), args.second.resolve(strict=True))
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"passed": result["passed"],
                      "first_ratio": result["first_run"]["geometry_origin_rejection_ratio"],
                      "second_ratio": result["second_run"]["geometry_origin_rejection_ratio"],
                      "first_primary_ratio": result["first_run"]["source_report"]["primary_guidance_rejection_ratio"],
                      "second_primary_ratio": result["second_run"]["source_report"]["primary_guidance_rejection_ratio"],
                      "first_source_differences_report_only": result["first_run"]["source_report"]["differences_source_minus_existing"],
                      "second_source_differences_report_only": result["second_run"]["source_report"]["differences_source_minus_existing"],
                      "common_edits": result["common_edit_prefix"]["compared_count"],
                      "first_mismatch_index": result["common_edit_prefix"]["first_mismatch_index"]}))
    if not result["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
