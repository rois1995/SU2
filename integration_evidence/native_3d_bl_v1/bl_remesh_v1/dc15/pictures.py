#!/usr/bin/env python3
"""DC-15 pictures: planes y = 20/50/90 % of the span, LE and TE zooms, through viz/make_slices_full.page (same drawing).
Usage: nice -n 19 python3 pictures.py <event_final.mesh> <report.json> <out_dir> <title>"""
import os, sys, json
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../viz'))
import numpy as np
import make_slices as ms
import make_slices_full as mf
mesh, report, outdir, title = sys.argv[1:5]
os.makedirs(outdir, exist_ok=True)
m = ms.read_mesh(mesh)
rep = json.load(open(report)) if os.path.exists(report) else {}
# DC-15 reports: intermediate events carry cached/estimated deficits instead of an uncached audit (header shows them)
if "cells" not in rep.get("audit", {}): rep["audit"] = rep.get("final_cached") or rep.get("final_estimated") or {}
W = m["Ftri"][m["F"][:, 5] == 1]; ys = m["X"][np.unique(W), 1]; span = ys.max() - ys.min()
base = os.path.basename(mesh).replace("_final.mesh", "")
for f in (20, 50, 90):
    for where in ("le", "te"):
        sp = dict(name=f"span{f}_{where}", off=ys.min() + f / 100 * span + 1e-7, where=where,
                  label=f"{f} % span, {'leading' if where == 'le' else 'trailing'} edge")
        print(mf.page(m, rep, sp, os.path.join(outdir, f"{base}_{sp['name']}.png"), title))
