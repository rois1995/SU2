"""Run the remaining C2 cohorts sequentially with actual-host preflight and wait receipts."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def identity(pid):
    """Record a host process identity for later terminal checks."""
    fields = (Path('/proc') / str(pid) / 'stat').read_text().rsplit(')', 1)[1].split()
    return {'pid': pid, 'start_ticks': int(fields[19])}


def run_case(name, argv):
    """Launch one controller and preserve its actual wait result."""
    subprocess.run([sys.executable, str(preflight), str(root / f'{name}_host_preflight.json')], check=True)
    with (root / f'{name}_outer.log').open('x') as log:
        child = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT)
        handle = identity(child.pid)
        start = time.time()
        (root / f'{name}_launch.json').write_text(
            json.dumps({'controller': handle, 'supervisor': identity(os.getpid()), 'unix': start}, indent=2) + '\n')
        status = child.wait()
    receipt = {'controller': handle, 'returncode': status, 'elapsed_seconds': time.time() - start,
               'actual_wait': True}
    (root / f'{name}_wait.json').write_text(json.dumps(receipt, indent=2) + '\n')
    if status:
        raise SystemExit(f'{name} controller exit {status}. See {log.name}.')
    return receipt


root = Path(__file__).resolve().parent
phase = root.parents[1]
preflight = phase / 'end_B_preparation/primary_control_archive/end_b_actual_preflight.py'
assert (Path('/proc/1/comm').read_text().strip() == 'systemd'), 'requires actual-host /proc'
for cohort in ('serial', 'mpi2', 'mpi4'):
    run_case(f'G1_{cohort}', [sys.executable, str(root / 'run_g1_560.py'), '--cohort', cohort])
    result = json.loads((root / f'G1_candidate_{cohort}_summary.json').read_text())
    assert result['identity_pass'] and result['changed_case_count'] == 0
run_case('G2', [sys.executable, str(root / 'run_g2_gq_prefix_560.py'), '--gate', 'G2',
                '--case-name', 'C2_G2_560'])
comparison = root / 'cases/C2_G2_560/g2_build560_comparison.json'
result = json.loads(comparison.read_text())
assert result['passed'] and result['strict_old_integer_rows']
