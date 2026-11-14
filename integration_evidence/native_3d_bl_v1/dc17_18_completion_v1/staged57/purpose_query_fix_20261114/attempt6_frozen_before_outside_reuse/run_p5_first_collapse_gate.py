#!/usr/bin/env python3
"""Run the bounded P5 no-BL first-collapse gate; deliberately does not build."""
import argparse
import hashlib
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

SCRIPT = Path(__file__).resolve()
PURPOSE = SCRIPT.parent
R = PURPOSE.parent
REPO = R.parents[3]
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
BINARY = BUILD / "SU2_CFD/src/SU2_CFD"
BUILD_STATUS = PURPOSE / "build_status.json"
SOURCE_PINS = PURPOSE / "source_pins.json"
CLEAN = R / "nobl_clean_check_20261114/run"
REFERENCE494 = R / "nobl_clean_check_20261114/first_collapse_receipt.json"
REFERENCE293 = R / "nobl_cause_confirm_20261114/reference_counts.json"
EXPECTED494 = {0: 480, 1: 472, 2: 344, 3: 263, 4: 473, 5: 464}
MAX_WORK = 108459426
_MPI_PREFIX_STRIPPER = None


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def clean_prefixes(line):
    global _MPI_PREFIX_STRIPPER
    if _MPI_PREFIX_STRIPPER is None:
        sys.path.insert(0, str(R / "nobl_clean_check_20261114"))
        from extract_markers import strip_mpi_prefixes
        _MPI_PREFIX_STRIPPER = strip_mpi_prefixes
    return _MPI_PREFIX_STRIPPER(line)


def read_processes():
    rows = {}
    for proc in Path("/proc").iterdir():
        if not proc.name.isdigit():
            continue
        try:
            fields = (proc / "stat").read_text().rsplit(")", 1)[1].split()
            rows[int(proc.name)] = {
                "pid": int(proc.name), "state": fields[0], "ppid": int(fields[1]),
                "pgid": int(fields[2]), "sid": int(fields[3]),
                "start_ticks": int(fields[19]), "comm": (proc / "comm").read_text().strip(),
            }
        except (OSError, ValueError, IndexError):
            continue
    return rows


def identity_alive(identity, rows):
    row = rows.get(identity["pid"])
    return bool(row and row["start_ticks"] == identity["start_ticks"] and row["state"] not in ("Z", "X"))


def verify_pins(strict=True):
    build_status = json.loads(BUILD_STATUS.read_text())
    if strict:
        assert build_status.get("state") == "terminal" and build_status.get("exit_code") == 0
        assert build_status.get("pins_verified") is True
        assert Path(build_status.get("binary", BINARY)).resolve() == BINARY.resolve()
    binary_sha = sha(BINARY)
    binary_matches_build = build_status.get("binary_sha256") == binary_sha
    if strict:
        assert binary_matches_build, "new binary hash differs from build_status.json"
    pins = json.loads(SOURCE_PINS.read_text())
    if isinstance(pins, dict) and "source_pins" in pins:
        pins = pins["source_pins"]
    if strict:
        assert isinstance(pins, dict) and pins, "source_pins.json must be a nonempty flat absolute path -> sha256 map"
    extra_path=PURPOSE / "unchanged_build494_source_pins.json"
    extra=json.loads(extra_path.read_text())
    assert not (pins.keys() & extra.keys())
    pins.update(extra)
    normalized = {}
    for raw_path, expected in pins.items():
        path = Path(raw_path)
        if strict:
            assert path.is_absolute(), f"source pin path is not absolute: {raw_path}"
            assert re.fullmatch(r"[0-9a-f]{64}", str(expected)), f"invalid SHA256 pin: {raw_path}"
        if not path.is_absolute():
            continue
        path = path.resolve(strict=strict)
        normalized[str(path)] = str(expected)
    mismatched = {}
    for path, expected in normalized.items():
        try:
            actual = sha(path)
        except OSError:
            actual = None
        if actual != expected:
            mismatched[path] = {"expected": expected, "actual": actual}
    if strict:
        assert not mismatched, f"source pin mismatch: {mismatched}"
    return {"binary": str(BINARY), "binary_sha256": binary_sha, "binary_matches_build_status": binary_matches_build,
            "source_pins": normalized, "source_pin_mismatches": mismatched,
            "build_status": str(BUILD_STATUS), "source_pins_sha256": sha(SOURCE_PINS),
            "additional_source_pins_sha256": sha(extra_path)}


def collapse_rows(case):
    result = {}
    for path in sorted(case.glob("rank_output/*/rank.*/stdout")):
        for number, raw in enumerate(path.open(errors="replace"), 1):
            if not raw.endswith("\n"):
                continue
            line = clean_prefixes(raw)
            if "NATIVE3D_STAGE " not in line or "stage=length_collapse_exit " not in line:
                continue
            row = dict(re.findall(r"(\w+)=([^\s]+)", line))
            required = ("rank", "elapsed", "upper_work", "upper_calls", "upper_seconds", "accepted", "length_collapses")
            if not all(key in row for key in required):
                continue
            try:
                rank = int(row["rank"])
                if not (0 <= rank < 6) or float(row["elapsed"]) <= 0:
                    continue
                for key in ("upper_work", "upper_calls", "accepted", "length_collapses"):
                    int(row[key])
                float(row["upper_seconds"])
            except (ValueError, TypeError):
                continue
            row.update(source_file=str(path), source_line=number)
            result[rank] = row
            break
    return result


def host_snapshot():
    return {
        "unix": time.time(), "loadavg": Path("/proc/loadavg").read_text().strip(),
        "MemAvailable": next(line for line in Path("/proc/meminfo").read_text().splitlines()
                              if line.startswith("MemAvailable:")),
        "disk_free_bytes": shutil.disk_usage(PURPOSE).free,
    }


def validate_full_workflow(case):
    """Read-only adaptation of validate_m6293_completion.py's acceptance checks."""
    import csv
    import io
    import math
    import sys

    os.environ.update(OMP_NUM_THREADS="1",OPENBLAS_NUM_THREADS="1",MKL_NUM_THREADS="1")
    import numpy as np

    capability = REPO / "TestCases/adaptation/capability"
    sys.path.insert(0, str(capability))
    import capcheck

    errors = []
    event_rows = []
    rank_logs = sorted(case.glob("rank_output/*/rank.*/stdout"))
    if len(rank_logs) != 6:
        return {"workflow_accepted": False, "errors": [f"expected 6 rank logs, found {len(rank_logs)}"], "events": []}
    per_rank = {rank: {"stages": [], "details": [], "summaries": []} for rank in range(6)}
    for path in rank_logs:
        for number, raw in enumerate(path.open(errors="replace"), 1):
            line = clean_prefixes(raw)
            if "NATIVE3D_STAGE " in line:
                part = line[line.find("NATIVE3D_STAGE "):]
                row = dict(re.findall(r"([\w]+)=([^\s]+)", part))
                if row.get("stage") == "exit":
                    row.update(source_file=str(path), source_line=number)
                    rank = int(row.get("rank", -1))
                    if rank in per_rank:
                        per_rank[rank]["stages"].append(row)
            if "NATIVE3D_DETAIL stage=refine " in line:
                part = line[line.find("NATIVE3D_DETAIL "):]
                row = dict(re.findall(r"([\w]+)=([^\s]+)", part))
                row.update(source_file=str(path), source_line=number)
                rank = int(row.get("rank", -1))
                if rank in per_rank:
                    per_rank[rank]["details"].append(row)
            if "NATIVE3D rank=" in line:
                part = line[line.find("NATIVE3D rank="):]
                row = dict(re.findall(r"([\w]+)=([^\s]+)", part))
                if "final" in row and "total" in row:
                    row.update(source_file=str(path), source_line=number)
                    rank = int(row.get("rank", -1))
                    if rank in per_rank:
                        per_rank[rank]["summaries"].append(row)

    authoritative_logs = list(case.glob("rank_output/*/rank.0/stdout"))
    if len(authoritative_logs) != 1:
        return {"workflow_accepted": False, "errors": [f"expected one isolated rank.0 stdout, found {len(authoritative_logs)}"], "events": []}
    authoritative_log = authoritative_logs[0]
    log = "".join(clean_prefixes(line) for line in authoritative_log.open(errors="replace"))
    history_path = case / "history.csv"
    if not history_path.is_file():
        return {"workflow_accepted": False, "errors": ["missing history.csv"], "events": []}
    history = []
    try:
        for row in csv.DictReader(io.StringIO(history_path.read_text(errors="replace")), skipinitialspace=True):
            row = {key.strip(): value.strip() for key, value in row.items() if key and value is not None}
            if "Adap_Cycle" in row:
                history.append(row)
    except (csv.Error, OSError) as exc:
        errors.append(f"history parse failed: {exc}")

    for event in (1, 2):
        stage_rows = [per_rank[rank]["stages"] for rank in range(6)]
        detail_rows = [per_rank[rank]["details"] for rank in range(6)]
        summary_rows = [per_rank[rank]["summaries"] for rank in range(6)]
        index = event - 1
        if not all(len(rows) == 2 for rows in stage_rows):
            errors.append(f"event {event}: require exactly two stage=exit rows on every rank")
            continue
        if not all(len(rows) == 2 for rows in detail_rows):
            errors.append(f"event {event}: require exactly two refine detail rows on every rank")
            continue
        if not all(len(rows) == 2 for rows in summary_rows):
            errors.append(f"event {event}: require exactly two native summary rows on every rank")
            continue

        stages = [rows[index] for rows in stage_rows]
        details = [rows[index] for rows in detail_rows]
        summaries = [rows[index] for rows in summary_rows]
        for rows, name in ((stages, "stage exit"), (details, "refine detail"), (summaries, "native summary")):
            if {int(row.get("rank", -1)) for row in rows} != set(range(6)):
                errors.append(f"event {event}: {name} rank coverage is not 0..5")

        whole_event_seconds = max(float(row["total"]) for row in summaries)
        refine_seconds = max(float(row["total"]) for row in details)
        native_complete = all(row.get("complete") == "1" and row.get("ranks") == "6" and row.get("workers") == "6"
                              for row in summaries)
        native_complete = native_complete and all(row.get("complete") == "1" and row.get("stop") == "complete" and
                                                  row.get("ranks") == "6" and row.get("workers") == "6" for row in details)
        final_cells = {int(row["final"]) for row in summaries}
        if len(final_cells) != 1:
            errors.append(f"event {event}: native summaries disagree on final cell count")

        summed = {key: sum(int(row[key]) for row in stages) for key in
                  ("cells", "measured", "q_measured", "j_measured", "q_bad", "j_bad", "size_bad", "geometry_bad")}
        flags = ("cache_consistent", "census_complete", "shape_complete", "geometry_complete")
        cert_complete = all(all(row.get(flag) == "1" for flag in flags) for row in stages)
        cert_complete = cert_complete and summed["cells"] == summed["measured"] == summed["q_measured"] == summed["j_measured"]
        cert_complete = cert_complete and all(summed[key] == 0 for key in ("q_bad", "j_bad", "size_bad", "geometry_bad"))
        if whole_event_seconds > 5400:
            errors.append(f"event {event}: whole NATIVE3D event elapsed {whole_event_seconds:.6f}s exceeds 5400s")
        if refine_seconds > 5400:
            errors.append(f"event {event}: refine detail elapsed {refine_seconds:.6f}s exceeds 5400s")
        if not native_complete:
            errors.append(f"event {event}: native rank/detail completion flags failed")
        if not cert_complete:
            errors.append(f"event {event}: exit certification incomplete or failed a final q/J/geometry/size gate")

        mesh_name = f"adapted_mesh_adap_{event:05d}.su2"
        ref_name = mesh_name + ".native_ref"
        restart_name = f"restart_flow_adap_{event:05d}.dat"
        mesh_path, ref_path, restart_path = (case / name for name in (mesh_name, ref_name, restart_name))
        if not all(path.is_file() and path.stat().st_size for path in (mesh_path, ref_path, restart_path)):
            errors.append(f"event {event}: missing or empty adapted mesh/reference/restart output")
            continue
        try:
            mesh = capcheck.read_su2(mesh_path)
            positions, fields, precision = capcheck.read_restart(restart_path)
            if mesh.dim != 3 or not np.isfinite(mesh.P).all() or mesh.E.size == 0:
                raise ValueError("mesh is not a finite nonempty 3D mesh")
            if mesh.E.min() < 0 or mesh.E.max() >= len(mesh.P):
                raise ValueError("mesh cell connectivity is outside point range")
            cell_volumes = capcheck.volumes(mesh.P, mesh.E, mesh.dim)
            if not np.isfinite(cell_volumes).all() or np.any(cell_volumes <= 0):
                raise ValueError("mesh has nonpositive or nonfinite cell volumes")
            if summed["cells"] != len(mesh.E) or (final_cells and next(iter(final_cells)) != len(mesh.E)):
                raise ValueError("certified/final cell count differs from actual mesh")
            if positions.shape != mesh.P.shape or not np.isfinite(positions).all():
                raise ValueError("restart coordinates are missing, nonfinite, or wrong-sized")
            coordinate_error = float(np.max(np.abs(positions - mesh.P)))
            if coordinate_error > 1e-12:
                raise ValueError(f"restart coordinates differ from actual mesh by {coordinate_error:.3e}")
            required = ("Density", "Momentum_x", "Momentum_y", "Momentum_z", "Energy")
            if any(name not in fields or not np.isfinite(fields[name]).all() for name in required):
                raise ValueError("restart lacks finite conservative solution fields")
            rho = fields["Density"]
            internal = fields["Energy"] - sum(fields[f"Momentum_{axis}"] ** 2 for axis in "xyz") / (2 * rho)
            if np.any(rho <= 0) or np.any(internal <= 0):
                raise ValueError("restart contains nonpositive density or internal energy")
            mesh_counts = {"points": int(len(mesh.P)), "cells": int(len(mesh.E))}
            positive_solution = {"minimum_density": float(rho.min()),
                                 "minimum_internal_energy_density": float(internal.min()),
                                 "restart_precision": precision}
        except Exception as exc:
            errors.append(f"event {event}: mesh/restart gate failed: {exc}")
            mesh_counts, coordinate_error, positive_solution = {}, None, {}

        cycle_start, cycle_end = f"Cycle {event-1} of 2:", f"Cycle {event} of 2:"
        if cycle_start not in log or cycle_end not in log:
            errors.append(f"event {event}: missing cycle boundary markers")
            section = ""
        else:
            section = log.split(cycle_start, 1)[1].split(cycle_end, 1)[0]
        required_markers = ["------------------- Solution Transfer (conservative P1) -------------------",
                            "Change of the integrals (sum of value x control volume)", "Time: setup ",
                            "The problem uses the new mesh:", "Mesh replaced in ",
                            f"Writing the adapted mesh of cycle {event:05d}: adapted_mesh_adap_{event:05d}.su2."]
        marker_positions = [section.find(marker) for marker in required_markers]
        native_position = section.rfind("NATIVE3D rank=")
        ordered = (native_position >= 0 and all(position >= 0 for position in marker_positions) and
                   [native_position] + marker_positions == sorted([native_position] + marker_positions))
        if not ordered:
            errors.append(f"event {event}: transfer/replacement/publication markers missing or out of order")
        conservation = ""
        if ordered:
            conservation = section[marker_positions[1]:marker_positions[3]]
        residuals = {}
        table = ""
        if "Per field: S_n content and S_d content" in section and "States not admissible after the projection" in section:
            table = section.split("Per field: S_n content and S_d content", 1)[1].split(
                "States not admissible after the projection", 1)[0]
        for field in ("Density", "Momentum-x", "Momentum-y", "Momentum-z", "Energy"):
            try:
                line = next(line for line in table.splitlines() if line.strip().startswith(field + ":"))
                values = line.split(":", 1)[1].split()
                value = float(values[-2]) if len(values) == 7 else math.nan
            except (StopIteration, ValueError, IndexError):
                value = math.nan
            if math.isfinite(value):
                residuals[field] = value
        if len(residuals) != 5 or any(value > 1e-10 for value in residuals.values()):
            errors.append(f"event {event}: conservative transfer residual gate failed")
        if not re.search(r"States not admissible after the projection[^\n]*: 0;", section):
            errors.append(f"event {event}: projected inadmissible states were not zero")

        cycle_next = event
        next_section = log.split(cycle_end, 1)[1] if cycle_end in log else ""
        flow = re.search(r"\|\s*0\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|", next_section)
        following_history = []
        for row in history:
            try:
                if (int(row["Adap_Cycle"]) == cycle_next and
                    all(math.isfinite(float(row[name])) for name in ("Inner_Iter", "Adap_Iter", "rms[Rho]"))):
                    following_history.append(row)
            except (KeyError, ValueError):
                continue
        if not flow or not all(math.isfinite(float(value)) for value in flow.groups()) or not following_history:
            errors.append(f"event {event}: following-cycle solve/history evidence missing")

        event_rows.append({
            "event": event,
            "whole_event_seconds_max_rank": whole_event_seconds,
            "refine_detail_seconds_max_rank": refine_seconds,
            "native_seconds_limit": 5400,
            "native_summaries": summaries, "refine_details": details,
            "certification_six_rank_sums": summed, "certification_complete": cert_complete,
            "actual_mesh_counts": mesh_counts, "mesh_ref_sha256": sha(ref_path),
            "coordinate_maximum_error": coordinate_error, "positive_solution": positive_solution,
            "transfer_residuals": residuals, "projected_inadmissible_zero": bool(re.search(
                r"States not admissible after the projection[^\n]*: 0;", section)),
            "ordered_transfer_replacement_publication": ordered,
            "continued_cfd_row": list(map(float, flow.groups())) if flow else None,
            "continued_history_rows": following_history,
            "outputs": {name: sha(case / name) for name in (mesh_name, ref_name, restart_name)},
        })
    return {"workflow_accepted": len(event_rows) == 2 and not errors,
            "strict_gates": {"events": 2, "per_event_native_seconds_max": 5400,
                             "no_fixed_mesh_point_or_cell_target": True,
                             "mesh_counts_must_match_certification_totals": True,
                             "q_j_geometry_size_bad_must_be_zero": True},
            "errors": errors, "events": event_rows,
            "authoritative_event_text_source": str(authoritative_log),
            "native_row_sources": {str(rank): {kind: [
                {"source_file": row["source_file"], "source_line": row["source_line"]}
                for row in per_rank[rank][kind]] for kind in ("stages", "details", "summaries")}
                for rank in range(6)},
            "timing_conventions": {
                "NATIVE3D_rank_total": "whole CNativeRemesher3D::Remesh duration (stats.total_seconds from parent.seconds()-start); this is the 5400s gate.",
                "NATIVE3D_DETAIL_refine_total": "nested Refine-stage duration; reported independently and also bounded by 5400s.",
                "attribution": "rank summary is the whole-event timing; refine detail is a nested component, not additive. Both are checked, not summed."
            },
            "validation_scope": "Read-only rank.0 stdout, all isolated rank exit rows, actual mesh, transfer and continued-CFD checks adapted from validate_m6293_completion.py; no isolated disk-restart campaign claimed."}


def descendants(rows, roots):
    owned = set(roots)
    changed = True
    while changed:
        added = {pid for pid, row in rows.items() if row["ppid"] in owned}
        changed = not added.issubset(owned)
        owned.update(added)
    return owned


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case_dir", help="fresh output directory under purpose_query_fix_20261114")
    parser.add_argument("--full", action="store_true", help="run the complete two-event no-BL workflow")
    args = parser.parse_args()
    full_mode = args.full
    case = Path(args.case_dir).expanduser().resolve()
    assert case != PURPOSE and PURPOSE in case.parents, "case_dir must be a child of purpose_query_fix_20261114"
    assert not case.exists(), f"refusing existing output directory: {case}"
    assert not any((case / name).exists() for name in ("status.json", "provenance.json", "comparison.json"))

    # Validate prerequisites before creating output or launching any process.
    pins_before = verify_pins()
    clean_provenance = json.loads((CLEAN / "provenance.json").read_text())
    baseline293 = json.loads(REFERENCE293.read_text())
    baseline494 = baseline293["build494"]
    if not full_mode:
        baseline494_receipt = json.loads(REFERENCE494.read_text())
        assert isinstance(baseline494_receipt, list) and len(baseline494_receipt) == 6
        assert {int(row["rank"]): int(row["accepted"]) for row in baseline494_receipt} == EXPECTED494
    assert {int(row["rank"]): int(row["accepted"]) for row in baseline494["per_rank"]} == EXPECTED494

    existing = read_processes()
    conflicting = [row for row in existing.values() if row["comm"] in ("SU2_CFD", "test_driver", "cc1plus", "lto1")]
    assert not conflicting, f"owned native/build process already present: {conflicting}"
    assert shutil.disk_usage(PURPOSE).free > 2_000_000_000, "less than 2 GB free on evidence filesystem"
    launch = host_snapshot()

    case.mkdir(parents=True)
    (case / "mpi_tmp").mkdir()
    config = (R / "m6_inputs/euler_adapted293_event1_v1/run.cfg") if full_mode else (CLEAN / "run.cfg")
    assert sha(config) == clean_provenance["config_sha256"]
    if full_mode:
        config_text = config.read_text()
        assert re.search(r"(?m)^ADAP_NATIVE_MAX_TIME\s*=\s*5400\s*$", config_text)
        assert re.search(r"(?m)^ADAP_SUBITER\s*=\s*\(\s*2\s*\)\s*$", config_text)
        assert re.search(r"(?m)^ADAP_NATIVE_RANKS\s*=\s*6\s*$", config_text)
    shutil.copyfile(config, case / "run.cfg")
    input_hashes = {str(case / "run.cfg"): sha(case / "run.cfg"), str(BINARY): sha(BINARY)}
    sys.path.insert(0,str(R))
    import run_cases as runtime_checks
    runtime_checks.BINARY=BINARY
    runtime_manifest=R / "runtime_20261113_474/loaded_runtime_pins.json"
    runtime_pins=json.loads(runtime_manifest.read_text())
    assert set(runtime_checks.loaded_libraries()).issubset(runtime_pins), "unadmitted loaded runtime library"
    assert all(sha(path)==digest for path,digest in runtime_pins.items()), "runtime dependency changed"
    input_hashes.update(runtime_pins)
    for path in (SCRIPT,Path(runtime_checks.__file__),runtime_manifest,BUILD_STATUS,SOURCE_PINS,
                 PURPOSE / "unchanged_build494_source_pins.json",R / "nobl_clean_check_20261114/extract_markers.py",
                 REFERENCE494,REFERENCE293,CLEAN / "provenance.json"):
        input_hashes[str(path)]=sha(path)
    for name in ("m6.su2", "m6.su2.native_ref", "baseline_euler_seed.dat"):
        source = (R / "m6_inputs/euler_original_force_seed_v1" / name).resolve(strict=True)
        expected = clean_provenance["input_hashes"][str(source)]
        assert sha(source) == expected, f"baseline input hash changed: {source}"
        (case / name).symlink_to(source)
        input_hashes[str(source)] = expected

    old_case = str(CLEAN)
    argv = [item.replace(old_case, str(case)) for item in clean_provenance["argv"]]
    old_binary = next(path for path in clean_provenance["input_hashes"]
                      if Path(path).name == "SU2_CFD" and "build-integrated-v3-currenthost" in path)
    assert argv.count(old_binary) == 1
    argv = [str(BINARY) if item == old_binary else item for item in argv]
    env = os.environ.copy()
    for key in list(env):
        if key.startswith("SU2_NATIVE_"):
            del env[key]
    env.update(clean_provenance["environment"])
    env["OMPI_MCA_orte_tmpdir_base"] = str(case / "mpi_tmp")
    assert "perf" not in argv and "gdb" not in argv and argv[-1] == "run.cfg"
    assert "--output-filename" in argv and str(case / "rank_output") in argv
    provenance = {
        "argv": argv,
        "environment": {key: value for key, value in env.items() if key in clean_provenance["environment"]},
        "input_hashes": input_hashes,
        "config_sha256": sha(case / "run.cfg"),
        "binary_sha256": pins_before["binary_sha256"],
        "baseline_binary_sha256": clean_provenance["input_hashes"][old_binary],
        "source_pins_sha256": pins_before["source_pins_sha256"],
        "controller_sha256": sha(SCRIPT),
        "additional_source_pins_sha256": pins_before["additional_source_pins_sha256"],
        "reference_494": str(REFERENCE494), "reference_293": str(REFERENCE293),
        "max_work_units": MAX_WORK,
        "mode": "full_two_event_workflow" if full_mode else "first_collapse_only",
        "native_time_limit_seconds_per_event": 5400 if full_mode else None,
        "application_watchdog_seconds": 14400,
        "expected_build494_accepted_by_rank": {str(k): v for k, v in EXPECTED494.items()},
        "stop_rule": "all six rank-separated length_collapse_exit rows or 1500 seconds from process launch",
        "command_basis": str(CLEAN / "provenance.json"),
        "host_at_launch": launch,
    }
    save(case / "provenance.json", provenance)
    seen = {}
    start = time.monotonic()
    started_unix = time.time()
    stop_reason = None
    captured = {}
    with (case / "run.log").open("x") as log, (case / "host_resources.jsonl").open("x") as receipts:
        proc = subprocess.Popen(argv, cwd=case, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        current = read_processes()
        root = current.get(proc.pid)
        assert root, "could not verify launched process identity"
        root_stamp = {"pid": root["pid"], "start_ticks": root["start_ticks"], "pgid": root["pgid"]}
        seen[(root["pid"], root["start_ticks"])] = root
        save(case / "status.json", {"state": "running", "started_unix": started_unix, "identity": root_stamp})
        print(json.dumps({"launched": root_stamp, "case": str(case)}), flush=True)
        while True:
            rows = read_processes()
            owned = descendants(rows, {row["pid"] for row in seen.values() if identity_alive(row, rows)})
            sampled = []
            for pid in owned:
                row = rows.get(pid)
                if not row:
                    continue
                seen[(pid, row["start_ticks"])] = row
                memory = {}
                try:
                    for line in (Path("/proc") / str(pid) / "status").read_text().splitlines():
                        if ":" in line:
                            key, value = line.split(":", 1)
                            if key in ("VmRSS", "VmHWM"):
                                memory[key] = int(value.split()[0])
                except OSError:
                    pass
                sampled.append({**row, **memory})
            elapsed = time.monotonic() - start
            receipts.write(json.dumps({"unix": time.time(), "elapsed": elapsed,
                                      "host": host_snapshot(), "owned_processes": sampled}) + "\n")
            receipts.flush()
            if len(captured)<6:captured = collapse_rows(case)
            if not full_mode and set(captured) == set(range(6)):
                stop_reason = "all_six_first_collapse_exit_rows"
                save(case / "collapse_records.json", captured)
                break
            if proc.poll() is not None:
                stop_reason = "normal_process_exit" if full_mode else "process_exited_before_all_six_first_collapse_exit_rows"
                break
            if not full_mode and elapsed >= 1500:
                stop_reason = "1500_second_deadline"
                save(case / "partial_collapse_records.json", captured)
                break
            if full_mode and elapsed >= 14500:
                stop_reason = "controller_watchdog_after_14400_second_command_timeout"
                break
            time.sleep(10 if full_mode else min(5, max(.05, 1500 - elapsed)))

        stop_elapsed = time.monotonic() - start
        current = read_processes()
        live = [row for row in seen.values() if identity_alive(row, current)]
        # Signal the process group only if its verified session leader is still the process we launched.
        if proc.poll() is None and identity_alive(root_stamp, current) and os.getpgid(proc.pid) == root_stamp["pgid"]:
            os.killpg(root_stamp["pgid"], signal.SIGTERM)
        for attempt in range(15):
            current = read_processes()
            live = [row for row in seen.values() if identity_alive(row, current)]
            if not live:
                break
            if attempt in (4, 9):
                sig = signal.SIGTERM if attempt == 4 else signal.SIGKILL
                for row in live:
                    if identity_alive(row, read_processes()):
                        try:
                            os.kill(row["pid"], sig)
                        except ProcessLookupError:
                            pass
            time.sleep(1)
        code = proc.wait(timeout=10)

    if captured:
        save(case / ("first_collapse_records.json" if full_mode else "collapse_records.json"), captured)
    if full_mode and set(captured) != set(range(6)):
        save(case / "partial_first_collapse_records.json", captured)
    current = read_processes()
    live = [row for row in seen.values() if identity_alive(row, current)]
    inputs_changed = []
    for path, expected in input_hashes.items():
        try:
            actual = sha(path)
        except OSError:
            actual = None
        if actual != expected:
            inputs_changed.append({"path": path, "expected": expected, "actual": actual})
    pins_after = verify_pins(strict=False)
    pin_changes = (pins_before["source_pins"] != pins_after["source_pins"] or
                   pins_before["source_pins_sha256"] != pins_after["source_pins_sha256"] or
                   pins_before["additional_source_pins_sha256"] != pins_after["additional_source_pins_sha256"] or
                   pins_before["binary_sha256"] != pins_after["binary_sha256"] or
                   bool(pins_after["source_pin_mismatches"]) or not pins_after["binary_matches_build_status"])
    first_collapse_rows = []
    for rank in range(6):
        row = captured.get(rank)
        if not row:
            first_collapse_rows.append({"rank": rank, "captured": False, "build494_accepted_expected": EXPECTED494[rank]})
            continue
        first_collapse_rows.append({
            "rank": rank, "accepted": int(row["accepted"]), "upper_work": int(row["upper_work"]),
            "elapsed_seconds": float(row["elapsed"]), "upper_calls": int(row["upper_calls"]),
            "source_file": row["source_file"], "source_line": row["source_line"],
            "build494_accepted_expected": EXPECTED494[rank],
            "accepted_matches_build494": int(row["accepted"]) == EXPECTED494[rank],
            "accepted_difference_from_build494": int(row["accepted"]) - EXPECTED494[rank],
        })
    work_sum = sum(int(row["upper_work"]) for row in captured.values())
    first_collapse_gate = {
        "schema": "p5_first_collapse_gate_comparison_v1",
        "captured_ranks": sorted(captured), "all_six_captured": set(captured) == set(range(6)),
        "upper_work_sum": work_sum, "upper_work_limit": MAX_WORK,
        "upper_work_pass": set(captured) == set(range(6)) and work_sum <= MAX_WORK,
        "per_rank": first_collapse_rows,
        "accepted_counts_match_build494": set(captured) == set(range(6)) and all(
            int(captured[rank]["accepted"]) == EXPECTED494[rank] for rank in range(6)),
        "accepted_count_differences_are_reported_without_tuning": True,
        "build293_reference": baseline293["build293"],
        "build494_reference_six_rank_sums": baseline494["six_rank_sums"],
        "build494_reference_per_rank": baseline494["per_rank"],
    }
    save(case / "comparison.json", first_collapse_gate)
    full_receipt = None
    if full_mode and stop_reason == "normal_process_exit" and code == 0:
        full_receipt = validate_full_workflow(case)
    elif full_mode:
        full_receipt = {"workflow_accepted": False,
                        "errors": [f"full run did not end normally with exit 0 (stop={stop_reason}, exit={code})"],
                        "events": [], "validation_scope": "full workflow acceptance validation not reached"}
    if full_mode:
        save(case / "full_workflow_receipt.json", full_receipt)
    rank0_logs = list(case.glob("rank_output/*/rank.0/stdout"))
    native_started = bool(captured) or any("Begin Solver" in path.read_text(errors="replace") for path in rank0_logs)
    result = {
        "state": "terminal", "exit_code": code, "stop_reason": stop_reason,
        "seconds_until_stop": stop_elapsed, "seconds_total": time.monotonic() - start,
        "captured_ranks": sorted(captured), "all_owned_handles_terminal": not live,
        "live_owned_handles": live, "owned_identities": list(seen.values()),
        "inputs_unchanged": not inputs_changed, "changed_inputs": inputs_changed,
        "source_and_binary_pins_unchanged": not pin_changes,
        "source_pin_mismatches_after": pins_after["source_pin_mismatches"],
        "native_case_started": native_started,
        "mode": "full_two_event_workflow" if full_mode else "first_collapse_only",
        "first_collapse_gate": first_collapse_gate,
        "full_workflow_accepted": full_receipt["workflow_accepted"] if full_receipt else None,
        "full_workflow_errors": full_receipt["errors"] if full_receipt else None,
    }
    save(case / "status.json", result)
    print(json.dumps(result), flush=True)
    raise SystemExit(bool(live) or bool(inputs_changed) or pin_changes or not first_collapse_gate["upper_work_pass"] or
                     not first_collapse_gate["accepted_counts_match_build494"] or not result["native_case_started"] or
                     (full_mode and not result["full_workflow_accepted"]))


if __name__ == "__main__":
    main()
