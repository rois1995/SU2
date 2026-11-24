"""Compare FP2-C3 repeats and seal the contact and embedding qualification."""

from argparse import ArgumentParser
import json
import sys

import run_c2_lab as run

sys.path.insert(0, str(run.C1.parent))
from FP2_C1 import compare_lab as compare_lab_c1

HERE, lab, a = run.HERE, run.lab, run.a
BASELINE = run.S0_BUILD
ROW_NAMES = (*compare_lab_c1.ROW_NAMES, 'FP2_A LAB_S_ENTITY', 'FP2_C collapse_probe', run.CONTACT)


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


def repeat_fields(marker, row):
    fields = integer_fields(row)
    if marker == run.CONTACT:
        fields.pop('peak_bytes', None)
        fields.update(kind=row['kind'], stage=row['stage'])
    return fields


def repeat_differences(name, reference, candidate):
    markers = [marker for marker in ROW_NAMES if applies(name, marker)]
    return compare_lab_c1.compare_rows(
        reference, candidate, markers, lambda data, marker: data.get(compare_lab_c1.row_key(marker), ()),
        repeat_fields)


def sweep_observations(surface, rows):
    return [
        {'sweep_number': sweep, 'accepted_collapses': accepted,
         'bound_refusals_by_entity': [
             {key: int(row[key]) for key in ('sweep', 'body', 'sheet', 'marker', 'count')}
             for row in rows if row['pass'] == surface['pass'] and row['sweep_number'] == str(sweep)
         ]}
        for sweep, accepted in enumerate(run.changes(surface), 1)
    ]


def contact_report(rows, phase, surface, passed):
    contact = [row for row in rows.get(run.CONTACT, ()) if row['pass'] == str(phase)]
    verify = next((row for row in contact if row['kind'] == 'verify'), None)
    embedding = next((row for row in rows.get('embedding', ()) if row['pass'] == str(phase)), None)
    collapses = int(surface['collapses_surface'])
    contact_seconds = float(verify['seconds_verify']) if verify else None
    verify_gather = float(verify['seconds_gather']) if verify else None
    # C3 samples seconds_pass after the collective check and subtracts test measurements.
    through_verify = float(passed['seconds_pass']) if verify else None
    return {
        'contact_rows': contact,
        'embedding': embedding,
        'contact_counters': {key: int(surface[key]) if key in surface else None
                             for key in (*run.CONTACT_COUNTERS, 'c_refused_contact_remote', 'c_refused_contact')},
        'contact_clocks': {key: float(surface[key]) if key in surface else None for key in run.CONTACT_CLOCKS},
        'seconds_per_accepted_collapse': {
            key: float(surface[key]) / collapses if collapses and key in surface else None
            for key in ('seconds_c_contact', 'seconds_c_bound', 'seconds_c')},
        'remote_refusals_over_outward_trials': [int(surface[key]) for key in
                                               ('c_refused_contact_remote', 'c_outward_tried')]
        if 'c_outward_tried' in surface else None,
        'verify_contact_seconds': verify_gather + contact_seconds if verify else None,
        'verify_check_seconds': contact_seconds,
        'seconds_through_verify': through_verify,
        'verify_share_through_verify': (verify_gather + contact_seconds) / through_verify if through_verify else None,
    }


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
                'points_removed_by_c': int(surface.get('points_before_c', surface['points_out'])) -
                                       int(surface['points_out']),
                **contact_report(rows, phase, surface, passed),
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
                'smallest_metric_edge': next((row for row in rows.get('edge', ()) if row['checkpoint'] == 'out'
                                              and int(row['pass']) == phase and row['order'] == '1'), None),
                'short_faces': [row for row in rows.get('SHORT_FACE', ()) if int(row['pass']) == phase],
                'surface': {key: int(surface[key]) if key in surface else 'not available'
                            for key in ('splits_distance', 'sweeps_distance', 'path_splits', 'path_max',
                                        'waits_neighbor', 'refused_near', 'collapses_surface',
                                        *sorted(field for field in surface if field.startswith('left_')),
                                        *run.D_FIELDS)},
                'physical_geometry': at(rows, 'altitude', phase) or 'not available',
                'distance_sweeps': [{key: int(row[key]) for key in
                                     ('sweep', 'candidates', 'splits', 'deferred', 'border_edges', 'refused_near',
                                      'refused_ineligible', 'refused_walk', 'refused_protected', 'refused_feature',
                                      'refused_trace', 'refused_duplicate', 'refused_outward', 'refused_entity',
                                      'refused_valid', 'refused_size', 'refused_topology', 'refused_memory')}
                                    for row in rows.get('sweep', ())
                                    if row['kind'] == 'surface_distance' and int(row['pass']) == phase],
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
    reports = HERE / 'reports'
    reports.mkdir(exist_ok=True)
    path = reports / f'FP2_C3_lab_{build}.json'
    checkpoints = run.verify_checkpoints(build, fingerprints)
    fixture = checkpoints[0]
    artifacts.update({str(item): a.sha(item) for item in checkpoints})
    totals = {}
    for row in report:
        key = row['case'] + ' ' + row['trial']
        total = totals.setdefault(key, dict.fromkeys(
            ('points_removed_by_c', 'c_refused_contact_remote', 'c_outward_tried'), 0))
        total['points_removed_by_c'] += row['points_removed_by_c']
        values = row['remote_refusals_over_outward_trials']
        for field, value in zip(('c_refused_contact_remote', 'c_outward_tried'), values or (None, None)):
            total[field] = None if value is None else total[field] + value
    baseline_s = run.D / 'reports' / f'FP2_D_{BASELINE}.json'
    run.c3.c2.committed(baseline_s, run.S0)
    baseline_events = a.cut.read(baseline_s)['runs'][f'build{BASELINE}']['events']
    artifacts[str(baseline_s)] = a.sha(baseline_s)
    engine_removals = {row['summary']['event']: row['engine_surface_removal'] for row in baseline_events}
    a.sealed(path, {'build': build, 'baseline_build': BASELINE, 'per_pass': report, 'totals': totals,
                    'S0_S_engine_surface_removal': {'report': str(baseline_s), 'by_event': engine_removals}})
    artifacts[str(path)] = a.sha(path)
    result = {**fingerprints, 'passed': all(row['passed'] for row in results.values()),
              'baseline_build': BASELINE, 'cases': results, 'fixture_receipt': str(fixture),
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
    assert len(run.REFUSALS) == 18
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
    contact = {'rank': '0', 'stage': 'local', 'pass': '1', 'kind': 'verify', 'owner_rows': '2',
               'owner_bytes': '128', 'peak_bytes': '12', 'verify_cells': '30', 'verify_candidates': '6',
               'verify_calls': '6', 'seconds_gather': '0.5', 'seconds_verify': '0.2'}
    embedding = {'rank': '0', 'pass': '1', 'violations': '0', 'pairs': '12', 'calls': '12',
                 'peak_bytes': '30', 'seconds': '0.2'}
    for marker, row in ((run.CONTACT, contact), ('FP2_C3 embedding', embedding)):
        assert run.LINE.fullmatch(marker + ' ' + ' '.join(f'{key}={value}' for key, value in row.items()))
    rows = {run.CONTACT: [contact], 'embedding': [embedding], 'pass': [{'pass': '1', 'cells_out': '30'}]}
    assert not run.contact_problems(0, rows, ['1'], 1)
    assert run.contact_problems(0, rows, ['1'], 6)[0]['row'] == run.CONTACT
    assert run.contact_problems(0, {**rows, run.CONTACT: [{**contact, 'verify_cells': '29'}]}, ['1'], 1)
    assert run.contact_problems(0, {**rows, 'embedding': []}, ['1'], 1)
    assert repeat_fields(run.CONTACT, contact) == repeat_fields(
        run.CONTACT, {**contact, 'peak_bytes': '20', 'seconds_verify': '1'})
    assert repeat_fields(run.CONTACT, contact) != repeat_fields(run.CONTACT, {**contact, 'verify_calls': '5'})
    print('FP2-C3 lab comparator self-check passed.')


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
