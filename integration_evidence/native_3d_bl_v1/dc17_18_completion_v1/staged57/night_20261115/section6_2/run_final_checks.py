#!/usr/bin/env python3
"""Run 481, current-target, and 2D checks in order; stop on the first failure."""
import json
import os
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
MODES = ("merit481", "saved_current", "2d1", "2d2", "2d4")
MAX_JOB_PEAK_BYTES = 19_999_490_048
HEAVY_NAMES = {"SU2_CFD", "test_driver", "cc1plus", "ninja", "lto1",
               "mpirun", "orterun", "orted", "prterun", "prte"}
HELPER_MARKERS = ("run_units.py", "run_remaining_checks.py", "run_first_collapse_section6_2.py",
                  "run_final_checks.py", "run_full_section6_2.py", "launch_full_section6_2.py")
TICKS_PER_SECOND = os.sysconf("SC_CLK_TCK")


def save(path, value):
    """Write the current validation receipt."""
    path.write_text(json.dumps(value, indent=2) + "\n")


def process_start_ticks(pid):
    """Read the process identity used to reject PID reuse."""
    try:
        stat = (Path("/proc") / str(pid) / "stat").read_text()
        return int(stat.rsplit(")", 1)[1].split()[19])
    except (FileNotFoundError, ProcessLookupError):
        return None


def proc_cpu_snapshot():
    """Read CPU ticks and stable identities for every visible process."""
    result = {}
    for path in Path("/proc").glob("[0-9]*"):
        try:
            fields = (path / "stat").read_text().rsplit(")", 1)[1].split()
            identity = (int(path.name), int(fields[19]))
            result[identity] = {"pid": identity[0], "start_ticks": identity[1],
                                "comm": (path / "comm").read_text().strip(),
                                "cpu_ticks": int(fields[11]) + int(fields[12]),
                                "sid": int(fields[3]), "command": (path / "cmdline").read_bytes().decode(errors="replace")}
        except (OSError, ValueError, IndexError):
            continue
    return result


def cpu_delta_rows(before, after, seconds):
    """Return per-process CPU use in percent of one core for stable identities."""
    rows = []
    for identity, current in after.items():
        prior = before.get(identity)
        ticks = current["cpu_ticks"] - prior["cpu_ticks"] if prior else None
        percent = 100.0 * ticks / TICKS_PER_SECOND / seconds if ticks is not None else None
        rows.append({"pid": identity[0], "start_ticks": identity[1],
                     "comm": current["comm"], "cpu_percent_of_one_core": percent,
                     "appeared_during_sample": prior is None})
    for identity, prior in before.items():
        if identity not in after:
            rows.append({"pid": identity[0], "start_ticks": identity[1],
                         "comm": prior["comm"], "cpu_percent_of_one_core": None,
                         "disappeared_during_sample": True})
    return rows


def is_runner_process(name, command):
    """Match Python helpers only; shell argv text is not a runner identity."""
    return name.startswith("python") and any(marker in command for marker in HELPER_MARKERS)


def competing_processes(preflight, controller_pid):
    """Return owned heavy jobs and long-running owned helper processes only."""
    rows = list(preflight["active_heavy_processes"])
    rows.extend(row for row in preflight["owned_helper_processes_over_50_percent_60s"]
                if row["pid"] != controller_pid)
    unique = {(row["pid"], row["start_ticks"]): row for row in rows}
    return list(unique.values())


def receipt_identities(value):
    """Collect PID/start-tick identities from this section's own launch receipts."""
    found = []
    if isinstance(value, dict):
        if isinstance(value.get("pid"), int) and isinstance(value.get("start_ticks"), int):
            found.append((value["pid"], value["start_ticks"]))
        for child in value.values():
            found.extend(receipt_identities(child))
    elif isinstance(value, list):
        for child in value:
            found.extend(receipt_identities(child))
    return found


def owned_sessions():
    """Read only this section's own controller receipts to identify active sessions."""
    roots = set()
    files = list(HERE.glob("*controller_launch.json"))
    files.extend(HERE.glob("final_*_receipt.json"))
    files.extend(HERE.glob("*/status.json"))
    files.extend((HERE / "cases").glob("**/status.json"))
    for path in files:
        try:
            record = json.loads(path.read_text())
            roots.update(receipt_identities(record))
            pid = record.get("pid")
            if isinstance(pid, int):
                fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
                if int(fields[3]) == pid:
                    roots.add((pid, int(fields[19])))
        except (OSError, ValueError, IndexError, json.JSONDecodeError):
            continue
    return roots


def owned_helper_sustained(row, uptime, owned_sids):
    """Require a section helper to average above half a core for at least 60 seconds."""
    elapsed = max(0.0, uptime - row["start_ticks"] / TICKS_PER_SECOND)
    average_cpu = row["cpu_ticks"] / TICKS_PER_SECOND
    return (row["pid"] != os.getpid() and row["sid"] in owned_sids
            and is_runner_process(row["comm"], row["command"])
            and elapsed >= 60.0 and average_cpu / elapsed >= 0.50)


def host_preflight():
    """Observe all processes, but block only owned workloads and long-running helpers."""
    before = proc_cpu_snapshot()
    started = time.time()
    time.sleep(3)
    elapsed = time.time() - started
    after = proc_cpu_snapshot()
    rows = cpu_delta_rows(before, after, elapsed)
    hot = [row for row in rows if row["cpu_percent_of_one_core"] is not None
           and row["cpu_percent_of_one_core"] > 50 and row["pid"] != os.getpid()]
    heavy = []
    roots = owned_sessions()
    owned_sids = {pid for pid, ticks in roots if (pid, ticks) in after}
    own_helpers = []
    uptime = float(Path("/proc/uptime").read_text().split()[0])
    for identity, row in after.items():
        if row["sid"] in owned_sids and row["comm"] in HEAVY_NAMES:
            heavy.append({"pid": row["pid"], "start_ticks": row["start_ticks"],
                          "name": row["comm"], "sid": row["sid"]})
        average_cpu = row["cpu_ticks"] / TICKS_PER_SECOND
        if owned_helper_sustained(row, uptime, owned_sids):
            elapsed = max(0.0, uptime - row["start_ticks"] / TICKS_PER_SECOND)
            own_helpers.append({"pid": row["pid"], "start_ticks": row["start_ticks"],
                                "name": row["comm"], "average_cpu_seconds": average_cpu,
                                "observed_running_seconds": elapsed})
    available = next(line for line in Path("/proc/meminfo").read_text().splitlines()
                     if line.startswith("MemAvailable:"))
    return {"started_unix": started, "sample_seconds": elapsed,
            "loadavg": Path("/proc/loadavg").read_text().strip(),
            "MemAvailable": available, "process_samples": rows,
            "hot_other_processes_over_50_percent_core": hot,
            "active_heavy_processes": heavy,
            "owned_helper_processes_over_50_percent_60s": own_helpers,
            "owned_controller_roots": sorted([{"pid": pid, "start_ticks": ticks} for pid, ticks in roots],
                                              key=lambda row: (row["pid"], row["start_ticks"])),
            "controller_pid_excluded": os.getpid()}


def rss_peak_bound_text(text, ranks):
    """Preserve the requested validation data and process identity."""
    peaks = [int(value) * 1024 for value in
             re.findall(r"Maximum resident set size \(kbytes\):\s*(\d+)", text)]
    largest = max(peaks) if peaks else None
    return largest, ranks * largest if largest is not None else None


def rss_peak_bound(case_log, ranks):
    """Read the conservative rank peak bound from the actual case log."""
    return rss_peak_bound_text(Path(case_log).read_text(errors="replace"), ranks)


def case_rss_log(case):
    """Preserve the requested validation data and process identity."""
    return Path(case) / "run.log"


def child_identity(process):
    """Preserve the requested validation data and process identity."""
    return {"pid": process.pid, "start_ticks": process_start_ticks(process.pid)}


def case_status(case):
    """Preserve the requested validation data and process identity."""
    path = case / "status.json"
    return json.loads(path.read_text()) if path.is_file() else None


def run_mode(mode):
    """Run one isolated mode and preserve its terminal gate receipt."""
    case_root = HERE / "cases"
    before = set(case_root.iterdir())
    preflight = host_preflight()
    command = [sys.executable, str(HERE / "run_remaining_checks.py"), mode]
    started = time.time()
    log_path = HERE / f"final_{mode}.log"
    receipt_path = HERE / f"final_{mode}_receipt.json"
    receipt = {"mode": mode, "command": command, "preflight": preflight,
               "controller": {"pid": os.getpid(), "start_ticks": process_start_ticks(os.getpid())},
               "started_unix": started, "state": "preflight_complete"}
    blockers = competing_processes(preflight, os.getpid())
    if blockers:
        error = "competing process present in preflight sample"
        receipt.update(state="preflight_blocked", result={"passed": False,
                       "launched": False, "error": error,
                       "blockers": blockers}, terminal_unix=time.time())
        save(receipt_path, receipt)
        return receipt["result"]
    with log_path.open("x") as log:
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                   start_new_session=True)
        receipt["child"] = child_identity(process)
        receipt["state"] = "running"
        save(receipt_path, receipt)
        while process.poll() is None:
            time.sleep(1)
        code = process.wait()
    new_cases = sorted(set(case_root.iterdir()) - before)
    ranks = int(mode[-1]) if mode.startswith("2d") else 1
    case = new_cases[0] if len(new_cases) == 1 else None
    status = case_status(case) if case else None
    case_log = case_rss_log(case) if case else None
    largest, bound = rss_peak_bound(case_log, ranks) if case_log and case_log.is_file() else (None, None)
    passed = (len(new_cases) == 1 and code == 0 and status is not None
              and status.get("exit_code") == 0 and status.get("pins_verified")
              and not status.get("changed") and bound is not None
              and bound <= MAX_JOB_PEAK_BYTES)
    result = {"mode": mode, "exit_code": code, "seconds": time.time() - started,
              "case": str(case) if case else None, "status": status,
              "rank_count": ranks, "rss_source": str(case_log) if case_log else None,
              "largest_descendant_peak_RSS_bytes": largest,
              "sum_rank_peak_RSS_upper_bound_bytes": bound,
              "strict_19073_MiB_cap_bytes": MAX_JOB_PEAK_BYTES, "passed": passed}
    receipt.update(state="terminal", exit_code=code, result=result, terminal_unix=time.time())
    save(receipt_path, receipt)
    return result


def self_check():
    """Exercise helper setup without launching a numerical run."""
    delta = int(TICKS_PER_SECOND * 3 * 0.60)
    before = {(11, 101): {"cpu_ticks": 20, "comm": "python3"},
              (12, 202): {"cpu_ticks": 0, "comm": "shell"}}
    after = {(11, 101): {"pid": 11, "start_ticks": 101, "comm": "python3", "cpu_ticks": 20 + delta},
             (12, 202): {"pid": 12, "start_ticks": 202, "comm": "shell", "cpu_ticks": 0}}
    rows = cpu_delta_rows(before, after, 3)
    hot = [row for row in rows if row["cpu_percent_of_one_core"] > 50]
    assert len(hot) == 1 and hot[0]["pid"] == 11
    synthetic = {"active_heavy_processes": [], "owned_helper_processes_over_50_percent_60s": [],
                 "hot_other_processes_over_50_percent_core": hot}
    assert competing_processes(synthetic, 99) == []
    synthetic["owned_helper_processes_over_50_percent_60s"] = [{"pid": 21, "start_ticks": 202}]
    assert competing_processes(synthetic, 99)[0]["start_ticks"] == 202
    assert is_runner_process("python3", "python3 run_remaining_checks.py 2d2")
    assert not is_runner_process("bash", "bash -c python3 run_remaining_checks.py 2d2")
    helper = {"pid": 22, "start_ticks": 10_000, "cpu_ticks": 35 * TICKS_PER_SECOND,
              "comm": "python3", "command": "python3 run_units.py", "sid": 22}
    assert owned_helper_sustained(helper, 170, {22})
    assert not owned_helper_sustained(dict(helper, cpu_ticks=10 * TICKS_PER_SECOND), 170, {22})
    assert not owned_helper_sustained(helper, 159, {22})
    assert not owned_helper_sustained(helper, 170, set())
    with tempfile.TemporaryDirectory() as folder:
        case = Path(folder) / "case"
        case.mkdir()
        (case / "run.log").write_text("Maximum resident set size (kbytes): 1000\n")
        controller = Path(folder) / "controller.log"
        controller.write_text("Maximum resident set size (kbytes): 9000\n")
        assert rss_peak_bound(case_rss_log(case), 4) == (1_024_000, 4_096_000)
        assert rss_peak_bound(controller, 4) == (9_216_000, 36_864_000)
    assert rss_peak_bound_text("no RSS row", 4) == (None, None)
    assert case_rss_log("case-dir") == Path("case-dir/run.log")
    assert MAX_JOB_PEAK_BYTES == 19_999_490_048


def main():
    """Run the selected ordered validation action."""
    if sys.argv[1:] == ["--self-check"]:
        self_check()
        print(json.dumps({"passed": True, "launched": False,
                          "job_peak_cap_bytes": MAX_JOB_PEAK_BYTES}))
        return
    case_root = HERE / "cases"
    case_root.mkdir(exist_ok=True)
    sequence_path = HERE / "final_checks_sequence.json"
    assert not sequence_path.exists(), "refusing to overwrite an existing check sequence"
    for mode in MODES:
        assert not (HERE / f"final_{mode}.log").exists()
        assert not (HERE / f"final_{mode}_receipt.json").exists()
    rows = []
    save(sequence_path, {"completed": rows, "next": MODES[0], "state": "running"})
    for mode in MODES:
        row = run_mode(mode)
        rows.append(row)
        passed = row["passed"]
        next_mode = MODES[len(rows)] if passed and len(rows) < len(MODES) else None
        state = "preflight_blocked" if row.get("launched") is False else (
            "complete" if passed and next_mode is None else "running" if passed else "failed")
        save(sequence_path, {"completed": rows, "next": next_mode, "state": state})
        print(json.dumps(row), flush=True)
        if not passed:
            raise SystemExit(2 if row.get("launched") is False else 1)


if __name__ == "__main__":
    main()
