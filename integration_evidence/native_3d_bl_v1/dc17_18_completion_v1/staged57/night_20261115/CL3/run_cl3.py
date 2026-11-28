"""Launch CL3 using the sealed CL2 controller, watchdog and rank receipts."""

from argparse import ArgumentParser
import difflib
import hashlib
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
REPO = HERE.parents[5]
CL2 = NIGHT / 'CL2'
TOL1 = NIGHT / 'TOL1'
CL2_COMMIT = '149167bba3'
ORDER_SHA = '3639f6013fe690eee3ec9f38a4bda45562e4f8baca491b92506e887f6cbe162d'
SCALAR = 'ADAP_HAUSD= 1E-4\n'
LOOSE = 'ADAP_HAUSD= (WING3D, 1E-4, SYMMETRY, 1E-4, FARFIELD, 1.0)\n'
LABEL = 'TOL1: native tolerance per marker; standing and equal-list checks passed'


def committed(path, rev=CL2_COMMIT):
    output = subprocess.check_output(['git', 'show', f'{rev}:{path.relative_to(REPO)}'], cwd=REPO)
    assert output == path.read_bytes(), path


for name in ('run_cl2.py', 'finish_cl2.py', 'validate.py', 'runtime_checks.py', 'report_cl2.py'):
    committed(CL2 / name)
sys.path.insert(0, str(CL2))
import run_cl2 as inherited


def derive(template_text):
    assert template_text.splitlines(keepends=True).count(SCALAR) == 1
    config = template_text.replace(SCALAR, LOOSE)
    assert inherited.prepare_diff(template_text, config) == {'ADAP_HAUSD'}
    return config


def reference():
    directory = CL2 / 'run'
    paths = {'config': directory / 'run.cfg', 'launch': directory / 'launch.json',
             'terminal': directory / 'terminal.json', 'validation': directory / 'validation.json',
             'report': CL2 / 'reports/CL2_681.json'}
    for path in paths.values():
        committed(path)
    launch, terminal = (inherited.load(paths[name]) for name in ('launch', 'terminal'))
    assert launch['mode'] == 'CL2' and terminal['all_owned_handles_terminal'] and not terminal['live_handles']
    inherited.checked(paths['config'], launch['config_sha256'])
    for name in inherited.INPUT_FILES:
        source = launch['original_inputs'][name]
        linked = directory / name
        assert linked.resolve(strict=True) == Path(source['path']).resolve(strict=True)
        assert terminal['inputs_verified_after'][str(linked)]
        inherited.checked(linked, source['sha256'])
    return paths, launch


def standing_build(path):
    assert path.parent.parent == TOL1 and path.name == 'section3.json', path
    committed(path, 'HEAD')
    seal = inherited.load(path)
    assert seal['passed'] and seal['status'] == 'passed' and seal['label'] == LABEL
    status_pin = seal['build_status']
    status = inherited.checked(status_pin['path'], status_pin['sha256'])
    assert status.parent == path.parent
    build = inherited.load(status)
    assert build['passed'] and build['source_commit'] == seal['source_commit']
    assert build['binary_sha256'] == seal['binary_sha256']
    inherited.checked(build['binary'], build['binary_sha256'])
    for path_name, digest in seal['pins'].items():
        inherited.checked(path_name, digest)
    assert not inherited.git('diff', seal['source_commit'], '--', 'Common', 'SU2_CFD',
                             'UnitTests', 'TestCases', 'config_template.cfg')
    return seal, build


def first_attempt(case, binary_digest):
    if case.parent.name != 'repeat':
        return None
    original = HERE / 'run'
    launch = inherited.load(original / 'launch.json')
    assert launch['mode'] == 'CL3' and launch['original_attempt'] is None
    original_paths = {'launch': original / 'launch.json', 'terminal': original / 'terminal.json',
                      'validation': original / 'validation.json', 'config': original / 'run.cfg',
                      'report': HERE / 'reports' / f'CL3_{launch["build"]}.json'}
    for path in original_paths.values():
        committed(path, 'HEAD')
    terminal = inherited.load(original_paths['terminal'])
    report = inherited.load(original_paths['report'])
    assert terminal['all_owned_handles_terminal'] and not terminal['live_handles']
    assert report['tolerance_wiring']['wiring_defect'], 'only a wiring defect authorizes a repeat'
    assert launch['binary_sha256'] != binary_digest
    return {name: inherited.pin(path) for name, path in original_paths.items()}


def plan(case, mode, standing_path, _):
    assert mode == 'CL3'
    inherited.checked(TOL1 / 'inputs/TOL1_MARKER_TOLERANCE.md', ORDER_SHA)
    seal, build = standing_build(standing_path)
    paths, old = reference()
    template = paths['config'].read_text()
    config = derive(template)
    env = dict(old['environment'])
    env['OMPI_MCA_orte_tmpdir_base'] = str(case / 'mpi_tmp')
    assert all(env[key] == '1' for key in inherited.THREADS)
    assert {key: val for key, val in env.items() if key.startswith('SU2_NATIVE_')} == inherited.NATIVE
    argv = ['mpirun', '--bind-to', 'none', '-n', str(inherited.RANKS), '--timestamp-output',
            '--output-filename', str(case / 'rank_output')]
    for key in inherited.NATIVE:
        argv.extend(('-x', key))
    argv.extend([sys.executable, '-B', str(Path(__file__).resolve()), 'rank', str(case), build['binary'], 'run.cfg'])
    digest = hashlib.sha256(config.encode()).hexdigest()
    original = first_attempt(case, build['binary_sha256'])
    if original:
        assert original['config']['sha256'] == digest
    return config, {
        'mode': mode, 'build': build['build'], 'checkout_commit': inherited.git('rev-parse', 'HEAD').decode().strip(),
        'binary_source_commit': seal['source_commit'], 'binary': build['binary'], 'binary_sha256': build['binary_sha256'],
        'config_diff': ''.join(difflib.unified_diff(template.splitlines(keepends=True), config.splitlines(keepends=True),
                                                   fromfile=str(paths['config']), tofile='CL3/run/run.cfg')),
        'config_sha256': digest, 'argv': argv, 'environment': env, 'ranks': inherited.RANKS,
        'nice': 19, 'watchdog_seconds': inherited.WATCHDOG, 'original_inputs': old['original_inputs'],
        'standing': inherited.pin(standing_path), 'build_status': seal['build_status'],
        'template': inherited.pin(paths['config']), 'cl2_prerequisite': {key: inherited.pin(path) for key, path in paths.items()},
        'controller': inherited.pin(Path(__file__).resolve()), 'original_attempt': original,
    }


def rank(case, binary, args):
    # CL2 has the same native rank environment; its rank code distinguishes it from MMG2 by mode.
    read = inherited.load

    def native_receipt(path):
        row = read(path)
        if path == case / 'launch.json':
            assert row['mode'] == 'CL3'
            row['mode'] = 'CL2'
        return row

    with patch.object(inherited, 'load', native_receipt):
        return inherited.rank(case, binary, args)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('prepare', 'self-check', 'launch', 'rank'))
    parser.add_argument('rank_args', nargs='*')
    parser.add_argument('--standing', type=lambda value: Path(value).resolve())
    parser.add_argument('--repeat', action='store_true')
    cli = parser.parse_args()
    if cli.action == 'rank':
        case, binary, *args = cli.rank_args
        raise SystemExit(rank(Path(case), Path(binary), args))
    if cli.action == 'self-check':
        paths, manifest = reference()
        assert manifest['watchdog_seconds'] == inherited.WATCHDOG == 21600
        assert manifest['ranks'] == inherited.RANKS == 6
        assert derive(paths['config'].read_text()).replace(LOOSE, SCALAR) == paths['config'].read_text()
        print('Sealed CL2 inputs and one-line CL3 configuration change verified.')
    else:
        if cli.standing is None:
            parser.error('--standing is required for prepare and launch')
        case = HERE / ('repeat/run' if cli.repeat else 'run')
        with patch.object(inherited, 'plan', plan):
            getattr(inherited, cli.action)(case, 'CL3', cli.standing, None)
