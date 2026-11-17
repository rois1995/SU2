#!/usr/bin/env python3
"""Reuse the accepted P7/P8 workflow controller for a frozen cut candidate."""
import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
NIGHT = PHASE.parent.parent
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
BUILD = PHASE / "build545"
SECTION1 = NIGHT / "section6_1"
SECTION2 = NIGHT / "section6_2"
N8 = NIGHT / "N8/A2_A3_N12/section6_8/validation/validate_n8.py"
PREFIX = PHASE / "candidate_prefix_preparation/run_candidate_prefix.py"
PAYLOADS = SECTION1 / "accepted_reference_payloads.json"
SCRIPT = Path(__file__).resolve()
FULL_REFERENCE = N8.parent / "full_workflow"
STRICT = PHASE / "candidate_prefix_preparation/compare_candidate_prefix.py"


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def configured():
    """Retain the accepted controller's exact collapse and standing gates."""
    candidate = load("cut_full_candidate_build", PREFIX)
    _, verified = candidate.verify_candidate_build(BUILD)
    status = verified["status"]
    wrapper = load("cut_full_section62", SECTION2 / "run_full_section6_2.py")
    wrapper.HERE, wrapper.STAGED, wrapper.REPO = HERE, STAGED, REPO
    wrapper.OLD = STAGED / "p7p8_r3_20261115"
    runner = wrapper.load_runner(status["binary"])
    runner.update(SCRIPT=SCRIPT, PURPOSE=HERE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=BUILD / "build_status.json",
                  SOURCE_PINS=BUILD / "source_pins.json", BINARY=Path(status["binary"]))
    assert runner["sha"](runner["BINARY"]) == status["binary_sha256"]
    return runner, status


def expected_payloads():
    accepted = json.loads(PAYLOADS.read_text())
    assert accepted["accepted"] is True and len(accepted["hashes"]) == 6
    compare = load("cut_full_payload_comparator", SECTION2 / "compare_reference_payloads.py")
    actual = compare.payload_hashes(SECTION1 / "full_workflow")
    assert not compare.compare_hashes(actual, accepted["hashes"])
    return compare, accepted["hashes"]


def full_identity(reference, candidate):
    """Compare every old integer row and timer call through both complete events."""
    strict = load("cut_full_strict_rows", STRICT)
    base = strict.base
    integer = base.N4.compare_streams(base.N4.streams(reference), base.N4.streams(candidate), "all")

    def ends(case):
        return {label: {str(rank): len(path.read_text().splitlines()) + 1 for rank in range(6)}
                for label, path in base.source_files(case).items()}

    timer = strict.strict_timer_compare(reference, candidate, ends(reference), ends(candidate))
    return {"passed": integer["passed"] and timer["passed"],
            "integer_rows": integer, "timer_rows": timer,
            "reference": str(reference), "all_old_rows_strict": True}


def self_check():
    """Check real setup and prior accepted results without launching CFD."""
    runner, status = configured()
    compare, expected = expected_payloads()
    workflow = compare.workflow_passed(SECTION1 / "full_workflow")
    assert workflow["P7_P8_events_checked"] == [1, 2]
    pins = runner["verify_pins"]()
    assert not pins["source_pin_mismatches"]
    assert runner["assess_p6"]({}, True)["passed"] is False
    report = load("cut_full_wall_report", N8).full_wall_timer_report(SECTION1 / "full_workflow")
    identity = full_identity(FULL_REFERENCE, FULL_REFERENCE)
    assert identity["passed"], identity
    return {"passed": True, "launched": False, "build": str(BUILD),
            "binary_sha256": status["binary_sha256"], "pin_count": len(pins["source_pins"]),
            "accepted_payload_count": len(expected), "accepted_reference_timer_report": report,
            "strict_full_reference_self_identity": identity}


def run(case):
    """Run one full workflow, then enforce the current six-file references."""
    assert HERE in case.parents and not case.exists()
    runner, status = configured()
    compare, expected = expected_payloads()
    sys.path.insert(0, str(HERE))
    sys.path.insert(1, str(STAGED / "p7p8_r3_20261115"))
    sys.argv = [str(SCRIPT), str(case), "--full"]
    runner["main"]()
    wall = load("cut_full_wall_report", N8).full_wall_timer_report(case, require_wall=True)
    actual = compare.payload_hashes(case)
    mismatches = compare.compare_hashes(actual, expected)
    identity = full_identity(FULL_REFERENCE, case)
    receipt = {"passed": not mismatches and identity["passed"], "workflow_gates_passed": True,
               "case": str(case), "candidate_binary_sha256": status["binary_sha256"],
               "reference": str(PAYLOADS), "reference_sha256": sha(PAYLOADS),
               "hashes": actual, "mismatches": mismatches, "timer_and_wall_report": wall,
               "strict_full_identity": identity}
    (case / "cut_g5_payload_identity.json").write_text(json.dumps(receipt, indent=2) + "\n")
    assert receipt["passed"], "G5 payload or old integer/timer identity differs from the accepted reference"
    print(json.dumps({"passed": True, "case": str(case), "hashes": actual}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("case", nargs="?", type=Path)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
    else:
        assert args.case
        run(args.case.resolve())
