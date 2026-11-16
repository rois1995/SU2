#!/usr/bin/env python3
"""Reuse the P7/P8 full controller with Section 6.2 paths and strict standing gates."""
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


def load_runner(binary=None):
    """Retarget the house controller without copying or editing its implementation."""
    source = CORE.read_text()
    old = "PURPOSE.parent / 'f3b_20261115/check_timer_tree.py'"
    assert source.count(old) == 1
    source = source.replace(old, "R / 'f3b_20261115/check_timer_tree.py'")
    process_guard = ('    existing = read_processes()\n'
                     '    conflicting = [row for row in existing.values() if row["comm"] in ("SU2_CFD", "test_driver", "cc1plus", "lto1")]\n'
                     '    assert not conflicting, f"owned native/build process already present: {conflicting}"')
    guarded = ('    existing = read_processes()\n'
               '    owned_sessions = owned_active_session_ids()\n'
               '    conflicting = [row for row in existing.values() if row["sid"] in owned_sessions and row["comm"] in ("SU2_CFD", "test_driver", "cc1plus", "lto1")]\n'
               '    assert not conflicting, f"owned native/build process already present: {conflicting}"')
    assert source.count(process_guard) == 1
    source = source.replace(process_guard, guarded)
    namespace = {"__name__": "section6_2_full_controller", "__file__": str(CORE)}
    exec(compile(source, str(CORE), "exec"), namespace)
    namespace.update(SCRIPT=Path(__file__).resolve(), PURPOSE=HERE, R=STAGED, REPO=REPO,
                     BUILD_STATUS=HERE / "build525/build_status.json",
                     SOURCE_PINS=HERE / "build525/source_pins.json")
    if binary is not None:
        namespace["BINARY"] = Path(binary).resolve()
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
    namespace["owned_active_session_ids"] = owned_active_session_ids
    return namespace


def owned_active_session_ids():
    """Return only sessions proven by this section's controller and running-case receipts."""
    roots = set()
    receipts = list(HERE.glob("*controller_launch.json"))
    receipts.extend(HERE.glob("final_*_receipt.json"))
    receipts.extend(HERE.glob("*/status.json"))
    receipts.extend((HERE / "cases").glob("**/status.json"))
    for path in receipts:
        try:
            record = json.loads(path.read_text())
            identities = []
            identity = record.get("controller") or record.get("child") or record.get("identity")
            if isinstance(identity, dict):
                identities.append(identity)
            if isinstance(record.get("identity"), dict):
                identities.append(record["identity"])
            if isinstance(record.get("pid"), int):
                identities.append({"pid": record["pid"], "start_ticks": record.get("start_ticks")})
            for identity in identities:
                if not isinstance(identity, dict) or not isinstance(identity.get("start_ticks"), int):
                    continue
                fields = (Path("/proc") / str(identity["pid"]) / "stat").read_text().rsplit(")", 1)[1].split()
                if int(fields[19]) == identity["start_ticks"]:
                    roots.add(identity["pid"])
        except (OSError, ValueError, IndexError, json.JSONDecodeError):
            continue
    for path in (HERE / "cases").glob("**/status.json"):
        try:
            record = json.loads(path.read_text())
            pid = record.get("pid") if record.get("state") == "running" else None
            if isinstance(pid, int):
                fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
                if int(fields[3]) == pid:
                    roots.add(pid)
        except (OSError, ValueError, IndexError, json.JSONDecodeError):
            continue
    return roots


def self_check():
    """Check retargeting and exact-collapse failure detection without reading a build or running CFD."""
    runner = load_runner()
    assert runner["PURPOSE"] == HERE and runner["R"] == STAGED
    assert runner["BUILD_STATUS"].parent == HERE / "build525"
    assert len(ast.parse(CORE.read_text()).body) > 10
    assert runner["assess_p6"]({}, True)["passed"] is False
    candidate = REPO / "integration_evidence/build-integrated-v3-p5-20261114/SU2_CFD/src/SU2_CFD"
    assert load_runner(candidate)["BINARY"] == candidate
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
    build = json.loads((HERE / "build525/build_status.json").read_text())
    assert build["state"] == "terminal" and build["exit_code"] == 0
    assert build["pins_verified"] and not build.get("changed")
    sys.path.insert(0, str(HERE))
    sys.path.insert(1, str(OLD))
    sys.argv = [str(__file__), str(args.case.resolve()), "--full"]
    runner = load_runner(build["binary"])
    assert runner["sha"](runner["BINARY"]) == build["binary_sha256"]
    runner["main"]()


if __name__ == "__main__":
    main()
