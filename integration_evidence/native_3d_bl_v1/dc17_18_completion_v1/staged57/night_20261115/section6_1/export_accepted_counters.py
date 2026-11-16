#!/usr/bin/env python3
"""Export accepted N1 M18/M20/M21 counters using the existing event parser."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
PARSER = HERE.parent.parent / "p7p8_r3_20261115" / "check_connected_events.py"
VALIDATOR = HERE / "validate_disk_restart522.py"
SPEC = importlib.util.spec_from_file_location("accepted_event_parser", PARSER)
parser = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(parser)
VALIDATOR_SPEC = importlib.util.spec_from_file_location("section61_acceptance", VALIDATOR)
validator = importlib.util.module_from_spec(VALIDATOR_SPEC)
VALIDATOR_SPEC.loader.exec_module(validator)
M18_KEYS = (
    "distinct_facets", "prepared_facet_mean_bytes", "prepared_facet_builds",
    "facet_misses", "facet_build_seconds", "distinct_scope_keys", "facet_hits",
    "facet_resolutions", "scope_hits", "scope_misses", "scope_attempts",
    "scope_builds", "scope_build_work", "scope_build_seconds", "facet_build_work",
    "cache_evictions", "cache_evicted_bytes", "retention_refusals", "prepared_bytes",
    "prepared_limit", "facet_bytes", "scope_bytes", "facet_entries", "scope_entries",
)
M21_KEYS = (
    "current_rss", "process_peak_rss", "max_exchange_work_bytes", "bytes_sent",
    "pending_bytes", "outgoing_bytes", "record_bytes",
)
M21_BOUNDARIES = ("before", "staged", "committed")


def digest(path):
    """Hash one source log for row provenance."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_location(lines, line, start):
    """Find an emitted row monotonically in its original rank log."""
    for index in range(start, len(lines)):
        if lines[index] == line:
            return index + 1, index + 1
    return None, start


def event_start(lines, event, rank):
    """Locate the start of one event segment using existing timer summaries."""
    snapshots = parser.timer.snapshots("".join(lines))
    roots = []
    cursor = 0
    for item in snapshots:
        while cursor < len(lines) and parser.fields(
                lines[cursor], "NATIVE3D_TIMER_SUMMARY") != item["summary"]:
            cursor += 1
        if cursor == len(lines):
            return 0
        index = cursor
        cursor += 1
        summary = item["summary"]
        if (summary.get("root") == "remesh"
                and summary.get("boundary") == "final"
                and summary.get("rank") == str(rank)):
            roots.append(index)
    if len(roots) < event:
        return 0
    return roots[event - 2] + 1 if event > 1 else 0


def row_record(fields, kind, case, event, rank, path, sha, line_no):
    """Attach stable event, rank and source provenance to one counter row."""
    return {"kind": kind, "case": str(case), "event": event, "rank": rank,
            "source_path": str(path), "source_sha256": sha,
            "source_line": line_no, "fields": fields}


def export_m18(case, event, exits, paths):
    """Export last engine-exit M18 fields without substituting final rebind data."""
    result = []
    for rank, fields in sorted(exits.items()):
        values = {key: fields.get(key) for key in M18_KEYS}
        missing = [key for key, value in values.items() if value is None]
        record = row_record(values, "last_engine_exit", case, event, rank,
                            paths[rank][0], paths[rank][1], paths[rank][3])
        record["status"] = "complete" if not missing else "partial"
        record["missing_fields"] = missing
        result.append(record)
    return result


def local_records(case, event, local, paths, marker, names):
    """Collect one marker family with line-aware provenance inside parsed event."""
    result = []
    for rank, lines in sorted(local.items()):
        path, sha, full_lines, _ = paths[rank]
        cursor = event_start(full_lines, event, rank)
        for text in lines:
            if marker not in text:
                continue
            fields = parser.fields(text, marker)
            if not fields:
                continue
            line_no, cursor = source_location(full_lines, text, cursor)
            values = {key: fields.get(key) for key in names}
            result.append(row_record(values, marker, case, event, rank,
                                     path, sha, line_no))
    return result


def export_m21(case, event, local, paths):
    """Export before/staged/committed records and explicit absent-boundary rows."""
    rows = local_records(case, event, local, paths, "NATIVE3D_OWNERSHIP_MEMORY",
                           ("boundary",) + M21_KEYS)
    for row in rows:
        row["status"] = "emitted"
    seen = {(row["rank"], row["fields"].get("boundary")) for row in rows}
    for rank in sorted(local):
        for boundary in M21_BOUNDARIES:
            if (rank, boundary) in seen:
                continue
            path, sha, _, _ = paths[rank]
            missing = {key: None for key in M21_KEYS}
            rows.append({"kind": "NATIVE3D_OWNERSHIP_MEMORY", "case": str(case),
                         "event": event, "rank": rank, "source_path": str(path),
                         "source_sha256": sha, "source_line": None,
                         "status": "not_emitted", "expected_boundary": boundary,
                         "fields": missing})
    return rows


def source_acceptance(case):
    """Require terminal, unchanged, accepted workflow and identity receipts."""
    status = json.loads((case / "status.json").read_text())
    workflow = json.loads((case / "full_workflow_receipt.json").read_text())
    identity = json.loads((case / "section6_1_reference_identity.json").read_text())
    events = [json.loads((case / f"p7p8_event{event}_gate.json").read_text())
              for event in (1, 2)]
    timers = [json.loads((case / f"f3b_event{event}_timer_gate.json").read_text())
              for event in (1, 2)]
    if not validator.source_receipts_pass(status, workflow, identity, events, timers):
        raise RuntimeError("case lacks terminal, unchanged, accepted Section 6.1 receipts")
    return {"status": status, "workflow": workflow, "identity": identity,
            "event_gates": events, "timer_gates": timers}


def gate_summary(assessed):
    """Keep gate evidence while omitting duplicated raw M20/M21 record lists."""
    diagnostic = assessed["diagnostics"]
    return {"passed": assessed["passed"], "gates": assessed["gates"],
            "proof": assessed["proof"], "memory": assessed["memory"],
            "diagnostics": {key: diagnostic[key] for key in (
                "final_proof_geometry_bound_calls", "expected_calls", "call_count_matches",
                "m18_rank_rows_with_fields", "m20_private_work_rows",
                "m21_ownership_memory_rows", "counter_presence_is_non_gating")}}


def event_export(case, event):
    """Parse one complete event and build the three requested counter sections."""
    assessed = parser.assess_event(case, event)
    parsed = parser.read_event(case, event)
    if assessed is None or parsed is None or not assessed["passed"]:
        raise RuntimeError(f"event {event} is incomplete or failed its existing gates")
    _, local, exits, _ = parsed
    base = Path(case) / "rank_output" / "1"
    paths = {}
    for rank in range(parser.RANKS):
        path = base / f"rank.{rank}" / "stdout"
        full_lines = path.read_text(errors="replace").splitlines(keepends=True)
        exit_line = next((line for line in reversed(local[rank])
                          if "NATIVE3D_STAGE" in line and "stage=exit" in line), "")
        line_no, _ = source_location(
            full_lines, exit_line, event_start(full_lines, event, rank))
        paths[rank] = (path, digest(path), full_lines, line_no)
    m20 = local_records(case, event, local, paths, "NATIVE3D_QUALITY",
                        ("round", "window", "window_seconds", "census", "stage",
                         "private_work_used", "busy_seconds", "join_call_seconds",
                         "commits", "commits_total"))
    m20 += local_records(case, event, local, paths, "NATIVE3D_PRIVATE_WORK",
                         ("round", "private_work_used", "busy_seconds"))
    return {"event": event, "gate_receipt": gate_summary(assessed),
            "M18": export_m18(case, event, exits, paths),
            "M20": m20, "M21": export_m21(case, event, local, paths)}


def synthetic_self_check():
    """Check omission semantics and event/rank schema without opening workflow logs."""
    exits = {0: {"distinct_facets": "4", "prepared_facet_mean_bytes": "12"}}
    fake_paths = {0: (Path("synthetic/rank.0/stdout"), "sha", [], 17)}
    status = {"state": "terminal", "exit_code": 0,
              "all_owned_handles_terminal": True, "inputs_unchanged": True,
              "source_and_binary_pins_unchanged": True, "full_workflow_accepted": True}
    workflow = {"workflow_accepted": True, "P7_P8_events_checked": [1, 2],
                "F3b_timer_events_checked": [1, 2]}
    identity = {"identity_passed": True}
    gates = [{"event": event, "passed": True} for event in (1, 2)]
    assert validator.source_receipts_pass(status, workflow, identity, gates, gates)
    assert not validator.source_receipts_pass(
        dict(status, state="running"), workflow, identity, gates, gates)
    assert not validator.source_receipts_pass(
        dict(status, inputs_unchanged=False), workflow, identity, gates, gates)
    assert not validator.source_receipts_pass(
        status, workflow, {"identity_passed": False}, gates, gates)
    m18 = export_m18("synthetic", 1, exits, fake_paths)
    assert m18[0]["status"] == "partial"
    assert m18[0]["fields"]["facet_misses"] is None
    assert m18[0]["source_line"] == 17 and m18[0]["event"] == 1
    fake_local = {0: ["NATIVE3D_OWNERSHIP_MEMORY rank=0 boundary=before current_rss=9\n"]}
    paths = {0: (Path("synthetic/rank.0/stdout"), "sha",
             [fake_local[0][0]], 17)}
    m21 = export_m21("synthetic", 1, fake_local, paths)
    absent = [row for row in m21 if row["status"] == "not_emitted"]
    assert {row["expected_boundary"] for row in absent} == {"staged", "committed"}
    assert all(row["fields"]["current_rss"] is None for row in absent)


def main():
    """Require explicit terminal confirmation before reading run logs."""
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("case", nargs="?", type=Path)
    args.add_argument("--output", type=Path)
    args.add_argument("--self-check", action="store_true")
    opts = args.parse_args()
    if opts.self_check:
        synthetic_self_check()
        print("synthetic self-check passed")
        return
    if opts.case is None or opts.output is None:
        raise SystemExit("actual export requires CASE and --output")
    acceptance = source_acceptance(opts.case)
    records = {"schema": "accepted_n1_counter_export_v1", "case": str(opts.case),
               "acceptance_receipts": acceptance,
               "events": [event_export(opts.case, event) for event in (1, 2)]}
    opts.output.write_text(json.dumps(records, indent=2, sort_keys=True) + "\n")
    print(f"wrote {opts.output}")


if __name__ == "__main__":
    main()
