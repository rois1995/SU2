#!/usr/bin/env python3
"""Strictly compare a cut candidate prefix against a law-enabled reference."""
import importlib.util
import sys
from pathlib import Path

ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / ".git").exists())
N8V = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N8/A2_A3_N12/section6_8/validation"
COMPARATOR68 = N8V.parent / "drafts/compare_integer_rows_68.py"
spec = importlib.util.spec_from_file_location("strict_candidate_n8_comparator68", COMPARATOR68)
if spec is None or spec.loader is None:
    raise RuntimeError(f"cannot load qualified physical-prefix comparator: {COMPARATOR68}")
base = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = base
spec.loader.exec_module(base)


def strict_timer_compare(reference, current, ref_cutoffs, cur_cutoffs):
    """Require identical timer row structure and every call count, including Census."""
    ref_files, cur_files = base.source_files(reference), base.source_files(current)
    if ref_files.keys() != cur_files.keys():
        return {"passed": False, "first_difference": "physical timer stream set differs"}
    ref_law, cur_law = {}, {}
    timer_snapshot_count = 0
    for label in sorted(ref_files):
        ref_rank = base.stream_rank(ref_files, label)
        cur_rank = base.stream_rank(cur_files, label)
        assert ref_rank == cur_rank
        ref_physical_end = ref_cutoffs[label].get(str(ref_rank), max(ref_cutoffs[label].values()))
        cur_physical_end = cur_cutoffs[label].get(str(cur_rank), max(cur_cutoffs[label].values()))
        ref_timer_limits = {str(rank): min(ref_cutoffs[label].get(str(rank), ref_physical_end),
                                           ref_physical_end) for rank in range(6)}
        cur_timer_limits = {str(rank): min(cur_cutoffs[label].get(str(rank), cur_physical_end),
                                           cur_physical_end) for rank in range(6)}
        for destination, path, cutoffs in ((ref_law, ref_files[label], ref_cutoffs[label]),
                                           (cur_law, cur_files[label], cur_cutoffs[label])):
            is_reference = destination is ref_law
            rank = ref_rank if is_reference else cur_rank
            timer_limits = ref_timer_limits if is_reference else cur_timer_limits
            base.verify_law_ranks(path, rank, timer_limits)
            destination[label] = base.count_wall_law(path, timer_limits)
        left = base.parse_timer_stream(ref_files[label], ref_timer_limits)
        right = base.parse_timer_stream(cur_files[label], cur_timer_limits)
        if len(left) != len(right):
            return {"passed": False, "first_difference": {"stream": label, "key": "timer_snapshot_count"}}
        timer_snapshot_count += len(left)
        for index, (before, after) in enumerate(zip(left, right), 1):
            if before["key"] != after["key"]:
                return {"passed": False, "first_difference": {"stream": label, "snapshot": index,
                        "key": "snapshot_identity", "reference": before["key"], "candidate": after["key"]}}
            if before["law_lines_before"] != after["law_lines_before"]:
                return {"passed": False, "first_difference": {"stream": label, "snapshot": index,
                        "key": "wall_law_lines_before_boundary", "reference": before["law_lines_before"],
                        "candidate": after["law_lines_before"]}}
            for side, snap in (("reference", before), ("candidate", after)):
                if snap["summary"].get("fixedstorage") != "4120":
                    return {"passed": False, "first_difference": {"stream": label, "snapshot": index,
                            "key": f"{side}.fixedstorage", "value": snap["summary"].get("fixedstorage")}}
            difference = base.exact_fields(before["summary"], after["summary"], base.SUMMARY_MEASUREMENTS)
            if difference:
                return {"passed": False, "first_difference": {"stream": label, "snapshot": index,
                        "key": f"timer_summary.{difference}"}}
            if len(before["rows"]) != len(after["rows"]):
                return {"passed": False, "first_difference": {"stream": label, "snapshot": index,
                        "key": "timer_row_count", "reference": len(before["rows"]),
                        "candidate": len(after["rows"])} }
            for row_index, (row_a, row_b) in enumerate(zip(before["rows"], after["rows"]), 1):
                difference = base.exact_fields(row_a["fields"], row_b["fields"], base.TIMER_MEASUREMENTS)
                if difference:
                    return {"passed": False, "first_difference": {"stream": label, "snapshot": index,
                            "timer_row": row_index, "key": difference}}
    if ref_law != cur_law:
        return {"passed": False, "first_difference": {"key": "wall_law_line_count_by_physical_stream",
                "reference": ref_law, "candidate": cur_law}}
    return {"passed": True, "timer_snapshots": timer_snapshot_count,
            "wall_law_lines_by_physical_stream": ref_law,
            "rule": "strict per-row timer fields/calls and exact wall-law line counts; only timing values excluded"}


def compare_cases(reference_path, candidate_path, gate):
    """Apply the same physical rank cutoff to both cases, then strict old rows and timers."""
    reference_path, candidate_path = Path(reference_path), Path(candidate_path)
    ref_rows, cur_rows = base.N4.streams(reference_path), base.N4.streams(candidate_path)
    ref_files, cur_files = base.source_files(reference_path), base.source_files(candidate_path)
    ref_cutoffs = {name: base.cutoff_lines(ref_rows[name], gate) for name in ref_rows}
    cur_cutoffs = {name: base.cutoff_lines(cur_rows[name], gate) for name in cur_rows}
    ref_plain = {name: base.plain_prefix(rows, ref_cutoffs[name], name, ref_files, gate)
                 for name, rows in ref_rows.items()}
    cur_plain = {name: base.plain_prefix(rows, cur_cutoffs[name], name, cur_files, gate)
                 for name, rows in cur_rows.items()}
    integer = base.N4.compare_streams(ref_plain, cur_plain, gate)
    timer = strict_timer_compare(reference_path, candidate_path, ref_cutoffs, cur_cutoffs)
    return {"passed": integer.get("passed") is True and timer.get("passed") is True,
            "gate": gate, "integer_rows": integer, "timer_rows": timer,
            "comparison_rule": "all old rows strict; every timer row and census call strict; no census allowance"}


def self_check():
    """Synthetic physical logs prove row-wise Census calls, structure and law counts are strict."""
    import tempfile
    from pathlib import Path

    def create(case, changed=None, overshoot=False):
        for rank in range(6):
            path = case / "rank_output/1" / f"rank.{rank}" / "stdout"
            path.parent.mkdir(parents=True, exist_ok=True)
            law_count = 2 if changed == (rank, "law") else 1
            lines = [f"NATIVE3D_WALL_LAW rank={rank} source=primary boundary=entry\n" for _ in range(law_count)]
            calls_a, calls_b = (2, 1) if changed == (rank, "calls") else (1, 2)
            lines.extend((f"NATIVE3D_TIMER root=engine boundary=entry rank={rank} phase=quality operation=census calls={calls_a} inclusive_call_seconds=0.1 exclusive_seconds=0.1\n",
                          f"NATIVE3D_TIMER root=engine boundary=entry rank={rank} phase=quality operation=census calls={calls_b} inclusive_call_seconds=0.1 exclusive_seconds=0.1\n",
                          f"NATIVE3D_TIMER root=engine boundary=entry rank={rank} phase=quality operation=optimizer calls=2 inclusive_call_seconds=0.1 exclusive_seconds=0.1\n",
                          f"NATIVE3D_TIMER_SUMMARY root=engine boundary=entry rank={rank} ranks=6 fixedstorage=4120 root_seconds=0.2 sumexclusive=0.1 unattributed=0.1 coverage=1.0\n",
                          f"NATIVE3D_STAGE rank={rank} stage=length_collapse_exit accepted=1\n"))
            if overshoot:
                other = (rank + 1) % 6
                lines.extend((f"NATIVE3D_TIMER root=late boundary=overshoot rank={other} phase=quality operation=census calls=999 inclusive_call_seconds=9 exclusive_seconds=9\n",
                              f"NATIVE3D_TIMER_SUMMARY root=late boundary=overshoot rank={other} ranks=6 fixedstorage=4120 root_seconds=9 sumexclusive=9 unattributed=0 coverage=1\n"))
            path.write_text("".join(lines))

    with tempfile.TemporaryDirectory(prefix="strict-candidate-prefix-") as temporary:
        root = Path(temporary)
        reference, same, wrong_calls, wrong_law = (root / name for name in ("ref", "same", "calls", "law"))
        create(reference); create(same, overshoot=True); create(wrong_calls, (2, "calls")); create(wrong_law, (2, "law"))
        good = compare_cases(reference, same, "G2")
        assert good["passed"], good
        call_result = compare_cases(reference, wrong_calls, "G2")
        law_result = compare_cases(reference, wrong_law, "G2")
        assert not call_result["passed"] and not law_result["passed"], (call_result, law_result)
        assert "calls" in str(call_result["timer_rows"]["first_difference"])
    return {"passed": True, "launched": False, "cases": 6,
            "strict_census_calls": True, "strict_timer_row_structure": True,
            "strict_wall_law_line_counts": True, "physical_stream_crop": True}


if __name__ == "__main__":
    import argparse, json
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--gate", choices=("G2", "GQ"))
    parser.add_argument("--reference")
    parser.add_argument("--candidate")
    args = parser.parse_args()
    if args.self_check:
        result = self_check()
    else:
        assert args.gate and args.reference and args.candidate
        result = compare_cases(args.reference, args.candidate, args.gate)
    print(json.dumps(result, indent=2))
    if not args.self_check and not result["passed"]:
        raise SystemExit(1)
