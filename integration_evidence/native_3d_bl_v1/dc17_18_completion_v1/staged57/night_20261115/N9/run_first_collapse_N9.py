#!/usr/bin/env python3
"""Run the bounded exact G2 prefix and compare it with accepted N1 rows."""
import argparse
import hashlib
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
REPO = STAGED.parents[3]
BASE = STAGED / "night_20261115" / "section6_2" / "run_first_collapse_section6_2.py"
COMPARATOR = STAGED / "night_20261115" / "N4" / "compare_integer_rows.py"
REFERENCE = STAGED / "night_20261115" / "section6_1" / "first_collapse"
spec = importlib.util.spec_from_file_location("section62_g2", BASE)
controller = importlib.util.module_from_spec(spec)
spec.loader.exec_module(controller)
controller.HERE = HERE
controller.STAGED = STAGED
controller.REPO = REPO
controller.OLD = STAGED / "p7p8_r3_20261115"
# The reused loader owns its functions' real globals; retarget those, not only a copied namespace.
controller.load_runner.__globals__.update(HERE=HERE, STAGED=STAGED, REPO=REPO)


def verify_build():
    """Require the immutable candidate build and materialize its aliases."""
    from build527_aliases import main as expose_build
    expose_build()
    build = json.loads((HERE / "build527/build_status.json").read_text())
    assert build["state"] == "terminal" and build["exit_code"] == 0
    assert build["pins_verified"] and not build.get("changed")
    return build


def compare_g2(case):
    """Apply the existing integer comparator at its exact G2 cutoff."""
    result = subprocess.run([sys.executable, str(COMPARATOR), str(REFERENCE), str(case),
                             "--gate", "G2"], cwd=HERE, check=False,
                            capture_output=True, text=True)
    comparison = json.loads(result.stdout)
    receipt = {"passed": result.returncode == 0 and comparison["passed"], "gate": "G2", "reference": str(REFERENCE),
               "candidate": str(case), "comparator": str(COMPARATOR),
               "comparator_sha256": hashlib.file_digest(COMPARATOR.open("rb"), "sha256").hexdigest(),
               "result": comparison}
    (case / "g2_integer_comparison.json").write_text(json.dumps(receipt, indent=2) + "\n")
    if not receipt["passed"]:
        print(json.dumps(receipt), flush=True)
        raise SystemExit(1)


def self_check():
    """Check exact G2 behavior and parse the accepted N1 rank streams read-only."""
    controller.gate_self_check()
    runner = controller.load_runner()
    runner.update(PURPOSE=HERE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=HERE / "build527/build_status.json",
                  SOURCE_PINS=HERE / "build527/source_pins.json",
                  SCRIPT=HERE / "run_first_collapse_N9.py")
    assert runner["PURPOSE"] == HERE and runner["SCRIPT"].is_file()
    assert runner["owned_active_session_ids"].__globals__["HERE"] == HERE
    assert all((HERE / name).is_file() for name in (
        "check_connected_events.py", "check_complete_events.py", "check_timer_tree.py"))
    check = subprocess.run([sys.executable, str(COMPARATOR), str(REFERENCE), str(REFERENCE),
                            "--gate", "G2"], cwd=HERE, check=False,
                           capture_output=True, text=True)
    assert check.returncode == 0, check.stdout + check.stderr
    return {"passed": True, "launched": False, "reference": str(REFERENCE),
            "comparator_stdout": check.stdout.strip()}


def main():
    """Run one G2 controller case, then enforce unchanged accepted integer rows."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", nargs="?", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check()))
        return
    assert args.case is not None
    case = args.case.resolve()
    assert HERE in case.parents and not case.exists()
    build = verify_build()
    runner = controller.load_runner(build["binary"])
    runner.update(PURPOSE=HERE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=HERE / "build527/build_status.json",
                  SOURCE_PINS=HERE / "build527/source_pins.json",
                  SCRIPT=HERE / "run_first_collapse_N9.py",
                  BINARY=Path(build["binary"]).resolve())
    assert runner["sha"](runner["BINARY"]) == build["binary_sha256"]
    sys.argv = [str(BASE), str(case)]
    try:
        runner["main"]()
    except SystemExit as exit_result:
        if exit_result.code not in (None, 0, False):
            raise
    compare_g2(case)


if __name__ == "__main__":
    main()
