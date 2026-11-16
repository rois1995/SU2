#!/usr/bin/env python3
"""Complete strict postprocessing after the inherited controller exits normally."""
import hashlib
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
GROUP = HERE.parent
VALIDATION = GROUP / "validation"
CASE = VALIDATION / "full_workflow"


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    """Verify original receipts, all timers, and all six exact reference payloads."""
    spec = importlib.util.spec_from_file_location("post534_original", VALIDATION / "validate_n8.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.prepare_build()
    comparator = module.load("post534_payloads", module.SECTION2 / "compare_reference_payloads.py")
    comparator.workflow_passed(CASE)
    timer_report = module.full_wall_timer_report(CASE, require_wall=True)
    (CASE / "n8_wall_timer_report.json").write_text(json.dumps(timer_report, indent=2) + "\n")
    reference = json.loads(module.PAYLOAD_REFERENCE.read_text())
    actual = comparator.payload_hashes(CASE)
    mismatches = comparator.compare_hashes(actual, reference["hashes"])
    receipt = {"case": str(CASE), "reference": str(module.PAYLOAD_REFERENCE),
               "reference_sha256": sha(module.PAYLOAD_REFERENCE), "hashes": actual,
               "mismatches": mismatches, "passed": not mismatches,
               "workflow_gates_passed": True, "timer_and_wall_report": timer_report,
               "postprocessor_sha256": sha(__file__),
               "frozen_parent_helper_sha256": sha(VALIDATION / "validate_n8.py"),
               "workflow_receipt_sha256": sha(CASE / "full_workflow_receipt.json"),
               "rank_stdout_sha256": {str(p.relative_to(CASE)): sha(p)
                   for p in sorted(CASE.glob("rank_output/*/rank.*/stdout"))}}
    (CASE / "n8_payload_identity.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"passed": receipt["passed"], "mismatches": mismatches,
                      "timer_summaries": timer_report["timer_summary_count"],
                      "law_rows": timer_report["wall_law_row_count"]}))
    assert receipt["passed"], "six payloads differ from accepted references"


if __name__ == "__main__":
    main()
