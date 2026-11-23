"""Build and supervise the test-only C2 step 0 measurements."""

from argparse import ArgumentParser
import json
from pathlib import Path
import re
import subprocess
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
DRIVER = Path(__file__).resolve()
HERE = DRIVER.parent
C1 = HERE.parent / 'FP2_C1'
sys.path.insert(0, str(C1))
import audit_c1
import compare_lab as compare
import run_c1_lab as c1

lab, a = c1.lab, c1.a
import build_fp1

BASE = '4730bd59ec00e57a261e10b7e46e257c131a18c2'
BASE_BUILD = 647
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_C2_STEP0.md'
ORDER_SHA = '39927af9b81cccd8a2dcaf5be40ebe964edef0340bad75ff76bf40d55aea9c96'
SECONDS = 3600
RUNS = {'N1': (1, 'Serial'), 'N6P': (6, 'Production1')}
OBSERVATION = re.compile(r'^FP2_C2 (edge|direction|timing) (.*)$')
LOCATION = re.compile(r'(?<!\S)(rank|pass)=([0-9]+)(?=\s|$)')


def verify_order():
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]


def selector(name):
    return f'[.][NativeFP2C2{RUNS[name][1]}]'


def immutable(path):
    c1.c3.c2.committed(path, BASE)
    return path


def production():
    reference = a.cut.read(immutable(C1 / f'build{BASE_BUILD}/build_status.json'))
    manifest = immutable(C1 / f'build{BASE_BUILD}/source_pins.json')
    assert reference['passed'] and a.sha(manifest) == reference['source_pins_sha256']
    diff = subprocess.check_output(['git', 'diff', '--stat', BASE, '--', 'Common', 'SU2_CFD'],
                                   cwd=a.cut.REPO, text=True)
    production_pins = {path: digest for path, digest in a.cut.read(manifest).items()
                       if not Path(path).is_relative_to(a.cut.REPO / 'UnitTests')}
    differences = [path for path, digest in production_pins.items() if a.sha(Path(path)) != digest]
    solver = a.sha(Path(reference['binary']))
    result = {'base': BASE, 'base_build': BASE_BUILD, 'source_pins_sha256': a.sha(manifest),
              'checked_non_unit_test_paths': len(production_pins), 'differences': differences,
              'git_diff_stat': diff, 'solver_sha256': solver, 'expected_solver_sha256': reference['binary_sha256'],
              'passed': not differences and not diff and solver == reference['binary_sha256']}
    assert result['passed'], result
    return result


def reference_trial(name):
    campaign = immutable(C1 / f'cases/build{BASE_BUILD}/lab_acceptance.json')
    gate = a.cut.read(campaign)
    assert gate['passed'] and gate['cases'][name]['passed'], campaign
    path, receipt = compare.load_trial(BASE_BUILD, name, 1)
    immutable(path)
    assert receipt['passed'] and a.sha(path) == gate['artifacts'][str(path)], path
    assert all(receipt[key] == gate[key] for key in lab.FINGERPRINTS), path
    return (campaign, path), receipt


def audit_jobs():
    old = audit_c1.audit()
    current = audit_c1.audit(HERE)
    current['prior_c1_identities_checked'] = len(old['identities'])
    return current


def build_test_driver(label):
    verify_order()
    audit_jobs()
    before = production()
    reference_trial('N1')
    reference_trial('N6P')
    work = HERE / f'build{label}'
    ok = build_fp1.build(work, label, solver=False)
    after = production()
    a.sealed(work / 'production_build.json', {'before': before, 'after': after})
    assert ok, work / 'build_status.json'
    print(f'C2 build{label} test driver ready.', flush=True)


def measurement_rows(path, rank, passes):
    problems = []
    counts = {phase: dict.fromkeys(('edge', 'direction', 'timing'), 0) for phase in range(1, passes + 1)}
    after_cells = False
    with lab.archived.open_log(path) as stream:
        for number, raw in enumerate(stream, 1):
            if raw.startswith('FP2_C cells '):
                after_cells = True
            if not raw.startswith('FP2_C2 '):
                continue
            match = OBSERVATION.fullmatch(raw.rstrip('\r\n'))
            if not match:
                problems.append({'row': 'FP2_C2', 'rank': rank, 'line': number, 'issue': 'invalid marker'})
                continue
            kind, fields = match.groups()
            location = dict(LOCATION.findall(fields))
            phase = int(location.get('pass', '0'))
            if location.get('rank') != str(rank) or phase not in counts or counts[phase]['timing']:
                problems.append({'row': f'FP2_C2 {kind}', 'rank': rank, 'line': number, 'location': location})
                continue
            if kind == 'timing' and not after_cells:
                problems.append({'row': 'FP2_C2 timing', 'rank': rank, 'line': number,
                                 'issue': 'timing precedes the hand-over digest'})
            counts[phase][kind] += 1
    for phase, rows in counts.items():
        if rows['timing'] != 1:
            problems.append({'row': 'FP2_C2 timing', 'rank': rank, 'pass': phase, 'observed': rows['timing']})
    return counts, problems


def evaluate(case, name, data, reference):
    ranks, _ = RUNS[name]
    marked = {str(rank): rows for rank, rows in data.items()}
    compared = compare.repeat_differences(name, reference['rows_by_rank'], marked, compare.ROW_NAMES)
    counts, problems = {}, list(compared)
    for rank, path in lab.b0.log_paths(case, ranks).items():
        measured, issues = measurement_rows(path, rank, 1 if ranks == 1 else 2)
        counts[str(rank)] = {str(phase): rows for phase, rows in measured.items()}
        problems.extend(issues)
    return {'problems': problems, 'failed_limits': [], 'sanity': compare.comparison(compared),
            'compared_rows': [marker for marker in compare.ROW_NAMES if compare.applies(name, marker)],
            'measurement_rows': counts}


def run(build, name, verify_only=False):
    verify_order()
    audit_jobs()
    before = production()
    paths, reference = reference_trial(name)
    label = a.cut.read(build / 'build_status.json')['build']
    case = HERE / 'cases' / f'build{label}' / name

    def prepare(directory):
        with patch.object(lab, 'HERE', c1.B):
            receipt, binary, pins = c1.c3.prior.BASELINE_PREPARE(directory)
        assert receipt['binary_sha256'] == before['solver_sha256']
        support = (DRIVER, ORDER, ORDER.with_suffix('.sha256'), Path(c1.__file__), Path(compare.__file__),
                   Path(audit_c1.__file__), directory / 'production_build.json', *paths)
        pins.update({str(path): a.sha(path) for path in support})
        return receipt, binary, pins

    with patch.multiple(lab, HERE=HERE, RUNS=RUNS, LINE=c1.LINE, SECONDS=SECONDS, selector=selector,
                        __file__=str(DRIVER), prepare=prepare,
                        evaluate=lambda name, data, _: evaluate(case, name, data, reference)):
        receipt = lab.run(build, name, verify_only)
    after = production()
    a.sealed(case / 'production_run.json', {'before': before, 'after': after})
    return receipt


def self_check():
    verify_order()
    production()
    for name in RUNS:
        _, reference = reference_trial(name)
        assert not compare.repeat_differences(name, reference['rows_by_rank'], reference['rows_by_rank'],
                                              compare.ROW_NAMES)
    print('C2 order, production pins, and C1 sanity references verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path, nargs='?')
    method = parser.add_mutually_exclusive_group(required=True)
    method.add_argument('--build', type=int)
    method.add_argument('--only', choices=tuple(RUNS))
    method.add_argument('--owned', type=Path)
    method.add_argument('--audit', action='store_true')
    method.add_argument('--self-check', action='store_true')
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--record', type=Path, help='Save the terminal audit when using --audit.')
    args = parser.parse_args()
    if args.owned:
        with patch.multiple(lab, RUNS=RUNS, SECONDS=SECONDS, selector=selector):
            return lab.owned(args.owned.resolve())
    if args.build is not None:
        build_test_driver(args.build)
    elif args.audit:
        result = audit_jobs()
        if args.record:
            a.sealed(args.record.resolve(), result)
        print(f'{len(result["identities"])} C2 identities and '
              f'{result["prior_c1_identities_checked"]} C1 identities terminal. No live C2 workload matched.')
    elif args.self_check:
        self_check()
    else:
        if args.directory is None:
            parser.error('a build directory is required with --only')
        result = run(args.directory.resolve(), args.only, args.verify_only)
        print(json.dumps({key: result[key] for key in ('case', 'passed', 'measurement')}, indent=2))
        return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
