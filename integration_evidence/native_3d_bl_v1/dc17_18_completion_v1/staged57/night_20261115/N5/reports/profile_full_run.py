#!/usr/bin/env python3
"""Reuse the F3b extractors for a completed full run; no numerical execution."""
import argparse
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
N5 = HERE.parent
STAGED = N5.parents[1]


def extractor():
    path = N5 / "measure_existing_profile.py"
    spec = importlib.util.spec_from_file_location("existing_profile_reader", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.self_check()
    return module


def window_totals(windows):
    result = []
    for event in (1, 2):
        for rank in range(6):
            rows = [r for r in windows if r["event"] == event and r["rank"] == rank]
            assert rows, (event, rank)
            item = {"event": event, "rank": rank, "windows": len(rows)}
            for key in ("busy_seconds", "join_call_seconds", "tracked_collective_call_seconds"):
                item[key] = sum(float(r[key]) for r in rows)
            result.append(item)
    return result


def all_timer_rows(case, reader):
    result = []
    for event in (1, 2):
        path = case / f"f3b_event{event}_timer_gate.json"
        receipt = json.loads(path.read_text())
        assert receipt["passed"] and len(receipt["ranks"]) == 6
        for rank_index, rank in enumerate(receipt["ranks"]):
            for index, row in enumerate(rank["ledger"]["rows"]):
                result.append(dict(row, event=event, rank=int(rank["rank"]),
                    source_file=str(path), source_sha256=reader.sha256(path),
                    source_json_pointer=f"/ranks/{rank_index}/ledger/rows/{index}"))
    return result


def compare_timers(current, reference, reader, stem):
    def keyed(rows):
        data = {}
        for row in rows:
            key = tuple(row[k] for k in ("event", "rank", "root", "boundary", "phase", "operation"))
            assert key not in data, key
            data[key] = row
        return data
    old, new = keyed(all_timer_rows(reference, reader)), keyed(current)
    rows = []
    for key in sorted(set(old) | set(new)):
        item = dict(zip(("event", "rank", "root", "boundary", "phase", "operation"), key))
        for label, source in (("reference", old), ("candidate", new)):
            row = source.get(key)
            item[label + "_present"] = row is not None
            for field in ("calls", "inclusive_call_seconds", "exclusive_seconds", "source_file", "source_sha256"):
                item[label + "_" + field] = row.get(field) if row else None
        item["calls_equal"] = key in old and key in new and int(old[key]["calls"]) == int(new[key]["calls"])
        rows.append(item)
    reader.write_csv(Path(str(stem) + "_vs_reference_timer_rows.csv"), rows, list(rows[0]))
    return {"reference": str(reference), "rows": len(rows),
            "equal_call_rows": sum(r["calls_equal"] for r in rows),
            "scope": "observation only; each case keeps its own acceptance/reference gates"}


def write_markdown(path, case, totals, summaries):
    lines = ["# Full-run quality profile", "", f"Case: `{case}`", "",
        "Times are rank wall-seconds, not CPU time. Join-call and tracked-collective intervals may overlap.",
        "Inclusive tree rows are nested and non-additive; the separate exclusive columns follow the existing tree.", "",
        "| Event | Rank | Windows | Busy wall s | Join call s | Tracked collective s | Exclusive join s | Exclusive nonjoin s | Unattributed s |",
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    by_key = {(r["event"], r["rank"]): r for r in summaries}
    for row in totals:
        item = by_key[(row["event"], row["rank"])]
        values = [row["event"], row["rank"], row["windows"], row["busy_seconds"],
            row["join_call_seconds"], row["tracked_collective_call_seconds"], item["join_wait_seconds"],
            item["quality_named_nonjoin_exclusive_seconds"], item["quality_unattributed_seconds"]]
        lines.append("| " + " | ".join(str(v) for v in values) + " |")
    lines += ["", "The adjacent JSON/CSVs preserve per-row source paths, line numbers, hashes and timer-receipt JSON pointers.", ""]
    path.write_text("\n".join(lines))


def extract(case, stem, reference):
    reader = extractor()
    reader.WORKFLOW = case
    physical = json.loads((case / "full_workflow_receipt.json").read_text())
    assert physical["workflow_accepted"]
    windows, timers, sources = reader.gather()
    summaries, gates = reader.make_report(windows, timers, sources)
    totals = window_totals(windows)
    all_rows = all_timer_rows(case, reader)
    receipt = {"case": str(case), "window_row_count": len(windows), "timer_row_count": len(timers),
        "all_timer_row_count": len(all_rows), "summaries": summaries, "window_totals": totals,
        "rank_logs": sources, "timer_gates": gates, "all_timer_rows": all_rows,
        "reader_source_sha256": reader.sha256(N5 / "measure_existing_profile.py"),
        "physical_receipt_sha256": reader.sha256(case / "full_workflow_receipt.json")}
    qualification = case / "unattended_qualification.json"
    receipt["unattended_qualification"] = json.loads(qualification.read_text()) if qualification.exists() else None
    if reference:
        receipt["timer_comparison"] = compare_timers(all_rows, reference, reader, stem)
    reader.write_csv(Path(str(stem) + "_windows.csv"), windows,
        ["event", "rank", "source_file", "source_line", "stage"] + reader.WINDOW_FIELDS)
    reader.write_csv(Path(str(stem) + "_all_timer_rows.csv"), all_rows,
        ["event", "rank", "source_file", "source_sha256", "source_json_pointer"] + reader.TIMER_FIELDS)
    stem.with_suffix(".json").write_text(json.dumps(receipt, indent=2) + "\n")
    write_markdown(stem.with_suffix(".md"), case, totals, summaries)
    return {"passed": True, "case": str(case), "windows": len(windows), "all_timer_rows": len(all_rows)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", type=Path)
    parser.add_argument("--stem", type=Path)
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        extractor()
        rows = [dict(event=e, rank=r, busy_seconds="1", join_call_seconds="2",
                     tracked_collective_call_seconds="3") for e in (1, 2) for r in range(6)]
        assert len(window_totals(rows)) == 12 and window_totals(rows)[0]["busy_seconds"] == 1
        print(json.dumps({"passed": True, "launched": False}))
        return
    assert args.case and args.stem
    print(json.dumps(extract(args.case.resolve(), args.stem.resolve(), args.reference.resolve() if args.reference else None)))


if __name__ == "__main__":
    main()
