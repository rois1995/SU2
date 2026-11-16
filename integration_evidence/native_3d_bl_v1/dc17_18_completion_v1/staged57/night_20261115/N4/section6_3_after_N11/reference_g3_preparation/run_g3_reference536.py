#!/usr/bin/env python3
"""Prepare and run the fresh identical-cut G3 reference on accepted build536."""
import argparse
import hashlib
import importlib.util
import inspect
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
N4 = PHASE.parent
NIGHT = N4.parent
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
SECTION1 = NIGHT / "section6_1"
SECTION2 = NIGHT / "section6_2"
BUILD = PHASE / "build536"
N8_VALIDATOR_SOURCE = NIGHT / "N8/A2_A3_N12/section6_8/validation/validate_n8.py"
BASE_CHECKS = SECTION2 / "run_remaining_checks.py"
IDENTITY_SOURCE = NIGHT / "N9/section6_6/drafts/check_g3_identity_66.py"
REFERENCE = SECTION1 / "cases/section6_1_bl_20261116T082025_3350201"
P6C = STAGED / "cases/p6c_bl_20261115T003316_3200535"
WALL_BASELINE = NIGHT / "N9/section6_6/cases/N9_66_bl_20261116T135717_78861/run.log"
COMPARATOR = N4 / "compare_integer_rows.py"
CASE_PREFIX = "n4_g3_reference_bl"
ALIAS = HERE / "build536_aliases.py"
SCRIPT = Path(__file__).resolve()


class LaunchStub(Exception):
    """Mark the safe self-check boundary immediately before solver Popen."""


def sha(path):
    """Hash one source, reference, or case artifact."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load(name, path):
    """Load an existing helper by its frozen path."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def proc_start_ticks(pid):
    """Read Linux process start ticks, rejecting disappeared or malformed identities."""
    stat = Path(f"/proc/{int(pid)}/stat").read_text()
    fields = stat[stat.rfind(")") + 2:].split()
    return int(fields[19])


def live_heavy_processes():
    """List active SU2, unit, compiler, and build processes for the one-job guard."""
    found = []
    for proc in Path("/proc").glob("[0-9]*"):
        try:
            comm = (proc / "comm").read_text().strip()
            if comm in {"SU2_CFD", "test_driver", "ninja", "cc1plus", "lto1"}:
                found.append({"pid": int(proc.name), "comm": comm,
                              "start_ticks": proc_start_ticks(proc.name)})
        except (FileNotFoundError, PermissionError, ProcessLookupError, ValueError):
            continue
    return found


def host_preflight(require_idle=False):
    """Record actual host identity, load, RAM, scratch space, and heavy-process state."""
    available_kib = int(next(row.split()[1] for row in Path("/proc/meminfo").read_text().splitlines()
                             if row.startswith("MemAvailable:")))
    result = {"hostname": os.uname().nodename,
              "boot_id": Path("/proc/sys/kernel/random/boot_id").read_text().strip(),
              "loadavg": Path("/proc/loadavg").read_text().strip(),
              "mem_available_mib": available_kib // 1024,
              "scratch_free_bytes": shutil.disk_usage(HERE).free,
              "active_heavy_processes": live_heavy_processes()}
    assert result["boot_id"] and Path("/proc/1/stat").is_file()
    assert result["scratch_free_bytes"] > 2_000_000_000
    if require_idle:
        assert result["mem_available_mib"] >= 19_073, result
        assert not result["active_heavy_processes"], result["active_heavy_processes"]
    return result


def reference_inputs():
    """List the accepted N1/P6c records consumed by the G3 identity evaluation."""
    return [PHASE / "source_check.md", PHASE / "inputs/CUT_PLAN_8_2.md"] + [
            REFERENCE / name for name in ("run.log", "nose_target_observation.jsonl",
            "actual_saved_surface_stage_rank0.json", "cheap_bl_receipt.json", "status.json")] + [
            P6C / "nose_target_observation.jsonl", WALL_BASELINE]


def adapted_safeguard_source(base):
    """Retarget the accepted Section 6.2 BL safeguard to this folder and build536."""
    source = base.adapted_source("bl")
    changes = (
        ("HERE=Path(__file__).resolve().parent;STAGED=HERE.parents[1];R=STAGED;REPO=STAGED.parents[3]",
         "HERE=Path(__file__).resolve().parent;STAGED=HERE.parents[3];R=STAGED;REPO=STAGED.parents[3];A2=HERE.parent"),
        ("from build525_aliases import main as expose_build",
         "from build536_aliases import main as expose_build"),
        ("expose_build();checks.ROOT=HERE;checks.REPO=REPO",
         "sys.path.insert(0,str(HERE));sys.path.insert(1,str(A2));expose_build();checks.ROOT=HERE;checks.REPO=REPO"),
        ("case='section6_2_'+mode", "case='n4_g3_reference_'+mode"),
    )
    for before, after in changes:
        assert source.count(before) == 1, before
        source = source.replace(before, after, 1)
    source = source.replace("build525", "build536")
    alias_pins = ("extra_pins={str(OLD.resolve()):sha(OLD),str(Path(__file__).resolve()):sha(__file__),"
                  "str((HERE/'build536_aliases.py').resolve()):sha(HERE/'build536_aliases.py')}")
    assert source.count(alias_pins) == 1
    pinned = ("extra_pins={str(OLD.resolve()):sha(OLD),str(Path(__file__).resolve()):sha(__file__),"
              "str((HERE/'build536_aliases.py').resolve()):sha(HERE/'build536_aliases.py'),"
              "str(VALIDATOR_SOURCE.resolve()):sha(VALIDATOR_SOURCE),"
              "str(BASE_CHECKS.resolve()):sha(BASE_CHECKS),"
              "str(IDENTITY_SOURCE.resolve()):sha(IDENTITY_SOURCE),"
              "str(COMPARATOR.resolve()):sha(COMPARATOR),"
              "**{str(path.resolve()):sha(path) for path in REFERENCE_INPUTS}}")
    source = source.replace(alias_pins, pinned, 1)
    compile(source, str(SCRIPT), "exec")
    assert "build536_status.json" in source and "n4_g3_reference_" in source
    return source


def set_fixture_timeout(checks):
    """Set only this serial fixture's outer timeout to its authorized 900 seconds."""
    source = inspect.getsource(checks.main)
    anchor = 'timeout_seconds = 2400 if case == "m6_stage" else 600'
    assert source.count(anchor) == 1
    updated = source.replace(anchor,
        f'timeout_seconds = 900 if case == "{CASE_PREFIX}" else 2400 if case == "m6_stage" else 600', 1)
    exec(compile(updated, checks.__file__, "exec"), checks.__dict__)


def execute_safeguard(validator, stub=None):
    """Run the inherited BL gate; optionally intercept its exact solver launch."""
    base = load("n4_g3_section62_base", BASE_CHECKS)
    source = adapted_safeguard_source(base)
    source = validator.bl_wrapper_source(source)
    anchor = "gate.self_check();"
    assert source.count(anchor) == 1
    source = source.replace(anchor, anchor + "match_bl_wrapper(gate);", 1)
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    namespace = {"__name__": "__main__", "__file__": str(SCRIPT), "OLD": base.OLD,
                 "HERE": HERE, "STAGED": STAGED, "REPO": REPO,
                 "set_bl_wrapper_timeout": set_fixture_timeout,
                 "match_bl_wrapper": validator.match_bl_wrapper,
                 "VALIDATOR_SOURCE": N8_VALIDATOR_SOURCE, "BASE_CHECKS": BASE_CHECKS,
                 "IDENTITY_SOURCE": IDENTITY_SOURCE, "COMPARATOR": COMPARATOR,
                 "REFERENCE_INPUTS": reference_inputs()}
    sys.argv = [str(SCRIPT), "bl"]
    if stub is not None:
        original = subprocess.Popen
        def popen_stub(*args, **kwargs):
            argv = args[0] if args else kwargs.get("args")
            argv = [str(value) for value in argv] if isinstance(argv, (list, tuple)) else [str(argv)]
            if argv and argv[0] == "nice":
                stub.update(argv=argv, cwd=str(kwargs.get("cwd")), env=kwargs.get("env", {}),
                            start_new_session=kwargs.get("start_new_session"))
                raise LaunchStub("intercepted at run_cases solver Popen")
            return original(*args, **kwargs)
        subprocess.Popen = popen_stub
    try:
        exec(compile(source, str(SCRIPT), "exec"), namespace)
    finally:
        if stub is not None:
            subprocess.Popen = original


def with_process_identity(sample_tree):
    """Add start ticks to each process sample produced by the existing run_cases monitor."""
    def sample(root_pid):
        rows = sample_tree(root_pid)
        for row in rows:
            try:
                row["start_ticks"] = proc_start_ticks(row["pid"])
            except (FileNotFoundError, PermissionError, ProcessLookupError, ValueError):
                row["start_ticks"] = None
        return rows
    return sample


def terminal_handles(case, root_identity):
    """Require every process identity observed by run_cases to be terminal by PID/start."""
    identities = {(root_identity["pid"], root_identity["start_ticks"])}
    for line in (case / "resources.jsonl").read_text().splitlines():
        for row in json.loads(line).get("processes", []):
            if row.get("start_ticks") is not None:
                identities.add((int(row["pid"]), int(row["start_ticks"])))
    alive = []
    for pid, start_ticks in sorted(identities):
        try:
            if proc_start_ticks(pid) == start_ticks:
                alive.append({"pid": pid, "start_ticks": start_ticks})
        except (FileNotFoundError, PermissionError, ProcessLookupError, ValueError):
            pass
    assert not alive, f"owned run-case process identities remain live: {alive}"
    return {"actual_host_proc": True, "all_owned_handles_terminal": True,
            "identities": [{"pid": pid, "start_ticks": start, "alive": False}
                           for pid, start in sorted(identities)]}


def evaluate_case(case, host, launch_identity):
    """Apply the accepted physical and 6.6 G3 identity gates, keeping old integers report-only."""
    cheap = json.loads((case / "cheap_bl_receipt.json").read_text())
    if cheap.get("passed") is not True:
        raise RuntimeError("first failed gate: inherited cheap-BL physical guard")
    diagnostic = subprocess.run([sys.executable, str(COMPARATOR), str(REFERENCE), str(case),
                                 "--gate", "all"], cwd=HERE, capture_output=True, text=True, check=False)
    try:
        old_rows = json.loads(diagnostic.stdout)
    except json.JSONDecodeError:
        old_rows = {"stdout": diagnostic.stdout, "stderr": diagnostic.stderr}
    (case / "g3_integer_comparison.json").write_text(json.dumps(
        {"returncode": diagnostic.returncode, "comparison": old_rows,
         "comparator_sha256": sha(COMPARATOR), "gate_used": False}, indent=2) + "\n")
    identity = load("n4_g3_identity_66", IDENTITY_SOURCE)
    decision = identity.evaluate(REFERENCE, case)
    pins = json.loads((case / "pins.json").read_text())
    assert pins.get(str(SCRIPT)) == sha(SCRIPT)
    assert pins.get(str(ALIAS)) == sha(ALIAS)
    handles = terminal_handles(case, launch_identity)
    report_helper = load("n8_g3_report_helpers", N8_VALIDATOR_SOURCE)
    wall = report_helper.wall_diagnostic_report(case / "run.log", WALL_BASELINE)
    result = {"schema": "fresh_build536_identical_cut_g3_reference_v1",
              "passed": decision["passed"], "case": str(case),
              "host_preflight": host, "launch_identity": launch_identity,
              "terminal_handles": handles, "cheap_bl_receipt_sha256": sha(case / "cheap_bl_receipt.json"),
              "g3_identity": decision, "old_integer_comparison_gate": False,
              "old_integer_comparison": old_rows, "wall_diagnostic_report": wall,
              "source_hashes": {str(path): sha(path) for path in
                  (SCRIPT, ALIAS, N8_VALIDATOR_SOURCE, BASE_CHECKS, IDENTITY_SOURCE, COMPARATOR)}}
    (case / "g3_reference536.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def fresh_case(before):
    """Find exactly one newly created G3 physical-guard attempt directory."""
    made = set((HERE / "cases").glob(CASE_PREFIX + "_*")) - before
    assert len(made) == 1, f"expected one new G3 attempt, found {sorted(map(str, made))}"
    return made.pop()


def prepare_runtime():
    """Verify build536, publish only local aliases, and configure accepted N8 helpers."""
    aliases = load("n4_g3_build536_aliases", ALIAS)
    verified = aliases.publish()
    status = verified["status"]
    assert status["state"] == "terminal" and status["exit_code"] == 0
    validator = load("n8_g3_accepted_validator", N8_VALIDATOR_SOURCE)
    validator.HERE, validator.STAGED, validator.REPO = HERE, STAGED, REPO
    validator.NIGHT, validator.SECTION1, validator.SECTION2 = NIGHT, SECTION1, SECTION2
    validator.BUILD = BUILD
    validator.IDENTITY_PATH = IDENTITY_SOURCE
    validator.REFERENCE_CASE = REFERENCE
    validator.WALL_REFERENCE_LOG = WALL_BASELINE
    validator.prepare_build = aliases.publish
    return aliases, validator, status


def self_check():
    """Run actual accepted setup to the solver Popen stub and verify command/pins."""
    host = host_preflight()
    aliases, validator, status = prepare_runtime()
    identity = load("n4_g3_identity_selfcheck", IDENTITY_SOURCE)
    identity_result = identity.self_check()
    assert identity_result["launched"] is False
    assert "at_least_336_splits" in identity.PHYSICAL_GATES
    assert "first_336_edits_identical" in identity.PHYSICAL_GATES
    case_root = HERE / "cases"
    before = set(case_root.glob(CASE_PREFIX + "_*")) if case_root.exists() else set()
    stub = {}
    try:
        try:
            execute_safeguard(validator, stub)
        except LaunchStub:
            pass
        case = fresh_case(before)
        command = json.loads((case / "command.json").read_text())
        pins = json.loads((case / "pins.json").read_text())
        assert stub and stub["start_new_session"] is True and Path(stub["cwd"]).resolve() == case.resolve()
        assert command["case"] == CASE_PREFIX and command["ranks"] == 1
        assert command["timeout_seconds"] == 900 and command["argv"][:3] == ["nice", "-n", "19"]
        assert str(status["test_binary"]) in command["argv"]
        assert pins.get(str(SCRIPT)) == sha(SCRIPT) and pins.get(str(ALIAS)) == sha(ALIAS)
        assert pins.get(str(N8_VALIDATOR_SOURCE)) == sha(N8_VALIDATOR_SOURCE)
        assert pins.get(str(BASE_CHECKS)) == sha(BASE_CHECKS)
        assert pins.get(str(IDENTITY_SOURCE)) == sha(IDENTITY_SOURCE)
        assert all(pins.get(str(path.resolve())) == sha(path) for path in reference_inputs())
    finally:
        case_root = HERE / "cases"
        made = set(case_root.glob(CASE_PREFIX + "_*")) - before if case_root.exists() else set()
        for attempt in made:
            shutil.rmtree(attempt)
    return {"passed": True, "launched": False, "host_preflight": host,
            "build536_binary_sha256": status["binary_sha256"],
            "build536_test_binary_sha256": status["test_binary_sha256"],
            "identity_selfcheck": identity_result, "stubbed_command": command,
            "published_alias_folder": str(HERE), "stub_reached_solver_boundary": True}


def launch():
    """Run one serial fresh G3 reference with existing monitored safeguards and strict identity."""
    host = host_preflight(require_idle=True)
    aliases, validator, status = prepare_runtime()
    case_root = HERE / "cases"
    case_root.mkdir(exist_ok=True)
    before = set(case_root.glob(CASE_PREFIX + "_*"))
    sys.path.insert(0, str(STAGED))
    import run_cases
    assert Path(run_cases.__file__).resolve() == STAGED / "run_cases.py"
    run_cases.ROOT = HERE
    run_cases.REPO = REPO
    run_cases.BINARY = Path(status["test_binary"])
    original_sample = run_cases.sample_tree
    run_cases.sample_tree = with_process_identity(original_sample)
    original_popen = subprocess.Popen
    launch_record = {}
    def tracked_popen(*args, **kwargs):
        argv = args[0] if args else kwargs.get("args")
        values = [str(value) for value in argv] if isinstance(argv, (tuple, list)) else [str(argv)]
        process = original_popen(*args, **kwargs)
        if values and values[0] == "nice":
            identity = {"pid": process.pid, "start_ticks": proc_start_ticks(process.pid),
                        "argv": values, "cwd": str(kwargs.get("cwd")), "started_unix": time.time()}
            launch_record.update(identity)
            attempt = Path(identity["cwd"])
            (attempt / "controller_launch_identity.json").write_text(json.dumps(identity, indent=2) + "\n")
        return process
    subprocess.Popen = tracked_popen
    runner_exit = 0
    try:
        execute_safeguard(validator)
    except SystemExit as result:
        runner_exit = int(result.code or 0)
    finally:
        subprocess.Popen = original_popen
        run_cases.sample_tree = original_sample
    case = fresh_case(before)
    status_case = json.loads((case / "status.json").read_text())
    terminal = (terminal_handles(case, launch_record) if launch_record else
                {"actual_host_proc": True, "all_owned_handles_terminal": True, "identities": []})
    if launch_record:
        assert (case / "controller_launch_identity.json").is_file()
    (case / "host_terminal_handles.json").write_text(json.dumps(terminal, indent=2) + "\n")
    if runner_exit or status_case.get("state") != "terminal" or status_case.get("exit_code") != 0:
        cheap_path = case / "cheap_bl_receipt.json"
        failure = {"schema": "fresh_build536_identical_cut_g3_reference_v1",
                   "passed": False, "first_failed_gate": "inherited cheap-BL physical guard or run status",
                   "case": str(case), "runner_exit_code": runner_exit,
                   "case_status": status_case,
                   "cheap_bl_receipt": json.loads(cheap_path.read_text()) if cheap_path.is_file() else None,
                   "host_preflight": host, "launch_identity": launch_record,
                   "terminal_handles": terminal}
        (case / "g3_reference536.json").write_text(json.dumps(failure, indent=2) + "\n")
        return failure
    assert launch_record, "successful case status lacks an observed launch identity"
    result = evaluate_case(case, host, launch_record)
    return {"passed": result["passed"], "case": str(case),
            "build536_binary_sha256": status["binary_sha256"],
            "build536_test_binary_sha256": status["test_binary_sha256"],
            "host_terminal_handles": terminal, "g3_reference": result}


def main():
    """Dispatch a no-solver setup self-check or the single fresh G3 reference run."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "launch"))
    args = parser.parse_args()
    result = self_check() if args.action == "self-check" else launch()
    print(json.dumps(result, indent=2))
    if args.action == "launch" and result.get("passed") is not True:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
