"""Run the unchanged six-rank M6 Euler case in a fresh, pinned directory."""
import json
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


def assess_native_lines(lines, solver_exit_code):
    rows = {}
    rejected = False
    pattern = re.compile(r"^NATIVE3D rank=(\d+) ranks=(\d+) .*?\bcomplete=(\d+)\b.*?\bfinal=(\d+)\b")
    for line in lines:
        if "Native3D rejected event:" in line:
            rejected = True
        match = pattern.search(line)
        if match:
            rank, ranks, complete, final = map(int, match.groups())
            rows[rank] = {"ranks": ranks, "complete": bool(complete), "final_cells": final}
    if len(rows) != 6 or set(rows) != set(range(6)) or any(row["ranks"] != 6 for row in rows.values()):
        summary_complete = None
        state = "unknown: expected one final Native3D summary from each of six ranks"
    else:
        summary_complete = all(row["complete"] and row["final_cells"] > 0 for row in rows.values()) and not rejected
        state = "accepted" if summary_complete else "rejected or incomplete"
    accepted = None if summary_complete is None else (solver_exit_code == 0 and summary_complete)
    if solver_exit_code != 0:
        state = "solver exit failure; native acceptance not claimed"
    return {"native_event_accepted": accepted, "native_summary_complete": summary_complete,
            "solver_exit_code": solver_exit_code, "native_state": state,
            "rank_summaries": rows, "rejected_event_message": rejected,
            "transfer_reload_validation": "separate existing validation required"}


def native_assessment(log_paths, solver_exit_code):
    lines = []
    for path in log_paths:
        if path.is_file():
            lines.extend(path.read_text(errors="replace").splitlines())
    return assess_native_lines(lines, solver_exit_code)


def acceptance_self_check():
    old = native_assessment([BASE / "run.log"], 0)
    assert old["native_event_accepted"] is False
    complete = [f"NATIVE3D rank={rank} ranks=6 workers=6 complete=1 detailed=1 initial=10 final=12" for rank in range(6)]
    assert assess_native_lines(complete, 0)["native_event_accepted"] is True
    assert assess_native_lines(complete, 1)["native_event_accepted"] is False
    assert assess_native_lines(complete[:-1], 0)["native_event_accepted"] is None
    assert assess_native_lines(complete + ["Native3D rejected event: synthetic rejection"], 0)["native_event_accepted"] is False


def main():
    if len(sys.argv) != 3 or not re.fullmatch(r"build[1-9][0-9]*", sys.argv[1]) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,60}", sys.argv[2]):
        raise SystemExit("usage: run_m6.py buildN unique_attempt")
    build, unique_attempt = sys.argv[1:]
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
        if build_status.get("state") != "terminal" or build_status.get("exit_code") != 0 or not build_status.get("pins_verified"):
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
        for filename, target in (("m6.su2", REPO / "integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/Coarse.su2"),
                                 ("m6.su2.native_ref", REPO / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation22/m6_authority.json.native_ref")):
            if not target.is_file():
                raise RuntimeError(f"missing local M6 input: {target}")
            (attempt / filename).symlink_to(target)
        if sha(attempt / "run.cfg") != sha(config_source):
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
        argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "timeout", "2400", "mpirun",
                "--output-filename", "rank_output", "-n", "6", str(CFD), "run.cfg"]
        save(attempt / "command.json", {
            "build": build, "case": "M6 Euler full adaptation", "ranks": 6,
            "native_budget_seconds": 1800, "outer_timeout_seconds": 2400,
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
                time.sleep(2)
            code = process.wait()

        changed = [path for path, digest in pins.items() if sha(path) != digest]
        rank_logs = [attempt / "run.log", *attempt.glob("rank_output/*/rank.*/stdout")]
        assessment = native_assessment(rank_logs, code)
        save(attempt / "native_assessment.json", assessment)
        result = {"state": "terminal", "solver_exit_code": code,
                  "native_event_accepted": assessment["native_event_accepted"],
                  "native_state": assessment["native_state"],
                  "pins_verified": not changed, "changed": changed,
                  "seconds": time.time() - started,
                  "largest_sampled_individual_process_rss_bytes": largest_sampled_rss,
                  "rss_note": "sampled process RSS; not a whole-job peak",
                  "transfer_reload_validation": "separate existing validation required"}
        save(status_path, result)
        print(json.dumps(result))
        raise SystemExit(code or int(bool(changed)) or int(assessment["native_event_accepted"] is not True))
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
