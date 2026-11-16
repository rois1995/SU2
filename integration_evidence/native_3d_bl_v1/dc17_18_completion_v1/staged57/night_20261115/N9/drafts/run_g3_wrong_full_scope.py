#!/usr/bin/env python3
"""Run one N9 full C492 workflow after the unchanged cheap-BL standing guard."""
import argparse
import hashlib
import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
COMPARATOR = HERE.parent / "N4/compare_integer_rows.py"
REFERENCE = HERE.parent / "section6_1/cases/section6_1_bl_20261116T082025_3350201"
G2_HELPER = HERE / "run_first_collapse_N9.py"
BL_HELPER = HERE / "run_remaining_checks.py"
spec = importlib.util.spec_from_file_location("n9_g2_setup", G2_HELPER)
g2 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g2)


def sha(path):
    """Hash one frozen check input without reading a mesh into memory."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def save(path, value):
    """Preserve a report before any failure is returned to the caller."""
    path.write_text(json.dumps(value, indent=2) + "\n")


def accepted_reference():
    """Require the recorded accepted N1 full-workflow case before comparison."""
    status = json.loads((REFERENCE / "status.json").read_text())
    workflow = json.loads((REFERENCE / "full_workflow_receipt.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("all_owned_handles_terminal") is True
    assert status.get("full_workflow_accepted") is True
    assert workflow.get("workflow_accepted") is True and len(workflow.get("events", [])) == 2
    assert workflow.get("P7_P8_events_checked") == [1, 2]
    assert workflow.get("F3b_timer_events_checked") == [1, 2]
    return status, workflow


def compare(case, output):
    """Apply the existing ALL-old-integer gate with its original measurement exclusions."""
    command = [sys.executable, str(COMPARATOR), str(REFERENCE), str(case), "--gate", "all"]
    result = subprocess.run(command, capture_output=True, text=True)
    receipt = {"returncode": result.returncode, "reference": str(REFERENCE), "candidate": str(case),
               "comparator_sha256": sha(COMPARATOR), "stdout": result.stdout, "stderr": result.stderr}
    receipt["comparison"] = json.loads(result.stdout)
    receipt["passed"] = result.returncode == 0 and receipt["comparison"]["passed"]
    save(output, receipt)
    return receipt


def observation_rows(case):
    """Read additive wall/D9 records; they cannot turn a failed result gate into a pass."""
    rows = []
    paths = sorted(case.glob("rank_output/*/rank.*/stdout"))
    if not paths and (case / "run.log").is_file():
        paths = [case / "run.log"]
    assert paths, f"no native rank logs found under {case}"
    hashes = {}
    for path in paths:
        hashes[str(path)] = sha(path)
        for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            if "NATIVE3D_WALL_" not in line and not ("D9_QUERY " in line and "phase=surface" in line):
                continue
            marker = "D9_QUERY" if "D9_QUERY " in line else line[line.index("NATIVE3D_WALL_"):].split()[0]
            body = line[line.index(marker) + len(marker):]
            fields = dict(word.split("=", 1) for word in body.split() if "=" in word)
            rows.append({"marker": marker, "fields": fields, "source": str(path), "line": number})
    save(case / "wall_observations.json", {"rows": rows, "source_sha256": hashes})
    return rows


def run_cheap_bl_guard():
    """Run the existing saved-surface physical gate once and retain its case receipt."""
    prior = set((HERE / "cases").glob("N9_bl_*"))
    result = subprocess.run([sys.executable, str(BL_HELPER), "bl"], cwd=HERE,
                            capture_output=True, text=True)
    created = set((HERE / "cases").glob("N9_bl_*")) - prior
    assert len(created) == 1, f"cheap-BL setup did not create one case: {sorted(map(str, created))}"
    case = created.pop()
    receipt = json.loads((case / "cheap_bl_receipt.json").read_text())
    summary = {"passed": result.returncode == 0 and receipt.get("passed") is True,
               "case": str(case), "returncode": result.returncode,
               "receipt_sha256": sha(case / "cheap_bl_receipt.json"),
               "stdout": result.stdout, "stderr": result.stderr}
    save(HERE / "cheap_bl_standing_guard.json", summary)
    assert summary["passed"], "cheap-BL standing physical gate failed"
    return summary


def run_full_candidate(case):
    """Run the existing Section6.2 full-workflow controller with build527 aliases."""
    build = g2.verify_build()
    runner = g2.controller.load_runner(build["binary"])
    runner.update(PURPOSE=HERE, R=g2.STAGED, REPO=g2.REPO,
                  BUILD_STATUS=HERE / "build527/build_status.json",
                  SOURCE_PINS=HERE / "build527/source_pins.json",
                  SCRIPT=Path(__file__).resolve(), BINARY=Path(build["binary"]).resolve())
    sys.argv = [str(g2.BASE), str(case), "--full"]
    try:
        runner["main"]()
    except SystemExit as result:
        if result.code not in (None, 0, False):
            raise


def main():
    """Execute the cheap-BL physical gate first, then its unchanged integer identity gate."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--case", type=Path, default=HERE / "g3_C492_build527")
    args = parser.parse_args()
    if args.self_check:
        result = compare(REFERENCE, HERE / "g3_comparator_selfcheck.json")
        assert result["passed"]
        accepted_reference()
        assert G2_HELPER.is_file() and BL_HELPER.is_file()
        print(json.dumps({"passed": True, "launched": False}))
        return
    accepted_reference()
    summary = HERE / "g3_gate.json"
    case = args.case.resolve()
    assert HERE in case.parents and not summary.exists() and not case.exists()
    cheap = run_cheap_bl_guard()
    receipt = {"candidate": str(case), "cheap_bl_passed": cheap["passed"],
               "cheap_bl_receipt_sha256": cheap["receipt_sha256"], "reference": str(REFERENCE)}
    run_full_candidate(case)
    rows = observation_rows(case)
    result = compare(case, case / "g3_integer_comparison.json")
    save(summary, dict(receipt, passed=result["passed"], observation_rows=len(rows),
                       first_failed_gate=None if result["passed"] else "G3_all_integer_rows"))
    print(json.dumps(json.loads(summary.read_text())), flush=True)
    if not result["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
