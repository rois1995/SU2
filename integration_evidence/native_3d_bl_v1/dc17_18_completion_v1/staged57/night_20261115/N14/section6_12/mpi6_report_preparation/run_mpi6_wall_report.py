#!/usr/bin/env python3
"""Prepare one six-rank saved-surface wall-stage report; never classify mesh acceptance."""
import hashlib
import importlib.util
import json
import os
import signal
import shlex
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = HERE.parent
VALIDATION = N11 / "validation"
STAGED = N11.parents[2]
REPO = STAGED.parents[3]
RUN_CASES = STAGED / "run_cases.py"
SECTION2 = STAGED / "night_20261115/section6_2/run_remaining_checks.py"
FIXTURE = REPO / "UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_cavity_planning.inc"
BODY = REPO / "UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_cavity_planning_body.inc"
STAGE = REPO / "UnitTests/SU2_CFD/adaptation/fixtures/native_actual_saved_surface_stage.inc"
SERIAL = STAGED / "cases/curved492_saved_production_surface_serial_20261113T163756_3724702"
CAPTURE = STAGED / "surface486_current_state"
PRODUCER = STAGED / "cases/curved492_ramp5_metric_stage_20261113T161313_3653005"
PRODUCER_RECEIPT = STAGED / "target_checkpoint492_producer.json"
QUALIFICATION = STAGED / "target_checkpoint492_qualification.json"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
BUILD = N11 / "build552"
TEST_BINARY = REPO / "integration_evidence/build-integrated-v3-p5-20261114/UnitTests/test_driver"
WALL_TAG = "[NativeActualSavedSurfaceStage3D]"
JOB_LIMIT = 19_999_490_048


def sha(path):
    """Return a file's SHA256."""
    return hashlib.file_digest(Path(path).open("rb"), "sha256").hexdigest()


def verify_inputs(check_runtime=True):
    """Verify the terminal build and exact qualified six-shard fixture inputs."""
    status = json.loads((VALIDATION / "build552_status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert Path(status["test_binary"]).resolve() == TEST_BINARY.resolve()
    assert sha(TEST_BINARY) == status["test_binary_sha256"] == "efbfd37bb53d947f1b194860b76c76c9e29bf1b27feae19ecea8808017feac8a"
    pins = json.loads((VALIDATION / "build552_source_pins.json").read_text())
    changed = [name for name, digest in pins.items() if sha(name) != digest]
    assert not changed, f"build552 source pins changed: {changed[:8]}"
    qualified = json.loads(QUALIFICATION.read_text())
    receipt = json.loads(PRODUCER_RECEIPT.read_text())
    assert qualified["actual_target_checkpoint_qualified"] and qualified["recomposed_target_checked_after_reload"]
    assert qualified["producer_receipt_sha256"] == sha(PRODUCER_RECEIPT)
    assert Path(receipt["producer"]).resolve() == PRODUCER.resolve()
    for rank in range(6):
        path = PRODUCER / f"target_rank{rank}.bin"
        assert path.is_file() and sha(path) == receipt["files"][path.name]["sha256"]
    assert (CAPTURE / "trajectory_working_owned.bin").is_file()
    assert (CAPTURE / "trajectory_working_parameters.bin").is_file()
    assert SERIAL.joinpath("run.cfg").is_file() and RUNTIME.is_file()
    assert sha(SERIAL / "run.cfg") == json.loads((SERIAL / "pins.json").read_text())[str(SERIAL / "run.cfg")]
    runtime = verify_runtime(strict=check_runtime)
    status["runtime_pin_mismatches"] = runtime["changed"]
    return status


def verify_runtime(strict=True):
    """Require the loaded test-binary libraries to remain inside the frozen runtime pins."""
    pins = json.loads(RUNTIME.read_text())
    changed = [name for name, digest in pins.items() if sha(name) != digest]
    if strict:
        assert not changed, f"loaded runtime pins changed: {changed[:8]}"
    namespace = {"__name__": "mpi6_report_run_cases", "__file__": str(RUN_CASES)}
    exec(compile(RUN_CASES.read_text(), str(RUN_CASES), "exec"), namespace)
    libraries = namespace["loaded_libraries"]()
    outside = sorted(set(libraries) - set(pins))
    if strict:
        assert not outside, "test binary loads libraries outside frozen runtime"
    return {"libraries": libraries, "changed": changed, "outside_runtime": outside}


def plan(check_runtime=True):
    """Build the immutable six-rank command and provenance inputs."""
    status = verify_inputs(check_runtime=check_runtime)
    command = json.loads((SERIAL / "command.json").read_text())
    env = dict(command["environment"])
    assert command["ranks"] == 1 and env["SU2_NATIVE_BL_PLANNING_CAPTURE"] == str(CAPTURE)
    assert Path(env["SU2_NATIVE_TARGET_CHECKPOINT3D"]).parent == PRODUCER
    env["SU2_NATIVE_FROZEN_CONFIG"] = str(SERIAL / "run.cfg")
    env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1", OMPI_MCA_osc="pt2pt")
    argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "timeout", "900", "mpirun",
            "--bind-to", "none", "-n", "6", str(TEST_BINARY), WALL_TAG, "--durations", "yes"]
    cfg = (SERIAL / "run.cfg").read_text()
    settings = {key: next(line.split("=", 1)[1].strip() for line in cfg.splitlines()
                          if line.startswith(key + "="))
                for key in ("ADAP_NATIVE_MAX_TIME", "ADAP_NATIVE_MAX_WORK", "ADAP_NATIVE_MEMORY_LIMIT",
                            "ADAP_BL_FIRST_HEIGHT", "ADAP_BL_GROWTH", "ADAP_BL_THICKNESS", "ADAP_BL_H0_RAMP")}
    input_hashes = pinned_inputs(env)
    return {"scope": "wall-stage report only; not mesh acceptance or BL success",
            "ranks": 6, "argv": argv, "environment": env,
            "build_status": str(VALIDATION / "build552_status.json"),
            "build_sha256": status["test_binary_sha256"],
            "runtime_pin_mismatches": status["runtime_pin_mismatches"],
            "runtime_pins": str(RUNTIME), "source_runner": str(SECTION2),
            "saved_fixture": str(SERIAL), "capture": str(CAPTURE),
            "qualified_target_producer": str(PRODUCER), "job_memory_limit_bytes": JOB_LIMIT,
            "frozen_config_sha256": sha(SERIAL / "run.cfg"), "frozen_config_settings": settings,
            "stage_limits": {"seconds": 388.8, "work": 233220},
            "wrapper_sha256": sha(Path(__file__)), "input_hashes": input_hashes,
            "pin_manifest_hashes": {str(path): sha(path) for path in (
                VALIDATION / "build552_status.json", VALIDATION / "build552_source_pins.json", RUNTIME)}}


def pinned_inputs(env):
    """Hash all frozen config, target, reference, capture, and test source inputs."""
    paths = [Path(env[name]) for name in ("SU2_NATIVE_TARGET_CHECKPOINT3D", "SU2_NATIVE_FROZEN_SENSOR3D",
            "SU2_NATIVE_BL_REFERENCE", "SU2_NATIVE_NOSE_FIXED_FACETS", "SU2_NATIVE_FROZEN_CONFIG")]
    paths.extend((PRODUCER_RECEIPT, QUALIFICATION, SECTION2, FIXTURE, BODY, STAGE,
                  CAPTURE / "trajectory_working_owned.bin", CAPTURE / "trajectory_working_parameters.bin"))
    paths.extend(PRODUCER / f"target_rank{rank}.bin" for rank in range(6))
    return {str(path.resolve()): sha(path) for path in paths}


def verify_pins(command):
    """Return post-run mutations across code, runtime, frozen inputs, and this wrapper."""
    groups = {"build": json.loads((VALIDATION / "build552_source_pins.json").read_text()),
              "runtime": json.loads(RUNTIME.read_text()), "inputs": command["input_hashes"]}
    changed = {kind: [path for path, digest in pins.items() if sha(path) != digest]
               for kind, pins in groups.items()}
    changed["pin_manifests"] = [path for path, digest in command["pin_manifest_hashes"].items()
                                if sha(path) != digest]
    if sha(Path(__file__)) != command["wrapper_sha256"]:
        changed["wrapper"] = [str(Path(__file__))]
    return {kind: paths for kind, paths in changed.items() if paths}


def host_preflight(proc_root=Path("/proc"), meminfo=Path("/proc/meminfo"), loadavg=Path("/proc/loadavg")):
    """Capture actual-host RAM/load and label process inventory as observation only."""
    available = next(int(row.split()[1]) * 1024 for row in meminfo.read_text().splitlines()
                     if row.startswith("MemAvailable:"))
    load = loadavg.read_text().split()[:3]
    observed = []
    for entry in proc_root.iterdir():
        if entry.name.isdigit():
            try:
                name = (entry / "comm").read_text().strip()
                if name in ("SU2_CFD", "test_driver", "mpirun", "ninja", "cc1plus"):
                    observed.append({"pid": int(entry.name), "name": name})
            except OSError:
                continue
    assert available >= JOB_LIMIT, f"host MemAvailable below {JOB_LIMIT} bytes"
    return {"actual_host_proc": True, "MemAvailable_bytes": available, "loadavg": load,
            "process_observations_only": observed, "requested_owned_ranks": 6,
            "maximum_owned_ranks": 6}


def run_command(command, launch, preflight=None):
    """Run through an injectable launcher; tests use a stub and never start a process."""
    snapshot = preflight if preflight is not None else host_preflight()
    return launch(command, snapshot)


def identity(pid):
    """Read Linux PID and start tick to distinguish PID reuse."""
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return {"pid": int(pid), "start_ticks": int(fields[19]), "state": fields[0]}
    except (OSError, ValueError, IndexError):
        return None


def tree(root):
    """Sample the wrapper's owned process tree with per-process RSS."""
    pending, seen, rows = [root], set(), []
    while pending:
        pid = pending.pop()
        if pid in seen:
            continue
        seen.add(pid)
        proc = Path("/proc") / str(pid)
        try:
            pending.extend(int(child) for child in (proc / "task" / str(pid) / "children").read_text().split())
            rss = int((proc / "statm").read_text().split()[1]) * os.sysconf("SC_PAGE_SIZE")
            ident = identity(pid)
            if ident:
                status = (proc / "status").read_text().splitlines()
                hwm = next(int(line.split()[1]) * 1024 for line in status if line.startswith("VmHWM:"))
                env = (proc / "environ").read_bytes().split(b"\0")
                rank = next((item.split(b"=", 1)[1].decode() for item in env
                             if item.startswith(b"OMPI_COMM_WORLD_RANK=")), None)
                rows.append(dict(ident, rss_bytes=rss, vmhwm_bytes=hwm, mpi_rank=rank,
                                 name=(proc / "comm").read_text().strip()))
        except (OSError, ValueError):
            continue
    return rows


def launch_owned(command, other_owned_ranks, popen=subprocess.Popen, sampler=tree,
                 pause=time.sleep, case_dir=None, preflight_override=None,
                 signal_sender=os.kill, identity_reader=identity):
    """Run the prepared report command with six-rank ownership and terminal receipts."""
    if other_owned_ranks + command["ranks"] > 6:
        raise RuntimeError("six-rank launch would exceed the owned-rank limit")
    command = json.loads(json.dumps(command))
    preflight = preflight_override if preflight_override is not None else host_preflight()
    out = case_dir or HERE / "cases" / f"mpi6_wall_report_{time.strftime('%Y%m%dT%H%M%S', time.gmtime())}_{os.getpid()}"
    out.mkdir(parents=True, exist_ok=False)
    (out / "host_preflight.json").write_text(json.dumps(preflight, indent=2) + "\n")
    mpi = command["argv"].index("mpirun")
    command["argv"][mpi + 1:mpi + 1] = ["--output-filename", str(out / "rank_output")]
    (out / "command.json").write_text(json.dumps(command, indent=2) + "\n")
    env = os.environ.copy()
    env.update(command["environment"])
    owned, highwater, rank_highwater = {}, {}, {}
    start = time.time()
    over_limit_since, cap_signal = None, None
    with (out / "run.log").open("w") as log:
        proc = popen(command["argv"], cwd=out, env=env, stdout=log, stderr=subprocess.STDOUT,
                     start_new_session=True)
        record_identity(owned, proc.pid, "launcher", None)
        with (out / "resources.jsonl").open("w") as resources:
            while proc.poll() is None:
                rows = sampler(proc.pid)
                update_highwater(rows, owned, highwater, rank_highwater)
                bound = sum(highwater.values())
                if bound > JOB_LIMIT:
                    over_limit_since = over_limit_since or time.time()
                    signal_name = "SIGKILL" if time.time() - over_limit_since >= 5 else "SIGTERM"
                    cap_signal = stop_verified(owned, signal_name, signal_sender, identity_reader)
                record = resource_row(time.time() - start, rows, highwater, rank_highwater, cap_signal)
                resources.write(json.dumps(record) + "\n")
                resources.flush()
                pause(1)
            rows = sampler(proc.pid)
            update_highwater(rows, owned, highwater, rank_highwater)
            resources.write(json.dumps(resource_row(time.time() - start, rows, highwater,
                                                    rank_highwater, cap_signal)) + "\n")
            resources.flush()
        code = proc.wait()
    return write_receipt(out, code, start, owned, highwater, rank_highwater, preflight,
                         other_owned_ranks, cap_signal, verify_pins(command))


def record_identity(owned, pid, name, rank):
    """Record one owned PID together with its Linux start tick."""
    ident = identity(pid)
    if ident:
        owned[(ident["pid"], ident["start_ticks"])] = dict(ident, name=name, mpi_rank=rank)


def update_highwater(rows, owned, highwater, rank_highwater):
    """Accumulate per-PID high-water marks into rank and conservative job sums."""
    for row in rows:
        key = (row["pid"], row["start_ticks"])
        owned[key] = {name: row.get(name) for name in ("pid", "start_ticks", "name", "mpi_rank")}
        highwater[key] = max(highwater.get(key, 0), row.get("vmhwm_bytes", row.get("rss_bytes", 0)))
        rank = row.get("mpi_rank")
        if rank is not None:
            rank_highwater[rank] = sum(value for key, value in highwater.items()
                                       if owned[key].get("mpi_rank") == rank)


def resource_row(elapsed, rows, highwater, rank_highwater, cap_signal):
    """Make one flushed resource row with the conservative VMHWM budget bound."""
    return {"elapsed_seconds": elapsed, "processes": rows,
            "pid_start_vmhwm_bytes": {f"{pid}:{tick}": value for (pid, tick), value in highwater.items()},
            "rank_vmhwm_sum_bytes": rank_highwater, "owned_job_vmhwm_sum_bytes": sum(highwater.values()),
            "job_limit_bytes": JOB_LIMIT, "over_limit": sum(highwater.values()) > JOB_LIMIT,
            "stop_signal": cap_signal}


def stop_verified(owned, signal_name, sender=os.kill, identity_reader=identity):
    """Signal only sampled processes whose PID/start tick still matches this report run."""
    sig = signal.SIGTERM if signal_name == "SIGTERM" else signal.SIGKILL
    for row in owned.values():
        current = identity_reader(row["pid"])
        if current and current["start_ticks"] == row["start_ticks"] and current["state"] not in ("Z", "X"):
            try:
                sender(row["pid"], sig)
            except ProcessLookupError:
                pass
    return signal_name


def write_receipt(out, code, start, owned, highwater, rank_highwater, preflight,
                  other_owned_ranks, cap_signal, pin_changes):
    """Seal process terminal identities and report parsed per-rank output without acceptance claims."""
    terminal = []
    for row in owned.values():
        now = identity(row["pid"])
        terminal.append(dict(row, terminal=not now or now["start_ticks"] != row["start_ticks"] or now["state"] in ("Z", "X")))
    peak_bound = sum(highwater.values())
    report = {"scope": "report only; no accepted BL claim or purpose gate", "exit_code": code,
              "seconds": time.time() - start, "owned_ranks_at_start": other_owned_ranks,
              "requested_ranks": 6, "all_owned_handles_terminal": bool(terminal) and all(x["terminal"] for x in terminal),
              "terminal_processes": terminal, "host_preflight": preflight,
              "source_pins_unchanged": not pin_changes, "source_pin_changes": pin_changes,
              "cap_stop_signal": cap_signal, "per_rank_vmhwm_sum_bytes": rank_highwater,
              "conservative_owned_job_vmhwm_sum_bytes": peak_bound, "job_limit_bytes": JOB_LIMIT,
              "job_memory_within_limit": peak_bound <= JOB_LIMIT,
              "memory_cap_triggered": cap_signal is not None,
              "memory_cap_stop_reason": "conservative owned-job VmHWM sum exceeded 19073 MiB" if cap_signal else None,
              "resource_scope": "sum of per-process VmHWM peaks including owned wrappers; conservative bound"}
    if code == 0:
        report["wall_stage_rows"] = summarize(out)
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def summarize(case_dir):
    """Collect six physical rows and final stage counters without purpose gates."""
    rows = []
    for rank in range(6):
        path = Path(case_dir) / f"actual_saved_surface_stage_rank{rank}.json"
        row = json.loads(path.read_text())
        assert row["rank"] == rank and row["ranks"] == 6
        rows.append({"rank": rank, "cells": row.get("output_cells"),
                     "positive_cells": row.get("positive_cells"), "sensor_cells": row.get("sensor_cells"),
                     "engine_valid": row.get("engine_valid"), "surface_prepared": row.get("surface_prepared"),
                     "final_mesh_accepted": row.get("final_mesh_accepted"),
                     "splits": row.get("splits"), "geometry_guidance_trials": row.get("geometry_guidance_trials"),
                     "geometry_guidance_rejected": row.get("geometry_guidance_rejected"),
                     "seconds": row.get("seconds"), "work": row.get("work"),
                     "maximum_work_bytes": row.get("maximum_work_bytes"), "upper_calls": row.get("upper_calls"),
                     "upper_work": row.get("upper_work"), "upper_seconds": row.get("upper_seconds"),
                     "surface_required": row.get("surface_required"), "surface_measured": row.get("surface_measured")})
    stage_rows = stage_exit_rows(Path(case_dir) / "run.log", Path(case_dir))
    counters = wall_counter_tables(Path(case_dir))
    return {"scope": "report only; no accepted BL claim and no purpose gate",
            "rank_rows": rows, "stage_exit_rows": stage_rows, "wall_counters": counters,
            "rank_count": len(rows)}


def marker_fields(line, marker):
    """Parse one exact marker and its key/value fields."""
    if marker + " " not in line:
        return None
    text = line[line.index(marker + " "):]
    fields = {}
    for token in shlex.split(text, posix=True):
        if "=" in token:
            key, value = token.split("=", 1)
            fields[key] = value
    return fields


def wall_counter_tables(case_dir):
    """Read source/guidance counters from rank stdout, merged log only as fallback."""
    report = {"source_rows": [], "guidance_rows": [], "rank_stdout": {}, "fallback": False}
    for rank in range(6):
        stdout = case_dir / "rank_output/1" / f"rank.{rank}" / "stdout"
        lines = stdout.read_text(errors="replace").splitlines() if stdout.is_file() else None
        if lines is None:
            lines = (case_dir / "run.log").read_text(errors="replace").splitlines()
            report["fallback"] = True
        report["rank_stdout"][str(rank)] = str(stdout) if stdout.is_file() else "merged_run_log_fallback"
        for marker, dest in (("NATIVE3D_WALL_SOURCE", "source_rows"),
                             ("NATIVE3D_WALL_GUIDANCE", "guidance_rows")):
            for line in lines:
                row = marker_fields(line, marker)
                if row is not None and row.get("rank") == str(rank):
                    report[dest].append(row)
    report["source_sum"] = {name: sum(int(row[name]) for row in report["source_rows"] if name in row)
                            for name in ("validated", "guidance_rejected", "approved")}
    report["guidance_count_sum"] = sum(int(row["count"]) for row in report["guidance_rows"] if "count" in row)
    report["scope"] = "source and guidance counters are report-only; no equality gate"
    return report


def stage_exit_rows(log_path, case_dir=None):
    """Parse rank/stage exit counters, including upper refinements, from rank stdout."""
    rows = []
    for rank in range(6):
        stdout = Path(case_dir) / "rank_output/1" / f"rank.{rank}" / "stdout" if case_dir else None
        source = stdout if stdout and stdout.is_file() else Path(log_path)
        for number, line in enumerate(source.read_text(errors="replace").splitlines(), 1):
            marker = "NATIVE3D_STAGE "
            if marker not in line:
                continue
            fields = {}
            for token in shlex.split(line[line.index(marker):], posix=True):
                if "=" in token:
                    key, value = token.split("=", 1)
                    fields[key] = value
            if fields.get("stage") == "exit":
                assert "rank" in fields and "phase" in fields, f"malformed stage exit at line {number}"
                if stdout and stdout.is_file():
                    assert fields["rank"] == str(rank)
                fields["line"] = number
                rows.append(fields)
    ranks = {int(row["rank"]) for row in rows}
    assert ranks == set(range(6)), f"expected exit rows for six ranks, got {sorted(ranks)}"
    return rows


def self_check():
    """Exercise resolved inputs, six-rank binding, and stubbed process/resource receipts."""
    status = verify_inputs(check_runtime=False)
    prepared = plan(check_runtime=False)
    seen = []
    fake = {"actual_host_proc": True, "MemAvailable_bytes": JOB_LIMIT,
            "loadavg": ["0", "0", "0"], "process_observations_only": [],
            "requested_owned_ranks": 6, "maximum_owned_ranks": 6}
    result = run_command(prepared, lambda command, host: seen.append((command, host)) or "stubbed", fake)
    assert result == "stubbed" and len(seen) == 1
    argv = seen[0][0]["argv"]
    rank_flag = argv.index("-n", argv.index("mpirun"))
    assert seen[0][0]["ranks"] == 6 and argv[rank_flag + 1] == "6"
    assert seen[0][1]["actual_host_proc"] and status["completed_build"] is True
    assert all((PRODUCER / f"target_rank{rank}.bin").is_file() for rank in range(6))
    assert Path(prepared["environment"]["SU2_NATIVE_FROZEN_CONFIG"]) == SERIAL / "run.cfg"
    class StubProcess:
        pid = 987654321
        calls = 0
        def poll(self):
            self.calls += 1
            return None if self.calls == 1 else 9
        def wait(self):
            return 9
    process = StubProcess()
    def stub_popen(*_args, **_kwargs):
        return process
    def stub_sample(pid):
        assert pid == process.pid
        return [{"pid": pid, "start_ticks": 77, "state": "S", "rss_bytes": 1234, "name": "stub"}]
    with tempfile.TemporaryDirectory(prefix="mpi6-report-selfcheck-") as tmp:
        case = Path(tmp) / "case"
        receipt = launch_owned(prepared, 0, popen=stub_popen, sampler=stub_sample,
            pause=lambda _seconds: None, case_dir=case, preflight_override=fake)
        assert receipt["exit_code"] == 9 and receipt["requested_ranks"] == 6
        assert receipt["conservative_owned_job_vmhwm_sum_bytes"] == 1234
        assert len(receipt["terminal_processes"]) == 1 and receipt["all_owned_handles_terminal"]
        for rank in range(6):
            row = {"rank": rank, "ranks": 6, "output_cells": 4, "positive_cells": 4,
                   "sensor_cells": 4, "upper_calls": 1, "maximum_work_bytes": 16}
            (case / f"actual_saved_surface_stage_rank{rank}.json").write_text(json.dumps(row))
        (case / "run.log").write_text("".join(
            f"NATIVE3D_STAGE rank={rank} stage=exit phase=construction upper_refinements=0\n"
            for rank in range(6)))
        for rank in range(6):
            stdout = case / "rank_output/1" / f"rank.{rank}" / "stdout"
            stdout.parent.mkdir(parents=True, exist_ok=True)
            stdout.write_text(f"NATIVE3D_STAGE rank={rank} stage=exit phase=construction upper_refinements=0\n"
                f"NATIVE3D_WALL_SOURCE rank={rank} source=primary validated=3 guidance_rejected=2 approved=1\n"
                f"NATIVE3D_WALL_GUIDANCE rank={rank} origin=geometry cause=incomplete count=2\n"
                f"NATIVE3D_WALL_SOURCELESS rank={rank} validated=999\n")
        table = summarize(case)
        assert len(table["rank_rows"]) == len(table["stage_exit_rows"]) == 6
        assert all(row["upper_refinements"] == "0" for row in table["stage_exit_rows"])
        assert len(table["wall_counters"]["source_rows"]) == 6
        assert table["wall_counters"]["source_sum"]["validated"] == 18
        overshoot_process, sent = StubProcess(), []
        overshoot_process.pid = 987654322
        def overshoot_popen(*_args, **_kwargs):
            return overshoot_process
        def overshoot_sample(pid):
            return [{"pid": pid, "start_ticks": 88, "state": "S", "rss_bytes": JOB_LIMIT + 1,
                     "vmhwm_bytes": JOB_LIMIT + 1, "mpi_rank": "0", "name": "stub"}]
        def stub_identity(pid):
            return {"pid": pid, "start_ticks": 88, "state": "S"} if pid == overshoot_process.pid else None
        def stub_signal(pid, sig):
            sent.append((pid, sig))
        cap_dir = Path(tmp) / "cap_case"
        cap = launch_owned(prepared, 0, popen=overshoot_popen, sampler=overshoot_sample,
            pause=lambda _seconds: None, case_dir=cap_dir, preflight_override=fake,
            signal_sender=stub_signal, identity_reader=stub_identity)
        assert cap["memory_cap_triggered"] and cap["cap_stop_signal"] == "SIGTERM"
        assert sent == [(overshoot_process.pid, signal.SIGTERM)]
    return {"passed": True, "launched": False, "launch_ready": not status["runtime_pin_mismatches"],
            "runtime_pin_mismatches": status["runtime_pin_mismatches"], "rank_count": 6,
            "binary_sha256": status["test_binary_sha256"],
            "target_shards": [str(PRODUCER / f"target_rank{rank}.bin") for rank in range(6)],
            "stub_monitor": "terminal identities and RSS receipt verified"}


if __name__ == "__main__":
    if sys.argv[1:] == ["--self-check"]:
        print(json.dumps(self_check(), indent=2))
    elif sys.argv[1:] == ["--plan"]:
        print(json.dumps(plan(), indent=2))
    elif len(sys.argv) == 3 and sys.argv[1] == "--launch":
        print(json.dumps(launch_owned(plan(), int(sys.argv[2])), indent=2))
    else:
        raise SystemExit("usage: --self-check | --plan | --launch OTHER_OWNED_RANKS")
