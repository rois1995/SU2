#!/usr/bin/env python3
"""DC-13 summary table: every dc13_*.json / volonly13_*.json in the given directories (one row per case/step)."""
import json, sys, glob, os, statistics

def wall_aspect(mesh):
    """median Euclidean wall-edge length / h0 of a *_final.mesh (the near-wall anisotropy the construction must reach)"""
    try:
        L = open(mesh).read().split("\n"); i = 0; X = {}; E = set(); h0 = None
        while i < len(L):
            t = L[i].split()
            if t and t[0] == "nodes":
                for k in range(int(t[1])): a = L[i + 1 + k].split(); X[int(a[0])] = tuple(map(float, a[1:4]))
                i += int(t[1]) + 1; continue
            if t and t[0] == "faces":
                for k in range(int(t[1])):
                    a = L[i + 1 + k].split()
                    if a[5] == "1":
                        v = list(map(int, a[1:4]))
                        for j in range(3): E.add(tuple(sorted((v[j], v[(j + 1) % 3]))))
                i += int(t[1]) + 1; continue
            if t and t[0] == "h0": h0 = float(t[1])
            i += 1
        d = [sum((X[a][c] - X[b][c]) ** 2 for c in range(3)) ** .5 for a, b in E]
        return statistics.median(d) / h0 if d and h0 else float("nan")
    except Exception:
        return float("nan")

def row(path):
    d = json.load(open(path))
    name = os.path.basename(path)[:-5]
    if name.startswith("volonly13_"):
        f = d.get("final", {})
        return (f"{name:42s} vol-only  complete={f.get('complete')} cells={f.get('cells')} bad={f.get('bad_shape')} long={f.get('long_edges')}"
                f" q={f.get('q_min', 0):.3g} J={f.get('J_min', 0):.3g} stop={d.get('stop')} rounds={d.get('rounds')} t={d.get('seconds', 0):.0f}s"
                f" reason={d.get('reason', '')[:120]}")
    f = d["final"]; r = d.get("reaudit", {}); L = d.get("layers_final", {}); V = d.get("V", {})
    out = (f"{name:42s} complete={f['complete']}/{r.get('complete')} cells={f['cells']} bad={f['bad_shape']} long={f['long_edges']}"
           f" h={f['height_failures']} unmeas={f['unmeasurable_edges']} q={f['q_min']:.3g} J={f['J_min']:.3g}"
           f" A2={L.get('fraction', 0):.2f} audit_viol={d.get('first_row_audit', {}).get('violations')}"
           f" V={V.get('stage', {}).get('outcome')} t={d.get('total_seconds', 0):.0f}s"
           f" wall_edge/h0={wall_aspect(path[:-5] + '_final.mesh'):.1f}")
    if "change" in d:
        c = d["change"]; h = d.get("halve", {})
        out += (f" | halved={h.get('split')} refused={h.get('refused')} cells {c['cells_before']}->{c['cells_after']}"
                f" unchanged={c['cells_unchanged']} vertices kept={c['vertices_kept']}/{c['vertices_before']}"
                f" max_disp_kept={c['max_kept_displacement']:.3g}")
    return out

if __name__ == "__main__":
    for d in sys.argv[1:]:
        for p in sorted(glob.glob(os.path.join(d, "dc13_*.json")) + glob.glob(os.path.join(d, "volonly13_*.json"))):
            try: print(row(p))
            except Exception as e: print(p, "unreadable:", e)
