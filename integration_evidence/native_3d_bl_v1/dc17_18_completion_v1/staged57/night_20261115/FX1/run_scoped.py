"""Run the approved FX1 OFF prefix and saved-state checks."""

from argparse import ArgumentParser
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
FP1 = HERE.parent / 'FP1'
sys.path[:0] = [str(FP1), str(FP1.parent / 'N18')]
import run_prefix_fp1 as prefix
import run_resume_scoped as scoped
from run_g1 import cut, verify_build
import runpy
FX1 = runpy.run_path(str(HERE / 'compare_prefix.py'))
compare_cases, metric_gate, self_check = (FX1[name] for name in ('compare_cases', 'metric_gate', 'self_check'))


def progress(line):
    with (FP1.parent / 'progress.md').open('a') as out:
        out.write(f'\nFX1 M. {line}\n')


def run(build):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    receipt = cut.read(build / 'build_status.json')
    _, pins = verify_build(receipt, build / 'source_pins.json')
    label = receipt['build']
    scoped.legacy_build_receipts(build, receipt)
    records = []
    for cohort, count in zip(('serial', 'mpi2', 'mpi4'), (375, 114, 228), strict=True):
        case = HERE / f'G1_{label}' / cohort
        result = cut.read(case / 'comparison.json')
        assert result['identity_pass'] and result['records'] == count
        assert cut.read(case / 'pins.json')['test_binary'] == receipt['test_binary_sha256']
        records.append(count)

    root = HERE / 'scoped'
    root.mkdir(exist_ok=True)
    captured = root / 'prefix'
    self_check()
    # The pinned capture and provenance checks are unchanged. FX1 checks the adopted Resume 6 calls.
    prefix.compare_cases = compare_cases
    for name in ('G2', 'GQ'):
        comparison = captured / f'{name}_comparison.json'
        if not comparison.is_file():
            assert prefix.run(build, name, captured, corrected=True)
            status = cut.read(captured / 'cases' / name / 'status.json')
            progress(f'{name} passed the corrected comparator at {status["seconds_until_stop"]:.3f} s.')
        assert cut.read(comparison)['passed']
        source = cut.read(captured / 'cases' / name / 'provenance.json')
        assert source['source_pins_sha256'] == pins and source['binary_sha256'] == receipt['binary_sha256']
        reference = prefix.G2_REFERENCE if name == 'G2' else prefix.GQ_REFERENCE
        metric = metric_gate(reference, captured / 'cases' / name, 1)
        scoped.seal(captured / f'{name}_metric_identity.json', metric)
        assert metric['passed'], metric['first_difference']
        progress(f'{name} initial metric SHA256 and gradation rows match the reference.')
    g3 = scoped.g3(root / 'g3', build, label)
    progress(f'G3 passed with {g3["accepted_splits"]} splits and {g3["common_edit_count"]} identical shared edits.')
    g4 = scoped.g4(root / 'g4', build, label)
    progress('G4 passed at 1, 2 and 4 ranks.')
    safeguards = scoped.saved_safeguards(root / 'safeguards', build, label)
    scoped.seal(root / 'safeguards.json',
                {'passed': True, 'build': label, 'G3': g3, 'G4': g4, 'safeguards': safeguards})
    scoped.seal(root / 'scoped.json',
                {'passed': True, 'build': label, 'binary_sha256': receipt['binary_sha256'], 'G1': records,
                 'G2': str(captured / 'G2_comparison.json'), 'GQ': str(captured / 'GQ_comparison.json'), 'G3': g3, 'G4': g4})
    progress('Saved-state safeguards passed. The Part M checkpoint is next, with G5 pending.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    run(parser.parse_args().build.resolve())
