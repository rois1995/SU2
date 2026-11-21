"""Render FP2-A Correction 2 results from the rank-check runner receipts."""

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
    runs = {name: read_json(root / name / 'rank_check_receipt.json') for name in RUNS}
    order = ('K2', 'K1', 'K3') if runs['K2'].get('ended') is False else tuple(RUNS)
    lines = ['# FP2-A Correction 2 rank check', '', f'Evidence directory `{root}`.', '',
             'Times are seconds. Pass time covers the meeting through writeback on that rank. '
             'The star observation after the first completed volume sweep is the post-surface fallback. '
             'A dash marks a missing observation.', '']
    if summary.get('proceed_11_3') is True:
        lines += ['All three runs ended and K3 had no pass above 600 s. Continue with §11.2 and then §11.3.', '']
    elif summary.get('proceed_11_3') is False:
        lines += ['Continue with §11.2 and stop before §11.3.', '']

    for name in order:
        ranks, surface = RUNS[name]
        receipt = runs[name]
        result = summary.get('results', {}).get(name, {})
        state = receipt.get('state', result.get('state'))
        ended = receipt.get('ended', result.get('ended'))
        lines += [f'## {name} · {ranks} ranks · surface {surface}', '']
        if state == 'skipped_k1_did_not_end':
            lines += ['Skipped because K1 did not end within its limit.', '']
            continue
        if not receipt:
            lines += ['No run receipt has been recorded.', '']
            continue
        outcome = {True: 'Ended', False: 'Did not end', None: 'Outcome unavailable'}[ended]
        lines += [f'**{outcome}.**' + (f' Receipt state `{state}`.' if state else ''), '']
        if receipt.get('wait') is not None:
            lines += [f'Launcher exit evidence `{json.dumps(receipt["wait"], ensure_ascii=False)}`.', '']
        if difference := receipt.get('first_difference'):
            lines += [f'First observed difference `{json.dumps(difference, ensure_ascii=False)}`.', '']
        if alarms := receipt.get('alarm_over_600'):
            lines += row_section('Passes above the 600 s alarm', [row_text('ALARM', row) for row in alarms])

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
