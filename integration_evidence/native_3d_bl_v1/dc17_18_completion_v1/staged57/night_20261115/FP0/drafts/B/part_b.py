#!/usr/bin/env python3
"""Validate the FP0 frozen replay captures and report the standalone Refine."""

import argparse
import gzip
import hashlib
import json
import math
from pathlib import Path
import runpy
import struct


RANKS = range(6)
LOGS = runpy.run_path(str(Path(__file__).resolve().parents[2] / "fixed_parts.py"))
PHASES = ("preparation", "construction", "quality", "final_proof")
CAPTURE_LIMIT = 128 << 20


def inspect_payload(root, name, expected_size, expected_rows=None):
    path = root / name
    if path.stat().st_size != expected_size or not 0 < expected_size <= CAPTURE_LIMIT:
        raise ValueError(f"{path}: expected {expected_size} bytes within the capture bound")
    with path.open("rb") as stream:
        if expected_rows is not None and struct.unpack("=Q", stream.read(8))[0] != expected_rows:
            raise ValueError(f"{path}: stream count differs from capture manifest")
        stream.seek(0)
        sha256 = hashlib.file_digest(stream, "sha256").hexdigest()
    return {"name": name, "bytes": expected_size, "sha256": sha256}


def prepare(capture):
    shards = []
    for rank in RANKS:
        stem = f"stage_exit_rank{rank}"
        path = capture / f"{stem}_manifest.json"
        manifest = json.loads(path.read_text())
        if (manifest["rank"] != rank or manifest["stage"] != "exit" or manifest["record_format"] != 1
                or not all(manifest[key] is True for key in ("complete_shard", "source_valid", "parameters_present"))):
            raise ValueError(f"{path}: invalid or incomplete exit shard")
        expected_files = {kind: f"{stem}_{kind}.bin" for kind in ("owned", "donors", "parameters")}
        if manifest["files"] != expected_files:
            raise ValueError(f"{path}: exit files differ from the observer format")

        policies = manifest["policies"]
        if policies["wall_heights"] or policies["region_walls"]:
            raise ValueError(f"{path}: FP0 requires the no-BL event")
        rows = manifest["working_rows"]
        integers = [rows, policies["maximum_rounds"], policies["maximum_work"], policies["memory_ceiling"]]
        reals = [policies[key] for key in ("maximum_seconds", "surface_tolerance", "wall_height_tolerance")]
        if any(type(value) is not int or value < 0 for value in integers):
            raise ValueError(f"{path}: malformed execution limits")
        if not rows or not policies["maximum_work"] or not policies["memory_ceiling"]:
            raise ValueError(f"{path}: missing execution limits or owned cells")
        if any(not math.isfinite(value) or value <= 0 for value in reals):
            raise ValueError(f"{path}: malformed execution policy")
        values = integers + reals

        payloads = [
            inspect_payload(capture, expected_files["owned"], 8 + rows * manifest["owned_record_bytes"], rows),
            inspect_payload(capture, expected_files["donors"],
                            8 + manifest["donor_rows"] * manifest["donor_record_bytes"], manifest["donor_rows"]),
            inspect_payload(capture, expected_files["parameters"], manifest["parameters_payload_bytes"]),
        ]
        shards.append({"rank": rank, "manifest": path.name, "owned_cells": rows, "payloads": payloads,
                       "options": values})

    if any(shard["options"][1:] != shards[0]["options"][1:] for shard in shards):
        raise ValueError("Effective engine options differ across ranks")
    for shard in shards:
        options_path = capture / f"stage_exit_rank{shard['rank']}_fp0_options.txt"
        options_path.write_text(" ".join(map(str, shard["options"])) + "\n")
    receipt = {"source_valid": True, "cells": sum(shard["owned_cells"] for shard in shards), "shards": shards}
    (capture / "fp0_exit_capture_receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(f"Prepared {len(shards)} verified exit shards with {receipt['cells']} owned cells")


def measured_row(path, marker):
    gz = path.with_name(path.name + ".gz")
    opener = gzip.open if not path.is_file() and gz.is_file() else open
    source = gz if opener is gzip.open else path
    decoder = json.JSONDecoder()
    lines = []
    with opener(source, "rt", errors="replace") as stream:
        for raw in stream:
            line = LOGS["strip_mpi_prefixes"](raw)
            if marker in line:
                lines.append(decoder.raw_decode(line.split(marker, 1)[1])[0])
    if len(lines) != 1:
        raise ValueError(f"{path}: expected one {marker!r} record, found {len(lines)}")
    return lines[0]


def final_rows(path, rank):
    records = {}
    found_summary = False
    for marker, fields in LOGS["read"](path):
        if (fields.get("root"), fields.get("boundary"), fields.get("rank")) != ("native_engine", "final", str(rank)):
            continue
        if marker == "NATIVE3D_TIMER_SUMMARY":
            if found_summary or int(fields["ranks"]) != len(RANKS) or int(fields["fixedstorage"]) != 4120:
                raise ValueError(f"Rank {rank}: malformed final engine timer summary")
            found_summary = True
        elif marker == "NATIVE3D_TIMER":
            key = fields["phase"], fields["operation"]
            if found_summary or key in records:
                raise ValueError(f"Rank {rank}: duplicate or late engine timer row {key}")
            records[key] = fields
    if not found_summary:
        raise ValueError(f"Rank {rank}: missing final engine timer summary")
    return records


def report(capture, replay_ranks, refine_ranks):
    receipt = json.loads((capture / "fp0_exit_capture_receipt.json").read_text())
    replay = measured_row(replay_ranks / "rank.0/stdout", "FP0 B_exit ")
    if (replay["valid"] is not True or replay["ranks"] != len(RANKS) or replay["cells"] != receipt["cells"]
            or replay["points"] <= 0):
        raise ValueError("Frozen replay outputs are incomplete or differ from the saved valid exit")
    rank_paths = [refine_ranks / f"rank.{rank}/stdout" for rank in RANKS]
    results = [measured_row(path, "FP0 B_refine ") for path in rank_paths]
    ledger = [final_rows(path, rank) for rank, path in enumerate(rank_paths)]
    if [row["rank"] for row in results] != list(RANKS):
        raise ValueError("Standalone Refine rank outputs differ from the six-rank run")
    if [row["input_cells"] for row in results] != [shard["owned_cells"] for shard in receipt["shards"]]:
        raise ValueError("Standalone Refine input counts differ from the captured shards")

    print("## Part B — frozen-volume exit, then standalone Refine")
    print()
    print("| Exit | Cells | Points |")
    print("|---|---:|---:|")
    print(f"| Frozen-volume replay | {replay['cells']:,} | {replay['points']:,} |")
    print("| Pinned N21 Part P G5 | 726,442 | 146,676 |")
    print()
    print(f"Replay native wall seconds, maximum rank {replay['seconds']:.6f}. "
          f"Native job peak {replay['job_peak_rss_bytes']:,} bytes.")
    print()
    print("| Rank | Refine wall s | Seed census s | Final proof exclusive s | Rounds | Accepted | Valid |"
          " Same local mesh | Output cells | Process peak B | Stop |")
    print("|---:|---:|---:|---:|---:|---:|---|---|---:|---:|---|")
    for row, timers in zip(results, ledger):
        final = sum(float(fields["exclusive_seconds"]) for (phase, _), fields in timers.items()
                    if phase == "final_proof")
        print(f"| {row['rank']} | {row['seconds']:.6f} | {row['seed_seconds']:.6f} | {final:.6f} | "
              f"{row['rounds']} | {row['accepted']} | {row['valid']} | {row['same_local_mesh']} | "
              f"{row['result_cells']:,} | {row['process_peak_rss_bytes']:,} | {row['stop']} |")
    print()
    peak_sum = sum(row["process_peak_rss_bytes"] for row in results)
    if any(row["job_peak_rss_bytes"] != peak_sum for row in results):
        raise ValueError("Refine job peak differs from the sum of the six process peaks")
    print(f"Standalone Refine wall seconds, maximum rank {max(row['seconds'] for row in results):.6f}. "
          f"Reported accepted operations, rank sum {sum(row['accepted'] for row in results):,}. "
          f"Job peak {peak_sum:,} bytes.")
    for row in results:
        if row["reason"]:
            print(f"- Rank {row['rank']} stopped with {row['reason']}")
    print()
    print("| Phase | Operation | Calls, ranks 0–5 | Inclusive seconds, ranks 0–5 | Exclusive seconds, ranks 0–5 |")
    print("|---|---|---|---|---|")
    keys = {key for timers in ledger for key in timers}
    for phase in PHASES:
        phase_keys = sorted(key for key in keys if key[0] == phase)
        if not phase_keys:
            print(f"| {phase} | No active operation rows | 0 | 0 | 0 |")
        for key in phase_keys:
            fields = [timers.get(key, {}) for timers in ledger]
            calls = " / ".join(row.get("calls", "0") for row in fields)
            inclusive = " / ".join(f"{float(row.get('inclusive_call_seconds', '0')):.6f}" for row in fields)
            exclusive = " / ".join(f"{float(row.get('exclusive_seconds', '0')):.6f}" for row in fields)
            print(f"| {phase} | {key[1]} | {calls} | {inclusive} | {exclusive} |")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    command = parser.add_subparsers(dest="command", required=True)
    command.add_parser("prepare").add_argument("capture", type=Path)
    summary = command.add_parser("report")
    summary.add_argument("capture", type=Path)
    summary.add_argument("replay_ranks", type=Path, help="Directory holding rank.0 through rank.5")
    summary.add_argument("refine_ranks", type=Path, help="Directory holding rank.0 through rank.5")
    args = parser.parse_args()
    if args.command == "prepare":
        prepare(args.capture)
    else:
        report(args.capture, args.replay_ranks, args.refine_ranks)


if __name__ == "__main__":
    main()
