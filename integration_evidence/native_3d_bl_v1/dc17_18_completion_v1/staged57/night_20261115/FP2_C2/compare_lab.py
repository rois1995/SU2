"""Compare the C2 repeat trials and report their observations beside build647."""

from argparse import ArgumentParser
import json
import sys

import run_c2_lab as run

sys.path.insert(0, str(run.C1.parent))
from FP2_C1 import compare_lab as compare_lab_c1

HERE, lab, a = run.HERE, run.lab, run.a
BASELINE = 647
ROW_NAMES = (*compare_lab_c1.ROW_NAMES, 'FP2_A LAB_S_ENTITY', 'FP2_C collapse_probe')
STEP0_ESTIMATE = {'N1': (1, 1920), 'N6P': (2, 2340)}


def load_trial(build, name, repeat):
    case = run.case_path(build, name, repeat)
    path = case / 'measurement.json'
    watchdog = case / 'watchdog.json'
    if watchdog.is_file() and a.cut.read(watchdog)['status'] == 'retry_allowed':
        retry = run.case_path(build, name, repeat, watchdog_retry=True)
        if (retry / 'measurement.json').is_file():
            assert a.cut.read(watchdog)['measurement_sha256'] == a.sha(path), path
            case, path = retry, retry / 'measurement.json'
    row = a.cut.read(path)
    assert row['build'] == build and row['case'] == name and row['ranks'] == run.RUNS[name][0], path
    assert row['wait']['terminal'] and a.cut.terminal(row['wait']['controller']), path
    assert row['wait'] == a.cut.read(case / 'test_wait.json'), path
    processes = a.cut.read(case / 'owned_processes.json')
    assert processes['all_observed_terminal'] and all(a.cut.terminal(identity) for identity in processes['identities'])
    lab.archived.verify_artifacts(case, row['artifacts'])
    return path, row


def applies(name, marker):
    if marker == 'FP2_A LAB_S_ENTITY':
        return name == 'N1'
    return compare_lab_c1.applies(name, marker)


def integer_fields(row):
    values = compare_lab_c1.integer_fields(row)
    if 'c_changes' in row:
        values['c_changes'] = run.changes(row)
    return values


def repeat_differences(name, reference, candidate):
    markers = [marker for marker in ROW_NAMES if applies(name, marker)]
    return compare_lab_c1.compare_rows(
        reference, candidate, markers, lambda data, marker: data.get(compare_lab_c1.row_key(marker), ()),
        lambda marker, row: integer_fields(row))


def report_rows(receipt, trial):
    at = run.c3.c2.fields_for_pass
    for rank, rows in receipt['rows_by_rank'].items():
        end = rows['end'][0]
        for surface in rows['NATIVE3D_LOCAL_SURFACE']:
            phase = int(surface['pass'])
            bad = at(rows, 'bad', phase)
            before_v = at(rows, 'bad_before_v', phase)
            passed = at(rows, 'pass', phase)
            estimate = STEP0_ESTIMATE.get(receipt['case'])
            yield {
                'case': receipt['case'], 'build': receipt['build'], 'trial': trial, 'rank': int(rank), 'pass': phase,
                'faces_before_c': int(surface.get('faces_before_c', surface['faces_out'])),
                'faces_after_c': int(surface['faces_out']),
                'points_before_c': int(surface.get('points_before_c', surface['points_out'])),
                'points_after_c': int(surface['points_out']),
                'collapses_by_sweep': run.changes(surface),
                'refused': {key.removeprefix('c_refused_'): int(surface.get(key, 0)) for key in run.REFUSALS},
                'reverse_max': float(surface.get('c_reverse_max', 0)),
                'seconds_c': float(surface.get('seconds_c', 0)),
                'seconds_c_samples': float(surface.get('seconds_c_samples', 0)),
                'seconds_c_entity': float(surface.get('seconds_c_entity', 0)),
                'bad_before_v': {key: int(value) for key, value in before_v.items() if key.startswith('bad')},
                'bad_after_v': {key: int(value) for key, value in bad.items() if key.startswith('bad')},
                'cells_in': int(passed['cells_in']), 'cells_out': int(passed['cells_out']),
                'modeled_peak_through_pass': int(passed['modeled_peak']) if 'modeled_peak' in passed else None,
                'modeled_peak_final': int(end['modeled_peak']),
                'collapse_probe': at(rows, 'collapse_probe', phase),
                'edge_bins': run.c3.c2.measured_edge(rows, 'out', phase),
                'step0_estimate_pass': estimate[0] if estimate else None,
            }


def first_sweep_estimate(name, rows):
    if name not in STEP0_ESTIMATE:
        return {}
    phase, estimate = STEP0_ESTIMATE[name]
    observed = {trial: sum((row['collapses_by_sweep'] or [0])[0] for row in rows
                           if row['pass'] == phase and row['trial'] == trial)
                for trial in ('repeat1', 'repeat2')}
    return {'pass': phase, 'sweep': 1, 'estimate': estimate, 'observed': observed}


def compare_case(name, first, second):
    differences = repeat_differences(name, first['rows_by_rank'], second['rows_by_rank'])
    equality = compare_lab_c1.comparison(differences)
    status = {label: trial['passed'] for label, trial in (('repeat1', first), ('repeat2', second))}
    return {'case': name, 'passed': equality['passed'] and all(status.values()), 'trial_passed': status,
            'first_difference': equality['first_difference'], 'repeat': equality}


def run_campaign(build):
    assert build != BASELINE
    artifacts, fingerprints, summaries, report, comparison = {}, None, {}, [], {}
    for name in run.RUNS:
        reference_path, reference = compare_lab_c1.load_trial(BASELINE, name, 1)
        assert reference['passed'], reference_path
        trials = [load_trial(build, name, repeat) for repeat in (1, 2)]
        first, second = (receipt for _, receipt in trials)
        for trial in (first, second):
            pins = {key: trial[key] for key in lab.FINGERPRINTS}
            if fingerprints is None:
                fingerprints = pins
            assert pins == fingerprints, name
        result = compare_case(name, first, second)
        paths = [reference_path, *(path for path, _ in trials)]
        measured = {str(path): a.sha(path) for path in paths}
        result.update(artifacts=measured, compared_rows=[marker for marker in ROW_NAMES if applies(name, marker)])
        path = run.case_path(build, name, 1) / 'comparison.json'
        a.sealed(path, result)
        artifacts.update(measured)
        artifacts[str(path)] = a.sha(path)
        summaries[name] = {'passed': result['passed'], 'first_difference': result['first_difference'],
                           'receipt': str(path)}
        labels = ('build647', 'repeat1', 'repeat2')
        receipts = (reference, first, second)
        rows = [row for label, receipt in zip(labels, receipts, strict=True) for row in report_rows(receipt, label)]
        report.extend(rows)
        details = {}
        for label, receipt in zip(labels, receipts, strict=True):
            measure = receipt['measurement']
            state = receipt['rows_by_rank']['0']['state'][0]
            details[label] = {
                'final_faces': int(state['faces']), 'final_points': int(state['points']),
                'owned_cells': measure['totals']['final_cells'],
                'bad_after_v': measure['totals']['handover_bad'],
                'bad_by_class': measure['bad_by_class'],
                'classify_above_tol': int(measure['proof']['classify_above_tol']),
                'classify_unexplained': int(measure['proof']['classify_unexplained']),
            }
        comparison[name] = {'trials': details, 'step0_one_sweep': first_sweep_estimate(name, rows)}
    reports = HERE / 'reports'
    reports.mkdir(exist_ok=True)
    path = reports / f'FP2_C2_lab_{build}.json'
    a.sealed(path, {'baseline_build': BASELINE, 'build': build, 'cases': comparison, 'per_pass': report})
    artifacts[str(path)] = a.sha(path)
    result = {**fingerprints, 'passed': all(row['passed'] for row in summaries.values()),
              'baseline_build': BASELINE, 'cases': summaries, 'artifacts': artifacts, 'report': str(path),
              'compared_rows': ROW_NAMES}
    a.sealed(run.case_path(build, 'N1', 1).parent / 'lab_acceptance.json', result)
    return result


def self_check():
    old = {'c_changes': '"1,0"', 'c_sweeps': '2', 'seconds_c': '2.5', 'c_refused_topology': '30'}
    new = {**old, 'seconds_c': '9.0'}
    assert integer_fields(old) == integer_fields(new)
    new['c_changes'] = '"0,1"'
    assert next(compare_lab_c1.field_differences(integer_fields(old), integer_fields(new)))['field'] == 'c_changes'
    assert run.changes({}) == []
    assert len(run.REFUSALS) == 15
    for name in run.RUNS:
        path, receipt = compare_lab_c1.load_trial(BASELINE, name, 1)
        assert receipt['passed'], path
        assert list(report_rows(receipt, 'build647'))
    print('FP2-C2 lab comparator self-check passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return 0
    if args.build is None:
        parser.error('candidate build number is required')
    result = run_campaign(args.build)
    print(json.dumps({key: result[key] for key in ('passed', 'cases', 'report')}, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
