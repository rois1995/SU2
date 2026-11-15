#!/usr/bin/env python3
"""Retarget existing N4 checks to one verified cut build; does not build binaries."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
REPO = HERE.parents[5]
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
UNIT_HELPER = HERE / "run_unit_gates.py"
PREFIX_HELPER = HERE / "run_prefix_controller.py"
INTEGER_ROWS = HERE / "compare_integer_rows.py"
G3_REFERENCE_RECEIPT = HERE / "G3_reference_receipt.json"
P6C_FULL_RECEIPT = STAGED / "f3b_20261115/full_workflow/full_workflow_receipt.json"
P6C_COMPLETE_RECEIPT = STAGED / "f3b_20261115/complete_milestone_receipt.json"


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def checked_step(step_arg: str, build_label: str) -> tuple[Path, Path, Path]:
    assert re.fullmatch(r"build[1-9][0-9]*", build_label), "build label must use the run_cases buildN format"
    step = Path(step_arg).expanduser().resolve()
    assert HERE in step.parents and step != HERE, "step directory must be an N4 child"
    status_path, pins_path = step / "build_status.json", step / "source_pins.json"
    plan = json.loads((step / "cut_build_plan.json").read_text())
    assert plan.get("build_label") == build_label and plan.get("source_pins_sha256") == sha(pins_path)
    status = json.loads(status_path.read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0 and status.get("pins_verified") is True
    for key in ("binary", "test_binary"):
        path = Path(status[key]).resolve(strict=True)
        assert sha(path) == status[key + "_sha256"], f"step {key} hash mismatch"
    pins = json.loads(pins_path.read_text())
    assert isinstance(pins, dict) and pins
    changed = [path for path, digest in pins.items() if sha(Path(path)) != digest]
    assert not changed, f"step source pins changed: {changed[:8]}"
    terminal_path = step / "terminal_handles.json"
    terminal = json.loads(terminal_path.read_text())
    assert terminal.get("actual_host_proc") is True and terminal.get("all_owned_handles_terminal") is True
    return step, status_path, pins_path


def retarget_unit_source(build_label: str, step: Path) -> tuple[str, dict[str, str]]:
    source = UNIT_HELPER.read_text()
    old_ref = 'REF = HERE / "reference_build"'
    assert source.count(old_ref) == 1
    source = source.replace(old_ref, "REF = Path(REF_DIR)")
    assert source.count("build509") >= 1
    source = source.replace("build509", build_label)
    old_output = '(HERE / "g3_integer_rows.json")'
    assert source.count(old_output) == 1
    source = source.replace(old_output, '(Path(REF_DIR) / "g3_integer_rows.json")')
    old_case = 'text = text.replace("case=\'f3b_\'+mode", "case=\'n4_\'+mode")'
    assert source.count(old_case) == 1
    source = source.replace(old_case, old_case.replace("n4_", build_label + "_"))
    old_glob = 'glob("n4_bl_*")'
    assert source.count(old_glob) == 1
    source = source.replace(old_glob, f'glob("{build_label}_bl_*")')
    compile(source, str(UNIT_HELPER), "exec")
    assert "REF = Path(REF_DIR)" in source and build_label in source
    return source, {"reference_root": str(UNIT_HELPER.parent / "reference_build"), "step_root": str(step),
                    "build_label": build_label}


def execute_unit_gate(action: str, build_label: str, step: Path) -> None:
    transformed, _ = retarget_unit_source(build_label, step)
    namespace = {"__name__": "__main__", "__file__": str(UNIT_HELPER),
                 "REF_DIR": str(step)}
    sys.argv = [str(UNIT_HELPER), action]
    exec(compile(transformed, str(UNIT_HELPER), "exec"), namespace)


def load_prefix_helper(step: Path, status_path: Path, pins_path: Path):
    spec = importlib.util.spec_from_file_location("n4_cut_prefix_controller", PREFIX_HELPER)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load existing prefix controller: {PREFIX_HELPER}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    module.PURPOSE = HERE
    module.R = STAGED
    module.BUILD = BUILD
    module.BINARY = BUILD / "SU2_CFD/src/SU2_CFD"
    module.BUILD_BINARY = module.BINARY
    module.BUILD_STATUS = status_path
    module.SOURCE_PINS = pins_path
    module.SCRIPT = Path(__file__).resolve()
    return module


def run_prefix(action: str, step: Path, status_path: Path, pins_path: Path,
               reference_case: Path, case_dir: Path) -> None:
    module = load_prefix_helper(step, status_path, pins_path)
    gate = "G2" if action == "g2" else "GQ"
    sys.argv = [str(module.SCRIPT), str(case_dir), "--gate", gate, "--reference-case", str(reference_case)]
    try:
        module.main()
    except SystemExit as result:
        if result.code not in (None, 0):
            raise


def hash_payload_map(case: Path, payloads: dict[str, str]) -> dict[str, str]:
    root = case.resolve(strict=True)
    actual = {}
    for name, expected in payloads.items():
        path = (root / name).resolve(strict=True)
        assert root in path.parents, f"payload escapes case directory: {name}"
        actual[name] = sha(path)
        assert actual[name] == expected, f"payload hash changed: {name}"
    return actual


def accepted_p6c_payload_hashes() -> dict[str, str]:
    full = json.loads(P6C_FULL_RECEIPT.read_text())
    complete = json.loads(P6C_COMPLETE_RECEIPT.read_text())
    assert full.get("workflow_accepted") is True and full.get("F3b_timer_events_checked") == [1, 2]
    assert complete.get("passed") is True and complete.get("payload_identity", {}).get("passed") is True
    expected = {}
    for event in full.get("events", []):
        assert event.get("event") in (1, 2)
        expected.update(event.get("outputs", {}))
    identity = {row["path"]: row["P6c_sha256"] for row in complete["payload_identity"]["records"]
                if row.get("byte_identical") is True}
    assert len(expected) == len(identity) == 6 and expected == identity
    return expected


def run_full(step: Path, status_path: Path, pins_path: Path, case_dir: Path) -> dict[str, object]:
    module = load_prefix_helper(step, status_path, pins_path)
    sys.argv = [str(module.SCRIPT), str(case_dir), "--full"]
    code = 0
    try:
        module.main()
    except SystemExit as result:
        code = 0 if result.code in (None, 0) else int(result.code)
    status = json.loads((case_dir / "status.json").read_text())
    full = json.loads((case_dir / "full_workflow_receipt.json").read_text())
    expected = accepted_p6c_payload_hashes()
    candidate = {}
    for event in full.get("events", []):
        candidate.update(event.get("outputs", {}))
    identity = candidate == expected and len(candidate) == 6
    receipt = {"schema": "n4_cut_g5_payload_identity_v1", "passed": bool(
        code == 0 and status.get("gate_passed") is True and full.get("workflow_accepted") is True and identity),
        "candidate_payload_sha256": candidate, "accepted_p6c_payload_sha256": expected,
        "candidate_matches_accepted_p6c": identity, "controller_exit_code": code,
        "controller_gate_passed": status.get("gate_passed"),
        "physical_and_timer_receipt_passed": full.get("workflow_accepted"),
        "accepted_full_receipt_sha256": sha(P6C_FULL_RECEIPT),
        "accepted_complete_receipt_sha256": sha(P6C_COMPLETE_RECEIPT)}
    (case_dir / "g5_payload_identity_receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2), flush=True)
    if not receipt["passed"]:
        raise SystemExit(1)
    return receipt


def compare_g3(step: Path, reference_case: Path) -> dict[str, object]:
    receipt = json.loads(G3_REFERENCE_RECEIPT.read_text())
    expected_case = Path(receipt["case"]).resolve(strict=True)
    assert receipt.get("state") == "terminal" and receipt.get("reference_capture_passed") is True
    assert receipt.get("first_336_identical_P6c") is True and receipt.get("actual_splits") == 473
    assert reference_case.resolve(strict=True) == expected_case, "G3 comparison must use the captured N4 reference case"
    reference_rows = HERE / "g3_integer_rows.json"
    ref_hashes = receipt.get("files", {})
    assert ref_hashes.get(str(reference_case / "cheap_bl_receipt.json")) == sha(reference_case / "cheap_bl_receipt.json")
    assert ref_hashes.get(str(reference_case / "run.log")) == sha(reference_case / "run.log")
    assert ref_hashes.get(str(reference_rows.resolve())) == sha(reference_rows)
    reference_result = json.loads((reference_case / "cheap_bl_receipt.json").read_text())
    current_rows_path = step / "g3_integer_rows.json"
    current_rows = json.loads(current_rows_path.read_text())
    current_case = Path(current_rows["case"]).resolve(strict=True)
    current_result = json.loads((current_case / "cheap_bl_receipt.json").read_text())
    assert current_result.get("passed") is True
    comparison_proc = subprocess.run([sys.executable, str(INTEGER_ROWS), str(reference_case), str(current_case),
                                      "--gate", "all"], capture_output=True, text=True, check=False)
    try:
        comparison = json.loads(comparison_proc.stdout)
    except json.JSONDecodeError:
        comparison = {"passed": False, "error": comparison_proc.stderr or comparison_proc.stdout}
    expected_splits = int(receipt["actual_splits"])
    reference_hashes = reference_result.get("actual_output_sha256", {})
    candidate_hashes = current_result.get("actual_output_sha256", {})
    reference_actual = hash_payload_map(reference_case, reference_hashes)
    candidate_actual = hash_payload_map(current_case, candidate_hashes)
    output_equal = candidate_hashes == reference_hashes and candidate_actual == reference_actual
    first_336_equal = bool(reference_result.get("gates", {}).get("first_336_identical_P6c") and
                           current_result.get("gates", {}).get("first_336_identical_P6c"))
    splits_equal = int(current_result.get("actual_splits", -1)) == expected_splits == 473
    edits_equal = int(current_result.get("actual_edits", -1)) == int(reference_result.get("actual_edits", -2))
    passed = bool(comparison_proc.returncode == 0 and comparison.get("passed") is True and
                  first_336_equal and output_equal and splits_equal and edits_equal)
    final = {"schema": "n4_cut_g3_candidate_receipt_v1", "passed": passed,
             "reference_case": str(reference_case), "candidate_case": str(current_case),
             "reference_actual_splits": expected_splits,
             "candidate_actual_splits": current_result.get("actual_splits"),
             "reference_actual_edits": reference_result.get("actual_edits"),
             "candidate_actual_edits": current_result.get("actual_edits"),
             "first_336_identical_to_P6c": first_336_equal,
             "saved_state_payload_hashes_equal": output_equal,
             "reference_saved_state_payload_actual_sha256": reference_actual,
             "candidate_saved_state_payload_actual_sha256": candidate_actual,
             "integer_rows_comparison": comparison, "integer_rows_exit_code": comparison_proc.returncode,
             "reference_receipt_sha256": sha(G3_REFERENCE_RECEIPT),
             "reference_case_log_sha256": sha(reference_case / "run.log"),
             "candidate_case_log_sha256": sha(current_case / "run.log"),
             "reference_cheap_bl_receipt_sha256": sha(reference_case / "cheap_bl_receipt.json"),
             "candidate_cheap_bl_receipt_sha256": sha(current_case / "cheap_bl_receipt.json"),
             "reference_integer_rows_sha256": sha(reference_rows),
             "candidate_integer_rows_sha256": sha(current_rows_path),
             "comparator_sha256": sha(INTEGER_ROWS)}
    output = step / "g3_candidate_receipt.json"
    output.write_text(json.dumps(final, indent=2) + "\n")
    return final


def self_check() -> dict[str, object]:
    source, roots = retarget_unit_source("cut_demo", HERE / "cut_demo")
    assert "REF = Path(REF_DIR)" in source and "build509" not in source
    assert "REF_DIR" in source and "cut_demo" in source
    assert '(Path(REF_DIR) / "g3_integer_rows.json")' in source
    assert 'glob("cut_demo_bl_*")' in source
    assert 'case=\'cut_demo_\'+mode' in source
    assert str(UNIT_HELPER) == str(Path(UNIT_HELPER).resolve())
    import tempfile
    with tempfile.TemporaryDirectory() as temporary:
        payload_case = Path(temporary)
        sample = payload_case / "sample.bin"
        sample.write_bytes(b"payload")
        assert hash_payload_map(payload_case, {"sample.bin": sha(sample)}) == {"sample.bin": sha(sample)}
        try:
            hash_payload_map(payload_case, {"sample.bin": "0" * 64})
        except AssertionError:
            pass
        else:
            raise AssertionError("payload hash mismatch was not rejected")
    expected_p6c = accepted_p6c_payload_hashes()
    assert len(expected_p6c) == 6
    prefix_source = PREFIX_HELPER.read_text()
    assert 'parser.add_argument("--reference-case"' in prefix_source
    assert "G2_gate_receipt" in prefix_source
    compare_source = INTEGER_ROWS.read_text()
    assert 'ANCHORS = {' in compare_source and '"GQ"' in compare_source
    return {"passed": True, "unit_helper_retarget_syntax": True,
            "preserves_original_helper_file_root": str(UNIT_HELPER),
            "step_reference_root": roots["step_root"], "prefix_candidate_requires_reference": True,
            "g3_new_reference_contract": True, "payload_map_rehash_and_mismatch_rejection": True,
            "accepted_p6c_payload_records": len(expected_p6c), "g5_accepted_receipts_valid": True,
            "launched": False}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("step_dir", nargs="?")
    parser.add_argument("build_label", nargs="?")
    parser.add_argument("action", nargs="?", choices=("g1", "g2", "gq", "g3", "g4", "g5"))
    parser.add_argument("--reference-case")
    parser.add_argument("--case-dir", help="fresh N4 child directory used by G2/GQ")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if not args.step_dir or not args.build_label or not args.action:
        parser.error("step_dir, build_label, and action are required")
    if args.action in ("g2", "gq", "g3") and not args.reference_case:
        parser.error("G2/GQ/G3 candidate checks require --reference-case from the matching N4 capture")
    if args.action in ("g1", "g4", "g5") and args.reference_case:
        parser.error("--reference-case is only used by G2, GQ, and G3 candidates")
    if args.action == "g5" and not args.case_dir:
        parser.error("G5 requires --case-dir for full workflow output")
    step, status_path, pins_path = checked_step(args.step_dir, args.build_label)
    if args.action in ("g2", "gq"):
        if not args.case_dir:
            parser.error("G2/GQ require --case-dir for fresh prefix output")
        reference_case = Path(args.reference_case).expanduser().resolve(strict=True)
        assert HERE in reference_case.parents, "prefix reference must be a captured N4 child"
        run_prefix(args.action, step, status_path, pins_path, reference_case,
                   Path(args.case_dir).expanduser().resolve())
    elif args.action == "g3":
        reference_case = Path(args.reference_case).expanduser().resolve(strict=True)
        execute_unit_gate("g3", args.build_label, step)
        receipt = compare_g3(step, reference_case)
        print(json.dumps(receipt, indent=2), flush=True)
        raise SystemExit(not receipt["passed"])
    elif args.action == "g5":
        case_dir = Path(args.case_dir).expanduser().resolve()
        assert HERE in case_dir.parents, "full workflow output must be an N4 child"
        run_full(step, status_path, pins_path, case_dir)
    else:
        execute_unit_gate(args.action, args.build_label, step)


if __name__ == "__main__":
    main()
