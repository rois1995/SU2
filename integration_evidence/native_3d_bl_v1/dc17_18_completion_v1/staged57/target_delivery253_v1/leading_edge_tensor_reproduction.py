"""Compare captured nodal target directions with actual leading-edge triangle extents."""
from pathlib import Path
import hashlib
import json
import time

import meshio
import numpy as np


HERE = Path(__file__).resolve().parent
RUN = HERE.parents[0] / "m6_runs/euler_build247_canonical_mach_only_real_cfd50k"
VTU = RUN / "inspection/rejected_mesh.vtu"
SU2 = RUN / "inspection/rejected_mesh.su2"
SOURCE_MESH = RUN / "m6.su2"
PREVIOUS = HERE / "leading_edge_diagnosis.json"


def sha(path):
    return hashlib.file_digest(path.open("rb"), "sha256").hexdigest()


def quantiles(a):
    a = np.asarray(a, dtype=float)
    assert a.size and np.isfinite(a).all()
    return dict(zip(("min", "p10", "median", "p90", "max"),
                    map(float, np.quantile(a, [0, .1, .5, .9, 1]))))


def main():
    started = time.monotonic()
    before = {str(p): sha(p) for p in (VTU, SU2, SOURCE_MESH, PREVIOUS)}
    volume = meshio.read(VTU)
    surface = meshio.read(SU2)
    assert np.array_equal(volume.points, surface.points)
    assert len(volume.cells) == 1 and volume.cells[0].type == "tetra"
    assert len(surface.cells) == 2 and surface.cells[1].type == "triangle"
    tet = volume.cells[0].data
    su2_tet = surface.cells[0].data
    assert np.array_equal(tet, su2_tet), "volume tetrahedra differ between files"
    points = volume.points
    wing_faces_all = surface.cells[1].data
    tags = surface.cell_data["su2:tag"][1].astype(int)
    assert set(np.unique(tags)) == {1, 2, 3}
    output_marker_names = [line.split("=", 1)[1].strip() for line in SU2.read_text().splitlines()
                           if line.startswith("MARKER_TAG")]
    source_marker_names = [line.split("=", 1)[1].strip() for line in SOURCE_MESH.read_text().splitlines()
                           if line.startswith("MARKER_TAG")]
    assert output_marker_names == ["Marker_0", "Marker_1", "Marker_2"], output_marker_names
    assert source_marker_names == ["FARFIELD", "SYMMETRY", "WING3D"], source_marker_names
    marker_names = source_marker_names
    wing_faces = wing_faces_all[tags == 3]
    assert len(wing_faces) == 53500, len(wing_faces)
    assert "captured_nodal_metric" in volume.point_data
    assert "captured_nodal_metric_available" in volume.point_data
    available = volume.point_data["captured_nodal_metric_available"].reshape(-1)
    tensors = volume.point_data["captured_nodal_metric"].reshape(-1, 3, 3)
    assert np.all(available == 1) and tensors.shape == (len(points), 3, 3)
    assert np.isfinite(tensors).all()
    eigenvalues = np.linalg.eigvalsh(tensors)
    assert np.all(eigenvalues > 0)
    names = marker_names

    wing_points = points[np.unique(wing_faces)]
    xi = wing_points[:, 0] - wing_points[:, 1] / np.sqrt(3)
    x0 = float(xi.min())
    station_values = [0.2, 0.4, 0.6, 0.8, 1.0]
    old_report = json.loads(PREVIOUS.read_text())
    expected_counts = [row["current_triangles"] for row in old_report["stations"]]
    station_reports = []
    for y0, expected in zip(station_values, expected_counts):
        point_band = wing_points[np.abs(wing_points[:, 1] - y0) < .025]
        band_xi = point_band[:, 0] - point_band[:, 1] / np.sqrt(3)
        chord = float(np.ptp(band_xi))
        x_le = float(band_xi.min())
        centers = points[wing_faces].mean(axis=1)
        center_xi = centers[:, 0] - centers[:, 1] / np.sqrt(3)
        mask = (np.abs(centers[:, 1] - y0) < .025) & ((center_xi - x_le) / chord < .03)
        faces = wing_faces[mask]
        assert len(faces) == expected, (y0, len(faces), expected)
        xyz = points[faces]
        edge1, edge2 = xyz[:, 1] - xyz[:, 0], xyz[:, 2] - xyz[:, 0]
        normal = np.cross(edge1, edge2)
        normal /= np.linalg.norm(normal, axis=1)[:, None]
        span = np.broadcast_to(np.array([1 / np.sqrt(3), 1., 0.]), normal.shape).copy()
        span -= np.einsum("ni,ni->n", span, normal)[:, None] * normal
        span_norm = np.linalg.norm(span, axis=1)
        assert np.all(span_norm > 1e-12)
        span /= span_norm[:, None]
        nose = np.cross(normal, span)
        tri_metric = tensors[faces].mean(axis=1)
        h_span = 1 / np.sqrt(np.einsum("ni,nij,nj->n", span, tri_metric, span))
        h_nose = 1 / np.sqrt(np.einsum("ni,nij,nj->n", nose, tri_metric, nose))
        mss = np.einsum("ni,nij,nj->n", span, tri_metric, span)
        msn = np.einsum("ni,nij,nj->n", span, tri_metric, nose)
        mnn = np.einsum("ni,nij,nj->n", nose, tri_metric, nose)
        eig2, vec2 = np.linalg.eigh(np.stack((np.stack((mss, msn), axis=-1),
                                               np.stack((msn, mnn), axis=-1)), axis=-2))
        long_axis_span_alignment = np.abs(vec2[:, 0, 0])
        ext_span = np.ptp(np.einsum("nvi,ni->nv", xyz, span), axis=1)
        ext_nose = np.ptp(np.einsum("nvi,ni->nv", xyz, nose), axis=1)
        station_reports.append({
            "y": y0, "first_3pct_chord_faces": int(len(faces)), "chord_definition": chord,
            "requested_h_span": quantiles(h_span), "requested_h_nose": quantiles(h_nose),
            "h_span_over_h_nose": quantiles(h_span / h_nose),
            "long_axis_span_alignment_abs_cos": quantiles(long_axis_span_alignment),
            "triangle_span_extent_over_h_span": quantiles(ext_span / h_span),
            "triangle_nose_extent_over_h_nose": quantiles(ext_nose / h_nose),
            "smallest_projected_eigenvalue": quantiles(eig2[:, 0]),
            "largest_projected_eigenvalue": quantiles(eig2[:, 1]),
        })

    after = {str(p): sha(p) for p in (VTU, SU2, SOURCE_MESH, PREVIOUS)}
    assert before == after
    result = {
        "scope": "Captured nodal target tensors averaged over each actual rejected-mesh wing triangle, restricted to the triangle tangent plane. This is not production quadrature, an edge certification, nor proof of pointwise target drift or causality.",
        "inputs": {p: {"sha256": before[p], "bytes": Path(p).stat().st_size} for p in before},
        "script_sha256": sha(Path(__file__).resolve()),
        "reader": {"meshio": meshio.__version__, "numpy": np.__version__},
        "mesh": {"points": int(len(points)), "tetrahedra": int(len(tet)),
                 "marker_tags": dict(zip(names, map(int, np.bincount(tags, minlength=4)[1:4]))),
                 "actual_wing_marker_tag": 3, "actual_wing_faces": int(len(wing_faces)),
                 "marker_name_order": marker_names,
                 "diagnostic_tag_to_source_marker": dict(zip(output_marker_names, marker_names)),
                 "vtu_su2_coordinates_and_tetrahedra_identical": True,
                 "captured_metric_available_points": int(available.sum()),
                 "captured_metric_minimum_eigenvalue": float(eigenvalues.min())},
        "stations": station_reports,
        "method": "At each y station use |y-y0|<0.025 and first 3% of the local swept chord xi=x-y/sqrt(3), matching analyze_leading_edge.py. Project span=(1/sqrt(3),1,0) onto each actual face; nose=unit_normal cross projected span. Mean the three nodal tensors, compute directional h=1/sqrt(d^T M d), tangent-plane principal axes and actual triangle projected extents.",
        "hashes_before_after_equal": True,
        "execution": {"elapsed_seconds": time.monotonic() - started,
                      "requested_niceness": 19, "single_thread_libraries": True,
                      "timeout_seconds": 30},
    }
    output = HERE / "leading_edge_tensor_reproduction.json"
    output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"output": str(output), "mesh": result["mesh"], "stations": station_reports}, indent=2))


if __name__ == "__main__":
    main()
