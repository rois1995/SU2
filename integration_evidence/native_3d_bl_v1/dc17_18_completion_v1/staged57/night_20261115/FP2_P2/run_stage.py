"""Execute the independent FP2-P2 checks in sequence under their own supervisors."""
from argparse import ArgumentParser
from pathlib import Path
import subprocess
import sys
import tempfile
import time

from run_p2 import CAMPAIGN, G1, HERE, HIDDEN, INHERITED, SHARED, cut, selected_summary, verify_checkpoint
from retry_c492_watchdog import wrapper_timeout

PYTHON = [sys.executable, '-B']


def wait_host():
    """Wait for the existing admission check to clear before preparing an owned child."""
    announced = False
    with tempfile.TemporaryDirectory(prefix='fp2-p2-host-') as folder:
        preflight = Path(folder) / 'receipt.json'
        while True:
            check = subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(preflight)], cwd=cut.REPO,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            if check.returncode == 0:
                if announced:
                    print('Inherited host preflight cleared.', flush=True)
                return
            if 'assert not owned,owned' not in check.stdout:
                raise RuntimeError(check.stdout)
            if not announced:
                print('Waiting for the unchanged host preflight to clear.', check.stdout, sep=chr(10), flush=True)
                announced = True
            time.sleep(30)


def run(build, stage):
    directory = HERE / f'build{build}'
    lab = [*PYTHON, str(HERE / 'run_p2.py'), str(directory), '--only']
    shared = [*PYTHON, str(HERE / 'run_shared.py'), str(build)]
    root = HERE / 'shared' / directory.name

    def shared_run(closure, name):
        return [*shared, str(closure), name], root / f'closure{closure}' / name / 'case_summary.json'

    def trial(name):
        return [*lab, name], selected_summary(build, name)

    if stage == '1':
        commands = [*(trial(name) for name in (*HIDDEN, *G1, *INHERITED)), trial('default-N1'),
                    *(shared_run(0, name) for name in SHARED)]
    else:
        _, checkpoint = verify_checkpoint(directory)
        assert checkpoint['status'] in ('passed', 'OPEN')
        if stage == 'D1':
            commands = [trial('campaign-N1-1')]
        elif stage == 'D2':
            from seal_d1 import verify_d1
            assert verify_d1(directory)[1]['promote']
            commands = [shared_run(1, 'first-collapse')]
        else:
            from seal_d1 import verify_d1
            from seal_d2 import verify_d2
            assert verify_d1(directory)[1]['promote']
            assert verify_d2(directory)[1]['promote']
            comparison = HERE / 'campaign' / directory.name / 'comparison.json'
            commands = [trial('campaign-N1-2'),
                        *(trial(name) for name in CAMPAIGN if '-N1-' not in name),
                        ([*lab, 'compare'], comparison), trial('anatomy-N1'),
                        *(shared_run(1, name) for name in SHARED if name != 'first-collapse')]

    for argv, path in commands:
        name = argv[-1]
        print(f'FP2-P2 step {stage} preparing {name}', flush=True)
        code = None
        if not path.is_file():
            if name != 'compare':
                wait_host()
            code = subprocess.call(argv)
        raw = cut.read(path) if path.is_file() else {'status': 'missing receipt'}
        if name == 'exact-c492':
            attempt = path.parent / 'bl'
            result = raw.get('subruns', {}).get('bl', {})
            if result.get('first_failure') == 'test exit' and 'watchdog' not in result:
                timeout = wrapper_timeout(attempt)
                if timeout is not None:
                    wait_host()
                    code = subprocess.call([*PYTHON, str(HERE / 'retry_c492_watchdog.py'), str(build), argv[-2]])
                    raw = cut.read(path)
        if name == 'saved-current' and raw.get('status') == 'FAILED':
            finding = raw['subruns']['saved_current']['first_failure']
            if isinstance(finding, str) and finding.startswith('missing evaluation: [Errno 2]'):
                from recover_saved_current import recover
                raw = recover(build, int(argv[-2]))
                code = {'passed': 0, 'OPEN': 75, 'FAILED': 1}[raw['status']]
        details = raw.get('first_difference') or raw.get('identity', {}).get('status')
        if 'subruns' in raw:
            details = {key: row['first_failure'] or row['missed_comparison'] or row['status']
                       for key, row in raw['subruns'].items()}
        message = f"FP2-P2 build{build} step {stage}, {name}: {raw['status']}. Exit {code}. Evidence {path.relative_to(HERE)}."
        if details:
            message += f' Gate result {details}.'
        with (HERE.parent / 'progress.md').open('a') as stream:
            stream.write(f'\n{message}\n')
        print(message, flush=True)

    if stage == 'D1':
        from seal_d1 import seal_d1
        path, record = seal_d1(directory)
    elif stage == 'D2':
        from seal_d2 import seal_d2
        path, record = seal_d2(directory)
    else:
        return
    message = f"FP2-P2 build{build} {stage} sealed {record['status']}, promote={record['promote']}. Evidence {path.relative_to(HERE)}."
    if record['first_difference']:
        message += f" First finding {record['first_difference']}."
    with (HERE.parent / 'progress.md').open('a') as stream:
        stream.write(f'\n{message}\n')
    print(message, flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('stage', choices=('1', 'D1', 'D2', '2'))
    args = parser.parse_args()
    run(args.build, args.stage)
