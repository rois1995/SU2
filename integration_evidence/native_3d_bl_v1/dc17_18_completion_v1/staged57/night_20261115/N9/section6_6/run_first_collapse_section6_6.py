#!/usr/bin/env python3
"""Run the exact saved-C492 G2 prefix and compare all old integer rows with N1."""
import argparse
import hashlib
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[2]
REPO = STAGED.parents[3]
BASE = STAGED / "night_20261115/section6_2/run_first_collapse_section6_2.py"
COMPARATOR = STAGED / "night_20261115/N4/compare_integer_rows.py"
REFERENCE = STAGED / "night_20261115/section6_1/first_collapse"
spec = importlib.util.spec_from_file_location("section62_g2_66", BASE)
controller = importlib.util.module_from_spec(spec)
spec.loader.exec_module(controller)
controller.HERE = HERE
controller.STAGED = STAGED
controller.REPO = REPO
controller.OLD = STAGED / "p7p8_r3_20261115"
controller.load_runner.__globals__.update(HERE=HERE, STAGED=STAGED, REPO=REPO,
                                          OLD=controller.OLD)


def sha(path):
    """Hash one G2 provenance input."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verify_build():
    """Verify build529 and create its deterministic aliases."""
    from build529_aliases import main as expose_build
    expose_build()
    status = json.loads((HERE / "build529/build_status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0
    assert status["pins_verified"] and not status.get("changed")
    return status


def compare_g2(case):
    """Apply the existing exact G2 integer-row comparator against accepted N1."""
    result = subprocess.run([sys.executable, str(COMPARATOR), str(REFERENCE), str(case),
                             "--gate", "G2"], cwd=HERE, capture_output=True,
                            text=True, check=False)
    parsed = json.loads(result.stdout)
    receipt = {"passed": result.returncode == 0 and parsed.get("passed") is True,
               "gate": "G2", "reference": str(REFERENCE), "candidate": str(case),
               "comparator_sha256": sha(COMPARATOR), "comparison": parsed,
               "stdout": result.stdout, "stderr": result.stderr}
    (case / "g2_integer_comparison_66.json").write_text(json.dumps(receipt, indent=2) + "\n")
    if not receipt["passed"]:
        raise SystemExit(1)


def self_check():
    """Check inherited collapse invariants and compare the accepted G2 case to itself."""
    controller.gate_self_check()
    runner = controller.load_runner()
    runner.update(PURPOSE=HERE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=HERE / "build529/build_status.json",
                  SOURCE_PINS=HERE / "build529/source_pins.json",
                  SCRIPT=HERE / "run_first_collapse_section6_6.py")
    assert runner["PURPOSE"] == HERE and runner["SCRIPT"].is_file()
    assert runner["owned_active_session_ids"].__globals__["HERE"] == HERE
    result = subprocess.run([sys.executable, str(COMPARATOR), str(REFERENCE), str(REFERENCE),
                             "--gate", "G2"], cwd=HERE, capture_output=True,
                            text=True, check=False)
    assert result.returncode == 0, result.stdout + result.stderr
    parsed = json.loads(result.stdout)
    assert parsed.get("passed") is True
    return {"passed": True, "launched": False, "expected_work": 90_809_869,
            "reference": str(REFERENCE), "comparison": parsed}


def run_case(case):
    """Run one first-collapse prefix, then enforce the accepted N1 integer baseline."""
    assert HERE in case.resolve().parents and not case.exists()
    build = verify_build()
    runner = controller.load_runner(build["binary"])
    runner.update(PURPOSE=HERE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=HERE / "build529/build_status.json",
                  SOURCE_PINS=HERE / "build529/source_pins.json",
                  SCRIPT=HERE / "run_first_collapse_section6_6.py",
                  BINARY=Path(build["binary"]).resolve())
    assert runner["sha"](runner["BINARY"]) == build["binary_sha256"]
    sys.argv = [str(BASE), str(case)]
    try:
        runner["main"]()
    except SystemExit as result:
        if result.code not in (None, 0, False):
            raise
    compare_g2(case)


def main():
    """Dispatch no-launch checks or one explicitly named G2 case."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", nargs="?", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check()))
        return
    assert args.case is not None
    run_case(args.case.resolve())


if __name__ == "__main__":
    main()
