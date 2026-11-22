"""Compare the correction 2 S1 diagnostics with build633 and D1."""
from argparse import ArgumentParser
from collections import Counter, defaultdict
from dataclasses import asdict
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
import report_b

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'FP2_B_D1'))
import report_d1 as d1
from correction2_donor_evidence import donor_evidence

LABELS = ('build633', 'D1', 'correction2')
SURFACE_FIELDS = ('splits_length', 'sweeps_length', 'faces_in', 'faces_out',
                  'left_long_neighbor', 'waits_length', 'refused_near', 'left_long',
                  'left_border', 'left_neighbor_locked')
BIN_KEYS = ('lt_1e_6', 'lt_1e_4', 'lt_1e_2', 'lt_01')


def scope_key(row):
    stage = row.stage if row.stage is not None else -1
    return row.event, stage, row.rank


def pass_key(row):
    return *scope_key(row), int(row.value('pass', '0'))


def minimums(rows):
    for row in rows[d1.SHORT]:
        checkpoint = row.value('checkpoint')
        order = row.value('order')
        if checkpoint in ('c1', 'shifted') and order in ('1', '—'):
            yield ((*pass_key(row), checkpoint, 1),
                   [*(row.value(key) for key in BIN_KEYS), row.value('metric'), row.value('a'),
                    row.value('b'), row.source, row.value('error')])
    ordinal = Counter()
    for row in rows[d1.SEED]:
        key = scope_key(row)
        ordinal[key] += 1
        yield ((*key, 0, 'seed', ordinal[key]),
               [*(row.value(key) for key in BIN_KEYS[:3]), row.value('lt_0_1'), row.value('e0_metric'),
                row.value('e0_a'), row.value('e0_b'), row.source, row.value('error')])


def classify_near(row):
    distance, limit = (d1.number(row.value(field)) for field in ('distance', 'limit'))
    metric = d1.number(row.value('min_metric'))
    spatial = distance is not None and limit is not None and distance < limit
    metric_short = metric is not None and metric < d1.EDGE_FLOOR
    trigger = ('i+ii' if metric_short else 'i') if spatial else ('ii' if metric_short else 'unavailable')
    in_parents = row.fields.get('nearest_in_parents')
    if in_parents is None:
        parents = row.value('face0', '').split(',') + row.value('face1', '').split(',')
        in_parents = str(int(row.value('nearest') in parents))
    return trigger, in_parents, spatial and in_parents == '1'


def report(rows_by_label):
    lines = ['# FP2-B correction 2 · S1 diagnostics', '',
             'Stage −1 denotes a row without an observed preceding local-core stage. Build633 predates P1–P4. '
             'For D1, membership of the nearest vertex in the old parents is read from the printed face IDs. '
             'P2 counts describe the printed rows, with capped rows shown separately.', '',
             '## P1 and P4 short-edge minima and census pairs', '']
    minimum_by_group = defaultdict(dict)
    for label, rows in rows_by_label.items():
        for key, values in minimums(rows):
            minimum_by_group[key][label] = values
    lines += d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Checkpoint', 'Ordinal', 'Run', '<1e-6',
                       '<1e-4', '<1e-2', '<0.1', 'Minimum metric', 'A', 'B', 'Log row', 'Error'),
                      [[*key, label, *values] for key, runs in sorted(minimum_by_group.items())
                       for label in LABELS if (values := runs.get(label))])

    lines += ['## Surface counters', '']
    surface = [[*pass_key(row), label, *(row.value(key) for key in SURFACE_FIELDS), row.source]
               for label in LABELS for row in rows_by_label[label][d1.SURFACE]]
    lines += d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Run', *SURFACE_FIELDS, 'Log row'), sorted(surface))
    sums = defaultdict(int)
    for event, stage, rank, phase, label, _, _, _, faces, *_ in surface:
        sums[event, stage, phase, label] += int(faces)
    lines += d1.table(('Event', 'Stage', 'Pass', 'Run', 'Summed faces_out'),
                      [[*key, count] for key, count in sorted(sums.items())])

    lines += ['## P2 near-split classifications', '']
    counts, caps, faults = Counter(), [], []
    for label, rows in rows_by_label.items():
        for row in rows[d1.NEAR]:
            trigger, in_parents, fault = classify_near(row)
            counts[*pass_key(row), label, row.value('step'), trigger, in_parents] += 1
            if label == 'correction2' and fault:
                faults.append([*pass_key(row), row.value('step'), row.value('m'), row.value('nearest'),
                               row.value('distance'), row.value('limit'), row.source])
        for row in rows[d1.NEAR_LEFT]:
            caps.append([*pass_key(row), label, row.value('printed'), row.value('left_out'), row.source])
    lines += d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Run', 'Step', 'Trigger', 'Nearest in parents', 'Count'),
                      [[*key, count] for key, count in sorted(counts.items())])
    lines += d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Run', 'Printed', 'Capped', 'Log row'), sorted(caps))
    lines += ['### Corrected rule-2 faults', '',
              *d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Step', 'New point', 'Nearest', 'Distance', 'Limit', 'Log row'),
                        faults)]

    lines += ['## P3 metric rejection and donor text', '']
    rejects = []
    for label, rows in rows_by_label.items():
        for row in rows[d1.REJECT]:
            reason = row.value('reason')
            evidence = 'No donor tuple supplied'
            if ' donor_cell=' in reason:
                evidence = 'Exact donor membership requires the same-input hand-over tetrahedron or an allocation bound'
            rejects.append([*scope_key(row), label, row.value('phase'), row.value('cell'), reason, evidence, row.source])
    lines += d1.table(('Event', 'Stage', 'Rank', 'Run', 'Phase', 'Query cell', 'Reason', 'Donor evidence', 'Log row'),
                      sorted(rejects))
    return '\n'.join(lines), faults


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    args = parser.parse_args()
    cases = (HERE / 'G5/build633/S1/full633_run1',
             HERE.parent / 'FP2_B_D1/G5/build635/S1/full635_run1',
             HERE / f'G5/build{args.build}/S1/full{args.build}_run1')
    rows = {}
    for label, case in zip(LABELS, cases):
        rows[label], ranks = d1.read_case(case)
        assert ranks, case
    text, faults = report(rows)
    target = HERE / 'reports' / f'FP2_B_{args.build}_correction_2_s1.md'
    engine = {'S1 correction 2': report_b.engine_case(cases[2].parent),
              'S1 build633': report_b.engine_case(cases[0].parent),
              'S1 build627': report_b.engine_case(report_b.HISTORY / 'S1'),
              'CORE build627': report_b.engine_case(report_b.HISTORY / 'CORE')}
    history = target.with_name(f'FP2_B_{args.build}_correction_2_engine_event1.json')
    report_b.write_report(history, json.dumps(engine, indent=2) + '\n')
    text += '\n'.join(donor_evidence(cases[2]))
    text += '\n' + report_b.format_engine(engine, history.name)
    report_b.write_report(target, text + '\n')
    target_json = target.with_suffix('.json')
    raw = {label: {kind: [asdict(row) for row in data[kind]]
                   for kind in (d1.SHORT, d1.SEED, d1.NEAR, d1.NEAR_LEFT, d1.REJECT, d1.SURFACE)}
           for label, data in rows.items()}
    report_b.write_report(target_json, json.dumps({'rows': raw, 'rule2_faults': faults}, indent=2) + '\n')
    print(target)


if __name__ == '__main__':
    main()
