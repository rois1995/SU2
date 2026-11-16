#!/usr/bin/env python3
"""N5 measurement runners; self-check is read-only and launch actions require primary receipts."""

import argparse
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

N5 = Path(__file__).resolve().parent
STAGED = N5.parents[1]
REPO = N5.parents[5]
N4 = N5.parent / "N4"
PREFIX = N4 / "run_prefix_controller.py"
CUT_CHECKS = N4 / "run_cut_checks.py"
COMPARATOR = N4 / "compare_integer_rows.py"
BUILD513 = N5 / "build513"
ALIAS_STATUS = N5 / "build513_status.json"
ALIAS_PINS = N5 / "build513_source_pins.json"
ALIAS_HANDLES = N5 / "build513_terminal_handles.json"
RUN_READY = N5 / "primary_run_ready.json"
HEAVY = {"SU2_CFD", "test_driver", "cc1plus", "ninja", "lto1"}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def dump(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + "\n")


def controller_source():
    source = PREFIX.read_text()
    old = '''SCRIPT = Path(__file__).resolve()
PURPOSE = SCRIPT.parent
R = PURPOSE.parents[1]
REPO = R.parents[3]
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
BUILD_BINARY = BUILD / "SU2_CFD/src/SU2_CFD"
BINARY = PURPOSE / "reference_binaries/SU2_CFD"
BUILD_STATUS = PURPOSE / "reference_build/build_status.json"
SOURCE_PINS = PURPOSE / "reference_build/source_pins.json"
CLEAN = R / "nobl_clean_check_20261114/run"
REFERENCE494 = R / "nobl_clean_check_20261114/first_collapse_receipt.json"
REFERENCE293 = R / "nobl_cause_confirm_20261114/reference_counts.json"
REFERENCE_RUN = R / "f3b_20261115/full_workflow"'''
    assert source.count(old) == 1, "N4 prefix-controller constants changed; review retarget"
    new = f'''SCRIPT = Path({str(PREFIX)!r})
PURPOSE = Path({str(N4)!r})
N5ROOT = Path({str(N5)!r})
R = Path({str(STAGED)!r})
REPO = Path({str(REPO)!r})
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
BUILD_BINARY = BUILD / "SU2_CFD/src/SU2_CFD"
BINARY = BUILD_BINARY
BUILD_STATUS = N5ROOT / "build513/build_status.json"
SOURCE_PINS = N5ROOT / "build513/source_pins.json"
CLEAN = R / "nobl_clean_check_20261114/run"
REFERENCE494 = R / "nobl_clean_check_20261114/first_collapse_receipt.json"
REFERENCE293 = R / "nobl_cause_confirm_20261114/reference_counts.json"
REFERENCE_RUN = R / "f3b_20261115/full_workflow"'''
    source = source.replace(old, new)
    old_case = 'assert case != PURPOSE and PURPOSE in case.parents, "case_dir must be a child of the N4 evidence directory"'
    assert source.count(old_case) == 1
    source = source.replace(old_case, 'assert case != N5ROOT and N5ROOT in case.parents, "case_dir must be a fresh N5 child"')
    old_help = 'help="fresh output directory under night_20261115/N4"'
    assert source.count(old_help) == 1
    source = source.replace(old_help, 'help="fresh output directory under night_20261115/N5"')
    compile(source, str(PREFIX), "exec")
    assert 'assert PURPOSE in reference_case.parents' in source
    assert 'N5ROOT in case.parents' in source
    return source


def load_prefix():
    namespace = {"__name__": "n5_retargeted_prefix", "__file__": str(PREFIX)}
    exec(compile(controller_source(), str(PREFIX), "exec"), namespace)
    return namespace


def accepted_payload_hashes():
    spec = importlib.util.spec_from_file_location("n4_cut_checks_readonly", CUT_CHECKS)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.accepted_p6c_payload_hashes()


def self_check():
    prefix = controller_source()
    for token in ("G2_gate_receipt", "compare_integer_prefix", "MAX_JOB_PEAK_RSS", "time.sleep(10 if full_mode"):
        assert token in prefix, f"retarget omitted required controller behavior {token}"
    assert "N4ROOT" not in prefix and str(N5) in prefix and str(N4) in prefix
    payloads = accepted_payload_hashes()
    assert len(payloads) == 6
    comparator_text = COMPARATOR.read_text()
    assert 'ANCHORS = {' in comparator_text and '"G2"' in comparator_text and '"GQ"' in comparator_text
    assert 'timeout_seconds = 2400 if case == "m6_stage" else 600' in (STAGED / "run_cases.py").read_text()
    return {"passed": True, "launched": False, "wrote_files": False,
            "prefix_retarget_compiles": True, "reference_case_guard_remains_N4": True,
            "candidate_case_guard_is_N5": True, "accepted_P6c_payload_hashes": len(payloads),
            "unit_wrapper_timeout_seconds": 600}


def verify_build():
    status = json.loads((BUILD513 / "build_status.json").read_text())
    pins = json.loads((BUILD513 / "source_pins.json").read_text())
    handles = json.loads((BUILD513 / "terminal_handles.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0 and status.get("pins_verified") is True
    assert handles.get("actual_host_proc") is True and handles.get("all_owned_handles_terminal") is True
    assert all(not row.get("alive") for row in handles.get("identities", []))
    assert not handles.get("heavy_processes")
    binary = REPO / "integration_evidence/build-integrated-v3-p5-20261114/SU2_CFD/src/SU2_CFD"
    test_binary = REPO / "integration_evidence/build-integrated-v3-p5-20261114/UnitTests/test_driver"
    assert status["binary"] == str(binary) and status["test_binary"] == str(test_binary)
    assert sha(binary) == status["binary_sha256"] and sha(test_binary) == status["test_binary_sha256"]
    changed = [path for path, digest in pins.items() if sha(path) != digest]
    assert not changed, f"build513 source freeze changed: {changed[:8]}"
    assert sha(BUILD513 / "source_pins.json") == json.loads((BUILD513 / "build_plan.json").read_text())["source_pins_sha256"]
    return status, pins, handles, binary, test_binary


def no_heavy_processes():
    rows = []
    for proc in Path("/proc").glob("[0-9]*"):
        try:
            name = (proc / "comm").read_text().strip()
        except OSError:
            continue
        if name in HEAVY:
            rows.append({"pid": int(proc.name), "name": name})
    assert not rows, f"heavy process already active: {rows[:8]}"
    return rows


def sync_build_alias():
    status, pins, handles, _, _ = verify_build()
    no_heavy_processes()
    alias = dict(status, completed_build=True)
    values = ((ALIAS_STATUS, alias), (ALIAS_PINS, pins), (ALIAS_HANDLES, handles))
    for path, value in values:
        assert not path.exists(), f"refusing to overwrite build alias: {path}"
        dump(path, value)
    return {"build_alias": str(ALIAS_STATUS), "source_pin_count": len(pins),
            "terminal_handles_verified": True, "no_heavy_processes": True}


def verify_run_ready():
    status, pins, _, _, _ = verify_build()
    ready = json.loads(RUN_READY.read_text())
    assert ready.get("run_ready") is True
    assert ready.get("build_status_sha256") == sha(BUILD513 / "build_status.json")
    assert ready.get("source_pins_sha256") == sha(BUILD513 / "source_pins.json")
    expected_helpers = {str(path.resolve()): sha(path) for path in sorted(N5.glob("*.py"))}
    assert ready.get("helper_sha256") == expected_helpers
    assert status.get("pins_verified") and pins
    no_heavy_processes()
    return status


def run_prefix(gate, case_dir, reference_case):
    verify_run_ready()
    expected_ref = N4 / ("reference_G2" if gate == "G2" else "reference_GQ")
    assert reference_case.resolve(strict=True) == expected_ref.resolve(strict=True)
    namespace = load_prefix()
    sys.argv = [str(PREFIX), str(case_dir), "--gate", gate, "--reference-case", str(expected_ref)]
    try:
        namespace["main"]()
    except SystemExit as result:
        if result.code not in (None, 0):
            raise


def run_full(case_dir):
    verify_run_ready()
    for name in ("G2_candidate", "GQ_candidate"):
        receipt = json.loads((N5 / name / "status.json").read_text())
        assert receipt.get("gate_passed") is True and receipt.get("all_owned_handles_terminal") is True
        assert receipt.get("source_and_binary_pins_unchanged") is True
    namespace = load_prefix()
    sys.argv = [str(PREFIX), str(case_dir), "--full"]
    try:
        namespace["main"]()
    except SystemExit as result:
        if result.code not in (None, 0):
            raise
    status = json.loads((case_dir / "status.json").read_text())
    full = json.loads((case_dir / "full_workflow_receipt.json").read_text())
    outputs = {}
    for event in full.get("events", []):
        outputs.update(event.get("outputs", {}))
    expected = accepted_payload_hashes()
    checker = importlib.util.spec_from_file_location("n4_cut_checks_payload", CUT_CHECKS)
    module = importlib.util.module_from_spec(checker)
    checker.loader.exec_module(module)
    actual = module.hash_payload_map(case_dir, outputs)
    receipt = {"schema": "n5_g5_payload_identity_v1", "workflow_accepted": full.get("workflow_accepted"),
               "candidate_payload_sha256": actual, "accepted_p6c_payload_sha256": expected,
               "candidate_matches_accepted_p6c": len(actual) == 6 and actual == expected,
               "passed": bool(status.get("gate_passed") and full.get("workflow_accepted") and actual == expected)}
    dump(case_dir / "g5_payload_identity_receipt.json", receipt)
    assert receipt["passed"], json.dumps(receipt, indent=2)
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "sync-build-alias", "prefix", "full"))
    parser.add_argument("--gate", choices=("G2", "GQ"))
    parser.add_argument("--case-dir", type=Path)
    parser.add_argument("--reference-case", type=Path)
    args = parser.parse_args()
    if args.action == "self-check":
        print(json.dumps(self_check(), indent=2))
    elif args.action == "sync-build-alias":
        print(json.dumps(sync_build_alias(), indent=2))
    elif args.action == "prefix":
        if not args.gate or not args.case_dir or not args.reference_case:
            parser.error("prefix requires --gate, --case-dir and --reference-case")
        assert args.case_dir.resolve().parent == N5
        run_prefix(args.gate, args.case_dir, args.reference_case)
    elif args.action == "full":
        if not args.case_dir:
            parser.error("full requires --case-dir")
        assert args.case_dir.resolve().parent == N5
        print(json.dumps(run_full(args.case_dir.resolve()), indent=2))


if __name__ == "__main__":
    main()
