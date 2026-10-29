"""Run the unchanged six-rank M6 Euler case in a fresh, pinned directory."""
import csv
import io
import json
import math
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DIAGNOSTIC = ROOT.parent / "diagnostic56"
sys.path.insert(0, str(DIAGNOSTIC))
from run import sha, save
from run_cases import sample_tree

REPO = ROOT.parents[3]
BASE = ROOT.parent / "validation54/m6_euler"
CFD = REPO / "integration_evidence/build-integrated-v2/SU2_CFD/src/SU2_CFD"
RUNTIME = BASE / "loaded_runtime_pins.json"
INPUTS = BASE / "input_pins.json"


def ldd_paths(binary):
    result = subprocess.run(["ldd", str(binary)], check=True, text=True, capture_output=True)
    paths = set()
    for line in result.stdout.splitlines():
        if "not found" in line:
            raise RuntimeError("unresolved CFD runtime library: " + line.strip())
        match = re.search(r"(?:=>\s+)?(/[^\s]+)", line)
        if match and Path(match.group(1)).is_file():
            paths.add(str(Path(match.group(1)).resolve()))
    if not paths:
        raise RuntimeError("ldd returned no CFD runtime library paths")
    return sorted(paths)


def assess_native_lines(lines, solver_exit_code, expected_events=1):
    events = []
    rows = {}
    malformed = False
    rejected = False
    pattern = re.compile(r"^NATIVE3D rank=(\d+) ranks=(\d+) .*?\bcomplete=(\d+)\b.*?\bfinal=(\d+)\b")
    for line in lines:
        if "Native3D rejected event:" in line:
            rejected = True
        match = pattern.search(line)
        if match:
            rank, ranks, complete, final = map(int, match.groups())
            if rank in rows or rank not in range(6) or ranks != 6:
                malformed = True
            rows[rank] = {"ranks": ranks, "complete": bool(complete), "final_cells": final}
            if len(rows) == 6:
                events.append(rows)
                rows = {}
    if malformed or rows or len(events) != expected_events:
        summary_complete = None
        state = "unknown: missing, duplicate, or unexpected Native3D event summaries"
    else:
        summary_complete = all(row["complete"] and row["final_cells"] > 0
                               for event in events for row in event.values()) and not rejected
        state = "accepted" if summary_complete else "rejected or incomplete"
    accepted = None if summary_complete is None else (solver_exit_code == 0 and summary_complete)
    if solver_exit_code != 0:
        state = "solver exit failure; native acceptance not claimed"
        accepted = False
    return {"native_event_accepted": accepted, "native_summary_complete": summary_complete,
            "solver_exit_code": solver_exit_code, "native_state": state,
            "expected_events": expected_events, "event_summaries": events,
            "rank_summaries": events[-1] if events else rows, "rejected_event_message": rejected,
            "transfer_reload_validation": "separate existing validation required"}


def native_assessment(log_paths, solver_exit_code, expected_events=1):
    # mpirun duplicates rank stdout into run.log; read exactly one copy.
    rank_zero = [path for path in log_paths if path.name == "stdout" and path.parent.name == "rank.0"]
    paths = rank_zero if rank_zero else log_paths[:1]
    lines = []
    for path in paths:
        if path.is_file():
            lines.extend(path.read_text(errors="replace").splitlines())
    return assess_native_lines(lines, solver_exit_code, expected_events)


def assess_workflow(lines, history, outputs, expected_events=2):
    """Assess each completed event in its own cycle; files are supplied as name -> hash."""
    text = "\n".join(lines)
    headers = list(re.finditer(r"(?m)^Cycle (\d+) of (\d+):[^\n]*$", text))
    sections = {int(m.group(1)): text[m.end():headers[i+1].start() if i+1 < len(headers) else len(text)]
                for i, m in enumerate(headers) if int(m.group(2)) == expected_events}
    header_text = {int(m.group(1)): m.group(0) for m in headers if int(m.group(2)) == expected_events}
    history_rows = {}
    try:
        for row in csv.DictReader(io.StringIO(history), skipinitialspace=True):
            row = {k.strip(): v.strip() for k, v in row.items() if k is not None and v is not None}
            cycle = int(row["Adap_Cycle"])
            if all(math.isfinite(float(row[name])) for name in ("Inner_Iter", "Adap_Iter", "rms[Rho]")):
                history_rows.setdefault(cycle, []).append(row)
    except (KeyError, ValueError, csv.Error):
        pass
    receipts = []
    for cycle in range(expected_events):
        section = sections.get(cycle, "")
        native = assess_native_lines(section.splitlines(), 0)
        markers = ["------------------- Solution Transfer (conservative P1) -------------------",
                   "Change of the integrals (sum of value x control volume)", "Time: setup ",
                   "The problem uses the new mesh:", "Mesh replaced in ",
                   f"Writing the adapted mesh of cycle {cycle+1}: adapted_mesh_adap_{cycle+1:05d}.su2."]
        # A native rank summary must precede transfer and publication in this same cycle.
        positions = [section.rfind("NATIVE3D rank=")] + [section.find(marker) for marker in markers]
        ordered = all(p >= 0 for p in positions) and positions == sorted(positions)
        conservation = ""
        if ordered:
            conservation = section[positions[2]:positions[3]]
        fields = ("Density", "Momentum-x", "Momentum-y", "Momentum-z", "Energy")
        parsed = {}
        for field in fields:
            match = re.search(r"(?m)^\s*" + re.escape(field) + r"\s*:\s*(\S+)", conservation)
            try:
                value = float(match.group(1)) if match else float("nan")
            except ValueError:
                value = float("nan")
            if math.isfinite(value): parsed[field] = value
        names = [f"adapted_mesh_adap_{cycle+1:05d}.su2", f"adapted_mesh_adap_{cycle+1:05d}.su2.native_ref"]
        next_cycle = cycle + 1
        advanced = ("transferred solution." in header_text.get(next_cycle, "") and bool(history_rows.get(next_cycle)))
        complete = (native["native_event_accepted"] is True and ordered and len(parsed) == len(fields)
                    and all(name in outputs for name in names) and advanced)
        receipts.append({"event": next_cycle, "complete": complete, "native": native,
                         "ordered_transfer_replacement_publication": ordered,
                         "conservation_report": conservation, "reported_integral_changes": parsed,
                         "output_sha256": {name: outputs[name] for name in names if name in outputs},
                         "next_cycle_solve_observed": advanced,
                         "next_cycle_history_rows": history_rows.get(next_cycle, [])})
    return receipts


def workflow_receipts(attempt, expected_events, reuse=False):
    log = attempt / "run.log"
    history = attempt / "history.csv"
    if not log.is_file() or not history.is_file(): return []
    lines = log.read_text(errors="replace").splitlines()
    log_text = "\n".join(lines)
    outputs = {}
    for event in range(1, expected_events + 1):
        if f"Cycle {event} of {expected_events}:" not in log_text: continue
        saved = attempt / f"workflow_event_{event:05d}.json"
        if reuse and saved.is_file():
            outputs.update(json.loads(saved.read_text())["output_sha256"])
            continue
        for suffix in ("", ".native_ref"):
            name = f"adapted_mesh_adap_{event:05d}.su2{suffix}"
            path = attempt / name
            if path.is_file() and path.stat().st_size: outputs[name] = sha(path)
    return assess_workflow(lines, history.read_text(errors="replace"), outputs, expected_events)


def save_completed_workflow(attempt, expected_events, reuse=False):
    receipts = workflow_receipts(attempt, expected_events, reuse)
    for receipt in receipts:
        path = attempt / f"workflow_event_{receipt['event']:05d}.json"
        if receipt["complete"] and not path.exists():
            receipt["provenance"] = "pins.json; verify input and runtime pins before reuse"
            save(path, receipt)
    return receipts


def acceptance_self_check():
    old = native_assessment([BASE / "run.log"], 0)
    assert old["native_event_accepted"] is False
    complete = [f"NATIVE3D rank={rank} ranks=6 workers=6 complete=1 detailed=1 initial=10 final=12" for rank in range(6)]
    assert assess_native_lines(complete, 0)["native_event_accepted"] is True
    assert assess_native_lines(complete, 1)["native_event_accepted"] is False
    assert assess_native_lines(complete[:-1], 0)["native_event_accepted"] is None
    assert assess_native_lines(complete + ["Native3D rejected event: synthetic rejection"], 0)["native_event_accepted"] is False
    assert assess_native_lines(complete * 2, 0, 2)["native_event_accepted"] is True
    assert assess_native_lines(complete, 0, 2)["native_event_accepted"] is None
    assert assess_native_lines(complete + complete[:-1], 0, 2)["native_event_accepted"] is None
    assert assess_native_lines(complete + complete[:1] + complete, 0, 2)["native_event_accepted"] is None
    assert assess_native_lines(complete * 2 + ["Native3D rejected event: synthetic rejection"], 0, 2)["native_event_accepted"] is False
    history = 'Adap_Cycle,Inner_Iter,Adap_Iter,rms[Rho]\n1,0,1,-2\n2,0,2,-3\n'
    outputs = {f"adapted_mesh_adap_{n:05d}.su2{suffix}": "hash" for n in (1, 2) for suffix in ("", ".native_ref")}
    def event(n):
        return [f"Cycle {n-1} of 2: transferred solution.", *complete,
                "------------------- Solution Transfer (conservative P1) -------------------",
                "Change of the integrals (sum of value x control volume)",
                *[f"  {name}: 0.0" for name in ("Density", "Momentum-x", "Momentum-y", "Momentum-z", "Energy")],
                "Time: setup 1 s", "The problem uses the new mesh:", "Mesh replaced in 1 s",
                f"Writing the adapted mesh of cycle {n}: adapted_mesh_adap_{n:05d}.su2."]
    lines = event(1) + event(2) + ["Cycle 2 of 2: transferred solution."]
    assert all(r["complete"] for r in assess_workflow(lines, history, outputs))
    for missing in ("Solution Transfer (conservative P1)", "Mesh replaced in ", "Cycle 1 of 2:"):
        assert not assess_workflow([line for line in lines if missing not in line], history, outputs)[0]["complete"]
    wrong_order = event(1); wrong_order[7], wrong_order[14] = wrong_order[14], wrong_order[7]
    assert not assess_workflow(wrong_order + event(2), history, outputs)[0]["complete"]
    assert not assess_workflow(lines, "", outputs)[0]["complete"]
    assert not assess_workflow(lines, history, {})[0]["complete"]


def main():
    if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != "2") or not re.fullmatch(r"build[1-9][0-9]*", sys.argv[1]) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,60}", sys.argv[2]):
        raise SystemExit("usage: run_m6.py buildN unique_attempt [2]")
    build, unique_attempt = sys.argv[1:3]
    expected_events = int(sys.argv[3]) if len(sys.argv) == 4 else 1
    native_budget = 5400 if expected_events == 2 else 1800
    outer_timeout = 14400 if expected_events == 2 else 2400
    acceptance_self_check()
    attempt = ROOT / "m6_runs" / unique_attempt
    attempt.mkdir(parents=True, exist_ok=False)
    status_path = attempt / "status.json"
    started = time.time()
    try:
        build_status_path = ROOT / f"{build}_status.json"
        source_pins_path = ROOT / f"{build}_source_pins.json"
        build_command_path = ROOT / f"{build}_command.json"
        build_status = json.loads(build_status_path.read_text())
        if build_status.get("state") != "terminal" or build_status.get("exit_code") != 0 or not build_status.get("pins_verified") or build_status.get("completed_build") is False:
            raise RuntimeError(f"{build} is not a verified successful terminal build")
        source_pins = json.loads(source_pins_path.read_text())
        changed = [path for path, digest in source_pins.items() if sha(path) != digest]
        if changed:
            raise RuntimeError("build source pins changed: " + ", ".join(changed[:8]))
        build_command = json.loads(build_command_path.read_text())
        targets_cfd = any(Path(argument).as_posix().endswith("SU2_CFD/src/SU2_CFD")
                          for argument in build_command.get("argv", []))
        if not targets_cfd:
            raise RuntimeError(f"{build} build command did not include the CFD executable target: {CFD}")

        inputs = json.loads(INPUTS.read_text())
        input_paths = {name if Path(name).is_absolute() else str(REPO / name): digest
                       for name, digest in inputs.items()}
        changed = [path for path, digest in input_paths.items() if sha(path) != digest]
        if changed:
            raise RuntimeError("saved M6 input pins changed: " + ", ".join(changed))
        runtime_pins = json.loads(RUNTIME.read_text())
        changed = [path for path, digest in runtime_pins.items() if sha(path) != digest]
        if changed:
            raise RuntimeError("saved M6 runtime pins changed: " + ", ".join(changed[:8]))

        config_source = BASE / "run.cfg"
        shutil.copyfile(config_source, attempt / "run.cfg")
        if expected_events == 2:
            config = (attempt / "run.cfg").read_text()
            config, count = re.subn(r"(?m)^ADAP_SUBITER=[ \t]*\([ \t]*1[ \t]*\)[ \t]*$", "ADAP_SUBITER= ( 2 )", config)
            if count != 1:
                raise RuntimeError("expected exactly one baseline ADAP_SUBITER= ( 1 )")
            config, count = re.subn(r"(?m)^ADAP_NATIVE_MAX_TIME=[ \t]*1800[ \t]*$", f"ADAP_NATIVE_MAX_TIME= {native_budget}", config)
            if count != 1:
                raise RuntimeError("expected exactly one baseline ADAP_NATIVE_MAX_TIME= 1800")
            (attempt / "run.cfg").write_text(config)
        for filename, target in (("m6.su2", REPO / "integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/Coarse.su2"),
                                 ("m6.su2.native_ref", REPO / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation22/m6_authority.json.native_ref")):
            if not target.is_file():
                raise RuntimeError(f"missing local M6 input: {target}")
            (attempt / filename).symlink_to(target)
        if expected_events == 1 and sha(attempt / "run.cfg") != sha(config_source):
            raise RuntimeError("fresh M6 config copy differs from the pinned config")

        libraries = ldd_paths(CFD)
        utilities = [Path(shutil.which(name) or "/nonexistent").resolve()
                     for name in ("nice", "timeout", "mpirun")]
        utilities.append(Path("/usr/bin/time"))
        if any(not path.is_file() for path in utilities):
            raise RuntimeError("could not resolve M6 execution utilities")
        helpers = [Path(__file__).resolve(), (DIAGNOSTIC / "run.py").resolve(),
                   (ROOT / "run_cases.py").resolve()]
        pins = dict(source_pins)
        pins.update(input_paths)
        pins.update(runtime_pins)
        for path in [CFD, *libraries, *utilities, *helpers, config_source, INPUTS, RUNTIME,
                     build_status_path, source_pins_path, build_command_path, attempt / "run.cfg",
                     attempt / "m6.su2", attempt / "m6.su2.native_ref"]:
            pins[str(path)] = sha(path)
        save(attempt / "pins.json", pins)

        env = os.environ.copy()
        env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1")
        argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "timeout", str(outer_timeout), "mpirun",
                "--output-filename", "rank_output", "-n", "6", str(CFD), "run.cfg"]
        save(attempt / "command.json", {
            "build": build, "case": "M6 Euler full adaptation", "ranks": 6, "expected_events": expected_events,
            "native_budget_seconds": native_budget, "outer_timeout_seconds": outer_timeout,
            "argv": argv,
            "environment": {key: value for key, value in env.items()
                            if key.startswith(("SU2_NATIVE_", "OMPI_")) or
                            key in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS")},
            "config_sha256": sha(attempt / "run.cfg"),
            "input_manifest": str(INPUTS), "runtime_manifest": str(RUNTIME),
        })
        largest_sampled_rss = 0
        with (attempt / "run.log").open("w") as log, (attempt / "resources.jsonl").open("w") as receipt:
            process = subprocess.Popen(argv, cwd=attempt, env=env, stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            save(status_path, {"state": "running", "pid": process.pid, "started_at": started,
                               "attempt": str(attempt)})
            print(json.dumps({"state": "running", "attempt": str(attempt), "pid": process.pid}), flush=True)
            while process.poll() is None:
                rows = sample_tree(process.pid)
                largest_sampled_rss = max(largest_sampled_rss,
                                          max((row["rss_bytes"] for row in rows), default=0))
                receipt.write(json.dumps({"elapsed_seconds": time.time() - started,
                                          "processes": rows,
                                          "sampled_process_tree_rss_bytes": sum(row["rss_bytes"] for row in rows)}) + "\n")
                receipt.flush()
                if expected_events == 2:
                    save_completed_workflow(attempt, expected_events, reuse=True)
                time.sleep(2)
            code = process.wait()

        changed = [path for path, digest in pins.items() if sha(path) != digest]
        rank_logs = [attempt / "run.log", *attempt.glob("rank_output/*/rank.*/stdout")]
        assessment = native_assessment(rank_logs, code, expected_events)
        save(attempt / "native_assessment.json", assessment)
        workflow = save_completed_workflow(attempt, expected_events) if expected_events == 2 else []
        workflow_accepted = (code == 0 and not changed and assessment["native_event_accepted"] is True
                             and len(workflow) == expected_events and all(row["complete"] for row in workflow)) if expected_events == 2 else None
        if expected_events == 2:
            save(attempt / "workflow_assessment.json", {"workflow_accepted": workflow_accepted, "events": workflow,
                 "restart_and_unsteady_history": "not validated"})
        result = {"state": "terminal", "solver_exit_code": code,
                  "native_event_accepted": assessment["native_event_accepted"],
                  "native_state": assessment["native_state"], "workflow_accepted": workflow_accepted,
                  "pins_verified": not changed, "changed": changed,
                  "seconds": time.time() - started,
                  "largest_sampled_individual_process_rss_bytes": largest_sampled_rss,
                  "rss_note": "sampled process RSS; not a whole-job peak",
                  "transfer_reload_validation": "separate existing validation required"}
        save(status_path, result)
        print(json.dumps(result))
        raise SystemExit(code or int(bool(changed)) or int(assessment["native_event_accepted"] is not True) or int(expected_events == 2 and not workflow_accepted))
    except SystemExit:
        raise
    except Exception as error:
        save(status_path, {"state": "terminal", "solver_exit_code": None, "native_event_accepted": None,
                           "seconds": time.time() - started, "pins_verified": False,
                           "error": str(error),
                           "transfer_reload_validation": "separate existing validation required"})
        raise


if __name__ == "__main__":
    main()
