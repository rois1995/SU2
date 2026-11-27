"""Launch CL2 or serial MMG2 with the CL1 watchdog and rank wait receipts."""

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
REPO = HERE.parents[5]
DEF1 = NIGHT / 'DEF1'
CL1 = NIGHT / 'CL1'
CL1_COMMIT = 'efbab5351e'
INPUT_FILES = ('m6.su2', 'm6.su2.native_ref', 'baseline_euler_seed.dat')
ORDER = {'DEF1_METRIC_DEFAULTS.md': '480c7ddeb2751bc4748b5cd1105011e80dfa188f8bfdd816419084105bb80025',
         'shock_refinement_measure.py': 'b74990859163f7936d897f886928eb79200d63ce27322fa59864bf96b80dc1ee'}
NATIVE = {'SU2_NATIVE_GEOMETRY_WORK_TRACE': '0', 'SU2_NATIVE_FP3_PROBE': '0'}
THREADS = ('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS')
RANKS = 6
WATCHDOG = 21600
HAUSD = '(WING3D, 1E-3, SYMMETRY, 1E-3, FARFIELD, 1.0)'
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


def verify_order():
    for filename, digest in ORDER.items():
        checked(DEF1 / 'inputs' / filename, digest)
    sums = (DEF1 / 'inputs/SHA256SUMS').read_text().splitlines()
    listed = dict(line.split(maxsplit=1)[::-1] for line in sums)
    assert all(listed.get(name) == digest for name, digest in ORDER.items())


def derive(template_text, mode):
    rows, found = [], {}
    for line in template_text.splitlines(keepends=True):
        match = OPTION.fullmatch(line.rstrip('\n'))
        if match:
            name, value = match.groups()
            assert name not in found
            found[name] = value
            if mode == 'CL2' and name == 'ADAP_HGRAD':
                assert float(value) == 1.3
                continue
            if mode == 'CL2' and name == 'ADAP_HMIN':
                line = 'ADAP_HMIN= 3E-3\n'
            if mode == 'MMG2' and name.startswith('ADAP_NATIVE_'):
                continue
            if mode == 'MMG2' and name == 'ADAP_REMESHER':
                assert value == 'NATIVE_CAVITY'
                line = 'ADAP_REMESHER= MMG\n'
            if mode == 'MMG2' and name == 'ADAP_HAUSD':
                line = f'ADAP_HAUSD= {HAUSD}\n'
        rows.append(line)
    assert 'ADAP_NORM' not in found and 'ADAP_NORMS' not in found
    if mode == 'CL2':
        assert 'ADAP_HGRAD' in found and 'ADAP_HMIN' in found and found['ADAP_REMESHER'] == 'NATIVE_CAVITY'
    else:
        assert 'ADAP_HGRAD' not in found and found['ADAP_HMIN'] == '3E-3'
        assert 'ADAP_REMESHER' in found
        if 'ADAP_HAUSD' not in found:
            rows.append(f'ADAP_HAUSD= {HAUSD}\n')
    config = ''.join(rows)
    assert config != template_text
    return config


def checks_passed(standing_path):
    standing_path = Path(standing_path).resolve()
    assert standing_path.parent.parent == DEF1 and standing_path.name == 'section3.json'
    assert standing_path.read_bytes() == committed(standing_path, 'HEAD')
    standing = load(standing_path)
    assert standing['passed'] is True and standing['status'] == 'passed'
    status_pin = standing['build_status']
    build_path = checked(status_pin['path'], status_pin['sha256'])
    assert build_path.parent == standing_path.parent and build_path.name == 'build_status.json'
    build = load(build_path)
    assert build['passed'] is True and standing['binary_sha256'] == build['binary_sha256']
    checked(build['binary'], build['binary_sha256'])
    source = standing['source_commit']
    assert git('rev-parse', source).decode().strip() == source
    subprocess.run(['git', 'merge-base', '--is-ancestor', source, 'HEAD'], cwd=REPO, check=True)
    assert not git('diff', source, '--', 'Common', 'SU2_CFD', 'UnitTests', 'TestCases', 'config_template.cfg')
    assert standing['gates']
    return standing, build


def cl1_template():
    for name in ('launch.json', 'terminal.json', 'run.cfg'):
        path = CL1 / 'run' / name
        assert path.read_bytes() == committed(path, CL1_COMMIT)
    manifest = load(CL1 / 'run/launch.json')
    terminal = load(CL1 / 'run/terminal.json')
    assert terminal['all_owned_handles_terminal'] and terminal['returncode'] == 0
    template = checked(CL1 / 'run/run.cfg', manifest['config_sha256'])
    for name in INPUT_FILES:
        expected = manifest['original_inputs'][name]
        linked = (CL1 / 'run' / name).resolve(strict=True)
        assert linked == Path(expected['path']).resolve() and terminal['inputs_verified_after'][str(CL1 / 'run' / name)]
        checked(linked, expected['sha256'])
    return template, manifest


def terminal_case(case):
    launch, terminal, validation = (case / name for name in ('launch.json', 'terminal.json', 'validation.json'))
    receipt = load(launch)
    report_path = case.parent / 'reports' / f"{receipt['mode']}_{receipt['build']}.json"
    for path in (launch, terminal, validation, report_path, case / 'run.cfg'):
        assert path.read_bytes() == committed(path, 'HEAD')
    status = load(terminal)
    report = load(report_path)
    assert status['all_owned_handles_terminal'] is True and status['live_handles'] == []
    assert report['runtime_defaults'] == load(validation)['runtime_defaults']
    pins = {'launch': pin(launch), 'terminal': pin(terminal), 'validation': pin(validation), 'report': pin(report_path)}
    return receipt, report, pins


def prerequisite(mode, standing_path, cl2, binary_digest, original):
    if mode == 'CL2':
        return None
    receipt, report, pins = terminal_case(cl2)
    assert receipt['mode'] == 'CL2'
    assert not report['runtime_defaults']['wiring_defect'], 'CL2 showed a wrong effective default'
    assert report.get('runtime_repeat', {}).get('status', 'passed') == 'passed'
    if original is None:
        assert receipt['binary_sha256'] == binary_digest
        assert receipt['standing'] == pin(standing_path)
    else:
        assert original['launch_record']['cl2_prerequisite'] == pins
    return pins


def first_attempt(case, mode, binary_digest):
    if case.parent.name != 'repeat':
        return None
    receipt, report, pins = terminal_case(NIGHT / mode / 'run')
    assert receipt['mode'] == mode and receipt['binary_sha256'] != binary_digest
    assert report['runtime_defaults']['wiring_defect'], 'a capability result never authorizes a second run'
    assert receipt.get('original_attempt') is None
    return {'launch_record': receipt, 'artifacts': pins, 'runtime_defaults': report['runtime_defaults']}


def plan(case, mode, standing_path, cl2):
    assert mode in ('CL2', 'MMG2')
    verify_order()
    standing, build = checks_passed(standing_path)
    template, cl1 = cl1_template()
    originals = {name: cl1['original_inputs'][name] for name in INPUT_FILES}
    cl1_config = template.read_text()
    base = derive(cl1_config, 'CL2')
    original = first_attempt(case, mode, build['binary_sha256'])
    previous = prerequisite(mode, standing_path, cl2, build['binary_sha256'], original)
    if mode == 'MMG2':
        template = cl2 / 'run.cfg'
        assert template.read_text() == base
        assert sha(template) == load(cl2 / 'launch.json')['config_sha256']
    template_text = template.read_text()
    config = derive(template_text, mode)
    config_hash = hashlib.sha256(config.encode()).hexdigest()
    if original:
        assert config_hash == original['launch_record']['config_sha256']
    differences = ''.join(difflib.unified_diff(template_text.splitlines(keepends=True),
                                               config.splitlines(keepends=True),
                                               fromfile=str(template), tofile=f'{mode}/run/run.cfg'))
    environment = {key: value for key, value in cl1['environment'].items() if not key.startswith('SU2_NATIVE_')}
    assert all(environment[key] == '1' for key in THREADS)
    if mode == 'CL2':
        environment.update(NATIVE)
    environment['OMPI_MCA_orte_tmpdir_base'] = str(case / 'mpi_tmp')
    argv = ['mpirun', '--bind-to', 'none', '-n', str(RANKS), '--timestamp-output',
            '--output-filename', str(case / 'rank_output')]
    for key in NATIVE if mode == 'CL2' else ():
        argv += ['-x', key]
    argv += [sys.executable, '-B', str(Path(__file__).resolve()), 'rank', str(case), build['binary'], 'run.cfg']
    return config, {
        'mode': mode, 'build': build['build'], 'checkout_commit': git('rev-parse', 'HEAD').decode().strip(),
        'binary_source_commit': standing['source_commit'],
        'binary': build['binary'], 'binary_sha256': build['binary_sha256'], 'config_diff': differences,
        'config_sha256': config_hash, 'argv': argv, 'environment': environment,
        'ranks': RANKS, 'nice': 19, 'watchdog_seconds': WATCHDOG, 'original_inputs': originals,
        'standing': pin(standing_path), 'build_status': standing['build_status'],
        'template': pin(template), 'cl1_launch': pin(CL1 / 'run/launch.json'),
        'cl1_terminal': pin(CL1 / 'run/terminal.json'), 'cl2_prerequisite': previous,
        'controller': pin(Path(__file__).resolve()),
        'original_attempt': original['artifacts'] if original else None,
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
    assert all(actual[key] == '1' for key in THREADS)
    number = int(os.environ['OMPI_COMM_WORLD_RANK'])
    expected = load(case / 'launch.json')
    assert ranked_env(expected['environment']) == actual
    native = NATIVE if expected.get('mode') == 'CL2' else {}
    assert {name: value for name, value in actual.items() if name.startswith('SU2_NATIVE_')} == native
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


def no_reviewer_unit():
    active = subprocess.check_output(['systemctl', '--user', 'list-units', '--state=active,activating',
                                      '--no-legend', '--plain', 'reviewer-*'], text=True)
    assert not active.strip(), f'reviewer unit active: {active.strip()}'


def launch(case, mode, standing_path, cl2):
    assert os.nice(0) == 19
    config, receipt = plan(case, mode, standing_path, cl2)
    case.mkdir(parents=True, exist_ok=True)
    no_reviewer_unit()
    assert not (case / 'launch.json').exists() and not (case / 'run.log').exists(), f'one {mode} solver launch'
    subprocess.run([sys.executable, str(NIGHT / 'N4/section6_3_after_N11/end_B_preparation/primary_control_archive/end_b_actual_preflight.py'),
                    str(case / 'host_preflight.json')], cwd=REPO, check=True)
    assert shutil.disk_usage(case).free >= 2 * 1024**3
    for name, entry in receipt['original_inputs'].items():
        destination = case / name
        if not destination.is_file():
            destination.symlink_to(entry['path'])
        checked(destination, entry['sha256'])
    cfg = case / 'run.cfg'
    if cfg.is_file():
        assert cfg.read_text() == config
    else:
        cfg.write_text(config)
    checked(cfg, receipt['config_sha256'])
    (case / 'mpi_tmp').mkdir(exist_ok=True)
    (case / 'rank_receipts').mkdir(exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith('SU2_NATIVE_')}
    env.update(receipt['environment'])
    no_reviewer_unit()
    receipt['started_utc'] = utc()
    save(case / 'launch.json', receipt)
    result = invocation(case, receipt['argv'], env, WATCHDOG)
    stable = {str(cfg): sha(cfg) == receipt['config_sha256'],
              receipt['binary']: sha(receipt['binary']) == receipt['binary_sha256'],
              **{str(case / name): sha(case / name) == entry['sha256']
                 for name, entry in receipt['original_inputs'].items()}}
    result['inputs_verified_after'] = stable
    save(case / 'terminal.json', result)
    print(json.dumps({key: result[key] for key in ('cause', 'returncode', 'elapsed_seconds',
                                                   'all_owned_handles_terminal')}), flush=True)


def prepare(case, mode, standing_path, cl2):
    config, receipt = plan(case, mode, standing_path, cl2)
    case.mkdir(parents=True, exist_ok=True)
    cfg = case / 'run.cfg'
    if cfg.is_file():
        assert cfg.read_text() == config
    else:
        cfg.write_text(config)
    reports = case.parent / 'reports'
    reports.mkdir(exist_ok=True)
    (reports / 'config.diff').write_text(receipt['config_diff'])
    save(reports / 'input_authentication.json', receipt)


def self_check(mode):
    assert os.nice(0) == 19
    template, _ = cl1_template()
    cl2 = derive(template.read_text(), 'CL2')
    mmg = derive(cl2, 'MMG2')
    assert 'ADAP_NATIVE_' not in mmg and 'ADAP_NORM=' not in mmg and 'ADAP_HGRAD=' not in cl2
    assert prepare_diff(template.read_text(), cl2) == {'ADAP_HGRAD', 'ADAP_HMIN'}
    assert all(name.startswith('ADAP_NATIVE_') or name in ('ADAP_REMESHER', 'ADAP_HAUSD')
               for name in prepare_diff(cl2, mmg))
    native = NATIVE if mode == 'CL2' else {}
    with tempfile.TemporaryDirectory(prefix='su2-capability-preflight-') as directory:
        case = Path(directory)
        short_mpi_tmp = case / 'mpi_tmp'
        short_mpi_tmp.mkdir()
        (case / 'rank_receipts').mkdir()
        expected = {**native, **dict.fromkeys(THREADS, '1'), 'OMPI_MCA_orte_tmpdir_base': str(short_mpi_tmp)}
        save(case / 'launch.json', {'mode': mode, 'ranks': RANKS, 'environment': expected})
        environment = {k: v for k, v in os.environ.items() if k not in ranked_env(os.environ)}
        environment.update(expected)
        switches = [argument for name in native for argument in ('-x', name)]
        recipe = ['mpirun', '--bind-to', 'none', '-n', str(RANKS), *switches, '--output-filename', str(case / 'rank_output'),
                  sys.executable, '-B', str(Path(__file__).resolve()), 'rank', str(case), '/usr/bin/sleep', '0.25']
        result = invocation(case, recipe, environment, 60)
        assert result['returncode'] == 0 and result['all_owned_handles_terminal']
        assert [row['rank'] for row in result['rank_waits']] == list(range(RANKS))
        assert all(row['peak_rss_bytes'] > 0 and row['returncode'] == 0 and row['child'] for row in result['rank_waits'])
        print('Six-rank launch, settings and wait4 self-check passed without a solver launch.')


def prepare_diff(before, after):
    def options(text):
        return dict(OPTION.fullmatch(line).groups() for line in text.splitlines() if OPTION.fullmatch(line))
    a, b = options(before), options(after)
    return {key for key in a.keys() | b.keys() if a.get(key) != b.get(key)}


def main(mode):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('prepare', 'self-check', 'launch', 'rank'))
    parser.add_argument('rank_args', nargs='*')
    parser.add_argument('--standing', type=Path)
    parser.add_argument('--repeat', action='store_true', help='One authorized R5 wiring correction')
    parser.add_argument('--cl2-repeat', action='store_true', help='Use the sealed CL2 correction as MMG2 input')
    cli = parser.parse_args()
    if cli.action == 'rank':
        case, path, *args = cli.rank_args
        raise SystemExit(rank(Path(case), Path(path), args))
    if cli.action == 'self-check':
        self_check(mode)
        return
    if cli.standing is None:
        parser.error('--standing is required for prepare and launch')
    if cli.cl2_repeat and mode != 'MMG2':
        parser.error('--cl2-repeat is only used for the MMG2 prerequisite')
    case = NIGHT / mode / ('repeat/run' if cli.repeat else 'run')
    cl2 = NIGHT / 'CL2' / ('repeat/run' if cli.cl2_repeat else 'run')
    {'prepare': prepare, 'launch': launch}[cli.action](case, mode, cli.standing.resolve(), cl2)


if __name__ == '__main__':
    main('CL2')
