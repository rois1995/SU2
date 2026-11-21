"""Run and seal one serial FP2-0b measurement on a pinned input and build."""

from argparse import ArgumentParser
import gzip
import hashlib
import json
import os
from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory

HERE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
FP2_0 = HERE.parent / 'FP2_0/run_measurements.py'
previous = runpy.run_path(str(FP2_0))
cut = previous['cut']
sha = previous['sha']
measurements = previous['measurements']
peak_kib = previous['peak_kib']

ORDER = HERE / 'inputs/FAST_PATH_STAGE2_STEP0B.md'
ORDER_SHA = '1074f5410e4bf3bedcde83179140d5366d89dfbfc9a47e83e239ff8e57d4dc4a'
SELECTOR = '[.][NativeFastPathStage2Split]'
SECONDS = 5400
MEMORY_MIB = 19073
ORIENTATION_REFUSAL = 'Physical face violates its model entity/orientation.'
REQUIRED_ROWS = ('IN1_load', 'IN1_setup', 'IN1_sensor', 'P_feature_start',
                 'P_L', 'P_D', 'P_entities', 'P_STRICT', 'P3', 'P4')
LEDGERS = ('points.jsonl', 'faces.jsonl', 'values.jsonl')
COMMAND_ARTIFACTS = ('command.json', 'test_outer.log', 'time.txt', 'test_wait.json',
                     'test_launch.json', 'test_host_preflight.json')


def checked_inputs():
    checksum = ORDER.with_suffix('.sha256')
    assert sha(ORDER) == ORDER_SHA and checksum.read_text().split() == [ORDER_SHA, ORDER.name], ORDER
    return previous['inputs']()


def row_values(rows, name):
    return [row['value'] for row in rows if row['name'] == name]


def missing_rows(rows):
    emitted = {row['name'] for row in rows}
    return [name for name in REQUIRED_ROWS if name not in emitted]


def run_b_reason(receipt):
    if receipt['case'] != 'A':
        return 'Run B requires a Run A receipt.'
    p3 = row_values(receipt['rows'], 'P3')
    if len(p3) != 1:
        return 'Run A did not emit one complete P3 row.'
    if p3[0].get('accepted') is True:
        return 'P3 accepted Run A.'
    if p3[0].get('accepted') is not False or p3[0].get('constructor_error') != ORIENTATION_REFUSAL:
        return p3[0].get('constructor_error') or 'P3 did not report the specified orientation refusal.'
    strict = row_values(receipt['rows'], 'P_STRICT')
    if len(strict) != 1 or receipt['malformed_rows']:
        return 'Run A lacks a complete strict comparison and diagnostic log.'
    counts = strict[0].get('PD', {}).get('all', {})
    if counts.get('faces') is None or counts.get('contains_measured') != counts['faces']:
        return 'Run A did not test entity membership on every final face.'
    diagnoses = row_values(receipt['rows'], 'P_ENTITY_FAILURE')
    if len(diagnoses) != counts.get('contains_failed'):
        return 'Run A did not save a diagnosis for every failed final face.'
    if any(not row.get('face') or len(row.get('corners', ())) != 3 for row in diagnoses):
        return 'Run A has an incomplete failing-face diagnosis.'
    return None


def case_environment(case, run, pins):
    environment = {name: entry['path'] for name, entry in pins.items()}
    environment['SU2_FP2_VALUES'] = str(case / 'values.jsonl')
    if run == 'B':
        environment['SU2_FP2_STRICT_CHILDREN'] = '1'
    return environment


def archived_file(case, name, uncompressed_digest):
    path = case / name
    packed = path.with_suffix(path.suffix + '.gz')
    with gzip.open(packed, 'rb') as stream:
        assert hashlib.file_digest(stream, 'sha256').hexdigest() == uncompressed_digest, packed
    return packed


def log_rows(case):
    raw = case / 'test_outer.log'
    if raw.is_file():
        with raw.open(errors='replace') as stream:
            return measurements(stream)
    with gzip.open(raw.with_suffix('.log.gz'), 'rt', errors='replace') as stream:
        return measurements(stream)


def verified(case, pinned_inputs):
    receipt_file, provenance_file = (case / name for name in ('measurement.json', 'provenance.json'))
    receipt, provenance = map(cut.read, (receipt_file, provenance_file))
    assert sha(provenance_file) == receipt['provenance_sha256'], case
    assert receipt['case'] == provenance['case'] and receipt['case'] in ('A', 'B'), case
    assert receipt['build'] == provenance['build'] and provenance['order_sha256'] == ORDER_SHA, case
    assert receipt['test_binary_sha256'] == provenance['test_binary_sha256'], case
    assert provenance['inputs'] == pinned_inputs, case
    assert provenance['launcher_sha256'] == sha(Path(__file__).resolve()), case
    assert provenance['inherited_launcher_sha256'] == sha(FP2_0), case
    for key in ('build_status', 'source_pins', 'fp0_manifest'):
        assert sha(Path(provenance[key])) == provenance[f'{key}_sha256'], (case, key)
    build = cut.read(provenance['build_status'])
    assert build['passed'] and not build.get('new_warnings'), case
    assert (build['test_binary_sha256'], build['source_pins_sha256']) == (
        receipt['test_binary_sha256'], provenance['source_pins_sha256']), case

    for name, digest in receipt['artifacts'].items():
        path = case / name
        if path.exists():
            assert sha(path) == digest, path
        else:
            assert name in (*LEDGERS, 'test_outer.log'), path
            archived_file(case, name, digest)
    for name in ('test_outer.log', *LEDGERS):
        if name in receipt['artifacts'] and Path(str(case / name) + '.gz').exists():
            archived_file(case, name, receipt['artifacts'][name])
    assert set(COMMAND_ARTIFACTS) <= receipt['artifacts'].keys(), case
    wait = cut.read(case / 'test_wait.json')
    launch = cut.read(case / 'test_launch.json')
    assert wait['actual_host_proc'] and launch['actual_host_proc'], case
    assert wait['terminal'] and wait['controller'] == receipt['controller'] == launch['controller'], case
    assert cut.terminal(wait['controller']), case
    assert wait['returncode'] == receipt['returncode'] and wait['elapsed_seconds'] == receipt['elapsed_seconds'], case
    command = cut.read(case / 'command.json')
    assert command['ranks'] == provenance['ranks'] == 1, case
    assert command['seconds_limit'] == provenance['seconds_limit'] == SECONDS, case
    assert provenance['memory_limit_mib'] == MEMORY_MIB, case
    assert command['selector'] == provenance['selector'] == SELECTOR, case
    argv = command['argv']
    assert argv[:3] == ['env', '-C', str(case)] and argv[-2:] == [provenance['test_binary'], SELECTOR], case
    assert argv[-11:-2] == ['nice', '-n', '19', '/usr/bin/time', '-v', '-o', str(case / 'time.txt'),
                            'timeout', str(SECONDS)], case
    env = case_environment(case, receipt['case'], pinned_inputs)
    assert env == provenance['environment'], case
    assert {entry for entry in argv if entry.startswith(('SU2_FP2_', 'SU2_NATIVE_'))} == {
        f'{key}={value}' for key, value in env.items()}, case
    for key in ('OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS'):
        assert f'{key}=1' in argv, case
    rows, malformed = log_rows(case)
    assert rows == receipt['rows'] and malformed == receipt['malformed_rows'], case
    assert missing_rows(rows) == receipt['missing_rows'], case
    assert peak_kib(case / 'time.txt') == receipt['process_peak_kib'], case
    peak = receipt['process_peak_kib']
    assert receipt['process_peak_within_limit'] == (None if peak is None else peak <= MEMORY_MIB * 1024), case
    return dict(receipt, case_path=str(case), provenance=provenance, receipt_sha256=sha(receipt_file))


def measure(run, status_file, pins_file, case, run_a):
    if Path('/proc/1/comm').read_text().strip() != 'systemd':
        raise RuntimeError('Run FP2-0b on the actual host for process and resource receipts.')
    binary, source_hash, build = previous['active_build'](status_file, pins_file)
    inputs = checked_inputs()
    fp0_manifest = previous['FP0_PINS']
    provenance = {
        'case': run, 'build': build['build'], 'selector': SELECTOR, 'ranks': 1,
        'seconds_limit': SECONDS, 'memory_limit_mib': MEMORY_MIB,
        'test_binary': str(binary), 'test_binary_sha256': build['test_binary_sha256'],
        'build_status': str(status_file), 'build_status_sha256': sha(status_file),
        'source_pins': str(pins_file), 'source_pins_sha256': source_hash,
        'fp0_manifest': str(fp0_manifest), 'fp0_manifest_sha256': sha(fp0_manifest),
        'order_sha256': ORDER_SHA, 'launcher_sha256': sha(Path(__file__).resolve()),
        'inherited_launcher_sha256': sha(FP2_0), 'inputs': inputs,
        'environment': case_environment(case, run, inputs),
    }
    if run == 'B':
        first = verified(run_a, inputs)
        reason = run_b_reason(first)
        if reason:
            raise RuntimeError(f'Run B is not authorized. {reason}')
        assert first['test_binary_sha256'] == provenance['test_binary_sha256'], 'Run A binary differs.'
        provenance['run_a'] = {'path': str(run_a), 'receipt_sha256': first['receipt_sha256']}

    provenance_file = case / 'provenance.json'
    receipt_file = case / 'measurement.json'
    if receipt_file.is_file():
        receipt = verified(case, inputs)
        assert receipt['provenance'] == provenance, case
        return receipt
    if case.exists():
        wait_file = case / 'test_wait.json'
        if not wait_file.is_file():
            raise RuntimeError(f'Inspect existing launch, handle and log in {case} before retrying.')
        assert cut.read(provenance_file) == provenance, case
        wait = cut.read(wait_file)
    else:
        case.mkdir(parents=True, exist_ok=False)
        cut.save(provenance_file, provenance)
        for key in list(os.environ):
            if key.startswith(('SU2_FP2_', 'SU2_NATIVE_')):
                del os.environ[key]
        wait = previous['run_case'](case, binary, 1, SELECTOR, SECONDS,
                                    provenance['environment'], cwd=case)
    assert wait['terminal'] and cut.terminal(wait['controller']), case
    assert previous['active_build'](status_file, pins_file)[:2] == (binary, source_hash), case
    assert checked_inputs() == inputs and sha(status_file) == provenance['build_status_sha256'], case
    rows, malformed = log_rows(case)
    peak = peak_kib(case / 'time.txt')
    files = {name: sha(case / name) for name in (*COMMAND_ARTIFACTS, *LEDGERS)
             if (case / name).is_file()}
    for name in ('test_outer.log', *LEDGERS):
        if name in files:
            cut.pack(case / name)
            archived_file(case, name, files[name])
    status = ('outer_time_limit' if wait['returncode'] == 124 else
              'measurement_terminal' if wait['returncode'] == 0 else 'fixture_failed')
    receipt = {
        'case': run, 'build': build['build'], 'test_binary_sha256': build['test_binary_sha256'],
        'provenance_sha256': sha(provenance_file), 'artifacts': files, 'status': status,
        'returncode': wait['returncode'], 'controller': wait['controller'],
        'elapsed_seconds': wait['elapsed_seconds'], 'process_peak_kib': peak,
        'process_peak_within_limit': None if peak is None else peak <= MEMORY_MIB * 1024,
        'missing_rows': missing_rows(rows), 'malformed_rows': malformed, 'rows': rows,
    }
    cut.save(receipt_file, receipt)
    print(f'Run {run} {status}, {len(rows)} rows, peak {peak} KiB. Receipt {receipt_file}', flush=True)
    return verified(case, inputs)


def check_receipt():
    # Exercise the real verifier with no build, process launch, or fixture data writes.
    with TemporaryDirectory(prefix='fp2-0b-receipt-') as temp:
        case = Path(temp)
        binary = case / 'test_driver'
        pins_file = case / 'source_pins.json'
        status_file = case / 'build_status.json'
        fp0_manifest = previous['FP0_PINS']
        cut.save(pins_file, {})
        cut.save(status_file, {'passed': True, 'test_binary_sha256': 'driver',
                               'source_pins_sha256': sha(pins_file)})
        provenance = {
            'case': 'A', 'build': 1, 'selector': SELECTOR, 'ranks': 1,
            'seconds_limit': SECONDS, 'memory_limit_mib': MEMORY_MIB,
            'test_binary': str(binary), 'test_binary_sha256': 'driver',
            'build_status': str(status_file), 'build_status_sha256': sha(status_file),
            'source_pins': str(pins_file), 'source_pins_sha256': sha(pins_file),
            'fp0_manifest': str(fp0_manifest), 'fp0_manifest_sha256': sha(fp0_manifest),
            'order_sha256': ORDER_SHA, 'launcher_sha256': sha(Path(__file__).resolve()),
            'inherited_launcher_sha256': sha(FP2_0), 'inputs': {},
            'environment': case_environment(case, 'A', {}),
        }
        cut.save(case / 'provenance.json', provenance)
        controller = {'pid': int(Path('/proc/sys/kernel/pid_max').read_text()) + 1, 'start_ticks': 1}
        cut.save(case / 'test_launch.json', {'actual_host_proc': True, 'controller': controller})
        cut.save(case / 'test_wait.json', {'actual_host_proc': True, 'controller': controller,
                                          'terminal': True, 'returncode': 0, 'elapsed_seconds': 0.1})
        cut.save(case / 'test_host_preflight.json', {})
        command = ['env', '-C', str(case), 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
                   f"SU2_FP2_VALUES={case / 'values.jsonl'}", 'nice', '-n', '19', '/usr/bin/time', '-v', '-o',
                   str(case / 'time.txt'), 'timeout', str(SECONDS), str(binary), SELECTOR]
        cut.save(case / 'command.json', {'argv': command, 'ranks': 1, 'selector': SELECTOR, 'seconds_limit': SECONDS})
        (case / 'time.txt').write_text('Maximum resident set size (kbytes): 42\n')
        log = case / 'test_outer.log'
        log.write_text('FP2_0 P3 {"accepted":true}\n')
        rows, malformed = log_rows(case)
        receipt = {
            'case': 'A', 'build': 1, 'test_binary_sha256': 'driver',
            'provenance_sha256': sha(case / 'provenance.json'),
            'artifacts': {name: sha(case / name) for name in COMMAND_ARTIFACTS},
            'controller': controller, 'returncode': 0, 'elapsed_seconds': 0.1,
            'rows': rows, 'malformed_rows': malformed, 'missing_rows': missing_rows(rows),
            'process_peak_kib': 42, 'process_peak_within_limit': True,
        }
        cut.save(case / 'measurement.json', receipt)
        assert verified(case, {})['rows'] == rows
        packed = cut.pack(log)
        log.unlink()
        assert verified(case, {})['rows'] == rows
        with gzip.open(packed, 'wt') as stream:
            stream.write('FP2_0 P3 {"accepted":false}\n')
        try:
            verified(case, {})
        except AssertionError:
            pass
        else:
            raise AssertionError('Altered receipt log passed verification.')


def self_check():
    rows, malformed = measurements([
        'FP2_0 P_STRICT {"PD":{"all":{"faces":1,"contains_measured":1,"contains_failed":1}}}',
        'FP2_0 P_ENTITY_FAILURE {"face":{"id":1},"corners":[{},{},{}]}',
        f'FP2_0 P3 {json.dumps(dict(accepted=False, constructor_error=ORIENTATION_REFUSAL))}',
    ])
    start = {'case': 'A', 'rows': rows, 'malformed_rows': malformed}
    assert not run_b_reason(start)
    assert run_b_reason(dict(start, rows=[rows[0], rows[2]]))
    assert run_b_reason(dict(start, malformed_rows=[{'line': 2}]))
    assert run_b_reason(dict(start, rows=rows[:2]))
    for outcome in ({'accepted': True}, {'accepted': False, 'constructor_error': 'trace failure'},
                    {'accepted': False, 'constructor_error': ''}):
        assert run_b_reason(dict(start, rows=[*rows[:2], {'name': 'P3', 'value': outcome}]))
    assert 'SU2_FP2_STRICT_CHILDREN' not in case_environment(HERE, 'A', {})
    assert case_environment(HERE, 'B', {})['SU2_FP2_STRICT_CHILDREN'] == '1'
    assert missing_rows(rows) == list(name for name in REQUIRED_ROWS if name not in ('P_STRICT', 'P3'))
    check_receipt()
    print('FP2-0b input pins, row parser, Run B activation, and receipt verifier self-checks passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build_status', nargs='?', type=Path)
    parser.add_argument('run', nargs='?', choices=('A', 'B'))
    parser.add_argument('--source-pins', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--run-a', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(checked_inputs(), indent=2))
        self_check()
        return
    if not args.build_status or not args.run or (args.run == 'A' and args.run_a):
        parser.error('give BUILD_STATUS and A or B, with --run-a available for B')
    status = args.build_status.resolve()
    pins = (args.source_pins or status.with_name('source_pins.json')).resolve()
    case = (args.output or HERE / 'cases' / args.run).resolve()
    run_a = (args.run_a or HERE / 'cases/A').resolve()
    assert HERE in case.parents and HERE in run_a.parents, 'Use case paths below the FP2-0b evidence folder.'
    receipt = measure(args.run, status, pins, case, run_a)
    raise SystemExit(receipt['status'] == 'fixture_failed')


if __name__ == '__main__':
    main()
