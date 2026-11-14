#!/usr/bin/env python3
"""Analyze Run B engine markers against profiler samples with epoch uncertainty.

Usage: python3 analyze_B.py CASE_DIR
Reads CASE_DIR/profile/rankN.program.log, rankN.engine_clocks.jsonl,
rankN.exec.json and rankN.samples.txt; writes CASE_DIR/run_B_phase_samples.json.
Only profiler sample rows are streamed; no sample-row list is retained.
"""
import bisect
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from summarize_samples import GROUPS, ROW, group


STAGE_PREFIX = "NATIVE3D_STAGE"
DETAIL_PREFIX = "NATIVE3D_DETAIL"
COUNTERS = ("accepted", "rounds", "length_splits", "length_collapses",
            "election_calls", "election_seconds", "collective_calls", "collective_seconds",
            "surface_seconds", "interior_seconds", "upper_seconds", "upper_calls", "guidance_seconds")
ENGINE_EPOCH_KEYS = ("epoch_ns", "actual_engine_epoch_monotonic_ns", "engine_epoch_monotonic_ns",
                     "adapt_epoch_monotonic_ns", "timer_epoch_monotonic_ns")


def value(row, key, default=None):
    raw = row.get(key, default)
    try:
        return float(raw)
    except (TypeError, ValueError):
        return None


def parse_fields(line):
    parts = line.strip().split()
    if not parts:
        return None
    row = {"record": parts[0]}
    for token in parts[1:]:
        if "=" in token:
            k, v = token.split("=", 1)
            row[k] = v.rstrip(",")
    return row


def read_log(path):
    stages, details = [], []
    if not path.exists():
        return stages, details
    with path.open(errors="replace") as src:
        for line_no, line in enumerate(src, 1):
            if not (line.startswith(STAGE_PREFIX) or line.startswith(DETAIL_PREFIX)):
                continue
            row = parse_fields(line)
            row["source_line"] = line_no
            if row["record"] == STAGE_PREFIX and value(row, "elapsed") is not None:
                row["elapsed"] = value(row, "elapsed")
                row["rank"] = int(value(row, "rank", 0))
                stages.append(row)
            elif row["record"] == DETAIL_PREFIX and row.get("stage") in ("partition", "refine"):
                row["rank"] = int(value(row, "rank", 0))
                details.append(row)
    return stages, details


def split_invocations(stages):
    """Preserve program order; exit and elapsed reset start the next event."""
    result, current, previous_elapsed = [], [], None
    for row in stages:
        elapsed = row["elapsed"]
        if current and (current[-1].get("stage") == "exit" or elapsed < previous_elapsed):
            result.append(current)
            current = []
        current.append(row)
        previous_elapsed = elapsed
    if current:
        result.append(current)
    return result


def stage_counter(row, key):
    if row.get("stage") == "engine_start":
        return 0.0
    return value(row, key)


def lane_of(row):
    lane = value(row, "split_lane", 0)
    return "surface" if lane == 1 else "interior" if lane == 2 else "other"


def intervals_for_invocation(rows, invocation):
    """Return disjoint phase gaps; epochs here remain relative seconds."""
    baseline = {"record": STAGE_PREFIX, "stage": "engine_start", "phase": "construction",
                "split_lane": 0, "elapsed": 0.0, "source_line": 0}
    timeline = [baseline] + rows
    intervals = []
    mode = "collapse"
    for idx, (left, right) in enumerate(zip(timeline, timeline[1:])):
        t0, t1 = left["elapsed"], right["elapsed"]
        if idx == 0:
            label = "initialization_collapse" if right.get("stage") == "length_collapse_exit" else "initialization"
            note = "cumulative initialization+collapse" if label == "initialization_collapse" else "initialization before first marker"
        elif right.get("stage") == "ownership_staged":
            label, note = "phase_setup_repartition_staging", "ownership staging boundary; not a pure repartition timer"
        elif left.get("stage") == "ownership_staged" and right.get("stage") == "ownership_committed":
            label, note = "repartition_commit", "ownership staged-to-committed interval; separate partition total remains authoritative"
        elif left.get("stage") == "quality_exit" and right.get("stage") == "exit":
            label, note = "finish", "strict quality_exit-to-exit interval"
        elif left.get("stage") == "quality_exit":
            label, note = "transition", "quality_exit not followed immediately by exit"
        elif right.get("stage") == "exit" and right.get("phase") == "construction":
            active_lane = lane_of(right)
            label = ("surface_with_setup_until_failed_return" if active_lane == "surface" and left.get("stage") == "length_split_exit"
                     else active_lane + "_until_failed_return" if active_lane in ("surface", "interior") else "unresolved_setup_until_failed_return")
            note = "unfinished construction lane from exit state; generation setup included after length_split_exit"
        elif mode == "after_quality":
            label, note = "transition", "post-quality work; no immediate quality_exit-to-exit marker pair"
        elif right.get("stage") == "split_surface_exit" and lane_of(right) == "surface" and (
                left.get("stage") in ("length_split_exit", "ownership_committed") or
                left.get("stage") == "ownership_staged"):
            label, note = "surface_with_setup", "surface generation/setup bounded by prior checkpoint and surface exit"
        elif right.get("stage") == "exit" and (mode == "quality" or left.get("phase") == "improvement"):
            label, note = "quality_until_failed_return", "improvement ended at exit without a quality_exit marker"
        elif left.get("stage") in ("split_surface_exit", "split_interior_exit"):
            label, note = "transition", "lane exit to next lane/phase boundary"
        elif left.get("stage") == "ownership_committed":
            label, note = "transition", "ownership marker; not a pure repartition interval"
        elif left.get("phase") == "improvement" or mode == "quality":
            label, note = "quality", "improvement phase / quality window"
        elif mode == "collapse":
            label, note = "collapse", "construction collapse interval"
        else:
            lane = lane_of(left)
            label = lane if lane in ("surface", "interior") else mode
            note = "split lane from NATIVE3D_STAGE split_lane" if label in ("surface", "interior") else "not a named pure phase"

        deltas = {}
        for key in COUNTERS:
            a, b = stage_counter(left, key), value(right, key)
            if a is not None and b is not None:
                deltas[key] = b - a
        intervals.append({"invocation": invocation, "phase": label, "semantics": note,
                          "start_elapsed_s": t0, "end_elapsed_s": t1,
                          "wall_interval_s": t1 - t0,
                          "start_marker": left.get("stage"), "end_marker": right.get("stage"),
                          "counter_deltas": deltas})

        stage = right.get("stage")
        if stage == "quality_entry":
            mode = "quality"
        elif stage == "quality_exit":
            mode = "after_quality"
        elif stage == "length_collapse_exit":
            mode = "other"
        elif stage in ("split_surface_exit", "split_interior_exit", "ownership_staged", "ownership_committed"):
            mode = "transition"
        elif stage == "split_surface_resume":
            mode = "surface"
        elif stage == "split_interior_entry":
            mode = "interior"
        elif stage == "construction_entry":
            mode = "collapse" if lane_of(right) == "other" else lane_of(right)
        elif right.get("phase") == "improvement":
            mode = "quality"
        elif lane_of(right) in ("surface", "interior"):
            mode = lane_of(right)
        elif mode == "after_quality":
            mode = "after_quality"
    return intervals


def epochs_from_rows(rows):
    groups = {}
    for row in rows:
        try:
            adapt = int(row.get("adaptation", 1))
        except (TypeError, ValueError):
            continue
        if adapt <= 0:
            continue
        groups.setdefault(adapt, []).append(row)
    result = []
    for adapt in sorted(groups):
        events = groups[adapt]
        events.sort(key=lambda x: x.get("monotonic_ns", 0))
        entry = next((int(row["monotonic_ns"]) for row in events if row.get("event") == "AdaptMetric_entry"), None)
        if entry is None:
            continue
        exact = next((int(row[key]) for row in events for key in ENGINE_EPOCH_KEYS if row.get(key) is not None), None)
        unavailable = next((int(row["monotonic_ns"]) for row in events
                            if row.get("event") == "Adapt_timer_epoch_unavailable"), None)
        if exact is not None:
            result.append({"adaptation": adapt, "epoch_lower_ns": exact, "epoch_upper_ns": exact,
                           "epoch_source": "direct", "bracket_width_ns": 0})
        elif entry is not None and unavailable is not None and entry <= unavailable:
            result.append({"adaptation": adapt, "epoch_lower_ns": entry, "epoch_upper_ns": unavailable,
                           "epoch_source": "AdaptMetric_entry <= actual start <= Adapt_timer_epoch_unavailable",
                           "bracket_width_ns": unavailable - entry})
        else:
            result.append({"adaptation": adapt, "epoch_lower_ns": None, "epoch_upper_ns": None,
                           "epoch_source": "unavailable", "bracket_width_ns": None})
    return result


def clock_epochs(path):
    rows = []
    if path.exists():
        with path.open(errors="replace") as src:
            for line in src:
                try:
                    row = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if isinstance(row, dict):
                    rows.append(row)
    return epochs_from_rows(rows)


def guaranteed_sample_window(epoch, interval):
    """For epoch in [L,U], only [U+t0,L+t1) is unambiguously inside phase."""
    if epoch["epoch_lower_ns"] is None:
        return None
    lo = epoch["epoch_upper_ns"] + interval["start_elapsed_s"] * 1e9
    hi = epoch["epoch_lower_ns"] + interval["end_elapsed_s"] * 1e9
    return (lo, hi) if lo < hi else None


def self_check():
    bracket = {"epoch_lower_ns": 100_000_000_000, "epoch_upper_ns": 102_000_000_000}
    interval = {"start_elapsed_s": 5.0, "end_elapsed_s": 10.0}
    assert guaranteed_sample_window(bracket, interval) == (107_000_000_000.0, 110_000_000_000.0)
    assert 108_000_000_000.0 >= guaranteed_sample_window(bracket, interval)[0]
    assert 110_000_000_000.0 == guaranteed_sample_window(bracket, interval)[1]
    records = [{"stage": "length_collapse_exit", "elapsed": 4.0},
               {"stage": "exit", "elapsed": 8.0},
               {"stage": "construction_entry", "elapsed": 0.2}]
    split = split_invocations(records)
    assert len(split) == 2 and split[1][0]["elapsed"] == 0.2
    clock_rows = [
        {"event": "process_starti", "adaptation": 0, "monotonic_ns": 10},
        {"event": "AdaptMetric_entry", "adaptation": 1, "monotonic_ns": 100},
        {"event": "Adapt_timer_epoch_unavailable", "adaptation": 1, "monotonic_ns": 110},
    ]
    epochs = epochs_from_rows(clock_rows)
    assert len(epochs) == 1 and epochs[0]["adaptation"] == 1
    assert epochs[0]["epoch_lower_ns"] == 100 and epochs[0]["epoch_upper_ns"] == 110
    exact_rows = [
        {"event": "AdaptMetric_entry", "adaptation": 1, "monotonic_ns": 200},
        {"event": "Adapt_timer_epoch_unavailable", "adaptation": 1, "monotonic_ns": 210, "epoch_ns": 205},
    ]
    exact = epochs_from_rows(exact_rows)[0]
    assert exact["epoch_source"] == "direct" and exact["epoch_lower_ns"] == exact["epoch_upper_ns"] == 205
    quality_rows = [
        {"stage": "quality_entry", "phase": "improvement", "elapsed": 1.0, "split_lane": 0},
        {"stage": "quality_exit", "phase": "improvement", "elapsed": 5.0, "split_lane": 0},
        {"stage": "construction_entry", "phase": "construction", "elapsed": 6.0, "split_lane": 1},
    ]
    assert intervals_for_invocation(quality_rows, 0)[-1]["phase"] == "transition"
    quality_rows[-1] = {"stage": "exit", "phase": "improvement", "elapsed": 6.0, "split_lane": 0}
    assert intervals_for_invocation(quality_rows, 0)[-1]["phase"] == "finish"
    failed_quality = [
        {"stage": "quality_entry", "phase": "improvement", "elapsed": 1.0, "split_lane": 0},
        {"stage": "window", "phase": "improvement", "elapsed": 5.0, "split_lane": 0},
        {"stage": "exit", "phase": "improvement", "elapsed": 6.0, "split_lane": 0},
    ]
    assert intervals_for_invocation(failed_quality, 0)[-1]["phase"] == "quality_until_failed_return"
    lanes = [
        {"stage": "length_split_exit", "phase": "construction", "split_lane": 2,
         "elapsed": 1680.0446977, "surface_seconds": 10, "interior_seconds": 20},
        {"stage": "split_surface_exit", "phase": "construction", "split_lane": 1,
         "elapsed": 2577.249299, "surface_seconds": 907.2046013, "interior_seconds": 20},
        {"stage": "split_interior_entry", "phase": "construction", "split_lane": 2,
         "elapsed": 2577.3, "surface_seconds": 907.2046013, "interior_seconds": 20},
        {"stage": "split_interior_exit", "phase": "construction", "split_lane": 2,
         "elapsed": 2600.0, "surface_seconds": 907.2046013, "interior_seconds": 42.7},
        {"stage": "length_split_exit", "phase": "construction", "split_lane": 2,
         "elapsed": 2600.1, "surface_seconds": 907.2046013, "interior_seconds": 42.8},
    ]
    lane_intervals = intervals_for_invocation(lanes, 0)
    setup_gap = next(row for row in lane_intervals if row["start_marker"] == "length_split_exit")
    assert setup_gap["phase"] == "surface_with_setup"
    assert abs(setup_gap["wall_interval_s"] - 897.2046013) < 1e-9
    lane_handoff = next(row for row in lane_intervals if row["start_marker"] == "split_surface_exit")
    assert lane_handoff["phase"] == "transition"
    assert next(row for row in lane_intervals if row["start_marker"] == "split_interior_entry")["phase"] == "interior"
    plain_surface = [
        {"stage": "construction_entry", "phase": "construction", "split_lane": 1, "elapsed": 1.0},
        {"stage": "split_surface_exit", "phase": "construction", "split_lane": 1, "elapsed": 3.0},
    ]
    assert intervals_for_invocation(plain_surface, 0)[-1]["phase"] == "surface"
    ownership = [
        {"stage": "construction_entry", "phase": "construction", "split_lane": 1, "elapsed": 0.5},
        {"stage": "ownership_staged", "phase": "construction", "split_lane": 0, "elapsed": 1.0},
        {"stage": "ownership_committed", "phase": "improvement", "split_lane": 0, "elapsed": 4.0},
    ]
    ownership_intervals = intervals_for_invocation(ownership, 0)
    assert ownership_intervals[1]["phase"] == "phase_setup_repartition_staging"
    assert ownership_intervals[2]["phase"] == "repartition_commit"


def pid_for_rank(profile, rank):
    meta_path = profile / f"rank{rank}.exec.json"
    if meta_path.exists():
        try:
            meta = json.loads(meta_path.read_text())
            if meta.get("pid") is not None:
                return int(meta["pid"])
        except (OSError, json.JSONDecodeError, ValueError):
            pass
    clocks_path = profile / f"rank{rank}.engine_clocks.jsonl"
    if clocks_path.exists():
        with clocks_path.open(errors="replace") as src:
            for line in src:
                try:
                    row = json.loads(line)
                    if row.get("pid") is not None:
                        return int(row["pid"])
                except (json.JSONDecodeError, ValueError):
                    continue
    return None


def analyze_rank(profile, rank):
    log = profile / f"rank{rank}.program.log"
    stages, details = read_log(log)
    invocations = split_invocations(stages)
    clocks_path = profile / f"rank{rank}.engine_clocks.jsonl"
    epochs = clock_epochs(clocks_path)
    sample_file = profile / f"rank{rank}.samples.txt"
    pid = pid_for_rank(profile, rank)

    all_intervals = []
    event_results = []
    for number, rows in enumerate(invocations):
        intervals = intervals_for_invocation(rows, number)
        epoch_by_adaptation = {int(row["adaptation"]): row for row in epochs}
        epoch = epoch_by_adaptation.get(number + 1, {"adaptation": number + 1,
            "epoch_lower_ns": None, "epoch_upper_ns": None,
            "epoch_source": "unavailable", "bracket_width_ns": None})
        for interval in intervals:
            interval["rank"] = rank
            interval["epoch_source"] = epoch["epoch_source"]
            interval["epoch_bracket_width_s"] = (epoch["bracket_width_ns"] / 1e9
                if epoch["bracket_width_ns"] is not None else None)
            window = guaranteed_sample_window(epoch, interval)
            interval["assigned_sample_window_monotonic_s"] = [x / 1e9 for x in window] if window else None
            interval["sample_groups"] = {g: 0 for g in GROUPS}
            interval["assigned_samples"] = 0
            interval["sample_resolution"] = "no unambiguous time window" if not window else None
            if window:
                all_intervals.append((window[0] / 1e9, window[1] / 1e9, interval))
        event_results.append({"rank": rank, "invocation": number, "epoch": epoch,
                              "first_stage": rows[0].get("stage") if rows else None,
                              "elapsed_exit_s": rows[-1].get("elapsed") if rows else None,
                              "intervals": intervals})

    all_intervals.sort(key=lambda row: row[0])
    starts = [row[0] for row in all_intervals]
    outside_pid_or_bad_rows = outside_window = ambiguous = assigned_total = 0
    sample_total_by_group = Counter()
    if sample_file.exists():
        with sample_file.open(errors="replace") as src:
            for line in src:
                match = ROW.match(line)
                if not match:
                    continue
                sample_pid, _tid, stamp, _ip, symbol, dso = match.groups()
                if pid is not None and int(sample_pid) != pid:
                    outside_pid_or_bad_rows += 1
                    continue
                stamp = float(stamp)
                index = bisect.bisect_right(starts, stamp) - 1
                if index >= 0 and stamp < all_intervals[index][1]:
                    item = all_intervals[index][2]
                    category = group(symbol, dso)
                    item["sample_groups"][category] += 1
                    item["assigned_samples"] += 1
                    sample_total_by_group[category] += 1
                    assigned_total += 1
                else:
                    possible = False
                    for event in event_results:
                        epoch = event["epoch"]
                        if epoch["epoch_lower_ns"] is None:
                            continue
                        last = event["elapsed_exit_s"] or 0.0
                        low = epoch["epoch_lower_ns"] / 1e9
                        high = (epoch["epoch_upper_ns"] + last * 1e9) / 1e9
                        if low <= stamp < high:
                            possible = True; break
                    if possible:
                        ambiguous += 1
                    else:
                        outside_window += 1

    for _start, _end, interval in all_intervals:
        total = interval["assigned_samples"]
        interval["sample_resolution"] = "not resolved" if total < 100 else "resolved"
        interval["sample_group_shares"] = {
            name: {"samples": count, "percent": (100 * count / total if total else None),
                   "resolution": "not resolved" if count < 100 else "resolved"}
            for name, count in interval["sample_groups"].items()}
    total = sum(sample_total_by_group.values())
    return {"rank": rank, "pid": pid, "program_log": str(log), "sample_file": str(sample_file),
            "engine_epochs": epochs, "invocations": event_results,
            "sample_assignment": {"assigned_samples": assigned_total,
                                  "unambiguously_assigned_group_shares": {
                                      name: {"samples": n, "percent": 100*n/total if total else None,
                                             "resolution": "not resolved" if n < 100 else "resolved"}
                                      for name, n in sample_total_by_group.items()},
                                  "boundary_ambiguous_samples": ambiguous,
                                  "outside_engine_window_samples": outside_window,
                                  "other_pid_samples": outside_pid_or_bad_rows},
            "partition_and_aggregate_details": details}


def main(case_dir):
    case = Path(case_dir).resolve()
    profile = case / "profile"
    ranks = sorted(int(m.group(1)) for p in profile.glob("rank*.program.log")
                   if (m := re.match(r"rank(\d+)\.program\.log$", p.name)))
    output = {"case_dir": str(case), "method": "On-CPU perf samples binned only inside guaranteed epoch-bracket intervals",
              "ranks": [analyze_rank(profile, rank) for rank in ranks],
              "limitations": ["Sample shares describe sampled on-CPU time, not wall wait or slowest-rank causal attribution.",
                             "For epoch bracket [L,U], only samples in [U+t0,L+t1) are assigned to phase [t0,t1).",
                             "Samples in possible engine time but outside guaranteed intervals are boundary-ambiguous and excluded.",
                             "Split-lane gaps and ownership staging/commit gaps are transitions, not pure repartition phases.",
                             "Partition DETAIL totals remain separate and are not mapped onto sample phases.",
                             "Counts under 100 are marked not resolved; DSO-aware MPI classification comes from summarize_samples.group."]}
    path = case / "run_B_phase_samples.json"
    path.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    print(path)


if __name__ == "__main__":
    self_check()
    if len(sys.argv) != 2:
        raise SystemExit("usage: analyze_B.py CASE_DIR")
    main(sys.argv[1])
