#!/usr/bin/env python3
"""Validate the sealed M25 CSV and generate its requested tables without rerunning the test."""
import argparse
import collections
import decimal
import hashlib
import importlib.util
import json
from pathlib import Path
HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.file_digest(Path(path).open("rb"), "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", type=Path)
    args = parser.parse_args()
    case = args.case.resolve(strict=True)
    assert HERE / "cases" in case.parents
    status = json.loads((case / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    pins = json.loads((case / "pins.json").read_text())
    assert all(sha(path) == digest for path, digest in pins.items())
    decimal.getcontext().prec = 80
    spec = importlib.util.spec_from_file_location("m25_report", HERE / "aggregate_m25.py")
    report = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(report)
    rows = report.read_rows(case / "m25_split_outcomes.csv", HERE / "inputs/m25_edges.csv")
    exact_median = report.median
    def display_median(values):
        value = exact_median(values)
        return "n/a" if value is None else format(value, ".6g")
    assert display_median([decimal.Decimal(1), decimal.Decimal(3)]) == "2"
    report.median = display_median
    report.write_report(rows, case / "M25_table.md")
    summary = {"passed": True, "postprocessing_only": True, "numeric_decimal_precision": 80,
               "rows": len(rows), "attempt_status": dict(collections.Counter(row["attempt_status"] for row in rows)),
               "selfchecked_rows": sum(row["selfcheck"] == "1" for row in rows),
               "improving_complete_predictions": sum(report.improves(row) for row in rows),
               "supplemental_bound_rows": sum(row.get("supplemental_bounds") == "1" for row in rows),
               "supplemental_query_refusals": dict(collections.Counter(row.get("bounds_error") for row in rows if row.get("bounds_error"))),
               "prediction_evaluated_face_counts": dict(collections.Counter(row["evaluated_faces"] for row in rows)),
               "unavailable_parent_bounds": sum(row[f"parent{face}_valid"] != "1" for row in rows for face in range(2)),
               "unavailable_child_bounds": sum(row[f"child{face}{child}_valid"] != "1" for row in rows for face in range(2) for child in range(2)),
               "all_case_pins_unchanged": True, "pin_count": len(pins),
               "artifacts": {name: sha(case / name) for name in ["m25_split_outcomes.csv", "M25_table.md", "m25_fixture_summary.json"]},
               "helpers": {str(path): sha(path) for path in [Path(__file__), HERE / "aggregate_m25.py"]}}
    assert summary["selfchecked_rows"] == len(rows)
    (case / "M25_postprocessing_receipt.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
