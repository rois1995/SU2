#!/usr/bin/env python3
"""Compare the full six-rank FX2 workflows through the final timer snapshot."""

import argparse
import json
import runpy
from pathlib import Path

NIGHT = Path(__file__).resolve().parent.parent
PREFIX = runpy.run_path(str(NIGHT / "N22/compare_prefix.py"))
PARSER = PREFIX["PARSER"]
C_FIELDS = PREFIX["C_FIELDS"]
REFERENCE = NIGHT / "FP2_B/G5/build640"
STORAGE = "NATIVE3D_STORAGE "
SURFACE = "NATIVE3D_LOCAL_SURFACE_STATE "


def scan(path, rank):
    """Read modeled storage, surface state, and C-field observations to EOF."""
    storage, surface, counters = [], [], []
    with path.open(errors="replace") as stream:
        for number, raw in enumerate(stream, 1):
            line = PARSER.N4.strip_mpi_prefixes(raw)
            if not line.startswith(("NATIVE3D_", "NATIVE3D ")):
                continue
            marker = line.partition(" ")[0] + " "
            if marker not in (STORAGE, SURFACE) and not any(key + "=" in line for key in C_FIELDS):
                continue
            fields = PARSER.fields_after(line, marker)
            if fields.get("rank") != str(rank):
                continue
            if marker == STORAGE:
                storage.append(fields)
            if marker == SURFACE:
                surface.append(fields)
            observation = {key: fields[key] for key in sorted(C_FIELDS & fields.keys())}
            if observation:
                counters.append({"line": number, "marker": marker.strip(), "fields": observation})
    return storage, surface, counters


def different_fields(old, new):
    return {key: [old.get(key), new.get(key)]
            for key in sorted(old.keys() | new.keys()) if old.get(key) != new.get(key)}


def ordered_differences(kind, before, after):
    if len(before) != len(after):
        yield {"kind": kind, "counts": [len(before), len(after)]}
    for index, (old, new) in enumerate(zip(before, after), 1):
        if kind == "timer":
            wrong_storage = [row["fixedstorage"] for row in (old, new)]
            if any(value != "4120" for value in wrong_storage):
                yield {"kind": kind, "row": index, "fixedstorage": wrong_storage, "expected": "4120"}
        differences = different_fields(old, new)
        if kind == "timer" and "calls" in differences:
            differences["calls"] = different_fields(*differences["calls"])
        if differences:
            yield {"kind": kind, "row": index, "fields": differences}


def timer_snapshot(snapshot, rank):
    summary = snapshot["summary"]
    path, line = snapshot["path"], snapshot["line"]
    assert summary["rank"] == str(rank) and summary["ranks"] == "6", (path, line, summary)
    calls, measurements, communication = {}, {}, {}
    for row in snapshot["rows"]:
        fields = row["fields"]
        operation = fields["operation"]
        name = fields["phase"] + "/" + operation
        assert name not in calls, (path, row["line"], name)
        count = PARSER.parse_calls(fields, path, row["line"])
        measurements[name] = {key: float(fields[key]) for key in sorted(PARSER.TIMER_MEASUREMENTS)}
        calls[name] = count if operation != "communication" else "report only"
        if operation == "communication":
            communication[name] = count
    exact = {key: value for key, value in summary.items() if key not in PARSER.SUMMARY_MEASUREMENTS}
    observed = {key: float(summary[key]) for key in sorted(PARSER.SUMMARY_MEASUREMENTS)}
    return {**exact, "calls": calls}, {
        **exact, **observed, "operations": measurements, "communication_calls": communication,
    }


def parse_case(case):
    files = PARSER.source_files(case)
    expected_ranks = set(range(6))
    assert len(files) == 6 and {PARSER.stream_rank(files, label) for label in files} == expected_ranks, case
    by_rank = {}
    for label, path in files.items():
        rank = PARSER.stream_rank(files, label)
        snapshots = [timer_snapshot(snapshot, rank) for snapshot in PARSER.parse_timer_stream(path)]
        assert snapshots, path
        exact_timers, timer_measurements = zip(*snapshots, strict=True)
        storage, surface, counters = scan(path, rank)
        assert storage, path
        exact_surface = [{key: value for key, value in row.items()
                          if not key.startswith("seconds_") and key not in ("rss_before", "rss_after")}
                         for row in surface]
        observed_surface = [{key: value for key, value in row.items() if key not in exact}
                            for row, exact in zip(surface, exact_surface, strict=True)]
        by_rank[rank] = {
            "exact": {"timer": exact_timers, "storage": storage, "surface_state": exact_surface},
            "report_only": {"timer": timer_measurements, "C_FIELDS": counters, "surface_state": observed_surface},
        }
    return by_rank


def replay_checks(measurements, mode):
    phases = ("transfer", "construction") if mode == "s" else ("transfer",)
    events = [row for row in measurements if row["root"] == "remesh" and row["boundary"] == "final"]
    if len(events) != 2:
        yield {"kind": "replay", "final_remesh_snapshots": len(events), "expected": 2}
    for event, row in enumerate(events, 1):
        for phase in phases:
            seconds = row["operations"].get(phase + "/mobility_policy", {}).get("exclusive_seconds")
            if seconds is None or not 0 <= seconds < 1:
                yield {"kind": "replay", "event": event, "phase": phase, "exclusive_seconds": seconds}


def compare_cases(reference, candidate, mode=None):
    before, after = parse_case(reference), parse_case(candidate)
    differences, counts, measurements = [], {}, {}
    for rank, old in sorted(before.items()):
        new = after[rank]
        counts[rank] = {}
        for kind, old_rows in old["exact"].items():
            new_rows = new["exact"][kind]
            counts[rank][kind] = [len(old_rows), len(new_rows)]
            differences.extend({"rank": rank, **row} for row in ordered_differences(kind, old_rows, new_rows))
        measurements[rank] = {"reference": old["report_only"], "candidate": new["report_only"]}
    replay_differences = [] if mode is None else [
        {"rank": rank, **row}
        for rank, entry in measurements.items()
        for row in replay_checks(entry["candidate"]["timer"], mode)
    ]
    failures = differences + replay_differences
    return {
        "passed": not failures, "reference": str(reference), "candidate": str(candidate), "mode": mode,
        "first_difference": failures[0] if failures else None, "exact_difference_count": len(differences),
        "counts_by_rank": counts, "exact_differences": differences, "replay_differences": replay_differences,
        "report_only_by_rank": measurements,
    }


def self_check():
    result = compare_cases(REFERENCE / "S1/full640_run1", REFERENCE / "S2/full640_run1")
    expected = {"timer": [16, 16], "storage": [69, 69], "surface_state": [2, 2]}
    assert result["passed"] and all(counts == expected for counts in result["counts_by_rank"].values()), {
        key: result[key] for key in ("counts_by_rank", "first_difference")
    }
    return {key: value for key, value in result.items() if key != "report_only_by_rank"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", nargs="?", type=Path)
    parser.add_argument("candidate", nargs="?", type=Path)
    parser.add_argument("--mode", choices=("off", "s"), help="also enforce the FX2 replay timing gate")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        result = self_check()
    else:
        if args.reference is None or args.candidate is None:
            parser.error("reference and candidate case paths are required")
        result = compare_cases(args.reference, args.candidate, args.mode)
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({key: result[key] for key in ("passed", "exact_difference_count", "first_difference",
                                                   "counts_by_rank")}))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
