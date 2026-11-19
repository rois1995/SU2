"""Run N21's frozen two-event workflow gates and record their results."""
import argparse
from collections import Counter
from datetime import datetime
from decimal import Decimal
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
sys.path.insert(0, str(NIGHT / 'N20'))
from run_w import F_WORKFLOW, actual_host, cut, retarget

from proof_gates import N4, STRICT, campaign_proofs, identity, proof_outcomes, stream_identity

P_G5 = cut.PHASE / 'round_8_5t_P/G5'
W_G5 = cut.PHASE / 'round_8_5t_W/G5'
T0 = cut.PHASE / 'round_8_5r_B/G5/full574'
PINS = NIGHT / 'section6_1/accepted_reference_payloads.json'
SUMMARY = cut.PHASE / 'phase_C/collect_g5_summary.py'
FACTOR = Decimal('0.95')


def progress(part, message):
    stamp = datetime.now().astimezone().isoformat(timespec='seconds')
    with (NIGHT / 'progress.md').open('a') as file:
        file.write(f'\n- {stamp} N21 {part}. {message}\n')


def workflow(part, workspace):
    """Bind N18's tested workflow stages and the accepted construction prefix comparator."""
    runner = retarget(F_WORKFLOW, (
        ("assert receipt['passed'] and receipt['build'] == build, path",
         "assert (receipt.get('passed') or receipt.get('state') == 'PASSED') and receipt['build'] == build, path"),
        ("row[0].startswith('NATIVE3D_M28_')",
         "row[0].startswith(('NATIVE3D_M28_', 'NATIVE3D_M29_', 'NATIVE3D_FINE_', 'NATIVE3D_FINAL_'))"),
    ))
    runner['WORKSPACE'] = workspace
    if part == 'P':
        runner['RUNNERS'] = ('run_full',)
    return runner


def prepare(part, build, workspace, runner):
    """Bind the exact accepted-path or changed-mesh command chain without launching it."""
    if part == 'W':
        runner['prepare'](build)
    else:
        workspace.mkdir(parents=True)
        source = cut.PHASE / 'phase_D/G5'
        bound = {}
        for name in runner['HELPERS']:
            target = workspace / name
            shutil.copyfile(source / name, target)
            bound[target] = source / name
        full = workspace / f'run_full{build}.py'
        original = source / 'run_full572.py'
        text = original.read_text().replace('572', str(build))
        compile(text, str(full), 'exec')
        full.write_text(text)
        bound[full] = original
        pinned = {'build': build, 'source': {str(path): cut.sha(path) for path in bound.values()},
                  'bound': {str(path): cut.sha(path) for path in bound}, 'comparison_source': {}}
        cut.save(workspace / 'helper_pins.json', pinned)
    sources = (Path(__file__).resolve(), HERE / 'proof_gates.py', F_WORKFLOW, STRICT, SUMMARY, PINS,
               HERE / 'inputs/IMPLEMENTATION_8_5T.md', HERE / 'inputs/SHA256SUMS')
    cut.save(workspace / 'adapter_pins.json', {str(path): cut.sha(path) for path in sources})


def bind(part, build, workspace, scoped, safeguards):
    actual_host()
    runner = workflow(part, workspace)
    if not workspace.exists():
        prepare(part, build, workspace, runner)
    assert not (workspace / 'failed_attempt.json').exists(), 'Existing failure requires R5 disposition.'
    assert not (workspace / 'acceptance_summary.json').exists(), 'G5 is already terminal.'
    runner['verify_pins'](cut.read(workspace / 'adapter_pins.json'))
    return runner, runner['preflight'](build, scoped, safeguards)


def collect(case):
    """Read standing gates and add the means of each final exclusive phase."""
    events = runpy.run_path(str(SUMMARY))['collect_events'](case)
    for event in events:
        timed = cut.read(case / f"f3b_event{event['event']}_timer_gate.json")
        for phase in ('preparation', 'construction', 'quality', 'final_proof'):
            per_rank = [sum(float(row['exclusive_seconds']) for row in entry['ledger']['rows']
                            if row['phase'] == phase) for entry in timed['ranks']]
            event['phase_exclusive_clocks'][phase]['seconds_mean'] = sum(per_rank) / len(per_rank)
    return events


def run_part_p(build, scoped, safeguards):
    runner, binary_sha = bind('P', build, P_G5, scoped, safeguards)
    fixture = cut.read(HERE / 'fixtures/acceptance_summary.json')
    assert fixture['passed'] and fixture['build'] == build
    case = P_G5 / f'full{build}'
    preflight = cut.read(P_G5 / 'run_full_selfcheck.json')
    assert preflight['passed'] and preflight['accepted_payload_count'] == 6
    stage = 'G5 standing and six-file identity'
    try:
        progress('P', f'Build{build} passed scoped checks and safeguards. Starting the two-event G5.')
        runner['run_or_resume'](build, 'P_full', 'run_full', '--case', case)
        payloads = runner['verify_full'](case, binary_sha)
        assert payloads['hashes'] == cut.read(PINS)['hashes']
        stage = 'G5 shared final proof on each rank'
        proof = {'T0': campaign_proofs(T0, False), 'T1': campaign_proofs(case, True)}
        cut.save(P_G5 / 'proof_rows.json', {'runs': proof})
        purpose = proof_outcomes(proof['T1'], proof['T0'])
        cut.save(P_G5 / 'purpose_rule.json', purpose)
        assert purpose['standing_gates_passed'], str(P_G5 / 'purpose_rule.json')
        stage = 'G5 old integer, timer and modeled-byte identity'
        result = identity(T0, case)
        cut.save(P_G5 / 'identity.json', result)
        assert result['passed'], str(P_G5 / 'identity.json')
        events = {'T0': collect(T0), 'T1': collect(case)}
    except (AssertionError, OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        failed_attempt(P_G5, runner, stage, binary_sha, [case], error)
        raise
    passed = purpose['passed']
    cut.save(P_G5 / 'acceptance_summary.json', {
        'passed': passed, 'build': build, 'binary_sha256': binary_sha, 'standing_gates_passed': True,
        'case': str(case), 'reference_case': str(T0), 'hashes': payloads['hashes'], 'events': events,
        'purpose': purpose, 'identity_receipt': str(P_G5 / 'identity.json'),
        'proof_receipt': str(P_G5 / 'proof_rows.json'),
    })
    outcome = 'passed' if passed else 'failed'
    progress('P', f'Build{build} matched all six P6c/P7 payloads and old rows. The proof purpose gate {outcome}.')
    return passed


def wide_purpose(baseline, first, second):
    """Compare measured native maxima against the Part P run with the confirmed factor."""
    clocks = []
    for run in (baseline, first, second):
        assert [row['event'] for row in run] == [1, 2]
        values = {row['event']: Decimal(str(row['native_seconds'])) for row in run}
        assert all(seconds.is_finite() and seconds > 0 for seconds in values.values())
        clocks.append(values)
    t1, run1, run2 = clocks
    selected = {event: min(run1[event], run2[event]) for event in t1}
    combined = sum(selected.values())
    limit = FACTOR * sum(t1.values())
    events = [
        {'event': event, 'T1_native_limit_seconds': str(value), 'run1_native_max_seconds': str(run1[event]),
         'run2_native_max_seconds': str(run2[event]), 'smaller_native_max_seconds': str(selected[event]),
         'passed': selected[event] <= value} for event, value in t1.items()
    ]
    combined_pass = combined <= limit
    return {'passed': combined_pass and all(row['passed'] for row in events), 'events': events,
            'factor': str(FACTOR), 'smaller_native_max_sum_seconds': str(combined),
            'combined_limit_seconds': str(limit), 'combined_passed': combined_pass}


def failed_attempt(workspace, runner, stage, binary_sha, cases, error):
    """Record intact terminal cases and preserve the required W native rejection rows."""
    terminal = [receipt for case in cases if (receipt := runner['observe_failed_case'](case))]
    incomplete = []
    logs = [(case, log) for case in cases for log in sorted(case.glob('rank_output/*/rank.*/stdout'))]
    for case, log in logs:
        event = Counter()
        for row in N4.marker_rows(log):
            if row.marker != 'NATIVE3D rank':
                continue
            rank = row.rank()
            event[rank] += 1
            if row.fields.get('complete') == '0':
                incomplete.append({'case': case.name, 'event': event[rank], 'rank': rank,
                                   'log': str(log), 'line': row.line, 'complete': 0})
    if workspace == W_G5 and incomplete:
        dump = HERE / 'reports/W_dump.txt'
        dump.parent.mkdir(exist_ok=True)
        with dump.open('x') as file:
            for case, stdout in logs:
                for log in (stdout, stdout.with_name('stderr')):
                    if not log.is_file():
                        continue
                    with log.open(errors='replace') as source:
                        for raw in source:
                            line = N4.strip_mpi_prefixes(raw)
                            if line.startswith(('NATIVE3D_M28_', 'NATIVE3D_M29_', 'NATIVE3D_FINE_', 'NATIVE3D_FINAL_')):
                                file.write(f'{case.name} {log.parent.name}/{log.name} {line.rstrip()}\n')
    cut.save(workspace / 'failed_attempt.json', {
        'passed': False, 'interrupted_stage': stage, 'error': str(error), 'purpose_gate_evaluated': False,
        'binary_sha256': binary_sha, 'terminal_cases': terminal, 'incomplete_native_rows': incomplete,
    })


def run_part_w(build, scoped, safeguards):
    accepted = cut.read(P_G5 / 'acceptance_summary.json')
    assert accepted['passed'] and accepted['purpose']['passed']
    runner, binary_sha = bind('W', build, W_G5, scoped, safeguards)
    t1 = Path(accepted['case'])
    assert runner['verify_full'](t1, accepted['binary_sha256'])['hashes'] == accepted['hashes']
    first = W_G5 / f'full{build}_run1'
    second = W_G5 / f'full{build}_run2'
    stage = 'G5 run one'
    try:
        progress('W', f'Build{build} passed scoped checks and safeguards. Starting G5 run one.')
        runner['run_or_resume'](build, 'W_run1', 'run_full', '--case', first)
        first_receipt = runner['verify_full'](first, binary_sha)
        stage = 'T0 event-one construction prefix'
        runner['prefix_identity'](first)
        stage = 'isolated event-two restart'
        progress('W', 'Run one passed its workflow and exact T0 prefix. Starting the isolated event-two restart.')
        runner['run_or_resume'](build, 'W_restart', 'run_restart', '--source-case', first)
        restart = runner['verify_restart'](build, first)
        stage = 'G5 run two'
        progress('W', 'The isolated restart passed. Starting G5 run two on the same binary.')
        runner['run_or_resume'](build, 'W_run2', 'run_full', '--case', second)
        second_receipt = runner['verify_full'](second, binary_sha)
        stage = 'six-file repeatability'
        compared = {
            name: {'run1': first_receipt['hashes'].get(name), 'run2': second_receipt['hashes'].get(name)}
            for name in sorted(first_receipt['hashes'].keys() | second_receipt['hashes'].keys())
        }
        identical = len(compared) == 6 and all(row['run1'] == row['run2'] for row in compared.values())
        cut.save(W_G5 / 'repeat_payloads.json', {'passed': identical, 'sha256_by_run': compared})
        assert identical, str(W_G5 / 'repeat_payloads.json')
        stage = 'G5 final proof reports'
        proof = {name: campaign_proofs(case, True) for name, case in (('run1', first), ('run2', second))}
        cut.save(W_G5 / 'proof_rows.json', {'runs': proof})
        events = {**accepted['events'], 'run1': collect(first), 'run2': collect(second)}
    except (AssertionError, OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        failed_attempt(W_G5, runner, stage, binary_sha, [first, second], error)
        raise
    purpose = wide_purpose(events['T1'], events['run1'], events['run2'])
    cut.save(W_G5 / 'purpose_rule.json', purpose)
    passed = purpose['passed']
    cut.save(W_G5 / 'acceptance_summary.json', {
        'passed': passed, 'build': build, 'binary_sha256': binary_sha, 'standing_gates_passed': True,
        'event_one_prefix_identity': True, 'isolated_restart_receipt': str(restart),
        'repeat_payloads_exact': True, 'purpose': purpose, 'events': events,
        'two_run_payload_sha256': second_receipt['hashes'], 'T0_case': str(T0), 'T1_case': str(t1),
        'proof_receipt': str(W_G5 / 'proof_rows.json'),
    })
    if passed:
        cut.save(W_G5 / 'qualified_reference_payloads.json',
                 {'accepted': True, 'hashes': second_receipt['hashes'], 'binary_sha256': binary_sha,
                  'first_case': str(first), 'repeat_case': str(second),
                  'acceptance_sha256': cut.sha(W_G5 / 'acceptance_summary.json')})
    outcome = 'passed' if passed else 'failed'
    progress('W', f'Both workflows and isolated restart passed with six exact matching files. The time rule {outcome}.')
    return passed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('part', choices=('P', 'W', 'self-check'))
    parser.add_argument('--build', type=int)
    parser.add_argument('--scoped', type=Path)
    parser.add_argument('--safeguards', type=Path)
    args = parser.parse_args()
    if args.part == 'self-check':
        self_check()
        return
    part = args.part
    build = args.build or (587 if part == 'P' else 588)
    scoped = args.scoped or cut.PHASE / f'N21{part}/validation/acceptance_summary.json'
    safeguards = args.safeguards or cut.PHASE / f'round_8_5t_{part}/safeguards/acceptance_summary.json'
    run = run_part_p if part == 'P' else run_part_w
    raise SystemExit(0 if run(build, scoped, safeguards) else 2)


def self_check():
    """Exercise pinned adapters, exact directional time edges and historical reports without MPI."""
    def rows(a, b):
        return [{'event': 1, 'native_seconds': a}, {'event': 2, 'native_seconds': b}]

    t1 = rows(2000, 1000)
    assert wide_purpose(t1, rows(1850, 1030), rows(1900, 1000))['passed']
    assert not wide_purpose(t1, rows(1850.001, 1030), rows(1900, 1000))['passed']
    assert not wide_purpose(t1, rows(1700, 1000.001), rows(1800, 1010))['passed']
    t0_proof = campaign_proofs(T0, False)
    assert all(len(e['ranks']) == 6 and all(r['certification_calls'] == 2 for r in e['ranks'])
               for e in t0_proof)
    exact = identity(T0, cut.PHASE / 'phase_N6/G5/full578')
    assert exact['passed'] and exact['timer_snapshots'] == 96
    measure = [{**event, 'ranks': [{**row, 'certification_exclusive_seconds': '53.4',
                                  'proof': {'rank': str(row['rank']), 'shared': '1', 'own_roots': '1',
                                            'reference_facets': '6'}} for row in event['ranks']]}
               for event in t0_proof]
    assert proof_outcomes(measure)['passed']
    measure[1]['ranks'][5]['certification_exclusive_seconds'] = '53.4001'
    assert not proof_outcomes(measure)['passed']
    with TemporaryDirectory(prefix='n21-workflow-') as temp:
        temporary = Path(temp)
        for rank in range(6):
            name = f'rank_output/1/rank.{rank}'
            target = temporary / 'synthetic-proof' / name
            target.mkdir(parents=True)
            (target / 'stdout').symlink_to(T0 / name / 'stdout')
            with (target / 'stderr').open('w') as output:
                for event in (1, 2):
                    output.write(f'<stderr>:NATIVE3D_FINAL_PROOF rank={rank} entities=1 shared=1 '
                                 f'own_roots={event} reference_facets={6 * event}\n')
                    if rank == 0:
                        output.write(f'<stderr>:NATIVE3D_FINAL_CERT entity={event}/1/4 faces=17 within=1\n')
        synthetic = campaign_proofs(temporary / 'synthetic-proof', True)
        assert [event['certificates'][0]['entity'] for event in synthetic] == ['1/1/4', '2/1/4']
        assert all(row['root_coverage_report_matches'] for row in proof_outcomes(synthetic)['events'])
        historical = T0 / 'rank_output/1/rank.0/stdout'
        candidate = temporary / 'candidate-stdout'
        raw = historical.read_text()
        allowed = raw.replace('phase=final_proof operation=communication calls=7',
                              'phase=final_proof operation=communication calls=16')
        allowed = allowed.replace('atlas_bytes=4185262916 global_work=2040954',
                                  'atlas_bytes=4185262915 global_work=2040954')
        candidate.write_text(allowed)
        assert raw != allowed and stream_identity(historical, candidate, 0)['passed']
        candidate.write_text(allowed.replace('atlas_bytes=4185262915', 'atlas_bytes=4185262917'))
        check = stream_identity(historical, candidate, 0)
        assert check['modeled_byte_increases'][0]['field'] == 'atlas_bytes' and not check['passed']
        candidate.write_text(allowed.replace('phase=final_proof operation=certification calls=2',
                                             'phase=final_proof operation=certification calls=3'))
        check = stream_identity(historical, candidate, 0)
        assert check['timer_first_difference']['operation'] == 'certification' and not check['passed']
        for part, build in (('P', 587), ('W', 588)):
            workspace = Path(temp) / part / 'G5'
            runner = workflow(part, workspace)
            prepare(part, build, workspace, runner)
            pins = cut.read(workspace / 'helper_pins.json')
            for name in ('bound', 'source', 'comparison_source'):
                runner['verify_pins'](pins[name])
            runner['verify_pins'](cut.read(workspace / 'adapter_pins.json'))
    wide = workflow('W', W_G5)
    assert wide['compare_event_one'](T0, cut.PHASE / 'round_8_5s_W/G5/full585_run1')['passed']
    assert [row['event'] for row in collect(T0)] == [1, 2]
    print('N21 G5 no-launch checks passed.')


if __name__ == '__main__':
    main()
