"""Run one isolated MPI6 CFD continuation of R3's accepted P7/P8 event-two mesh."""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57')))
from run_cases import sample_tree

HERE = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/validation')
STAGED = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57')
SOURCE = Path(os.environ["SU2_SECTION61_SOURCE_CASE"]).resolve(strict=True)
FOLDER = Path(os.environ["SU2_SECTION61_RESTART_CASE"]).resolve()
EVENTS = SOURCE / "full_workflow_receipt.json"
BUILD = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/build552') / "build_status.json"
SOURCE_PINS = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N14/section6_12/build552') / "source_pins.json"
EXTRA_SOURCE_PINS = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N8/A2_A3_N12/section6_8/validation/unchanged_build499_source_pins.json')
RUNTIME_MANIFEST = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
CFD = Path(json.loads(BUILD.read_text())["binary"]).resolve()

sha = lambda p: hashlib.file_digest(Path(p).open("rb"), "sha256").hexdigest()


def proc_stat(pid):
    fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
    return {"state": fields[0], "pgid": int(fields[2]), "sid": int(fields[3]),
            "start_ticks": int(fields[19])}


def proc_tree(pid):
    pending, seen = [pid], set()
    while pending:
        current = pending.pop()
        if current in seen:
            continue
        seen.add(current)
        try:
            pending.extend(int(x) for x in (Path("/proc") / str(current) / "task" /
                                            str(current) / "children").read_text().split())
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            pass
    return seen


def session_members(session):
    result = []
    for path in Path("/proc").glob("[0-9]*"):
        try:
            pid = int(path.name)
            s = proc_stat(pid)
            if s["sid"] == session:
                result.append({"pid": pid, "start_ticks": s["start_ticks"], "state": s["state"]})
        except (FileNotFoundError, ProcessLookupError, PermissionError, ValueError):
            pass
    return result


def rank_from_environ(pid):
    raw = (Path("/proc") / str(pid) / "environ").read_bytes()
    match = next((x.split(b"=", 1)[1] for x in raw.split(b"\0")
                  if x.startswith(b"OMPI_COMM_WORLD_RANK=")), None)
    return int(match) if match is not None else None


def runtime_for_rank(pid, expected, runtime_pins, binary_hash):
    proc = Path("/proc") / str(pid)
    exe_stat = (proc / "exe").stat()
    exec_match = (exe_stat.st_dev, exe_stat.st_ino) == (expected.st_dev, expected.st_ino)
    if not exec_match:
        raise RuntimeError(f"rank {pid} executes an unexpected inode")
    mapped = set()
    for line in (proc / "maps").read_text().splitlines():
        fields = line.split(None, 5)
        if len(fields) != 6 or not fields[5].startswith("/"):
            continue
        raw_path = re.sub(r"\\([0-7]{3})", lambda m: chr(int(m.group(1), 8)), fields[5])
        if raw_path.endswith(" (deleted)"):
            # MPI shared-memory data may be unlinked; reject deleted ELF mappings, not ordinary data.
            if int(fields[2],16)==0:
                with (proc / "mem").open("rb",buffering=0) as memory:
                    memory.seek(int(fields[0].split("-",1)[0],16));magic=memory.read(4)
                if magic==b"\x7fELF":raise RuntimeError(f"rank {pid} has deleted mapped ELF {raw_path}")
            continue
        path = str(Path(raw_path).resolve(strict=True))
        try:
            with open(path, "rb") as f:
                is_elf = f.read(4) == b"\x7fELF"
        except OSError as exc:
            raise RuntimeError(f"cannot inspect mapped runtime file {path}: {exc}") from exc
        if is_elf:
            mapped.add(path)
    files = {}
    for path in sorted(mapped):
        digest = sha(path)
        expected_hash = binary_hash if path == str(CFD) else runtime_pins.get(path)
        if expected_hash is None:
            raise RuntimeError(f"mapped ELF is absent from known runtime pins: {path}")
        if digest != expected_hash:
            raise RuntimeError(f"mapped ELF hash differs from known runtime pin: {path}")
        files[path] = digest
    if str(CFD) not in files:
        raise RuntimeError("executing CFD ELF is not present in the mapped pinned ELF set")
    return {"pid": pid, "start_ticks": proc_stat(pid)["start_ticks"],
            "elf_count": len(files), "executing_inode_verified": True,
            "mapped_elf_files": files, "unmatched": []}


def loaded_runtime(ranks, expected, runtime_pins, binary_hash):
    found = {}
    for pid in ranks:
        try:rank = rank_from_environ(pid)
        except (FileNotFoundError,ProcessLookupError):continue
        if rank is None:
            continue
        if rank in found:
            raise RuntimeError(f"duplicate live MPI rank {rank}")
        try:found[rank] = runtime_for_rank(pid, expected, runtime_pins, binary_hash)
        except (FileNotFoundError,ProcessLookupError):continue
    if set(found) != set(range(6)):
        return None
    return {"time": time.time(),
            "all_six_executing_inodes_and_loaded_elf_pinned": True,
            "ranks": [dict(rank=i, **found[i]) for i in range(6)]}



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

def observe_resources(session):
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
            processes[identity] = {"comm": name, "ticks": int(fields[11]) + int(fields[12]), "session_owned": int(fields[3]) == session}
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
            row["session_owned"] = processes[identity]["session_owned"]
        if cpu_window_elapsed >= 3.0:
            all_busy = busy_cpu_rows(cpu_window_deltas, cpu_window_elapsed)
            busy = [row for row in all_busy if cpu_window_deltas[(row["pid"], row["start_ticks"])]["session_owned"]]
            completed_window = {"start_monotonic": cpu_window_started,
                "end_monotonic": sampled_at, "duration_seconds": cpu_window_elapsed,
                "processes_compared": len(cpu_window_deltas), "unmatched_starts": cpu_unmatched_starts,
                "unmatched_exits": cpu_unmatched_exits, "busy_processes": busy,
                "busy_count": len(busy), "busy_processes_observed": all_busy, "scope": "all-process observation; gate counts owned CFD session"}
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

def resource_summary():
    'Report sticky memory and the actual CPU-window coverage.'
    total = sum(worker_peaks.values())
    windows = [row["duration_seconds"] for row in cpu_windows]
    cpu_covered = bool(windows) and min(windows) >= 3.0 and cpu_max_busy_processes <= MAX_BUSY_PROCESSES
    passed = len(worker_peaks) == 6 and total <= MAX_JOB_RSS and cpu_covered and not affinity_violations
    observed = max(0.0, (cpu_monitor_finished or 0.0) - (cpu_monitor_started or 0.0))
    covered = sum(windows)
    cpu_monitor = {"passed": cpu_covered, "window_count": len(windows),
        "gate_scope": "owned CFD session; user processes are observations only",
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

def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + "\n")


assert not FOLDER.exists(), "Never overwrite an isolated restart attempt."
source_status=json.loads((SOURCE / "status.json").read_text())
assert source_status["state"]=="terminal" and source_status["exit_code"]==0 and source_status["all_owned_handles_terminal"]
assert source_status.get("inputs_unchanged") is True and source_status.get("source_and_binary_pins_unchanged") is True
assert source_status.get("full_workflow_accepted") is True
for path in Path("/proc").glob("[0-9]*/comm"):
    try:assert path.read_text().strip() not in ("SU2_CFD","test_driver","cc1plus","ninja","lto1","mpirun")
    except (FileNotFoundError,ProcessLookupError):pass
build = json.loads(BUILD.read_text())
assert build["state"] == "terminal" and build["exit_code"] == 0 and build["pins_verified"]
assert build["test_binary_sha256"] == "efbfd37bb53d947f1b194860b76c76c9e29bf1b27feae19ecea8808017feac8a"
assert sha(CFD) == build["binary_sha256"] == "9ef34ecc72357e5deb31256e4c5739fedca2170f80f304996a5284e3a280ca35"
source_pins = json.loads(SOURCE_PINS.read_text())
extra_source_pins = json.loads(EXTRA_SOURCE_PINS.read_text())
assert not source_pins.keys() & extra_source_pins.keys()
source_pins.update(extra_source_pins)
for helper in (HERE / "run_restart552.py", HERE / "validate_restart552.py", STAGED / "night_20261115/section6_1/run_disk_restart522.py", STAGED / "night_20261115/section6_1/validate_disk_restart522.py"):
    key = str(helper.resolve())
    actual = sha(helper)
    assert key not in source_pins or source_pins[key] == actual, key
    source_pins[key] = actual
assert all(sha(p) == digest for p, digest in source_pins.items())
workflow = json.loads(EVENTS.read_text())
assert workflow["workflow_accepted"] is True and len(workflow["events"]) == 2
assert workflow["P7_P8_events_checked"] == [1, 2] and workflow["F3b_timer_events_checked"] == [1, 2]
identity = json.loads((SOURCE / "n8_payload_identity.json").read_text())
assert identity["passed"] is True and identity["workflow_gates_passed"] is True
for checked_event in (1, 2):
    assert json.loads((SOURCE / f"p7p8_event{checked_event}_gate.json").read_text())["passed"] is True
    assert json.loads((SOURCE / f"f3b_event{checked_event}_timer_gate.json").read_text())["passed"] is True
event = workflow["events"][1]
assert event["event"] == 2 and event["certification_complete"]
assert event["certification_six_rank_sums"]["q_bad"] == event["certification_six_rank_sums"]["j_bad"] == 0
assert event["certification_six_rank_sums"]["size_bad"] == event["certification_six_rank_sums"]["geometry_bad"] == 0
counts = event["actual_mesh_counts"]
assert set(counts) == {"points", "cells"} and all(isinstance(v, int) and v > 0 for v in counts.values())
assert event["certification_six_rank_sums"]["cells"] == counts["cells"]
payloads = {"adapted_mesh_adap_00002.su2": "accepted508.su2",
            "adapted_mesh_adap_00002.su2.native_ref": "accepted508.su2.native_ref",
            "restart_flow_adap_00002.dat": "accepted508_input.dat"}
for name, digest in event["outputs"].items():
    assert sha(SOURCE / name) == digest, name
runtime_pins = json.loads(RUNTIME_MANIFEST.read_text())
assert runtime_pins and all(sha(p) == digest for p, digest in runtime_pins.items())
binary_hash = build["binary_sha256"]
assert sha(CFD) == binary_hash
available = int(next(x.split()[1] for x in Path("/proc/meminfo").read_text().splitlines()
                     if x.startswith("MemAvailable:"))) * 1024
assert available > 16 * 1024**3

config = (SOURCE / "run.cfg").read_text()
for old, new in (("MESH_FILENAME= m6.su2", "MESH_FILENAME= accepted508.su2"),
                 ("SOLUTION_FILENAME= baseline_euler_seed", "SOLUTION_FILENAME= accepted508_input"),
                 ("RESTART_FILENAME= restart_flow", "RESTART_FILENAME= continued508_restart"),
                 ("CONV_FILENAME= history", "CONV_FILENAME= continued508_history"),
                 ("ADAP_LOOP= YES", "ADAP_LOOP= NO")):
    assert config.splitlines().count(old) == 1
    config = config.replace(old + "\n", new + "\n")
for line in ("RESTART_SOL= YES", "READ_BINARY_RESTART= YES", "ITER= 1", "ADAP_SENSOR= ( MACH )"):
    assert config.splitlines().count(line) == 1

FOLDER.mkdir()
for source_name, local_name in payloads.items():
    shutil.copy2(SOURCE / source_name, FOLDER / local_name)
    assert sha(SOURCE / source_name) == sha(FOLDER / local_name) == event["outputs"][source_name]
(FOLDER / "run.cfg").write_text(config)
scratch_root = STAGED.parents[2] / "build-integrated-v2/compile-tmp"
assert scratch_root.is_dir()
scratch = Path(tempfile.mkdtemp(prefix="adapted-disk508-", dir=scratch_root))
env = {k: v for k, v in os.environ.items() if not k.startswith("SU2_NATIVE_")}
env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1",
           OMPI_MCA_osc="pt2pt", OMPI_MCA_mpi_yield_when_idle="1", TMPDIR=str(scratch_root), OMPI_MCA_orte_tmpdir_base=str(scratch))
argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "timeout", "600", "mpirun", "--oversubscribe", "--bind-to", "none",
        "--output-filename", "rank_output", "-n", "6", str(CFD), "run.cfg"]
pins = dict(source_pins)
pins.update(runtime_pins)
for path in (Path(__file__), STAGED / "p7p8_r3_20261115/run_disk_restart508.py", STAGED / "p7p8_r3_20261115/validate_disk_restart508.py", BUILD, SOURCE_PINS, EXTRA_SOURCE_PINS, RUNTIME_MANIFEST, EVENTS, CFD, SOURCE / "n8_payload_identity.json", SOURCE / "p7p8_event1_gate.json", SOURCE / "p7p8_event2_gate.json", SOURCE / "f3b_event1_timer_gate.json", SOURCE / "f3b_event2_timer_gate.json",
             SOURCE / "run.cfg", SOURCE / "provenance.json", SOURCE / "status.json",
             HERE / "validate_restart552.py", *[FOLDER / n for n in payloads.values()],
             FOLDER / "run.cfg"):
    pins[str(path.resolve())] = sha(path)
for name in payloads:
    source_path = SOURCE / name
    pins[str(source_path.resolve())] = sha(source_path)
save(FOLDER / "pins.json", pins)
own={"pid":os.getpid(),"start_ticks":proc_stat(os.getpid())["start_ticks"]}
save(FOLDER / "command.json", {"argv": argv, "wrapper":own, "saved_checkpoint": str(SOURCE),
    "source_attempt": str(SOURCE), "initial_available_bytes": available,
    "loadavg": Path("/proc/loadavg").read_text(),
    "environment": {k: v for k, v in env.items() if k.startswith(("OMPI_", "OMP_", "OPENBLAS_", "MKL_")) or k == "TMPDIR"},
    "scope": "Actual isolated steady CFD disk restart on R3 P7/P8 accepted event2 mesh and matching following-CFD state; one further CFD iteration; no adaptation or convergence claim."})

started = time.time()
seen = {}
runtime_written = False
runtime_error = None
with (FOLDER / "run.log").open("w") as log, (FOLDER / "resources.jsonl").open("w") as resources:
    process = subprocess.Popen(argv, cwd=FOLDER, env=env, stdout=log, stderr=subprocess.STDOUT,
                               start_new_session=True)
    run_session = process.pid
    save(FOLDER / "status.json", {"state": "running", "pid": process.pid,
                                  "session": run_session, "wrapper":own, "started_at": started})
    while process.poll() is None:
        tree = proc_tree(process.pid)
        for member in session_members(run_session):
            tree.add(member["pid"])
        for pid in tree:
            try:
                s = proc_stat(pid)
                if s["sid"] == run_session:
                    seen[(pid, s["start_ticks"])] = {"pid": pid, "start_ticks": s["start_ticks"],
                                                     "sid": s["sid"]}
            except (FileNotFoundError, ProcessLookupError, PermissionError):
                pass
        rank_pids=[]
        for identity in seen.values():
            try:
                current=proc_stat(identity["pid"])
                if current["start_ticks"]==identity["start_ticks"] and current["state"] not in ("Z","X"):rank_pids.append(identity["pid"])
            except (FileNotFoundError,ProcessLookupError):pass
        resource_snapshot = observe_resources(run_session)
        if not runtime_written and runtime_error is None:
            try:
                receipt = loaded_runtime(rank_pids, CFD.stat(), runtime_pins, binary_hash)
                if receipt is not None:
                    save(FOLDER / "loaded_runtime_startup.json", receipt)
                    runtime_written = True
            except (OSError,RuntimeError,ValueError) as exc:
                # Preserve the failed observation while continuing to own/reap the bounded CFD job.
                runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})
        rows = sample_tree(process.pid)
        resources.write(json.dumps({"elapsed_seconds": time.time() - started, "processes": rows,
            "sampled_process_tree_rss_bytes": sum(x["rss_bytes"] for x in rows), **resource_snapshot}) + "\n")
        resources.flush()
        time.sleep(1)
    code = process.wait()

for p in Path("/proc").glob("[0-9]*"):
    try:
        pid, s = int(p.name), proc_stat(int(p.name))
        if s["sid"] == run_session:
            seen[(pid, s["start_ticks"])] = {"pid": pid, "start_ticks": s["start_ticks"], "sid": s["sid"]}
    except (FileNotFoundError, ProcessLookupError, PermissionError, ValueError):
        pass
terminal = []
for identity in seen.values():
    try:
        current = proc_stat(identity["pid"])
        gone = current["start_ticks"] != identity["start_ticks"]
    except (FileNotFoundError, ProcessLookupError, PermissionError):
        gone = True
    terminal.append({**identity, "absent": gone, "terminal": gone or current["state"] in ("Z","X")})
live_session = [row for row in session_members(run_session) if row["state"] not in ("Z","X")]
save(FOLDER / "terminal_handles.json", {"time": time.time(),
    "all_owned_handles_terminal": bool(terminal) and all(x["terminal"] for x in terminal) and not live_session,
    "actual_host_handles": terminal, "live_session_members": live_session,
    "manual_stop_sent": False, "controller_identity":own,
    "controller_terminal_verification":"post_terminal_handles.json is required after this wrapper exits"})
changed = [path for path, digest in pins.items() if sha(path) != digest]
resource_receipt = resource_summary()
save(FOLDER / "resource_peak_receipt.json", resource_receipt)
save(FOLDER / "cpu_process_audit.json", {"actual_host_proc": True,
    "windows": cpu_windows, "summary": resource_receipt["cpu_monitor"]})
save(FOLDER / "status.json", {"state": "terminal", "exit_code": code,
    "seconds": time.time() - started, "pins_verified": not changed, "changed": changed,
    "disk_restart_validated": False, "runtime_receipt_written": runtime_written,
    "runtime_observation_error": runtime_error, "resource_gate": resource_receipt})
print(json.dumps({"state": "terminal", "exit_code": code, "changed": changed,
                  "runtime_receipt_written": runtime_written,
                  "all_owned_handles_terminal": not live_session and all(x["terminal"] for x in terminal)}), flush=True)
assert not changed, changed
assert runtime_written and not live_session and all(x["terminal"] for x in terminal) and resource_receipt["passed"]
raise SystemExit(code)
