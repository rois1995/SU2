"""Compare the paper's two-pass Clément recovery on matched SU2 fields (NumPy only).

Export manufactured inputs with test_driver '[HessianRecoveryComparison]' and
SU2_HESSIAN_BENCHMARK_DIR pointing to an existing directory. Run this script with
--manufactured DIRECTORY --output REPORT.json. Optional --frozen LABEL CFG CSV
arguments compare a frozen CFD Hessian to the same independent operator.
This is a serial global-mesh benchmark, not a production or distributed backend.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import re
import time

import numpy as np


def checksum(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def read_mesh(path):
    dim = None
    markers = {}
    with Path(path).open() as stream:
        for line in stream:
            if line.startswith('NDIME'):
                dim = int(line.split('=')[1])
                assert dim in (2, 3)
            elif line.startswith('NELEM'):
                records = [list(map(int, next(stream).split())) for _ in range(int(line.split('=')[1]))]
                assert all(row[0] == (5 if dim == 2 else 10) for row in records), 'Simplex volumes required.'
                cells = np.array([row[1:dim + 2] for row in records], dtype=np.int64)
            elif line.startswith('NPOIN'):
                count = int(line.split('=')[1].split()[0])
                x = np.array([[float(v) for v in next(stream).split()[:dim]] for _ in range(count)])
            elif line.startswith('MARKER_TAG'):
                tag = line.split('=')[1].strip()
                count = int(next(stream).split('=')[1])
                rows = [list(map(int, next(stream).split())) for _ in range(count)]
                assert all(row[0] == (3 if dim == 2 else 5) for row in rows), 'Simplex boundary faces required.'
                markers[tag] = np.unique([v for row in rows for v in row[1:dim + 1]])
    assert cells.min() >= 0 and cells.max() < len(x)
    return x, cells, markers


def read_fields(path, x, sensor_names):
    with Path(path).open() as stream:
        names = next(csv.reader(stream))
    data = np.loadtxt(path, delimiter=',', skiprows=1)
    data = data[np.argsort(data[:, names.index('PointID')])]
    assert np.array_equal(data[:, names.index('PointID')], np.arange(len(x)))
    columns = {name: data[:, i] for i, name in enumerate(names)}
    assert np.max(abs(np.stack([columns[a] for a in 'xyz'[:x.shape[1]]], axis=1) - x)) < 1e-12
    values = np.stack([columns['SENSOR_' + name] for name in sensor_names], axis=1)
    H = np.zeros((len(x), len(sensor_names), x.shape[1], x.shape[1]))
    for v, name in enumerate(sensor_names):
        for i in range(x.shape[1]):
            for j in range(i, x.shape[1]):
                H[:, v, i, j] = H[:, v, j, i] = columns['Hessian_' + name + '_' + 'XYZ'[i] + 'XYZ'[j]]
    return values, H, columns


def project_symmetry(H, symmetry):
    for points, normal in symmetry:
        hn = H[points] @ normal
        mixed = hn - (hn @ normal)[..., None]*normal
        H[points] -= mixed[..., :, None]*normal + normal[:, None]*mixed[..., None, :]


def clement(x, cells, values, symmetry=()):
    """Alauzet/Frazza 2021 §7.3: exact P1 gradients, volume average, repeat.

    Boundary extrapolation is separate. Optional planar even-scalar symmetry
    follows the existing SU2 gradient/Hessian policy, beyond the paper's operator.
    Cache simplex geometry for the two passes, only for this benchmark call.
    """
    start = time.perf_counter()
    dim = x.shape[1]
    edges = x[cells[:, 1:]] - x[cells[:, :1]]
    # Equilibrate physical coordinates before inversion on stretched/rotated cells.
    scale = np.max(abs(edges), axis=1)
    assert np.all(scale > 0) and np.isfinite(scale).all(), 'Collapsed coordinate direction.'
    balanced = edges / scale[:, None, :]
    determinant = np.linalg.det(balanced)
    assert np.all(abs(determinant) > 0) and np.isfinite(determinant).all(), 'Degenerate simplex.'
    inverse = np.linalg.inv(balanced) / scale[:, :, None]
    volume = abs(determinant) * scale.prod(axis=1) / math.factorial(dim)
    star_volume = np.bincount(cells.ravel(), weights=np.repeat(volume, dim + 1), minlength=len(x))
    assert np.all(star_volume > 0), 'Unused vertex.'
    geometry_seconds = time.perf_counter() - start

    def project(field):
        delta = field[cells[:, 1:]] - field[cells[:, :1]]
        cell_gradient = np.einsum('eij,ejv->evi', inverse, delta, optimize=True)
        result = np.zeros((len(x), field.shape[1], dim))
        for node in range(dim + 1):
            np.add.at(result, cells[:, node], volume[:, None, None] * cell_gradient)
        return result / star_volume[:, None, None]

    gradient = project(values)
    for points, normal in symmetry:
        gradient[points] -= (gradient[points] @ normal)[..., None]*normal
    derivative = project(gradient.reshape(len(x), -1)).reshape(len(x), values.shape[1], dim, dim)
    H = .5 * (derivative + derivative.swapaxes(-1, -2))
    project_symmetry(H, symmetry)
    assert np.isfinite(H).all()
    timing = {'geometry_seconds': geometry_seconds, 'two_pass_seconds': time.perf_counter() - start,
              'cached_geometry_bytes': inverse.nbytes + volume.nbytes + star_volume.nbytes,
              'implementation': 'serial NumPy; cannot compare directly to C++ kernel timings'}
    return H, star_volume / (dim + 1), timing


def graph(cells, npoint):
    pairs = np.concatenate([cells[:, [i, j]] for i in range(cells.shape[1]) for j in range(i + 1, cells.shape[1])])
    pairs = np.unique(np.sort(pairs, axis=1), axis=0)
    return np.concatenate([pairs[:, 0], pairs[:, 1]]), np.concatenate([pairs[:, 1], pairs[:, 0]])


def extend_boundary(H, boundary, row, col, weights, exclude_first_layer=False):
    """Controlled constant extension, not claimed to duplicate refine's code.

    Volume-average adjacent interior donors; for the second policy exclude donors
    touching a physical boundary. Synchronous graph layers avoid traversal-order
    dependence. Only physical boundary Hessians are replaced in the returned field.
    Report uncovered points explicitly, retaining their original values.
    """
    start = time.perf_counter()
    eligible = ~boundary.copy()
    if exclude_first_layer:
        touches = np.bincount(row, weights=boundary[col], minlength=len(H)) > 0
        eligible &= ~touches
    known = eligible.copy()
    work = H.reshape(len(H), -1).copy()
    depth = np.zeros(len(H), dtype=int)
    for layer in range(1, 9):
        use = ~known[row] & known[col]
        receivers, donors = row[use], col[use]
        denominator = np.bincount(receivers, weights=weights[donors], minlength=len(H))
        fill = ~known & (denominator > 0)
        if not fill.any():
            break
        total = np.zeros_like(work)
        np.add.at(total, receivers, weights[donors, None] * work[donors])
        work[fill] = total[fill] / denominator[fill, None]
        known[fill] = True
        depth[fill] = layer
        if known[boundary].all():
            break
    covered = boundary & known
    result = H.copy()
    result[covered] = work[covered].reshape(result[covered].shape)
    return result, {'eligible_donors': int(eligible.sum()), 'boundary_points': int(boundary.sum()),
                    'covered_boundary_points': int(covered.sum()), 'uncovered_boundary_points': int((boundary & ~known).sum()),
                    'max_graph_extension_depth': int(depth[covered].max(initial=0)),
                    'seconds': time.perf_counter() - start}


def manufactured_errors(x, parameter, H, curved, weights, wall, baseline):
    dim = x.shape[1]
    c, s, aspect, bend, pi = math.cos(.37), math.sin(.37), 1000, (.02 if curved else 0), math.pi
    q = np.zeros((len(x), 3))
    q[:, 0] = c*x[:, 0] + s*x[:, 1]
    if dim == 3:
        q[:, 2] = x[:, 2]
    q[:, 1] = aspect*(-s*x[:, 0] + c*x[:, 1]) - bend*np.sin(pi*q[:, 0])
    if dim == 3:
        q[:, 1] -= bend*np.sin(pi*q[:, 2])
    exact = np.array([[2, .5, -.3], [.5, 4, .2], [-.3, .2, 6]])[:dim, :dim]
    g = q[:, :dim] @ exact
    ref = np.broadcast_to(exact, (len(x), dim, dim)).copy()
    ref[:, 0, 0] += g[:, 1]*bend*pi*pi*np.sin(pi*q[:, 0])
    if dim == 3:
        ref[:, 2, 2] += g[:, 1]*bend*pi*pi*np.sin(pi*q[:, 2])
    T = np.zeros((len(x), dim, dim))
    first = bend*pi*np.cos(pi*q[:, 0])
    T[:, 0, 0], T[:, 1, 0] = c-s*first/aspect, s+c*first/aspect
    T[:, 0, 1], T[:, 1, 1] = -s/aspect, c/aspect
    if dim == 3:
        span_first = bend*pi*np.cos(pi*q[:, 2])
        T[:, 0, 2], T[:, 1, 2], T[:, 2, 2] = -s*span_first/aspect, c*span_first/aspect, 1
    recovered = np.einsum('nai,nab,nbj->nij', T, H[:, 0], T, optimize=True)
    delta = recovered-ref
    difference = np.einsum('nai,nab,nbj->nij', T, H[:, 0]-baseline[:, 0], T, optimize=True)
    inner = (parameter[:, 0] >= .4) & (parameter[:, 0] <= (1.6 if dim == 2 else .6))
    inner &= (parameter[:, 1] >= .4) & (parameter[:, 1] <= .6)
    if dim == 3:
        inner &= (parameter[:, 2] >= .4) & (parameter[:, 2] <= .6)
    wall_inner = wall & (parameter[:, 0] > 1e-12) & (parameter[:, 0] < (2 if dim == 2 else 1)-1e-12)
    if dim == 3:
        wall_inner &= (parameter[:, 2] > 1e-12) & (parameter[:, 2] < 1-1e-12)
    report = {}
    for name, mask in [('all', np.ones(len(x), dtype=bool)), ('interior', inner), ('wall', wall),
                       ('wall_interior', wall_inner), ('wall_edge', wall & ~wall_inner)]:
        assert mask.any()
        error = delta[mask]
        mixed = [abs(error[:, i, j]) for i in range(dim) for j in range(dim) if (i == 1) != (j == 1)]
        tangent = [abs(error[:, i, j]) for i in range(dim) for j in range(dim) if i != 1 and j != 1]
        report[name] = {'points': int(mask.sum()), 'max_component': float(abs(error).max()),
                        'max_layer_difference_to_su2': float(abs(difference[mask]).max()),
                        'relative_normal_max': float(abs(error[:, 1, 1]/4).max()),
                        'tangent_max': float(np.max(tangent)), 'mixed_max': float(np.max(mixed)),
                        'volume_weighted_layer_rms': float(np.sqrt(np.sum(weights[mask]*np.sum(error**2, axis=(1, 2))) / weights[mask].sum()))}
    return report


def self_check():
    # Independent exact affine derivatives, signed orientation, and a hand-computed
    # one-sided quadratic derivative: raw Clément is deliberately not boundary-exact.
    for dim in (2, 3):
        x = np.vstack([np.zeros(dim), np.eye(dim)])
        cells = np.arange(dim+1)[None, :]
        b = np.arange(1, dim+1)
        H, weights, _ = clement(x, cells, (x @ b + 7)[:, None])
        assert abs(H).max() < 1e-13
        permuted, _, _ = clement(x, cells[:, ::-1], (x @ b + 7)[:, None])
        assert np.allclose(H, permuted, atol=1e-13)
        assert abs(weights.sum() - 1/math.factorial(dim)) < 1e-14
    x = np.array([[0., 0.], [1, 0], [0, 1], [1, 1]])
    cells = np.array([[0, 1, 3], [0, 2, 3]])
    H, _, _ = clement(x, cells, (.5*(x*x).sum(axis=1))[:, None])
    assert abs(H).max() < 1e-13  # each P1 gradient is (.5,.5); a boundary-only mesh has zero recovered curvature
    row, col = graph(cells, len(x))
    fixed = np.broadcast_to(np.eye(2), (4, 1, 2, 2)).copy()
    extension, receipt = extend_boundary(fixed, np.array([True, False, False, True]), row, col, np.ones(4))
    assert np.array_equal(extension, fixed) and receipt['uncovered_boundary_points'] == 0
    _, receipt = extend_boundary(fixed, np.ones(4, dtype=bool), row, col, np.ones(4))
    assert receipt['uncovered_boundary_points'] == 4
    H = np.array([[[[2., .4], [.4, -3.]]]])
    project_symmetry(H, [(np.array([0]), np.array([1., 0.]))])
    assert np.array_equal(H[0, 0], np.diag([2., -3.]))
    # A fixed near-wall ring in y_i=(i/n)^1.3 retains its spacing ratio under
    # refinement. Repeating volume-weighted P1 recovery does not become exact
    # there, even on this noise-free quadratic with exact Hessian 1.
    graded = []
    for n in (8, 16, 32):
        y = (np.arange(n+1)/n)**1.3
        def recover_1d(field):
            width = np.diff(y)
            element = np.diff(field)/width
            return np.r_[element[0], (width[:-1]*element[:-1]+width[1:]*element[1:]) /
                         (width[:-1]+width[1:]), element[-1]]
        recovered = recover_1d(recover_1d(.5*y*y))[2]
        graded.append({'n': n, 'ring': 2, 'exact_hessian': 1., 'recovered_hessian': float(recovered)})
    assert np.ptp([r['recovered_hessian'] for r in graded]) < 1e-12
    assert abs(graded[0]['recovered_hessian']-1) > .04
    return graded


def compare(path, fields, sensors, manufactured=False, symmetry_tags=()):
    x, cells, markers = read_mesh(path)
    values, reference, columns = read_fields(fields, x, sensors)
    symmetry = []
    for tag in symmetry_tags:
        points = markers[tag]
        centered = x[points] - x[points].mean(axis=0)
        _, _, directions = np.linalg.svd(centered, full_matrices=False)
        normal = directions[-1]
        assert np.max(abs(centered @ normal)) < 1e-10*np.linalg.norm(np.ptp(x, axis=0)), 'Planar symmetry required.'
        symmetry.append((points, normal))
    H, weights, timing = clement(x, cells, values, symmetry)
    boundary = np.zeros(len(x), dtype=bool)
    boundary[np.concatenate(list(markers.values()))] = True
    row, col = graph(cells, len(x))
    variants = {'CLEMENT': H}
    extensions = {}
    for strict in (False, True):
        name = 'CLEMENT_INTERIOR_2' if strict else 'CLEMENT_INTERIOR_1'
        variants[name], extensions[name] = extend_boundary(H, boundary, row, col, weights, strict)
        project_symmetry(variants[name], symmetry)
    result = {'points': len(x), 'cells': len(cells), 'inputs': {str(p): checksum(p) for p in (path, fields)},
              'sensor_values_sha256': hashlib.sha256(values.astype('<f8').tobytes()).hexdigest(),
              'symmetry_tags': list(symmetry_tags),
              'timing': timing, 'boundary_extension': extensions, 'methods': {}}
    if manufactured:
        parameter = np.stack([columns['Parameter_' + a] for a in 'XYZ'[:x.shape[1]]], axis=1)
        wall = parameter[:, 1] < 1e-12
        curved = '-curved-' in fields.name
        original_method = fields.stem.split('-')[-1]
        variants[original_method] = reference
        if original_method == 'GREEN_GAUSS':
            name = 'GREEN_GAUSS_INTERIOR_2'
            variants[name], extensions[name] = extend_boundary(reference, boundary, row, col, weights, True)
        for name, tensor in variants.items():
            result['methods'][name] = manufactured_errors(x, parameter, tensor, curved, weights, wall, reference)
    else:
        # No exact CFD Hessian: disagreement and spectra are diagnostics, not accuracy.
        for name, tensor in variants.items():
            per_sensor = {}
            for v, sensor in enumerate(sensors):
                scale = np.linalg.norm(reference[:, v], axis=(1, 2))
                difference = np.linalg.norm(tensor[:, v]-reference[:, v], axis=(1, 2)) / np.maximum(scale, 1e-14*scale.max())
                per_sensor[sensor] = {'relative_difference_q50_q90_q99_max': np.quantile(difference, [.5, .9, .99, 1]).tolist(),
                                      'max_absolute_eigenvalue': float(abs(np.linalg.eigvalsh(tensor[:, v])).max()),
                                      'finite': bool(np.isfinite(tensor[:, v]).all())}
            result['methods'][name] = per_sensor
    return result


def validate_manufactured(cases):
    """Check identical inputs, exact flat interiors/extensions, and curved convergence.

    Boundary convergence is measured, not asserted: this benchmark exposes its
    absence on self-similar graded near-wall patches.
    """
    groups = {}
    for label, result in cases.items():
        groups.setdefault(label.rsplit('-', 1)[0], []).append(result)
        for name, extension in result['boundary_extension'].items():
            assert extension['uncovered_boundary_points'] == 0, (label, name)
        if label.endswith('-GREEN_GAUSS'):
            assert result['methods']['CLEMENT']['interior']['max_layer_difference_to_su2'] < 2e-7, label
        if '-flat-' in label:
            assert result['methods']['CLEMENT']['interior']['max_component'] < 1e-7, label
            assert result['methods']['CLEMENT_INTERIOR_2']['wall']['max_component'] < 1e-7, label
    for group in groups.values():
        assert len(group) == 3, 'GG/WLS/QR matched inputs are required.'
        assert len({r['sensor_values_sha256'] for r in group}) == 1
        assert len({next(iter(r['inputs'].values())) for r in group}) == 1, 'Mesh mismatch.'
    for dim in (2, 3):
        sequence = [cases[f'd{dim}-curved-n{n}-GREEN_GAUSS']['methods']['CLEMENT']['interior']['max_component']
                    for n in (8, 16, 32)]
        assert sequence[1] < .6*sequence[0] and sequence[2] < .5*sequence[1]
    return {'matched_mesh_and_sensor_groups': len(groups), 'flat_interior_and_boundary_extension_exact': True,
            'curved_interior_converges': True, 'boundary_convergence_required': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manufactured', type=Path)
    parser.add_argument('--frozen', nargs=3, action='append', default=[], metavar=('LABEL', 'CONFIG', 'FIELDS'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    graded = self_check()
    report = {'scope': 'Matched global-mesh serial benchmark; no production backend, MPI certification, metric or aerodynamic accuracy claim.',
              'boundary_policy': 'Controlled constant volume-average extension from one/two graph layers away from physical boundaries. Not an exact port of refine.',
              'self_check_passed': True, 'noise_free_graded_1d_counterexample': graded,
              'script_sha256': checksum(__file__), 'manufactured': {}, 'frozen': {}}
    def save():
        args.output.write_text(json.dumps(report, indent=2)+'\n')
    if args.manufactured:
        for fields in sorted(args.manufactured.glob('d*-*.csv')):
            report['manufactured'][fields.stem] = compare(fields.with_suffix('.su2'), fields, ['PRESSURE'], True)
            save()
            print(fields.stem, 'complete', flush=True)
        report['manufactured_validation'] = validate_manufactured(report['manufactured'])
    for label, config, fields in args.frozen:
        text = Path(config).read_text()
        mesh = Path(re.search(r'^MESH_FILENAME\s*=\s*(.*)$', text, re.M)[1].strip())
        sensors = [s.strip() for s in re.search(r'^ADAP_SENSOR\s*=\s*\((.*?)\)', text, re.M)[1].split(',')]
        sym = re.search(r'^MARKER_SYM\s*=\s*\((.*?)\)', text, re.M)
        symmetry_tags = [s.strip() for s in sym[1].split(',') if s.strip()] if sym else []
        report['frozen'][label] = compare(mesh, Path(fields), sensors, symmetry_tags=symmetry_tags)
        report['frozen'][label]['configuration_sha256'] = checksum(config)
        save()
        print(label, 'complete', flush=True)
    save()


if __name__ == '__main__':
    main()
