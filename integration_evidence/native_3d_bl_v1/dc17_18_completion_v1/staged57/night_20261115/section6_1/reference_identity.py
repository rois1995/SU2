#!/usr/bin/env python3
"""Check only the output identity policy explicitly authorized by Night section 6.1."""
import argparse
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
N5_RECEIPT = HERE.parent / "N5/full_workflow_R1_repeat/g5_payload_identity_receipt.json"
EVENT2 = {
    "adapted_mesh_adap_00002.su2": "63690652de1af70f890994fb5054523403ba1dc27200123a660e573827aebc36",
    "adapted_mesh_adap_00002.su2.native_ref": "36c45abe03d1dc38e523583cc3103cfb9c83b17983b3679df0c424fab8ff088a",
    "restart_flow_adap_00002.dat": "44766f7f95f14d60011fb27ef1bb864c44e9649f782f2b9a0139c6766c93bc94",
}


def sha(path):
    """Hash a payload without loading it into RAM."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def reference():
    """Keep P6c event 1 and the user's three explicit P7 event-2 hashes."""
    old = json.loads(N5_RECEIPT.read_text())
    assert old["passed"] and old["candidate_matches_accepted_p6c"]
    values = old["accepted_p6c_payload_sha256"]
    return {name: digest for name, digest in values.items() if "00001" in name} | EVENT2


def assess(first, expected, repeated=None):
    """Separate expected reference drift from a failed event-1 or repeatability gate."""
    assert len(first) == len(expected) == 6 and set(first) == set(expected)
    event1 = [name for name in expected if "00001" in name]
    event2 = list(EVENT2)
    first_event1 = all(first[name] == expected[name] for name in event1)
    original_event2 = all(first[name] == expected[name] for name in event2)
    result = {"event1_P6c_identical": first_event1, "event2_original_P7_identical": original_event2,
              "first_run_hashes": first, "original_reference_hashes": expected,
              "scope": "output identity only; standing production gates are assessed separately"}
    if not first_event1:
        result["state"] = "FAILED_EVENT1_IDENTITY"
    elif original_event2:
        result["state"] = "PASSED_ORIGINAL_P7_REFERENCE"
    elif repeated is None:
        result["state"] = "ONE_SAME_BINARY_REPEAT_REQUIRED"
    else:
        assert set(repeated) == set(first)
        result["repeat_run_hashes"] = repeated
        same = all(repeated[name] == first[name] for name in expected)
        result["state"] = "PASSED_NEW_REPEATABLE_P7_REFERENCE" if same else "FAILED_REPEATABILITY"
        result["note"] = "differs from the first N1 run; the build differs by the N5 observation code"
    result["identity_passed"] = result["state"].startswith("PASSED_")
    return result


def self_check():
    """Cover unchanged, allowed drift, repeat acceptance and both failure paths."""
    expected = reference()
    assert assess(expected, expected)["identity_passed"]
    changed = expected | {next(iter(EVENT2)): "0" * 64}
    assert assess(changed, expected)["state"] == "ONE_SAME_BINARY_REPEAT_REQUIRED"
    assert assess(changed, expected, changed)["identity_passed"]
    assert assess(changed, expected, expected)["state"] == "FAILED_REPEATABILITY"
    bad = changed | {next(name for name in expected if "00001" in name): "1" * 64}
    assert assess(bad, expected)["state"] == "FAILED_EVENT1_IDENTITY"


def main():
    """Write an independent identity receipt after the numerical campaign closes."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", nargs="?", type=Path)
    parser.add_argument("--repeat", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    self_check()
    if args.self_check:
        print(json.dumps({"passed": True, "launched": False}))
        return
    assert args.case
    expected = reference()
    first = {name: sha(args.case / name) for name in expected}
    repeated = {name: sha(args.repeat / name) for name in expected} if args.repeat else None
    result = assess(first, expected, repeated)
    result["helper_sha256"] = sha(__file__)
    result["P6c_reference_receipt_sha256"] = sha(N5_RECEIPT)
    (args.case / "section6_1_reference_identity.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result))
    raise SystemExit(result["state"].startswith("FAILED_"))


if __name__ == "__main__":
    main()
