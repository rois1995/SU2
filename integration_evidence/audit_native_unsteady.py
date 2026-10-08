"""Audit saved 2D Euler and straight-wall RANS window meshes against their actual frozen donor fields.

VTUs remain on the donor mesh; rewritten current/history restarts belong to the
new mesh. This audit uses both rather than assuming they share connectivity.
"""
import argparse
import hashlib
import json
import math
import re
import sys
from pathlib import Path
import numpy as np
from frozen_field_audit import FrozenField
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'TestCases/adaptation/capability'))
sys.path.insert(0, str(ROOT / 'TestCases/adaptation/custom_sensors'))
import capcheck
from run_cases import point_fields


def read_mesh(path):
    if path.suffix == '.su2': return capcheck.read_su2(path)
    import h5py
    with h5py.File(path) as f:
        zone = f['Base/Zone']
        points = np.column_stack([zone[f'GridCoordinates/Coordinate{axis}/ data'][()].ravel() for axis in ('X','Y')])
        cells, markers = [], {}
        for name, group in zone.items():
            if isinstance(group, h5py.Group) and group.attrs.get('label') == b'Elements_t':
                code = int(group[' data'][()].ravel()[0])
                ids = group['ElementConnectivity/ data'][()].ravel().astype(np.int64) - 1
                if code == 5: cells.extend(ids.reshape(-1, 3).tolist())
                elif code == 3: markers[name] = ids.reshape(-1, 2)
                else: raise ValueError(f'Unsupported CGNS element type {code}')
        return capcheck.Mesh(2, points, np.asarray(cells), markers)


def integral(mesh, values):
    areas = capcheck.volumes(mesh.P, mesh.E, 2)
    assert (areas > 0).all()
    return np.sum(areas[:, None] * values[mesh.E].mean(axis=1), axis=0)


def state(mesh, path):
    if path.suffix == '.dat':
        points, fields, _ = capcheck.read_restart(path)
        momenta = np.column_stack([fields['Momentum_x'], fields['Momentum_y']])
    else:
        points, fields, _ = capcheck.read_vtu(path)
        momenta = np.asarray(point_fields(path)['Momentum']).reshape(-1, 3)[:, :2]
    assert points.shape[0] == len(mesh.P) and np.max(np.abs(points[:, :2] - mesh.P)) < 1e-12 * mesh.size
    rho, energy = fields['Density'], fields['Energy']
    pressure = .4 * (energy - np.sum(momenta * momenta, axis=1) / (2 * rho))
    assert np.isfinite(rho).all() and np.isfinite(pressure).all() and min(rho) > 0 and min(pressure) > 0
    return np.column_stack([rho, momenta, energy]), dict(min_density=float(min(rho)), min_pressure=float(min(pressure)))


def audit(case, require_native_reference=True):
    cfg = (case / 'run.cfg').read_text()
    def setting(key): return re.search(r'^' + key + r'\s*=\s*(.*)', cfg, re.M)[1].strip()
    assert setting('SOLVER') in ('EULER', 'RANS') and setting('VOLUME_OUTPUT_PRECISION') == 'DOUBLE'
    if not require_native_reference:
        assert setting('ADAP_REMESHER') == 'MMG' and setting('SOLVER') == 'EULER', \
            'Reference-sidecar exemption is only for an explicit MMG Euler control'
    freq, steps = int(setting('ADAP_FREQ')), int(setting('TIME_ITER'))
    order = 2 if '2ND' in setting('TIME_MARCHING') else 1
    restart = re.search(r'^RESTART_SOL\s*=\s*YES\s*$', cfg, re.M) is not None
    start = int(setting('RESTART_ITER')) if restart else 0
    assert 0 <= start < steps and start % freq == 0, 'This audit covers complete windows from a window-boundary restart'
    meshes = {start: read_mesh(case / 'input.su2')}
    reference = meshes[start]
    if restart:
        origin = json.loads((case / 'restart_origin.json').read_text())
        assert origin['restart_step'] == start
        parent = Path(origin['parent'])
        reference = read_mesh(parent / 'input.su2')
        checkpoint = read_mesh(parent / origin['source_mesh'])
        assert np.array_equal(checkpoint.P, meshes[start].P) and np.array_equal(checkpoint.E, meshes[start].E)
        if require_native_reference:
            assert (case / 'input.su2.native_ref').is_file(), 'Retain original geometry reference across restart'
    rows = []
    for first in range(start + freq, steps, freq):
        mesh_path = case / f'mesh_{first:05d}.su2'
        if not mesh_path.exists(): mesh_path = case / f'mesh_{first:05d}.cgns'
        donor = meshes[first - freq]
        candidate = meshes[first] = read_mesh(mesh_path)
        metric_path = case / f'flow_{first - 1:05d}.vtu'
        metric, precision = capcheck.metric_of(donor, metric_path)
        assert precision == 'Float64'
        frozen = FrozenField(donor.P, donor.E, metric[:, (0, 0, 1), (0, 1, 1)], donor.M, float(setting('ADAP_HAUSD')))
        target = frozen
        height_error = None
        if setting('SOLVER') == 'RANS':
            from audit_native_composite_rae import intersection
            original = reference
            wall_edges = original.M['lower']
            assert np.max(np.abs(original.P[wall_edges, 1])) < 1e-14
            spans = np.linalg.norm(original.P[wall_edges[:, 1]] - original.P[wall_edges[:, 0]], axis=1)
            assert np.ptp(spans) < 1e-14
            ht = float(spans[0]); h = float(setting('ADAP_BL_FIRST_HEIGHT').strip('( )'))
            g, thickness = float(setting('ADAP_BL_GROWTH')), float(setting('ADAP_BL_THICKNESS'))
            core = float(np.min(np.linalg.eigvalsh(metric)))
            def target(point):
                xx, xy, yy = frozen(point)
                d = abs(point[1]);full = max(h, .9 * thickness)
                fade = float(np.clip((d - full) / (thickness - full), 0, 1))
                weight = 1 - fade * fade * (3 - 2 * fade)
                if weight == 0: return xx, xy, yy
                hn = max(h, 2 * (h + (g - 1) * d) / (g + 1))
                wall = np.diag([math.exp(weight * math.log(1 / ht**2) + (1 - weight) * math.log(core)),
                                math.exp(weight * math.log(1 / hn**2) + (1 - weight) * math.log(core))])
                out = intersection(np.array([[xx, xy], [xy, yy]]), wall)
                return float(out[0, 0]), float(out[0, 1]), float(out[1, 1])
            incident = {tuple(sorted(e)): t for t in candidate.E for e in (t[[0,1]], t[[1,2]], t[[2,0]])}
            heights = []
            for e in candidate.M['lower']:
                t = incident[tuple(sorted(e))];a,b = candidate.P[e]
                v = candidate.P[next(i for i in t if i not in e)]
                height = abs((b[0]-a[0])*(v[1]-a[1])-(b[1]-a[1])*(v[0]-a[0])) / np.linalg.norm(b-a)
                heights.append(abs(height/h - 1))
            height_error = max(heights)
            assert height_error < 1e-8
        minq, maxl = 1., 0.
        for ids in candidate.E:
            p = candidate.P[ids]; xx, xy, yy = target(tuple(p.mean(axis=0)))
            d = np.roll(p, -1, axis=0) - p
            area2 = (p[1, 0] - p[0, 0]) * (p[2, 1] - p[0, 1]) - (p[1, 1] - p[0, 1]) * (p[2, 0] - p[0, 0])
            assert area2 > 0
            minq = min(minq, float(2 * math.sqrt(3) * area2 * math.sqrt(xx * yy - xy * xy) /
                                  np.sum(xx * d[:, 0]**2 + 2 * xy * d[:, 0] * d[:, 1] + yy * d[:, 1]**2)))
        edges = np.unique(np.sort(np.vstack([candidate.E[:, [0, 1]], candidate.E[:, [1, 2]], candidate.E[:, [2, 0]]]), axis=1), axis=0)
        for ids in edges:
            a, b = candidate.P[ids]; d = b - a; lengths = []
            for p in (a, (a + b) / 2, b):
                xx, xy, yy = target(tuple(p))
                lengths.append(math.sqrt(xx * d[0]**2 + 2 * xy * d[0] * d[1] + yy * d[1]**2))
            maxl = max(maxl, (lengths[0] + 4 * lengths[1] + lengths[2]) / 6)
        assert minq >= .18 - 1e-8 and maxl <= 1.8 + 1e-8
        history = []
        for index in range(first - order, first):
            donor_state_path = case / f'flow_{index:05d}.vtu'
            new, admissibility = state(candidate, case / f'solution_{index:05d}.dat')
            if not donor_state_path.exists():
                history.append(dict(step=index, relative_integral_defect=None, donor_snapshot_available=False, **admissibility))
                continue
            old, _ = state(donor, donor_state_path)
            before, after = integral(donor, old), integral(candidate, new)
            # Momentum can integrate to zero: normalize by the integral of |U|.
            norm = np.maximum(integral(donor, np.abs(old)), 1e-14)
            defect = float(np.max(np.abs(after - before) / norm))
            assert defect < 1e-10
            history.append(dict(step=index, relative_integral_defect=defect, donor_snapshot_available=True, **admissibility))
        if require_native_reference:
            assert (case / (mesh_path.name + '.native_ref')).is_file()
        rows.append(dict(first_step=first, points=len(candidate.P), triangles=len(candidate.E), min_quality=minq,
                         max_simpson_length=maxl, donor_metric=metric_path.name, history=history, relative_first_height_error=height_error))
    final = meshes[max(meshes)]
    _, final_admissibility = state(final, case / f'solution_{steps - 1:05d}.dat')
    log = (case / 'solver.log').read_text()
    assert 'Exit Success' in log
    events = log.count('Native adaptation:' if require_native_reference else 'Mesh Adaptation Cycle')
    assert events == len(rows)
    return dict(status='PASS', restart_step=start if restart else None, scope='2D ideal-gas Euler or constant-span straight-wall RANS, shared fade and geometric BL composition when present, double saved metrics, original-connectivity P1 target, current/history conservation, final positivity and restart pairing; not a flow-accuracy or cost certificate', windows=rows, final_admissibility=final_admissibility,
                checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                input_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in case.iterdir() if p.suffix in ('.cfg','.su2','.dat','.vtu')})


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('cases', type=Path, nargs='+');args=parser.parse_args()
    for case in args.cases:
        output = case / 'independent_unsteady_audit.json'
        assert not output.exists(), 'Preserve existing audit evidence'
        row = audit(case);output.write_text(json.dumps(row, indent=2)+'\n')
        print(case, row['status'], [(w['min_quality'], w['max_simpson_length']) for w in row['windows']], flush=True)
