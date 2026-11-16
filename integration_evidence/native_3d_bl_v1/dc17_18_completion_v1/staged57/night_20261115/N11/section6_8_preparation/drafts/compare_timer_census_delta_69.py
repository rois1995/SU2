#!/usr/bin/env python3
"""Draft exact two-sided Census-call comparison for the N11 B2 reference."""
from __future__ import annotations

import argparse
import importlib.util
import json
import sys
from pathlib import Path

STAGED = Path(__file__).resolve().parents[4]
BASE = STAGED / "night_20261115/N8/A2_A3_N12/section6_8/drafts/compare_integer_rows_68.py"
SPEC = importlib.util.spec_from_file_location("census_delta_68_parser", BASE)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot load existing timer parser: {BASE}")
PARSER = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PARSER
SPEC.loader.exec_module(PARSER)


def wall_law_before(snapshot):
    """Use the parser's rank-local law count reset at any root's final boundary."""
    return snapshot["law_lines_before"]


def compare_snapshot(reference, current):
    """Apply the exact per-boundary two-sided law-line adjustment."""
    if reference["key"] != current["key"]:
        return {"passed": False, "difference": "timer snapshot identity differs"}
    before, after = reference["summary"], current["summary"]
    if before.get("fixedstorage") != "4120" or after.get("fixedstorage") != "4120":
        return {"passed": False, "difference": "fixedstorage must remain 4120"}
    difference = PARSER.exact_fields(before, after, PARSER.SUMMARY_MEASUREMENTS)
    if difference:
        return {"passed": False, "difference": f"summary.{difference}"}
    old_census, new_census = [], []
    old_other = [row["fields"] for row in reference["rows"]
                 if row["fields"].get("operation") != "census"]
    new_other = [row["fields"] for row in current["rows"]
                 if row["fields"].get("operation") != "census"]
    for row in reference["rows"]:
        if row["fields"].get("operation") == "census":
            old_census.append(PARSER.parse_calls(row["fields"], row["path"], row["line"]))
    for row in current["rows"]:
        if row["fields"].get("operation") == "census":
            new_census.append(PARSER.parse_calls(row["fields"], row["path"], row["line"]))
    if len(old_other) != len(new_other):
        return {"passed": False, "difference": "non-census timer row count differs"}
    for index, (left, right) in enumerate(zip(old_other, new_other), 1):
        difference = PARSER.exact_fields(left, right, PARSER.TIMER_MEASUREMENTS)
        if difference:
            return {"passed": False, "difference": f"timer_row[{index}].{difference}"}
    old_calls, new_calls = sum(old_census), sum(new_census)
    old_law, new_law = wall_law_before(reference), wall_law_before(current)
    expected = old_calls + new_law - old_law
    return {"passed": new_calls == expected,
            "difference": None if new_calls == expected else
                f"census.calls {new_calls} != {old_calls} + {new_law} - {old_law}",
            "key": reference["key"], "reference_census_calls": old_calls,
            "candidate_census_calls": new_calls,
            "reference_wall_law_lines_before": old_law,
            "candidate_wall_law_lines_before": new_law,
            "expected_candidate_census_calls": expected}


def compare_timer_streams(reference, candidate, reference_cutoffs, candidate_cutoffs):
    """Compare timer snapshots without requiring either side to be pre-law."""
    if reference.keys() != candidate.keys():
        return {"passed": False, "first_difference": "rank stdout stream set differs"}
    parsed = {"reference": {}, "candidate": {}}
    for label in sorted(reference):
        rank = PARSER.stream_rank(candidate, label)
        for side, files, limits in (("reference", reference, reference_cutoffs),
                                    ("candidate", candidate, candidate_cutoffs)):
            PARSER.verify_law_ranks(files[label], rank, limits.get(label))
            parsed[side][label] = PARSER.parse_timer_stream(files[label], limits.get(label))
    rows = []
    for label in sorted(reference):
        old, new = parsed["reference"][label], parsed["candidate"][label]
        if len(old) != len(new):
            return {"passed": False, "first_difference": f"{label}: timer snapshot count differs"}
        for index, (left, right) in enumerate(zip(old, new), 1):
            result = compare_snapshot(left, right)
            rows.append({"stream": label, "snapshot": index, **result})
            if not result["passed"]:
                return {"passed": False, "first_difference": rows[-1], "snapshots": rows}
    return {"passed": True, "snapshots": rows,
            "reference_wall_law_lines": sum(PARSER.count_wall_law(path, reference_cutoffs.get(label))
                                             for label, path in reference.items()),
            "candidate_wall_law_lines": sum(PARSER.count_wall_law(path, candidate_cutoffs.get(label))
                                             for label, path in candidate.items())}


def compare_cases(reference_path, candidate_path, gate="all"):
    """Compare old integer rows plus the generalized timer Census rule."""
    old_rows, new_rows = PARSER.N4.streams(reference_path), PARSER.N4.streams(candidate_path)
    old_files, new_files = PARSER.source_files(reference_path), PARSER.source_files(candidate_path)
    old_cut = {name: PARSER.cutoff_lines(old_rows[name], gate) for name in old_rows}
    new_cut = {name: PARSER.cutoff_lines(new_rows[name], gate) for name in new_rows}
    old_plain = {name: PARSER.plain_prefix(rows, old_cut[name], name, old_files, gate)
                 for name, rows in old_rows.items()}
    new_plain = {name: PARSER.plain_prefix(rows, new_cut[name], name, new_files, gate)
                 for name, rows in new_rows.items()}
    integer = PARSER.N4.compare_streams(old_plain, new_plain, gate)
    timer = compare_timer_streams(old_files, new_files, old_cut, new_cut)
    return {"passed": integer["passed"] and timer["passed"], "gate": gate,
            "integer_rows": integer, "timer_rows": timer,
            "census_rule": "candidate calls - reference calls = candidate law lines - reference law lines per rank/boundary"}


def timer_block(boundary, census_calls, law_lines, other_calls=7, root="Adapt"):
    """Build one synthetic summary block for the parser's existing log format."""
    rows = [f'NATIVE3D_TIMER root={root} boundary={boundary} rank=0 operation=census calls={census_calls} exclusive_seconds=0.01',
            f'NATIVE3D_TIMER root={root} boundary={boundary} rank=0 operation=other calls={other_calls} exclusive_seconds=0.02']
    summary = (f'NATIVE3D_TIMER_SUMMARY root={root} boundary={boundary} rank=0 ranks=1 '
               'fixedstorage=4120 root_seconds=1.0 sumexclusive=0.1 unattributed=0.9 coverage=0.1')
    return [*("NATIVE3D_WALL_LAW rank=0 boundary=synthetic seconds=0" for _ in range(law_lines)),
            *rows, summary]


def self_check():
    """Test both-sided line deltas and strict old timer fields without launching work."""
    from tempfile import TemporaryDirectory
    old_lines = (timer_block("entry", 5, 1) + timer_block("final", 4, 1) +
                 timer_block("entry_again", 6, 1, root="native_engine") +
                 timer_block("final", 5, 1, root="native_engine"))
    new_lines = (timer_block("entry", 4, 0) + timer_block("final", 3, 1) +
                 timer_block("entry_again", 5, 0, root="native_engine") +
                 timer_block("final", 4, 1, root="native_engine"))
    with TemporaryDirectory() as directory:
        root = Path(directory)
        old_path, new_path = root / "old.log", root / "new.log"
        old_path.write_text("\n".join(old_lines) + "\n")
        new_path.write_text("\n".join(new_lines) + "\n")
        old = PARSER.parse_timer_stream(old_path)
        new = PARSER.parse_timer_stream(new_path)
        assert compare_snapshot(old[0], new[0])["passed"]
        assert compare_snapshot(old[1], new[1])["passed"]
        assert compare_snapshot(old[2], new[2])["passed"]
        assert compare_snapshot(old[3], new[3])["passed"]
        bad_calls = root / "bad_calls.log"
        bad_calls.write_text("\n".join(timer_block("entry", 5, 0)) + "\n")
        assert not compare_snapshot(old[0], PARSER.parse_timer_stream(bad_calls)[0])["passed"]
        bad_other = root / "bad_other.log"
        bad_other.write_text("\n".join(line.replace("operation=other calls=7", "operation=other calls=8")
                                          for line in new_lines) + "\n")
        assert not compare_snapshot(old[0], PARSER.parse_timer_stream(bad_other)[0])["passed"]
        reset_roots = [row["key"][0] for row in new]
    return {"passed": True, "launched": False,
            "two_sided_law_line_delta": "passed",
            "two_sessions_mixed_root_reset": reset_roots == ["Adapt", "Adapt", "native_engine", "native_engine"],
            "wrong_census_delta_rejected": True,
            "other_operation_integer_remains_strict": True}


def actual_gq_prefix_self_check():
    """Reuse the frozen physical truncation fixture with this comparator hooked in."""
    full = (PARSER.ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/"
            "staged57/night_20261115/section6_1/full_workflow")
    assert full.is_dir(), full
    original = PARSER.compare_cases
    PARSER.compare_cases = compare_cases
    try:
        result = PARSER.actual_truncated_prefix_check(full)
    finally:
        PARSER.compare_cases = original
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", nargs="?", type=Path)
    parser.add_argument("candidate", nargs="?", type=Path)
    parser.add_argument("--gate", choices=("all", "G2", "GQ"), default="all")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        result = self_check()
        result["actual_full_n1_vs_physically_truncated_gq_prefix"] = actual_gq_prefix_self_check()
        print(json.dumps(result, indent=2))
        return
    if args.reference is None or args.candidate is None:
        parser.error("reference and candidate case paths are required")
    result = compare_cases(args.reference, args.candidate, args.gate)
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"passed": result["passed"], "gate": result["gate"],
                      "timer_rows": result["timer_rows"].get("passed")}))
    if not result["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
