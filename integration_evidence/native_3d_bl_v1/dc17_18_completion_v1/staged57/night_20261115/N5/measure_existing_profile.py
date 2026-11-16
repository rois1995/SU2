#!/usr/bin/env python3
"""Extract existing F3b quality-window and timer-tree evidence; no solver runs."""

import csv
import hashlib
import json
import re
from pathlib import Path

N5 = Path(__file__).resolve().parent
ROOT = N5.parents[1]
F3B = ROOT / "f3b_20261115"
WORKFLOW = F3B / "full_workflow"
WINDOW_FIELDS = (
    "census round elapsed window_seconds busy_seconds join_call_seconds "
    "tracked_collective_call_seconds diagnostic_seconds commits commits_total "
    "private_tracked_collectives owned_q_removed owned_j_removed "
    "owned_debt_removed q_removed q_created j_removed j_created q_retired "
    "q_introduced j_retired j_introduced unmeasured_churn construction_edits "
    "ownership_changes queue blocked_features stuck_shape stuck_required missing_features"
).split()
TIMER_FIELDS = (
    "root boundary phase operation calls inclusive_call_seconds exclusive_seconds"
).split()
MARKER = re.compile(r"NATIVE3D_(QUALITY|TIMER)(?:\s+)(.*)")
CONSTRUCTION = "NATIVE3D_STAGE"


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_fields(payload):
    result = {}
    for token in payload.split():
        if "=" in token:
            key, value = token.split("=", 1)
            result[key] = value
    return result


def parse_row(line):
    match = MARKER.search(line)
    if not match:
        return None
    fields = parse_fields(match.group(2))
    fields["marker"] = match.group(1)
    return fields


def self_check():
    sample = "Sun <stdout>:NATIVE3D_QUALITY rank=2 stage=window census=3 busy_seconds=2.5 commits=8"
    fields = parse_row(sample)
    assert fields["marker"] == "QUALITY" and fields["rank"] == "2"
    assert float(fields["busy_seconds"]) + 1.5 == 4.0
    assert parse_row("NATIVE3D_NOT_A_MARKER x=1") is None
    assert parse_fields("operation=quality_optimization calls=4")["calls"] == "4"


def event_lines(path, rank):
    event = 0
    windows, timers = [], []
    for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
        if CONSTRUCTION in line and f"rank={rank} " in line and "stage=ownership_staged" in line and "phase=construction" in line:
            event += 1
        parsed = parse_row(line)
        if not parsed or event not in (1, 2) or parsed.get("rank") != str(rank):
            continue
        parsed.update(event=event, rank=rank, source_file=str(path), source_line=number)
        if parsed["marker"] == "QUALITY" and parsed.get("stage") == "window":
            windows.append(parsed)
    assert event == 2, f"{path}: expected 2 ownership_staged construction boundaries, got {event}"
    return windows, timers


def as_number(value):
    try:
        return int(value)
    except ValueError:
        return float(value)


def write_csv(path, rows, fields):
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def gather():
    all_windows, all_timers = [], []
    rank_paths = {}
    for rank in range(6):
        path = WORKFLOW / f"rank_output/1/rank.{rank}/stdout"
        rank_paths[str(rank)] = {"path": str(path), "sha256": sha256(path)}
        windows, timers = event_lines(path, rank)
        all_windows.extend(windows)
        all_timers.extend(timers)
    for event in (1, 2):
        receipt = WORKFLOW / f"f3b_event{event}_timer_gate.json"
        data = json.loads(receipt.read_text())
        digest = sha256(receipt)
        for rank_index, item in enumerate(data["ranks"]):
            for row_index, row in enumerate(item["ledger"]["rows"]):
                if row.get("phase") != "quality":
                    continue
                record = dict(row)
                record.update(event=event, rank=int(item["rank"]), source_file=str(receipt),
                              source_sha256=digest,
                              source_json_pointer=f"/ranks/{rank_index}/ledger/rows/{row_index}")
                all_timers.append(record)
    return all_windows, all_timers, rank_paths


def quality_rows(timer_rows):
    return [row for row in timer_rows if row.get("boundary") == "final"]


def make_report(windows, timers, rank_paths):
    timer_gates = {}
    summaries = []
    for event in (1, 2):
        gate_path = WORKFLOW / f"f3b_event{event}_timer_gate.json"
        gate = json.loads(gate_path.read_text())
        assert gate["passed"] and gate["event"] == event and len(gate["ranks"]) == 6
        timer_gates[str(event)] = {"path": str(gate_path), "sha256": sha256(gate_path), "passed": gate["passed"]}
        for item in gate["ranks"]:
            rank = int(item["rank"])
            qrows = [r for r in quality_rows(timers) if r["event"] == event and r["rank"] == rank]
            by_op = {}
            for row in qrows:
                op = row["operation"]
                by_op.setdefault(op, {"calls": 0, "exclusive_seconds": 0.0, "inclusive_call_seconds": 0.0})
                by_op[op]["calls"] += int(row["calls"])
                by_op[op]["exclusive_seconds"] += float(row["exclusive_seconds"])
                by_op[op]["inclusive_call_seconds"] += float(row["inclusive_call_seconds"])
            gap = float(item["quality_unattributed_seconds"])
            observed_gap = by_op.get("unattributed", {}).get("exclusive_seconds", 0.0)
            assert abs(gap - observed_gap) < 1e-6, (event, rank, gap, observed_gap)
            named = sum(v["exclusive_seconds"] for k, v in by_op.items() if k != "unattributed")
            summaries.append({
                "event": event, "rank": rank,
                "window_rows": sum(1 for w in windows if w["event"] == event and w["rank"] == rank),
                "quality_named_exclusive_seconds": named,
                "quality_named_nonjoin_exclusive_seconds": named - by_op.get("join_wait", {}).get("exclusive_seconds", 0.0),
                "quality_unattributed_seconds": gap,
                "quality_all_exclusive_seconds": named + gap,
                "join_wait_seconds": float(item["join_wait_seconds"]),
                "legacy_join_seconds": float(item["legacy_join"]["join_seconds"]),
                "legacy_join_calls_or_windows": int(item["legacy_join"]["windows"]),
                "timer_quality_operations": by_op,
            })
    assert len(summaries) == 12
    assert all(s["window_rows"] > 0 for s in summaries)
    return summaries, timer_gates


def main():
    self_check()
    windows, timers, rank_paths = gather()
    assert len(windows) == 204 and len(timers) > 0, (len(windows), len(timers))
    assert {(w["event"], w["rank"]) for w in windows} == {(e, r) for e in (1, 2) for r in range(6)}
    for event in (1, 2):
        expected = 20 if event == 1 else 14
        for rank in range(6):
            rows = [w for w in windows if w["event"] == event and w["rank"] == rank]
            assert len(rows) == expected
            assert all(all(key in row for key in WINDOW_FIELDS) for row in rows)
    summaries, timer_gates = make_report(windows, timers, rank_paths)
    write_csv(N5 / "existing_quality_windows.csv", windows,
              ["event", "rank", "source_file", "source_line", "stage"] + WINDOW_FIELDS)
    write_csv(N5 / "existing_quality_timer_rows.csv", timers,
              ["event", "rank", "source_file", "source_sha256", "source_json_pointer"] + TIMER_FIELDS)
    receipt = json.loads((F3B / "complete_milestone_receipt.json").read_text())
    assert receipt["passed"] and receipt["physical"]["workflow_accepted"]
    provenance_path = WORKFLOW / "provenance.json"
    provenance = json.loads(provenance_path.read_text())
    freeze_path = F3B / "source_freeze.json"
    freeze = json.loads(freeze_path.read_text())
    inputs = {"complete_milestone_receipt": str(F3B / "complete_milestone_receipt.json"),
              "complete_milestone_receipt_sha256": sha256(F3B / "complete_milestone_receipt.json"),
              "full_workflow_receipt": str(WORKFLOW / "full_workflow_receipt.json"),
              "full_workflow_receipt_sha256": sha256(WORKFLOW / "full_workflow_receipt.json"),
              "provenance": str(provenance_path), "provenance_sha256": sha256(provenance_path),
              "source_freeze": str(freeze_path), "source_freeze_sha256": sha256(freeze_path),
              "binary_sha256": provenance["binary_sha256"],
              "source_pins_sha256": provenance["source_pins_sha256"],
              "source_commit_base": freeze["source_commit_base"], "build": freeze["build"],
              "accepted": receipt["passed"], "physical_workflow_accepted": receipt["physical"]["workflow_accepted"],
              "rank_stdout": rank_paths, "timer_gate_receipts": timer_gates}
    (N5 / "existing_profile.json").write_text(json.dumps({
        "purpose": "existing accepted F3b timing extraction; no new solver run",
        "extraction_started_unix": 1794786493.3039729,
        "source_scope": "all six rank stdout streams, two event segments, quality window markers and final-boundary quality timer rows",
        "self_check": "PASS: parser fields, marker rejection, and busy/join arithmetic smoke checks",
        "window_row_count": len(windows), "timer_row_count": len(timers),
        "summaries": summaries, "provenance": inputs,
        "limits": ["busy_seconds is observed wall interval inside the quality window; it is not CPU time.",
                   "join_call_seconds is a measured call interval; do not add it to tracked_collective_call_seconds without accounting for overlap.",
                   "Timer inclusive_call_seconds are nested/inclusive; operation rows are not additive. Use exclusive_seconds for tree accounting.",
                   "No private_work_used field exists in these window markers; M20 work-unit cost is unavailable.",
                   "No direct per-candidate M16 export/projection/q-J/move/guidance record is present; aggregate operation rows do not supply that split."]
    }, indent=2) + "\n")
    write_report(windows, summaries, inputs)


def write_report(windows, summaries, provenance):
    lines = ["# Existing F3b quality timing extraction", "",
             "Read-only extraction from accepted F3b evidence; no numerical run was performed.",
             "The timer receipts report both event gates passed. Exact marker paths, SHA-256 hashes, source commit/build, and binary/source-pin hashes are in `existing_profile.json`.", "",
             "## M8 window rows by event and rank", "",
             "| Event | Rank | Windows | Sum busy (s) | Sum join call (s) | Sum tracked collective call (s) | Join wait tree exclusive (s) | Quality unattributed (s) | Named quality exclusive, including join (s) |", "|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for item in summaries:
        selected = [w for w in windows if w["event"] == item["event"] and w["rank"] == item["rank"]]
        total = lambda key: sum(float(w[key]) for w in selected)
        lines.append(f"| {item['event']} | {item['rank']} | {len(selected)} | {total('busy_seconds'):.6f} | {total('join_call_seconds'):.6f} | {total('tracked_collective_call_seconds'):.6f} | {item['join_wait_seconds']:.6f} | {item['quality_unattributed_seconds']:.6f} | {item['quality_named_exclusive_seconds']:.6f} |")
    lines += ["", "Window-level source lines and all extracted fields are in `existing_quality_windows.csv`; timer tree rows are traced by JSON Pointer and receipt SHA-256 in `existing_quality_timer_rows.csv`. The timer receipts independently validate `quality_unattributed_seconds` against the final-boundary `unattributed` exclusive timer row for every rank/event.", "",
              "`busy_seconds` is wall time within the marker's busy interval, not CPU time. `join_call_seconds` and tracked collective time may overlap; their sums are shown as separate observations and are not additive. Timer inclusive values are nested; use exclusive values for attribution. `private_tracked_collectives` is included as printed, but is not the absent `private_work_used` counter, so M20 unit cost remains unavailable.", "",
              "M16: the existing timer tree has aggregate quality operation calls/times (including candidate, geometry projection, quality optimization, and clearance) but no direct per-wall-move candidate split into export, projection, q/J, move, and guidance. Those requested component timings remain unavailable from these logs.", "",
              "This is reuse of existing measurements only; it does not claim M8's newly requested handoff-step-2 profile was performed or that N5 is complete/accepted.", ""]
    (N5 / "existing_profile.md").write_text("\n".join(lines))


if __name__ == "__main__":
    main()
