#!/usr/bin/env python3
"""Compare A1 edge-law CSVs by ordered edge row and the section 6.8 tolerances."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path

STAGED = Path(__file__).resolve().parents[5]
NIGHT = STAGED / "night_20261115"
N5 = NIGHT / "N5"
A1 = NIGHT / "N8/A1"
EXACT_FIELDS = ("tau", "need_threshold", "need", "supported_sides_a", "supported_sides_b",
                "unsupported_sides_a", "unsupported_sides_b")
REQUIRED_FIELDS = ("edge_a", "edge_b", "score_max", "tau", "need_threshold", "need",
                   "limiting_side_a", "limiting_side_b", "supported_sides_a", "supported_sides_b",
                   "unsupported_sides_a", "unsupported_sides_b")
EXPECTED_NEEDS = {"euler293": 7521, "saved492": 6759, "euler_input_decode12": 4368}


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def read_rows(path):
    with Path(path).open(newline="") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != REQUIRED_FIELDS:
            raise ValueError(f"unexpected CSV header in {path}: {reader.fieldnames}")
        return list(reader)


def integer_fields(m1):
    return {key for key, value in m1.items() if type(value) is int}


def read_summary(path):
    data = json.loads(Path(path).read_text())
    if data.get("schema") != "d9_geometry_measurement_v1":
        raise ValueError(f"unexpected summary schema: {path}")
    return data["M1"]


def first_m1_difference(reference, candidate, expected_need):
    old, new = read_summary(reference), read_summary(candidate)
    old_ints, new_ints = integer_fields(old), integer_fields(new)
    if old_ints != new_ints:
        return {"field": "integer_field_set", "reference": sorted(old_ints), "candidate": sorted(new_ints)}
    for key in sorted(old_ints):
        if old[key] != new[key]:
            return {"field": key, "reference": old[key], "candidate": new[key]}
    for key in ("tau", "edge_need_threshold", "complete_edge_coverage"):
        if old.get(key) != new.get(key):
            return {"field": key, "reference": old.get(key), "candidate": new.get(key)}
    if old.get("geometry_need_edges") != expected_need:
        return {"field": "reference_geometry_need_edges_expected", "reference": old.get("geometry_need_edges"),
                "expected": expected_need}
    if new.get("geometry_need_edges") != expected_need:
        return {"field": "candidate_geometry_need_edges_expected", "candidate": new.get("geometry_need_edges"),
                "expected": expected_need}
    return None


def compare_rows(reference_csv, candidate_csv, reference_summary, candidate_summary, expected_need):
    ref_rows, new_rows = read_rows(reference_csv), read_rows(candidate_csv)
    limits = {"limiting_side_a": 0, "limiting_side_b": 0, "either_side": 0}
    result = {"pass": True, "rows": len(ref_rows), "candidate_rows": len(new_rows),
              "expected_need_edges": expected_need, "limiting_side_changes": limits,
              "largest_relative_score_difference": 0.0, "first_difference": None,
              "reference_csv_sha256": sha(reference_csv), "candidate_csv_sha256": sha(candidate_csv)}
    def fail(detail):
        if result["pass"]:
            result["pass"] = False
            result["first_difference"] = detail
    if len(ref_rows) != len(new_rows):
        fail({"csv_line": min(len(ref_rows), len(new_rows)) + 2, "field": "row_count",
              "reference": len(ref_rows), "candidate": len(new_rows)})
    for index, (old, new) in enumerate(zip(ref_rows, new_rows), start=2):
        edge_old, edge_new = (old["edge_a"], old["edge_b"]), (new["edge_a"], new["edge_b"])
        if edge_old != edge_new:
            fail({"csv_line": index, "field": "edge_order", "reference": edge_old, "candidate": edge_new})
            continue
        for field in EXACT_FIELDS:
            if old[field] != new[field]:
                fail({"csv_line": index, "edge": edge_old, "field": field,
                      "reference": old[field], "candidate": new[field]})
                break
        try:
            ref_score, new_score = float(old["score_max"]), float(new["score_max"])
            threshold = float(old["need_threshold"])
            if not all(math.isfinite(value) for value in (ref_score, new_score, threshold)):
                raise ValueError("nonfinite score or threshold")
        except ValueError as error:
            fail({"csv_line": index, "edge": edge_old, "field": "score_max", "error": str(error)})
            continue
        difference = abs(new_score - ref_score)
        tolerance = 1e-6 * max(ref_score, threshold)
        denominator = abs(ref_score) if ref_score else threshold
        relative = difference / denominator if denominator else (0.0 if difference == 0 else math.inf)
        result["largest_relative_score_difference"] = max(
            result["largest_relative_score_difference"], relative)
        if difference > tolerance:
            fail({"csv_line": index, "edge": edge_old, "field": "score_max",
                  "reference": old["score_max"], "candidate": new["score_max"],
                  "absolute_difference": difference, "tolerance": tolerance})
        side_a = old["limiting_side_a"] != new["limiting_side_a"]
        side_b = old["limiting_side_b"] != new["limiting_side_b"]
        limits["limiting_side_a"] += side_a
        limits["limiting_side_b"] += side_b
        limits["either_side"] += side_a or side_b
    m1_diff = first_m1_difference(reference_summary, candidate_summary, expected_need)
    result["m1_integer_fields"] = sorted(integer_fields(read_summary(reference_summary)))
    result["m1_first_difference"] = m1_diff
    if m1_diff:
        fail({"field": "M1", **m1_diff})
    return result


def a1_euler_paths():
    failure = json.loads((A1 / "first_result_failure.json").read_text())
    candidate = Path(failure["candidate_case"])
    reference = Path(failure["reference_case"])
    return (reference / "d9_geometry_edges.csv", candidate / "d9_geometry_edges.csv",
            reference / "d9_geometry_summary.json", candidate / "d9_geometry_summary.json")


def self_check():
    def row(score, need="1", a="4", b="5"):
        return {"edge_a": "10", "edge_b": "20", "score_max": str(score), "tau": "0.0001",
                "need_threshold": "0.000075", "need": need, "limiting_side_a": a,
                "limiting_side_b": b, "supported_sides_a": "2", "supported_sides_b": "2",
                "unsupported_sides_a": "0", "unsupported_sides_b": "0"}
    def summary(need=7521):
        return {"tau": 0.0001, "edge_need_threshold": 0.000075, "geometry_need_edges": need,
                "unique_selected_edges": 82559, "need_vertices": 7127,
                "edges_with_valid_support_at_both_endpoints": 82559,
                "first_pass_midpoint_candidates": need, "unvisited_edges_at_deadline": 0,
                "unsupported_endpoint_side_evaluations": 0, "complete_edge_coverage": True}
    old = [row("0.0002"), dict(row("0.0003"), edge_a="11")]
    okay = [row("0.00020000001", a="9"), dict(row("0.0003"), edge_a="11")]
    assert compare_data(old, okay, summary(), summary(), 7521)["pass"]
    bad_score = [row("0.0002000003"), old[1]]
    assert not compare_data(old, bad_score, summary(), summary(), 7521)["pass"]
    bad_need = [row("0.0002", need="0"), old[1]]
    assert not compare_data(old, bad_need, summary(), summary(), 7521)["pass"]
    reordered = [old[1], old[0]]
    assert not compare_data(old, reordered, summary(), summary(), 7521)["pass"]
    assert not compare_data(old, okay, summary(), summary(7522), 7521)["pass"]
    ref_csv, cand_csv, ref_summary, cand_summary = a1_euler_paths()
    actual = compare_rows(ref_csv, cand_csv, ref_summary, cand_summary, EXPECTED_NEEDS["euler293"])
    assert actual["pass"], actual["first_difference"]
    assert actual["rows"] == actual["candidate_rows"] == 82559
    assert actual["largest_relative_score_difference"] == 3.959977415686591e-08
    assert actual["limiting_side_changes"] == {"limiting_side_a": 298, "limiting_side_b": 296,
                                                "either_side": 592}
    return {"passed": True, "actual_prior_a1_euler293_passes_new_rule": True,
            "rows": actual["rows"], "M1_need_edges": EXPECTED_NEEDS["euler293"],
            "M1_integer_fields": actual["m1_integer_fields"],
            "reference_csv_sha256": actual["reference_csv_sha256"],
            "candidate_csv_sha256": actual["candidate_csv_sha256"],
            "limiting_side_changes": actual["limiting_side_changes"],
            "largest_relative_score_difference": actual["largest_relative_score_difference"],
            "launched_numerical_job": False}


def compare_data(reference_rows, candidate_rows, reference_m1, candidate_m1, expected_need):
    import tempfile
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        fields = ",".join(REQUIRED_FIELDS) + "\n"
        for name, rows in (("ref", reference_rows), ("cand", candidate_rows)):
            with (root / f"{name}.csv").open("w") as stream:
                stream.write(fields)
                writer = csv.DictWriter(stream, fieldnames=REQUIRED_FIELDS)
                writer.writerows(rows)
        for name, m1 in (("ref", reference_m1), ("cand", candidate_m1)):
            (root / f"{name}.json").write_text(json.dumps({"schema": "d9_geometry_measurement_v1", "M1": m1}))
        return compare_rows(root / "ref.csv", root / "cand.csv", root / "ref.json", root / "cand.json",
                            expected_need)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--reference-csv", type=Path)
    parser.add_argument("--candidate-csv", type=Path)
    parser.add_argument("--reference-summary", type=Path)
    parser.add_argument("--candidate-summary", type=Path)
    parser.add_argument("--expected-need", type=int, choices=tuple(EXPECTED_NEEDS.values()))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.self_check:
        result = self_check()
        out = Path(__file__).resolve().parent / "existing_A1_Euler293_rowwise_result.json"
        out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
        print(json.dumps(result, indent=2, sort_keys=True))
        return
    required = (args.reference_csv, args.candidate_csv, args.reference_summary,
                args.candidate_summary, args.expected_need, args.output)
    if any(value is None for value in required):
        parser.error("supply both CSVs, both summaries, --expected-need and --output")
    result = compare_rows(args.reference_csv, args.candidate_csv, args.reference_summary,
                          args.candidate_summary, args.expected_need)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps(result, sort_keys=True))
    raise SystemExit(0 if result["pass"] else 1)


if __name__ == "__main__":
    main()
