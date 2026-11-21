"""Build and measure FP2-B0 with the existing host preflight and owned MPI supervisor."""

from argparse import ArgumentParser
from collections import defaultdict
from decimal import Decimal
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FP2_A = HERE.parent / 'FP2_A'
FP1 = HERE.parent / 'FP1'
sys.path.insert(0, str(FP2_A))
import run_a as a
import run_rank_check as legacy
from run_fp0 import TMPDIR

ORDER = HERE / 'inputs/FAST_PATH_STAGE2_B0.md'
ORDER_SHA = '6e0d9bcbeaf8cebd4dcbd9257b165492a1b20535b1ee01d0d4f0b6bc810955d2'
BASE = '852e566d17'
REFERENCE = FP2_A / 'build627'
SOURCE_CHECK = HERE / 'source_check.md'
SECONDS = 1800
MODE = {
    'SurfaceA': (1, False, 'none'),
    'H1': (1, True, 'none'), 'H0': (1, False, 'none'),
    'H6': (6, True, 'none'), 'H6off': (6, False, 'none'),
    'X1': (1, True, 'split'), 'X2': (1, True, 'collapse'), 'X3': (1, True, 'both'),
    'X6': (6, True, None),
}
HANDOVER = tuple(mode for mode in MODE if mode != 'SurfaceA')
INPUT = {
    1: FP2_A / 'scoped/build627/lab/LAB1_V2',
    6: FP2_A / 'rank_check/build627/K3',
}
INPUT_NAMES = ('run.cfg', 'm6.su2', 'm6.su2.native_ref', 'restart_flow_adap_00000.dat', 'baseline_euler_seed.dat')
SENSOR = HERE.parent / 'FP0/cases/B/sensor.bin'
ADDED_TEST_INPUTS = (
    a.cut.REPO / 'UnitTests/Common/adaptation/native_local_fixture_access.hpp',
    *(a.cut.REPO / 'UnitTests/SU2_CFD/adaptation/fixtures' / f'native_handover{suffix}.inc'
      for suffix in ('', '_quality', '_volume')),
)
ROW = re.compile(r'^FP2_B0 (?P<kind>[a-z0-9_]+) (?P<fields>.+)$')
FIELD = re.compile(r'(?P<key>\w+)=(?P<value>"[^"]*"|\S+)')


def verify_order():
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    assert SOURCE_CHECK.is_file(), SOURCE_CHECK


def host():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd', 'Use the real host for process receipts.'
    old = a.cut.read(FP2_A / 'terminal_audit.json')
    assert old['all_owned_handles_terminal']
    assert all(a.cut.terminal(row) for row in old['identities'])


def production_audit():
    manifest = REFERENCE / 'source_pins.json'
    reference = a.cut.read(REFERENCE / 'build_status.json')
    assert a.sha(manifest) == reference['source_pins_sha256']
    unit_tests = a.cut.REPO / 'UnitTests'
    pins = {name: digest for name, digest in a.cut.read(manifest).items()
            if not Path(name).is_relative_to(unit_tests)}
    differences = []
    for name, expected in pins.items():
        path = Path(name)
        actual = a.sha(path) if path.is_file() else None
        if actual != expected:
            differences.append({'path': name, 'expected': expected, 'actual': actual})
    diff = subprocess.check_output(['git', 'diff', '--stat', BASE, '--', 'Common', 'SU2_CFD'],
                                   cwd=a.cut.REPO, text=True)
    solver = a.sha(Path(reference['binary']))
    return {'source_pins': str(manifest), 'source_pins_sha256': a.sha(manifest),
            'checked_non_unit_tests_paths': len(pins), 'differences': differences,
            'git_diff_stat': diff, 'solver_sha256': solver, 'expected_solver_sha256': reference['binary_sha256'],
            'passed': not differences and not diff and solver == reference['binary_sha256']}


def build_path(label):
    return HERE / f'build{label}'


def cases_path(label):
    return HERE / 'cases' / f'build{label}'


def build(label):
    host()
    verify_order()
    verify_terminal()
    before = production_audit()
    assert before['passed'], before

    inherited = runpy.run_path(str(FP1 / 'build_fp1.py'))['build']
    scope = inherited.__globals__
    pins = scope['source_pins']

    def source_pins():
        return {**pins(), **{str(path): a.sha(path) for path in ADDED_TEST_INPUTS}}

    scope['source_pins'] = source_pins
    work = build_path(label)
    passed = inherited(work, label, solver=False)
    after = production_audit()
    a.sealed(work / 'production_build.json', {'before': before, 'after': after})
    assert after['passed'], after
    assert passed, work / 'build_status.json'
    print(f'FP2-B0 build{label} test driver ready.', flush=True)


def selector(name, ranks):
    if name == 'SurfaceA':
        return a.NAME_BOX
    return '[.][NativeLocalCoreHandoverSerial]' if ranks == 1 else '[.][NativeLocalCoreHandoverMPI]'


def input_data(ranks):
    source = INPUT[ranks]
    manifest = source / 'pins.json'
    path = manifest.relative_to(a.cut.REPO)
    sealed = subprocess.check_output(['git', 'show', f'{BASE}:{path}'], cwd=a.cut.REPO)
    assert a.sha(manifest) == hashlib.sha256(sealed).hexdigest(), manifest
    saved = a.cut.read(manifest)
    tracked = {str(path): saved[str(path)] for path in (source / name for name in INPUT_NAMES)}
    tracked[str(SENSOR)] = saved[str(SENSOR)]
    tracked[str(manifest)] = a.sha(manifest)
    for name, digest in tracked.items():
        assert a.sha(Path(name)) == digest, name
    config = (source / 'run.cfg').read_bytes()
    assert config.splitlines().count(f'ADAP_NATIVE_RANKS= {ranks}'.encode()) == 1
    for line in (b'ADAP_NATIVE_MEMORY_LIMIT= 12288', b'ADAP_NATIVE_JOB_MEMORY_LIMIT= 19073'):
        assert config.splitlines().count(line) == 1
    copies = {source / name: tracked[str(source / name)] for name in INPUT_NAMES if name != 'run.cfg'}
    return config, SENSOR, copies, tracked, None


def command(case, binary, config, sensor, ranks, surface, extra=None):
    name = case.name
    if extra is None:
        extra = a.cut.read(case / 'command.json')['extra']
    env = ['LC_ALL=C', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
           'OMPI_MCA_osc=pt2pt', f'TMPDIR={TMPDIR}']
    if name != 'SurfaceA':
        env += [f'SU2_NATIVE_FROZEN_CONFIG={config}', f'SU2_NATIVE_FROZEN_SENSOR3D={sensor}',
                f'SU2_FP2_B0_RUN={name}', f'SU2_FP2_B0_EXTRA={extra}']
    argv = ['env', '-C', str(case), *env, 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(case / 'time.txt'),
            'timeout', '--verbose', '--signal=KILL', str(SECONDS)]
    if ranks == 6:
        argv += ['mpirun', '--bind-to', 'none', '-n', str(ranks),
                 '--output-filename', str(case / 'rank_output')]
    return [*argv, str(binary), selector(name, ranks)]


def owned(case):
    ranks, _, _ = MODE[case.name]
    legacy.RUNS = {name: details[:2] for name, details in MODE.items()}
    legacy.WALL = SECONDS
    legacy.SELECTOR = selector(case.name, ranks)
    legacy.command = command
    return legacy.owned(case)


def log_paths(case, ranks):
    if ranks == 1:
        path = case / 'test_outer.log'
        return {0: path} if path.is_file() else {}
    logs = {}
    for path in sorted(case.glob('rank_output/*/rank.*/stdout')):
        rank = int(path.parent.name.rsplit('.', 1)[1])
        assert rank not in logs and 0 <= rank < ranks, path
        logs[rank] = path
    return logs


def measurement_rows(lines):
    for line in lines:
        if not line.endswith(('\n', '\r')):
            continue
        line = a.ANSI.sub('', line).rstrip('\r\n')
        if match := ROW.match(line):
            fields = {f['key']: f['value'].removeprefix('"').removesuffix('"')
                      for f in FIELD.finditer(match['fields'])}
            assert 'rank' in fields, line
            yield match['kind'], fields


def summarize_rows(name, rank_rows, ranks):
    """Check the per-rank checkpoint grid and reduce the specified final rows."""
    if name == 'SurfaceA':
        return {}
    passes = 1 if ranks == 1 else 2
    x_run = name.startswith('X')
    checkpoints = ('c1', 'c2', 'c3', 'c4') if x_run else ('c1', 'c2', 'c3')
    handover = checkpoints[-1]
    handover_bad = post_shift_cells = final_cells = 0
    faces_out = [0] * passes
    differences, free_differences = [], []
    problems = []
    for rank in range(ranks):
        groups = defaultdict(list)
        for kind, row in rank_rows.get(rank, ()):
            assert int(row['rank']) == rank, (kind, row, rank)
            groups[kind].append(row)
        grid = [(str(p), c) for p in range(1, passes + 1) for c in checkpoints]
        observed = [(row['pass'], row['checkpoint']) for row in groups['checkpoint']]
        if observed != grid or len(groups['pass']) != passes:
            problems.append(f'rank {rank} checkpoint/pass grid incomplete')
            continue
        for row in groups['checkpoint']:
            if int(row['bad']) != sum(int(row[f'bad_{name}']) for name in 'abcd'):
                problems.append(f'rank {rank} checkpoint counts do not sum')
            if (row['pass'], row['checkpoint']) == (str(passes), handover):
                handover_bad += int(row['bad'])
        for p, row in enumerate(groups['pass'], 1):
            final = next(r for r in groups['checkpoint'] if (r['pass'], r['checkpoint']) == (str(p), handover))
            if row['bad_out'] != final['bad'] or row['cells_out'] != final['cells']:
                problems.append(f'rank {rank} pass {p} totals differ from {handover}')
            if row['bad_out_free'] != final['bad_d']:
                free_differences.append({'rank': rank, 'pass': p, 'checkpoint': handover,
                                         'classification_d': int(final['bad_d']),
                                         'bad_out_free': int(row['bad_out_free'])})
            faces_out[p - 1] += int(row['faces_out'])
            if p == passes:
                final_cells += int(row['cells_out'])
        if name != 'H0':
            if len(groups['region']) != 1:
                problems.append(f'rank {rank} final region missing')
            else:
                post_shift_cells += int(groups['region'][0]['cells'])
        if x_run:
            if not groups['exact'] or any(int(row['nonpositive']) for row in groups['exact']):
                problems.append(f'rank {rank} exact positivity missing or failed')
            rounds = {p: [int(row['round']) for row in groups['volume_round'] if row['pass'] == str(p)]
                      for p in range(1, passes + 1)}
            if any(not values or values != list(range(1, len(values) + 1)) or len(values) > 10
                   for values in rounds.values()):
                problems.append(f'rank {rank} unexpected volume round rows')
        observed_edges = {(row['pass'], row['checkpoint']) for row in groups['edge'] if row['order'] == '1'}
        expected_edges = {(str(p), handover) for p in range(1, passes + 1)}
        if not expected_edges <= observed_edges:
            problems.append(f'rank {rank} shortest-edge row missing')
        if name in ('H1', 'H6') or x_run:
            for cp in ('c3', 'c4') if x_run else ('c3',):
                classes = {row['class'] for row in groups['quality_summary']
                           if (row['pass'], row['checkpoint']) == (str(passes), cp)}
                if classes != set('abcd'):
                    problems.append(f'rank {rank} {cp} refusal summary incomplete')
    if not problems and name in ('H1', 'H6'):
        want = (554_558, 84_102) if name == 'H1' else (549_519, 84_124, 84_184)
        seen = (final_cells if name == 'H1' else post_shift_cells, *faces_out)
        fields = ('cells', *(f'pass{p}_faces_out' for p in range(1, passes + 1)))
        differences = [{'field': key, 'observed': actual, 'reference': expected}
                       for key, actual, expected in zip(fields, seen, want) if actual != expected]
    return {'handover_bad': handover_bad, 'handover_checkpoint': handover,
            'handover_cells': final_cells, 'post_shift_cells': None if name == 'H0' else post_shift_cells,
            'faces_out_by_pass': faces_out, 'problems': problems, 'sanity_differences': differences,
            'bad_out_free_differences': free_differences}


def x_choice(folder):
    candidates = []
    for name in ('X1', 'X2', 'X3'):
        path = folder / name / 'measurement.json'
        receipt = a.cut.read(path)
        assert receipt['passed'] and receipt['case'] == name
        candidates.append({'case': name, 'extra': MODE[name][2], 'bad_cells': receipt['measurement']['handover_bad'],
                           'receipt': str(path), 'receipt_sha256': a.sha(path)})
    return {'selected': min(candidates, key=lambda row: row['bad_cells']), 'candidates': candidates,
            'tie_break': 'first in X1, X2, X3 order'}


def run(label, only):
    host()
    verify_order()
    verify_terminal()
    work = build_path(label)
    receipt_path, manifest = (work / name for name in ('build_status.json', 'source_pins.json'))
    receipt = a.cut.read(receipt_path)
    binary, source_hash = a.verify_build(receipt, manifest)
    assert receipt['build'] == label and a.cut.terminal(receipt['wait']['controller'])
    audit = a.cut.read(work / 'production_build.json')
    assert audit['before']['passed'] and audit['after']['passed'] and production_audit()['passed']
    for name in tuple(os.environ):
        if name.startswith(('SU2_FP1_', 'SU2_FP2_', 'SU2_NATIVE_')):
            del os.environ[name]

    folder = cases_path(label)
    folder.mkdir(parents=True, exist_ok=True)
    proof = {key: receipt[key] for key in ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    pinned = {str(path): a.sha(path) for path in
              (ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, receipt_path, work / 'production_build.json',
               binary, Path(__file__).resolve(), Path(legacy.__file__).resolve())}
    pinned[str(manifest)] = source_hash
    names = tuple(MODE) if only is None else tuple(dict.fromkeys(('SurfaceA', only)))
    for name in names:
        case = folder / name
        case.mkdir(exist_ok=True)
        ranks, surface, extra = MODE[name]
        if name == 'X6':
            choice = x_choice(folder)
            a.sealed(folder / 'x6_choice.json', choice)
            extra = choice['selected']['extra']
        run_case(case, binary, ranks, surface, extra, proof, pinned)


def run_case(case, binary, ranks, surface, extra, proof, pinned):
    name = case.name
    config, sensor, pins = case / 'run.cfg', '', pinned
    if name != 'SurfaceA':
        original = runpy.run_path(str(a.LAB_ORIGINAL))
        config, sensor, pins = original['prepare'](case, input_data(ranks), pinned)
    a.sealed(case / 'provenance.json', {**proof, 'case': name, 'ranks': ranks, 'surface': surface, 'extra': extra,
                                       'selector': selector(name, ranks), 'wall_seconds': SECONDS, 'pins': pins})
    wait = a.resumed(case)
    recipe = command(case, binary, config, sensor, ranks, surface, extra)
    if wait is None:
        wrapper = ['nice', '-n', '19', a.cut.PYTHON, '-B', str(Path(__file__).resolve()), 'owned', str(case)]
        a.sealed(case / 'command.json', {'argv': recipe, 'wrapper_argv': wrapper, 'ranks': ranks,
                                         'selector': selector(name, ranks), 'seconds_limit': SECONDS,
                                         'binary': str(binary), 'sensor': str(sensor), 'extra': extra})
        try:
            a.cut.run_gate(case, 'test', wrapper)
        except RuntimeError:
            assert a.resumed(case) is not None
        wait = a.resumed(case)
    assert recipe == a.cut.read(case / 'command.json')['argv']
    processes = a.cut.read(case / 'owned_processes.json')
    assert processes['all_observed_terminal'] and all(a.cut.terminal(row) for row in processes['identities'])
    for path, digest in pins.items():
        assert a.sha(Path(path)) == digest, path
    logs = log_paths(case, ranks)
    catch = a.summarize_box(list(logs.values()))
    ranks_rows = {}
    if name != 'SurfaceA':
        for rank, path in logs.items():
            with path.open(errors='replace') as stream:
                ranks_rows[rank] = list(measurement_rows(stream))
    measurement = summarize_rows(name, ranks_rows, ranks)
    timed_peak = (a.peak_kib(case / 'time.txt') or 0) * 1024
    sampled_peak = processes['sampled_rank_peak_lower_bound_bytes']
    peak = max(timed_peak, sampled_peak)
    passed = (wait['returncode'] == 0 and processes['stop'] is None and len(logs) == ranks and catch['passed']
              and 0 < peak <= a.MEMORY * 1024 and not measurement.get('problems'))
    artifacts = {case / name for name in (*a.ARTIFACTS, 'pins.json', 'provenance.json') if (case / name).is_file()}
    artifacts.update(path for log in logs.values() for path in (log, log.with_name('stderr')) if path.is_file())
    result = {**proof, **catch, 'passed': passed, 'case': name, 'ranks': ranks, 'surface': surface, 'extra': extra,
              'wait': wait, 'sampled_rank_peak_lower_bound_bytes': sampled_peak, 'time_peak_bytes': timed_peak,
              'stop': processes['stop'], 'measurement': measurement,
              'artifacts': {str(path.relative_to(case)): a.sha(path) for path in sorted(artifacts)}}
    a.sealed(case / 'measurement.json', result)
    print(f'{name}: {"completed" if passed else "incomplete"} in {wait["elapsed_seconds"]:.3f} s '
          f'({measurement.get("handover_bad", "serial fixture")} hand-over bad).', flush=True)
    assert passed, case / 'measurement.json'


def verify_terminal():
    """Refuse a new launch while any witnessed FP2-B0 process is live."""
    identities = set()
    for launch in sorted(HERE.glob('build*/build_launch.json')) + sorted(HERE.glob('cases/build*/*/test_launch.json')):
        prefix = launch.name.removesuffix('_launch.json')
        start = a.cut.read(launch)
        done = a.cut.read(launch.with_name(f'{prefix}_wait.json'))
        assert done['terminal'] and start['controller'] == done['controller'], launch
        assert a.cut.terminal(done['controller']), launch
        owner = start['supervisor']
        assert owner['pid'] == os.getpid() or a.cut.terminal(owner), launch
        identities.update((row['pid'], row['start_ticks']) for row in (done['controller'], owner))
        processes = launch.parent / 'owned_processes.json'
        if processes.is_file():
            for row in a.cut.read(processes)['identities']:
                assert a.cut.terminal(row), row
                identities.add((row['pid'], row['start_ticks']))
    return len(identities)


def audit(label):
    host()
    verify_order()
    count = verify_terminal()
    result = production_audit()
    assert result['passed'], result
    work = build_path(label)
    status = a.cut.read(work / 'build_status.json')
    _, source_hash = a.verify_build(status, work / 'source_pins.json')
    dest = work / 'final_audit.json'
    a.sealed(dest, {**result, 'all_witnessed_owned_handles_terminal': True, 'owned_identities_checked': count,
                    'test_binary_sha256': status['test_binary_sha256'], 'test_source_pins_sha256': source_hash})
    print(f'{result["checked_non_unit_tests_paths"]} production paths, zero differences. '
          f'Solver hash unchanged. All {count} witnessed owned identities terminal.')


def self_check():
    rows = list(measurement_rows(['\x1b[0mFP2_B0 edge rank=0 pass=1 checkpoint=c3 order=1 metric=5.0e-10\n',
                                  'FP2_B0 edge rank=0 unterminated=1',
                                  'unrelated output\n']))
    assert len(rows) == 1 and Decimal(rows[0][1]['metric']) == Decimal('5e-10')
    with_case = HERE / 'cases/build628/H6'
    argv = command(with_case, Path('/test_driver'), with_case / 'run.cfg', SENSOR, 6, True, 'none')
    assert argv[argv.index('timeout') + 3] == '1800' and argv[argv.index('mpirun') + 4] == '6'
    assert argv[-1] == '[.][NativeLocalCoreHandoverMPI]'
    serial = []
    for cp in ('c1', 'c2', 'c3'):
        serial.append(('checkpoint', {'rank': '0', 'pass': '1', 'checkpoint': cp, 'cells': '8', 'bad': '1',
                                      'bad_a': '0', 'bad_b': '0', 'bad_c': '0', 'bad_d': '1'}))
    serial += [('pass', {'rank': '0', 'cells_out': '8', 'bad_out': '1', 'bad_out_free': '1', 'faces_out': '4'}),
               ('edge', {'rank': '0', 'pass': '1', 'checkpoint': 'c3', 'order': '1'})]
    summary = summarize_rows('H0', {0: serial}, 1)
    assert not summary['problems'] and summary['handover_bad'] == 1 and summary['faces_out_by_pass'] == [4]
    incomplete = summarize_rows('H0', {0: serial[1:]}, 1)
    assert incomplete['problems']
    print('FP2-B0 launcher, row parser and checkpoint grid self-check passed.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('build', 'run', 'audit', 'owned', 'self-check'))
    parser.add_argument('target', nargs='?')
    parser.add_argument('--only', choices=tuple(MODE))
    args = parser.parse_args()
    if args.mode == 'self-check':
        self_check()
    elif args.mode == 'owned':
        raise SystemExit(owned(Path(args.target).resolve()))
    elif args.mode == 'build':
        build(int(args.target))
    elif args.mode == 'audit':
        audit(int(args.target))
    else:
        run(int(args.target), args.only)
