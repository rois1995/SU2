#!/usr/bin/env python3
"""Attach-only monitor and terminal validator for the existing N5 repeat."""
import hashlib
import importlib.util
import json
import os
import sys
import threading
import time
from pathlib import Path

N5 = Path(__file__).resolve().parents[1]
CASE = N5 / "full_workflow_R1_repeat"
PREFIX = N5.parent / "N4" / "run_prefix_controller.py"
TIMER = N5.parents[1] / "f3b_20261115" / "check_complete_events.py"
ROOT_ID = {"pid": 2977080, "start_ticks": 30072631, "pgid": 2977080}
ROOT_START = 1794808420.1578557
MAX_RSS = 19999490048
MAX_OUTER = 14500
MAX_EVENT = 5400


def dump(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def read_json(path):
    return json.loads(path.read_text())


def load_prefix():
    measurements = N5 / "run_measurements.py"
    spec = importlib.util.spec_from_file_location("n5_resume_measurements", measurements)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.load_prefix()


def process_rows():
    rows = {}
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            fields = (entry / "stat").read_text().rsplit(")", 1)[1].split()
            rows[int(entry.name)] = {"pid": int(entry.name), "state": fields[0],
                "ppid": int(fields[1]), "pgid": int(fields[2]), "sid": int(fields[3]),
                "start_ticks": int(fields[19]), "comm": (entry / "comm").read_text().strip()}
        except (OSError, ValueError, IndexError):
            continue
    return rows


def hwm_kb(pid):
    try:
        for line in (Path("/proc") / str(pid) / "status").read_text().splitlines():
            if line.startswith("VmHWM:"):
                return int(line.split()[1])
    except (OSError, ValueError, IndexError):
        return None
    return None


def restore_peaks():
    peaks = {}
    path = CASE / "host_resources.jsonl"
    with path.open() as stream:
        for line in stream:
            row = json.loads(line)
            for key, value in row.get("rank_process_peak_hwm_bytes", {}).items():
                peaks[key] = max(peaks.get(key, 0), int(value))
    return peaks


def restore_seen():
    seen = {(ROOT_ID["pid"], ROOT_ID["start_ticks"]): dict(ROOT_ID, comm="time")}
    with (CASE / "host_resources.jsonl").open() as stream:
        for line in stream:
            row = json.loads(line)
            for item in row.get("owned_processes", []):
                if "pid" in item and "start_ticks" in item:
                    seen[(int(item["pid"]), int(item["start_ticks"]))] = item
    return seen


def sample_workers(rows, peaks):
    sample = []
    for row in rows.values():
        if row["sid"] != ROOT_ID["pid"] or row["comm"] != "SU2_CFD":
            continue
        kb = hwm_kb(row["pid"])
        if kb is None:
            continue
        key = f"{row['pid']}:{row['start_ticks']}"
        peaks[key] = max(peaks.get(key, 0), kb * 1024)
        sample.append({**row, "VmHWM_kB": kb})
    return sample


def paced_sha(path, duty=0.20):
    digest = hashlib.sha256()
    cpu_start = time.process_time()
    wall_start = time.monotonic()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(4 << 20), b""):
            digest.update(block)
            cpu = time.process_time() - cpu_start
            wall = time.monotonic() - wall_start
            wait = cpu / duty - wall
            if wait > 0:
                time.sleep(min(wait, 1.0))
    return digest.hexdigest()


def verify_hash_map(expected):
    mismatches = []
    for raw_path, digest in expected.items():
        path = Path(raw_path)
        try:
            actual = paced_sha(path)
        except OSError:
            actual = None
        if actual != digest:
            mismatches.append({"path": raw_path, "expected": digest, "actual": actual})
    return mismatches


def verify_inputs_and_pins(ns, provenance):
    input_errors = verify_hash_map(provenance["input_hashes"])
    build_status_path = N5 / "build521" / "build_status.json"
    pin_path = N5 / "build521" / "source_pins.json"
    extra_path = N5.parent / "N4" / "unchanged_build499_source_pins.json"
    build_status = read_json(build_status_path)
    pins = read_json(pin_path)
    extra = read_json(extra_path)
    if isinstance(pins, dict) and "source_pins" in pins:
        pins = pins["source_pins"]
    overlap = sorted(set(pins) & set(extra))
    pins.update(extra)
    pin_errors = verify_hash_map(pins)
    binary = Path(build_status["binary"])
    binary_sha = paced_sha(binary)
    pin_errors.extend(verify_hash_map({str(binary): build_status.get("binary_sha256")}))
    manifest_ok = paced_sha(pin_path) == read_json(N5 / "build521" / "build_plan.json")["source_pins_sha256"]
    provenance_ok = (paced_sha(pin_path) == provenance.get("source_pins_sha256") and
        paced_sha(extra_path) == provenance.get("additional_source_pins_sha256") and
        paced_sha(PREFIX) == provenance.get("controller_sha256"))
    pin_state = {"binary": str(binary), "binary_sha256": binary_sha,
        "binary_matches_build_status": binary_sha == build_status.get("binary_sha256"),
        "source_pin_mismatches": pin_errors, "source_pin_overlap": overlap,
        "source_pin_manifest_matches": manifest_ok, "provenance_manifests_match": provenance_ok,
        "source_pins_sha256": paced_sha(pin_path),
        "additional_source_pins_sha256": paced_sha(extra_path)}
    return {"inputs_unchanged": not input_errors, "input_mismatches": input_errors,
        "source_and_binary_pins_unchanged": not overlap and manifest_ok and provenance_ok and
        not pin_state["source_pin_mismatches"] and pin_state["binary_matches_build_status"],
        "pin_state": pin_state}


def run_pin_check(result, ns, provenance):
    try:
        result.update(verify_inputs_and_pins(ns, provenance))
    except Exception as error:
        result["error"] = f"{type(error).__name__}: {error}"


def timer_module():
    assert TIMER.is_file(), f"timer validator missing: {TIMER}"
    spec = importlib.util.spec_from_file_location("n5_resume_timer", TIMER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_timer(ns, event):
    path = CASE / f"f3b_event{event}_timer_gate.json"
    if path.exists():
        return read_json(path)
    receipt = timer_module().assess_event(CASE, event)
    if receipt is not None:
        dump(path, receipt)
    return receipt


def check_first_collapse(ns):
    rows = ns["collapse_rows"](CASE)
    if set(rows) != set(range(6)):
        return None
    receipt = ns["assess_p6"](rows, True)
    receipt["work_equals_P6c"] = receipt["upper_work_sum"] == 90809869
    receipt["passed"] = bool(receipt["passed"] and receipt["count_prediction_matches"] and
                              receipt["work_equals_P6c"])
    return receipt


def check_refinements(ns):
    rows = ns["collapse_rows"](CASE, None, latest=True)
    failed = {rank: row for rank, row in rows.items() if int(row.get("upper_refinements", 0)) > 0}
    milestones = {}
    for rank, generation in ((5, 1), (2, 4)):
        row = ns["collapse_rows"](CASE, "split_surface_exit", generation).get(rank)
        if row:
            milestones[rank] = row
    missing = {rank: row for rank, row in milestones.items()
               if int(row.get("upper_deferred_bounds", 0)) < 1}
    if failed or len(milestones) != 2 or missing:
        return {"passed": False, "nonzero_refinement_rows": failed,
                "missing_completion_rows": missing, "deferred_milestones": milestones}
    return {"passed": True, "deferred_milestones": milestones}


def refinement_guard():
    records = []
    for path in sorted(CASE.glob("rank_output/*/rank.*/stderr")):
        with path.open(errors="replace") as stream:
            for number, line in enumerate(stream, 1):
                if "NATIVE_SURFACE_REFINE " in line:
                    records.append({"source_file": str(path), "source_line": number,
                                    "record": line.strip()})
    return {"passed": not records, "records": records}


def event_peak_gate(ns):
    summaries = ns["event_summaries"](CASE)
    rows = []
    for event in range(2):
        ranks = {rank: values[event] for rank, values in summaries.items() if len(values) > event}
        peaks = [int(row["process_peak_rss"]) for row in ranks.values()
                 if row.get("process_peak_rss") is not None]
        passed = set(ranks) == set(range(6)) and len(peaks) == 6 and sum(peaks) <= MAX_RSS
        rows.append({"event": event + 1, "rank_peaks": peaks, "rank_count": len(peaks),
                     "sum_bytes": sum(peaks), "limit_bytes": MAX_RSS, "passed": passed})
    return {"passed": all(row["passed"] for row in rows), "events": rows}


def payload_gate(ns):
    measurements = N5 / "run_measurements.py"
    spec = importlib.util.spec_from_file_location("n5_resume_payloads", measurements)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    expected1 = module.accepted_payload_hashes()
    expected = dict(expected1)
    actual = {}
    errors = []
    for filename, digest in expected.items():
        path = CASE / filename
        try:
            actual[filename] = paced_sha(path)
        except OSError:
            actual[filename] = None
        if actual[filename] != digest:
            errors.append({"file": filename, "expected": digest, "actual": actual[filename]})
    return {"passed": not errors and len(actual) == 6, "actual": actual,
            "expected_accepted_P6c_all_six": expected, "mismatches": errors}


def outer_exit_zero():
    return outer_exit_value((CASE / "outer.time.txt").read_text(errors="replace"))


def outer_exit_value(text):
    marker = "Exit status:"
    pos = text.rfind(marker)
    if pos < 0:
        return None
    try:
        return int(text[pos + len(marker):].splitlines()[0].strip()) == 0
    except (ValueError, IndexError):
        return False


def terminal_gate(ns, provenance, peaks, pin_state, timer_receipts, stop_reason, outer_ok):
    physical = ns["validate_full_workflow"](CASE)
    payloads = payload_gate(ns)
    first = check_first_collapse(ns)
    p6check3 = check_refinements(ns)
    guard = refinement_guard()
    event_peaks = event_peak_gate(ns)
    timer_ok = all(timer_receipts.get(event, {}).get("passed") is True for event in (1, 2))
    peak = sum(peaks.values())
    memory_ok = len(peaks) == 6 and peak <= MAX_RSS
    errors = []
    if not physical.get("workflow_accepted"):
        errors.extend(physical.get("errors", ["physical validator failed"]))
    if not payloads["passed"]:
        errors.append("six-file P6c payload identity failed")
    if not first or not first.get("passed"):
        errors.append("P6c first-collapse gate failed or incomplete")
    if not p6check3["passed"]:
        errors.append("P6c zero-refinement/deferred-completion gate failed")
    if not guard["passed"]:
        errors.append("P6c one-sided surface refinement guard fired")
    if not event_peaks["passed"]:
        errors.append("native event rank peak RSS sum gate failed")
    if not timer_ok:
        errors.append("F3b event timer gates incomplete or failed")
    if not outer_ok:
        errors.append("original GNU time wrapper did not exit zero")
    if stop_reason:
        errors.append(f"monitor observed gate/watchdog failure: {stop_reason}")
    if not memory_ok:
        errors.append("whole-job sticky six-rank VmHWM gate failed")
    if not pin_state["inputs_unchanged"] or not pin_state["source_and_binary_pins_unchanged"]:
        errors.append("input or source/binary provenance changed")
    accepted = not errors
    full = dict(physical)
    full["workflow_accepted"] = accepted
    full["errors"] = errors
    full["F3b_timer_events_checked"] = [event for event in (1, 2) if timer_receipts.get(event)]
    full["payload_identity"] = payloads
    full["resource_gate"] = {"rank_process_count": len(peaks),
        "rank_process_peak_hwm_bytes": peaks, "sum_rank_process_peak_hwm_bytes": peak,
        "limit_bytes": MAX_RSS, "passed": memory_ok}
    full["p6_check2"] = first
    full["p6_check3"] = p6check3
    full["p6c_refinement_guard"] = guard
    full["native_event_peak_RSS_gate"] = event_peaks
    dump(CASE / "full_workflow_receipt.json", full)
    return full


def finish(ns, provenance, peaks, pin_result, timer_receipts, fail_reason, state_path, seen):
    for event in (1, 2):
        if event not in timer_receipts:
            receipt = check_timer(ns, event)
            if receipt is not None:
                timer_receipts[event] = receipt
    if "pin_state" not in pin_result:
        pin_result.update({"inputs_unchanged": False, "source_and_binary_pins_unchanged": False,
                           "pin_state": {"error": pin_result.get("error", "provenance check missing")}})
    pin_state = pin_result
    if not pin_state["inputs_unchanged"] or not pin_state["source_and_binary_pins_unchanged"]:
        fail_reason = fail_reason or "provenance_changed"
    outer_ok = outer_exit_zero() is True
    rows = process_rows()
    live = [identity for identity in seen.values() if ns["identity_alive"](identity, rows)]
    terminal = outer_ok and not live
    final = terminal_gate(ns, provenance, peaks, pin_state, timer_receipts, fail_reason, outer_ok)
    if not terminal:
        final["workflow_accepted"] = False
        final["errors"].append("original owned process identities are not all terminal")
        dump(CASE / "full_workflow_receipt.json", final)
    payloads = final["payload_identity"]
    g5 = {"schema": "n5_g5_payload_identity_v1", "workflow_accepted": final["workflow_accepted"],
        "candidate_payload_sha256": payloads["actual"], "accepted_p6c_payload_sha256": payloads["expected_accepted_P6c_all_six"],
        "candidate_matches_accepted_p6c": payloads["passed"],
        "passed": bool(final["workflow_accepted"] and payloads["passed"])}
    dump(CASE / "g5_payload_identity_receipt.json", g5)
    original_status = read_json(CASE / "status.json")
    dump(CASE / "resume_original_running_status.json", original_status)
    captured = final.get("p6_check2") or {}
    result = {"state": "terminal", "exit_code": 0 if outer_ok else 1,
        "stop_reason": fail_reason or "normal_process_exit", "all_owned_handles_terminal": not live,
        "live_owned_handles": live, "owned_identities": list(seen.values()),
        "seconds_until_stop": time.time() - ROOT_START, "seconds_total": time.time() - ROOT_START,
        "inputs_unchanged": pin_state["inputs_unchanged"],
        "source_and_binary_pins_unchanged": pin_state["source_and_binary_pins_unchanged"],
        "changed_inputs": pin_state.get("input_mismatches", []),
        "source_pin_mismatches_after": pin_state["pin_state"].get("source_pin_mismatches", []),
        "native_case_started": True, "mode": "full_two_event_workflow", "gate": "full",
        "gate_receipt": None, "gate_passed": bool(final["workflow_accepted"] and terminal),
        "full_workflow_accepted": final["workflow_accepted"], "full_workflow_errors": final["errors"],
        "captured_ranks": sorted(int(row["rank"]) for row in captured.get("per_rank", [])),
        "original_outer_time_exit_zero": outer_ok,
        "whole_job_peak_RSS_bytes": sum(peaks.values()), "rank_process_peak_hwm_bytes": peaks,
        "F3b_timer_events_checked": sorted(timer_receipts), "first_collapse_gate": captured,
        "attached_without_launch": True}
    result.update({key: final.get(key) for key in ("p6_check2", "p6_check3", "native_event_peak_RSS_gate")})
    dump(state_path, result)
    dump(CASE / "resume_full_supervisor_receipt.json", result)
    dump(CASE / "status.json", result)


def main():
    ns = load_prefix()
    provenance = read_json(CASE / "provenance.json")
    status = read_json(CASE / "status.json")
    assert provenance["gate"] == "full" and provenance["application_watchdog_seconds"] == 14400
    assert provenance["native_time_limit_seconds_per_event"] == MAX_EVENT
    assert status["identity"]["pid"] == ROOT_ID["pid"]
    assert status["identity"]["start_ticks"] == ROOT_ID["start_ticks"]
    assert status["started_unix"] == ROOT_START
    peaks = restore_peaks()
    seen = restore_seen()
    pin_result = {}
    pin_thread = threading.Thread(target=run_pin_check, args=(pin_result, ns, provenance),
                                  name="paced-provenance-check", daemon=False)
    pin_thread.start()
    resource_path = CASE / "resume_supervisor_resources.jsonl"
    state_path = CASE / "resume_full_supervisor_status.json"
    timer_receipts = {}
    started = time.monotonic()
    last_diagnostic = 0.0
    fail_reason = None
    with resource_path.open("a", buffering=1) as log:
        while True:
            rows = process_rows()
            seen.update({(pid, row["start_ticks"]): row for pid, row in rows.items()
                         if row["sid"] == ROOT_ID["pid"]})
            workers = sample_workers(rows, peaks)
            total = sum(peaks.values())
            elapsed = time.time() - ROOT_START
            log.write(json.dumps({"unix": time.time(), "elapsed_from_original_start": elapsed,
                "workers": workers, "sticky_worker_peak_bytes": peaks,
                "sum_sticky_worker_peak_bytes": total, "limit_bytes": MAX_RSS}) + "\n")
            if total > MAX_RSS:
                fail_reason = "whole_job_peak_RSS_gate_failed"
            first = None
            if time.monotonic() - last_diagnostic >= 60:
                first = check_first_collapse(ns)
                if first is not None and not first["passed"]:
                    fail_reason = "final_source_first_collapse_gate_failed"
                if first and first["passed"] and not check_refinements(ns)["passed"]:
                    fail_reason = "p6c_check3_failed"
                for event in (1, 2):
                    if event not in timer_receipts:
                        receipt = check_timer(ns, event)
                        if receipt is not None:
                            timer_receipts[event] = receipt
                            if not receipt.get("passed"):
                                fail_reason = f"f3b_event{event}_timer_gate_failed"
                last_diagnostic = time.monotonic()
            alive = ns["identity_alive"](ROOT_ID, rows)
            session_live = [r for r in rows.values() if r["sid"] == ROOT_ID["pid"]
                            and r["state"] not in ("Z", "X")]
            outer = outer_exit_zero()
            record = {"state": "monitoring", "original_identity_alive": alive,
                "session_processes_live": len(session_live), "outer_time_exit_zero": outer,
                "elapsed_from_original_start": elapsed, "seconds_until_stop": time.monotonic() - started,
                "memory_bytes": total, "timer_events_checked": sorted(timer_receipts),
                "fail_reason": fail_reason, "attached_without_launch": True,
                "provenance_check_complete": bool(pin_result)}
            dump(state_path, record)
            if elapsed > MAX_OUTER:
                fail_reason = "outer_watchdog_14500_seconds_exceeded"
            if not alive and not session_live and outer is not None:
                break
            time.sleep(20)
    pin_thread.join()
    finish(ns, provenance, peaks, pin_result, timer_receipts, fail_reason, state_path, seen)


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--self-check":
        assert outer_exit_value("Exit status: 0\n") is True
        assert outer_exit_value("Exit status: 1\n") is False
        print(json.dumps({"passed": True, "launched": False}))
    else:
        main()
