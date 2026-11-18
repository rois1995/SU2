"""Seal the passed M27 qualification as its one implementation checkpoint."""
from pathlib import Path
import subprocess

import run_cut_step as cut


def main():
    phase, night = cut.PHASE, cut.NIGHT
    workspace = phase / 'M27/validation'
    build = phase / 'build570'
    summary = cut.read(workspace / 'acceptance_summary.json')
    assert summary['state'] == 'PASSED' and all(summary[gate]['passed'] for gate in ('G2','G3','G4','GQ'))
    assert all(cut.sha(path) == digest for path, digest in cut.read(build / 'source_pins.json').items())
    controller = cut.read(phase / 'phase_C/M27_scoped_campaign_launch.json')
    assert cut.terminal(controller)
    candidate = cut.read(workspace / 'candidate_source.json')
    assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip() == candidate['base_commit']
    paths = [cut.REPO / path for path in candidate['source_paths']]
    paths += list(workspace.glob('*.json')) + list(workspace.glob('*.py'))
    paths += list(build.glob('*.json')) + [build / 'build.log', build / 'build.py']
    paths += [phase / 'build570_aliases.py', night / 'N17/source_check.md', night / 'N17/report_m27.py',
              night / 'N17/m27_observation_self_check.cpp', night / 'QUESTIONS_FOR_REVIEWER.md']
    control = phase / 'phase_C'
    paths += [control / name for name in ('qualify_m27.py','seal_m27.py','M27_handoff_check.json',
                                         'M27_scoped_campaign_launch.json','M27_scoped_campaign.log',
                                         'inputs/MEASUREMENT_M27.md','inputs/MEASUREMENT_M27.md.sha256')]
    for gate in ('G2','GQ'):
        _, artifacts = cut.seal_prefix(workspace,11,570,gate)
        paths += artifacts
    for cohort in ('serial','mpi2','mpi4'):
        result = summary['G1'][cohort]
        assert result['identity_pass'] and result['changed_case_count'] == 0
        case = Path(result['candidate_attempt'])
        terminal = cut.read(case / 'n11_terminal_receipt.json')
        assert terminal['all_owned_handles_terminal'] and all(cut.terminal(row) for row in terminal['owned_processes'])
        paths += [case / name for name in ('status.json','command.json','n11_terminal_receipt.json')]
        paths += [cut.pack(case / name) for name in ('run.log','resources.jsonl')]
    cases = [Path(summary['G3']['case']), *(Path(row['case']) for row in summary['G4']['new_cases'])]
    for case in cases:
        paths += list(case.glob('*.json'))
        paths += [cut.pack(case / name) for name in ('run.log','resources.jsonl')]
    paths += [workspace / 'run_g2_gq_prefix.py', night / 'progress.md']
    progress = night / 'progress.md'
    live = progress.read_text()
    changed = live.index('Current action — N17 M27:')
    updated = ('Current action — N17 M27 scoped gates passed on build570. '
               'One print-only checkpoint is being sealed for the single phase C G5.\n')
    progress.write_text(live[:changed] + updated)
    subprocess.run(['git','diff','--cached','--quiet'],check=True)
    subprocess.run(['git','add','-f','--',*map(str,sorted(set(paths)))],check=True)
    subprocess.run(['git','diff','--cached','--check','--','Common'],check=True)
    subprocess.run(['git','commit','-m','Measure native wall union queries and smoothing chains (M27)'],check=True)
    head = subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
    subprocess.run(['git','push','origin','HEAD:refs/heads/codex/native-3d-bl'],check=True)
    cut.save(control / 'M27_checkpoint.json',{
        'scoped_state':'PASSED','build':570,'commit':head,'full_phase_C_gate':'PENDING',
        'all_scoped_controller_handles_terminal':True,'source_pins_sha256':cut.sha(build/'source_pins.json')})
    progress.write_text(live[:changed] + f'- N17 M27 scoped gates passed at `{head[:10]}`, build570. '
        'G1/G2/G3/G4/GQ receipts are in `N4/section6_3_after_N11/M27/validation`.\n\n'
        'Current action — launch the single phase C G5 with build570.\n')
    print(f'M27 scoped checkpoint pushed: {head}',flush=True)


if __name__ == '__main__':
    main()
