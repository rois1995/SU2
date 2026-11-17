#!/usr/bin/env python3
"""Finish the unchanged G5 comparisons skipped by the reused runner SystemExit(0)."""
import importlib.util
import json
from pathlib import Path
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("frozen_cut_full", HERE / "validate_full.py")
full = importlib.util.module_from_spec(spec)
spec.loader.exec_module(full)
case = HERE / "full_workflow"
status = json.loads((case / "status.json").read_text())
assert status["state"] == "terminal" and status["exit_code"] == 0
assert status["all_owned_handles_terminal"] and status["full_workflow_accepted"]
runner, build = full.configured()
pins = runner["verify_pins"]()
assert not pins["source_pin_mismatches"], pins["source_pin_mismatches"]
compare, expected = full.expected_payloads()
workflow = compare.workflow_passed(case)
assert workflow["P7_P8_events_checked"] == [1, 2]
wall = full.load("cut_full_wall_postonly", full.N8).full_wall_timer_report(case, require_wall=True)
actual = compare.payload_hashes(case)
mismatches = compare.compare_hashes(actual, expected)
identity = full.full_identity(full.FULL_REFERENCE, case)
receipt = {"passed": not mismatches and identity["passed"], "workflow_gates_passed": True,
           "case": str(case), "candidate_binary_sha256": build["binary_sha256"],
           "reference": str(full.PAYLOADS), "reference_sha256": full.sha(full.PAYLOADS),
           "hashes": actual, "mismatches": mismatches, "timer_and_wall_report": wall,
           "strict_full_identity": identity,
           "postprocessing_only": True, "numerical_run_repeated": False}
(case / "cut_g5_payload_identity.json").write_text(json.dumps(receipt, indent=2) + "\n")
assert receipt["passed"], "G5 payload or old integer/timer identity differs from the accepted reference"
print(json.dumps({"passed": True, "hashes": actual, "all_old_integer_and_timer_rows_identical": identity["passed"]}))
