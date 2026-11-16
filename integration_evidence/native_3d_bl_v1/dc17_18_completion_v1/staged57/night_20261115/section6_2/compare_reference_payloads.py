#!/usr/bin/env python3
"""Compare six accepted N1 payload hashes after strict workflow gates pass."""
import argparse
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
PAYLOAD_NAMES = (
    "adapted_mesh_adap_00001.su2",
    "adapted_mesh_adap_00001.su2.native_ref",
    "restart_flow_adap_00001.dat",
    "adapted_mesh_adap_00002.su2",
    "adapted_mesh_adap_00002.su2.native_ref",
    "restart_flow_adap_00002.dat",
)


def sha(path):
    """Hash one workflow payload using SHA-256."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def workflow_passed(case):
    """Require the full workflow and both native and F3b event gates to pass."""
    status = json.loads((case / "status.json").read_text())
    workflow = json.loads((case / "full_workflow_receipt.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("all_owned_handles_terminal") is True
    assert status.get("inputs_unchanged") is True
    assert status.get("source_and_binary_pins_unchanged") is True
    assert status.get("full_workflow_accepted") is True
    assert workflow.get("workflow_accepted") is True
    strict = workflow.get("strict_gates", {})
    assert strict.get("events") == 2
    assert strict.get("per_event_native_seconds_max") == 5400
    assert strict.get("no_fixed_mesh_point_or_cell_target") is True
    assert strict.get("mesh_counts_must_match_certification_totals") is True
    assert strict.get("q_j_geometry_size_bad_must_be_zero") is True
    assert len(workflow.get("events", [])) == 2
    assert workflow.get("P7_P8_events_checked") == [1, 2]
    assert workflow.get("F3b_timer_events_checked") == [1, 2]
    for event in (1, 2):
        native = json.loads((case / f"p7p8_event{event}_gate.json").read_text())
        timer = json.loads((case / f"f3b_event{event}_timer_gate.json").read_text())
        assert native.get("event") == event and native.get("passed") is True
        assert timer.get("event") == event and timer.get("passed") is True
    return workflow


def payload_hashes(case):
    """Hash exactly the six event outputs named by the full workflow receipt."""
    workflow = workflow_passed(case)
    outputs = {}
    for event in workflow["events"]:
        outputs.update(event["outputs"])
    assert set(PAYLOAD_NAMES) <= set(outputs)
    result = {}
    for name in PAYLOAD_NAMES:
        actual = sha(case / name)
        assert actual == outputs[name], f"workflow output digest mismatch: {name}"
        result[name] = actual
    return result


def compare_hashes(actual, expected):
    """Return name-specific mismatches without diagnosing expected drift."""
    assert set(actual) == set(PAYLOAD_NAMES) and set(expected) == set(PAYLOAD_NAMES)
    return {name: {"expected": expected[name], "actual": actual[name]}
            for name in PAYLOAD_NAMES if actual[name] != expected[name]}


def self_check():
    """Exercise exact-six comparison and report drift without failing the gates."""
    reference = {name: f"n1-{index}" for index, name in enumerate(PAYLOAD_NAMES)}
    same = dict(reference)
    assert compare_hashes(same, reference) == {}
    drifted = dict(same, **{PAYLOAD_NAMES[4]: "new-native-ref"})
    mismatch = compare_hashes(drifted, reference)
    assert list(mismatch) == [PAYLOAD_NAMES[4]]
    assert set(reference) == set(PAYLOAD_NAMES)
    accepted = HERE.parent / "section6_1/full_workflow"
    workflow = workflow_passed(accepted)
    assert [event["event"] for event in workflow["events"]] == [1, 2]
    assert set(name for event in workflow["events"] for name in event["outputs"]) == set(PAYLOAD_NAMES)


def main():
    """Record first-run baseline drift or exact repeat consistency."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", required=True, type=Path)
    parser.add_argument("--repeat-of", type=Path)
    parser.add_argument("--reference", type=Path, default=HERE / "accepted_reference_payloads.json")
    args = parser.parse_args()
    reference = json.loads(args.reference.read_text())
    assert reference.get("accepted") is True
    expected = reference.get("hashes", {})
    assert set(expected) == set(PAYLOAD_NAMES)
    case = args.case.resolve(strict=True)
    actual = payload_hashes(case)
    mismatches = compare_hashes(actual, expected)
    receipt = {"case": str(case), "reference": str(args.reference.resolve()),
               "reference_sha256": sha(args.reference), "reference_mismatches": mismatches,
               "identity_match": not mismatches, "workflow_gates_passed": True,
               "hashes": actual}
    if args.repeat_of:
        first = args.repeat_of.resolve(strict=True)
        first_hashes = payload_hashes(first)
        repeat_mismatches = compare_hashes(actual, first_hashes)
        receipt.update(first_case=str(first), first_case_hashes=first_hashes,
                       repeat_mismatches=repeat_mismatches,
                       repeat_identity_passed=not repeat_mismatches,
                       expected_drift_from_n1=bool(compare_hashes(first_hashes, expected)))
    else:
        receipt["expected_drift_from_n1"] = bool(mismatches)
        receipt["repeat_required_after_all_gates"] = bool(mismatches)
    (case / "section6_2_payload_comparison.json").write_text(json.dumps(receipt, indent=2) + "\n")
    if args.repeat_of and receipt["repeat_mismatches"]:
        raise SystemExit(2)
    print(json.dumps({"workflow_gates_passed": True, "identity_match": not mismatches,
                      "expected_drift_from_n1": receipt.get("expected_drift_from_n1"),
                      "repeat_identity_passed": receipt.get("repeat_identity_passed")}))


if __name__ == "__main__":
    import sys
    if sys.argv[1:] == ["--self-check"]:
        self_check()
        print(json.dumps({"passed": True, "launched": False}))
    else:
        main()
