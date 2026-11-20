#!/usr/bin/env python3
"""Markdown tables from results/runs_*.csv (peak anon RSS summed over ranks)."""
import csv, glob, os
R = os.path.join(os.path.dirname(__file__), "..", "results")
def load(suffix):
    d = {}
    for f in glob.glob(f"{R}/runs_[23]D_[SML]{suffix}.csv"):
        for r in csv.DictReader(open(f)):
            d[(r["dim"], r["size"], r["type"], r["physics"], r["time"], int(r["ranks"]), r["variant"])] = r
    return d
dp = {**load(""), **load("_np")}; mp = load("_mp")
GiB = lambda r, k="peak_sum_anon_kib": int(r[k]) / 2**20
kib = lambda r, k="peak_sum_anon_kib": int(r[k]) / int(r["npoin"])
TYPES = {"2": ("quad", "tri", "mixed"), "3": ("hexa", "tet", "mixed")}
PH = ("euler", "ns", "sa", "sst")
out = []
# Table A: KiB/point, size L, 1 rank (largest mesh = least affected by the fixed start-up cost)
for size in ("M", "L"):
    out.append(f"\n### Peak RAM per point [KiB], size {size}, 1 rank, double precision\n")
    out.append("| Grid | points | " + " | ".join(f"{p.upper()} impl | {p.upper()} expl" for p in PH) + " |")
    out.append("|---|---:|" + "---:|" * 8)
    for dim in "23":
        for t in TYPES[dim]:
            k0 = (dim, size, t, "euler", "impl", 1, "base")
            if k0 not in dp: continue
            cells = []
            for p in PH:
                for ts in ("impl", "expl"):
                    r = dp.get((dim, size, t, p, ts, 1, "base")); cells.append(f"{kib(r):.1f}" if r else "-")
            out.append(f"| {dim}D {t} | {int(dp[k0]['npoin']):,} | " + " | ".join(cells) + " |")
# Table B: size scaling SA impl np1
out.append("\n### Peak RAM [GiB] vs mesh size, SA implicit, 1 rank\n")
out.append("| Grid | S | M | L | L/M (points x4.0 2D, x3.96 3D) |"); out.append("|---|---:|---:|---:|---:|")
for dim in "23":
    for t in TYPES[dim]:
        v = [dp.get((dim, s, t, "sa", "impl", 1, "base")) for s in "SML"]
        out.append(f"| {dim}D {t} | " + " | ".join(f"{GiB(r):.2f}" if r else "-" for r in v) + f" | {GiB(v[2]) / GiB(v[1]):.2f} |" if v[1] and v[2] else f"| {dim}D {t} | " + " | ".join(f"{GiB(r):.2f}" if r else "-" for r in v) + " | - |")
# Table C: setup vs solver peak, size L, 1 rank
out.append("\n### Setup peak vs solver peak [KiB/point], size L, 1 rank\n")
out.append("| Grid | Euler impl setup / solver | Euler expl setup / solver | SA impl setup / solver |"); out.append("|---|---|---|---|")
for dim in "23":
    for t in TYPES[dim]:
        c = []
        for p, ts in (("euler", "impl"), ("euler", "expl"), ("sa", "impl")):
            r = dp.get((dim, "L", t, p, ts, 1, "base"))
            c.append(f"{kib(r, 'peak_pre_anon_kib'):.1f} / {kib(r, 'peak_solve_anon_kib'):.1f}" if r else "-")
        out.append(f"| {dim}D {t} | " + " | ".join(c) + " |")
# Table D: ranks, size L, SA impl and Euler expl, total GiB
out.append("\n### Total peak RAM [GiB] vs MPI ranks, size L (default Open MPI with UCX)\n")
out.append("| Grid | SA impl np1 | np4 | np16 | Euler expl np1 | np4 | np16 |"); out.append("|---|---:|---:|---:|---:|---:|---:|")
for dim in "23":
    for t in TYPES[dim]:
        c = [dp.get((dim, "L", t, p, ts, n, "base")) for p, ts in (("sa", "impl"), ("euler", "expl")) for n in (1, 4, 16)]
        out.append(f"| {dim}D {t} | " + " | ".join(f"{GiB(r):.2f}" if r else "-" for r in c) + " |")
# Table E: mixed precision and options, size M, SA impl np1
out.append("\n### Options, size M, SA implicit, 1 rank [KiB/point]\n")
V = ("base", "prec_jacobi", "prec_lusgs", "no_muscl", "grad_wls", "krylov50")
out.append("| Grid | precision | ILU(0) (base) | Jacobi | LU-SGS | no MUSCL | WLS gradients | 50 Krylov vectors |"); out.append("|---|---|" + "---:|" * 6)
for dim in "23":
    for t in TYPES[dim]:
        for name, d in (("double", dp), ("mixed", mp)):
            c = [d.get((dim, "M", t, "sa", "impl", 1, v)) for v in V]
            out.append(f"| {dim}D {t} | {name} | " + " | ".join(f"{kib(r):.1f}" if r else "-" for r in c) + " |")
open(os.path.join(os.path.dirname(__file__), "..", "tables", "tables.md"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
