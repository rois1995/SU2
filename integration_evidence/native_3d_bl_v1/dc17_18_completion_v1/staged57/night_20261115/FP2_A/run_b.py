"""Run the FP2-A Part B MPI cases, inherited FP1 fixtures and G1 on one pinned build."""

from argparse import ArgumentParser
import json
import os
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
import run_a as a

EXCHANGE = '[.][NativeLocalCoreSurfaceExchange]'
RANKS = (2, 4)
INPUT_CELLS = 12
PART_A = a.HERE / 'part_a/build621/part_a_receipt.json'
PART_A_SHA = '7b54bfb65affac6704a1207d907bec0ed489871a38a74c189f763039fe6fea39'
MPI_ROW = re.compile(r'\bFP2_A MPI_BUILD (?P<fields>.*)$')
SAME = ('splits', 'total_owned_faces', 'faces', 'points', 'traces', 'revision', 'snapshot_bytes')
NUMBERS = (*SAME, 'input_cells', 'output_cells', 'owned_faces', 'peak_rss')


def read_rows(log, pattern):
    log = a.ANSI.sub('', log)
    return [dict(a.FIELD.findall(match['fields'])) for line in log.splitlines(keepends=True)
            if line.endswith('\n') and (match := pattern.search(line.rstrip('\r\n')))]


def compare_build(rank_logs, ranks, input_cells):
    rank_data = {}
    for rank, log in sorted(rank_logs.items()):
        rows = read_rows(log, MPI_ROW)
        rank_data[str(rank)] = rows
    result = {'ranks': ranks, 'by_rank': rank_data, 'passed': False}

    def fail(reason):
        return {**result, 'first_difference': reason}

    if set(rank_logs) != set(range(ranks)):
        return fail({'rank_streams': sorted(rank_logs), 'expected': list(range(ranks))})
    counts = []
    for rank, data in rank_data.items():
        if len(data) != 1:
            return fail({'rank': rank, 'MPI_BUILD_rows': len(data), 'expected': 1})
        row = data[0]
        if row.get('rank') != rank or row.get('ranks') != str(ranks) or row.get('snapshot_equal') != '1':
            return fail({'rank': rank, 'build_row': row})
        missing = [key for key in NUMBERS if not row.get(key, '').isdecimal()]
        if missing:
            return fail({'rank': rank, 'missing_integer_fields': missing})
        counts.append({key: int(row[key]) for key in NUMBERS})
    reference = counts[0]
    for rank, row in enumerate(counts):
        difference = next((key for key in SAME if row[key] != reference[key]), None)
        if difference:
            return fail({'rank': rank, 'field': difference, 'observed': row[difference],
                         'expected': reference[difference]})
    for observed, expected, label in (
        (sum(row['input_cells'] for row in counts), input_cells, 'input_cells'),
        (sum(row['owned_faces'] for row in counts), reference['faces'], 'owned_faces_sum'),
        (reference['total_owned_faces'], reference['faces'], 'total_owned_faces'),
    ):
        if observed != expected:
            return fail({'field': label, 'observed': observed, 'expected': expected})
    if (reference['splits'], reference['faces'], reference['points']) != (2, 24, 14) or reference['snapshot_bytes'] <= 0:
        return fail({'splits': reference['splits'], 'snapshot_bytes': reference['snapshot_bytes']})
    peak = sum(row['peak_rss'] for row in counts)
    limit = a.MEMORY * 1024
    result.update(job_peak_rss_bytes=peak, job_peak_limit_bytes=limit)
    if not all(row['peak_rss'] > 0 for row in counts) or peak > limit:
        return fail({'field': 'job_peak_rss_bytes', 'observed': peak, 'maximum': limit})
    return {**result, 'passed': True, 'state': {key: reference[key] for key in SAME},
            'output_cells_sum': sum(row['output_cells'] for row in counts)}


def run(build, output, verify_only, corrected_a=None):
    status_path, manifest = build / 'build_status.json', build / 'source_pins.json'
    status = a.cut.read(status_path)
    binary, source_hash = a.verify_build(status, manifest)
    assert not status['new_warnings']
    header = a.cut.REPO / 'Common/include/adaptation/CNativeLocalSurface3D.hpp'
    assert a.cut.read(manifest)[str(header)] == a.sha(header)
    baseline = subprocess.check_output(['git', 'show', f'{a.BASE}:{a.NRR}'], cwd=a.cut.REPO)
    assert (a.cut.REPO / a.NRR).read_bytes() == baseline, 'Part B changed the NRR call path.'
    original, _, pins, _, _, parity = a.evidence_inputs()
    assert a.cut.read(PART_A)['passed']
    pins.update({str(status_path): a.sha(status_path), str(manifest): source_hash,
                 str(binary): status['test_binary_sha256'], str(Path(__file__).resolve()): a.sha(Path(__file__)),
                 str(PART_A): PART_A_SHA})
    original['verify'](pins)
    output = output or a.HERE / 'part_b' / f"build{status['build']}"
    if verify_only:
        return {'verified': True, 'launched': False, 'build': status['build'], 'output': str(output),
                'selector': EXCHANGE, 'ranks': RANKS}

    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert status['wait']['terminal'] and a.cut.terminal(status['wait']['controller'])
    for key in tuple(os.environ):
        if key.startswith(('SU2_NATIVE_', 'SU2_FP2_', 'SU2_FP1_')):
            del os.environ[key]
    prov = {'build': status['build'], 'test_binary_sha256': status['test_binary_sha256'],
            'source_pins_sha256': source_hash, 'output': str(output), 'pins': pins}
    results = {}
    for ranks in RANKS:
        case = output / f'exchange/rank{ranks}'
        run_result = a.fixture(case, binary, ranks, EXCHANGE, {}, prov)
        rank_logs = {}
        for name, digest in run_result['artifacts'].items():
            path = case / name
            if path.name != 'stdout':
                continue
            rank = int(path.parent.name.rsplit('.', 1)[1])
            assert rank not in rank_logs and a.sha(path) == digest, path
            rank_logs[rank] = path.read_text(errors='replace')
        state = compare_build(rank_logs, ranks, INPUT_CELLS)
        a.sealed(case / 'state_comparison.json', state)
        assert state['passed'], state['first_difference']
        results[str(ranks)] = state

    regressions = corrected_a or output
    inherited = prov
    if corrected_a:
        receipt = a.cut.read(corrected_a / 'part_a_receipt.json')
        assert receipt['passed'] and not a.cut.read(corrected_a / 'correction_1_alarm.json')['stopped']
        for field in ('build', 'test_binary_sha256', 'source_pins_sha256'):
            assert receipt[field] == prov[field], field
        inherited = {key: receipt[key] for key in prov}
        assert inherited['output'] == str(corrected_a)
    fixtures = []
    for tag, cohorts in a.FIXTURES.items():
        for ranks, selector in cohorts:
            env = {'SU2_FP1_G_OWNED_BASELINE': parity} if tag == 'H' else {}
            case = regressions / f'FP1/{tag}/rank{ranks}'
            if corrected_a:
                assert (case / 'fixture_status.json').is_file()
            a.fixture(case, binary, ranks, selector, env, inherited)
            fixtures.append(str(case / 'fixture_status.json'))
    assert len(fixtures) == 18
    g1 = regressions / 'G1'
    if not corrected_a:
        subprocess.run([a.cut.PYTHON, '-B', str(a.FP1 / 'run_g1.py'), str(g1), str(status_path), str(manifest)],
                       cwd=a.cut.REPO, check=True)
    counts = {}
    for cohort, count in zip(('serial', 'mpi2', 'mpi4'), (375, 114, 228), strict=True):
        row = a.cut.read(g1 / cohort / 'comparison.json')
        assert row['identity_pass'] and row['records'] == count
        pins_g1 = a.cut.read(g1 / cohort / 'pins.json')
        assert pins_g1['test_binary'] == status['test_binary_sha256'] and pins_g1['test_source_pins'] == source_hash
        wait = a.cut.read(g1 / cohort / 'test_wait.json')
        assert wait['terminal'] and a.cut.terminal(wait['controller'])
        counts[cohort] = count
    original['verify'](pins)
    assert a.verify_build(status, manifest) == (binary, source_hash)
    path = output / 'part_b_receipt.json'
    a.sealed(path, {**prov, 'passed': True, 'NRR_matches_base': a.BASE, 'state_comparisons': results,
                    'fp1_fixture_receipts': fixtures, 'G1': counts, 'regression_root': str(regressions)})
    return {'passed': True, 'build': status['build'], 'receipt': str(path), 'G1': counts}


def self_check():
    def log(rank):
        return (f'\x1b[0mFP2_A MPI_BUILD rank={rank} ranks=2 input_cells=6 output_cells=8 splits=2 '
                'owned_faces=12 total_owned_faces=24 faces=24 points=14 traces=20 revision=3 '
                'snapshot_bytes=42 snapshot_equal=1 peak_rss=100\n')

    logs = {rank: log(rank) for rank in range(2)}
    assert compare_build(logs, 2, 12)['passed']
    assert not compare_build(logs, 2, 13)['passed']
    assert not compare_build({**logs, 1: logs[1].replace('snapshot_equal=1', 'snapshot_equal=0')}, 2, 12)['passed']
    assert not compare_build({**logs, 1: logs[1].replace('owned_faces=12', 'owned_faces=11')}, 2, 12)['passed']


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', nargs='?', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--corrected-a', type=Path)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--verify-only', action='store_true')
    group.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print('Part B reconstruction receipt checks passed.')
    elif args.build:
        print(json.dumps(run(args.build.resolve(), args.output.resolve() if args.output else None, args.verify_only,
                             args.corrected_a.resolve() if args.corrected_a else None),
                         indent=2))
    else:
        parser.error('supply the pinned build directory or --self-check')
