#!/usr/bin/env python3
"""Mesh and metric gates of the adaptation capability cases (numpy only).

Library of run_capability.py, and a command line tool:
  capcheck.py --selftest
  capcheck.py input.su2 output.su2 [--metric file] [--complexity C] [--fixed] [--hausd H] [--angle A]
              [--bl-marker wall --bl-h0 1e-3] [--analytic bump3d|bump2d]
--metric: a non-compact SU2 binary restart (.dat, double) or an SU2 PARAVIEW file (.vtu, Float32) of the INPUT mesh
holding Metric_* (the metric that made the output mesh). Each gate is [status, value] with status PASS, FAIL,
REPORT (measured, not gated) or NA.
"""

import argparse
import itertools
import json
import math
import re
import struct
import tempfile
from pathlib import Path

import numpy as np

EDGE_WINDOW = (0.71, 1.41)
# Default thresholds (README.txt; run_capability.py overrides some per case).
DEFAULTS = {
    "points_per_complexity": {2: (0.7, 2.0), 3: (0.7, 3.0)},
    "complexity_rtol": 1e-6,
    "edge_mean": (0.8, 1.3),
    "edge_fraction": 0.6,
    "quality_p01": 0.1,
    "metric_spread": 4.0,
    "bound_slack": 0.01,
    "bl_window": (0.5, 2.0),
    "bl_fraction": 0.8,
    "bl_ridge_fraction": 0.6,
    "bl_coverage_fraction": 0.5,
    "geom_rtol": 1e-12,
}


# ------------------------------------------------------------------------------------------------ readers

class Mesh:
    def __init__(self, dim, points, elems, markers):
        self.dim, self.P, self.E, self.M = dim, points, elems, markers
        self.size = float(np.ptp(points, axis=0).max())


def read_su2(path):
    """SU2 ASCII mesh with simplices (triangles, tetrahedra) and line / triangle markers."""
    lines = Path(path).read_text().splitlines()
    dim, P, E, M, i = None, None, None, {}, 0
    nvert = {3: 2, 5: 3, 10: 4}
    while i < len(lines):
        s = lines[i].split("%")[0].strip()
        if s.startswith("NDIME"):
            dim = int(s.split("=")[1])
        elif s.startswith("NELEM"):
            n = int(s.split("=")[1])
            rows = [lines[i + 1 + k].split() for k in range(n)]
            kinds = {int(r[0]) for r in rows}
            if kinds != {5 if dim == 2 else 10}:
                raise ValueError(f"{path}: only triangles (2D) / tetrahedra (3D) are supported, types {sorted(kinds)}")
            E = np.array([r[1:1 + nvert[int(r[0])]] for r in rows], dtype=np.int64)
            i += n
        elif s.startswith("NPOIN"):
            n = int(s.split("=")[1].split()[0])
            P = np.array([lines[i + 1 + k].split()[:dim] for k in range(n)], dtype=float)
            i += n
        elif s.startswith("MARKER_TAG"):
            name = s.split("=")[1].strip()
            n = int(lines[i + 1].split("=")[1])
            rows = [lines[i + 2 + k].split() for k in range(n)]
            if any(int(r[0]) != (3 if dim == 2 else 5) for r in rows):
                raise ValueError(f"{path}: marker {name} has non-simplex faces")
            M[name] = np.array([r[1:1 + dim] for r in rows], dtype=np.int64).reshape(n, dim)
            i += n + 1
        i += 1
    if dim is None or P is None or E is None:
        raise ValueError(f"{path}: not a complete SU2 mesh")
    return Mesh(dim, P, E, M)


def read_vtu(path):
    """Points and point data of an SU2 PARAVIEW file (raw appended, UInt64 headers)."""
    data = Path(path).read_bytes()
    end = data.index(b"<AppendedData")
    start = data.index(b"_", end) + 1
    header = data[:end].decode()
    codes = {"Float32": "<f4", "Float64": "<f8", "Int32": "<i4", "UInt8": "u1", "UInt64": "<u8"}

    def array(tag):
        offset = int(re.search(r'offset="([0-9]+)"', tag)[1])
        dtype = codes[re.search(r'type="([^"]*)"', tag)[1]]
        ncomp = int(re.search(r'NumberOfComponents= *"([0-9]+)"', tag)[1])
        nbytes = struct.unpack_from("<Q", data, start + offset)[0]
        values = np.frombuffer(data, dtype=dtype, count=nbytes // np.dtype(dtype).itemsize, offset=start + offset + 8)
        return values.reshape(-1, ncomp).astype(float)

    points = array(re.search(r"<Points>\s*(<DataArray[^>]+/>)", header)[1])
    section = header.split("<PointData>", 1)[1].split("</PointData>", 1)[0]
    fields = {re.search(r'Name="([^"]*)"', tag)[1]: array(tag)[:, 0]
              for tag in re.findall(r"<DataArray[^>]+/>", section) if 'NumberOfComponents= "1"' in tag}
    return points, fields, "Float32"


def read_restart(path):
    """SU2 binary restart: header (magic, fields, points, sizeof double, 0), 33-character names, data by point."""
    data = Path(path).read_bytes()
    magic, nvar, npoint, size, _ = struct.unpack_from("<5i", data, 0)
    if magic != 535532 or size != 8:
        raise ValueError(f"{path}: not an SU2 binary restart in double precision")
    names = [data[20 + 33 * k:20 + 33 * (k + 1)].split(b"\0")[0].decode() for k in range(nvar)]
    values = np.frombuffer(data, dtype="<f8", count=nvar * npoint, offset=20 + 33 * nvar).reshape(npoint, nvar)
    fields = {name: values[:, k].copy() for k, name in enumerate(names)}
    dims = [c for c in ("x", "y", "z") if c in fields]
    return np.column_stack([fields[c] for c in dims]), fields, "Float64"


def metric_of(mesh, path):
    """(N x d x d metric tensors, precision) of a restart / VTU file, checked to belong to the mesh."""
    points, fields, precision = (read_restart if str(path).endswith(".dat") else read_vtu)(path)
    d = mesh.dim
    if len(points) != len(mesh.P):
        raise ValueError(f"{Path(path).name}: {len(points)} points, the mesh has {len(mesh.P)}")
    if points.shape[1] < d or not np.isfinite(points[:, :d]).all():
        raise ValueError(f"{Path(path).name}: missing or nonfinite coordinates")
    dev = float(np.abs(points[:, :d] - mesh.P).max())
    if dev > (1e-6 if precision == "Float32" else 1e-14) * mesh.size:
        raise ValueError(f"{Path(path).name}: points differ from the mesh by {dev:.3e}")
    names = ["XX", "XY", "YY"] if d == 2 else ["XX", "XY", "XZ", "YY", "YZ", "ZZ"]
    if any(f"Metric_{n}" not in fields for n in names):
        raise ValueError(f"{Path(path).name}: no Metric_* fields")
    M = np.zeros((len(points), d, d))
    for n in names:
        a, b = "XYZ".index(n[0]), "XYZ".index(n[1])
        M[:, a, b] = M[:, b, a] = fields[f"Metric_{n}"]
    return M, precision


# ------------------------------------------------------------------------------------------------ geometry helpers

LOCAL_FACES = {2: [(1, 2), (2, 0), (0, 1)], 3: [(1, 2, 3), (0, 3, 2), (0, 1, 3), (0, 2, 1)]}


def faces_of(E, dim):
    """All element faces (sorted node ids) with their element."""
    F = np.vstack([E[:, list(f)] for f in LOCAL_FACES[dim]])
    return np.sort(F, axis=1), np.tile(np.arange(len(E)), len(LOCAL_FACES[dim]))


def volumes(P, E, dim):
    X = P[E]
    if dim == 2:
        u, v = X[:, 1] - X[:, 0], X[:, 2] - X[:, 0]
        return 0.5 * (u[:, 0] * v[:, 1] - u[:, 1] * v[:, 0])
    u, v, w = X[:, 1] - X[:, 0], X[:, 2] - X[:, 0], X[:, 3] - X[:, 0]
    return np.einsum("ij,ij->i", u, np.cross(v, w)) / 6.0


def face_measure(P, F):
    if F.shape[1] == 2:
        return np.linalg.norm(P[F[:, 1]] - P[F[:, 0]], axis=1)
    return 0.5 * np.linalg.norm(np.cross(P[F[:, 1]] - P[F[:, 0]], P[F[:, 2]] - P[F[:, 0]]), axis=1)


def face_normals(P, F):
    if F.shape[1] == 2:
        t = P[F[:, 1]] - P[F[:, 0]]
        n = np.column_stack([-t[:, 1], t[:, 0]])
    else:
        n = np.cross(P[F[:, 1]] - P[F[:, 0]], P[F[:, 2]] - P[F[:, 0]])
    return n / np.maximum(np.linalg.norm(n, axis=1), 1e-300)[:, None]


def edges_of(E, dim):
    pairs = list(itertools.combinations(range(dim + 1), 2))
    return np.unique(np.sort(np.vstack([E[:, list(p)] for p in pairs]), axis=1), axis=0)


def unique_rows(A):
    return np.unique(A, axis=0, return_counts=True)


def point_markers(mesh):
    owner = {}
    for name, F in mesh.M.items():
        for p in np.unique(F):
            owner.setdefault(int(p), set()).add(name)
    return owner


def seg_dist(X, A, B):
    """Distance of each point of X (n x d) to the segments A-B (m x d): n x m."""
    AB = B - A
    L2 = np.maximum((AB ** 2).sum(-1), 1e-300)
    t = np.clip(np.einsum("nmd,md->nm", X[:, None, :] - A[None], AB) / L2, 0.0, 1.0)
    return np.sqrt(((X[:, None, :] - (A[None] + t[..., None] * AB[None])) ** 2).sum(-1))


def surface_dist(X, P, F, chunk=400):
    """Distance of points to a polyline (2D: segments) or a triangulated surface (3D), brute force."""
    out = np.empty(len(X))
    if F.shape[1] == 2:
        A, B = P[F[:, 0]], P[F[:, 1]]
        for s in range(0, len(X), chunk):
            out[s:s + chunk] = seg_dist(X[s:s + chunk], A, B).min(1)
        return out
    A, B, C = P[F[:, 0]], P[F[:, 1]], P[F[:, 2]]
    nn = face_normals(P, F)
    for s in range(0, len(X), chunk):
        Xs = X[s:s + chunk]
        x = Xs[:, None, :]
        h = np.einsum("nmd,md->nm", x - A[None], nn)
        q = x - h[..., None] * nn[None]
        inside = np.ones(h.shape, bool)
        for a, b in ((A, B), (B, C), (C, A)):
            inside &= np.einsum("nmd,md->nm", np.cross(b[None] - a[None], q - a[None]), nn) >= 0
        d = np.where(inside, np.abs(h), np.inf)
        for a, b in ((A, B), (B, C), (C, A)):
            d = np.minimum(d, seg_dist(Xs, a, b))
        out[s:s + chunk] = d.min(1)
    return out


def sagitta(P, F):
    """Largest chord-to-curve distance of a curved input patch, kappa L^2 / 8 with kappa from turn / dihedral angles."""
    if len(F) < 2:
        return 0.0
    n = face_normals(P, F)
    L = np.array([np.linalg.norm(P[f][:, None] - P[f][None], axis=-1).max() for f in F])
    owner = {}
    for k, f in enumerate(F):
        for a, b in (itertools.combinations(f, 2) if F.shape[1] == 3 else [(f[0], f[0]), (f[1], f[1])]):
            owner.setdefault(tuple(sorted((int(a), int(b)))), []).append(k)
    kappa = np.zeros(len(F))
    centroid = P[F].mean(1)
    for faces in owner.values():
        if len(faces) != 2:
            continue
        i, j = faces
        angle = math.acos(max(-1.0, min(1.0, float(abs(n[i] @ n[j])))))
        k = angle / max(float(np.linalg.norm(centroid[i] - centroid[j])), 1e-300)
        kappa[i], kappa[j] = max(kappa[i], k), max(kappa[j], k)
    return float((kappa * L ** 2 / 8.0).max())


class Locator:
    """Point location in a simplex mesh with a uniform bucket grid of the element bounding boxes."""

    def __init__(self, mesh):
        self.mesh = mesh
        P, E, d = mesh.P, mesh.E, mesh.dim
        X = P[E]
        self.origin = P.min(0)
        n = max(1, int(round(len(E) ** (1.0 / d) / 1.5)))
        self.h = np.maximum(np.ptp(P, 0) / n, 1e-300)
        self.n = np.full(d, n)
        a, b = self.cell(X.min(1)), self.cell(X.max(1))
        buckets = {}
        for e in range(len(E)):
            for c in itertools.product(*[range(a[e, k], b[e, k] + 1) for k in range(d)]):
                buckets.setdefault(c, []).append(e)
        self.buckets = {c: np.array(v) for c, v in buckets.items()}
        T = np.stack([X[:, k + 1] - X[:, 0] for k in range(d)], axis=2)
        self.Tinv = np.linalg.inv(T)
        self.x0 = X[:, 0]

    def cell(self, x):
        return np.clip(((x - self.origin) / self.h).astype(int), 0, self.n - 1)

    def locate(self, X):
        """Element and barycentric coordinates of each point (clipped in the best element when outside)."""
        elem = np.empty(len(X), dtype=np.int64)
        lam = np.empty((len(X), self.mesh.dim + 1))
        cells = self.cell(X)
        outside = 0
        for i, x in enumerate(X):
            cand = self.buckets.get(tuple(cells[i]))
            if cand is None:
                near = int(np.argmin(((self.mesh.P - x) ** 2).sum(1)))
                cand = np.where((self.mesh.E == near).any(1))[0]
            l = np.einsum("eij,ej->ei", self.Tinv[cand], x[None] - self.x0[cand])
            b = np.hstack([1.0 - l.sum(1, keepdims=True), l])
            k = int(np.argmax(b.min(1)))
            outside += b[k].min() < -1e-9
            bk = np.clip(b[k], 0.0, None)
            elem[i], lam[i] = cand[k], bk / bk.sum()
        return elem, lam, outside


def corners_2d(mesh, angle):
    """Points of >= 2 markers and turns > angle inside one marker: {point: marker set}."""
    owner = point_markers(mesh)
    out = {p: s for p, s in owner.items() if len(s) >= 2}
    for name, F in mesh.M.items():
        nb = {}
        for a, b in F:
            nb.setdefault(int(a), []).append(int(b))
            nb.setdefault(int(b), []).append(int(a))
        for p, q in nb.items():
            if len(q) != 2 or p in out:
                continue
            u, v = mesh.P[p] - mesh.P[q[0]], mesh.P[q[1]] - mesh.P[p]
            c = np.dot(u, v) / np.linalg.norm(u) / np.linalg.norm(v)
            if math.degrees(math.acos(max(-1.0, min(1.0, c)))) > angle:
                out[p] = {name}
    return out


def coord_key(x):
    return tuple(float(c) for c in x)


# ------------------------------------------------------------------------------------------------ gates

def gate(ok, value):
    return ["PASS" if ok else "FAIL", value]


def classify(gates, known_fail=()):
    failed = {name for name, (status, _) in gates.items() if status == "FAIL"}
    if not failed:
        return "PASS"
    return "XFAIL" if failed <= set(known_fail) and gates.get("run", [None])[0] == "PASS" else "FAIL"


def load_metric(mesh, path, mode):
    """Read mandatory remesh evidence; only FIXED_POINT can omit the file."""
    if path is None:
        status = "NA" if mode == "FIXED_POINT" else "FAIL"
        return None, None, {"metric_file": [status, f"no metric file for {mode}"]}
    try:
        M, precision = metric_of(mesh, path)
    except (OSError, ValueError, KeyError, IndexError, TypeError, struct.error) as error:
        return None, None, {"metric_file": ["FAIL", f"{Path(path).name}: {error}"]}
    return M, precision, {"metric_file": ["PASS", Path(path).name]}


def check_validity(mesh):
    res = {}
    d = mesh.dim
    vol = volumes(mesh.P, mesh.E, d)
    res["positive_volumes"] = gate(bool(vol.min() > 0.0),
                                   f"min {vol.min():.3e} mean {vol.mean():.3e} non-positive {int((vol <= 0).sum())}")
    F, _ = faces_of(mesh.E, d)
    uf, cnt = unique_rows(F)
    boundary = {tuple(r) for r in uf[cnt == 1]}
    interior = {tuple(r) for r in uf[cnt == 2]}
    over = int((cnt > 2).sum())
    marker_faces = np.sort(np.vstack(list(mesh.M.values())), axis=1) if mesh.M else np.zeros((0, d), np.int64)
    um, mcnt = unique_rows(marker_faces)
    mset = {tuple(r) for r in um}
    missing = len(boundary - mset)
    on_interior = len(mset & interior)
    absent = len(mset - boundary - interior)
    repeated = int((mcnt > 1).sum())
    res["conformity"] = gate(over == 0 and missing == 0 and on_interior == 0 and absent == 0 and repeated == 0,
                             f"faces in >2 elements {over}, boundary faces without marker {missing}, marker faces: "
                             f"interior {on_interior}, not an element face {absent}, repeated {repeated}")
    used = np.zeros(len(mesh.P), bool)
    used[mesh.E.ravel()] = True
    _, pc = unique_rows(np.round(mesh.P / (1e-12 * mesh.size)))
    _, ec = unique_rows(np.sort(mesh.E, axis=1))
    res["points_elements"] = gate(bool(used.all() and (pc == 1).all() and (ec == 1).all()),
                                  f"unused points {int((~used).sum())}, duplicate points {int((pc > 1).sum())}, "
                                  f"duplicate elements {int((ec > 1).sum())}")
    return res


def check_markers(reference, out):
    names_in, names_out = list(reference.M), list(out.M)
    empty = [n for n, F in out.M.items() if len(F) == 0]
    return {"markers": gate(set(names_in) == set(names_out) and not empty,
                            f"{names_out}" + (f" (input {names_in})" if set(names_in) != set(names_out) else "")
                            + (f" empty {empty}" if empty else ""))}


def dual_volumes(mesh):
    vol = np.abs(volumes(mesh.P, mesh.E, mesh.dim)) / (mesh.dim + 1)
    dual = np.zeros(len(mesh.P))
    np.add.at(dual, mesh.E.ravel(), np.repeat(vol, mesh.dim + 1))
    return dual


def metric_complexity(mesh, M):
    """SU2's discrete complexity: sum over vertices of sqrt(det M) x median-dual volume (|K| / (d + 1) per simplex)."""
    return float((np.sqrt(np.maximum(np.linalg.det(M), 0.0)) * dual_volumes(mesh)).sum())


def check_metric_field(M, hmin, hmax, armax, bl=False, opts=DEFAULTS, h0_min=None):
    """The metric file: finite, SPD, within the size and aspect-ratio bounds, and not uniform."""
    res = {}
    finite = bool(np.isfinite(M).all())
    lam = np.linalg.eigvalsh(M) if finite else np.ones((len(M), M.shape[1]))
    spd = bool((lam > 0).all())
    s = opts["bound_slack"]
    h = 1.0 / np.sqrt(np.maximum(lam, 1e-300))
    ar = h.max(1) / h.min(1)
    if bl:
        hmin = 0.5 * min(hmin, h0_min if h0_min is not None else hmin)
    in_size = bool((h.min() >= hmin * (1 - s)) and (h.max() <= hmax * (1 + s)))
    in_ar = bool(ar.max() <= armax * (1 + s))
    value = (f"finite {finite}, SPD {spd}, sizes {h.min():.3e} .. {h.max():.3e} (bounds {hmin:g} .. {hmax:g}), "
             f"largest aspect ratio {ar.max():.4g} (bound {armax:g})")
    ok = finite and spd and in_size and (in_ar or bl)
    res["metric_field"] = gate(ok, value + (" (aspect ratio not gated: boundary-layer metric)" if bl else ""))
    density = 1.0 / np.prod(h, axis=1)
    spread = float(np.percentile(density, 95) / np.percentile(density, 5))
    res["metric_nontrivial"] = gate(spread >= opts["metric_spread"], f"95th / 5th percentile of the metric density "
                                                                 f"sqrt(det M) {spread:.3g} "
                                                                 f"(>= {opts['metric_spread']})")
    return res


def check_complexity(src, out, target, M=None, precision="Float64", bl=False, opts=DEFAULTS):
    """Points of the output vs the target complexity (BL cases: vs the complexity of the metric file, which includes
    the boundary-layer metric) and the complexity of the metric file vs the target."""
    res = {}
    band = opts["points_per_complexity"]
    lo, hi = band[out.dim] if isinstance(band, dict) else band
    c = metric_complexity(src, M) if M is not None else None
    denominator, what = (c, "metric complexity with BL") if (bl and c is not None) else (target, "target")
    ratio = len(out.P) / denominator
    res["points_vs_complexity"] = gate(lo <= ratio <= hi, f"{len(out.P)} points / {denominator:.0f} ({what}) = "
                                                          f"{ratio:.3f} (band [{lo}, {hi}])")
    if c is not None:
        rel = c / target - 1.0
        tol = opts["complexity_rtol"] if precision == "Float64" else max(opts["complexity_rtol"], 2e-3)
        if bl:
            res["metric_complexity"] = ["REPORT", f"{c:.1f} ({rel:+.2%}; the boundary-layer metric is applied after "
                                                  f"the complexity scaling)"]
        else:
            res["metric_complexity"] = gate(abs(rel) <= tol, f"{c:.3f} vs {target} ({rel:+.2e}, tolerance {tol:g}, "
                                                             f"{precision})")
    return res


def interpolate_metric(src, M, X):
    elem, lam, outside = Locator(src).locate(X)
    return np.einsum("nk,nkij->nij", lam, M[src.E[elem]]), outside


def check_edges(src, out, M, opts=DEFAULTS, report_only=False, interior_only=False):
    """Edge lengths and element quality of the output mesh in the metric (P1 components interpolated from src)."""
    Mp, outside = interpolate_metric(src, M, out.P)
    edges = edges_of(out.E, out.dim)
    if interior_only:
        onb = np.zeros(len(out.P), bool)
        for F in out.M.values():
            onb[np.unique(F)] = True
        edges = edges[~(onb[edges[:, 0]] | onb[edges[:, 1]])]
    e = out.P[edges[:, 1]] - out.P[edges[:, 0]]
    la = np.sqrt(np.maximum(np.einsum("ni,nij,nj->n", e, Mp[edges[:, 0]], e), 0.0))
    lb = np.sqrt(np.maximum(np.einsum("ni,nij,nj->n", e, Mp[edges[:, 1]], e), 0.0))
    L = 0.5 * (la + lb)
    frac = float(((L >= EDGE_WINDOW[0]) & (L <= EDGE_WINDOW[1])).mean())
    mean = float(L.mean())
    lo, hi = opts["edge_mean"]
    value = (f"mean {mean:.3f} (band [{lo}, {hi}]), in [0.71, 1.41] {frac:.1%} (>= {opts['edge_fraction']:.0%}), "
             f"{len(L)} {'interior ' if interior_only else ''}edges, {outside} points outside the donor mesh")
    # mean-ratio quality with the mean metric of the element's vertices (1 for a unit simplex)
    d = out.dim
    Me = Mp[out.E].mean(1)
    vol = np.abs(volumes(out.P, out.E, d)) * np.sqrt(np.maximum(np.linalg.det(Me), 0.0))
    s = np.zeros(len(out.E))
    for a, b in itertools.combinations(range(d + 1), 2):
        v = out.P[out.E[:, b]] - out.P[out.E[:, a]]
        s += np.einsum("ni,nij,nj->n", v, Me, v)
    q = 4.0 * math.sqrt(3.0) * vol / s if d == 2 else 12.0 * (3.0 * vol) ** (2.0 / 3.0) / s
    p01 = float(np.percentile(q, 1))
    qvalue = f"1st percentile {p01:.3f} (>= {opts['quality_p01']}), median {np.median(q):.3f}, min {q.min():.2e}"
    if report_only:
        return {"metric_edges": ["REPORT", value], "metric_quality": ["REPORT", qvalue]}
    return {"metric_edges": gate(lo <= mean <= hi and frac >= opts["edge_fraction"], value),
            "metric_quality": gate(p01 >= opts["quality_p01"], qvalue)}


def fit_flat(X):
    """Line (2D) / plane (3D) through the points: (point, unit normal, max distance)."""
    c = X.mean(0)
    _, _, vt = np.linalg.svd(X - c)
    n = vt[-1]
    return c, n, float(np.abs((X - c) @ n).max())


def check_geometry(ref, out, fixed, hausd, analytic=None, opts=DEFAULTS):
    """Fixed: boundary bitwise. Free: flat patches stay flat (and in their box), curved ones stay near the input."""
    res = {}
    tol = opts["geom_rtol"] * ref.size
    analytic = analytic or {}
    if fixed:
        bad = []
        for name, F0 in ref.M.items():
            F = out.M.get(name, np.zeros((0, ref.dim), np.int64))
            s0 = {tuple(sorted(coord_key(ref.P[p]) for p in f)) for f in F0}
            s1 = {tuple(sorted(coord_key(out.P[p]) for p in f)) for f in F}
            if s0 != s1 or len(F) != len(F0):
                bad.append(f"{name} ({len(F0)} -> {len(F)} faces, {len(s0 ^ s1)} differ)")
        b0 = {coord_key(x) for x in ref.P[np.unique(np.vstack(list(ref.M.values())))]}
        b1 = {coord_key(x) for x in out.P[np.unique(np.vstack(list(out.M.values())))]}
        same = not bad and b0 == b1
        res["fixed_boundary_bitwise"] = gate(same, "identical" if same else
                                             f"differ: {bad}, boundary points {len(b0)} -> {len(b1)} "
                                             f"({len(b0 ^ b1)} differ)")
        return res
    parts, ok, extra = [], True, []
    joints = {p for p, sets in point_markers(out).items() if len(sets) >= 2}
    for name, F0 in ref.M.items():
        F = out.M.get(name)
        if F is None or len(F) == 0:
            ok = False
            parts.append(f"{name}: missing")
            continue
        X0, X = ref.P[np.unique(F0)], out.P[np.unique(F)]
        cen = out.P[F].mean(1)
        c, n, flat0 = fit_flat(X0)
        if flat0 <= tol:
            dev = max(float(np.abs((X - c) @ n).max()), float(np.abs((cen - c) @ n).max()))
            # MMG may bow a ridge (a joint of two patches) within its Hausdorff distance: joint points get 2 hausd
            ids = np.unique(F)
            slack = np.where(np.isin(ids, list(joints)), 2.0 * hausd, tol)[:, None]
            inbox = bool(((X >= X0.min(0) - slack) & (X <= X0.max(0) + slack)).all())
            good = dev <= tol and inbox
            parts.append(f"{name} (flat): {dev:.1e} <= {tol:.0e}" + ("" if inbox else " OUTSIDE its box"))
        else:
            limit = 2.0 * hausd + sagitta(ref.P, F0)
            d_out = max(float(surface_dist(X, ref.P, F0).max()), float(surface_dist(cen, ref.P, F0).max()))
            d_in = float(surface_dist(X0, out.P, F).max())
            good = max(d_out, d_in) <= limit
            parts.append(f"{name} (curved): out->in {d_out:.2e}, in->out {d_in:.2e} <= {limit:.2e}")
            if name in analytic:
                f = analytic[name]
                extra.append(f"{name}: output points {float(np.abs(f(X)).max()):.2e}, face centroids "
                             f"{float(np.abs(f(cen)).max()):.2e} (input face centroids "
                             f"{float(np.abs(f(ref.P[F0].mean(1))).max()):.2e})")
        ok &= good
        if not good:
            parts[-1] += " FAIL"
    res["free_boundary_geometry"] = gate(ok, "; ".join(parts))
    if extra:
        res["analytic_surface"] = ["REPORT", "; ".join(extra)]
    return res


def check_corners(ref, out, angle, opts=DEFAULTS, seam_tol=None):
    """Corners kept (within 1e-12 of the size) with their markers; no new corners; 3D seam points on the input seams
    within seam_tol (fixed: 1e-12 of the size; free: 2 ADAP_HAUSD, MMG reconstructs ridges as curves)."""
    res = {}
    tol = opts["geom_rtol"] * ref.size
    seam_tol = tol if seam_tol is None else max(seam_tol, tol)
    if ref.dim == 2:
        C0, C1 = corners_2d(ref, angle), corners_2d(out, angle)
    else:
        o0, o1 = point_markers(ref), point_markers(out)
        C0 = {p: s for p, s in o0.items() if len(s) >= 3}
        C1 = {p: s for p, s in o1.items() if len(s) >= 3}
    X1 = out.P[list(C1)] if C1 else np.zeros((0, ref.dim))
    keys1 = list(C1)
    lost, matched = [], set()
    for p, s in C0.items():
        d = np.linalg.norm(X1 - ref.P[p], axis=1) if len(X1) else np.array([np.inf])
        k = int(np.argmin(d))
        if d[k] <= tol and C1[keys1[k]] == s:
            matched.add(keys1[k])
        else:
            lost.append(coord_key(ref.P[p]))
    new = [coord_key(out.P[q]) for q in C1 if q not in matched]
    res["corners"] = gate(not lost and not new, f"input {len(C0)}, kept {len(C0) - len(lost)}, new {len(new)}"
                                                + (f", lost e.g. {lost[:2]}" if lost else "")
                                                + (f", new e.g. {new[:2]}" if new else ""))
    if ref.dim == 3:
        o0, o1 = point_markers(ref), point_markers(out)
        seams0 = {}
        for F in ref.M.values():
            for f in F:
                for a, b in ((f[0], f[1]), (f[1], f[2]), (f[2], f[0])):
                    s = o0[int(a)] & o0[int(b)]
                    if len(s) >= 2:
                        seams0.setdefault(frozenset(s), set()).add(tuple(sorted((int(a), int(b)))))
        worst, bad = 0.0, 0
        for p, s in o1.items():
            if len(s) != 2:
                continue
            seg = seams0.get(frozenset(s))
            if not seg:
                bad += 1
                continue
            seg = np.array(sorted(seg))
            d = float(seg_dist(out.P[p][None], ref.P[seg[:, 0]], ref.P[seg[:, 1]]).min())
            worst = max(worst, d)
            bad += d > seam_tol
        res["seams"] = gate(bad == 0, f"seam points off the input seams {bad}, largest distance {worst:.2e} "
                                      f"(<= {seam_tol:.1e})")
    return res


def check_bl(out, markers, opts=DEFAULTS, report_only=False, require_ridge=False):
    """Heights of the wall-adjacent cells (d volume / face measure) in units of h0, as face-measure weighted fractions.
    markers: {name: h0}. Faces touching a joint with a non-BL marker are excluded; faces touching a joint of two BL
    markers (a BL ridge) are gated separately."""
    d = out.dim
    F, owner = faces_of(out.E, d)
    lookup = {tuple(f): e for f, e in zip(F, owner)}
    pm = point_markers(out)
    vol = np.abs(volumes(out.P, out.E, d))
    bl = set(markers)
    lo, hi = opts["bl_window"]
    values = {"bl_coverage": [], "bl_cell_height": [], "bl_ridge_cell_height": []}
    passed = dict.fromkeys(values, True)
    for name, h0 in markers.items():
        W = out.M.get(name, np.empty((0, d), dtype=np.int64))
        w = face_measure(out.P, W)
        groups = {"wall": [], "ridge": []}
        excluded = 0
        for k, f in enumerate(W):
            elem = lookup.get(tuple(sorted(f)))
            if elem is None or w[k] <= 0.0:
                excluded += 1
                continue
            h = d * vol[elem] / w[k] / h0
            sets = [pm[int(p)] for p in f if len(pm[int(p)]) >= 2]
            if not sets:
                groups["wall"].append((h, w[k]))
            elif all(s <= bl for s in sets):
                groups["ridge"].append((h, w[k]))
            else:
                excluded += 1
        eligible = sum(area for rows in groups.values() for _, area in rows)
        total = float(w.sum())
        frac = eligible / total if total > 0.0 else 0.0
        need = opts["bl_coverage_fraction"]
        passed["bl_coverage"] &= eligible > 0.0 and frac >= need
        values["bl_coverage"].append(f"{name}: eligible area {eligible:.3g} / {total:.3g} = {frac:.1%} "
                                     f"(>= {need:.0%}), excluded faces {excluded}")
        for group, rows in groups.items():
            if not rows:
                continue
            h, area = np.array(rows).T
            inside = (h >= lo) & (h <= hi)
            frac = float((area * inside).sum() / area.sum())
            key = "bl_cell_height" if group == "wall" else "bl_ridge_cell_height"
            need = opts["bl_fraction"] if group == "wall" else opts["bl_ridge_fraction"]
            passed[key] &= frac >= need
            values[key].append(f"{name}: {len(h)} faces, cell height in [{lo}, {hi}] h0: {frac:.1%} of the area "
                               f"(>= {need:.0%}), median {np.median(h):.3g} h0, below {int((h < lo).sum())}, "
                               f"above {int((h > hi).sum())}")
    if require_ridge and not values["bl_ridge_cell_height"]:
        passed["bl_ridge_cell_height"] = False
        values["bl_ridge_cell_height"].append("no eligible ridge faces (required)")
    res = {key: gate(passed[key], "; ".join(rows)) for key, rows in values.items() if rows}
    if report_only:
        res = {key: ["REPORT", value] for key, (_, value) in res.items()}
    return res


def reference_metric(mesh, fields, sensors, p, complexity, hmin, hmax, armax, per_sensor_scaling=True):
    """Independent re-implementation of CSolver::ComputeMetric without corner and BL metrics: Lp metric of each
    sensor from its written Hessian scaled to the complexity, intersection, global factor with the bounds."""
    d = mesh.dim
    names = ["XX", "XY", "YY"] if d == 2 else ["XX", "XY", "XZ", "YY", "YZ", "ZZ"]
    V = dual_volumes(mesh)

    def tensor(prefix):
        T = np.zeros((len(mesh.P), d, d))
        for n in names:
            a, b = "XYZ".index(n[0]), "XYZ".index(n[1])
            T[:, a, b] = T[:, b, a] = fields[f"{prefix}{n}"]
        return T

    def recompose(vec, lam):
        return np.einsum("nij,nj,nkj->nik", vec, lam, vec)

    metrics = []
    for sensor in sensors:
        H = tensor(f"Hessian_{sensor}_")
        H[~np.isfinite(H).all(axis=(1, 2))] = 0.0
        lam, vec = np.linalg.eigh(H)
        lam = np.maximum(np.abs(lam), 1e-16)
        det = lam.prod(1)
        scale = (det ** (p / (2 * p + d)) * V).sum()
        factor = (complexity / scale) ** (2.0 / d) if per_sensor_scaling else 1.0
        lam = lam * (factor * det ** (-1.0 / (2 * p + d)))[:, None]
        metrics.append((vec, lam))
    if len(metrics) == 1:
        vec, lam = metrics[0]
    else:
        Ms = [recompose(v, np.maximum(l, 1e-14 * l.max(1, keepdims=True))) for v, l in metrics]
        M = Ms[0]
        for B in Ms[1:]:
            la, va = np.linalg.eigh(M)
            la = np.maximum(la, 1e-16)
            sq, isq = recompose(va, np.sqrt(la)), recompose(va, 1.0 / np.sqrt(la))
            lt, vt = np.linalg.eigh(isq @ B @ isq)
            M = sq @ recompose(vt, np.maximum(lt, 1.0)) @ sq
        lam, vec = np.linalg.eigh(M)
    eig_min, eig_max, ar2 = 1.0 / hmax ** 2, 1.0 / hmin ** 2, armax ** 2

    def bounded(log_scale):
        l = np.clip(math.exp(log_scale) * lam, eig_min, eig_max)
        return np.maximum(l, l.max(1, keepdims=True) / ar2)

    def error(x):
        return math.log((np.sqrt(bounded(x).prod(1)) * V).sum() / complexity)

    lo_x, hi_x = math.log(eig_min / lam.max()), math.log(eig_max / max(lam.min(), 1e-300))
    if error(lo_x) >= 0.0:
        x = lo_x
    elif error(hi_x) <= 0.0:
        x = hi_x
    else:
        for _ in range(200):
            x = 0.5 * (lo_x + hi_x)
            if error(x) < 0.0:
                lo_x = x
            else:
                hi_x = x
    return recompose(vec, bounded(x))


def metric_difference(A, B):
    return float((np.linalg.norm(A - B, axis=(1, 2)) / np.linalg.norm(B, axis=(1, 2))).max())


def check_reference_metric(mesh, fields, M, sensors, p, complexity, hmin, hmax, armax, tol=1e-5, sensitivity=1e-3):
    """The written metric vs the numpy re-implementation (largest per-point relative Frobenius difference). Scope:
    Hessians -> metric (sensors and Hessians are SU2's). With several sensors, the variants without one sensor and
    without the per-sensor scaling must differ by more than 'sensitivity' (else the match would not test them)."""
    R = reference_metric(mesh, fields, sensors, p, complexity, hmin, hmax, armax)
    worst = metric_difference(R, M)
    value = f"largest relative difference to the numpy re-implementation {worst:.2e} (<= {tol:g}), sensors {sensors}"
    ok = worst <= tol
    if len(sensors) > 1:
        variants = {f"without {s}": reference_metric(mesh, fields, [t for t in sensors if t != s], p, complexity, hmin,
                                                     hmax, armax) for s in sensors}
        variants["without per-sensor scaling"] = reference_metric(mesh, fields, sensors, p, complexity, hmin, hmax,
                                                                  armax, per_sensor_scaling=False)
        diffs = {k: metric_difference(v, M) for k, v in variants.items()}
        ok &= all(v > sensitivity for v in diffs.values())
        value += "; variants differ by " + ", ".join(f"{k} {v:.2e}" for k, v in diffs.items()) + \
                 f" (> {sensitivity:g})"
    return {"metric_reference": gate(ok, value)}


def sym_tensor(fields, prefix, d):
    names = ["XX", "XY", "YY"] if d == 2 else ["XX", "XY", "XZ", "YY", "YZ", "ZZ"]
    n = len(fields[prefix + names[0]])
    T = np.zeros((n, d, d))
    for name in names:
        a, b = "XYZ".index(name[0]), "XYZ".index(name[1])
        T[:, a, b] = T[:, b, a] = fields[prefix + name]
    return T


def abs_tensor(H):
    lam, vec = np.linalg.eigh(H)
    return np.einsum("nij,nj,nkj->nik", vec, np.abs(lam), vec)


def check_window_identity(steps, sensors, d, V, tol=1e-5):
    """WINDOW_AVERAGE / FIXED_POINT: the Hessian written at the window end is the mean |H| of the window, so
    n H_end - sum of |H| of the earlier steps (written at their steps) is |H| of the last step: positive semidefinite.
    The residual's volume-weighted norm must be comparable to the previous instantaneous |H|."""
    n = len(steps)
    if n < 2 or not sensors or not np.isfinite(V).all() or np.any(V < 0.0) or np.sum(V) <= 0.0:
        return {"window_average": ["FAIL", "window samples, sensors or dual volumes unavailable"]}
    ok, values = True, []
    for sensor in sensors:
        prefix = f"Hessian_{sensor}_"
        try:
            tensors = [sym_tensor(f, prefix, d) for f in steps]
        except (KeyError, ValueError, IndexError, TypeError) as error:
            return {"window_average": ["FAIL", f"{sensor}: Hessian evidence unavailable: {error}"]}
        if any(T.shape != (len(V), d, d) or not np.isfinite(T).all() for T in tensors):
            return {"window_average": ["FAIL", f"{sensor}: missing or nonfinite Hessian evidence"]}
        past = [abs_tensor(T) for T in tensors[:-1]]
        mean = tensors[-1]
        R = n * mean - sum(past)
        if not np.isfinite(R).all():
            return {"window_average": ["FAIL", f"{sensor}: nonfinite residual"]}
        norm = float(np.sqrt((V * (R ** 2).sum(axis=(1, 2))).sum()))
        previous = float(np.sqrt((V * (past[-1] ** 2).sum(axis=(1, 2))).sum()))
        if not np.isfinite([norm, previous]).all() or norm <= 0.0 or previous <= 0.0:
            return {"window_average": ["FAIL", f"{sensor}: residual or previous Hessian norm is zero or nonfinite"]}
        scale = float(np.abs(np.linalg.eigvalsh(n * mean)).max())
        rel = float(np.linalg.eigvalsh(R).min()) / max(scale, 1e-300)
        ratio = norm / previous
        ok &= rel >= -tol and 0.5 <= ratio <= 2.0
        values.append(f"{sensor}: residual eigenvalue {rel:.2e} of the largest (>= -{tol:g}), "
                      f"weighted norm / previous |H| {ratio:.3g} (in [0.5, 2])")
    return {"window_average": gate(ok, f"{n} steps: " + "; ".join(values))}


def density_centre(mesh, M):
    """Centroid of the metric density above its median (weights: excess density x median-dual volume)."""
    rho = np.sqrt(np.maximum(np.linalg.det(M), 0.0))
    w = np.maximum(rho - np.median(rho), 0.0) * dual_volumes(mesh)
    return (w[:, None] * mesh.P).sum(0) / w.sum()


def check_predict_lookahead(mesh, fields, M, sensors, p, complexity, hmin, hmax, armax, speed, horizon, direction,
                            fraction=0.25):
    """PREDICT: the written metric (moved over the horizon) is refined ahead of the instantaneous metric of the
    window end (re-computed from its written Hessians): its density centre moves along the flow by at least
    fraction x speed x horizon (half of the path is expected)."""
    inst = reference_metric(mesh, fields, sensors, p, complexity, hmin, hmax, armax)
    now = density_centre(mesh, inst)
    shift = density_centre(mesh, M) - now
    along = float(shift @ direction)
    need = fraction * speed * horizon
    # the prediction intersects the instants of the horizon from the current one: it must stay refined at the current
    # feature (gated) and is refined ahead of it (reported: too few points per probe on these small meshes to gate)
    path = speed * horizon
    rho_p, rho_i = (np.sqrt(np.maximum(np.linalg.det(T), 0.0)) for T in (M, inst))
    ratio = []
    for centre in (now, now + path * direction):
        near = np.linalg.norm(mesh.P - centre, axis=1) <= 0.5 * path
        ratio.append(float(rho_p[near].mean() / rho_i[near].mean()) if near.any() else 0.0)
    ok = along >= need and ratio[0] >= 0.25
    return {"predict_lookahead": gate(ok, f"density centre moved {along:.3f} along the flow (>= {need:.3f} = "
                                          f"{fraction} x speed {speed:.3g} x horizon {horizon}), across "
                                          f"{float(np.linalg.norm(shift - along * direction)):.3f}; density predicted /"
                                          f" instantaneous at the feature {ratio[0]:.2f} (>= 0.25), one path ahead "
                                          f"{ratio[1]:.2f} (reported)")}


def analytic_references(kind):
    from meshgen import BUMP_HEIGHT, BUMP2D_HEIGHT

    def bump3d(X):
        x, z = X[:, 0], X[:, 2]
        return X[:, 1] - np.where(np.abs(x) < 1.0, BUMP_HEIGHT * np.cos(0.5 * np.pi * x) ** 2 *
                                  np.sin(np.pi * z) ** 2, 0.0)

    def bump2d(X):
        x = X[:, 0]
        return X[:, 1] - np.where(np.abs(x) < 1.0, BUMP2D_HEIGHT * np.cos(0.5 * np.pi * x) ** 2, 0.0)

    return {"bump3d": {"lower": bump3d}, "bump2d": {"lower": bump2d}}[kind]


# ------------------------------------------------------------------------------------------------ self test

def selftest():
    """The gates on generated meshes: identity passes, targeted corruptions fail, analytic metric values."""
    import meshgen
    import run_capability as runner
    results = []

    def expect(label, res, gate_name, status):
        got = res[gate_name][0]
        results.append((label, gate_name, status, got))
        if got != status:
            raise AssertionError(f"selftest {label}: {gate_name} is {got}, expected {status}: {res[gate_name][1]}")

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        meshgen.rectangle(tmp / "r.su2", nx=8, ny=8, x0=0.0, x1=1.0, y0=0.0, y1=1.0)
        meshgen.bump3d(tmp / "b.su2", nx=6, ny=3, nz=3)
        meshgen.plate(tmp / "p.su2")
        r, b, p = read_su2(tmp / "r.su2"), read_su2(tmp / "b.su2"), read_su2(tmp / "p.su2")
        # Successful logs and meshes cannot replace mandatory metric evidence.
        for mode in ("steady", "WINDOW_AVERAGE", "PREDICT"):
            directory = tmp / mode
            directory.mkdir()
            (directory / "mesh.su2").write_bytes((tmp / "r.su2").read_bytes())
            number = 1 if mode == "steady" else 2
            (directory / f"mesh_out_{number:05d}.su2").write_bytes((tmp / "r.su2").read_bytes())
            cfg = {"ADAP_SIZES": "(81)", "ADAP_HMIN": "0.001", "ADAP_HMAX": "1", "ADAP_ARMAX": "10"}
            if mode != "steady":
                cfg.update(TIME_DOMAIN="YES", ADAP_UNSTEADY_METRIC=mode, ADAP_FREQ="2", TIME_ITER="3")
            log = ("Exit Success\nMMG2D status SUCCESS\nMesh complexity: 81 (ADAP_COMPLEXITY= 81)\n"
                   "mean |Hessian| of the sensors over 2 time steps\nspeed of the features 0.1 per time step\n")
            res, _, _ = runner.check_case({}, cfg, directory, log, 0)
            expect(f"{mode} successful run", res, "run", "PASS")
            expect(f"{mode} missing metric", res, "metric_file", "FAIL")
            expect(f"{mode} classification", {"case": [classify(res), ""]}, "case", "FAIL")
        expect("FIXED_POINT missing metric", load_metric(r, None, "FIXED_POINT")[2], "metric_file", "NA")
        (tmp / "bad.dat").write_bytes(b"bad")
        (tmp / "bad.vtu").write_bytes(b"bad")
        for path in (tmp / "absent.dat", tmp, tmp / "bad.dat", tmp / "bad.vtu"):
            expect(f"unreadable/malformed {path.name}", load_metric(r, path, "steady")[2], "metric_file", "FAIL")
        # XFAIL covers only declared gates, with a successful run.
        for case in (c for c in runner.CASES if c.get("known_fail")):
            known = case["known_fail"]
            res = {name: ["FAIL", "expected"] for name in known}
            res["run"] = ["PASS", "success"]
            expect(f"{case['name']} expected failures", {"case": [classify(res, known), ""]}, "case", "XFAIL")
            for name in sorted(known):
                single = {"run": ["PASS", ""], name: ["FAIL", "expected"]}
                expect(f"{case['name']} only {name}", {"case": [classify(single, known), ""]}, "case", "XFAIL")
            for name in ("run", "metric_file", "restart_bitwise", "mpi_vs_np1", "mpi_metric_vs_np1"):
                bad = {**res, name: ["FAIL", "unrelated"]}
                expect(f"{case['name']} + {name}", {"case": [classify(bad, known), ""]}, "case", "FAIL")
            expect(f"{case['name']} no failures", {"case": [classify({"run": ["PASS", ""]}, known), ""]},
                   "case", "PASS")
        for label, m in (("rect", r), ("bump3d", b)):
            for name, res in {**check_validity(m), **check_markers(m, m), **check_geometry(m, m, False, 0.01),
                              **check_geometry(m, m, True, 0.01), **check_corners(m, m, 45.0)}.items():
                expect(f"{label} identity", {name: res}, name, "PASS")
        # corruptions
        bad = read_su2(tmp / "r.su2")
        bad.E[0, [1, 2]] = bad.E[0, [2, 1]]
        expect("flipped element", check_validity(bad), "positive_volumes", "FAIL")
        bad = read_su2(tmp / "r.su2")
        bad.M["lower"] = bad.M["lower"][1:]
        expect("missing marker face", check_validity(bad), "conformity", "FAIL")
        bad = read_su2(tmp / "r.su2")
        bad.M["left"] = np.vstack([bad.M["left"], bad.M["left"][:1]])
        expect("repeated marker face", check_validity(bad), "conformity", "FAIL")
        bad = read_su2(tmp / "b.su2")
        q = int(bad.M["upper"][len(bad.M["upper"]) // 2, 0])
        bad.P[q, 1] += 1e-6
        expect("point off a plane", check_geometry(b, bad, False, 0.01), "free_boundary_geometry", "FAIL")
        bad = read_su2(tmp / "r.su2")
        q = int(np.unique(bad.M["upper"])[3])
        bad.P[q, 0] = np.nextafter(bad.P[q, 0], 2.0)
        expect("1 ulp on a fixed boundary", check_geometry(r, bad, True, 0.01), "fixed_boundary_bitwise", "FAIL")
        bad = read_su2(tmp / "r.su2")
        corner = [k for k, x in enumerate(bad.P) if x[0] == 1.0 and x[1] == 1.0][0]
        bad.P[corner] = (1.0, 0.95)
        expect("moved corner", check_corners(r, bad, 45.0), "corners", "FAIL")
        # analytic metric on the unit square, h = 1/8: M = I / h^2 -> complexity 64, edges 1, 1 and sqrt(2)
        M = np.tile(np.eye(2) * 64.0, (len(r.P), 1, 1))
        c = metric_complexity(r, M)
        assert abs(c - 64.0) < 1e-9, c
        res = check_edges(r, r, M)
        mean = float(res["metric_edges"][1].split()[1])
        exact = (144.0 + 64.0 * math.sqrt(2.0)) / 208.0  # 144 axis edges of length 1, 64 diagonals sqrt(2)
        assert abs(mean - exact) < 2e-3, res
        results.append(("analytic metric", "complexity / edge mean", f"64, {exact:.3f}", f"{c:.6f}, {mean:.3f}"))
        expect("uniform metric", check_metric_field(M, 1e-3, 1.0, 10.0), "metric_nontrivial", "FAIL")
        # Oblique intersection follows CSolver::IntersectMetrics, after global bounds.
        normal = np.array([np.cos(np.pi / 6.0), np.sin(np.pi / 6.0)])
        B = np.eye(2) * 4.0 + (1.0 / 0.005 ** 2 - 4.0) * np.outer(normal, normal)
        sq, isq = np.diag([1e3, 10.0]), np.diag([1e-3, 0.1])
        lam, vec = np.linalg.eigh(isq @ B @ isq)
        intersection = sq @ (vec @ np.diag(np.maximum(lam, 1.0)) @ vec.T) @ sq
        for label, T, status in (("oblique intersection", intersection, "PASS"),
                                 ("NaN", np.diag([np.nan, 100.0]), "FAIL"),
                                 ("non-SPD", np.diag([-1.0, 100.0]), "FAIL"),
                                 ("undersized", np.diag([1e8, 100.0]), "FAIL"),
                                 ("oversized", np.diag([1e6, 1.0]), "FAIL"),
                                 ("high aspect ratio", np.diag([1e6, 100.0]), "PASS")):
            res = check_metric_field(np.tile(T, (len(r.P), 1, 1)), 0.001, 0.5, 10.0, bl=True, h0_min=0.005)
            expect(f"BL metric {label}", res, "metric_field", status)
        T = np.tile(np.diag([1.0 / 0.0003 ** 2, 100.0]), (len(r.P), 1, 1))
        expect("BL h0 below HMIN", check_metric_field(T, 0.001, 0.5, 10.0, bl=True, h0_min=0.0005),
               "metric_field", "PASS")
        expect("ordinary metric bounds", check_metric_field(T, 0.001, 0.5, 10.0), "metric_field", "FAIL")
        # BL cell height of the plate's first row (2e-3)
        expect("plate rows", check_bl(p, {"wall": 2e-3}), "bl_cell_height", "PASS")
        expect("plate coverage", check_bl(p, {"wall": 2e-3}), "bl_coverage", "PASS")
        expect("plate rows, wrong h0", check_bl(p, {"wall": 2e-2}), "bl_cell_height", "FAIL")
        meshgen.rectangle(tmp / "coarse.su2", nx=1, ny=1)
        coarse = read_su2(tmp / "coarse.su2")
        expect("sole wall face excluded", check_bl(coarse, {"lower": 0.001}), "bl_coverage", "FAIL")
        meshgen.rectangle(tmp / "partial.su2", nx=3, ny=2)
        partial = read_su2(tmp / "partial.su2")
        expect("wall coverage below half", check_bl(partial, {"lower": 5.0}), "bl_coverage", "FAIL")
        expect("required ridge absent", check_bl(r, {"lower": 0.125, "upper": 0.125}, require_ridge=True),
               "bl_ridge_cell_height", "FAIL")
        expect("required ridge present", check_bl(r, {"lower": 0.125, "left": 0.125}, require_ridge=True),
               "bl_ridge_cell_height", "PASS")
        expect("one good wall cannot mask another", check_bl(r, {"lower": 0.125, "upper": 1.0}),
               "bl_cell_height", "FAIL")
        # reference metric of an analytic quadratic sensor on the unit square: H = diag(2, 8), one sensor, p = 2
        fields = {"Hessian_S_XX": np.full(len(r.P), 2.0), "Hessian_S_XY": np.zeros(len(r.P)),
                  "Hessian_S_YY": np.full(len(r.P), 8.0)}
        R = reference_metric(r, fields, ["S"], 2.0, 100.0, 1e-4, 10.0, 1e3)
        assert abs(metric_complexity(r, R) - 100.0) < 1e-6 and abs(R[0, 1, 1] / R[0, 0, 0] - 4.0) < 1e-12, R[0]
        results.append(("reference metric", "complexity / anisotropy", "100, 4", f"{metric_complexity(r, R):.6f}, "
                        f"{R[0, 1, 1] / R[0, 0, 0]:.6f}"))
        expect("reference vs itself", check_reference_metric(r, fields, R, ["S"], 2.0, 100.0, 1e-4, 10.0, 1e3),
               "metric_reference", "PASS")
        expect("reference vs scaled", check_reference_metric(r, fields, 1.01 * R, ["S"], 2.0, 100.0, 1e-4, 10.0, 1e3),
               "metric_reference", "FAIL")
        # window identity: a feature moving over 3 points; the right mean passes, the last step alone fails
        Hk = [np.zeros((4, 2, 2)) for _ in range(3)]
        for k in range(3):
            Hk[k][k] = np.diag([-3.0, 1.0])
        to_fields = lambda T: {f"Hessian_S_{n}": T[:, "XYZ".index(n[0]), "XYZ".index(n[1])] for n in ("XX", "XY", "YY")}
        mean = sum(abs_tensor(H) for H in Hk) / 3.0
        V = np.array([1.0, 2.0, 2.0, 1.0])
        steps = [to_fields(Hk[0]), to_fields(Hk[1]), to_fields(mean)]
        expect("window mean", check_window_identity(steps, ["S"], 2, V),
               "window_average", "PASS")
        expect("last step only", check_window_identity(steps[:-1] + [to_fields(abs_tensor(Hk[2]))],
                                                       ["S"], 2, V), "window_average", "FAIL")
        past = sum(abs_tensor(H) for H in Hk[:-1])
        for label, end in (("zero residual", past / 3.0),
                           ("oversized PSD residual", (past + 3.0 * abs_tensor(Hk[-1])) / 3.0),
                           ("undersized PSD residual", (past + 0.25 * abs_tensor(Hk[-1])) / 3.0)):
            expect(label, check_window_identity(steps[:-1] + [to_fields(end)], ["S"], 2, V),
                   "window_average", "FAIL")
        expect("missing window Hessian", check_window_identity(steps, ["missing"], 2, V), "window_average", "FAIL")
        expect("zero previous Hessian", check_window_identity([steps[0], to_fields(Hk[1] * 0.0), steps[-1]],
                                                              ["S"], 2, V),
               "window_average", "FAIL")
        expect("unequal dual volumes", check_window_identity(steps, ["S"], 2, np.array([1.0, 16.0, 1.0, 1.0])),
               "window_average", "FAIL")
        bad = {**steps[-1], "Hessian_S_XX": np.full(4, np.nan)}
        expect("nonfinite window Hessian", check_window_identity(steps[:-1] + [bad], ["S"], 2, V),
               "window_average", "FAIL")
    for row in results:
        print("  ".join(str(v) for v in row))
    print(f"selftest PASS ({len(results)} checks)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", nargs="?")
    ap.add_argument("output", nargs="?")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--metric")
    ap.add_argument("--complexity", type=float)
    ap.add_argument("--fixed", action="store_true")
    ap.add_argument("--hausd", type=float, default=0.01)
    ap.add_argument("--angle", type=float, default=45.0)
    ap.add_argument("--bl-marker")
    ap.add_argument("--bl-h0", type=float)
    ap.add_argument("--analytic", choices=["bump2d", "bump3d"])
    a = ap.parse_args()
    if a.selftest:
        selftest()
        return
    inp, out = read_su2(a.input), read_su2(a.output)
    res = {}
    res.update(check_validity(out))
    res.update(check_markers(inp, out))
    if a.metric:
        M, precision = metric_of(inp, a.metric)
        if a.complexity:
            res.update(check_complexity(inp, out, a.complexity, M, precision, bl=a.bl_marker is not None))
        res.update(check_edges(inp, out, M, interior_only=a.fixed))
    res.update(check_geometry(inp, out, a.fixed, a.hausd, analytic_references(a.analytic) if a.analytic else None))
    res.update(check_corners(inp, out, a.angle))
    if a.bl_marker:
        res.update(check_bl(out, {a.bl_marker: a.bl_h0}))
    print(json.dumps(res, indent=1))
    raise SystemExit(any(v[0] == "FAIL" for v in res.values()))


if __name__ == "__main__":
    main()
