"""Qualify the ownership correction and print-only M28 on the default-width path."""
from pathlib import Path
import re
import subprocess
import sys

N18 = Path(__file__).resolve().parent
sys.path.insert(0, str(N18.parent / 'N6'))
from run_cut import bind_identity, check_build, cut, seal


def git(*args):
    return subprocess.check_output(['git', *map(str, args)], cwd=cut.REPO, text=True).strip()


def progress(message):
    with (cut.NIGHT / 'progress.md').open('a') as file:
        file.write(f'\nN18. {message}\n')


def bind_build(previous, current, changed, base_pins):
    target = cut.PHASE / f'build{current}'
    pins = cut.read(base_pins)
    for path in changed:
        absolute = cut.REPO / path
        assert str(absolute) in pins
        pins[str(absolute)] = cut.sha(absolute)
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    target.mkdir()
    cut.save(target / 'source_pins.json', pins)
    previous_builder = cut.PHASE / f'build{previous}/build.py'
    (target / 'build.py').write_text(previous_builder.read_text().replace(str(previous), str(current)))
    return target


def verify_g4(workspace, reference):
    current = cut.read(workspace / 'g4_case_paths.json')
    original = cut.read(reference / 'g4_case_paths.json')
    assert current['passed'] and original['passed']
    signature = re.compile(r'All tests passed \(\d+ assertions? in \d+ test cases?\)')
    ansi = re.compile(r'\x1b\[[0-9;]*m')
    for pair in zip(original['new_cases'], current['new_cases'], strict=True):
        output = [(Path(row['case']) / 'run.log').read_text() for row in pair]
        tables = [sorted(signature.findall(ansi.sub('', log))) for log in output]
        assert tables[0] and tables[0] == tables[1], tables


def checkpoint(message, paths):
    unique = sorted({Path(path) for path in paths})
    assert not git('diff', '--cached', '--name-only')
    assert all(Path(path).is_file() for path in unique)
    git('add', '-f', '--', *unique)
    git('diff', '--cached', '--check', '--', 'Common', 'SU2_CFD', 'UnitTests', cut.NIGHT / 'progress.md')
    git('commit', '-m', message)
    git('push', 'origin', 'HEAD:refs/heads/codex/native-3d-bl')
    return git('rev-parse', 'HEAD')


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert cut.terminal(cut.read(N18 / 'entry_boundary_recovery_launch.json'))
    assert git('rev-parse', 'HEAD') == cut.read(N18 / 'amendment_checkpoint.json')['commit']
    assert (N18 / 'source_check.md').is_file() and cut.read(N18 / 'ready_after_n6.json')['n6_passed']
    candidate = N18 / 'drafts/DE/parts_D_E.patch'
    expected = subprocess.check_output(['git', 'diff', '--', 'Common/src/adaptation'], cwd=cut.REPO)
    assert candidate.read_bytes() == expected
    changed = git('diff', '--name-only', '--', 'Common/src/adaptation').splitlines()
    assert len(changed) == 13 and all(not path.endswith('.orig') for path in changed)
    assert 'ProductionEngineOptions' not in (cut.REPO / 'Common/include/adaptation/CNativeEngine3D.hpp').read_text()
    build = 579
    target = bind_build(578, build, changed, N18 / 'base578_source_pins.json')
    workspace = bind_identity('N18DE', build)
    progress('Applied the ownership cool-down and print-only M28. Compiling build579 with the default repair width.')
    cut.qualify(11, build, workspace, target)
    check_build(target)
    verify_g4(workspace, cut.PHASE / 'N6Selection/validation')
    seal('N18DE', 11, build, workspace, target)
    summary = cut.read(workspace / 'acceptance_summary.json')
    assert [row['records'] for row in summary['G1'].values()] == [375, 114, 228]
    g3 = summary['G3']
    progress(f"D/E build579 passed G1 375/114/228, G2 {summary['G2']['work']:,}, "
             f"G3 {g3['accepted_splits']} splits with {g3['common_edit_count']} exact shared edits, "
             f"G4 and GQ at {summary['GQ']['seconds_until_stop']:.3f} s. No full D/E run.")
    extras = [path for path in N18.glob('entry_boundary*.json')] + [path for path in N18.glob('entry_boundary*.log')]
    extras += [N18 / name for name in ('enter_after_n6.py', 'enter_after_n6.initial.py', 'ready_after_n6.json',
               'amendment_checkpoint.json', 'before_DE_host_preflight.json', 'base578_source_pins.json',
               'DE_campaign_launch.json', 'source_check.md', 'qualified_build578/manifest.json',
               'drafts/E2/E2.patch', 'drafts/E2/manifest.json', 'drafts/F/wide.patch')]
    paths = [*cut.read(workspace / 'checkpoint_paths.json'), *changed, *extras,
             workspace / 'checkpoint_paths.json', workspace / 'run_g2_gq_prefix.py',
             candidate, Path(__file__).resolve(), cut.NIGHT / 'progress.md', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md']
    head = checkpoint('Correct wide-round ownership cool-down and measure M28', paths)
    cut.save(N18 / 'DE_checkpoint.json', {'passed': True, 'build': build, 'commit': head, 'source_paths': changed})
    progress(f'D/E checkpoint pushed as `{head[:10]}`. Part F follows.')


if __name__ == '__main__':
    main()
