#!/usr/bin/env python3
"""Independent saved-cylinder BL tensors and full-edge quadrature; no SU2 execution."""
import csv
import json
import heapq
import math
import re
import sys
import time
from pathlib import Path

import numpy as np
import scipy
from scipy.integrate import quad

from construct import HERE, authorities, sha


def symmetric_function(a, function):
    values, vectors = np.linalg.eigh((a + a.T) / 2)
    return (vectors * function(values)) @ vectors.T


def unpack(values):
    xx, xy, xz, yy, yz, zz = map(float, values)
    return np.array([[xx, xy, xz], [xy, yy, yz], [xz, yz, zz]])


def rotation(a, b):
    a = a if a @ b >= 0 else -a
    v = np.cross(a, b)
    k = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + k + k @ k / (1 + a @ b)


def intersection(a, b):
    # A Cholesky frame is independent of the production sum-whitening eigensolve.
    factor = np.linalg.cholesky(a)
    whitened = np.linalg.solve(factor, np.linalg.solve(factor, b).T).T
    return factor @ symmetric_function(whitened, lambda x: np.maximum(1, x)) @ factor.T


class CylinderField:
    def __init__(self, case, data_name='core_accepted.json'):
        data = json.loads((case / data_name).read_text())
        config = dict(line.split('=', 1) for line in (case / 'core.cfg').read_text().splitlines() if '=' in line)
        config = {k.strip(): v.strip() for k, v in config.items()}
        def scalar(key):
            return float(config[key].strip('() '))
        wall = config['ADAP_BL_MARKER'].strip('() ')
        assert ',' not in wall and config['ADAP_HESSIAN_NOISE'] == '0'
        self.h, self.growth, self.thickness = [scalar('ADAP_BL_' + k) for k in ('FIRST_HEIGHT', 'GROWTH', 'THICKNESS')]
        self.full = max(self.h, .9 * self.thickness)
        self.width = self.thickness - self.full
        assert self.h > 0 and self.growth >= 1 and self.width > 0
        geometry, names = authorities(case / 'core_initial.su2.native_ref')
        faces = [f for f in data['facets'] if names[f['marker']] == wall]
        g = geometry[faces[0]['id']]
        assert g[0] == 3 and all(all(np.array_equal(a, b) for a, b in zip(g, geometry[f['id']])) for f in faces)
        _, self.origin, extent, self.geometry_tolerance, axis = g
        self.axis = axis / np.linalg.norm(axis)
        self.radius, self.z0, self.z1 = float(extent[0]), 0., float(extent[1])
        assert self.radius > 0 and self.z1 > self.z0
        self.triangles = np.array([[v[1:] for v in f['vertices']] for f in faces], dtype=float)
        ids = [[v[0] for v in f['vertices']] for f in faces]
        vertex_normals = {i: np.zeros(3) for face in ids for i in face}
        face_normals, face_covariances, angles = [], [], []
        for points, face in zip(self.triangles, ids):
            edges = np.roll(points, -1, axis=0) - points
            normal = np.cross(edges[0], edges[1]); normal /= np.linalg.norm(normal)
            covariance = (2 / 3) * edges.T @ edges
            projection = np.eye(3) - np.outer(normal, normal)
            covariance = projection @ covariance @ projection
            covariance += np.linalg.eigvalsh(covariance)[-1] * np.outer(normal, normal)
            face_angles = []
            for i in range(3):
                a, b = points[(i + 1) % 3] - points[i], points[(i - 1) % 3] - points[i]
                angle = math.atan2(np.linalg.norm(np.cross(a, b)), a @ b)
                vertex_normals[face[i]] += angle * normal
                face_angles.append(angle)
            face_normals.append(normal); face_covariances.append(covariance); angles.append(face_angles)
        for normal in vertex_normals.values():
            normal /= np.linalg.norm(normal)
        logs = {i: np.zeros((3, 3)) for i in vertex_normals}; weights = {i: 0. for i in logs}
        for face, normal, covariance, face_angles in zip(ids, face_normals, face_covariances, angles):
            for i, angle in zip(face, face_angles):
                r = rotation(normal, vertex_normals[i])
                logs[i] += angle * symmetric_function(r @ covariance @ r.T, np.log)
                weights[i] += angle
        self.normals = np.array([[vertex_normals[i] for i in face] for face in ids])
        self.logs = np.array([[logs[i] / weights[i] for i in face] for face in ids])
        self.face_ab = self.triangles[:, 1] - self.triangles[:, 0]
        self.face_ac = self.triangles[:, 2] - self.triangles[:, 0]
        self.gram_inverse = np.linalg.inv(np.stack((np.stack((np.sum(self.face_ab**2, axis=1), np.sum(self.face_ab*self.face_ac, axis=1)), axis=1),
                                                   np.stack((np.sum(self.face_ab*self.face_ac, axis=1), np.sum(self.face_ac**2, axis=1)), axis=1)), axis=1))
        sensors = [v[4] for d in data['original'] for v in d['vertices']]
        assert sensors and all(s == sensors[0] for s in sensors), 'This audit requires a constant frozen sensor, including collars.'
        self.sensor = unpack(sensors[0])
        self.curvature = (1.8 / (2 * math.sqrt(min(scalar('ADAP_HAUSD'), .5*self.h, self.radius) *
                         (2*self.radius-min(scalar('ADAP_HAUSD'), .5*self.h, self.radius)))))**2

    def closest_proxy(self, p):
        relative = p - self.triangles[:, 0]
        rhs = np.stack((np.sum(relative*self.face_ab, axis=1), np.sum(relative*self.face_ac, axis=1)), axis=1)
        uv = np.einsum('nij,nj->ni', self.gram_inverse, rhs)
        weights = np.column_stack((1-uv.sum(axis=1), uv))
        projected = np.einsum('ni,nij->nj', weights, self.triangles)
        distances = np.sum((projected-p)**2, axis=1); distances[np.any(weights < 0, axis=1)] = np.inf
        for i in range(3):
            j = (i+1) % 3; a = self.triangles[:, i]; direction = self.triangles[:, j]-a
            along = np.clip(np.sum((p-a)*direction, axis=1)/np.sum(direction**2, axis=1), 0, 1)
            distance = np.sum((a+along[:, None]*direction-p)**2, axis=1)
            replace = distance < distances
            weights[replace] = 0; weights[replace, i] = 1-along[replace]; weights[replace, j] = along[replace]
            distances[replace] = distance[replace]
        index = np.argmin(distances)
        return index, weights[index]

    def __call__(self, p):
        v = p-self.origin; z = v @ self.axis; radial = v-z*self.axis; radius = np.linalg.norm(radial)
        distance = abs(radius-self.radius)
        if distance >= self.thickness:
            return self.sensor.copy()
        assert self.z0-self.geometry_tolerance <= z <= self.z1+self.geometry_tolerance and radius > 0
        n = radial/radius
        foot = self.origin+np.clip(z, self.z0, self.z1)*self.axis+self.radius*n
        index, weights = self.closest_proxy(foot)
        log_size = np.zeros((3, 3))
        for k in range(3):
            r = rotation(self.normals[index, k], n)
            log_size += weights[k] * (r @ self.logs[index, k] @ r.T)
        size = symmetric_function(log_size, np.exp)
        tangent = np.cross(self.axis, n); tangent /= np.linalg.norm(tangent)
        basis = np.column_stack((tangent, self.axis))
        covariance = basis.T @ size @ basis
        tangential = np.linalg.inv(covariance)
        tangential[0, 0] += max(0, self.curvature-1/covariance[0, 0])
        height = max(self.h, 2*(self.h+(self.growth-1)*distance)/(self.growth+1))
        wall = basis @ tangential @ basis.T + np.outer(n, n)/(height*height)
        s = np.clip((distance-self.full)/self.width, 0, 1); fade = 1-s*s*(3-2*s)
        faded = symmetric_function(wall, lambda x: x**fade)
        return intersection(self.sensor, faded)

    def batch(self, points):
        points = np.asarray(points)
        result = np.broadcast_to(self.sensor, (len(points), 3, 3)).copy()
        v = points-self.origin; z = v@self.axis
        radial = v-z[:, None]*self.axis; radius = np.linalg.norm(radial, axis=1)
        distance = np.abs(radius-self.radius); active = np.flatnonzero(distance < self.thickness)
        if not len(active): return result
        z, distance, radial, radius = z[active], distance[active], radial[active], radius[active]
        assert np.all((z >= self.z0-self.geometry_tolerance) & (z <= self.z1+self.geometry_tolerance)) and np.all(radius > 0)
        n = radial/radius[:, None]
        foot = self.origin+np.clip(z, self.z0, self.z1)[:, None]*self.axis+self.radius*n
        relative = foot[:, None, :]-self.triangles[None, :, 0, :]
        rhs = np.stack((np.sum(relative*self.face_ab, axis=2), np.sum(relative*self.face_ac, axis=2)), axis=2)
        uv = np.einsum('fij,nfj->nfi', self.gram_inverse, rhs)
        weights = np.concatenate((1-uv.sum(axis=2, keepdims=True), uv), axis=2)
        projected = np.einsum('nfi,fij->nfj', weights, self.triangles)
        distances = np.sum((projected-foot[:, None, :])**2, axis=2)
        distances[np.any(weights < 0, axis=2)] = np.inf
        for i in range(3):
            j = (i+1) % 3; a = self.triangles[:, i]; direction = self.triangles[:, j]-a
            along = np.clip(np.sum((foot[:, None, :]-a)*direction, axis=2)/np.sum(direction**2, axis=1), 0, 1)
            candidate = np.sum((a+along[:, :, None]*direction-foot[:, None, :])**2, axis=2)
            replace = candidate < distances
            weights[replace] = 0; weights[:, :, i][replace] = 1-along[replace]; weights[:, :, j][replace] = along[replace]
            distances[replace] = candidate[replace]
        index = distances.argmin(axis=1); rows = np.arange(len(active)); weights = weights[rows, index]
        source = self.normals[index]; target = n[:, None, :]
        source = source*np.where(np.sum(source*target, axis=2, keepdims=True) < 0, -1, 1)
        cross = np.cross(source, target); k = np.zeros((len(active), 3, 3, 3))
        k[:, :, 0, 1] = -cross[:, :, 2]; k[:, :, 0, 2] = cross[:, :, 1]
        k[:, :, 1, 0] = cross[:, :, 2]; k[:, :, 1, 2] = -cross[:, :, 0]
        k[:, :, 2, 0] = -cross[:, :, 1]; k[:, :, 2, 1] = cross[:, :, 0]
        rotation = np.eye(3)+k+(k@k)/(1+np.sum(source*target, axis=2))[:, :, None, None]
        log_size = np.sum(weights[:, :, None, None]*(rotation@self.logs[index]@np.swapaxes(rotation, -1, -2)), axis=1)
        def spectral(matrix, function):
            values, vectors = np.linalg.eigh((matrix+np.swapaxes(matrix, -1, -2))/2)
            return (vectors*function(values)[:, None, :])@np.swapaxes(vectors, -1, -2)
        size = spectral(log_size, np.exp)
        tangent = np.cross(self.axis, n); tangent /= np.linalg.norm(tangent, axis=1)[:, None]
        basis = np.stack((tangent, np.broadcast_to(self.axis, tangent.shape)), axis=2)
        covariance = np.swapaxes(basis, -1, -2)@size@basis
        tangential = np.linalg.inv(covariance)
        tangential[:, 0, 0] += np.maximum(0, self.curvature-1/covariance[:, 0, 0])
        height = np.maximum(self.h, 2*(self.h+(self.growth-1)*distance)/(self.growth+1))
        wall = basis@tangential@np.swapaxes(basis, -1, -2)+n[:, :, None]*n[:, None, :]/height[:, None, None]**2
        s = np.clip((distance-self.full)/self.width, 0, 1); fade = 1-s*s*(3-2*s)
        faded = spectral(wall, lambda x: x**fade[:, None])
        factor = np.linalg.cholesky(self.sensor); inverse = np.linalg.inv(factor)
        result[active] = factor@spectral(inverse@faded@inverse.T, lambda x: np.maximum(1, x))@factor.T
        return result

    def cuts(self, a, b):
        p = a-self.origin; d = b-a
        radial = p-(p@self.axis)*self.axis; tangent = d-(d@self.axis)*self.axis
        aa, bb = tangent@tangent, 2*(radial@tangent)
        result = {0., 1.}
        for distance in (0., .5*self.h, self.full, self.thickness):
            for r in (self.radius-distance, self.radius+distance):
                if r <= 0 or aa == 0: continue
                cc = radial@radial-r*r; disc = bb*bb-4*aa*cc
                if disc < 0: continue
                root = math.sqrt(disc)
                # Stable quadratic roots; split at physical growth/fade shells, not native quadrature panels.
                q = -.5*(bb+math.copysign(root, bb))
                for t in ((q/aa, cc/q) if q else (-bb/(2*aa),)):
                    if 0 < t < 1: result.add(float(t))
        return sorted(result)


GAUSS = [np.polynomial.legendre.leggauss(order) for order in (16, 32)]


def integrate(field, a, b):
    direction = b-a
    cuts = field.cuts(a, b)
    pending = []; total = error = 0.; evaluations = 0; sequence = 0
    def measure(panels):
        nonlocal evaluations, sequence
        low = np.array([p[0] for p in panels]); high = np.array([p[1] for p in panels])
        middle = (low+high)/2; half = (high-low)/2
        values = []
        for nodes, weights in GAUSS:
            t = middle[:, None]+half[:, None]*nodes
            x = a+t[:, :, None]*direction
            tensors = field.batch(x.reshape((-1, 3)))
            speeds = np.sqrt(np.einsum('i,nij,j->n', direction, tensors, direction)).reshape(t.shape)
            values.append(half*(speeds@weights)); evaluations += len(tensors)
        result = []
        for i, (left, right, depth) in enumerate(panels):
            sequence += 1
            result.append((-abs(values[1][i]-values[0][i]), sequence, values[1][i], left, right, depth))
        return result
    panels = []
    for low, high in zip(cuts, cuts[1:]):
        seeds = np.linspace(low, high, 9)
        panels.extend((a, b, 0) for a, b in zip(seeds, seeds[1:]))
    for row in measure(panels):
        total += row[2]; error -= row[0]; heapq.heappush(pending, row)
    while error > 1e-10*abs(total)+1e-12:
        row = heapq.heappop(pending); negative_error, _, value, low, high, depth = row
        assert depth < 20 and evaluations < 262144, ('Independent quadrature unresolved', error, evaluations)
        middle = (low+high)/2; assert low < middle < high
        total -= value; error = max(0., error+negative_error)
        for child in measure([(low, middle, depth+1), (middle, high, depth+1)]):
            total += child[2]; error -= child[0]; heapq.heappush(pending, child)
    return total, error, evaluations


def self_check():
    a = np.diag([1., 4., 9.]); b = np.diag([4., 1., 16.])
    assert np.allclose(intersection(a, b), np.diag([4., 4., 16.]), rtol=1e-13, atol=1e-13)
    r = rotation(np.array([1., 0, 0]), np.array([0., 0, 1.]))
    assert np.allclose(r @ np.array([1., 0, 0]), [0, 0, 1]) and np.allclose(r.T@r, np.eye(3))
    random = np.random.default_rng(4703)
    for _ in range(20):
        p, q = random.normal(size=(3, 3)), random.normal(size=(3, 3))
        a, b = p@p.T+np.eye(3), q@q.T+np.eye(3)
        c = intersection(a, b)
        assert np.linalg.eigvalsh(c-a)[0] > -1e-11 and np.linalg.eigvalsh(c-b)[0] > -1e-11
        assert np.allclose(c, intersection(b, a), rtol=1e-12, atol=1e-12)
    assert abs(quad(lambda t: math.sqrt(1+3*t), 0, 1, epsabs=1e-12)[0]-14/9) < 1e-12


def main():
    self_check()
    case = HERE/sys.argv[1]; name = sys.argv[2]
    assert re.fullmatch('full_metric_audit_v[0-9]+', name)
    folder = HERE/name; folder.mkdir(exist_ok=True)
    output = folder/(case.name+'.json')
    assert not output.exists(), 'Preserve prior results; use a new version for a changed audit.'
    receipt_path = case.with_suffix('.json'); receipt = json.loads(receipt_path.read_text())
    assert receipt['status'] == 'PASS' and all(sha(case/p) == h for p, h in receipt['artifacts'].items())
    inputs = ['core_accepted.json', 'core.cfg', 'core_initial.su2.native_ref', 'core_samples.csv', 'core_edges.csv']
    contract = {'native_receipt_sha256': sha(receipt_path), 'inputs': {p: sha(case/p) for p in inputs},
                'helpers': {p.name: sha(p) for p in (Path(__file__).resolve(), HERE/'construct.py')}}
    started = time.monotonic()
    report = {'status': 'RUNNING', 'contract': contract, 'numpy_version': np.__version__, 'scipy_version': scipy.__version__, 'case': case.name}
    output.write_text(json.dumps(report, indent=2)+'\n')
    try:
        field = CylinderField(case); data = json.loads((case/'core_accepted.json').read_text())
        nodes = {n[0]: np.array(n[1:]) for n in data['nodes']}; cells = {c['id']: c for c in data['cells']}
        maximum = 0.; samples = list(csv.DictReader((case/'core_samples.csv').open()))
        assert len(samples) == len(cells) and {int(r['cell']) for r in samples} == set(cells)
        centers = np.array([sum((nodes[n] for n in cells[int(row['cell'])]['nodes']))/4 for row in samples])
        expected = field.batch(centers)
        for i in np.linspace(0, len(centers)-1, 64, dtype=int):
            assert np.allclose(expected[i], field(centers[i]), rtol=2e-12, atol=2e-12), 'Scalar/batch reference mismatch'
        for i, row in enumerate(samples):
            actual = unpack(row[k] for k in ('xx', 'xy', 'xz', 'yy', 'yz', 'zz'))
            error = np.linalg.norm(actual-expected[i])/max(1., np.linalg.norm(expected[i])); maximum = max(maximum, error)
            assert error < 1e-10, ('Point tensor mismatch', row['cell'], error, actual.tolist(), expected[i].tolist())
        report.update(status='PASS_POINT_TENSORS_EDGES_PENDING', point_samples=len(samples), maximum_point_relative_error=maximum)
        output.write_text(json.dumps(report, indent=2)+'\n'); print(report['status'], maximum, flush=True)
        edges = list(csv.DictReader((case/'core_edges.csv').open())); maximum_error = maximum_length = maximum_indicator = 0.
        with (folder/(case.name+'_edges.csv')).open('w') as stream:
            writer = csv.writer(stream); writer.writerow(['a', 'b', 'independent_length', 'estimated_error', 'native_length'])
            for number, row in enumerate(edges):
                a, b = nodes[int(row['a'])], nodes[int(row['b'])]; direction = b-a
                length, indicator, evaluations = integrate(field, a, b)
                native = float(row['length']); error = abs(length-native)/(1+length)
                assert indicator <= 1e-8*(1+length) and error <= 1e-7 and length+indicator <= 1.8, ('Full edge mismatch', row, length, indicator)
                writer.writerow([row['a'], row['b'], format(length, '.17g'), format(indicator, '.17g'), row['length']])
                maximum_error = max(maximum_error, error); maximum_length = max(maximum_length, length); maximum_indicator = max(maximum_indicator, indicator)
                if (number+1) % 500 == 0:
                    stream.flush(); print('Full edges checked:', number+1, '/', len(edges), flush=True)
        assert sha(Path(__file__).resolve()) == contract['helpers']['audit_full_metric.py']
        assert all(sha(case/p) == h for p, h in contract['inputs'].items())
        report.update(status='PASS_INDEPENDENT_COMPLETE_CYLINDER_METRIC_SCOPE_LIMITED', full_edges=len(edges), maximum_full_length=maximum_length,
                      maximum_native_relative_edge_difference=maximum_error, maximum_estimated_quadrature_error=maximum_indicator,
                      edge_csv_sha256=sha(folder/(case.name+'_edges.csv')),
                      scope='Saved constant original-P1 sensor including collars; wall covariance/log interpolation/analytic cylinder/growth/fade and Cholesky-frame intersection reconstructed independently with NumPy. All exported centroid target tensors compared and every full edge integrated by independent adaptive Gauss-Legendre16/32 quadrature split at physical growth/fade shells and eight seeded panels per shell. Error estimates are numerical indicators, not rigorous enclosures. No varying sensor, general geometry, CFD, performance or full Goal3 qualification.')
    except Exception as error:
        report.update(status='FAIL', error=repr(error)); raise
    finally:
        report['elapsed_seconds'] = time.monotonic()-started
        output.write_text(json.dumps(report, indent=2)+'\n')
    print(report['status'], flush=True)


if __name__ == '__main__':
    main()
