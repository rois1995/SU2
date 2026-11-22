"""Report correction 3 S1 from the terminal workflow transcripts."""

from argparse import ArgumentParser
from collections import Counter, defaultdict
from dataclasses import asdict
from decimal import Decimal
import json
import re
import sys

sys.dont_write_bytecode = True

import report_correction2_s1 as c2
from run_correction3_lab import walk_balance

d1, report, HERE = c2.d1, c2.report_b, c2.HERE
ENGINE = re.compile(r'\b(NATIVE3D_(?:STAGE|LENGTH|LOCAL_CORE|PHASE_OWNERSHIP|QUALITY(?:_LOCATION)?|TIMER_SUMMARY))\s+(.*)')
BOUNDARIES = {'construction_entry': 'construction', 'quality_exit': 'construction', 'quality_entry': 'quality',
              'geometry_repair_entry': 'geometry repair', 'surface_repair_entry': 'surface repair',
              'size_repair_entry': 'size repair', 'exit': None}
ENGINE_FIELDS = ('stage', 'phase', 'elapsed', 'geometry_bad', 'q_bad', 'j_bad', 'size_bad', 'shape_complete',
                 'geometry_complete', 'length_collapses')
SURFACE_FIELDS = ('splits_length', 'splits_distance', 'splits_feature', 'faces_in', 'faces_out', 'refused_walk',
                  'walk_tol8', 'walk_far', 'walk_no_start', 'walk_exception', 'left_long_neighbor', 'waits_length',
                  'refused_near', 'left_long', 'left_refused', 'left_border', 'left_neighbor_locked', 'star_max',
                  'sweeps_distance', 'seconds_length', 'seconds_distance', 'seconds_place')


def scan_engine(lines, rank):
    """Associate complete census histograms with their emitted stage."""
    out = {'stages': [], 'first_length': None, 'core': [], 'ownership_commits': []}
    census, histogram = {}, {}
    for line in lines:
        if not line.endswith('\n') or not (match := ENGINE.search(report.full.PREFIX.sub('', line))):
            continue
        marker, body = match.groups()
        row = report.full.fields(body)
        if row.get('rank') != str(rank):
            continue
        if marker == 'NATIVE3D_TIMER_SUMMARY' and row.get('root') == 'remesh' and row.get('boundary') == 'final':
            break
        if marker == 'NATIVE3D_STAGE' and 'elapsed' in row:
            stage = report.select(row, ENGINE_FIELDS)
            if row['stage'] == census.get('stage') and census.get('census') == histogram.get('census'):
                stage.update(report.select(histogram, ('geometry_excess_hist', 'geometry_authority_types')))
                stage['census'] = census['census']
            out['stages'].append(stage)
            census, histogram = {}, {}
        elif marker == 'NATIVE3D_QUALITY':
            census, histogram = row, {}
        elif marker == 'NATIVE3D_QUALITY_LOCATION':
            histogram = row
        elif marker == 'NATIVE3D_LENGTH' and out['first_length'] is None:
            out['first_length'] = report.select(row, ('pass', 'elapsed', 'minimum_length', 'collapse_accepted'))
        elif marker == 'NATIVE3D_LOCAL_CORE':
            out['core'].append(report.select(row, ('stage', 'pass', 'short_out', 'wall_near_points')))
        elif marker == 'NATIVE3D_PHASE_OWNERSHIP' and row.get('committed') == '1':
            out['ownership_commits'].append(row)
    return out


def engine_case(case):
    ranks = {}
    for log in sorted(case.glob('rank_output/*/rank.*/stdout*')):
        if log.name not in ('stdout', 'stdout.gz'):
            continue
        rank = int(log.parent.name.rsplit('.', 1)[1])
        if rank in ranks:
            continue
        with report.lab.archived.open_log(log if log.name == 'stdout' else log.with_suffix('')) as stream:
            ranks[rank] = scan_engine(stream, rank)
    return ranks


def spans(stages):
    """Time consecutive phases by the engine's monotonic elapsed clock."""
    invocation = 1
    phase, start, marker = 'construction', Decimal(0), 'engine start'
    latest = None
    for row in stages:
        elapsed = Decimal(row['elapsed'])
        if latest is not None and elapsed < latest[0]:
            if phase:
                yield invocation, phase, marker, 'last observed ' + latest[1], latest[0] - start
            invocation += 1
            phase, start, marker = 'construction', Decimal(0), 'engine start'
        latest = elapsed, row['stage']
        if row['stage'] not in BOUNDARIES:
            continue
        next_phase = BOUNDARIES[row['stage']]
        if phase == next_phase:
            continue
        if phase:
            yield invocation, phase, marker, row['stage'], elapsed - start
        phase, start, marker = next_phase, elapsed, row['stage']
    if phase and latest is not None:
        yield invocation, phase, marker, 'last observed ' + latest[1], latest[0] - start


def engine_report(cases, history_name):
    lines = ['## Event 1 engine timing and geometry census', '',
             'Phase spans use each rank’s engine elapsed clock. The first construction span starts with that '
             'clock. Later construction spans begin at quality exit, and shape-quality spans end at the next '
             'repair boundary or phase exit. The final observed stage bounds an unfinished span.', '',
             f'[Per-rank stage history]({history_name})', '']
    families, census, rank_counts, cores = [], [], [], []
    for name, ranks in cases.items():
        for rank, rows in sorted(ranks.items()):
            stages = rows['stages']
            families.extend([name, rank, invocation, phase, start, end, f'{seconds:.6f}']
                            for invocation, phase, start, end, seconds in spans(stages))
            first_repair = next((row for row in stages if row['stage'] in
                                ('geometry_repair_entry', 'surface_repair_entry')), None)
            for row in stages:
                if row['stage'] in ('length_collapse_exit', 'quality_entry') or row is first_repair:
                    census.append([name, rank, row['stage'], row['elapsed'], row['geometry_bad'],
                                   row['q_bad'] if row['shape_complete'] == '1' else '—',
                                   row['j_bad'] if row['shape_complete'] == '1' else '—',
                                   row.get('geometry_excess_hist', '—')])
            rank_counts.append([name, rank, (rows['first_length'] or {}).get('minimum_length', '—'),
                                stages[-1].get('length_collapses', '—') if stages else '—',
                                len(rows['ownership_commits'])])
            cores.extend([name, rank, c.get('stage'), c.get('pass'), c.get('short_out'), c.get('wall_near_points')]
                         for c in rows['core'])
    lines += d1.table(('Run', 'Rank', 'Engine invocation', 'Phase', 'From', 'Through', 'Seconds'), families)
    lines += ['Geometry bins count geometry-bad cells by worst relative excess, using boundaries 0.01, 0.1, 1 '
              'and 10, with a sixth bin for nonfinite or unresolved values. The engine emits histograms for '
              'complete improvement censuses. The length-collapse exit has counts without bins.', '']
    lines += d1.table(('Run', 'Rank', 'Stage', 'Engine seconds', 'Geometry bad', 'q bad', 'J bad', 'Geometry bins'),
                      census)
    lines += d1.table(('Run', 'Rank', 'First LENGTH minimum', 'Length collapses', 'Committed ownership rows'),
                      rank_counts)
    lines += d1.table(('Run', 'Rank', 'Stage', 'Pass', 'Short out', 'Wall-near points'), cores)
    return lines


def diagnostic_report(rows_by_label, candidate_label):
    lines = ['## P1 and P4 minima and counts', '']
    minima, surface, caps, rejects, faults = [], [], [], [], []
    face_sums, near = defaultdict(int), Counter()
    for label, rows in rows_by_label.items():
        minima.extend([*key, label, *values] for key, values in c2.minimums(rows))
        for row in rows[d1.SURFACE]:
            key = c2.pass_key(row)
            surface.append([*key, label, *(row.value(field) for field in SURFACE_FIELDS),
                            walk_balance(row.fields), row.source])
            face_sums[key[0], key[1], key[3], label] += int(row.value('faces_out'))
        for row in rows[d1.NEAR]:
            trigger, parents, fault = c2.classify_near(row)
            near[*c2.pass_key(row), label, row.value('step'), trigger, parents] += 1
            if label == candidate_label and fault:
                faults.append([*c2.pass_key(row), row.value('step'), row.value('m'), row.value('nearest'),
                               row.value('distance'), row.value('limit'), row.source])
        for row in rows[d1.NEAR_LEFT]:
            caps.append([*c2.pass_key(row), label, row.value('printed'), row.value('left_out'), row.source])
        for row in rows[d1.REJECT]:
            rejects.append([*c2.scope_key(row), label, row.value('phase'), row.value('cell'), row.value('reason'),
                            row.source])
    lines += ['Stage −1 is a row without an observed preceding local-core stage. Build633 predates P1–P4.', '']
    lines += d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Checkpoint', 'Ordinal', 'Run', '<1e-6',
                       '<1e-4', '<1e-2', '<0.1', 'Minimum metric', 'A', 'B', 'Log row', 'Error'), sorted(minima))
    lines += ['## Surface and walk outcomes', '',
              *d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Run', *SURFACE_FIELDS, 'Walk balance', 'Log row'),
                        sorted(surface)),
              *d1.table(('Event', 'Stage', 'Pass', 'Run', 'Summed faces_out'),
                        [[*key, count] for key, count in sorted(face_sums.items())]),
              '## P2 near-split classes and caps', '',
              'D1 parent membership can be read from its two printed old faces.', '',
              *d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Run', 'Step', 'Trigger', 'Nearest in parents', 'Count'),
                        [[*key, count] for key, count in sorted(near.items())]),
              *d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Run', 'Printed', 'Capped', 'Log row'), sorted(caps)),
              '### Candidate rule-2 faults', '',
              *d1.table(('Event', 'Stage', 'Rank', 'Pass', 'Step', 'New point', 'Nearest', 'Distance', 'Limit',
                        'Log row'), faults),
              '## P3 metric rejects', '',
              *d1.table(('Event', 'Stage', 'Rank', 'Run', 'Phase', 'Query cell', 'Reason', 'Log row'),
                        sorted(rejects))]
    return lines, faults


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    args = parser.parse_args()
    label = f'build{args.build}'
    case = HERE / f'G5/{label}/S1/full{args.build}_run1'
    runs = {'build633': HERE / 'G5/build633/S1/full633_run1',
            'D1': HERE.parent / 'FP2_B_D1/G5/build635/S1/full635_run1',
            'build638': HERE / 'G5/build638/S1/full638_run1', label: case}
    rows = {name: d1.read_case(source)[0] for name, source in runs.items()}
    engine = {name: engine_case(runs[name]) for name in ('build638', label)}
    for mode in ('S1', 'CORE'):
        old, = (report.HISTORY / mode).glob('full*_run1')
        engine[f'{mode} build627'] = engine_case(old)

    target = HERE / 'reports' / f'FP2_B_{args.build}_correction_3_s1.md'
    history = target.with_name(f'FP2_B_{args.build}_correction_3_engine_event1.json')
    report.write_report(history, json.dumps(engine, indent=2) + '\n')
    lines, faults = diagnostic_report(rows, label)
    text = [f'# FP2-B correction 3 · S1 evidence for {label}', '', *engine_report(engine, history.name),
            *lines, *c2.donor_evidence(case)]
    report.write_report(target, '\n'.join(text))
    raw = {name: {kind: [asdict(row) for row in data[kind]] for kind in
                 (d1.SHORT, d1.SEED, d1.NEAR, d1.NEAR_LEFT, d1.REJECT, d1.SURFACE)}
           for name, data in rows.items()}
    report.write_report(target.with_suffix('.json'),
                        json.dumps({'rows': raw, 'rule2_faults': faults}, indent=2) + '\n')
    print(target)


if __name__ == '__main__':
    main()
