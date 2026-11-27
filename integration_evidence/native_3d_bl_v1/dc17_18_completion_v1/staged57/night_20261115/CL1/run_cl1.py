"""Launch the single CL1 workflow and preserve the actual per-rank wait receipts."""

import argparse
from datetime import datetime, timezone
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
CASE = HERE / 'run'
REPO = HERE.parents[5]
REFERENCE = NIGHT / 'FP3_E/accepted_reference.json'
BASE = 'c31f95b071'
SOLVER_SOURCE = '62a270c4d6'
BUILD_FIXTURE = '04f7bcd8ea'
TEMPLATE_CASE = NIGHT / 'FP3_D/G5/build678/S1/trial1/full678_run1'
TEMPLATE_PROVENANCE = TEMPLATE_CASE / 'provenance.json'
INPUT_FILES = ('m6.su2', 'm6.su2.native_ref', 'baseline_euler_seed.dat')
ORDER = {'CL1_CONVERGED_LOOP_RUN.md': '020525a8fde98f034c8c9e3e28eb1034147cb499de9d7e7ceeb9f83fbb207817',
         'shock_refinement_measure.py': 'b74990859163f7936d897f886928eb79200d63ce27322fa59864bf96b80dc1ee'}
NATIVE = {'SU2_NATIVE_GEOMETRY_WORK_TRACE': '0', 'SU2_NATIVE_FP3_PROBE': '0'}
THREADS = ('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS')
RANKS = 6
WATCHDOG = 21600
EDITOR = {
    'ADAP_SIZES': '(50000)', 'ADAP_SUBITER': '(4)', 'ITER': '1', 'ADAP_FLOW_ITER': '(2000)',
    'ADAP_FLOW_CFL': '(5.0)', 'CFL_ADAPT': 'NO', 'LINEAR_SOLVER_ERROR': '1E-6', 'LINEAR_SOLVER_ITER': '20',
    'CONV_FIELD': '(LIFT, DRAG)', 'CONV_CAUCHY_ELEMS': '100', 'CONV_CAUCHY_EPS': '1E-7',
    'CONV_STARTITER': '300', 'HISTORY_OUTPUT': '(ITER, RMS_RES, AERO_COEFF, CAUCHY, LINSOL, CFL_NUMBER)',
    'OUTPUT_FILES': '(RESTART, PARAVIEW, SURFACE_PARAVIEW)', 'OUTPUT_WRT_FREQ': '(100000)',
}
OPTION = re.compile(r'^([A-Z][A-Z0-9_]*)\s*=\s*(.*?)\s*$')


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def save(path, data):
    scratch = path.with_suffix('.tmp')
    scratch.write_text(json.dumps(data, indent=2) + '\n')
    scratch.replace(path)


def utc():
    return datetime.now(timezone.utc).isoformat()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=REPO)


def committed(path, revision):
    return git('show', f'{revision}:{path.relative_to(REPO)}')


def checked(path, digest):
    assert sha(path) == digest, path
    return Path(path)


def pin(path):
    return {'path': str(path), 'sha256': sha(path)}


def ranked_env(environment):
    return {name: value for name, value in environment.items() if
            name.startswith('SU2_NATIVE_') or name in (*THREADS, 'TMPDIR', 'OMPI_MCA_orte_tmpdir_base', 'OMPI_MCA_osc')}


def plan():
    for filename, digest in ORDER.items():
        checked(HERE / 'inputs' / filename, digest)
    lines = (HERE / 'inputs/SHA256SUMS').read_text().splitlines()
    assert dict(line.split(maxsplit=1)[::-1] for line in lines) == ORDER
    assert b'FP3_E/accepted_reference.json' in (REPO / 'AGENTS.md').read_bytes()
    assert REFERENCE.read_bytes() == committed(REFERENCE, BASE)
    reference = load(REFERENCE)
    assert reference['accepted'] and reference['build'] == 680
    run_pin = reference['workflows']['S']['runs'][0]['full_run_receipt']
    sealed = load(checked(run_pin['path'], run_pin['sha256']))
    assert sealed['passed'] and sealed['probe'] == 0
    assert sealed['binary_sha256'] == reference['binary_sha256']
    build_path = REFERENCE.parent / 'build680/build_status.json'
    assert build_path.read_bytes() == committed(build_path, BASE)
    build = load(build_path)
    assert build['passed'] and build['binary_sha256'] == sealed['binary_sha256']
    binary = checked(build['binary'], build['binary_sha256'])
    first_build_path = REFERENCE.parent / 'build679/build_status.json'
    assert first_build_path.read_bytes() == committed(first_build_path, BASE)
    assert load(first_build_path)['binary_sha256'] == build['binary_sha256']
    source_paths = [name for name in build['changed'] if name.startswith('Common/')]
    assert not git('diff', SOLVER_SOURCE, '--', *source_paths)
    assert not git('status', '--porcelain', '-uno', '--', 'Common', 'SU2_CFD', 'UnitTests', 'AGENTS.md')
    source_manifest = checked(build_path.parent / 'source_pins.json', build['source_pins_sha256'])
    pinned_sources = load(source_manifest)
    for name in source_paths:
        assert hashlib.sha256(committed(REPO / name, SOLVER_SOURCE)).hexdigest() == pinned_sources[str(REPO / name)]

    assert TEMPLATE_PROVENANCE.read_bytes() == committed(TEMPLATE_PROVENANCE, BASE)
    template = load(TEMPLATE_PROVENANCE)
    config_in = checked(TEMPLATE_CASE / 'run.cfg', template['config_sha256'])
    assert template['input_hashes'][str(config_in)] == template['config_sha256']
    originals = {}
    for name in INPUT_FILES:
        path = (TEMPLATE_CASE / name).resolve(strict=True)
        checked(path, template['input_hashes'][str(path)])
        originals[name] = pin(path)

    template_text = config_in.read_text()
    config_lines, found = [], set()
    for line in template_text.splitlines(keepends=True):
        match = OPTION.fullmatch(line.rstrip('\n'))
        if match and match[1] in EDITOR:
            key, old = match.groups()
            assert key not in found
            found.add(key)
            if old != EDITOR[key]:
                line = f'{key}= {EDITOR[key]}\n'
        config_lines.append(line)
    config_lines += [f'{key}= {value}\n' for key, value in EDITOR.items() if key not in found]
    config = ''.join(config_lines)
    differences = ''.join(difflib.unified_diff(template_text.splitlines(keepends=True), config_lines,
                                               fromfile=str(config_in), tofile='CL1/run/run.cfg'))

    environment = {key: value for key, value in template['environment'].items() if not key.startswith('SU2_NATIVE_')}
    assert all(environment[key] == '1' for key in THREADS)
    environment.update(NATIVE)
    environment['OMPI_MCA_orte_tmpdir_base'] = str(CASE / 'mpi_tmp')
    inherited = template['argv']
    mpi = inherited[inherited.index('mpirun'):-2]
    mpi[mpi.index('--output-filename') + 1] = str(CASE / 'rank_output')
    assert mpi[mpi.index('-n') + 1] == str(RANKS)
    assert {mpi[i + 1] for i, token in enumerate(mpi) if token == '-x'} == NATIVE.keys()

    argv = [*mpi, sys.executable, '-B', str(Path(__file__).resolve()), 'rank', str(CASE), str(binary), 'run.cfg']
    return config, {
        'build': build['build'], 'checkout_commit': git('rev-parse', 'HEAD').decode().strip(),
        'binary_source_commit': git('rev-parse', SOLVER_SOURCE).decode().strip(), 'qualification_commit': git('rev-parse', BUILD_FIXTURE).decode().strip(),
        'binary': str(binary), 'binary_sha256': build['binary_sha256'], 'config_diff': differences,
        'config_sha256': hashlib.sha256(config.encode()).hexdigest(), 'argv': argv, 'environment': environment,
        'ranks': RANKS, 'nice': 19, 'watchdog_seconds': WATCHDOG, 'original_inputs': originals,
        'reference': pin(REFERENCE), 'build_status': pin(build_path), 'source_pins': pin(source_manifest),
        'sealed_run': run_pin, 'template': pin(config_in), 'template_provenance': pin(TEMPLATE_PROVENANCE),
        'controller': pin(Path(__file__).resolve()),
    }


def process(pid):
    try:
        fields = Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()
        return dict(pid=pid, state=fields[0], ppid=int(fields[1]), pgid=int(fields[2]),
                    start_ticks=int(fields[19]), comm=Path(f'/proc/{pid}/comm').read_text().strip())
    except (OSError, ValueError, IndexError):
        return None


def rank(case, binary, args):
    actual = ranked_env(os.environ)
    assert {k: v for k, v in actual.items() if k.startswith('SU2_NATIVE_')} == NATIVE
    assert all(actual[key] == '1' for key in THREADS)
    number = int(os.environ['OMPI_COMM_WORLD_RANK'])
    expected = load(case / 'launch.json')
    assert ranked_env(expected['environment']) == actual
    assert 0 <= number < expected['ranks'] == int(os.environ['OMPI_COMM_WORLD_SIZE'])
    assert os.nice(0) == 19
    forwarded = []
    child = None

    def forward(signum, _):
        forwarded.append(signum)
        if child is not None:
            try:
                os.kill(child, signum)
            except ProcessLookupError:
                pass

    signal.signal(signal.SIGTERM, forward)
    signal.signal(signal.SIGINT, forward)
    started = utc()
    child = os.posix_spawn(str(binary), [str(binary), *args], os.environ,
                          setsigdef=(signal.SIGTERM, signal.SIGINT))
    for signum in tuple(forwarded):
        try:
            os.kill(child, signum)
        except ProcessLookupError:
            pass
    identity = process(child)
    save(case / 'rank_receipts' / f'rank{number}_start.json', {
        'rank': number, 'world_size': expected['ranks'], 'settings': actual, 'nice': os.nice(0),
        'child': identity, 'wrapper': process(os.getpid()), 'started_utc': started,
        'exec': str(binary),
    })
    _, status, usage = os.wait4(child, 0)
    child_pid, child = child, None
    returncode = os.waitstatus_to_exitcode(status)
    save(case / 'rank_receipts' / f'rank{number}_wait.json', {
        'rank': number, 'pid': child_pid, 'child': identity, 'start_utc': started, 'finished_utc': utc(),
        'returncode': returncode, 'wait_status': status, 'peak_rss_bytes': usage.ru_maxrss * 1024,
        'user_seconds': usage.ru_utime, 'system_seconds': usage.ru_stime, 'forwarded_signals': forwarded,
        'method': 'os.wait4 direct solver child; Linux ru_maxrss in KiB',
    })
    return returncode if returncode >= 0 else 128 - returncode


def snapshot(root, seen, case):
    rows = {row['pid']: row for path in Path('/proc').iterdir() if path.name.isdecimal()
            if (row := process(int(path.name)))}
    identities = [root]
    for path in (case / 'rank_receipts').glob('rank*_start.json'):
        row = load(path)
        identities.extend(item for item in (row['child'], row['wrapper']) if item)
    for row in identities:
        seen.setdefault(row['pid'], row)
    live = {pid for pid, row in seen.items() if pid in rows and rows[pid]['start_ticks'] == row['start_ticks']}
    while True:
        extra = {pid for pid, row in rows.items() if row['ppid'] in live and pid not in live}
        if not extra:
            break
        live |= extra
        seen.update({pid: rows[pid] for pid in extra})
    return [rows[pid] for pid in sorted(live) if rows[pid]['state'] not in ('Z', 'X')]


def resource_row(live):
    solver = []
    for row in live:
        if row['comm'] != 'SU2_CFD':
            continue
        try:
            fields = dict(line.split(':', 1) for line in Path(f"/proc/{row['pid']}/status").read_text().splitlines()
                          if ':' in line)
            solver.append({**row, 'rss_kib': int(fields['VmRSS'].split()[0]),
                           'hwm_kib': int(fields['VmHWM'].split()[0])})
        except (KeyError, OSError, ValueError):
            pass
    available = next(line for line in Path('/proc/meminfo').read_text().splitlines()
                     if line.startswith('MemAvailable:'))
    return {'utc': utc(), 'unix': time.time(), 'available_kib': int(available.split()[1]),
            'loadavg': Path('/proc/loadavg').read_text().strip(), 'ranks': solver}


def stop_owned(root, child, seen, case):
    signals = []
    for signum, grace in ((signal.SIGTERM, 10), (signal.SIGKILL, 5)):
        live = snapshot(root, seen, case)
        if not live:
            break
        if any(row['pid'] == root['pid'] and row['pgid'] == root['pid'] for row in live):
            try:
                os.killpg(root['pgid'], signum)
                signals.append({'group': root['pgid'], 'signal': signum.name, 'utc': utc()})
            except ProcessLookupError:
                pass
        # MPI may create separate rank process groups.
        for row in live:
            if row['pgid'] == root['pgid']:
                continue
            current = process(row['pid'])
            if current and current['start_ticks'] == row['start_ticks'] and current['state'] not in ('Z', 'X'):
                try:
                    os.kill(row['pid'], signum)
                    signals.append({'process': row, 'signal': signum.name, 'utc': utc()})
                except ProcessLookupError:
                    pass
        deadline = time.monotonic() + grace
        while time.monotonic() < deadline:
            child.poll()
            if not snapshot(root, seen, case):
                break
            time.sleep(0.2)
    return signals


def invocation(case, recipe, environment, watchdog):
    interrupted = []

    def interrupt(signum, _):
        interrupted.append(signal.Signals(signum).name)

    old_handlers = {sig: signal.signal(sig, interrupt) for sig in (signal.SIGINT, signal.SIGTERM)}
    start = time.monotonic()
    cause = 'natural'
    cleanup = []
    controller_error = None
    seen = {}
    with (case / 'run.log').open('x') as log, (case / 'resources.jsonl').open('x') as resources:
        child = subprocess.Popen(recipe, cwd=case, env=environment, start_new_session=True,
                                 stdout=log, stderr=subprocess.STDOUT)
        root = process(child.pid)
        assert root and root['pgid'] == child.pid
        save(case / 'owner.json', root)
        try:
            while child.poll() is None:
                live = snapshot(root, seen, case)
                resources.write(json.dumps(resource_row(live)) + '\n')
                resources.flush()
                if interrupted or time.monotonic() - start >= watchdog:
                    cause = 'interruption' if interrupted else 'watchdog'
                    cleanup = stop_owned(root, child, seen, case)
                    break
                time.sleep(min(2, max(0, watchdog - (time.monotonic() - start))))
            returncode = child.wait(timeout=5)
            if snapshot(root, seen, case):
                cleanup.extend(stop_owned(root, child, seen, case))
        except Exception as error:
            cause = cause if cause != 'natural' else 'controller_error'
            controller_error = f'{type(error).__name__}: {error}'
            cleanup.extend(stop_owned(root, child, seen, case))
            returncode = child.poll()
        finally:
            for sig, handler in old_handlers.items():
                signal.signal(sig, handler)
        alive = snapshot(root, seen, case)
        waits = [load(path) for path in sorted((case / 'rank_receipts').glob('rank*_wait.json'))]
        terminal = {'cause': cause, 'returncode': returncode, 'interruption_signals': interrupted,
                    'controller_error': controller_error,
                    'elapsed_seconds': time.monotonic() - start, 'finished_utc': utc(),
                    'all_owned_handles_terminal': not alive, 'live_handles': alive,
                    'owned_identities': list(seen.values()), 'launcher': root, 'signals': cleanup, 'rank_waits': waits}
        save(case / 'terminal.json', terminal)
        return terminal


def launch():
    assert os.nice(0) == 19
    config, receipt = plan()
    CASE.mkdir(exist_ok=True)
    assert not (CASE / 'launch.json').exists() and not (CASE / 'run.log').exists(), 'one CL1 solver launch'
    subprocess.run([sys.executable, str(NIGHT.parent / 'primary_control_archive/end_b_actual_preflight.py'),
                    str(CASE / 'host_preflight.json')], cwd=REPO, check=True)
    assert shutil.disk_usage(CASE).free >= 2 * 1024**3
    for name, entry in receipt['original_inputs'].items():
        destination = CASE / name
        if not destination.is_file():
            destination.symlink_to(entry['path'])
        checked(destination, entry['sha256'])
    cfg = CASE / 'run.cfg'
    if cfg.is_file():
        assert cfg.read_text() == config
    else:
        cfg.write_text(config)
    checked(cfg, receipt['config_sha256'])
    (CASE / 'mpi_tmp').mkdir(exist_ok=True)
    (CASE / 'rank_receipts').mkdir(exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith('SU2_NATIVE_')}
    env.update(receipt['environment'])
    receipt['started_utc'] = utc()
    save(CASE / 'launch.json', receipt)
    result = invocation(CASE, receipt['argv'], env, WATCHDOG)
    stable = {str(cfg): sha(cfg) == receipt['config_sha256'],
              receipt['binary']: sha(receipt['binary']) == receipt['binary_sha256'],
              **{str(CASE / name): sha(CASE / name) == entry['sha256']
                 for name, entry in receipt['original_inputs'].items()}}
    result['inputs_verified_after'] = stable
    save(CASE / 'terminal.json', result)
    print(json.dumps({key: result[key] for key in ('cause', 'returncode', 'elapsed_seconds',
                                                   'all_owned_handles_terminal')}), flush=True)


def prepare():
    config, receipt = plan()
    CASE.mkdir(exist_ok=True)
    cfg = CASE / 'run.cfg'
    if cfg.is_file():
        assert cfg.read_text() == config
    else:
        cfg.write_text(config)
    (HERE / 'reports/config.diff').write_text(receipt['config_diff'])
    save(HERE / 'reports/input_authentication.json', receipt)


def self_check():
    assert os.nice(0) == 19
    with tempfile.TemporaryDirectory(prefix='su2-cl1-preflight-') as directory:
        case = Path(directory)
        short_mpi_tmp = case / 'mpi_tmp'
        short_mpi_tmp.mkdir()
        (case / 'rank_receipts').mkdir()
        expected = {**NATIVE, **dict.fromkeys(THREADS, '1'), 'OMPI_MCA_orte_tmpdir_base': str(short_mpi_tmp)}
        save(case / 'launch.json', {'ranks': RANKS, 'environment': expected})
        environment = {k: v for k, v in os.environ.items() if k not in ranked_env(os.environ)}
        environment.update(expected)
        recipe = ['mpirun', '--bind-to', 'none', '-n', str(RANKS), '-x', 'SU2_NATIVE_FP3_PROBE',
                  '-x', 'SU2_NATIVE_GEOMETRY_WORK_TRACE', '--output-filename', str(case / 'rank_output'),
                  sys.executable, '-B', str(Path(__file__).resolve()), 'rank', str(case), '/usr/bin/sleep', '0.25']
        result = invocation(case, recipe, environment, 60)
        assert result['returncode'] == 0 and result['all_owned_handles_terminal']
        assert [row['rank'] for row in result['rank_waits']] == list(range(RANKS))
        assert all(row['peak_rss_bytes'] > 0 and row['returncode'] == 0 and row['child'] for row in result['rank_waits'])
        print('Six-rank launch, settings and wait4 self-check passed without a solver launch.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('prepare', 'self-check', 'launch', 'rank'))
    parser.add_argument('rank_args', nargs='*')
    cli = parser.parse_args()
    if cli.action == 'rank':
        case, path, *args = cli.rank_args
        raise SystemExit(rank(Path(case), Path(path), args))
    {'prepare': prepare, 'self-check': self_check, 'launch': launch}[cli.action]()
