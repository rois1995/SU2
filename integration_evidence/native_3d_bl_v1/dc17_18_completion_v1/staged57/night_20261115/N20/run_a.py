"""Build and qualify the default-width N20 fine-placement change on the actual host."""

from pathlib import Path
import shlex
import subprocess
import sys

N20 = Path(__file__).resolve().parent
sys.path.insert(0, str(N20.parent / 'N18'))
from run_de import bind_build, bind_identity, check_build, cut, git, seal, verify_g4
from run_scoped import test_command

BUILD = 584
NAME = 'N20A'
TARGET = cut.PHASE / f'build{BUILD}'
WORK = cut.PHASE / NAME / 'validation'
FIXTURES = N20 / 'fixtures'
SOURCES = (
    'Common/include/adaptation/CNativeOptimize3D.hpp',
    'Common/src/adaptation/CNativeEngine3D.cpp',
    'Common/src/adaptation/native_engine3d/EngineControl.cpp',
    'Common/src/adaptation/native_engine3d/EngineReports.cpp',
    'Common/src/adaptation/native_engine3d/EngineSelection.hpp',
    'Common/src/adaptation/native_engine3d/EngineSelectionQueues.cpp',
    'Common/src/adaptation/native_engine3d/EngineTrial.cpp',
    'Common/src/adaptation/native_engine3d/MeshOptimizer.cpp',
    'UnitTests/Common/adaptation/CNativeMetric3D_tests.cpp',
)


def progress(message):
    with (cut.NIGHT / 'progress.md').open('a') as output:
        output.write(f'\nN20 A. {message}\n')


def build():
    assert git('rev-parse', '--short=10', 'HEAD') == 'da20c9e9fe'
    assert git('diff', '--name-only', '--', 'Common', 'SU2_CFD', 'UnitTests').splitlines() == list(SOURCES)
    subprocess.run(['sha256sum', '-c', 'SHA256SUMS'], cwd=N20 / 'inputs', check=True)
    bind_build(583, BUILD, SOURCES, cut.PHASE / 'build583/source_pins.json')
    bind_identity(NAME, BUILD)
    wait = cut.run_gate(TARGET, 'build', [cut.PYTHON, str(TARGET / 'build.py')])
    status = check_build(TARGET)
    handles = cut.read(TARGET / 'terminal_handles.json')['identities'] + [wait['controller']]
    cut.save(TARGET / 'primary_terminal_receipt.json', {'actual_host_proc': True, 'passed': True,
                                                       'all_owned_handles_terminal': True, 'identities': handles,
                                                       'controller_wait': wait})
    cut.save(TARGET / 'cut_build_plan.json', {'build_label': f'build{BUILD}',
                                             'source_pins_sha256': cut.sha(TARGET / 'source_pins.json')})
    progress(f'Build{BUILD} compiled without warnings in {wait["elapsed_seconds"]:.3f} s. Running F1–F4.')
    return status


def compile_selection():
    """Link the real private Selection fixture with the existing Meson libraries."""
    root = cut.REPO / 'integration_evidence/build-integrated-v3-p5-20261114'
    entry, = [row for row in cut.read(root / 'compile_commands.json')
              if row['file'].endswith('/CNativeEngine3D.cpp')]
    directory = Path(entry['directory'])
    argv = shlex.split(entry['command'])
    flags = argv[1:argv.index('-MD')]
    flags[flags.index('-O2')] = '-O1'
    ninja = (root / 'build.ninja').read_text().splitlines()
    target = next(i for i, line in enumerate(ninja) if line.startswith('build UnitTests/test_driver: '))
    args = shlex.split(ninja[target + 1].split(' = ', 1)[1])
    assert ninja[target + 1].startswith(' LINK_ARGS = ')
    source = FIXTURES / 'fine_selection_fixture.cpp'
    binary = FIXTURES / 'fine_selection_fixture'
    command = ['nice', '-n', '19', argv[0], *flags, '-I' + str(cut.REPO), str(source),
               '-o', str(binary), *args]
    cut.save(FIXTURES / 'selection_compiler.json',
             {'argv': command, 'directory': str(directory), 'source_sha256': cut.sha(source)})
    environment = cut.read(TARGET / 'build_plan.json')['environment']
    subprocess.run(command, cwd=directory, env=environment, check=True)


def fixtures(status):
    cut.run_gate(FIXTURES, 'optimizer', test_command(status['test_binary'], '[NativeFinePlacement3D]'))
    assert 'All tests passed' in (FIXTURES / 'optimizer_outer.log').read_text()
    cut.run_gate(FIXTURES, 'selection_build', [cut.PYTHON, str(Path(__file__).resolve()), 'selection-build'])
    cut.run_gate(FIXTURES, 'selection', ['nice', '-n', '19', str(FIXTURES / 'fine_selection_fixture')])
    assert 'good_reopened=0 reopened=4 done=4 repeated=0 reset=4' in (FIXTURES / 'selection_outer.log').read_text()
    progress('F1–F4 passed on the pinned production sources. Starting G1.')


def qualify():
    for cohort, counts in (('serial', 375), ('mpi2', 114), ('mpi4', 228)):
        cut.run_gate(WORK, f'G1_{cohort}',
                     [cut.PYTHON, str(WORK / f'run_g1_{BUILD}.py'), '--cohort', cohort])
        receipt = cut.read(WORK / f'G1_candidate_{cohort}_summary.json')
        assert receipt['identity_pass'] and receipt['changed_case_count'] == 0
        assert receipt['expected_cases_per_rank'] * receipt['ranks'] == counts
    progress('G1 matched all 375 / 114 / 228 records. Starting G2.')
    cut.run_gate(WORK, 'G2', [cut.PYTHON, str(WORK / f'run_g2_gq_prefix_{BUILD}.py'), '--gate', 'G2',
                             '--case-name', f'C11_G2_{BUILD}'])
    progress('G2 matched exact first-collapse work, rank counts and tracked rows. Starting G3.')
    qualify_after_g2()


def qualify_after_g2():
    cut.run_gate(WORK, 'G3', [cut.PYTHON, str(WORK / 'launch_g3.py')])
    cut.run_gate(WORK, 'G4', [cut.PYTHON, str(WORK / f'run_g4_{BUILD}.py'), '--run'])
    verify_g4(WORK, cut.PHASE / 'N18DE/validation')
    progress('G3 and G4 passed. Starting GQ.')
    cut.run_gate(WORK, 'GQ', [cut.PYTHON, str(WORK / f'run_g2_gq_prefix_{BUILD}.py'), '--gate', 'GQ',
                             '--case-name', f'C11_GQ_{BUILD}'])
    seal(NAME, 11, BUILD, WORK, TARGET)
    progress('GQ matched through the third quality window. Running the remaining standing safeguards.')
    subprocess.run([cut.PYTHON, str(N20 / 'run_w.py'), 'safeguards', '--build', str(BUILD)], check=True)


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    if sys.argv[1:] == ['selection-build']:
        compile_selection()
    elif sys.argv[1:] == ['continue-g3']:
        assert cut.terminal(cut.read(N20 / 'A_controller_launch.json'))
        check_build(TARGET)
        for cohort in ('serial', 'mpi2', 'mpi4'):
            receipt = cut.read(WORK / f'G1_candidate_{cohort}_summary.json')
            assert receipt['identity_pass'] and receipt['changed_case_count'] == 0
        g2 = WORK / f'cases/C11_G2_{BUILD}/g2_build{BUILD}_comparison.json'
        assert cut.read(g2)['passed']
        assert all(cut.sha(path) == digest for path, digest in cut.read(TARGET / 'source_pins.json').items())
        qualify_after_g2()
        print('N20 A passed the remaining identity gates and safeguards.', flush=True)
    else:
        previous = cut.read(N20.parent / 'N19/terminal_audit.json')
        assert all(cut.terminal(row) for row in previous['identities'])
        fixtures(build())
        qualify()
        print('N20 A passed all fixtures, identity gates and safeguards.', flush=True)


if __name__ == '__main__':
    main()
