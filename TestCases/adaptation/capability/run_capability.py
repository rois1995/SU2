#!/usr/bin/env python3
"""Adaptation capability regressions: small runs of the in-SU2 adaptation loop judged by mesh and metric gates.

  python3 run_capability.py --binary BUILD/SU2_CFD/src/SU2_CFD --output OUT [--ranks N] [--cases NAME|TAG ...]

Tags: all (default), 2d, 3d, steady, unsteady, fixed, free, bl, custom, mpi4 (the subset run with 4 ranks).
Every case runs in a fresh OUT/<case>_np<N>; OUT/results.json (all runs of OUT) and OUT/summary.txt are updated.
Runs with N > 1 compare with the np 1 run of the same case in OUT (run np 1 first). Exit code 1 if a gate fails
(a known failure, marked in CASES, is reported as XFAIL and does not fail the run). README.txt explains the gates.
"""

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time

import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
sys.path.insert(0, str(HERE))

import capcheck  # noqa: E402
import meshgen  # noqa: E402

LOAD_GATE = 7.0
NEEDS_333 = "needs the MMG #333 fix (MMG 5.8 -nosurf -nosizreq hangs in 3D without it)"

VORTEX_TRACKING = {"TIME_STEP": "0.17", "TIME_ITER": "9", "ADAP_FREQ": "3"}

# Each case: base config + changes (None removes an option). fixed: ADAP_SURFACE= NO; bl: {marker: h0};
# reference: compare with the numpy metric; opts: threshold overrides; known_fail: reason of an expected failure.
CASES = [
    dict(name="naca_free", tags={"2d", "steady", "free"}, base="naca_euler.cfg", mesh="naca",
         changes={"ADAP_SURFACE": "YES"},
         known_fail="free airfoil boundary deviates ~5e-3 (0.5 % chord) from the input near the leading edge, "
                    "independent of ADAP_HAUSD (probed 4e-4 .. 1e-2): MMG2D hausd not effective there"),
    dict(name="naca_fixed", tags={"2d", "steady", "fixed"}, base="naca_euler.cfg", mesh="naca", fixed=True,
         reference=True,
         changes={"ADAP_SURFACE": "NO", "ADAP_SIZES": "(3000)", "ADAP_SUBITER": "(1)", "ADAP_FLOW_ITER": "(60)"}),
    dict(name="plate_metric", tags={"2d", "steady", "free", "bl"}, base="plate_ns.cfg", mesh="plate",
         changes={"ADAP_SURFACE": "YES", "ADAP_BL_METHOD": "METRIC"}, bl={"wall": 5e-5}),
    dict(name="plate_metric_fixed", tags={"2d", "steady", "fixed", "bl"}, base="plate_ns.cfg", mesh="plate",
         changes={"ADAP_SURFACE": "NO", "ADAP_BL_METHOD": "METRIC"}, bl={"wall": 5e-5}, fixed=True),
    dict(name="bump2d_twopass", tags={"2d", "steady", "free", "bl"}, base="bump2d_ns.cfg", mesh="bump2d",
         changes={"ADAP_SURFACE": "YES", "ADAP_BL_METHOD": "TWO_PASS"}, bl={"lower": 5e-5}, twopass=True,
         analytic="bump2d", opts={"points_per_complexity": (0.7, 3.0)}),
    dict(name="vortex_wa", tags={"2d", "unsteady", "free", "custom", "mpi4"}, base="vortex.cfg", mesh="square",
         changes={**VORTEX_TRACKING, "ADAP_SURFACE": "YES", "ADAP_UNSTEADY_METRIC": "WINDOW_AVERAGE"},
         restart=True),
    dict(name="vortex_predict", tags={"2d", "unsteady", "fixed", "custom", "mpi4"}, base="vortex.cfg",
         mesh="square", fixed=True, restart=True,
         changes={**VORTEX_TRACKING, "ADAP_SURFACE": "NO", "ADAP_UNSTEADY_METRIC": "PREDICT",
                  "ADAP_CUSTOM_SENSORS": "'U2 : VELOCITY_X*VELOCITY_X+VELOCITY_Y*VELOCITY_Y; S : sqrt(U2)'"}),
    dict(name="vortex_fp", tags={"2d", "unsteady", "free"}, base="vortex.cfg", mesh="square", restart=True,
         changes={**VORTEX_TRACKING, "ADAP_SURFACE": "YES", "ADAP_UNSTEADY_METRIC": "FIXED_POINT", "ADAP_FP_ITER": "1",
                  "ADAP_SENSOR": "(PRESSURE)", "ADAP_CUSTOM_SENSORS": None}),
    dict(name="bump3d_free", tags={"3d", "steady", "free", "mpi4"}, base="bump3d_euler.cfg", mesh="bump3d",
         changes={"ADAP_SURFACE": "YES"}, analytic="bump3d", reference=True),
    dict(name="bump3d_fixed", tags={"3d", "steady", "fixed"}, base="bump3d_euler.cfg", mesh="bump3d",
         changes={"ADAP_SURFACE": "NO"}, fixed=True, needs=NEEDS_333, reference=True),
    dict(name="bump3d_bl", tags={"3d", "steady", "free", "bl"}, base="bump3d_euler.cfg", mesh="bump3d",
         changes={"ADAP_SURFACE": "YES", "ADAP_SIZES": "(2000)", "ADAP_BL_MARKER": "( lower )",
                  "ADAP_BL_FIRST_HEIGHT": "( 5e-3 )", "ADAP_BL_GROWTH": "( 1.3 )", "ADAP_BL_THICKNESS": "( 0.03 )"},
         analytic="bump3d", bl={"lower": 5e-3},
         known_fail="MMG3D does not follow the 3D boundary-layer metric: metric edges in [0.71, 1.41] ~50 %, metric "
                    "quality 1st percentile ~0.04, points ~4x the metric complexity (BL cell heights are fine)"),
    dict(name="bump3d_fixed_bl", tags={"3d", "steady", "fixed", "bl"}, base="bump3d_euler.cfg", mesh="bump3d",
         fixed=True, needs=NEEDS_333 + "; the BL ridge needs the refined #333 patch",
         changes={"ADAP_SURFACE": "NO", "ADAP_SIZES": "(2000)", "ADAP_BL_MARKER": "( lower, side0 )",
                  "ADAP_BL_FIRST_HEIGHT": "( 5e-3 )", "ADAP_BL_GROWTH": "( 1.3 )", "ADAP_BL_THICKNESS": "( 0.03 )"},
         bl={"lower": 5e-3, "side0": 5e-3}),
    dict(name="bump3d_wa", tags={"3d", "unsteady", "fixed", "custom"}, base="bump3d_euler.cfg", mesh="bump3d",
         fixed=True, restart=True, needs=NEEDS_333,
         changes={"ADAP_SURFACE": "NO", "TIME_DOMAIN": "YES", "TIME_MARCHING": "DUAL_TIME_STEPPING-2ND_ORDER",
                  "TIME_STEP": "1e-4", "TIME_ITER": "6", "INNER_ITER": "4", "ITER": None, "ADAP_FREQ": "2",
                  "ADAP_SUBITER": None, "ADAP_FLOW_ITER": None, "ADAP_UNSTEADY_METRIC": "WINDOW_AVERAGE",
                  "OUTPUT_WRT_FREQ": "(1, 1)", "SCREEN_OUTPUT": "(TIME_ITER, INNER_ITER, RMS_DENSITY)",
                  "ADAP_ARMAX": "100", "ADAP_SENSOR": "(S)", "ADAP_CUSTOM_SENSORS": "'S : MACH*MACH'",
                  "WRT_RESTART_COMPACT": "YES",
                  "VOLUME_OUTPUT": "(COORDINATES, SOLUTION, PRIMITIVE, HESSIAN, METRIC)"}),
]


def make_mesh(kind, path):
    if kind == "naca":
        shutil.copyfile(REPO / "QuickStart/mesh_NACA0012_inv.su2", path)
    elif kind == "square":
        meshgen.rectangle(path, nx=40, ny=40, x0=-2.0, x1=2.0, y0=-2.0, y1=2.0)
    else:
        getattr(meshgen, kind)(path)


# ------------------------------------------------------------------------------------------------ configs and logs

def config_text(base, changes):
    """Case config + common settings (the case config wins), with the per-case changes (None removes an option)."""
    lines, seen = [], set()
    changes = dict(changes)
    for path in (HERE / base, HERE / "common.cfg"):
        for line in path.read_text().splitlines():
            key = line.split("=", 1)[0].strip()
            if "=" not in line or not key or key.startswith("%") or key in seen:
                continue
            seen.add(key)
            if key in changes:
                value = changes.pop(key)
                if value is not None:
                    lines.append(f"{key}= {value}")
            else:
                lines.append(line)
    lines += [f"{key}= {value}" for key, value in changes.items() if value is not None]
    return "\n".join(lines) + "\n"


def options(text):
    out = {}
    for line in text.splitlines():
        if "=" in line and not line.lstrip().startswith("%"):
            key, value = line.split("=", 1)
            out[key.strip()] = value.strip()
    return out


def as_list(value):
    return [v.strip() for v in value.strip("() ").split(",") if v.strip()]


def wait_load():
    while os.getloadavg()[0] > LOAD_GATE:
        print(f"  load {os.getloadavg()[0]:.2f} > {LOAD_GATE}; waiting", flush=True)
        time.sleep(30)


def run_su2(binary, directory, ranks, timeout):
    """Run SU2 in directory (run.cfg -> run.log); a timeout terminates the whole process group."""
    wait_load()
    command = [str(binary), "run.cfg"]
    if ranks > 1:
        command = ["mpirun", "--oversubscribe", "--bind-to", "none", "-n", str(ranks)] + command
    env = {**os.environ, "OMP_NUM_THREADS": "1", "OPENBLAS_NUM_THREADS": "1"}
    start = time.monotonic()
    with (directory / "run.log").open("w") as log:
        proc = subprocess.Popen(command, cwd=directory, stdout=log, stderr=subprocess.STDOUT, env=env,
                                start_new_session=True)
        try:
            code = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGTERM)
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.wait()
            code = "timeout"
    return code, time.monotonic() - start, (directory / "run.log").read_text(errors="replace")


def numbered(directory, pattern):
    """{number: path} of files like mesh_out_00002.su2 / flow_adap_00001.vtu (trailing number)."""
    out = {}
    for path in directory.glob(pattern):
        m = re.search(r"_(\d+)\.[a-z0-9]+$", path.name)
        if m:
            out[int(m[1])] = path
    return dict(sorted(out.items()))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()[:16]


def log_gates(case, cfg, log, code, n_meshes, expected):
    """G1 from the log: exit, MMG status, warnings, counts of meshes and of the mode's own steps."""
    statuses = re.findall(r"MMG[23]D status (SUCCESS|LOWFAILURE)", log)
    twopass = re.findall(r"TWO_PASS: pass A (accepted|rejected)", log)
    warnings = len(re.findall(r"MMG could not fully adapt|MMG made the metric coarser", log))
    calls = len(twopass) if case.get("twopass") else len(statuses)
    ok = (code == 0 and "Exit Success" in log and warnings == 0 and n_meshes == expected and calls >= expected
          and "LOWFAILURE" not in statuses and "rejected" not in twopass)
    value = (f"exit {code}, meshes {n_meshes} of {expected}, " +
             (f"TWO_PASS pass A {twopass}" if case.get("twopass") else
              f"MMG {statuses.count('SUCCESS')} SUCCESS / {len(statuses)}") + f", MMG warnings {warnings}")
    if code == "timeout" and case.get("needs"):
        value += f" ({case['needs']})"
    gates = {"run": capcheck.gate(ok, value)}
    counters, ok = [], True
    if not case.get("bl"):
        pairs = re.findall(r"Mesh complexity: (\S+) \(ADAP_COMPLEXITY= (\d+)\)", log)
        bad = [p for p in pairs if abs(float(p[0]) / float(p[1]) - 1.0) > 1e-4]
        ok &= bool(pairs) and not bad
        counters.append(f"printed complexity == target {len(pairs) - len(bad)}/{len(pairs)}")
    mode = cfg.get("ADAP_UNSTEADY_METRIC") if cfg.get("TIME_DOMAIN") == "YES" else None
    freq = int(cfg.get("ADAP_FREQ", "0") or 0)
    if mode in ("WINDOW_AVERAGE", "FIXED_POINT"):
        samples = [int(n) for n in re.findall(r"mean \|Hessian\| of the sensors over (\d+) time steps", log)]
        ok &= bool(samples) and all(n == freq for n in samples)
        counters.append(f"window samples {samples} (== {freq})")
    if mode == "PREDICT":
        speeds = [float(v) for v in re.findall(r"speed of the features (\S+) per time step", log)]
        none = len(re.findall(r"No motion found|no motion \(the metric", log))
        ok &= bool(speeds) and all(v > 0.0 for v in speeds) and none == 0
        counters.append(f"feature speeds {speeds}, windows without motion {none}")
    if mode == "FIXED_POINT":
        solves = [int(n) for n in re.findall(r"Fixed point of time steps \d+ to \d+: (\d+) solves", log)]
        want = int(cfg.get("ADAP_FP_ITER", "2")) + 1
        ok &= bool(solves) and all(n == want for n in solves)
        counters.append(f"solves per window {solves} (== {want})")
    gates["log_counters"] = capcheck.gate(ok, ", ".join(counters))
    return gates


def remesh_pairs(cfg, directory):
    """[(donor mesh, metric file or None, output mesh, target, window steps or None)] and the expected count."""
    meshes = numbered(directory, "mesh_out*.su2")
    sizes = [int(s) for s in as_list(cfg["ADAP_SIZES"])]
    pairs = []
    if cfg.get("TIME_DOMAIN", "NO") != "YES":
        restarts = numbered(directory, "restart_flow*.dat")
        subiter = [int(s) for s in as_list(cfg.get("ADAP_SUBITER", "(1)"))]
        targets = [size for size, n in zip(sizes, subiter) for _ in range(n)]
        previous = directory / "mesh.su2"
        for k, (number, path) in enumerate(meshes.items()):
            pairs.append((previous, restarts.get(number - 1), path, targets[min(k, len(targets) - 1)], None))
            previous = path
        return pairs, len(targets)
    freq, n_time = int(cfg["ADAP_FREQ"]), int(cfg["TIME_ITER"])
    vtus = numbered(directory, "flow*.vtu")
    if cfg.get("ADAP_UNSTEADY_METRIC") == "FIXED_POINT":
        # the accepted mesh of a window comes from a discarded solve (no files); its window is [first, last]
        previous = directory / "mesh.su2"
        for number, path in meshes.items():
            pairs.append((previous, None, path, sizes[0], (number, min(number + freq, n_time) - 1, path)))
            previous = path
        return pairs, (n_time + freq - 1) // freq
    previous, start = directory / "mesh.su2", 0
    for number, path in meshes.items():
        pairs.append((previous, vtus.get(number - 1), path, sizes[0], (start, number - 1, previous)))
        previous, start = path, number
    expected = len([n for n in range(n_time) if (n + 1) % freq == 0 and n + 1 < n_time])
    return pairs, expected


def merge(total, res, tag):
    """Keep the worst status of each gate over the remeshes of a case (FAIL > PASS > REPORT > NA)."""
    rank = {"FAIL": 3, "PASS": 2, "REPORT": 1, "NA": 0}
    for gate_name, (status, value) in res.items():
        old = total.get(gate_name)
        if old is None or rank[status] > rank[old[0]]:
            total[gate_name] = [status, f"{tag}: {value}"]


def window_gates(case, cfg, directory, window, speeds, opts):
    """Unsteady checks on the VTUs of one window (all on the window's mesh): WINDOW_AVERAGE / FIXED_POINT: the
    window-end Hessian is the mean |H| of the window and the written metric is the numpy metric of it; PREDICT: the
    written metric is refined ahead of the instantaneous metric of the window end."""
    first, last, mesh_path = window
    vtus = numbered(directory, "flow*.vtu")
    if any(k not in vtus for k in range(first, last + 1)):
        return {"window_files": ["FAIL", f"missing VTUs of steps {first}..{last}"]}
    mesh = capcheck.read_su2(mesh_path)
    steps = []
    for k in range(first, last + 1):
        points, fields, _ = capcheck.read_vtu(vtus[k])
        if len(points) != len(mesh.P) or np.abs(points[:, :mesh.dim] - mesh.P).max() > 1e-6 * mesh.size:
            return {"window_files": ["FAIL", f"{vtus[k].name} is not on {mesh_path.name}"]}
        steps.append(fields)
    M, _ = capcheck.metric_of(mesh, vtus[last])
    sensors = as_list(cfg["ADAP_SENSOR"])
    p, target = float(cfg["ADAP_NORM"]), int(as_list(cfg["ADAP_SIZES"])[0])
    hmin, hmax, armax = (float(cfg[k]) for k in ("ADAP_HMIN", "ADAP_HMAX", "ADAP_ARMAX"))
    if cfg["ADAP_UNSTEADY_METRIC"] == "PREDICT":
        if not speeds:
            return {"predict_lookahead": ["FAIL", "no feature speed in the log"]}
        horizon = int(cfg.get("ADAP_PREDICT_HORIZON", "0") or 0) or int(cfg["ADAP_FREQ"])
        aoa = np.radians(float(cfg.get("AOA", "0")))
        return capcheck.check_predict_lookahead(mesh, steps[-1], M, sensors, p, target, hmin, hmax, armax,
                                                speeds.pop(0), horizon, np.array([np.cos(aoa), np.sin(aoa)]))
    res = capcheck.check_window_identity(steps, sensors, mesh.dim)
    res.update(capcheck.check_reference_metric(mesh, steps[-1], M, sensors, p, target, hmin, hmax, armax, tol=1e-4))
    return res


def check_case(case, cfg, directory, log, code):  # noqa: C901
    opts = copy.deepcopy(capcheck.DEFAULTS)
    opts.update(case.get("opts", {}))
    pairs, expected = remesh_pairs(cfg, directory)
    gates = log_gates(case, cfg, log, code, len(pairs), expected)
    if not pairs:
        return gates, None, None
    original = capcheck.read_su2(directory / "mesh.su2")
    analytic = capcheck.analytic_references(case["analytic"]) if case.get("analytic") else None
    hausd, angle = float(cfg.get("ADAP_HAUSD", "0.01")), float(cfg.get("ADAP_ANGLE", "45"))
    hmin, hmax, armax = (float(cfg[k]) for k in ("ADAP_HMIN", "ADAP_HMAX", "ADAP_ARMAX"))
    bl = case.get("bl")
    last, first_metric = None, None
    speeds = [float(v) for v in re.findall(r"speed of the features (\S+) per time step", log)]
    for src_path, metric_path, out_path, target, window in pairs:
        src, out = capcheck.read_su2(src_path), capcheck.read_su2(out_path)
        res = {}
        res.update(capcheck.check_validity(out))
        res.update(capcheck.check_markers(original, out))
        M, precision = None, None
        if metric_path is not None:
            try:
                M, precision = capcheck.metric_of(src, metric_path)
            except ValueError as error:
                res["metric_file"] = ["FAIL", str(error)]
        if M is not None:
            if first_metric is None:
                first_metric = M
            res.update(capcheck.check_metric_field(M, hmin, hmax, armax, bl=bl is not None, opts=opts))
        cplx = capcheck.check_complexity(src, out, target, M, precision or "Float64", bl=bl is not None,
                                         opts=opts)
        res.update(cplx)
        if M is not None:
            report = case.get("twopass", False) or case.get("fixed", False)
            res.update(capcheck.check_edges(src, out, M, opts, report_only=report))
        else:
            res["metric_edges"] = ["NA", "no file with the metric of this remesh (FIXED_POINT: discarded solve)"]
        if case.get("reference") and M is not None:
            _, fields, _ = capcheck.read_restart(metric_path)
            res.update(capcheck.check_reference_metric(src, fields, M, as_list(cfg["ADAP_SENSOR"]),
                                                       float(cfg["ADAP_NORM"]), target, hmin, hmax, armax))
        res.update(capcheck.check_geometry(original, out, case.get("fixed", False), hausd, analytic, opts))
        res.update(capcheck.check_corners(original, out, angle, opts,
                                          seam_tol=None if case.get("fixed") else 2.0 * hausd))
        if bl:
            res.update(capcheck.check_bl(out, bl, opts))
        if window is not None:
            res.update(window_gates(case, cfg, directory, window, speeds, opts))
        merge(gates, res, out_path.name)
        last = out
    return gates, last, first_metric


def restart_check(binary, text, directory, ranks, timeout):
    """Restart from the first adapted mesh and its rewritten restart files; the following restart files (across the
    next remesh, which the restart run must do itself) must be bitwise those of the uninterrupted run."""
    cfg = options(text)
    freq, n_time = int(cfg["ADAP_FREQ"]), int(cfg["TIME_ITER"])
    restart = directory.parent / f"{directory.name}_restart"
    if restart.exists():
        shutil.rmtree(restart)
    restart.mkdir()
    mesh = directory / f"mesh_out_{freq:05d}.su2"
    if not mesh.exists():
        return capcheck.gate(False, f"missing {mesh.name}")
    shutil.copyfile(mesh, restart / "mesh.su2")
    for step in (freq - 2, freq - 1):
        for path in directory.glob(f"restart_flow_{step:05d}.*"):
            shutil.copyfile(path, restart / path.name)
    lines = [line for line in text.splitlines() if line.split("=", 1)[0].strip() not in
             ("RESTART_SOL", "RESTART_ITER", "SOLUTION_FILENAME")]
    lines += ["RESTART_SOL= YES", f"RESTART_ITER= {freq}", "SOLUTION_FILENAME= restart_flow"]
    (restart / "run.cfg").write_text("\n".join(lines) + "\n")
    code, _, log = run_su2(binary, restart, ranks, timeout)
    remeshes = len(re.findall(r"Mesh Adaptation Cycle|Fixed-Point Iteration", log))
    if code != 0 or "Exit Success" not in log:
        return capcheck.gate(False, f"restart run failed (exit {code})")
    names = [f"restart_flow_{s:05d}.dat" for s in range(freq, n_time)]
    bad = [n for n in names if not (restart / n).exists() or not (directory / n).exists()
           or (restart / n).read_bytes() != (directory / n).read_bytes()]
    return capcheck.gate(not bad and remeshes > 0, f"{len(names) - len(bad)} of {len(names)} restart files bitwise "
                                                   f"(steps {freq}..{n_time - 1}), adaptation blocks in the restart "
                                                   f"run {remeshes}" + (f", differ: {bad}" if bad else ""))


def mpi_gates(case, cfg, directory, output, last, first_metric):
    serial = output / f"{case['name']}_np1"
    meshes = numbered(serial, "mesh_out*.su2") if serial.exists() else {}
    if not meshes or last is None:
        return {"mpi_vs_np1": ["FAIL", "no np 1 run of this case in the output folder (run --ranks 1 first)"]}
    hashes = {p.name: sha(p) for p in numbered(directory, "mesh_out*.su2").values()}
    same = hashes == {p.name: sha(p) for p in meshes.values()}
    n1 = len(capcheck.read_su2(list(meshes.values())[-1]).P)
    rel = len(last.P) / n1 - 1.0
    tol = case.get("mpi_points", 0.05)
    res = {"mpi_vs_np1": capcheck.gate(abs(rel) <= tol, f"meshes {'identical' if same else 'differ'} (sha256), last "
                                                        f"mesh points {rel:+.2%} vs np 1 (<= {tol:.0%})")}
    if first_metric is not None:
        pairs, _ = remesh_pairs(cfg, serial)
        M1, precision = capcheck.metric_of(capcheck.read_su2(serial / "mesh.su2"), pairs[0][1])
        lam = np.linalg.eigvalsh(M1).max(1)
        rel = float((np.abs(first_metric - M1).max(axis=(1, 2)) / lam).max())
        tol = 1e-8 if precision == "Float64" else 1e-5
        res["mpi_metric_vs_np1"] = capcheck.gate(rel <= tol, f"first remesh metric vs np 1: {rel:.2e} of the largest "
                                                             f"eigenvalue (<= {tol:g}, {precision})")
    return res


def select(names):
    if not names or "all" in names:
        return list(CASES)
    tags = set().union(*(c["tags"] for c in CASES))
    unknown = set(names) - {c["name"] for c in CASES} - tags
    if unknown:
        raise SystemExit(f"unknown cases/tags: {sorted(unknown)}")
    return [c for c in CASES if c["name"] in names or c["tags"] & set(names)]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--binary", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path)
    ap.add_argument("--ranks", type=int, default=1)
    ap.add_argument("--cases", nargs="*")
    ap.add_argument("--timeout", type=float, default=300.0, help="seconds per SU2 run (default 300)")
    a = ap.parse_args()
    binary, output = a.binary.resolve(), a.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if hasattr(os, "sched_setaffinity"):
        os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:2])
    capcheck.selftest()
    results_path = output / "results.json"
    results = json.loads(results_path.read_text()) if results_path.exists() else []
    for case in select(a.cases):
        name = f"{case['name']}_np{a.ranks}"
        directory = output / name
        if directory.exists():
            shutil.rmtree(directory)
        directory.mkdir()
        make_mesh(case["mesh"], directory / "mesh.su2")
        text = config_text(case["base"], case["changes"])
        (directory / "run.cfg").write_text(text)
        cfg = options(text)
        print(f"{name}: running", flush=True)
        code, seconds, log = run_su2(binary, directory, a.ranks, a.timeout)
        gates, last, first_metric = check_case(case, cfg, directory, log, code)
        if case.get("restart"):
            gates["restart_bitwise"] = (restart_check(binary, text, directory, a.ranks, a.timeout)
                                        if gates["run"][0] == "PASS" else ["FAIL", "no complete run"])
        if a.ranks > 1:
            gates.update(mpi_gates(case, cfg, directory, output, last, first_metric))
        failed = any(v[0] == "FAIL" for v in gates.values())
        status = ("XFAIL" if case.get("known_fail") else "FAIL") if failed else "PASS"
        record = {"case": case["name"], "ranks": a.ranks, "seconds": round(seconds, 1), "exit": code,
                  "points": len(last.P) if last is not None else None,
                  "meshes": {p.name: sha(p) for p in numbered(directory, "mesh_out*.su2").values()},
                  "needs": case.get("needs"), "known_fail": case.get("known_fail"), "gates": gates, "status": status}
        results = [r for r in results if not (r["case"] == case["name"] and r["ranks"] == a.ranks)] + [record]
        results_path.write_text(json.dumps(results, indent=1) + "\n")
        print(f"{name}: {status} in {seconds:.1f} s, {record['points']} points", flush=True)
        for gate_name, (gate_status, value) in gates.items():
            if gate_status != "PASS":
                print(f"    {gate_name}: {gate_status} {value}", flush=True)
    write_summary(results, output)
    failed = [r for r in results if r["status"] == "FAIL"]
    print(f"{'FAIL' if failed else 'PASS'}: {len(results) - len(failed)} of {len(results)} case runs pass or are "
          f"known failures (summary in {output / 'summary.txt'})")
    raise SystemExit(1 if failed else 0)


def write_summary(results, output):
    names = []
    for r in results:
        names += [g for g in r["gates"] if g not in names]
    short = {}
    for g in names:
        s = "".join(w[0] for w in g.split("_")).upper()
        while s in short.values():
            s += "'"
        short[g] = s
    order = [c["name"] for c in CASES]
    header = f"{'case':<18} {'np':>2} {'s':>6} {'points':>7} " + " ".join(f"{short[g]:>4}" for g in names) + "  status"
    rows = [header, "-" * len(header)]
    mark = {"PASS": "ok", "FAIL": "FAIL", "REPORT": "rep", "NA": "-"}
    for r in sorted(results, key=lambda r: (order.index(r["case"]) if r["case"] in order else 99, r["ranks"])):
        cells = " ".join(f"{mark[r['gates'][g][0]] if g in r['gates'] else '':>4}" for g in names)
        note = r.get("known_fail") or r.get("needs")
        rows.append(f"{r['case']:<18} {r['ranks']:>2} {r['seconds']:>6.1f} {str(r['points']):>7} {cells}  "
                    f"{r['status']}" + (f" ({note})" if note else ""))
    total = sum(r["seconds"] for r in results)
    rows += ["", f"solver time of the listed runs {total:.0f} s (restart runs and checks not included)", ""]
    rows += [f"{short[g]:>5} = {g}" for g in names]
    rows.append("")
    for r in results:
        for g, (status, value) in r["gates"].items():
            if status != "PASS":
                rows.append(f"{r['case']} np{r['ranks']} {g}: {status} {value}")
    text = "\n".join(rows) + "\n"
    (output / "summary.txt").write_text(text)
    print(text)


if __name__ == "__main__":
    main()
