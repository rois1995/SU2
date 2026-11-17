#!/usr/bin/env python3
"""Enforce precisely C2.1's case/failure table; retain successful-assert totals as observations."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("frozen_g1_runner", HERE / "run_g1_candidate_gate.py")
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def table_gate(parser, reference, candidate):
    result = parser.compare(reference, candidate, set())
    return bool(result["identity_pass"]), result


def self_check():
    parser = base.load_module("exact_g1_parser_check", base.PARSER_PATH)
    case = {"rank": "0", "name": "old failure", "success": "false", "assertions": [
        {"success": "false", "type": "CHECK", "file": "a.cpp", "line": "12",
         "sections": [], "original": "a == b", "expanded": "3 == 4"}]}
    old = {"case_results": [case], "reporter_totals": [{"successes": "10"}]}
    candidate = copy.deepcopy(old)
    candidate["reporter_totals"][0]["successes"] = "28"
    assert table_gate(parser, old, candidate)[0]
    assert not table_gate(parser, old, candidate)[1]["reporter_totals_equal"]
    for key, value in (("success", "true"), ("name", "different case")):
        changed = copy.deepcopy(candidate)
        changed["case_results"][0][key] = value
        assert not table_gate(parser, old, changed)[0]
    for key, value in (("expanded", "3 == 5"), ("line", "13"), ("file", "b.cpp")):
        changed = copy.deepcopy(candidate)
        changed["case_results"][0]["assertions"][0][key] = value
        assert not table_gate(parser, old, changed)[0]
    assert not table_gate(parser, old, {"case_results": [], "reporter_totals": []})[0]
    return {"passed": True, "launched": False,
            "strict_fields": ["case name", "rank", "pass/fail", "failed assertion file/line/values"],
            "successful_assert_totals": "retained observations; not in C2.1 table"}


def evaluate(cohort, case_root, reference_root):
    parser = base.load_module("exact_g1_saved_parser", base.PARSER_PATH)
    manifest, references, _ = base.fresh_reference_bundle(reference_root, parser)
    case_root = Path(case_root).resolve(strict=True)
    original_path = case_root / f"G1_cut_candidate_{cohort}.json"
    original = json.loads(original_path.read_text())
    parsed_path = Path(original["candidate_report"])
    candidate = json.loads(parsed_path.read_text())
    expected = base.CASE_COUNTS[cohort]
    ranks = {"serial": 1, "mpi2": 2, "mpi4": 4}[cohort]
    names = {(row["rank"], row["name"]) for row in candidate["case_results"]}
    complete = len(names) == len(candidate["case_results"]) == ranks * expected
    assert candidate["ranks"] == ranks and original["case_records"] == ranks * expected
    passed, comparison = table_gate(parser, references[cohort], candidate)
    status = json.loads((Path(candidate["run"]) / "status.json").read_text())
    assert status["state"] == "terminal" and status["pins_verified"]
    pins = json.loads((Path(candidate["run"]) / "pins.json").read_text())
    assert all(base.sha(path) == digest for path, digest in pins.items())
    differences = [{"reference": old, "candidate": new}
                   for old, new in zip(references[cohort]["reporter_totals"], candidate["reporter_totals"])
                   if old != new]
    result = {"gate_passed": passed and complete, "written_rule": "CUT_PLAN_8_2.md section2.1 items1/2",
              "cohort": cohort, "case_records": len(names), "changed_case_count": len(comparison["unexpected_changes"]),
              "first_difference": next(iter(comparison["unexpected_changes"]), None),
              "zero_exceptions": True, "candidate_case": candidate["run"],
              "all_case_pins_unchanged": True, "pin_count": len(pins),
              "reporter_totals_equal": comparison["reporter_totals_equal"],
              "successful_assert_totals_are_report_only": True,
              "reporter_totals_differences": differences,
              "original_helper_result": str(original_path), "original_helper_result_sha256": base.sha(original_path),
              "candidate_report_sha256": base.sha(parsed_path),
              "reference_report_sha256": manifest["cohorts"][cohort]["report_sha256"],
              "validation_helpers": {str(path): base.sha(path) for path in [Path(__file__), Path(base.__file__), base.PARSER_PATH]},
              "numerical_run_repeated_for_correction": False}
    output = case_root / f"G1_C2_1_exact_{cohort}.json"
    assert not output.exists(), output
    output.write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cohort", choices=tuple(base.CASE_COUNTS), nargs="?")
    parser.add_argument("--build-root", type=Path)
    parser.add_argument("--case-root", type=Path)
    parser.add_argument("--reference-root", type=Path, default=base.PHASE / "reference_validation")
    parser.add_argument("--host-preflight", type=Path)
    parser.add_argument("--post-only", action="store_true")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    assert args.cohort and args.case_root
    if not args.post_only:
        assert args.build_root and args.host_preflight
        base.run_cohort(args.cohort, args.build_root, args.case_root, args.reference_root, args.host_preflight)
    result = evaluate(args.cohort, args.case_root, args.reference_root)
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result["gate_passed"] else 1)


if __name__ == "__main__":
    main()
