"""Render the FP2-A rank checks from complete and partial receipts."""

from argparse import ArgumentParser
import json
from pathlib import Path
from tempfile import TemporaryDirectory

from report_c import read_json, shown, table


RUNS = {'K1': (2, 'on'), 'K2': (2, 'off'), 'K3': (6, 'on')}
PASS_TIMES = (
    ('Pass', 'seconds_pass'), ('RunPass', 'seconds_run_pass'),
    ('Surface L', 'seconds_surface_length'), ('Surface D', 'seconds_surface_distance'),
    ('Length work', 'seconds_length_work'), ('Quality work', 'seconds_quality_work'),
    ('Meeting', 'seconds_meeting'), ('Mesh build', 'seconds_build'), ('Writeback', 'seconds_write'),
)
STAR_COUNTS = (
    ('Max star', 'max_star'), ('Stars >100', 'star_gt100'), ('Stars >1,000', 'star_gt1000'),
    ('Unresolved points', 'unresolved_points'), ('Blocked cells', 'blocked_cells'),
)


def row_text(marker, row):
    parts = [marker]
    for key, value in row.items():
        if not isinstance(value, str) or not value or any(c.isspace() or c in '\\"' for c in value):
            value = json.dumps(value, ensure_ascii=False)
        parts.append(f'{key}={value}')
    return ' '.join(parts)


def row_section(title, rows):
    return [f'### {title}', '', '```text', *rows, '```', ''] if rows else []


def render(root):
    summary = read_json(root / 'rank_check_summary.json')
    results = summary.get('results', {})
    runs = {}
    for name in RUNS:
        result = results.get(name, {})
        receipt = (Path(result['receipt']) if 'reused_from_build' in result
                   else root / name / 'rank_check_receipt.json')
        runs[name] = read_json(receipt)
    order = summary.get('progress_order') or (('K2', 'K1', 'K3') if runs['K2'].get('ended') is False else tuple(RUNS))
    correction = summary.get('correction', 2)
    lines = [f'# FP2-A Correction {correction} rank check', '', f'Evidence directory `{root}`.', '',
             'Times are seconds. Pass time covers the meeting through writeback on that rank. '
             'The corrected active runs record stars after the surface steps. The earlier runs use the '
             'first completed volume sweep as the post-surface fallback. A dash marks a missing observation.', '']
    if summary.get('proceed_11_3') is True:
        lines += ['All three checks ended and no alarm fired. '
                  'Continue with §11.2 and then §11.3.', '']
    elif summary.get('proceed_11_3') is False:
        lines += ['Continue with §11.2 and stop before §11.3.', '']

    for name in order:
        ranks, surface = RUNS[name]
        receipt = runs[name]
        result = results.get(name, {})
        state = receipt.get('state', result.get('state'))
        ended = receipt.get('ended', result.get('ended'))
        lines += [f'## {name} · {ranks} ranks · surface {surface}', '']
        if state == 'skipped_k1_did_not_end':
            lines += ['Skipped because K1 did not meet its completion conditions.', '']
            continue
        if not receipt:
            lines += ['No run receipt has been recorded.', '']
            continue
        if 'reused_from_build' in result:
            lines += [f'Retained from build {result["reused_from_build"]}. '
                      f'Receipt SHA256 `{result["receipt_sha256"]}`.', '']
        outcome = {True: 'Ended', False: 'Did not end', None: 'Outcome unavailable'}[ended]
        lines += [f'**{outcome}.**' + (f' Receipt state `{state}`.' if state else ''), '']
        if receipt.get('wait') is not None:
            lines += [f'Launcher exit evidence `{json.dumps(receipt["wait"], ensure_ascii=False)}`.', '']
        if difference := receipt.get('first_difference'):
            lines += [f'First observed difference `{json.dumps(difference, ensure_ascii=False)}`.', '']
        time_alarms = receipt.get('alarm_over_600', [])
        if time_alarms:
            lines += row_section('Passes above the 600 s alarm', [row_text('ALARM', row) for row in time_alarms])
        surface_alarms = receipt.get('surface_alarms', [])
        if surface_alarms:
            lines += row_section('Post-surface alarms', [row_text('ALARM', row) for row in surface_alarms])

        by_rank = receipt.get('by_rank', {})
        times, stars, surfaces, exact, final = [], [], [], [], []
        for rank in range(ranks):
            seen = by_rank.get(str(rank), {})
            for pass_number in (1, 2):
                passes = [row for row in seen.get('passes', []) if str(row.get('pass')) == str(pass_number)]
                times.extend((rank, pass_number, *(shown(row.get(key)) for _, key in PASS_TIMES))
                             for row in passes or [{}])
                sweeps = [row for row in seen.get('sweeps', []) if str(row.get('pass')) == str(pass_number)]
                stars.extend((rank, pass_number, *(shown(row.get(key)) for key in ('kind', 'round', 'sweep', 'seconds')),
                              *(shown(row.get(key)) for _, key in STAR_COUNTS), shown(row.get('first_volume_sweep')))
                             for row in sweeps or [{}])
            surfaces += [row_text('NATIVE3D_LOCAL_SURFACE', row) for row in seen.get('surface', [])]
            exact += [row_text('FP2_A RANK_EXACT', row) for row in seen.get('exact', [])]
            final += [row_text('FP2_A RANK_CHECK', row) for row in seen.get('final_state', [])]

        lines += ['### Pass times', '']
        lines += table(('Rank', 'Pass no.', *(label for label, _ in PASS_TIMES)), times)
        lines += ['### Cell-star and blocking observations', '']
        lines += table(('Rank', 'Pass no.', 'Sweep kind', 'Round', 'Sweep', 'Seconds', *(label for label, _ in STAR_COUNTS),
                        'First volume sweep'), stars)
        lines += row_section('Section 9 surface rows', surfaces)
        lines += row_section('Exact positivity and section 8 evidence', exact + final)

        alarm_passes = sorted({(row['rank'], row['pass']) for row in [*surface_alarms, *time_alarms]})
        for rank, pass_number in alarm_passes:
            seen = by_rank.get(rank, {})
            per_pass = lambda rows: [row for row in seen.get(rows, []) if row.get('pass') == pass_number]
            distance = [row_text('NATIVE3D_LOCAL_SWEEP', row) for row in per_pass('sweeps')
                        if row.get('kind') == 'surface_distance']
            lines += row_section(f'Rank {rank} pass {pass_number} distance sweep diagnostics',
                                 distance or ['— no surface distance sweep observed'])
            stars = [row_text('FP2_A RANK_STAR', row) for row in per_pass('largest_stars')]
            edges = [row_text('FP2_A RANK_STAR_EDGE', row) for row in per_pass('star_edges')]
            lines += row_section(f'Rank {rank} pass {pass_number} largest post-surface stars and refused edges',
                                 stars + (edges or ['— no refused incident edges observed']))

        if ended is not True:
            last = receipt.get('last_sweep_by_rank', {})
            lines += row_section('Last sweep observed on each rank', [
                row_text('NATIVE3D_LOCAL_SWEEP', last[str(rank)]) if last.get(str(rank))
                else f'rank={rank} — no sweep observed' for rank in range(ranks)
            ])
            slowest = receipt.get('five_slowest_sweeps', [])
            lines += row_section('Five longest observed sweeps', [
                row_text('NATIVE3D_LOCAL_SWEEP', row) for row in slowest
            ] or ['— no sweep observed'])
    return '\n'.join(lines)


def self_check():
    with TemporaryDirectory(prefix='fp2a-rank-check-report-') as directory:
        root = Path(directory)
        sweeps = [{'rank': '0', 'pass': '1', 'seconds': '37.54', 'kind': 'quality', 'max_star': '1024',
                   'star_gt100': '12', 'star_gt1000': '1', 'unresolved_points': '7', 'blocked_cells': '15',
                   'first_volume_sweep': '1', 'accepted_moves': '0'}]
        receipts = {
            'K1': {'ended': False, 'state': 'timeout', 'by_rank': {'0': {'sweeps': sweeps}},
                   'last_sweep_by_rank': {'0': sweeps[0], '1': None}, 'five_slowest_sweeps': sweeps},
            'K2': {'ended': False, 'state': 'failed', 'by_rank': {'1': {
                'passes': [{'rank': '1', 'pass': '1', 'seconds_pass': '160.2', 'seconds_length_work': '18.12'}],
                'surface': [{'rank': '1', 'pass': '1', 'applicable': '0', 'refused_memory': '0',
                             'first_feature_reason': 'quoted words'}],
                'exact': [{'rank': '1', 'cells': '118', 'nonpositive': '0'}]}}},
        }
        for name, receipt in receipts.items():
            (root / name).mkdir()
            (root / name / 'rank_check_receipt.json').write_text(json.dumps(receipt))
        (root / 'rank_check_summary.json').write_text(json.dumps({
            'proceed_11_3': False, 'results': {'K3': {'ended': False, 'state': 'skipped_k1_did_not_end'}},
        }))
        output = render(root)
        assert output.index('## K2') < output.index('## K1')
        assert 'stop before §11.3' in output and 'Skipped because K1' in output
        assert '160.2 | — | — | — | 18.12 | —' in output
        assert '1024 | 12 | 1 | 7 | 15 | 1' in output
        assert 'refused_memory=0 first_feature_reason="quoted words"' in output
        assert 'FP2_A RANK_EXACT rank=1 cells=118 nonpositive=0' in output
        assert 'rank=1 — no sweep observed' in output
        assert output.count('kind=quality max_star=1024') == 2
        assert '### Five longest observed sweeps' in output
        (root / 'rank_check_summary.json').unlink()
        assert 'No run receipt has been recorded.' in render(root)
        receipts['K1']['surface_alarms'] = [{'rank': '0', 'pass': '1', 'field': 'star_max', 'observed': 1024}]
        seen = receipts['K1']['by_rank']['0']
        seen['sweeps'].append({'rank': '0', 'pass': '1', 'kind': 'surface_distance', 'refused_walk': '1'})
        seen['largest_stars'] = [{'rank': '0', 'pass': '1', 'place': '1', 'point': '10', 'x': '0', 'y': '0', 'z': '0',
                                  'surface': '1', 'border': '0', 'incident': '1024'}]
        seen['star_edges'] = [{'rank': '0', 'pass': '1', 'point': '10', 'endpoint_a': '10', 'endpoint_b': '11',
                               'reason': 'walk'}]
        k2 = root / 'K2' / 'rank_check_receipt.json'
        previous = k2.with_name('historical_rank_check_receipt.json')
        k2.rename(previous)
        (root / 'K1/rank_check_receipt.json').write_text(json.dumps(receipts['K1']))
        (root / 'rank_check_summary.json').write_text(json.dumps({'correction': 3, 'proceed_11_3': False,
            'results': {'K2': {'reused_from_build': 626, 'receipt': str(previous), 'receipt_sha256': 'abc'}}}))
        output = render(root)
        assert 'Correction 3' in output and 'Retained from build 626' in output
        assert 'Rank 0 pass 1 distance sweep diagnostics' in output and 'refused_walk=1' in output
        assert 'largest post-surface stars and refused edges' in output and 'endpoint_b=11 reason=walk' in output
    print('FP2-A rank-check report self-check passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.root is None or not args.root.is_dir():
        parser.error('an existing rank-check build directory is required')
    print(render(args.root), end='')


if __name__ == '__main__':
    main()
