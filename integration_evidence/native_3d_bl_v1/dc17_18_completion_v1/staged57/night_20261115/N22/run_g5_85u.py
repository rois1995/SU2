#!/usr/bin/env python3
"""Run N22's sealed full workflows and apply its decision, file and time rules."""

import argparse
from collections import deque
from decimal import Decimal
from itertools import zip_longest
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
sys.path.insert(0, str(NIGHT / 'N21'))
from run_g5 import F_WORKFLOW, SUMMARY, actual_host, cut, prepare, workflow

N21 = NIGHT / 'N21/accepted_reference.json'
N20_REPORT = NIGHT / 'N20/report_85s.py'
REPORT = runpy.run_path(str(N20_REPORT))
OWNERSHIP = REPORT['M28']['OWNERSHIP']
ORIGINAL_CLOCKS = {'1': '2048.286', '2': '1278.700'}
ROUND = {part: cut.PHASE / f'round_8_5u_{part}' for part in ('M', 'C')}
G5 = {part: root / 'G5' for part, root in ROUND.items()}
PHASES = ('preparation', 'construction', 'quality', 'final_proof')


def sealed_write(path, value):
    if path.exists():
        assert cut.read(path) == value, path
    else:
        cut.save(path, value)


def decisions(case):
    """Extract one all-rank-verified evaluation sequence per event."""
    events = REPORT['by_event'](REPORT['read_rank_events'](case))
    assert list(events) == [1, 2], (case, list(events))
    output = []
    for event, ranks in events.items():
        assert set(ranks) == set(range(6)), (case, event, list(ranks))
        per_rank = {
            rank: [row for row in tags[OWNERSHIP] if row.fields.get('evaluate') == '1']
            for rank, tags in ranks.items()
        }
        keys = {tuple((row.fields['quality'], row.fields['migrate']) for row in rows)
                for rows in per_rank.values()}
        assert len(keys) == 1, (case, event, 'rank disagreement in ownership trace', keys)
        for number, row in enumerate(per_rank[0], 1):
            assert row.fields['migrate'] in ('0', '1') and row.fields['quality'] in ('0', '1')
            output.append({'event': event, 'evaluation': number, **row.fields,
                           'source': row.source, 'line': row.number})
    assert output, (case, 'missing ownership evaluations')
    return output


def decision_differences(before, after):
    key = lambda row: (row['event'], row['evaluation'], row['quality'], row['migrate']) if row else None
    by_event = lambda rows, event: [row for row in rows if row['event'] == event]
    return [{'reference': old, 'candidate': new} for event in (1, 2)
            for old, new in zip_longest(by_event(before, event), by_event(after, event)) if key(old) != key(new)]


def current_clocks(events):
    assert [event['event'] for event in events] == [1, 2]
    return {str(event['event']): str(event['native_seconds']) for event in events}


def time_result(clocks, baseline, part):
    """M reports its five-percent overage. C gates both events and the exact 0.95 sum."""
    ref = {event: Decimal(baseline[event]) for event in ('1', '2')}
    now = {event: Decimal(clocks[event]) for event in ref}
    assert all(value.is_finite() and value > 0 for value in (*ref.values(), *now.values()))
    event_factor = Decimal('1.05') if part == 'M' else Decimal(1)
    rows = [{'event': int(event), 'reference_seconds': str(value), 'candidate_seconds': str(now[event]),
             'limit_seconds': str(value * event_factor), 'within_limit': now[event] <= value * event_factor}
            for event, value in ref.items()]
    total, reference_total = sum(now.values()), sum(ref.values())
    individual = all(row['within_limit'] for row in rows)
    result = {'events': rows, 'sum_seconds': str(total), 'reference_sum_seconds': str(reference_total)}
    if part == 'M':
        return {**result, 'report_only': True, 'open_for_user': not individual}
    limit = Decimal('0.95') * reference_total
    return {**result, 'factor': '0.95', 'combined_limit_seconds': str(limit),
            'passed': individual and total <= limit}


def n21_reference(runner):
    """Authenticate both historical W captures, including the adopted payload manifest."""
    entry = cut.read(N21)
    assert entry['accepted']
    checks = {}
    for name in ('payloads', 'timing_rule', 'workflow_acceptance'):
        ref = entry[name]
        path = (N21.parent / ref['path']).resolve()
        assert cut.sha(path) == ref['sha256'], path
        checks[name] = cut.read(path)
    adoption = checks['payloads']
    assert adoption['accepted'] and checks['timing_rule']['passed'] and checks['workflow_acceptance']['passed']
    assert adoption['acceptance_sha256'] == entry['workflow_acceptance']['sha256']
    cases = [Path(adoption['first_case']), Path(adoption['repeat_case'])]
    for case in cases:
        assert runner['verify_full'](case, entry['solver_sha256'])['hashes'] == adoption['hashes']
    ownership = [decisions(case) for case in cases]
    assert not decision_differences(*ownership), 'The two adopted W runs disagree on ownership outcomes.'
    return {'case': str(cases[0]), 'repeat_case': str(cases[1]), 'hashes': adoption['hashes'],
            'clocks': ORIGINAL_CLOCKS, 'decisions': ownership[0], 'sha256': cut.sha(N21)}


def event_diagnostics(case):
    """Collect rank-local coverage and unattributed seconds even after a failed standing gate."""
    rows = []
    for event in (1, 2):
        path = case / f'f3b_event{event}_timer_gate.json'
        if not path.exists():
            continue
        gate = cut.read(path)
        for rank in gate.get('ranks', []):
            summary = rank.get('ledger', {}).get('summary', {})
            rows.append({'event': event, 'rank': rank['rank'], 'coverage': summary.get('coverage'),
                         'unattributed_seconds': summary.get('unattributed'), 'receipt': str(path)})
    return rows


def measure_phases(case):
    """Use the authoritative final remesh ledgers, once per rank and event."""
    full = runpy.run_path(str(SUMMARY))['collect_events'](case)
    for event in full:
        timer = cut.read(case / f"f3b_event{event['event']}_timer_gate.json")
        ranks = timer['ranks']
        phases = {}
        for phase in PHASES:
            rows = [row for rank in ranks for row in rank['ledger']['rows'] if row['phase'] == phase]
            comm = [row for row in rows if row['operation'] == 'communication']
            census = [row for row in rows if row['operation'] == 'census']
            phases[phase] = {
                'seconds_mean': sum(float(row['exclusive_seconds']) for row in rows) / len(ranks),
                'communication_calls_mean': sum(int(row['calls']) for row in comm) / len(ranks),
                'communication_seconds_mean': sum(float(row['exclusive_seconds']) for row in comm) / len(ranks),
                'census_inclusive_seconds_mean': sum(float(row['inclusive_call_seconds']) for row in census) / len(ranks),
            }
        event['phase_means'] = phases
        event['coverage_by_rank'] = {str(rank['rank']): rank['ledger']['summary']['coverage'] for rank in ranks}
    return full


def prepare_campaign(part, build, workspace):
    # W-style wrappers validate standing gates without using the stale P6c/P7 payload reference.
    runner = workflow('W', workspace)
    runner['RUNNERS'] = ('run_full',)
    guard = workspace / 'n22_adapter_pins.json'
    if not workspace.exists():
        prepare('W', build, workspace, runner)
        sources = (Path(__file__).resolve(), F_WORKFLOW, SUMMARY, N21, N20_REPORT,
                   HERE / 'inputs/SHA256SUMS', HERE / 'inputs/IMPLEMENTATION_8_5U.md')
        cut.save(guard, {str(path): cut.sha(path) for path in sources})
    runner['verify_pins'](cut.read(guard))
    return runner


def execute_run(runner, workspace, build, binary, run):
    case = workspace / f'full{build}_run{run}'
    runner['run_or_resume'](build, f'run{run}', 'run_full', '--case', case)
    receipt = runner['verify_full'](case, binary)
    assert len(receipt['hashes']) == 6
    return {'case': str(case), 'events': measure_phases(case), 'hashes': receipt['hashes'],
            'decisions': decisions(case)}


def require_hashes(workspace, expected, run, label):
    actual = run['hashes']
    details = {name: {'reference': expected.get(name), 'candidate': actual.get(name)}
               for name in sorted(expected.keys() | actual.keys()) if expected.get(name) != actual.get(name)}
    sealed_write(workspace / 'payload_comparison.json',
                 {'passed': not details, 'compare_to': label, 'differences': details,
                  'reference': expected, 'candidate': actual})
    assert not details, (label, next(iter(details.items())))


def measure_m(runner, build, binary, scoped):
    workspace = G5['M']
    reference = n21_reference(runner)
    first = execute_run(runner, workspace, build, binary, 1)
    differences = decision_differences(reference['decisions'], first['decisions'])
    gq = cut.read(scoped)['GQ']
    assert isinstance(gq['decisions_changed'], bool), gq
    case = 5 if differences or gq['decisions_changed'] else 4
    sealed_write(workspace / 'ownership_comparison.json',
                 {'case': case, 'reference': reference, 'candidate': first['decisions'],
                  'changed_evaluations': differences, 'GQ_decisions_changed': gq['decisions_changed']})
    runs = [first]
    if case == 5:
        second = execute_run(runner, workspace, build, binary, 2)
        require_hashes(workspace, first['hashes'], second, 'M run one')
        runs.append(second)
    else:
        require_hashes(workspace, reference['hashes'], first, 'N21 W')
    clocks = current_clocks(first['events'])
    timing = [time_result(current_clocks(run['events']), reference['clocks'], 'M') for run in runs]
    opened = any(row['open_for_user'] for row in timing)
    sealed_write(workspace / 'time_report.json', {'reference_run_for_C': 1, 'runs': timing, 'open_for_user': opened})
    return {'passed': True, 'status': 'OPEN for the user' if opened else 'PASSED',
            'case': case, 'runs': runs, 'N21_reference': reference, 'GQ_decisions_changed': gq['decisions_changed'],
            'changed_evaluations': differences, 'timing': timing,
            'reference_after': {'case': first['case'], 'hashes': first['hashes'], 'clocks': clocks}}


def measure_c(runner, build, binary):
    before = cut.read(G5['M'] / 'acceptance_summary.json')
    assert before['passed'] and before['standing_gates_passed']
    reference = before['reference_after']
    previous = Path(reference['case'])
    assert runner['verify_full'](previous, before['binary_sha256'])['hashes'] == reference['hashes']
    run = execute_run(runner, G5['C'], build, binary, 1)
    require_hashes(G5['C'], reference['hashes'], run, 'qualified Part M')
    clocks = current_clocks(run['events'])
    purpose = time_result(clocks, reference['clocks'], 'C')
    sealed_write(G5['C'] / 'purpose_rule.json', purpose)
    return {'passed': purpose['passed'], 'status': 'PASSED' if purpose['passed'] else 'OPEN for the user',
            'retained': True, 'new_timing_reference': purpose['passed'], 'runs': [run],
            'M_reference': reference, 'M_reference_events': before['runs'][0]['events'], 'purpose': purpose,
            'reference_after': {'case': run['case'], 'hashes': run['hashes'], 'clocks': clocks}
                               if purpose['passed'] else reference}


def record_failure(workspace, build, error):
    cases = sorted(workspace.glob(f'full{build}_run*'))
    rows = []
    for case in cases:
        tails = {}
        for rank in range(6):
            for stream in ('stdout', 'stderr'):
                log = case / f'rank_output/1/rank.{rank}/{stream}'
                if log.is_file():
                    with log.open(errors='replace') as file:
                        tails[str(log)] = list(deque(file, maxlen=8))
        rows.append({'case': str(case), 'timing': event_diagnostics(case), 'last_rank_log_lines': tails})
    sealed_write(workspace / 'failed_attempt.json',
                 {'passed': False, 'build': build, 'error': str(error), 'cases': rows,
                  'purpose_only': False})


def run(part, build, scoped, safeguards):
    actual_host()
    workspace = G5[part]
    assert not (workspace / 'failed_attempt.json').exists(), 'Preserve the failed result for R5 disposition.'
    assert not (workspace / 'acceptance_summary.json').exists(), 'G5 is already terminal.'
    runner = prepare_campaign(part, build, workspace)
    binary = runner['preflight'](build, scoped, safeguards)
    try:
        result = measure_m(runner, build, binary, scoped) if part == 'M' else measure_c(runner, build, binary)
    except (AssertionError, OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        record_failure(workspace, build, error)
        raise
    sealed_write(workspace / 'acceptance_summary.json',
                 {'build': build, 'binary_sha256': binary, 'standing_gates_passed': True, **result})
    print(f"N22 {part} {result['status']}. Receipt {workspace / 'acceptance_summary.json'}")


def self_check():
    clocks = {'1': '1900', '2': '950.0'}
    baseline = {'1': '2000', '2': '1000'}
    assert time_result(clocks, baseline, 'C')['passed']
    assert not time_result({**clocks, '2': '950.0001'}, baseline, 'C')['passed']
    assert not time_result({'1': '1800', '2': '1000.001'}, baseline, 'C')['passed']
    assert not time_result({'1': '2100', '2': '1050'}, baseline, 'M')['open_for_user']
    assert time_result({'1': '2100.001', '2': '1050'}, baseline, 'M')['open_for_user']
    entry = cut.read(N21)
    adoption = cut.read((N21.parent / entry['payloads']['path']).resolve())
    first, repeat = (Path(adoption[key]) for key in ('first_case', 'repeat_case'))
    before, after = decisions(first), decisions(repeat)
    assert len(before) == len(after) == 19 and not decision_differences(before, after)
    assert decision_differences(before, [{**after[0], 'migrate': '0'}, *after[1:]])
    assert decision_differences(before, after[:-1])
    events = measure_phases(first)
    assert all(len(event['coverage_by_rank']) == 6 and event['phase_means'] for event in events)
    with tempfile.TemporaryDirectory(prefix='n22-g5-') as directory:
        for part, build in (('M', 590), ('C', 591)):
            workspace = Path(directory) / part / 'G5'
            helper = prepare_campaign(part, build, workspace)
            assert helper['RUNNERS'] == ('run_full',)
            pins = cut.read(workspace / 'helper_pins.json')
            for name in ('bound', 'source', 'comparison_source'):
                helper['verify_pins'](pins[name])
            assert not list(workspace.glob('*launch.json'))
    print('N22 G5 no-launch checks passed.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('part', choices=('M', 'C', 'self-check'))
    parser.add_argument('--build', type=int)
    parser.add_argument('--scoped', type=Path)
    parser.add_argument('--safeguards', type=Path)
    args = parser.parse_args()
    if args.part == 'self-check':
        self_check()
    else:
        part = args.part
        run(part, args.build or (590 if part == 'M' else 591),
            args.scoped or HERE / part / 'scoped_acceptance.json',
            args.safeguards or ROUND[part] / 'safeguards/acceptance_summary.json')


if __name__ == '__main__':
    main()
