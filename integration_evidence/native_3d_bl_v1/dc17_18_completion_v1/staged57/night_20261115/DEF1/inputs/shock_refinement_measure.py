"""Read-only shock-refinement measure for ONERA M6 Euler adaptation cycles.

Usage: python3 shock_refinement_measure.py mesh_0.su2 restart_0.dat [mesh_1.su2 restart_1.dat ...]
  Pair mesh N with restart_flow_adap_0000N.dat (the solution and metric written at the end of cycle N on mesh N).
  The metric in restart N is computed from solution N and is the request for mesh N+1.

Prints, per cycle (one JSON line each):
  A. the reviewer's measure (shock_metric_check.py), reproduced with the same selection;
  B. a direction-aware edge measure: for the same sonic-crossing edges, the edge extent along the in-surface
     Mach-gradient direction n (h_n = |dx . n|), and the metric sizes along n and along the wall normal;
  C. Cp station cuts (eta = y / b at 0.20, 0.44, 0.65, 0.80, 0.90; b = 1.1963 m): on the upper surface between
     15 % and 95 % local chord, the largest Cp rise between consecutive cut points is the shock; the shock width is
     the x distance over which Cp rises from 10 % to 90 % of the rise between the Cp minimum upstream of the shock
     and the Cp maximum within 15 % chord downstream of it, divided by the local chord; also the number of cut
     points inside that 10-90 % interval.
No production code, no solver; numpy only.
"""
import json
import struct
import sys

import numpy as np

ETAS = (0.20, 0.44, 0.65, 0.80, 0.90)
SEMI_SPAN = 1.1963  # ONERA M6 semi-span b [m]; the experimental Cp stations are y = eta * b


def read_restart(path):
    data = open(path, "rb").read()
    header = struct.unpack("5i", data[:20])
    assert header[0] == 535532, header
    nvar, npoint = header[1], header[2]
    names = [data[20 + 33 * i:20 + 33 * (i + 1)].split(b"\0")[0].decode() for i in range(nvar)]
    offset = 20 + 33 * nvar
    assert len(data) - offset >= 8 * nvar * npoint
    values = np.frombuffer(data, np.float64, nvar * npoint, offset).reshape(npoint, nvar)
    return {name: values[:, i] for i, name in enumerate(names)}, npoint


def read_mesh(path):
    """Wing triangles and all point coordinates of an SU2 ASCII mesh."""
    lines = open(path).read().splitlines()
    tag, wing, coords = None, None, None
    i = 0
    while i < len(lines):
        s = lines[i].strip()
        if s.startswith("NPOIN="):
            n = int(s.split("=")[1].split()[0])
            coords = np.array([[float(v) for v in lines[i + 1 + k].split()[:3]] for k in range(n)])
            i += n
        elif s.startswith("MARKER_TAG="):
            tag = s.split("=")[1].strip()
        elif s.startswith("MARKER_ELEMS=") and tag == "WING3D":
            n = int(s.split("=")[1])
            rows = [lines[i + 1 + k].split() for k in range(n)]
            assert all(r[0] == "5" for r in rows), "WING3D must be triangles"
            wing = np.array([[int(v) for v in r[1:4]] for r in rows])
            i += n
        i += 1
    return wing, coords


def metric_tensor(f, points):
    m = np.empty((len(points), 3, 3))
    for (a, b), name in {(0, 0): "XX", (0, 1): "XY", (0, 2): "XZ", (1, 1): "YY", (1, 2): "YZ", (2, 2): "ZZ"}.items():
        m[:, a, b] = m[:, b, a] = f["Metric_" + name][points]
    return m


def chord_coordinate(x, y, wing_points, X):
    """Local chord coordinate without the band-centre bias: LE/TE from the extreme point of each band, placed at that
    point's own y (not at the band centre)."""
    yw, xw = X[wing_points, 1], X[wing_points, 0]
    edges = np.linspace(0.0, yw.max(), 81)
    le, te = [], []
    for k in range(80):
        sel = (yw >= edges[k]) & (yw <= edges[k + 1])
        if sel.any():
            j = np.argmin(np.where(sel, xw, np.inf)); le.append((yw[j], xw[j]))
            j = np.argmax(np.where(sel, xw, -np.inf)); te.append((yw[j], xw[j]))
    le, te = np.array(sorted(le)), np.array(sorted(te))
    xle = np.interp(y, le[:, 0], le[:, 1]); xte = np.interp(y, te[:, 0], te[:, 1])
    return (x - xle) / np.maximum(xte - xle, 1e-9), xte - xle


def reviewer_chord(X, wing_points):
    """The band-centre construction of shock_metric_check.py (for the reproduction)."""
    pts = wing_points; y = X[pts, 1]; bands = np.linspace(0, y.max(), 41)
    centres = (bands[:-1] + bands[1:]) / 2
    mins = [X[pts[(y >= bands[k]) & (y < bands[k + 1])], 0].min() if ((y >= bands[k]) & (y < bands[k + 1])).any()
            else np.nan for k in range(40)]
    maxs = [X[pts[(y >= bands[k]) & (y < bands[k + 1])], 0].max() if ((y >= bands[k]) & (y < bands[k + 1])).any()
            else np.nan for k in range(40)]
    xle = np.interp(X[:, 1], centres, mins); xte = np.interp(X[:, 1], centres, maxs)
    return (X[:, 0] - xle) / np.maximum(xte - xle, 1e-9)


def surface_gradients(F, X, values):
    """Area-weighted in-surface P1 gradient of a point field, and area-weighted unit normals, per point."""
    a, b, c = X[F[:, 0]], X[F[:, 1]], X[F[:, 2]]
    normal = np.cross(b - a, c - a); area2 = np.linalg.norm(normal, axis=1); unit = normal / area2[:, None]
    grad = np.zeros((len(F), 3))
    for (i, j, k) in ((0, 1, 2), (1, 2, 0), (2, 0, 1)):
        edge = X[F[:, k]] - X[F[:, j]]  # opposite edge of vertex i
        grad += values[F[:, i]][:, None] * np.cross(unit, edge) / area2[:, None]
    point_grad = np.zeros_like(X); point_normal = np.zeros_like(X); weight = np.zeros(len(X))
    for col in range(3):
        for r in range(3):
            np.add.at(point_grad[:, r], F[:, col], grad[:, r] * area2)
            np.add.at(point_normal[:, r], F[:, col], normal[:, r])
        np.add.at(weight, F[:, col], area2)
    point_grad /= np.maximum(weight, 1e-300)[:, None]
    point_normal /= np.maximum(np.linalg.norm(point_normal, axis=1), 1e-300)[:, None]
    return point_grad, point_normal, grad


def edges_of(F):
    e = np.sort(np.concatenate([F[:, [0, 1]], F[:, [1, 2]], F[:, [2, 0]]]), axis=1)
    return np.unique(e, axis=0)


def directional_size(m, d):
    return 1.0 / np.sqrt(np.einsum("ni,nij,nj->n", d, m, d))


def station_cut(F, X, cp, y0, xc_of, chord_of):
    """Upper-surface Cp along the cut y = y0 (segment end points of the triangles crossed)."""
    ys = X[F, 1]
    crossed = (ys.min(axis=1) < y0) & (ys.max(axis=1) > y0)
    pts = []
    for tri in F[crossed]:
        for p, q in ((tri[0], tri[1]), (tri[1], tri[2]), (tri[2], tri[0])):
            yp, yq = X[p, 1], X[q, 1]
            if (yp - y0) * (yq - y0) < 0:
                t = (y0 - yp) / (yq - yp)
                pts.append((X[p, 0] + t * (X[q, 0] - X[p, 0]), X[p, 2] + t * (X[q, 2] - X[p, 2]),
                            cp[p] + t * (cp[q] - cp[p])))
    if not pts:
        return None
    pts = np.unique(np.round(np.array(pts), 12), axis=0)
    chord = float(chord_of(y0)); xle = float(xc_of(y0))
    xc = (pts[:, 0] - xle) / chord
    upper = pts[(pts[:, 1] > 0) & (xc > 0.15) & (xc < 0.95)]
    upper = upper[np.argsort(upper[:, 0])]
    if len(upper) < 6:
        return None
    x, c = upper[:, 0], upper[:, 2]
    rise = np.diff(c)
    k = int(np.argmax(rise))
    lo = int(np.argmin(c[:k + 1]))
    window = (x >= x[k]) & (x <= x[k] + 0.15 * chord)
    hi = int(np.flatnonzero(window)[np.argmax(c[window])])
    jump = c[hi] - c[lo]
    if jump <= 0:
        return None
    seg_x, seg_c = x[lo:hi + 1], np.maximum.accumulate(c[lo:hi + 1])
    x10 = np.interp(c[lo] + 0.1 * jump, seg_c, seg_x); x90 = np.interp(c[lo] + 0.9 * jump, seg_c, seg_x)
    inside = int(((x >= x10) & (x <= x90)).sum())
    # a lambda shock gives a second strong rise; report it so a two-shock 10-90 % width is recognised
    others = np.flatnonzero((rise > 0.5 * rise[k]) & (np.abs(x[:-1] - x[k]) > 0.05 * chord))
    second = float((x[others[np.argmax(rise[others])]] - xle) / chord) if len(others) else None
    return {"second_strong_rise_xc": round(second, 4) if second is not None else None,"eta_y": round(y0, 4), "shock_xc": round(float((x[k] - xle) / chord), 4),
            "cp_jump": round(float(jump), 4), "width_10_90_over_chord": round(float((x90 - x10) / chord), 5),
            "cut_points_in_10_90": inside, "cut_points_upper_15_95": int(len(x)),
            "max_rise_one_segment": round(float(rise[k]), 4)}


def measure(mesh_path, restart_path):
    f, npoint = read_restart(restart_path)
    F, coords = read_mesh(mesh_path)
    X = np.stack([f["x"], f["y"], f["z"]], axis=1)
    coordinate_error = float(np.abs(coords - X).max())
    M, cp = f["Mach"], f["Pressure_Coefficient"]
    wing_points = np.unique(F)
    E = edges_of(F)
    length = np.linalg.norm(X[E[:, 0]] - X[E[:, 1]], axis=1); jumpM = np.abs(M[E[:, 0]] - M[E[:, 1]])
    out = {"restart": restart_path.split("/")[-1], "points": int(npoint), "wing_triangles": int(len(F)),
           "restart_vs_mesh_coordinate_max_error": coordinate_error}

    # A. Reproduction of shock_metric_check.py (same selection).
    xc_r = reviewer_chord(X, wing_points)
    up = (X[E[:, 0], 2] > 0) & (X[E[:, 1], 2] > 0)
    mid = np.minimum(xc_r[E[:, 0]], xc_r[E[:, 1]]) > 0.15
    hiM, loM = np.maximum(M[E[:, 0]], M[E[:, 1]]), np.minimum(M[E[:, 0]], M[E[:, 1]])
    shock = up & mid & (hiM > 1.05) & (loM < 1.0)
    m_all = metric_tensor(f, np.arange(npoint))
    eig = np.linalg.eigvalsh(m_all); hmin_all = 1 / np.sqrt(np.clip(eig[:, -1], 1e-300, None))
    hmax_all = 1 / np.sqrt(np.clip(eig[:, 0], 1e-300, None))
    sp = np.unique(E[shock].ravel())
    le = np.unique(E[np.minimum(xc_r[E[:, 0]], xc_r[E[:, 1]]) < 0.02].ravel())
    out["A_reviewer"] = {
        "shock_edges": int(shock.sum()),
        "edge_length_median": float(np.median(length[shock])) if shock.any() else None,
        "edge_mach_jump_median": float(np.median(jumpM[shock])) if shock.any() else None,
        "edge_mach_jump_max": float(jumpM[shock].max()) if shock.any() else None,
        "metric_hmin_shock_median": float(np.median(hmin_all[sp])) if shock.any() else None,
        "metric_hmin_le_median": float(np.median(hmin_all[le])),
        "metric_hmax_shock_median": float(np.median(hmax_all[sp])) if shock.any() else None,
    }

    # B. Direction-aware version of the same edges, bias-free chord coordinate.
    xc, _ = chord_coordinate(X[:, 0], X[:, 1], wing_points, X)
    gradM, wall_normal, _ = surface_gradients(F, X, M)
    mid_b = np.minimum(xc[E[:, 0]], xc[E[:, 1]]) > 0.15
    # all sonic crossings (the 1.05 threshold of A keeps only the edges with the largest jumps, which on a
    # coarser mesh are the longer edges: a selection bias); the strict subset is counted for comparison
    shock_b = up & mid_b & (hiM > 1.0) & (loM < 1.0)
    strict_b = shock_b & (hiM > 1.05)
    g = gradM[E[:, 0]] + gradM[E[:, 1]]
    n = g / np.maximum(np.linalg.norm(g, axis=1), 1e-300)[:, None]
    dx = X[E[:, 1]] - X[E[:, 0]]
    h_n = np.abs(np.einsum("ni,ni->n", dx, n))
    sb = np.flatnonzero(shock_b)
    pts_b = np.unique(E[sb].ravel())
    # metric sizes at the edge end points along n (shock normal in the surface) and along the wall normal
    ends = np.concatenate([E[sb, 0], E[sb, 1]]); nn = np.concatenate([n[sb], n[sb]])
    m_ends = m_all[ends]
    req_n = directional_size(m_ends, nn) if len(sb) else np.array([])
    req_wall = directional_size(m_ends, wall_normal[ends]) if len(sb) else np.array([])
    # leading edge: chordwise in-surface direction (wall normal x spanwise), within 2 % chord
    le_b = wing_points[xc[wing_points] < 0.02]
    span = np.array([0.0, 1.0, 0.0]); chordwise = np.cross(span, wall_normal[le_b])
    chordwise /= np.maximum(np.linalg.norm(chordwise, axis=1), 1e-300)[:, None]
    out["B_direction_aware"] = {
        "shock_edges": int(len(sb)), "of_which_max_mach_above_1_05": int(strict_b.sum()),
        "shock_points": int(len(pts_b)), "wing_points": int(len(wing_points)),
        "edge_length_median": float(np.median(length[sb])) if len(sb) else None,
        "edge_extent_along_shock_normal_median": float(np.median(h_n[sb])) if len(sb) else None,
        "edge_extent_along_shock_normal_p10": float(np.percentile(h_n[sb], 10)) if len(sb) else None,
        "edge_mach_jump_median": float(np.median(jumpM[sb])) if len(sb) else None,
        "surface_grad_mach_magnitude_median": float(np.median(np.linalg.norm(gradM[pts_b], axis=1))) if len(sb) else None,
        "metric_h_along_shock_normal_median": float(np.median(req_n)) if len(sb) else None,
        "metric_h_along_wall_normal_median_at_shock": float(np.median(req_wall)) if len(sb) else None,
        "metric_hmin_any_direction_median_at_shock": float(np.median(hmin_all[pts_b])) if len(sb) else None,
        "metric_h_chordwise_median_at_le": float(np.median(directional_size(m_all[le_b], chordwise))),
        "metric_h_along_wall_normal_median_at_le": float(np.median(directional_size(m_all[le_b], wall_normal[le_b]))),
        "metric_hmin_any_direction_median_at_le": float(np.median(hmin_all[le_b])),
        "le_points": int(len(le_b)),
    }

    # C. Cp station cuts.
    ywing = X[wing_points, 1]; b = SEMI_SPAN
    yw, xw = X[wing_points, 1], X[wing_points, 0]
    edges_y = np.linspace(0.0, float(ywing.max()), 81); le_pts, te_pts = [], []
    for k in range(80):
        sel = (yw >= edges_y[k]) & (yw <= edges_y[k + 1])
        if sel.any():
            j = np.argmin(np.where(sel, xw, np.inf)); le_pts.append((yw[j], xw[j]))
            j = np.argmax(np.where(sel, xw, -np.inf)); te_pts.append((yw[j], xw[j]))
    le_pts, te_pts = np.array(sorted(le_pts)), np.array(sorted(te_pts))
    xle_of = lambda y0: np.interp(y0, le_pts[:, 0], le_pts[:, 1])
    chord_of = lambda y0: np.interp(y0, te_pts[:, 0], te_pts[:, 1]) - xle_of(y0)
    out["C_cp_stations"] = [station_cut(F, X, cp, eta * b, xle_of, chord_of) for eta in ETAS]
    out["semi_span_used"] = b
    out["wing_max_y"] = float(ywing.max())
    return out


if __name__ == "__main__":
    args = sys.argv[1:]
    assert args and len(args) % 2 == 0, __doc__
    for mesh_path, restart_path in zip(args[::2], args[1::2]):
        print(json.dumps(measure(mesh_path, restart_path)), flush=True)
