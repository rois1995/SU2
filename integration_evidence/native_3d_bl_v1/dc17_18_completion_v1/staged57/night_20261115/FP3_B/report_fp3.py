"""Seal Step 1 and publish the FP3-B result from its selected receipts."""

from argparse import ArgumentParser
import hashlib
from pathlib import Path
import re
import runpy
import subprocess

from build_fp3 import HERE, cut, verify_build
from collect_fp3_full import ENGINE_COUNTS, collect
from run_fp3_full import A_REFERENCE, FINGERPRINTS, ORDER_SHA, P3_REFERENCE, check_pins, path_pin, reference_runs, root, workflow


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
    print(record['status'])
    return record['status']


def add_final_memo(case, collection):
    """Project the finalized union memo counters as report-only observations."""
    events = collection['events']
    totals = [[0, 0] for event in events]
    logs = sorted(case.glob('rank_output/*/rank.*/stdout'))
    if len(logs) != 6:
        return collection
    for log in logs:
        rank = int(log.parent.name.removeprefix('rank.'))
        rows = []
        for line in log.read_text(errors='replace').splitlines():
            if f'D9_QUERY rank={rank} phase=final_proof site=g ' not in line:
                continue
            fields = dict(re.findall(r'(\w+)=(\d+)(?=\s|$)', line))
            if not {'memo_union_hits', 'upper_calls'} <= fields.keys():
                return collection
            rows.append((int(fields['memo_union_hits']), int(fields['upper_calls'])))
        if len(rows) != len(events):
            return collection
        for total, (hits, calls) in zip(totals, rows, strict=True):
            total[0] += hits
            total[1] += calls
    for event, (hits, calls) in zip(events, totals, strict=True):
        event['final_proof_union_memo'] = {'hits': hits, 'upper_calls': calls, 'fraction': hits / calls if calls else None}
    return collection


def historic():
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    result = {}
    for key, reference in (('P3', P3_REFERENCE), ('A', A_REFERENCE)):
        runs, _ = reference_runs(reference, payloads)
        for trial, entry in enumerate(runs, 1):
            run = read_pin(entry['full_run_receipt'])
            baseline = collect(Path(entry['case']), baseline=True, event_receipts=run['events'])
            assert baseline['status'] == 'passed', baseline['first_difference']
            result[f'{key} S{trial}'] = add_final_memo(Path(entry['case']), baseline)
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
    return table('Ownership rows by worker and rank',
                 ('Run', 'Event', 'Stage', 'Rank', 'Quality', 'Evaluate', 'Migrate', 'Committed',
                  'Attempts', 'Moved cells', 'Predicted gain', 'Predicted cost', 'Seconds'),
                 [(name, *(row.get(key) for key in ('event', 'stage', 'rank', 'quality', 'evaluate', 'migrate',
                                                   'committed', 'attempts', 'moved_cells', 'predicted_gain',
                                                   'predicted_cost', 'total_seconds'))) for name, row in rows])


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


def write_report(label, status, checks, measurements, probes, parts, revert):
    md = [f'# FP3-B build{label}', '', f'**{status}**', '',
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
        md += [f'Production change reverted from `{revert}`. The FP3-A ownership rule stays.', '']
    md += table('Checks', ('Check', 'Status', 'Finding'),
                [(key, row['status'], row.get('first_difference') or '') for key, row in checks.items()])
    md += [f'[Step 1 seal](../build{label}/step1.json) · '
           f'[S1 seal](../G5/build{label}/S1/case_summary.json) · '
           f'[S2 seal](../G5/build{label}/S2/case_summary.json)', '']
    comparisons = [(name, 'FP3-A S', parts[name].get('historical_comparison')) for name in ('S1', 'S2')]
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
    md += return_tables(measurements)
    md += ['Native starts are reconstructed from the native maximum and the logged remesh end. '
           'The load columns use actual samples nearest to those boundaries. Host load was recorded and '
           'uncontrolled. Rank maxima for nested phases are reported independently.', '']
    md += event_tables(measurements)
    md += table('Final proof union memo', ('Run', 'Event', 'Hits', 'Upper calls', 'Hit %'),
                [(name, event['event'], memo.get('hits'), memo.get('upper_calls'),
                  None if memo.get('fraction') is None else 100 * memo['fraction'])
                 for name, data in measurements.items() for event in data['events']
                 for memo in (event.get('final_proof_union_memo', {}),)])
    row = parts['probe2']
    data = probes.get('probe2', {})
    md += ['## The behaviour of P3 with the accepted-collapse probe active', '',
           f'Observation status **{row.get("probe_status", row["status"])}**.', '']
    timed = {name: measurements[name] for name in ('S1', 'S2') if name in measurements}
    if data:
        timed['P3 behaviour, probe 2'] = data
    stamps = [event.get(key) for run in timed.values() for event in run['events']
              for key in ('native_start_utc', 'native_end_utc')]
    if len(timed) == 3 and len(stamps) == 12 and all(stamps):
        dates = {stamp[:10] for stamp in stamps}
        if len(dates) == 1:
            md += [f'All three workflows were observed on {next(iter(dates))} UTC.', '']
    md += event_tables(timed)
    if row.get('comparison') is not None:
        comparison = row['comparison']
        md += [f'Compared all six payload hashes with the FP2-P3 S reference. '
               f'Differing files **{len(comparison["differences"])}**.', '']
    md += table('Accepted-collapse observations by rank', ('Event', 'Stage', 'Rank', 'Status', *ENGINE_COUNTS),
                [(entry['event'], entry['stage'], entry['rank'], entry['status'],
                  *(entry.get(field) for field in ENGINE_COUNTS)) for entry in data.get('probe_rows', [])])
    (HERE / f'reports/FP3_B_{label}.md').write_text('\n'.join(md).rstrip() + '\n')


def report(label, validated_commit=None, revert=None):
    directory = HERE / f'build{label}'
    build = cut.read(directory / 'build_status.json')
    scoped_path = HERE / 'controls' / directory.name / 'step1_scoped.json'
    scoped = read_part(scoped_path, build)
    step = read_part(directory / 'step1.json', build)
    parts = {name: read_part(root(label, name) / 'case_summary.json', build)
             for name in ('S1', 'S2', 'probe2')}
    checks = {'build': {'status': 'passed' if build['passed'] else 'FAILED'},
              **scoped.get('parts', {}), 'Step 1': step,
              **{name: parts[name] for name in ('S1', 'S2')},
              'full CORE workflow': {'status': 'not evaluated'},
              'full OFF workflow': {'status': 'not evaluated'}}
    results = {row['status'] for row in checks.values()}
    accepted = all(row['status'] == 'passed' for row in (step, parts['S1'], parts['S2']))
    status = ('FP3-B: FAILED; the exchange rule stays' if revert else 'FP3-B: FAILED' if 'FAILED' in results else
              'FP3-B: OPEN' if 'OPEN' in results else
              'FP3-B: accepted; no exchange at the empty collapse exit' if accepted else 'FP3-B: not evaluated')
    measurements = historic()
    probes = {}
    for name, row in parts.items():
        if row.get('collection'):
            collection = read_pin(row['collection'])
            if row.get('run'):
                collection = add_final_memo(Path(read_pin(row['run'])['case']), collection)
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
              'checks': {name: brief(row) for name, row in checks.items()},
              'measurements': measurements, 'probe_runs': {name: brief(parts[name]) for name in ('probe2',)},
              'probe_measurements': probes}
    cut.save(HERE / f'reports/FP3_B_{label}.json', record)
    write_report(label, status, checks, measurements, probes, parts, revert)
    if validated_commit:
        verify_build(label)
        assert accepted and not revert
        assert all(parts[name]['status'] != 'not evaluated' for name in ('probe2',))
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
