"""Independently audit every accepted airfoil cycle after the sequential MPI run ends."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('directory', type=Path)
parser.add_argument('--label', required=True)
args = parser.parse_args()
directory = args.directory.resolve(strict=True)
root = Path(__file__).resolve().parent
runtime = json.loads((directory/'evidence.json').read_text())
if runtime.get('current_run') or not runtime.get('archived_binary'):
    raise RuntimeError('The MPI runner must finish and archive its executable before auditing.')
if [r['ranks'] for r in runtime['runs']] != [1, 2, 4] or not all(r['verified'] for r in runtime['runs']):
    raise RuntimeError('Full airfoil evidence requires successful sequential MPI1/2/4 runs.')
destination = directory/args.label
destination.mkdir(exist_ok=False)
scripts = ('run_native_airfoil_audits.py', 'audit_native_airfoil.py', 'audit_native_bl.py',
           'frozen_field_audit.py', 'airfoil_reference_audit.py')
(destination/'sources').mkdir()
for script in scripts:
    shutil.copy2(root/script, destination/'sources'/script)
record = {'runner_pid': os.getpid(), 'runtime_evidence': str(directory/'evidence.json'),
          'source_sha256': {p: hashlib.sha256((root/p).read_bytes()).hexdigest() for p in scripts}, 'runs': []}
state = destination/'evidence.json'
all_ok = True
for ranks in (1, 2, 4):
    for cycle in range(3):
        output = destination/f'np{ranks}_cycle{cycle}.json'
        command = [sys.executable, str(destination/'sources/audit_native_airfoil.py'),
                   str(directory/f'audit_np{ranks}'), '--cycle', str(cycle), '--height', '.0002',
                   '--suffix', 'adapted', '--reference', str(directory/'airfoil_input.su2'), '--output', str(output)]
        with (destination/f'np{ranks}_cycle{cycle}.log').open('x') as log:
            child = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                     env=dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1'))
            record['current_run'] = {'ranks': ranks, 'cycle': cycle, 'pid': child.pid, 'command': command}
            state.write_text(json.dumps(record, indent=2)+'\n')
            code = child.wait()
        checks = {}
        if code == 0:
            row = json.loads(output.read_text())
            checks = {key: row[key] for key in ('all_exact_positive', 'manifold_oriented_edges', 'physical_equals_exposed')}
            checks.update(annulus_euler=row['disk_euler'] == 0,
                          quality=math.isfinite(row['min_quality']) and row['min_quality'] >= .18,
                          length=math.isfinite(row['max_simpson_length']) and row['max_simpson_length'] <= 1.8,
                          first_height=math.isfinite(row['max_relative_height_error']) and row['max_relative_height_error'] <= 1e-8,
                          no_residuals=not row['bad_cells'],
                          reference=all(r['all_declared_features_exactly_retained'] and
                                        abs(r['covered_arc_over_period']-1) <= 1e-12 and
                                        r['original_components'] == r['candidate_components'] and
                                        r['max_corresponding_parameter_deviation'] <= 1e-6 and
                                        r['max_vertex_projection_distance'] <= 1e-6 and
                                        (r['leading_extremum_shift'] is None or r['leading_extremum_shift'] == 0)
                                        for r in row['original_reference_checks']))
        passed = code == 0 and bool(checks) and all(checks.values())
        all_ok &= passed
        record['runs'].append({'ranks': ranks, 'cycle': cycle, 'exit': code, 'verified': passed,
                               'checks': checks, 'output': str(output)})
        record.pop('current_run', None)
        state.write_text(json.dumps(record, indent=2)+'\n')
        print(f'np{ranks} cycle{cycle}: {"PASS" if passed else "FAIL"}', flush=True)
record.update(terminal=True, verified=all_ok)
state.write_text(json.dumps(record, indent=2)+'\n')
raise SystemExit(0 if all_ok else 1)
