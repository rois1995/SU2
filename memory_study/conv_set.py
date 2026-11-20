#!/usr/bin/env python3
"""Linear solver study: RAM (1 rank, 5 iterations) and convergence (NP ranks, to -6 relative density residual).

Usage: conv_set.py --prec_tag dp|mp [--np 16] [--maxiter 3000]
3D tet grid of size M, SA implicit. Env: STUDY, SU2_CFD (binary of the precision). One run at a time.
Rows go to results/conv_<prec_tag>.csv. A run with a DONE file is skipped on a resubmit.
"""
import argparse, csv, os, subprocess, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_set  # noqa: E402

STUDY = os.environ["STUDY"]; SU2 = os.environ["SU2_CFD"]; JOBS = os.path.join(STUDY, "jobs")
SOLVERS = {
    "fgmres10": {"LINEAR_SOLVER": "FGMRES", "LINEAR_SOLVER_ITER": "10"},
    "fgmres30": {"LINEAR_SOLVER": "FGMRES", "LINEAR_SOLVER_ITER": "30"},
    "rfgmres30r10": {"LINEAR_SOLVER": "RESTARTED_FGMRES", "LINEAR_SOLVER_ITER": "30", "LINEAR_SOLVER_RESTART_FREQUENCY": "10"},
    "fgcrodr30r10d4": {"LINEAR_SOLVER": "FGCRODR", "LINEAR_SOLVER_ITER": "30", "LINEAR_SOLVER_RESTART_FREQUENCY": "10",
                       "LINEAR_SOLVER_RESTART_DEFLATION": "4"},
    "bcgstab10": {"LINEAR_SOLVER": "BCGSTAB", "LINEAR_SOLVER_ITER": "10"},
}
PRECS = ("ILU", "LU_SGS")


def summary(path):
    d = {}
    if os.path.exists(path):
        for line in open(path):
            k, _, v = line.strip().partition("="); d[k] = v
    return d


def history_stats(path):
    """Last inner iteration, last relative density residual, mean linear iterations."""
    with open(path) as f:
        rows = list(csv.reader(f))
    head = [h.strip().strip('"') for h in rows[0]]
    data = [[x.strip() for x in r] for r in rows[1:] if r]
    def col(pred):
        for i, h in enumerate(head):
            if pred(h): return i
        return None
    it = col(lambda h: h == "Inner_Iter"); rel = col(lambda h: h.startswith("rel") and "Rho" in h)
    lin = col(lambda h: h == "Linear_Solver_Iterations")
    last = data[-1]
    lins = [float(r[lin]) for r in data] if lin is not None else []
    return (int(float(last[it])) if it is not None else len(data),
            float(last[rel]) if rel is not None else float("nan"),
            sum(lins) / len(lins) if lins else float("nan"), head)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--prec_tag", required=True)
    ap.add_argument("--np", type=int, default=16)
    ap.add_argument("--maxiter", type=int, default=3000)
    ap.add_argument("--size", default="M")
    ap.add_argument("--ram_only", action="store_true", help="skip the convergence run")
    ap.add_argument("--only", default="", help="comma list of run names, e.g. fgmres10_ilu")
    a = ap.parse_args()
    root = os.path.join(STUDY, "cases", f"conv_{a.prec_tag}_{a.size}")
    os.makedirs(root, exist_ok=True)
    mesh = os.path.join(root, "mesh.su2")
    ni, nj, nk = run_set.SIZES[3][a.size]
    npoin = ni * nj * nk
    if not os.path.exists(mesh):
        subprocess.run([sys.executable, os.path.join(JOBS, "memgrid.py"), "--dim", "3", "--type", "tet",
                        "--ni", str(ni), "--nj", str(nj), "--nk", str(nk), "--out", mesh], check=True)
    out_csv = os.path.join(STUDY, "results", f"conv_{a.prec_tag}_{a.size}.csv")
    new = not os.path.exists(out_csv)
    f = open(out_csv, "a", newline=""); w = csv.writer(f)
    if new:
        w.writerow(["precision", "solver", "prec", "npoin", "ram_anon_kib_np1", "ram_kib_per_point", "conv_np", "iters",
                    "rel_rho_final", "converged", "mean_lin_iters", "conv_wall_s", "status"]); f.flush()
    for solver, sx in SOLVERS.items():
        for prec in PRECS:
            name = f"{solver}_{prec.lower()}"
            d = os.path.join(root, name); os.makedirs(d, exist_ok=True)
            if os.path.exists(os.path.join(d, "DONE")) or (a.only and name not in a.only.replace(":", ",").split(",")):
                continue
            extra = dict(sx); extra["LINEAR_SOLVER_PREC"] = prec; extra["MESH_FILENAME"] = "../mesh.su2"
            # 1) RAM: 1 rank, 5 iterations (same as the main study).
            ram_dir = os.path.join(d, "ram"); os.makedirs(ram_dir, exist_ok=True)
            open(os.path.join(ram_dir, "config.cfg"), "w").write(run_set.config(3, "sa", "impl", {**extra, "MESH_FILENAME": "../../mesh.su2"}))
            subprocess.run([sys.executable, os.path.join(JOBS, "rsswatch.py"), os.path.join(ram_dir, "run"), "0.05", "--",
                            "mpirun", "-np", "1", "--bind-to", "core", SU2, "config.cfg"], cwd=ram_dir)
            ram = int(summary(os.path.join(ram_dir, "run.summary")).get("peak_sum_anon_kib", 0))
            if a.ram_only:
                w.writerow([a.prec_tag, solver, prec, npoin, ram, f"{ram / npoin:.3f}", "", "", "", "", "", "", "ram_only"]); f.flush()
                open(os.path.join(d, "DONE"), "w").write("done\n")
                print(f"[{time.strftime('%H:%M:%S')}] {a.prec_tag} {name}: RAM {ram / npoin:.2f} KiB/pt", flush=True)
                continue
            # 2) Convergence: NP ranks, adaptive CFL, stop at -6 orders of the relative density residual.
            conv = {**extra, "ITER": str(a.maxiter), "CFL_ADAPT": "YES", "CFL_NUMBER": "5.0",
                    "CFL_ADAPT_PARAM": "( 0.5, 1.5, 1.0, 100.0, 0.001 )", "SLOPE_LIMITER_FLOW": "NONE",
                    "CONV_FIELD": "REL_RMS_DENSITY", "CONV_RESIDUAL_MINVAL": "-6", "CONV_STARTITER": "10",
                    "SCREEN_WRT_FREQ_INNER": "50",
                    "HISTORY_OUTPUT": "( ITER, RMS_RES, REL_RMS_RES, LINSOL, WALL_TIME, CFL_NUMBER )"}
            open(os.path.join(d, "config.cfg"), "w").write(run_set.config(3, "sa", "impl", conv))
            t0 = time.time()
            subprocess.run(["mpirun", "-np", str(a.np), "--bind-to", "core", SU2, "config.cfg"], cwd=d,
                           stdout=open(os.path.join(d, "run.log"), "w"), stderr=subprocess.STDOUT)
            wall = time.time() - t0
            try:
                iters, rel, lin, _ = history_stats(os.path.join(d, "history.csv"))
            except Exception as e:  # noqa: BLE001
                iters, rel, lin = -1, float("nan"), float("nan"); print("history error", name, e)
            logtxt = open(os.path.join(d, "run.log"), errors="replace").read()
            status = "ok" if "Exit Success" in logtxt else "noexit"
            if rel != rel: status += "_nan"
            w.writerow([a.prec_tag, solver, prec, npoin, ram, f"{ram / npoin:.3f}", a.np, iters, f"{rel:.3f}",
                        int(rel <= -6.0), f"{lin:.2f}", f"{wall:.1f}", status]); f.flush()
            open(os.path.join(d, "DONE"), "w").write("done\n")
            print(f"[{time.strftime('%H:%M:%S')}] {a.prec_tag} {name}: RAM {ram / npoin:.2f} KiB/pt, iters {iters}, "
                  f"rel {rel:.2f}, lin {lin:.1f}, {wall:.0f}s, {status}", flush=True)
    f.close()
    os.remove(mesh)
    print("CONV_DONE", a.prec_tag, flush=True)


if __name__ == "__main__":
    main()
