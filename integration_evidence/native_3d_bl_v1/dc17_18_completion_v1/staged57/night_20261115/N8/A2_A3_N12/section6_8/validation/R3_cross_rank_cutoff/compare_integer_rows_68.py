#!/usr/bin/env python3
"""Compare G2/GQ rows with the exact Section 6.8 Census-call addition rule."""
from __future__ import annotations

import argparse
import importlib.util
import json
import re
import sys
from pathlib import Path

ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / ".git").exists())
N4_PATH = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/compare_integer_rows.py"
SPEC = importlib.util.spec_from_file_location("n4_integer_row_comparator", N4_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot load existing N4 comparator: {N4_PATH}")
N4 = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = N4
SPEC.loader.exec_module(N4)

TIMER_ROW = "NATIVE3D_TIMER "
TIMER_SUMMARY = "NATIVE3D_TIMER_SUMMARY "
WALL_LINE = "NATIVE3D_WALL_LAW "
TIMER_MEASUREMENTS = {"inclusive_call_seconds", "exclusive_seconds"}
SUMMARY_MEASUREMENTS = {"root_seconds", "sumexclusive", "unattributed", "coverage"}
FIELD = re.compile(r"([A-Za-z_]\w*)=([^\s]+)")
INT = re.compile(r"^[+-]?\d+$")


def fields_after(line: str, marker: str) -> dict[str, str]:
    """Extract whitespace-separated fields after an exact output marker."""
    return dict(FIELD.findall(line.split(marker, 1)[1]))


def source_files(case: Path) -> dict[str, Path]:
    """Find the same deterministic rank stdout streams used by the N4 checker."""
    if case.is_file():
        return {case.name: case}
    files = sorted(case.glob("rank_output/**/rank.*/stdout"))
    if files:
        return {str(path.relative_to(case)): path for path in files}
    log = case / "run.log"
    if log.is_file():
        return {"run.log": log}
    raise ValueError(f"No rank stdout or run.log under {case}")


def parse_timer_stream(path: Path, cutoffs: dict[str, int] | None = None) -> list[dict]:
    """Parse snapshots through the per-rank gate line and count cumulative law calls."""
    snapshots, pending = [], []
    law_since_final: dict[str, int] = {}
    pending_law_count: int | None = None
    limits = cutoffs or {}
    for number, raw in enumerate(path.open(errors="replace"), 1):
        line = N4.strip_mpi_prefixes(raw)
        if WALL_LINE in line:
            wall = fields_after(line, WALL_LINE)
            rank = wall.get("rank", "")
            assert INT.fullmatch(rank), f"missing wall rank at {path}:{number}"
            if number <= limits.get(rank, number):
                law_since_final[rank] = law_since_final.get(rank, 0) + 1
            continue
        if line.startswith(TIMER_ROW):
            row_fields = fields_after(line, TIMER_ROW)
            rank = row_fields.get("rank", "")
            if number > limits.get(rank, number):
                continue
            if not pending:
                pending_law_count = law_since_final.get(rank, 0)
            pending.append({"fields": row_fields, "line": number, "path": str(path)})
            continue
        if not line.startswith(TIMER_SUMMARY):
            continue
        summary = fields_after(line, TIMER_SUMMARY)
        rank = summary.get("rank", "")
        if number > limits.get(rank, number):
            continue
        required = ("root", "boundary", "rank", "ranks", "fixedstorage")
        assert all(key in summary for key in required), f"incomplete timer summary at {path}:{number}"
        assert all(row["fields"].get("root") == summary["root"] and
                   row["fields"].get("boundary") == summary["boundary"] and
                   row["fields"].get("rank") == summary["rank"] for row in pending), \
            f"timer rows do not belong to following summary at {path}:{number}"
        rank = summary["rank"]
        boundary_law_count = (pending_law_count if pending_law_count is not None
                              else law_since_final.get(rank, 0))
        law_delta = boundary_law_count
        snapshots.append({"key": (summary["root"], summary["boundary"], summary["rank"]),
                          "rows": pending, "summary": summary, "law_lines_before": law_delta,
                          "path": str(path), "line": number})
        pending = []
        pending_law_count = None
        if summary["boundary"] == "final":
            law_since_final[rank] = 0
    assert not pending, f"timer operation rows lack a summary in {path}"
    return snapshots


def stream_rank(files: dict[str, Path], label: str) -> int | None:
    """Return the rank from a rank_output path, or None for an aggregate log."""
    match = re.search(r"rank\.(\d+)/stdout$", label)
    return int(match.group(1)) if match else None


def verify_law_ranks(path: Path, expected_rank: int | None,
                     cutoffs: dict[str, int] | None = None) -> None:
    """Validate in-scope wall-law lines without treating rank overshoot as gated data."""
    limits = cutoffs or {}
    for number, raw in enumerate(path.open(errors="replace"), 1):
        line = N4.strip_mpi_prefixes(raw)
        if WALL_LINE not in line:
            continue
        fields = fields_after(line, WALL_LINE)
        rank = fields.get("rank", "")
        assert INT.fullmatch(rank), f"invalid wall-law rank at {path}:{number}"
        if number > limits.get(rank, number):
            continue
        if expected_rank is not None:
            assert int(rank) == expected_rank, f"wall-law rank does not match stream at {path}:{number}"


def exact_fields(left: dict[str, str], right: dict[str, str], ignored: set[str]) -> str | None:
    """Compare all unignored timer fields exactly, including every integer field."""
    if left.keys() != right.keys():
        return f"field_names: {sorted(left)} != {sorted(right)}"
    for key in sorted(left.keys() - ignored):
        if left[key] != right[key]:
            return f"{key}: {left[key]!r} != {right[key]!r}"
    return None


def compare_snapshot(reference: dict, current: dict) -> str | None:
    """Compare one snapshot; aggregate Census calls and keep all other rows strict."""
    if reference["key"] != current["key"]:
        return f"snapshot identity: {reference['key']} != {current['key']}"
    left_summary, right_summary = reference["summary"], current["summary"]
    if left_summary.get("fixedstorage") != "4120" or right_summary.get("fixedstorage") != "4120":
        return f"fixedstorage: {left_summary.get('fixedstorage')} != 4120 or {right_summary.get('fixedstorage')} != 4120"
    difference = exact_fields(left_summary, right_summary, SUMMARY_MEASUREMENTS)
    if difference:
        return f"summary.{difference}"
    left_census = right_census = 0
    left_other = [row["fields"] for row in reference["rows"] if row["fields"].get("operation") != "census"]
    right_other = [row["fields"] for row in current["rows"] if row["fields"].get("operation") != "census"]
    for row in reference["rows"]:
        if row["fields"].get("operation") == "census":
            left_census += parse_calls(row["fields"], row["path"], row["line"])
    for row in current["rows"]:
        if row["fields"].get("operation") == "census":
            right_census += parse_calls(row["fields"], row["path"], row["line"])
    if len(left_other) != len(right_other):
        return f"non-census timer rows: {len(left_other)} != {len(right_other)}"
    for index, (left, right) in enumerate(zip(left_other, right_other), 1):
        difference = exact_fields(left, right, TIMER_MEASUREMENTS)
        if difference:
            return f"timer_row[{index}].{difference}"
    expected = left_census + current["law_lines_before"]
    if right_census != expected:
        return f"census.calls sum: {right_census} != reference {left_census} + wall-law lines {current['law_lines_before']}"
    return None


def parse_calls(fields: dict[str, str], path: str, line: int) -> int:
    """Read a nonnegative census call count without accepting malformed values."""
    value = fields.get("calls", "")
    assert INT.fullmatch(value) and int(value) >= 0, f"invalid calls at {path}:{line}"
    assert not (fields.keys() - {"root", "boundary", "rank", "phase", "operation", "calls"} - TIMER_MEASUREMENTS), \
        f"unexpected Census fields at {path}:{line}: {fields}"
    return int(value)


def cutoff_lines(rows: list, gate: str) -> dict[str, int]:
    """Reuse N4's per-rank anchor cutoff, including rank-specific polling overshoot."""
    if gate == "all":
        return {}
    selected, anchors = N4.cutoff_rows(rows, gate)
    assert anchors, f"no {gate} cutoff anchors found"
    result = {}
    for row in selected:
        rank = row.rank()
        if rank in anchors:
            result[rank] = max(result.get(rank, 0), row.line)
    assert anchors <= set(result), f"cutoff line missing for anchors {sorted(anchors - set(result))}"
    return result


def compare_timer_streams(reference: dict[str, Path], current: dict[str, Path],
                          reference_cutoffs: dict[str, dict[str, int]],
                          current_cutoffs: dict[str, dict[str, int]]) -> dict:
    """Apply exact timer comparisons only through each rank's selected gate line."""
    if reference.keys() != current.keys():
        return {"passed": False, "first_difference": "rank stdout stream set differs"}
    snapshots_ref, snapshots_cur = {}, {}
    reference_wall_count = sum(count_wall_law(path) for path in reference.values())
    if reference_wall_count:
        return {"passed": False, "first_difference":
                f"reference is not pre-law: contains {reference_wall_count} NATIVE3D_WALL_LAW lines"}
    for label in sorted(reference):
        rank = stream_rank(current, label)
        verify_law_ranks(current[label], rank, current_cutoffs.get(label))
        snapshots_ref[label] = parse_timer_stream(reference[label], reference_cutoffs.get(label))
        snapshots_cur[label] = parse_timer_stream(current[label], current_cutoffs.get(label))
    for label in sorted(reference):
        left, right = snapshots_ref[label], snapshots_cur[label]
        if len(left) != len(right):
            return {"passed": False, "first_difference": f"{label}: timer snapshot count {len(left)} != {len(right)}"}
        for index, (before, after) in enumerate(zip(left, right), 1):
            difference = compare_snapshot(before, after)
            if difference:
                return {"passed": False, "first_difference": {"stream": label, "snapshot": index,
                        "key": difference, "reference": before["key"], "current": after["key"]}}
    current_wall_count = sum(count_wall_law(path, current_cutoffs.get(label))
                             for label, path in current.items())
    return {"passed": True, "snapshot_count": sum(map(len, snapshots_cur.values())),
            "reference_wall_law_lines": reference_wall_count,
            "wall_law_lines_before_cutoffs": current_wall_count}


def count_wall_law(path: Path, cutoffs: dict[str, int] | None = None) -> int:
    """Count rank-tagged wall-law records through each rank's selected anchor."""
    limits = cutoffs or {}
    total = 0
    for number, raw in enumerate(path.open(errors="replace"), 1):
        line = N4.strip_mpi_prefixes(raw)
        if WALL_LINE not in line:
            continue
        rank = fields_after(line, WALL_LINE).get("rank", "")
        if INT.fullmatch(rank) and number <= limits.get(rank, number):
            total += 1
    return total


def compare_cases(reference_path: Path, current_path: Path, gate: str) -> dict:
    """Compare existing integer rows and the Section 6.8 timer adjustment."""
    reference_rows = N4.streams(reference_path)
    current_rows = N4.streams(current_path)
    reference_plain = {name: [row for row in rows if row.marker != "NATIVE3D_TIMER"]
                       for name, rows in reference_rows.items()}
    current_plain = {name: [row for row in rows if row.marker != "NATIVE3D_TIMER"]
                     for name, rows in current_rows.items()}
    integer = N4.compare_streams(reference_plain, current_plain, gate)
    reference_files, current_files = source_files(reference_path), source_files(current_path)
    reference_cutoffs = {name: cutoff_lines(reference_rows[name], gate) for name in reference_rows}
    current_cutoffs = {name: cutoff_lines(current_rows[name], gate) for name in current_rows}
    timer = compare_timer_streams(reference_files, current_files, reference_cutoffs, current_cutoffs)
    return {"passed": integer["passed"] and timer["passed"], "gate": gate,
            "integer_rows": integer, "timer_rows": timer}


def synthetic_snapshot(lines: list[str]) -> list[dict]:
    """Parse synthetic output text with the same timer/session parser as real logs."""
    from tempfile import NamedTemporaryFile
    with NamedTemporaryFile("w+", encoding="utf-8") as stream:
        stream.write("\n".join(lines) + "\n")
        stream.flush()
        return parse_timer_stream(Path(stream.name))


def synthetic_cutoff_check() -> dict:
    """Show rank-zero post-GQ overshoot is excluded while rank-one lines remain."""
    from tempfile import NamedTemporaryFile
    lines = ["NATIVE3D_WALL_LAW rank=0 boundary=entry",
             "NATIVE3D_TIMER root=remesh boundary=window rank=0 phase=quality operation=census calls=1 inclusive_call_seconds=1 exclusive_seconds=1",
             "NATIVE3D_TIMER_SUMMARY root=remesh boundary=window rank=0 ranks=2 root_seconds=1 sumexclusive=1 unattributed=0 coverage=1 fixedstorage=4120"]
    lines += ["NATIVE3D_QUALITY rank=0 stage=window census=1"] * 3
    lines += ["NATIVE3D_WALL_LAW rank=0 boundary=overshoot",
              "NATIVE3D_TIMER root=remesh boundary=overshoot rank=0 phase=quality operation=census calls=2 inclusive_call_seconds=1 exclusive_seconds=1",
              "NATIVE3D_TIMER_SUMMARY root=remesh boundary=overshoot rank=0 ranks=2 root_seconds=1 sumexclusive=1 unattributed=0 coverage=1 fixedstorage=4120",
              "NATIVE3D_WALL_LAW rank=1 boundary=entry",
              "NATIVE3D_TIMER root=remesh boundary=window rank=1 phase=quality operation=census calls=1 inclusive_call_seconds=1 exclusive_seconds=1",
              "NATIVE3D_TIMER_SUMMARY root=remesh boundary=window rank=1 ranks=2 root_seconds=1 sumexclusive=1 unattributed=0 coverage=1 fixedstorage=4120"]
    lines += ["NATIVE3D_QUALITY rank=1 stage=window census=1"] * 3
    with NamedTemporaryFile("w+", encoding="utf-8") as stream:
        stream.write("\n".join(lines) + "\n")
        stream.flush()
        rows = N4.marker_rows(Path(stream.name))
        cutoffs = cutoff_lines(rows, "GQ")
        snapshots = parse_timer_stream(Path(stream.name), cutoffs)
        assert cutoffs == {"0": 6, "1": 15}
        assert [row["summary"]["rank"] for row in snapshots] == ["0", "1"]
        assert [row["law_lines_before"] for row in snapshots] == [1, 1]
    return {"passed": True, "rank0_cutoff_line": 6, "rank1_cutoff_line": 15,
            "post_cut_rank0_timer_removed": True}


def synthetic_root_reset_check() -> dict:
    """Require a final summary of any root to reset the rank's law-line count."""
    rows = ["NATIVE3D_WALL_LAW rank=0 boundary=entry"]
    for root, boundary, calls in (("remesh", "entry", 2), ("native_engine", "final", 2),
                                  ("remesh", "entry_again", 1)):
        rows.extend([
            f"NATIVE3D_TIMER root={root} boundary={boundary} rank=0 phase=construction operation=census calls={calls} inclusive_call_seconds=1 exclusive_seconds=1",
            f"NATIVE3D_TIMER_SUMMARY root={root} boundary={boundary} rank=0 ranks=1 root_seconds=1 sumexclusive=1 unattributed=0 coverage=1 fixedstorage=4120"])
    snapshots = synthetic_snapshot(rows)
    deltas = [row["law_lines_before"] for row in snapshots]
    assert deltas == [1, 1, 0]
    return {"passed": True, "law_deltas": deltas, "reset_root": "native_engine/final"}


def self_check() -> dict:
    """Check census delta, old calls/bytes, new operations and actual N1 G2 self-identity."""
    prefix = ["NATIVE3D_WALL_LAW rank=0 boundary=entry", "NATIVE3D_WALL_LAW rank=0 boundary=exit"]
    summary = "NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank=0 ranks=1 root_seconds=1 sumexclusive=1 unattributed=0 coverage=1 fixedstorage=4120"
    base_rows = ["NATIVE3D_TIMER root=remesh boundary=final rank=0 phase=construction operation=census calls=2 inclusive_call_seconds=1 exclusive_seconds=1",
                 "NATIVE3D_TIMER root=remesh boundary=final rank=0 phase=quality operation=census calls=1 inclusive_call_seconds=1 exclusive_seconds=1",
                 "NATIVE3D_TIMER root=remesh boundary=final rank=0 phase=construction operation=geometry_bound calls=7 inclusive_call_seconds=1 exclusive_seconds=1"]
    reference = synthetic_snapshot(base_rows + [summary])
    adjusted = synthetic_snapshot(prefix + [base_rows[0].replace("calls=2", "calls=3"),
        base_rows[1].replace("calls=1", "calls=2"), base_rows[2], summary])
    assert compare_snapshot(reference[0], adjusted[0]) is None
    wrong_delta = synthetic_snapshot(prefix + [base_rows[0], base_rows[1].replace("calls=1", "calls=2"),
        base_rows[2], summary])
    assert "census.calls sum" in compare_snapshot(reference[0], wrong_delta[0])
    wrong_calls = synthetic_snapshot(prefix + [base_rows[0], base_rows[1],
        base_rows[2].replace("calls=7", "calls=8"), summary])
    assert "timer_row" in compare_snapshot(reference[0], wrong_calls[0])
    wrong_bytes = synthetic_snapshot(prefix + base_rows + [summary.replace("fixedstorage=4120", "fixedstorage=4128")])
    assert "fixedstorage" in compare_snapshot(reference[0], wrong_bytes[0])
    new_operation = synthetic_snapshot(prefix + base_rows + [
        "NATIVE3D_TIMER root=remesh boundary=final rank=0 phase=quality operation=wall_law calls=2 inclusive_call_seconds=1 exclusive_seconds=1", summary])
    assert "non-census timer rows" in compare_snapshot(reference[0], new_operation[0])
    n1 = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/section6_1/first_collapse"
    actual = compare_cases(n1, n1, "G2")
    assert actual["passed"] and actual["timer_rows"]["wall_law_lines_before_cutoffs"] == 0
    full = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/section6_1/full_workflow"
    actual_gq = compare_cases(full, full, "GQ")
    assert actual_gq["passed"]
    overshoot = synthetic_cutoff_check()
    assert overshoot["passed"]
    resets = synthetic_root_reset_check()
    assert resets["passed"]
    old_stage = N4.Row("NATIVE3D_STAGE", {"rank": "0", "stage": "x", "maximum_work_bytes": "10"}, "synthetic", 1)
    changed_stage = N4.Row("NATIVE3D_STAGE", {"rank": "0", "stage": "x", "maximum_work_bytes": "11"}, "synthetic", 1)
    assert "maximum_work_bytes" in N4.compare_row(old_stage, changed_stage)
    return {"passed": True, "launched": False, "actual_n1_g2_self_compare": actual,
            "actual_n1_gq_prefix_self_compare": actual_gq,
            "synthetic_overshoot": overshoot, "synthetic_any_root_reset": resets,
            "synthetic": {"census_delta": "pass", "wrong_delta": "fail", "old_calls": "fail",
                          "modeled_fixedstorage": "fail", "modeled_old_row_bytes": "fail",
                          "unexpected_operation": "fail"}}


def main() -> None:
    """Run draft synthetic checks or compare two existing G2/GQ case directories."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", nargs="?", type=Path)
    parser.add_argument("current", nargs="?", type=Path)
    parser.add_argument("--gate", choices=("G2", "GQ"), default="G2")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    assert args.reference and args.current
    result = compare_cases(args.reference.resolve(), args.current.resolve(), args.gate)
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()
