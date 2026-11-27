"""Validate one terminal capability run and report from saved files."""

from argparse import ArgumentParser
import os
import subprocess
import sys

from run_cl2 import NIGHT, DEF1, INPUT_FILES, ORDER, RANKS, THREADS, checked, load, pin, save

# The postprocessors follow the same single-thread limit as the capability launcher.
os.environ.update(dict.fromkeys(THREADS, '1'))
from validate import validate
from report_cl2 import write_report
from runtime_checks import annotate_native, marker_faces


def rank_peaks(case, terminal):
    rows = terminal['rank_waits']
    if [row['rank'] for row in rows] != list(range(RANKS)):
        return None
    for row in rows:
        rank = row['rank']
        root = case / 'rank_receipts'
        wait = root / f'rank{rank}_wait.json'
        start = root / f'rank{rank}_start.json'
        if not start.is_file() or not wait.is_file():
            return None
        child = load(start).get('child')
        if not child or child['pid'] != row['pid'] or child != row['child'] or load(wait) != row:
            return None
    return [row['peak_rss_bytes'] for row in rows]


def measure(case):
    script = checked(DEF1 / 'inputs/shock_refinement_measure.py', ORDER['shock_refinement_measure.py'])
    pairs = []
    for cycle in range(5):
        mesh = case / ('m6.su2' if cycle == 0 else f'adapted_mesh_adap_{cycle:05}.su2')
        restart = case / f'restart_flow_adap_{cycle:05}.dat'
        if all(path.is_file() and path.stat().st_size for path in (mesh, restart)):
            pairs.append((cycle, mesh, restart))
    output = case / 'shock_measure.jsonl'
    receipt = case / 'shock_measure_receipt.json'
    if receipt.is_file():
        checked(output, load(receipt)['output']['sha256'])
        return output
    argv = [sys.executable, '-B', str(script), *(str(path) for _, mesh, restart in pairs for path in (mesh, restart))]
    env = {**os.environ, **dict.fromkeys(('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS'), '1')}
    log = case / 'shock_measure.stderr.log'
    with output.open('w') as out, log.open('w') as err:
        code = subprocess.run(argv, cwd=case, env=env, stdout=out, stderr=err).returncode if pairs else None
    save(receipt, {'argv': argv, 'returncode': code, 'cycles': [cycle for cycle, _, _ in pairs],
                   'input_pairs': [[pin(mesh), pin(restart)] for _, mesh, restart in pairs],
                   'script': pin(script), 'output': pin(output), 'stderr': pin(log)})
    return output


def check_repeat(launch, result):
    original = launch.get('original_attempt')
    if original is None:
        return None
    pin = original['validation']
    saved = load(checked(pin['path'], pin['sha256']))
    old, new = saved['runtime_defaults'], result['runtime_defaults']
    checks = []
    if launch['mode'] == 'CL2':
        for name in ('norm', 'hgrad'):
            if old[name]['status'] == 'FAILED':
                checks.append({'check': name, 'status': new[name]['status'], 'reason': new[name]['reason']})
    else:
        for defect in old['defects']:
            cycle, name = defect['cycle'], defect['check']
            check = new['norm'] if cycle == 0 else result['events'][cycle - 1]['checks'][name]
            checks.append({'cycle': cycle, 'check': name, 'status': check['status'], 'reason': check['reason']})
    assert checks
    status = 'FAILED' if new['wiring_defect'] else 'passed' if all(row['status'] == 'passed' for row in checks) else 'OPEN'
    return {'status': status, 'required_checks': checks}


def finish(mode, repeat):
    case = NIGHT / mode / ('repeat/run' if repeat else 'run')
    assert os.nice(0) == 19
    launch, terminal = (load(case / name) for name in ('launch.json', 'terminal.json'))
    assert launch['mode'] == mode
    assert terminal['all_owned_handles_terminal'], terminal.get('live_handles')
    stable = terminal['inputs_verified_after']
    for path, digest in [(case / 'run.cfg', launch['config_sha256']), (launch['binary'], launch['binary_sha256']),
                         *((case / name, launch['original_inputs'][name]['sha256']) for name in INPUT_FILES)]:
        assert stable[str(path)] and checked(path, digest)
    complete = terminal['cause'] == 'natural' and terminal['returncode'] == 0
    peaks = rank_peaks(case, terminal)
    if mode == 'CL2':
        result = annotate_native(validate(case, terminal_peaks=peaks, complete=complete), case)
        for cycle in result['cycles']:
            cycle['marker_faces'] = marker_faces(case / cycle['mesh'])
    else:
        sys.path.insert(0, str(NIGHT / 'MMG2'))
        from validate_mmg2 import validate_mmg2
        result = validate_mmg2(case, terminal_peaks=peaks)
    if repeat_result := check_repeat(launch, result):
        result['runtime_repeat'] = repeat_result
    save(case / 'validation.json', result)
    metrics = measure(case)
    report = write_report(case, launch=launch, events=result, terminal=terminal, measure=metrics)
    print(report['label'])


def main(mode):
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--repeat', action='store_true', help='Read the one authorized R5 correction run')
    finish(mode, parser.parse_args().repeat)


if __name__ == '__main__':
    main('CL2')
