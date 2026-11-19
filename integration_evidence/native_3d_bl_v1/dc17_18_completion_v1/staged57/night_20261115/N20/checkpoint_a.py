"""Commit the N20 Part A identity checkpoint once every executed gate has passed."""

from pathlib import Path

from run_a import BUILD, FIXTURES, N20, SOURCES, TARGET, WORK, cut, progress
from run_de import checkpoint

SAFEGUARDS = cut.PHASE / 'round_8_5s_A/safeguards'


def gate_evidence():
    paths = [*cut.read(WORK / 'checkpoint_paths.json'), WORK / 'checkpoint_paths.json',
             WORK / 'run_g2_gq_prefix.py']
    paths += list(SAFEGUARDS.glob('*.json')) + list(SAFEGUARDS.glob('*.py'))
    for case in (SAFEGUARDS / 'cases').iterdir():
        if case.is_dir():
            paths += [*case.glob('*.json'), *case.glob('*.cfg')]
            paths += [cut.pack(log) for log in (case / 'run.log', case / 'resources.jsonl') if log.is_file()]
    paths += [path for path in FIXTURES.iterdir() if path.suffix in ('.cpp', '.inc', '.template', '.py', '.json', '.md')]
    paths += [cut.pack(path) for path in FIXTURES.glob('*.log')]
    recovery = N20 / 'R3_G3_prelaunch'
    if recovery.is_dir():
        paths += list(recovery.iterdir())
    return paths


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert cut.terminal(cut.read(N20 / 'A_controller_launch.json'))
    recovery = N20 / 'A_recovery_launch.json'
    if recovery.is_file():
        assert cut.terminal(cut.read(recovery))
    summary = cut.read(WORK / 'acceptance_summary.json')
    assert summary['state'] == 'PASSED' and summary['build'] == BUILD
    assert cut.read(SAFEGUARDS / 'acceptance_summary.json')['passed']
    assert 'All tests passed' in (FIXTURES / 'optimizer_outer.log').read_text()
    assert 'good_reopened=0 reopened=4 done=4 repeated=0 reset=4' in (FIXTURES / 'selection_outer.log').read_text()
    pins = cut.read(TARGET / 'source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    gate = TARGET / 'build_status.json'
    build = cut.read(gate)
    assert build['pins_verified'] and build['exit_code'] == 0
    assert all(cut.sha(build[name]) == build[f'{name}_sha256'] for name in ('binary', 'test_binary'))
    g2, g3, gq = (summary[name] for name in ('G2', 'G3', 'GQ'))
    progress(f"Build{BUILD} passed F1–F4, G1 375/114/228, G2 {g2['work']:,}, "
             f"G3 {g3['accepted_splits']:,} splits with {g3['common_edit_count']:,} matching edits, "
             f"G4 1/2/4 and GQ {gq['seconds_until_stop']:.3f} s. Fixed storage 4,120. "
             "All three standing geometry safeguards passed. Next is W.")
    paths = [*SOURCES, *gate_evidence(), cut.NIGHT / 'progress.md', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md']
    paths += [N20 / name for name in ('run_a.py', 'run_w.py', 'checkpoint_a.py', 'source_check.md',
                                    'readability.md', 'A_controller_launch.json', 'A_fine_rows.json',
                                    'inputs/IMPLEMENTATION_8_5S.md',
                                    'inputs/SHA256SUMS')]
    paths.append(cut.pack(N20 / 'A_controller.log'))
    if recovery.is_file():
        paths += [recovery, cut.pack(N20 / 'A_recovery.log')]
    head = checkpoint('Try exact fine placement for stuck native 3D vertices', paths)
    cut.save(N20 / 'A_checkpoint.json', {'passed': True, 'build': BUILD, 'commit': head,
                                         'source_pins': str(TARGET / 'source_pins.json')})
    progress(f'Pushed `{head[:10]}`. Apply the exact historical overlay for W.')
    print(head, flush=True)


if __name__ == '__main__':
    main()
