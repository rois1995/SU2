#!/usr/bin/env python3
"""Run one block of the SU2 memory study: one dimension, one mesh size, all element types.

Usage: run_set.py --dim 2 --size M [--ranks 1,4,16] [--ablate] [--dry]
For each element type: make the mesh, run every physics x time scheme x ranks (+ ablations),
delete the mesh. Each run: cases/<dim>D_<size>/<type>/<run>/ with config, timed log, RAM timeline,
summary. One CSV row per run is appended to results/runs_<dim>D_<size>.csv.
A run with a summary that says rc=0 is skipped on a resubmit.
"""
import argparse, csv, os, re, subprocess, sys, time

STUDY = os.environ["STUDY"]
SU2 = os.environ["SU2_CFD"]
JOBS = os.path.join(STUDY, "jobs")
ITER = 5

SIZES = {2: {"T": (41, 21, 1), "S": (321, 161, 1), "M": (641, 321, 1), "L": (1281, 641, 1)},
         3: {"T": (11, 9, 7), "S": (81, 41, 31), "M": (129, 65, 49), "L": (205, 103, 77)}}
TYPES = {2: ("quad", "tri", "mixed"), 3: ("hexa", "tet", "mixed")}
PHYSICS = ("euler", "ns", "sa", "sst")
TIMES = ("impl", "expl")
# Ablations: on top of sa + impl. name -> extra/overriding config lines.
ABLATIONS = {
    "prec_jacobi": {"LINEAR_SOLVER_PREC": "JACOBI"},
    "prec_lusgs": {"LINEAR_SOLVER_PREC": "LU_SGS"},
    "no_muscl": {"MUSCL_FLOW": "NO"},
    "grad_wls": {"NUM_METHOD_GRAD": "WEIGHTED_LEAST_SQUARES"},
    "krylov50": {"LINEAR_SOLVER_ITER": "50"},
}


def config(dim, physics, tscheme, extra):
    c = {
        "SOLVER": {"euler": "EULER", "ns": "NAVIER_STOKES", "sa": "RANS", "sst": "RANS"}[physics],
        "MATH_PROBLEM": "DIRECT",
        "MACH_NUMBER": "0.5", "AOA": "0.0", "SIDESLIP_ANGLE": "0.0",
        "FREESTREAM_TEMPERATURE": "288.15",
        "REYNOLDS_NUMBER": "1.0E4" if physics == "ns" else "1.0E6", "REYNOLDS_LENGTH": "1.0",
        "REF_ORIGIN_MOMENT_X": "0.0", "REF_ORIGIN_MOMENT_Y": "0.0", "REF_ORIGIN_MOMENT_Z": "0.0",
        "REF_LENGTH": "1.0", "REF_AREA": "1.0",
        "MARKER_FAR": "( inlet, outlet, top )",
        "MARKER_PLOTTING": "( wall )", "MARKER_MONITORING": "( wall )",
        "NUM_METHOD_GRAD": "GREEN_GAUSS",
        "CONV_NUM_METHOD_FLOW": "ROE", "MUSCL_FLOW": "YES",
        "SLOPE_LIMITER_FLOW": "VENKATAKRISHNAN", "VENKAT_LIMITER_COEFF": "0.05",
        "TIME_DISCRE_FLOW": "EULER_IMPLICIT" if tscheme == "impl" else "EULER_EXPLICIT",
        "CFL_NUMBER": "5.0" if tscheme == "impl" else "0.5", "CFL_ADAPT": "NO",
        "LINEAR_SOLVER": "FGMRES", "LINEAR_SOLVER_PREC": "ILU", "LINEAR_SOLVER_ILU_FILL_IN": "0",
        "LINEAR_SOLVER_ERROR": "1E-2", "LINEAR_SOLVER_ITER": "10",
        "ITER": str(ITER),
        "MESH_FILENAME": "../mesh.su2", "MESH_FORMAT": "SU2",
        "OUTPUT_FILES": "( SURFACE_CSV )", "OUTPUT_WRT_FREQ": "100000",
        "SCREEN_OUTPUT": "( INNER_ITER, WALL_TIME, RMS_DENSITY, LINSOL_ITER )",
        "HISTORY_OUTPUT": "( ITER, RMS_RES, LINSOL, WALL_TIME )",
        "CONV_FILENAME": "history",
    }
    if physics == "euler":
        c["MARKER_EULER"] = "( wall )"
    else:
        c["MARKER_HEATFLUX"] = "( wall, 0.0 )"
    if physics in ("sa", "sst"):
        c["KIND_TURB_MODEL"] = physics.upper()
        c["CONV_NUM_METHOD_TURB"] = "SCALAR_UPWIND"
        c["MUSCL_TURB"] = "NO"
        c["TIME_DISCRE_TURB"] = "EULER_IMPLICIT"
    if dim == 3:
        c["MARKER_SYM"] = "( side )"
    c.update(extra)
    return "".join(f"{k}= {v}\n" for k, v in c.items())


def read_summary(path):
    d = {}
    if os.path.exists(path):
        for line in open(path):
            k, _, v = line.strip().partition("=")
            d[k] = v
    return d


def phase_peaks(rundir):
    """Peak sum anon RSS before and after 'Begin Solver' (preprocessing vs solver)."""
    tb = None
    for line in open(os.path.join(rundir, "run.log"), errors="replace"):
        if "Begin Solver" in line:
            tb = float(line.split()[0]); break
    pre = post = 0
    with open(os.path.join(rundir, "run.timeline.csv")) as f:
        for r in csv.DictReader(f):
            a = int(r["anon_kib"])
            if tb is None or float(r["t"]) < tb: pre = max(pre, a)
            else: post = max(post, a)
    return tb, pre, post


def log_status(rundir):
    txt = open(os.path.join(rundir, "run.log"), errors="replace").read()
    if re.search(r"\bnan\b", txt, re.I): return "nan"
    if "Exit Success" in txt: return "ok"
    return "noexit"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dim", type=int, required=True)
    ap.add_argument("--size", required=True)
    ap.add_argument("--ranks", default="1")
    ap.add_argument("--ablate", action="store_true")
    ap.add_argument("--types", default="")
    ap.add_argument("--physics", default=",".join(PHYSICS))
    ap.add_argument("--dry", action="store_true")
    a = ap.parse_args()
    ni, nj, nk = SIZES[a.dim][a.size]
    ranks = [int(r) for r in re.split(r"[,:]", a.ranks)]  # ":" because qsub -v splits at ","
    types = a.types.split(",") if a.types else TYPES[a.dim]
    block = f"{a.dim}D_{a.size}" + os.environ.get("BLOCK_SUFFIX", "")  # suffix: separate folders for extra runs
    os.makedirs(os.path.join(STUDY, "results"), exist_ok=True)
    out_csv = os.path.join(STUDY, "results", f"runs_{block}.csv")
    cols = ["dim", "size", "type", "npoin", "nelem", "physics", "time", "ranks", "variant", "rc", "status", "wall_s",
            "peak_sum_rss_kib", "peak_sum_anon_kib", "peak_pre_anon_kib", "peak_solve_anon_kib", "t_begin_solver",
            "sum_rank_hwm_kib", "max_rank_hwm_kib", "rundir"]
    new = not os.path.exists(out_csv)
    fcsv = open(out_csv, "a", newline="")
    w = csv.writer(fcsv)
    if new:
        w.writerow(cols); fcsv.flush()

    for etype in types:
        tdir = os.path.join(STUDY, "cases", block, etype)
        os.makedirs(tdir, exist_ok=True)
        runs = [(p, t, r, "base", {}) for p in a.physics.split(",") for t in TIMES for r in ranks]
        if a.ablate:
            runs += [("sa", "impl", 1, name, extra) for name, extra in ABLATIONS.items()]
        todo = [x for x in runs if read_summary(os.path.join(tdir, f"{x[0]}_{x[1]}_np{x[2]}_{x[3]}", "run.summary")).get("rc") != "0"]
        print(f"[{time.strftime('%H:%M:%S')}] {block} {etype}: {len(todo)}/{len(runs)} runs to do", flush=True)
        if not todo:
            continue
        mesh = os.path.join(tdir, "mesh.su2")
        mesh_info = os.path.join(tdir, "mesh.info")
        if not a.dry:
            out = subprocess.run([sys.executable, os.path.join(JOBS, "memgrid.py"), "--dim", str(a.dim), "--type", etype,
                                  "--ni", str(ni), "--nj", str(nj), "--nk", str(nk), "--out", mesh],
                                 check=True, stdout=subprocess.PIPE, universal_newlines=True).stdout
            open(mesh_info, "w").write(out)
            print(out.strip(), flush=True)
        info = dict(re.findall(r"(\w+)=(\S+)", open(mesh_info).read())) if os.path.exists(mesh_info) else {}
        for physics, tscheme, np_, variant, extra in todo:
            rundir = os.path.join(tdir, f"{physics}_{tscheme}_np{np_}_{variant}")
            os.makedirs(rundir, exist_ok=True)
            open(os.path.join(rundir, "config.cfg"), "w").write(config(a.dim, physics, tscheme, extra))
            if a.dry:
                continue
            cmd = [sys.executable, os.path.join(JOBS, "rsswatch.py"), os.path.join(rundir, "run"), os.environ.get("MEM_DT", "0.2"), "--",
                   "mpirun", "-np", str(np_), "--bind-to", "core", SU2, "config.cfg"]
            t0 = time.time()
            subprocess.run(cmd, cwd=rundir)
            s = read_summary(os.path.join(rundir, "run.summary"))
            tb, pre, post = phase_peaks(rundir)
            w.writerow([a.dim, a.size, etype, info.get("npoin", ""), info.get("nelem", ""), physics, tscheme, np_, variant,
                        s.get("rc"), log_status(rundir), s.get("wall_s"), s.get("peak_sum_rss_kib"), s.get("peak_sum_anon_kib"),
                        pre, post, tb, s.get("sum_rank_hwm_kib"), s.get("max_rank_hwm_kib"), os.path.relpath(rundir, STUDY)])
            fcsv.flush()
            print(f"[{time.strftime('%H:%M:%S')}] {etype} {physics} {tscheme} np{np_} {variant}: rc={s.get('rc')} "
                  f"anon={int(s.get('peak_sum_anon_kib', 0)) / 1048576:.2f} GiB  {time.time() - t0:.0f}s", flush=True)
        if os.path.exists(mesh):
            os.remove(mesh)
    fcsv.close()
    print("SET_DONE", block, flush=True)


if __name__ == "__main__":
    main()
