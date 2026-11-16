#!/usr/bin/env python3
"""Run one N9 C492 saved-surface case and compare its unchanged integer rows."""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
COMPARATOR = HERE.parent / "N4/compare_integer_rows.py"
REFERENCE = HERE.parent / "section6_1/cases/section6_1_bl_20261116T082025_3350201"


def sha(path):
    """Hash one saved-surface check input."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def save(path, value):
    """Preserve a gate receipt before returning failure."""
    path.write_text(json.dumps(value, indent=2) + "\n")


def compare(case, output):
    """Apply the existing comparator to all unchanged integer rows."""
    command = [sys.executable, str(COMPARATOR), str(REFERENCE), str(case), "--gate", "all"]
    result = subprocess.run(command, capture_output=True, text=True)
    receipt = {"returncode": result.returncode, "reference": str(REFERENCE),
               "candidate": str(case), "comparator_sha256": sha(COMPARATOR),
               "stdout": result.stdout, "stderr": result.stderr}
    receipt["comparison"] = json.loads(result.stdout)
    receipt["passed"] = result.returncode == 0 and receipt["comparison"]["passed"]
    save(output, receipt)
    return receipt


def observation_rows(case):
    """Export additive wall/D9 query rows from the saved-surface rank log."""
    rows = []
    path = case / "run.log"
    for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
        if "NATIVE3D_WALL_" not in line and not ("D9_QUERY " in line and "phase=surface" in line):
            continue
        marker = "D9_QUERY" if "D9_QUERY " in line else line[line.index("NATIVE3D_WALL_"):].split()[0]
        body = line[line.index(marker) + len(marker):]
        fields = dict(word.split("=", 1) for word in body.split() if "=" in word)
        rows.append({"marker": marker, "fields": fields, "source": str(path), "line": number})
    save(case / "wall_observations.json", {"rows": rows, "source_sha256": sha(path)})
    return rows


def main():
    """Run the existing cheap-BL physical guard and then exact integer comparison."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        result = compare(REFERENCE, HERE / "g3_comparator_selfcheck.json")
        assert result["passed"]
        print(json.dumps({"passed": True, "launched": False}))
        return
    summary = HERE / "g3_gate.json"
    assert not summary.exists(), "refusing to repeat or overwrite a completed G3"
    before = set((HERE / "cases").glob("N9_bl_*"))
    child_log = HERE / "g3_child_controller.log"
    with child_log.open("x") as output:
        child = subprocess.run([sys.executable, str(HERE / "run_remaining_checks.py"), "bl"],
                               stdout=output, stderr=subprocess.STDOUT)
    cases = set((HERE / "cases").glob("N9_bl_*")) - before
    assert len(cases) == 1, f"case setup failed; inspect {child_log}: {cases}"
    case = cases.pop()
    cheap = json.loads((case / "cheap_bl_receipt.json").read_text())
    receipt = {"case": str(case), "child_returncode": child.returncode,
               "cheap_bl_passed": cheap["passed"],
               "cheap_bl_receipt_sha256": sha(case / "cheap_bl_receipt.json"),
               "reference": str(REFERENCE)}
    observation_rows(case)
    if child.returncode or not cheap["passed"]:
        save(summary, dict(receipt, passed=False, first_failed_gate="cheap_BL"))
        raise SystemExit(1)
    result = compare(case, case / "g3_integer_comparison.json")
    save(summary, dict(receipt, passed=result["passed"],
                       first_failed_gate=None if result["passed"] else "G3_integers"))
    print(json.dumps(json.loads(summary.read_text())), flush=True)
    if not result["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
