#!/usr/bin/env python3
"""Check G4 build/house pins and stop each rank case before test_driver Popen."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
N4 = PHASE.parent
STAGED = N4.parents[1]
REPO = STAGED.parents[3]
RUNNER = HERE / "run_g4_558.py"
CUT_CHECKS = N4 / "run_cut_checks.py"
UNIT_GATES = N4 / "run_unit_gates.py"
BUILD = PHASE / "build558"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
FILTER = "[NativeEngine2D],[NativeMesh2D],[NativeBL2D]"
GATES = (("2d1", 1), ("2d2", 2), ("2d4", 4))


class SetupReached(Exception):
    """The harness reached case Popen after command and pin receipts were saved."""


def sha(path: Path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def prepare() -> tuple[dict, Path]:
    """Run checked_step against build558 and require all sealed provenance."""
    runner = load("endb_g4_runner_for_stub", RUNNER)
    receipt = runner.self_check()
    adapter = load("endb_g4_cut_checks", CUT_CHECKS)
    step, status_path, pins_path = adapter.checked_step(str(BUILD), "build558")
    status = json.loads(status_path.read_text())
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    cut_plan = json.loads((BUILD / "cut_build_plan.json").read_text())
    assert status.get("state") == "terminal" and status.get("pins_verified") is True
    assert handles.get("actual_host_proc") is True and handles.get("all_owned_handles_terminal") is True
    assert cut_plan.get("build_label") == "build558"
    assert (STAGED / "run_cases.py").is_file() and RUNTIME.is_file()
    return {"runner_self_check": receipt, "step": step, "status": status,
            "status_path": status_path, "pins_path": pins_path}, step


def setup_one(context: dict, case_name: str, ranks: int) -> dict:
    """Execute run_cases preparation in a temp root and intercept its case Popen."""
    step = context["step"]
    with tempfile.TemporaryDirectory(prefix=f"endb_g4_{case_name}_setup_") as folder:
        root = Path(folder)
        for name in ("build558_status.json", "build558_source_pins.json", "build558_terminal_handles.json"):
            source = step / ("build_status.json" if name.endswith("status.json") else
                             "source_pins.json" if name.endswith("source_pins.json") else "terminal_handles.json")
            value = json.loads(source.read_text())
            if name == "build558_status.json":
                value["completed_build"] = True
            (root / name).write_text(json.dumps(value, indent=2) + "\n")

        namespace = {"__name__": "endb_g4_stub_run_cases", "__file__": str(STAGED / "run_cases.py")}
        house_path = STAGED / "run_cases.py"
        exec(compile(house_path.read_text(), str(house_path), "exec"), namespace)
        namespace.update(ROOT=root, REPO=REPO, BINARY=Path(context["status"]["test_binary"]))
        case = f"build558_{case_name}"
        namespace["CASES"][case] = (ranks, FILTER)
        original_save = namespace["save"]
        pinned = [RUNNER, Path(__file__).resolve(), CUT_CHECKS, UNIT_GATES,
                  step / "build_status.json", step / "source_pins.json",
                  step / "terminal_handles.json", step / "primary_terminal_receipt.json",
                  step / "cut_build_plan.json", RUNTIME]

        def save(path, value):
            if Path(path).name == "pins.json":
                for item in pinned:
                    value[str(item.resolve())] = namespace["sha"](item)
            original_save(path, value)

        namespace["save"] = save
        subprocess = namespace["subprocess"]
        original_popen = subprocess.Popen
        observed = []

        def intercept(argv, *args, **kwargs):
            values = [str(value) for value in argv]
            binary = str(namespace["BINARY"])
            if values and values[0] != "ldd" and binary in values:
                attempt = Path(kwargs["cwd"])
                assert (attempt / "pins.json").is_file()
                assert (attempt / "command.json").is_file()
                command = json.loads((attempt / "command.json").read_text())
                pins = json.loads((attempt / "pins.json").read_text())
                assert command["case"] == case and command["ranks"] == ranks
                assert command["argv"][-3] == FILTER
                assert all(str(item.resolve()) in pins and pins[str(item.resolve())] == sha(item)
                           for item in pinned)
                observed.append({"case": case, "ranks": ranks, "argv": command["argv"],
                                 "pins_sha256": sha(attempt / "pins.json"),
                                 "command_sha256": sha(attempt / "command.json"),
                                 "pin_count": len(pins)})
                raise SetupReached("intentional stop before test_driver Popen")
            return original_popen(argv, *args, **kwargs)

        subprocess.Popen = intercept
        old_argv = sys.argv[:]
        try:
            sys.argv = [str(house_path), "build558", case]
            try:
                namespace["main"](RUNTIME)
            except SetupReached:
                pass
        finally:
            subprocess.Popen = original_popen
            sys.argv = old_argv
        assert len(observed) == 1, (case, len(observed))
        return observed[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_mutually_exclusive_group(required=True)
    actions.add_argument("--static-check", action="store_true")
    actions.add_argument("--self-check", action="store_true",
                         help="run the no-launch pin/Popen stub after build558 is sealed")
    args = parser.parse_args()
    if args.static_check:
        runner_source = RUNNER.read_text()
        compile(runner_source, str(RUNNER), "exec")
        assert CUT_CHECKS.is_file() and UNIT_GATES.is_file()
        assert '"g4": ("2d1", "2d2", "2d4")' in UNIT_GATES.read_text()
        assert GATES == (("2d1", 1), ("2d2", 2), ("2d4", 4))
        print(json.dumps({"passed": True, "launched": False, "built": False,
                          "g4_runner": str(RUNNER), "gates": GATES,
                          "stub_action": "--self-check requires a sealed build558"}, indent=2))
        return
    if (HERE / "g4_prelaunch_stub_receipt.json").exists():
        raise FileExistsError(HERE / "g4_prelaunch_stub_receipt.json")
    context, _ = prepare()
    rows = [setup_one(context, name, ranks) for name, ranks in GATES]
    assert [(row["case"], row["ranks"]) for row in rows] == [
        (f"build558_{name}", ranks) for name, ranks in GATES]
    receipt = {"passed": True, "launched": False, "built": False,
               "stage": "checked_step, source/binary/terminal provenance and run_cases pins",
               "intercept": "after command.json and pins.json, before test_driver Popen",
               "gates": rows}
    out = HERE / "g4_prelaunch_stub_receipt.json"
    out.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
