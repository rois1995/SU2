"""Run FP0's pinned test-only measurements and the three inherited G1 cohorts."""

from argparse import ArgumentParser
from contextlib import redirect_stdout
import json
import math
import os
from pathlib import Path
import re
import runpy
import sys

from run_build import BUILD, ITEM, REPO, cut, sha


REF = ITEM.parent / 'N14/section6_12/validation/adopted_reference'
PARSER = ITEM.parent / 'N10/parse_g1_identity.py'
PART_B = ITEM / 'drafts/B/part_b.py'
TMPDIR = REPO / 'integration_evidence/build-integrated-v2/compile-tmp'
A_CASES = {
    'kernels': ('[NativeFP0Kernels]', {}),
    'optimize_10000': ('[NativeFP0Optimize]', {'SU2_NATIVE_FP0_OPTIMIZER_CELLS': '10000'}),
    'optimize_100000': ('[NativeFP0Optimize]', {'SU2_NATIVE_FP0_OPTIMIZER_CELLS': '100000'}),
    'census': ('[NativeFP0Census]', {}),
    'proof': ('[NativeFP0Proof]', {}),
    'float_surface': ('[NativeFP0FloatSurface]', {}),
}
EXPECTED_A_ROWS = {
    'kernels': {*(f'A{index}' for index in range(1, 9)), 'A7_post', 'A8_post'},
    'optimize_10000': {'A9'}, 'optimize_100000': {'A9'},
    'census': {'A10'}, 'proof': {'A11'}, 'float_surface': {'A12'},
}


def verified_build():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    active = ITEM / 'active_build.txt'
    label = int(active.read_text()) if active.is_file() else 592
    launch = cut.read(ITEM / ('build_controller_launch.json' if label == 592
                              else f'build{label}_controller_launch.json'))
    assert cut.terminal(launch), launch
    state = cut.read(ITEM / f'build{label}.json')
    assert state['passed'] and cut.terminal(state['wait']['controller'])
    source = Path(state.get('source_pins', ITEM / 'test_source_pins.json'))
    assert state['source_pins_sha256'] == sha(source)
    assert all(sha(Path(path)) == digest for path, digest in cut.read(source).items())
    assert all(sha(BUILD / name) == digest for name, digest in state['graph'].items())
    binary = Path(state['test_binary'])
    assert sha(binary) == state['test_binary_sha256']
    return binary, state


def case_input(part):
    info = cut.read(ITEM / 'case_input_manifest.json')[part]
    assert sha(Path(info['config'])) == info['config_sha256']
    assert sha(Path(info['sensor'])) == info['sensor_sha256']
    home = Path(info['config']).parent
    for name, digest in info['pinned_sources'].items():
        assert sha(Path(name)) == sha(home / Path(name).name) == digest, name
    return home, {'SU2_NATIVE_FROZEN_CONFIG': info['config'], 'SU2_NATIVE_FROZEN_SENSOR3D': info['sensor']}


def run_case(case, binary, ranks, selector, seconds, environment=None, cwd=None, xml=False, optimizer=False):
    case.mkdir(parents=True, exist_ok=True)
    values = {'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1', 'MKL_NUM_THREADS': '1',
              'OMPI_MCA_osc': 'pt2pt', 'TMPDIR': str(TMPDIR)}
    values.update(environment or {})
    argv = ['env', '-C', str(cwd or case), *(f'{k}={v}' for k, v in values.items()),
            'nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(case / 'time.txt'), 'timeout']
    if optimizer:
        argv.append('--foreground')
    argv.append(str(seconds))
    if ranks > 1:
        argv += ['mpirun', '--bind-to', 'none', '-n', str(ranks), '--output-filename', str(case / 'rank_output')]
    argv += [str(binary), selector]
    if xml:
        argv += ['--durations', 'yes', '--reporter', 'xml']
    if optimizer:
        argv = [sys.executable, '-B', str(ITEM / 'optimizer_deadline.py'), *argv]
    cut.save(case / 'command.json', {'argv': argv, 'ranks': ranks, 'selector': selector, 'seconds_limit': seconds})
    try:
        wait = cut.run_gate(case, 'test', argv)
    except RuntimeError:
        wait = cut.read(case / 'test_wait.json')
        assert wait['terminal']
    assert cut.terminal(wait['controller'])
    return wait


def rows(path):
    values = []
    for raw in path.read_text(errors='replace').splitlines():
        match = re.search(r'FP0 ([A-Z]\w*) (\{.*\})\s*$', raw)
        if match:
            values.append({'measurement': match[1], **json.loads(match[2])})
    return values


def peak_kib(file):
    value = re.search(r'Maximum resident set size \(kbytes\):\s*(\d+)', file.read_text())
    return int(value[1]) if value else None


def measure_a(binary, build, names):
    home, environment = case_input('A')
    for name in names:
        selector, additional = A_CASES[name]
        case = home / name
        if (case / 'measurement.json').is_file():
            result = cut.read(case / 'measurement.json')
            assert sha(case / 'test_outer.log') == result['log_sha256']
            continue
        for input_path in home.iterdir():
            if input_path.is_file() and input_path.name != 'run.cfg':
                case.mkdir(exist_ok=True)
                os.link(input_path, case / input_path.name)
        optimizer = name.startswith('optimize_')
        wait = run_case(case, binary, 1, '[.]' + selector, 5400,
                        {**environment, **additional}, optimizer=optimizer)
        log = case / 'test_outer.log'
        values = rows(log)
        missing = EXPECTED_A_ROWS[name] - {row['measurement'] for row in values}
        timed_optimizer = optimizer and wait['returncode'] == 124 and any(
            row.get('stop') == 'outer_time_limit' for row in values)
        if missing or (wait['returncode'] != 0 and not timed_optimizer):
            cut.save(case / 'failed_fixture.json', {'wait': wait, 'rows': values, 'missing': sorted(missing),
                                                     'log_sha256': sha(log)})
            raise RuntimeError(f'{name} failed before a measurement receipt. See {log}.')
        cut.save(case / 'measurement.json', {'case': name, 'peak_kib': peak_kib(case / 'time.txt'),
                 'elapsed_seconds': wait['elapsed_seconds'], 'rows': values, 'log_sha256': sha(log),
                 'build': build['build'], 'test_binary_sha256': build['test_binary_sha256']})
        progress(f'Part A {name} terminal, {len(values)} measurement rows.')


def measure_b(binary):
    home, environment = case_input('B')
    capture = home / 'replay'
    if not (capture / 'test_wait.json').is_file():
        capture.mkdir(exist_ok=True)
        for path in home.iterdir():
            if path.is_file() and path.name != 'run.cfg':
                os.link(path, capture / path.name)
        wait = run_case(capture, binary, 6, '[.][NativeFrozenVolume3D]', 5400,
                        {**environment, 'SU2_NATIVE_STAGE_CAPTURE': '1'})
        assert wait['returncode'] == 0, wait
    else:
        wait = cut.read(capture / 'test_wait.json')
        assert wait['returncode'] == 0 and cut.terminal(wait['controller'])
    helper = runpy.run_path(str(PART_B))
    receipt = capture / 'fp0_exit_capture_receipt.json'
    if not receipt.is_file():
        helper['prepare'](capture)
    sealed = cut.read(receipt)
    assert sealed['source_valid'] and len(sealed['shards']) == 6
    for shard in sealed['shards']:
        for payload in shard['payloads']:
            path = capture / payload['name']
            assert path.stat().st_size == payload['bytes'] and sha(path) == payload['sha256'], path
        options = capture / f"stage_exit_rank{shard['rank']}_fp0_options.txt"
        assert options.read_text().split() == list(map(str, shard['options'])), options
    progress('Part B frozen replay and six valid exit shards verified. Starting standalone Refine.')
    refine = home / 'refine'
    if not (refine / 'test_wait.json').is_file():
        wait = run_case(refine, binary, 6, '[.][NativeFastPathExitRefine3D]', 5400, {
            'SU2_NATIVE_FP0_EXIT_ROOT': str(capture),
            'SU2_NATIVE_FP0_EXIT_REFERENCE': str(home / 'm6.su2.native_ref'),
            'SU2_NATIVE_FROZEN_CONFIG': environment['SU2_NATIVE_FROZEN_CONFIG'],
        }, cwd=capture)
        assert wait['returncode'] == 0, wait
    else:
        wait = cut.read(refine / 'test_wait.json')
        assert wait['returncode'] == 0 and cut.terminal(wait['controller'])
    with (home / 'measurements.md').open('w') as output, redirect_stdout(output):
        helper['report'](capture, capture / 'rank_output/1', refine / 'rank_output/1')
    progress('Part B standalone Refine terminal and measured.')


def run_g1(binary, build, cohorts):
    parser = runpy.run_path(str(PARSER))
    manifest = cut.read(REF / 'G1_shared_reference_manifest.json')
    selection = cut.read(REF / 'G1_actual_selection.json')
    for cohort in cohorts:
        ranks = {'serial': 1, 'mpi2': 2, 'mpi4': 4}[cohort]
        case = ITEM / 'G1' / cohort
        if (case / 'comparison.json').is_file():
            assert cut.read(case / 'comparison.json')['identity_pass']
            continue
        reference_path = REF / f'G1_candidate_{cohort}.json'
        reference = cut.read(reference_path)
        limits = manifest['cohorts'][cohort]
        assert sha(reference_path) == limits['report_sha256']
        filter_key = 'serial_filter' if ranks == 1 else 'mpi_filter'
        seconds = math.ceil(limits['seconds'] * 1.5)
        wait = run_case(case, binary, ranks, selection[filter_key], seconds, xml=True)
        assert sha(binary) == build['test_binary_sha256']
        source = Path(build.get('source_pins', ITEM / 'test_source_pins.json'))
        assert all(sha(Path(path)) == digest for path, digest in cut.read(source).items())
        (case / 'run.log').symlink_to('test_outer.log')
        cut.save(case / 'pins.json', {'test_binary': build['test_binary_sha256'], 'reference': sha(reference_path),
                 'parser': sha(PARSER), 'manifest': sha(REF / 'G1_shared_reference_manifest.json'),
                 'selection': sha(REF / 'G1_actual_selection.json'),
                 'test_source_pins': sha(source)})
        cut.save(case / 'status.json', {'state': 'terminal', 'pins_verified': True,
                 'seconds': wait['elapsed_seconds'], 'exit_code': wait['returncode']})
        parsed = parser['parse_run'](case, ranks, 375 if ranks == 1 else 57)
        result = parser['compare'](reference, parsed, set())
        cut.save(case / 'comparison.json', {'identity_pass': result['identity_pass'],
                 'records': len(parsed['case_results']), 'elapsed_seconds': wait['elapsed_seconds'],
                 'candidate_returncode': wait['returncode'], 'reference_returncode': limits['process_returncode'],
                 'reporter_totals_equal': result['reporter_totals_equal'], 'changes': result['changes']})
        assert wait['returncode'] == limits['process_returncode'] and result['identity_pass'], result['changes'][:1]
        progress(f"G1 {cohort} matched {len(parsed['case_results'])} case records.")


def progress(message):
    with (ITEM.parent / 'progress.md').open('a') as output:
        output.write(f'\nFP0. {message}\n')


def run():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('A', 'B', 'G1', 'ordered'))
    parser.add_argument('cases', nargs='*')
    args = parser.parse_args()
    for name in list(os.environ):
        if name.startswith('SU2_NATIVE_'):
            del os.environ[name]
    binary, build = verified_build()
    if args.mode in ('A', 'ordered'):
        measure_a(binary, build, args.cases or A_CASES)
    if args.mode in ('B', 'ordered'):
        measure_b(binary)
    if args.mode in ('G1', 'ordered'):
        run_g1(binary, build, args.cases or ('serial', 'mpi2', 'mpi4'))


if __name__ == '__main__':
    run()
