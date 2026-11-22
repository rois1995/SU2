"""Compare the C1 repeated hand-over trials and the S0 pass-1 surface rows."""

from argparse import ArgumentParser
from hashlib import sha256
import json
from pathlib import Path
import re

import run_c1_lab as run

HERE, lab, a = run.HERE, run.lab, run.a
ROW_NAMES = (
    'NATIVE3D_LOCAL_SURFACE', 'FP2_B pass', 'FP2_B sweep', 'FP2_B bad_before_v', 'FP2_B bad', 'FP2_B state',
    'FP2_B donor_scan', 'FP2_B end', 'FP2_B partition', 'FP2_A LAB_S_PROOF', 'FP2_B proof', 'FP2_B proof_entity',
    'FP2_C core', 'FP2_C cells',
)
PARTITION = 'FP2_B partition'
SERIAL_PROOF = 'FP2_A LAB_S_PROOF'
MPI_PROOF = ('FP2_B proof', 'FP2_B proof_entity')
NEW_VOLUME_FIELDS = ('short_free', 'short_wall_near', 'short_border', 'short_surface', 'four_surface_cells')
WALL_COUNTERS = run.WALL_FIELDS - set(NEW_VOLUME_FIELDS)
PASS1_VOLUME_FIELDS = frozenset(('v_rounds', 'v_limit', 'v_collapses', 'v_splits', 'v_changes', 'bad_before_v'))
PASS1_SWEEPS = frozenset(('surface_length', 'surface_distance'))
INTEGER = re.compile(r'[+-]?[0-9]+')


def load_trial(build, name, repeat):
    case = run.case_path(build, name, repeat)
    path = case / 'measurement.json'
    row = a.cut.read(path)
    assert row['build'] == build and row['case'] == name and row['ranks'] == run.RUNS[name][0], path
    assert row['wait']['terminal'] and a.cut.terminal(row['wait']['controller']), path
    assert row['wait'] == a.cut.read(case / 'test_wait.json'), path
    lab.archived.verify_artifacts(case, row['artifacts'])
    return path, row


def row_key(marker):
    return marker.rsplit(' ', 1)[-1]


def applies(name, marker):
    if marker == PARTITION:
        return name == 'N6P'
    if marker in MPI_PROOF:
        return name != 'N1'
    if marker == SERIAL_PROOF:
        return name == 'N1'
    return True


def integer_fields(row):
    values = {}
    for key, value in row.items():
        if key.startswith('seconds') or 'rss' in key:
            continue
        if INTEGER.fullmatch(value):
            values[key] = int(value)
        elif key == 'digest' and value.startswith('0x'):
            values[key] = int(value, 16)
    return values


def field_differences(expected, observed):
    for key in sorted(expected.keys() | observed.keys()):
        old, new = expected.get(key), observed.get(key)
        if old != new:
            yield {'field': key, 'expected': old, 'observed': new}


def compare_rows(reference, candidate, names, rank_rows, project):
    problems = []
    if reference.keys() != candidate.keys():
        problems.append({'row': 'ranks', 'expected': sorted(reference), 'observed': sorted(candidate)})
    for marker in names:
        reference_count = observed_count = 0
        for rank in sorted(reference.keys() & candidate.keys(), key=int):
            before = rank_rows(reference[rank], marker)
            after = rank_rows(candidate[rank], marker)
            reference_count += len(before)
            observed_count += len(after)
            if len(before) != len(after):
                problems.append({'row': marker, 'rank': int(rank), 'field': 'row_count',
                                 'expected': len(before), 'observed': len(after)})
            for index, (old, new) in enumerate(zip(before, after), 1):
                problems.extend({'row': marker, 'rank': int(rank), 'index': index, **difference}
                                for difference in field_differences(project(marker, old), project(marker, new)))
        if not reference_count and not observed_count:
            problems.append({'row': marker, 'field': 'row_count', 'expected': 'positive', 'observed': 0})
    return problems


def repeat_differences(name, reference, candidate, names):
    markers = [marker for marker in names if applies(name, marker)]
    return compare_rows(reference, candidate, markers, lambda data, marker: data.get(row_key(marker), ()),
                        lambda marker, row: integer_fields(row))


def pass1_rows(data, marker):
    rows = [row for row in data.get(row_key(marker), ()) if row['pass'] == '1']
    if marker == 'FP2_B sweep':
        return [row for row in rows if row['kind'] in PASS1_SWEEPS]
    return rows


def pass1_fields(marker, row):
    # The sweep prints its elapsed clock as seconds= via PrintSweepFields.
    if marker == 'FP2_B sweep':
        return {key: value for key, value in row.items() if key != 'seconds'}
    return {key: value for key, value in row.items()
            if key not in PASS1_VOLUME_FIELDS and not key.startswith('seconds_')}


def pass1_differences(reference, candidate):
    return compare_rows(reference, candidate, ('NATIVE3D_LOCAL_SURFACE', 'FP2_B sweep'),
                        pass1_rows, pass1_fields)


def comparison(differences):
    return {'passed': not differences, 'first_difference': next(iter(differences), None),
            'differences': differences}


def schemas(reference, candidate):
    problems = []
    for label, receipt in [('S0', reference), *candidate]:
        differs = set() if label == 'S0' else run.WALL_FIELDS
        for rank, data in receipt['rows_by_rank'].items():
            for row in data.get('core', ()):
                fields = row.keys() & run.WALL_FIELDS
                if fields != differs:
                    problems.append({'trial': label, 'row': 'FP2_C core', 'rank': int(rank),
                                     'pass': int(row['pass']), 'expected': sorted(differs), 'observed': sorted(fields)})
                elif differs:
                    total = sum(int(row[key]) for key in NEW_VOLUME_FIELDS if key.startswith('short_'))
                    if total != int(row['short_out']):
                        problems.append({'trial': label, 'row': 'FP2_C core', 'rank': int(rank),
                                         'pass': int(row['pass']), 'short_class_sum': total,
                                         'short_out': int(row['short_out'])})
    return problems


def report_rows(receipt, trial):
    for rank, rows in receipt['rows_by_rank'].items():
        for core in rows.get('core', ()):
            phase = int(core['pass'])
            find = lambda kind: run.c3.c2.fields_for_pass(rows, kind, phase)
            surface = find('NATIVE3D_LOCAL_SURFACE')
            edge = run.c3.c2.measured_edge(rows, 'out', phase)
            yield {
                'case': receipt['case'], 'build': receipt['build'], 'trial': trial, 'rank': int(rank), 'pass': phase,
                'core': ({**dict.fromkeys(sorted(WALL_COUNTERS), '0'), **core} if trial == 'S0' else core),
                'bad_before_v': find('bad_before_v'), 'bad_after_v': find('bad'),
                'v': {key: value for key, value in surface.items()
                      if key.startswith('v_') or key in ('seconds_v', 'bad_before_v')},
                'edge': edge,
                'star_max': surface.get('star_max'), 'faces_in': surface.get('faces_in'),
                'faces_out': surface.get('faces_out'), 'modeled_peak': rows['end'][0]['modeled_peak'],
            }


def compare_case(name, trials, names):
    baseline, first, second = (trial['rows_by_rank'] for trial in trials)
    checks = {
        'repeat': comparison(repeat_differences(name, first, second, names)),
        'pass1_repeat1': comparison(pass1_differences(baseline, first)),
        'pass1_repeat2': comparison(pass1_differences(baseline, second)),
        'schemas': comparison(schemas(trials[0], [('repeat1', trials[1]), ('repeat2', trials[2])])),
    }
    first_difference = next(({'check': check, **row['first_difference']} for check, row in checks.items()
                             if not row['passed']), None)
    status = {trial: receipt['passed'] for trial, receipt in zip(('S0', 'repeat1', 'repeat2'), trials, strict=True)}
    return {'case': name, 'passed': first_difference is None and all(status.values()),
            'first_difference': first_difference, 'trial_passed': status, **checks}


def run_campaign(baseline, candidate, rows):
    assert baseline != candidate
    artifacts, fingerprints, cases, report = {}, None, {}, []
    for name in run.RUNS:
        paths, trials = zip(*(load_trial(build, name, repeat)
                              for build, repeat in ((baseline, 1), (candidate, 1), (candidate, 2))), strict=True)
        for candidate_trial in trials[1:]:
            observed = {key: candidate_trial[key] for key in lab.FINGERPRINTS}
            if fingerprints is None:
                fingerprints = observed
            assert observed == fingerprints, candidate_trial['case']
        result = compare_case(name, trials, rows)
        measurements = {str(path): a.sha(path) for path in paths}
        result.update(artifacts=measurements, compared_rows=[marker for marker in rows if applies(name, marker)])
        path = paths[1].with_name('comparison.json')
        a.sealed(path, result)
        artifacts.update(measurements)
        artifacts[str(path)] = a.sha(path)
        cases[name] = {'passed': result['passed'], 'first_difference': result['first_difference'],
                       'receipt': str(path)}
        for trial, receipt in zip(('S0', 'repeat1', 'repeat2'), trials, strict=True):
            report.extend(report_rows(receipt, trial))
    reports = HERE / 'reports'
    reports.mkdir(exist_ok=True)
    path = reports / f'FP2_C1_lab_{candidate}.json'
    a.sealed(path, {'baseline_build': baseline, 'build': candidate, 'per_pass': report})
    artifacts[str(path)] = a.sha(path)
    result = {**fingerprints, 'passed': all(row['passed'] for row in cases.values()),
              'baseline_build': baseline, 'cases': cases, 'artifacts': artifacts, 'report': str(path),
              'compared_rows': rows}
    a.sealed(HERE / 'cases' / f'build{candidate}' / 'lab_acceptance.json', result)
    return result


def self_check():
    # Use a representative pass-1 row with nondeterministic elapsed and independent V work.
    surface = {'pass': '1', 'rank': '0', 'faces': '8', 'seconds_place': '0.01',
               'v_rounds': '1', 'bad_before_v': '2', 'seconds_v': '0.1'}
    sweep = {'pass': '1', 'rank': '0', 'kind': 'surface_length', 'seconds': '1.2', 'splits': '3'}
    old = {'0': {'NATIVE3D_LOCAL_SURFACE': [surface], 'sweep': [sweep],
                 'cells': [{'rank': '0', 'cells': '1', 'digest': '14695981039346656037'}]}}
    new = {'0': {**old['0'], 'NATIVE3D_LOCAL_SURFACE': [{**surface, 'seconds_place': '0.02', 'v_rounds': '2'}],
                 'sweep': [{**sweep, 'seconds': '1.8'}]}}
    assert not pass1_differences(old, new)
    new['0']['sweep'][0]['splits'] = '4'
    assert pass1_differences(old, new)[0]['field'] == 'splits'
    assert not repeat_differences('N1', old, new, ['FP2_C cells'])
    new['0']['cells'] = [{'rank': '0', 'cells': '1', 'digest': '14695981039346656036'}]
    assert repeat_differences('N1', old, new, ['FP2_C cells'])[0]['field'] == 'digest'
    assert not repeat_differences('N1', old, new, ['FP2_B proof'])
    assert repeat_differences('N6', old, new, ['FP2_B proof'])[0]['field'] == 'row_count'
    digest = sha256(run.ORDER.read_bytes()).hexdigest()
    assert digest == run.ORDER_SHA
    print('FP2-C1 lab comparator self-check passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-build', type=int)
    parser.add_argument('--candidate-build', type=int)
    parser.add_argument('--rows', nargs='+', choices=('all', *ROW_NAMES),
                        help='Use all to select every ordered row')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return 0
    if None in (args.baseline_build, args.candidate_build, args.rows):
        parser.error('provide --baseline-build, --candidate-build and a row list')
    rows = list(ROW_NAMES) if args.rows == ['all'] else args.rows
    if len(rows) != len(ROW_NAMES) or set(rows) != set(ROW_NAMES):
        parser.error('lab acceptance requires every ordered row once')
    result = run_campaign(args.baseline_build, args.candidate_build, rows)
    print(json.dumps({key: result[key] for key in ('build', 'passed', 'cases', 'report')}, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
