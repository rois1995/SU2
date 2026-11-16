#!/usr/bin/env python3
"""Extract two-event D9 counter and timing reports from completed N5 rank logs."""

import argparse
import csv
import hashlib
import json
import math
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
N5 = HERE.parent
WORKFLOW = N5 / "full_workflow"
SITES = ("a", "b", "c", "d", "e", "g")
PHASES = ("quality", "final_proof")
PARTS = ("other", "export", "projection", "move", "guidance")
MARKER = re.compile(r"D9_(QUERY|MOVE_PART|MOVE)\s+(.+)")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_fields(payload):
    fields = {}
    for token in payload.split():
        if "=" in token:
            key, value = token.split("=", 1)
            fields[key] = value
    return fields


def marker_row(line):
    match = MARKER.search(line)
    if not match:
        return None
    return match.group(1), parse_fields(match.group(2))


def number(fields, key, integer=False):
    assert key in fields, f"missing counter field {key}"
    value = int(fields[key]) if integer else float(fields[key])
    assert math.isfinite(value) and value >= 0, f"invalid {key}: {fields[key]}"
    return value


def expected_query_keys():
    return {(phase, site) for phase in PHASES for site in SITES}


def validate_event(event, rank, path):
    queries = event["queries"]
    keys = [(row["phase"], row["site"]) for row in queries]
    assert len(keys) == 12 and set(keys) == expected_query_keys(), (path, rank, keys)
    assert len(set(keys)) == len(keys), f"duplicate D9_QUERY row: {path} rank {rank}"
    assert event["move"] is not None, f"missing D9_MOVE: {path} rank {rank}"
    parts = event["parts"]
    assert set(parts) == set(PARTS) and len(parts) == 5, (path, rank, sorted(parts))
    assert event["move"]["rank"] == str(rank)
    for row in queries:
        assert row["rank"] == str(rank)
        assert row["phase"] in PHASES and row["site"] in SITES
        for key in ("scopes", "direct_calls", "upper_calls", "classification_calls", "magnitude_calls",
                    "limit_calls", "memo_direct_hits", "memo_union_hits", "upper_work"):
            number(row, key, integer=True)
        for key in ("seconds", "direct_seconds", "upper_seconds"):
            number(row, key)
    for part, row in parts.items():
        assert row["rank"] == str(rank) and row["part"] == part
        number(row, "calls", integer=True)
        number(row, "inclusive_seconds")
        number(row, "qj_seconds")
    for key in ("stars", "candidates", "shared_qj_calls", "qj_calls"):
        number(event["move"], key, integer=True)
    for key in ("star_seconds", "candidate_seconds", "shared_qj_seconds"):
        number(event["move"], key)


def parse_rank(path, rank):
    digest = sha256(path)
    events = []
    current = None
    for line_number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
        item = marker_row(line)
        if item is None:
            continue
        kind, fields = item
        if fields.get("rank") != str(rank):
            continue
        row = dict(fields, source_file=str(path), source_line=line_number, source_sha256=digest)
        if kind == "QUERY":
            if current is None or (len(current["queries"]) == 12 and current["move"] is not None and len(current["parts"]) == 5):
                current = {"queries": [], "move": None, "parts": {}}
                events.append(current)
            current["queries"].append(row)
        elif kind == "MOVE":
            assert current is not None and current["move"] is None, f"duplicate/orphan D9_MOVE at {path}:{line_number}"
            current["move"] = row
        else:
            assert current is not None, f"orphan D9_MOVE_PART at {path}:{line_number}"
            part = fields.get("part")
            assert part in PARTS and part not in current["parts"], f"duplicate/unknown D9_MOVE_PART at {path}:{line_number}"
            current["parts"][part] = row
    assert len(events) == 2, f"{path}: expected 2 event report groups, got {len(events)}"
    for event in events:
        validate_event(event, rank, path)
    return events, {"path": str(path), "sha256": digest}


def accounting(event, rank, event_index):
    move = event["move"]
    parts = event["parts"]
    inc = {name: number(parts[name], "inclusive_seconds") for name in PARTS}
    qj = {name: number(parts[name], "qj_seconds") for name in PARTS}
    candidates = int(move["candidates"])
    candidate_seconds = float(move["candidate_seconds"])
    export = inc["export"]
    shared = float(move["shared_qj_seconds"])
    residual = candidate_seconds - inc["projection"] - inc["move"] - inc["guidance"] - qj["other"]
    star_residual = float(move["star_seconds"]) - candidate_seconds - export - shared
    return {
        "event": event_index, "rank": rank, "stars_including_early_returns": int(move["stars"]),
        "candidates": candidates, "candidate_seconds": candidate_seconds,
        "inclusive_seconds_by_part": inc, "qj_seconds_by_part": qj,
        "projection_minus_qj": inc["projection"] - qj["projection"],
        "move_minus_qj": inc["move"] - qj["move"],
        "guidance_minus_qj": inc["guidance"] - qj["guidance"],
        "qj_total": sum(qj.values()), "qj_other": qj["other"], "qj_export": qj["export"],
        "qj_export_unexpected": qj["export"] != 0,
        "candidate_residual_includes_qj_export": qj["export"] != 0,
        "candidate_residual_after_projection_move_guidance_and_other_qj": residual,
        "export_seconds_raw": export,
        "export_seconds_amortized_per_candidate": export / candidates if candidates else None,
        "shared_qj_seconds": shared,
        "shared_qj_seconds_per_candidate": shared / candidates if candidates else None,
        "all_non_candidate_star_residual": star_residual,
        "negative_residual_overlap_warning": star_residual < 0 or residual < 0,
        "source_file": move["source_file"], "source_line": move["source_line"],
        "source_sha256": move["source_sha256"],
        "interpretation": "negative residual indicates overlap or nonseparable inclusive scopes; not an exclusive partition",
    }


def collect(workflow, ranks):
    raw, provenance, summaries = [], {}, []
    for rank in range(ranks):
        path = workflow / "rank_output" / "1" / f"rank.{rank}" / "stdout"
        events, source = parse_rank(path, rank)
        provenance[str(rank)] = source
        for event_index, event in enumerate(events, 1):
            for row in event["queries"]:
                raw.append(dict(row, event=event_index, record_type="query"))
            raw.append(dict(event["move"], event=event_index, record_type="move"))
            raw.extend(dict(row, event=event_index, record_type="move_part") for row in event["parts"].values())
            summaries.append(accounting(event, rank, event_index))
    assert len(summaries) == ranks * 2
    return raw, summaries, provenance


def write_csv(path, rows):
    fields = ["event", "record_type", "rank", "phase", "site", "part", "scopes", "direct_calls",
              "upper_calls", "classification_calls", "magnitude_calls", "limit_calls", "memo_direct_hits",
              "memo_union_hits", "upper_work", "seconds", "direct_seconds", "upper_seconds", "stars",
              "candidates", "star_seconds", "candidate_seconds", "shared_qj_calls", "shared_qj_seconds",
              "qj_calls", "calls", "inclusive_seconds", "qj_seconds", "source_file", "source_line", "source_sha256"]
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def report_markdown(path, summaries, provenance, workflow, raw):
    lines = ["# M16 D9 raw counter extraction", "",
             "No actual numbers are embedded in this helper; this report is generated only from the supplied workflow logs.",
             "Counts remain separated by phase and site. Site `d` is inclusive and may nest `b`/`c`; site counts are not summed as an exclusive partition.",
             "Query `seconds` is the instrumented function scope, not pure backend time. `MetricAt` argument lookup is outside kernel QJ timing; the candidate residual is reported explicitly.",
             "Work part `other` has inclusive time 0 by design while its QJ time is real; no QJ value is subtracted from that zero inclusive value.",
             "Star counts include early-return searches. Negative residuals indicate overlap or nonseparable inclusive scopes.", "",
             f"Workflow: `{workflow}`", "", "## Event/rank accounting", "",
             "| Event | Rank | Stars | Candidates | Candidate s | Export s | Export/candidate s | Shared QJ s | Shared QJ/candidate s | Candidate residual s | Noncandidate star residual s | Overlap warning | Source line | Source SHA-256 |",
             "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|:---:|---:|---|"]
    for row in summaries:
        values = [row["event"], row["rank"], row["stars_including_early_returns"], row["candidates"],
                  row["candidate_seconds"], row["export_seconds_raw"], row["export_seconds_amortized_per_candidate"],
                  row["shared_qj_seconds"], row["shared_qj_seconds_per_candidate"],
                  row["candidate_residual_after_projection_move_guidance_and_other_qj"],
                  row["all_non_candidate_star_residual"], row["negative_residual_overlap_warning"],
                  row["source_line"], row["source_sha256"]]
        lines.append("| " + " | ".join("null" if value is None else str(value) for value in values) + " |")
    lines += ["", "## Per-site query counters", "",
              "| Event | Rank | Phase | Site | Direct | Upper | Classification | Magnitude | Limit | Memo direct | Memo union | Upper work | Source line | Source SHA-256 |",
              "|---:|---:|---|:---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|"]
    query_rows = [row for row in raw if row["record_type"] == "query"]
    for row in query_rows:
        values = [row["event"], row["rank"], row["phase"], row["site"], row["direct_calls"],
                  row["upper_calls"], row["classification_calls"], row["magnitude_calls"], row["limit_calls"],
                  row["memo_direct_hits"], row["memo_union_hits"], row["upper_work"],
                  row["source_line"], row["source_sha256"]]
        lines.append("| " + " | ".join(str(value) for value in values) + " |")
    lines += ["", "## Per-rank log provenance", ""]
    for rank, source in provenance.items():
        lines.append(f"- Rank {rank}: `{source['path']}` SHA-256 `{source['sha256']}`")
    lines += ["", "Detailed per-site counts, source line numbers, and source hashes are in the CSV and JSON outputs.", ""]
    path.write_text("\n".join(lines))


def synthetic_lines(rank, event):
    rows = []
    prefix = f"[1,{rank}] <stdout>: "
    for phase in PHASES:
        for site in SITES:
            rows.append(prefix + f"D9_QUERY rank={rank} phase={phase} site={site} scopes=0 direct_calls=1 upper_calls=2 classification_calls=3 magnitude_calls=4 limit_calls=5 memo_direct_hits=6 memo_union_hits=7 upper_work=8 seconds=0 direct_seconds=0 upper_seconds=0")
    rows.append(prefix + f"D9_MOVE rank={rank} stars=1 candidates=1 star_seconds=2 candidate_seconds=1 shared_qj_calls=1 shared_qj_seconds=0.1 qj_calls=4")
    for part in PARTS:
        rows.append(prefix + f"D9_MOVE_PART rank={rank} part={part} calls=1 inclusive_seconds=0.2 qj_seconds=0.05")
    return rows


def self_check():
    from tempfile import TemporaryDirectory
    with TemporaryDirectory() as directory:
        path = Path(directory) / "stdout"
        lines = synthetic_lines(0, 1) + synthetic_lines(0, 2)
        path.write_text("\n".join(lines) + "\n")
        events, _ = parse_rank(path, 0)
        assert len(events) == 2 and len(events[0]["queries"]) == 12
        assert len(accounting(events[0], 0, 1)) > 10
        path.write_text("\n".join(lines[:-1]) + "\n")
        try:
            parse_rank(path, 0)
        except AssertionError:
            pass
        else:
            raise AssertionError("missing-row synthetic case did not fail")
    return {"passed": True, "synthetic_one_rank_two_events": True, "missing_row_fails": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--workflow", type=Path, default=WORKFLOW)
    parser.add_argument("--ranks", type=int, default=6)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    raw, summaries, provenance = collect(args.workflow, args.ranks)
    stem = HERE / "m16_d9_counters"
    write_csv(stem.with_suffix(".csv"), raw)
    payload = {"schema": "n5_m16_d9_extraction_v1", "workflow": str(args.workflow),
               "rank_sources": provenance, "raw_rows": raw, "accounting": summaries,
               "method": {"expected_query_rows_per_event_rank": 12, "expected_move_rows_per_event_rank": 1,
                          "expected_move_parts_per_event_rank": 5, "site_d_inclusive_not_exclusive": True,
                          "metric_at_argument_lookup_timed": False}}
    stem.with_suffix(".json").write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    report_markdown(stem.with_suffix(".md"), summaries, provenance, args.workflow, raw)
    print(json.dumps({"passed": True, "events": 2, "ranks": args.ranks, "rows": len(raw), "outputs": str(stem)}, indent=2))


if __name__ == "__main__":
    main()
