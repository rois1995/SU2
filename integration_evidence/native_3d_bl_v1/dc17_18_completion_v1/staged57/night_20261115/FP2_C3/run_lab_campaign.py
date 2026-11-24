"""Run the six FP2-C3 lab trials in order and seal each completed result."""

from pathlib import Path
import sys

sys.dont_write_bytecode = True
import compare_lab
import run_c2_lab as runner


def main(build):
    label = runner.a.cut.read(build / 'build_status.json')['build']
    progress = runner.HERE.parent / 'progress.md'
    campaign = runner.HERE / f'lab_campaign{label}'
    campaign.mkdir(exist_ok=True)
    sequence = [(name, repeat) for name in ('N1', 'N6', 'N6P') for repeat in (1, 2)]
    for index, (name, repeat) in enumerate(sequence):
        trial_receipt = campaign / f'{name}_{repeat}.json'
        already_recorded = trial_receipt.is_file()
        if already_recorded:
            saved = runner.a.cut.read(trial_receipt)
            path, saved_measurement = compare_lab.load_trial(label, name, repeat)
            assert str(path) == saved['measurement'] and runner.a.sha(path) == saved['sha256'], path
            assert saved_measurement['passed'] == (saved['status'] == 'passed'), path
            if not saved_measurement['passed']:
                print(f'FP2-C3 build{label} {name} trial {repeat} already ended with {saved["status"]}.',
                      flush=True)
                return 75 if saved['status'] == 'open' else 1
            continue
        result, watchdog = runner.run(build, name, repeat, False, False)
        retry = watchdog.get('status') == 'retry_allowed'
        if retry:
            result, watchdog = runner.run(build, name, repeat, False, True)
        path = runner.case_path(label, name, repeat, watchdog_retry=retry)
        receipt = path / 'measurement.json'
        measurement = result['measurement']
        status = 'passed' if result['passed'] else watchdog.get('status', 'failed')
        runner.a.sealed(trial_receipt, {
            'status': status, 'measurement': str(receipt), 'sha256': runner.a.sha(receipt),
            'wait': result['wait'], 'failed_limits': measurement['failed_limits'], 'problems': measurement['problems'],
            'watchdog': watchdog,
        })
        next_action = 'stop for gate disposition'
        if result['passed']:
            next_action = f'run {sequence[index+1][0]} trial {sequence[index+1][1]}' if index+1 < len(sequence) else (
                'compare repetitions and seal the lab qualification')
        elapsed = result['wait']['elapsed_seconds']
        if result['passed']:
            passes = sum(len(rows.get('embedding', ())) for rows in result['rows_by_rank'].values())
            gates = (f"strict_checked={measurement['proof']['strict_checked']}, unexplained=0, model_gap_entities=0, "
                     f"zero embedding violations on {passes} rank-pass checks")
        else:
            first = (measurement['failed_limits'] or measurement['problems'] or [status])[0]
            gates = f'first recorded difference={first}'
        message = (f'FP2-C3 build{label} {name} trial {repeat} {status} in {elapsed:.3f} s, '
                   f'{gates}. Receipt {receipt.relative_to(runner.HERE.parent)}. Next {next_action}.')
        with progress.open('a') as stream:
            stream.write(f'\n{message}\n')
        print(message, flush=True)
        if not result['passed']:
            return 75 if status == 'open' else 1
    result = compare_lab.run_campaign(label)
    print({key: result[key] for key in ('passed', 'cases', 'report')}, flush=True)
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main(Path(sys.argv[1]).resolve()))
