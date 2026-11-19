"""Run the adopted G1 identity cohorts on a sealed FP1 test build."""

from argparse import ArgumentParser
import os
from pathlib import Path
import runpy
import sys

ITEM = Path(__file__).resolve().parent
sys.path.insert(0, str(ITEM.parent / 'FP0'))
from run_fp0 import PARSER, REF, cut, run_case, sha

MANIFEST = REF / 'G1_shared_reference_manifest.json'
SELECTION = REF / 'G1_actual_selection.json'


def verify_build(receipt, source_pins):
    succeeded = receipt.get('passed', receipt.get('pins_verified') and receipt.get('exit_code') == 0)
    if receipt['state'] != 'terminal' or not succeeded:
        raise RuntimeError('G1 requires a successful terminal build receipt.')
    source_sha = sha(source_pins)
    if receipt.get('source_pins_sha256', source_sha) != source_sha:
        raise RuntimeError('Build source manifest differs from its receipt.')
    for path, digest in cut.read(source_pins).items():
        if sha(Path(path)) != digest:
            raise RuntimeError(f'Build source pin changed: {path}')
    binary = Path(receipt['test_binary'])
    if sha(binary) != receipt['test_binary_sha256']:
        raise RuntimeError(f'Build test driver pin changed: {binary}')
    return binary, source_sha


def run_cohort(case, cohort, receipt, source_pins, selection, limits, identity):
    reference_path = REF / f'G1_candidate_{cohort}.json'
    reference_sha = sha(reference_path)
    if reference_sha != limits['report_sha256']:
        raise RuntimeError(f'Adopted G1 report changed: {reference_path}')
    binary, source_sha = verify_build(receipt, source_pins)
    ranks = limits['ranks']
    selector = selection['serial_filter' if ranks == 1 else 'mpi_filter']
    pins = {'test_binary': receipt['test_binary_sha256'], 'test_source_pins': source_sha,
            'reference': reference_sha, 'parser': sha(PARSER),
            'manifest': sha(MANIFEST), 'selection': sha(SELECTION)}
    if (case / 'status.json').is_file():
        if pins != cut.read(case / 'pins.json'):
            raise RuntimeError(f'G1 receipt belongs to a different build or reference: {case}')
        wait = cut.read(case / 'test_wait.json')
    else:
        wait = run_case(case, binary, ranks, selector, limits['wrapper_seconds'], xml=True)
        verify_build(receipt, source_pins)
        (case / 'run.log').symlink_to('test_outer.log')
        cut.save(case / 'pins.json', pins)
        cut.save(case / 'status.json', {
            'state': 'terminal', 'pins_verified': True, 'seconds': wait['elapsed_seconds'],
            'exit_code': wait['returncode'],
        })
    if not wait['terminal'] or not cut.terminal(wait['controller']):
        raise RuntimeError(f'G1 test is still active: {case}')
    candidate = identity['parse_run'](case, ranks, limits['cases_per_rank'])
    result = identity['compare'](cut.read(reference_path), candidate, set())
    result.update(records=len(candidate['case_results']), candidate_returncode=wait['returncode'],
                  reference_returncode=limits['process_returncode'],
                  elapsed_seconds=wait['elapsed_seconds'], candidate_sources=candidate['source_logs'])
    identity['write_json'](case / 'comparison.json', result)
    if not result['identity_pass'] or result['candidate_returncode'] != result['reference_returncode']:
        raise RuntimeError(f'G1 {cohort} differs from the adopted reference. See {case / "comparison.json"}')
    print(f'G1 {cohort}: {result["records"]} case records matched in {wait["elapsed_seconds"]:.3f} s.',
          flush=True)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('case_root', type=Path)
    parser.add_argument('build_receipt', type=Path)
    parser.add_argument('source_pins', type=Path)
    args = parser.parse_args()
    if Path('/proc/1/comm').read_text().strip() != 'systemd':
        raise RuntimeError('Run G1 on the actual host for its resource and process receipts.')
    for name in list(os.environ):
        if name.startswith('SU2_NATIVE_'):
            del os.environ[name]
    manifest = cut.read(MANIFEST)
    selection = cut.read(SELECTION)
    receipt = cut.read(args.build_receipt)
    identity = runpy.run_path(str(PARSER))
    identity['self_check']()
    root = args.case_root.resolve()
    for cohort in ('serial', 'mpi2', 'mpi4'):
        run_cohort(root / cohort, cohort, receipt, args.source_pins, selection,
                   manifest['cohorts'][cohort], identity)


if __name__ == '__main__':
    main()
