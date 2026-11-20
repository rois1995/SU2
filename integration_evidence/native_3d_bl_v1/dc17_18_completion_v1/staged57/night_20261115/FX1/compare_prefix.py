"""Copy the FP1 prefix comparison and enforce the accepted scheduling calls."""

from pathlib import Path
import re
import runpy

HERE = Path(__file__).resolve().parent
RESUME6 = HERE.parent / 'FP1/resume6'
BASE = runpy.run_path(str(RESUME6 / 'compare_scheduling.py'))
cut = runpy.run_path(str(RESUME6.parent / 'run_g1.py'))['cut']
REFERENCE = RESUME6 / 'scheduling_calls_reference.json'
INITIAL_RESTART_SHA = '61ebd3b0cd3a00889784772378ce7897ec6245c517f2ee382492288c43dbafd4'
NUMBER = r'([0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)'
METRIC_WORK = re.compile(
    r'Native metric work: (\d+) complexity trials, (\d+) total gradation sweeps, (\d+) owned-point visits; '
    rf'gradation {NUMBER} s, halo {NUMBER} s, sweep reductions {NUMBER} s \(maximum accumulated rank times\)\.')
UPDATES = re.compile(r'Native gradation tensor updates: (\d+) of (\d+) owned-point visits\.')
SENSOR = re.compile(r'Native sensor gradation: [^\r\n]+')


def schedule_difference(rows, gate):
    key = lambda row: (row['snapshot'], row['rank'])
    expected = {key(row): row['calls'] for row in cut.read(REFERENCE) if row['case'] == gate}
    assert len(expected) == {'G2': 6, 'GQ': 12}[gate]
    observed = {key(row): row['calls'] for row in rows}
    if len(rows) != len(observed) or observed.keys() != expected.keys():
        return {'scheduling_row_set': sorted(observed), 'expected': sorted(expected)}
    return next(({'snapshot': boundary, 'rank': rank, 'calls': observed[boundary, rank], 'expected': value}
                 for (boundary, rank), value in expected.items() if observed[boundary, rank] != value), None)


def compare_cases(part, gate, reference, candidate):
    result = BASE['compare_cases'](part, gate, reference, candidate)
    if result['passed']:
        first = schedule_difference(result['scheduling_calls'], gate)
        result.update(passed=first is None, first_difference=first,
                      scheduling_reference=str(REFERENCE))
    return result


def read_metric(case):
    """Pair each metric summary with its update and sensor rows, stripping MPI prefixes."""
    clean = BASE['N4'].strip_mpi_prefixes
    rows = []
    for raw in (case / 'run.log').open(errors='replace'):
        line = clean(raw)
        if match := METRIC_WORK.search(line):
            trials, sweeps, visits = map(int, match.groups()[:3])
            gradation, halo, reductions = map(float, match.groups()[3:])
            rows.append(dict(trials=trials, sweeps=sweeps, visits=visits, gradation_seconds=gradation,
                             halo_seconds=halo, sweep_reductions_seconds=reductions))
        elif match := UPDATES.search(line):
            assert rows and 'updates' not in rows[-1] and int(match[2]) == rows[-1]['visits'], case
            rows[-1]['updates'] = int(match[1])
        elif match := SENSOR.search(line):
            assert rows and 'sensor' not in rows[-1], case
            rows[-1]['sensor'] = match[0].strip()
    assert rows and all(row.keys() >= {'updates', 'sensor'} for row in rows), case
    return rows


def metric_gate(reference, candidate, count):
    expected, actual = map(read_metric, (reference, candidate))
    assert len(actual) == len(expected) == count
    reduce = lambda rows: [{k: v for k, v in row.items() if not k.endswith('_seconds')} for row in rows]
    before, after = reduce(expected), reduce(actual)
    restart_sha = cut.sha(candidate / 'restart_flow_adap_00000.dat')
    first = {'initial_restart': restart_sha, 'expected': INITIAL_RESTART_SHA} if restart_sha != INITIAL_RESTART_SHA else None
    if first is None:
        first = next(({'metric': index, 'field': key, 'expected': row[key], 'actual': new[key]}
                      for index, (row, new) in enumerate(zip(before, after, strict=True), 1)
                      for key in row if row[key] != new[key]), None)
    return {'passed': first is None, 'first_difference': first, 'initial_restart_sha256': restart_sha,
            'reference': expected, 'candidate': actual}


def self_check():
    rows = [row for row in cut.read(REFERENCE) if row['case'] == 'G2']
    assert schedule_difference(rows, 'G2') is None
    assert schedule_difference(rows[:-1], 'G2')
    assert schedule_difference([dict(row, calls=row['calls'] + 1) for row in rows], 'G2')
    case = RESUME6 / 'scoped/prefix/cases/G2'
    metric = metric_gate(case, case, 1)
    assert metric['passed']
    assert [metric['candidate'][0][key] for key in ('trials', 'sweeps', 'visits', 'updates')] == [7, 800, 14153877, 9853577]
    print('FX1 prefix comparator self-check passed.')


if __name__ == '__main__':
    self_check()
