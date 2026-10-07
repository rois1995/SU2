"""Check actual ideal-gas BDF2 remesh checkpoints and optional unchanged CFD continuation."""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'TestCases/adaptation/capability'))
import capcheck


def check(case, baseline=None):
    cfg = (case / 'run.cfg').read_text()
    def setting(key, default=None):
        match = re.search(r'^' + key + r'\s*=\s*([^%\n]+)', cfg, re.M)
        return match[1].strip() if match else default
    assert '2ND' in setting('TIME_MARCHING')
    assert setting('WRT_RESTART_COMPACT') == 'NO'
    fluid = setting('FLUID_MODEL', 'STANDARD_AIR')
    assert fluid in ('STANDARD_AIR', 'IDEAL_GAS')
    gamma = 1.4 if fluid == 'STANDARD_AIR' else float(setting('GAMMA_VALUE', '1.4'))
    gas = 287.058 if fluid == 'STANDARD_AIR' else float(setting('GAS_CONSTANT', '287.058'))
    rows, inputs = [], [case / 'run.cfg']
    freq, steps = int(setting('ADAP_FREQ')), int(setting('TIME_ITER'))
    for first in range(freq, steps, freq):
        for step in (first - 2, first - 1):
            path = case / f'solution_{step:05d}.dat'
            _, fields, _ = capcheck.read_restart(path)
            inputs.append(path)
            rho = fields['Density']
            mom = np.column_stack([fields['Momentum_x'], fields['Momentum_y']])
            velocity = mom / rho[:, None]
            pressure = (gamma - 1) * (fields['Energy'] - np.sum(mom * mom, axis=1) / (2 * rho)
                                        - rho * fields.get('Turb_Kin_Energy', 0))
            assert np.isfinite(pressure).all() and pressure.min() > 0
            expected = {'Pressure': pressure, 'Temperature': pressure / (rho * gas),
                        'Mach': np.linalg.norm(velocity, axis=1) / np.sqrt(gamma * pressure / rho),
                        'Velocity_x': velocity[:, 0], 'Velocity_y': velocity[:, 1]}
            errors = {key: float(np.max(np.abs(fields[key] - value)) / max(1e-30, np.max(np.abs(value))))
                      for key, value in expected.items()}
            assert max(errors.values()) < 1e-10, (path, errors)
            rows.append(dict(step=step, primitive_relative_errors=errors))
    result = dict(status='PASS', scope='Ideal-gas rewritten BDF2 pressure, temperature, Mach and velocity; wall-derived outputs and metric diagnostics excluded', checkpoints=rows)
    if baseline:
        path = case / f'solution_{steps - 1:05d}.dat'
        old = baseline / path.name
        p, f, _ = capcheck.read_restart(path)
        q, g, _ = capcheck.read_restart(old)
        assert np.array_equal(p, q)
        primary = ['Density', 'Momentum_x', 'Momentum_y', 'Energy']
        primary += [key for key in ('Nu_Tilde', 'Turb_Kin_Energy', 'Omega') if key in f]
        errors = {key: float(np.max(np.abs(f[key]-g[key])) / max(1e-30, np.max(np.abs(g[key])))) for key in primary}
        assert max(errors.values()) < 1e-12, errors
        result['continued_primary_relative_errors'] = errors
        for path in sorted(case.glob('mesh_*.su2')):
            prior = baseline / path.name
            assert path.read_bytes() == prior.read_bytes()
            inputs.extend((path, prior))
        inputs.extend((case / f'solution_{steps - 1:05d}.dat', old))
    inputs.append(Path(__file__))
    result['inputs_sha256'] = {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    (case / 'checkpoint_primitive_audit.json').write_text(json.dumps(result, indent=2)+'\n')
    print(case, 'checkpoint primitives PASS')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path)
    parser.add_argument('--baseline', type=Path)
    args = parser.parse_args()
    check(args.case.resolve(), args.baseline.resolve() if args.baseline else None)
