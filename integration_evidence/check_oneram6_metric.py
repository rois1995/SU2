"""Audit matched frozen ONERA M6 RANS runs; requires NumPy.

Usage: python3 integration_evidence/check_oneram6_metric.py BUILD_DIRECTORY
Reads metric-isolation/oneram6-rans-matched-mpi{1,2,4}/fields.csv and run.cfg.
Checks mesh/restart coordinates, six retained solution fields, SPD, complexity
and MPI tensors. A real restart supplies no exact Hessian accuracy reference.
Writes metric-isolation/oneram6-rans-validation.json even if a metric gate fails.
"""
from pathlib import Path
import csv, hashlib, json, re, sys
import numpy as np

root = Path(sys.argv[1]).resolve() / 'metric-isolation'
cfg = (root / 'oneram6-rans-matched-mpi1/run.cfg').read_text()
method = re.search(r'^NUM_METHOD_HESS= (.*)$', cfg, re.M)[1].strip()
symmetry_tags = [tag.strip() for tag in re.search(r'^MARKER_SYM= \((.*?)\)', cfg, re.M)[1].split(',')]
meshfile = Path(re.search(r'^MESH_FILENAME= (.*)$', cfg, re.M)[1])
restart = Path(re.search(r'^SOLUTION_FILENAME= (.*)$', cfg, re.M)[1] + '.csv')
with restart.open() as stream:
    original_names = next(csv.reader(stream))
original = np.loadtxt(restart, delimiter=',', skiprows=1, usecols=range(len(original_names)))
original = original[np.argsort(original[:, 0])]
symmetry_points = {}
with meshfile.open() as stream:
    for line in stream:
        if line.startswith('NELEM'):
            elements = np.array([list(map(int, next(stream).split()))[:5]
                                 for _ in range(int(line.split('=')[1]))])
        if line.startswith('NPOIN'):
            coordinates = np.array([[float(x) for x in next(stream).split()[:3]]
                                    for _ in range(int(line.split('=')[1].split()[0]))])
        if line.startswith('MARKER_TAG') and line.split('=')[1].strip() in symmetry_tags:
            tag = line.split('=')[1].strip()
            count = int(next(stream).split('=')[1])
            faces = [list(map(int, next(stream).split())) for _ in range(count)]
            assert all(face[0] == 5 for face in faces), 'This symmetry audit supports triangular faces only.'
            symmetry_points[tag] = np.unique([point for face in faces for point in face[1:4]])
assert np.array_equal(original[:, 0], np.arange(len(coordinates)))
assert np.max(abs(coordinates - original[:, 1:4])) < 1e-12
assert np.all(elements[:, 0] == 10), 'This independent integration supports tetrahedra only.'
tet = elements[:, 1:5]
vertices = coordinates[tet]
volume = np.zeros(len(coordinates))
cell_volume = abs(np.linalg.det(vertices[:, 1:] - vertices[:, :1])) / 6
np.add.at(volume, tet.ravel(), np.repeat(cell_volume / 4, 4))
symmetry_normals = {}
for tag, points in symmetry_points.items():
    centered = coordinates[points] - coordinates[points].mean(axis=0)
    _, _, directions = np.linalg.svd(centered, full_matrices=False)
    normal = directions[-1]
    assert np.max(abs(centered @ normal)) < 1e-10 * np.linalg.norm(np.ptp(coordinates, axis=0))
    symmetry_normals[tag] = normal
assert set(symmetry_points) == set(symmetry_tags)

def load(folder):
    with (folder / 'fields.csv').open() as stream:
        names = next(csv.reader(stream))
    data = np.loadtxt(folder / 'fields.csv', delimiter=',', skiprows=1)
    data = data[np.argsort(data[:, 0])]
    assert np.array_equal(data[:, 0], original[:, 0])
    return {name: data[:, k] for k, name in enumerate(names)}

def tensor(data, prefix):
    matrix = np.zeros((len(coordinates), 3, 3))
    for i, j, component in [(0, 0, 'XX'), (0, 1, 'XY'), (0, 2, 'XZ'),
                            (1, 1, 'YY'), (1, 2, 'YZ'), (2, 2, 'ZZ')]:
        matrix[:, i, j] = matrix[:, j, i] = data[prefix + '_' + component]
    return matrix

report = {'scope': 'Frozen matched ONERA M6 RANS/SA snapshot, zero flow/remesh iterations; ' + method +
                  '. No exact Hessian, aerodynamic or 3D native-remeshing accuracy claim.',
          'hessian_method': method,
          'points': len(coordinates), 'tetrahedra': len(tet),
          'max_mesh_restart_coordinate_difference': float(np.max(abs(coordinates - original[:, 1:4]))),
          'criteria': {'relative_complexity': 2e-6, 'relative_metric_mpi': 1e-9,
                       'relative_hessian_mpi_with_global_floor': 1e-9, 'relative_solution': 1e-14,
                       'relative_symmetry_mixed_hessian_with_global_floor': 1e-12}, 'cases': {},
          'inputs': {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in [meshfile, restart]}}
base = load(root / 'oneram6-rans-matched-mpi1')
M = tensor(base, 'Metric')
target = float(re.search(r'^ADAP_COMPLEXITY= (.*)$', cfg, re.M)[1])
minimum_size = float(re.search(r'^ADAP_HMIN= (.*)$', cfg, re.M)[1])
maximum_size = float(re.search(r'^ADAP_HMAX= (.*)$', cfg, re.M)[1])
maximum_aspect = float(re.search(r'^ADAP_ARMAX= (.*)$', cfg, re.M)[1])
report['criteria']['relative_metric_bounds'] = 1e-6  # eigenvalue roundoff at aspect ratios near 10000
passed = True
for rank in (1, 2, 4):
    folder = root / f'oneram6-rans-matched-mpi{rank}'
    data = load(folder)
    B = tensor(data, 'Metric')
    eigenvalues = np.linalg.eigvalsh(B)
    state_error = {}
    for k, field in enumerate(['Density', 'Momentum_x', 'Momentum_y', 'Momentum_z', 'Energy', 'Nu_Tilde']):
        reference = original[:, original_names.index(f'Conservative_{k + 1}')]
        state_error[field] = float(np.max(abs(data[field] - reference) / np.maximum(abs(reference), 1e-300)))
    hessians = {}
    symmetry = {}
    for sensor in ['MACH', 'PRESSURE']:
        A = tensor(base, 'Hessian_' + sensor)
        H = tensor(data, 'Hessian_' + sensor)
        norm = np.linalg.norm(A, axis=(1, 2))
        hessians[sensor] = {'finite': bool(np.isfinite(H).all()),
                            'max_relative_difference_with_global_floor': float(np.max(np.linalg.norm(A - H, axis=(1, 2)) /
                                                                                     np.maximum(norm, 1e-14 * norm.max())))}
        symmetry[sensor] = {}
        for tag, points in symmetry_points.items():
            normal = symmetry_normals[tag]
            normal_derivative = H[points] @ normal
            normal_curvature = normal_derivative @ normal
            mixed = normal_derivative - normal_curvature[:, None] * normal
            symmetry[sensor][tag] = {
                'points': len(points), 'normal': normal.tolist(),
                'max_relative_mixed_hessian_with_global_floor': float(np.max(np.linalg.norm(mixed, axis=1) /
                                                                            np.maximum(norm[points], 1e-14 * norm.max()))),
                'max_absolute_normal_curvature': float(np.max(abs(normal_curvature)))}
    entry = {'minimum_metric_eigenvalue': float(eigenvalues.min()),
             'maximum_metric_eigenvalue': float(eigenvalues.max()),
             'maximum_metric_aspect_ratio': float(np.sqrt(eigenvalues[:, -1] / eigenvalues[:, 0]).max()),
             'independent_complexity': float(np.sum(np.sqrt(eigenvalues.prod(axis=1)) * volume)),
             'max_relative_metric_difference': float(np.max(np.linalg.norm(M - B, axis=(1, 2)) / np.linalg.norm(M, axis=(1, 2)))),
             'solution_max_relative_difference': state_error, 'hessians': hessians, 'symmetry': symmetry,
             'fields_sha256': hashlib.sha256((folder / 'fields.csv').read_bytes()).hexdigest()}
    entry['passed'] = bool(np.isfinite(B).all() and eigenvalues.min() > 0 and
                           eigenvalues.min() >= (1 - 1e-6) / maximum_size**2 and
                           eigenvalues.max() <= (1 + 1e-6) / minimum_size**2 and
                           entry['maximum_metric_aspect_ratio'] <= (1 + 1e-6) * maximum_aspect and
                           abs(entry['independent_complexity'] / target - 1) < 2e-6 and
                           entry['max_relative_metric_difference'] < 1e-9 and max(state_error.values()) < 1e-14 and
                           all(x['max_relative_mixed_hessian_with_global_floor'] < 1e-12
                               for sensor in symmetry.values() for x in sensor.values()) and
                           all(x['finite'] and x['max_relative_difference_with_global_floor'] < 1e-9
                               for x in hessians.values()))
    passed &= entry['passed']
    report['cases'][str(rank)] = entry
report['passed'] = passed
(root / 'oneram6-rans-validation.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
raise SystemExit(0 if passed else 1)
