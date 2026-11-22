"""Run the FP2-C1 fixtures, S0 regression fixtures, and G1."""

from argparse import ArgumentParser
from pathlib import Path
import os
import sys
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
sys.path[:0] = [str(NIGHT / name) for name in ("FP1", "FX2", "FP2_B")]
import run_fx2_fixtures as fx2
import run_scoped_fx2 as scope
from run_g1 import cut, run_case, sha, verify_build

ORDER = HERE / "inputs/FAST_PATH_STAGE2_C1.md"
ORDER_SHA = "6029450b746aed4493a4790fe5cc6504835387b70c7fc658e3bd3279fd96f35d"


def wall_fixtures(build):
    status = cut.read(build / "build_status.json")
    manifest = build / "source_pins.json"
    binary, _ = verify_build(status, manifest)
    case = HERE / "fixtures" / build.name / "wall"
    if (case / "test_wait.json").exists():
        wait = cut.read(case / "test_wait.json")
    else:
        assert not (case / "test_launch.json").exists(), case
        wait = run_case(case, binary, 1, "[.][NativeFP2C1]", 5400, cwd=case)
    verify_build(status, manifest)
    assert wait["terminal"] and cut.terminal(wait["controller"]) and wait["returncode"] == 0, case
    log = case / "test_outer.log"
    assert "in 5 test cases" in log.read_text(), log
    return case


def mobility_fixtures(build):
    with patch.object(fx2, "HERE", HERE):
        data = {
            "F1-F4": fx2.fixture(build, "F1-F4", "[.][NativeFX2Memo]", {}),
            "F5": fx2.fixture(build, "F5", "[.][NativeFastPathStage2Bounds]",
                             {"SU2_FP2_OUT": fx2.inputs()["SU2_FP2_OUT"]}),
        }
        baseline = NIGHT / "FX2/fixtures/build644"
        reference = {name: cut.read(baseline / name / "rows.json") for name in data}
        proof = fx2.compare(build, data, reference)
    assert proof["passed"], proof["differences"]
    return proof["constructor_saving_seconds"]


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    build = args.build.resolve()
    fx2.audit()
    for key in tuple(os.environ):
        if key.startswith(("SU2_NATIVE_", "SU2_FP1_", "SU2_FP2_")):
            del os.environ[key]
    wall = wall_fixtures(build)
    print(f"Five wall fixtures passed. {wall}", flush=True)
    saving = mobility_fixtures(build)
    print(f"FX2 F1–F5 matched. B1 constructor savings {saving:.3f} s against the original F0.", flush=True)
    with patch.multiple(scope, HERE=HERE, ORDER=ORDER, ORDER_SHA=ORDER_SHA,
                        DRIVER=Path(__file__).resolve(), SOURCE_CHECK=HERE / "source_check.md"):
        receipt = scope.fixtures(build)
    assert receipt["passed"] and receipt["G1"] == [375, 114, 228], receipt
    print(receipt, flush=True)


if __name__ == "__main__":
    main()
