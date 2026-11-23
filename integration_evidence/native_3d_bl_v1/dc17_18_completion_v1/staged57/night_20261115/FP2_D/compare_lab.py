"""Compare FP2-D repeats, seal the invariant and evaluate the engine hand-over alarm."""

from argparse import ArgumentParser
import json
import sys

import run_c2_lab as run

sys.path.insert(0, str(run.C1.parent))
from FP2_C1 import compare_lab as compare_lab_c1

HERE, lab, a = run.HERE, run.lab, run.a
BASELINE = run.S0_BUILD
ROW_NAMES = (*compare_lab_c1.ROW_NAMES, 'FP2_A LAB_S_ENTITY', 'FP2_C collapse_probe')


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
    values.pop('strict_mismatch', None)
    if 'c_changes' in row:
        values['c_changes'] = run.changes(row)
    return values


def repeat_differences(name, reference, candidate):
    markers = [marker for marker in ROW_NAMES if applies(name, marker)]
    return compare_lab_c1.compare_rows(
        reference, candidate, markers, lambda data, marker: data.get(compare_lab_c1.row_key(marker), ()),
        lambda marker, row: integer_fields(row))


def sweep_observations(surface, rows):
    return [
        {'sweep_number': sweep, 'accepted_collapses': accepted,
         'bound_refusals_by_entity': [
             {key: int(row[key]) for key in ('sweep', 'body', 'sheet', 'marker', 'count')}
             for row in rows if row['pass'] == surface['pass'] and row['sweep_number'] == str(sweep)
         ]}
        for sweep, accepted in enumerate(run.changes(surface), 1)
    ]


def report_rows(receipt, trial):
    at = run.c3.c2.fields_for_pass
    for rank, rows in receipt['rows_by_rank'].items():
        end = rows['end'][0]
        proof, = rows.get('LAB_S_PROOF', rows.get('proof', ())) or ({},)
        observation, = rows.get('strict_observation', ()) or ({},)
        last_pass = max(int(row['pass']) for row in rows['NATIVE3D_LOCAL_SURFACE'])
        for surface in rows['NATIVE3D_LOCAL_SURFACE']:
            phase = int(surface['pass'])
            bad = at(rows, 'bad', phase)
            before_v = at(rows, 'bad_before_v', phase)
            passed = at(rows, 'pass', phase)
            query_budget = at(rows, 'query_budget', phase) or {}
            yield {
                'case': receipt['case'], 'build': receipt['build'], 'trial': trial, 'rank': int(rank), 'pass': phase,
                'faces_before_c': int(surface.get('faces_before_c', surface['faces_out'])),
                'faces_after_c': int(surface['faces_out']),
                'points_before_c': int(surface.get('points_before_c', surface['points_out'])),
                'points_after_c': int(surface['points_out']),
                'collapses_by_sweep': run.changes(surface),
                'sweep_observations': sweep_observations(surface, rows.get('bound_refusal', ()))
                if 'bound_refusal' in receipt['measurement'] else 'not available',
                'refused': {key.removeprefix('c_refused_'): int(surface[key]) if key in surface else None
                            for key in run.REFUSALS},
                'reverse_max': float(surface.get('c_reverse_max', 0)),
                'seconds_c': float(surface.get('seconds_c', 0)),
                'seconds_c_samples': float(surface.get('seconds_c_samples', 0)),
                'seconds_c_entity': float(surface.get('seconds_c_entity', 0)),
                'seconds_c_bound': float(surface['seconds_c_bound']) if 'seconds_c_bound' in surface else None,
                'c_bound_queries': int(surface['c_bound_queries']) if 'c_bound_queries' in surface else None,
                'seconds_pass': float(passed['seconds_pass']),
                'bad_before_v': {key: int(value) for key, value in before_v.items() if key.startswith('bad')},
                'bad_after_v': {key: int(value) for key, value in bad.items() if key.startswith('bad')},
                'cells_in': int(passed['cells_in']), 'cells_out': int(passed['cells_out']),
                'modeled_peak_through_pass': int(passed['modeled_peak']) if 'modeled_peak' in passed else None,
                'modeled_peak_final': int(end['modeled_peak']),
                'query_budget': {key: int(query_budget[key]) if key in query_budget else None
                                 for key in run.BUDGET_FIELDS},
                'query_budget_over_ceiling': int(query_budget['conservative_sum']) > int(query_budget['ceiling'])
                if query_budget else None,
                'collapse_probe': at(rows, 'collapse_probe', phase),
                'edge_bins': run.c3.c2.measured_edge(rows, 'out', phase),
                'surface': {key: int(surface[key]) if key in surface else 'not available'
                            for key in ('splits_distance', 'sweeps_distance', 'path_splits', 'path_max',
                                        'waits_neighbor', 'refused_near', 'collapses_surface',
                                        *sorted(field for field in surface if field.startswith('left_')),
                                        *run.D_FIELDS)},
                'physical_geometry': at(rows, 'altitude', phase) or 'not available',
                **({'strict_observation': observation or None, 'proof': proof or None} if phase == last_pass else {}),
            }


def slowest_passes(rows):
    groups = {(row['trial'], row['pass']) for row in rows}
    result = []
    for trial, phase in sorted(groups):
        group = [row for row in rows if row['trial'] == trial and row['pass'] == phase]
        slowest = max(group, key=lambda row: (row['seconds_pass'], -row['rank']))
        result.append({key: slowest[key] for key in ('trial', 'pass', 'rank', 'seconds_pass')})
    return result


def compare_case(name, first, second):
    differences = repeat_differences(name, first['rows_by_rank'], second['rows_by_rank'])
    equality = compare_lab_c1.comparison(differences)
    status = {label: trial['passed'] for label, trial in (('repeat1', first), ('repeat2', second))}
    return {'case': name, 'passed': equality['passed'] and all(status.values()), 'trial_passed': status,
            'first_difference': equality['first_difference'], 'repeat': equality}


def alarm_values(receipt):
    data = receipt['rows_by_rank']
    if set(data) != {str(rank) for rank in range(6)}:
        raise ValueError('N6P requires six ranks')
    counts = [int(next(row['left_distance'] for row in rows['NATIVE3D_LOCAL_SURFACE'] if row['pass'] == '2'))
              for rows in data.values()]
    proof, = data['0']['proof']
    above = int(proof['classify_above_tol'])
    if above < 0 or any(count < 0 for count in counts):
        raise ValueError('N6P alarm counts must be nonnegative')
    return {'classify_above_tol': above, 'left_distance': sum(counts)}


def alarm(reference, trials):
    try:
        values = alarm_values(reference)
    except (KeyError, ValueError, StopIteration):
        return {'status': 'alarm not evaluated', 'passed': False}
    limit = {key: max(1.5 * value, value + 25) for key, value in values.items()}
    observed = [alarm_values(trial) for trial in trials]
    passed = all(row[key] <= maximum for row in observed for key, maximum in limit.items())
    return {'status': 'passed' if passed else 'alarm hit', 'passed': passed,
            'reference': values, 'limit': limit, 'observed': observed}


def run_campaign(build):
    assert build != BASELINE
    references = run.reference_trials()
    artifacts = {str(run.S0_LAB): a.sha(run.S0_LAB)}
    fingerprints, results, report = None, {}, []
    for name, (reference_path, reference) in references.items():
        trials = [load_trial(build, name, repeat) for repeat in (1, 2)]
        first, second = (receipt for _, receipt in trials)
        for _, trial in trials:
            pins = {key: trial[key] for key in lab.FINGERPRINTS}
            if fingerprints is None:
                fingerprints = pins
            assert pins == fingerprints, name
        result = compare_case(name, first, second)
        paths = [reference_path, *(path for path, _ in trials)]
        result['artifacts'] = {str(path): a.sha(path) for path in paths}
        path = run.case_path(build, name, 1) / 'comparison.json'
        a.sealed(path, result)
        artifacts.update(result['artifacts'])
        artifacts[str(path)] = a.sha(path)
        results[name] = {'passed': result['passed'], 'first_difference': result['first_difference'],
                         'receipt': str(path)}
        for label, receipt in zip(('S0', 'repeat1', 'repeat2'), (reference, first, second), strict=True):
            report.extend(report_rows(receipt, label))
        if name == 'N1':
            n1_first = first
        if name == 'N6P':
            alarm_result = alarm(reference, (first, second)) if result['passed'] else {
                'status': 'alarm not evaluated', 'passed': False}

    path, invariant = load_trial(build, 'N1Invariant', 1)
    assert all(invariant[key] == value for key, value in fingerprints.items()), path
    artifacts[str(path)] = a.sha(path)
    invariant_rows = invariant['rows_by_rank']['0']
    expected, = n1_first['rows_by_rank']['0']['NATIVE3D_LOCAL_SURFACE']
    observed, = invariant_rows['NATIVE3D_LOCAL_SURFACE']
    differences = list(compare_lab_c1.field_differences(integer_fields(expected), integer_fields(observed)))
    invariant_result = {'passed': invariant['passed'] and not differences, 'receipt': str(path),
                        'first_difference': next(iter(differences), None), 'rows': invariant_rows['invariant']}
    reports = HERE / 'reports'
    reports.mkdir(exist_ok=True)
    path = reports / f'FP2_D_lab_{build}.json'
    a.sealed(path, {'build': build, 'baseline_build': BASELINE, 'per_pass': report,
                    'wing_regions': [row for row in invariant_rows.get('region', ()) if 'bin' in row],
                    'wing_stations': invariant_rows.get('station', ()), 'alarm': alarm_result})
    artifacts[str(path)] = a.sha(path)
    result = {**fingerprints, 'passed': all(row['passed'] for row in results.values()) and invariant_result['passed'],
              'baseline_build': BASELINE, 'cases': results, 'invariant': invariant_result, 'alarm': alarm_result,
              'artifacts': artifacts, 'report': str(path), 'compared_rows': list(ROW_NAMES)}
    a.sealed(run.case_path(build, 'N1', 1).parent / 'lab_acceptance.json', result)
    return result


def self_check():
    old = {'c_changes': '"1,0"', 'c_sweeps': '2', 'seconds_c': '2.5', 'c_refused_topology': '30',
           'strict_mismatch': '2', 'strict_sink_mismatch': '0'}
    new = {**old, 'seconds_c': '9.0', 'strict_mismatch': '5'}
    assert integer_fields(old) == integer_fields(new)
    assert integer_fields({**new, 'strict_sink_mismatch': '1'}) != integer_fields(old)
    new['c_changes'] = '"0,1"'
    assert next(compare_lab_c1.field_differences(integer_fields(old), integer_fields(new)))['field'] == 'c_changes'
    assert run.changes({}) == []
    assert len(run.REFUSALS) == 16
    surface = {'pass': '2', 'c_changes': '"3,1"'}
    refusal = {'pass': '2', 'sweep': '34', 'sweep_number': '2', 'body': '0',
               'sheet': '10154', 'marker': '2', 'count': '6'}
    assert run.LINE.fullmatch('FP2_C bound_refusal ' + ' '.join(f'{key}={value}' for key, value in refusal.items()))
    assert run.LINE.fullmatch('FP2_C entity_faces rank=0 body=0 sheet=10154 marker=2 faces=6000')
    first, second = sweep_observations(surface, (refusal, {**refusal, 'pass': '1'}))
    assert first['accepted_collapses'] == 3 and first['bound_refusals_by_entity'] == []
    assert second['accepted_collapses'] == 1 and second['bound_refusals_by_entity'] == [
        {'sweep': 34, 'body': 0, 'sheet': 10154, 'marker': 2, 'count': 6}]
    assert not {'FP2_C entity_faces', 'FP2_C bound_refusal'}.intersection(ROW_NAMES)
    for _, receipt in run.reference_trials().values():
        assert list(report_rows(receipt, 'S0'))
    assert run.LINE.fullmatch('FP2_D altitude rank=0 pass=1 faces=3 altitude=0.002 limit=0.001')
    assert alarm({}, ())['status'] == 'alarm not evaluated'
    print('FP2-D lab comparator self-check passed.')

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
    print(json.dumps({key: result[key] for key in ('passed', 'cases', 'invariant', 'alarm', 'report')}, indent=2))
    return 1 if not result['passed'] else 0 if result['alarm']['passed'] else 75


if __name__ == '__main__':
    raise SystemExit(main())
