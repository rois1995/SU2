"""Assemble the FP2-M1 observation report from sealed trial and fixture receipts."""

from argparse import ArgumentParser
from collections import defaultdict
import json
from pathlib import Path
import re

import run_m1 as runner

HERE, a, cut = runner.HERE, runner.a, runner.cut
OUTCOMES = ('calls', 'direct_eligible', 'empty', 'same', 'no_larger_box', 'same_as_1', 'same_as_full',
            'proved', 'fail', 'resource', 'threw', 'select_full_resource', 'select_full_threw',
            'full_yes_small_proved', 'full_yes_small_other', 'full_no_small_proved', 'full_no_small_other')
NOT_AVAILABLE = 'not available'
NOTE = ('Both replay estimates use this run’s cache history. An earlier stage-2 query can alter a later '
        'stage-1 cost, and each production forward can alter the cost of later forwards. The same and '
        'same_as_full skips depend on full-support information that a real small-first policy must compute '
        'or do without. Full-support selection failures are excluded from the replay estimates.')


def number(value):
    if value is None or not re.fullmatch(r'[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?', str(value)):
        return None
    return int(value) if re.fullmatch(r'[+-]?\d+', str(value)) else float(value)


def total(rows, key):
    values = [number(row.get(key)) for row in rows]
    return sum(values) if values and all(value is not None for value in values) else None


def mean(rows, sum_key, count_key):
    numerator, denominator = total(rows, sum_key), total(rows, count_key)
    return numerator / denominator if numerator is not None and denominator else None


def sum_known(*values):
    return sum(values) if all(value is not None for value in values) else None


def metrics(raw):
    grouped = defaultdict(list)
    for row in raw:
        grouped[(row.get('kind'), row.get('bin'))].append(row)
    metrics = []
    for (kind, key), rows in sorted(grouped.items()):
        metrics.append({
            'kind': kind, 'bin': key,
            **{field: total(rows, field) for field in
               ('calls', 'proved_1', 'proved_2', 'needs_full', 'seconds_small', 'seconds_full',
                'seconds_full_needs_full', 'k_full_sum', 'k_small_sum')},
            'mean_k_full': mean(rows, 'k_full_sum', 'k_full_calls'),
            'mean_k_small': mean(rows, 'k_small_sum', 'k_small_calls'),
            'mean_r_over_tol': mean(rows, 'r_over_tol_sum', 'radius_calls'),
        })
    return metrics


def pass_summary(rows, baseline_rows, phase, ranks):
    def get(marker):
        return [row for rank in ranks for row in rows.get(rank, {}).get(marker, ()) if row.get('pass') == phase]

    support, forwards = get('SUPPORT'), get('FALLBACK')
    by_stage = {}
    for stage in ('1', '2'):
        stage_rows = [row for row in support if row.get('stage') == stage]
        by_stage[stage] = {
            **{field: total(stage_rows, field) for field in OUTCOMES},
            'interleaved': sorted({row['interleaved'] for row in stage_rows}),
            **{field: total(stage_rows, field) for field in
               ('seconds_select_small', 'seconds_select_full', 'seconds_small', 'work_small',
                'classification_work_small', 'selected_pieces_small', 'support_pieces_small', 'k_full_sum',
                'k_small_sum', 'k_full_calls', 'k_small_calls', 'radius_calls', 'r_over_tol_sum')},
            'mean_k_full': mean(stage_rows, 'k_full_sum', 'k_full_calls'),
            'mean_k_small': mean(stage_rows, 'k_small_sum', 'k_small_calls'),
            'mean_r_over_tol': mean(stage_rows, 'r_over_tol_sum', 'radius_calls'),
        }
    groups = {}
    for group in runner.GROUPS:
        group_rows = [row for row in forwards if row.get('group') == group]
        groups[group] = {field: total(group_rows, field) for field in
                         ('calls', 'seconds_full', 'work_full', 'full_memo_hits', 'replay_calls', 'replay_seconds_full')}
    small = [sum_known(stage['seconds_select_small'], stage['seconds_small']) for stage in by_stage.values()]
    timings = lambda keys: [groups[key]['replay_seconds_full'] for key in keys]
    old = [row for rank in ranks for row in baseline_rows.get(rank, {}).get('BOUND_OBSERVER', ())
           if row.get('pass') == phase]
    diff_totals = get('DIFF_TOTAL')
    return {
        'ranks': ranks, 'stage': by_stage, 'full': groups, 'seconds_full_total': total(forwards, 'seconds_full'),
        'build664_full_calls': total(old, 'calls'), 'build664_full_seconds': total(old, 'seconds'),
        'small_path_seconds': small, 'diagnostic_full_selection_seconds': total(support, 'seconds_select_full'),
        'replay_1_seconds': sum_known(small[0], *timings(('proved_2', 'needs_full', 'direct_eligible'))),
        'replay_1_plus_2_seconds': sum_known(*small, *timings(('needs_full', 'direct_eligible'))),
        'replay_note': NOTE,
        'bins': metrics(get('SUPPORT_BIN')),
        'diff_totals': [
            {'stage': stage, 'case': case,
             'count': total([row for row in diff_totals if row['stage'] == stage and row['case'] == case], 'count')}
            for stage in ('1', '2') for case in ('full_no_small_proved', 'full_yes_small_other')],
        'diffs': get('DIFF'), 'exceptions': get('EXCEPTION'), 'observer': get('OBSERVER'), 'memo': get('MEMO'),
    }


def summarise(name, old, new):
    rows, baseline_rows = new['rows_by_rank'], old['rows_by_rank']
    ranks = sorted(rows, key=int)
    phases = sorted({row['pass'] for raw in rows.values() for row in raw.get('SUPPORT', ())}, key=int)
    phases = {phase: {
        'all': pass_summary(rows, baseline_rows, phase, ranks),
        'slowest': pass_summary(rows, baseline_rows, phase, [max(ranks, key=lambda rank: (
            total([row for row in rows[rank].get('FALLBACK', ()) if row.get('pass') == phase], 'seconds_full') or 0,
            -int(rank)))])} for phase in phases}
    handover = []
    for rank in ranks:
        for row in rows[rank].get('HANDOVER', ()):
            strict = rows[rank].get('strict_observation', [{}])
            handover.append({**row, 'strict_observation_checked': strict[0].get('checked') if strict else None,
                             'strict_checked': new['measurement']['proof'].get('strict_checked')})
    return {
        'case': name, 'passes': phases, 'handover': handover,
        'shadow_load': [row for rank in ranks for marker in ('SHADOW', 'SHADOW_LOAD')
                        for row in rows[rank].get(marker, ())],
        'shadow_open': [row for rank in ranks for marker in ('SHADOW', 'SHADOW_LOAD', 'OBSERVER')
                        for row in rows[rank].get(marker, ()) if row.get('status', '').lower() == 'open'],
        'elapsed_seconds': new['wait']['elapsed_seconds'], 'build664_elapsed_seconds': old['wait']['elapsed_seconds'],
        'peak_lower_bound_bytes': new['peak_lower_bound_bytes'],
        'build664_peak_lower_bound_bytes': old['peak_lower_bound_bytes'],
    }


def shown(value):
    if value is None:
        return NOT_AVAILABLE
    if isinstance(value, float):
        return f'{value:.6f}'
    return str(value).replace('|', '\\|').replace('\n', ' ')


def table(columns, rows):
    return ['| ' + ' | '.join(columns) + ' |', '|' + '|'.join('---' for _ in columns) + '|',
            *('| ' + ' | '.join(shown(value) for value in row) + ' |' for row in rows), '']


def text(report):
    lines = [f"# FP2-M1 build{report['build']}", '', report['label'], '',
             'The build664 S native maxima were 951.321 and 689.037 s. Build660 took 550.220 and 330.735 s. '
             'Event 1 geometry-bound exclusive time increased from 80–188 to 197–450 s across ranks while '
             'the call count fell from 166–286k to 148–277k. Event 2 increased from 35–130 to 72–392 s. '
             'The event 1 communication plus join wait rose from 211–308 to 369–613 s. '
             'The build664 N1 bound-call mean increased from 1.81 ms below 2 mm to 22.96 ms at 20 mm and more.', '',
             NOTE, '',
             'The observer uses a separate ledger. Job peak is the measured lower bound for the full job. '
             'RSS figures in the observer rows are process snapshots around shadow load.', '',
             'C3 BOUND timing and M1 FALLBACK time only the production forward. The lab hand-over pass clock '
             'subtracts the observer hook wall time. Inclusive Mesh::RunPass step and sweep clocks may include '
             'shadow work and are outside the exact projection.', '']
    for name, case in report['cases'].items():
        status = case['status']
        lines.extend([f'## {name}', '', f'Case result {status}.', ''])
        if case.get('first_difference'):
            lines.extend([f"First difference {shown(case['first_difference'])}.", ''])
        if case.get('observer', {}).get('problems'):
            lines.extend([f"Observer issues {shown(case['observer']['problems'])}.", ''])
        if 'observations' not in case:
            continue
        measured = case['observations']
        lines.extend(table(('Trial s', 'Build664 trial s', 'Job peak bytes', 'Build664 job peak bytes'),
                           [(measured[key] for key in ('elapsed_seconds', 'build664_elapsed_seconds',
                                                      'peak_lower_bound_bytes', 'build664_peak_lower_bound_bytes'))]))
        if measured['shadow_load']:
            keys = tuple(dict.fromkeys(key for row in measured['shadow_load'] for key in row))
            lines.extend(table(keys, [[row.get(key) for key in keys] for row in measured['shadow_load']]))
        if measured['shadow_open']:
            lines.extend([f"Open shadow measurement {shown(measured['shadow_open'])}.", ''])
        for phase, scopes in measured['passes'].items():
            lines.extend([f'### Pass {phase}', ''])
            lines.extend(table(('Scope', 'Stage', 'Interleaved', *OUTCOMES),
                               [(scope, stage, ','.join(row['interleaved']), *(row[field] for field in OUTCOMES))
                                for scope, data in scopes.items() for stage, row in data['stage'].items()]))
            columns = ('seconds_select_small', 'seconds_small', 'seconds_select_full', 'work_small',
                       'classification_work_small', 'selected_pieces_small', 'support_pieces_small',
                       'mean_k_full', 'mean_k_small', 'mean_r_over_tol')
            lines.extend(table(('Scope', 'Stage', *columns),
                               [(scope, stage, *(row[field] for field in columns))
                                for scope, data in scopes.items() for stage, row in data['stage'].items()]))
            lines.extend(table(('Scope', 'Rank set', 'Small 1 s', 'Small 2 s', 'Full selection diagnostic s',
                                'Full forwards s', 'Build664 forwards s', 'Replay 1 s', 'Replay 1+2 s'),
                               [(scope, ','.join(data['ranks']), *data['small_path_seconds'],
                                 *(data[field] for field in ('diagnostic_full_selection_seconds',
                                                             'seconds_full_total', 'build664_full_seconds',
                                                             'replay_1_seconds', 'replay_1_plus_2_seconds')))
                                for scope, data in scopes.items()]))
            lines.extend(table(('Scope', 'Group', 'Calls', 'Forward s', 'Classification work', 'Memo hits',
                                'Replay calls', 'Replay forward s'),
                               [(scope, group, *row.values()) for scope, data in scopes.items()
                                for group, row in data['full'].items()]))
            bins = [(scope, row) for scope, data in scopes.items() for row in data['bins']]
            if bins:
                lines.extend(table(('Scope', *bins[0][1]), [(scope, *row.values()) for scope, row in bins]))
            columns = ('rank', 'status', 'seconds_observer', 'scratch_bytes', 'shadow_bytes',
                       'shadow_load_peak', 'ledger_peak', 'rss_before', 'rss_after', 'rss_high_water')
            lines.extend(table(columns, [[row.get(field) for field in columns] for row in scopes['all']['observer']]))
            memo = ('rank', 'at', 'authority', 'hits', 'misses', 'entries', 'evictions', 'retention_refusals',
                    'bytes', 'memo_limit', 'prepared_limit')
            lines.extend(table(memo, [[row.get(field) for field in memo] for row in scopes['all']['memo']]))
            lines.extend(table(('Stage', 'Agreement difference', 'Count'),
                               [(row['stage'], row['case'], row['count']) for row in scopes['all']['diff_totals']]))
            diffs = [row for row in scopes['all']['diffs'] if row['case'] == 'full_no_small_proved']
            lines.extend(f"- {shown(row)}" for row in diffs)
            if diffs:
                lines.append('')
        if measured['handover']:
            fields = tuple(measured['handover'][0])
            lines.extend(['### Strict hand-over', '',
                          'Memo hits can come from earlier strict queries, including ReportCollapseFaces.', '',
                          *table(fields, [[row.get(field) for field in fields] for row in measured['handover']])])
    lines.extend(['## Focused fixture', '', f"Replica {shown(report['replica'])}.", ''])
    return '\n'.join(lines)


def choose(label, name):
    paths = [runner.case_path(label, name, HERE / 'r5_retry') / 'case_summary.json',
             runner.case_path(label, name) / 'case_summary.json']
    return next((path for path in paths if path.is_file()), None)


def report(build):
    label = cut.read(build / 'build_status.json')['build']
    cases, artifacts = {}, {}
    for name in runner.CASES:
        path = choose(label, name)
        if path is None:
            cases[name] = {'status': 'FAILED', 'first_difference': 'No completed case receipt.'}
            continue
        record = cut.read(path)
        artifacts[str(path)] = a.sha(path)
        for value, digest in record['attempts'].items():
            attempt = Path(value)
            receipt = cut.read(attempt)
            assert a.sha(attempt) == digest and receipt['wait']['terminal']
            runner.lab.archived.verify_artifacts(attempt.parent, receipt['artifacts'])
            artifacts[value] = digest
        old_path, old = runner.baseline(name)
        artifacts[str(old_path)] = a.sha(old_path)
        new = cut.read(Path(record['used']))
        cases[name] = {**record, 'observations': summarise(name, old, new)}
    fixture = runner.fixture_result(label, 'fixture')
    replica = []
    if fixture.is_file():
        record = cut.read(fixture)
        artifacts[str(fixture)] = a.sha(fixture)
        parsed = json.loads(record['details']['trace_stdout'])
        log = Path(record['result']['case']) / 'test_outer.log'
        with runner.patch.object(runner.lab, 'LINE', runner.LINE):
            replica = runner.lab.parse_rows(log).get('REPLICA', [])
        assert parsed['passed'] and replica
    states = {item['status'] for item in cases.values()}
    state = 'measurement FAILED' if 'FAILED' in states else ('measurement OPEN' if 'OPEN' in states else 'measurement done')
    data = {'build': label, 'baseline_build': runner.BASE_BUILD, 'label': f'FP2-M1: {state}; default path not yet proven',
            'cases': cases, 'replica': replica, 'artifacts': artifacts}
    directory = HERE / 'reports'
    directory.mkdir(exist_ok=True)
    path = directory / f'FP2_M1_{label}'
    a.sealed(path.with_suffix('.json'), data)
    markdown = text(data) + '\n'
    output = path.with_suffix('.md')
    if output.exists():
        assert output.read_text() == markdown
    else:
        output.write_text(markdown)
    return data


def self_check():
    def fallback(group, cost):
        return {'group': group, 'pass': '1', 'calls': '1', 'seconds_full': str(cost), 'replay_seconds_full': str(cost)}
    rows = {'0': {'FALLBACK': [fallback(group, value) for group, value in zip(runner.GROUPS, (10, 20, 30, 40))],
                   'SUPPORT': [{'pass': '1', 'stage': str(stage), 'interleaved': '0', 'seconds_select_full': '20',
                                'seconds_select_small': '1', 'seconds_small': '2'} for stage in (1, 2)]}}
    summary = pass_summary(rows, {}, '1', ['0'])
    assert summary['seconds_full_total'] == 100 and summary['small_path_seconds'] == [3, 3]
    assert summary['replay_1_seconds'] == 93 and summary['replay_1_plus_2_seconds'] == 76
    assert summary['build664_full_seconds'] is None
    print('FP2-M1 replay arithmetic and unavailable reporting verified.')


if __name__ == '__main__':
    cli = ArgumentParser(description=__doc__)
    cli.add_argument('build', type=Path, nargs='?')
    cli.add_argument('--self-check', action='store_true')
    args = cli.parse_args()
    if args.self_check:
        self_check()
    elif args.build:
        print(report(args.build.resolve())['label'])
    else:
        cli.error('a build directory is required')
