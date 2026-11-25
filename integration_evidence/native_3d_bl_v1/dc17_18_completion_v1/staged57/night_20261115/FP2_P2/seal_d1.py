"""Seal the signed D1 comparison before first collapse can launch."""

from argparse import ArgumentParser
from math import isfinite
from pathlib import Path

import run_p2 as p

MODES = ('default-N1', 'campaign-N1-1')
KEY_FIELDS = ('rank', 'pass', 'bin', 'direct_eligible')
KEYS = {(0, 1, length, direct) for length in range(5) for direct in (0, 1)}
WORK_FIELDS = ('classification_work', 'traversal_work', 'scope_build_work')
SETTINGS = {'SU2_NATIVE_FP2_M2': '1', 'SU2_NATIVE_GEOMETRY_WORK_TRACE': '0',
            'SU2_NATIVE_FP2_M1': '0', 'SU2_NATIVE_FP2_M2_TABLE': '0',
            'SU2_NATIVE_FP2_M3_FACETS': '0'}


def unsigned(value):
    if not str(value).isascii() or not str(value).isdecimal():
        raise ValueError(f'Expected a nonnegative integer, observed {value!r}')
    return int(value)


def stage_totals(measurement):
    ranks = measurement['rows_by_rank']
    rank = ranks['0']
    if any(rows.get('STAGE') or rows.get('OBSERVER') for key, rows in ranks.items() if key != '0'):
        raise ValueError('Unexpected FP2_M2 stage or observer row on a nonzero rank')
    stages = rank.get('STAGE', ())
    keys = [tuple(unsigned(row[field]) for field in KEY_FIELDS) for row in stages]
    if len(keys) != len(KEYS) or set(keys) != KEYS:
        raise ValueError(f'Expected the ten distinct FP2_M2 STAGE keys, observed {keys}')
    observers = rank.get('OBSERVER', ())
    if len(observers) != 1 or (observers[0].get('rank'), observers[0].get('pass'),
                               observers[0].get('status')) != ('0', '1', 'complete'):
        raise ValueError(f'Expected one complete FP2_M2 OBSERVER on rank 0, pass 1, observed {observers}')
    totals = {'classification_work': sum(unsigned(row['classification_work']) for row in stages)}
    for field in WORK_FIELDS[1:]:
        try:
            totals[field] = sum(unsigned(row[field]) for row in stages)
        except (KeyError, ValueError):
            totals[field] = None
    return totals


def qualified_measurement(summary, measurement, closure):
    if not (summary['status'] == 'passed' and summary['evaluate_qualified']
            and summary['switches']['passed'] and measurement['passed']
            and measurement['wait']['returncode'] == 0 and measurement['supervisor_stop'] is None
            and measurement['case'] == 'N1' and measurement['ranks'] == 1):
        raise ValueError('Selected N1 receipt did not attest a qualified serial result')
    expected = {**SETTINGS, p.SWITCH: str(closure)}
    if summary['switches']['environment'] != expected:
        raise ValueError(f'Unexpected N1 process settings {summary["switches"]["environment"]}')
    case = Path(summary['used']).parent
    command = p.cut.read(case / 'command.json')['argv']
    for key, value in expected.items():
        if [word for word in command if word.startswith(key + '=')] != [key + '=' + value]:
            raise ValueError(f'Selected N1 child did not attest {key}={value}')
    config = case / 'run.cfg'
    if [word for word in command if word.startswith('SU2_NATIVE_FROZEN_CONFIG=')] != [
            f'SU2_NATIVE_FROZEN_CONFIG={config}'] or len(p.m2.HAUSD.findall(config.read_bytes())) != 1:
        raise ValueError('Selected N1 child did not attest one pinned ADAP_HAUSD= 1E-4 assignment')
    rows = measurement['rows_by_rank']['0'].get('NATIVE3D_LOCAL_SURFACE', ())
    row = rows[0] if len(rows) == 1 and rows[0].get('pass') == '1' else {}

    def seconds(fields, name):
        try:
            value = float(fields[name])
        except (KeyError, TypeError, ValueError):
            return None
        return value if isfinite(value) and value >= 0 else None

    return {**stage_totals(measurement), 'elapsed_seconds': seconds(measurement['wait'], 'elapsed_seconds'),
            'step_c_seconds': seconds(row, 'seconds_c'), 'bound_seconds': seconds(row, 'seconds_c_bound'),
            'settings': {'ADAP_HAUSD': '1E-4', **expected}, 'config_sha256': p.cut.sha(config)}


def path_for(build):
    return (p.HERE / f'build{build}' if isinstance(build, int) else build) / 'd1_promotion.json'


def decision(build, finalize_measurement_failure):
    path = path_for(build)
    receipt, _, pins = p.prepare(path.parent)
    pins.update(p.hashes((Path(__file__),)))
    record = {**p.fingerprint(receipt), 'status': 'not evaluated', 'promote': False,
              'first_difference': None, 'pins': pins, 'selected': {}, 'observations': {},
              'finalize_measurement_failure': finalize_measurement_failure}
    pending = []
    for closure, mode in enumerate(MODES):
        summary = p.selected_summary(receipt['build'], mode)
        if not summary.is_file():
            pending.append({'mode': mode, 'field': 'selected trial receipt', 'observed': 'missing'})
            continue
        selected = p.verify_summary(receipt, mode, summary)
        measurement = Path(selected['used'])
        pins.update(p.hashes((summary, measurement, measurement.with_name('run.cfg'))))
        record['selected'][mode] = {'status': selected['status'], 'summary': str(summary),
                                    'measurement': str(measurement)}
        if selected['status'] == 'OPEN' and selected['watchdog'].get('status') == 'open':
            pending.append({'mode': mode, 'field': 'watchdog', 'observed': 'OPEN'})
            continue
        if selected['status'] == 'FAILED':
            record.update(status='FAILED', first_difference=selected['first_difference'],
                          failure_kind='trial qualification')
            continue
        try:
            record['observations'][mode] = qualified_measurement(
                selected, p.cut.read(measurement), closure)
        except (KeyError, TypeError, ValueError) as error:
            pending.append({'mode': mode, 'field': 'D1 measurement', 'observed': str(error)})
    record['measurement_findings'] = pending
    record['observation_findings'] = [{'mode': mode, 'field': field, 'observed': 'not available'}
                                      for mode, fields in record['observations'].items()
                                      for field, value in fields.items() if value is None]
    if record['status'] == 'FAILED':
        return path, record
    if pending:
        first = pending[0]
        if any(row['field'] == 'watchdog' for row in pending):
            record.update(status='OPEN', first_difference=first)
        elif finalize_measurement_failure:
            record.update(status='FAILED', first_difference=first, failure_kind='test measurement')
        else:
            record.update(first_difference=first)
        return path, record
    control, candidate = (record['observations'][mode] for mode in MODES)
    if control['config_sha256'] != candidate['config_sha256']:
        record.update(status='FAILED' if finalize_measurement_failure else 'not evaluated',
                      first_difference={'field': 'N1 frozen configuration SHA256',
                                        'observed': candidate['config_sha256'],
                                        'expected': control['config_sha256']},
                      failure_kind='test measurement')
        return path, record
    before, after = (row['classification_work'] for row in (control, candidate))
    if after < before:
        record.update(status='passed', promote=True)
    else:
        record.update(status='FAILED', failure_kind='result gate',
                      first_difference={'field': 'classification_work', 'observed': after,
                                        'required_strictly_below': before})
    return path, record


def seal_d1(build, finalize_measurement_failure=False):
    path, record = decision(build, finalize_measurement_failure)
    if record['status'] != 'not evaluated':
        if path.is_file():
            assert p.cut.read(path) == record, path
        else:
            pending = path.with_suffix('.tmp')
            p.cut.save(pending, record)
            pending.replace(path)
    return path, record


def verify_d1(build):
    path = path_for(build)
    stored = p.cut.read(path)
    _, current = decision(build, stored['finalize_measurement_failure'])
    assert stored == current, path
    return path, stored


def historical_measurement(branch, closure):
    path = p.HERE.parent / f'FP2_P1/{branch}/build670/N1/trial1/case_summary.json'
    p.inherited.c3.c2.committed(path, 'dec45f8a71')
    summary = p.cut.read(path)
    used = Path(summary['used'])
    assert p.cut.sha(used) == summary['attempts'][str(used)], used
    measurement = p.cut.read(used)
    return qualified_measurement(summary, measurement, closure), measurement


def self_check():
    for branch, closure, expected in [('default', 0, 463538756), ('campaign', 1, 685848058)]:
        measured, measurement = historical_measurement(branch, closure)
        assert measured['classification_work'] == expected, branch
        del measurement['rows_by_rank']['0']['STAGE'][0]['traversal_work']
        assert stage_totals(measurement)['traversal_work'] is None
        assert stage_totals(measurement)['classification_work'] == expected
        measurement['rows_by_rank']['0']['STAGE'][-1] = measurement['rows_by_rank']['0']['STAGE'][0]
        try:
            stage_totals(measurement)
        except ValueError as error:
            assert 'ten distinct' in str(error)
        else:
            raise AssertionError('Duplicated stage key passed D1 validation')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--finalize-measurement-failure', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    else:
        if args.build is None:
            parser.error('provide a build number')
        _, record = (verify_d1(args.build) if args.verify
                     else seal_d1(args.build, args.finalize_measurement_failure))
        print(record['status'], record['observations'], record['first_difference'])
        raise SystemExit(p.TERMINAL[record['status']])
