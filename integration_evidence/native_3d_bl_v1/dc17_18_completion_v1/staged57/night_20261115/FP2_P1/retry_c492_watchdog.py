"""Preserve the shared C492 wrapper timeout and repeat once with the inherited limit."""
from argparse import ArgumentParser
from pathlib import Path
import re
import runpy
import sys

import run_shared as shared


def wrapper_timeout(root):
    cut = shared.cut
    case, = (root / 'cases').iterdir()
    result = cut.read(case / 'status.json')
    if result['exit_code'] != 124:
        return None
    command = cut.read(case / 'command.json')
    wait = cut.read(root / 'controller_wait.json')
    process = cut.read(case / 'safeguard_process_receipt.json')
    assert wait['terminal'] and cut.terminal(wait['controller'])
    assert process['state'] == 'terminal' and not process['live_owned_descendants']
    assert all(cut.terminal(r) for r in process['identities'])
    assert result['pins_verified'] and not result['changed']
    assert command['argv'][command['argv'].index('timeout') + 1] == '600'
    with (case / 'run.log').open(errors='replace') as rows:
        text = rows.read()
    if re.search(r'test cases:.*failed|assertions:.*failed|Curved atlas (?:work|storage) exhausted|\bFAILED:', text):
        return None
    if process['peak_sampled_tree_rss_bytes'] > shared.LIMIT:
        return None
    for line in text.splitlines():
        if 'NATIVE3D_STAGE ' in line and 'stage=exit ' in line:
            fields = dict(re.findall(r'(\w+)=(\S+)', line))
            if int(fields['upper_refinements']) or int(fields['length_splits']) < 300:
                return None
    if (case / 'actual_saved_surface_stage_rank0.json').is_file():
        row = cut.read(case / 'actual_saved_surface_stage_rank0.json')
        gate = runpy.run_path(str(shared.PURPOSE / 'check_bl_regression.py'))
        tests = gate['conditions'](result, command, row, dict.fromkeys(gate['FILES'], True))
        tests.pop('executed_successfully')
        if not all(tests.values()):
            return None
    assert 'Exit status: 124' in text
    return {'seconds': 600, 'status': str(case / 'status.json'), 'status_sha256': cut.sha(case / 'status.json'),
            'log': str(case / 'run.log'), 'log_sha256': cut.sha(case / 'run.log')}


def run(label):
    cut = shared.cut
    shared.verify_build(label)
    build = shared.HERE / f'build{label}'
    group = shared.HERE / 'shared' / build.name / 'closure1/exact-c492'
    retry = group / 'watchdog_retry'
    path = group / 'case_summary.json'
    record = cut.read(path)
    initial = wrapper_timeout(group / 'bl')
    assert initial is not None, 'The first check has a result finding or ran to completion.'
    retry.mkdir(exist_ok=True)
    if not (retry / 'controller_wait.json').is_file():
        try:
            wait = cut.run_gate(retry, 'controller', [sys.executable, '-B', str(Path(__file__)), str(label),
                                                      '--worker'])
        except RuntimeError:
            wait = cut.read(retry / 'controller_wait.json')
    else:
        wait = cut.read(retry / 'controller_wait.json')
    raw, switch, failure, missed = shared.evaluate(retry, 'bl', 1, wait)
    second = wrapper_timeout(retry)
    status = 'OPEN' if second is not None else 'FAILED' if failure else 'passed'
    shared.verify_build(label)
    original = group / 'case_summary_before_watchdog_retry.json'
    if not original.is_file():
        original.write_bytes(path.read_bytes())
    record['status'] = status
    record['subruns']['bl'] = {'status': status, 'raw': str(raw), 'raw_sha256': cut.sha(raw),
                               'first_failure': 'wrapper-only timeout' if second else failure,
                               'missed_comparison': missed, 'switch_receipt': switch, 'wait': wait,
                               'watchdog': {'first': initial, 'second': second, 'before': str(original)}}
    pins = [original, raw, Path(__file__), Path(initial['log']), Path(initial['status']),
            retry / 'controller_wait.json', retry / 'controller_launch.json']
    pins.extend(retry.glob('cases/*/command.json'))
    pins.extend(retry.glob('cases/*/pins.json'))
    pins.extend(retry.glob('cases/*/safeguard_process_receipt.json'))
    record['pins'].update({str(p): cut.sha(p) for p in pins})
    pending = path.with_suffix('.tmp')
    cut.save(pending, record)
    pending.replace(path)
    print('Exact C492 same-watchdog retry', status, flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('--worker', action='store_true')
    args = parser.parse_args()
    if args.worker:
        build = shared.HERE / f'build{args.build}'
        case = shared.HERE / 'shared' / build.name / 'closure1/exact-c492/watchdog_retry'
        shared.other(case, build, 'bl', 1)
    else:
        run(args.build)
