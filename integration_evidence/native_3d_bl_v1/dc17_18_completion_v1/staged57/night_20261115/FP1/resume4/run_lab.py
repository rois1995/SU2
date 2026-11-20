"""Run the four H6 lab pairs serially and preserve complete rows through the hard deadline."""

from argparse import ArgumentParser
from collections import Counter
import hashlib
import json
import math
import os
from pathlib import Path
import re
import runpy
import signal
import subprocess
import sys
import time

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREVIOUS = HERE.parent / 'resume3/run_lab.py'
previous = runpy.run_path(str(PREVIOUS))
from run_g1 import cut, sha, verify_build
from run_fp0 import TMPDIR, peak_kib

HOUSE = HERE.parents[2] / 'run_cases.py'
ORDER = HERE.parent / 'inputs/FP1_RESUME_4.md'
ORDER_SHA = '742c936b94e08cb02badd7f3b77757395f2123075ebcffe442cbd064ab1f363f'
PAIRS = (('LAB1', 'V2'), ('LAB2', 'V2'), ('LAB1', 'V5'), ('LAB2', 'V3'))
SELECTOR = '[.][NativeLocalCoreLab]'
WALL = 1200
DEADLINE = 'timeout: sending signal KILL to command '
EDITS = ('collapses', 'splits', 'edge_removals', 'face_swaps', 'moves')
COUNTERS = (*EDITS, 'evaluated_collapse', 'evaluated_split', 'evaluated_quality', 'deferred', 'skipped_memory',
            'blocked', 'refused_valid', 'refused_size', 'refused_topology', 'refused_memory', 'refused_numeric',
            'refused_shape', 'refused_exact')
STATES = {'load', 'before', 'pass', 'after', 'exact', 'strict'}
verify = previous['verify']
write_once = previous['write_once']


def round_table(sweeps):
    groups = {}
    for row in sweeps:
        kind, index, sweep = row['kind'], int(row['round']), int(row['sweep'])
        group = groups.setdefault((kind, index), {'kind': kind, 'round': index, 'first_sweep': sweep,
                                                 'sweeps': 0, **dict.fromkeys(EDITS, 0), 'seconds': 0.0})
        group['sweeps'] += 1
        group['seconds'] += float(row['seconds'])
        for name in EDITS:
            group[name] += int(row[name])
        group.update(last_sweep=sweep, live_cells=int(row['live_cells']))
    return list(groups.values())


def observe(log, code):
    cut_at = log.find(DEADLINE)
    timed_out = cut_at >= 0 and code in (124, 137)
    assert cut_at < 0 or timed_out, (cut_at, code)
    prefix = log[:cut_at] if timed_out else log
    rows, sweeps = previous['observe'](prefix)
    partial = prefix.rsplit('\n', 1)[-1]
    exact = rows.get('exact')
    valid = None if exact is None else exact['nonpositive'] == '0'
    passed = rows.get('pass')
    strict = rows.get('strict')
    strict_complete = (strict is not None and exact is not None and strict['cells'] == exact['cells'])
    if timed_out:
        state = 'wall_limit'
    elif code != 0:
        state = 'strict_audit_failed' if valid and 'after' in rows and not strict_complete else 'test_failure'
    else:
        state = 'completed' if set(rows) == STATES and valid and strict_complete else 'incomplete_evidence'
    error = re.search(r'unexpected exception with message:\s*([^\n]+)', prefix, re.IGNORECASE)
    counters = {name: sum(int(row[name]) for row in sweeps) for name in COUNTERS}
    by_kind = Counter(row['kind'] for row in sweeps)
    comparison = None if passed is None else (
        all(by_kind[kind] == int(passed[f'sweeps_{kind}']) for kind in ('collapse', 'split', 'quality')) and
        all(counters[name] == int(passed[name]) for name in COUNTERS))
    return {'state': state, 'pass_seconds': float(passed['seconds']) if passed else None,
            'self_check': passed['self_check'] == '1' if passed else None, 'exact_validity': valid,
            'strict_audit_completed': strict_complete, 'first_exception': error[1].strip() if error else None,
            'deadline_log_offset': cut_at if timed_out else None, 'unflushed_tail': partial or None,
            'rows': rows, 'sweeps': sweeps, 'round_table': round_table(sweeps), 'sweep_counters': counters,
            'sweeps_match_pass': comparison, 'completed_sweep_wall_seconds': math.fsum(float(row['seconds'])
                                                                                    for row in sweeps)}


def v2_holds(rows):
    return [row['dataset'] for row in rows if row['variant'] == 'V2' and
            (row['self_check'] is not True or row['exact_validity'] is not True or row['pass_seconds'] is None or
             row['pass_seconds'] > 600)]


def sample(root, sample_tree, peaks):
    live = []
    for row in sample_tree(root):
        pid = row['pid']
        try:
            identity = cut.actual_identity(pid)
            status = Path(f'/proc/{pid}/status').read_text().splitlines()
            mark = next((int(line.split()[1]) for line in status if line.startswith('VmHWM:')), 0)
            if cut.actual_identity(pid) != identity:
                continue
        except (OSError, ProcessLookupError):
            continue
        entry = {**row, **identity, 'hwm_kib': mark}
        key = (pid, identity['start_ticks'])
        if key not in peaks or mark > peaks[key]['hwm_kib']:
            peaks[key] = entry
        live.append(entry)
    return live


def owned(case):
    """Supervise the pinned timeout group from the outset and retain process evidence if it is killed."""
    assert case.parent == HERE / 'lab' and case.name in {f'{data}_{variant}' for data, variant in PAIRS}
    recipe = cut.read(case / 'command.json')
    assert recipe['argv'][-6:-1] == ['timeout', '--verbose', '--signal=KILL', str(WALL), recipe['binary']]
    assert recipe['argv'][-1] == SELECTOR
    sample_tree = runpy.run_path(str(HOUSE))['sample_tree']
    peaks = {}
    with (case / 'resources.jsonl').open('x') as output:
        start = time.monotonic()
        child = subprocess.Popen(recipe['argv'], start_new_session=True)
        while True:
            rows = sample(child.pid, sample_tree, peaks)
            output.write(json.dumps({'elapsed_seconds': time.monotonic() - start, 'processes': rows}) + '\n')
            output.flush()
            try:
                code = child.wait(timeout=1)
                break
            except subprocess.TimeoutExpired:
                continue
    survivors = [row for row in peaks.values() if not cut.terminal(row)]
    if survivors and code in (124, 137, -signal.SIGKILL) and DEADLINE in (case / 'test_outer.log').read_text(errors='replace'):
        # A singleton MPI helper can leave the timeout process group. Remove only witnessed, still-matching descendants.
        for row in survivors:
            if not cut.terminal(row):
                try:
                    os.kill(row['pid'], signal.SIGKILL)
                except ProcessLookupError:
                    pass
    for _ in range(10):
        if all(cut.terminal(row) for row in survivors):
            break
        time.sleep(.1)
    terminal = all(cut.terminal(row) for row in peaks.values())
    cut.save(case / 'owned_processes.json', {'all_observed_terminal': terminal, 'identities': list(peaks.values()),
             'sampled_job_peak_lower_bound_kib': sum(row['hwm_kib'] for row in peaks.values()),
             'sampled_test_peak_kib': max((row['hwm_kib'] for row in peaks.values() if row['name'] == 'test_driver'),
                                          default=None)})
    assert terminal, survivors
    return code if code >= 0 else 128 - code


def launch(case, binary, cfg, sensor, variant):
    assert not any((case / name).exists() for name in ('test_launch.json', 'test_outer.log', 'resources.jsonl')), case
    if (case / 'command.json').exists():
        prior = case / f'preflight_{time.time_ns()}'
        prior.mkdir()
        for name in ('command.json', 'test_host_preflight.json'):
            if (case / name).exists():
                (case / name).rename(prior / name)
    settings = ('LC_ALL=C', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1', 'OMPI_MCA_osc=pt2pt',
                f'TMPDIR={TMPDIR}', f'SU2_NATIVE_FROZEN_CONFIG={cfg}', f'SU2_NATIVE_FROZEN_SENSOR3D={sensor}',
                f'SU2_NATIVE_LOCAL_LAB_VARIANT={variant}')
    argv = ['env', '-C', str(case), *settings, 'nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(case / 'time.txt'),
            'timeout', '--verbose', '--signal=KILL', str(WALL), str(binary), SELECTOR]
    wrapper = ['nice', '-n', '19', cut.PYTHON, '-B', str(Path(__file__).resolve()), '--owned', str(case)]
    cut.save(case / 'command.json', {'argv': argv, 'wrapper_argv': wrapper, 'ranks': 1, 'selector': SELECTOR,
                                    'seconds_limit': WALL, 'binary': str(binary)})
    try:
        cut.run_gate(case, 'test', wrapper)
    except RuntimeError:
        wait = cut.read(case / 'test_wait.json')
        assert wait['terminal'] and cut.terminal(wait['controller'])


def finish(case, dataset, variant, pins, counts):
    wait = cut.read(case / 'test_wait.json')
    assert wait['controller'] == cut.read(case / 'test_launch.json')['controller']
    assert wait['terminal'] and cut.terminal(wait['controller'])
    processes = cut.read(case / 'owned_processes.json')
    assert processes['all_observed_terminal'] and all(cut.terminal(row) for row in processes['identities'])
    verify(pins)
    result = observe((case / 'test_outer.log').read_text(errors='replace'), wait['returncode'])
    rows = result['rows']
    if 'before' in rows:
        assert all(int(rows['before'][key]) == counts[key] for key in ('cells', 'points'))
    if 'pass' in rows:
        assert rows['pass']['variant'] == variant
        if 'exact' in rows:
            assert rows['pass']['cells_out'] == rows['exact']['cells']
    outer_peak = peak_kib(case / 'time.txt') if (case / 'time.txt').is_file() else None
    lower = processes['sampled_job_peak_lower_bound_kib']
    peak = processes['sampled_test_peak_kib'] if result['state'] == 'wall_limit' else outer_peak
    memory = (False if max(lower, peak or 0) > 19073 * 1024 else
              None if peak is None or result['state'] == 'wall_limit' else True)
    artifacts = ('command.json', 'test_host_preflight.json', 'test_launch.json', 'test_wait.json', 'test_outer.log',
                 'time.txt', 'resources.jsonl', 'owned_processes.json')
    receipt = {'dataset': dataset, 'variant': variant, 'wait': wait, 'peak_rss_kib': peak,
               'gnu_time_reported_peak_kib': outer_peak, 'sampled_job_peak_lower_bound_kib': lower,
               'within_job_memory': memory, **result, 'pins': pins,
               'artifacts': {name: sha(case / name) for name in artifacts if (case / name).is_file()}}
    write_once(case / 'lab_receipt.json', receipt)
    return {key: value for key, value in receipt.items() if key not in ('pins', 'artifacts', 'sweeps')}


def run(build, verify_only):
    status_path, source_pins = build / 'build_status.json', build / 'source_pins.json'
    status = cut.read(status_path)
    binary, source_sha = verify_build(status, source_pins)
    kept = (PREVIOUS, previous['MANIFEST'], Path(__file__).resolve(), previous['FP0'] / 'run_fp0.py',
            HERE.parent / 'run_g1.py', cut.PREFLIGHT, HOUSE)
    pins = {**{str(path): sha(path) for path in kept}, str(ORDER): ORDER_SHA,
            str(previous['ORDER']): previous['ORDER_SHA'], str(status_path): sha(status_path),
            str(source_pins): source_sha, str(binary): status['test_binary_sha256'], status['binary']: status['binary_sha256']}
    verify(pins)
    inputs = {name: previous['load_input'](name) for name in ('LAB1', 'LAB2')}
    if verify_only:
        return {'verified': True, 'launched': False, 'build': status['build'], 'pairs': PAIRS,
                'config_sha256': {name: hashlib.sha256(rows[0]).hexdigest() for name, rows in inputs.items()}}
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert status['wait']['terminal'] and cut.terminal(status['wait']['controller'])
    for key in tuple(os.environ):
        if key.startswith('SU2_NATIVE_'):
            del os.environ[key]
    root = HERE / 'lab'
    root.mkdir(exist_ok=True)
    results = []
    for dataset, variant in PAIRS:
        case = root / f'{dataset}_{variant}'
        case.mkdir(exist_ok=True)
        cfg, sensor, bound = previous['prepare'](case, inputs[dataset], pins)
        if not (case / 'test_wait.json').exists():
            launch(case, binary, cfg, sensor, variant)
        result = finish(case, dataset, variant, bound, inputs[dataset][-1])
        verify_build(status, source_pins)
        results.append(result)
        print(f"{case.name}: {result['state']}, pass wall {result['pass_seconds']}, "
              f"exact validity {result['exact_validity']}", flush=True)
    summary = {'build': status['build'], 'test_binary_sha256': status['test_binary_sha256'],
               'source_pins_sha256': source_sha, 'v2_hold': v2_holds(results), 'results': results}
    write_once(root / 'lab_summary.json', summary)
    return summary


def self_check():
    fields = dict.fromkeys(COUNTERS, 0)
    fields['splits'] = 3
    sweep = ('NATIVE3D_LOCAL_SWEEP rank=0 stage=0 pass=1 kind=split round=1 sweep=5 live_cells=23 seconds=0.25 ' +
             ' '.join(f'{name}={value}' for name, value in fields.items()) + '\n')
    interrupted = sweep + sweep.replace('sweep=5', 'sweep=6').rstrip('\n') + DEADLINE + "'/bin/test'\n"
    stopped = observe(interrupted, 137)
    assert stopped['state'] == 'wall_limit' and stopped['unflushed_tail'].startswith('NATIVE3D_LOCAL_SWEEP ')
    assert len(stopped['sweeps']) == 1 and stopped['round_table'][0]['live_cells'] == 23
    totals = ' '.join(f'{name}={value}' for name, value in fields.items())
    done = ('FP1_LAB load variant=V2 seconds=10\nFP1_LAB before cells=20\n' + sweep +
            'FP1_LAB pass variant=V2 seconds=600 self_check=1 sweeps_collapse=0 sweeps_split=1 '
            f'sweeps_quality=0 cells_out=23 {totals}\nFP1_LAB exact cells=23 nonpositive=0\nFP1_LAB after cells=23\n')
    failure = observe(done + 'due to unexpected exception with message:\n  Degenerate metric donor\n', 1)
    assert failure['state'] == 'strict_audit_failed' and failure['exact_validity'] is True
    assert failure['first_exception'] == 'Degenerate metric donor' and not failure['strict_audit_completed']
    clean = observe(done + 'FP1_LAB strict cells=23 q_bad=4 j_bad=2 long_cells=2\n', 0)
    assert clean['state'] == 'completed' and clean['strict_audit_completed'] and clean['sweeps_match_pass']
    assert clean['round_table'][0]['splits'] == 3 and clean['completed_sweep_wall_seconds'] == .25
    assert v2_holds([dict(clean, dataset='LAB1', variant='V2'),
                    dict(clean, pass_seconds=600.001, dataset='LAB2', variant='V2')]) == ['LAB2']
    print('FP1 H6 lab parser self-check passed.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--owned', type=Path, help='internal timeout supervisor')
    args = parser.parse_args()
    if args.owned:
        raise SystemExit(owned(args.owned.resolve()))
    if args.self_check:
        self_check()
    elif args.build:
        print(json.dumps(run(args.build.resolve(), args.verify_only), indent=2))
    else:
        parser.error('provide a sealed build directory or --self-check')
