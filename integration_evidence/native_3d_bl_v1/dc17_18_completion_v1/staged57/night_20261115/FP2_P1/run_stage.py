"""Execute the independent FP2-P1 checks in sequence under their own supervisors."""
from argparse import ArgumentParser
from pathlib import Path
import subprocess
import sys

from run_p1 import CAMPAIGN, HERE, SHARED, cut, selected_summary, verify_checkpoint

PYTHON = [sys.executable, '-B']


def run(build, stage):
    directory = HERE / f'build{build}'
    lab = [*PYTHON, str(HERE / 'run_p1.py'), str(directory), '--only']
    shared = [*PYTHON, str(HERE / 'run_shared.py'), str(build)]
    root = HERE / 'shared' / directory.name

    def shared_runs(closure):
        for name in SHARED:
            yield [*shared, str(closure), name], root / f'closure{closure}' / name / 'case_summary.json'

    def trial(name):
        return [*lab, name], selected_summary(build, name)

    if stage == 1:
        commands = [trial('default-N1'), *shared_runs(0)]
    else:
        _, checkpoint = verify_checkpoint(directory)
        assert checkpoint['status'] in ('passed', 'OPEN')
        comparison = HERE / 'campaign' / directory.name / 'comparison.json'
        commands = [*(trial(name) for name in CAMPAIGN), ([*lab, 'compare'], comparison),
                    trial('anatomy-N1'), *shared_runs(1)]

    for argv, path in commands:
        print(f'FP2-P1 step {stage} preparing {argv[-1]}', flush=True)
        code = subprocess.call(argv) if not path.is_file() else None
        raw = cut.read(path) if path.is_file() else {'status': 'missing receipt'}
        name = argv[-1]
        details = raw.get('first_difference') or raw.get('identity', {}).get('status')
        if 'subruns' in raw:
            details = {key: row['first_failure'] or row['missed_comparison'] or row['status']
                       for key, row in raw['subruns'].items()}
        message = f"FP2-P1 build{build} step {stage}, {name}: {raw['status']}. Exit {code}. Evidence {path.relative_to(HERE)}."
        if details:
            message += f' Gate result {details}.'
        with (HERE.parent / 'progress.md').open('a') as stream:
            stream.write(f'\n{message}\n')
        print(message, flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('stage', type=int, choices=(1, 2))
    args = parser.parse_args()
    run(args.build, args.stage)
