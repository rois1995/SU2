"""Locate actual residuals and measure existing star sizes before choosing a closure hypothesis."""
from pathlib import Path
import hashlib, json, time
import meshio
import numpy as np

ROOT = Path(__file__).resolve().parent
RUN = ROOT / 'm6_runs/euler_build227_witness_real_cfd50k'


def sha(path):
    return hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()


def main():
    started = time.time()
    output = RUN / 'shape_inspection/residual_location.json'
    assert not output.exists()
    inputs = [RUN / 'inspection/rejected_mesh.su2', RUN / 'inspection/rejected_mesh.vtu',
              RUN / 'shape_inspection/bad_cells.vtu', RUN / 'shape_inspection/assessment.json', Path(__file__)]
    before = {str(p.resolve()): sha(p) for p in inputs}
    su2 = meshio.read(inputs[0]); full = meshio.read(inputs[1]); bad = meshio.read(inputs[2])
    assert np.array_equal(su2.points, full.points) and np.array_equal(bad.points, full.points)
    tetra = full.cells[0].data; cells = bad.cells[0].data
    ids = bad.cell_data['global_cell_id'][0]
    q = bad.cell_data['q_centroid_diagnostic'][0]; j = bad.cell_data['J_centroid_diagnostic'][0]
    assert ((q < .2) | (j < .05)).all()
    triangles = su2.cells[1].data; tags = su2.cell_data['su2:tag'][1]
    assert su2.cells[1].type == 'triangle' and set(tags) == {1, 2, 3}
    physical = {tuple(sorted(face)): int(tag) for face, tag in zip(triangles, tags)}
    assert len(physical) == len(triangles)
    boundary_nodes = np.zeros(len(full.points), dtype=bool); boundary_nodes[triangles.ravel()] = True
    wing_nodes = np.zeros(len(full.points), dtype=bool); wing_nodes[triangles[tags == 3].ravel()] = True
    valence = np.bincount(tetra.ravel(), minlength=len(full.points))
    kinds = []
    for cell in cells:
        face_tags = [physical.get(tuple(sorted(np.delete(cell, slot))), 0) for slot in range(4)]
        kinds.append('wing_face' if 3 in face_tags else 'other_physical_face' if any(face_tags)
                     else 'interior_touching_wing' if wing_nodes[cell].any()
                     else 'interior_touching_other_boundary' if boundary_nodes[cell].any() else 'interior_free')
    kinds = np.array(kinds); centers = full.points[cells].mean(axis=1)
    result = dict(accepted=False, geometry_status='Unknown', bad_union=len(ids), classifications={}, candidates=[])
    for kind in sorted(set(kinds)):
        indices = np.flatnonzero(kinds == kind)
        result['classifications'][kind] = dict(cells=len(indices), q_bad=int((q[indices] < .2).sum()),
            J_bad=int((j[indices] < .05).sum()), centroid_min=centers[indices].min(axis=0).tolist(),
            centroid_max=centers[indices].max(axis=0).tolist(),
            cells_with_any_star_over64=int((valence[cells[indices]].max(axis=1) > 64).sum()))
        # Deterministic worst normalized miss, with two spatially separated actual controls per class.
        order = sorted(indices, key=lambda i: (min(q[i] / .2, j[i] / .05), int(ids[i])))
        selected = []
        for i in order:
            if selected and min(np.linalg.norm(centers[i] - centers[k]) for k in selected) < .05:
                continue
            support = np.flatnonzero(np.any(np.isin(tetra, cells[i]), axis=1))
            result['candidates'].append(dict(kind=kind, id=int(ids[i]), q=float(q[i]), J=float(j[i]),
                centroid=centers[i].tolist(), vertices=full.point_data['global_node_id'][cells[i]].tolist(),
                star_sizes=valence[cells[i]].tolist(), context_cells=len(support),
                context_within128=bool(len(support) <= 128),
                support_cells=full.cell_data['global_cell_id'][0][support].tolist()))
            selected.append(i)
            if len(selected) == 2:
                break
    after = {name: sha(Path(name)) for name in before}; assert before == after
    result.update(input_hashes_before=before, input_hashes_after=after, seconds=time.time()-started,
        scope='Actual centroid-shape residual locations and complete existing single-vertex-star unions; no feature/geometry classification inferred')
    output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ('bad_union', 'classifications', 'seconds')}))


if __name__ == '__main__':
    main()
