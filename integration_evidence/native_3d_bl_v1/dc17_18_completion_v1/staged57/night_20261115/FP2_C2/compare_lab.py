"""Compare C2 repeats and report the historical and continuation observations."""

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
            estimate = STEP0_ESTIMATE.get(receipt['case'])
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
                'step0_estimate_pass': estimate[0] if estimate else None,
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
    assert build not in (BASELINE, run.CONTINUATION_BASE)
    continuation_sources = run.continuation_sources()
    continuation_path, continuation = load_trial(run.CONTINUATION_BASE, 'N1', 1)
    assert not continuation['passed'] and continuation_path == continuation_sources[-1]
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
        if name == 'N1':
            paths.append(continuation_path)
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
        if name == 'N1':
            labels = (*labels, 'build652')
            receipts = (*receipts, continuation)
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
                **{key: int(measure['proof'][key]) if key in measure['proof'] else None
                   for key in run.PROOF_COUNTS},
                'seconds_classify': float(measure['proof']['seconds_classify']),
                'entity_faces': [{key: int(row[key]) for key in ('body', 'sheet', 'marker', 'faces')}
                                 for row in measure.get('entity_faces', ())] or 'not available',
                'diagnostics': {key: records for rows in receipt['rows_by_rank'].values()
                                for key in run.DIAGNOSTIC_ROWS if (records := rows.get(key))},
            }
        if name == 'N1':
            details['build653'] = {'entity_faces': 'not available'}
        comparison[name] = {'trials': details, 'step0_one_sweep': first_sweep_estimate(name, rows),
                            'slowest_rank_per_pass': slowest_passes(rows)}
    reports = HERE / 'reports'
    reports.mkdir(exist_ok=True)
    path = reports / f'FP2_C2_lab_{build}.json'
    baseline = {'baseline_build': BASELINE, 'continuation_baseline_build': run.CONTINUATION_BASE,
                'continuation_baseline': {'N1': str(continuation_path), 'N6': None, 'N6P': None}}
    a.sealed(path, {**baseline, 'build': build, 'cases': comparison, 'per_pass': report})
    artifacts[str(path)] = a.sha(path)
    result = {**fingerprints, 'passed': all(row['passed'] for row in summaries.values()),
              **baseline, 'cases': summaries, 'artifacts': artifacts, 'report': str(path),
              'compared_rows': ROW_NAMES}
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
    for name in run.RUNS:
        path, receipt = compare_lab_c1.load_trial(BASELINE, name, 1)
        assert receipt['passed'], path
        assert list(report_rows(receipt, 'build647'))
    continuation_path, continuation = load_trial(run.CONTINUATION_BASE, 'N1', 1)
    assert continuation_path == run.continuation_sources()[-1] and not continuation['passed']
    row, = report_rows(continuation, 'build652')
    assert sum(row['collapses_by_sweep']) == 5937 and row['seconds_c_bound'] is None
    assert set(row['query_budget'].values()) == {None}
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
