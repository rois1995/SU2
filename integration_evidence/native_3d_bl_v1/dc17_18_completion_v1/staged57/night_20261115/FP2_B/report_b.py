"""Report FP2-B laboratory and workflow measurements from terminal evidence."""

from argparse import ArgumentParser
from collections import defaultdict
from decimal import Decimal
from pathlib import Path
import json
import re
import sys

sys.dont_write_bytecode = True
import run_lab as lab
import report_c as full

HERE = lab.HERE
HISTORY = HERE.parent / 'FP2_A/G5/build627'
BASELINE = lab.B0 / 'cases/build629'
OLD_LABS = ('H0', 'H1', 'X3', 'H6off', 'H6', 'X6')
ENGINE_ROW = re.compile(r'\b(NATIVE3D_(?:STAGE|LENGTH|LOCAL_CORE|PHASE_OWNERSHIP|TIMER_SUMMARY))\s+(.*)')
STAGE_FIELDS = ('elapsed', 'stage', 'phase', 'cells', 'shape_complete', 'geometry_complete', 'q_bad', 'j_bad',
                'geometry_bad', 'length_collapses')
STARS = ('n1_50', 'n51_100', 'n101_200', 'over200')
EDGE_BINS = ('lt_1e_6', 'lt_1e_4', 'lt_1e_2', 'lt_01')
SURFACE = ('left_long', 'left_distance', 'left_border', 'left_refused', 'left_limit', 'left_short',
           'left_all_feature', 'left_neighbor_locked', 'star_max', 'sweeps_distance', 'seconds_length',
           'seconds_distance', 'bad_before_v', 'v_rounds', 'v_limit', 'v_collapses', 'v_splits', 'v_changes',
           'seconds_v')


def select(row, keys):
    return {key: row[key] for key in keys if key in row}


def collect_labs(build):
    """Read historical and current lab logs and validate the sealed artifact hashes."""
    history = defaultdict(lambda: defaultdict(list))
    for name in OLD_LABS:
        case = BASELINE / name
        receipt = lab.a.cut.read(case / 'measurement.json')
        assert receipt['passed'] and receipt['case'] == name
        wanted = 'test_outer.log' if receipt['ranks'] == 1 else 'stdout'
        for file, digest in receipt['artifacts'].items():
            path = case / file
            if path.name != wanted:
                continue
            lab.archived.verify_artifacts(case, {file: digest})
            with lab.archived.open_log(path) as stream:
                for line in stream:
                    if line.startswith('NATIVE3D_LOCAL_SURFACE ') and line.endswith('\n'):
                        history[name]['surface'].append(full.fields(line))
                    elif line.startswith('FP2_B0 '):
                        for kind, row in lab.b0.measurement_rows([line]):
                            if kind in ('checkpoint', 'pass', 'edge', 'edge_bins', 'stars', 'star_top',
                                        'region', 'volume_round'):
                                history[name][kind].append(row)
    current = {}
    for name in ('N1', 'N6'):
        case = HERE / 'cases' / f'build{build}' / name
        path = case / 'measurement.json'
        if path.is_file():
            receipt = lab.a.cut.read(path)
            assert receipt['case'] == name and receipt['build'] == build and receipt['wait']['terminal']
            lab.archived.verify_artifacts(case, receipt['artifacts'])
            current[name] = receipt
    return history, current


def lab_pass_rows(history, current):
    selected = defaultdict(list)
    for name, rows in history.items():
        checkpoint = 'c4' if name.startswith('X') else 'c3'
        matches = {(row['rank'], row['pass']): row for row in rows['checkpoint']
                   if row['checkpoint'] == checkpoint}
        bins = {(row['rank'], row['pass']): row for row in rows['edge_bins'] if row['checkpoint'] == checkpoint}
        surface = {(row['rank'], row['pass']): row for row in rows['surface']}
        rounds = defaultdict(list)
        for row in rows['volume_round']:
            rounds[row['rank'], row['pass']].append(row)
        for row in sorted(rows['pass'], key=lambda row: (int(row['rank']), int(row['pass']))):
            key = row['rank'], row['pass']
            selected[name].append({'case': name, **select(row, ('rank', 'pass', 'cells_out', 'bad_out', 'faces_out')),
                                   **select(matches[key], ('bad_a', 'bad_b', 'bad_c', 'bad_d')),
                                   **select(bins.get(key, {}), EDGE_BINS),
                                   **select(row, ('seconds_quality', 'seconds_check')),
                                   **select(surface.get(key, {}), SURFACE),
                                   **({'v_rounds': len(rounds[key]),
                                       'v_collapses': sum(int(item['collapses']) for item in rounds[key]),
                                       'v_splits': sum(int(item['splits']) for item in rounds[key]),
                                       'v_changes': ','.join(f"c{item['collapses']}s{item['splits']}q{item['quality_sweeps']}"
                                                             for item in rounds[key]),
                                       'seconds_v': f"{sum(float(item['seconds']) for item in rounds[key]):.6f}"}
                                      if rounds[key] else {})})
    for name, receipt in current.items():
        for rank, rows in receipt['rows_by_rank'].items():
            groups = {key: {row['pass']: row for row in rows.get(key, ())}
                      for key in ('bad', 'edge_bins', 'NATIVE3D_LOCAL_SURFACE')}
            for row in rows.get('pass', ()):
                phase = row['pass']
                surface = groups['NATIVE3D_LOCAL_SURFACE'].get(phase, {})
                selected[name].append({'case': name, **select(row, ('rank', 'pass', 'cells_out', 'bad_out', 'faces_out')),
                                       **select(groups['bad'].get(phase, {}), ('bad_a', 'bad_b', 'bad_c', 'bad_d')),
                                       **select(groups['edge_bins'].get(phase, {}), EDGE_BINS),
                                       **select(row, ('seconds_pass', 'seconds_run_pass')),
                                       **select(surface, SURFACE)})
    return selected


def lab_last_rows(history, current, kind):
    for name, rows in history.items():
        last = '2' if name in ('H6off', 'H6', 'X6') else '1'
        checkpoint = 'c4' if name.startswith('X') else 'c3'
        yield name, [row for row in rows[kind] if row['pass'] == last and row['checkpoint'] == checkpoint]
    for name, receipt in current.items():
        last = '1' if name == 'N1' else '2'
        yield name, [row for rows in receipt['rows_by_rank'].values()
                     for row in rows.get(kind, ()) if row['pass'] == last]


def render_labs(build, history, current):
    table = lab.archived.table
    passes = lab_pass_rows(history, current)
    lines = [f'# FP2-B hand-over evidence for build{build}', '',
             'End-of-pass bad counts and edge bins are measured immediately before rank shifts. '
             'H cases use checkpoint c3, X cases c4, and N cases the end of production RunPass. '
             'The historical H6 pass-two count below 1e-2 is 18 on rank 4 and 5 on rank 5, for 23 summed.',
             '',
             '## Hand-over totals', '']
    totals = []
    for name, rows in passes.items():
        if not rows:
            continue
        last = [row for row in rows if row['pass'] == max(item['pass'] for item in rows)]
        keys = ('bad_out', 'bad_a', 'bad_b', 'bad_c', 'bad_d', 'cells_out', *EDGE_BINS)
        totals.append({'case': name, **{key: sum(int(row[key]) for row in last) for key in keys
                                        if all(key in row for row in last)}})
    lines += table(totals)
    for name, result in current.items():
        measurement = result['measurement']
        lines += [f'- **{name}** {"passed" if result["passed"] else "failed"}. '
                  f'Wrapper {float(result["wait"]["elapsed_seconds"]):.3f} s, sampled RSS lower bound '
                  f'{result["peak_lower_bound_bytes"]:,} bytes. '
                  f'[Receipt](../cases/build{build}/{name}/measurement.json)']
        for issue in (*measurement['failed_limits'], *measurement['problems']):
            lines.append(f'  - `{json.dumps(issue)}`')
    lines += ['', '## Rank/pass comparison', '',
              *table([row for name in ('H0', 'H1', 'X3', 'N1') for row in passes[name]]),
              *table([row for name in ('H6off', 'H6', 'X6', 'N6') for row in passes[name]]),
              '## Shortest edge at hand-over', '']
    minima = [{'case': name, **min(rows, key=lambda row: (Decimal(row['metric']), int(row['rank']),
                                                              int(row['a']), int(row['b'])))}
              for name, rows in lab_last_rows(history, current, 'edge') if rows]
    lines += table(minima)
    lines += ['## End-of-pass star histograms', '',
              *table([{'case': name, **select(row, ('rank', 'pass', 'group', *STARS))}
                      for name, rows in lab_last_rows(history, current, 'stars') for row in rows]),
              '## Largest stars at the last pass', '',
              *table([{'case': name, **row}
                      for name, rows in lab_last_rows(history, current, 'star_top') for row in rows]),
              '## Post-shift rank regions', '']
    regions = [{'case': name, **row} for name, rows in history.items() for row in rows['region']]
    regions += [{'case': name, **row} for name, receipt in current.items()
                for rows in receipt['rows_by_rank'].values() for row in rows.get('region', ())]
    lines += table(regions)
    lines += ['## Strict surface proof census', '',
              *table([{'case': name, **result['measurement']['proof']} for name, result in current.items()
                      if result['measurement']['proof']])]
    return '\n'.join(lines)


def scan_engine(lines, rank):
    """Read event-one source-rank rows using the final remesh boundary."""
    stages, lengths, cores, ownership = [], [], [], []
    for line in lines:
        if not line.endswith('\n') or not (match := ENGINE_ROW.search(full.PREFIX.sub('', line))):
            continue
        marker, text = match.groups()
        row = full.fields(text)
        if row.get('rank') != str(rank):
            continue
        if marker == 'NATIVE3D_TIMER_SUMMARY' and row.get('root') == 'remesh' and row.get('boundary') == 'final':
            break
        if marker == 'NATIVE3D_LENGTH':
            lengths.append(select(row, ('pass', 'elapsed', 'cells', 'minimum_length', 'collapse_accepted')))
        elif marker == 'NATIVE3D_STAGE' and 'elapsed' in row:
            stages.append(select(row, STAGE_FIELDS))
        elif marker == 'NATIVE3D_LOCAL_CORE':
            cores.append(select(row, ('stage', 'pass', 'short_out', 'wall_near_points', 'seconds_quality')))
        elif marker == 'NATIVE3D_PHASE_OWNERSHIP' and row.get('committed') == '1':
            ownership.append(row)
    return {'stages': stages, 'first_length': lengths[0] if lengths else None,
            'core': cores, 'ownership_commits': ownership}


def engine_case(mode):
    case, = mode.glob('full*_run1')
    ranks = {}
    for path in sorted(case.glob('rank_output/*/rank.*/stdout*')):
        if path.name not in ('stdout', 'stdout.gz'):
            continue
        rank = int(path.parent.name.rsplit('.', 1)[1])
        if rank in ranks:
            continue
        log = path if path.name == 'stdout' else path.with_suffix('')
        with lab.archived.open_log(log) as stream:
            ranks[rank] = scan_engine(stream, rank)
    return ranks


def engine_spans(stages):
    """Use engine clock resets to distinguish invocations inside one remesh event."""
    groups, group = [], []
    for row in stages:
        if group and Decimal(row['elapsed']) < Decimal(group[-1]['elapsed']):
            groups.append(group)
            group = []
        group.append(row)
    if group:
        groups.append(group)
    for ordinal, invocation in enumerate(groups, 1):
        def find(stage):
            return next((row for row in invocation if row['stage'] == stage), {})

        quality, collapse, end = (find(stage) for stage in ('quality_entry', 'length_collapse_exit', 'exit'))
        yield {'engine_call': ordinal, 'construction_to_quality_entry_seconds': quality.get('elapsed'),
               'improvement_to_exit_seconds': (str(Decimal(end['elapsed']) - Decimal(quality['elapsed']))
                                               if end and quality else None),
               'last_stage': invocation[-1]['stage'], 'last_elapsed': invocation[-1]['elapsed'],
               'quality_entry_q_bad': quality.get('q_bad'), 'quality_entry_j_bad': quality.get('j_bad'),
               'length_collapse_exit_geometry_bad': collapse.get('geometry_bad'),
               'length_collapses_last_observed': invocation[-1].get('length_collapses')}


def format_engine(data, history_name):
    table = lab.archived.table
    lines = ['## Event 1 engine and core observations', '',
             'Construction seconds use the engine clock at quality_entry. Improvement seconds use '
             'the elapsed difference from quality_entry to exit. Open invocations show the last observed clock. '
             'Committed ownership rows count the same collective operation on each participating rank. '
             'q and J are counts at the reported shape census. The history preserves measured-complete flags.',
             '', f'[Event 1 stage history]({history_name})', '']
    spans, fields, cores = [], [], []
    for name, ranks in data.items():
        for rank, row in sorted(ranks.items()):
            spans.extend({'case': name, 'rank': rank, **span} for span in engine_spans(row['stages']))
            minimum = row['first_length']
            fields.append({'case': name, 'rank': rank, 'first_minimum_metric_length':
                           minimum['minimum_length'] if minimum and 'minimum_length' in minimum else None,
                           'committed_ownership_rows': len(row['ownership_commits'])})
            cores.extend({'case': name, 'rank': rank, **core} for core in row['core'])
    lines += table(spans)
    lines += table(fields)
    lines += ['### Event 1 core work by rank, stage and pass', '', *table(cores)]
    return '\n'.join(lines)


def write_report(path, contents):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.is_file():
        with path.open('x') as stream:
            stream.write(contents)
    assert path.read_text() == contents, path


def workflow_report(build, report):
    root = HERE / 'G5' / f'build{build}'
    run = full.collect(root)
    acceptance = full.read_json(root / 'acceptance_summary.json')
    if acceptance:
        for name in ('S1', 'S2'):
            run['runs'][name]['time_rule'] = acceptance['runs'][name]['time_rule']
    data = {}
    for folder, suffix, cases in ((root, '', full.MODES), (HISTORY, '-627', ('CORE', 'S1'))):
        for mode in cases:
            if list((folder / mode).glob('full*_run1')):
                data[mode + suffix] = engine_case(folder / mode)
    history = report.with_name(f'FP2_B_{build}_engine_event1.json')
    write_report(history, json.dumps(data, indent=2) + '\n')
    report_text = full.format_markdown(run).replace('# FP2-A Part C workflow comparison',
                                                     f'# FP2-B workflow comparison for build{build}', 1)
    write_report(report, report_text + '\n' + format_engine(data, history.name))


def self_check():
    rows = [
        'NATIVE3D_STAGE rank=0 stage=length_collapse_exit elapsed=3 phase=construction geometry_bad=5\n',
        'NATIVE3D_LENGTH rank=0 elapsed=3.1 pass=collapse minimum_length=0.031\n',
        'NATIVE3D_STAGE rank=0 stage=quality_entry elapsed=4 phase=improvement q_bad=100 j_bad=50\n',
        'NATIVE3D_PHASE_OWNERSHIP rank=0 quality=1 committed=1 moved_cells=5\n',
        'NATIVE3D_PHASE_OWNERSHIP rank=0 quality=1 evaluate=1 migrate=0\n',
        'NATIVE3D_STAGE rank=0 stage=exit elapsed=12 phase=improvement length_collapses=3\n',
        'NATIVE3D_TIMER_SUMMARY rank=0 root=remesh boundary=final\n',
        'NATIVE3D_LENGTH rank=0 elapsed=1 pass=collapse minimum_length=0.03\n',
    ]
    result = scan_engine(rows, 0)
    assert result['first_length']['minimum_length'] == '0.031'
    assert len(result['ownership_commits']) == 1
    span, = engine_spans(result['stages'])
    assert span['length_collapse_exit_geometry_bad'] == '5'
    assert span['quality_entry_q_bad'] == '100'
    assert span['improvement_to_exit_seconds'] == '8'


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--lab', action='store_true')
    group.add_argument('--workflow', action='store_true')
    group.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.build is None:
        parser.error('a build number is required')
    if args.lab:
        report = HERE / 'reports' / f'FP2_B_{args.build}_lab.md'
        write_report(report, render_labs(args.build, *collect_labs(args.build)))
    else:
        report = HERE / 'reports' / f'FP2_B_{args.build}_workflow.md'
        workflow_report(args.build, report)
    print(report)


if __name__ == '__main__':
    main()
