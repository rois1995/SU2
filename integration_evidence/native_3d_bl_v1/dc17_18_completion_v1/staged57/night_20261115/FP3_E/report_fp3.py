"""Seal Step 1 and publish the FP3-E result from its selected receipts."""

from argparse import ArgumentParser
import hashlib
from pathlib import Path
import runpy
import subprocess

from build_fp3 import HERE, cut, verify_build
from collect_fp3_full import collect
from run_fp3_full import D_REFERENCE, FINGERPRINTS, ORDER_SHA, check_pins, path_pin, reference_runs, root, workflow


def read_pin(pointer):
    path = Path(pointer['path'])
    assert cut.sha(path) == pointer['sha256'], path
    return cut.read(path)


def read_part(path, build):
    if not path.is_file():
        return {'status': 'not evaluated', 'failure_kind': None, 'first_difference': None}
    row = cut.read(path)
    assert all(row[key] == build[key] for key in FINGERPRINTS), path
    check_pins(row['pins'])
    return row


def step1(label):
    build = verify_build(label)
    scope = HERE / 'controls' / f'build{label}' / 'step1_scoped.json'
    row = read_part(scope, build)
    assert row['status'] in ('passed', 'FAILED', 'OPEN'), row
    record = {key: row[key] for key in (*FINGERPRINTS, 'status', 'passed', 'failure_kind', 'first_difference')}
    record['parts'] = row['parts']
    record['pins'] = {**row['pins'], str(scope): cut.sha(scope)}
    path = HERE / f'build{label}/step1.json'
    if path.is_file():
        assert cut.read(path) == record, path
    else:
        cut.save(path, record)
    print({'passed': 'FP3-E: default paths unchanged',
           'FAILED': 'FP3-E: default paths unchanged FAILED',
           'OPEN': 'FP3-E: default paths unchanged OPEN'}[record['status']])
    return record['status']


def historic():
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    result = {}
    runs, _ = reference_runs(D_REFERENCE, payloads)
    for trial, entry in enumerate(runs, 1):
        run = read_pin(entry['full_run_receipt'])
        baseline = collect(Path(entry['case']), baseline=True, event_receipts=run['events'])
        assert baseline['status'] == 'passed', baseline['first_difference']
        result[f'FP3-D S{trial}'] = baseline
    return result


def show(value, digits=3):
    if value is None:
        return 'unavailable'
    if isinstance(value, float):
        return str(round(value, digits))
    return str(value)


def table(heading, columns, rows):
    if not rows:
        return [f'### {heading}', '', 'No rows available.', '']
    return [f'### {heading}', '', '| ' + ' | '.join(columns) + ' |',
            '| ' + ' | '.join('---' for _ in columns) + ' |',
            *['| ' + ' | '.join(map(show, row)) + ' |' for row in rows], '']


def ownership_tables(measurements):
    rows = [(name, row) for name, data in measurements.items()
            for packet in data['worker_stages'] for row in packet['ownership_rows']]
    fields = ('event', 'stage', 'rank', 'site', 'policy', 'owned_ratio', 'predicted_imbalance', 'predicted_gain',
              'attempts', 'migrate', 'committed', 'moved_cells', 'total_seconds', 'bytes_sent',
              'local_cells_before', 'cells', 'construction_entry_to_evaluation_seconds')
    by_rank = table('Ownership rows by worker and rank',
                    ('Run', 'Event', 'Stage', 'Rank', 'Site', 'Policy', 'Owned ratio', 'Raw imbalance', 'Raw gain',
                     'Attempts', 'Migrate', 'Committed', 'Moved cells', 'Seconds', 'Bytes sent',
                     'Rank cells before', 'Rank cells after', 'Entry to eval s'),
                    [(name, *(row.get(key) for key in fields)) for name, row in rows])
    commits = table('Completed exchanges counted once',
                    ('Run', 'Event', 'Stage', 'Site', 'Policy', 'Moved cells', 'Maximum s', 'Sum bytes sent'),
                    [(name, event['event'], exchange['stage'], exchange['site'], exchange['policy'],
                      exchange['moved_cells'], exchange['seconds_max'], exchange['bytes_sent_sum'])
                     for name, data in measurements.items() for event in data['events']
                     for exchange in event.get('exchanges', [])])
    return by_rank + commits


def census_table(measurements):
    rows = [(name, row, row.get('construction_census_delta', {}))
            for name, data in measurements.items() for row in data['worker_stages']]
    return table('Seed, quality entry and construction census by rank',
                 ('Run', 'Event', 'Stage', 'Rank', 'Seed cells', 'Quality entry cells', 'Entry boundary',
                  'Next boundary', 'Calls delta', 'Inclusive seconds delta', 'Exclusive seconds delta'),
                 [(name, row['event'], row['stage'], row['rank'], (row['seed'] or {}).get('cells'),
                   (row.get('quality_entry') or {}).get('cells'), (census.get('entry') or {}).get('boundary'),
                   (census.get('next') or {}).get('boundary'), census.get('calls'),
                   census.get('inclusive_call_seconds'), census.get('exclusive_seconds'))
                  for name, row, census in rows])


def event_tables(measurements):
    rows = [(name, event) for name, data in measurements.items() for event in data['events']]
    return table('Event phase maxima, final proof and host load',
                 ('Run', 'Event', 'Partition UTC', 'Native start UTC', 'Native end UTC',
                  'Native s', 'Construction s', 'Quality s', 'Final proof s', 'Preparation s',
                  'Reverse proof s', 'Share proof s', 'Start load', 'Start sample UTC',
                  'End load', 'End sample UTC'),
                 [(name, event['event'], event.get('partition_utc'), event.get('native_start_utc'),
                   event.get('native_end_utc'), event['native_seconds'],
                   *(event.get('phase_seconds_max', {}).get(key)
                     for key in ('construction', 'quality', 'final_proof', 'preparation')),
                   event.get('final_proof_reverse_seconds_max'), event.get('final_proof_share_seconds_max'),
                   *((event.get('host_load_start') or {}).get(key) for key in ('loadavg', 'sample_utc')),
                   *((event.get('host_load_end') or {}).get(key) for key in ('loadavg', 'sample_utc')))
                  for name, event in rows])


def return_tables(measurements):
    counts = table('Quality returns by event and rank', ('Run', 'Event', 'Rank', 'Count'),
                   [(name, row['event'], rank, count) for name, data in measurements.items()
                    for row in data['events']
                    for rank, count in row.get('construction_reentry_counts_by_rank', {}).items()])
    returns = table('Quality returns by worker and rank',
                    ('Run', 'Event', 'Stage', 'Rank', 'Direction', 'Transition s', 'Subsequent construction s'),
                    [(name, row['event'], row['stage'], row['rank'], entry['direction'],
                      entry.get('transition_seconds'), entry.get('construction_seconds'))
                     for name, data in measurements.items() for row in data['worker_stages']
                     for entry in row['construction_reentries']])
    return counts + returns


def stall_tables(measurements):
    packets, differences = [], []
    for name, data in measurements.items():
        for event in data['events']:
            stall = event.get('stall', {})
            differences.append((name, event['event'], stall.get('observed_debt_comparisons'),
                                str(stall.get('maximum_absolute_debt_difference')),
                                str(stall.get('maximum_relative_debt_difference'))))
            packets.extend(((name, event['event'], packet['worker_stage']), packet)
                           for packet in stall.get('packets', []))
    windows, handoffs, requests, quality, activity, tails, wait, construction, entries = [], [], [], [], [], [], [], [], []
    for prefix, packet in packets:
        for step, row in enumerate(packet['quality_steps'], 1):
            key = (*prefix, step)
            quality.append((*key, row['entry_round'], row['exit_round'], row['end_stage'],
                            row['seconds_max'], row['window_count']))
            for name in ('first_window', 'last_window', 'fired_window'):
                if value := row[name]:
                    windows.append((*key, name.removesuffix('_window'), value['round'], value['observed'],
                                    value['bad'], repr(value['debt']), repr(value['best']), repr(value['best_before']),
                                    value['count'], value['stalled']))
            for rank, counts in sorted(row['by_rank'].items(), key=lambda item: int(item[0])):
                activity.append((*key, rank, counts['private_round_rows'], counts['rounds_with_choices'],
                                 counts['total_choices'], counts['private_busy_seconds']))
                entry = counts['entry']
                entries.append((*key, rank, entry['stage'], entry['round'], *entry['debt'].values()))
            if tail := row['longest_single_rank']:
                count = tail['round_count']
                tails.append((*key, tail['rank'], tail['first_round'], tail['last_round'], count,
                              tail['total_choices'], tail['private_busy_seconds'], tail['private_busy_seconds'] / count))
                for rank, receipt in sorted(tail['other_ranks_join_wait'].items(), key=lambda item: int(item[0])):
                    intervals = ', '.join(f"{span['first_round']}–{span['last_round']}"
                                          for span in receipt['covered_intervals'])
                    wait.append((*key, rank, intervals or 'none', receipt['covered_join_wait_seconds'],
                                 receipt['exact_span_join_wait_seconds']))
        for row in packet['handoffs']:
            handoffs.append((*prefix, row['round'], row['taken'], row['by'], row['failed']))
        for row in packet['request_episodes']:
            requests.append((*prefix, row['fire']['round'], row['resolution'], row.get('preempted_by'),
                             row.get('next_boundary'), row.get('next_boundary_round')))
        for span in packet['construction_after_quality']:
            for rank, row in sorted(span['by_rank'].items(), key=lambda item: int(item[0])):
                if entry := row['construction_entry']:
                    stages = ', '.join(f"{stage['stage']}@{stage['round']}" for stage in row['stages'])
                    construction.append((*prefix, rank, span['quality_exit_round'], entry['round'],
                                         (row.get('end') or {}).get('round'), row.get('rounds'),
                                         row.get('splits'), row.get('seconds'), stages or 'none'))
                    entries.append((*prefix, '—', rank, entry['stage'], entry['round'], *entry['debt'].values()))
    prefix = ('Run', 'Event', 'Worker')
    step = (*prefix, 'Quality step')
    md = table('Stall evidence and independent debt comparison',
               ('Run', 'Event', 'Observed debt comparisons', 'Max absolute difference', 'Max relative difference'),
               differences)
    md += table('First, last and fired stall windows',
                (*step, 'Window', 'Round', 'Observed', 'Bad', 'Debt', 'Best', 'Best four windows earlier', 'Count',
                 'Fired'), windows)
    md += table('Requests and hand-offs', (*prefix, 'Fire round', 'Resolution', 'Preempted by', 'Next phase or exit',
                                           'Next round'), requests)
    md += table('Hand-off rows', (*prefix, 'Round', 'Taken', 'By', 'Failed guard'), handoffs)
    md += table('Quality step boundaries', (*step, 'Entry round', 'Exit round', 'Exit stage', 'Max s', 'Stall windows'),
                quality)
    md += table('Quality choices by rank', (*step, 'Rank', 'Private round rows', 'Rounds with choices',
                                            'Choices', 'Busy s'), activity)
    md += table('Longest span with one active rank', (*step, 'Rank', 'First round', 'Last round', 'Rounds',
                                                      'Choices', 'Busy s', 'Mean busy s per round'), tails)
    md += ['The JSON report includes choices and busy seconds for each round of these spans.', '']
    md += table('Other ranks’ join wait over covered census intervals',
                (*step, 'Rank', 'Covered rounds', 'Covered join wait s', 'Exact span join wait s'), wait)
    md += table('Construction after a quality hand-off', (*prefix, 'Rank', 'Quality exit round',
              'Construction entry round', 'Construction end round', 'Rounds', 'Splits', 'Seconds', 'Stages'), construction)
    md += table('Debt at quality and subsequent construction entries',
                (*step, 'Rank', 'Entry stage', 'Round', 'q debt', 'J debt', 'Size debt', 'Geometry debt'), entries)
    return md


def write_report(label, status, checks, measurements, probes, parts, revert):
    md = [f'# FP3-E build{label}', '', f'**{status}**', '',
          'Full CORE and full OFF are unevaluated under this order.',
          'The [pre-edit source check](source_check.md) records the R4 readings.', '']
    saved = checks.get('saved-current', {})
    if saved.get('status') == 'passed':
        md += ['The two 126,680,744-byte saved-current fixture files are retained locally with their SHA256 pins. '
               f'[Saved-current receipt](../controls/build{label}/saved-current/case_summary.json)', '']
    watchdog = HERE / f'controls/build{label}/exact-c492/bl/first_watchdog_receipt.json'
    if watchdog.is_file():
        seconds = cut.read(watchdog)['watchdog']['seconds']
        md += [f'C492 reached the {seconds} s helper watchdog without a recorded result finding. Its prescribed '
               f'{3 * seconds} s helper retry used the same native limits and effective probe value. '
               f'[First watchdog receipt](../controls/build{label}/exact-c492/bl/first_watchdog_receipt.json)', '']
    if revert:
        md += [f'Production change reverted from `{revert}`. The quality step of FP3-D stays.', '']
    md += table('Checks', ('Check', 'Status', 'Finding'),
                [(key, row['status'], row.get('first_difference') or '') for key, row in checks.items()])
    md += [f'[Step 1 seal](../build{label}/step1.json) · '
           f'[S1 seal](../G5/build{label}/S1/case_summary.json) · '
           f'[S2 seal](../G5/build{label}/S2/case_summary.json)', '']
    comparisons = [(name, 'FP3-D S', parts[name].get('historical_comparison')) for name in ('S1', 'S2')]
    comparisons.append(('S2', 'S1', parts['S2'].get('comparison')))
    md += table('Six-payload comparisons', ('Run', 'Reference', 'Differing files'),
                [(name, reference, len(row['differences'])) for name, reference, row in comparisons if row is not None])
    md += table('Full S event results', ('Run', 'Event', 'Workers', 'Native seconds', 'Cells', 'Points',
                                         'Surface faces', 'Job peak bytes', 'Minimum coverage %'),
                [(name, row['event'], row['worker_stage_count'], row['native_seconds'], row['final_cells'],
                  row['final_points'], row['final_surface_faces'], row['job_peak_rss_bytes'],
                  None if row['minimum_timer_coverage'] is None else 100 * row['minimum_timer_coverage'])
                 for name, data in measurements.items() for row in data['events']])
    md += ['', 'Core `bad_out` uses nodal tensors. Engine `bad_union` uses centroid-target q and J.',
           'Each time maximum is taken over the participating ranks. Hand-over intervals use whole-second '
           'launcher timestamps. Short-edge sums count rank-local incidences.', '']
    md += table('S worker stages', ('Run', 'Event', 'Stage', 'Ranks', 'Hand-over to initial exit s (max)',
                                    'Raw collapse elapsed s (max)', 'Collapse pass s (max)',
                                    'Core bad_out sum', 'Engine bad_union sum', 'Quality seconds (max)',
                                    'Core short edges', 'Short edges at initial exit', 'Short edges at last length row'),
                [(name, event['event'], row['stage'], row['ranks'], row['handover_to_first_collapse_exit_seconds_max'],
                  row['collapse_length_elapsed_seconds_max'], row['collapse_pass_seconds_max']
                  if row['collapse_pass_seconds_max'] is not None else 'pass time not available',
                  row['core_last_pass_bad_out_sum'], row['quality_entry_bad_union_sum'], row['quality_seconds_max'],
                  row['core_last_pass_short_edges_sum'], row['short_edges_at_initial_collapse_exit_sum'],
                  row['short_edges_at_last_length_boundary_sum'])
                 for name, data in measurements.items() for event in data['events'] for row in event['worker_stages']])
    md += table('Split generations by rank', ('Run', 'Event', 'Stage', 'Rank', 'Generation sequence', 'Cumulative splits', 'Seconds'),
                [(name, row['event'], row['stage'], row['rank'], i, generation['split_accepted'],
                  generation['seconds_since_previous_length_boundary']) for name, data in measurements.items()
                 for row in data['worker_stages'] for i, generation in enumerate(row['split_generations'])])
    md += table('Completed metric cleanup counts', ('Run', 'Rank-local pairs'),
                [(name, sum(len(row['metric_cleanups']) for row in data['worker_stages']))
                 for name, data in measurements.items()])
    md += table('Metric cleanup by rank', ('Run', 'Event', 'Stage', 'Rank', 'Entry round', 'Exit round', 'Accepted'),
                [(name, row['event'], row['stage'], row['rank'], cleanup['entry']['rounds'],
                  cleanup['exit']['rounds'], cleanup['accepted']) for name, data in measurements.items()
                 for row in data['worker_stages'] for cleanup in row['metric_cleanups']])
    md += ['Moved-cell counts are global and repeated in the per-rank commit rows.', '']
    md += ownership_tables(measurements)
    md += census_table(measurements)
    md += return_tables(measurements)
    md += stall_tables(measurements)
    md += ['Native starts are reconstructed from the native maximum and the logged remesh end. '
           'The load columns use actual samples nearest to those boundaries. Host load was recorded and '
           'uncontrolled. Rank maxima for nested phases are reported independently.', '']
    md += event_tables(measurements)
    md += table('Final proof union memo', ('Run', 'Event', 'Union work', 'Hits', 'Upper calls', 'Hit %'),
                [(name, event['event'], event.get('final_proof_union_work'),
                  event.get('final_proof_union_memo_hits'), event.get('final_proof_union_calls'),
                  None if fraction is None else 100 * fraction)
                 for name, data in measurements.items() for event in data['events']
                 for fraction in (event.get('final_proof_union_memo_hit_fraction'),)])
    md += table('Final proof by rank', ('Run', 'Event', 'Rank', 'Reverse s', 'Share s', 'Union work', 'Memo hits'),
                [(name, event['event'], row['rank'], row.get('reverse_seconds'), row.get('share_seconds'),
                  row.get('union_work'), row.get('union_memo_hits'))
                 for name, data in measurements.items() for event in data['events']
                 for row in event.get('final_proof_by_rank', [])])
    row = parts['probe4']
    data = probes.get('probe4', {})
    md += ['## The quality step of FP3-D', '',
           f'Observation status **{row["status"]}**.', '']
    if row.get('first_difference'):
        md += [f'First observation finding: `{row["first_difference"]}`.', '']
    timed = {name: measurements[name] for name in ('S1', 'S2') if name in measurements}
    if data:
        timed['FP3-D quality step, probe 4'] = data
    md += ['Host load was recorded and uncontrolled. The timing comparison is observational.', '']
    md += event_tables(timed)
    if data:
        md += stall_tables({'FP3-D quality step, probe 4': data})
    if row.get('comparison') is not None:
        comparison = row['comparison']
        md += [f'Compared all six payload hashes with the FP3-D S reference. '
               f'Differing files **{len(comparison["differences"])}**.', '']
    (HERE / f'reports/FP3_E_{label}.md').write_text('\n'.join(md).rstrip() + '\n')


def report(label, validated_commit=None, revert=None):
    directory = HERE / f'build{label}'
    build = cut.read(directory / 'build_status.json')
    scoped_path = HERE / 'controls' / directory.name / 'step1_scoped.json'
    scoped = read_part(scoped_path, build)
    step = read_part(directory / 'step1.json', build)
    parts = {name: read_part(root(label, name) / 'case_summary.json', build)
             for name in ('S1', 'S2', 'probe4')}
    checks = {'build': {'status': 'passed' if build['passed'] else 'FAILED'},
              **scoped.get('parts', {}), 'Step 1': step,
              **{name: parts[name] for name in ('S1', 'S2')},
              'full CORE workflow': {'status': 'not evaluated'},
              'full OFF workflow': {'status': 'not evaluated'}}
    results = {row['status'] for row in checks.values()}
    accepted = all(row['status'] == 'passed' for row in (step, parts['S1'], parts['S2']))
    status = ('FP3-E: FAILED; the quality step of FP3-D stays' if revert else 'FP3-E: FAILED' if 'FAILED' in results else
              'FP3-E: OPEN' if 'OPEN' in results else
              'FP3-E: accepted; a stalled quality step requests the hand-off to construction in mode S' if accepted else 'FP3-E: not evaluated')
    measurements = historic() if any(row.get('collection') for row in parts.values()) else {}
    probes = {}
    for name, row in parts.items():
        if row.get('collection'):
            collection = read_pin(row['collection'])
            (probes if name.startswith('probe') else measurements)[name] = collection
        elif row['status'] != 'not evaluated':
            partial = sorted(root(label, name).glob('**/partial_collection.json'))
            full = sorted(root(label, name).glob('**/collection.json'))
            if full or partial:
                selected = (full or partial)[-1]
                assert row['pins'][str(selected)] == cut.sha(selected)
                (probes if name.startswith('probe') else measurements)[name] = cut.read(selected)
    def brief(row):
        return {key: value for key, value in row.items() if key != 'pins'}

    record = {'status': status, 'build': label, 'reverted_production_commit': revert,
              'source_check': path_pin(HERE / 'reports/source_check.md'),
              'checks': {name: brief(row) for name, row in checks.items()},
              'measurements': measurements, 'probe_runs': {name: brief(parts[name]) for name in ('probe4',)},
              'probe_measurements': probes}
    cut.save(HERE / f'reports/FP3_E_{label}.json', record)
    write_report(label, status, checks, measurements, probes, parts, revert)
    if validated_commit:
        verify_build(label)
        assert accepted and not revert
        assert all(parts[name]['status'] != 'not evaluated' for name in ('probe4',))
        work_order = cut.read(directory / 'source_pins.json')[str(cut.REPO / 'AGENTS.md')]
        data = subprocess.check_output(['git', 'show', f'{validated_commit}:AGENTS.md'], cwd=cut.REPO)
        assert hashlib.sha256(data).hexdigest() == work_order
        runs = []
        pins = step['pins'].copy()
        for name in ('S1', 'S2'):
            summary = parts[name]
            run = read_pin(summary['run'])
            case = Path(run['case'])
            assert all(cut.sha(case / name) == digest for name, digest in run['hashes'].items())
            assert len(run['hashes']) == 6
            runs.append({'case': str(case), 'full_run_receipt': summary['run'],
                         'isolated_restart_receipt': path_pin(Path(run['restart'])),
                         'effective_config': path_pin(case / 'run.cfg'), 'payload_hashes': run['hashes'],
                         'native_maximum_seconds': {str(event['event']): event['native_seconds']
                                                    for event in run['events']}})
            pins.update(summary['pins'])
        assert runs[0]['payload_hashes'] == runs[1]['payload_hashes']
        assert runs[0]['effective_config']['sha256'] == runs[1]['effective_config']['sha256']
        check_pins(pins)
        if str(cut.REPO / 'AGENTS.md') in pins:
            assert pins.pop(str(cut.REPO / 'AGENTS.md')) == work_order
        cut.save(HERE / 'accepted_reference.json', {
            **{key: build[key] for key in FINGERPRINTS}, 'accepted': True, 'accepted_commit': validated_commit,
            'order_sha256': ORDER_SHA, 'validation_work_order': {
                'commit': validated_commit, 'path': 'AGENTS.md', 'sha256': work_order},
            'gate_receipts': {
                'step1': path_pin(directory / 'step1.json'),
                'S1': path_pin(root(label, 'S1') / 'case_summary.json'),
                'S2': path_pin(root(label, 'S2') / 'case_summary.json'),
            },
            'workflows': {'S': {'reference_case': runs[0]['case'], 'runs': runs}}, 'pins': pins})
    print(status)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('--step1', action='store_true')
    parser.add_argument('--validated-commit')
    parser.add_argument('--revert')
    args = parser.parse_args()
    if args.step1:
        raise SystemExit(0 if step1(args.label) == 'passed' else 1)
    report(args.label, args.validated_commit, args.revert)
