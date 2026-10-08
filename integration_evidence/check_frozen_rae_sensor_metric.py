"""Audit frozen RAE sensor metrics with noise off/on; no flow or remeshing.

Usage: python3 integration_evidence/check_frozen_rae_sensor_metric.py CAMPAIGN [METHOD ...]
Uses the existing independent geometric BL reference. The reported complexity
is the production composed integral; nodal integration below is a diagnostic,
not an independent integral of the continuous geometric field.
"""
import json
import re
import sys
from pathlib import Path

import numpy as np
from audit_native_metric_integration import (
    GeometricWall, batch_wall, capcheck, load, original_wall, sha, stats, tensor, transport,
)


def audit(root, methods=('weighted_least_squares', 'quadratic_least_squares', 'quadratic_least_squares_noise')):
    config = (root / 'rae' / (methods[0] + '-mpi1') / 'run.cfg').read_text()
    meshpath = Path(re.search(r'^MESH_FILENAME= (.*)$', config, re.M)[1])
    restart = Path(re.search(r'^SOLUTION_FILENAME= (.*)$', config, re.M)[1] + '.dat')
    mesh = capcheck.read_su2(meshpath)
    _, original, _ = capcheck.read_restart(restart)
    points, faces, paths = original_wall(meshpath)
    edges = np.unique(np.sort(np.vstack([mesh.E[:, [0, 1]], mesh.E[:, [1, 2]],
                                        mesh.E[:, [2, 0]]]), axis=1), axis=0)
    report = {'scope': 'Frozen RANS/SA, zero flow/remesh iterations; sensor-only MPI, SPD, size/aspect and transport checks; independent nodal geometric BL composition. No composed-gradation, continuous-complexity or aerodynamic accuracy certificate.',
              'criteria': {'relative_metric_mpi': 1e-9, 'relative_hessian_mpi_with_global_floor': 1e-9,
                           'relative_solution': 1e-14, 'relative_reported_complexity': 1e-5,
                           'sensor_transport': 1 + 1e-5},
              'inputs': {str(p): sha(p) for p in paths + [restart]}, 'cases': {}, 'passed': True}
    for method in methods:
        base = load(root / 'rae' / (method + '-mpi1') / 'fields.csv')
        reference = tensor(base)
        value = np.linalg.eigvalsh(reference)
        core = float(np.min((reference[:, 0, 0].astype(np.longdouble) * reference[:, 1, 1] -
                             reference[:, 0, 1].astype(np.longdouble)**2) / value[:, 1]))
        wall = GeometricWall(points, faces, 1e-5, 1.2, .02, 45, core)
        composed, distance, bl = batch_wall(wall, mesh.P, reference)
        val, vec = np.linalg.eigh(reference)
        inverse = np.einsum('nai,ni,nbi->nab', vec, 1 / np.sqrt(val), vec)
        domination = float(np.linalg.eigvalsh(inverse @ composed @ inverse).min())
        active = distance < .02
        blval, blvec = np.linalg.eigh(bl[active])
        blinverse = np.einsum('nai,ni,nbi->nab', blvec, 1 / np.sqrt(blval), blvec)
        bl_domination = float(np.linalg.eigvalsh(blinverse @ composed[active] @ blinverse).min())
        combined_ratios = np.concatenate([transport(composed[a], composed[b], mesh.P[a] - mesh.P[b])
                                          for a, b in (edges.T, edges[:, ::-1].T)])
        entry = {'minimum_generalized_sensor_domination': domination,
                 'minimum_generalized_bl_domination': bl_domination,
                 'composed_nodal_transport_diagnostic': stats(combined_ratios), 'mpi': {}}
        for rank in (1, 2, 4):
            case = root / 'rae' / (method + '-mpi' + str(rank))
            cfg = (case / 'run.cfg').read_text()
            assert re.search(r'^ADAP_HGRAD= (.*)$', cfg, re.M)[1] == '1.3'
            data = load(case / 'fields.csv')
            metric = tensor(data)
            eigen = np.linalg.eigvalsh(metric)
            target = float(re.search(r'^ADAP_COMPLEXITY= (.*)$', cfg, re.M)[1])
            hmin = float(re.search(r'^ADAP_HMIN= (.*)$', cfg, re.M)[1])
            hmax = float(re.search(r'^ADAP_HMAX= (.*)$', cfg, re.M)[1])
            armax = float(re.search(r'^ADAP_ARMAX= (.*)$', cfg, re.M)[1])
            error = float(np.max(np.linalg.norm(metric - reference, axis=(1, 2)) /
                                 np.linalg.norm(reference, axis=(1, 2))))
            herrors = {}
            for sensor in ('MACH', 'PRESSURE'):
                h = tensor(base, 'Hessian_' + sensor)
                hh = tensor(data, 'Hessian_' + sensor)
                norm = np.linalg.norm(h, axis=(1, 2))
                herrors[sensor] = float(np.max(np.linalg.norm(hh - h, axis=(1, 2)) /
                                               np.maximum(norm, 1e-14 * norm.max())))
            states = {f: bool(np.allclose(data[f], original[f], rtol=1e-14, atol=0))
                      for f in ('Density', 'Momentum_x', 'Momentum_y', 'Energy', 'Nu_Tilde')}
            ratios = np.concatenate([transport(metric[a], metric[b], mesh.P[a] - mesh.P[b])
                                     for a, b in (edges.T, edges[:, ::-1].T)])
            complexity = float(re.search(r'Mesh complexity after native constraints: ([\d.e+\-]+)',
                                         (case / 'solver.log').read_text())[1])
            row = {'minimum_sensor_eigenvalue': float(eigen.min()),
                   'maximum_sensor_aspect_ratio': float(np.sqrt(eigen[:, 1] / eigen[:, 0]).max()),
                   'relative_metric_mpi': error, 'hessian_relative_with_global_floor': herrors,
                   'solution_matches_restart': states, 'sensor_nodal_transport': stats(ratios),
                   'reported_composed_complexity': complexity, 'fields_sha256': sha(case / 'fields.csv')}
            row['passed'] = bool(np.isfinite(metric).all() and eigen.min() > 0 and
                                 eigen.min() >= (1 - 1e-6) / hmax**2 and eigen.max() <= (1 + 1e-6) / hmin**2 and
                                 row['maximum_sensor_aspect_ratio'] <= (1 + 1e-6) * armax and
                                 np.allclose(np.column_stack([data['x'], data['y']]), mesh.P, rtol=0, atol=1e-12) and
                                 error < 1e-9 and all(np.isfinite(x) and x < 1e-9 for x in herrors.values()) and
                                 all(states.values()) and abs(complexity / target - 1) < 1e-5 and
                                 ratios.max() <= 1 + 1e-5 and domination >= 1 - 1e-6 and bl_domination >= 1 - 1e-6)
            entry['mpi'][str(rank)] = row
            report['passed'] &= row['passed']
        report['cases'][method] = entry
        print(method, {r: (x['relative_metric_mpi'], x['passed']) for r, x in entry['mpi'].items()}, flush=True)
    report['auditor_sha256'] = sha(__file__)
    (root / 'rae-validation.json').write_text(json.dumps(report, indent=2) + '\n')
    return report['passed']


if __name__ == '__main__':
    root = Path(sys.argv[1]).resolve()
    raise SystemExit(0 if (audit(root, tuple(sys.argv[2:])) if len(sys.argv) > 2 else audit(root)) else 1)
