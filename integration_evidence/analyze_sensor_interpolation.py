"""Compare SPD interpolation on frozen 2D sensor fields; no production policy is changed.

Edge samples are diagnostics, not a continuous gradation certificate. The centroid
integral is a common sensor-only estimator, not geometric-BL complexity quadrature.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'TestCases/adaptation/capability'))
import capcheck
from audit_native_metric_integration import load, tensor


def spectral(a, function):
    values, vectors = np.linalg.eigh(a)
    return (vectors * function(values)[..., None, :]) @ np.swapaxes(vectors, -1, -2)


def encode(a, method):
    if method == 'p1':
        return a
    if method == 'log':
        return spectral(a, np.log)
    return spectral(a, lambda values: values ** (-1 if method == 'inverse' else -.5))


def decode(a, method):
    if method == 'p1':
        return a
    if method == 'log':
        return spectral(a, np.exp)
    return spectral(a, lambda values: values ** (-1 if method == 'inverse' else -2))


def transport(dst, src, delta, growth):
    distance = np.sqrt(np.maximum(0, np.einsum('...i,...ij,...j->...', delta, src, delta)))
    inverse = spectral(dst, lambda values: 1 / np.sqrt(values))
    demand = src / (1 + growth * distance)[..., None, None] ** 2
    return np.linalg.eigvalsh(inverse @ demand @ inverse)[..., -1]


def p1_endpoint_slopes(points, metric, edges, growth):
    delta = points[edges[:, 1]]-points[edges[:, 0]]
    difference = metric[edges[:, 1]]-metric[edges[:, 0]]
    ratios = []
    for endpoint in (0, 1):
        value = metric[edges[:, endpoint]]
        inverse = spectral(value, lambda values: 1/np.sqrt(values))
        slope = np.max(np.abs(np.linalg.eigvalsh(inverse @ difference @ inverse)), axis=1)
        distance = np.sqrt(np.einsum('ni,nij,nj->n', delta, value, delta))
        ratios.append(slope / (2*growth*distance))
    ratios = np.stack(ratios, axis=1)
    index = np.unravel_index(np.argmax(ratios), ratios.shape)
    return {'scope': 'Necessary infinitesimal transport condition for affine P1 edges; not a continuous certificate.',
            'maximum_slope_over_allowed_slope': float(ratios.max()),
            'endpoint_edge_pairs_above_1_plus_1e_5': int((ratios > 1+1e-5).sum()),
            'worst_edge_gids': edges[index[0]].tolist(), 'worst_endpoint_gid': int(edges[index[0], index[1]])}


def self_check():
    # Isotropic endpoint sizes satisfy the exact linear-size envelope, while
    # componentwise and log interpolation fail on its last quarter segment.
    growth, fine, length = np.log(1.3), .01, 1.
    coarse = fine + growth * length
    ends = np.array([np.eye(2) / fine ** 2, np.eye(2) / coarse ** 2])
    delta = np.array([length, 0.])
    assert max(transport(ends[1], ends[0], delta, growth),
               transport(ends[0], ends[1], delta, growth)) <= 1 + 1e-12
    slopes = p1_endpoint_slopes(np.array([[0., 0.], [length, 0.]]), ends, np.array([[0, 1]]), growth)
    expected = (ends[0, 0, 0]-ends[1, 0, 0]) / (2*growth*length*ends[1, 0, 0]**1.5)
    assert np.isclose(slopes['maximum_slope_over_allowed_slope'], expected, rtol=1e-12)
    assert slopes['endpoint_edge_pairs_above_1_plus_1e_5'] == 1
    t = np.linspace(0, 1, 5)
    for method in ('p1', 'log', 'inverse', 'inverse_root'):
        transformed = encode(ends, method)
        samples = decode((1-t)[:, None, None] * transformed[0] + t[:, None, None] * transformed[1], method)
        assert np.allclose(samples[[0, -1]], ends, rtol=1e-12, atol=1e-12)
        ratios = transport(samples[1:], samples[:-1], delta / 4, growth)
        if method == 'inverse_root':
            assert ratios.max() <= 1 + 1e-12
        elif method in ('p1', 'log'):
            assert ratios.max() > 1.01
    # Noncommuting tensors check reconstruction and orthogonal covariance.
    a = np.array([[[5., 1.], [1., 2.]], [[3., -.5], [-.5, 7.]]])
    rotation = np.array([[.6, -.8], [.8, .6]])
    for method in ('p1', 'log', 'inverse', 'inverse_root'):
        assert np.allclose(decode(encode(a, method), method), a, rtol=1e-12, atol=1e-12)
        value = decode(encode(a, method).mean(axis=0), method)
        rotated = decode(encode(rotation @ a @ rotation.T, method).mean(axis=0), method)
        assert np.allclose(rotated, rotation @ value @ rotation.T, rtol=1e-12, atol=1e-12)


def analyze(mesh_path, fields_path, hgrad, subdivisions):
    mesh = capcheck.read_su2(mesh_path)
    assert mesh.P.shape[1] == 2 and mesh.E.shape[1] == 3, 'Only 2D triangular donors are supported.'
    fields = load(fields_path)
    assert np.allclose(np.column_stack([fields['x'], fields['y']]), mesh.P, rtol=0, atol=1e-12)
    metric = tensor(fields)
    assert np.isfinite(metric).all() and np.linalg.eigvalsh(metric).min() > 0
    edges = np.unique(np.sort(np.vstack([mesh.E[:, [0, 1]], mesh.E[:, [1, 2]], mesh.E[:, [2, 0]]]), axis=1), axis=0)
    growth = np.log(hgrad)
    fractions = np.linspace(0, 1, subdivisions+1)
    xyz = mesh.P[mesh.E]
    u, v = xyz[:, 1]-xyz[:, 0], xyz[:, 2]-xyz[:, 0]
    areas = .5 * np.abs(u[:, 0]*v[:, 1]-u[:, 1]*v[:, 0])
    baseline = metric[mesh.E].mean(axis=1)
    baseline_integral = float(areas @ np.sqrt(np.linalg.det(baseline)))
    result = {}
    for method in ('p1', 'log', 'inverse', 'inverse_root'):
        encoded = encode(metric, method)
        count, maximum, domination_count, domination_min = 0, 0., 0, 1.
        worst = None
        for begin in range(0, len(edges), 2048):
            selected = edges[begin:begin+2048]
            a, b = metric[selected[:, 0]], metric[selected[:, 1]]
            alpha = fractions[None, :, None, None]
            linear = (1-alpha)*a[:, None] + alpha*b[:, None]
            sampled = decode((1-alpha)*encoded[selected[:, 0], None] + alpha*encoded[selected[:, 1], None], method)
            assert np.isfinite(sampled).all() and np.linalg.eigvalsh(sampled).min() > 0
            inv = spectral(linear, lambda values: 1/np.sqrt(values))
            domination = np.linalg.eigvalsh(inv @ sampled @ inv)[..., 0]
            domination_min = min(domination_min, float(domination.min()))
            domination_count += int((domination < 1-1e-7).sum())
            step = (mesh.P[selected[:, 1]]-mesh.P[selected[:, 0]])[:, None] / subdivisions
            for reverse in (False, True):
                dst, src = (sampled[:, :-1], sampled[:, 1:]) if reverse else (sampled[:, 1:], sampled[:, :-1])
                ratios = transport(dst, src, -step if reverse else step, growth)
                count += int((ratios > 1+1e-5).sum())
                index = np.unravel_index(np.argmax(ratios), ratios.shape)
                value = float(ratios[index])
                if value > maximum:
                    maximum = value
                    edge, segment = index
                    start, end = (segment+1, segment) if reverse else (segment, segment+1)
                    worst = {'edge_gids': selected[edge].tolist(), 'source_fraction': float(fractions[start]),
                             'destination_fraction': float(fractions[end])}
        centroid = decode(encoded[mesh.E].mean(axis=1), method)
        result[method] = {'maximum_sampled_transport_ratio': maximum,
                          'directed_samples_above_1_plus_1e_5': count,
                          'minimum_domination_of_P1': domination_min,
                          'samples_weaker_than_P1_by_1e_7': domination_count,
                          'centroid_sensor_integral_relative_to_P1': float(areas @ np.sqrt(np.linalg.det(centroid))) / baseline_integral,
                          'worst_sample': worst}
    return {'scope': __doc__.strip(), 'hgrad': hgrad, 'subdivisions': subdivisions,
            'points': len(metric), 'edges': len(edges), 'directed_sample_count': 2*subdivisions*len(edges),
            'P1_endpoint_slopes': p1_endpoint_slopes(mesh.P, metric, edges, growth),
            'self_checks_passed': True, 'methods': result,
            'inputs_sha256': {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in (mesh_path, fields_path)}}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mesh', type=Path)
    parser.add_argument('fields', type=Path)
    parser.add_argument('--hgrad', type=float, required=True)
    parser.add_argument('--subdivisions', type=int, default=4)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not np.isfinite(args.hgrad) or args.hgrad <= 1 or args.subdivisions < 1:
        parser.error('hgrad must be finite and greater than one; subdivisions must be positive.')
    self_check()
    report = analyze(args.mesh, args.fields, args.hgrad, args.subdivisions)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report['methods'], indent=2))
