#!/usr/bin/env python3
"""Section 6.1 isolated restart wrapper; only runs after an accepted full case is selected."""
import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
BASE_RUNNER = STAGED / "p7p8_r3_20261115" / "run_disk_restart508.py"
CFD_SHA = "949359f28cd46490bd5b4e1389ccc22948952c73f360671b593e3e09a4f2d752"
TEST_SHA = "6de087f99ba03a0b27d73c369f7a72109f19fcd645869dee9b28cac8bed4cb91"
MAX_JOB_RSS = 19999490048
EVENTS_REQUIRED = (1, 2)


def replace_once(source, old, new):
    """Require one reviewed source anchor before adapting the house helper."""
    assert source.count(old) == 1, f"expected one source match: {old[:70]!r}"
    return source.replace(old, new)


def source_receipts_pass(status, workflow, identity, event_gates, timer_gates):
    """Require all numerical, identity and provenance receipts for the source case."""
    terminal = (status.get("state") == "terminal" and status.get("exit_code") == 0 and
                status.get("all_owned_handles_terminal") is True)
    provenance = (status.get("inputs_unchanged") is True and
                  status.get("source_and_binary_pins_unchanged") is True)
    full = workflow.get("workflow_accepted") is True and status.get("full_workflow_accepted") is True
    identity_ok = identity.get("identity_passed") is True
    checked = (workflow.get("P7_P8_events_checked") == [1, 2] and
               workflow.get("F3b_timer_events_checked") == [1, 2])
    gates_ok = all(len(items) == 2 and all(item.get("event") == event and
                    item.get("passed") is True for event, item in zip(EVENTS_REQUIRED, items))
                   for items in (event_gates, timer_gates))
    return bool(terminal and provenance and full and identity_ok and checked and gates_ok)


def self_check_records():
    """Exercise accepted and rejected source receipt shapes without CFD."""
    status = {"state": "terminal", "exit_code": 0, "all_owned_handles_terminal": True,
              "inputs_unchanged": True, "source_and_binary_pins_unchanged": True,
              "full_workflow_accepted": True}
    workflow = {"workflow_accepted": True, "P7_P8_events_checked": [1, 2],
                "F3b_timer_events_checked": [1, 2]}
    identity = {"identity_passed": True}
    events = [{"event": event, "passed": True} for event in EVENTS_REQUIRED]
    timers = [{"event": event, "passed": True} for event in EVENTS_REQUIRED]
    assert source_receipts_pass(status, workflow, identity, events, timers)
    for bad_status, bad_events in ((dict(status, state="running"), events),
                                   (dict(status, inputs_unchanged=False), events),
                                   (status, [events[0], {"event": 2, "passed": False}])):
        assert not source_receipts_pass(bad_status, workflow, identity, bad_events, timers)


def read_source_receipts(case):
    """Verify the selected complete source before opening restart payloads."""
    status = json.loads((case / "status.json").read_text())
    workflow = json.loads((case / "full_workflow_receipt.json").read_text())
    identity = json.loads((case / "section6_1_reference_identity.json").read_text())
    events = [json.loads((case / f"p7p8_event{event}_gate.json").read_text()) for event in EVENTS_REQUIRED]
    timers = [json.loads((case / f"f3b_event{event}_timer_gate.json").read_text()) for event in EVENTS_REQUIRED]
    assert source_receipts_pass(status, workflow, identity, events, timers), "selected case lacks complete Section 6.1 acceptance receipts"
    return status, workflow, identity, events, timers


def retargeted_source():
    """Adapt the existing restart paths and retain its physical checks."""
    source = BASE_RUNNER.read_text()
    edits = (
        ('sys.path.insert(0, str(Path(__file__).resolve().parents[1]))',
         'sys.path.insert(0, str(Path(__file__).resolve().parents[2]))'),
        ('STAGED = HERE.parent', 'STAGED = HERE.parents[1]'),
        ('SOURCE = HERE / "full_workflow"',
         'SOURCE = Path(os.environ["SU2_SECTION61_SOURCE_CASE"]).resolve(strict=True)'),
        ('FOLDER = HERE / "disk_restart_event2"',
         'FOLDER = Path(os.environ["SU2_SECTION61_RESTART_CASE"]).resolve()'),
        ('EVENTS = SOURCE / "independent_workflow_receipt.json"',
         'EVENTS = SOURCE / "full_workflow_receipt.json"'),
        ('assert source_status["state"]=="terminal" and source_status["exit_code"]==0 and source_status["all_owned_handles_terminal"]',
         'assert source_status["state"]=="terminal" and source_status["exit_code"]==0 and source_status["all_owned_handles_terminal"]\n'
         'assert source_status.get("inputs_unchanged") is True and source_status.get("source_and_binary_pins_unchanged") is True\n'
         'assert source_status.get("full_workflow_accepted") is True'),
        ('assert workflow["workflow_accepted"] is True and len(workflow["events"]) == 2',
         'assert workflow["workflow_accepted"] is True and len(workflow["events"]) == 2\n'
         'assert workflow["P7_P8_events_checked"] == [1, 2] and workflow["F3b_timer_events_checked"] == [1, 2]\n'
         'identity = json.loads((SOURCE / "section6_1_reference_identity.json").read_text())\n'
         'assert identity["identity_passed"] is True\n'
         'for checked_event in (1, 2):\n'
         '    assert json.loads((SOURCE / f"p7p8_event{checked_event}_gate.json").read_text())["passed"] is True\n'
         '    assert json.loads((SOURCE / f"f3b_event{checked_event}_timer_gate.json").read_text())["passed"] is True'),
        ('BUILD = HERE / "build_status.json"', 'BUILD = HERE / "build522/build_status.json"'),
        ('SOURCE_PINS = HERE / "source_pins.json"', 'SOURCE_PINS = HERE / "build522/source_pins.json"'),
        ('EXTRA_SOURCE_PINS = HERE / "unchanged_build499_source_pins.json"',
         'EXTRA_SOURCE_PINS = HERE / "unchanged_build499_source_pins.json"'),
        ('4657124b61fb6e0c0bf731c0086851fe0020cb562067d6d811d04e698db450f5', CFD_SHA),
        ('assert build["state"] == "terminal" and build["exit_code"] == 0 and build["pins_verified"]',
         'assert build["state"] == "terminal" and build["exit_code"] == 0 and build["pins_verified"]\n'
         f'assert build["test_binary_sha256"] == "{TEST_SHA}"'),
        ('for path in (Path(__file__), BUILD, SOURCE_PINS, EXTRA_SOURCE_PINS, RUNTIME_MANIFEST, EVENTS, CFD,',
         'for path in (Path(__file__), STAGED / "p7p8_r3_20261115/run_disk_restart508.py", '
         'STAGED / "p7p8_r3_20261115/validate_disk_restart508.py", BUILD, SOURCE_PINS, '
         'EXTRA_SOURCE_PINS, RUNTIME_MANIFEST, EVENTS, CFD, SOURCE / "section6_1_reference_identity.json", '
         'SOURCE / "p7p8_event1_gate.json", SOURCE / "p7p8_event2_gate.json", '
         'SOURCE / "f3b_event1_timer_gate.json", SOURCE / "f3b_event2_timer_gate.json",'),
        ('"600", "mpirun",\n        "--output-filename"',
         '"600", "mpirun", "--oversubscribe", "--bind-to", "none",\n        "--output-filename"'),
        ('OMPI_MCA_osc="pt2pt", TMPDIR=str(scratch_root),',
         'OMPI_MCA_osc="pt2pt", OMPI_MCA_mpi_yield_when_idle="1", TMPDIR=str(scratch_root),'),
        ('HERE / "validate_disk_restart508.py"', 'HERE / "validate_disk_restart522.py"'),
    )
    for old, new in edits:
        source = replace_once(source, old, new)
    source = inject_resource_receipts(source)
    compile(source, str(BASE_RUNNER), "exec")
    return source


def inject_resource_receipts(source):
    """Attach independent CPU and memory observation to the house controller."""
    old_record = '"sampled_process_tree_rss_bytes": sum(x["rss_bytes"] for x in rows)}) + "\\n")'
    new_record = '"sampled_process_tree_rss_bytes": sum(x["rss_bytes"] for x in rows), **resource_snapshot}) + "\\n")'
    source = replace_once(source, old_record, new_record)
    old_loop = '        if not runtime_written and runtime_error is None:\n'
    source = replace_once(source, old_loop, '        resource_snapshot = observe_resources(run_session)\n' + old_loop)
    old_rows = '        rows = sample_tree(process.pid)\n'
    source = replace_once(source, old_rows, old_rows)
    old_status = '''save(FOLDER / "status.json", {"state": "terminal", "exit_code": code,
    "seconds": time.time() - started, "pins_verified": not changed, "changed": changed,
    "disk_restart_validated": False, "runtime_receipt_written": runtime_written, "runtime_observation_error": runtime_error})'''
    new_status = '''resource_receipt = resource_summary()
save(FOLDER / "resource_peak_receipt.json", resource_receipt)
save(FOLDER / "cpu_process_audit.json", {"actual_host_proc": True,
    "windows": cpu_windows, "summary": resource_receipt["cpu_monitor"]})
save(FOLDER / "status.json", {"state": "terminal", "exit_code": code,
    "seconds": time.time() - started, "pins_verified": not changed, "changed": changed,
    "disk_restart_validated": False, "runtime_receipt_written": runtime_written,
    "runtime_observation_error": runtime_error, "resource_gate": resource_receipt})'''
    source = replace_once(source, old_status, new_status)
    old_assert = 'assert runtime_written and not live_session and all(x["terminal"] for x in terminal)'
    new_assert = old_assert + ' and resource_receipt["passed"]'
    source = replace_once(source, old_assert, new_assert)
    return source.replace('def save(path, value):\n', resource_helpers() + '\ndef save(path, value):\n', 1)


def resource_helpers():
    """Combine the generated resource services used by the controller."""
    return resource_helper_state() + resource_helper_observer() + resource_helper_summary()


def resource_helper_state():
    """Define retained process peaks and CPU observation state."""
    return """
MAX_JOB_RSS = 19999490048
MAX_BUSY_PROCESSES = 6
OWNED_CPUS = set(int(value) for value in os.environ["SU2_SECTION61_ALLOWED_CPUS"].split(","))
affinity_violations = set()
CPU_TICKS_PER_SECOND = os.sysconf("SC_CLK_TCK")
worker_peaks = {}
cpu_windows = []
cpu_previous = None
cpu_last_sample = None
cpu_window_started = None
cpu_window_elapsed = 0.0
cpu_window_deltas = {}
cpu_monitor_started = None
cpu_monitor_finished = None
cpu_unmatched_starts = 0
cpu_unmatched_exits = 0
cpu_max_busy_processes = 0

def keep_peak(peaks, key, value):
    'Keep each observed process peak monotone.'
    peaks[key] = max(peaks.get(key, 0), int(value))
    return peaks[key]

def busy_cpu_rows(deltas, elapsed, tick_rate=CPU_TICKS_PER_SECOND):
    'Count every process identity above half of one core.'
    if elapsed < 3.0:
        return None
    busy = []
    for identity, row in deltas.items():
        fraction = int(row["ticks"]) / tick_rate / elapsed * 100.0
        if fraction > 50.0:
            busy.append({"pid": identity[0], "start_ticks": identity[1],
                         "comm": row["comm"], "percent_one_core": fraction})
    return busy

"""


def resource_helper_observer():
    """Generate the nonblocking all-process CPU and sticky-HWM observer."""
    return """def observe_resources(session):
    'Sample all processes without delaying the owned CFD job.'
    global cpu_previous, cpu_last_sample, cpu_window_started, cpu_window_elapsed
    global cpu_window_deltas, cpu_monitor_started, cpu_monitor_finished
    global cpu_unmatched_starts, cpu_unmatched_exits, cpu_max_busy_processes
    sampled_at = time.monotonic()
    if cpu_monitor_started is None:
        cpu_monitor_started = sampled_at
        cpu_window_started = sampled_at
    cpu_monitor_finished = sampled_at
    workers = []
    processes = {}
    for proc in Path("/proc").glob("[0-9]*"):
        try:
            pid = int(proc.name)
            name = (proc / "comm").read_text().strip()
            fields = (proc / "stat").read_text().rsplit(")", 1)[1].split()
            identity = (pid, int(fields[19]))
            processes[identity] = {"comm": name, "ticks": int(fields[11]) + int(fields[12])}
            if int(fields[3]) == session and name == "SU2_CFD":
                hwm = next(line for line in (proc / "status").read_text().splitlines()
                           if line.startswith("VmHWM:"))
                peak = int(hwm.split()[1]) * 1024
                key = f"{pid}:{identity[1]}"
                keep_peak(worker_peaks, key, peak)
                allowed = set(os.sched_getaffinity(pid))
                if not allowed.issubset(OWNED_CPUS):
                    affinity_violations.add(key)
                workers.append({"pid": pid, "start_ticks": identity[1], "VmHWM_bytes": peak})
        except (OSError, ValueError, StopIteration, IndexError):
            continue
    completed_window = None
    if cpu_previous is not None:
        elapsed = sampled_at - cpu_last_sample
        cpu_window_elapsed += elapsed
        previous_ids = set(cpu_previous)
        current_ids = set(processes)
        cpu_unmatched_starts += len(current_ids - previous_ids)
        cpu_unmatched_exits += len(previous_ids - current_ids)
        for identity in previous_ids & current_ids:
            delta = max(0, processes[identity]["ticks"] - cpu_previous[identity]["ticks"])
            row = cpu_window_deltas.setdefault(identity, {"comm": processes[identity]["comm"], "ticks": 0})
            row["ticks"] += delta
            row["comm"] = processes[identity]["comm"]
        if cpu_window_elapsed >= 3.0:
            busy = busy_cpu_rows(cpu_window_deltas, cpu_window_elapsed)
            completed_window = {"start_monotonic": cpu_window_started,
                "end_monotonic": sampled_at, "duration_seconds": cpu_window_elapsed,
                "processes_compared": len(cpu_window_deltas), "unmatched_starts": cpu_unmatched_starts,
                "unmatched_exits": cpu_unmatched_exits, "busy_processes": busy,
                "busy_count": len(busy), "scope": "all /proc processes keyed by PID/start_ticks"}
            cpu_windows.append(completed_window)
            cpu_max_busy_processes = max(cpu_max_busy_processes, len(busy))
            cpu_window_started = sampled_at
            cpu_window_elapsed = 0.0
            cpu_window_deltas = {}
            cpu_unmatched_starts = 0
            cpu_unmatched_exits = 0
    cpu_previous = processes
    cpu_last_sample = sampled_at
    cpu_covered = sum(row["duration_seconds"] for row in cpu_windows)
    cpu_elapsed = max(0.0, (cpu_monitor_finished or sampled_at) - (cpu_monitor_started or sampled_at))
    cpu_monitor = {"passed": bool(cpu_windows) and cpu_max_busy_processes <= MAX_BUSY_PROCESSES,
        "window_count": len(cpu_windows), "window_min_seconds": min(
            (row["duration_seconds"] for row in cpu_windows), default=0.0),
        "window_max_seconds": max((row["duration_seconds"] for row in cpu_windows), default=0.0),
        "covered_seconds": cpu_covered, "observed_seconds": cpu_elapsed,
        "coverage_fraction": cpu_covered / cpu_elapsed if cpu_elapsed else 0.0,
        "max_busy_processes": cpu_max_busy_processes, "busy_process_limit": MAX_BUSY_PROCESSES,
        "process_scope": "all /proc processes keyed by PID/start_ticks",
        "window_method": "sample CPU ticks opportunistically; emit when accumulated interval reaches 3s; no added wait"}
    return {"rank_process_peak_hwm_bytes": worker_peaks,
            "sum_rank_process_peak_hwm_bytes": sum(worker_peaks.values()),
            "rank_process_count": len(worker_peaks), "rank_processes": workers,
            "cpu_window": completed_window, "cpu_monitor": cpu_monitor}

"""


def resource_helper_summary():
    """Generate the final resource evidence and coverage assessment."""
    return """def resource_summary():
    'Report sticky memory and the actual CPU-window coverage.'
    total = sum(worker_peaks.values())
    windows = [row["duration_seconds"] for row in cpu_windows]
    cpu_covered = bool(windows) and min(windows) >= 3.0 and cpu_max_busy_processes <= MAX_BUSY_PROCESSES
    passed = len(worker_peaks) == 6 and total <= MAX_JOB_RSS and cpu_covered and not affinity_violations
    observed = max(0.0, (cpu_monitor_finished or 0.0) - (cpu_monitor_started or 0.0))
    covered = sum(windows)
    cpu_monitor = {"passed": cpu_covered, "window_count": len(windows),
        "window_min_seconds": min(windows, default=0.0), "window_max_seconds": max(windows, default=0.0),
        "covered_seconds": covered, "observed_seconds": observed,
        "coverage_fraction": covered / observed if observed else 0.0,
        "max_busy_processes": cpu_max_busy_processes, "busy_process_limit": MAX_BUSY_PROCESSES,
        "process_scope": "all /proc processes keyed by PID/start_ticks",
        "window_method": "sample CPU ticks opportunistically; emit when accumulated interval reaches 3s; no added wait"}
    return {"passed": passed, "rank_process_count": len(worker_peaks),
            "rank_process_peak_hwm_bytes": worker_peaks,
            "sum_rank_process_peak_hwm_bytes": total,
            "limit_bytes": MAX_JOB_RSS, "hwm_sample_before_runtime_check": True,
            "owned_cpu_ids": sorted(OWNED_CPUS), "affinity_violations": sorted(affinity_violations),
            "cpu_monitor": cpu_monitor}
"""

def wait_identity(pid):
    """Capture the child start stamp before supervising it."""
    deadline = time.monotonic() + 3.0
    while time.monotonic() < deadline:
        try:
            return process_identity(pid)
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            time.sleep(0.01)
    raise RuntimeError(f"could not capture child identity for pid {pid}")


def self_check_resource_helpers():
    """Exercise unknown-process CPU detection and monotone process peaks."""
    os.environ.setdefault("SU2_SECTION61_ALLOWED_CPUS", "0,1")
    namespace = {"os": os, "Path": Path, "time": time}
    exec(resource_helpers(), namespace)
    keep_peak = namespace["keep_peak"]
    peaks = {}
    assert keep_peak(peaks, "101:5", 120) == 120
    assert keep_peak(peaks, "101:5", 80) == 120
    rows = {(101, 5): {"comm": "SU2_CFD", "ticks": 220},
            (202, 9): {"comm": "python3", "ticks": 210}}
    busy = namespace["busy_cpu_rows"](rows, 3.1, tick_rate=100)
    assert {row["comm"] for row in busy} == {"SU2_CFD", "python3"}
    assert namespace["busy_cpu_rows"](rows, 2.99, tick_rate=100) is None
    namespace["worker_peaks"].update({str(rank): 1 for rank in range(6)})
    namespace["cpu_windows"].append({"duration_seconds": 3.1})
    assert namespace["resource_summary"]()["passed"]
    namespace["affinity_violations"].add("synthetic-outside-allocation")
    assert not namespace["resource_summary"]()["passed"]


def apply_owned_cpu_budget():
    """Keep six MPI ranks and their inspector inside one two-core CPU allocation."""
    allowed = sorted(os.sched_getaffinity(0))
    assert len(allowed) >= 2, "isolated restart needs two available CPU identities"
    selected = allowed[:2]
    os.sched_setaffinity(0, selected)
    os.environ["SU2_SECTION61_ALLOWED_CPUS"] = ",".join(map(str, selected))
    return selected


def process_identity(pid):
    """Read a PID start stamp so reuse cannot be mistaken for ownership."""
    fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
    return {"pid": pid, "start_ticks": int(fields[19]), "state": fields[0], "sid": int(fields[3])}


def post_terminal_receipt(case, controller):
    """Verify the restart controller and every observed owned process are terminal."""
    terminal = json.loads((case / "terminal_handles.json").read_text())
    controller_id = terminal["controller_identity"]
    try:
        now = process_identity(controller_id["pid"])
        controller_gone = now["start_ticks"] != controller_id["start_ticks"] or now["state"] in ("Z", "X")
    except (FileNotFoundError, ProcessLookupError, PermissionError):
        controller_gone = True
    session = terminal["actual_host_handles"]
    rows = []
    for identity in session:
        try:
            current = process_identity(identity["pid"])
            alive = current["start_ticks"] == identity["start_ticks"] and current["state"] not in ("Z", "X")
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            alive = False
        rows.append({"identity": identity, "alive": alive})
    active_session = [row for row in rows if row["alive"]]
    receipt = {"actual_host_proc": True, "all_owned_handles_terminal": not active_session and controller_gone,
        "controller_identity": controller_id, "controller_terminal": controller_gone,
        "controller_wrapper_identity": controller, "owned_identity_checks": rows,
        "live_session_members": active_session, "scope": "restart runner/controller plus its owned CFD session"}
    (case / "post_terminal_handles.json").write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def main():
    """Check arguments and execute only the selected validation action."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-case", type=Path)
    parser.add_argument("--case-dir", type=Path)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--internal", action="store_true")
    args = parser.parse_args()
    source = retargeted_source()
    if args.self_check:
        assert "full_workflow_receipt.json" in source
        assert CFD_SHA in source and "resource_peak_receipt.json" in source
        assert '"--oversubscribe", "--bind-to", "none"' in source
        assert 'OMPI_MCA_mpi_yield_when_idle="1"' in source
        self_check_records()
        self_check_resource_helpers()
        print(json.dumps({"passed": True, "launched": False, "build_sha": CFD_SHA}))
        return
    assert args.source_case is not None and args.source_case.is_dir()
    source_case = args.source_case.resolve(strict=True)
    read_source_receipts(source_case)
    apply_owned_cpu_budget()
    case = args.case_dir.resolve() if args.case_dir else HERE / f"disk_restart_{source_case.name}_event2"
    assert HERE in case.parents and not case.exists()
    if args.internal:
        os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
        os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
        exec(compile(source, str(BASE_RUNNER), "exec"), {"__name__": "__main__", "__file__": str(HERE / "run_disk_restart522.py")})
        return
    supervise(source, source_case, case)


def supervise(source, source_case, case):
    """Wait for the isolated controller and seal its actual terminal identities."""
    argv = [sys.executable, str(HERE / "run_disk_restart522.py"), "--internal",
            "--source-case", str(source_case), "--case-dir", str(case)]
    child = subprocess.Popen(argv, cwd=HERE, start_new_session=True)
    identity = wait_identity(child.pid)
    wrapper = {"pid": child.pid, "start_ticks": identity["start_ticks"],
               "started_unix": time.time(), "argv": argv}
    (case.parent / f"{case.name}_controller_launch.json").write_text(json.dumps(wrapper, indent=2) + "\n")
    wrapper["owned_cpu_ids"] = sorted(os.sched_getaffinity(0))
    code = child.wait()
    receipt = post_terminal_receipt(case, wrapper)
    assert receipt["all_owned_handles_terminal"], receipt
    raise SystemExit(code)


if __name__ == "__main__":
    main()
