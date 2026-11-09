"""Read-only bounded assessment of the terminal saved-state RANS continuation."""
from pathlib import Path
import csv
import hashlib
import json
import sys
import numpy as np

R = Path(__file__).resolve().parent
sys.path.insert(0, str(R.parents[3] / 'TestCases/adaptation/capability'))
import capcheck


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def span(values):
    values = np.asarray(values, dtype=float)
    assert values.size and np.isfinite(values).all()
    return dict(first=float(values[0]), last=float(values[-1]), min=float(values.min()),
                max=float(values.max()), mean=float(values.mean()),
                delta=float(values[-1] - values[0]),
                relative_span=float(np.ptp(values) / max(abs(values.mean()), 1e-300)))


def state(path, mesh):
    points, fields, _ = capcheck.read_restart(path)
    assert np.array_equal(points, mesh.P), f'coordinate mismatch: {path}'
    assert all(np.isfinite(v).all() for v in fields.values()), path
    rho = fields['Density']
    internal = fields['Energy'] - .5 * sum(fields[k] ** 2 for k in
        ('Momentum_x', 'Momentum_y', 'Momentum_z')) / rho
    pressure = .4 * internal
    nu = fields['Nu_Tilde']
    assert (rho > 0).all() and (internal > 0).all() and (pressure > 0).all()
    assert (nu >= 0).all()
    wall = np.unique(mesh.M['WING3D'])
    floor = np.flatnonzero(nu == 1e-16)
    return dict(sha256=sha(path), fields=list(fields), all_finite=True,
        coordinates_exact=True, density_range=[float(rho.min()), float(rho.max())],
        internal_energy_density_range=[float(internal.min()), float(internal.max())],
        derived_pressure_range=[float(pressure.min()), float(pressure.max())],
        sa_range=[float(nu.min()), float(nu.max())],
        sa_zero_count=int((nu == 0).sum()), sa_below_floor_count=int((nu < 1e-16).sum()),
        sa_floor_count=len(floor), wall_vertices=len(wall),
        sa_floor_on_wall=len(np.intersect1d(floor, wall)),
        sa_floor_off_wall=len(np.setdiff1d(floor, wall)),
        wall_sa_range=[float(nu[wall].min()), float(nu[wall].max())])


def assess(folder):
    status = json.loads((folder / 'status.json').read_text())
    assert status['state'] == 'terminal' and status['exit_code'] == 0
    assert status['pins_verified'] and status['mapped_runtime_verified']
    assert json.loads((folder / 'terminal_handles.json').read_text())['all_owned_handles_terminal']
    pins = json.loads((folder / 'pins.json').read_text())
    assert all(sha(path) == digest for path, digest in pins.items())
    prepared = json.loads((R / 'm6_inputs/rans_seed_continuation303_v1/provenance.json').read_text())
    origin = Path(prepared['original_directory'])
    assert all(sha(origin / name) == digest for name, digest in prepared['original_sha256'].items())
    mesh = capcheck.read_su2(folder / 'Coarse.su2')
    before = state(folder / 'baseline_sa_seed.dat', mesh)
    after = state(folder / 'restart_flow.dat', mesh)
    with (folder / 'history.csv').open() as stream:
        rows = [{k.strip().strip('"'): float(v) for k, v in row.items()}
                for row in csv.DictReader(stream)]
    assert len(rows) == 80 and [r['Inner_Iter'] for r in rows] == list(range(80))
    assert all(np.isfinite(list(row.values())).all() for row in rows)
    for field in ('CL', 'CD', 'rms[Rho]', 'rms[nu]'):
        assert field in rows[0], field
    trajectory = {field: dict(full=span([r[field] for r in rows]),
        last20=span([r[field] for r in rows[-20:]]),
        last10=span([r[field] for r in rows[-10:]]))
        for field in ('CL', 'CD', 'rms[Rho]', 'rms[nu]')}
    surface = {}
    for label, directory in [('original', origin), ('continued', folder)]:
        path = directory / 'surface_flow.vtu'
        if not path.exists():
            surface[label] = dict(available=False)
            continue
        points, fields, precision = capcheck.read_vtu(path)
        assert np.isfinite(points).all() and all(np.isfinite(v).all() for v in fields.values())
        selected = {k: dict(min=float(v.min()), max=float(v.max())) for k, v in fields.items()
                    if 'mach' in k.lower() or 'pressure' in k.lower()}
        # Quantized coordinates must map uniquely to the original requested wall.
        digits = 6 if precision == 'Float32' else 13
        wall = mesh.P[np.unique(mesh.M['WING3D'])]
        keys = {tuple(x) for x in np.round(wall, digits)}
        matches = sum(tuple(x) in keys for x in np.round(points, digits))
        surface[label] = dict(available=True, sha256=sha(path), points=len(points),
            precision=precision, finite=True, fields=list(fields), selected_ranges=selected,
            wall_coordinate_matches=matches,
            scope='Nodal field ranges and coordinate association only; no spatial shock/separation or Cp-distribution qualification.')
    log = (folder / 'run.log').read_text(errors='replace')
    messages = [line for line in log.splitlines() if any(word in line.lower()
                for word in ('clipp', 'nonphysical', 'negative density', 'negative pressure', 'nan detected'))]
    return dict(status='ADMISSIBLE_CONTINUATION_ONLY_REASONABLE_FLOW_UNQUALIFIED',
        scope='80-step same-mesh saved RANS/SA continuation; no adaptation, physical convergence, external validation or mesh-resolution qualification.',
        mesh=dict(points=len(mesh.P), tets=len(mesh.E), sha256=sha(folder / 'Coarse.su2')),
        original_inputs_unchanged=True, pins_verified=True, initial=before, continued=after,
        history_rows=len(rows), trajectory=trajectory, surface=surface,
        solver_bounds_messages=messages,
        bounds_correction_ledger='Not independently available; wall floor values and absence of log messages do not prove zero corrections or exact SA integral conservation.',
        remaining='Parent must assess force trajectory and spatial pressure/shock/separation and wall-resolution evidence. Stability/admissibility alone do not qualify reasonable flow. Mesh underresolution is not established by these checks.',
        helper_sha256=sha(__file__), output_sha256={name:sha(folder/name) for name in
            ('restart_flow.dat', 'history.csv', 'run.log', 'status.json', 'mapped_runtime_pins.json')})


if __name__ == '__main__':
    folder = R / 'm6_runs/rans_seed_continuation303_v1'
    report = assess(folder)
    output = folder / 'continuation303_independent_assessment.json'
    assert not output.exists(), 'Preserve prior assessment; review before rerunning.'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
