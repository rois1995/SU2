#!/usr/bin/env python3
"""Compare native 3D integer output rows without merging rank streams.

Excluded measurements follow the output writers: RuntimeProfile3D's durations
and RSS (CNativeRuntimeProfile3D.hpp:70-82), engine quality/stage seconds and
debt (CNativeEngine3D.cpp:1765-83, 1846-74), remesher phases and RSS
(CNativeRemesher3D.cpp:859-74), detail timings (CNativeRemesher3D.cpp:890-1052),
and timer-tree seconds (CNativeTimerTree3D.hpp:287-307). Integer fields such as
modeled bytes, work, calls, and integer vectors remain compared.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import re
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[6]
EXTRACTOR = (ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/"
             "staged57/nobl_clean_check_20261114/extract_markers.py")
HELPER_START_UNIX = time.time()
spec = importlib.util.spec_from_file_location("native_extract_markers", EXTRACTOR)
if spec is None or spec.loader is None:
    raise RuntimeError(f"Cannot load existing marker helper: {EXTRACTOR}")
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)
strip_mpi_prefixes = extractor.strip_mpi_prefixes

ROW_MARKER = re.compile(r"NATIVE3D_(?:STAGE|QUALITY(?:_[A-Z_]+)?|DETAIL|TIMER)\b|NATIVE3D rank=")
INT = re.compile(r"^[+-]?\d+$")
FIELD = re.compile(r"([A-Za-z_]\w*)=([^\s]+)")
IGNORED_MEASUREMENTS = {"total", "other", "cpu", "cpu_other", "idle_cpu", "process_peak_rss", "elapsed",
                        "root_seconds", "sumexclusive", "unattributed", "coverage",
                        "inclusive_call_seconds", "exclusive_seconds", "partition", "migration",
                        "preprocess", "setup", "sensors", "hessian", "construction", "mesh", "reference",
                        "geometry", "geometry_checks", "solver", "transfer", "finalize", "metric_remesh",
                        "replace", "sensor_frames", "geometry_rule", "complexity_gradation_audit",
                        "publication", "halo", "original_family", "complexity", "nested_gradation",
                        "nested_gradation_halo", "nested_gradation_reduction", "sample_q", "sample_j", "sample_x", "sample_y", "sample_z",
                        "sample_old_shape_debt", "sample_new_shape_debt", "geometry_estimate",
                        "geometry_weight_scale", "debt_removed", "report_seconds", "communication_seconds",
                        "parent_transport_seconds", "worker_transport_seconds", "short09_share", "short05_share",
                        "maximum_relative_frobenius", "owned_maximum_relative_frobenius"}
RANK_TIMES = {"support", "reference", "import", "source", "weights", "partition", "donor_migration",
              "communicator", "refine", "coarsen", "wait", "return", "policy", "reassessment", "normalization",
              "nested_tracked_collectives", "nested_tracked_import", "nested_tracked_parent", "nested_tracked_worker"}
DETAIL_TIMES = {"points", "cells", "faces", "source", "query", "reader", "reduction", "nested_field_build",
                "nested_search", "nested_interpolation", "nested_composition", "nested_tracked_collectives",
                "source_point_search", "source_point_interpolation", "source_point_seconds", "geometry_estimate",
                "nested_gradation", "nested_gradation_halo", "nested_gradation_reduction", "nested_routing", "nested_filter", "nested_exchange",
                "graph", "parmetis", "migration", "nested_directory", "nested_routing", "nested_search",
                "nested_source_envelope", "nested_envelope_payload", "parameter_return", "build", "imports",
                "initialization", "seed", "first_layer", "selection", "dependency", "reservation", "donor",
                "reconstruction", "metric", "collision", "routing", "publication", "update", "completion",
                "nested_directory_election", "process_cpu", "private_reconstruction_cpu", "private_metric_cpu",
                "private_measurement_cpu", "planning_nested_s", "layer_metadata_nested_s", "layer_donor_nested_s",
                "layer_geometry_nested_s", "layer_metric_nested_s", "layer_publication_nested_s", "region_cap_nested_s",
                "region_sampling_s", "region_first_band_s", "region_transition_s", "region_junction_s", "region_core_s",
                "region_validation_s", "nested_collapse_correction", "nested_change_notifications",
                "nested_collapse_orientation", "nested_collapse_geometry", "nested_field_query", "nested_cache",
                "nested_field_query", "nested_edge_geometry", "nested_edge_composition", "nested_edge",
                "nested_edge_search", "nested_edge_trace", "nested_edge_integral", "nested_donor_search",
                "nested_donor_filter"}
IDENTITY_FIELDS = {"rank", "ranks", "stage", "phase", "operation", "root", "boundary", "census", "kind"}
VECTOR_HINTS = {"seed_cell_classes", "updated_cell_classes", "rejected", "q_miss_hist",
                "j_miss_hist", "geometry_excess_hist", "geometry_authority_types"}
ANCHORS = {
    "G2": ("NATIVE3D_STAGE", lambda f: f.get("stage") == "length_collapse_exit"),
    "GQ": ("NATIVE3D_QUALITY", lambda f: f.get("stage") == "window"),
}


@dataclass
class Row:
    marker: str
    fields: dict[str, str]
    path: str
    line: int

    def rank(self) -> str | None:
        return self.fields.get("rank")


def marker_rows(path: Path) -> list[Row]:
    rows = []
    with path.open(errors="replace") as source:
        for line_no, raw in enumerate(source, 1):
            line = strip_mpi_prefixes(raw)
            for match in ROW_MARKER.finditer(line):
                marker = match.group(0)
                body = line[match.end():]
                if marker == "NATIVE3D rank=":
                    body = "rank=" + body
                elif marker == "NATIVE3D_TIMER":
                    body = "timer=" + body
                fields = dict(FIELD.findall(body))
                kind = "NATIVE3D rank" if marker == "NATIVE3D rank=" else marker
                rows.append(Row(kind, fields, str(path), line_no))
    return rows


def streams(case: Path) -> dict[str, list[Row]]:
    if case.is_file():
        return {case.name: marker_rows(case)}
    files = sorted(case.glob("rank_output/**/rank.*/stdout"))
    if files:
        return {str(path.relative_to(case)): marker_rows(path) for path in files}
    log = case / "run.log"
    if log.is_file():
        return {"run.log": marker_rows(log)}
    raise ValueError(f"No rank_output/**/rank.*/stdout or run.log under {case}")


def is_measurement(row: Row, key: str) -> bool:
    if row.marker == "NATIVE3D_TIMER":
        return key != "calls"
    if row.marker == "NATIVE3D rank" and key in RANK_TIMES:
        return True
    if row.marker == "NATIVE3D_DETAIL" and (key in DETAIL_TIMES or key.endswith("_s")):
        return True
    return (key in IGNORED_MEASUREMENTS or key.endswith("_seconds") or key.endswith("_cpu") or
            key.endswith("_rss") or "_rss_" in key or key.endswith("_cpu") or "_debt" in key)


def integer_values(row: Row) -> dict[str, Any]:
    result = {}
    for key, value in row.fields.items():
        if key in IDENTITY_FIELDS or is_measurement(row, key):
            continue
        if "," in value:
            if key not in VECTOR_HINTS and not all(INT.fullmatch(item) for item in value.split(",")):
                continue
            if not all(INT.fullmatch(item) for item in value.split(",")):
                raise ValueError(f"expected integer vector {key} became noninteger at {row.path}:{row.line}")
            result[key] = tuple(int(item) for item in value.split(","))
        elif INT.fullmatch(value):
            result[key] = int(value)
        elif key in VECTOR_HINTS:
            raise ValueError(f"expected integer vector {key} missing/invalid at {row.path}:{row.line}")
    return result


def compare_row(reference: Row, current: Row) -> str | None:
    if reference.marker != current.marker:
        return f"record: {reference.marker!r} != {current.marker!r}"
    required = {"NATIVE3D_STAGE": {"rank", "stage"}, "NATIVE3D_QUALITY": {"rank", "stage", "census"},
                "NATIVE3D_QUALITY_DRIFT": {"rank", "stage"},
                "NATIVE3D_QUALITY_LOCATION": {"rank", "census"},
                "NATIVE3D_QUALITY_LADDER": {"rank", "census", "kind"},
                "NATIVE3D_DETAIL": {"rank", "stage"}, "NATIVE3D rank": {"rank", "ranks"},
                "NATIVE3D_TIMER": {"root", "boundary", "rank", "phase", "operation", "calls"}}
    for key in required[reference.marker]:
        if key not in reference.fields or key not in current.fields:
            return f"identity.{key}: required field missing"
    for key in sorted(IDENTITY_FIELDS):
        if reference.fields.get(key) != current.fields.get(key):
            return f"identity.{key}: {reference.fields.get(key)!r} != {current.fields.get(key)!r}"
    before, after = integer_values(reference), integer_values(current)
    for key in sorted(before.keys() | after.keys()):
        if key not in before or key not in after:
            return f"integer.{key}: {before.get(key, '<missing>')!r} != {after.get(key, '<missing>')!r}"
        if before[key] != after[key]:
            return f"integer.{key}: {before[key]!r} != {after[key]!r}"
        raw = current.fields.get(key, "")
        if isinstance(before[key], int) and not INT.fullmatch(raw):
            return f"integer.{key} changed type: {raw!r}"
        if isinstance(before[key], tuple) and not all(INT.fullmatch(x) for x in raw.split(",")):
            return f"integer vector {key} changed type: {raw!r}"
    return None


def cutoff_rows(rows: list[Row], gate: str) -> tuple[list[Row], set[str]]:
    if gate == "all":
        return rows, set()
    anchor_kind, predicate = ANCHORS[gate]
    need = 1 if gate == "G2" else 3
    seen: dict[str, int] = {}
    anchors = set()
    for row in rows:
        rank = row.rank()
        if row.marker == anchor_kind and rank is not None and predicate(row.fields):
            seen[rank] = seen.get(rank, 0) + 1
            if seen[rank] == need:
                anchors.add(rank)
    consumed: dict[str, int] = {}
    ended: set[str] = set()
    kept = []
    for row in rows:
        rank = row.rank()
        if rank is None or rank not in anchors or rank in ended:
            if rank not in ended:
                kept.append(row)
            continue
        if row.marker == anchor_kind and predicate(row.fields):
            consumed[rank] = consumed.get(rank, 0) + 1
            kept.append(row)
            if consumed[rank] == need:
                ended.add(rank)
        else:
            kept.append(row)
    return kept, anchors


def compare_streams(reference: dict[str, list[Row]], current: dict[str, list[Row]], gate: str) -> dict[str, Any]:
    if reference.keys() != current.keys():
        missing = sorted(reference.keys() - current.keys())
        extra = sorted(current.keys() - reference.keys())
        return {"passed": False, "first_difference": {"file": "<case>", "row": 0,
                "key": "stream_files", "reference": missing, "current": extra}}
    if gate != "all":
        required = {str(rank) for rank in range(6)}
        for label, case in (("reference", reference), ("current", current)):
            present = set()
            for rows in case.values():
                _, anchors = cutoff_rows(rows, gate)
                present.update(anchors)
            if present != required:
                return {"passed": False, "first_difference": {"file": "<case>", "row": 0,
                        "key": f"{label}_anchor_ranks", "reference": sorted(required),
                        "current": sorted(present)}}
    for filename in sorted(reference):
        left, anchors_left = cutoff_rows(reference[filename], gate)
        right, anchors_right = cutoff_rows(current[filename], gate)
        if anchors_left != anchors_right:
            return {"passed": False, "first_difference": {"file": filename, "row": 0,
                    "key": "anchor_ranks", "reference": sorted(anchors_left), "current": sorted(anchors_right)}}
        for index in range(max(len(left), len(right))):
            if index >= len(left) or index >= len(right):
                a = left[index] if index < len(left) else None
                b = right[index] if index < len(right) else None
                return {"passed": False, "first_difference": {"file": filename, "row": index + 1,
                        "key": "row_presence/order", "reference": a.marker if a else "<missing>",
                        "current": b.marker if b else "<missing>"}}
            diff = compare_row(left[index], right[index])
            if diff:
                return {"passed": False, "first_difference": {"file": filename, "row": index + 1,
                        "key": diff, "reference": left[index].fields, "current": right[index].fields}}
    return {"passed": True, "first_difference": None, "streams": sorted(reference)}


def load_case(path: str, gate: str) -> dict[str, list[Row]]:
    all_streams = streams(Path(path))
    return {name: rows for name, rows in all_streams.items()}


def self_check() -> dict[str, Any]:
    """Exercise counter, byte, ordering, anchor and ignored-measurement behavior."""
    def make_case(bump: str = "", overshoot: bool = False) -> dict[str, list[Row]]:
        result = {}
        for physical in range(6):
            rows = []
            for rank in range(6):
                vector = "1,2,4" if bump != "vector" else "1,3,4"
                stage = f"NATIVE3D_STAGE rank={rank} stage=window rounds=2 maximum_work_bytes={10 + (1 if bump == 'bytes' else 0)} seed_cell_classes={vector} seconds={1.2 if bump != 'seconds' else 900.0} process_peak_rss={100 + (1 if bump == 'rss' else 0)}"
                if bump == "counter":
                    stage = stage.replace("rounds=2", "rounds=3")
                rows.append(Row("NATIVE3D_STAGE", dict(FIELD.findall(stage.replace("NATIVE3D_STAGE ", ""))), f"rank.{physical}/stdout", 1))
                for n in range(3):
                    debt_removed = 0.125 if bump == "debt_removed" and rank == 2 and n == 0 else 0.0
                    q = f"NATIVE3D_QUALITY rank={rank} stage=window census={n} commits={n} elapsed={n}.5 debt_removed={debt_removed}"
                    if bump == "quality" and rank == 2 and n == 0:
                        q = q.replace("commits=0", "commits=1")
                    rows.append(Row("NATIVE3D_QUALITY", dict(FIELD.findall(q.replace("NATIVE3D_QUALITY ", ""))), f"rank.{physical}/stdout", n + 2))
                drift = (f"NATIVE3D_QUALITY_DRIFT rank={rank} stage=window initialized=1 available=1 samples="
                         f"{5 if bump == 'drift' else 4} supported=3 lost=1 owned=1 maximum_relative_frobenius=0.2 "
                         "owned_maximum_relative_frobenius=0.3 current_nodal_sensor=1 full_target=0")
                rows.append(Row("NATIVE3D_QUALITY_DRIFT", dict(FIELD.findall(drift.replace("NATIVE3D_QUALITY_DRIFT ", ""))),
                                f"rank.{physical}/stdout", 5))
                location = (f"NATIVE3D_QUALITY_LOCATION rank={rank} census=2 q_surface="
                            f"{2 if bump == 'location' else 1} q_interior=2 short09_share=0.5 q_miss_hist=1,2,3")
                rows.append(Row("NATIVE3D_QUALITY_LOCATION", dict(FIELD.findall(location.replace("NATIVE3D_QUALITY_LOCATION ", ""))),
                                f"rank.{physical}/stdout", 5))
                for kind in range(3):
                    ladder = (f"NATIVE3D_QUALITY_LADDER rank={rank} census=2 kind={kind} trials="
                              f"{2 if bump == 'ladder' and rank == 0 and kind == 0 else 1} certified=1 commits=1 "
                              "optimizer_work=2 seconds=0.1 debt_removed=0.2 q_removed=1 j_removed=1 rejected=0,1,0")
                    rows.append(Row("NATIVE3D_QUALITY_LADDER", dict(FIELD.findall(ladder.replace("NATIVE3D_QUALITY_LADDER ", ""))),
                                    f"rank.{physical}/stdout", 5))
                detail = "NATIVE3D_DETAIL stage=refine rank={} ranks=6 requests=3 hits=1 source_point_queries=2".format(rank)
                rows.append(Row("NATIVE3D_DETAIL", dict(FIELD.findall(detail.replace("NATIVE3D_DETAIL ", ""))), f"rank.{physical}/stdout", 6))
                native = (f"NATIVE3D rank={rank} ranks=6 workers=6 complete=1 detailed=1 initial_cells=7 final_cells=9 "
                          "tracked_bytes=20 process_peak_rss=100 total=1.0 elapsed=0 "
                          f"nested_tracked_collectives={9 if bump == 'ranktimes' else 0}")
                rows.append(Row("NATIVE3D rank", dict(FIELD.findall(native.replace("NATIVE3D ", ""))), f"rank.{physical}/stdout", 7))
                timer = "NATIVE3D_TIMER root=n boundary=final rank={} phase=quality operation=census calls=4 inclusive_call_seconds=1.0 exclusive_seconds=0.5".format(rank)
                if bump == "timer" and rank == 0:
                    timer = timer.replace("calls=4", "calls=5")
                rows.append(Row("NATIVE3D_TIMER", dict(FIELD.findall(timer.replace("NATIVE3D_TIMER ", ""))), f"rank.{physical}/stdout", 8))
                anchor = f"NATIVE3D_STAGE rank={rank} stage=length_collapse_exit rounds=1 maximum_work_bytes=9"
                rows.append(Row("NATIVE3D_STAGE", dict(FIELD.findall(anchor.replace("NATIVE3D_STAGE ", ""))),
                                f"rank.{physical}/stdout", 9))
            if overshoot:
                rows.append(Row("NATIVE3D_STAGE", {"rank": str(physical), "stage": "after_anchor", "rounds": "99"},
                                f"rank.{physical}/stdout", 9))
            result[f"rank_output/1/rank.{physical}/stdout"] = rows
        return result

    base = make_case()
    current = make_case(overshoot=True)
    assert compare_streams(base, current, "G2")["passed"]
    assert compare_streams(base, make_case("rss"), "G2")["passed"]
    assert compare_streams(base, make_case("seconds"), "G2")["passed"]
    assert compare_streams(base, make_case("ranktimes"), "G2")["passed"]
    assert compare_streams(base, make_case("counter"), "G2")["first_difference"]["key"].startswith("integer.rounds")
    assert compare_streams(base, make_case("bytes"), "G2")["first_difference"]["key"].startswith("integer.maximum_work_bytes")
    assert compare_streams(base, make_case("vector"), "G2")["first_difference"]["key"].startswith("integer.seed_cell_classes")
    assert compare_streams(base, make_case("timer"), "G2")["first_difference"]["key"].startswith("integer.calls")
    assert compare_streams(base, make_case("quality"), "GQ")["first_difference"]["key"].startswith("integer.commits")
    assert compare_streams(base, make_case("debt_removed"), "GQ")["passed"]
    assert compare_streams(base, make_case("drift"), "all")["first_difference"]["key"].startswith("integer.samples")
    assert compare_streams(base, make_case("location"), "all")["first_difference"]["key"].startswith("integer.q_surface")
    assert compare_streams(base, make_case("ladder"), "all")["first_difference"]["key"].startswith("integer.trials")
    missing = make_case(); missing["rank_output/1/rank.3/stdout"] = missing["rank_output/1/rank.3/stdout"][:-1]
    assert not compare_streams(base, missing, "all")["passed"]
    reordered = make_case(); reordered["rank_output/1/rank.0/stdout"][0:2] = reversed(reordered["rank_output/1/rank.0/stdout"][0:2])
    assert not compare_streams(base, reordered, "all")["passed"]
    anchor_missing = make_case()
    for name in anchor_missing:
        anchor_missing[name] = [row for row in anchor_missing[name]
                                if not (row.rank() == "5" and row.fields.get("stage") == "length_collapse_exit")]
    assert not compare_streams(base, anchor_missing, "G2")["passed"]
    # A candidate changing an expected integer counter to decimal must fail by type.
    typed = make_case(); typed["rank_output/1/rank.0/stdout"][0].fields["rounds"] = "2.0"
    assert not compare_streams(base, typed, "all")["passed"]
    # MPI prefixes are stripped by the same helper used in marker extraction.
    assert strip_mpi_prefixes("Sat Nov 14 14:42:00 2026<stdout>:NATIVE3D rank=0") == "NATIVE3D rank=0"
    families = ROW_MARKER.findall("NATIVE3D_QUALITY_LOCATION NATIVE3D_QUALITY_LADDER NATIVE3D_QUALITY_DRIFT NATIVE3D_QUALITY")
    assert families == ["NATIVE3D_QUALITY_LOCATION", "NATIVE3D_QUALITY_LADDER", "NATIVE3D_QUALITY_DRIFT", "NATIVE3D_QUALITY"]
    return {"passed": True, "started_unix": HELPER_START_UNIX, "checks": 19,
            "causes_and_corrections": [
                {"cause": "Initial helper root was set for the shallower repository-root R path.",
                 "correction": "After relocation to staged57/night_20261115/N4, set root to parents[6]."},
                {"cause": "Synthetic G2 anchor was initially absent, then placed before all rows.",
                 "correction": "Emit one length_collapse_exit anchor after the rows under comparison."},
                {"cause": "The initial marker matcher collapsed QUALITY_LOCATION, QUALITY_LADDER and QUALITY_DRIFT into the base family.",
                 "correction": "Capture each full marker literal, require identity fields by exact marker, and anchor GQ only on base QUALITY."},
                {"cause": "The rank summary prints tracked-collective timings and elapsed fields without a seconds suffix.",
                 "correction": "Added the source-listed rank timing names and elapsed to the measured-field exclusions; byte/call counters remain compared."},
                {"cause": "Set iteration made the first reported differing key nondeterministic.",
                 "correction": "Sort identity and integer key traversals."}],
            "limitations": ["Requires rank_output files to pair by relative path; run.log fallback is one stream.",
                            "G2/GQ anchors are required in every logical rank represented across physical streams."]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("reference", nargs="?")
    parser.add_argument("current", nargs="?")
    parser.add_argument("--gate", choices=("all", "G2", "GQ"), default="all")
    args = parser.parse_args()
    if args.self_check:
        result = self_check()
        print(json.dumps(result, indent=2))
        return 0
    if not args.reference or not args.current:
        parser.error("provide reference case and candidate case")
    try:
        result = compare_streams(load_case(args.reference, args.gate), load_case(args.current, args.gate), args.gate)
    except (OSError, ValueError) as error:
        result = {"passed": False, "error": str(error)}
    print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
