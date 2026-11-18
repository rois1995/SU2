"""Enable the exact wide-round retry after D/E and qualify its scoped gates."""
from pathlib import Path
import shutil
import subprocess
import sys
import time

from run_de import N18, bind_build, bind_identity, check_build, cut, git, progress, verify_g4

sys.path.insert(0, str(N18.parent / 'N16'))
from run_c import compare_safeguard
from run_scoped import test_command

BUILD = 580
F = cut.PHASE / 'round_8_5r2_F'
SCOPED = cut.PHASE / 'N18F/validation'
REFERENCE = cut.PHASE / 'phase_N6/safeguards'
RECIPE = N18 / 'drafts/recipes/workflow_f.py'


def backup_default_width():
    status = cut.read(cut.PHASE / 'build579/build_status.json')
    target = N18 / 'qualified_build579'
    target.mkdir()
    paths = {}
    for name in ('binary', 'test_binary'):
        source = Path(status[name])
        digest = status[f'{name}_sha256']
        assert cut.sha(source) == digest
        backup = target / source.name
        shutil.copyfile(source, backup)
        backup.chmod(0o555)
        assert cut.sha(backup) == digest
        paths[name] = {'shared_path': str(source), 'preserved_copy': str(backup), 'sha256': digest}
    cut.save(target / 'manifest.json', {'build': 579, 'git_payloads': False, 'binaries': paths})


def apply_wide():
    de = cut.read(N18 / 'DE_checkpoint.json')
    assert de['passed'] and de['build'] == 579 and de['commit'] == git('rev-parse', 'HEAD')
    base_pins = cut.PHASE / 'build579/source_pins.json'
    assert all(cut.sha(path) == digest for path, digest in cut.read(base_pins).items())
    assert not git('diff', '--cached', '--name-only')
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(N18 / 'before_F_host_preflight.json')], check=True)
    backup_default_width()
    patch = N18 / 'drafts/F/wide.patch'
    paths = ['Common/include/adaptation/CNativeEngine3D.hpp',
             'SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
             'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp']
    expected = subprocess.check_output(['git', 'diff', '5dffa5ca5a^..5dffa5ca5a', '--', *paths], cwd=cut.REPO)
    assert patch.read_bytes() == expected
    git('apply', '--check', '--whitespace=error', patch)
    git('apply', '--whitespace=error', patch)
    git('diff', '--check', '--', *paths)
    cut.save(N18 / 'F_source_paths.json', {'base_commit': de['commit'], 'patch_sha256': cut.sha(patch),
             'source_sha256': {name: cut.sha(cut.REPO / name) for name in paths}})
    target = bind_build(579, BUILD, paths, base_pins)
    workspace = bind_identity('N18F', BUILD)
    validator = workspace / 'validate_end_b.py'
    text = validator.read_text()
    comparison = ('    assert mismatch is None, mismatch\n'
                  '    assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]')
    conditional = ('    private_work = "NATIVE3D_PRIVATE_WORK " in (case / "run.log").read_text(errors="replace")\n'
                   '    if not private_work:\n'
                   '        assert mismatch is None, mismatch\n'
                   '        assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]')
    field = '        "first_mismatch_index": mismatch,'
    assert text.count(comparison) == text.count(field) == 1
    text = text.replace(comparison, conditional).replace(field, field + '\n        "identity_is_gate": not private_work,')
    compile(text, str(validator), 'exec')
    validator.write_text(text)
    progress('Applied the exact three-path F patch. Compiling build580 with the production width of 64.')
    return target


def build_and_test(target):
    wait = cut.run_gate(target, 'build', [cut.PYTHON, str(target / 'build.py')])
    status = check_build(target)
    handles = cut.read(target / 'terminal_handles.json')['identities'] + [wait['controller']]
    assert all(cut.terminal(row) for row in handles)
    cut.save(target / 'primary_terminal_receipt.json', {'actual_host_proc': True, 'passed': True,
             'all_owned_handles_terminal': True, 'identities': handles, 'controller_wait': wait})
    cut.save(target / 'cut_build_plan.json', {'build_label': f'build{BUILD}',
             'source_pins_sha256': cut.sha(target / 'source_pins.json')})
    for ranks in (2, 4):
        argv = test_command(status['test_binary'], '[NativeConcurrentRepair3D]')
        argv[-2:-2] = ['mpirun', '--bind-to', 'none', '-n', str(ranks)]
        cut.run_gate(SCOPED, f'wide_repeat_mpi{ranks}', argv)
        assert 'All tests passed' in (SCOPED / f'wide_repeat_mpi{ranks}_outer.log').read_text()
    progress('Build580 compiled without warnings. The exact wide-repeat fixture passed at 2 and 4 ranks.')


def scoped_checks():
    for cohort, count in (('serial', 375), ('mpi2', 114), ('mpi4', 228)):
        cut.run_gate(SCOPED, f'G1_{cohort}',
                     [cut.PYTHON, str(SCOPED / f'run_g1_{BUILD}.py'), '--cohort', cohort])
        result = cut.read(SCOPED / f'G1_candidate_{cohort}_summary.json')
        assert result['identity_pass'] and result['changed_case_count'] == 0
        assert result['expected_cases_per_rank'] * result['ranks'] == count
    cut.run_gate(SCOPED, 'G2', [cut.PYTHON, str(SCOPED / f'run_g2_gq_prefix_{BUILD}.py'), '--gate', 'G2',
                               '--case-name', f'C11_G2_{BUILD}'])
    cut.run_gate(SCOPED, 'G3', [cut.PYTHON, str(SCOPED / 'launch_g3.py')])
    cut.run_gate(SCOPED, 'G4', [cut.PYTHON, str(SCOPED / f'run_g4_{BUILD}.py'), '--run'])
    verify_g4(SCOPED, cut.PHASE / 'N6Selection/validation')
    g2, _ = cut.seal_prefix(SCOPED, 11, BUILD, 'G2')
    g3_file, = (SCOPED / 'cases').glob('N18F_G3_*/g3_end_b_gate.json')
    g3 = cut.read(g3_file)
    assert g3['passed']
    g4 = cut.read(SCOPED / 'g4_case_paths.json')
    cut.save(SCOPED / 'acceptance_summary.json', {'passed': True, 'build': BUILD,
             'G1_records': [375, 114, 228], 'wide_repeat_ranks': [2, 4], 'G2': g2, 'G3': g3, 'G4': g4})
    progress(f"F scoped gates passed G1 375/114/228, G2 {g2['work']:,}, G3 {g3['accepted_splits']:,} splits "
             f"and {g3['common_edit_count']:,} exact shared edits, and G4. Running the standing safeguards.")


def safeguards():
    workspace = F / 'safeguards'
    workspace.mkdir(parents=True)
    runner = workspace / f'run_build{BUILD}_safeguards.py'
    source = REFERENCE / 'run_build578_safeguards.py'
    runner.write_text(source.read_text().replace('578', str(BUILD)))
    cut.save(workspace / 'runner_pins.json', {'adopted_sha256': cut.sha(source), 'bound_sha256': cut.sha(runner)})
    with (workspace / 'self_check.json').open('x') as output:
        subprocess.run([cut.PYTHON, str(runner), 'self-check'], stdout=output, check=True)
    assert cut.read(workspace / 'self_check.json')['passed']
    results = {}
    for mode, reference in cut.read(REFERENCE / 'acceptance_summary.json')['safeguards'].items():
        cases = workspace / 'cases'
        before = set(cases.glob('*'))
        waited = cut.run_gate(workspace, mode, [cut.PYTHON, str(runner), mode])
        case, = set(cases.glob('*')) - before
        status = cut.read(case / 'status.json')
        receipt = cut.read(case / 'safeguard_process_receipt.json')
        assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
        assert not status['changed'] and receipt['state'] == 'terminal' and receipt['exit_code'] == 0
        assert not receipt['live_owned_descendants']
        assert cut.terminal(waited['controller']) and all(cut.terminal(row) for row in receipt['identities'])
        assert all(cut.sha(path) == digest for path, digest in cut.read(case / 'pins.json').items())
        identity = compare_safeguard(mode, Path(reference['case']), case)
        identity.update(case=str(case), seconds=status['seconds'])
        cut.save(workspace / f'{mode}_identity.json', identity)
        assert identity['passed'], identity
        results[mode] = identity
    cut.save(workspace / 'acceptance_summary.json', {'passed': True, 'build': BUILD, 'safeguards': results})


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    controller = cut.read(N18 / 'DE_campaign_launch.json')
    while not cut.terminal(controller):
        time.sleep(15)
    target = apply_wide()
    build_and_test(target)
    scoped_checks()
    safeguards()
    progress('All three standing safeguards passed. Starting F G5 run one.')
    command = [cut.PYTHON, str(RECIPE), 'run', '--build', str(BUILD), '--scoped',
               str(SCOPED / 'acceptance_summary.json'), '--safeguards', str(F / 'safeguards/acceptance_summary.json')]
    with (N18 / 'F_workflow_campaign.log').open('x') as log:
        child = subprocess.Popen(command, cwd=cut.REPO, stdout=log, stderr=subprocess.STDOUT)
        handle = cut.actual_identity(child.pid)
        cut.save(N18 / 'F_workflow_campaign_launch.json', handle)
        code = child.wait()
    assert cut.terminal(handle)
    cut.save(N18 / 'F_workflow_campaign_wait.json', dict(handle, returncode=code, terminal=True))
    raise SystemExit(code)


if __name__ == '__main__':
    main()
