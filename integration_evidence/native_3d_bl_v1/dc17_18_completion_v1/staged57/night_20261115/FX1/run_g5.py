"""Run FX1's OFF and ON cases through copies of the FP1 G5 qualification steps."""

from argparse import ArgumentParser
from pathlib import Path
import runpy
import subprocess

HERE = Path(__file__).resolve().parent
FP1 = HERE.parent / 'FP1'
R5 = runpy.run_path(str(FP1 / 'resume5/run_g5.py'))
workflow, cut, sha = (R5[name] for name in ('workflow', 'cut', 'sha'))
timer = runpy.run_path(str(FP1.parents[1] / 'f3b_20261115/check_timer_tree.py'))
metric = runpy.run_path(str(HERE / 'compare_prefix.py'))
ORDER = HERE / 'inputs/FIXED_PARTS_1.md'
ORDER_SHA = '5c6bcd9b1bf25e94fb8a15a52abaa39de783ec30c3f27a7bde0e1615264a8dd5'
OFF_615 = FP1 / 'resume5/G5/OFF/full615_run1'
OFF_591 = workflow.N22 / 'full591_run1'
ON_REF = FP1 / 'on_reference.json'
COUNT_FIELDS = ('trials', 'sweeps', 'visits', 'updates')
OFF_COUNTS = [(7, 800, 14153877, 9853577), (8, 1478, 43004068, 30146553), (8, 1199, 41122066, 28821833)]
ON_COUNTS = [(7, 800, 14153877, 9853577), (8, 1056, 28977415, 21001471), (8, 1246, 28674333, 20088842)]
MODES = ('OFF', 'ON1')


def rank_snapshots(case, rank, root):
    path = case / f'rank_output/1/rank.{rank}/stdout'
    rows = [row for row in timer['snapshots'](path.read_text(errors='replace'))
            if all(row['summary'][key] == value for key, value in
                   (('root', root), ('rank', str(rank)), ('boundary', 'final')))]
    return [row for index, row in enumerate(rows) if index == 0 or row != rows[index - 1]]


def one_operation(snapshot, phase, operation):
    entries = [row for row in snapshot['rows'] if (row['phase'], row['operation']) == (phase, operation)]
    assert len(entries) <= 1
    return (int(entries[0]['calls']), float(entries[0]['exclusive_seconds'])) if entries else (0, 0.0)


def timing_rows(case):
    """Keep output, transfer and metric clocks in their respective timer roots."""
    transfer = []
    for event in (1, 2):
        passage = cut.read(case / f'f3b_event{event}_timer_gate.json')
        assert passage['event'] == event and passage['passed'] and len(passage['ranks']) == 6
        by_rank = []
        for rank in passage['ranks']:
            ledger = rank['ledger']
            assert ledger['passed'] and ledger['summary']['fixedstorage'] == '4120'
            seconds = sum(float(row['exclusive_seconds']) for row in ledger['rows'] if row['phase'] == 'transfer')
            calls, policy = one_operation(ledger, 'transfer', 'mobility_policy')
            prep_calls, _ = one_operation(ledger, 'preparation', 'mobility_policy')
            by_rank.append(dict(rank=rank['rank'], seconds=seconds, mobility_calls=calls, mobility_seconds=policy,
                                preparation_mobility_calls=prep_calls, coverage=float(ledger['summary']['coverage'])))
        assert [row['rank'] for row in by_rank] == list(range(6))
        transfer.append(by_rank)
    output = []
    for snapshot in rank_snapshots(case, 0, 'mesh_output'):
        assert snapshot['summary']['fixedstorage'] == '4120'
        calls, seconds = one_operation(snapshot, 'output', 'mobility_policy')
        output.append(dict(seconds=float(snapshot['summary']['root_seconds']), mobility_calls=calls,
                           mobility_seconds=seconds))
    assert len(output) == 2
    composition = []
    for rank in range(6):
        roots = rank_snapshots(case, rank, 'metric')
        assert len(roots) == 3
        composition.append([one_operation(row, 'metric', 'target_composition')[1] for row in roots])
    counts = metric['read_metric'](case)
    assert len(counts) == 3
    for index, row in enumerate(counts):
        row['target_composition_max_seconds'] = max(rank[index] for rank in composition)
    return dict(transfer=transfer, reference_output_rank0=output, metric=counts)


def differences(before, actual):
    fields = ('mobility_calls', 'preparation_mobility_calls')
    result = [{'event': event, 'rank': left['rank'], 'field': field, 'expected': left[field], 'actual': right[field]}
              for event, (old, new) in enumerate(zip(before['transfer'], actual['transfer'], strict=True), 1)
              for left, right in zip(old, new, strict=True) for field in fields if left[field] != right[field]]
    result.extend({'event': event, 'root': 'mesh_output', 'field': 'mobility_calls',
                   'expected': old['mobility_calls'], 'actual': new['mobility_calls']}
                  for event, (old, new) in enumerate(zip(before['reference_output_rank0'],
                                                       actual['reference_output_rank0'], strict=True), 1)
                  if old['mobility_calls'] != new['mobility_calls'])
    return result


def count_tuples(rows):
    return [tuple(row[key] for key in COUNT_FIELDS) for row in rows]


def fx1_gate(mode, case, on_reference):
    """Check exact seed, metric counts, and the unchanged mobility timer calls."""
    on = mode != 'OFF'
    base = Path(on_reference['repeat_case']) if on else OFF_615
    before, actual = map(timing_rows, (base, case))
    expected_counts = count_tuples(before['metric']) if on else OFF_COUNTS
    actual_counts = count_tuples(actual['metric'])
    authored_counts = ON_COUNTS if on else OFF_COUNTS
    seed = sha(case / 'restart_flow_adap_00000.dat')
    first = None
    if seed != metric['INITIAL_RESTART_SHA']:
        first = {'initial_restart_sha256': seed, 'expected': metric['INITIAL_RESTART_SHA']}
    if first is None:
        first = next(({'metric_computation': index, 'field': field, 'expected': expected[i], 'actual': got[i]}
                      for index, (expected, got) in enumerate(zip(expected_counts, actual_counts, strict=True), 1)
                      for i, field in enumerate(COUNT_FIELDS) if expected[i] != got[i]), None)
    calls = differences(before, actual)
    if first is None and calls:
        first = calls[0]
    return dict(passed=first is None, first_difference=first, initial_restart_sha256=seed, retained_mobility_rows=True,
                expected_counts=list(map(list, expected_counts)), actual_counts=list(map(list, actual_counts)),
                authored_counts=list(map(list, authored_counts)),
                count_reference_changed=expected_counts != authored_counts, reference_case=str(base), before=before,
                actual=actual, mobility_differences=calls)


def run(workspace, build_dir, scoped, on_reference, only):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    build = cut.read(build_dir / 'build_status.json')
    _, pins = workflow.verify_build(build, build_dir / 'source_pins.json')
    label, solver = build['build'], build['binary_sha256']
    assert cut.sha(build['binary']) == solver and cut.terminal(build['wait']['controller'])
    for report_name in ('scoped', 'safeguards'):
        gate = cut.read(scoped / f'{report_name}.json')
        assert gate['passed'] and gate['build'] == label
    sources = [Path(__file__).resolve(), ORDER, ORDER.with_suffix('.sha256'), HERE / 'source_check.md',
               HERE / 'compare_prefix.py', metric['REFERENCE'], ON_REF,
               scoped / 'scoped.json', scoped / 'safeguards.json']
    env = R5['bind_campaign']('on', sources)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    collect = env['event_collector']()
    parse = runpy.run_path(str(workflow.N22 / 'check_connected_events.py'))
    off_reference = workflow.off_reference(payloads)
    results = {}
    off_seal = workspace / 'off_proof.json'
    if only == 'on':
        proof = cut.read(off_seal)
        assert proof['passed'] and proof['binary_sha256'] == solver and proof['source_pins_sha256'] == pins
        off = workspace / 'OFF'
        case = off / f'full{label}_run1'
        for name, digest in proof['receipts'].items():
            assert sha(off / name) == digest, name
        assert workflow.workflow_payloads(case, solver, payloads) == off_reference
        results['OFF'] = cut.read(off / 'run_receipt.json')

    for mode in (('OFF',) if only == 'off' else ('ON1',)):
        on = mode != 'OFF'
        work = workspace / mode
        case = work / f'full{label}_run1'
        config = workflow.select_config(on)
        expected = on_reference['hashes'] if on else off_reference
        assert not (work / 'failed_attempt.json').exists()
        env['prepare'](work, label, build_dir, config)
        stage = 'full event gates'
        try:
            workflow.run_stage(work, 'full', [cut.PYTHON, '-B', str(Path(workflow.__file__)), str(build_dir),
                                               '--output', str(workspace), '--full', mode])
            actual = workflow.seal_run(case, solver, config, payloads, on)
            changed = payloads['compare_hashes'](actual, expected)
            workflow.sealed_write(work / 'payload_identity.json', {'passed': not changed, 'differences': changed,
                                                                   'expected': expected, 'actual': actual})
            stage = 'FX1 result checks'
            extra = fx1_gate(mode, case, on_reference)
            workflow.sealed_write(work / 'fx1_gate.json', extra)
            assert not changed, next(iter(changed.items()), None)
            assert extra['passed'], extra['first_difference']
            stage = 'isolated restart'
            isolated = workflow.isolate_restart(work, label, case)
            stage = 'report rows'
            events = collect(case)
            for event in events:
                event.update(env['report_rows'](case, event['event'], parse))
            info = dict(case=str(case), hashes=actual, restart=str(isolated), events=events, config_sha256=sha(config))
            if on:
                info['time_rule'] = [dict(row, report_only=True) for row in
                                     workflow.timings(results['OFF']['events'], events)]
            workflow.sealed_write(work / 'run_receipt.json', info)
            results[mode] = info
        except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
            workflow.sealed_write(work / 'failed_attempt.json', {
                'passed': False, 'mode': mode, 'stage': stage, 'case': str(case), 'error': str(error),
                'status': cut.read(case / 'status.json') if (case / 'status.json').exists() else None,
            })
            raise
        assert cut.sha(build['binary']) == solver

    if only == 'off':
        off = workspace / 'OFF'
        receipts = [off / name for name in ('run_receipt.json', 'fx1_gate.json', 'payload_identity.json',
                                            'full_wait.json', 'restart_wait.json')]
        workflow.sealed_write(off_seal, dict(passed=True, build=label, binary_sha256=solver, source_pins_sha256=pins,
                                              receipts={path.name: sha(path) for path in receipts}))
        print('FX1 OFF PASSED.', flush=True)
    else:
        workflow.sealed_write(workspace / 'acceptance_summary.json', dict(passed=True, build=label, binary_sha256=solver,
                                                                         source_pins_sha256=pins, runs=results))
        print('FX1 ON PASSED.', flush=True)


def self_check(on_reference):
    assert on_reference['accepted'] and len(on_reference['hashes']) == 6
    off, on = OFF_615, Path(on_reference['repeat_case'])
    for mode, case in (('OFF', off), ('ON1', on)):
        gate = fx1_gate(mode, case, on_reference)
        assert gate['passed'] and not gate['count_reference_changed']
        assert all(x['mobility_calls'] == 1 for x in gate['actual']['reference_output_rank0'])
    assert count_tuples(timing_rows(OFF_591)['metric']) == OFF_COUNTS
    print('FX1 G5 no-launch checks passed against N22 C, build615 OFF and build616 ON.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('--scoped', type=Path, default=HERE / 'scoped')
    parser.add_argument('--output', type=Path, default=HERE / 'G5')
    parser.add_argument('--only', choices=('off', 'on'))
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    assert sha(ORDER) == ORDER_SHA and ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    on_reference = cut.read(ON_REF)
    assert on_reference['accepted'] and on_reference['status'] == 'PASSED'
    if args.self_check:
        self_check(on_reference)
        return
    if args.build is None or args.only is None:
        parser.error('provide the sealed build and --only off or on')
    run(args.output.resolve(), args.build.resolve(), args.scoped.resolve(), on_reference, args.only)


if __name__ == '__main__':
    main()
