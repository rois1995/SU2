"""Run and seal one FP2-C part 1 hand-over trial on the frozen input."""

from argparse import ArgumentParser
import json
from pathlib import Path
import re
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
DRIVER = Path(__file__).resolve()
HERE = DRIVER.parent
B = HERE.parent / 'FP2_B'
sys.path.insert(0, str(B))
import run_correction3_lab as c3

lab, a = c3.lab, c3.a
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_C1.md'
ORDER_SHA = '6029450b746aed4493a4790fe5cc6504835387b70c7fc658e3bd3279fd96f35d'
RUNS = {name: c3.RUNS[name] for name in ('N1', 'N6', 'N6P')}
LINE = re.compile(c3.LINE.pattern.replace('|FP2_A', '|FP2_C (?:core|cells)|FP2_A'))
CORE_FIELDS = frozenset('''
    rank pass cells_in cells_out points_in points_out surface_points border_points wall_near_points
    collapses splits edge_removals face_swaps moves deferred skipped_memory evaluated_collapse
    refused_valid refused_size refused_topology refused_shape refused_exact blocked short_out edges_out
    bad_in bad_out sweeps_collapse sweeps_split sweeps_quality rounds limit round_changes
    seconds_collapse seconds_split seconds_quality
'''.split())
WALL_FIELDS = frozenset('''
    collapses_wall_near refused_size_wall refused_shape_wall refused_topology_wall refused_valid_wall
    blocked_wall four_surface_cells short_free short_wall_near short_border short_surface
'''.split())


def case_path(build, name, repeat=1):
    root = HERE if repeat == 1 else HERE / 'repeat2'
    return root / 'cases' / f'build{build}' / name


def observations(name, rows_by_rank):
    expected_passes = [1] if RUNS[name][0] == 1 else [1, 2]
    problems = []
    for rank, rows in rows_by_rank.items():
        core = rows.get('core', ())
        if [int(row['pass']) for row in core] != expected_passes:
            problems.append({'row': 'FP2_C core', 'rank': rank, 'expected_passes': expected_passes})
        for row in core:
            missing = sorted(CORE_FIELDS - row.keys())
            extra = row.keys() & WALL_FIELDS
            if missing or (extra and extra != WALL_FIELDS):
                problems.append({'row': 'FP2_C core', 'rank': rank, 'pass': int(row['pass']),
                                 'missing': missing, 'wall_fields': sorted(extra)})
            if int(row['rank']) != rank:
                problems.append({'row': 'FP2_C core', 'rank': rank, 'observed': row['rank']})
        cells = rows.get('cells', ())
        end = rows.get('end', ())
        if (len(cells) != 1 or len(end) != 1 or cells[0]['rank'] != str(rank)
                or cells[0]['cells'] != end[0]['cells_out']
                or not re.fullmatch(r'[0-9]+|0x[0-9a-fA-F]+', cells[0]['digest'])):
            problems.append({'row': 'FP2_C cells', 'rank': rank, 'observed': cells})
    return problems


def evaluate(name, data, expected, reference):
    result = c3.evaluate(name, data, expected, reference)
    # Keep the historical alarm and shifted census visible alongside the ordered C1 gates.
    report_fields = ('classify_above_tol', 'shifted_lt_1e_2')
    result['previous_limits_observed'] = [row for row in result['failed_limits'] if row['field'] in report_fields]
    result['failed_limits'] = [row for row in result['failed_limits'] if row['field'] not in report_fields]
    result['problems'].extend(observations(name, data))
    if name == 'N6P' and result['valid']:
        # This limit now applies to N6P as well as the equal-partition experiments.
        for rank, rows in data.items():
            for row in rows['NATIVE3D_LOCAL_SURFACE']:
                count = int(row['sweeps_distance'])
                if not 0 <= count < 40:
                    result['failed_limits'].append({'rank': rank, 'pass': int(row['pass']),
                                                    'field': 'sweeps_distance',
                                                    'observed': count, 'maximum': 39})
    return result


def verify_launches_terminal():
    for parent in (HERE, HERE / 'repeat2'):
        for launch in sorted((parent / 'cases').glob('build*/*/test_launch.json')):
            a.resumed(launch.parent)
            processes = a.cut.read(launch.with_name('owned_processes.json'))
            assert processes['all_observed_terminal'], launch
            assert all(a.cut.terminal(row) for row in processes['identities']), launch


def run(build, name, repeat, verify_only):
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    label = a.cut.read(build / 'build_status.json')['build']
    case = case_path(label, name, repeat)
    reference_path, reference = c3.baseline(name)
    verify_launches_terminal()

    def prepare(directory):
        support = (DRIVER, ORDER, ORDER.with_suffix('.sha256'), c3.DRIVER, c3.c2.DRIVER,
                   c3.prior.DRIVER, c3.prior.OLD_LAB, reference_path)
        with patch.object(lab, 'HERE', B):
            receipt, binary, pins = c3.prior.BASELINE_PREPARE(directory)
        pins.update({str(path): a.sha(path) for path in support})
        return receipt, binary, pins

    with patch.multiple(lab, HERE=case.parents[2], RUNS=RUNS, LINE=LINE, __file__=str(DRIVER),
                        prepare=prepare,
                        evaluate=lambda name, data, expected: evaluate(name, data, expected, reference)):
        return lab.run(build, name, verify_only)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    method = parser.add_mutually_exclusive_group(required=True)
    method.add_argument('--only', choices=tuple(RUNS))
    method.add_argument('--owned', type=Path)
    parser.add_argument('--repeat', type=int, choices=(1, 2), default=1)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    if args.owned:
        with patch.object(lab, 'RUNS', RUNS):
            return lab.owned(args.owned)
    if args.build is None:
        parser.error('a build directory is required with --only')
    result = run(args.build.resolve(), args.only, args.repeat, args.verify_only)
    print(json.dumps({key: result[key] for key in ('case', 'passed', 'measurement')}, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
