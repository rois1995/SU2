"""Run and report the correction 1 fixtures and frozen hand-over checks."""

from argparse import ArgumentParser
from decimal import Decimal
import json
from pathlib import Path
import re
import runpy
import subprocess
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
import run_lab as lab

import report_b as report
import run_fp2_b_g5 as full

scope = full.scope

HERE = lab.HERE
a = lab.a
OLD_LAB = Path(lab.__file__).resolve()
DRIVER = Path(__file__).resolve()
ORDER = HERE / 'inputs/FP2_B_CORRECTION_1.md'
ORDER_SHA = 'b735b39ea86da2b780d8108e79782466f9d8eb5f492b570b87c9b3b160455b45'
SOURCE_CHECK = HERE / 'source_check_correction_1.md'
BASE_COMMIT = '457822008c'
GATE_CASES = ('N1', 'N6', 'N1Invariant')
INSTRUMENTED = {'N1Invariant': 'N1', 'N6Diagnosis': 'N6'}
RUNS = {name: lab.RUNS[name] for name in ('N1', 'N6')}
RUNS.update({'N1Invariant': (1, 'HandoverInvariant'), 'N6Diagnosis': (6, 'HandoverDiagnosis')})
FACE_LIMIT = {'N1': (95413,), 'N6': (95305, 95422)}
BASELINE_PREPARE = lab.prepare
BASELINE_EVALUATE = lab.evaluate
EXTRA_ROWS = re.compile(r'^(FP2_B (?:pass|end|bad|state|proof|invariant|split|short_edge)|'
                        r'FP2_A LAB_S_PROOF|NATIVE3D_LOCAL_SURFACE) (.+)$')
PATH_FIELDS = ('waits_neighbor', 'path_splits', 'path_max')


def prepare(build, extra_pins=()):
    receipt, binary, pins = BASELINE_PREPARE(build)
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    pins.update({str(path): a.sha(path) for path in
                 (DRIVER, OLD_LAB, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, *extra_pins)})
    return receipt, binary, pins


def build_fingerprints(receipt):
    return {key: receipt[key] for key in lab.FINGERPRINTS}


def case_path(build, name):
    return HERE / 'cases' / f'build{build}' / name


def scoped_fixtures(output, fingerprints):
    """Authenticate the fixtures at the pre-lab checkpoint."""
    gate = scope.verified(output / 'fixtures.json', fingerprints)
    assert gate['fp1_cohorts'] == 18 and gate['G1'] == list(scope.prior.COHORTS.values())
    for name in gate['receipts']:
        path = output / name
        if path.name == 'fixture_status.json':
            scope.verify_fixture(path)
    for cohort, count in scope.prior.COHORTS.items():
        case = output / 'G1' / cohort
        row = a.cut.read(case / 'comparison.json')
        assert row['identity_pass'] and row['records'] == count
        pins = a.cut.read(case / 'pins.json')
        assert pins['test_binary'] == fingerprints['test_binary_sha256']
        assert pins['test_source_pins'] == fingerprints['source_pins_sha256']
        assert a.resumed(case)['returncode'] == row['candidate_returncode']
        lab.archived.verify_artifacts(case, {str(Path(log['stdout']).relative_to(case)): log['sha256']
                                             for log in row['candidate_sources']})
    return gate


def run_scoped(build, mode):
    receipt, binary, pins = prepare(build)
    original, measurements, inherited_pins, in1, _, parity = a.evidence_inputs()
    prior = scope.prior
    support = (Path(scope.__file__).resolve(), Path(prior.__file__).resolve(),
               Path(prior.b.__file__).resolve(), Path(prior.legacy.__file__).resolve(),
               Path(prior.prefix.__file__).resolve(), prior.FX1, prior.SCHEDULING.parent / 'compare_scheduling.py',
               prior.prefix.CAPTURE, a.FP1.parents[1] / 'shared_engine_regression_gates.json')
    pins.update({**inherited_pins, **prior.PINS, **{str(path): a.sha(path) for path in support}})
    original['verify'](pins)
    fingerprints = build_fingerprints(receipt)
    output = HERE / 'scoped' / f'build{receipt["build"]}'
    output.mkdir(parents=True, exist_ok=True)
    provenance = {**fingerprints, 'output': str(output), 'pins': pins}
    a.sealed(output / 'provenance.json', provenance)

    if mode == 'default':
        verify_handover(fingerprints)
        assert not (output / 'default_failure.json').exists(), output
        return scope.default(build, output, receipt, original, pins)
    if (output / 'fixtures.json').is_file():
        return scoped_fixtures(output, fingerprints)
    with patch.object(scope, 'verify_fixtures', scoped_fixtures):
        return scope.fixtures(build, output, receipt, binary, provenance, original, measurements, in1, parity)


def verify_handover(fingerprints):
    paths = []
    for name in GATE_CASES:
        directory = case_path(fingerprints['build'], name)
        row = lab.checked_receipt(directory, fingerprints)
        assert row['case'] == name and row['passed'], directory
        paths.append(directory / 'measurement.json')
    invariant = a.cut.read(paths[-1])['measurement']
    assert invariant['compared_gate_sha256'] == a.sha(paths[0]) and invariant['violations'] == 0
    return paths


def surface_integers(surface):
    return {key: int(value) for key, value in surface.items()
            if not key.startswith(('seconds_', 'first_')) and key not in ('outward_max', 'v_changes')}


def compare_invariant(data, gate):
    baseline = gate['rows_by_rank']['0']
    actual = data.get(0, {})
    want = [surface_integers(row) for row in baseline['NATIVE3D_LOCAL_SURFACE']]
    got = [surface_integers(row) for row in actual.get('NATIVE3D_LOCAL_SURFACE', ())]
    differences = {}
    if len(want) != len(got):
        differences['surface_rows'] = {'observed': len(got), 'expected': len(want)}
    else:
        for index, (left, right) in enumerate(zip(got, want), 1):
            for key in sorted(left.keys() | right.keys()):
                if left.get(key) != right.get(key):
                    differences[f'pass{index}.{key}'] = {'observed': left.get(key), 'expected': right.get(key)}
    want = [row['bad_out'] for row in baseline['pass']]
    got = [row['bad_out'] for row in actual.get('pass', ())]
    if got != want:
        differences['bad_out'] = {'observed': got, 'expected': want}
    return differences


def compare_replay(data, gate):
    differences = []
    for rank, old in gate['rows_by_rank'].items():
        current = data.get(int(rank), {})
        for kind, fields in (('NATIVE3D_LOCAL_SURFACE', ('pass', 'faces_out', 'splits_distance')),
                             ('edge_bins', None), ('edge', None)):
            want = old.get(kind, [])
            got = current.get(kind, [])
            if fields:
                want = [report.select(row, fields) for row in want]
                got = [report.select(row, fields) for row in got]
            if got != want:
                first = next((i for i, pair in enumerate(zip(got, want)) if pair[0] != pair[1]),
                             min(len(got), len(want)))
                differences.append({'rank': int(rank), 'row': kind, 'index': first,
                                    'observed': got[first] if first < len(got) else None,
                                    'expected': want[first] if first < len(want) else None})
    return differences


def short_edge_lineage(data):
    problems, short_edges, splits = [], [], {}
    point_bases = {int(row['point_base']) for rows in data.values() for row in rows.get('invariant', ())}
    if len(point_bases) != 1:
        return [], [{'row': 'point_base', 'observed': sorted(point_bases)}]
    point_base = point_bases.pop()
    for rank, rows in sorted(data.items()):
        for split in rows.get('split', ()):
            point = int(split['new'])
            if point <= point_base or point in splits or int(split['rank']) != rank:
                problems.append({'row': 'split_point_id', 'split': split})
            splits[point] = split
        edges = rows.get('short_edge', [])
        bins = [row for row in rows.get('edge_bins', ()) if row['pass'] == '2']
        census = {(int(edge['a']), int(edge['b'])): Decimal(edge['metric']) for edge in edges}
        top_short = [edge for edge in rows.get('edge', ()) if edge['pass'] == '2' and Decimal(edge['metric']) < Decimal('.01')]
        if (len(bins) != 1 or len(edges) != int(bins[0]['lt_1e_2']) or len(census) != len(edges)
                or any(int(edge['rank']) != rank or int(edge['pass']) != 2 or int(edge['a']) >= int(edge['b'])
                       or not 0 < Decimal(edge['metric']) < Decimal('.01') for edge in edges)
                or any(census.get(tuple(sorted((int(edge['a']), int(edge['b']))))) != Decimal(edge['metric'])
                       for edge in top_short)):
            problems.append({'rank': rank, 'row': 'short_edge_census', 'edge_bins': bins, 'rows': len(edges)})
        short_edges.extend(edges)
    lineage = []
    for edge in sorted(short_edges, key=lambda row: (int(row['rank']), int(row['a']), int(row['b']))):
        ends = {}
        for key in ('a', 'b'):
            point = int(edge[key])
            ends[key] = 'input' if point <= point_base else splits.get(point)
            if ends[key] is None:
                problems.append({'row': 'split_lineage', 'point': point, 'edge': edge})
        lineage.append({**edge, 'a_split': ends['a'], 'b_split': ends['b']})
    return lineage, problems


def evaluate(name, data, expected, gate_path=None):
    result = BASELINE_EVALUATE(name, data, expected)
    limits = result['failed_limits']
    problems = result['problems']
    mode = INSTRUMENTED.get(name, name)
    for phase, (observed, limit) in enumerate(zip(result['totals']['faces'], FACE_LIMIT[mode]), 1):
        if observed > limit:
            limits.append({'field': 'correction1_faces', 'pass': phase, 'observed': observed, 'maximum': limit})
    for rank, rows in data.items():
        for row in rows.get('NATIVE3D_LOCAL_SURFACE', ()):
            if any(not row.get(key, '').isdigit() for key in PATH_FIELDS):
                problems.append({'rank': rank, 'pass': int(row['pass']), 'row': 'path_counters'})
    if not gate_path:
        return result

    gate = a.cut.read(gate_path)
    result['compared_gate_sha256'] = a.sha(gate_path)
    violations = 0
    for rank in range(RUNS[name][0]):
        rows = data.get(rank, {}).get('invariant', [])
        if [int(row['pass']) for row in rows] != list(range(1, 2 if name == 'N1Invariant' else 3)):
            problems.append({'rank': rank, 'row': 'invariant', 'observed': rows})
        else:
            violations += sum(int(row['violations']) for row in rows)
    result['violations'] = violations
    if name == 'N1Invariant':
        if difference := compare_invariant(data, gate):
            problems.append({'row': 'invariant_reproduction', 'differences': difference, 'rule': 'R3'})
        if violations:
            limits.append({'field': 'longest_edge_violations', 'observed': violations, 'maximum': 0})
    else:
        result['replayed_limits'] = list(limits)
        limits[:] = [row for row in limits if row not in gate['measurement']['failed_limits']]
        problems.extend({'row': 'diagnostic_reproduction', 'difference': row} for row in compare_replay(data, gate))
        result['short_edge_lineage'], issues = short_edge_lineage(data)
        problems.extend(issues)
    return result


def run_lab(build, name, verify_only):
    label = a.cut.read(build / 'build_status.json')['build']
    gate_path = case_path(label, INSTRUMENTED[name]) / 'measurement.json' if name in INSTRUMENTED else None

    def with_correction(directory):
        receipt, binary, pins = prepare(directory, (gate_path,) if gate_path else ())
        fingerprints = build_fingerprints(receipt)
        scoped_fixtures(HERE / 'scoped' / f'build{label}', fingerprints)
        if gate_path:
            gate = lab.checked_receipt(gate_path.parent, fingerprints)
            assert gate['case'] == INSTRUMENTED[name]
            if name == 'N6Diagnosis':
                assert any(row['field'] == 'lt_1e_2' for row in gate['measurement']['failed_limits'])
        return receipt, binary, pins

    with patch.multiple(lab, RUNS=RUNS, LINE=EXTRA_ROWS, __file__=str(DRIVER), prepare=with_correction,
                        evaluate=lambda case, data, expected: evaluate(case, data, expected, gate_path)):
        return lab.run(build, name, verify_only)


def committed_baseline(name):
    path = case_path(632, name) / 'measurement.json'
    archived = subprocess.check_output(['git', 'show', f'{BASE_COMMIT}:{path.relative_to(a.cut.REPO)}'],
                                       cwd=a.cut.REPO)
    assert path.read_bytes() == archived, path
    return a.cut.read(path)


def render_report(build):
    table = lab.archived.table
    label = a.cut.read(build / 'build_status.json')['build']
    results = {name: a.cut.read(path) for name in (*GATE_CASES, 'N6Diagnosis')
               if (path := case_path(label, name) / 'measurement.json').is_file()}
    comparison = {f'{name} build{version}': row for name in ('N1', 'N6')
                  for version, row in ((632, committed_baseline(name)), (label, results.get(name))) if row}
    with patch.object(report, 'SURFACE', (*report.SURFACE, 'splits_length', 'splits_distance', 'splits_feature',
                                          *PATH_FIELDS)):
        passes = report.lab_pass_rows({}, comparison)
    lines = [f'# FP2-B correction 1 hand-over evidence · build{label}', '',
             '## Rank and pass comparison', '', *table([row for rows in passes.values() for row in rows]),
             '## Smallest final edge', '']
    lines.extend(table([{'case': name, **row['measurement']['smallest_edge']}
                        for name, row in comparison.items() if row['measurement']['smallest_edge']]))
    lines.extend(['## Geometry proof census', '', *table([
        {'case': name, **row['measurement']['proof']}
        for name, row in comparison.items() if row['measurement']['proof']])])
    lines.extend(['## Hand-over gates', ''])
    for name, row in results.items():
        m = row['measurement']
        lines.extend([f'- **{name}** {"passed" if row["passed"] else "failed"}, '
                      f'{row["wait"]["elapsed_seconds"]:.3f} s. '
                      f'[Receipt](../cases/build{label}/{name}/measurement.json)'])
        for issue in (*m['failed_limits'], *m['problems']):
            lines.append(f'  - `{json.dumps(issue)}`')
    if 'N6Diagnosis' in results:
        m = results['N6Diagnosis']['measurement']
        lines += ['', '## Short-edge diagnosis', '',
                  'Each split record gives the step, rank, pass, sweep, parent edge and both parent faces.', '',
                  *table([{'rank': edge['rank'], 'pass': edge['pass'], 'a': edge['a'], 'b': edge['b'],
                           'metric': edge['metric'],
                           'a split': json.dumps(edge['a_split']), 'b split': json.dumps(edge['b_split'])}
                          for edge in m['short_edge_lineage']])]
    path = HERE / 'reports' / f'FP2_B_{label}_correction_1_lab.md'
    report.write_report(path, '\n'.join(lines))
    return path


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=('fixtures', *RUNS, 'report', 'default'))
    group.add_argument('--owned', type=Path)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    if args.owned:
        with patch.object(lab, 'RUNS', RUNS):
            raise SystemExit(lab.owned(args.owned))
    if not args.build:
        parser.error('a build directory is required')
    build = args.build.resolve()
    name = args.only
    if name == 'report':
        print(render_report(build))
    elif name in ('fixtures', 'default'):
        result = run_scoped(build, name)
        print(json.dumps({key: result[key] for key in ('build', 'passed', 'G1')}, indent=2))
    else:
        result = run_lab(build, name, args.verify_only)
        summary = {key: result[key] for key in ('case', 'passed', 'measurement')}
        print(json.dumps(summary, indent=2))
        if not result['passed']:
            raise SystemExit(1)


if __name__ == '__main__':
    main()
