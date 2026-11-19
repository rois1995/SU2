#!/usr/bin/env python3
"""Report FP0 section 6 from the pinned N21 Part P G5 rank logs."""

import argparse
import gzip
import json
from pathlib import Path
import re
import runpy


NIGHT = Path(__file__).resolve().parents[1]
G5 = NIGHT / "N4/section6_3_after_N11/round_8_5t_P/G5/full587"
EXTRACTOR = NIGHT.parent / "nobl_clean_check_20261114/extract_markers.py"
strip_mpi_prefixes = runpy.run_path(str(EXTRACTOR))["strip_mpi_prefixes"]
MARKER = re.compile(r"\bNATIVE3D(?:_TIMER_SUMMARY|_TIMER|_STAGE|_DETAIL|_FINAL_PROOF)?(?= )")
FIELD = re.compile(r"([A-Za-z_]\w*)=([^\s]+)")
RANKS = range(6)
EVENT_ORDER = ["metric", "remesh", "replace", "mesh_output"] * 2 + ["metric"]


def parse(lines):
    for raw in lines:
        line = strip_mpi_prefixes(raw)
        markers = list(MARKER.finditer(line))
        for index, marker in enumerate(markers):
            end = markers[index + 1].start() if index + 1 < len(markers) else len(line)
            yield marker.group(), dict(FIELD.findall(line[marker.end():end]))


def read(path):
    compressed = path.with_name(path.name + ".gz")
    if not path.is_file() and compressed.is_file():
        with gzip.open(compressed, "rt", errors="replace") as stream:
            return list(parse(stream))
    with path.open(errors="replace") as stream:
        return list(parse(stream))


def select(rows, marker, count, **conditions):
    selected = [
        fields for kind, fields in rows
        if kind == marker and all(fields.get(key) == value for key, value in conditions.items())
    ]
    if len(selected) != count:
        raise ValueError(f"{marker} {conditions}: expected {count} rows, found {len(selected)}")
    return selected


def final_timers(rows, rank):
    pending = {root: {} for root in EVENT_ORDER}
    snapshots = {root: [] for root in EVENT_ORDER}
    order = []
    for kind, fields in rows:
        root = fields.get("root")
        if root not in pending or fields.get("boundary") != "final" or fields.get("rank") != str(rank):
            continue
        if kind == "NATIVE3D_TIMER":
            key = fields["phase"], fields["operation"]
            if key in pending[root]:
                raise ValueError(f"Rank {rank}, {root}: duplicate timer {key}")
            pending[root][key] = fields
        if kind == "NATIVE3D_TIMER_SUMMARY":
            if not pending[root] or int(fields["fixedstorage"]) != 4120 or int(fields["ranks"]) != len(RANKS):
                raise ValueError(f"Rank {rank}, {root}: incomplete or incompatible final timer")
            snapshots[root].append(pending[root])
            pending[root] = {}
            order.append(root)
    if order != EVENT_ORDER or any(pending.values()):
        raise ValueError(f"Rank {rank}: unexpected final timer order {order}")
    return snapshots


def load_events(case):
    ranks = case / "rank_output/1"
    stdout = [read(ranks / f"rank.{rank}/stdout") for rank in RANKS]
    stderr = [read(ranks / f"rank.{rank}/stderr") for rank in RANKS]
    events = [{} for _ in range(2)]
    for rank in RANKS:
        all_ranks = stdout[0]
        identity = {"rank": str(rank), "ranks": str(len(RANKS))}
        sources = {
            "native": select(all_ranks, "NATIVE3D", 2, complete="1", **identity),
            "refine": select(all_ranks, "NATIVE3D_DETAIL", 2, stage="refine", complete="1", **identity),
            "return": select(all_ranks, "NATIVE3D_DETAIL", 2, stage="return", complete="1", **identity),
            "proof": select(stderr[rank], "NATIVE3D_FINAL_PROOF", 2, rank=str(rank), shared="1"),
        }
        for stage in ("replace", "mesh_output", "metric"):
            sources[stage] = select(all_ranks, "NATIVE3D_STAGE", EVENT_ORDER.count(stage), stage=stage, **identity)
        for root, snapshots in final_timers(stdout[rank], rank).items():
            sources[f"{root}_timers"] = snapshots
        for index, event in enumerate(events):
            event[rank] = {key: values[index] for key, values in sources.items()}
    return events


def span(values, precision=6):
    return f"{min(values):.{precision}f}–{max(values):.{precision}f}"


def timing(event, source, key):
    return span([float(event[rank][source][key]) for rank in RANKS])


def mobility(event, root, phase=None):
    return [
        fields for rank in RANKS
        for (row_phase, operation), fields in event[rank][f"{root}_timers"].items()
        if operation == "mobility_policy" and (phase is None or row_phase == phase)
    ]


def report_mobility(rows):
    if not rows:
        return "0 / 0 (0)"
    calls = sum(int(row["calls"]) for row in rows)
    seconds = span([float(row["exclusive_seconds"]) for row in rows], 9)
    return f"{calls} / {seconds} ({len(rows)})"


def build_summary(event, event_number):
    def calls(rank, root, phase=None):
        return sum(
            int(fields["calls"]) for (row_phase, operation), fields in event[rank][f"{root}_timers"].items()
            if operation == "mobility_policy" and (phase is None or row_phase == phase)
        )

    # Preparation uses two PrepareMobility calls per rank. The additional
    # nested operation is BuildMobilityPolicy during event 1.
    preparation = [calls(rank, "remesh", "preparation") - 2 for rank in RANKS]
    returned = [calls(rank, "remesh", "transfer") for rank in RANKS]
    output = [calls(rank, "mesh_output") for rank in RANKS]
    if preparation != [2 - event_number] * len(RANKS) or returned != [1] * len(RANKS):
        raise ValueError(f"Event {event_number}: unexpected preparation or return mobility call counts")
    if output != [1, 0, 0, 0, 0, 0]:
        raise ValueError(f"Event {event_number}: unexpected sidecar mobility call counts")
    counts = [sum(parts) for parts in zip(preparation, returned, output)]
    return_seconds = span([float(fields["exclusive_seconds"]) for fields in mobility(event, "remesh", "transfer")], 9)
    output_seconds = float(mobility(event, "mesh_output")[0]["exclusive_seconds"])
    print(f"| {event_number} | {' / '.join(map(str, counts))} | {sum(counts)} | {return_seconds} | {output_seconds:.9f} |")


def fixed_parts(events):
    rows = [
        ("PrepareReference", "native", "reference", ("remesh", "preparation")),
        ("ImportGeometry", "native", "import", ("remesh", "preparation")),
        ("Seed census", "refine", "seed", None),
        ("Final proof reverse", "proof", "reverse_seconds", None),
        ("Final proof share", "proof", "share_seconds", None),
        ("Return (ReaderSlices path)", "return", "total", ("remesh", "transfer")),
        ("Replace", "replace", "total", ("replace",)),
        ("Mesh file output", "mesh_output", "mesh", None),
        ("Native-reference file output", "mesh_output", "reference", ("mesh_output",)),
        ("ComputeMetric", "metric", "total", ("metric",)),
    ]
    print("## Fixed parts — accepted G5 build587")
    print()
    print("| Stage | Event 1 wall s | Event 1 mobility | Event 2 wall s | Event 2 mobility |")
    print("|---|---:|---:|---:|---:|")
    for label, source, key, scope in rows:
        result = [label]
        for event in events:
            amount = report_mobility(mobility(event, *scope) if scope else [])
            if scope == ("remesh", "preparation"):
                amount += " combined"
            result.extend((timing(event, source, key), amount))
        print("| " + " | ".join(result) + " |")
    print()
    print("Wall seconds span the six ranks. Mobility entries give job-wide operation calls, exclusive seconds"
          " per reporting rank, and the number of reporting ranks in parentheses.")
    print("The reference and import rows repeat one combined preparation timer. The two PrepareMobility"
          " calls straddle the end of ImportGeometry. The ledger merges them with the nested build,"
          " so it cannot give per-stage or per-build seconds.")
    print("The Return row is the full ReaderSlices path. Its nested FromDistributed timer is "
          + " / ".join(timing(event, "return", "reader") for event in events)
          + " s for events 1 / 2 and excludes the subsequent policy rebuild.")
    print("The third ComputeMetric sample belongs to the post-adaptation workflow and is omitted.")
    print()
    print("| Event | BuildMobilityPolicy calls, ranks 0–5 | Job calls | Return build s per rank | Reference output build s, rank 0 |")
    print("|---|---|---:|---:|---:|")
    for number, event in enumerate(events, 1):
        build_summary(event, number)
    print()
    print("The calls are distinguished by source scope. Each rank rebuilds once in ReaderSlices on both"
          " events, and rank 0 rebuilds once while writing each sidecar. Event 1 additionally builds"
          " once per rank during preparation. The event-1 preparation build duration is unidentifiable"
          " from the merged timer row.")


def fallback(event, case):
    receipt = json.loads((case / "full_workflow_receipt.json").read_text())
    if receipt["workflow_accepted"] is not True or receipt["events"][1]["event"] != 2:
        raise ValueError("Part B fallback requires the accepted event-2 workflow receipt")
    measurement = receipt["events"][1]
    print("## Part B fallback — event 2, target changed")
    print()
    print(f"- Engine seconds, maximum rank: {measurement['refine_detail_seconds_max_rank']:.9f}.")
    print(f"- Native event seconds, maximum rank: {measurement['whole_event_seconds_max_rank']:.9f}.")
    for key in ("rounds", "accepted"):
        values = [int(event[rank]["refine"][key]) for rank in RANKS]
        print(f"- {key.capitalize()} by rank: {' / '.join(map(str, values))}.")
        if key == "accepted":
            print(f"- Accepted changes by local counter, rank sum: {sum(values)}.")
    print(f"- Valid: {measurement['certification_complete']}. Cells / points: "
          f"{measurement['actual_mesh_counts']['cells']} / {measurement['actual_mesh_counts']['points']}.")
    print(f"- Seed census: {timing(event, 'refine', 'seed')} s across ranks.")
    proof_seconds = [
        sum(float(row["exclusive_seconds"]) for (phase, _), row in event[rank]["remesh_timers"].items()
            if phase == "final_proof")
        for rank in RANKS
    ]
    print(f"- Final proof phase: {span(proof_seconds)} s summed exclusive work per rank.")
    for key in ("reverse_seconds", "share_seconds"):
        print(f"- Final proof {key.removesuffix('_seconds')}: {timing(event, 'proof', key)} s across ranks.")
    print()
    print("| Phase | Operation | Calls by rank (0–5) | Rank-exclusive seconds by rank (0–5) |")
    print("|---|---|---|---|")
    keys = {key for rank in RANKS for key in event[rank]["remesh_timers"]}
    for phase in ("preparation", "construction", "quality", "final_proof"):
        phase_keys = sorted(key for key in keys if key[0] == phase)
        if not phase_keys:
            raise ValueError(f"Event 2: missing timer phase {phase}")
        for key in phase_keys:
            rows = [event[rank]["remesh_timers"].get(key, {}) for rank in RANKS]
            calls = " / ".join(row.get("calls", "0") for row in rows)
            seconds = " / ".join(f"{float(row.get('exclusive_seconds', '0')):.6f}" for row in rows)
            print(f"| {phase} | {key[1]} | {calls} | {seconds} |")


def self_check():
    stamped = (
        "Thu Nov 19 09:29:48 2026<stdout>:NATIVE3D_STAGE stage=mesh_"
        "Thu Nov 19 09:29:48 2026<stdout>:output rank=0 total=35.04 "
        "NATIVE3D_DETAIL stage=return rank=1 total=30.21\n"
    )
    rows = list(parse([stamped]))
    assert rows == [
        ("NATIVE3D_STAGE", {"stage": "mesh_output", "rank": "0", "total": "35.04"}),
        ("NATIVE3D_DETAIL", {"stage": "return", "rank": "1", "total": "30.21"}),
    ]
    log = []
    for root in EVENT_ORDER:
        common = f"root={root} boundary=final rank=3"
        log.append(f"NATIVE3D_TIMER {common} phase=preparation operation=mobility_policy calls=1")
        log.append(f"NATIVE3D_TIMER_SUMMARY {common} ranks=6 fixedstorage=4120")
    snapshots = final_timers(list(parse(log)), 3)
    assert {key: len(value) for key, value in snapshots.items()} == {
        root: EVENT_ORDER.count(root) for root in EVENT_ORDER
    }
    print("FP0 fixed-parts parser self-check passed.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", type=Path, default=G5)
    parser.add_argument("--fallback-b", action="store_true")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    events = load_events(args.case)
    fixed_parts(events)
    if args.fallback_b:
        print()
        fallback(events[1], args.case)


if __name__ == "__main__":
    main()
