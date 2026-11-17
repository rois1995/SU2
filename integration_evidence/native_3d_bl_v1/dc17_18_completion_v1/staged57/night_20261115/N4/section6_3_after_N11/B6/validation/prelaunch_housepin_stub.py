#!/usr/bin/env python3
"""Exercise G1 house setup through saved pins while intercepting test_driver Popen."""
from __future__ import annotations
import argparse
import importlib.util
import json
import shutil
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
P = HERE.parent.parent
BUILD = P / "build556"
ALIAS = P / "build556_aliases.py"
WRAPPER = HERE / "run_g1_556.py"


class SetupReached(Exception):
    """Raised at the exact test-process boundary after all run_cases pins are saved."""


def load_g1():
    """Import only the new B6 G1 wrapper and retain its source globals."""
    spec = importlib.util.spec_from_file_location("b6_g1_prelaunch_stub", WRAPPER)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def host_receipt(path):
    """Save a short real-host snapshot tied to the sealed terminal build handles."""
    if path.is_file():
        receipt = json.loads(path.read_text())
        assert receipt.get("sample_seconds", 0) >= 3 and receipt.get("setup_stub_only")
        return receipt
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    assert handles.get("all_owned_handles_terminal")
    assert not any(row.get("alive") for row in handles.get("identities", []))
    samples = []
    started = time.monotonic()
    while time.monotonic() - started < 3.05:
        mem = next(row for row in Path("/proc/meminfo").read_text().splitlines()
                   if row.startswith("MemAvailable:"))
        samples.append({"unix": time.time(), "MemAvailable": mem,
                        "loadavg": Path("/proc/loadavg").read_text().strip()})
        time.sleep(0.5)
    receipt = {"sample_seconds": time.monotonic() - started,
               "active_OWNED_heavy_processes": [], "owned_process_basis": str(BUILD / "terminal_handles.json"),
               "terminal_build_handles_sha256": __import__("hashlib").sha256(
                   (BUILD / "terminal_handles.json").read_bytes()).hexdigest(),
               "MemAvailable": samples[-1]["MemAvailable"], "loadavg": samples[-1]["loadavg"],
               "samples": samples, "setup_stub_only": True,
               "must_refresh_before_real_launch": True}
    path.write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def prepare_g1(cohort, workspace=None):
    """Verify all exact cohort inputs, save READY and house pins, then stop before Popen."""
    g1 = load_g1()
    if workspace is not None:
        g1.HERE = Path(workspace)
    runner = g1.configure_runner()
    assert runner.WALL_EXCEPTIONS == frozenset()
    alias = runner.load_aliases()
    verified_alias = alias.verify_and_publish()
    status = json.loads((BUILD / "build_status.json").read_text())
    host = g1.HERE / f"G1_{cohort}_host_preflight.json"
    if workspace is None:
        host_receipt(host)
    else:
        host.write_text(json.dumps({"sample_seconds": 3.1, "active_OWNED_heavy_processes": [],
                                    "MemAvailable": "setup-stub-only", "loadavg": "setup-stub-only",
                                    "setup_stub_only": True, "not_launch_evidence": True}) + "\n")
    module = runner.load_candidate()
    module.READY = g1.HERE / f"validation_run_ready_{cohort}.json"
    g1.ready_receipt(runner, module, status, cohort)
    parser = module.load_module("b6_g1_prelaunch_identity", runner.PARSER)
    manifest, reports, selection = g1.verify_fresh536(parser)
    build_status, pins, graph, handles = module.verify_build()
    runtime = module.check_runtime()
    module.expose_build_alias(build_status, pins)
    ranks = {"serial": 1, "mpi2": 2, "mpi4": 4}[cohort]
    case, house = module.prepare_house(cohort, ranks,
        __import__("math").ceil(manifest["cohorts"][cohort]["seconds"] * 1.5),
        selection, build_status, pins, host)
    return {"g1": g1, "runner": runner, "module": module, "parser": parser, "house": house,
            "case": case, "cohort": cohort, "status": build_status,
            "pins": pins, "graph": graph, "handles": handles,
            "runtime": module.RUNTIME, "runtime_check": runtime, "alias": verified_alias,
            "workspace": Path(g1.HERE)}


def stop_at_test_popen(prepared):
    """Run run_cases setup in a temporary root and intercept only its test binary launch."""
    with tempfile.TemporaryDirectory(prefix="b6_g1_housepin_") as directory:
        root = Path(directory)
        for name in ("build556_status.json", "build556_source_pins.json", "build556_terminal_handles.json"):
            (root / name).symlink_to(prepared["workspace"] / name)
        house = prepared["house"]
        house["ROOT"] = root
        original = house["subprocess"].Popen
        seen = []

        def intercept(argv, *args, **kwargs):
            values = [str(value) for value in argv]
            binary = str(prepared["status"]["test_binary"])
            if values and values[0] == "nice" and binary in values:
                attempt = Path(kwargs["cwd"])
                required = (attempt / "pins.json", attempt / "command.json")
                assert all(path.is_file() for path in required), "Popen reached before housepin-save"
                seen.append({"argv": values, "case_dir": str(attempt),
                             "saved": {path.name: __import__("hashlib").sha256(path.read_bytes()).hexdigest()
                                       for path in required}})
                raise SetupReached("intentional stop before test_driver Popen")
            return original(argv, *args, **kwargs)

        house["subprocess"].Popen = intercept
        try:
            import sys
            old_argv = sys.argv[:]
            sys.argv = [str(prepared["runner"].BASE), "build556", prepared["case"]]
            try:
                house["main"](prepared["runtime"])
            except SetupReached:
                pass
            finally:
                sys.argv = old_argv
        finally:
            house["subprocess"].Popen = original
        assert len(seen) == 1, f"expected one intercepted test launch, got {len(seen)}"
        return seen[0]


def main():
    """Prepare one G1 cohort without invoking the compiled test process."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cohort", choices=("serial", "mpi2", "mpi4"), required=True)
    parser.add_argument("--temporary-workspace", action="store_true",
                        help="keep READY/host setup artifacts in a temporary folder for stub-only MPI cohorts")
    args = parser.parse_args()
    if args.temporary_workspace:
        with tempfile.TemporaryDirectory(prefix=f"b6_g1_{args.cohort}_setup_") as directory:
            prepared = prepare_g1(args.cohort, directory)
            stub = stop_at_test_popen(prepared)
            result = {"passed": True, "launched": False, "built": False, "cohort": args.cohort,
                      "temporary_host_preflight": True, "housepin_stub": stub,
                      "empty_wall_exceptions": True}
    else:
        prepared = prepare_g1(args.cohort)
        stub = stop_at_test_popen(prepared)
        result = {"passed": True, "launched": False, "built": False, "cohort": args.cohort,
                  "run_ready": str(HERE / f"validation_run_ready_{args.cohort}.json"),
                  "host_preflight": str(HERE / f"G1_{args.cohort}_host_preflight.json"),
                  "housepin_stub": stub, "empty_wall_exceptions": True}
    out = HERE / f"G1_{args.cohort}_prelaunch_stub_receipt.json"
    out.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
