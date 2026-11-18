#!/usr/bin/env python3
"""Validate N14 G2/GQ prefixes against fresh build536 with the exact 6.8/6.9 rule."""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

P = Path(__file__).resolve().parents[2]
HERE = P / "C7/validation"
NIGHT = P.parent.parent
N14 = NIGHT / "N14/section6_12"
B3_VALIDATION = P / "B3/validation"
REFERENCES = B3_VALIDATION / "references"
BUILD = P / "build565"
TIMER_COMPARATOR = NIGHT / "N11/section6_8_preparation/drafts/compare_timer_census_delta_69.py"
COMPARATOR68 = TIMER_COMPARATOR
EXPECTED_WORK = 90_809_869
EXPECTED_COUNTS = (480, 473, 346, 263, 469, 464)
INPUT_BASENAMES = ("m6.su2", "m6.su2.native_ref", "baseline_euler_seed.dat")
N1_COLLAPSE = REFERENCES / "G2_reference"
N1_FULL = REFERENCES / "GQ_reference"


def sha(path):
    """Hash a small provenance file in bounded blocks."""
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def read_json(path):
    """Read a JSON provenance record for the inherited prefix controller."""
    return json.loads(Path(path).read_text())


def build565():
    """Expose the sealed build565 aliases through the inherited build API."""
    status = read_json(BUILD / "build_status.json")
    alias = read_json(HERE / "build565_status.json")
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and status.get("changed") == []
    assert alias.get("completed_build") is True
    assert alias.get("binary_sha256") == status.get("binary_sha256")
    assert alias.get("test_binary_sha256") == status.get("test_binary_sha256")
    binary = Path(status["binary"])
    assert binary.is_file() and sha(binary) == status["binary_sha256"]
    return {"binary": str(binary), "status": status}


build534 = build565


def load_comparator():
    """Load the existing exact integer plus two-sided census comparator."""
    spec = importlib.util.spec_from_file_location("n14_timer_census_delta_69", TIMER_COMPARATOR)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import comparator: {TIMER_COMPARATOR}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def reference_case(gate):
    """Return the sealed fresh536 reference case for one gate."""
    assert gate in ("G2", "GQ")
    return REFERENCES / ("G2_reference" if gate == "G2" else "GQ_reference")


def verify_reference(case, gate):
    """Require the saved reference to be a terminal, passing prefix with matching inputs."""
    status = json.loads((case / "status.json").read_text())
    provenance = json.loads((case / "provenance.json").read_text())
    assert status.get("state") == "terminal" and status.get("stop_reason") == gate + "_prefix_passed"
    assert status.get("gate") == gate and status.get("inputs_unchanged")
    assert status.get("source_and_binary_pins_unchanged") and status.get("all_owned_handles_terminal")
    assert provenance.get("gate") == gate
    assert provenance.get("config_sha256") == sha(case / "run.cfg")
    return status, provenance


def validate(gate, case):
    """Match the inherited prefix runner API while keeping local gate logic."""
    return validate_case(case, gate)


def validate_case(case, gate):
    """Check candidate prefix gates, inputs, strict rows, and timer census adjustments."""
    case = Path(case).resolve(strict=True)
    assert case != HERE and HERE in case.parents, "candidate case must be under this validation tree"
    reference = reference_case(gate).resolve(strict=True)
    ref_status, ref_provenance = verify_reference(reference, gate)
    status, provenance = verify_reference(case, gate)
    assert provenance.get("binary_sha256") == json.loads((BUILD / "build_status.json").read_text()).get("binary_sha256")
    assert provenance.get("source_pins_sha256") == sha(BUILD / "source_pins.json")
    assert provenance.get("config_sha256") == ref_provenance.get("config_sha256")
    for basename in INPUT_BASENAMES:
        ref_hashes = {Path(path).name: value for path, value in ref_provenance.get("input_hashes", {}).items()
                      if Path(path).name == basename}
        new_hashes = {Path(path).name: value for path, value in provenance.get("input_hashes", {}).items()
                      if Path(path).name == basename}
        assert len(ref_hashes) == len(new_hashes) == 1 and ref_hashes == new_hashes, basename
    if gate == "G2":
        receipt = json.loads((case / "G2_gate_receipt.json").read_text())
        collapse = receipt.get("P6c_collapse_gate", {})
        assert receipt.get("passed") and collapse.get("passed")
        assert collapse.get("upper_work_sum") == EXPECTED_WORK
        assert tuple(int(row["accepted"]) for row in collapse.get("per_rank", [])) == EXPECTED_COUNTS
        assert len(collapse.get("per_rank", [])) == 6 and collapse.get("zero_upper_refinements")
    else:
        receipt = json.loads((case / "GQ_gate_receipt.json").read_text())
        rows = {int(rank): int(count) for rank, count in receipt.get("quality_window_rows", {}).items()}
        assert receipt.get("passed") and receipt.get("anchor") == "third NATIVE3D_QUALITY stage=window row per rank"
        assert rows == dict.fromkeys(range(6), 3)
        assert receipt.get("G2_gate_receipt", {}).get("passed")
    comparison = load_comparator().compare_cases(reference, case, gate)
    assert comparison.get("passed") is True, comparison.get("first_difference")
    result = {"passed": True, "gate": gate, "candidate": str(case), "reference": str(reference),
              "candidate_status": status.get("state"), "reference_status": ref_status.get("state"),
              "strict_old_integer_rows": comparison["integer_rows"].get("passed"),
              "census_adjustment": comparison["timer_rows"],
              "census_rule": comparison["census_rule"],
              "fixedstorage_required": 4120,
              "comparator_sha256": sha(TIMER_COMPARATOR)}
    out = case / f"{gate.lower()}_build565_comparison.json"
    out.write_text(json.dumps(result, indent=2) + "\n")
    return result


def self_check():
    """Exercise synthetic two-sided census adjustments and frozen real GQ-prefix check."""
    comparator = load_comparator()
    synthetic = comparator.self_check()
    assert synthetic.get("passed") and synthetic.get("wrong_census_delta_rejected")
    gq_prefix = comparator.actual_gq_prefix_self_check()
    assert gq_prefix.get("passed")
    assert BUILD == P / "build565"
    assert reference_case("G2").name == "G2_reference"
    assert reference_case("GQ").name == "GQ_reference"
    return {"passed": True, "launched": False, "built": False,
            "G2_reference": str(reference_case("G2")), "GQ_reference": str(reference_case("GQ")),
            "census_comparator": synthetic, "actual_gq_truncation": gq_prefix}


def main():
    """Validate a completed candidate prefix or run helper-only self-checks."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--gate", choices=("G2", "GQ"))
    parser.add_argument("--case", type=Path)
    args = parser.parse_args()
    if args.self_check:
        assert args.case is None and args.gate is None
        print(json.dumps(self_check(), indent=2))
        return
    if args.case is None or args.gate is None:
        parser.error("--gate and --case are required")
    print(json.dumps(validate_case(args.case, args.gate), indent=2))


if __name__ == "__main__":
    main()
