#!/usr/bin/env python3
"""Run the accepted isolated restart workflow against the build640 full case."""
import argparse
import importlib.util
import json
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
NIGHT = N11.parents[1]
STAGED = NIGHT.parent
SECTION68 = NIGHT / "N8/A2_A3_N12/section6_8"
SECTION1 = NIGHT / "section6_1"
BUILD = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP2_B/build640')
BASE = SECTION68 / "validation/restart534/run_disk_restart534.py"


def load(path):
    spec = importlib.util.spec_from_file_location("n11_restart_runner534", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def runner_context():
    helper = load(BASE)
    helper.HERE = HERE
    helper.VALIDATION = HERE
    helper.SECTION6_8 = SECTION68
    helper.SECTION1 = SECTION1
    helper.STAGED = STAGED
    status = json.loads((BUILD / "build_status.json").read_text())
    helper.CFD_SHA = status["binary_sha256"]
    helper.TEST_SHA = status["test_binary_sha256"]
    original_load = helper.load_base

    def load_base():
        core = original_load()
        original_source = core.retargeted_source

        def retargeted_source():
            source = original_source()
            replacements = (
                ('sys.path.insert(0, str(Path(__file__).resolve().parents[6]))',
                 f'sys.path.insert(0, str(Path({str(STAGED)!r})))'),
                ("HERE = Path(__file__).resolve().parent", f"HERE = Path({str(HERE)!r})"),
                ("STAGED = Path(__file__).resolve().parents[6]", f"STAGED = Path({str(STAGED)!r})"),
                ('Path(__file__).resolve().parents[2] / "build534/build_status.json"',
                 f'Path({str(BUILD)!r}) / "build_status.json"'),
                ('Path(__file__).resolve().parents[2] / "build534/source_pins.json"',
                 f'Path({str(BUILD)!r}) / "source_pins.json"'),
                ('Path(__file__).resolve().parents[1] / "unchanged_build499_source_pins.json"',
                 f'Path({str(SECTION68 / "validation/unchanged_build499_source_pins.json")!r})'),
                ("run_disk_restart534.py", "run_restart640.py"),
                ("validate_disk_restart534.py", "validate_restart640.py"),
            )
            for old, new in replacements:
                assert source.count(old) >= 1, old
                source = source.replace(old, new)
            compile(source, str(BASE), "exec")
            return source

        core.retargeted_source = retargeted_source

        def supervise(source, source_case, case):
            argv = [sys.executable, str(HERE / "run_restart640.py"), "--internal",
                    "--source-case", str(source_case), "--case-dir", str(case)]
            child = subprocess.Popen(argv, cwd=HERE, start_new_session=True)
            identity = core.wait_identity(child.pid)
            wrapper = {"pid": child.pid, "start_ticks": identity["start_ticks"],
                       "started_unix": time.time(), "argv": argv,
                       "owned_cpu_ids": sorted(os.sched_getaffinity(0))}
            (case.parent / f"{case.name}_controller_launch.json").write_text(json.dumps(wrapper, indent=2) + "\n")
            code = child.wait()
            receipt = core.post_terminal_receipt(case, wrapper)
            assert receipt["all_owned_handles_terminal"], receipt
            raise SystemExit(code)

        core.supervise = supervise
        return core

    helper.load_base = load_base
    return helper


def self_check():
    helper = runner_context()
    core = helper.load_base()
    source = core.retargeted_source()
    assert "run_restart640.py" in source and "validate_restart640.py" in source
    assert str(BUILD) in source and "build534" not in source
    assert str(SECTION68 / "validation/unchanged_build499_source_pins.json") in source
    validator = (HERE / "validate_restart640.py").read_text()
    assert 'log=(case / "run.log").read_text(errors="replace")' in validator
    status = json.loads((BUILD / "build_status.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    return {"passed": True, "launched": False, "build": str(BUILD),
            "accepted_runner": str(BASE), "validator": str(HERE / "validate_restart640.py"),
            "build_sha256": status["binary_sha256"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--internal", action="store_true")
    parser.add_argument("--source-case", type=Path)
    parser.add_argument("--case-dir", type=Path)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if args.source_case is None:
        parser.error("provide --source-case")
    helper = runner_context()
    core = helper.load_base()
    source_case = args.source_case.resolve(strict=True)
    reference = source_case / "accepted_reference_payloads.json"
    payloads = json.loads(reference.read_text())
    assert len(payloads["hashes"]) == 6
    assert source_case.name == "full640_run1"
    # Bind only the accepted validator's old reference path and expected hashes.
    helper.verify_n8_identity.__globals__.update(SECTION1=source_case, REFERENCE_HASHES=payloads["hashes"])
    helper.read_source_receipts(source_case)
    core.apply_owned_cpu_budget()
    case = args.case_dir.resolve() if args.case_dir else HERE / f"disk_restart_{source_case.name}_event2"
    assert HERE in case.parents and not case.exists()
    source = core.retargeted_source()
    if args.internal:
        os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
        os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
        exec(compile(source, str(BASE), "exec"),
             {"__name__": "__main__", "__file__": str(HERE / "run_restart640.py")})
        return
    core.supervise(source, source_case, case)


if __name__ == "__main__":
    main()
