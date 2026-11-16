#!/usr/bin/env python3
"""Postprocess N11 G3 physical receipts, run pairs, and timer comparisons."""
import argparse
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = HERE.parent
STAGED = N11.parents[1]
REPO = STAGED.parents[3]
BUILD = N11 / "build535"
PAIR_HELPER = N11 / "section6_8_preparation/drafts/check_n11_g3_pair.py"
TIMER_HELPER = N11 / "section6_8_preparation/drafts/compare_timer_census_delta_69.py"
GUARD_SOURCE = STAGED / "p7p8_r3_20261115/check_cheap_bl.py"


def load_module(name, path):
    """Import one pinned helper from its explicit evidence path."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def guard_source():
    """Adapt the existing physical guard to 6.9's authorized N11 predicates."""
    source = GUARD_SOURCE.read_text()
    removals = (
        "    gates['at_least_336_splits']=row.get('splits',-1)>=336\n",
        "    gates['first_336_edits_identical']=len(reference_edits)>=336 and len(actual_edits)>=336 and reference_edits[:336]==actual_edits[:336]\n",
    )
    for fragment in removals:
        assert source.count(fragment) == 1, fragment
        source = source.replace(fragment, "", 1)
    assert source.count("20_000_000_000") == 1
    source = source.replace("20_000_000_000", "19_999_490_048", 1)
    source = source.replace("whole_job_peak_RSS_20GB_serial",
                            "whole_job_peak_RSS_19073_MiB_serial")
    source = source.replace("20GB serial peak gate", "19073 MiB serial peak gate")
    return source


def load_physical_guard():
    """Load existing G3 checks while removing only withdrawn split predicates."""
    namespace = {"__name__": "n11_69_physical_guard", "__file__": str(GUARD_SOURCE)}
    source = guard_source()
    exec(compile(source, str(GUARD_SOURCE), "exec"), namespace)
    prior = namespace["prior"]
    namespace["self_check"]()
    original = prior.conditions

    def conditions(status, command, row, identical):
        gates = original(status, command, row, identical)
        gates.pop("at_least_300_splits", None)
        gates["serial_bounded_command"] = command.get("ranks") == 1 and command.get("timeout_seconds") == 900
        return gates

    prior.conditions = conditions
    status = {"state": "terminal", "exit_code": 0, "pins_verified": True, "changed": []}
    row = {"same_target_policy": True, "input_parameters_unchanged": True,
           "exact_transported_input": True, "captured": True, "maximum_seconds": 388.8,
           "maximum_work": 233220, "stage_height": 6.4e-5, "splits": 1,
           "output_cells": 1, "positive_cells": 1, "sensor_cells": 1,
           "maximum_work_bytes": 1}
    assert conditions(status, {"ranks": 1, "timeout_seconds": 900}, row, {})["serial_bounded_command"]
    assert not conditions(status, {"ranks": 1, "timeout_seconds": 600}, row, {})["serial_bounded_command"]
    return namespace


def physical_case(case):
    """Write the inherited physical receipt with the N11/N10 6.9 scope."""
    guard = load_physical_guard()
    failed = guard["main"](Path(case).resolve(strict=True))
    receipt = json.loads((Path(case) / "cheap_bl_receipt.json").read_text())
    receipt["n11_69_scope"] = {
        "split_count_is_report_only": receipt.get("actual_splits"),
        "withdrawn_gates": ["at_least_300_splits", "at_least_336_splits",
                            "first_336_edits_identical"],
        "retained_physical_gates": list(receipt["gates"]),
        "normal_return_required": True,
    }
    receipt["scope"] = (
        "N11/N10 physical validity only: terminal bounded command, frozen inputs/target/config, "
        "positive private cells with sensors, zero upper refinements, modeled storage within 12 GiB, "
        "job peak within 19073 MiB; split count and old edit identity are report-only.")
    receipt["passed"] = not failed and all(receipt["gates"].values())
    (Path(case) / "cheap_bl_receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def compare_pair(first, second):
    """Apply the exact N11 geometry-ratio and common-edit-prefix postcheck."""
    helper = load_module("n11_pair_helper", PAIR_HELPER)
    return helper.compare_pair(Path(first).resolve(strict=True), Path(second).resolve(strict=True))


def compare_timers(reference, candidate, gate):
    """Apply the two-sided law-line Census adjustment with strict other rows."""
    helper = load_module("n11_timer_helper", TIMER_HELPER)
    return helper.compare_cases(Path(reference), Path(candidate), gate)


def publish_build_aliases():
    """Verify a terminal N11 build and publish aliases for the reused checks."""
    alias_path = N11 / "build535_aliases.py"
    alias = load_module("n11_build535_aliases", alias_path)
    status_path = BUILD / "build_status.json"
    status = json.loads(status_path.read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    return alias.verify_and_publish()


def self_check():
    """Check helper path and function bindings without writing or launching work."""
    assert N11.name == "N11" and STAGED.name == "staged57"
    assert REPO.name == "SU2_NativeIntegrated" and BUILD.name == "build535"
    assert PAIR_HELPER.is_file() and TIMER_HELPER.is_file() and GUARD_SOURCE.is_file()
    alias = load_module("n11_alias_binding_check", N11 / "build535_aliases.py")
    assert alias.verify_and_publish.__globals__["HERE"] == HERE
    assert alias.verify_and_publish.__globals__["BUILD"] == BUILD
    guard = load_physical_guard()
    assert guard["main"].__globals__ is guard
    prior_original = guard["prior"].conditions.__closure__[0].cell_contents
    assert prior_original.__globals__["HERE"] == STAGED / "purpose_query_fix_20261114"
    pair = load_module("n11_pair_binding_check", PAIR_HELPER)
    timer = load_module("n11_timer_binding_check", TIMER_HELPER)
    assert pair.compare_pair.__globals__["IDENTITY_HELPER"].is_file()
    assert timer.compare_cases.__globals__["PARSER"].ROOT == REPO
    return {"passed": True, "launched": False, "aliases_written": False,
            "validation_root": str(HERE), "build_root": str(BUILD),
            "guard_source": str(GUARD_SOURCE), "pair_helper": str(PAIR_HELPER),
            "timer_helper": str(TIMER_HELPER), "build_alias_function": "verify_and_publish"}


def main():
    """Dispatch non-launching N11 receipt checks."""
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("self-check")
    physical = sub.add_parser("physical")
    physical.add_argument("case", type=Path)
    pair = sub.add_parser("pair")
    pair.add_argument("first", type=Path)
    pair.add_argument("second", type=Path)
    timers = sub.add_parser("timers")
    timers.add_argument("reference", type=Path)
    timers.add_argument("candidate", type=Path)
    timers.add_argument("--gate", choices=("all", "G2", "GQ"), default="all")
    args = parser.parse_args()
    if args.action == "self-check":
        result = self_check()
    elif args.action == "physical":
        result = physical_case(args.case)
    elif args.action == "pair":
        result = compare_pair(args.first, args.second)
    else:
        result = compare_timers(args.reference, args.candidate, args.gate)
    print(json.dumps(result, indent=2, default=str))
    if result.get("passed") is False:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
