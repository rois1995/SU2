#!/usr/bin/env python3
"""Launch or inspect one detached Section 6.2 full workflow controller."""
import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def launch_path(case):
    """Return a distinct durable launch receipt path for each case attempt."""
    return HERE / f"{case.name}_controller_launch.json"


def log_path(case):
    """Return a distinct controller log path for each case attempt."""
    return HERE / f"{case.name}_controller.log"


def proc_identity(pid):
    """Read a PID with its start tick so reused identities are rejected."""
    fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
    return {"pid": pid, "state": fields[0], "sid": int(fields[3]),
            "start_ticks": int(fields[19])}


def sha(path):
    """Hash the controller script recorded in the durable launch receipt."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def repeat_launch_allowed(cases):
    """Allow one repeat only after a gate-passing first run recorded N1 drift."""
    if not cases:
        return True
    if len(cases) != 1:
        return False
    case = cases[0]
    status_path = case / "status.json"
    compare_path = case / "section6_2_payload_comparison.json"
    if not status_path.is_file() or not compare_path.is_file():
        return False
    status = json.loads(status_path.read_text())
    comparison = json.loads(compare_path.read_text())
    return (status.get("state") == "terminal" and status.get("exit_code") == 0
            and status.get("full_workflow_accepted") is True
            and comparison.get("workflow_gates_passed") is True
            and comparison.get("repeat_required_after_all_gates") is True)


def launch_case(case):
    """Start one full workflow in a new session and persist its identity."""
    assert case.parent == HERE and not case.exists()
    prior_cases = sorted(path for path in HERE.iterdir() if path.is_dir()
                         and (path / "full_workflow_receipt.json").is_file())
    assert repeat_launch_allowed(prior_cases), "repeat requires one accepted first run with N1 payload drift"
    launch_file = launch_path(case)
    log_file = log_path(case)
    assert not launch_file.exists() and not log_file.exists()
    sys.path.insert(0, str(HERE))
    from build525_aliases import main as expose_build
    expose_build()
    build = json.loads((HERE / "build525/build_status.json").read_text())
    assert build.get("state") == "terminal" and build.get("exit_code") == 0
    assert build.get("pins_verified") is True and not build.get("changed")
    command = [sys.executable, str(HERE / "run_full_section6_2.py"), str(case)]
    receipt = {"state": "launching", "case": str(case), "argv": command,
               "started_unix": time.time(), "detached_session": True,
               "log": str(log_file), "launch_receipt": str(launch_file),
               "launcher_sha256": sha(__file__),
               "workflow_helper_sha256": sha(HERE / "run_full_section6_2.py")}
    launch_file.write_text(json.dumps(receipt, indent=2) + "\n")
    with log_file.open("x") as output:
        child = subprocess.Popen(command, cwd=HERE, stdout=output, stderr=subprocess.STDOUT,
                                 start_new_session=True)
    try:
        identity = proc_identity(child.pid)
        assert identity["start_ticks"] > 0 and identity["sid"] == child.pid
    except Exception as error:
        receipt.update(state="identity_capture_failed", child_pid=child.pid,
                       error=repr(error))
        launch_file.write_text(json.dumps(receipt, indent=2) + "\n")
        raise
    receipt.update(state="running", controller=identity)
    launch_file.write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def workflow_terminal(case, launch):
    """Check final physical, native-event, timer, pin, and ownership receipts."""
    status_path = case / "status.json"
    if not status_path.is_file():
        return {"state": "running", "status_present": False}
    status = json.loads(status_path.read_text())
    if status.get("state") != "terminal":
        return {"state": "running", "source_status": status}
    required = [case / "full_workflow_receipt.json"]
    required.extend(case / f"p7p8_event{event}_gate.json" for event in (1, 2))
    required.extend(case / f"f3b_event{event}_timer_gate.json" for event in (1, 2))
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        return {"state": "terminal", "passed": False,
                "missing_gate_receipts": missing, "source_status": status}
    workflow = json.loads((case / "full_workflow_receipt.json").read_text())
    checks = (status.get("exit_code") == 0 and status.get("all_owned_handles_terminal") is True
              and status.get("inputs_unchanged") is True
              and status.get("source_and_binary_pins_unchanged") is True
              and status.get("full_workflow_accepted") is True
              and workflow.get("workflow_accepted") is True
              and len(workflow.get("events", [])) == 2
              and workflow.get("P7_P8_events_checked") == [1, 2]
              and workflow.get("F3b_timer_events_checked") == [1, 2])
    strict = workflow.get("strict_gates", {})
    checks = (checks and strict.get("events") == 2
              and strict.get("per_event_native_seconds_max") == 5400
              and strict.get("no_fixed_mesh_point_or_cell_target") is True
              and strict.get("mesh_counts_must_match_certification_totals") is True
              and strict.get("q_j_geometry_size_bad_must_be_zero") is True)
    helpers_unchanged = (launch.get("launcher_sha256") == sha(__file__) and
                         launch.get("workflow_helper_sha256") == sha(HERE / "run_full_section6_2.py"))
    for event in (1, 2):
        native = json.loads((case / f"p7p8_event{event}_gate.json").read_text())
        timer = json.loads((case / f"f3b_event{event}_timer_gate.json").read_text())
        checks = checks and native.get("event") == event and native.get("passed") is True
        checks = checks and timer.get("event") == event and timer.get("passed") is True
    result = {"state": "terminal", "passed": bool(checks and helpers_unchanged), "source_status": status,
              "launch_helper_hashes_unchanged": helpers_unchanged,
              "workflow_accepted": workflow.get("workflow_accepted"),
              "owned_handles_terminal": status.get("all_owned_handles_terminal"),
              "P7_P8_events_checked": workflow.get("P7_P8_events_checked"),
              "F3b_timer_events_checked": workflow.get("F3b_timer_events_checked"),
              "controller_launch": launch}
    (case / "section6_2_controller_terminal.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def inspect(case):
    """Attach read-only to the recorded process and summarize terminal state."""
    launch = json.loads(launch_path(case).read_text())
    identity = launch["controller"]
    try:
        current = proc_identity(identity["pid"])
        alive = current["start_ticks"] == identity["start_ticks"] and current["state"] not in ("Z", "X")
    except (FileNotFoundError, ProcessLookupError, PermissionError):
        alive = False
    recorded_case = Path(launch["case"])
    assert recorded_case.resolve() == case.resolve()
    terminal = workflow_terminal(recorded_case, launch)
    if not alive and terminal.get("state") == "running":
        terminal = {"state": "controller_disappeared_without_terminal_receipt", "passed": False,
                    "source_status": terminal}
    result = {"launch_identity_alive": alive, "controller_identity": identity,
              "workflow": terminal}
    print(json.dumps(result))
    return result


def self_check():
    """Verify PID identity parsing and detached launcher paths without starting a job."""
    identity = proc_identity(os.getpid())
    assert identity["pid"] == os.getpid() and identity["start_ticks"] > 0
    case = HERE / "synthetic_case"
    assert launch_path(case).parent == HERE and log_path(case).parent == HERE
    assert repeat_launch_allowed([])
    assert not repeat_launch_allowed([case, case])
    assert "run_full_section6_2.py" in str(HERE / "run_full_section6_2.py")


def main():
    """Expose a nonblocking launch or attach-and-inspect action."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("launch", "inspect"))
    parser.add_argument("case", nargs="?", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print(json.dumps({"passed": True, "launched": False}))
        return
    if args.action == "launch":
        assert args.case is not None
        print(json.dumps(launch_case(args.case.resolve())))
    else:
        assert args.case is not None
        inspect(args.case.resolve())


if __name__ == "__main__":
    main()
