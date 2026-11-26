"""Seal Step 1 and publish the FP3-A result from its selected receipts."""

from argparse import ArgumentParser
import hashlib
from pathlib import Path
import subprocess

from build_fp3 import HERE, cut, verify_build
from collect_fp3_full import CORE_COUNTS, ENGINE_COUNTS, collect
from run_fp3_full import FINGERPRINTS, ORDER_SHA, REFERENCE, check_pins, path_pin, root


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
    core = root(label, 'CORE') / 'case_summary.json'
    parts = {key: read_part(path, build) for key, path in (('scoped', scope), ('CORE', core))}
    failed = [row for row in parts.values() if row['status'] == 'FAILED']
    states = {row['status'] for row in parts.values()}
    finding = next((row for row in failed if row['failure_kind'] == 'result'), failed[0] if failed else {})
    status = ('FAILED' if failed else 'OPEN' if 'OPEN' in states else
              'passed' if states == {'passed'} else 'not evaluated')
    assert status != 'not evaluated', parts
    pins = {}
    for path, row in zip((scope, core), parts.values(), strict=True):
        if path.is_file():
            pins.update({str(path): cut.sha(path), **row['pins']})
    record = {key: build[key] for key in FINGERPRINTS}
    record.update(status=status, passed=status == 'passed', part='step1', pins=pins,
                  parts={name: row['status'] for name, row in parts.items()},
                  failure_kind=finding.get('failure_kind'), first_difference=finding.get('first_difference'))
    path = HERE / f'build{label}/step1.json'
    if path.is_file():
        assert cut.read(path) == record, path
    else:
        cut.save(path, record)
    print(status)
    return status


def historic():
    result = {}
    for trial, reference in enumerate(cut.read(REFERENCE)['workflows']['S']['runs'], 1):
        run = read_pin(reference['full_run_receipt'])
        work = Path(reference['full_run_receipt']['path']).parent
        baseline = collect(Path(reference['case']), baseline=True, event_receipts=run['events'])
        assert baseline['status'] == 'passed', baseline['first_difference']
        for name, digest in baseline['log_sha256'].items():
            path = Path(name)
            if not path.is_absolute():
                path = Path(reference['case']) / path
            assert run['artifacts'][str(path.relative_to(work))] == digest == cut.sha(path), path
        result[f'P3 S{trial}'] = baseline
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


def write_report(label, status, checks, measurements, probes, parts, revert):
    md = [f'# FP3-A build{label}', '', f'**{status}**', '',
          'The full OFF workflow was not evaluated under this order.',
          'The [pre-edit source check](source_check.md) records the R4 readings.', '']
    fixture_failure = HERE / 'controls/build674/hidden/attempt1/test_outer.log'
    if fixture_failure.is_file() and label != 674:
        md += ['Build674 ran two hidden cases successfully. The mapped fixture stopped during authority construction '
               'because two shared Bernstein controls were transposed. Build675 corrects only their fixture order. '
               'The candidate production sources and all admission values are identical. '
               '[Raw Catch log](../controls/build674/hidden/attempt1/test_outer.log)', '']
    supervisor_log = HERE / f'controls/build{label}/scoped_initial_supervisor.log'
    if supervisor_log.is_file():
        md += ['The first scoped supervisor selected the wrong historical module after first-collapse had finished. '
               'The continuation bound the P3 evaluator by path, verified the terminal receipts, and sealed the '
               'same execution. [Initial supervisor log](../controls/build'
               f'{label}/scoped_initial_supervisor.log)', '']
    if revert:
        md += [f'Production change reverted from `{revert}`. The engine construction-collapse pass stays.', '']
    md += table('Checks', ('Check', 'Status', 'Finding'),
                [(key, row['status'], row.get('first_difference') or '') for key, row in checks.items()])
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
    md += table('Split generations by rank', ('Run', 'Event', 'Stage', 'Rank', 'Generation', 'Accepted', 'Seconds'),
                [(name, row['event'], row['stage'], row['rank'], i, generation['split_accepted'],
                  generation['seconds_since_previous_length_boundary']) for name, data in measurements.items()
                 for row in data['worker_stages'] for i, generation in enumerate(row['split_generations'])])
    md += table('Metric cleanup by rank', ('Run', 'Event', 'Stage', 'Rank', 'Entry round', 'Exit round', 'Accepted'),
                [(name, row['event'], row['stage'], row['rank'], cleanup['entry']['rounds'],
                  cleanup['exit']['rounds'], cleanup['accepted']) for name, data in measurements.items()
                 for row in data['worker_stages'] for cleanup in row['metric_cleanups']])
    md += table('Time rule', ('Run', 'Event', 'Native seconds', 'Faster P3 seconds', '10% threshold s', 'Status'),
                [(name, row['event'], row['candidate_seconds'], row['faster_p3_seconds'], row['at_most_seconds'],
                  row['status']) for name in ('S1', 'S2') for row in parts[name].get('time', [])])
    for name, fields in (('probe1', CORE_COUNTS), ('probe2', ENGINE_COUNTS)):
        row = parts[name]
        data = probes.get(name, {})
        comparison = row.get('comparison')
        md += [f'## {name}', '', f'Observation status **{row.get("probe_status", row["status"])}**.', '']
        if comparison is not None:
            md += [f'Six-payload comparison equal **{comparison["equal"]}**.', '']
        if name == 'probe1':
            md += ['Totals sum rank-local edge incidences. Proof was not evaluated by the probe.', '']
        md += table('By event, worker stage and rank', ('Event', 'Stage', 'Rank', 'Status', *fields),
                    [(entry['event'], entry['stage'], entry['rank'], entry['status'],
                      *(entry.get(field) for field in fields)) for entry in data.get('probe_rows', [])])
        md += table('Totals', ('Event', 'Stage', 'Ranks reported', 'Status', *fields),
                    [(entry['event'], entry['stage'], entry['ranks_reported'], entry['status'],
                      *((entry['counts'] or {}).get(field) for field in fields))
                     for entry in data.get('probe_totals', [])])
    (HERE / f'reports/FP3_A_{label}.md').write_text('\n'.join(md) + '\n')


def report(label, validated_commit=None, revert=None):
    directory = HERE / f'build{label}'
    build = cut.read(directory / 'build_status.json')
    scoped_path = HERE / 'controls' / directory.name / 'step1_scoped.json'
    scoped = read_part(scoped_path, build)
    step = read_part(directory / 'step1.json', build)
    parts = {name: read_part(root(label, name) / 'case_summary.json', build)
             for name in ('CORE', 'S1', 'S2', 'probe1', 'probe2')}
    checks = {'build': {'status': 'passed' if build['passed'] else 'FAILED'},
              **scoped.get('parts', {}), 'Step 1': step,
              **{name: parts[name] for name in ('CORE', 'S1', 'S2')},
              'full OFF workflow': {'status': 'not evaluated'}}
    results = {row['status'] for row in checks.values()}
    accepted = all(row['status'] == 'passed' for row in (step, parts['S1'], parts['S2']))
    status = ('FP3-A: FAILED; the engine pass stays' if revert else 'FP3-A: FAILED' if 'FAILED' in results else
              'FP3-A: OPEN' if 'OPEN' in results else
              'FP3-A: accepted; no engine construction collapse in mode S' if accepted else 'FP3-A: not evaluated')
    measurements = historic()
    probes = {}
    for name, row in parts.items():
        if row.get('collection'):
            (probes if name.startswith('probe') else measurements)[name] = read_pin(row['collection'])
        elif name != 'CORE' and row['status'] != 'not evaluated':
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
              'measurements': measurements, 'probe_runs': {name: brief(parts[name]) for name in ('probe1', 'probe2')},
              'probe_measurements': probes}
    cut.save(HERE / f'reports/FP3_A_{label}.json', record)
    write_report(label, status, checks, measurements, probes, parts, revert)
    if validated_commit:
        verify_build(label)
        assert accepted and not revert
        assert all(parts[name]['status'] != 'not evaluated' for name in ('probe1', 'probe2'))
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
