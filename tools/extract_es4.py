#!/usr/bin/env python3
"""Summarize ES4 pairs and MMS orders. Full history stays in each run directory."""

import argparse
import csv
import math
from pathlib import Path
import re
import sys


def options(path):
    return dict((key.strip(), value.strip()) for line in path.read_text().splitlines()
                if not line.lstrip().startswith("%")
                for key, sep, value in [line.partition("%")[0].partition("=")] if sep)


def table(path):
    with path.open(newline="") as stream:
        return [{key.strip().strip('"'): value.strip() for key, value in row.items()}
                for row in csv.DictReader(stream, skipinitialspace=True)]


def wall_metrics(run, cfg):
    name = cfg.get("SURFACE_FILENAME", "surface")
    surface = run / (name if name.endswith(".csv") else name + ".csv")
    if not surface.is_file():
        return {}
    cf_key = "Skin_Friction_Coefficient_x"
    rows = [row for row in table(surface)
            if row.get(cf_key) and math.isfinite(float(row[cf_key])) and float(row[cf_key]) != 0.0]
    wall = {}
    speeds = [math.hypot(float(row["Velocity_x"]), float(row["Velocity_y"])) for row in rows
              if row.get("Velocity_x") and row.get("Velocity_y")]
    if speeds:
        wall.update(wall_slip_rms=math.sqrt(sum(v * v for v in speeds) / len(speeds)), wall_slip_max=max(speeds))
    if run.parent.name == "T2_lam_plate" and rows:
        # The campaign plate leading edge is x=0. Sample the same Re_x interval as the paper.
        reynolds_grad = float(cfg["REYNOLDS_NUMBER"]) / float(cfg.get("REYNOLDS_LENGTH", 1.0))
        samples = [(float(row["x"]) * reynolds_grad, float(row[cf_key])) for row in rows if "x" in row]
        error = [abs(cf / (0.664 / math.sqrt(re_x)) - 1) for re_x, cf in samples if 1e5 < re_x < 8e5]
        if error:
            wall["cf_abs_percent"] = 100 * sum(error) / len(error)
    return wall


def final(run):
    cfg = options(run / "config.cfg")
    info = {"status": "pending", "target": float(cfg.get("CONV_RESIDUAL_MINVAL", "-14"))}
    status = run / "es4_status.csv"
    if not status.is_file():
        return info
    metadata, = table(status)
    info.update({key: metadata[key] for key in ("seconds", "job", "ranks", "binary_sha256")})
    rc = int(metadata["rc"])
    name = cfg.get("CONV_FILENAME", "history")
    history = run / (name if name.endswith(".csv") else name + ".csv")
    log = (run / "run.log").read_text(errors="replace")
    rows = table(history) if history.is_file() else []
    if rows:
        last = rows[-1]
        info.update(iter=int(last["Inner_Iter"]) + 1, rho=float(last["rms[Rho]"]),
                    cd=last.get("CD", ""), cl=last.get("CL", ""))
        settled = [row for row in rows if int(row["Inner_Iter"]) >= info["iter"] - 200]
        if len(settled) >= 3:
            info["forces_settled"] = all(force_settled(settled, key) for key in ("CD", "CL"))
        tail = [float(row["rms[Rho]"]) for row in rows if int(row["Inner_Iter"]) >= info["iter"] - 2000]
        span = int(last["Inner_Iter"]) - int(rows[0]["Inner_Iter"])
        info["stall"] = span >= 1999 and max(tail) - min(tail) < 0.2 and info["rho"] > info["target"]
    error_blocks = log.split("Global Error Analysis")[1:]
    if error_blocks:
        fields = ("Rho", "RhoE")
        errors = [dict(re.findall(r"RMS Error\s*\[(\w+)\]:\s*([0-9.eE+-]+)", block)) for block in error_blocks]
        h = re.search(r"hEff\s*:\s*([0-9.eE+-]+)", error_blocks[-1])
        if h and all(key in errors[-1] for key in fields):
            info.update(h=float(h[1]), err_rho=float(errors[-1]["Rho"]), err_rhoe=float(errors[-1]["RhoE"]))
        # MMS error settling requires more than a single error evaluation.
        if len(errors) >= 2 and all(key in error for error in errors[-2:] for key in fields):
            info["errors_settled"] = all(math.isclose(float(errors[-2][key]), float(errors[-1][key]), rel_tol=0.001)
                                         for key in fields)
    cap = int(cfg.get("ITER", "999999"))
    if run.parent.name.startswith(("T2_", "T4_")):
        info.update(wall_metrics(run, cfg))
    if rc == 124:
        state = "timeout"
    elif re.search(r"diverged|divergence detected|\bNaN\b", log, re.IGNORECASE):
        state = "diverged"
    elif rc or not rows or not math.isfinite(info["rho"]):
        state = "failed"
    elif info["rho"] <= info["target"]:
        state = "target"
    elif info["stall"]:
        state = "stall"
    elif info["iter"] >= cap:
        state = "cap"
    else:
        state = "early exit"
    info["status"] = state
    return info


def force_settled(rows, key):
    if not rows[-1].get(key):
        return False
    vals = [float(row[key]) for row in rows]
    return all(math.isfinite(v) for v in vals) and max(vals) - min(vals) <= max(1e-6, 1e-3 * abs(vals[-1]))


def paired(a, b, mms):
    field = "errors_settled" if mms else "forces_settled"
    return (a["status"] == b["status"] == "target" and a["target"] == b["target"]
            and a.get(field, False) and b.get(field, False))


def relative(a, b, key):
    if a.get(key) in (None, "", 0) or b.get(key) in (None, ""):
        return ""
    x, y = float(a[key]), float(b[key])
    return (y - x) / abs(x) if x else ""


def same_job(a, b):
    return all(a.get(key) and a.get(key) == b.get(key) for key in ("job", "ranks", "binary_sha256"))


def export(root, out):
    out.mkdir(parents=True, exist_ok=True)
    mms = {}
    fields = ("status", "iter", "rho", "cd", "cl", "seconds", "h", "err_rho", "err_rhoe",
              "cf_abs_percent", "wall_slip_rms", "wall_slip_max")
    columns = ["case", "mesh", "variant", "matched", "cd_percent", "cl_delta", "iteration_ratio",
               "sec_iter_ratio", "gate"]
    columns += [f"{label}_{field}" for label in ("base", "es4") for field in fields]
    with (out / "pairs.csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(columns)
        for exp in sorted(root.glob("*/*_es4__*/config.cfg")):
            run = exp.parent
            variant, mesh = run.name.split("__")
            variant = variant[:-4]
            case = run.parent.name
            a, b = final(run.with_name(f"{variant}__{mesh}")), final(run)
            is_mms = mesh.startswith("N")
            matched = paired(a, b, is_mms)
            delta_cd = relative(a, b, "cd") if matched else ""
            delta_cl = float(b["cl"]) - float(a["cl"]) if matched and a.get("cl") and b.get("cl") else ""
            ratio_iter = b["iter"] / a["iter"] if matched else ""
            timed = case == "T2_lam_plate" or (case == "T4_naca_a10" and mesh == "449x129") or mesh == "N256"
            ratio_time = ""
            if timed and matched and same_job(a, b) and float(a["seconds"]) > 0:
                ratio_time = (float(b["seconds"]) / b["iter"]) / (float(a["seconds"]) / a["iter"])
            if not timed or not same_job(a, b):
                a.pop("seconds", None)
                b.pop("seconds", None)
            gate = "pending"
            if b["status"] in ("stall", "diverged") and a["status"] != b["status"]:
                gate = "new " + b["status"]
            elif matched:
                gate = "pass"
                if ratio_iter > 1.25:
                    gate = "iteration limit exceeded"
                elif case.startswith("T4_") and delta_cd != "" and abs(delta_cd) > 0.01:
                    gate = "drag tolerance exceeded"
                elif case == "T2_lam_plate":
                    if "cf_abs_percent" not in a or "cf_abs_percent" not in b:
                        gate = "Cf pending"
                    elif b["cf_abs_percent"] > a["cf_abs_percent"] + 1.0:
                        gate = "Cf tolerance exceeded"
                elif is_mms and mesh == "N512" and any((relative(a, b, key) or 0.0) > 0.10
                                                       for key in ("err_rho", "err_rhoe")):
                    gate = "MMS error tolerance exceeded"
            row = [case, mesh, variant, matched, "" if delta_cd == "" else 100 * delta_cd,
                   delta_cl, ratio_iter, ratio_time, gate]
            writer.writerow(row + [res.get(field, "") for res in (a, b) for field in fields])
            if is_mms:
                mms[case, int(mesh[1:])] = (matched, a, b)
    with (out / "mms_orders.csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(("family", "levels", "matched", "base_rho", "es4_rho", "base_rhoe", "es4_rhoe", "gate"))
        for case in sorted({case for case, _ in mms}):
            for coarse, fine in ((128, 256), (256, 512)):
                ca, fa = mms.get((case, coarse)), mms.get((case, fine))
                if ca is None or fa is None:
                    continue
                order = [math.log(c[key] / f[key]) / math.log(c["h"] / f["h"]) if c.get(key) and f.get(key) else ""
                         for key in ("err_rho", "err_rhoe") for c, f in zip(ca[1:], fa[1:])]
                matched = ca[0] and fa[0]
                gate = "pending"
                if matched and all(value != "" for value in order):
                    gate = "order tolerance exceeded" if max(order[0] - order[1], order[2] - order[3]) > 0.15 else "pass"
                writer.writerow((case, f"{coarse}-{fine}", matched, *order, gate))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cases", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if not args.cases.is_dir():
        sys.exit(f"Missing case tree {args.cases}")
    export(args.cases, args.output)


if __name__ == "__main__":
    main()
