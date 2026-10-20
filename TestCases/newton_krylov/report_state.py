"""Evaluate an SU2 restart with the native Newton test reporter, without advancing it."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time


def option(text, key, value):
    lines, found, continuation = [], False, False
    for line in text.splitlines():
        continued = line.split('%', 1)[0].rstrip().endswith('\\')
        if continuation:
            continuation = continued
        elif re.match(r'^\s*' + re.escape(key) + r'\s*=', line):
            lines.append(key + '= ' + value)
            found, continuation = True, continued
        else:
            lines.append(line)
    if not found:
        lines.append(key + '= ' + value)
    return '\n'.join(lines) + '\n'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reporter', type=Path, required=True, help='UnitTests/newton_equation_tests executable')
    parser.add_argument('--config', type=Path, required=True, help='Original single-zone solver configuration')
    parser.add_argument('--restart', type=Path, required=True, help='SU2 .dat or .csv restart')
    parser.add_argument('--output', type=Path, required=True, help='New directory for report and provenance')
    parser.add_argument('--ranks', type=int, default=1)
    parser.add_argument('--threads', type=int, default=1)
    args = parser.parse_args()
    if args.ranks < 1 or args.threads < 1:
        parser.error('ranks and threads must be positive')
    reporter, config, restart = [path.resolve() for path in [args.reporter, args.config, args.restart]]
    if any(not path.is_file() for path in [reporter, config, restart]):
        parser.error('an input file does not exist')
    if restart.suffix not in ['.dat', '.csv']:
        parser.error('the reporter requires a .dat or .csv restart')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    original = {str(path): sha(path) for path in [config, restart]}
    text = config.read_text()
    settings = {'RESTART_SOL': 'YES', 'READ_BINARY_RESTART': 'YES' if restart.suffix == '.dat' else 'NO',
                'SOLUTION_FILENAME': str(restart.with_suffix('')), 'OUTPUT_FILES': 'NONE',
                'CONV_FILENAME': str(output / 'unused_history'), 'COMM_LEVEL': 'FULL',
                'NEWTON_EQUATIONS': 'NONE', 'NEWTON_COUPLED': 'NO', 'NEWTON_BLOCK_LOWER': 'NO',
                'NEWTON_ASSEMBLED': 'NO', 'NEWTON_KRYLOV': 'NO', 'SST_NEWTON': 'NO'}
    for key, value in settings.items():
        text = option(text, key, value)
    report_config, report = output / 'report.cfg', output / 'report.json'
    report_config.write_text(text)
    command = ['mpirun', '--bind-to', 'none', '-n', str(args.ranks), str(reporter), '[NewtonStateReport]']
    started = time.perf_counter()
    exit_code = None
    timed_out = False
    with (output / 'log.txt').open('w') as log:
        try:
            process = subprocess.run(command, cwd=config.parent,
                env=dict(os.environ, OMP_NUM_THREADS=str(args.threads), SU2_NK_REPORT_CONFIG=str(report_config),
                         SU2_NK_REPORT_OUTPUT=str(report)), stdout=log, stderr=subprocess.STDOUT, timeout=120)
            exit_code = process.returncode
        except subprocess.TimeoutExpired:
            timed_out = True
    manifest = {'command': command, 'working_directory': str(config.parent), 'threads': args.threads,
                'exit_code': exit_code, 'timed_out': timed_out, 'elapsed_seconds': time.perf_counter() - started,
                'reporter_sha256': sha(reporter), 'inputs': original,
                'report_config_sha256': sha(report_config), 'inputs_unchanged': all(sha(Path(p)) == digest
                    for p, digest in original.items())}
    if report.is_file():
        manifest['report_sha256'] = sha(report)
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    if exit_code != 0 or not manifest['inputs_unchanged'] or not report.is_file():
        raise SystemExit('Physical report failed; inspect ' + str(output / 'log.txt'))
    result = json.loads(report.read_text())
    maximum = max(value for system in result['systems'].values() for value in system['normalized_rms'])
    print('Maximum normalized physical RMS:', maximum)


if __name__ == '__main__':
    main()
