#!/usr/bin/env python3
"""Reuse the P7/P8 full controller with section 6.1 paths and unchanged standing gates."""
import argparse
import ast
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
REPO = STAGED.parents[3]
OLD = STAGED / "p7p8_r3_20261115"
CORE = OLD / "run_full_workflow.py"


def load_runner():
    """Retarget the house controller without copying or editing its implementation."""
    source = CORE.read_text()
    old = "PURPOSE.parent / 'f3b_20261115/check_timer_tree.py'"
    assert source.count(old) == 1
    source = source.replace(old, "R / 'f3b_20261115/check_timer_tree.py'")
    namespace = {"__name__": "section6_1_full_controller", "__file__": str(CORE)}
    exec(compile(source, str(CORE), "exec"), namespace)
    namespace.update(SCRIPT=Path(__file__).resolve(), PURPOSE=HERE, R=STAGED, REPO=REPO,
                     BUILD_STATUS=HERE / "build522/build_status.json",
                     SOURCE_PINS=HERE / "build522/source_pins.json")
    original = namespace["assess_p6"]

    def exact_collapse(rows, collapse):
        """Enforce the current exact collapse gate in addition to the older diagnostics."""
        result = original(rows, collapse)
        if collapse:
            result["work_equals_P6c"] = result["upper_work_sum"] == 90809869
            result["passed"] = (result["passed"] and result["work_equals_P6c"]
                                and result["count_prediction_matches"])
            result["count_difference_is_not_failure"] = False
        return result

    namespace["assess_p6"] = exact_collapse
    return namespace


def self_check():
    """Check retargeting and exact-collapse failure detection without reading a build or running CFD."""
    runner = load_runner()
    assert runner["PURPOSE"] == HERE and runner["R"] == STAGED
    assert runner["BUILD_STATUS"].parent == HERE / "build522"
    assert len(ast.parse(CORE.read_text()).body) > 10
    assert runner["assess_p6"]({}, True)["passed"] is False
    assert runner["BINARY"] == REPO / "integration_evidence/build-integrated-v3-p5-20261114/SU2_CFD/src/SU2_CFD"
    rows = {}
    for rank, count in enumerate((480, 473, 346, 263, 469, 464)):
        work = 90809869 if rank == 0 else 0
        rows[rank] = dict(upper_refinements=0, classification_calls=1, classification_work=work,
                          magnitude_calls=0, magnitude_work=0, limit_calls=0, limit_work=0,
                          facet_build_work=0, facet_misses=0, update_seconds=0,
                          upper_work=work, upper_calls=1, length_collapses=count)
    assert runner["assess_p6"](rows, True)["passed"]
    rows[0]["length_collapses"] += 1
    assert not runner["assess_p6"](rows, True)["passed"]
    rows[0]["length_collapses"] -= 1
    rows[0]["upper_work"] -= 1
    rows[0]["classification_work"] -= 1
    assert not runner["assess_p6"](rows, True)["passed"]


def main():
    """Launch one fresh full workflow; output-reference qualification follows separately."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", nargs="?", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    self_check()
    if args.self_check:
        print(json.dumps({"passed": True, "launched": False}))
        return
    assert args.case and HERE in args.case.resolve().parents
    sys.path.insert(0, str(HERE))
    sys.path.insert(1, str(OLD))
    sys.argv = [str(__file__), str(args.case.resolve()), "--full"]
    load_runner()["main"]()


if __name__ == "__main__":
    main()
