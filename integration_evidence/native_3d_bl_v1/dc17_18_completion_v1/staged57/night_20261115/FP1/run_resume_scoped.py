"""Run FP1 resume's scoped checks through the already-qualified gate controllers."""

from argparse import ArgumentParser
import json
from pathlib import Path
import runpy
import subprocess
import sys

from run_g1 import ITEM, cut, verify_build
from run_de import verify_g4
from build_fp1 import BUILD as NINJA_ROOT

N22_SCOPE = cut.PHASE / 'N22C/validation'
N6_GUARDS = cut.PHASE / 'phase_N6/safeguards'
N22 = ITEM.parent / 'N22'
sys.path.insert(0, str(ITEM.parent / 'N16'))
from run_c import compare_safeguard


def progress(message):
    with (ITEM.parent / 'progress.md').open('a') as stream:
        stream.write(f'\nFP1 resume C. {message}\n')


def seal(path, payload):
    if path.is_file():
        assert cut.read(path) == payload, path
    else:
        cut.save(path, payload)


def legacy_build_receipts(build, receipt):
    """Expose the terminal controller and verified graph in the existing adapter schema."""
    wait = receipt['wait']
    assert wait['terminal'] and cut.terminal(wait['controller'])
    identity = {**wait['controller'], 'alive': False}
    handles = {'actual_host_proc': True, 'all_owned_handles_terminal': True,
               'identities': [identity], 'verified_unix': cut.read(build / 'build_launch.json')['unix'] + wait['elapsed_seconds']}
    seal(build / 'terminal_handles.json', handles)
    seal(build / 'primary_terminal_receipt.json', {**handles, 'passed': True, 'controller_wait': wait})
    plan = cut.read(build / 'build_plan.json')
    manifest = cut.sha(build / 'source_pins.json')
    graph = {str(NINJA_ROOT / name): digest for name, digest in receipt['graph_after'].items()}
    assert all(cut.sha(name) == digest for name, digest in graph.items())
    seal(build / 'generated_build_graph_after.json', {
        'build_status_sha256': cut.sha(build / 'build_status.json'), 'source_pins_sha256': manifest,
        'binary_sha256': receipt['binary_sha256'], 'test_binary_sha256': receipt['test_binary_sha256'],
        'before': plan['graph_before'], 'after': graph})
    seal(build / 'cut_build_plan.json', {'build_label': f"build{receipt['build']}", 'source_pins_sha256': manifest})


def bind(path, source, label, original_label, pairs):
    text = source.read_text().replace(str(original_label), str(label))
    for old, new in pairs:
        assert text.count(old) == 1, (source, old)
        text = text.replace(old, new)
    compile(text, str(path), 'exec')
    if path.is_file():
        assert path.read_text() == text
    else:
        path.write_text(text)
    return path


def run_gate(root, name, args):
    existing = root / f'{name}_wait.json'
    if existing.is_file():
        wait = cut.read(existing)
        assert wait['terminal'] and wait['returncode'] == 0 and cut.terminal(wait['controller'])
        return wait
    return cut.run_gate(root, name, [cut.PYTHON, '-B', *map(str, args)])


def g3(root, build, label):
    root.mkdir(exist_ok=True)
    rebound = [('PHASE = HERE.parents[1]', f'PHASE = Path({str(cut.PHASE)!r})'),
               (f'BUILD = PHASE / "build{label}"', f'BUILD = Path({str(build)!r})')]
    runner = bind(root / f'run_g3_{label}.py', N22_SCOPE / 'run_g3_591.py', label, 591,
                  [*rebound, (f'BUILD == PHASE / "build{label}"', f'BUILD == Path({str(build)!r})'),
                   ('CASE = "N22C_G3"', 'CASE = "FP1F_G3"')])
    bind(root / 'validate_end_b.py', N22_SCOPE / 'validate_end_b.py', label, 591, rebound)
    launcher = bind(root / 'launch_g3.py', N22_SCOPE / 'launch_g3.py', label, 591,
                    [("v=p/'N22C/validation'", "v=Path(__file__).resolve().parent")])
    subprocess.run([cut.PYTHON, '-B', str(runner), '--self-check'], cwd=cut.REPO, check=True,
                   stdout=subprocess.DEVNULL)
    run_gate(root, 'G3', [launcher])
    gate, = (root / 'cases').glob('FP1F_G3_*/g3_end_b_gate.json')
    result = cut.read(gate)
    assert result['passed']
    return result


def g4(root, build, label):
    root.mkdir(exist_ok=True)
    runner = bind(root / f'run_g4_{label}.py', N22_SCOPE / 'run_g4_591.py', label, 591, [
        ('N4 = HERE.parents[2]', f'N4 = Path({str(ITEM.parent / "N4")!r})'),
        ('PHASE = HERE.parents[1]', f'PHASE = Path({str(cut.PHASE)!r})'),
        (f'BUILD_DIR = PHASE / "build{label}"', f'BUILD_DIR = Path({str(build)!r})'),
        ('    step, _, _ = adapter.checked_step', '    adapter.HERE = BUILD_DIR.parent\n    step, _, _ = adapter.checked_step'),
    ])
    run_gate(root, 'G4', [runner, '--run'])
    verify_g4(root, N22_SCOPE)
    return cut.read(root / 'g4_case_paths.json')


def saved_safeguards(root, build, label):
    root.mkdir(exist_ok=True)
    runner = bind(root / f'run_build{label}_safeguards.py', N6_GUARDS / 'run_build578_safeguards.py', label, 578, [
        ('N11 = HERE.parents[1]', f'N11 = Path({str(cut.PHASE)!r})'),
        (f'BUILD = N11 / "build{label}"', f'BUILD = Path({str(build)!r})'),
        (f'ALIAS_SOURCE = N11 / "build{label}_aliases.py"', 'ALIAS_SOURCE = N11 / "build578_aliases.py"'),
    ])
    if not (root / 'self_check.json').exists():
        data = subprocess.check_output([cut.PYTHON, '-B', str(runner), 'self-check'], cwd=cut.REPO)
        (root / 'self_check.json').write_bytes(data)
    assert cut.read(root / 'self_check.json')['passed']
    results = {}
    cases = root / 'cases'
    for mode, reference in cut.read(N6_GUARDS / 'acceptance_summary.json')['safeguards'].items():
        identity_path = root / f'{mode}_identity.json'
        if identity_path.exists():
            result = cut.read(identity_path)
        else:
            run_gate(root, mode, [runner, mode])
            log = (root / f'{mode}_outer.log').read_text()
            start = log.rindex('\n{\n') + 1
            summary = json.loads(log[start:])
            assert summary['mode'] == mode
            process_receipt = Path(summary['run_terminal_receipt'])
            case = process_receipt.parent
            assert case.parent == cases and cut.sha(process_receipt) == summary['run_terminal_receipt_sha256']
            status = cut.read(case / 'status.json')
            handles = cut.read(process_receipt)
            assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
            assert not status['changed'] and not handles['live_owned_descendants']
            assert all(cut.terminal(row) for row in handles['identities'])
            assert all(cut.sha(path) == digest for path, digest in cut.read(case / 'pins.json').items())
            result = compare_safeguard(mode, Path(reference['case']), case)
            result.update(case=str(case), seconds=status['seconds'])
            cut.save(identity_path, result)
        assert result['passed'], result
        results[mode] = result
    return results


def run(build, g1_root):
    receipt = cut.read(build / 'build_status.json')
    verify_build(receipt, build / 'source_pins.json')
    label = receipt['build']
    legacy_build_receipts(build, receipt)
    g1 = [cut.read(g1_root / cohort / 'comparison.json') for cohort in ('serial', 'mpi2', 'mpi4')]
    assert all(row['identity_pass'] for row in g1) and [row['records'] for row in g1] == [375, 114, 228]
    assert all(cut.read(g1_root / cohort / 'pins.json')['test_binary'] == receipt['test_binary_sha256']
               for cohort in ('serial', 'mpi2', 'mpi4'))
    root = ITEM / 'resume' / f'scoped{label}'
    root.mkdir(parents=True, exist_ok=True)
    prefix = root / 'prefix'
    for gate in ('G2', 'GQ'):
        comparison = prefix / f'{gate}_comparison.json'
        if not comparison.is_file():
            run_gate(root, gate, [ITEM / 'run_prefix_fp1.py', build, gate, '--corrected', '--output', prefix])
        assert cut.read(comparison)['passed']
        status = cut.read(prefix / 'cases' / gate / 'status.json')
        progress(f'{gate} passed on build{label} at {status["seconds_until_stop"]:.3f} s.')
    bl = g3(root / 'g3', build, label)
    progress(f'G3 passed {bl["accepted_splits"]} splits with {bl["common_edit_count"]} identical shared edits.')
    two_d = g4(root / 'g4', build, label)
    saved = saved_safeguards(root / 'safeguards', build, label)
    guards = {'passed': True, 'build': label, 'safeguards': saved, 'G3': bl, 'G4': two_d}
    seal(root / 'safeguards.json', guards)
    scoped = {'passed': True, 'build': label, 'binary_sha256': receipt['binary_sha256'],
              'G1': [row['records'] for row in g1], 'G2': str(prefix / 'G2_comparison.json'),
              'GQ': str(prefix / 'GQ_comparison.json'), 'G3': bl, 'G4': two_d}
    seal(root / 'scoped.json', scoped)
    progress('All scoped checks and standing safeguards passed. Next is G5 OFF followed by the two ON runs.')
    print(root, flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('g1', type=Path)
    args = parser.parse_args()
    run(args.build.resolve(), args.g1.resolve())
