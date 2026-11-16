#!/usr/bin/env python3
"""Run one serial cheap-BL guard, then apply the Section 6.6 G3 identity rule."""
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
REFERENCE = STAGED / "night_20261115/section6_1/cases/section6_1_bl_20261116T082025_3350201"
COMPARATOR = STAGED / "night_20261115/N4/compare_integer_rows.py"
IDENTITY = HERE / "drafts/check_g3_identity_66.py"
spec = importlib.util.spec_from_file_location("g3_identity_66", IDENTITY)
identity = importlib.util.module_from_spec(spec)
spec.loader.exec_module(identity)


def sha(path):
    """Hash one pinned input file."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def report_old_integers(reference, candidate):
    """Preserve the prior all-integer comparison as a nonblocking diagnostic."""
    command = [sys.executable, str(COMPARATOR), str(reference), str(candidate), "--gate", "all"]
    result = subprocess.run(command, cwd=HERE, capture_output=True, text=True, check=False)
    payload = json.loads(result.stdout)
    return {"returncode": result.returncode, "reference": str(reference),
            "candidate": str(candidate), "comparator_sha256": sha(COMPARATOR),
            "stdout": result.stdout, "stderr": result.stderr,
            "comparison": payload, "passed": result.returncode == 0 and payload.get("passed") is True}


def self_check():
    """Check both gates against the preserved N9 run without launching a case."""
    identity_result = identity.self_check()
    assert identity_result["passed"]
    return {"passed": True, "launched": False,
            "identity_case": identity_result.get("candidate"),
            "case_prefix": "N9_66_bl_", "physical_guard_cap_bytes": 19_999_490_048}


def run_case():
    """Run the inherited physical gate once, then preserve comparisons and identity result."""
    summary = HERE / "g3_gate_66.json"
    assert not summary.exists(), "refusing to overwrite a completed Section 6.6 G3"
    cases_dir = HERE / "cases"
    before = set(cases_dir.glob("N9_66_bl_*"))
    child_log = HERE / "g3_child_controller_66.log"
    with child_log.open("x") as output:
        child = subprocess.run([sys.executable, str(HERE / "run_remaining_checks.py"), "bl"],
                               cwd=HERE, stdout=output, stderr=subprocess.STDOUT, check=False)
    created = set(cases_dir.glob("N9_66_bl_*")) - before
    assert len(created) == 1, f"expected one physical-guard case; inspect {child_log}"
    case = created.pop()
    cheap = json.loads((case / "cheap_bl_receipt.json").read_text())
    physical_ok = child.returncode == 0 and cheap.get("passed") is True
    base = {"case": str(case), "child_returncode": child.returncode,
            "cheap_bl_passed": cheap.get("passed"),
            "cheap_bl_receipt_sha256": sha(case / "cheap_bl_receipt.json"),
            "reference": str(REFERENCE), "physical_gate_before_identity": True}
    if not physical_ok:
        result = dict(base, passed=False, first_failed_gate="cheap_bl_physical_guard")
        summary.write_text(json.dumps(result, indent=2) + "\n")
        raise SystemExit(1)
    old_integers = report_old_integers(REFERENCE, case)
    (case / "g3_integer_comparison.json").write_text(json.dumps(old_integers, indent=2) + "\n")
    decision = identity.evaluate(REFERENCE, case)
    (case / "g3_identity_66.json").write_text(json.dumps(decision, indent=2) + "\n")
    result = dict(base, passed=decision["passed"], first_failed_gate=None if decision["passed"] else "g3_identity_66",
                  old_integer_comparison_report_only=old_integers,
                  identity_result="g3_identity_66.json",
                  quoted_wall_reason_diagnostics=decision["wall_reason_diagnostics"])
    summary.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result), flush=True)
    if not result["passed"]:
        raise SystemExit(1)


def main():
    """Dispatch the no-launch self-check or the single G3 physical guard."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check()))
    else:
        run_case()


if __name__ == "__main__":
    main()
