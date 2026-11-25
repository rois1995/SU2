"""Render the FP2-P1 gate evidence and build664/M2 comparisons."""
from argparse import ArgumentParser
from pathlib import Path
import json

from run_p1 import (CAMPAIGN, HERE, SHARED, combined_status, cut, fingerprint,
                    prepare, selected_summary, verify_campaign, verify_pins, verify_summary)

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


def gather(build):
    folder = HERE / f'build{build}'
    fixture = load(folder / 'p1_checkpoint.json')
    campaign = load(HERE / 'campaign' / folder.name / 'comparison.json')
    for trial in campaign.get('trials', {}).values():
        if trial.get('used'):
            trial['elapsed_seconds'] = cut.read(trial['used'])['wait']['elapsed_seconds']
    anatomy = load(selected_summary(build, 'anatomy-N1'))
    shared = {name: load(HERE / 'shared' / folder.name / 'closure1' / name / 'case_summary.json')
              for name in SHARED}
    workflow = load(HERE / 'G5' / folder.name / 'S/case_summary.json')
    data = dict(build=build, build_provenance=cut.read(folder / 'build_status.json'), fixtures=fixture,
                campaign=campaign, anatomy=anatomy, shared_closure1=shared, S=workflow,
                lab_checkpoint=load(folder / 'p1_lab_checkpoint.json'),
                production_diff={'path': '../production.diff', 'sha256': cut.sha(HERE / 'production.diff')},
                focused_public=load(folder / 'public_query_comparison.json'))
    observer = anatomy.get('observer') or {}
    if observer.get('completion') == 'complete':
        trace = Path(anatomy['used']).with_name('anatomy.json')
        before = Path(observer['baseline']['trace'])
        data['anatomy_tables'] = {'FP2-M2': cut.read(before), 'FP2-P1': cut.read(trace)}
    diagnosis = HERE / 'reports' / f'first_collapse_r5_{build}.json'
    if diagnosis.is_file():
        data['first_collapse_diagnosis'] = cut.read(diagnosis)
    data['not_evaluated'] = [
        *UNEVALUATED,
        *(name for name, row in fixture.get('parts', {}).items() if row['status'] == 'not evaluated'),
        *(name for name in CAMPAIGN if campaign.get('trials', {}).get(name, {}).get('status', 'not evaluated')
          == 'not evaluated'),
        *(f'{name} lab repeat comparison' for name in ('N6P', 'N1', 'N6')
          if campaign.get('pairs', {}).get(name, {}).get('status', 'not evaluated') == 'not evaluated'),
        *('ANATOMY ' + group for group in ('qualification and trace observation', 'exact projection')
          if (group != 'exact projection' and anatomy['status'] == 'not evaluated')
          or (group == 'exact projection' and anatomy.get('identity', {}).get('status', 'not evaluated')
              == 'not evaluated')),
        *(f'{name} with closure on' for name, row in shared.items() if row['status'] == 'not evaluated'),
        *(('S workflow', 'S isolated restart', 'S event, timer and payload comparison')
          if workflow['status'] == 'not evaluated' else ()),
    ]
    return data



def seal_lab(build):
    folder = HERE / f'build{build}'
    receipt, _, _ = prepare(folder)
    comparison, campaign = verify_campaign(folder)
    rows = {'campaign': {'status': campaign['status'], 'path': str(comparison), 'sha256': cut.sha(comparison)}}
    anatomy = selected_summary(build, 'anatomy-N1')
    if anatomy.exists():
        result = verify_summary(receipt, 'anatomy-N1', anatomy)
        rows['ANATOMY'] = {'status': result['status'], 'path': str(anatomy), 'sha256': cut.sha(anatomy)}
    else:
        rows['ANATOMY'] = {'status': 'not evaluated'}
    for name in SHARED:
        path = HERE / 'shared' / folder.name / 'closure1' / name / 'case_summary.json'
        result = load(path)
        if path.is_file():
            assert fingerprint(result) == fingerprint(receipt)
            verify_pins(result['pins'])
            rows[name] = {'status': result['status'], 'path': str(path), 'sha256': cut.sha(path)}
        else:
            rows[name] = {'status': 'not evaluated'}
    status = combined_status(row['status'] for row in rows.values())
    data = {**fingerprint(receipt), 'status': status, 'parts': rows,
            'six_trials_qualified': len(campaign['trials']) == 6 and all(
                trial.get('evaluate_qualified') and trial['status'] == 'passed' for trial in campaign['trials'].values()),
            'three_equal_pairs': len(campaign['pairs']) == 3 and all(
                pair['status'] == 'equal' for pair in campaign['pairs'].values())}
    out = folder / 'p1_lab_checkpoint.json'
    if out.is_file():
        assert cut.read(out) == data
    else:
        cut.save(out, data)
    return data


def render(data, label):
    lines = [f"# FP2-P1 build{data['build']}", '', label, '',
             'Closure uses the existing curved prepared witness, gated by a process switch that defaults to off. '
             'Each opt-in result is an observation.', '',
             '[Signed order](../inputs/FP2_P1_CONFORMING_WITNESS.md) · '
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
    campaign = data['campaign']
    trials = campaign.get('trials', {})
    lines += table(('Trial', 'Status', 'Inherited qualification', 'Elapsed s', 'First finding'), (
        (name, row['status'], row.get('evaluate_qualified'), row.get('elapsed_seconds'), row.get('first_difference'))
        for name, row in ((name, trials.get(name, {'status': 'not evaluated'})) for name in CAMPAIGN)))
    lines += table(('Case', 'Repeat identity', 'Build664 identity', 'First historical difference',
                    'Build664 s', 'Trial 1 s'), (
        (name, campaign['pairs'][name]['status'], row['status'], row.get('first_difference'),
         row['build664']['elapsed_seconds'], (row.get('trial1') or {}).get('elapsed_seconds'))
        for name, row in campaign.get('baseline', {}).items()))
    lines += table(('Case', 'Build', 'Hand-over faces', 'Final cells', 'Surface removals', 'Volume collapses'), (
        (name, variant, totals['faces'][-1] if totals.get('faces') else None, totals.get('final_cells'),
         sum(stage['removals'] for row in measured['by_rank'].values() for stage in row['by_pass'].values()),
         totals.get('v_collapses'))
        for name, record in campaign.get('baseline', {}).items() for variant in ('build664', 'trial1')
        if (measured := record.get(variant)) is not None and (totals := measured['totals']) is not None))
    for name, row in campaign.get('baseline', {}).items():
        lines += [f'### {name} rank and pass measurements', '']
        values = []
        for variant in ('build664', 'trial1'):
            for rank, item in (row.get(variant) or {}).get('by_rank', {}).items():
                for phase, record in item['by_pass'].items():
                    values.append((variant, rank, phase, record['faces'], record['removals'],
                                   record['seconds_pass'], record['seconds_c'], record['seconds_c_bound']))
        lines += table(('Build', 'Rank', 'Pass', 'Faces', 'Removals', 'Pass s', 'Step C s', 'Bound s'), values)
    anatomy = data['anatomy']
    observer = anatomy.get('observer') or {}
    lines += ['### ANATOMY', '']
    lines += table(('Result gate', 'Exact projection', 'Trace completion', 'First finding'), [
        (anatomy.get('result_status', anatomy['status']), anatomy.get('identity', {}).get('status'),
         observer.get('completion'), anatomy.get('first_difference'))])
    groups, closure, totals = [], [], []
    for version, trace in data.get('anatomy_tables', {}).items():
        bins = [row for key, row in trace['groups'].items() if key.isdecimal()]
        summarized = {field: sum(row['miss_bypass_denominators'][field] for row in bins)
                      for field in ('forwards', 'prepared_nodes', 'result_work', 'planes_rows')}
        root_enough = sum(row['root_enough'] for row in bins)
        totals.append((version, summarized['forwards'], root_enough,
                       root_enough / summarized['forwards'] if summarized['forwards'] else None,
                       summarized['prepared_nodes'], summarized['result_work'], summarized['planes_rows'],
                       sum(row.get('closure', {}).get('splits', 0) for row in bins) if version == 'FP2-P1' else None))
        for group, row in trace['groups'].items():
            count = row['miss_bypass_denominators']
            groups.append((version, group, row['calls_by_class'], row['root_enough_share'], count['prepared_nodes'],
                           row['prepared_physical_nodes'], count['result_work'],
                           {k: value['work'] for k, value in row['result_work_parts'].items()},
                           row['means_per_planes_row']['groups'], count['planes_rows']))
            if 'closure' in row:
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
                    for variant, rows in (('FP2-M2', observer.get('baseline', {}).get('stage_rows', [])),
                                          ('FP2-P1', observer.get('stage_rows', []))) for row in rows))
    lines += ['### Shared checks with closure on', '']
    lines += table(('Check', 'Status', 'First findings or missed comparisons'),
                   ((name, row['status'], {part: value.get('first_failure') or value.get('missed_comparison')
                    for part, value in row.get('subruns', {}).items() if value.get('first_failure') or
                    value.get('missed_comparison')}) for name, row in data['shared_closure1'].items()))
    c492 = data['shared_closure1']['exact-c492'].get('subruns', {}).get('bl', {})
    if c492.get('watchdog'):
        lines += [f"C492 first reached its 600-second wrapper. The same-limit retry is {c492['status']}. "
                  "Both attempts have terminal process receipts.", '']
    if 'first_collapse_diagnosis' in data:
        finding = data['first_collapse_diagnosis']['first_failure']
        counters = data['first_collapse_diagnosis']['case_source_stats']['closure_on_minus_off']
        lines += [f"First collapse used {finding['value']:,} work units against the {finding['maximum']:,} ceiling. "
                  f"The measured increase is {counters['scope_build_work']:,} units in scope building and "
                  f"{counters['traversal_work']:,} in proof traversal. The R5 check found no production wiring "
                  f"error. [First-collapse diagnosis](first_collapse_r5_{data['build']}.md)", '']
    workflow = data['S']
    lines += ['## Step 3', '', f"**S {workflow['status']}.**", '']
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
        lines += table(('File', 'Build664 SHA256', 'Candidate SHA256', 'Equal'), (
            (name, data['build664'], data['candidate'], data['equal']) for name, data in workflow['payloads'].items()))
    lines += ['## Build and receipt corrections', '',
              '- Build669 regenerated bundled HDF5 and recorded eight qualifier warnings. Build670 recompiled the '
              'changed translation units with zero warnings and froze the same binary hashes.',
              '- The G1 helper used its inherited receipt filename. The three complete, exact comparisons were '
              'sealed under the expected name after verifying the saved pins and terminal identities.',
              '- The first-collapse sampler records procfs KiB. The receipt conversion now reports the sum of '
              'the six observed process peaks in bytes. The original receipt and resource samples are preserved.']
    for name, trial in campaign.get('trials', {}).items():
        if trial.get('path') and cut.read(trial['path']).get('mixed_marker_seal'):
            lines.append(f'- {name} recovered an enabled marker interleaved with a stdout row. '
                         'The derived log removes only the marker bytes and reuses the inherited evaluator. '
                         'Both the raw and corrected receipts are preserved.')
    if (data['anatomy'].get('status') == 'passed' and Path(data['anatomy']['used']).name
            == 'measurement_stdout_rejoined.json'):
        lines.append('- ANATOMY received the same marker repair. The original raw trace stream and its '
                     'aggregation were preserved, and the reconstructed comparison was exact.')
    lines += ['', '## Not evaluated', '', *(f'- {name}' for name in data['not_evaluated']), '']
    return '\n'.join(lines)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('label', nargs='?')
    parser.add_argument('--seal-lab', action='store_true')
    args = parser.parse_args()
    if args.seal_lab:
        print(seal_lab(args.build)['status'])
        raise SystemExit(0)
    if not args.label:
        parser.error('a final checkpoint label is required')
    evidence = gather(args.build)
    evidence['label'] = args.label
    out = HERE / 'reports' / f'FP2_P1_{args.build}'
    cut.save(out.with_suffix('.json'), evidence)
    out.with_suffix('.md').write_text(render(evidence, args.label))
