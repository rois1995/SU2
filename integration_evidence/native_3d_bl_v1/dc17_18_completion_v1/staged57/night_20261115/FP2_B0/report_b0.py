"""Render the requested FP2-B0 tables from sealed test transcripts and receipts."""

from argparse import ArgumentParser
from collections import defaultdict
from decimal import Decimal
import gzip
import hashlib
from pathlib import Path
import sys

import run_b0 as run

HERE = Path(__file__).resolve().parent
cut, sha = run.a.cut, run.a.sha
MINIMUM_CASES = ('H1', 'H6', 'H6off', 'X6')
TOP = (('H6off', 'H6', 'X6'), ('H0', 'H1', 'X1', 'X2', 'X3'))
SECTIONS = (
    ('3.1 Bad cells at each checkpoint', (
        ('checkpoint', 'Totals by rank, pass and checkpoint'),
        ('bad_cells', 'Cell class, surface-point count, physical-face incidence, stamp and shape bins'),
        ('bad_c3', 'Failed-point memory, blocked cells and bad margin bins'),
    )),
    ('3.2 Read-only quality outcomes at hand-over', (
        ('quality_summary', 'No-free, four-surface and no-closed-edge cases'),
        ('quality_option', 'First refusal reasons and only-reason cell counts'),
    )),
    ('3.3 Short edges and split placement', (
        ('edge_bins', 'Core metric edge thresholds'),
        ('edge', 'Twenty shortest edges per rank'),
        ('split_bins', 'Endpoint and nearest-rim ratios by split kind'),
        ('split_min', 'Smallest split-placement ratios with their point records'),
    )),
    ('3.4 Point stars', (
        ('stars', 'Live-cell incidence bins by point group'),
        ('star_top', 'Largest point stars'),
        ('star_growth', 'Free-point star growth above 50 cells by split path'),
    )),
    ('3.5 Longest edge in the two physical faces', (
        ('split_longest', 'Longest in both faces, one face or neither'),
    )),
    ('3.6 Final rank regions', (
        ('region', 'Final cells by class, physical faces and physical bounding boxes'),
    )),
    ('4 Test-only volume sweeps', (
        ('volume_round', 'Accepted operations, repeat quality sweeps and round timing'),
        ('volume_boundary', 'Physical and worker-border preservation'),
        ('exact', 'Exact positivity census'),
        ('pass', 'Pass output beside checkpoint counts'),
    )),
)


def archived(path):
    return path.with_suffix(path.suffix + '.gz')


def open_log(path):
    return path.open(errors='replace') if path.is_file() else gzip.open(archived(path), 'rt', errors='replace')


def verify_artifacts(case, artifacts):
    for name, expected in artifacts.items():
        path = case / name
        if path.is_file():
            actual = sha(path)
        else:
            with gzip.open(archived(path), 'rb') as stream:
                actual = hashlib.file_digest(stream, 'sha256').hexdigest()
        assert expected == actual, path


def collect(label):
    folder = run.cases_path(label)
    all_rows = defaultdict(list)
    receipts = {}
    for name in run.MODE:
        case = folder / name
        file = case / 'measurement.json'
        receipt = cut.read(file)
        assert receipt['passed'] and receipt['case'] == name and receipt['build'] == label, file
        verify_artifacts(case, receipt['artifacts'])
        receipts[name] = receipt
        if name == 'SurfaceA':
            continue
        ranks = receipt['ranks']
        for source in sorted(receipt['artifacts']):
            path = case / source
            wanted = path.name == ('test_outer.log' if ranks == 1 else 'stdout')
            if not wanted:
                continue
            with open_log(path) as stream:
                for kind, row in run.measurement_rows(stream):
                    if kind not in ('quality_cell', 'split'):
                        all_rows[kind].append({'run': name, **row})
    return folder, receipts, all_rows


def handover_rows(rows, case, receipt):
    final_pass = '1' if receipt['ranks'] == 1 else '2'
    checkpoint = receipt['measurement']['handover_checkpoint']
    return [row for row in rows if row['run'] == case and row['pass'] == final_pass
            and row['checkpoint'] == checkpoint]


def minimum_edges(rows, receipts):
    minima = []
    for case in MINIMUM_CASES:
        edges = handover_rows(rows['edge'], case, receipts[case])
        assert edges, case
        smallest = min(edges, key=lambda row: (Decimal(row['metric']), int(row['rank']),
                                               int(row['a']), int(row['b'])))
        minima.append(smallest)
    return minima


def no_free_counts(rows, receipts):
    counts = {}
    for case in ('H6', 'X6'):
        selected = handover_rows(rows['quality_summary'], case, receipts[case])
        assert len(selected) == 4 * receipts[case]['ranks'], (case, len(selected))
        counts[case] = tuple(sum(int(row[name]) for row in selected)
                             for name in ('no_free', 'four_surface', 'no_closed_edge'))
    return counts


def cell(value):
    return str(value).replace('|', r'\|').replace('\n', ' ')


def table(rows):
    if not rows:
        return ['No rows recorded in this scope.', '']
    names = list(dict.fromkeys(key for row in rows for key in row))
    return ['| ' + ' | '.join(names) + ' |', '| ' + ' | '.join('---' for _ in names) + ' |',
            *('| ' + ' | '.join(cell(row.get(key, '—')) for key in names) + ' |' for row in rows), '']


def render(label, receipts, rows, production, choice):
    bad = {name: entry['measurement']['handover_bad'] for name, entry in receipts.items() if name != 'SurfaceA'}
    triples = no_free_counts(rows, receipts)
    smallest = minimum_edges(rows, receipts)
    lines = [
        f'# FP2-B0 hand-over measurement, build{label}', '',
        '**Bad cells at hand-over.** ' + ' · '.join(' / '.join(f'{name} {bad[name]:,}' for name in group)
                                                   for group in TOP), '',
        '**No free point / four surface points / no closed-ring edge.** ' +
        ' · '.join(f'{name} ' + ' / '.join(f'{v:,}' for v in values) for name, values in triples.items()), '',
        '**Smallest core metric lengths at hand-over.** ' +
        ' · '.join(f"{row['run']} {row['metric']} on rank {row['rank']}, edge {row['a']}–{row['b']}" for row in smallest) +
        '. Complete §3.3 rows below.', '',
        '## Global shortest edges at hand-over', '',
        *table(smallest),
        '## Scope and checks', '',
        f"Compared all {production['checked_non_unit_tests_paths']:,} build627 source pins outside `UnitTests/` "
        f"against SHA256 with {len(production['differences'])} differences. "
        'The output of `git diff --stat 852e566d17 -- Common SU2_CFD` was empty. '
        f"The `SU2_CFD` SHA256 remained `{production['solver_sha256']}`.",
        '', f"[Production audit](../build{label}/final_audit.json) · "
        f"[Build receipt](../build{label}/build_status.json) · [Whole-item source check](../source_check.md)", '',
        'The build targeted `UnitTests/test_driver` only. The serial surface fixture used '
        '`[NativeLocalCoreSurfaceA]`. The MPI-only exchange cases retain their rank requirements. '
        'The measurements used the pinned LAB-S and K3 inputs with the FP0 B sensor. '
        'The 6-rank runs used K3 unit partition weights. The state constructor and strict geometry proof were '
        'outside this measurement. The surface option remains default NO and the FP2-A ON failure remains recorded.',
        '', '## Run receipts and sanity checks', '',
    ]
    status = []
    for name, entry in receipts.items():
        measurement = entry['measurement']
        status.append({'run': name, 'ranks': entry['ranks'], 'status': 'completed',
                       'wrapper_seconds': f"{entry['wait']['elapsed_seconds']:.3f}",
                       'sampled_rank_peak_lower_bound_bytes': entry['sampled_rank_peak_lower_bound_bytes'],
                       'bad_at_handover': measurement.get('handover_bad', '—'),
                       'cells_at_handover': measurement.get('handover_cells', '—'),
                       'cells_after_shift': measurement.get('post_shift_cells') or '—',
                       'faces_out_by_pass': ', '.join(map(str, measurement.get('faces_out_by_pass', []))) or '—',
                       'receipt': f'[Measurement](../cases/build{label}/{name}/measurement.json)'})
    lines += table(status)
    [[assertions, cases]] = receipts['SurfaceA']['catch_summaries'][0]
    lines += [f'The serial surface fixture passed {int(assertions):,} assertions in {cases} cases.', '']
    for name in ('H1', 'H6'):
        sanity = receipts[name]['measurement']
        if differences := sanity['sanity_differences']:
            first = differences[0]
            lines += [f"- {name} first sanity difference is {first['field']}, observed {first['observed']:,} "
                      f"against {first['reference']:,}. The table retains the measured values.", '']
        else:
            lines += [f"- {name} matches the pinned build627 cell and surface-face sanity counts.", '']
    free = [{'run': name, **row} for name, entry in receipts.items()
            for row in entry['measurement'].get('bad_out_free_differences', ())]
    if free:
        lines += ['### Class (d) and bad_out_free differences', '', *table(free)]
    else:
        lines += ['Class (d) and `bad_out_free` agree at the measured ends of every pass.', '']
    selected = choice['selected']
    tried = ' · '.join(f"{row['case']} {row['bad_cells']:,}" for row in choice['candidates'])
    lines += [f"X6 replayed {selected['case']}'s {selected['extra']} variant, which had the smallest c4 count. "
              f"The serial comparison was {tried}. Ties use X1, then X2, then X3. "
              f"[Choice receipt](../cases/build{label}/x6_choice.json)", '',
              '## H and X end-of-pass comparison', '']
    comparison = []
    for ranks, group in ((1, ('H0', 'H1', 'X1', 'X2', 'X3')), (6, ('H6off', 'H6', 'X6'))):
        for number in range(1, 2 if ranks == 1 else 3):
            for name in group:
                for checkpoint in ('c3', 'c4') if name.startswith('X') else ('c3',):
                    selected = [row for row in rows['checkpoint'] if
                                (row['run'], row['pass'], row['checkpoint']) == (name, str(number), checkpoint)]
                    assert len(selected) == ranks, (name, number, checkpoint)
                    comparison.append({'ranks': ranks, 'pass': number, 'run': name, 'checkpoint': checkpoint,
                                       **{col: sum(int(row[col]) for row in selected)
                                          for col in ('cells', 'bad', 'bad_a', 'bad_b', 'bad_c', 'bad_d')}})
    lines += table(comparison)
    lines += ['Per-cell quality outcomes and individual accepted split records are retained in the rank transcript '
              'archives listed in each run receipt. The report tables use their bounded summary rows.', '']
    for section, kinds in SECTIONS:
        lines += [f'## {section}', '']
        if section.startswith('3.1'):
            lines += ['Class (a) contains a surface-split point. Class (b) contains an input surface point '
                      'and no surface-split point. Class (c) has a border point and no surface point. '
                      'Class (d) contains four free points.', '',
                      'Shape bins 0–2 are <0.1, 0.1–0.3, and >0.3 for shortest-to-longest space edge. '
                      'Height bins 0–2 are <0.3, 0.3–3, and >3 for height divided by longest physical face edge. '
                      'Height −1 marks a cell without a physical face. Margin bins 0–4 are '
                      '<0.01, [0.01, 0.1), [0.1, 0.5), [0.5, 1), and [1, BadMargin).', '']
        if section.startswith('3.3'):
            lines += ['Split placement bins 0–4 are <0.01, [0.01, 0.1), [0.1, 0.25), [0.25, 0.4], '
                      'and >0.4. Distances am, mb and rim are divided by the original space length ab.', '']
        if section.startswith('3.2'):
            lines += ['The probes evaluate each option independently, including candidates skipped by failure memory '
                      'or by the production selection. Accepted probe counts can therefore appear outside a sweep '
                      'that reached its limit. The no-free, four-surface and no-closed-edge counts are nested.', '']
        if section.startswith('4 '):
            lines += ['Each round uses one stamp over both V sweeps and skips an already stamped seed or old '
                      'cavity cell. Each sweep takes a fresh Margin and cell-key ordering. Equal space lengths '
                      'use global edge IDs. Collapse seeds use point IDs and equally scored targets use '
                      'neighbour IDs. The new-edge test considers edges absent from the old cavity. '
                      'X3 and X6 order collapse before split. The ordinary QualityWork call also runs in the '
                      'terminating zero-operation round.', '']
        for kind, title in kinds:
            lines += [f'### {title}', '', *table(rows[kind])]
    named = {kind for _, kinds in SECTIONS for kind, _ in kinds}
    unknown = set(rows) - named
    if unknown:
        lines += ['## Other test-only rows', '']
        for kind in sorted(unknown):
            lines += [f'### {kind}', '', *table(rows[kind])]
    corrections = HERE / 'corrections.md'
    if corrections.is_file():
        lines += ['## Test harness corrections and recorded choices', '', corrections.read_text().strip(), '']
    return '\n'.join(lines)


def self_check():
    rows = defaultdict(list)
    receipts = {name: {'ranks': 1 if name == 'H1' else 6, 'measurement': {'handover_checkpoint': 'c3'}}
                for name in MINIMUM_CASES}
    receipts['X6']['measurement']['handover_checkpoint'] = 'c4'
    for name, receipt in receipts.items():
        p = '1' if receipt['ranks'] == 1 else '2'
        cp = receipt['measurement']['handover_checkpoint']
        base = {'run': name, 'pass': p, 'checkpoint': cp}
        rows['edge'].extend({**base, 'metric': value, 'rank': rank, 'a': '2', 'b': '3'}
                            for value, rank in (('0.01', '0'), ('3e-5', '1'), ('5.5e-10', '1')))
        rows['edge'].append({**base, 'pass': '0', 'metric': '1e-14', 'rank': '0', 'a': '2', 'b': '3'})
        if name in ('H6', 'X6'):
            rows['quality_summary'].extend({**base, 'no_free': '2', 'four_surface': '1', 'no_closed_edge': '0'}
                                           for _ in range(24))
    assert all(row['metric'] == '5.5e-10' for row in minimum_edges(rows, receipts))
    assert no_free_counts(rows, receipts) == {'H6': (48, 24, 0), 'X6': (48, 24, 0)}
    assert '| a | b |' == table([{'a': 1, 'b': 2}])[0]
    print('FP2-B0 report reductions self-check passed.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    else:
        run.verify_order()
        folder, receipts, rows = collect(args.build)
        production = cut.read(run.build_path(args.build) / 'final_audit.json')
        assert production['passed'] and production['all_witnessed_owned_handles_terminal']
        choice = cut.read(folder / 'x6_choice.json')
        target = HERE / 'reports' / f'FP2_B0_{args.build}.md'
        target.parent.mkdir(exist_ok=True)
        target.write_text(render(args.build, receipts, rows, production, choice), encoding='utf-8')
        print(target)
