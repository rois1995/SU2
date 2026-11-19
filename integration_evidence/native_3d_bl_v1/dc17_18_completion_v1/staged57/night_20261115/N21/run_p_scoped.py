"""Build N21 Part P, run its fixtures and seal the scoped identity and safeguard gates."""

from pathlib import Path
import re
import subprocess
import sys

N21 = Path(__file__).resolve().parent
sys.path.insert(0, str(N21.parent / 'N18'))
from run_de import bind_build, bind_identity, check_build, cut, git, seal, verify_g4
from run_scoped import test_command

sys.path.insert(0, str(N21.parent / 'N20'))
from run_w import scoped_helpers

BASE = 'f968325a69a8e03b03d9dd275a075b61236055c2'
PREVIOUS = 586
BUILD = 587
NAME = 'N21P'
TARGET = cut.PHASE / f'build{BUILD}'
WORK = cut.PHASE / NAME / 'validation'
BRANCH = cut.PHASE / 'round_8_5t_P'
FIXTURES = N21 / 'fixtures'
COHORTS = ('serial', 'mpi2', 'mpi4')
SELECTOR = '[NativeOwnedFaceProof3D][N21ProofShare]'
CASE_SUMMARY = re.compile(r'All tests passed \((\d+) assertions? in (\d+) test cases?\)')


def progress(message):
    with (cut.NIGHT / 'progress.md').open('a') as output:
        output.write(f'\nN21 P. {message}\n')


def build():
    old = cut.PHASE / f'build{PREVIOUS}'
    previous = cut.read(old / 'restore_receipt.json')
    assert previous['passed'] and previous['build'] == PREVIOUS
    assert all(cut.sha(row['path']) == row['sha256'] for row in previous['binaries'].values())
    assert cut.terminal(cut.read(N21.parent / 'N20/Restore_controller_launch.json'))
    assert all(cut.terminal(row) for row in cut.read(N21.parent / 'N20/terminal_audit.json')['identities'])
    assert (N21 / 'source_check.md').is_file()
    subprocess.run(['sha256sum', '-c', 'SHA256SUMS'], cwd=N21 / 'inputs', check=True)
    changed = git('diff', '--name-only', BASE, '--', 'Common', 'SU2_CFD', 'UnitTests').splitlines()
    assert changed and all(path.endswith(('.cpp', '.hpp')) for path in changed), changed
    assert not git('diff', '--check', BASE, '--', *changed)
    bind_build(PREVIOUS, BUILD, changed, old / 'source_pins.json')
    bind_identity(NAME, BUILD)
    cut.save(WORK / 'candidate_source.json', {'base_commit': BASE, 'build': BUILD, 'source_paths': changed})
    wait = cut.run_gate(TARGET, 'build', [cut.PYTHON, str(TARGET / 'build.py')])
    status = check_build(TARGET)
    handles = cut.read(TARGET / 'terminal_handles.json')['identities'] + [wait['controller']]
    assert all(cut.terminal(row) for row in handles)
    cut.save(TARGET / 'primary_terminal_receipt.json', {'actual_host_proc': True, 'passed': True,
             'all_owned_handles_terminal': True, 'identities': handles, 'controller_wait': wait})
    cut.save(TARGET / 'cut_build_plan.json', {'build_label': f'build{BUILD}',
             'source_pins_sha256': cut.sha(TARGET / 'source_pins.json')})
    progress(f'Build{BUILD} compiled without warnings in {wait["elapsed_seconds"]:.3f} s. Running T1–T4.')
    return status


def fixture_assertions(output, ranks, expected_cases):
    stripped = re.sub(r'\x1b\[[0-9;]*m', '', output)
    summaries = [(int(assertions), int(cases)) for assertions, cases in CASE_SUMMARY.findall(stripped)]
    assert len(summaries) == ranks and all(count > 0 and cases == expected_cases for count, cases in summaries), summaries
    return [count for count, _ in summaries]


def fixtures(status):
    FIXTURES.mkdir(exist_ok=True)
    passed = {}
    note = FIXTURES / 'T4_r3_note.md'
    for name, ranks, selector, cases in (('T1_T3', 1, SELECTOR + '~[mpi]', 3),
                                         ('T4_mpi2', 2, SELECTOR + '[mpi]', 1),
                                         ('T4_mpi4', 4, SELECTOR + '[mpi]', 1)):
        if ranks > 1 and note.is_file():
            passed[name] = {'state': 'R3_NOTE', 'note': str(note), 'note_sha256': cut.sha(note)}
            continue
        argv = test_command(status['test_binary'], selector)
        if ranks > 1:
            argv[-2:-2] = ['mpirun', '--bind-to', 'none', '-n', str(ranks)]
        wait = cut.run_gate(FIXTURES, name, argv)
        log = FIXTURES / f'{name}_outer.log'
        passed[name] = {'state': 'PASSED', 'ranks': ranks, 'cases_per_rank': cases, 'command': argv,
                        'checks_per_rank': fixture_assertions(log.read_text(), ranks, cases),
                        'seconds': wait['elapsed_seconds'], 'log': str(log), 'sha256': cut.sha(log)}
    cut.save(FIXTURES / 'acceptance_summary.json', {'passed': True, 'build': BUILD, 'fixtures': passed,
                                                  'test_binary_sha256': status['test_binary_sha256']})
    progress('The serial proof fixtures passed. T4 has its recorded MPI results or R3 note. Starting G1.')


def prefix_gate(name):
    cut.run_gate(WORK, name, [cut.PYTHON, str(WORK / f'run_g2_gq_prefix_{BUILD}.py'), '--gate', name,
                             '--case-name', f'C11_{name}_{BUILD}'])


def qualify():
    for cohort, counts in zip(COHORTS, (375, 114, 228)):
        cut.run_gate(WORK, f'G1_{cohort}',
                     [cut.PYTHON, str(WORK / f'run_g1_{BUILD}.py'), '--cohort', cohort])
        result = cut.read(WORK / f'G1_candidate_{cohort}_summary.json')
        assert result['identity_pass'] and result['changed_case_count'] == 0
        assert result['expected_cases_per_rank'] * result['ranks'] == counts
    progress('G1 matched all 375 / 114 / 228 records. Starting G2.')
    prefix_gate('G2')
    progress('G2 matched exact first-collapse work, rank counts and tracked rows. Starting G3.')
    cut.run_gate(WORK, 'G3', [cut.PYTHON, str(WORK / 'launch_g3.py')])
    cut.run_gate(WORK, 'G4', [cut.PYTHON, str(WORK / f'run_g4_{BUILD}.py'), '--run'])
    verify_g4(WORK, cut.PHASE / 'N20A/validation')
    progress('G3 and G4 passed. Starting GQ.')
    prefix_gate('GQ')
    seal(NAME, 11, BUILD, WORK, TARGET)
    summary = cut.read(WORK / 'acceptance_summary.json')
    g2, g3, gq = (summary[name] for name in ('G2', 'G3', 'GQ'))
    progress(f"Build{BUILD} scoped identity passed. G1 375/114/228, G2 {g2['work']:,}, G3 {g3['accepted_splits']:,} splits "
             f"with {g3['common_edit_count']:,} identical shared edits, G4 1/2/4, GQ {gq['seconds_until_stop']:.3f} s. "
             "Running the remaining standing safeguards.")


def safeguards():
    status = check_build(TARGET)
    assert all(cut.sha(status[name]) == status[f'{name}_sha256'] for name in ('binary', 'test_binary'))
    assert all(cut.sha(path) == digest for path, digest in cut.read(TARGET / 'source_pins.json').items())
    scoped_helpers(BUILD, WORK, BRANCH)['safeguards']()
    progress('All three standing geometry safeguards passed. Next is the default-width G5 and its purpose gate.')


def checkpoint_paths():
    safeguards_dir = BRANCH / 'safeguards'
    paths = [Path(__file__).resolve(), *map(Path, cut.read(WORK / 'checkpoint_paths.json')),
             WORK / 'checkpoint_paths.json', WORK / 'run_g2_gq_prefix.py']
    for directory in (FIXTURES, safeguards_dir, *(safeguards_dir / 'cases').iterdir()):
        paths.extend(path for path in directory.iterdir() if path.suffix in ('.cfg', '.json', '.md', '.py'))
        paths.extend(cut.pack(path) for path in directory.iterdir() if path.suffix in ('.log', '.jsonl'))
    return sorted(set(paths))


def self_check():
    assert fixture_assertions('\x1b[1;32mAll tests passed\x1b[0m (37 assertions in 3 test cases)', 1, 3) == [37]
    log = 'All tests passed (1 assertion in 1 test case)\n'
    assert fixture_assertions(log * 4, 4, 1) == [1] * 4
    for ranks, cases in ((3, 1), (4, 2)):
        try:
            fixture_assertions(log * 4, ranks, cases)
        except AssertionError:
            continue
        raise AssertionError('Unexpected fixture rank or case count accepted')
    assert SELECTOR + '[mpi]' == '[NativeOwnedFaceProof3D][N21ProofShare][mpi]'
    assert scoped_helpers(BUILD, WORK, BRANCH)['BUILD'] == BUILD
    print('N21 Part P scoped fixture parser and safeguard binding self-check passed.')


if __name__ == '__main__':
    if sys.argv[1:] == ['self-check']:
        self_check()
    else:
        assert sys.argv[1:] == ['run'] and Path('/proc/1/comm').read_text().strip() == 'systemd'
        fixtures(build())
        qualify()
        safeguards()
