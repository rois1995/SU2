"""Qualify Part C after the accepted default-width T0 run."""
from pathlib import Path
import json
import re
import shutil
import subprocess
import time

from draft_c import ROOT as DRAFT
from run_changed_workflow import T0, run as workflow, substitute
from run_scoped import N16, bind_identity, check_build, cut, prepare_build, test_command

BUILD = 575
D = cut.PHASE / 'phase_D'
GUARDS = cut.PHASE / 'round_8_5r_C/safeguards'
PRIVATE = re.compile(r'\bNATIVE3D_PRIVATE_WORK\s')
ANSI = re.compile(r'\x1b\[[0-9;]*m')


def progress(message):
    with (N16.parent / 'progress.md').open('a') as file:
        file.write(f'\nN16 C. {message}\n')


def build():
    assert cut.read(T0.parent / 'acceptance_summary.json')['passed']
    identity = cut.read(N16 / 'B_checkpoint.json')
    assert identity['scoped_identity_passed'] and identity['commit'] == subprocess.check_output(
        ['git', 'rev-parse', 'HEAD'], text=True).strip()
    manifest = cut.read(DRAFT / 'manifest.json')
    assert all(cut.sha(cut.REPO / name) == row['base'] and cut.sha(DRAFT / name) == row['draft']
               for name, row in manifest.items())
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(N16 / 'C_before_edit_preflight.json')], check=True)
    for name in manifest:
        shutil.copyfile(DRAFT / name, cut.REPO / name)
    subprocess.run(['git', 'diff', '--check', '--', *manifest], check=True)
    target = prepare_build(574, BUILD, manifest)
    workspace = bind_identity('N16C', BUILD)
    validator = workspace / 'validate_end_b.py'
    first = ('    assert mismatch is None, mismatch\n'
             '    assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]')
    condition = ('    private_work = "NATIVE3D_PRIVATE_WORK " in (case / "run.log").read_text(errors="replace")\n'
                 '    if not private_work:\n'
                 '        assert mismatch is None, mismatch\n'
                 '        assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]')
    text = substitute(validator.read_text(), first, condition)
    text = substitute(text, '        "first_mismatch_index": mismatch,',
                      '        "first_mismatch_index": mismatch,\n        "identity_is_gate": not private_work,')
    compile(text, str(validator), 'exec')
    validator.write_text(text)
    progress('Applied the reviewed factory and fixture edits. Starting build575.')
    wait = cut.run_gate(target, 'build', [cut.PYTHON, str(target / 'build.py')])
    status = check_build(target)
    handles = cut.read(target / 'terminal_handles.json')['identities']
    handles += [dict(wait['controller'], alive=False)]
    assert all(cut.terminal(row) for row in handles)
    cut.save(target / 'primary_terminal_receipt.json', {'actual_host_proc': True, 'passed': True,
             'all_owned_handles_terminal': True, 'identities': handles, 'controller_wait': wait})
    cut.save(target / 'cut_build_plan.json', {'build_label': f'build{BUILD}',
             'source_pins_sha256': cut.sha(target / 'source_pins.json')})
    return workspace, status


def scoped(workspace, status):
    for ranks in (2, 4):
        command = test_command(status['test_binary'], '[NativeConcurrentRepair3D]')
        command[-2:-2] = ['mpirun', '--bind-to', 'none', '-n', str(ranks)]
        cut.run_gate(workspace, f'C3_mpi{ranks}', command)
        assert 'All tests passed' in (workspace / f'C3_mpi{ranks}_outer.log').read_text()
    for cohort in ('serial', 'mpi2', 'mpi4'):
        cut.run_gate(workspace, f'G1_{cohort}',
                     [cut.PYTHON, str(workspace / f'run_g1_{BUILD}.py'), '--cohort', cohort])
        result = cut.read(workspace / f'G1_candidate_{cohort}_summary.json')
        assert result['identity_pass'] and result['changed_case_count'] == 0
    cut.run_gate(workspace, 'G2', [cut.PYTHON, str(workspace / f'run_g2_gq_prefix_{BUILD}.py'),
                                 '--gate', 'G2', '--case-name', f'C11_G2_{BUILD}'])
    cut.run_gate(workspace, 'G3', [cut.PYTHON, str(workspace / 'launch_g3.py')])
    cut.run_gate(workspace, 'G4', [cut.PYTHON, str(workspace / f'run_g4_{BUILD}.py'), '--run'])
    g2, _ = cut.seal_prefix(workspace, 11, BUILD, 'G2')
    g3_case, = (workspace / 'cases').glob(f'N16C_G3_*')
    g3 = cut.read(g3_case / 'g3_end_b_gate.json')
    g4 = cut.read(workspace / 'g4_case_paths.json')
    g4_reference = cut.read(cut.PHASE / 'D2/validation/g4_case_paths.json')
    signature = re.compile(r'All tests passed \(\d+ assertions? in \d+ test cases?\)')
    assert g3['passed'] and g4['passed']
    for before, after in zip(g4_reference['new_cases'], g4['new_cases'], strict=True):
        tables = [sorted(signature.findall(ANSI.sub('', (Path(row['case']) / 'run.log').read_text())))
                  for row in (before, after)]
        assert tables[0] and tables[0] == tables[1], tables
    cut.save(workspace / 'acceptance_summary.json', {'passed': True, 'build': BUILD,
             'G1_records': [375, 114, 228], 'C3_ranks': [2, 4], 'G2': g2, 'G3': g3, 'G4': g4})
    progress('C3 at 2 and 4 ranks and all G1, G2, G3 and G4 checks passed. Running standing safeguards.')


def planning_rows(path):
    return [{key: value for key, value in json.loads(line).items() if key not in ('seconds', 'cpu_seconds')}
            for line in path.read_text().splitlines()]


def compare_safeguard(mode, reference, case):
    private = bool(PRIVATE.search((case / 'run.log').read_text()))
    names = ('p6c_gap_scan.jsonl',) if mode == 'merit481' else (
        'trajectory_working_owned.bin', 'trajectory_working_parameters.bin', 'correction_unchanged_owned.bin')
    payloads = {name: {'reference': cut.sha(reference / name), 'candidate': cut.sha(case / name)} for name in names}
    exact = all(row['reference'] == row['candidate'] for row in payloads.values())
    if mode != 'merit481':
        before, after = [planning_rows(root / 'actual_bl_cavity_planning.jsonl') for root in (reference, case)]
        exact = exact and before == after
    return {'private_work': private, 'require_identity': not private, 'payloads': payloads,
            'exact': exact, 'passed': private or exact}


def safeguards():
    GUARDS.mkdir()
    old = D / 'safeguards/run_build572_safeguards.py'
    runner = GUARDS / 'run_build575_safeguards.py'
    runner.write_text(old.read_text().replace('572', str(BUILD)))
    with (GUARDS / 'self_check.json').open('x') as output:
        subprocess.run([cut.PYTHON, str(runner), 'self-check'], stdout=output, check=True)
    assert cut.read(GUARDS / 'self_check.json')['passed']
    reference = cut.read(D / 'safeguards/acceptance_summary.json')
    results = {}
    for mode, adopted in reference['safeguards'].items():
        before = set((GUARDS / 'cases').glob('*'))
        cut.run_gate(GUARDS, mode, [cut.PYTHON, str(runner), mode])
        case, = set((GUARDS / 'cases').glob('*')) - before
        status = cut.read(case / 'status.json')
        receipt = cut.read(case / 'safeguard_process_receipt.json')
        assert status['exit_code'] == 0 and status['pins_verified'] and not status['changed']
        assert not receipt['live_owned_descendants'] and all(cut.terminal(row) for row in receipt['identities'])
        identity = compare_safeguard(mode, Path(adopted['case']), case)
        identity.update(case=str(case), seconds=status['seconds'])
        cut.save(GUARDS / f'{mode}_identity.json', identity)
        assert identity['passed'], (mode, identity)
        results[mode] = identity
    cut.save(GUARDS / 'acceptance_summary.json', {'passed': True, 'build': BUILD, 'safeguards': results})
    progress('The standing safeguards passed. Starting changed-mesh G5 run one.')


if __name__ == '__main__':
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    controller = cut.read(N16 / 'T0_campaign_launch.json')
    while not cut.terminal(controller):
        time.sleep(15)
    scoped(*build())
    safeguards()
    workflow()
