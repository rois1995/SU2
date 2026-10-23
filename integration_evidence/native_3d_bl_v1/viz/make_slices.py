#!/usr/bin/env python3
"""Readable 2D-slice pictures of native 3D BL meshes (DC-10/DC-11/DC-12 *_final.mesh files).

Usage:  nice -n 19 python3 make_slices.py            (all DC-11 pages)
        nice -n 19 python3 make_slices.py m6_root_te  (only cases whose name contains the argument)
        DC_RUNS=dc12,dc12vo nice -n 19 python3 make_slices.py   (DC-12 pages into dc12/png_readable)

Each page cuts the tetrahedral mesh with one plane (exact tet/plane intersection polygons) and draws it like
a classic 2D mesh plot at three zoom levels, with a plain-words legend, failure counts split by location and
by distance from the wall (in multiples of the first-cell height h0), and a one-line verdict from the JSON.

Data used (read from the .mesh written by the DC harness, nothing recomputed by the solver):
  cell_gates  : per cell metric q, J, bad flag (q<0.20 or J<0.05), first-child flag, column id, class
                (0 = outside BL columns, 1 = first-row cell on the wall, 2 = other cell inside a BL column)
  long_edges  : every edge with metric length > 1.8 (class 0 interior / 1 wall / 2 seam / 3 other physical
                boundary / 4 unmeasurable; inside-columns-only flag)
First-height failures are not stored per cell; they are recomputed here geometrically (first-row cell height
above its wall face vs h0, tolerance 10%) and the count is cross-checked against the JSON 'height_failures'.
"""
import sys, os, json, numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection, LineCollection
from matplotlib.patches import Rectangle, Patch
from matplotlib.lines import Line2D

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.join(HERE, "..", "bl_remesh_v1")
DC11 = os.path.join(BASE, "dc11")
OUT = os.path.join(DC11, "png_readable")
PAIRS = [(0, 1), (0, 2), (0, 3), (1, 2), (1, 3), (2, 3)]
HEIGHT_TOL = 0.10

C_EDGE = "#9a9a9a"; C_WALL = "black"; C_FIRST = "#9fd3f5"; C_COL = "#fbf0c2"; C_BAD = "#e0262b"
C_LONG = "#ff8c00"; C_HGT = "#8e3fbf"; C_BND = "#555555"; C_VOID = "#d9d9d9"

CASES = {
    "m6_root_te": dict(what="ONERA M6 wing, root trailing edge, cut region (x: chord, y: span, z: up)",
                       normal=1, where="te", units="mm", plane_note="plane normal to the span through the middle of the trailing edge"),
    "m6_tip_te": dict(what="ONERA M6 wing, tip trailing edge, cut region (x: chord, y: span, z: up)",
                      normal=1, where="te", units="mm", plane_note="plane normal to the span through the middle of the trailing edge"),
    "m6_le": dict(what="ONERA M6 wing, leading edge near mid-span, cut region (x: chord, y: span, z: up)",
                  normal=1, where="le", units="mm", plane_note="plane normal to the span through the middle of the leading edge"),
    "tube_root_x": dict(what="Test fixture: inclined square tube leaving a symmetry plane at x=0 (root junction)",
                        normal=0, where="root", units="model", plane_note="plane x = const close to the root junction; tube inclined ~33 deg"),
    "two_tube_y": dict(what="Test fixture: two tubes leaving a symmetry plane at y=0 (root junction)",
                       normal=1, where="root", units="model", plane_note="plane y = const close to the root junction; tubes inclined ~17 deg"),
    "annulus": dict(what="Test fixture: sheared unit box with a square hole through it; wall = flat bottom face z=0 (coarse, 250 cells)",
                    normal=1, where="worst", units="model", plane_note="plane normal to y through the densest group of failures"),
}
FN = {"te": "focus", "le": "focus", "root": "focus", "worst": "focus"}
RUNS = {"single": ("dc11/runs_single", "DC-11 single lift"), "fan": ("dc11/runs_fan", "DC-11 crease fan"),
        "single600": ("dc11/runs_single_600", "DC-11 single lift, 600 s budget"), "dc10": ("dc10/diag2", "DC-10 diag2"),
        "dc12": (os.environ.get("DC12_RUN_DIR", "dc12/runs"), "DC-12 build-then-optimize (G growth layers + validity-only V + optimizer)"),
        "dc12vo": (os.environ.get("DC12_RUN_DIR", "dc12/runs"), "DC-12 production volume-only RefineOptimize (no wall heights)")}
PREFIX = {"dc10": "dc10", "dc12": "dc12", "dc12vo": "volonly12"}
OUTS = {"dc12": os.path.join(BASE, os.environ.get("DC12_PNG_DIR", "dc12/png_readable")), "dc12vo": os.path.join(BASE, os.environ.get("DC12_PNG_DIR", "dc12/png_readable"))}


# ----------------------------------------------------------------------------------------------- reading
def read_mesh(path):
    L = open(path).read().split("\n"); i = 0; ids = []; X = []; C = []; F = []; G = []; E = []; h0 = 1.0
    while i < len(L):
        t = L[i].split(); i += 1
        if not t: continue
        n = int(t[1]) if len(t) > 1 and t[1].lstrip("-").isdigit() else 0
        blk = L[i:i + n]
        if t[0] == "nodes":
            A = np.array([r.split() for r in blk], float); ids = A[:, 0].astype(np.int64); X = A[:, 1:4]
        elif t[0] == "cells": C = np.array([r.split() for r in blk], np.int64)
        elif t[0] == "faces": F = np.array([r.split() for r in blk], np.int64)
        elif t[0] == "h0": h0 = float(t[1])
        elif t[0] == "cell_gates": G = np.array([r.split() for r in blk], float)
        elif t[0] == "long_edges": E = np.array([r.split() for r in blk], float)
        i += n
    lut = np.full(ids.max() + 1, -1, np.int64); lut[ids] = np.arange(len(ids))
    m = dict(X=X, h0=h0, cid=C[:, 0], T=lut[C[:, 1:5]], F=F, Ftri=lut[F[:, 1:4]])
    gi = {int(c): k for k, c in enumerate(G[:, 0].astype(np.int64))}
    order = np.array([gi[int(c)] for c in m["cid"]])
    G = G[order]
    m.update(q=G[:, 1], J=G[:, 2], bad=G[:, 3] > 0.5, first=G[:, 4] > 0.5, col=G[:, 5].astype(np.int64), cls=G[:, 6].astype(int))
    if len(E):
        m.update(E=lut[E[:, 0:2].astype(np.int64)], Elen=E[:, 2], Ecls=E[:, 3].astype(int), Ecol=E[:, 4] > 0.5)
    else:
        m.update(E=np.zeros((0, 2), np.int64), Elen=np.zeros(0), Ecls=np.zeros(0, int), Ecol=np.zeros(0, bool))
    return m


def point_tri_dist(P, A, B, C):
    """Exact distance from points P (n,3) to triangles (m,3) each; returns (n,) minimum over triangles."""
    best = np.full(len(P), np.inf)
    for s in range(0, len(P), 1500):
        p = P[s:s + 1500, None, :]
        ab, ac = (B - A)[None], (C - A)[None]; ap = p - A[None]
        d1 = (ab * ap).sum(-1); d2 = (ac * ap).sum(-1)
        bp = p - B[None]; d3 = (ab * bp).sum(-1); d4 = (ac * bp).sum(-1)
        cp = p - C[None]; d5 = (ab * cp).sum(-1); d6 = (ac * cp).sum(-1)
        va = d3 * d6 - d5 * d4; vb = d5 * d2 - d1 * d6; vc = d1 * d4 - d3 * d2
        den = va + vb + vc; den = np.where(np.abs(den) < 1e-300, 1e-300, den)
        v = vb / den; w = vc / den
        inside = (va >= 0) & (vb >= 0) & (vc >= 0)
        proj = A[None] + v[..., None] * ab + w[..., None] * ac
        dist = np.where(inside, np.linalg.norm(p - proj, axis=-1), np.inf)
        for a, b in ((A, B), (B, C), (C, A)):
            e = (b - a)[None]; t = np.clip(((p - a[None]) * e).sum(-1) / np.maximum((e * e).sum(-1), 1e-300), 0, 1)
            dist = np.minimum(dist, np.linalg.norm(p - (a[None] + t[..., None] * e), axis=-1))
        best[s:s + 1500] = dist.min(1)
    return best


def analyse(m):
    X, T, F = m["X"], m["T"], m["F"]
    wall = F[:, 5] == 1
    W = m["Ftri"][wall]; m["W"] = W
    A, B, C = X[W[:, 0]], X[W[:, 1]], X[W[:, 2]]
    m["cent"] = X[T].mean(1)
    m["dcell"] = point_tri_dist(m["cent"], A, B, C) / m["h0"]
    mid = 0.5 * (X[m["E"][:, 0]] + X[m["E"][:, 1]]) if len(m["E"]) else np.zeros((0, 3))
    m["dedge"] = point_tri_dist(mid, A, B, C) / m["h0"] if len(mid) else np.zeros(0)
    # first-height check: first-row cell owning a wall face; height of the 4th vertex above that face
    row = {int(c): k for k, c in enumerate(m["cid"])}
    hf = np.zeros(len(T), bool); herr = np.full(len(T), np.nan)
    for f, tri in zip(F[wall], W):
        k = row.get(int(f[0]))
        if k is None or not m["first"][k]: continue
        apex = [v for v in T[k] if v not in tri]
        if len(apex) != 1: continue
        a, b, c = X[tri]; nrm = np.cross(b - a, c - a); nrm /= np.linalg.norm(nrm)
        h = abs(np.dot(X[apex[0]] - a, nrm)); herr[k] = h / m["h0"] - 1.0
        hf[k] = abs(herr[k]) > HEIGHT_TOL * (1 + 1e-6)
    m["hfail"] = hf; m["herr"] = herr
    # first BL row for display: the wall-face tet (class 1) plus the other column tets of the same layer (centre below h0)
    m["row1"] = (m["cls"] == 1) | ((m["cls"] == 2) & (m["dcell"] < 1.0))
    return m


# ----------------------------------------------------------------------------------------------- slicing
def basis(normal):
    n = np.zeros(3); n[normal] = 1
    u, v = {0: (1, 2), 1: (0, 2), 2: (0, 1)}[normal]
    return n, u, v


def cut(m, normal, off):
    """Exact polygons of all tets crossing the plane x[normal] = off; returns list of (k,2) arrays + cell ids."""
    X, T = m["X"], m["T"]; n, iu, iv = basis(normal)
    s = X[:, normal] - off
    S = s[T]; cross = (S.min(1) < 0) & (S.max(1) > 0)
    polys, cells = [], []
    for k in np.nonzero(cross)[0]:
        pts = []
        for a, b in PAIRS:
            sa, sb = S[k, a], S[k, b]
            if (sa < 0) != (sb < 0):
                t = sa / (sa - sb); P = X[T[k, a]] + t * (X[T[k, b]] - X[T[k, a]]); pts.append((P[iu], P[iv]))
        P = np.array(pts); c = P.mean(0); ang = np.arctan2(P[:, 1] - c[1], P[:, 0] - c[0])
        polys.append(P[np.argsort(ang)]); cells.append(k)
    return polys, np.array(cells, np.int64)


def cut_tris(m, tris, normal, off):
    X = m["X"]; _, iu, iv = basis(normal); segs = []
    s = X[:, normal] - off
    for tri in tris:
        st = s[tri]
        if not ((st.min() < 0) and (st.max() > 0)): continue
        pts = []
        for a, b in ((0, 1), (1, 2), (2, 0)):
            if (st[a] < 0) != (st[b] < 0):
                t = st[a] / (st[a] - st[b]); P = X[tri[a]] + t * (X[tri[b]] - X[tri[a]]); pts.append((P[iu], P[iv]))
        if len(pts) == 2: segs.append(pts)
    return np.array(segs).reshape(-1, 2, 2)


def cut_long_edges(m, normal, off):
    X = m["X"]; _, iu, iv = basis(normal)
    if not len(m["E"]): return np.zeros((0, 2, 2)), np.zeros(0, np.int64)
    s = X[:, normal] - off; a, b = m["E"][:, 0], m["E"][:, 1]
    hit = (s[a] < 0) != (s[b] < 0)
    L = np.linalg.norm(X[b] - X[a], axis=1); flat = hit & (np.abs(s[a] - s[b]) < np.sin(np.radians(25)) * L)
    segs = np.stack([X[a[flat]][:, [iu, iv]], X[b[flat]][:, [iu, iv]]], 1)   # nearly in the plane: true shape
    t = s[a[hit]] / (s[a[hit]] - s[b[hit]])
    pts = (X[a[hit]] + t[:, None] * (X[b[hit]] - X[a[hit]]))[:, [iu, iv]]   # every crossing: piercing point
    return (segs, pts), np.nonzero(hit)[0]


def choose_plane(m, spec):
    X, W, normal = m["X"], m["W"], spec["normal"]
    wn = np.unique(W); WX = X[wn]
    lo, hi = WX[:, normal].min(), WX[:, normal].max()
    if spec["where"] in ("te", "le"):
        return 0.5 * (lo + hi) + 1e-7 * (hi - lo)
    fail = m["bad"] | m["hfail"]
    if spec["where"] == "root":   # close to the root junction: scan the first 30% of the tube length
        cand = np.linspace(lo + 0.02 * (hi - lo), lo + 0.30 * (hi - lo), 15)
    else:
        cand = np.linspace(lo + 0.05 * (hi - lo), hi - 0.05 * (hi - lo), 25)
    best, bo = -1, cand[0]
    for o in cand:
        S = X[m["T"]][:, :, normal] - o; hit = (S.min(1) < 0) & (S.max(1) > 0)
        e = X[m["E"], normal] - o if len(m["E"]) else np.zeros((0, 2))
        score = 20 * int((hit & fail).sum()) + (int(((e[:, 0] < 0) != (e[:, 1] < 0)).sum()) if len(e) else 0)
        if score > best: best, bo = score, o
    return bo + 1e-7 * (hi - lo)


# ----------------------------------------------------------------------------------------------- drawing
def scale_of(spec, m):
    return (1000.0, "mm") if spec["units"] == "mm" else (1.0, "model units")


def focus_point(m, spec, wsegs, polys, cells, radius):
    """Wall point of the cut with the most failing cells within 'radius' (TE/LE cases: only points at the edge)."""
    P = wsegs.reshape(-1, 2)
    if not len(P): return np.array([p.mean(0) for p in polys]).mean(0)
    span = np.ptp(P[:, 0]) + 1e-300
    if spec["where"] == "te": P = P[P[:, 0] >= P[:, 0].max() - 0.02 * span]
    if spec["where"] == "le": P = P[P[:, 0] <= P[:, 0].min() + 0.02 * span]
    fail = np.array([m["bad"][c] or m["hfail"][c] for c in cells], bool) if len(cells) else np.zeros(0, bool)
    if not fail.any(): return P[np.argmax(P[:, 0])] if spec["where"] == "te" else P[np.argmin(P[:, 0])] if spec["where"] == "le" else P.mean(0)
    cs = np.array([p.mean(0) for p, f in zip(polys, fail) if f])
    D = np.linalg.norm(P[:, None, :] - cs[None], axis=-1)
    score = (D < radius).sum(1) * 1e6 + (D < 4 * radius).sum(1) * 1e3 - D.min(1) / radius
    return P[np.argmax(score)]


def draw_mesh(ax, m, polys, cells, wsegs, bsegs, lsegs, lw_edge=0.35, lw_wall=2.2, lw_long=1.8, ms_long=10):
    fc = []
    for c in cells:
        if m["hfail"][c]: fc.append(C_HGT)
        elif m["bad"][c]: fc.append(C_BAD)
        elif m["row1"][c]: fc.append(C_FIRST)
        elif m["cls"][c] == 2: fc.append(C_COL)
        else: fc.append("white")
    ax.add_collection(PolyCollection(polys, facecolors=fc, edgecolors=C_EDGE, linewidths=lw_edge, zorder=1))
    # failing cells again on top with an outline in their own colour, so that thin slivers stay visible
    for flag, col in ((m["bad"] & ~m["hfail"], C_BAD), (m["hfail"], C_HGT)):
        sel = [p for p, c in zip(polys, cells) if flag[c]]
        if sel: ax.add_collection(PolyCollection(sel, facecolors=col, edgecolors=col, linewidths=max(1.2, 4 * lw_edge), zorder=2.5))
    if len(bsegs): ax.add_collection(LineCollection(bsegs, colors=C_BND, linewidths=1.0, linestyles=(0, (4, 2)), zorder=2))
    lsegs, lpts = lsegs
    if len(lsegs): ax.add_collection(LineCollection(lsegs, colors=C_LONG, linewidths=lw_long, alpha=0.95, zorder=3))
    if len(lpts): ax.scatter(lpts[:, 0], lpts[:, 1], s=ms_long, c=C_LONG, edgecolors="#8a4b00", linewidths=0.4, zorder=5)
    if len(wsegs): ax.add_collection(LineCollection(wsegs, colors=C_WALL, linewidths=lw_wall, zorder=4))
    ax.set_aspect("equal", adjustable="box"); ax.set_facecolor(C_VOID)
    ax.tick_params(labelsize=8)


def window(center, half):
    return (center[0] - half, center[0] + half, center[1] - half, center[1] + half)


def zoom_halves(size, h0):
    hc = max(4.0 * h0, size / 2000.0)                 # C: about 8 h0 across (zoom at most x1000)
    hb = np.sqrt((size / 2) * hc) if size / 2 / hc > 2500 else max(size / 2 / 50.0, 3 * hc)
    hb = min(hb, size / 2 / 3.0)
    if hc >= hb: hc = hb / 4
    return hb, hc


def panel_views(m, spec, sl):
    """Return list of (title, (x0,x1,y0,y1) in model units) for panels A, B, C."""
    polys = sl["polys"]; allp = np.vstack(polys)
    lo, hi = allp.min(0), allp.max(0); size = (hi - lo).max()
    f = sl["focus"]; h0 = m["h0"]
    hb, hc = zoom_halves(size, h0)
    return [("A  whole cut", (lo[0], hi[0], lo[1], hi[1])), ("B", window(f, hb)), ("C", window(f, hc))], size


def make_slice(m, spec, off=None):
    normal = spec["normal"]
    if off is None: off = choose_plane(m, spec)
    polys, cells = cut(m, normal, off)
    wsegs = cut_tris(m, m["W"], normal, off)
    bsegs = cut_tris(m, m["Ftri"][m["F"][:, 5] != 1], normal, off)
    lsegs, lidx = cut_long_edges(m, normal, off)
    sl = dict(off=off, polys=polys, cells=cells, wsegs=wsegs, bsegs=bsegs, lsegs=lsegs, lidx=lidx)
    allp = np.vstack(polys); hb, hc = zoom_halves(np.ptp(allp, 0).max(), m["h0"])
    sl["focus"] = focus_point(m, spec, wsegs, polys, cells, 1.5 * hc)
    return sl


def draw_panels(axes, m, spec, sl, views, size, label_prefix="", compact=False):
    sc, un = scale_of(spec, m); h0 = m["h0"]; f = sl["focus"]
    for k, (ax, (name, (x0, x1, y0, y1))) in enumerate(zip(axes, views)):
        if k == 0:
            T = lambda P: (P - f) * sc
            xl = f"{'xyz'[basis(spec['normal'])[1]]} - {FN[spec['where']]}  [{un}]"; yl = f"{'xyz'[basis(spec['normal'])[2]]} - {FN[spec['where']]}  [{un}]"
            lw = (0.25, 1.6, 1.4, 9)
        else:
            T = lambda P: (P - f) / h0
            xl = f"{'xyz'[basis(spec['normal'])[1]]} - {FN[spec['where']]}  [multiples of h0]"; yl = f"{'xyz'[basis(spec['normal'])[2]]} - {FN[spec['where']]}  [multiples of h0]"
            lw = (0.4, 2.6, 2.6, 30)
        polys = [T(p) for p in sl["polys"]]
        ls, lp = sl["lsegs"] if len(sl["lsegs"]) == 2 else ([], [])
        draw_mesh(ax, m, polys, sl["cells"], T(sl["wsegs"]) if len(sl["wsegs"]) else [], T(sl["bsegs"]) if len(sl["bsegs"]) else [],
                  (T(ls) if len(ls) else [], T(lp) if len(lp) else []), lw_edge=lw[0], lw_wall=lw[1], lw_long=lw[2], ms_long=lw[3])
        a0, a1 = T(np.array([x0, y0])), T(np.array([x1, y1]))
        pad = 0.03 * max(a1 - a0) if k == 0 else 0
        ax.set_xlim(a0[0] - pad, a1[0] + pad); ax.set_ylim(a0[1] - pad, a1[1] + pad)
        ax.set_xlabel(xl, fontsize=8); ax.set_ylabel(yl, fontsize=8)
        zoom = size / (x1 - x0) if k else 1
        if k == 0:
            ax.set_title(f"{label_prefix}A  Whole cut" + ("" if compact else f" ({spec['plane_note']})"), fontsize=9.5, loc="left")
            ax.plot([0], [0], marker="o", ms=9, mfc="none", mec="#1f5fbf", mew=1.8, zorder=6)
        else:
            ax.set_title(f"{label_prefix}{name}  Zoom x{zoom:,.0f} (= blue box {name} of the previous panel)", fontsize=9 if compact else 9.5, loc="left")
        views_T = [(T(np.array([v[1][0], v[1][2]])), T(np.array([v[1][1], v[1][3]]))) for v in views]
        if k + 1 < len(views):
            (b0, b1) = views_T[k + 1]
            ax.add_patch(Rectangle(b0, *(b1 - b0), fill=False, ec="#1f5fbf", lw=1.6, zorder=7))
            ax.annotate(views[k + 1][0], xy=(b1[0], b1[1]), xytext=(3, 3), textcoords="offset points", color="#1f5fbf",
                        fontsize=10, fontweight="bold", zorder=8)


def stats(m):
    cls, bad, hf = m["cls"], m["bad"], m["hfail"]
    r1 = m["row1"]
    loc_bad = [int((bad & r1).sum()), int((bad & ~r1 & (cls == 2)).sum()), int((bad & (cls == 0)).sum()), 0]
    if len(m["E"]):
        w = (m["Ecls"] == 1) | (m["dedge"] < 1.0); colo = m["Ecol"] & ~w; bnd = (m["Ecls"] == 3) & ~w & ~colo
        vol = ~w & ~colo & ~bnd
        loc_long = [int(w.sum()), int(colo.sum()), int(vol.sum()), int(bnd.sum())]
    else:
        loc_long = [0, 0, 0, 0]
    loc_h = [int(hf.sum()), 0, 0, 0]
    return loc_bad, loc_long, loc_h


def draw_stats(axb, axh, m):
    loc_bad, loc_long, loc_h = stats(m)
    labels = ["at the wall /\nfirst row", "inside the\nBL columns", "volume, outside\nthe columns", "on a non-wall\nboundary face"]
    y = np.arange(len(labels)); hgt = 0.26
    for j, (vals, col, nm) in enumerate(((loc_bad, C_BAD, "bad cell shape"), (loc_long, C_LONG, "edge too long"),
                                          (loc_h, C_HGT, "first-cell height wrong"))):
        yy = y + (j - 1) * (hgt + 0.02)
        axb.barh(yy, vals, height=hgt, color=col, label=nm)
        for yv, v in zip(yy, vals):
            axb.text(v, yv, f" {v}", va="center", fontsize=7, color="#333")
    axb.set_yticks(y); axb.set_yticklabels(labels, fontsize=8); axb.invert_yaxis()
    axb.set_title("Where the failures are (whole mesh, counts)", fontsize=9, loc="left")
    axb.tick_params(axis="x", labelsize=7); axb.spines[["top", "right"]].set_visible(False)
    axb.legend(fontsize=7, frameon=False, loc="lower right")
    xm = max(max(loc_bad), max(loc_long), max(loc_h), 1); axb.set_xlim(0, xm * 1.22)
    # distance histogram in h0
    edges = np.array([0, 1, 2, 5, 10, 30, 100, 300, 1000, 3000, 1e9])
    lab = ["<1", "1-2", "2-5", "5-10", "10-30", "30-100", "100-300", "300-1k", "1k-3k", ">3k"]
    hb = np.histogram(m["dcell"][m["bad"]], edges)[0]; hl = np.histogram(m["dedge"], edges)[0]
    hh = np.histogram(m["dcell"][m["hfail"]], edges)[0]
    last = max(np.nonzero(hb + hl + hh)[0].max() + 1 if (hb + hl + hh).any() else 1, 4)
    x = np.arange(last); wd = 0.28
    axh.bar(x - wd, hb[:last], wd, color=C_BAD, label="bad cell shape (cell centre)")
    axh.bar(x, hl[:last], wd, color=C_LONG, label="edge too long (edge midpoint)")
    axh.bar(x + wd, hh[:last], wd, color=C_HGT, label="first-cell height wrong")
    axh.set_xticks(x); axh.set_xticklabels(lab[:last], fontsize=7, rotation=0)
    axh.set_xlabel("distance from the wall  [multiples of h0]", fontsize=8)
    axh.set_title("How far from the wall (whole mesh, counts)", fontsize=9, loc="left")
    axh.tick_params(axis="y", labelsize=7); axh.spines[["top", "right"]].set_visible(False)
    axh.set_ylim(0, max(hb.max(), hl.max(), hh.max(), 1) * 1.45)
    axh.legend(fontsize=7, frameon=False, loc="upper left", ncol=2)


def legend_handles():
    return [Line2D([], [], color=C_WALL, lw=3, label="black line = wall (solid surface)"),
            Patch(fc=C_FIRST, ec=C_EDGE, label="light blue = first boundary-layer row (cells between wall and h0)"),
            Patch(fc=C_COL, ec=C_EDGE, label="pale yellow = higher cells of the BL columns"),
            Patch(fc="white", ec=C_EDGE, label="white = ordinary volume cells (thin grey = cell edges)"),
            Patch(fc=C_BAD, ec=C_EDGE, label="red = bad cell shape (metric quality q<0.20 or J<0.05)"),
            Patch(fc=C_HGT, ec=C_EDGE, label="purple = first-cell height wrong (off h0 by >10%)"),
            Line2D([], [], color=C_LONG, marker="o", ms=6, mec="#8a4b00", lw=0, label="orange dot = an edge too long for the target size\n"
                   "   (metric length >1.8) pierces the cut here"),
            Line2D([], [], color=C_LONG, lw=3, label="orange line = such an edge lying almost in the cut"),
            Line2D([], [], color=C_BND, lw=1.2, ls=(0, (4, 2)), label="dashed = other domain boundary, not a wall\n   (cut box / symmetry plane / far field / hole)"),
            Line2D([], [], color="#1f5fbf", lw=1.6, label="blue box = area shown in the next zoom; circle = focus"),
            Patch(fc=C_VOID, ec="none", label="grey background = no mesh (inside the body / outside the box)")]


def verdict(js, m):
    f = js.get("final", {})
    ok = int(f.get("complete", 0)) == 1
    s = (f"{'ACCEPTED' if ok else 'NOT accepted'}: {f.get('long_edges')} long edges, {f.get('bad_shape')} bad cells, "
         f"{f.get('height_failures', 0)} first-height failures  ({f.get('cells')} cells, worst q={f.get('q_min', 0):.3g}, "
         f"worst J={f.get('J_min', 0):.2g}, h0={m['h0']:g})")
    return s, ok


def slice_counts(m, sl):
    c = sl["cells"]
    return int(m["bad"][c].sum()), len(sl["lidx"]), int(m["hfail"][c].sum()), int(m["row1"][c].sum())


def load_case(run, case):
    d, _ = RUNS[run]; pre = PREFIX.get(run, "dc11")
    mp = os.path.join(BASE, d, f"{pre}_{case}_final.mesh"); jp = os.path.join(BASE, d, f"{pre}_{case}.json")
    if not os.path.exists(mp): return None, None
    m = analyse(read_mesh(mp)); js = json.load(open(jp))
    jh = js.get("final", {}).get("height_failures")
    hk = np.nonzero(m["hfail"])[0]
    m["hcheck"] = (f"First-height failures recomputed here: {len(hk)} (harness: {jh})"
                   + (": first-row cells " + ", ".join(f"{1 + m['herr'][k]:.0f} h0" for k in hk[:4]) + " tall instead of 1 h0" if len(hk) else ""))
    if jh is not None and int(m["hfail"].sum()) != int(jh):
        print(f"  WARNING {run}/{case}: recomputed height failures {int(m['hfail'].sum())} != harness {jh}")
    return m, js


def case_page(run, case):
    spec = CASES[case]; m, js = load_case(run, case)
    if m is None: return None
    sl = make_slice(m, spec); views, size = panel_views(m, spec, sl)
    fig = plt.figure(figsize=(16, 10), dpi=100)
    gs = fig.add_gridspec(6, 20, left=0.045, right=0.985, top=0.855, bottom=0.06, wspace=3.0, hspace=1.2)
    axA = fig.add_subplot(gs[:, 0:8]); axB = fig.add_subplot(gs[0:3, 8:13]); axC = fig.add_subplot(gs[3:6, 8:13])
    axL = fig.add_subplot(gs[0:2, 13:20]); axS = fig.add_subplot(gs[2:4, 15:20]); axH = fig.add_subplot(gs[4:6, 14:20])
    draw_panels([axA, axB, axC], m, spec, sl, views, size)
    axL.axis("off"); axL.legend(handles=legend_handles(), loc="upper left", fontsize=7.6, frameon=True, title="How to read the cuts",
                                title_fontsize=9, borderaxespad=0, handlelength=2.2)
    draw_stats(axS, axH, m)
    v, ok = verdict(js, m); nb, nl, nh, nf = slice_counts(m, sl)
    sc, un = scale_of(spec, m); _, iu, iv = basis(spec["normal"])
    fig.suptitle(f"{case}  -  {spec['what']}", x=0.01, ha="left", y=0.985, fontsize=15, fontweight="bold")
    fig.text(0.01, 0.935, v, fontsize=12.5, color=("#1a7f37" if ok else "#c0141b"), fontweight="bold")
    fig.text(0.01, 0.918, f"Run: {RUNS[run][1]}.  Cut plane {'xyz'[spec['normal']]} = {sl['off']*sc:.4f} {un}; focus (circle) at "
             f"{'xyz'[iu]}={sl['focus'][0]*sc:.4f}, {'xyz'[iv]}={sl['focus'][1]*sc:.4f} {un}.  This cut crosses {len(sl['cells'])} cells: "
             f"{nf} first-row, {nb} bad, {nl} long edges, {nh} height failures.\n{m['hcheck']}.", fontsize=8.5, color="#333", va="top", linespacing=1.4)
    out = OUTS.get(run, OUT); os.makedirs(out, exist_ok=True); fn = os.path.join(out, f"{run}_{case}.png")
    fig.savefig(fn, dpi=100); plt.close(fig)
    print(f"{fn}: off={sl['off']:.6g} cut_cells={len(sl['cells'])} bad={nb} long={nl} hfail={nh} first={nf}")
    return fn


def before_after(case="m6_root_te"):
    spec = CASES[case]
    mb, jb = load_case("dc10", case); ma, ja = load_case("single", case)
    sla = make_slice(ma, spec); off = sla["off"]; slb = make_slice(mb, spec, off)
    slb["focus"] = sla["focus"]                       # identical windows on both meshes
    views, size = panel_views(ma, spec, sla)
    fig = plt.figure(figsize=(16, 11), dpi=100)
    gs = fig.add_gridspec(2, 4, left=0.04, right=0.99, top=0.88, bottom=0.05, wspace=0.28, hspace=0.32, width_ratios=[1.25, 1, 1, 0.95])
    for r, (m, js, sl, tag) in enumerate(((mb, jb, slb, "BEFORE (DC-10 diag2)"), (ma, ja, sla, "AFTER (DC-11 runs_single)"))):
        axs = [fig.add_subplot(gs[r, c]) for c in range(3)]
        draw_panels(axs, m, spec, sl, views, size, label_prefix="", compact=True)
        v, ok = verdict(js, m); nb, nl, nh, nf = slice_counts(m, sl)
        axs[0].text(0.0, 1.13, f"{tag}:  {v.split('  (')[0]}", transform=axs[0].transAxes, fontsize=11, fontweight="bold",
                    color=("#1a7f37" if ok else "#c0141b"))
        axT = fig.add_subplot(gs[r, 3]); axT.axis("off")
        lb, ll, lh = stats(m)
        rows = [["", "bad\ncells", "long\nedges", "height\nfailures"],
                ["wall / first row", lb[0], ll[0], lh[0]], ["inside BL columns", lb[1], ll[1], 0],
                ["volume", lb[2], ll[2], 0], ["non-wall boundary", "-", ll[3], 0],
                ["total", sum(lb), sum(ll), sum(lh)], ["in this cut", nb, nl, nh]]
        tb = axT.table(cellText=rows, loc="upper center", cellLoc="center", colWidths=[0.40, 0.18, 0.18, 0.22])
        tb.auto_set_font_size(False); tb.set_fontsize(8.5); tb.scale(1, 1.55)
        for c in range(4): tb[0, c].set_height(tb[0, c].get_height() * 1.7)
        axT.set_title(f"{tag}: counts (whole mesh)\n{js['final']['cells']} cells, worst q={js['final']['q_min']:.3g}", fontsize=9)
    axL = fig.add_axes([0.765, 0.0, 0.23, 0.33]); axL.axis("off")
    axL.legend(handles=legend_handles(), loc="lower left", fontsize=7.2, frameon=True, title="How to read the cuts", title_fontsize=8.5)
    fig.suptitle(f"{case}  -  {spec['what']}:  before / after, same cut plane, same zoom windows", x=0.01, ha="left", y=0.985,
                 fontsize=14, fontweight="bold")
    sc, un = scale_of(spec, ma)
    fig.text(0.01, 0.94, f"Cut plane y = {off*sc:.4f} mm ({spec['plane_note']}); panels B and C are centred on the trailing-edge point "
             f"of the DC-11 cut and use the same windows for both meshes. Neither mesh is accepted.", fontsize=9, color="#333")
    fn = os.path.join(OUT, f"before_after_{case}.png"); fig.savefig(fn, dpi=100); plt.close(fig)
    print(fn); return fn


if __name__ == "__main__":
    flt = sys.argv[1] if len(sys.argv) > 1 else ""
    runs = os.environ.get("DC_RUNS", "single,fan,single600").split(",")
    for run in runs:
        for case in CASES:
            if flt in f"{run}_{case}": case_page(run, case)
    if "single" in runs and flt in "before_after_m6_root_te": before_after()
