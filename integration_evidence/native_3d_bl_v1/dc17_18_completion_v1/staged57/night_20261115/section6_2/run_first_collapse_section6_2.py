#!/usr/bin/env python3
"""Run the exact G2 first-collapse gate through the Section 6.2 controller."""
import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
OLD = STAGED / "p7p8_r3_20261115"
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(OLD))
from run_full_section6_2 import load_runner, self_check as full_self_check


def gate_self_check():
    """Check exact-work acceptance and rejection with synthetic rank records."""
    full_self_check()
    assess = load_runner()["assess_p6"]
    counts = (480, 473, 346, 263, 469, 464)
    work = (15_134_978, 15_134_978, 15_134_978,
            15_134_978, 15_134_978, 15_134_979)
    required = ("upper_refinements", "classification_calls", "classification_work",
                "magnitude_calls", "magnitude_work", "limit_calls", "limit_work",
                "facet_build_work", "facet_misses", "update_seconds")
    rows = {}
    for rank in range(6):
        row = dict.fromkeys(required, 0)
        row.update(rank=rank, upper_refinements=0, upper_calls=0,
                   upper_work=work[rank], classification_work=work[rank],
                   length_collapses=counts[rank])
        rows[rank] = row
    assert sum(work) == 90_809_869
    assert assess(rows, True)["passed"]
    rows[5]["upper_work"] += 1
    rows[5]["classification_work"] += 1
    assert not assess(rows, True)["passed"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", nargs="?", type=Path, default=HERE / "first_collapse")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    gate_self_check()
    if args.self_check:
        print(json.dumps({"passed": True, "launched": False,
                          "gate": "exact G2: 90809869 work and six rank counts"}))
        return
    case = args.case.resolve()
    assert HERE in case.parents and not case.exists()
    build = json.loads((HERE / "build525/build_status.json").read_text())
    assert build["state"] == "terminal" and build["exit_code"] == 0
    assert build["pins_verified"] and not build.get("changed")
    sys.argv = [str(__file__), str(case)]
    runner = load_runner(build["binary"])
    assert runner["sha"](runner["BINARY"]) == build["binary_sha256"]
    runner["main"]()


if __name__ == "__main__":
    main()
