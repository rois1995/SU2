#!/usr/bin/env python3
"""Retarget the read-only isolated restart validator to the Section 6.8 source."""
import argparse
import importlib.util
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
VALIDATION = HERE.parent
SECTION6_8 = VALIDATION.parent
SECTION1 = HERE.parents[3] / "section6_1"
STAGED = HERE.parents[4]
BASE = STAGED / "night_20261115/section6_1/validate_disk_restart522.py"
CFD_SHA = "4ca1b712c3eb985d7d9f6132bec067508f3e7943365939a12d97005ab25276da"
TEST_SHA = "3e265eb16b5f8cc796e548a971f50bf9f7bf3268d12953ff58a3ba18af1e7a1d"


def load_base():
    """Load the reviewed validator retargeter without running its main."""
    spec = importlib.util.spec_from_file_location("section61_restart_validator", BASE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.HERE = HERE
    module.STAGED = STAGED
    module.CFD_SHA = CFD_SHA
    module.TEST_SHA = TEST_SHA
    module.MAX_JOB_RSS = 19999490048
    module.source_receipts_pass = source_receipts_pass
    module.source_receipts = source_receipts
    original = module.retargeted_source

    def retargeted_source():
        """Apply exact one-match root and N8 receipt changes to the inherited validator."""
        source = original()
        edits = (
            ('import sys\n', 'import sys\nimport os\n'),
            ('STAGED = HERE.parents[1]', 'STAGED = Path(__file__).resolve().parents[5]'),
            ('(HERE / "build522/build_status.json")',
             '(Path(__file__).resolve().parents[2] / "build545/build_status.json")'),
            ('SOURCE / "section6_1_reference_identity.json"',
             'SOURCE / "cut_g5_payload_identity.json"'),
        )
        for old, new in edits:
            assert source.count(old) == 1, f"expected exactly one validator anchor: {old}"
            source = source.replace(old, new, 1)
        return source

    module.retargeted_source = retargeted_source
    return module


_runner_spec = importlib.util.spec_from_file_location(
    "restart545_shared_receipts", HERE / "run_disk_restart545.py")
_runner_helpers = importlib.util.module_from_spec(_runner_spec)
_runner_spec.loader.exec_module(_runner_helpers)
REFERENCE_HASHES = _runner_helpers.REFERENCE_HASHES
digest = _runner_helpers.digest
verify_n8_identity = _runner_helpers.verify_n8_identity
source_receipts_pass = _runner_helpers.source_receipts_pass

def source_receipts(case):
    """Verify full workflow, event and timer receipts plus actual six-file reference identity."""
    case = Path(case).resolve(strict=True)
    status = json.loads((case / "status.json").read_text())
    workflow = json.loads((case / "full_workflow_receipt.json").read_text())
    identity = verify_n8_identity(case)
    events = [json.loads((case / f"p7p8_event{event}_gate.json").read_text()) for event in (1, 2)]
    timers = [json.loads((case / f"f3b_event{event}_timer_gate.json").read_text()) for event in (1, 2)]
    assert source_receipts_pass(status, workflow, identity, events, timers), "source full workflow lacks accepted event/timer receipts"


def main():
    """Validate the isolated restart output with the unchanged Section 6.1 checks."""
    sys.path.insert(0, str(HERE))
    module = load_base()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-case", type=Path)
    parser.add_argument("--case-dir", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        source = module.retargeted_source()
        compile(source, str(BASE), "exec")
        setup = {"source": VALIDATION / "full_workflow",
                 "build": SECTION6_8 / "build545/build_status.json",
                 "source_pins": SECTION6_8 / "build545/source_pins.json",
                 "extra_source_pins": VALIDATION / "unchanged_build499_source_pins.json"}
        required_paths = (HERE, VALIDATION, SECTION6_8, SECTION1, STAGED, BASE,
                          SECTION6_8 / "build545/build_status.json",
                          SECTION6_8 / "build545/source_pins.json",
                          VALIDATION / "unchanged_build499_source_pins.json",
                          SECTION1 / "accepted_reference_payloads.json",
                          STAGED / "runtime_20261113_474/loaded_runtime_pins.json",
                          STAGED / "night_20261115/section6_1/run_disk_restart522.py",
                          HERE / "run_disk_restart545.py",
                          STAGED / "night_20261115/section6_1/validate_disk_restart522.py")
        assert all(path.exists() for path in required_paths), required_paths
        assert "STAGED = Path(__file__).resolve().parents[5]" in source
        assert "build545/build_status.json" in source and "cut_g5_payload_identity.json" in source
        good = {"state": "terminal", "exit_code": 0, "all_owned_handles_terminal": True,
                "inputs_unchanged": True, "source_and_binary_pins_unchanged": True,
                "full_workflow_accepted": True}
        workflow = {"workflow_accepted": True, "P7_P8_events_checked": [1, 2],
                    "F3b_timer_events_checked": [1, 2]}
        identity = {"passed": True, "workflow_gates_passed": True, "strict_full_identity": {"passed": True}}
        gates = [{"event": i, "passed": True} for i in (1, 2)]
        assert source_receipts_pass(good, workflow, identity, gates, gates)
        assert not source_receipts_pass(good, workflow, {"passed": True, "workflow_gates_passed": True}, gates, gates)
        assert not source_receipts_pass(good, workflow,
                                       {"passed": True, "workflow_gates_passed": True,
                                        "strict_full_identity": {"passed": False}}, gates, gates)
        assert not source_receipts_pass(dict(good, exit_code=1), workflow, identity, gates, gates)
        receipt = {"passed": True, "launched": False, "build_sha": CFD_SHA,
                   "own_positive_negative_receipt_checks": "passed",
                   "helper_sha256": {path.name: digest(path) for path in
                                     (Path(__file__), HERE / "run_disk_restart545.py", BASE)},
                   "setup_paths": {key: {"path": str(value), "exists": value.exists()}
                                   for key, value in setup.items()}}
        (HERE / "validate_restart545_prelaunch_selfcheck.json").write_text(
            json.dumps(receipt, indent=2) + "\n")
        print(json.dumps(receipt, indent=2))
        return
    assert args.source_case is not None and args.case_dir is not None
    source_case = args.source_case.resolve(strict=True)
    case = args.case_dir.resolve(strict=True)
    assert HERE in case.parents and case.is_dir()
    source_receipts(source_case)
    source = module.retargeted_source()
    os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
    os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
    exec(compile(source, str(BASE), "exec"),
         {"__name__": "__main__", "__file__": str(HERE / "validate_disk_restart545.py"),
          "source_receipts_pass": source_receipts_pass,
          "conservative_time_bound": module.conservative_time_bound})


if __name__ == "__main__":
    main()
