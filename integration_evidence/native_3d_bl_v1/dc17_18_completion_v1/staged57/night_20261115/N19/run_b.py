"""Build and run the M29 diagnostic with the N18 F command and inputs."""
import argparse
import ast
from hashlib import sha256
from pathlib import Path
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "N18"))
from run_de import bind_build, cut

WORK = HERE / "PartB"
REFERENCE = cut.PHASE / "round_8_5r2_F/G5/full580_run1"
CANDIDATE = "6b56b42f16"
WRAPPER = cut.NIGHT / "section6_2/run_full_section6_2.py"
CORE = cut.NIGHT.parent / "p7p8_r3_20261115/run_full_workflow.py"
F_PATHS = (
    "Common/include/adaptation/CNativeEngine3D.hpp",
    "SU2_CFD/src/drivers/CSinglezoneDriver.cpp",
    "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp",
)
HELPERS = (
    "check_complete_events.py",
    "check_connected_events.py",
    "check_timer_tree.py",
    "unchanged_build499_source_pins.json",
)


def git(*args):
    return subprocess.check_output(["git", *args], cwd=cut.REPO)


def overlay():
    return git(
        "diff", "--binary", "--full-index", "--no-ext-diff", "--no-color",
        CANDIDATE + "^", CANDIDATE, "--", *F_PATHS
    )


def build(number):
    """Use the established build adapter and prove restored source is the Part A source."""
    part_a_pins = HERE / "A_checkpoint_source_pins.json"
    changed = F_PATHS if number == 582 else ()
    if changed:
        commit_paths = git("diff-tree", "--no-commit-id", "--name-only", "-r", "HEAD")
        assert set(commit_paths.decode().splitlines()) == set(F_PATHS)
        assert "DIAGNOSTIC" in git("log", "-1", "--format=%s").decode()
        subprocess.run(["git", "apply", "--reverse", "--check"], input=overlay(),
                       cwd=cut.REPO, check=True)
    target = bind_build(581, number, changed, part_a_pins)
    cut.run_gate(target, "build", [cut.PYTHON, str(target / "build.py")])
    status = verify_build(target)
    if not changed:
        assert cut.read(target / "source_pins.json") == cut.read(part_a_pins)
        subprocess.run(
            [cut.PYTHON, str(cut.PREFLIGHT), str(target / "restored_host_preflight.json")],
            cwd=cut.REPO, check=True,
        )
    print(f"Build {number} verified: {status['binary_sha256']}", flush=True)


def verify_build(target):
    status = cut.read(target / "build_status.json")
    handles = cut.read(target / "terminal_handles.json")
    assert status["state"] == "terminal" and status["exit_code"] == 0
    assert status["pins_verified"] and not status["changed"]
    assert handles["all_owned_handles_terminal"]
    assert all(cut.terminal(row) for row in handles["identities"])
    assert cut.sha(status["binary"]) == status["binary_sha256"]
    return status


def verify_case_inputs(provenance, case, binary, helper_dir=WORK):
    """Check against the recorded N18 launch before starting MPI."""
    original = cut.read(REFERENCE / "provenance.json")
    original_binary = Path(original["argv"][-2])
    expected_argv = [
        str(binary) if part == str(original_binary) else part.replace(str(REFERENCE), str(case))
        for part in original["argv"]
    ]
    assert provenance["argv"] == expected_argv
    assert provenance["environment"] == {
        key: value.replace(str(REFERENCE), str(case))
        for key, value in original["environment"].items()
    }
    for key in ("config_sha256", "baseline_binary_sha256", "command_basis", "mode",
                "native_time_limit_seconds_per_event", "application_watchdog_seconds",
                "max_work_units"):
        assert provenance[key] == original[key], key
    changed_names = {
        "run.cfg", "SU2_CFD", "run_full580.py", "build_status.json", "source_pins.json", *HELPERS
    }
    for path, digest in original["input_hashes"].items():
        name = Path(path).name
        if name in HELPERS and Path(path).parent == REFERENCE.parent:
            assert provenance["input_hashes"][str(helper_dir / name)] == digest, path
        elif name not in changed_names or Path(path).parent == cut.NIGHT.parent / "f3b_20261115":
            assert provenance["input_hashes"][path] == digest, path


def diagnostic_main(runner, controller_source):
    functions = ast.parse(controller_source).body
    function, = [node for node in functions if isinstance(node, ast.FunctionDef) and node.name == "main"]
    source = ast.get_source_segment(controller_source, function) + "\n"
    # Retain the inherited rejected-event check, resource limit, polling and owned shutdown.
    start = source.index("            if full_mode:\n                for completed_event in (1, 2):")
    end = source.index("                summaries=event_summaries(case)", start)
    source = source[:start] + "            if full_mode:\n" + source[end:]
    stop = """                if not p6_collapse['passed']:
                    stop_reason = 'final_source_first_collapse_gate_failed'
                    break
"""
    assert source.count(stop) == 1
    source = source.replace(stop, "")
    start = source.index("    full_receipt = None")
    end = source.index("    rank0_logs =", start)
    source = source[:start] + """    full_receipt = {
        "workflow_accepted": None, "errors": [], "validation_scope": "diagnostic only"
    }
    save(case / "full_workflow_receipt.json", full_receipt)
""" + source[end:]
    declaration = '    save(case / "provenance.json", provenance)'
    assert source.count(declaration) == 1
    source = source.replace(
        declaration, '    input_hashes.update(controller_sources)\n'
                     '    verify_case_inputs(provenance, case, BINARY)\n' + declaration
    ).replace(
        '"stop_rule": "P6 check2 failure or all six construction_entry rows; controller watchdog6000s; native event budget unchanged5400s",',
        '"stop_rule": "stop at first rejected adaptation; inherited resource caps and watchdog",',
    )
    start = source.index("    raise SystemExit(bool(live)")
    source = source[:start] + (
        '    raise SystemExit(bool(live) or bool(inputs_changed) or pin_changes '
        'or not result["native_case_started"])\n'
    )
    exec(compile(source, runner["__file__"], "exec"), runner)


def controller():
    pins = {}
    for path in (WRAPPER, CORE):
        original = git("show", f"{CANDIDATE}:{path.relative_to(cut.REPO)}")
        pins[str(path)] = sha256(original).hexdigest()
        assert cut.sha(path) == pins[str(path)], path
    prepared = {}

    def remember_core(source, filename, mode):
        assert Path(filename) == CORE and "source" not in prepared
        prepared["source"] = source
        return compile(source, filename, mode)

    wrapper = {"__file__": str(WRAPPER), "__name__": "m29_full_controller", "compile": remember_core}
    exec(compile(WRAPPER.read_text(), str(WRAPPER), "exec"), wrapper)
    wrapper["HERE"] = WORK
    runner = wrapper["load_runner"]()
    runner["controller_sources"] = pins
    diagnostic_main(runner, prepared["source"])
    return runner, wrapper["OLD"]


def collect(number, case):
    build_dir = cut.PHASE / f"build{number}"
    status = verify_build(build_dir)
    runner, old = controller()
    runner.update(
        SCRIPT=Path(__file__).resolve(), BUILD_STATUS=build_dir / "build_status.json",
        SOURCE_PINS=build_dir / "source_pins.json", BINARY=Path(status["binary"]).resolve(),
        verify_case_inputs=verify_case_inputs,
    )
    sys.path[0:0] = [str(WORK), str(old)]
    sys.argv = [__file__, str(case), "--full"]
    runner["main"]()


def run(number, retry):
    assert retry or number == 582
    WORK.mkdir(exist_ok=True)
    inputs = cut.read(REFERENCE / "provenance.json")["input_hashes"]
    for name in HELPERS:
        source = REFERENCE.parent / name
        target = WORK / name
        expected = inputs[str(source)]
        assert cut.sha(source) == expected
        if not target.exists():
            shutil.copyfile(source, target)
        assert cut.sha(target) == expected
    attempt = 2 if retry else 1
    if retry:
        previous = cut.read(WORK / "full582_run1/status.json")
        assert previous["state"] == "terminal" and previous["all_owned_handles_terminal"]
        assert all(cut.terminal(row) for row in previous["owned_identities"])
        assert not list(WORK.glob("full*_run2")), "the R3 repeat has already run"
    case = WORK / f"full{number}_run{attempt}"
    assert not case.exists(), case
    cut.run_gate(WORK, f"run{attempt}",
                 [cut.PYTHON, str(Path(__file__).resolve()), "_collect", str(number), str(case)])
    status = cut.read(case / "status.json")
    assert status["state"] == "terminal" and status["all_owned_handles_terminal"]
    assert all(cut.terminal(row) for row in status["owned_identities"])
    assert status["inputs_unchanged"] and status["source_and_binary_pins_unchanged"]
    print(f"{case}: {status['stop_reason']}", flush=True)


def self_check():
    runner, _ = controller()
    inputs = cut.read(REFERENCE / "provenance.json")
    verify_case_inputs(inputs, REFERENCE, Path(inputs["argv"][-2]), REFERENCE.parent)
    assert runner["main"].__globals__ is runner
    constants = runner["main"].__code__.co_consts
    assert "adaptation_rejected_stop_first_failure" in constants
    assert "whole_job_peak_RSS_gate_failed" in constants
    assert "final_source_first_collapse_gate_failed" not in constants
    assert "owned_active_session_ids" in runner["main"].__code__.co_names
    print("M29 diagnostic source and prior provenance checked. No build or workflow launched.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("patch")
    commands.add_parser("self-check")
    builder = commands.add_parser("build")
    builder.add_argument("number", type=int, choices=(582, 583))
    runner = commands.add_parser("run")
    runner.add_argument("--build", type=int, default=582)
    runner.add_argument("--retry", action="store_true")
    worker = commands.add_parser("_collect")
    worker.add_argument("number", type=int)
    worker.add_argument("case", type=Path)
    args = parser.parse_args()
    if args.command == "patch":
        sys.stdout.buffer.write(overlay())
    elif args.command == "build":
        build(args.number)
    elif args.command == "run":
        run(args.build, args.retry)
    elif args.command == "_collect":
        collect(args.number, args.case)
    else:
        self_check()


if __name__ == "__main__":
    main()
