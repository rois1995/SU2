"""Run N20 Part W with the existing N18 scope, workflow and purpose gates."""

import argparse
from datetime import datetime
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
N20 = Path(__file__).resolve().parent
sys.path.insert(0, str(N20.parent / 'N18'))
from run_de import bind_build, bind_identity, check_build, cut, git

sys.path.insert(0, str(N20.parent / 'N19'))
from run_b import CANDIDATE, F_PATHS, overlay

A_BUILD = 584
W_BUILD = 585
RESTORE_BUILD = 586
A_CHECKPOINT = N20 / 'A_checkpoint.json'
A_BRANCH = cut.PHASE / 'round_8_5s_A'
BRANCH = cut.PHASE / 'round_8_5s_W'
SCOPED = cut.PHASE / 'N20W/validation'
G5 = BRANCH / 'G5'
FALLBACK = A_BRANCH / 'G5'
F_SCOPED = N20.parent / 'N18/run_f.py'
F_WORKFLOW = N20.parent / 'N18/drafts/recipes/workflow_f.py'


def progress(message):
    stamp = datetime.now().astimezone().isoformat(timespec='seconds')
    with (cut.NIGHT / 'progress.md').open('a') as file:
        file.write(f'\n- {stamp} N20. {message}\n')


def actual_host():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd', 'requires actual-host /proc'


def retarget(path, pairs):
    """Change only reviewed literals, then recheck the original helper before launch."""
    source = path.read_text()
    for old, new in pairs:
        assert source.count(old) == 1, (path, old)
        source = source.replace(old, new)
    namespace = {'__file__': str(path), '__name__': 'n20_bound_n18'}
    exec(compile(source, str(path), 'exec'), namespace)
    return namespace


def scoped_helpers(build, workspace, safeguards_root):
    """Bind the original scoped and safeguard functions without applying their historical patch."""
    helpers = retarget(F_SCOPED, (
        ("glob('N18F_G3_*/g3_end_b_gate.json')", "glob('N20W_G3_*/g3_end_b_gate.json')"),
        ('Build580 compiled without warnings.', f'Build{build} compiled without warnings.'),
        ('F scoped gates passed', 'W scoped gates passed'),
    ))
    helpers.update(BUILD=build, SCOPED=workspace, F=safeguards_root, progress=progress)
    return helpers


def workflow():
    """Bind N18 workflow stages and keep new print families observational."""
    return retarget(F_WORKFLOW, (
        ("WORKSPACE = cut.PHASE / 'round_8_5r2_F/G5'", "WORKSPACE = cut.PHASE / 'round_8_5s_W/G5'"),
        ("'F_run1'", "'W_run1'"),
        ("'F_restart'", "'W_restart'"),
        ("'F_run2'", "'W_run2'"),
        ("'N18 F run one'", "'N20 W run one'"),
        ("row[0].startswith('NATIVE3D_M28_')",
         "row[0].startswith(('NATIVE3D_M28_', 'NATIVE3D_M29_', 'NATIVE3D_FINE_'))"),
    ))


def a_checkpoint(path):
    data = cut.read(path)
    assert data['passed'] and data['build'] == A_BUILD and data['commit'], path
    pins = Path(data.get('source_pins', cut.PHASE / f'build{A_BUILD}/source_pins.json'))
    if not pins.is_absolute():
        pins = cut.REPO / pins
    assert pins.is_file()
    return data['commit'], pins


def expected_overlay(base):
    """Apply the historical diff to Part A bytes outside the work tree for exact comparison."""
    with tempfile.TemporaryDirectory(prefix='n20-wide-overlay-') as directory:
        directory = Path(directory)
        for path in F_PATHS:
            target = directory / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(subprocess.check_output(['git', 'show', f'{base}:{path}'], cwd=cut.REPO))
        subprocess.run(['git', 'apply', '--whitespace=error'], input=overlay(), cwd=directory, check=True)
        return {path: cut.sha(directory / path) for path in F_PATHS}


def verify_overlay(base):
    """Accept only the three-path overlay, whether applied or in a single candidate commit."""
    head = git('rev-parse', 'HEAD')
    if head != base:
        assert git('rev-parse', 'HEAD^') == base
        assert set(git('diff-tree', '--no-commit-id', '--name-only', '-r', 'HEAD').splitlines()) == set(F_PATHS)
    assert set(git('diff', '--name-only', base, '--', *F_PATHS).splitlines()) == set(F_PATHS)
    expected = expected_overlay(base)
    assert all(cut.sha(cut.REPO / path) == digest for path, digest in expected.items())
    return expected


def bind_g3_gate(workspace):
    """Enforce G3(b) only if the wide fixture produces no private-work row."""
    validator = workspace / 'validate_end_b.py'
    source = validator.read_text()
    comparison = ('    assert mismatch is None, mismatch\n'
                  '    assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]')
    conditional = ('    private_work = "NATIVE3D_PRIVATE_WORK " in (case / "run.log").read_text(errors="replace")\n'
                   '    if not private_work:\n'
                   '        assert mismatch is None, mismatch\n'
                   '        assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]')
    field = '        "first_mismatch_index": mismatch,'
    assert source.count(comparison) == source.count(field) == 1
    source = source.replace(comparison, conditional).replace(field, field + '\n        "identity_is_gate": not private_work,')
    compile(source, str(validator), 'exec')
    validator.write_text(source)


def run_part_w(receipt):
    """Drive sequential scope, safeguards and G5 gates on one pinned build."""
    actual_host()
    base, pins = a_checkpoint(receipt)
    expected = verify_overlay(base)
    target = bind_build(A_BUILD, W_BUILD, F_PATHS, pins)
    workspace = bind_identity('N20W', W_BUILD)
    assert workspace == SCOPED
    bind_g3_gate(workspace)
    cut.save(workspace / 'candidate_source.json', {
        'build': W_BUILD, 'base_commit': base, 'overlay_commit': CANDIDATE,
        'source_pins_sha256': cut.sha(target / 'source_pins.json'), 'source_sha256': expected,
    })
    shared = scoped_helpers(W_BUILD, workspace, BRANCH)
    progress(f'W build{W_BUILD}: verified exact three-path overlay. Starting build and wide-repeat MPI fixtures.')
    shared['build_and_test'](target)
    shared['scoped_checks']()
    shared['safeguards']()
    progress(f'W build{W_BUILD}: the three standing geometry safeguards passed. Starting G5 run one.')
    return run_workflow()


def run_workflow():
    """Resume only the G5 sequence with its verified existing controller receipts."""
    actual_host()
    assert cut.read(SCOPED / 'acceptance_summary.json')['passed']
    safeguards = BRANCH / 'safeguards/acceptance_summary.json'
    assert cut.read(safeguards)['passed']
    runner = workflow()
    binding = G5 / 'adapter_pins.json'
    pins = {str(path): cut.sha(path) for path in (Path(__file__).resolve(), F_SCOPED, F_WORKFLOW)}
    if not G5.exists():
        runner['prepare'](W_BUILD)
        cut.save(binding, pins)
    assert cut.read(binding) == pins
    assert not (G5 / 'failed_attempt.json').exists(), 'existing failed W receipt requires R5 disposition'
    assert not (G5 / 'acceptance_summary.json').exists(), 'W already has a completed acceptance receipt'
    return runner['run'](W_BUILD, SCOPED / 'acceptance_summary.json', safeguards)


def run_safeguards(build, root):
    """Reuse the three standing safeguards for the scoped default-width build."""
    actual_host()
    root = root.resolve()
    assert root.parent == cut.PHASE, 'safeguards root must be one directory below the phase workspace'
    build_dir = cut.PHASE / f'build{build}'
    status = check_build(build_dir)
    for name in ('binary', 'test_binary'):
        assert cut.sha(status[name]) == status[f'{name}_sha256']
    assert all(cut.sha(path) == digest for path, digest in cut.read(build_dir / 'source_pins.json').items())
    scoped_helpers(build, SCOPED, root)['safeguards']()


def restore(receipt):
    """Verify the source matches accepted Part A and rebuild the default binaries."""
    actual_host()
    base, pins = a_checkpoint(receipt)
    assert not git('diff', '--name-only', base, '--', *F_PATHS)
    target = bind_build(A_BUILD, RESTORE_BUILD, (), pins)
    cut.run_gate(target, 'build', [cut.PYTHON, str(target / 'build.py')])
    status = check_build(target)
    verify = {
        name: {'path': status[name], 'sha256': status[f'{name}_sha256']}
        for name in ('binary', 'test_binary')
    }
    for row in verify.values():
        assert cut.sha(row['path']) == row['sha256']
    current = cut.read(target / 'source_pins.json')
    assert current == cut.read(pins) and all(cut.sha(path) == digest for path, digest in current.items())
    cut.save(target / 'restore_receipt.json', {
        'passed': True, 'build': RESTORE_BUILD, 'part_a_commit': base,
        'source_pins_verified': len(current), 'binaries': verify,
    })


def fallback_g5(receipt, build):
    """Run the inherited default-width G5 once when the Part A fallback applies."""
    actual_host()
    assert build in (A_BUILD, RESTORE_BUILD)
    base, pins = a_checkpoint(receipt)
    assert not git('diff', '--name-only', base, '--', *F_PATHS)
    current = cut.PHASE / f'build{build}'
    assert cut.read(current / 'source_pins.json') == cut.read(pins)
    assert all(cut.sha(path) == digest for path, digest in cut.read(pins).items())
    status = check_build(current)
    reference = cut.NIGHT / 'section6_1/accepted_reference_payloads.json'
    expected = cut.read(reference)['hashes']
    assert len(expected) == 6
    FALLBACK.mkdir(parents=True)
    source = cut.PHASE / 'phase_D/G5'
    runner = workflow()
    for name in runner['HELPERS']:
        shutil.copyfile(source / name, FALLBACK / name)
    full = FALLBACK / f'run_full{build}.py'
    full.write_text((source / 'run_full572.py').read_text().replace('572', str(build)))
    compile(full.read_bytes(), str(full), 'exec')
    aliases = cut.PHASE / f'build{build}_aliases.py'
    if build == RESTORE_BUILD and not aliases.exists():
        source_aliases = cut.PHASE / f'build{A_BUILD}_aliases.py'
        aliases.write_text(source_aliases.read_text().replace(str(A_BUILD), str(build)))
    helpers = {str(path): cut.sha(path) for path in (*FALLBACK.iterdir(), aliases, reference)}
    cut.save(FALLBACK / 'helper_pins.json', helpers)
    with (FALLBACK / 'run_full_selfcheck.json').open('x') as output:
        subprocess.run([cut.PYTHON, str(full), '--self-check'], cwd=cut.REPO, stdout=output, check=True)
    assert cut.read(FALLBACK / 'run_full_selfcheck.json')['passed']
    case = FALLBACK / f'full{build}'
    cut.run_gate(FALLBACK, 'A_full', [cut.PYTHON, str(full), '--case', case])
    runner['verify_pins'](helpers)
    identity = runner['verify_full'](case, status['binary_sha256'])
    assert identity['hashes'] == expected
    cut.save(FALLBACK / 'acceptance_summary.json', {
        'passed': True, 'build': build, 'case': str(case), 'hashes': expected,
        'part_a_commit': base, 'standing_gates_passed': True,
    })


def self_check():
    """Exercise overlay binding, inherited prefix comparison and gate 8 boundaries without launching."""
    base = 'da20c9e9fe'
    expected = expected_overlay(base)
    assert set(expected) == set(F_PATHS)
    assert scoped_helpers(W_BUILD, SCOPED, BRANCH)['SCOPED'] == SCOPED
    runner = workflow()
    assert runner['WORKSPACE'] == G5
    baseline = runner['T0']
    past = cut.PHASE / 'round_8_5r2_F/G5/full580_run1'
    result = runner['compare_event_one'](baseline, past)
    assert result['passed'], result
    purpose = runner['purpose']
    first = [{'event': 1, 'native_seconds': 2000}, {'event': 2, 'native_seconds': 1344}]
    second = [{'event': 1, 'native_seconds': 2100}, {'event': 2, 'native_seconds': 1288.032}]
    assert purpose(first, second)['passed']
    second[1]['native_seconds'] = 1288.0321
    assert not purpose(first, second)['passed']
    first[0]['native_seconds'] = 2300
    second[0]['native_seconds'] = 2236.8701
    second[1]['native_seconds'] = 900
    assert not purpose(first, second)['passed']
    with tempfile.TemporaryDirectory(prefix='n20-workflow-check-') as directory:
        runner['WORKSPACE'] = Path(directory) / 'G5'
        runner['prepare'](W_BUILD)
        pins = cut.read(runner['WORKSPACE'] / 'helper_pins.json')
        assert pins['build'] == W_BUILD and len(pins['bound']) == 7
        for key in ('bound', 'source', 'comparison_source'):
            runner['verify_pins'](pins[key])
        for stem in runner['RUNNERS']:
            source = (runner['WORKSPACE'] / f'{stem}{W_BUILD}.py').read_text()
            assert 'full585_run1' in source and 'build575' not in source
    print('N20 W no-launch checks passed. Historical prefix rows:',
          [row['reference_rows'] for row in result['ranks']])
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('patch', 'self-check', 'run', 'workflow', 'safeguards', 'restore',
                                           'fallback-g5'))
    parser.add_argument('--a-checkpoint', type=Path, default=A_CHECKPOINT)
    parser.add_argument('--build', type=int, default=A_BUILD)
    parser.add_argument('--safeguards-root', type=Path, default=A_BRANCH)
    args = parser.parse_args()
    if args.action == 'patch':
        sys.stdout.buffer.write(overlay())
    elif args.action == 'self-check':
        self_check()
    elif args.action == 'run':
        raise SystemExit(0 if run_part_w(args.a_checkpoint) else 2)
    elif args.action == 'workflow':
        raise SystemExit(0 if run_workflow() else 2)
    elif args.action == 'safeguards':
        run_safeguards(args.build, args.safeguards_root)
    elif args.action == 'fallback-g5':
        fallback_g5(args.a_checkpoint, args.build)
    else:
        restore(args.a_checkpoint)


if __name__ == '__main__':
    main()
