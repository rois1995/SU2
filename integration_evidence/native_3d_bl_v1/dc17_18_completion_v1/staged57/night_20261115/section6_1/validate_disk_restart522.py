#!/usr/bin/env python3
"""Read-only Section 6.1 validator wrapper for an explicitly selected accepted full case."""
import argparse
import json
import os
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
BASE_VALIDATOR = STAGED / "p7p8_r3_20261115" / "validate_disk_restart508.py"
CFD_SHA = "949359f28cd46490bd5b4e1389ccc22948952c73f360671b593e3e09a4f2d752"
TEST_SHA = "6de087f99ba03a0b27d73c369f7a72109f19fcd645869dee9b28cac8bed4cb91"
MAX_JOB_RSS = 19999490048
EVENTS_REQUIRED = (1, 2)


def conservative_time_bound(log_text):
    """Return a six-rank upper bound from GNU time's largest descendant RSS."""
    matches = re.findall(r"Maximum resident set size \(kbytes\):\s*(\d+)", log_text)
    exits = re.findall(r"Exit status:\s*(\d+)", log_text)
    if not matches or not exits or exits[-1] != "0":
        return {"parsed": False, "reason": "GNU time MaxRSS or successful exit record missing"}
    largest_kib = max(map(int, matches))
    return {"parsed": True, "maxrss_kib": largest_kib,
            "maxrss_bytes": largest_kib * 1024,
            "rank_count": 6, "upper_bound_bytes": largest_kib * 1024 * 6,
            "time_exit_status": int(exits[-1]),
            "method": "6 times largest GNU time descendant MaxRSS; kbytes converted by 1024"}


def self_check_time_bound():
    """Check synthetic GNU time parsing and KiB-to-byte upper-bound conversion."""
    log = ("Maximum resident set size (kbytes): 700\n"
           "Maximum resident set size (kbytes): 900\nExit status: 0\n")
    result = conservative_time_bound(log)
    assert result["parsed"] and result["maxrss_bytes"] == 900 * 1024
    assert result["upper_bound_bytes"] == 6 * 900 * 1024
    assert not conservative_time_bound("Exit status: 0\n")["parsed"]
    assert not conservative_time_bound("Maximum resident set size (kbytes): 900\nExit status: 143")["parsed"]


def replace_once(source, old, new):
    """Require one reviewed source anchor before adapting the house helper."""
    assert source.count(old) == 1, f"expected one source match: {old[:70]!r}"
    return source.replace(old, new)


def source_receipts_pass(status, workflow, identity, event_gates, timer_gates):
    """Require all numerical, identity and provenance receipts for the source case."""
    terminal = (status.get("state") == "terminal" and status.get("exit_code") == 0 and
                status.get("all_owned_handles_terminal") is True)
    unchanged = (status.get("inputs_unchanged") is True and
                 status.get("source_and_binary_pins_unchanged") is True)
    accepted = status.get("full_workflow_accepted") is True and workflow.get("workflow_accepted") is True
    checked = (workflow.get("P7_P8_events_checked") == [1, 2] and
               workflow.get("F3b_timer_events_checked") == [1, 2])
    identity_ok = identity.get("identity_passed") is True
    gates_ok = all(len(rows) == 2 and all(row.get("event") == event and row.get("passed") is True
                   for event, row in zip(EVENTS_REQUIRED, rows))
                   for rows in (event_gates, timer_gates))
    return bool(terminal and unchanged and accepted and checked and identity_ok and gates_ok)


def source_receipts(case):
    """Verify source acceptance independently of the isolated restart outputs."""
    status = json.loads((case / "status.json").read_text())
    workflow = json.loads((case / "full_workflow_receipt.json").read_text())
    identity = json.loads((case / "section6_1_reference_identity.json").read_text())
    events = [json.loads((case / f"p7p8_event{event}_gate.json").read_text()) for event in EVENTS_REQUIRED]
    timers = [json.loads((case / f"f3b_event{event}_timer_gate.json").read_text()) for event in EVENTS_REQUIRED]
    assert source_receipts_pass(status, workflow, identity, events, timers)


def self_check_records():
    """Exercise accepted and rejected source receipt shapes without CFD."""
    status = {"state": "terminal", "exit_code": 0, "all_owned_handles_terminal": True,
              "inputs_unchanged": True, "source_and_binary_pins_unchanged": True,
              "full_workflow_accepted": True}
    workflow = {"workflow_accepted": True, "P7_P8_events_checked": [1, 2],
                "F3b_timer_events_checked": [1, 2]}
    identity = {"identity_passed": True}
    gates = [{"event": event, "passed": True} for event in EVENTS_REQUIRED]
    assert source_receipts_pass(status, workflow, identity, gates, gates)
    assert not source_receipts_pass(dict(status, exit_code=143), workflow, identity, gates, gates)
    assert not source_receipts_pass(status, workflow, {"identity_passed": False}, gates, gates)
    assert not source_receipts_pass(status, workflow, identity, gates[:1], gates)


def retargeted_source():
    """Adapt the existing restart paths and retain its physical checks."""
    source = BASE_VALIDATOR.read_text()
    edits = (
        ('STAGED = HERE.parent', 'STAGED = HERE.parents[1]'),
        ('SOURCE = HERE / "full_workflow"',
         'SOURCE = Path(os.environ["SU2_SECTION61_SOURCE_CASE"]).resolve(strict=True)'),
        ('D = HERE / "disk_restart_event2"',
         'D = Path(os.environ["SU2_SECTION61_RESTART_CASE"]).resolve(strict=True)'),
        ('EVENTS = SOURCE / "independent_workflow_receipt.json"',
         'EVENTS = SOURCE / "full_workflow_receipt.json"'),
        ('assert workflow["workflow_accepted"] is True and len(workflow["events"]) == 2',
         'assert workflow["workflow_accepted"] is True and len(workflow["events"]) == 2\n'
         'source_status = json.loads((SOURCE / "status.json").read_text())\n'
         'identity = json.loads((SOURCE / "section6_1_reference_identity.json").read_text())\n'
         'event_gates = [json.loads((SOURCE / f"p7p8_event{e}_gate.json").read_text()) for e in (1, 2)]\n'
         'timer_gates = [json.loads((SOURCE / f"f3b_event{e}_timer_gate.json").read_text()) for e in (1, 2)]\n'
         'assert source_receipts_pass(source_status, workflow, identity, event_gates, timer_gates)'),
        ('(HERE / "build_status.json")', '(HERE / "build522/build_status.json")'),
        ('4657124b61fb6e0c0bf731c0086851fe0020cb562067d6d811d04e698db450f5', CFD_SHA),
        ('assert sha(binary_path) == build["binary_sha256"] == "' + CFD_SHA + '"',
         'assert sha(binary_path) == build["binary_sha256"] == "' + CFD_SHA + '"\n'
         'assert build["test_binary_sha256"] == "' + TEST_SHA + '"'),
        ('helpers = {str(p): sha(p) for p in (Path(__file__), Path(capcheck.__file__))}',
         'helpers = {str(p): sha(p) for p in (Path(__file__), Path(capcheck.__file__))}'),
        ('assert status["pins_verified"] and not status["changed"] and status["runtime_receipt_written"]',
         'assert status["pins_verified"] and not status["changed"] and status["runtime_receipt_written"]\n'
         'resource = json.loads((D / "resource_peak_receipt.json").read_text())\n'
         'assert resource["passed"] and resource["rank_process_count"] == 6\n'
         'assert resource["sum_rank_process_peak_hwm_bytes"] <= 19999490048\n'
         'audit = json.loads((D / "cpu_process_audit.json").read_text())\n'
         'assert audit["actual_host_proc"] and all(row["duration_seconds"] >= 3 and row["busy_count"] <= 6 for row in audit["windows"])\n'
         'time_bound = conservative_time_bound(log)\n'
         'proof = {"sampled_hwm_sum_bytes": resource["sum_rank_process_peak_hwm_bytes"], "sampled_hwm_is_lower_bound": True, "sampled_cpu_coverage": audit["summary"], "gnu_time": time_bound, "limit_bytes": 19999490048}\n'
         'proof["classification"] = "resource_proof_insufficient" if not time_bound["parsed"] or time_bound["upper_bound_bytes"] > proof["limit_bytes"] else "passed_conservative_upper_bound"\n'
         '(D / "resource_proof_receipt.json").write_text(json.dumps(proof, indent=2) + "\\n")'),
        ('assert post_terminal["actual_host_proc"] and post_terminal["all_owned_handles_terminal"]',
         'assert post_terminal["actual_host_proc"] and post_terminal["all_owned_handles_terminal"]\n'
         'assert post_terminal["controller_terminal"]'),
        ('"post_terminal_handles.json", "loaded_runtime_startup.json"',
         '"post_terminal_handles.json", "loaded_runtime_startup.json", "resource_peak_receipt.json", "cpu_process_audit.json", "resource_proof_receipt.json"'),
        ('receipt = {',
         'if proof["classification"] == "resource_proof_insufficient":\n'
         '    raise SystemExit("RESOURCE_PROOF_INSUFFICIENT: no production memory failure inferred")\n'
         'receipt = {'),
    )
    for old, new in edits:
        source = replace_once(source, old, new)
    compile(source, str(BASE_VALIDATOR), "exec")
    return source


def main():
    """Check arguments and execute only the selected validation action."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-case", type=Path)
    parser.add_argument("--case-dir", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    source = retargeted_source()
    if args.self_check:
        assert "full_workflow_receipt.json" in source
        assert CFD_SHA in source and "resource_peak_receipt.json" in source
        self_check_records()
        self_check_time_bound()
        print(json.dumps({"passed": True, "launched": False, "build_sha": CFD_SHA}))
        return
    assert args.source_case is not None and args.case_dir is not None
    source_case = args.source_case.resolve(strict=True)
    case = args.case_dir.resolve(strict=True)
    assert HERE in case.parents and case.is_dir()
    source_receipts(source_case)
    os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
    os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
    exec(compile(source, str(BASE_VALIDATOR), "exec"),
         {"__name__": "__main__", "__file__": str(HERE / "validate_disk_restart522.py"),
          "source_receipts_pass": source_receipts_pass,
          "conservative_time_bound": conservative_time_bound})


if __name__ == "__main__":
    main()
