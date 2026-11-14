#!/usr/bin/env python3
"""Summarize native timer markers from one measurement case (stdlib only).

Usage: python3 summarize_timers.py CASE_DIR
Reads CASE_DIR/profile/rankN.program.log, optional
CASE_DIR/stage_trace_rankN.jsonl (or profile/ as fallback) and
CASE_DIR/actual_bl_surface_stage.json; writes CASE_DIR/timers_summary.json.
"""
import json
import re
import sys
from pathlib import Path


MARKERS = ("NATIVE3D_STAGE", "NATIVE3D_DETAIL", "NATIVE3D_QUALITY",
           "NATIVE3D_PHASE_OWNERSHIP", "NATIVE3D_LENGTH")
COUNTERS = ("accepted", "rounds", "length_splits", "length_collapses",
            "election_calls", "election_seconds", "collective_calls",
            "collective_seconds", "split_accepted", "collapse_accepted")


def parse_kv_line(line):
    """Parse a NATIVE3D key=value row; values are whitespace-free tokens."""
    parts = line.strip().split()
    if not parts or not parts[0].startswith("NATIVE3D_"):
        return None
    row = {"record": parts[0]}
    for token in parts[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            row[key] = value.rstrip(",")
    return row


def number(value):
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def rank_of(row, fallback=None):
    try:
        return int(row.get("rank", fallback))
    except (TypeError, ValueError):
        return fallback


def load_jsonl(path):
    rows = []
    if path.exists():
        for line in path.read_text(errors="replace").splitlines():
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                continue
            if isinstance(obj, dict):
                rows.append(obj)
    return rows


def read_program_log(path, fallback_rank):
    rows = []
    if not path.exists():
        return rows
    for line_no, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
        row = parse_kv_line(line)
        if row and row["record"] in MARKERS:
            row["source_line"] = line_no
            row["rank"] = rank_of(row, fallback_rank)
            rows.append(row)
    return rows


def classify_interval(left, right, lane):
    """Classify nonoverlapping adjacent events using emitted engine markers."""
    a = left.get("stage", "")
    if right.get("stage") == "construction_entry":
        return "other"
    if a in ("split_surface_exit", "split_interior_exit"):
        return "other"
    if right.get("stage") == "length_collapse_exit" or lane == "collapse":
        return "collapse"
    if lane == "finish":
        return "finish"
    if lane == "quality":
        return "quality"
    try:
        split_lane = int(left.get("split_lane", 0))
    except (ValueError, TypeError):
        split_lane = 0
    if split_lane == 1 or a == "split_surface_resume":
        return "surface"
    if split_lane == 2 or a == "split_interior_entry":
        return "interior"
    if lane == "collapse":
        return "collapse"
    return lane or "other"


def make_intervals(rows):
    """Difference stage snapshots in program order, splitting engine resets."""
    grouped = {}
    for row in rows:
        if row.get("record") == "NATIVE3D_STAGE":
            rank = rank_of(row, 0)
            elapsed = number(row.get("elapsed"))
            if elapsed is not None:
                grouped.setdefault(rank, []).append(row)
    intervals = []
    for rank, items in grouped.items():
        items.sort(key=lambda row: row.get("source_line", 0))
        event_id, lane, prev = 0, "collapse", None
        for right in items:
            t1 = number(right.get("elapsed"))
            t0 = number(prev.get("elapsed")) if prev else None
            if prev and (prev.get("stage") == "exit" or t1 < t0):
                event_id += 1
                prev, t0 = None, None
                lane = "collapse"
            if prev:
                label = classify_interval(prev, right, lane)
                if prev.get("stage") == "quality_exit":
                    label = "finish"
                deltas = {}
                for key in COUNTERS:
                    x, y = number(prev.get(key)), number(right.get(key))
                    if x is not None and y is not None:
                        deltas[key] = y - x
                intervals.append({"rank": rank, "event": event_id, "label": label,
                                  "start_elapsed_s": t0, "end_elapsed_s": t1,
                                  "wall_interval_s": t1 - t0,
                                  "start_marker": prev.get("stage"), "end_marker": right.get("stage"),
                                  "counter_deltas": deltas})
            stage = right.get("stage")
            if stage == "length_collapse_exit":
                lane = "other"
            elif stage == "quality_entry":
                lane = "quality"
            elif stage == "quality_exit":
                lane = "finish"
            elif stage == "construction_entry":
                lane = "collapse" if number(right.get("split_lane")) == 0 else "other"
            elif stage in ("split_surface_exit", "split_interior_exit"):
                # Lane-valued interval starts at the preceding snapshot; handoff itself is other.
                lane = "other"
            prev = right
    return intervals


def event_coverage(rows):
    out = []
    for row in rows:
        if row.get("record") != "NATIVE3D_DETAIL" or row.get("stage") not in ("refine", "partition"):
            continue
        total, other = number(row.get("total")), number(row.get("other"))
        coverage = 1 - other / total if total and other is not None else None
        out.append({"rank": rank_of(row, 0), "stage": row.get("stage"),
                    "total_s": total, "other_s": other, "coverage_fraction": coverage,
                    "counters": {k: row[k] for k in COUNTERS if k in row}})
    return out


def actual_bl_records(path):
    if not path.exists():
        return []
    try:
        data = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError):
        return []
    return data if isinstance(data, dict) else {"records": data}


def self_check():
    row = parse_kv_line("NATIVE3D_STAGE rank=2 stage=split_surface_exit elapsed=4.5 accepted=9")
    assert row["record"] == "NATIVE3D_STAGE" and row["rank"] == "2"
    stages = [
        {"record": "NATIVE3D_STAGE", "stage": "construction_entry", "rank": 0,
         "phase": "construction", "split_lane": "1", "elapsed": "1", "accepted": "2", "source_line": 1},
        {"record": "NATIVE3D_STAGE", "stage": "split_surface_exit", "rank": 0,
         "phase": "construction", "split_lane": "1", "elapsed": "3", "accepted": "5", "source_line": 2},
        {"record": "NATIVE3D_STAGE", "stage": "exit", "rank": 0,
         "phase": "improvement", "elapsed": "5", "accepted": "8", "source_line": 3},
        {"record": "NATIVE3D_STAGE", "stage": "construction_entry", "rank": 0,
         "phase": "construction", "split_lane": "1", "elapsed": "0.5", "accepted": "1", "source_line": 4},
        {"record": "NATIVE3D_STAGE", "stage": "split_surface_exit", "rank": 0,
         "phase": "construction", "split_lane": "1", "elapsed": "2", "accepted": "4", "source_line": 5},
    ]
    intervals = make_intervals(stages)
    assert intervals[0]["label"] == "surface" and intervals[0]["wall_interval_s"] == 2
    assert intervals[0]["counter_deltas"]["accepted"] == 3
    assert intervals[1]["event"] == 0 and intervals[2]["event"] == 1
    assert intervals[-1]["label"] == "surface"
    q = [
        {"record": "NATIVE3D_STAGE", "stage": "quality_entry", "rank": 0, "elapsed": "1", "source_line": 1},
        {"record": "NATIVE3D_STAGE", "stage": "quality_exit", "rank": 0, "elapsed": "3", "source_line": 2},
        {"record": "NATIVE3D_STAGE", "stage": "exit", "rank": 0, "elapsed": "4", "source_line": 3},
    ]
    assert [x["label"] for x in make_intervals(q)] == ["quality", "finish"]
    coverage = event_coverage([{"record": "NATIVE3D_DETAIL", "stage": "refine",
                                "rank": "0", "total": "10", "other": "2"}])
    assert coverage[0]["coverage_fraction"] == 0.8


def main(case_dir):
    case = Path(case_dir).resolve()
    profile = case / "profile"
    logs = sorted(profile.glob("rank*.program.log"))
    rows = []
    per_rank = {}
    for log in logs:
        match = re.search(r"rank(\d+)\.program\.log$", log.name)
        fallback = int(match.group(1)) if match else 0
        parsed = read_program_log(log, fallback)
        rows.extend(parsed)
        per_rank[fallback] = len(parsed)
    traces, trace_paths = [], []
    ranks = set(per_rank)
    if not ranks:
        ranks = {0}
    for rank in sorted(ranks):
        trace = case / f"stage_trace_rank{rank}.jsonl"
        if not trace.exists():
            trace = profile / trace.name
        if trace.exists():
            trace_paths.append(str(trace))
        traces.extend(load_jsonl(trace))
    bl_path = case / "actual_bl_surface_stage.json"
    if not bl_path.exists():
        bl_path = profile / bl_path.name
    bl = actual_bl_records(bl_path)
    inputs = [str(p) for p in logs] + trace_paths + ([str(bl_path)] if bl_path.exists() else [])
    summary = {"case_dir": str(case), "input_files": inputs,
               "program_log_record_counts": {str(k): v for k, v in per_rank.items()},
               "event_timer_coverage": event_coverage(rows),
               "adjacent_marker_intervals": make_intervals(rows),
               "quality_intervals": [x for x in make_intervals(rows) if x["label"] == "quality"],
               "quality_events": [r for r in rows if r.get("record") == "NATIVE3D_QUALITY"],
               "stage_trace_events": traces,
               "phase_ownership_events": [r for r in rows if r.get("record") == "NATIVE3D_PHASE_OWNERSHIP"],
               "length_events": [r for r in rows if r.get("record") == "NATIVE3D_LENGTH"],
               "actual_bl_surface_stage_records": bl,
               "limitations": ["collective_calls are tracked collectives, not literal world exchanges",
                               "no exact per-rank per-phase CPU or wall-wait values are inferred",
                               "election/collective seconds are nested; do not add to exclusive timer buckets",
                               "adjacent marker intervals are disjoint elapsed-time bins; label follows emitted boundary semantics"]}
    out = case / "timers_summary.json"
    out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    print(out)


if __name__ == "__main__":
    self_check()
    if len(sys.argv) != 2:
        raise SystemExit("usage: summarize_timers.py CASE_DIR")
    main(sys.argv[1])
