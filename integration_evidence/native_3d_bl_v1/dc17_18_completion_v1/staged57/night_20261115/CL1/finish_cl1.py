"""Validate the sole CL1 run and publish its capability report from saved files."""

import os
import subprocess
import sys

from run_cl1 import CASE, HERE, INPUT_FILES, ORDER, RANKS, checked, load, pin, save
from validate import validate
from report_cl1 import write_report


def rank_peaks(terminal):
    rows = terminal['rank_waits']
    if [row['rank'] for row in rows] != list(range(RANKS)):
        return None
    for row in rows:
        rank = row['rank']
        root = CASE / 'rank_receipts'
        wait = root / f'rank{rank}_wait.json'
        start = root / f'rank{rank}_start.json'
        if not start.is_file() or not wait.is_file():
            return None
        child = load(start).get('child')
        if not child or child['pid'] != row['pid'] or child != row['child'] or load(wait) != row:
            return None
    return [row['peak_rss_bytes'] for row in rows]


def measure():
    script = checked(HERE / 'inputs/shock_refinement_measure.py', ORDER['shock_refinement_measure.py'])
    pairs = []
    for cycle in range(5):
        mesh = CASE / ('m6.su2' if cycle == 0 else f'adapted_mesh_adap_{cycle:05}.su2')
        restart = CASE / f'restart_flow_adap_{cycle:05}.dat'
        if all(path.is_file() and path.stat().st_size for path in (mesh, restart)):
            pairs.append((cycle, mesh, restart))
    output = CASE / 'shock_measure.jsonl'
    receipt = CASE / 'shock_measure_receipt.json'
    if receipt.is_file():
        checked(output, load(receipt)['output']['sha256'])
        return output
    argv = [sys.executable, '-B', str(script), *(str(path) for _, mesh, restart in pairs for path in (mesh, restart))]
    env = {**os.environ, **dict.fromkeys(('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS'), '1')}
    log = CASE / 'shock_measure.stderr.log'
    with output.open('w') as out, log.open('w') as err:
        code = subprocess.run(argv, cwd=CASE, env=env, stdout=out, stderr=err).returncode if pairs else None
    save(receipt, {'argv': argv, 'returncode': code, 'cycles': [cycle for cycle, _, _ in pairs],
                   'input_pairs': [[pin(mesh), pin(restart)] for _, mesh, restart in pairs],
                   'script': pin(script), 'output': pin(output), 'stderr': pin(log)})
    return output


def finish():
    assert os.nice(0) == 19
    launch, terminal = (load(CASE / name) for name in ('launch.json', 'terminal.json'))
    assert terminal['all_owned_handles_terminal'], terminal.get('live_handles')
    stable = terminal['inputs_verified_after']
    for path, digest in [(CASE / 'run.cfg', launch['config_sha256']), (launch['binary'], launch['binary_sha256']),
                         *((CASE / name, launch['original_inputs'][name]['sha256']) for name in INPUT_FILES)]:
        assert stable[str(path)] and checked(path, digest)
    complete = terminal['cause'] == 'natural' and terminal['returncode'] == 0
    result = validate(CASE, terminal_peaks=rank_peaks(terminal), complete=complete)
    save(CASE / 'validation.json', result)
    metrics = measure()
    report = write_report(CASE, launch=launch, events=result, terminal=terminal, measure=metrics)
    print(report['label'])


if __name__ == '__main__':
    finish()
