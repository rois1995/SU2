"""Independent readback of the selected, unapproved build237 inspection files."""
from pathlib import Path
import hashlib
import json
import sys
import time

import meshio
import numpy as np

attempt = Path(__file__).resolve().parent
folder = attempt / "inspection"
status = attempt / "inspection_reader_status.json"
started = time.time()


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


record = {"state": "running", "started_at": started,
          "reader": {"python": sys.version, "meshio": meshio.__version__,
                     "numpy": np.__version__}, "helper_sha256": sha(Path(__file__))}
status.write_text(json.dumps(record, indent=2) + "\n")
try:
    inputs = sorted(path for path in folder.iterdir() if path.is_file())
    before = {str(path): sha(path) for path in inputs}
    manifest = json.loads((folder / "inspect_manifest.json").read_text())
    assert not manifest["final_mesh_accepted"] and not manifest["partial"]
    assert manifest["full_mesh_measurements_complete"]
    summaries = {}
    full = None
    for name in ("rejected_mesh.vtu", "bad_cells.vtu", "cavities.vtu", "reference.vtu"):
        mesh = meshio.read(folder / name)
        for values in mesh.point_data.values():
            assert len(values) == len(mesh.points), (name, "PointData length")
        for values in mesh.cell_data.values():
            assert len(values) == len(mesh.cells), (name, "CellData blocks")
            assert all(len(value) == len(block.data)
                       for value, block in zip(values, mesh.cells)), (name, "CellData length")
        summaries[name] = {"points": len(mesh.points),
                           "cells": sum(len(block.data) for block in mesh.cells),
                           "point_arrays": sorted(mesh.point_data),
                           "cell_arrays": sorted(mesh.cell_data)}
        if name == "rejected_mesh.vtu":
            full = mesh
    assert full is not None
    assert len(full.points) == manifest["actual_full_mesh_points"]
    tetrahedra = full.cells_dict["tetra"]
    assert len(tetrahedra) == manifest["actual_full_mesh_cells"]
    assert summaries["bad_cells.vtu"]["cells"] == manifest["bad_cells_retained"]
    assert summaries["cavities.vtu"]["cells"] == manifest["cavity_cells_retained"]
    tensor = full.point_data["captured_nodal_metric"].reshape(-1, 3, 3)
    assert np.all(full.point_data["captured_nodal_metric_available"] == 1)
    assert np.isfinite(tensor).all() and np.array_equal(tensor, tensor.transpose(0, 2, 1))
    eigenvalues = np.linalg.eigvalsh(tensor)
    assert np.all(eigenvalues > 0)
    cells = {name: values["tetra"] for name, values in full.cell_data_dict.items()}
    assert np.all(cells["cache_valid"] == 1)
    for name in ("q", "J", "geometry_excess_l2", "geometry_bad", "metric_edge_min", "metric_edge_max"):
        assert np.isfinite(cells[name]).all(), name
    q_bad, j_bad, geometry_bad = cells["q"] < .2, cells["J"] < .05, cells["geometry_bad"] != 0
    counts = {"q_bad": int(q_bad.sum()), "j_bad": int(j_bad.sum()),
              "geometry_bad": int(geometry_bad.sum()),
              "shape_bad_union": int((q_bad | j_bad).sum()),
              "bad_union": int((q_bad | j_bad | geometry_bad).sum())}
    terminal = json.loads((attempt / "terminal_outcome_assessment.json").read_text())["final"]["totals"]
    assert all(counts[key] == terminal[key] for key in ("q_bad", "j_bad", "geometry_bad")), counts
    assert counts["bad_union"] == manifest["bad_cells_retained"]
    assert counts["shape_bad_union"] == 430
    assert np.all(cells["metric_edge_min"] > 0) and np.max(cells["metric_edge_max"]) <= 1.8
    assert len(np.unique(cells["global_cell_id"])) == len(tetrahedra)
    assert len(np.unique(full.point_data["global_node_id"])) == len(full.points)
    su2 = meshio.read(folder / "rejected_mesh.su2")
    assert np.array_equal(su2.points, full.points)
    assert np.array_equal(np.sort(su2.cells_dict["tetra"], axis=1), np.sort(tetrahedra, axis=1))
    assert before == {str(path): sha(path) for path in inputs}
    record.update(state="terminal", exit_code=0, all_reader_checks_passed=True,
                  seconds=time.time() - started, output_sha256=before,
                  files=summaries, measured_bad_counts=counts,
                  minimum_metric_eigenvalue=float(eigenvalues.min()),
                  maximum_metric_edge=float(cells["metric_edge_max"].max()),
                  su2_coordinates_and_tetrahedra_match=True,
                  scope="Independent meshio readback and array-length/tensor/count checks. No ParaView GUI qualification; mesh remains unapproved.")
except BaseException as error:
    record.update(state="terminal", exit_code=1, seconds=time.time() - started, error=repr(error))
    status.write_text(json.dumps(record, indent=2) + "\n")
    raise
status.write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps({key: value for key, value in record.items() if key not in ("output_sha256", "files")}))
