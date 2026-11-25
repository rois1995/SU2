"""Read-only P7/P8 event gates and diagnostic counter summary."""
import importlib.util
import json
import re
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = next(parent for parent in HERE.parents if parent.name == "staged57")
TIMER_PATH = STAGED / "f3b_20261115" / "check_timer_tree.py"
SPEC = importlib.util.spec_from_file_location("p7p8_timer_tree", TIMER_PATH)
timer = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(timer)

RANKS = 6
MAX_JOB_BYTES = 19073 * 1024 * 1024
EXPECTED_BOUND_CALLS = {1: 31622, 2: 35291}


def fields(line, marker):
    """Parse one marker after removing MPI prefixes."""
    clean = timer.strip_mpi_prefixes(line)
    match = re.search(r"\b" + re.escape(marker) + r"\s+(.*)", clean)
    return dict(re.findall(r"([\w]+)=([^\s]+)", match[1])) if match else None


def event_summary_blocks(lines):
    """Collect complete six-rank summary groups while ignoring partial final output."""
    blocks, current = [], {}
    for line in lines:
        if not line.endswith(("\n", "\r")):
            continue
        row = fields(line, "NATIVE3D")
        if not row or "complete" not in row or not row.get("rank", "").isdigit():
            continue
        rank = int(row["rank"])
        if rank not in range(RANKS):
            continue
        if rank in current or (rank == 0 and current):
            current = {}
        current[rank] = row
        if set(current) == set(range(RANKS)):
            blocks.append(current)
            current = {}
    return blocks


def read_event(case, event):
    """Pair event-ordinal summary, local exit rows, and passed terminal timer roots."""
    roots = sorted((Path(case) / "rank_output").glob("*"))
    if event not in (1, 2) or len(roots) != 1 or roots[0].name != "1":
        return None
    base = Path(case) / "rank_output" / "1"
    root = base / "rank.0" / "stdout"
    if not root.is_file():
        return None
    blocks = event_summary_blocks(root.read_text(errors="replace").splitlines(keepends=True))
    if len(blocks) > 2 or len(blocks) < event or any(row.get("complete") != "1" for row in blocks[event - 1].values()):
        return None
    local, exits, final_timers = {}, {}, {}
    for rank in range(RANKS):
        path = base / f"rank.{rank}" / "stdout"
        if not path.is_file():
            return None
        lines = [line for line in path.read_text(errors="replace").splitlines(keepends=True)
                 if line.endswith(("\n", "\r"))]
        snapshots = timer.snapshots("".join(lines))
        timer_roots, cursor = [], 0
        for item in snapshots:
            while cursor < len(lines) and fields(lines[cursor], "NATIVE3D_TIMER_SUMMARY") != item["summary"]:
                cursor += 1
            if cursor == len(lines):
                return None
            index = cursor
            cursor += 1
            summary = item["summary"]
            if summary.get("root") == "remesh" and summary.get("boundary") == "final" and summary.get("rank") == str(rank):
                if not timer_roots or json.dumps(item, sort_keys=True) != json.dumps(timer_roots[-1][1], sort_keys=True):
                    timer_roots.append((index, item))
        if len(timer_roots) < event or not timer.assess(timer_roots[event - 1][1])["passed"]:
            return None
        segment_start = timer_roots[event - 2][0] + 1 if event > 1 else 0
        segment_end, final_timers[rank] = timer_roots[event - 1]
        local[rank] = lines[segment_start:segment_end + 1]
        exit_rows = [fields(line, "NATIVE3D_STAGE") for line in local[rank]]
        exit_rows = [row for row in exit_rows if row and row.get("stage") == "exit" and row.get("rank") == str(rank)]
        if not exit_rows:
            return None
        exits[rank] = exit_rows[-1]
    return blocks[event - 1], local, exits, final_timers


def assess_event(case, event):
    """Wait for complete event data, then gate proof coverage and whole-job peak memory."""
    parsed = read_event(case, event)
    if parsed is None:
        return None
    summaries, local, exits, final_timers = parsed
    proof_rows = [row for row in summaries.values() if all(key in row for key in
                   ("final_proof_faces", "final_proof_surface_faces"))]
    surfaces = [int(row["final_proof_surface_faces"]) for row in proof_rows]
    owned_faces = sum(int(row["final_proof_faces"]) for row in proof_rows)
    coverage = len(proof_rows) == RANKS and len(set(surfaces)) == 1 and surfaces[0] > 0 and owned_faces == surfaces[0]
    job = [int(row["job_peak_rss"]) for row in summaries.values() if "job_peak_rss" in row]
    process = [int(row["process_peak_rss"]) for row in summaries.values() if "process_peak_rss" in row]
    exact = len(job) == RANKS and len(set(job)) == 1 and len(process) == RANKS and sum(process) == job[0]
    under_limit = len(job) == RANKS and job[0] <= MAX_JOB_BYTES
    lower = [int(row["job_peak_rss_lower"]) for row in exits.values() if "job_peak_rss_lower" in row]
    lower_ok = len(lower) == RANKS and len(job) == RANKS and max(lower) <= job[0]
    calls = sum(int(row["calls"]) for item in final_timers.values() for row in item["rows"]
                if row.get("phase") == "final_proof" and row.get("operation") == "geometry_bound")
    expected = EXPECTED_BOUND_CALLS.get(event)
    m18 = sum(all(name in exits[rank] for name in
                  ("distinct_facets", "distinct_scope_keys", "prepared_facet_mean_bytes")) for rank in exits)
    m20, m21 = [], []
    for rank, lines in local.items():
        for line in lines:
            private = fields(line, "NATIVE3D_PRIVATE_WORK")
            quality = fields(line, "NATIVE3D_QUALITY")
            ownership = fields(line, "NATIVE3D_OWNERSHIP_MEMORY")
            if private:
                m20.append(dict(private, rank=rank, record="private_round"))
            if quality and "private_work_used" in quality:
                m20.append({key: quality[key] for key in ("round", "window", "stage", "private_work_used", "busy_seconds")
                            if key in quality} | {"rank": rank, "record": "quality_window"})
            if ownership:
                m21.append(dict(ownership, rank=rank))
    gates = {"proof_face_coverage": coverage, "job_peak_equals_process_peak_sum": exact,
             "job_peak_within_19073_MiB": under_limit, "engine_exit_lower_bound_within_final_peak": lower_ok}
    return {"event": event, "passed": all(gates.values()), "gates": gates,
            "proof": {"owned_faces_sum": owned_faces, "surface_faces_by_rank": surfaces},
            "memory": {"job_peak_rss_bytes": job[0] if job else None,
                       "process_peak_sum_bytes": sum(process) if process else None,
                       "limit_bytes": MAX_JOB_BYTES, "exit_lower_bounds_bytes": lower},
            "diagnostics": {"final_proof_geometry_bound_calls": calls, "expected_calls": expected,
                            "call_count_matches": calls == expected if expected is not None else None,
                            "m18_rank_rows_with_fields": m18, "m20_private_work_rows": len(m20),
                            "m20_private_work_records": m20, "m21_ownership_memory_rows": len(m21),
                            "m21_ownership_memory_records": m21, "counter_presence_is_non_gating": True}}


def self_check():
    """Exercise two ordinal events in one rank log, timer selection, and all hard result gates."""
    with tempfile.TemporaryDirectory(prefix="p7p8-events-") as temporary:
        case = Path(temporary)
        base = case / "rank_output" / "1"
        (base / "rank.0").mkdir(parents=True)
        summaries = {event: [] for event in (1, 2)}
        local = {rank: {event: [] for event in (1, 2)} for rank in range(RANKS)}
        for event in (1, 2):
            for rank in range(RANKS):
                calls = 31622 // RANKS + (rank < 31622 % RANKS) if event == 1 else 9
                lower_bound = 550 if event == 1 else 575
                summaries[event].append(f"NATIVE3D rank={rank} complete=1 process_peak_rss=100 job_peak_rss=600 "
                                        "final_proof_faces=10 final_proof_surface_faces=60\n")
                local[rank][event].extend([
                    f"NATIVE3D_OWNERSHIP_MEMORY rank={rank} event={event} boundary=before current_rss=80\n",
                    f"NATIVE3D_OWNERSHIP_MEMORY rank={rank} event={event} boundary=staged current_rss=90\n",
                    f"NATIVE3D_OWNERSHIP_MEMORY rank={rank} event={event} boundary=committed current_rss=85\n",
                    f"NATIVE3D_PRIVATE_WORK rank={rank} event={event} private_work_used=2\n",
                    f"NATIVE3D_QUALITY rank={rank} round={event} busy_seconds=1 private_work_used=2\n",
                    f"NATIVE3D_STAGE rank={rank} stage=exit event={event} job_peak_rss_lower={lower_bound} distinct_facets=1 "
                    "distinct_scope_keys=1 prepared_facet_mean_bytes=100\n",
                    f"NATIVE3D_TIMER root=remesh boundary=exit rank={rank} phase=final_proof "
                    "operation=geometry_bound calls=500 inclusive_call_seconds=1 exclusive_seconds=1\n",
                    f"NATIVE3D_TIMER_SUMMARY root=remesh boundary=exit rank={rank} ranks=6 root_seconds=1 "
                    "sumexclusive=1 unattributed=0 coverage=1 fixedstorage=1\n",
                    f"NATIVE3D_TIMER root=remesh boundary=final rank={rank} phase=final_proof "
                    f"operation=geometry_bound calls={calls} inclusive_call_seconds=1 exclusive_seconds=1\n",
                    f"NATIVE3D_TIMER_SUMMARY root=remesh boundary=final rank={rank} ranks=6 root_seconds=1 "
                    "sumexclusive=1 unattributed=0 coverage=1 fixedstorage=1\n",
                ])
        # E1 has two engine exits; its final one belongs to the first remesh event.
        local[0][1].insert(5, "NATIVE3D_STAGE rank=0 stage=exit event=1 engine=normalization job_peak_rss_lower=1000\n")
        for rank in range(RANKS):
            folder = base / f"rank.{rank}"
            folder.mkdir(exist_ok=True)
            text = ("".join(summaries[1] + local[0][1] + summaries[2] + local[0][2]) if rank == 0 else
                    "".join(local[rank][1] + local[rank][2]))
            folder.joinpath("stdout").write_text(text)
        first, second = assess_event(case, 1), assess_event(case, 2)
        assert first and first["passed"] and second and second["passed"]
        assert first["diagnostics"]["final_proof_geometry_bound_calls"] == 31622
        assert not second["diagnostics"]["call_count_matches"] and second["passed"]
        assert first["memory"]["exit_lower_bounds_bytes"] == [550] * RANKS
        assert second["memory"]["exit_lower_bounds_bytes"] == [575] * RANKS
        assert all(row.get("event", row.get("round")) == "2" for row in second["diagnostics"]["m20_private_work_records"])
        assert all(row["event"] == "2" for row in second["diagnostics"]["m21_ownership_memory_records"])
        assert first["diagnostics"]["m21_ownership_memory_rows"] == 18

        # Extra launch roots are ambiguous; a partial second event remains live, not failed.
        root = base / "rank.0" / "stdout"
        full_root = root.read_text()
        root.write_text("".join(summaries[1] + local[0][1]))
        root.write_text(root.read_text() + "NATIVE3D rank=0 complete=1\n")
        assert assess_event(case, 1) and assess_event(case, 2) is None
        # Exact duplicate exit and final-timer records stay one event each.
        root.write_text(full_root)
        for rank in range(RANKS):
            path = base / f"rank.{rank}" / "stdout"
            log = path.read_text()
            exit_line = next(line for line in log.splitlines(keepends=True) if "NATIVE3D_STAGE" in line)
            timer_lines = [line for line in log.splitlines(keepends=True) if "boundary=final" in line]
            path.write_text(log + exit_line + "".join(timer_lines[-2:]))
        assert assess_event(case, 1) and assess_event(case, 2)
        (case / "rank_output" / "2").mkdir()
        assert assess_event(case, 1) is None
        (case / "rank_output" / "2").rmdir()

        root = base / "rank.0" / "stdout"
        original = root.read_text()
        root.write_text("".join(summaries[1] + local[0][1]))
        assert assess_event(case, 2) is None
        bad_coverage = [row.replace("final_proof_faces=10", "final_proof_faces=9", 1)
                        if "rank=0 " in row else row for row in summaries[1]]
        root.write_text("".join(bad_coverage + local[0][1]))
        result = assess_event(case, 1)
        assert result and not result["gates"]["proof_face_coverage"]
        root.write_text(original.replace("final_proof_faces=10", "final_proof_faces=0")
                        .replace("final_proof_surface_faces=60", "final_proof_surface_faces=0"))
        result = assess_event(case, 1)
        assert result and not result["gates"]["proof_face_coverage"]
        root.write_text(original.replace("job_peak_rss=600", "job_peak_rss=601", RANKS))
        result = assess_event(case, 1)
        assert result and not result["gates"]["job_peak_equals_process_peak_sum"]
        root.write_text(original.replace("job_peak_rss=600", "job_peak_rss=24000000000", RANKS)
                        .replace("process_peak_rss=100", "process_peak_rss=4000000000", RANKS))
        result = assess_event(case, 1)
        assert result and not result["gates"]["job_peak_within_19073_MiB"]


if __name__ == "__main__":
    self_check()
    if sys.argv[1:] != ["--self-check"]:
        result = assess_event(Path(sys.argv[1]), int(sys.argv[2]))
        print(json.dumps(result, indent=2))
        raise SystemExit(2 if result is None else not result["passed"])
