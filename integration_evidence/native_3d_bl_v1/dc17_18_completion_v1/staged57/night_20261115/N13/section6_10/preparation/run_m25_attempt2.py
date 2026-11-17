#!/usr/bin/env python3
"""Run one pinned, serial M25 case with the existing D9 resource guard."""
import argparse
import csv
import importlib.util
import json
import os
import sys
import types
from pathlib import Path
HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[2]
REPO = STAGED.parents[3]
BASE = STAGED / "run_cases.py"
BUILD = HERE / "build547"
TAG = "[.][NativeM25Split]"


def load_guard():
    sys.path.insert(0, str(STAGED / "night_20261115/N5"))
    import run_d9_units
    return run_d9_units.UnitResourceGuard


def configured():
    source = BASE.read_text()
    old = 'timeout_seconds = 2400 if case == "m6_stage" else 600'
    assert source.count(old) == 1
    source = source.replace(old, "timeout_seconds = 3600", 1)
    runtime = types.ModuleType("m25_saved492_runtime")
    runtime.__file__ = str(BASE)
    sys.path.insert(0, str(STAGED))
    exec(compile(source, str(BASE), "exec"), runtime.__dict__)
    build = json.loads((BUILD / "build_status.json").read_text())
    assert build["exit_code"] == 0 and build["pins_verified"]
    runtime.ROOT, runtime.BINARY = HERE, Path(build["test_binary"])
    runtime.CASES = {"m25_split": (1, TAG)}
    descriptor = json.loads((HERE / "descriptor.json").read_text())
    assert descriptor["test_filter"] == TAG and descriptor["ranks"] == 1
    for path, digest in descriptor["input_file_sha256"].items():
        assert runtime.sha(Path(path)) == digest, path
    original_save = runtime.save
    def save(path, value):
        if Path(path).name == "pins.json":
            value.update(descriptor["input_file_sha256"])
            for helper in [Path(__file__), HERE / "descriptor.json",
                           STAGED / "night_20261115/N5/run_d9_units.py"]:
                value[str(helper.resolve())] = runtime.sha(helper)
        return original_save(path, value)
    runtime.save = save
    return runtime, descriptor


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    runtime, descriptor = configured()
    rows = list(csv.DictReader((HERE / "inputs/m25_edges.csv").open()))
    assert len(rows) == 2408 and len(rows[0]) == 9
    if args.self_check:
        print(json.dumps({"passed": True, "launched": False, "rows": len(rows),
                          "tag": TAG, "timeout_seconds": 3600, "ranks": 1}))
        return
    for key in tuple(os.environ):
        if key.startswith(("SU2_D9_", "SU2_M25_")):
            os.environ.pop(key)
    os.environ.update(descriptor["environment"])
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    sys.argv = [str(BASE), "build547", "m25_split"]
    guard = load_guard()(runtime, HERE / "resource_handles_attempt2.json", 1)
    try:
        runtime.main(STAGED / "runtime_20261113_474/loaded_runtime_pins.json")
    except SystemExit as result:
        if result.code not in (None, 0):
            raise
    finally:
        guard.finish()


if __name__ == "__main__":
    main()
