"""Run and seal one FP2-0 serial measurement without copying its input data."""

from argparse import ArgumentParser
import json
import os
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
NIGHT = HERE.parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(NIGHT / 'FP1'))
from run_g1 import cut, run_case, sha, verify_build
from run_fp0 import peak_kib

FP0_PINS = NIGHT / 'FP0/case_input_manifest.json'
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_STEP0.md'
ORDER_SHA = '0d558dc46f3c8c6f0fd11391009acdaa7d56741750f704a458e5196080432331'
G5 = NIGHT / 'N4/section6_3_after_N11/round_8_5t_P/G5/full587'
TEST_SOURCE = cut.REPO / 'UnitTests/SU2_CFD/adaptation'
SELECTORS = {
    'bounds': 'NativeFastPathStage2Bounds',
    'projection': 'NativeFastPathStage2Projection',
    'split': 'NativeFastPathStage2Split',
}
EXPECTED = {
    'bounds': ('OUT1_load', 'B1', 'B2'),
    'projection': ('OUT1_load', 'G1', 'G2', 'G2'),
    'split': ('IN1_load', 'IN1_setup', 'P_L', 'P_D', 'P_entities', 'P_STRICT', 'T_IN1', 'T_PD', 'P3', 'P4'),
}
MARKER = re.compile(r'^FP2_0 (\w+) (\{.*\})\s*$')


def inputs():
    checksum = ORDER.with_suffix('.sha256')
    assert sha(ORDER) == ORDER_SHA and checksum.read_text().split() == [ORDER_SHA, ORDER.name]
    reference = cut.read(FP0_PINS)
    in1 = G5 / 'm6.su2.native_ref'
    out1 = G5 / 'adapted_mesh_adap_00001.su2.native_ref'
    sensor = NIGHT / 'FP0/cases/B/sensor.bin'
    assert Path(reference['B']['sensor']) == sensor
    pinned = {
        'SU2_FP2_IN': (in1, reference['B']['pinned_sources'][str(in1)]),
        'SU2_FP2_OUT': (out1, reference['A']['pinned_sources'][str(out1)]),
        'SU2_FP2_SENSOR': (sensor, reference['B']['sensor_sha256']),
    }
    for path, expected in pinned.values():
        assert sha(path) == expected, path
    for path, header in ((in1, 'SU2_NATIVE_REFERENCE_3D 7\n'), (out1, 'SU2_NATIVE_REFERENCE_3D 9\n')):
        with path.open() as stream:
            assert stream.readline() == header, path
    return {name: {'path': str(path), 'sha256': digest} for name, (path, digest) in pinned.items()}


def measurements(lines):
    """Keep each complete JSON row, plus provenance for any malformed line."""
    rows, malformed = [], []
    for line_number, line in enumerate(lines, 1):
        if not line.startswith('FP2_0 '):
            continue
        match = MARKER.fullmatch(line.rstrip('\r\n'))
        try:
            if match is None:
                raise ValueError('incomplete measurement marker')
            payload = json.loads(match[2])
            rows.append({'name': match[1], 'line': line_number, 'value': payload})
        except ValueError as error:
            malformed.append({'line': line_number, 'reason': str(error), 'text': line.rstrip()})
    return rows, malformed


def missing_rows(name, rows, splits_only):
    names = list(EXPECTED[name])
    if splits_only:
        names = names[:5]
    for row in rows:
        if row['name'] in names:
            names.remove(row['name'])
    return names


def active_build(status_file, pins_file):
    state = cut.read(status_file)
    binary, digest = verify_build(state, pins_file)
    wait = state['wait']
    assert wait['terminal'] and cut.terminal(wait['controller']) and not state.get('new_warnings')
    sources = cut.read(pins_file)
    test_files = [TEST_SOURCE / 'CNativeFastPathStage2_tests.cpp',
                  *sorted((TEST_SOURCE / 'fixtures').glob('native_fast_path_stage2*.inc')),
                  cut.REPO / 'UnitTests/meson.build']
    assert len(test_files) >= 4, test_files
    for path in test_files:
        assert str(path) in sources and sha(path) == sources[str(path)], path
    return binary, digest, state


def measure(name, status_file, pins_file, case, splits_only):
    if Path('/proc/1/comm').read_text().strip() != 'systemd':
        raise RuntimeError('Use the actual host for FP2-0 process and resource receipts.')
    binary, source_hash, build = active_build(status_file, pins_file)
    selector = '[.]' + f'[{SELECTORS[name]}]'
    provenance = {
        'case': name, 'build': build['build'], 'selector': selector, 'ranks': 1, 'seconds_limit': 5400,
        'splits_only': splits_only, 'test_binary': str(binary), 'test_binary_sha256': build['test_binary_sha256'],
        'build_status': str(status_file), 'build_status_sha256': sha(status_file),
        'source_pins': str(pins_file), 'source_pins_sha256': source_hash,
        'fp0_manifest': str(FP0_PINS), 'fp0_manifest_sha256': sha(FP0_PINS),
        'order_sha256': ORDER_SHA, 'launcher_sha256': sha(Path(__file__).resolve()), 'inputs': inputs(),
    }
    receipt_path = case / 'measurement.json'
    pins_path = case / 'provenance.json'
    if receipt_path.is_file():
        receipt = cut.read(receipt_path)
        assert cut.read(pins_path) == provenance and receipt['provenance_sha256'] == sha(pins_path)
        assert receipt['log_sha256'] == sha(case / 'test_outer.log')
        assert cut.terminal(receipt['controller'])
        return receipt
    if (case / 'test_launch.json').exists():
        wait_path = case / 'test_wait.json'
        if not wait_path.is_file():
            raise RuntimeError(f'Inspect the existing controller in {case / "test_launch.json"} before a retry.')
        assert cut.read(pins_path) == provenance
        wait = cut.read(wait_path)
    else:
        case.mkdir(parents=True, exist_ok=False)
        cut.save(pins_path, provenance)
        for key in list(os.environ):
            if key.startswith(('SU2_FP2_', 'SU2_NATIVE_')):
                del os.environ[key]
        environment = {key: row['path'] for key, row in provenance['inputs'].items()}
        if name == 'split':
            environment['SU2_FP2_VALUES'] = str(case / 'values.jsonl')
        if splits_only:
            environment['SU2_FP2_SPLITS_ONLY'] = '1'
        wait = run_case(case, binary, 1, selector, 5400, environment, cwd=case)
    assert wait['terminal'] and cut.terminal(wait['controller'])
    assert active_build(status_file, pins_file)[:2] == (binary, source_hash)
    assert inputs() == provenance['inputs'] and sha(status_file) == provenance['build_status_sha256']
    log = case / 'test_outer.log'
    with log.open(errors='replace') as stream:
        rows, malformed = measurements(stream)
    time_log = case / 'time.txt'
    peak = peak_kib(time_log) if time_log.is_file() else None
    files = {path.name: sha(path) for path in
             (log, time_log, case / 'values.jsonl', case / 'test_wait.json', case / 'test_launch.json',
              case / 'test_host_preflight.json', case / 'command.json') if path.is_file()}
    missing = missing_rows(name, rows, splits_only)
    status = ('outer_time_limit' if wait['returncode'] == 124 else
              'measurement_terminal' if wait['returncode'] == 0 else 'fixture_failed')
    receipt = {
        'case': name, 'build': build['build'], 'test_binary_sha256': build['test_binary_sha256'],
        'provenance_sha256': sha(pins_path), 'log_sha256': files[log.name], 'artifacts': files,
        'returncode': wait['returncode'], 'controller': wait['controller'], 'status': status,
        'elapsed_seconds': wait['elapsed_seconds'], 'process_peak_kib': peak,
        'missing_rows': missing, 'malformed_rows': malformed, 'rows': rows,
    }
    cut.save(receipt_path, receipt)
    print(f'{name} {status}, {len(rows)} rows, peak {peak} KiB. Receipt {receipt_path}', flush=True)
    return receipt


def self_check():
    rows, malformed = measurements([
        'irrelevant',
        'FP2_0 G1 {"samples": 1}',
        'FP2_0 G2 {"pass": 1, "error": "stopped"}',
        'FP2_0 G2 {"pass": 2}',
        'FP2_0 OUT1_load {bad',
        'FP2_0 P4 {"status": "not_applicable"}',
    ])
    assert len(rows) == 4 and malformed[0]['line'] == 5
    assert missing_rows('projection', rows, False) == ['OUT1_load']
    assert rows[-1]['value']['status'] == 'not_applicable'
    print('FP2-0 input and marker self-checks passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build_status', nargs='?', type=Path)
    parser.add_argument('case', nargs='?', choices=SELECTORS)
    parser.add_argument('--source-pins', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--splits-only', action='store_true')
    parser.add_argument('--check-inputs', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.check_inputs or args.self_check:
        print(json.dumps(inputs(), indent=2))
        if args.self_check:
            self_check()
        return
    if not args.build_status or not args.case or (args.splits_only and args.case != 'split'):
        parser.error('give BUILD_STATUS and CASE, with --splits-only available for split')
    status = args.build_status.resolve()
    pins = (args.source_pins or status.with_name('source_pins.json')).resolve()
    name = 'split_dev' if args.splits_only else args.case
    case = (args.output or HERE / 'cases' / name).resolve()
    assert HERE in case.parents, 'Write measurements inside the FP2-0 evidence folder.'
    receipt = measure(args.case, status, pins, case, args.splits_only)
    raise SystemExit(receipt['status'] == 'fixture_failed')


if __name__ == '__main__':
    main()
