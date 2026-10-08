"""Audit the saved matched cycle with the existing flow and geometric-BL checks."""
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import numpy as np

repo = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(repo / 'TestCases/adaptation/capability'))
import capcheck


def main(root):
    campaign = json.loads((root / 'campaign.json').read_text())
    assert campaign['phase'] == 'terminal'
    output = root / 'comparison.json'
    assert not output.exists(), 'Preserve prior analysis'
    result = dict(scope=campaign['scope'], cases={}, analysis_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    fields = {}
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1')
    for run in campaign['runs']:
        label = run['case']
        wd = root / label
        text = (wd / 'solver.log').read_text()
        row = dict(solver_exit=run['solver_exit'], process_seconds=run['elapsed_seconds'])
        row['hessian_seconds'] = list(map(float, re.findall(r'Hessian recovery: ([\d.e+\-]+)', text)))
        row['metric_seconds'] = list(map(float, re.findall(r'Metric computed in ([\d.e+\-]+)', text)))
        row['metric_work'] = re.findall(r'Native metric work: (.*)', text)
        row['sensor_gradation'] = re.findall(r'Native sensor gradation: (.*)', text)
        row['composed_gradation'] = re.findall(r'Native composed nodal audit: (.*)', text)
        row['native_summary'] = [line for line in text.splitlines() if line.startswith('Native ') or line.startswith('Conservative transfer')]
        restart = wd / 'solution_adap_00000.dat'
        if restart.exists():
            points, data, _ = capcheck.read_restart(restart)
            fields[label] = (points, data)
        row['accepted_mesh_written'] = (wd / 'mesh_adap_00001.su2').exists()
        row['rejected_meshes'] = [str(p) for p in wd.glob('*rejected.su2')]
        row['checks'] = []
        if run['solver_exit'] == 0 and row['accepted_mesh_written']:
            for checker, arguments in [('inspect_rae2822_solution.py', ['--cycles', '0', '1']),
                                       ('audit_native_composite_rae.py', ['--cycles', '1'])]:
                command = ['nice', '-n', '10', sys.executable, str(repo / 'integration_evidence' / checker), str(wd), *arguments]
                with (wd / (checker + '.log')).open('x') as log:
                    code = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT).returncode
                row['checks'].append(dict(checker=checker, exit_code=code))
                print(label, checker, code, flush=True)
            if (wd / 'inspection.json').exists():
                row['inspection'] = json.loads((wd / 'inspection.json').read_text())['cycles']
            if (wd / 'independent_composite_metric_00001.json').exists():
                audit = json.loads((wd / 'independent_composite_metric_00001.json').read_text())
                row['metric_audit'] = {key: value for key, value in audit.items() if key not in ('bad_quality_cells', 'bad_length_edges')}
                row['metric_audit']['bad_quality_cells'] = len(audit['bad_quality_cells'])
                row['metric_audit']['bad_length_edges'] = len(audit['bad_length_edges'])
            history = [{k.strip().strip('"'): v for k, v in item.items()} for item in csv.DictReader((wd / 'history.csv').open())]
            resumed = [item for item in history if int(item['Adap_Cycle']) == 1]
            row['resumed_flow'] = dict(iterations=len(resumed), first_density_residual=float(resumed[0]['rms[Rho]']),
                                      last_density_residual=float(resumed[-1]['rms[Rho]']),
                                      last_turbulence_residual=float(resumed[-1]['rms[nu]']),
                                      CL=float(resumed[-1]['CL']), CD=float(resumed[-1]['CD']))
        result['cases'][label] = row
        output.write_text(json.dumps(result, indent=2) + '\n')
    if set(fields) == {'baseline', 'improved'}:
        p, a = fields['baseline']
        q, b = fields['improved']
        assert np.array_equal(p, q)
        solution = ['Density', 'Momentum_x', 'Momentum_y', 'Energy', 'Nu_Tilde']
        assert all(np.array_equal(a[name], b[name]) for name in solution)
        differences = {}
        for prefix in ('Hessian_MACH', 'Hessian_PRESSURE', 'Metric'):
            x = np.column_stack([a[prefix + '_' + suffix] for suffix in ('XX', 'XY', 'YY')])
            y = np.column_stack([b[prefix + '_' + suffix] for suffix in ('XX', 'XY', 'YY')])
            norms = np.linalg.norm(x, axis=1)
            relative = np.linalg.norm(y-x, axis=1)/np.maximum(norms, 1e-14*norms.max())
            differences[prefix] = dict(relative_quantiles=np.quantile(relative, [0, .5, .95, .99, 1]).tolist())
        result['matched_pre_adaptation_flow_bitwise'] = True
        result['recovery_field_changes'] = differences
    result['accepted_and_independently_verified'] = all(row['solver_exit'] == 0 and row['accepted_mesh_written']
        and len(row['checks']) == 2 and all(check['exit_code'] == 0 for check in row['checks']) for row in result['cases'].values())
    result['both_flow_criteria_met'] = all('resumed_flow' in row and row['resumed_flow']['last_density_residual'] < -8
        and row['resumed_flow']['last_turbulence_residual'] < -8 for row in result['cases'].values())
    output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main(Path(sys.argv[1]).resolve())
