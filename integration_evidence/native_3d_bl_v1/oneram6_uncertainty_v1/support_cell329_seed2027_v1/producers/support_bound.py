#!/usr/bin/env python3
"""Pilot exact-arithmetic BL-support mass envelope; floating results are NOT machine certificates."""
import csv, itertools, json, math, sys, time
from pathlib import Path
import numpy as np
from run import HERE, ROOT, OLD, FROZEN, sha
sys.path.insert(0, str(FROZEN))
from audit_sensor_mass import duffy_rule, logdet
sys.path.insert(0, str(ROOT / 'TestCases/adaptation/capability'))
from capcheck import read_su2


def inverse_height_moments(h0, growth, radius):
    """Upper integrals of r**k/hn(r), using hn >= max(h0, b*r), k=0,1,2."""
    assert h0 > 0 and growth >= 1 and radius >= 0
    b = 2 * (growth - 1) / (growth + 1)
    split = min(radius, h0 / b) if b else radius
    result = np.array([split ** (k + 1) / (h0 * (k + 1)) for k in range(3)], dtype=np.longdouble)
    if radius > split:
        result += [np.log(radius / split) / b, (radius - split) / b,
                   (radius - split) * (radius + split) / (2 * b)]
    return result


def selfcheck():
    # The low-size upper integrand must majorize the actual geometric-row normal demand.
    for growth in (1., 1. + 1e-12, 1.2, 2., 5.):
        for radius in (0., 1e-7, 1e-5, .01):
            h0 = 2e-6
            r = np.linspace(0., radius, 1001, dtype=np.longdouble)
            hn = np.maximum(h0, 2 * (h0 + (growth - 1) * r) / (growth + 1))
            lower_size = np.maximum(h0, 2 * (growth - 1) / (growth + 1) * r)
            assert np.all(hn >= lower_size * (1 - 1e-15))
            assert np.all(np.isfinite(inverse_height_moments(h0, growth, radius)))
    # An intersection of two negative hemispheres has area 2*(pi-angle(e1,e2)).
    assert abs(2 * np.arccos(0.) - np.pi) < 1e-15
    assert 2 * np.arccos(1.) == 0
    rng = np.random.default_rng(123)
    for _ in range(64):
        edges = rng.normal(size=(3, 3));edges[2] = -edges[0] - edges[1]
        cross = np.cross(edges[0], edges[1]);product = 4 / 3 * np.dot(cross, cross)
        covariance = 2 / 3 * edges.T @ edges
        trace = np.trace(covariance)
        largest = (trace + np.sqrt(max(0., trace * trace - 4 * product))) / 2
        smallest = product / largest
        direct = np.linalg.eigvalsh(covariance + largest * np.outer(cross, cross) / np.dot(cross, cross))[0]
        assert abs(smallest - direct) < 1e-10 * largest
        raw = rng.normal(size=(4, 3, 3))
        matrices = raw @ np.swapaxes(raw, -1, -2) + .2 * np.eye(3)
        weight = rng.random(4);weight /= weight.sum();sensor = np.einsum('k,kij->ij', weight, matrices)
        beta = 2.;alpha = 10000.;direction = rng.normal(size=3);direction /= np.linalg.norm(direction)
        combined = matrices.sum(axis=0) + beta * np.eye(3);values = np.linalg.eigvalsh(combined)
        bound = np.sqrt(np.linalg.det(combined)) + np.sqrt(alpha * values[-1] * values[-2])
        density = np.sqrt(np.linalg.det(sensor + beta * np.eye(3) + alpha * np.outer(direction, direction)))
        assert density <= bound * (1 + 1e-12)


def wall_geometry(fixture):
    lines = fixture.read_text().splitlines();assert lines[0] == 'NATIVE_BL_REPLAY_3D 2'
    count, faces = map(int, lines[1].split());h0, growth, thickness = map(float, lines[2].split())
    p = np.array([line.split() for line in lines[3:3 + count]], dtype=np.longdouble)
    f = np.array([line.split() for line in lines[3 + count:3 + count + faces]], dtype=np.int64)
    x = p[f];edge = np.roll(x, -1, axis=1) - x
    normal = np.cross(x[:, 1] - x[:, 0], x[:, 2] - x[:, 0])
    norm2 = np.sum(normal * normal, axis=1);assert np.all(norm2 > 0)
    area = np.sqrt(norm2) / 2;normal /= np.sqrt(norm2)[:, None]
    trace = 2 / 3 * np.sum(edge * edge, axis=(1, 2));product = 4 / 3 * norm2
    largest = (trace + np.sqrt(np.maximum(0., trace * trace - 4 * product))) / 2
    beta_face = largest / product
    log_lower = np.zeros(len(p), dtype=np.longdouble);weight_sum = np.zeros(len(p), dtype=np.longdouble)
    lengths = np.sqrt(np.sum(edge * edge, axis=2))
    for k in range(3):
        cosine = -np.sum(edge[:, (k + 2) % 3] * edge[:, k], axis=1) / (lengths[:, (k + 2) % 3] * lengths[:, k])
        angle = np.arccos(np.clip(cosine, -1, 1))
        np.add.at(log_lower, f[:, k], -angle * np.log(beta_face))
        np.add.at(weight_sum, f[:, k], angle)
    assert np.all(weight_sum > 0)
    # lambda_min(sum(angle*R*log(C)*R.T)) >= sum(angle*log(lambda_min(C))).
    beta_vertex = np.exp(-log_lower / weight_sum)
    beta = np.max(beta_vertex[f], axis=1)
    pairs = np.sort(np.concatenate([f[:, [k, (k + 1) % 3]] for k in range(3)]), axis=1)
    edge_faces = np.tile(np.arange(len(f)), 3)
    unique, inverse, incidence = np.unique(pairs, axis=0, return_inverse=True, return_counts=True)
    assert np.all(incidence <= 2), 'Nonmanifold wall is outside this pilot.'
    first = np.full(len(unique), len(f), dtype=np.int64);last = np.zeros(len(unique), dtype=np.int64)
    np.minimum.at(first, inverse, edge_faces);np.maximum.at(last, inverse, edge_faces)
    angle = np.full(len(unique), np.pi, dtype=np.longdouble)
    shared = incidence == 2
    angle[shared] = np.arccos(np.clip(np.sum(normal[first[shared]] * normal[last[shared]], axis=1), -1, 1))
    length = np.sqrt(np.sum((p[unique[:, 0]] - p[unique[:, 1]]) ** 2, axis=1))
    adjacent = [[] for _ in p]
    for a, b in unique:
        adjacent[a].append(b);adjacent[b].append(a)
    omega = np.empty(len(p), dtype=np.longdouble)
    for i, neighbors in enumerate(adjacent):
        directions = p[neighbors] - p[i];directions /= np.sqrt(np.sum(directions * directions, axis=1))[:, None]
        minimum = np.min(directions @ directions.T)
        omega[i] = 2 * np.arccos(np.clip(-minimum, -1, 1))
    return {'p': p, 'f': f, 'x': x, 'area': area, 'normal': normal, 'beta': beta,
            'edges': unique, 'face_edges': inverse.reshape(3, -1).T, 'edge_angle_length': angle * length, 'omega': omega,
            'h0': h0, 'growth': growth, 'thickness': thickness}


def support(wall, tet):
    x = wall['x'];radius = wall['thickness']
    gap = np.maximum(np.maximum(tet.min(axis=0) - x.max(axis=1), x.min(axis=1) - tet.max(axis=0)), 0)
    padding = 256 * np.finfo(float).eps * max(1., float(abs(tet).max()), float(abs(x).max()))
    ids = np.flatnonzero(np.sum(gap * gap, axis=1) <= (radius + padding) ** 2)
    # Conservative separating projections; overlap admits support, never proves a BL intersection.
    candidates = x[ids] - tet[0];local = tet - tet[0]
    axes = [wall['normal'][ids]]
    for face in itertools.combinations(range(4), 3):
        n = np.cross(local[face[1]] - local[face[0]], local[face[2]] - local[face[0]])
        axes.append(np.broadcast_to(n, (len(ids), 3)))
    for a, b in itertools.combinations(range(4), 2):
        for k in range(3):
            axes.append(np.cross(local[b] - local[a], candidates[:, (k + 1) % 3] - candidates[:, k]))
    keep = np.ones(len(ids), dtype=bool)
    for axis in axes:
        norm = np.sqrt(np.sum(axis * axis, axis=1))
        a = local @ axis.T;b = np.einsum('nkj,nj->nk', candidates, axis)
        margin = (radius + padding) * norm
        keep &= (b.min(axis=1) - a.max(axis=0) <= margin) & (a.min(axis=0) - b.max(axis=1) <= margin)
    return ids[keep]


def envelope(wall, tet, raw_sensor, core):
    start = time.monotonic();ids = support(wall, tet)
    matrix = np.tril(raw_sensor) + np.swapaxes(np.tril(raw_sensor, -1), -2, -1)
    volume = abs(np.dot(tet[1] - tet[0], np.cross(tet[2] - tet[0], tet[3] - tet[0]))) / 6
    bary, weights = duffy_rule();nodal = np.exp(logdet(matrix) / 3)
    lower = volume * np.mean(nodal) ** 1.5
    mean_det = sum(weight * np.exp(logdet(np.einsum('k,kij->ij', beta, matrix))) for beta, weight in zip(bary, weights))
    sensor_upper = volume * np.sqrt(mean_det)
    if not len(ids):
        return dict(lower=float(lower), upper=float(sensor_upper), support_faces=0, seconds=time.monotonic() - start)
    vertices = np.unique(wall['f'][ids]);edge_ids = np.unique(wall['face_edges'][ids])
    radius = min(wall['thickness'], float(np.min(np.max(np.sqrt(np.sum((tet[None, :, :] - wall['p'][vertices, None, :]) ** 2, axis=2)), axis=1))))
    area = np.sum(wall['area'][ids]);length_angle = np.sum(wall['edge_angle_length'][edge_ids]);omega = np.sum(wall['omega'][vertices])
    support_volume = min(volume, 2 * area * radius + length_angle * radius ** 2 / 2 + omega * radius ** 3 / 3)
    moments = inverse_height_moments(wall['h0'], wall['growth'], radius)
    height_integral = min(volume / wall['h0'], 2 * area * moments[0] + length_angle * moments[1] + omega * moments[2])
    a = np.max(np.sum(abs(matrix), axis=2));beta = max(float(np.max(wall['beta'][ids])), core)
    # P1 A <= sum(A_vertex); retain that anisotropic matrix instead of its isotropic largest eigenvalue.
    combined = np.sum(matrix, axis=0) + beta * np.eye(3)
    eigenvalues = np.linalg.eigvalsh(combined);assert np.all(eigenvalues > 0)
    background_density = float(np.exp(logdet(combined) / 2))
    directional_gain = float(np.sqrt(eigenvalues[-1] * eigenvalues[-2]))
    # det(C+alpha*n*n.T) <= det(C)+alpha*lambda_max(adj(C)); sqrt(x+y) <= sqrt(x)+sqrt(y).
    point_lower = float(np.min(np.exp(logdet(matrix) / 2)))
    delta = max(0., background_density + directional_gain * math.sqrt(core) - point_lower) * support_volume + directional_gain * height_integral
    return {'lower': float(lower), 'upper': float(sensor_upper + delta), 'sensor_upper': float(sensor_upper),
            'correction_upper': float(delta), 'volume': float(volume), 'support_volume_upper': float(support_volume),
            'inverse_height_integral_upper': float(height_integral), 'sensor_spectral_upper': float(a), 'tangent_upper': beta,
            'matrix_background_density_upper': background_density, 'matrix_normal_gain_upper': directional_gain,
            'support_faces': len(ids), 'support_edges': len(edge_ids), 'support_vertices': len(vertices),
            'radius_upper': radius, 'face_area': float(area), 'edge_angle_length': float(length_angle),
            'vertex_solid_angle_upper': float(omega), 'seconds': time.monotonic() - start}


def main():
    folder = HERE / 'support_mass_envelope_v2';folder.mkdir(exist_ok=True)
    selfcheck();start = time.monotonic()
    capture = json.loads((FROZEN / 'result.json').read_text())
    assert sha(FROZEN / 'frozen_trial.npz') == capture['sha256']['frozen_trial.npz']
    fixture = OLD / 'cell329/replay.dat';wall = wall_geometry(fixture)
    mesh = read_su2(FROZEN / 'Coarse.su2');data = np.load(FROZEN / 'frozen_trial.npz', allow_pickle=False)
    unknown = {};source_paths = sorted((HERE / 'global_envelope_seed_np4_v1').glob('attempt*/field_estimates_rank*.csv'))
    for path in source_paths:
        for row in csv.DictReader(path.open()):
            if not np.isfinite(float(row['indicator'])):unknown[int(row['cell'])] = row
    assert len(unknown) == 14
    selected = sorted(set(unknown) | {88, 157, 329, 622, 2331})
    mappings = []
    for rank in range(4):
        mapping = np.full(len(mesh.P), -1, dtype=np.int64);ids = data['rank%d_global' % rank];mapping[ids] = np.arange(len(ids));mappings.append(mapping)
    result = {'scope': 'Saved original raw9 host fields; one faceted wall. Exact-arithmetic support envelope pilot, not production normalization or an outward-rounded floating certificate.',
              'machine_bound_proven': False, 'complete_field_integrated': False, 'CFD_steps': 0, 'cells': []}
    for cell in selected:
        nodes = np.sort(mesh.E[cell]);rank = int(data['cell_owner'][cell]);local = mappings[rank][nodes];assert np.all(local >= 0)
        bound = envelope(wall, mesh.P[nodes].astype(np.longdouble), data['rank%d_sensor' % rank][local], float(data['core']))
        bound.update(cell=cell, was_unknown_in_prefix=cell in unknown)
        if cell in unknown:
            value = float(unknown[cell]['value']);bound['retained_positive_estimate'] = value
            bound['finite_absolute_error_envelope'] = max(abs(value - bound['lower']), abs(bound['upper'] - value))
        if cell in (88, 157, 329):
            order = 72 if cell == 157 else 128
            ref_paths = [HERE.parent / 'oneram6_density_reference_v1' / ('cell%d' % cell) / ('order%d_apex%d' % (order, apex)) / 'result.json' for apex in (0, 3)]
            source_paths += ref_paths
            refs = [json.loads(path.read_text())['integral'] for path in ref_paths]
            reference = sum(refs) / 2;bound['historical_reference'] = reference;bound['upper_to_reference_ratio'] = bound['upper'] / reference
            assert bound['lower'] <= min(refs) and bound['upper'] >= max(refs)
        result['cells'].append(bound);print('BOUND', json.dumps(bound), flush=True)
    result['unknown_upper_sum'] = sum(x['upper'] for x in result['cells'] if x['was_unknown_in_prefix'])
    result['unknown_error_envelope_sum'] = sum(x['finite_absolute_error_envelope'] for x in result['cells'] if x['was_unknown_in_prefix'])
    result['seconds'] = time.monotonic() - start
    paths = [Path(__file__).resolve(), fixture, FROZEN / 'frozen_trial.npz', FROZEN / 'Coarse.su2', FROZEN / 'audit_sensor_mass.py',
             ROOT / 'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp', ROOT / 'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp', FROZEN / 'result.json'] + source_paths
    result['sha256'] = {str(path.relative_to(ROOT)): sha(path) for path in paths}
    (folder / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('SUMMARY', json.dumps({k:v for k,v in result.items() if k not in ('cells','sha256')}), flush=True)


if __name__ == '__main__':
    main()
