"""Render the FP2-P2 gate evidence and historical comparisons."""
from argparse import ArgumentParser
from pathlib import Path
import json

from collect_p2 import FOOTPRINT_FIELDS
import run_p2 as p
from run_p2 import (CAMPAIGN, HERE, SHARED, combined_status, cut, fingerprint,
                    prepare, selected_summary, verify_campaign, verify_pins, verify_summary)
from seal_d1 import historical_measurement, verify_d1

UNEVALUATED = ('G2 with closure off', 'GQ with closure off', 'G3 with closure off',
               'G5 with closure off', 'full two-event no-BL workflow with closure off',
               '18 FP1 cohorts', 'second S workflow', 'closure on by default')


def load(path):
    return cut.read(path) if path.is_file() else {'status': 'not evaluated'}


def compact(value):
    if value is None:
        return '—'
    if isinstance(value, float):
        return f'{value:.8g}'
    if isinstance(value, (dict, list)):
        value = json.dumps(value, separators=(',', ':'))
    return str(value).replace('|', '\\|')


def table(columns, rows):
    return ['| ' + ' | '.join(columns) + ' |', '| ' + ' | '.join(['---'] * len(columns)) + ' |',
            *('| ' + ' | '.join(map(compact, row)) + ' |' for row in rows), '']


def recovered_sum(mean, rows):
    if rows == 0:
        return 0
    estimate = round(mean * rows)
    if (estimate < 0 or estimate / rows != mean or (estimate + 1) / rows == mean
            or (estimate > 0 and (estimate - 1) / rows == mean)):
        raise ValueError(f'No unique nonnegative integer reproduces mean {mean!r} with {rows} rows')
    return estimate


def footprints(versions):
    tables, findings = {}, []
    for version, trace in versions.items():
        tables[version] = {}
        for cohort in ('0', '1', '2', '3', '4', 'direct_eligible_1'):
            row = trace['groups'].get(cohort, {})
            result = tables[version][cohort] = {}
            count = result['PLANES rows'] = row.get('miss_bypass_denominators', {}).get('planes_rows')
            for field in FOOTPRINT_FIELDS:
                try:
                    result[field] = (row['plane_sums'][field] if version == 'FP2-P2'
                                     else recovered_sum(None if count == 0 else row['means_per_planes_row'][field],
                                                        count))
                except (KeyError, OverflowError, TypeError, ValueError) as error:
                    result[field] = None
                    findings.append({'version': version, 'cohort': cohort, 'field': field, 'reason': str(error)})
    return tables, findings


def shared_path(folder, name):
    return HERE / 'shared' / folder.name / 'closure1' / name / 'case_summary.json'


def campaign_path(folder):
    return HERE / 'campaign' / folder.name / 'comparison.json'


def promotions(folder):
    from seal_d2 import verify_d2

    return {key: verify(folder) if (folder / f'{key.lower()}_promotion.json').is_file() else
            (None, {'status': 'not evaluated', 'promote': False})
            for key, verify in (('D1', verify_d1), ('D2', verify_d2))}


def historical():
    path = HERE.parent / 'FP2_P1/reports/FP2_P1_670.json'
    p.inherited.c3.c2.committed(path, 'dec45f8a71')
    reference = cut.read(path)
    reference['seal'] = {'path': str(path), 'sha256': cut.sha(path)}
    return reference


def gather(build):
    folder = HERE / f'build{build}'
    fixture = load(folder / 'p2_checkpoint.json')
    campaign = load(campaign_path(folder))
    d = {key: result for key, (_, result) in promotions(folder).items()}
    if 'trials' not in campaign:
        selected = selected_summary(build, 'campaign-N1-1')
        if selected.is_file():
            campaign['trials'] = {'campaign-N1-1': {**cut.read(selected), 'path': str(selected)}}
    for trial in campaign.get('trials', {}).values():
        if trial.get('used'):
            trial['elapsed_seconds'] = cut.read(trial['used'])['wait']['elapsed_seconds']
    anatomy = load(selected_summary(build, 'anatomy-N1'))
    shared = {name: load(shared_path(folder, name)) for name in SHARED}
    workflow = load(HERE / 'G5' / folder.name / 'S/case_summary.json')
    if workflow['status'] == 'passed':
        workflow['isolated_restarts'] = {}
        for version, source in (('build664', 'reference'), ('candidate', 'run')):
            receipt = Path(workflow[source])
            sealed = cut.read(receipt)
            path = Path(sealed['restart'])
            assert cut.sha(path) == sealed['artifacts'][str(path.relative_to(receipt.parent))]
            row = cut.read(path)
            assert row['disk_restart_verified'] and row['standalone_qualified_cfd']
            workflow['isolated_restarts'][version] = {
                'receipt': str(path), 'details': row}
    c492 = {}
    for closure in (0, 1):
        path = HERE / 'shared' / folder.name / f'closure{closure}/exact-c492/case_summary.json'
        child = load(path).get('subruns', {}).get('bl', {})
        if child.get('raw', '').endswith('.json'):
            raw = Path(child['raw'])
            assert cut.sha(raw) == child['raw_sha256']
            detail = cut.read(raw)
            if 'actual_splits' in detail:
                c492[str(closure)] = {key: detail[key] for key in ('actual_splits', 'first_differing_edit')}
    prior = historical()
    historical_d = {f'FP2-P1 closure {closure}': historical_measurement(branch, closure)[0]
                    for closure, branch in enumerate(('default', 'campaign'))}
    status = cut.read(folder / 'build_status.json')
    data = dict(build=build, build_provenance=status, fixtures=fixture, gate_d1=d['D1'], gate_d2=d['D2'],
                gate_d1_historical=historical_d, campaign=campaign, anatomy=anatomy, shared_closure1=shared,
                historical_shared_closure1=prior['shared_closure1'], c492_decision_evidence=c492, S=workflow,
                historical_campaign=prior['campaign']['baseline'], historical_seal=prior['seal'],
                lab_checkpoint=load(folder / 'p2_lab_checkpoint.json'),
                production_diff={'path': '../production.diff', 'sha256': cut.sha(HERE / 'production.diff')},
                focused_public=load(folder / 'public_query_comparison.json'), FP2_P1_S='not evaluated')
    observer = anatomy.get('observer') or {}
    trace_tables = data['anatomy_tables'] = dict(prior['anatomy_tables'])
    data['anatomy_stages'] = {'FP2-M2': prior['anatomy']['observer']['baseline']['stage_rows'],
                             'FP2-P1': prior['anatomy']['observer']['stage_rows']}
    if observer.get('completion') == 'complete':
        trace = Path(anatomy['used']).with_name('anatomy.json')
        before = Path(observer['baseline']['trace'])
        assert cut.read(before) == trace_tables['FP2-M2'], before
        trace_tables['FP2-P2'] = cut.read(trace)
        data['anatomy_stages']['FP2-P2'] = observer['stage_rows']
    data['footprint_sums'], data['footprint_findings'] = footprints(trace_tables)
    data['diagnoses'] = {}
    for name in ('d1_r5', 'first_collapse_r5'):
        path = HERE / 'reports' / f'{name}_{build}.md'
        if path.is_file():
            data['diagnoses'][name] = {'path': path.name, 'sha256': cut.sha(path)}
    corrections = HERE / 'reports/test_code_corrections.md'
    if corrections.is_file():
        data['test_code_corrections'] = {'path': corrections.name, 'sha256': cut.sha(corrections)}
    data['not_evaluated'] = [
        *UNEVALUATED, 'FP2-P1 run S',
        *(name for name in p.STEP1 if fixture.get('parts', {}).get(name, {}).get('status', 'not evaluated')
          == 'not evaluated'),
        *(f'gate {key}' for key, item in d.items() if item['status'] == 'not evaluated'),
        *(name for name in CAMPAIGN if campaign.get('trials', {}).get(name, {}).get('status', 'not evaluated')
          == 'not evaluated'),
        *(f'{name} lab repeat comparison' for name in ('N6P', 'N1', 'N6')
          if campaign.get('pairs', {}).get(name, {}).get('status', 'not evaluated') == 'not evaluated'),
        *(f'{name} contact and footprint counts (trace off)' for name in ('N6P', 'N6')),
        *('ANATOMY ' + group for group in ('qualification and trace observation', 'exact projection')
          if (group != 'exact projection' and anatomy['status'] == 'not evaluated')
          or (group == 'exact projection' and anatomy.get('identity', {}).get('status', 'not evaluated')
              == 'not evaluated')),
        *(f'{name} with closure on' for name, row in shared.items() if row['status'] == 'not evaluated'),
        *(('S workflow', 'S events 1 and 2', 'S isolated restart', 'S per-rank exclusive timer totals',
           'S proof and hand-over faces, cells and job peak', 'S six-payload comparison')
          if workflow['status'] == 'not evaluated' else ()),
    ]
    return data


def seal_lab(build):
    folder = HERE / f'build{build}'
    receipt, _, _ = prepare(folder)
    rows = {}
    d = promotions(folder)

    def part(path, result):
        return ({'status': result['status'], 'path': str(path), 'sha256': cut.sha(path)}
                if path is not None else {'status': result['status']})

    for key, (path, result) in d.items():
        rows[key] = part(path, result)
        if path is not None:
            rows[key]['promote'] = result['promote']
            rows[key]['selected'] = result.get('selected', result.get('shared_receipt'))
    if all(item[1]['promote'] for item in d.values()) and campaign_path(folder).is_file():
        comparison, campaign = verify_campaign(folder)
        rows['campaign'] = part(comparison, campaign)
    else:
        campaign = {'trials': {}, 'pairs': {}}
        rows['campaign'] = {'status': 'not evaluated'}
    anatomy = selected_summary(build, 'anatomy-N1')
    if anatomy.exists():
        rows['ANATOMY'] = part(anatomy, verify_summary(receipt, 'anatomy-N1', anatomy))
    else:
        rows['ANATOMY'] = {'status': 'not evaluated'}
    for name in SHARED[1:]:
        path = shared_path(folder, name)
        result = load(path)
        if path.is_file():
            assert fingerprint(result) == fingerprint(receipt)
            verify_pins(result['pins'])
            rows[name] = part(path, result)
        else:
            rows[name] = {'status': 'not evaluated'}
    data = {**fingerprint(receipt), 'status': combined_status(row['status'] for row in rows.values()),
            'parts': rows,
            'six_trials_qualified': len(campaign['trials']) == 6 and all(
                trial.get('evaluate_qualified') and trial['status'] == 'passed' for trial in campaign['trials'].values()),
            'three_equal_pairs': len(campaign['pairs']) == 3 and all(
                pair['status'] == 'equal' for pair in campaign['pairs'].values())}
    p.a.sealed(folder / 'p2_lab_checkpoint.json', data)
    return data


def render(data, label):
    lines = [f"# FP2-P2 build{data['build']}", '', label, '',
             'Closure uses the existing curved prepared witness, gated by a process switch that defaults to off. '
             'Each opt-in result is an observation.', '',
             '[Signed order](../inputs/FP2_P2_VERTEX_WITNESS.md) · '
             '[Whole-item source check](source_check.md) · [Production diff](../production.diff)', '',
             '## Step 1', '']
    lines += table(('Part', 'Status', 'Evidence SHA256'), (
        (name, row['status'], row.get('sha256')) for name, row in data['fixtures'].get('parts', {}).items()))
    query = data['focused_public']
    if query.get('completion') == 'complete':
        lines += table(('Closure', 'Prepared nodes', 'Plane groups', 'Complete lower', 'Cropped lower', 'Scope evictions'),
                       ((key, row['improvement']['prepared_nodes'], row['improvement']['planes']['groups'],
                         row['lower_complete']['root_lower'], row['lower_cropped']['root_lower'], row['cache']['evictions'])
                        for key, row in ((key, query[key]) for key in ('off', 'on'))))
    lines += [f"## Step 2 · {data['lab_checkpoint']['status']}", '',
              'The inherited C3 evaluator records the 600-second pass threshold as a timing observation. '
              'The watchdog limits and result gates use the inherited comparisons.','']
    d1, d2 = data['gate_d1'], data['gate_d2']
    lines += ['### D1 and D2', '']
    lines += table(('Gate', 'Status', 'Promotes', 'First finding'), (
        (gate, row['status'], row['promote'], row.get('first_difference'))
        for gate, row in (('D1', d1), ('D2', d2))))
    d1_rows = {
        'FP2-P2 control': d1.get('observations', {}).get('default-N1', {}),
        'FP2-P2 D1': d1.get('observations', {}).get('campaign-N1-1', {}),
        **data['gate_d1_historical'],
    }
    lines += table(('N1', 'Classification work', 'Traversal work', 'Scope build work',
                    'Elapsed s', 'Step C s', 'Bound s'), (
        (version, *(row.get(key) for key in ('classification_work', 'traversal_work', 'scope_build_work',
                                            'elapsed_seconds', 'step_c_seconds', 'bound_seconds')))
        for version, row in d1_rows.items()))
    if d1.get('observation_findings'):
        lines += table(('D1 trial', 'Report field', 'Finding'), (
            (row['mode'], row['field'], row['observed']) for row in d1['observation_findings']))
    if d2.get('hard_checks'):
        lines += table(('D2 hard check', 'Status'), d2['hard_checks'].items())
        obs = d2['observations']
        lines += table(('Captured ranks', 'Work', 'Work limit', 'Summed peak bytes', 'Resource limit bytes',
                        'Seconds until stop', 'Stop reason'), [(
            obs['captured_ranks'], obs['observed_work'], obs['work_limit'],
            obs['observed_solver_peak_sum_bytes'], obs['resource_limit_bytes'],
            obs['seconds_until_stop'], obs['stop_reason'])])
    if data['diagnoses']:
        lines += [' '.join(f"[R5 diagnosis]({rec['path']})" for rec in data['diagnoses'].values()), '']
    lines += ['### Lab campaign', '']
    campaign = data['campaign']
    trials = campaign.get('trials', {})
    lines += table(('Trial', 'Status', 'Inherited qualification', 'Elapsed s', 'First finding'), (
        (name, row['status'], row.get('evaluate_qualified'), row.get('elapsed_seconds'), row.get('first_difference'))
        for name, row in ((name, trials.get(name, {'status': 'not evaluated'})) for name in CAMPAIGN)))
    comparisons = campaign.get('baseline', {})
    historical = data['historical_campaign']
    lines += table(('Case', 'FP2-P2 repeat identity', 'Version', 'Build664 identity', 'First historical difference',
                    'Build664 s', 'Trial 1 s'), (
        (name, campaign.get('pairs', {}).get(name, {}).get('status', 'not evaluated')
         if version == 'FP2-P2' else None, version, row.get('status', 'not evaluated'),
         row.get('first_difference'), old['build664']['elapsed_seconds'],
         (row.get('trial1') or {}).get('elapsed_seconds'))
        for name, old in historical.items()
        for version, row in (('FP2-P1', old), ('FP2-P2', comparisons.get(name, {})))))
    for name, prior in historical.items():
        measured = {'build664': prior['build664'], 'FP2-P1': prior.get('trial1'),
                    'FP2-P2': comparisons.get(name, {}).get('trial1')}
        lines += [f'### {name} hand-over and rank measurements', '']
        lines += table(('Build', 'Hand-over faces', 'Final cells', 'Surface removals', 'Volume collapses'), (
            (version, totals['faces'][-1] if totals.get('faces') else None, totals.get('final_cells'),
             sum(stage['removals'] for row in trial['by_rank'].values() for stage in row['by_pass'].values()),
             totals.get('v_collapses'))
            for version, trial in measured.items() if trial and (totals := trial['totals']) is not None))
        lines += table(('Build', 'Rank', 'Pass', 'Faces', 'Removals', 'Pass s', 'Step C s', 'Bound s'), (
            (variant, rank, phase, record['faces'], record['removals'],
             record['seconds_pass'], record['seconds_c'], record['seconds_c_bound'])
            for variant, trial in measured.items() if trial
            for rank, item in trial['by_rank'].items() for phase, record in item['by_pass'].items()))
    anatomy = data['anatomy']
    observer = anatomy.get('observer') or {}
    lines += ['### ANATOMY', '']
    lines += table(('Result gate', 'Exact projection', 'Trace completion', 'First finding'), [
        (anatomy.get('result_status', anatomy['status']), anatomy.get('identity', {}).get('status'),
         observer.get('completion'), anatomy.get('first_difference'))])
    groups, closure, totals = [], [], []
    for version, trace in data.get('anatomy_tables', {}).items():
        bins = tuple(trace['groups'].values())
        summarized = {field: sum(row['miss_bypass_denominators'][field] for row in bins)
                      for field in ('forwards', 'prepared_nodes', 'result_work', 'planes_rows')}
        root_enough = sum(row['root_enough'] for row in bins)
        totals.append((version, summarized['forwards'], root_enough,
                       root_enough / summarized['forwards'] if summarized['forwards'] else None,
                       summarized['prepared_nodes'], summarized['result_work'], summarized['planes_rows'],
                       sum(row.get('closure', {}).get('splits', 0) for row in bins) if version != 'FP2-M2' else None))
        for group, row in trace['groups'].items():
            count = row['miss_bypass_denominators']
            groups.append((version, group, row['calls_by_class'], row['root_enough_share'], count['prepared_nodes'],
                           row['prepared_physical_nodes'], count['result_work'],
                           {k: value['work'] for k, value in row['result_work_parts'].items()},
                           row['means_per_planes_row']['groups'], count['planes_rows']))
            if version == 'FP2-P2' and 'closure' in row:
                closure.append((group, row['closure']))
    lines += table(('Version', 'Miss and bypass forwards', 'Root enough', 'Root-enough share', 'Prepared nodes',
                     'Result work', 'PLANES rows', 'Committed fans'), totals)
    lines += table(('Version', 'Length and direct group', 'Forwards by class', 'Root-enough share', 'Prepared nodes',
                     'Node distribution', 'Result work', 'Work by part', 'Mean groups', 'PLANES rows'), groups)
    if closure:
        fields = tuple(closure[0][1])
        lines += table(('Group', *fields), ((group, *(row[field] for field in fields)) for group, row in closure))
    lines += table(('Version', 'Pass', 'Length bin', 'Direct eligible', 'Outer spending shares', 'Upper spending shares'),
                   ((variant, row['pass'], row['bin'], row['direct_eligible'], row['outer_shares'], row['upper_shares'])
                    for variant, rows in data['anatomy_stages'].items() for row in rows))
    lines += ['### Footprint outcomes for completed misses and bypasses', '',
              'Cohorts 0–4 have direct_eligible=0. The direct_eligible=1 rows form one pooled cohort. '
              'N6 and N6P use trace off. Their contact and footprint counts are not evaluated.', '']
    lines += table(('Version', 'Cohort', 'PLANES rows', *FOOTPRINT_FIELDS), (
        (version, cohort, row['PLANES rows'], *(row[key] for key in FOOTPRINT_FIELDS))
        for version, cells in data['footprint_sums'].items() for cohort, row in cells.items()))
    fields = ('PLANES rows', *FOOTPRINT_FIELDS)
    footprint_totals = []
    for version, cells in data['footprint_sums'].items():
        columns = ([row[field] for row in cells.values()] for field in fields)
        footprint_totals.append((version, *(None if None in column else sum(column) for column in columns)))
    lines += table(('Version', *fields), footprint_totals)
    if data['footprint_findings']:
        lines += table(('Version', 'Cohort', 'Field', 'Measurement finding'), (
            (row['version'], row['cohort'], row['field'], row['reason']) for row in data['footprint_findings']))
    lines += ['### Shared checks with closure on', '']
    lines += table(('Check', 'Version', 'Status', 'First findings or missed comparisons'),
                   ((name, version, row['status'], {part: value.get('first_failure') or value.get('missed_comparison')
                     for part, value in row.get('subruns', {}).items() if value.get('first_failure') or
                     value.get('missed_comparison')})
                    for name in SHARED for version, groups in (('FP2-P1', data['historical_shared_closure1']),
                                                                 ('FP2-P2', data['shared_closure1']))
                    for row in (groups[name],)))
    if data['c492_decision_evidence']:
        lines += table(('C492 closure', 'Positive splits', 'First edit difference from RunA'), (
            (closure, row['actual_splits'], row['first_differing_edit'])
            for closure, row in data['c492_decision_evidence'].items()))
    c492 = data['shared_closure1']['exact-c492'].get('subruns', {}).get('bl', {})
    if c492.get('watchdog'):
        lines += [f"C492 first reached its 600-second wrapper. The same-limit retry is {c492['status']}. "
                  "Both attempts have terminal process receipts.", '']
    workflow = data['S']
    lines += ['## Step 3', '', f"**FP2-P2 S {workflow['status']}.** FP2-P1 S is not evaluated.", '']
    if workflow.get('reason'):
        lines.extend((workflow['reason'], ''))
    if workflow['status'] == 'passed':
        lines += table(('Build', 'Event', 'Native maximum s', 'Proof faces', 'Hand-over faces', 'Cells',
                        'Job peak bytes', 'Minimum timer coverage'), (
            (version, row['event'], row['native_seconds'], row['proof_faces'], row['handover_faces'],
             row['actual_mesh_counts']['cells'], row['job_peak_rss_bytes'], row['minimum_timer_coverage'])
            for version, events in workflow['events'].items() for row in events))
        lines += table(('Build', 'Rank', 'Event', 'Geometry bound s', 'Communication and join wait s', 'Other s'), (
            (version, rank, event, row['geometry_bound'], row['communication_and_join_wait'], row['other'])
            for version, ranks in workflow['final_rank_exclusive_seconds'].items() for rank, events in ranks.items()
            for event, row in events.items()))
        lines += table(('Build', 'Isolated restart', 'Seconds', 'Ranks', 'Points', 'Cells', 'Coordinate error'), (
            (version, 'passed' if row['disk_restart_verified'] else 'FAILED', row['seconds'], row['ranks'],
             row['adapted_points'], row['adapted_cells'], row['coordinate_maximum_error'])
            for version, record in workflow['isolated_restarts'].items() for row in (record['details'],)))
        lines += table(('File', 'Build664 SHA256', 'Candidate SHA256', 'Equal'), (
            (name, data['build664'], data['candidate'], data['equal']) for name, data in workflow['payloads'].items()))
    lines += ['## Build and receipt observations', '']
    if 'test_code_corrections' in data:
        lines += [f"[Test-code corrections]({data['test_code_corrections']['path']})", '']
    observations = data['build_provenance']
    lines += table(('Build', 'Solver SHA256', 'Test driver SHA256', 'Source pins SHA256'), [(
        data['build'], observations.get('binary_sha256'), observations.get('test_binary_sha256'),
        observations.get('source_pins_sha256'))])
    candidate_receipts = {**campaign.get('trials', {}), 'anatomy-N1': anatomy}
    for name, trial in candidate_receipts.items():
        normalized = Path(trial['used']).with_name(p.REJOINED) if trial.get('used') else None
        if normalized is not None and normalized.is_file():
            lines.append(f'- {name} recovered an enabled marker interleaved with a stdout row. '
                         'The derived log removes only the marker bytes and reuses the inherited evaluator. '
                         'Both the raw and corrected receipts are preserved.')
    lines += ['', '## Not evaluated', '', *(f'- {name}' for name in data['not_evaluated']), '']
    return '\n'.join(lines)


def self_check():
    assert recovered_sum(None, 0) == 0
    assert recovered_sum(2 / 3, 3) == 2
    for mean, count in ((.5, 3), (1., 2**54)):
        try:
            recovered_sum(mean, count)
        except ValueError:
            pass
        else:
            raise AssertionError('Accepted a missing or ambiguous historical integer')
    traces = historical()['anatomy_tables']
    tables, findings = footprints(traces)
    assert not findings, findings
    for label, total in (('FP2-M2', 16), ('FP2-P1', 11860)):
        assert sum(row['invalid'] for row in tables[label].values()) == total


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('label', nargs='?')
    parser.add_argument('--seal-lab', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        raise SystemExit(0)
    if args.build is None:
        parser.error('provide a build number')
    if args.seal_lab:
        print(seal_lab(args.build)['status'])
        raise SystemExit(0)
    if not args.label:
        parser.error('a final checkpoint label is required')
    evidence = gather(args.build)
    evidence['label'] = args.label
    out = HERE / 'reports' / f'FP2_P2_{args.build}'
    cut.save(out.with_suffix('.json'), evidence)
    out.with_suffix('.md').write_text(render(evidence, args.label))
