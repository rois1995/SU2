"""Seal FP3-C Step 1 and publish its sweep and workflow evidence."""

from argparse import ArgumentParser
import hashlib
from pathlib import Path
import runpy
import subprocess

from build_fp3 import HERE, cut, verify_build
from collect_fp3_full import SWEEP_COUNTS, SWEEP_SECONDS, collect
from run_fp3_full import (B_REFERENCE, FINGERPRINTS, ORDER_SHA, check_pins,
                          path_pin, reference_runs, root, workflow)

LAB_PARTS = ('N6P-off', 'N6P-on', 'N1-on')
UNEVALUATED = {'status': 'not evaluated', 'failure_kind': None, 'first_difference': None}


def read_pin(pointer):
    path = Path(pointer['path'])
    assert cut.sha(path) == pointer['sha256'], path
    return cut.read(path)


def read_part(path, build):
    if not path.is_file():
        return UNEVALUATED.copy()
    row = cut.read(path)
    assert all(row[key] == build[key] for key in FINGERPRINTS), path
    check_pins(row['pins'])
    return row


def lab_path(label, name):
    return HERE / f'lab/build{label}/{name}/case_summary.json'


def step1(label):
    build = verify_build(label)
    scope = HERE / f'controls/build{label}/step1_scoped.json'
    off_path = lab_path(label, 'N6P-off')
    scoped, off = (read_part(path, build) for path in (scope, off_path))
    parts = scoped.get('parts', {}).copy()
    parts['N6P-off'] = {key: off[key] for key in ('status', 'failure_kind', 'first_difference')}
    statuses = {scoped['status'], off['status']}
    status = next(flag for flag in ('FAILED', 'OPEN', 'not evaluated', 'passed') if flag in statuses)
    if status == 'not evaluated':
        return status
    if status == 'passed':
        assert off['identity']['status'] == 'equal' and off['identity']['differences'] == [], off['identity']
    first = next((row for row in (scoped, off) if row['status'] == status), {})
    pins = {str(path): cut.sha(path) for path in (scope, off_path) if path.is_file()}
    pins.update(scoped.get('pins', {}) | off.get('pins', {}))
    record = {**{key: build[key] for key in FINGERPRINTS}, 'status': status, 'passed': status == 'passed',
              'parts': parts, 'pins': pins, 'failure_kind': first.get('failure_kind'),
              'first_difference': first.get('first_difference')}
    path = HERE / f'build{label}/step1.json'
    if path.is_file():
        assert cut.read(path) == record, path
    else:
        cut.save(path, record)
    return status


def brief(row):
    return {key: value for key, value in row.items() if key != 'pins'}


def table(file, heading, columns, rows):
    def display(value):
        if value is None:
            return 'unavailable'
        if isinstance(value, float):
            return f'{value:.3f}'
        return str(value).replace('|', r'\|').replace('\n', ' ')
    print(f'\n### {heading}\n', file=file)
    print('| ' + ' | '.join(columns) + ' |', file=file)
    print('| ' + ' | '.join('---' for _ in columns) + ' |', file=file)
    for row in rows:
        assert len(row) == len(columns), (heading, row)
        print('| ' + ' | '.join(map(display, row)) + ' |', file=file)


def write_report(path, record):
    sweep_fields = (*SWEEP_COUNTS, *SWEEP_SECONDS)
    with path.open('w') as out:
        print(f'# FP3-C build{record["build"]}\n\n**{record["status"]}**\n', file=out)
        print('The [pre-edit source check](source_check.md) records the R4 readings and storage accounting.', file=out)
        if (HERE / 'attempts/compile1/build_outer.log').is_file():
            print('\nThe initial compile exposed a missing default remote argument at the existing fixture wrapper. '
                  'The rebuilt declaration restores that default. '
                  '[First compile log](../attempts/compile1/build_outer.log)', file=out)
        adoption = record['adoption']
        if adoption:
            print(f'\nThe sealed S1 result was **Y = {adoption["Y"]}** from {adoption["rank_stage_rows"]} '
                  f'rank and stage rows. The threshold was {adoption["threshold"]}. '
                  f'[S1 adoption seal](../G5/build{record["build"]}/S1/adoption.json)', file=out)
        if record['revert_commit']:
            print(f'\nProduction and dependent fixture sources were restored in `{record["revert_commit"]}`. '
                  'FP3-B supplies the ordinary S reference.', file=out)
        restored = record.get('restored_build')
        if restored:
            matched = [name for name, key in (('solver', 'binary_sha256'), ('test driver', 'test_binary_sha256'))
                       if restored['matches_build676'][key]]
            matches = f' The {" and ".join(matched)} matched build676 by SHA256.' if matched else ''
            print(f'\nBoth restored targets built successfully.{matches} '
                  '[Restore build receipt](../restore_build/build_status.json)', file=out)
        if record['checks'].get('saved-current', {}).get('status') == 'passed':
            print('\nThe two oversized saved-current files are retained locally under their recorded hashes. '
                  f'[Saved-current seal](../controls/build{record["build"]}/saved-current/case_summary.json)', file=out)
        table(out, 'Gates', ('Check', 'Status', 'First finding'),
              [(name, row['status'], row.get('first_difference') or '') for name, row in record['checks'].items()])
        for name, row in record['lab'].items():
            report = row.get('report')
            if not report:
                continue
            stats_fields = ('faces_out', 'cells_out', 'bad_out', 'short_out', 'seconds_pass')
            table(out, f'{name} last pass',
                  ('Case', 'Rank', *stats_fields, 'Hand-over cells', 'Hand-over faces'),
                  [(name if mode == 'trial' else report['comparison_run'], rank,
                    *(data.get(field) for field in stats_fields), data.get('handover_cells'),
                    report[mode].get('handover_faces'))
                   for mode in ('trial', 'comparison') if report.get(mode)
                   for rank, data in report[mode]['by_rank'].items()])
            rows = row.get('sweep_rows', [])
            if rows:
                totals = row['sweep_sums']
                table(out, f'{name} sweep', ('Rank', *sweep_fields),
                      [(data['rank'], *(data.get(field) for field in sweep_fields)) for data in rows] +
                      [('Sum', *(totals.get(field) for field in sweep_fields))])
        measurements = record['workflows']
        table(out, 'Two-event mode S results',
              ('Run', 'Event', 'Cells', 'Points', 'Faces', 'Native s', 'Proof s', 'Coverage %',
               'Peak bytes', 'Native start UTC', 'Native end UTC', 'Start load', 'End load'),
              [(name, row['event'], row['final_cells'], row['final_points'], row['final_surface_faces'],
                row['native_seconds'], row['phase_seconds_max'].get('final_proof'),
                None if row['minimum_timer_coverage'] is None else 100 * row['minimum_timer_coverage'],
                row['job_peak_rss_bytes'], row.get('native_start_utc'), row.get('native_end_utc'),
                (row.get('host_load_start') or {}).get('loadavg'), (row.get('host_load_end') or {}).get('loadavg'))
               for name, workflow_data in measurements.items() for row in workflow_data['events']])
        table(out, 'Worker stage mesh and quality',
              ('Run', 'Event', 'Stage', 'Core bad_out sum', 'Engine bad_union sum', 'Quality maximum s'),
              [(name, event['event'], stage['stage'], stage['core_last_pass_bad_out_sum'],
                stage['quality_entry_bad_union_sum'], stage['quality_seconds_max'])
               for name, data in measurements.items() for event in data['events'] for stage in event['worker_stages']])
        for name in ('S1', 'S2'):
            if name not in measurements:
                continue
            rows = measurements[name].get('sweep_rows', [])
            totals = measurements[name].get('sweep_totals', [])
            table(out, f'{name} final surface sweep', ('Event', 'Stage', 'Rank', 'Pass', *sweep_fields),
                  [(row['event'], row['stage'], row['rank'], row['pass'], *(row.get(field) for field in sweep_fields))
                   for row in rows] +
                  [(row['event'], row['stage'], 'Sum', '', *((row['counts'] | row['seconds']).get(field)
                   for field in sweep_fields)) for row in totals if row['status'] == 'complete'])
        table(out, 'Six-file comparisons', ('Run', 'Comparison', 'Differences'),
              [(name, field, len(row[field]['differences'])) for name, row in record['s_receipts'].items()
               for field in ('historical_comparison', 'comparison') if field in row])
        print('\nNative start times are reconstructed from the logged end and native maximum. Load values are '
              'the nearest recorded samples. Sweep and repair times in sum rows are rank-seconds.', file=out)


def report(label, validated_commit=None, revert_commit=None):
    work = HERE / f'build{label}'
    build = cut.read(work / 'build_status.json')
    scoped = read_part(HERE / f'controls/build{label}/step1_scoped.json', build)
    steps = {f'Step {n}': read_part(work / f'step{n}.json', build) for n in (1, 2)}
    lab = {name: read_part(lab_path(label, name), build) for name in LAB_PARTS}
    s = {name: read_part(root(label, name) / 'case_summary.json', build) for name in ('S1', 'S2')}
    checks = {'build': {'status': 'passed' if build['passed'] else 'FAILED'}, **scoped.get('parts', {}),
              **lab, **steps, **s, 'full CORE workflow': UNEVALUATED, 'full OFF workflow': UNEVALUATED}
    adoption = read_pin(s['S1']['adoption']) if s['S1'].get('adoption') else None
    statuses = {row['status'] for row in checks.values()}
    accepted = adoption and adoption['disposition'] == 'Y >= 273' and all(
        row['status'] == 'passed' for row in (*steps.values(), *s.values()))
    if 'FAILED' in statuses:
        status = 'FP3-C: FAILED; no final surface sweep' if revert_commit else 'FP3-C: FAILED'
    elif 'OPEN' in statuses:
        status = 'FP3-C: OPEN'
    elif adoption and adoption['disposition'] == 'Y < 273':
        assert revert_commit and s['S2']['status'] == 'not evaluated'
        status = f'FP3-C: not adopted; the sweep gives {adoption["Y"]} collapses'
    elif accepted:
        status = 'FP3-C: accepted; final surface sweep in the core'
    else:
        status = 'FP3-C: not evaluated'

    payloads = runpy.run_path(str(workflow.PAYLOADS))
    historical, _ = reference_runs(B_REFERENCE, payloads)
    measurements = {}
    for n, entry in enumerate(historical, 1):
        run = read_pin(entry['full_run_receipt'])
        data = collect(Path(entry['case']), baseline=True, event_receipts=run['events'])
        assert data['status'] == 'passed', data['first_difference']
        measurements[f'B S{n}'] = data
    for name, row in s.items():
        if row.get('collection'):
            measurements[name] = read_pin(row['collection'])
        elif row['status'] != 'not evaluated':
            data = sorted(root(label, name).glob('**/collection.json'))
            data = data or sorted(root(label, name).glob('**/partial_collection.json'))
            if data:
                path = data[-1]
                assert cut.sha(path) == row['pins'][str(path)], path
                measurements[name] = cut.read(path)
    record = {'status': status, 'build': label, 'adoption': adoption, 'revert_commit': revert_commit,
              'checks': {name: {key: row.get(key) for key in ('status', 'failure_kind', 'first_difference')}
                         for name, row in checks.items()},
              'lab': {name: brief(row) for name, row in lab.items()},
              's_receipts': {name: brief(row) for name, row in s.items()}, 'workflows': measurements}
    restore = HERE / 'restore_build/build_status.json'
    if revert_commit and restore.is_file():
        rebuilt = cut.read(restore)
        assert rebuilt['passed'] and rebuilt['commit'] == revert_commit
        record['restored_build'] = {**rebuilt, 'receipt': path_pin(restore)}
    cut.save(HERE / f'reports/FP3_C_{label}.json', record)
    write_report(HERE / f'reports/FP3_C_{label}.md', record)
    if validated_commit:
        assert accepted and not revert_commit
        verify_build(label)
        assert s['S2']['comparison']['equal']
        work_order_path = str(cut.REPO / 'AGENTS.md')
        expected = cut.read(work / 'source_pins.json')[work_order_path]
        blob = subprocess.check_output(['git', 'show', f'{validated_commit}:AGENTS.md'], cwd=cut.REPO)
        assert hashlib.sha256(blob).hexdigest() == expected
        runs, pins = [], {}
        for row in (*steps.values(), *s.values()):
            pins.update(row['pins'])
        for name in ('S1', 'S2'):
            summary = s[name]
            run = read_pin(summary['run'])
            case = Path(run['case'])
            assert len(run['hashes']) == 6 and all(cut.sha(case / key) == value for key, value in run['hashes'].items())
            runs.append({'case': str(case), 'full_run_receipt': summary['run'],
                         'isolated_restart_receipt': path_pin(Path(run['restart'])),
                         'effective_config': path_pin(case / 'run.cfg'), 'payload_hashes': run['hashes'],
                         'native_maximum_seconds': {str(row['event']): row['native_seconds'] for row in run['events']}})
        assert runs[0]['payload_hashes'] == runs[1]['payload_hashes']
        assert runs[0]['effective_config']['sha256'] == runs[1]['effective_config']['sha256']
        check_pins(pins)
        if work_order_path in pins:
            assert pins.pop(work_order_path) == expected
        gate_paths = {'step1': work / 'step1.json', 'step2': work / 'step2.json',
                      **{name: root(label, name) / 'case_summary.json' for name in ('S1', 'S2')}}
        cut.save(HERE / 'accepted_reference.json', {
            **{key: build[key] for key in FINGERPRINTS}, 'accepted': True, 'accepted_commit': validated_commit,
            'order_sha256': ORDER_SHA,
            'validation_work_order': {'commit': validated_commit, 'path': 'AGENTS.md', 'sha256': expected},
            'gate_receipts': {name: path_pin(path) for name, path in gate_paths.items()},
            'adoption': s['S1']['adoption'], 'workflows': {'S': {'reference_case': runs[0]['case'], 'runs': runs}},
            'pins': pins})
    return status


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('--step1', action='store_true')
    parser.add_argument('--validated-commit')
    parser.add_argument('--revert-commit')
    args = parser.parse_args()
    if args.step1:
        status = step1(args.label)
        print(status)
        raise SystemExit(status != 'passed')
    print(report(args.label, args.validated_commit, args.revert_commit))
