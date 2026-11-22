"""Run and compare the ordered FP2-B correction 3 hand-over checks."""

from argparse import ArgumentParser
import json
from pathlib import Path
import re
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True

import report_b as report
import run_correction2_lab as c2

lab, prior, a, HERE = c2.lab, c2.prior, c2.a, c2.HERE
DRIVER = Path(__file__).resolve()
ORDER = HERE / 'inputs/FP2_B_CORRECTION_3.md'
ORDER_SHA = '2b305d1c1fcd00aa5f4c59c1ca437068aee33a21095843fc344ac73d09e9adc4'
SOURCE_CHECK = HERE / 'source_check_correction_3.md'
BASE_COMMIT = 'da8088eae2'
REFERENCE = 638
RUNS = {name: c2.RUNS[name] for name in ('N1', 'N1Invariant', 'N6', 'N6P')}
ALARM = {'N1': 1204, 'N1Invariant': 1204, 'N6': 1203, 'N6P': 1365}
LINE = re.compile(r'^(FP2_B (?:pass|end|bad|bad_before_v|state|proof|proof_entity|invariant|partition|'
                  r'donor_scan|donor_reject|walk_probe|sweep)|FP2_A LAB_S_PROOF|NATIVE3D_LOCAL_SURFACE) (.+)$')
WALK = ('refused_walk', 'walk_tol8', 'walk_far', 'walk_no_start', 'walk_exception')
PROBE_A = ('a_protected', 'a_duplicate')
PROBE_C = ('c_outward', 'c_near', 'c_size', 'c_validity', 'c_topology', 'c_entity')
REFUSALS = ('refused_protected', 'refused_feature', 'refused_trace', 'refused_duplicate', 'refused_outward',
            'refused_size', 'refused_valid', 'refused_topology', 'refused_memory', 'refused_entity', 'refused_near')
SURFACE = ('left_distance', 'left_refused', 'left_neighbor_locked', 'left_border', 'left_long',
           'left_long_neighbor', 'splits_length', 'splits_distance', 'splits_feature', 'faces_out',
           'sweeps_distance', 'star_max', 'seconds_length', 'seconds_distance', 'seconds_place')
CLASSES = ('bad_a', 'bad_b', 'bad_c', 'bad_d')


def verify_order():
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    assert SOURCE_CHECK.is_file(), SOURCE_CHECK


def baseline(name):
    path = prior.case_path(REFERENCE, name) / 'measurement.json'
    c2.committed(path, BASE_COMMIT)
    receipt = a.cut.read(path)
    assert receipt['case'] == name and receipt['build'] == REFERENCE and receipt['passed'], path
    return path, receipt


def first_partition_difference(pairs, reference):
    assert len(pairs) == len(reference)
    for row, wanted in zip(pairs, reference):
        rank = row['rank']
        assert rank == wanted['rank']
        for field, expected in (('cells_after', row['cells_expected']), ('pass1_cells_in', row['cells_expected']),
                                ('pass1_faces_in', row['faces_expected'])):
            assert wanted[field] == expected, (rank, field)
            if row[field] != expected:
                return {'rank': rank, 'field': field, 'observed': row[field], 'expected': expected}
    return None


def probe_problem(data):
    rows = data.get(0, {})
    probe = c2.fields_for_pass(rows, 'walk_probe', 1)
    surface = c2.fields_for_pass(rows, 'NATIVE3D_LOCAL_SURFACE', 1)
    fields = (*PROBE_A, *PROBE_C, 'a', 'b', 'c', 'b_accepted', 'total')
    if all(probe.get(key, '').isdigit() for key in fields) and surface.get('walk_tol8', '').isdigit():
        values = {key: int(probe[key]) for key in fields}
        if (values['a'] == sum(values[key] for key in PROBE_A)
                and values['b'] == values['b_accepted'] >= 1
                and values['c'] == sum(values[key] for key in PROBE_C)
                and values['a'] + values['b'] + values['c'] == values['total'] == int(surface['walk_tol8'])):
            return None
    return {'row': 'walk_probe', 'observed': probe, 'walk_tol8': surface.get('walk_tol8')}


def evaluate(name, data, expected, reference, invariant_path=None):
    if name == 'N6P':
        pairs = c2.partition_pairs(name, data)
        difference = first_partition_difference(pairs, reference['measurement']['partition_pairs'])
        pass_rows = {rank: {**rows, 'edge_bins': [row for row in rows.get('edge_bins', ())
                                                if row['checkpoint'] != 'shifted']}
                     for rank, rows in data.items()}
        result = prior.BASELINE_EVALUATE(name, pass_rows, expected)
        result.update(valid=difference is None, partition_pairs=pairs, partition_first_difference=difference,
                      gate_applies=difference is None)
        result['failed_limits'] = [row for row in result['failed_limits']
                                   if row['field'] in ('star_max', 'seconds_pass', 'v_limit', 'lt_1e_2')]
        for rank in range(RUNS[name][0]):
            shift = c2.shifted(data.get(rank, {}))
            if shift is None:
                result['problems'].append({'rank': rank, 'row': 'shifted_edges'})
            elif int(shift['lt_1e_2']):
                result['failed_limits'].append({'rank': rank, 'field': 'shifted_lt_1e_2',
                                                'observed': int(shift['lt_1e_2']), 'maximum': 0})
        c2.donor_scan(data, RUNS[name][0], result, True)
    else:
        result = c2.evaluate(name, data, expected, {}, False, None, invariant_path)
        result['failed_limits'] = [row for row in result['failed_limits']
                                   if row['field'] not in ('handover_bad', 'faces')]
        if name == 'N1Invariant' and (issue := probe_problem(data)):
            result['problems'].append(issue)

    if above := result['proof'].get('classify_above_tol'):
        if int(above) > ALARM[name]:
            result['failed_limits'].append({'field': 'classify_above_tol', 'observed': int(above),
                                            'maximum': ALARM[name]})
    elif result['proof']:
        result['problems'].append({'row': 'classify_above_tol', 'observed': result['proof']})

    if name == 'N6P' and difference:
        result['report_only_findings'] = {key: result[key] for key in ('problems', 'failed_limits')}
        result.update(problems=[{'row': 'partition', **difference}], failed_limits=[])
    return result


def run(build, name, verify_only=False):
    verify_order()
    reference_path, reference = baseline(name)
    label = a.cut.read(build / 'build_status.json')['build']
    invariant_path = prior.case_path(label, 'N1') / 'measurement.json' if name == 'N1Invariant' else None

    def prepare(directory):
        files = (DRIVER, c2.DRIVER, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, reference_path)
        if invariant_path:
            files = (*files, invariant_path)
        receipt, binary, pins = prior.prepare(directory, files)
        if invariant_path:
            gate = lab.checked_receipt(invariant_path.parent, prior.build_fingerprints(receipt))
            assert gate['case'] == 'N1' and gate['passed']
        return receipt, binary, pins

    with patch.multiple(lab, RUNS=RUNS, LINE=LINE, __file__=str(DRIVER), prepare=prepare,
                        evaluate=lambda case, data, expected: evaluate(case, data, expected, reference, invariant_path)):
        return lab.run(build, name, verify_only)


def walk_balance(row):
    if not all(row.get(key, '').isdigit() for key in WALK):
        return '—'
    return sum(int(row[key]) for key in WALK[2:]) == int(row['refused_walk'])


def selected(receipt, kind):
    for rows in receipt['rows_by_rank'].values():
        for row in rows.get(kind, ()):
            yield {'run': receipt['case'], 'build': receipt['build'], **row}


def rows_for_pass(receipt):
    for rank, rows in receipt['rows_by_rank'].items():
        for surface in rows.get('NATIVE3D_LOCAL_SURFACE', ()):
            phase = int(surface['pass'])
            before = c2.fields_for_pass(rows, 'bad_before_v', phase)
            after = c2.fields_for_pass(rows, 'bad', phase)
            pass_row = c2.fields_for_pass(rows, 'pass', phase)
            yield ({'run': receipt['case'], 'build': receipt['build'], 'rank': rank, 'pass': phase},
                   surface, before, after, pass_row)


def minima(receipt):
    for rank, rows in receipt['rows_by_rank'].items():
        for checkpoint, phase in (('out', 1 if receipt['ranks'] == 1 else 2), ('shifted', 0)):
            edge = c2.measured_edge(rows, checkpoint, phase)
            if edge:
                yield {'run': receipt['case'], 'build': receipt['build'], 'rank': rank, 'checkpoint': checkpoint, **edge}


def write_report(build):
    verify_order()
    observed, compared = [], []
    for name in RUNS:
        case = prior.case_path(build, name)
        path = case / 'measurement.json'
        if path.exists():
            receipt = a.cut.read(path)
            assert receipt['case'] == name and receipt['build'] == build and receipt['wait']['terminal'], path
            lab.archived.verify_artifacts(case, receipt['artifacts'])
            observed.append(receipt)
            compared.extend([baseline(name)[1], receipt])
    assert observed, build

    table, pick = lab.archived.table, report.select
    lines = [f'# FP2-B correction 3 · hand-over evidence for build{build}', '',
             'Build638 records the pre-V total and the post-V classes. Its hand-over labs do not publish pre-V '
             'class bins or individual surface sweeps. Unreported fields are shown as —.', '',
             '## Gate receipts', '']
    for receipt in observed:
        name, m = receipt['case'], receipt['measurement']
        outcome = 'not valid' if m.get('valid') is False else 'passed' if receipt['passed'] else 'failed'
        lines.append(f'- **{name} {outcome}.** Wrapper {receipt["wait"]["elapsed_seconds"]:.3f} s. '
                     f'[Receipt](../cases/build{build}/{name}/measurement.json)')
        for issue in (*m['failed_limits'], *m['problems']):
            lines.append(f'  - `{json.dumps(issue)}`')

    lines += ['', '## Rank and pass walk and refusal counts', '']
    passes = [row for receipt in compared for row in rows_for_pass(receipt)]
    lines += table([{**key, **pick(surface, WALK), 'walk balance': walk_balance(surface)}
                    for key, surface, _, _, _ in passes])
    lines += table([{**key, **pick(surface, REFUSALS)} for key, surface, _, _, _ in passes])
    lines += ['## Rank and pass surface work', '',
              *table([{**key, **pick(surface, SURFACE)} for key, surface, _, _, _ in passes]),
              '## Rank and pass volume work', '',
              *table([{**key, 'pre-V total': surface['bad_before_v'],
                       **{f'pre-V {field[-1]}': before[field] for field in CLASSES if field in before},
                       'post-V total': after.get('bad', '—'),
                       **{f'post-V {field[-1]}': after[field] for field in CLASSES if field in after},
                       **pick(surface, ('v_rounds', 'v_limit', 'v_collapses', 'v_splits', 'v_changes', 'seconds_v')),
                       **pick(pass_row, ('seconds_pass', 'seconds_run_pass'))}
                      for key, surface, before, after, pass_row in passes]),
              '## Surface sweep refusals', '',
              *table([{**pick(row, ('run', 'build', 'rank', 'pass', 'kind', 'sweep', 'border_edges',
                                   'refused_ineligible', 'refused_walk', *REFUSALS))}
                      for receipt in observed for row in selected(receipt, 'sweep')]),
              '## N1 invariant walk outcomes', '',
              *table([row for receipt in observed for row in selected(receipt, 'walk_probe')]),
              '## N6P partition identity', '',
              *table([row for receipt in observed for row in receipt['measurement'].get('partition_pairs', ())]),
              '## Minimum metric edges and donor scans', '',
              *table([row for receipt in compared for row in minima(receipt)]),
              *table([row for receipt in compared for row in selected(receipt, 'donor_scan')]),
              '## Final geometry proof', '',
              'The excess bins are <0.01, [0.01,0.1), [0.1,1), [1,10) and ≥10, using bound / tol − 1.', '',
              *table([{'run': receipt['case'], 'build': receipt['build'], **proof}
                      for receipt in compared if (proof := receipt['measurement']['proof'])]), '']
    path = HERE / f'reports/FP2_B_{build}_correction_3_lab.md'
    report.write_report(path, '\n'.join(lines))
    return path


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=tuple(RUNS))
    group.add_argument('--owned', type=Path)
    group.add_argument('--report', type=int)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    if args.owned:
        with patch.object(lab, 'RUNS', RUNS):
            raise SystemExit(lab.owned(args.owned))
    if args.report is not None:
        print(write_report(args.report))
        return
    if not args.build:
        parser.error('a build directory is required')
    result = run(args.build.resolve(), args.only, args.verify_only)
    print(json.dumps({key: result[key] for key in ('case', 'passed', 'measurement')}, indent=2))
    raise SystemExit(0 if result['passed'] else 1)


if __name__ == '__main__':
    main()
