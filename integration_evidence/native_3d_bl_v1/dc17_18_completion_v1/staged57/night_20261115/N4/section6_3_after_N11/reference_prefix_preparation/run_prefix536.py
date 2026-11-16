#!/usr/bin/env python3
"""Fresh G2/GQ reference-prefix runner, pinned to build536 and accepted N1."""
import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N4 = HERE.parents[1]
PHASE = N4 / "section6_3_after_N11"
NIGHT = N4.parent
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
N8V = NIGHT / "N8/A2_A3_N12/section6_8/validation"
N8_GROUP = N8V.parent
PREFIX_SOURCE = N8V / "run_g2_gq_prefix.py"
VALIDATOR_SOURCE = N8V / "validate_g2_gq.py"
ALIAS_SOURCE = HERE / "build536_aliases.py"
BUILD = N4 / "section6_3_after_N11/build536"
N1 = NIGHT / "section6_1"


def sha(path):
    return hashlib.file_digest(Path(path).open("rb"), "sha256").hexdigest()


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def verify_build():
    alias = load("g2gq_build536_alias_source", N4 / "section6_3_after_N11/build536_aliases.py")
    checker = alias.load_verifier()
    checked = checker["verify_build"]()
    status = checked["status"]
    assert status["binary_sha256"] == "a144d90cdc50b6ca5d77e23f929294c271a7c15628b28136b65303f15e4083b1"
    assert status["test_binary_sha256"] == "b5fb04ffd903d94357467a35d8adc5e9c3ce32ce5dd562bd2539f55aa66f52f3"
    assert status["state"] == "terminal" and status["exit_code"] == 0
    alias_doc = {"binary": status["binary"], "binary_sha256": status["binary_sha256"],
                 "test_binary": status["test_binary"], "test_binary_sha256": status["test_binary_sha256"],
                 "build_alias_source": str(N4 / "section6_3_after_N11/build536_aliases.py"),
                 "build_alias_source_sha256": sha(N4 / "section6_3_after_N11/build536_aliases.py")}
    path = HERE / "build536_alias_receipt.json"
    payload = json.dumps(alias_doc, indent=2) + "\n"
    if path.exists():
        assert path.read_text() == payload
    else:
        path.write_text(payload)
    return checked


def host_preflight():
    mem = Path("/proc/meminfo").read_text().splitlines()
    available = int(next(row.split()[1] for row in mem if row.startswith("MemAvailable:"))) // 1024
    result = {"host": os.uname().nodename, "boot_id": Path("/proc/sys/kernel/random/boot_id").read_text().strip(),
              "pid1_present": Path("/proc/1/stat").is_file(), "loadavg": Path("/proc/loadavg").read_text().strip(),
              "mem_available_mib": available, "scratch_free_bytes": shutil.disk_usage(HERE).free}
    assert result["boot_id"] and result["pid1_present"] and available >= 19073 and result["scratch_free_bytes"] > 2_000_000_000
    return result


def configured():
    validator = load("g2gq_validator_build536", VALIDATOR_SOURCE)
    validator.HERE = HERE
    validator.BUILD = BUILD
    validator.COMPARATOR68 = N8V.parent / "drafts/compare_integer_rows_68.py"
    validator.COMPARATOR = N4 / "compare_integer_rows.py"
    # The validator requires the historic auxiliary pin map in its output root.
    extra = N8V / "unchanged_build499_source_pins.json"
    staged_extra = HERE / "unchanged_build499_source_pins.json"
    if not staged_extra.exists():
        shutil.copyfile(extra, staged_extra)
    assert sha(extra) == sha(staged_extra)
    sys.modules["validate_g2_gq"] = validator
    runner = load("g2gq_prefix_driver_build536", PREFIX_SOURCE)
    runner.__file__ = str(Path(__file__).resolve())
    runner.HERE = HERE
    runner.CASES = HERE / "cases"
    runner.BUILD = BUILD
    runner.GROUP = PHASE
    runner.A2 = PHASE
    runner.N1_FULL = N1 / "full_workflow"
    runner.N68_COMPARATOR = validator.COMPARATOR68
    runner.N4_DIR = N4
    # Hash the actual frozen helpers that this derived invocation executes.
    original_patch = runner.patched_controller
    def pinned_controller():
        source = original_patch()
        for old, new in (("PURPOSE / \"validate_g2_gq.py\"", repr(str(VALIDATOR_SOURCE))),
                         ("PURPOSE / \"run_g2_gq_prefix.py\"", repr(str(PREFIX_SOURCE)))):
            if old in source:
                source = source.replace(old, new)
        return source
    runner.patched_controller = pinned_controller
    env, returned_validator = runner.namespace()
    assert returned_validator is validator
    env.update(PURPOSE=HERE, SCRIPT=Path(__file__).resolve(), BUILD=BUILD,
               BUILD_STATUS=BUILD / "build_status.json", SOURCE_PINS=BUILD / "source_pins.json",
               N1_FULL=N1 / "full_workflow", REFERENCE_RUN=N1 / "full_workflow",
               N68_COMPARATOR=validator.COMPARATOR68)
    env["verify_pins"] = lambda strict=True: runner.verify_build_pins(validator, env)
    return runner, validator, env


class LaunchStop(Exception):
    pass


def self_check(gate):
    checked = verify_build()
    host = host_preflight()
    runner, validator, env = configured()
    assert env["PURPOSE"] == HERE and env["BUILD"] == BUILD
    assert env["GROUP"] == PHASE and env["A2"] == PHASE
    assert env["PURPOSE_SOURCE_CHECK"] == PHASE / "source_check.md"
    assert env["REFERENCE_RUN"] == N1 / "full_workflow"
    assert validator.HERE == HERE and validator.BUILD == BUILD
    assert validator.N1_COLLAPSE == N1 / "first_collapse" and validator.N1_FULL == N1 / "full_workflow"
    assert validator.COMPARATOR68.is_file()
    assert validator.accepted_n1()["workflow"].get("workflow_accepted") is True
    if gate == "G2":
        comp_check = subprocess.run([sys.executable, str(validator.COMPARATOR68), "--self-check"],
                                    capture_output=True, text=True, check=False)
        assert comp_check.returncode == 0, comp_check.stdout + comp_check.stderr
    else:
        validator.verify_gq_population(validator.accepted_n1())
    assert 'GROUP / "source_check.md"' in runner.patched_controller()
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    case = HERE / "cases" / f".selfcheck_{gate}_{os.getpid()}"
    old_popen = subprocess.Popen
    captured = {}
    def popen_stub(*args, **kwargs):
        argv = args[0] if args else kwargs.get("args")
        argv = list(map(str, argv)) if isinstance(argv, (list, tuple)) else [str(argv)]
        if argv and (argv[0] == "nice" or "mpirun" in argv):
            captured.update(argv=argv, cwd=str(kwargs.get("cwd")), env=kwargs.get("env", {}),
                            start_new_session=kwargs.get("start_new_session"))
            raise LaunchStop("stubbed before solver")
        return old_popen(*args, **kwargs)
    subprocess.Popen = popen_stub
    try:
        runner.CASES.mkdir(exist_ok=True)
        sys.argv = [str(N4 / "run_prefix_controller.py"), str(case), "--gate", gate]
        try:
            # Execute actual adapted setup up to subprocess launch.
            env["verify_pins"]()
            env["main"]()
        except LaunchStop:
            pass
    finally:
        subprocess.Popen = old_popen
    provenance = json.loads((case / "provenance.json").read_text())
    assert str(PHASE / "source_check.md") in provenance["input_hashes"]
    assert str(N8_GROUP / "source_check.md") not in provenance["input_hashes"]
    if case.exists():
        shutil.rmtree(case)
    assert captured, "did not reach intercepted solver launch"
    args = captured["argv"]
    mpi = args.index("mpirun")
    assert args[mpi + 1:mpi + 4] == ["--bind-to", "none", "-n"] and args[mpi + 4] == "6", args
    assert str(checked["status"]["binary"]) in args and captured["start_new_session"] is True
    assert Path(captured["cwd"]).resolve() == case.resolve()
    assert captured["env"].get("OMPI_MCA_osc") == "pt2pt"
    assert validator.COMPARATOR68.is_file() and validator.COMPARATOR68 != N4 / "compare_integer_rows.py"
    return {"passed": True, "launched": False, "gate": gate,
            "build_binary_sha256": checked["status"]["binary_sha256"],
            "test_binary_sha256": checked["status"]["test_binary_sha256"],
            "accepted_reference": str(validator.N1_COLLAPSE if gate == "G2" else validator.N1_FULL),
            "timer_census_comparator": str(validator.COMPARATOR68),
            "timer_census_comparator_sha256": sha(validator.COMPARATOR68),
            "source_check": str(PHASE / "source_check.md"),
            "ompi_mca_osc": captured["env"]["OMPI_MCA_osc"],
            "command": args, "cwd": captured["cwd"], "host_preflight": host,
            "stopped_at": "actual N8.6.8 adapted controller subprocess.Popen stub"}


def launch(gate, case_name):
    assert gate in ("G2", "GQ")
    checked = verify_build()
    host = host_preflight()
    runner, validator, env = configured()
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    case = (HERE / "cases" / case_name).resolve()
    assert case.parent == (HERE / "cases").resolve() and not case.exists()
    runner.CASES.mkdir(exist_ok=True)
    sys.argv = [str(N4 / "run_prefix_controller.py"), str(case), "--gate", gate]
    env["verify_pins"]()
    try:
        env["main"]()
    except SystemExit as exit_result:
        code = exit_result.code if isinstance(exit_result.code, int) else int(bool(exit_result.code))
        if code != 0:
            raise
    result = validator.validate(gate, case)
    receipt = {"passed": True, "gate": gate, "case": str(case),
               "build536_binary_sha256": checked["status"]["binary_sha256"],
               "build536_test_binary_sha256": checked["status"]["test_binary_sha256"],
               "accepted_N1_reference": str(validator.N1_COLLAPSE if gate == "G2" else validator.N1_FULL),
               "N8_comparator_sha256": sha(validator.COMPARATOR68), "host_preflight": host,
               "validation_result": result}
    path = case / "fresh_build536_prefix_reference.json"
    path.write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "G2", "GQ"))
    parser.add_argument("--case-name")
    args = parser.parse_args()
    result = ({gate: self_check(gate) for gate in ("G2", "GQ")}
              if args.action == "self-check" else launch(args.action, args.case_name))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
