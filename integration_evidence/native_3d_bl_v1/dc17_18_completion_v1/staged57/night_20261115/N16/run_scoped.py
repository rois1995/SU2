"""Bind step 8.5r to the frozen build and qualification adapters."""
from pathlib import Path
import re
import shutil
import subprocess
import sys

N16 = Path(__file__).resolve().parent
sys.path.insert(0, str(N16.parent / 'N4/section6_3_after_N11/phase_D'))
from run_identity import cut, seal

TEST_SOURCE = 'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp'


def prepare_build(previous, build, changes):
    prior = cut.PHASE / f'build{previous}'
    target = cut.PHASE / f'build{build}'
    pins = cut.read(prior / 'source_pins.json')
    for name in changes:
        path = cut.REPO / name
        assert str(path) in pins, path
        pins[str(path)] = cut.sha(path)
    changed = [path for path, digest in pins.items() if cut.sha(path) != digest]
    assert not changed, changed
    target.mkdir()
    cut.save(target / 'source_pins.json', pins)
    script = (prior / 'build.py').read_text().replace(str(previous), str(build))
    (target / 'build.py').write_text(script)
    return target


def check_build(target):
    status = cut.read(target / 'build_status.json')
    handles = cut.read(target / 'terminal_handles.json')
    assert status['pins_verified'] and status['exit_code'] == 0
    assert handles['all_owned_handles_terminal'] and all(cut.terminal(row) for row in handles['identities'])
    log = target / 'build.log'
    warnings = [line for line in log.read_text(errors='replace').splitlines() if re.search(r'\bwarning:', line, re.I)]
    assert not warnings, warnings
    return status


def test_command(binary, selector):
    command = ['env', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1', 'OMPI_MCA_osc=pt2pt',
               'TMPDIR=' + str(cut.REPO / 'integration_evidence/build-integrated-v2/compile-tmp')]
    return [*command, 'nice', '-n', '19', 'timeout', '180', str(binary), selector]


def part_a():
    workspace = N16 / 'A'
    workspace.mkdir()
    build = 573
    changes = ['Common/src/adaptation/CNativeDistributed3D.cpp', TEST_SOURCE]
    target = prepare_build(572, build, changes)
    cut.run_gate(target, 'build', [cut.PYTHON, str(target / 'build.py')])
    status = check_build(target)
    wait = cut.run_gate(workspace, 'A4', test_command(status['test_binary'], '[NativeQualityRoundBounds3D]'))
    assert 'All tests passed' in (workspace / 'A4_outer.log').read_text()
    cut.save(workspace / 'acceptance_summary.json', {'passed': True, 'build': build, 'changes': changes,
             'checked_peer_pairs': 2, 'fixture_seconds': wait['elapsed_seconds'], 'binary_sha256': status['test_binary_sha256']})


def bind_identity(name, build):
    source = cut.PHASE / 'D2/validation'
    workspace = cut.PHASE / name / 'validation'
    workspace.mkdir(parents=True)
    names = ('run_g1_572.py', 'run_g2_gq_prefix_572.py', 'validate_g2_gq.py',
             'run_g3_572.py', 'validate_end_b.py', 'launch_g3.py', 'run_g4_572.py')
    for filename in names:
        text = (source / filename).read_text().replace('D2/validation', f'{name}/validation')
        text = text.replace('572', str(build)).replace('D2_G3', f'{name}_G3')
        target = workspace / filename.replace('572', str(build))
        compile(text, str(target), 'exec')
        target.write_text(text)
    (workspace / 'run_g2_gq_prefix.py').symlink_to(f'run_g2_gq_prefix_{build}.py')
    aliases = (cut.PHASE / 'build572_aliases.py').read_text().replace('572', str(build))
    (cut.PHASE / f'build{build}_aliases.py').write_text(aliases.replace('D2/validation', f'{name}/validation'))
    for prefix in ('run_g1_', 'run_g2_gq_prefix_', 'run_g3_'):
        subprocess.run([cut.PYTHON, str(workspace / f'{prefix}{build}.py'), '--self-check'], check=True)
    return workspace


def part_b():
    assert cut.read(N16 / 'A/acceptance_summary.json')['passed']
    for handle in (cut.read(N16 / 'A_campaign_launch.json'), cut.read(N16 / 'A/A4_retry_wait.json')['controller']):
        assert cut.terminal(handle)
    draft = N16 / 'B_draft'
    pins = cut.read(draft / 'manifest.json')
    assert all(cut.sha(cut.REPO / name) == row['base'] and cut.sha(draft / name) == row['draft']
               for name, row in pins.items())
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(N16 / 'B_before_edit_preflight.json')], check=True)
    for name in pins:
        shutil.copyfile(draft / name, cut.REPO / name)
    subprocess.run(['git', 'diff', '--check', '--', *pins], check=True)
    target = prepare_build(573, 574, pins)
    workspace = bind_identity('N16B', 574)
    cut.qualify(11, 574, workspace, target)
    check_build(target)
    seal('N16B', 11, 574, workspace, target)
    print('N16 B scoped identity passed on build574.', flush=True)


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    {'A': part_a, 'B': part_b}[sys.argv[1]]()
