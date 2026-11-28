"""Seal SC1 count measurements and report all standing and mode-S results."""

from argparse import ArgumentParser
import json
import os
from pathlib import Path
import runpy

from run_full import E, HERE, NIGHT, ORDER_SHA, cut, path_pin, prior, read_run, root, sha, verify_build, workflow
from collect_sc1 import PHASES, collect, count_measurements

SOURCE_CHECK = HERE / 'reports/source_check.md'


def stages(label):
    folder = HERE / f'build{label}'
    return folder, cut.read(folder / 'build_status.json')


def seal_count(label, count_from=None):
    folder, build = stages(label)
    assert build == verify_build(label)
    scoped = HERE / 'controls' / folder.name / 'step1_scoped.json'
    E.require(scoped, build)
    first_build = build if count_from is None else stages(count_from)[1]
    first, _, run = read_run(first_build['build'], 'count', first_build)
    measured = count_measurements(Path(run['case']))
    first_count = {'receipt': path_pin(first), **measured}
    selected = first_count
    state = {'first_count': first_count, 'repeat': None}
    prerequisites = [scoped, first, SOURCE_CHECK, Path(__file__), HERE / 'collect_sc1.py',
                     folder / 'build_status.json']
    if count_from is not None:
        assert count_from != label and measured['repeat_required'], first
        assert first_build['binary_sha256'] != build['binary_sha256']
        repeat, repeat_summary, run = read_run(label, 'count_repeat', build)
        assert repeat_summary['count_from'] == count_from, repeat
        assert repeat_summary['pins'][str(first)] == sha(first), repeat
        prerequisites += [repeat, stages(count_from)[0] / 'build_status.json']
        selected = {'receipt': path_pin(repeat), **count_measurements(Path(run['case']))}
        state['repeat'] = selected

    ready = count_from is not None or not measured['repeat_required']
    state.update({key: build[key] for key in E.FINGERPRINTS})
    state.update(status='passed' if ready else 'repeat_required', passed=ready,
                 minimum_predicted_coverage=selected['minimum_predicted_coverage'],
                 pins={str(path): sha(path) for path in prerequisites})
    target = folder / ('step2.json' if ready else 'step2_attribution_required.json')
    workflow.sealed_write(target, state)
    print('SC1: count mode measured' if ready else 'SC1 count requires attribution and one repeat')
    print(f'Minimum predicted coverage {state["minimum_predicted_coverage"]:.6%}. {target}', flush=True)
    return ready


def checked(pointer):
    path = Path(pointer['path'])
    assert path_pin(path) == pointer, path
    return cut.read(path)


def read_part(path, build):
    if not path.is_file():
        return {'status': 'not evaluated'}
    row = cut.read(path)
    assert all(row[key] == build[key] for key in E.FINGERPRINTS), path
    E.check_pins(row['pins'])
    return {key: value for key, value in row.items() if key != 'pins'} | {'receipt': path_pin(path)}


def new_reference(label, build, selected, scoped, count, diagnostics):
    runs, pins, gate_receipts = [], {}, {'step1': path_pin(scoped), 'step2': path_pin(count)}
    work_order = cut.REPO / 'AGENTS.md'
    order_sha = cut.read(HERE / f'build{label}/source_pins.json')[str(work_order)]
    source_commit = build['source_commit']
    from build_fp3 import git

    assert prior.sha(work_order) == order_sha
    assert git('show', f'{source_commit}:AGENTS.md') == work_order.read_bytes()
    for part in ('S1', 'S2', 'probe5'):
        path, summary, row = read_run(label, part, build)
        gate_receipts[part] = path_pin(path)
        pins.update(summary['pins'])
        if part == 'probe5':
            continue
        case = Path(row['case'])
        assert row['hashes'] == selected and all(sha(case / name) == digest for name, digest in selected.items())
        runs.append({'case': str(case), 'payload_hashes': selected,
                     'full_run_receipt': summary['run'],
                     'isolated_restart_receipt': path_pin(Path(row['restart'])),
                     'effective_config': path_pin(case / 'run.cfg'),
                     'native_maximum_seconds': {str(event['event']): event['native_seconds'] for event in row['events']}})
    for path in (scoped, count):
        pins.update(cut.read(path)['pins'])
    pins.pop(str(work_order), None)
    pins.update({str(diagnostic): sha(diagnostic) for diagnostic in diagnostics})
    E.check_pins(pins)
    workflow.sealed_write(HERE / 'accepted_reference.json', {
        **{key: build[key] for key in E.FINGERPRINTS}, 'accepted': True, 'accepted_commit': source_commit,
        'source_commit': source_commit, 'order_sha256': ORDER_SHA,
        'validation_work_order': {'commit': source_commit, 'path': 'AGENTS.md', 'sha256': order_sha},
        'gate_receipts': gate_receipts, 'workflows': {'S': {'reference_case': runs[0]['case'], 'runs': runs}}, 'pins': pins})


def relative(pointer, report):
    return os.path.relpath(pointer['path'], report.parent)


def display(value, digits=4):
    if value is None:
        return 'unavailable'
    if isinstance(value, float):
        return f'{value:.{digits}f}'
    return str(value)


def table(title, columns, rows):
    if not rows:
        return []
    return [f'### {title}', '', '| ' + ' | '.join(columns) + ' |',
            '| ' + ' | '.join(['---'] * len(columns)) + ' |',
            *['| ' + ' | '.join(map(display, row)) + ' |' for row in rows], '']


def write_report(data, report):
    lines = [
        f'# SC1 build {data["build"]}', '', f'**{data["label"]}**', '',
        f'Source commit `{data["source_commit"]}`. Solver SHA256 `{data["binary_sha256"]}`.',
        f'[R4 whole-item source check]({relative(data["source_check"], report)}) · '
        f'[FP3-E accepted S]({relative(data["reference"], report)})', '',
    ]
    if data['revert']:
        lines += [f'Restored the previous production behavior in `{data["revert"]}`.', '']
    lines += table('Gates', ('Check', 'Status', 'Receipt'), [
        (name, row['status'], f'[Receipt]({relative(row["receipt"], report)})' if row.get('receipt') else '—')
        for name, row in data['checks'].items()])
    for name, row in data['checks'].items():
        if row.get('first_difference') is not None:
            lines.extend([f'First difference in {name}.', '', '```json',
                          json.dumps(row['first_difference'], indent=2), '```', ''])
    if data['failure']:
        lines.extend(['Final failure receipt.', '', '```json', json.dumps(data['failure'], indent=2), '```', ''])

    lines += table('Standing S workflow and isolated restart',
                   ('Run', 'Event', 'Native s', 'Sum of process peaks MiB', 'Coverage %', 'Proof faces',
                    'Maximum join difference', 'Isolated restart'),
                   [(name, event['event'], event['native_seconds'], event['job_peak_rss_bytes'] / 2**20,
                     100 * event['minimum_timer_coverage'], event['proof_faces'],
                     f'{event["maximum_join_relative_difference"]:.6g}', 'passed' if event['event'] == 2 else '—')
                    for name, row in data['workflows'].items() for event in row['events']])
    timings, facets, work, partitions, assignments, certs = [], [], [], [], [], []
    for name, rows in data['measurements'].items():
        for row in rows:
            key = (name, row['event'], row['rank'])
            cert = row.get('certificates')
            timings.append((*key, row['root_seconds'], row['attributed_seconds'], row['unattributed_seconds'],
                            100 * row['coverage'], cert.get('skip_seconds') if cert else None,
                            100 * cert['predicted_coverage'] if cert and 'predicted_coverage' in cert else None))
            for stage, engine in enumerate(row['engine']):
                limit = engine['atlas_work_limit']
                if limit is None and stage == len(row['engine']) - 1:
                    limit = row.get('atlas_work_limit')
                work.append((*key, stage, engine['seed'], engine['atlas_arithmetic_work'], limit, engine['work_limit']))
            for stage, pool in enumerate(row['seed_facet_pool']):
                mean = pool['prepared_facet_mean_bytes']
                distinct = pool['distinct_facets']
                ratio = int(distinct) * float(mean) / 175_451_136 if mean and distinct else None
                facets.append((*key, stage, distinct, mean, ratio, pool['facet_misses'],
                               pool['prepared_facet_builds'], pool['facet_build_seconds'], pool['retention_refusals']))
            p = row['event_partition_weights']
            partitions.append((*key, p.get('metric_before'), p.get('metric_after'), p.get('geometry_before'),
                               p.get('geometry_after'), f'`{row["partition_summary_sha256"]}`'))
            if row['partition_assignments']:
                assignments.extend((*key, item['site'], item['round'], item['cells'],
                                    item['assignments_digest']) for item in row['partition_assignments'])
            else:
                assignments.append((*key, '—', '—', '—', None))
            if cert:
                certs.append((*key, cert['reader_calls'], cert['cert_hit'], cert['cert_miss'], cert['hit_step_c'],
                              cert['hit_proof'], cert['hit_commit'], cert.get('disagree_finite'),
                              cert.get('disagree_nonfinite'), cert.get('skip_calls'), cert['records'], cert['bytes'],
                              cert['gather_seconds']))
    lines += table('Rank timer coverage', ('Run', 'Event', 'Rank', 'Root s', 'Attributed s', 'Unattributed s',
                                          'Coverage %', 'Skip s', 'Predicted coverage %'), timings)
    lines += table('Seed census and atlas work', ('Run', 'Event', 'Rank', 'Stage', 'Seed s', 'Atlas work',
                                                 'Effective atlas limit', 'Engine work limit'), work)
    lines += table('Seed facet pool', ('Run', 'Event', 'Rank', 'Stage', 'Distinct facets', 'Mean bytes',
                                     'Set / 175.45 MB', 'Census misses', 'Event builds', 'Build s',
                                     'Retention refusals'), facets)
    lines += table('Event partition weights and summary fingerprints',
                   ('Run', 'Event', 'Rank', 'Metric before', 'Metric after', 'Geometry before', 'Geometry after',
                    'Summary SHA256'), partitions)
    lines += table('Partition assignment diagnostics',
                   ('Run', 'Event', 'Rank', 'Site', 'Round', 'Cells', 'Assignment digest'), assignments)
    lines += table('Certificate counters', ('Run', 'Event', 'Rank', 'Reader calls', 'Hits', 'Misses', 'Step C', 'Proof',
                                            'Commit', 'Finite disagreements', 'Nonfinite disagreements', 'Skip calls',
                                            'Records', 'Bytes', 'Gather s'), certs)
    if data['count']:
        lines += ['### Count coverage attribution', '']
        for key in ('first_count', 'repeat'):
            if measured := data['count'][key]:
                lines += [f'- {key.replace("_", " ").capitalize()}. Minimum predicted coverage '
                          f'{measured["minimum_predicted_coverage"]:.6%}.']
        lines += ['', 'The JSON report includes every per-phase total, longest gap boundary, '
                      'individual disagreeing face and engine ownership decision.', '']
        gaps, disagreements = [], []
        for name, rows in data['measurements'].items():
            if not name.startswith('count'):
                continue
            for row in rows:
                cert = row['certificates']
                for phase in PHASES:
                    values = [span for span in cert['spans'] if span['phase'] == phase]
                    for span in values:
                        gaps.append((name, row['event'], row['rank'], phase, row['unattributed_by_phase'][phase],
                                     span['start'], span['end'], span['open'], span['close']))
                for diff in cert['disagreements']:
                    disagreements.append((name, row['event'], row['rank'], diff['stage'], diff['facet'],
                                          diff['kind'], diff['legacy'], diff['origin']))
        lines += table('Longest unattributed gaps by phase', ('Run', 'Event', 'Rank', 'Phase',
                                                              'Total s', 'Start s', 'End s', 'Open', 'Close'), gaps)
        lines += table('Disagreeing census faces', ('Run', 'Event', 'Rank', 'Stage', 'Facet anchor',
                                                    'Census classification', 'Legacy bound', 'Origin'), disagreements)
    lines += ['Timing changes are observations. The 5400-second event limit and 95% measured coverage remain standing gates.', '']
    report.write_text('\n'.join(lines))


def final_report(label, failure=None, revert=None):
    folder = HERE / f'build{label}'
    build_file = folder / 'build_status.json'
    scoped = HERE / 'controls' / folder.name / 'step1_scoped.json'
    count = folder / 'step2.json'
    build = cut.read(build_file) if build_file.is_file() else {'build': label, 'passed': False}
    scoped_row = read_part(scoped, build)
    count_row = read_part(count, build)
    measured_count = read_part(root(label, 'count') / 'case_summary.json', build)
    measured_repeat = read_part(root(label, 'count_repeat') / 'case_summary.json', build)
    if previous := measured_repeat.get('count_from'):
        previous_build = stages(previous)[1]
        measured_count = read_part(root(previous, 'count') / 'case_summary.json', previous_build)
    results = {part: read_part(root(label, part) / 'case_summary.json', build) for part in ('S1', 'S2', 'probe5')}
    checks = {'build': {'status': 'passed' if build['passed'] else 'FAILED',
                        'receipt': path_pin(build_file) if build_file.is_file() else None},
              'step 1': scoped_row, **scoped_row.get('parts', {}), 'count run': measured_count,
              'count repeat': measured_repeat, 'step 2': count_row, **results,
              'full OFF': {'status': 'not evaluated'}, 'full CORE': {'status': 'not evaluated'},
              'warm-versus-cold continuation equivalence': {'status': 'not evaluated'}}
    measurement = (checked(count_row['receipt']) if count_row['status'] == 'passed'
                   else cut.read(folder / 'step2_attribution_required.json')
                   if (folder / 'step2_attribution_required.json').is_file() else None)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    references, _ = prior.inherited_reference(payloads)
    measurements = {'FP3-E S1': collect(Path(references[0]['case']))}
    full = {'FP3-E S1': checked(references[0]['full_run_receipt'])}
    for name, row in (('count', measured_count), ('count repeat', measured_repeat), *results.items()):
        if row.get('run'):
            full[name] = checked(row['run'])
    workflows = {name: {key: row[key] for key in ('case', 'restart', 'events', 'hashes', 'switches')}
                 for name, row in full.items()}
    for part, row in results.items():
        if row.get('run'):
            measurements[part] = collect(Path(checked(row['run'])['case']), row['probe'])
    if measurement:
        measurements['count'] = measurement['first_count']['ranks']
        if measurement['repeat']:
            measurements['count repeat'] = measurement['repeat']['ranks']
    differs = results['S1'].get('comparison', {}).get('equal') is False
    required = [scoped_row, count_row, results['S1'], results['S2']]
    if differs:
        required.append(results['probe5'])
    accepted = not failure and all(check['status'] == 'passed' for check in required)
    if failure:
        assert revert and failure.get('step') is not None and failure.get('gate')
        status = f'SC1: FAILED at {failure["step"]} {failure["gate"]}'
    elif accepted:
        verify_build(label)
        status = 'SC1: accepted; geometry certificates in the mode-S census (' + (
            'new S reference)' if differs else 'identical results)')
    else:
        status = 'SC1: OPEN' if any(check['status'] == 'OPEN' for check in required) else 'SC1: incomplete'
    data = {'label': status, 'build': label, 'source_commit': build.get('source_commit'),
            'binary_sha256': build.get('binary_sha256'), 'revert': revert, 'failure': failure,
            'source_check': path_pin(SOURCE_CHECK), 'order_sha256': ORDER_SHA, 'checks': checks,
            'reference': path_pin(prior.FULL_REF), 'count': measurement, 'measurements': measurements,
            'workflows': workflows}
    report = HERE / f'reports/SC1_{label}.md'
    report_json = report.with_suffix('.json')
    workflow.sealed_write(report_json, data)
    write_report(data, report)
    if accepted and differs:
        expected = checked(results['S1']['run'])['hashes']
        assert expected == checked(results['S2']['run'])['hashes']
        assert checked(results['probe5']['run'])['hashes'] == references[0]['payload_hashes']
        new_reference(label, build, expected, scoped, count, (report, report_json))
    print(status, flush=True)
    return accepted


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('--step2', action='store_true')
    parser.add_argument('--count-from', type=int, help='build of the first count run when sealing the repeat')
    parser.add_argument('--failure-json', type=Path, help='failure record with step, gate and first differing number')
    parser.add_argument('--revert')
    args = parser.parse_args()
    if args.step2:
        raise SystemExit(0 if seal_count(args.label, args.count_from) else 76)
    failure = cut.read(args.failure_json) if args.failure_json else None
    final_report(args.label, failure, args.revert)
