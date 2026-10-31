"""Create and qualify one immutable steady Euler restart on the original M6 grid."""
import csv
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[3]
BASE = ROOT.parent / "validation54/m6_euler"
DIAGNOSTIC = ROOT.parent / "diagnostic56"
sys.path.insert(0, str(REPO / "TestCases/adaptation/capability"))
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(DIAGNOSTIC))
sys.path.insert(0, str(REPO / "integration_evidence"))
from run import sha, save
from run_cases import sample_tree
import capcheck

CFD = REPO / "integration_evidence/build-integrated-v2/SU2_CFD/src/SU2_CFD"
MESH = REPO / "integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/Coarse.su2"
AUTHORITY = REPO / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/validation22/m6_authority.json.native_ref"
INPUTS = BASE / "input_pins.json"
RUNTIME = BASE / "loaded_runtime_pins.json"
RESIDUALS = ("rms[Rho]", "rms[RhoU]", "rms[RhoV]", "rms[RhoW]", "rms[RhoE]")
MAX_ITER = 5000
TIMEOUT = 3600
CHECKPOINT_INTERVAL = 100


def set_directive(text, key, value):
    text, count = re.subn(r"(?m)^" + re.escape(key) + r"\s*=.*$", f"{key}= {value}", text)
    if count != 1:
        raise RuntimeError(f"expected one {key} directive, found {count}")
    return text


def set_or_append(text, key, value):
    if re.search(r"(?m)^" + re.escape(key) + r"\s*=", text):
        return set_directive(text, key, value)
    return text.rstrip() + f"\n{key}= {value}\n"


def seed_config(text, iterations=MAX_ITER, resume=False):
    for key, value in (("ADAP_LOOP", "NO"), ("COMPUTE_METRIC", "NO"),
                       ("CFL_ADAPT", "YES"), ("CFL_ADAPT_PARAM", "( 0.5, 1.25, 0.5, 20.0 )"),
                       ("MGLEVEL", "0"), ("SCREEN_WRT_FREQ_INNER", "1"),
                       ("OUTPUT_WRT_FREQ", f"( {CHECKPOINT_INTERVAL} )"),
                       ("RESTART_FILENAME", "baseline_euler_seed")):
        text = set_directive(text, key, value)
    text = set_or_append(text, "HISTORY_WRT_FREQ_INNER", "1")
    text = set_or_append(text, "HISTORY_OUTPUT", "( INNER_ITER, RMS_RES, AERO_COEFF )")
    text = set_or_append(text, "WRT_RESTART_OVERWRITE", "NO")
    text = set_directive(text, "ITER", str(iterations))
    if resume:
        text = set_directive(text, "RESTART_SOL", "YES")
        text = set_directive(text, "SOLUTION_FILENAME", "baseline_euler_resume_input")
        text = set_or_append(text, "READ_BINARY_RESTART", "YES")
        # The original adaptive-CFL continuation oscillated over five200-step windows.
        for key, value in (("CFL_NUMBER", "5.0"), ("CFL_ADAPT", "NO"),
                           ("LINEAR_SOLVER_ITER", "20"), ("LINEAR_SOLVER_ERROR", "1E-6")):
            text = set_directive(text, key, value)
        text = set_directive(text, "HISTORY_OUTPUT", "( INNER_ITER, RMS_RES, AERO_COEFF, CFL_NUMBER, LINSOL )")
    return text


def _history_rows(text):
    rows = []
    reader = csv.DictReader(text.splitlines(), skipinitialspace=True)
    for raw in reader:
        row = {(key or "").strip().strip('"'): (value or "").strip() for key, value in raw.items()}
        try:
            iteration = int(float(row["Inner_Iter"]))
            residual = [float(row[name]) for name in RESIDUALS]
            cl, cd = float(row["CL"]), float(row["CD"])
        except (KeyError, TypeError, ValueError):
            raise ValueError("history lacks per-iteration CL/CD or all five Euler residuals")
        if not all(math.isfinite(value) for value in (*residual, cl, cd)):
            raise ValueError("history contains nonfinite Euler residuals or coefficients")
        rows.append((iteration, residual, cl, cd))
    if len(rows) < 2:
        raise ValueError("history has fewer than two residual records")
    return rows


def assess_history(history_text):
    history = _history_rows(history_text)
    ordered = sorted(history, key=lambda row: row[0])
    initial_row = min(history, key=lambda row: row[0])
    final_row = max(history, key=lambda row: row[0])
    initial, final = initial_row[1], final_row[1]
    drops = [start - end for start, end in zip(initial, final)]
    recent = sorted(history, key=lambda row: row[0])[-200:]
    cl, cd = [row[2] for row in recent], [row[3] for row in recent]

    def stable(values, floor):
        mean = sum(values) / len(values)
        return (math.isfinite(mean) and max(values) - min(values) <= max(.001 * abs(mean), floor),
                max(values) - min(values), mean)

    cl_ok, cl_range, cl_mean = stable(cl, 1e-5) if len(cl) == 200 else (False, None, None)
    cd_ok, cd_range, cd_mean = stable(cd, 1e-6) if len(cd) == 200 else (False, None, None)
    consecutive_window = (len(recent) == 200 and recent[-1][0] == final_row[0] and
                          all(recent[i][0] + 1 == recent[i + 1][0] for i in range(199)))
    history_consecutive = (ordered[0][0] == 0 and
                           all(ordered[i][0] + 1 == ordered[i + 1][0] for i in range(len(ordered) - 1)))
    residual_criterion_met = final[0] <= -6.0 and all(drop >= 4.0 for drop in drops)
    # User accepts a reasonable stable-force seed; residual reduction remains evidence.
    qualified = (max(row[0] for row in history) <= MAX_ITER and cl_ok and cd_ok and
                 consecutive_window and history_consecutive)
    return {"qualified": qualified, "residual_criterion_met": residual_criterion_met,
            "qualification_policy": "user: reasonable stable CL/CD; finite admissible matching-grid state checked separately",
            "initial_residual_logs": dict(zip(RESIDUALS, initial)),
            "final_residual_logs": dict(zip(RESIDUALS, final)),
            "residual_drop_orders": dict(zip(RESIDUALS, drops)),
            "last_history_iteration": max(row[0] for row in history),
            "coefficient_rows": len(history), "coefficient_window_iterations": [r[0] for r in recent],
            "coefficient_window_consecutive": consecutive_window,
            "history_consecutive": history_consecutive,
            "CL": {"mean": cl_mean, "range": cl_range, "stable": cl_ok},
            "CD": {"mean": cd_mean, "range": cd_range, "stable": cd_ok}}


def merged_history(history_text):
    rows = _history_rows(history_text)
    lines = [",".join(("Inner_Iter", *RESIDUALS, "CL", "CD"))]
    lines.extend(",".join((str(iteration), *(format(value, ".17g") for value in (*residual, cl, cd))))
                 for iteration, residual, cl, cd in rows)
    return "\n".join(lines) + "\n"


def combine_history(prior_text, current_text, offset):
    # offset is the number of completed updates; both solver histories start at zero.
    prior = _history_rows(prior_text)
    current = _history_rows(current_text)
    if not prior or prior[-1][0] != offset - 1 or not current or current[0][0] != 0:
        raise ValueError("resume histories do not meet at the validated checkpoint iteration")
    rows = prior + [(iteration + offset, residual, cl, cd) for iteration, residual, cl, cd in current]
    if any(rows[i][0] + 1 != rows[i + 1][0] for i in range(len(rows) - 1)):
        raise ValueError("resumed history has an iteration gap or overlap")
    return merged_history_rows(rows)


def truncate_history(history_text, last_iteration):
    rows = [row for row in _history_rows(history_text) if row[0] <= last_iteration]
    if not rows or max(row[0] for row in rows) != last_iteration or \
            any(rows[i][0] + 1 != rows[i + 1][0] for i in range(len(rows) - 1)):
        raise ValueError("partial history does not reach the checkpoint on consecutive iterations")
    return merged_history_rows(rows)


def merged_history_rows(rows):
    lines = [",".join(("Inner_Iter", *RESIDUALS, "CL", "CD"))]
    lines.extend(",".join((str(iteration), *(format(value, ".17g") for value in (*residual, cl, cd))))
                 for iteration, residual, cl, cd in rows)
    return "\n".join(lines) + "\n"


def validate_state(attempt, restart_path=None):
    mesh = capcheck.read_su2(attempt / "m6.su2")
    points, fields, _ = capcheck.read_restart(restart_path or attempt / "baseline_euler_seed.dat")
    momentum_names = ("Momentum_x", "Momentum_y", "Momentum_z")
    if any(name not in fields for name in momentum_names + ("Density", "Energy")):
        raise ValueError("restart lacks Euler density, momentum, or energy fields")
    import numpy as np
    momentum = np.column_stack([fields[name] for name in momentum_names])
    if points.shape != mesh.P.shape or not np.isfinite(points).all() or \
            np.max(np.abs(points - mesh.P)) >= 1e-12 * max(mesh.size, 1e-300):
        raise ValueError("restart coordinates do not match original mesh ordering")
    rho, energy = fields["Density"], fields["Energy"]
    if not np.isfinite(rho).all() or np.min(rho) <= 0 or not np.isfinite(energy).all() or not np.isfinite(momentum).all():
        raise ValueError("restart has nonfinite conserved fields or nonpositive density")
    internal_energy = energy - np.sum(momentum * momentum, axis=1) / (2 * rho)
    if not np.isfinite(internal_energy).all() or np.min(internal_energy) <= 0:
        raise ValueError("restart has invalid density or nonpositive/nonfinite internal energy")
    values = np.column_stack([rho, momentum, energy])
    if values.shape != (len(mesh.P), 5) or not np.isfinite(values).all():
        raise ValueError("restart conserved state is incomplete or nonfinite")
    return {"points": len(points), "mesh_points": len(mesh.P), "min_density": float(np.min(rho)),
            "min_internal_energy": float(np.min(internal_energy))}


def latest_checkpoint(attempt, local_history):
    rows = {row[0] for row in _history_rows(local_history)}
    candidates = []
    for path in Path(attempt).glob("baseline_euler_seed_*.dat"):
        match = re.fullmatch(r"baseline_euler_seed_(\d{6})\.dat", path.name)
        if match and int(match.group(1)) in rows:
            candidates.append((int(match.group(1)), path))
    for iteration, path in sorted(candidates, reverse=True):
        try:
            state = validate_state(attempt, path)
            return {"iteration": iteration, "filename": path.name, "sha256": sha(path), "state": state}
        except (OSError, ValueError, struct.error):
            continue
    return None


def validate_seed(attempt):
    """Read-only check of a saved seed; historical generator pins need not match current code."""
    attempt = Path(attempt).resolve()
    receipt_path, pins_path = attempt / "qualification.json", attempt / "pins.json"
    receipt = json.loads(receipt_path.read_text())
    if receipt.get("qualified") is not True or sha(pins_path) != receipt.get("pins_manifest_sha256"):
        raise ValueError("seed is not qualified or its saved provenance manifest changed")
    artifacts = receipt.get("output_sha256", {})
    required = ("baseline_euler_seed.dat", "m6.su2", "m6.su2.native_ref", "run.cfg", "history.csv",
                "qualification_history.csv", "rank_output/1/rank.0/stdout")
    if any(name not in artifacts for name in required) or any(sha(attempt / name) != digest for name, digest in artifacts.items()):
        raise ValueError("a qualified seed artifact changed or is missing")
    state = validate_state(attempt)
    checkpoint_name = receipt.get("checkpoint_filename")
    if not checkpoint_name or checkpoint_name not in artifacts or sha(attempt / checkpoint_name) != artifacts[checkpoint_name]:
        raise ValueError("qualified seed lacks its validated numbered checkpoint")
    validate_state(attempt, attempt / checkpoint_name)
    if artifacts[checkpoint_name] != artifacts["baseline_euler_seed.dat"]:
        raise ValueError("canonical seed differs from its qualified numbered checkpoint")
    history = assess_history((attempt / "qualification_history.csv").read_text())
    if not history["qualified"]:
        raise ValueError("saved seed qualification history no longer passes its gates")
    return {"qualified": True, "output_sha256": artifacts, "pins_manifest_sha256": receipt["pins_manifest_sha256"],
            "state": state, "history": history}


def acceptance_self_check():
    original = (BASE / "run.cfg").read_text()
    configured = seed_config(original)
    for key in ("SOLVER", "MACH_NUMBER", "AOA", "CONV_NUM_METHOD_FLOW", "TIME_DISCRE_FLOW", "MARKER_EULER"):
        get = lambda text: re.search(r"(?m)^" + key + r"\s*=\s*(.*)$", text).group(1).strip()
        assert get(original) == get(configured)
    assert "ADAP_LOOP= NO" in configured and "COMPUTE_METRIC= NO" in configured
    assert "CFL_ADAPT= YES" in configured and "ITER= 5000" in configured
    assert "HISTORY_OUTPUT= ( INNER_ITER, RMS_RES, AERO_COEFF )" in configured
    lines = [",".join(("Inner_Iter", *RESIDUALS, "CL", "CD"))]
    for i in range(MAX_ITER + 1):
        level = -1.0 if i == 0 else -6.0
        cl, cd = .5 + (i % 2) * 1e-5, .02 + (i % 2) * 1e-6
        lines.append(f"{i},{level},{level},{level},{level},{level},{cl:.8f},{cd:.9f}")
    history = "\n".join(lines) + "\n"
    assert assess_history(history)["qualified"]
    stale = lines[:-1] + [lines[-1].replace(",0.50000000,", ",-0.5,")]
    assert not assess_history("\n".join(stale) + "\n")["qualified"]
    bad_residual = lines.copy()
    cells = bad_residual[-1].split(",");cells[1] = "-4";bad_residual[-1] = ",".join(cells)
    residual_only = assess_history("\n".join(bad_residual) + "\n")
    assert residual_only["qualified"] and not residual_only["residual_criterion_met"]
    continued = seed_config(original, 3000, True)
    assert "CFL_ADAPT= NO" in continued and "CFL_NUMBER= 5.0" in continued
    assert "LINEAR_SOLVER_ITER= 20" in continued and "READ_BINARY_RESTART= YES" in continued
    old = _history_rows(history)[:100]
    new = _history_rows(history)[:3]
    joined = _history_rows(combine_history(merged_history_rows(old), merged_history_rows(new), 100))
    assert [row[0] for row in joined] == list(range(103))
    assert joined[100][1:] == new[0][1:] and joined[0][1:] == old[0][1:]


def ldd_paths(binary):
    result = subprocess.run(["ldd", str(binary)], check=True, text=True, capture_output=True)
    paths = set()
    for line in result.stdout.splitlines():
        if "not found" in line:
            raise RuntimeError("unresolved CFD runtime library: " + line.strip())
        match = re.search(r"(?:=>\s+)?(/[^\s]+)", line)
        if match and Path(match.group(1)).is_file(): paths.add(str(Path(match.group(1)).resolve()))
    if not paths: raise RuntimeError("ldd returned no CFD runtime libraries")
    return sorted(paths)


def main():
    if len(sys.argv) not in (3, 4) or not re.fullmatch(r"build[1-9][0-9]*", sys.argv[1]) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,60}", sys.argv[2]):
        raise SystemExit("usage: run_m6_seed.py buildN unique_seed_attempt [previous_successful_unqualified_attempt]")
    build, name = sys.argv[1:3]
    acceptance_self_check()
    attempt = ROOT / "m6_seed_runs" / name
    attempt.mkdir(parents=True, exist_ok=False)
    started = time.time()
    status = attempt / "status.json"
    previous = Path(sys.argv[3]).resolve() if len(sys.argv) == 4 else None
    offset, prior_history = 0, None
    solver_exit_code = None
    try:
        build_status_path, source_pins_path = ROOT / f"{build}_status.json", ROOT / f"{build}_source_pins.json"
        build_command_path = ROOT / f"{build}_command.json"
        build_status = json.loads(build_status_path.read_text())
        if build_status.get("state") != "terminal" or build_status.get("exit_code") != 0 or not build_status.get("pins_verified"):
            raise RuntimeError(f"{build} is not a verified successful terminal build")
        source_pins = json.loads(source_pins_path.read_text())
        inputs = json.loads(INPUTS.read_text())
        input_pins = {str((REPO / p).resolve()) if not Path(p).is_absolute() else str(Path(p).resolve()): h for p, h in inputs.items()}
        runtime_pins = json.loads(RUNTIME.read_text())
        for label, mapping in (("build sources", source_pins), ("M6 inputs", input_pins), ("runtime", runtime_pins)):
            changed = [path for path, digest in mapping.items() if sha(path) != digest]
            if changed: raise RuntimeError(f"{label} changed: " + ", ".join(changed[:8]))
        build_command = json.loads(build_command_path.read_text())
        if not any(Path(arg).as_posix().endswith("SU2_CFD/src/SU2_CFD") for arg in build_command.get("argv", [])):
            raise RuntimeError(f"{build} did not build the CFD executable")
        (attempt / "m6.su2").symlink_to(MESH)
        (attempt / "m6.su2.native_ref").symlink_to(AUTHORITY)

        if previous:
            previous = previous.resolve()
            checkpoint_path = previous / "resume_checkpoint.json"
            previous_status = json.loads((previous / "status.json").read_text())
            checkpoint = json.loads(checkpoint_path.read_text())
            previous_receipt = json.loads((previous / "qualification.json").read_text())
            previous_pins = json.loads((previous / "pins.json").read_text())
            terminal_path = previous / "terminal_handles.json"
            terminal = json.loads(terminal_path.read_text()) if terminal_path.is_file() else {}
            stopped = previous / "convergence_stop_receipt.json"
            authorized_stop = (stopped.is_file() and terminal.get("all_terminal") is True and
                               terminal.get("pins_verified") is True)
            if previous_status.get("state") != "terminal" or \
                    (previous_status.get("solver_exit_code") not in (0, 124) and not authorized_stop) or \
                    previous_receipt.get("qualified") is not False:
                raise RuntimeError("resume source must be a terminal, unqualified solver attempt")
            identities = previous / "startup_verification.json"
            if identities.is_file():
                for handle in json.loads(identities.read_text())["owned_handles"]:
                    proc = Path("/proc") / str(handle["pid"])
                    if proc.exists():
                        fields = (proc / "stat").read_text().rsplit(")", 1)[1].split()
                        if int(fields[19]) == handle["startticks"] and fields[0] != "Z":
                            raise RuntimeError("resume source still has a live owned process")
            if sha(previous / "pins.json") != checkpoint["pins_manifest_sha256"] or \
                    sha(previous / checkpoint["checkpoint_filename"]) != checkpoint["checkpoint_sha256"] or \
                    sha(previous / "qualification_history.csv") != checkpoint["history_sha256"] or \
                    previous_receipt.get("pins_manifest_sha256", terminal.get("pins_manifest_sha256")) != checkpoint["pins_manifest_sha256"]:
                raise RuntimeError("resume checkpoint or its provenance changed")
            if any(previous_pins.get(path) != digest for path, digest in source_pins.items()):
                raise RuntimeError("resume source was produced by different C++ source pins")
            producer_hash = previous_pins.get(str(Path(__file__).resolve()))
            if producer_hash != sha(__file__):
                archived = previous / "source_snapshots/run_m6_seed.py"
                if not terminal.get("pins_verified") or not archived.is_file() or sha(archived) != producer_hash:
                    raise RuntimeError("historical checkpoint producer is not independently preserved")
            if previous_pins.get(str((BASE / "run.cfg").resolve())) != sha(BASE / "run.cfg"):
                raise RuntimeError("resume baseline physical configuration changed")
            offset = int(checkpoint["checkpoint_iteration"]) + 1
            if not 0 < offset < MAX_ITER or sha(CFD) != previous_pins.get(str(CFD.resolve())):
                raise RuntimeError("resume checkpoint is incompatible with this bounded original-grid run")
            for candidate in (MESH, AUTHORITY):
                if sha(candidate) != checkpoint["input_sha256"][candidate.name]:
                    raise RuntimeError("resume source grid or geometry authority changed")
            shutil.copyfile(previous / checkpoint["checkpoint_filename"], attempt / "baseline_euler_resume_input.dat")
            shutil.copyfile(previous / "qualification_history.csv", attempt / "prior_history.csv")
            prior_history = (attempt / "prior_history.csv").read_text()
            if max(row[0] for row in _history_rows(prior_history)) != offset - 1:
                raise RuntimeError("resume history does not end at its saved checkpoint")
            (attempt / "resume_source.json").write_text(json.dumps({"attempt": str(previous),
                "checkpoint_iteration": offset - 1, "completed_updates": offset, "checkpoint_filename": checkpoint["checkpoint_filename"],
                "checkpoint_sha256": checkpoint["checkpoint_sha256"],
                "history_sha256": checkpoint["history_sha256"], "pins_manifest_sha256": checkpoint["pins_manifest_sha256"]}, indent=2) + "\n")
            if validate_state(attempt, attempt / "baseline_euler_resume_input.dat")["points"] == 0:
                raise RuntimeError("resume checkpoint has no mesh points")

        shutil.copyfile(BASE / "run.cfg", attempt / "run.cfg")
        remaining = MAX_ITER - offset
        (attempt / "run.cfg").write_text(seed_config((attempt / "run.cfg").read_text(), remaining, bool(previous)))
        libraries = ldd_paths(CFD)
        utilities = [Path(shutil.which(n) or "/nonexistent").resolve() for n in ("nice", "timeout", "mpirun")]
        utilities.append(Path("/usr/bin/time"))
        helpers = [Path(__file__).resolve(), (DIAGNOSTIC / "run.py").resolve(), (ROOT / "run_cases.py").resolve(),
                   (REPO / "TestCases/adaptation/capability/capcheck.py").resolve()]
        pins = dict(source_pins); pins.update(input_pins); pins.update(runtime_pins)
        for path in [CFD, *libraries, *utilities, *helpers, BASE / "run.cfg", INPUTS, RUNTIME,
                     build_status_path, source_pins_path, build_command_path, attempt / "run.cfg",
                     attempt / "m6.su2", attempt / "m6.su2.native_ref"]:
            pins[str(Path(path).resolve())] = sha(path)
        if previous:
            for path in (attempt / "baseline_euler_resume_input.dat", attempt / "prior_history.csv", attempt / "resume_source.json",
                         previous / "pins.json", previous / "resume_checkpoint.json",
                         previous / checkpoint["checkpoint_filename"], previous / "qualification.json"):
                pins[str(Path(path).resolve())] = sha(path)
            for path in (previous / "terminal_handles.json", previous / "convergence_stop_receipt.json",
                         previous / "source_snapshots/run_m6_seed.py"):
                if not path.is_file():
                    continue
                pins[str(Path(path).resolve())] = sha(path)
        save(attempt / "pins.json", pins)
        argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "timeout", str(TIMEOUT), "mpirun",
                "--output-filename", "rank_output", "-n", "6", str(CFD), "run.cfg"]
        env = os.environ.copy(); env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1")
        save(attempt / "command.json", {"case": "original-grid M6 Euler seed", "build": build, "ranks": 6,
             "iterations_cap": remaining, "total_iterations_cap": MAX_ITER, "resume_offset": offset,
             "timeout_seconds": TIMEOUT, "argv": argv,
             "config_sha256": sha(attempt / "run.cfg"), "input_pins": str(INPUTS), "runtime_pins": str(RUNTIME)})
        memory = {line.split(":", 1)[0]: line.split(":", 1)[1].strip()
                  for line in Path("/proc/meminfo").read_text().splitlines()
                  if line.startswith(("MemTotal:", "MemAvailable:"))}
        save(attempt / "host_resources.json", {"load_average_1_5_15": os.getloadavg(), "memory_kb": memory,
             "cpu_count": os.cpu_count(), "recorded_before_launch": True})
        with (attempt / "resources.jsonl").open("w") as resources, (attempt / "run.log").open("w") as log:
            process = subprocess.Popen(argv, cwd=attempt, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            save(status, {"state": "running", "pid": process.pid, "started_at": started, "attempt": str(attempt)})
            print(json.dumps({"state": "running", "attempt": str(attempt), "pid": process.pid}), flush=True)
            largest = 0
            while process.poll() is None:
                tree = sample_tree(process.pid)
                largest = max(largest, max((p["rss_bytes"] for p in tree), default=0))
                resources.write(json.dumps({"elapsed_seconds": time.time() - started, "processes": tree,
                                           "sampled_tree_rss_bytes": sum(p["rss_bytes"] for p in tree)}) + "\n")
                resources.flush(); time.sleep(2)
            code = process.wait()
            solver_exit_code = code
        rank0 = attempt / "rank_output/1/rank.0/stdout"
        pin_changes = [path for path, digest in pins.items() if sha(path) != digest]
        if pin_changes: raise RuntimeError("generator provenance changed during solve: " + ", ".join(pin_changes[:8]))
        history_text = (attempt / "history.csv").read_text()
        local_history = merged_history(history_text)
        checkpoint = latest_checkpoint(attempt, local_history)
        combined_history = combine_history(prior_history, local_history, offset) if previous else local_history
        if code != 0:
            if code == 124 and checkpoint:
                checkpoint_history = truncate_history(local_history, checkpoint["iteration"])
                if previous:
                    checkpoint_history = combine_history(prior_history, checkpoint_history, offset)
                (attempt / "qualification_history.csv").write_text(checkpoint_history)
                save(attempt / "resume_checkpoint.json", {"checkpoint_iteration": offset + checkpoint["iteration"],
                     "checkpoint_local_iteration": checkpoint["iteration"], "checkpoint_filename": checkpoint["filename"],
                     "checkpoint_sha256": checkpoint["sha256"], "history_sha256": sha(attempt / "qualification_history.csv"),
                     "pins_manifest_sha256": sha(attempt / "pins.json"),
                     "input_sha256": {MESH.name: sha(MESH), AUTHORITY.name: sha(AUTHORITY)}, "state": checkpoint["state"]})
            raise RuntimeError(f"bounded CFD command exited {code}; raw history and numbered restart files preserved")
        history = assess_history(combined_history)
        (attempt / "qualification_history.csv").write_text(combined_history)
        if checkpoint is None or checkpoint["iteration"] != max(row[0] for row in _history_rows(local_history)):
            raise RuntimeError("final CFD history has no matching complete numbered restart")
        shutil.copyfile(attempt / checkpoint["filename"], attempt / "baseline_euler_seed.dat")
        if sha(attempt / "baseline_euler_seed.dat") != checkpoint["sha256"]:
            raise RuntimeError("canonical seed copy differs from its validated numbered checkpoint")
        state = validate_state(attempt)
        artifacts = ("baseline_euler_seed.dat", "m6.su2", "m6.su2.native_ref", "run.cfg", "history.csv",
                     "qualification_history.csv", "rank_output/1/rank.0/stdout")
        artifacts += (checkpoint["filename"],)
        if previous:
            artifacts += ("baseline_euler_resume_input.dat", "prior_history.csv", "resume_source.json")
        hashes = {name: sha(attempt / name) for name in artifacts}
        receipt = {"qualified": bool(history["qualified"]), "case": "original-grid M6 Euler seed",
                   "build": build, "history": history, "state": state, "output_sha256": hashes,
                   "checkpoint_filename": checkpoint["filename"],
                   "pins_manifest_sha256": sha(attempt / "pins.json"),
                   "history_ancestry": {"resume_offset": offset, "source_attempt": str(previous) if previous else None},
                   "generator_provenance": "pins.json records generator sources, executable, runtime, helper, config and input hashes",
                   "solver_exit_code": code, "seconds": time.time() - started,
                   "largest_sampled_individual_process_rss_bytes": largest,
                   "rss_note": "sampled individual-process RSS, not whole-job peak"}
        save(attempt / "qualification.json", receipt)
        save(status, {"state": "terminal", "solver_exit_code": code, "qualified": receipt["qualified"],
                      "pins_verified": not pin_changes, "seconds": time.time() - started,
                      "qualification": str(attempt / "qualification.json")})
        print(json.dumps(receipt))
        if not receipt["qualified"]:
            if checkpoint["iteration"] + offset < MAX_ITER:
                save(attempt / "resume_checkpoint.json", {"checkpoint_iteration": checkpoint["iteration"] + offset,
                     "checkpoint_local_iteration": checkpoint["iteration"], "checkpoint_filename": checkpoint["filename"],
                     "checkpoint_sha256": checkpoint["sha256"], "history_sha256": sha(attempt / "qualification_history.csv"),
                     "pins_manifest_sha256": sha(attempt / "pins.json"),
                     "input_sha256": {MESH.name: sha(MESH), AUTHORITY.name: sha(AUTHORITY)}, "state": state})
            raise SystemExit(1)
    except SystemExit:
        raise
    except Exception as error:
        failure = {"qualified": False, "error": str(error), "seconds": time.time() - started}
        if (attempt / "resume_checkpoint.json").is_file():
            failure["pins_manifest_sha256"] = sha(attempt / "pins.json")
            failure["checkpoint"] = json.loads((attempt / "resume_checkpoint.json").read_text())
        save(attempt / "qualification.json", failure)
        save(status, {"state": "terminal", "solver_exit_code": solver_exit_code,
                      "qualified": False, "error": str(error), "seconds": time.time() - started})
        raise


if __name__ == "__main__":
    main()
