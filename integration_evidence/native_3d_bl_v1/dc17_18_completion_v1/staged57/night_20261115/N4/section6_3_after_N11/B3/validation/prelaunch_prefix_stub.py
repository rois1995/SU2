#!/usr/bin/env python3
"""Exercise the G2/GQ controller through saved case provenance, stopping at solver Popen."""
from __future__ import annotations
import argparse
import json
import os
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
WRAPPER = HERE / "run_g2_gq_prefix_553.py"


class SetupReached(Exception):
    """Intentional no-launch stop immediately before the native solver starts."""


def load_wrapper():
    """Import the actual B3 prefix wrapper without invoking its CLI."""
    import importlib.util
    spec = importlib.util.spec_from_file_location("b3_prefix_prelaunch_stub", WRAPPER)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_inputs(module, gate):
    """Verify sealed build aliases and saved references before constructing the stub case."""
    module.publish_aliases()
    validator = module.load_validation()
    reference = validator.reference_case(gate)
    status, provenance = validator.verify_reference(reference, gate)
    assert status.get("all_owned_handles_terminal") and status.get("inputs_unchanged")
    assert provenance.get("gate") == gate
    runner = module.load_runner()
    env, imported = runner["namespace"]()
    assert imported.BUILD == module.BUILD and env["BUILD_STATUS"] == module.BUILD / "build_status.json"
    assert env["N68_COMPARATOR"] == module.TIMER_HELPER
    return runner, env, validator, reference, status


def stop_before_solver(env, controller, gate, case):
    """Run the controller's preparation path and intercept only its solver Popen."""
    original = env["subprocess"].Popen
    seen = []
    binary = str(env["BINARY"])

    def intercept(argv, *args, **kwargs):
        values = [str(value) for value in argv]
        if values and values[0] == "nice" and binary in values:
            folder = Path(kwargs["cwd"])
            required = (folder / "run.cfg", folder / "provenance.json", folder / "launch_preflight.json")
            assert all(path.is_file() for path in required), "solver Popen preceded saved case provenance"
            seen.append({"argv": values, "case_dir": str(folder),
                         "saved_sha256": {path.name: __import__("hashlib").sha256(
                             path.read_bytes()).hexdigest() for path in required}})
            raise SetupReached("intentional stop before SU2_CFD Popen")
        return original(argv, *args, **kwargs)

    env["subprocess"].Popen = intercept
    try:
        old_argv = sys.argv[:]
        sys.argv = [str(controller["N4_SCRIPT"]), str(case), "--gate", gate]
        try:
            controller["main"]()
        except SetupReached:
            pass
        finally:
            sys.argv = old_argv
    finally:
        env["subprocess"].Popen = original
    assert len(seen) == 1, f"expected one intercepted solver Popen, saw {len(seen)}"
    return seen[0]


def main():
    """Prepare one prefix case in a disposable folder, then remove its no-launch artifacts."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gate", choices=("G2", "GQ"), required=True)
    args = parser.parse_args()
    wrapper = load_wrapper()
    runner, env, validator, reference, status = check_inputs(wrapper, args.gate)
    case = HERE / "cases" / f".prelaunch_stub_{args.gate.lower()}_{os.getpid()}"
    assert not case.exists()
    stub = stop_before_solver(env, env, args.gate, case)
    shutil.rmtree(case)
    receipt = {"passed": True, "launched": False, "built": False, "gate": args.gate,
               "reference": str(reference), "reference_status": status_summary(reference),
               "stubbed_command": stub, "case_removed_after_stub": True}
    target = HERE / f"{args.gate}_prelaunch_stub_receipt.json"
    target.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


def status_summary(reference):
    """Return only terminal reference status fields used by this setup stub."""
    value = json.loads((reference / "status.json").read_text())
    return {key: value.get(key) for key in ("state", "gate", "gate_passed", "all_owned_handles_terminal")}


if __name__ == "__main__":
    main()
