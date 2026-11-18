"""Reuse the passed build577 gates and run the first GQ after disk recovery."""
from pathlib import Path
import shutil

from continue_cut import launch
from run_cut import N6, check_build, commit, cut, progress, seal


def run_missing_gq():
    prior = cut.read(N6 / 'Optimizer_campaign_launch.json')
    assert cut.terminal(prior)
    assert cut.read(N6 / 'Optimizer_campaign_wait.json')['returncode'] == 1
    workspace = cut.PHASE / 'N6Optimizer/validation'
    evidence = N6 / 'R3_disk_preflight'
    retained = evidence / 'first_GQ'
    assert 'less than 2 GB free on evidence filesystem' in (workspace / 'GQ_outer.log').read_text()
    assert not (workspace / 'cases/C10_GQ_577').exists()
    for cohort, count in (('serial', 375), ('mpi2', 114), ('mpi4', 228)):
        row = cut.read(workspace / f'G1_candidate_{cohort}_summary.json')
        assert row['identity_pass'] and row['ranks'] * row['expected_cases_per_rank'] == count
        receipt = cut.read(Path(row['candidate_attempt']) / 'n11_terminal_receipt.json')
        assert receipt['all_owned_handles_terminal']
        assert all(cut.terminal(item) for item in receipt['owned_processes'])
    cut.seal_prefix(workspace, 10, 577, 'G2')
    g3, = (workspace / 'cases').glob('N6Optimizer_G3_*/g3_end_b_gate.json')
    assert cut.read(g3)['passed']
    terminal = cut.read(g3.parent / 'g3_terminal_receipt.json')
    assert terminal['all_owned_handles_terminal'] and all(cut.terminal(row) for row in terminal['identities'])
    build = cut.PHASE / 'build577'
    status = check_build(build)
    assert all(cut.sha(status[name]) == status[f'{name}_sha256'] for name in ('binary', 'test_binary'))
    pins = cut.read(build / 'source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    space = cut.read(evidence / 'space_recovery.json')
    assert all(cut.sha(row['original_path']) == row['raw_sha256'] and
               cut.sha(row['compressed_path']) == row['compressed_sha256'] for row in space['archives'])
    assert shutil.disk_usage(workspace).free > 2_000_000_000
    retained.mkdir()
    for suffix in ('host_preflight.json', 'launch.json', 'outer.log', 'wait.json'):
        source = workspace / f'GQ_{suffix}'
        sha = cut.sha(source)
        target = retained / source.name
        source.rename(target)
        assert cut.sha(target) == sha
    progress(f"Optimizer GQ prelaunch recovery preserved all passed gates and restored "
             f"{shutil.disk_usage(workspace).free:,} B free without changing the guard. Starting GQ only.")
    cut.run_gate(workspace, 'GQ',
                 [cut.PYTHON, str(workspace / 'run_g2_gq_prefix_577.py'), '--gate', 'GQ',
                  '--case-name', 'C10_GQ_577'])
    seal('N6Optimizer', 10, 577, workspace, build)
    draft = N6 / 'drafts/optimizer'
    manifest = cut.read(draft / 'manifest.json')
    changed = [row['path'] for row in manifest['files']]
    assert all(cut.sha(cut.REPO / row['path']) == row['sha256'] for row in manifest['files'])
    extra = [draft / name for name in ('manifest.json', 'readability.json', 'render_optimizer.py',
                                      'syntax_attempt1.log', 'syntax_attempt1.exit')]
    extra += [N6 / 'Optimizer_before_edit_preflight.json', N6 / 'Remesh_checkpoint.json',
              N6 / 'Optimizer_campaign_launch.json', N6 / 'Optimizer_campaign_wait.json',
              N6 / 'recover_campaign_launch.json', Path(__file__).resolve()]
    extra += list(evidence.glob('*.json')) + list(retained.glob('*.json'))
    extra += [evidence / 'archive_binaries.py', evidence / 'README.md', retained / 'GQ_outer.log']
    commit('Optimizer', 577, workspace, changed, extra)


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    run_missing_gq()
    launch('Selection', N6 / 'continue_cut.py', 'Selection')
    phase = cut.PHASE / 'phase_N6'
    launch('G5', phase / 'run_g5.py', 578)
    launch('standing_safeguards', phase / 'run_standing_safeguards.py')


if __name__ == '__main__':
    main()
