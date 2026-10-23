#!/usr/bin/env python3
"""DC-11 copy of dc7/to_vtu.py (usage: to_vtu.py [runs-dir] [png-tag]; reads dc11_*_final.mesh). Convert DC-7 *_final.mesh (nodes/cells/faces/h0 text) to VTU/VTP and render bad-cell PNGs.
Final meshes written by the post-review harness carry the native gates: section 'cell_gates' (per cell: metric q, J,
bad flag, first-child flag, column id = apex id or -1, cell class 0 outside columns / 1 first child / 2 other column
cell) and section 'long_edges' (every metric edge > 1.8: node ids, composed metric length (-1 = unmeasurable), class
0 interior / 1 wall / 2 seam / 3 other physical / 4 unmeasurable, inside-columns-only flag). These are written to the
VTU/VTP as *_metric fields and drive the PNGs. Older files without them fall back to Euclidean PROXIES: mean-ratio q
(regular tet=1), signed volume, min/max edge/h0; 'bad_proxy' = q<BAD_Q or vol<=0 (NOT the solver metric gates)."""
import sys, glob, os, numpy as np, vtk
from vtk.util import numpy_support as ns
BAD_Q = 0.1
HERE = os.path.dirname(os.path.abspath(__file__))
PAIRS = [(0,1),(0,2),(0,3),(1,2),(1,3),(2,3)]

def read(path):
    L = open(path).read().split("\n"); i = 0; ids = []; X = []; C = []; F = []; h0 = 1.0; G = {}; E = []
    while i < len(L):
        t = L[i].split(); i += 1
        if not t: continue
        if t[0] == "nodes":
            for _ in range(int(t[1])):
                r = L[i].split(); i += 1; ids.append(int(r[0])); X.append([float(v) for v in r[1:4]])
        elif t[0] == "cells":
            for _ in range(int(t[1])):
                r = L[i].split(); i += 1; C.append([int(v) for v in r])
        elif t[0] == "faces":
            for _ in range(int(t[1])):
                r = L[i].split(); i += 1; F.append([int(v) for v in r])
        elif t[0] == "h0": h0 = float(t[1])
        elif t[0] == "cell_gates":
            for _ in range(int(t[1])):
                r = L[i].split(); i += 1; G[int(r[0])] = [float(r[1]), float(r[2]), int(r[3]), int(r[4]), int(r[5]), int(r[6])]
        elif t[0] == "long_edges":
            for _ in range(int(t[1])):
                r = L[i].split(); i += 1; E.append([int(r[0]), int(r[1]), float(r[2]), int(r[3]), int(r[4])])
    idx = {n: k for k, n in enumerate(ids)}
    read.gates, read.edges = G, E
    return np.array(X), idx, np.array(C), np.array(F), h0

def build(path):
    X, idx, C, F, h0 = read(path)
    cid = C[:, 0]; T = np.array([[idx[n] for n in r[1:5]] for r in C])
    P = X[T]; e = P[:, 1:] - P[:, :1]
    vol = np.einsum("ij,ij->i", e[:, 0], np.cross(e[:, 1], e[:, 2])) / 6.0
    el = np.stack([np.linalg.norm(P[:, a] - P[:, b], axis=1) for a, b in PAIRS], 1)
    q = 12 * (3 * np.abs(vol)) ** (2 / 3) / (el ** 2).sum(1)
    q = np.where(vol > 0, q, -q)
    return X, idx, cid, T, vol, q, el, F, h0

def arr(name, a, dt=None):
    v = ns.numpy_to_vtk(np.ascontiguousarray(a), deep=1, array_type=dt); v.SetName(name); return v

def pts(X):
    p = vtk.vtkPoints(); p.SetData(ns.numpy_to_vtk(np.ascontiguousarray(X, float), deep=1)); return p

def write(obj, fn):
    w = vtk.vtkXMLUnstructuredGridWriter() if obj.IsA("vtkUnstructuredGrid") else vtk.vtkXMLPolyDataWriter()
    w.SetFileName(fn); w.SetInputData(obj); w.Write()

def convert(path):
    X, idx, cid, T, vol, q, el, F, h0 = build(path)
    base = os.path.splitext(os.path.basename(path))[0].replace("_final", "")
    out = os.path.join(os.path.dirname(path), "vtu"); os.makedirs(out, exist_ok=True)
    bad = ((q < BAD_Q) | (vol <= 0)).astype(np.uint8)
    g = vtk.vtkUnstructuredGrid(); g.SetPoints(pts(X))
    ca = vtk.vtkCellArray()
    for t in T: ca.InsertNextCell(4, [int(v) for v in t])
    g.SetCells(vtk.VTK_TETRA, ca)
    d = g.GetCellData()
    d.AddArray(arr("cell_id", cid)); d.AddArray(arr("q_meanratio_proxy", q)); d.AddArray(arr("volume", vol))
    d.AddArray(arr("volume_sign", np.sign(vol).astype(np.int8))); d.AddArray(arr("bad_proxy", bad))
    d.AddArray(arr("edge_min_over_h0", el.min(1) / h0)); d.AddArray(arr("edge_max_over_h0", el.max(1) / h0))
    G, E = read.gates, read.edges
    metric = bool(G) and all(int(c) in G for c in cid)
    if metric:
        M = np.array([G[int(c)] for c in cid])
        d.AddArray(arr("q_metric", M[:, 0])); d.AddArray(arr("J_metric", M[:, 1])); d.AddArray(arr("bad_metric", M[:, 2].astype(np.uint8)))
        d.AddArray(arr("first_child", M[:, 3].astype(np.uint8))); d.AddArray(arr("column_id", M[:, 4].astype(np.int64)))
        d.AddArray(arr("cell_class", M[:, 5].astype(np.int8)))
        bad = M[:, 2].astype(np.uint8); q = M[:, 0]
    write(g, f"{out}/{base}.vtu")
    # boundary faces
    pd = vtk.vtkPolyData(); pd.SetPoints(pts(X)); fa = vtk.vtkCellArray()
    for r in F: fa.InsertNextCell(3, [idx[int(v)] for v in r[1:4]])
    pd.SetPolys(fa)
    pd.GetCellData().AddArray(arr("owner_cell", F[:, 0])); pd.GetCellData().AddArray(arr("marker", F[:, 4]))
    pd.GetCellData().AddArray(arr("is_wall", F[:, 5]))
    write(pd, f"{out}/{base}_faces.vtp")
    # long edges: file has no flag; proxy = unique edges with length > 1.5*h0 (lines)
    ed = {}
    for t, l in zip(T, el):
        for (a, b), x in zip(PAIRS, l): ed[tuple(sorted((int(t[a]), int(t[b]))))] = x
    long = [(k, v) for k, v in ed.items() if v > 1.5 * h0]
    ld = vtk.vtkPolyData(); ld.SetPoints(pts(X)); la = vtk.vtkCellArray()
    for (a, b), _ in long: la.InsertNextCell(2, [a, b])
    ld.SetLines(la); ld.GetCellData().AddArray(arr("length_over_h0", np.array([v / h0 for _, v in long])))
    write(ld, f"{out}/{base}_longedges_proxy.vtp")
    if E:
        md = vtk.vtkPolyData(); md.SetPoints(pts(X)); ma = vtk.vtkCellArray()
        for r in E: ma.InsertNextCell(2, [idx[r[0]], idx[r[1]]])
        md.SetLines(ma); A = np.array(E)
        md.GetCellData().AddArray(arr("length_metric", A[:, 2])); md.GetCellData().AddArray(arr("class", A[:, 3].astype(np.int8)))
        md.GetCellData().AddArray(arr("inside_columns_only", A[:, 4].astype(np.int8)))
        write(md, f"{out}/{base}_longedges_metric.vtp")
    return base, g, pd, bad, q, (len(E) if metric else len(long)), len(T), metric

def render(base, g, pd, bad, outpng, metric=False):
    if not bad.any(): return "no bad cells"
    qname, bname = ("q_metric", "bad_metric") if metric else ("q_meanratio_proxy", "bad_proxy")
    th = vtk.vtkThreshold(); th.SetInputData(g); th.SetInputArrayToProcess(0, 0, 0, 1, bname)
    th.SetLowerThreshold(0.5); th.SetUpperThreshold(1.5); th.Update()
    bg = th.GetOutput()
    wall = vtk.vtkThreshold(); wall.SetInputData(pd); wall.SetInputArrayToProcess(0, 0, 0, 1, "is_wall")
    wall.SetLowerThreshold(0.5); wall.SetUpperThreshold(1.5); wall.Update()
    ren = vtk.vtkRenderer(); ren.SetBackground(1, 1, 1)
    gf = vtk.vtkGeometryFilter(); gf.SetInputData(bg); gf.Update()
    qa = bg.GetCellData().GetArray(qname); rng = qa.GetRange()
    m = vtk.vtkDataSetMapper(); m.SetInputData(bg); m.SetScalarModeToUseCellFieldData(); m.SelectColorArray(qname)
    m.SetScalarRange(min(rng[0], 0), max(rng[1], 1e-3)); a = vtk.vtkActor(); a.SetMapper(m)
    a.GetProperty().EdgeVisibilityOn(); ren.AddActor(a)
    wm = vtk.vtkDataSetMapper(); wm.SetInputConnection(wall.GetOutputPort()); wm.ScalarVisibilityOff()
    wa = vtk.vtkActor(); wa.SetMapper(wm); wa.GetProperty().SetColor(.85, .85, .85); wa.GetProperty().SetOpacity(.6); ren.AddActor(wa)
    sb = vtk.vtkScalarBarActor(); sb.SetLookupTable(m.GetLookupTable()); sb.SetTitle("q (metric)" if metric else "q (proxy)"); ren.AddActor2D(sb)
    b = bg.GetBounds(); c = [(b[0]+b[1])/2, (b[2]+b[3])/2, (b[4]+b[5])/2]
    r = max(b[1]-b[0], b[3]-b[2], b[5]-b[4], 1e-6) * 2.5
    cam = ren.GetActiveCamera(); cam.SetFocalPoint(*c); cam.SetPosition(c[0]+r, c[1]-r, c[2]+r*0.8); cam.SetViewUp(0, 0, 1)
    ren.ResetCamera(b)
    rw = vtk.vtkRenderWindow(); rw.SetOffScreenRendering(1); rw.SetSize(1000, 800); rw.AddRenderer(ren); rw.Render()
    wf = vtk.vtkWindowToImageFilter(); wf.SetInput(rw); wf.Update()
    pw = vtk.vtkPNGWriter(); pw.SetFileName(outpng); pw.SetInputConnection(wf.GetOutputPort()); pw.Write()
    return "ok"

if __name__ == "__main__":
    root = sys.argv[1] if len(sys.argv) > 1 else HERE + "/runs"
    tag = sys.argv[2] if len(sys.argv) > 2 else ""
    files = sorted(glob.glob(root + "/dc11_*_final.mesh"))
    os.makedirs(HERE + "/png", exist_ok=True)
    for f in files:
        base, g, pd, bad, q, nl, n, metric = convert(f)
        try: st = render(base, g, pd, bad, f"{HERE}/png/{tag}{base}.png", metric)
        except Exception as ex: st = "render failed: %r" % ex
        kind = "metric" if metric else "proxy"
        print(f"{tag}{base}: cells={n} bad_{kind}={int(bad.sum())} qmin={q.min():.3g} longedges_{kind}={nl} png={st}")
