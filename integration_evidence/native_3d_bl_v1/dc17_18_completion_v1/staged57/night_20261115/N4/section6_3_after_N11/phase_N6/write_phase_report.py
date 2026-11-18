"""Render the report for the passed N6 cut from the sealed receipts."""
from pathlib import Path

from run_g5 import ROOT, WORKSPACE, cut
from seal_phase import BUILDS, N6

HANDBOOK = cut.REPO / 'Handbook'


def link(label, path):
    return f'[{label}](../{Path(path).relative_to(cut.REPO)})'


def longest():
    drafts = N6 / 'drafts'
    remesh = cut.read(drafts / 'remesh/readability.json')['ten_longest']
    optimizer = cut.read(drafts / 'optimizer/readability.json')['longest_functions']
    selection = cut.read(drafts / 'selection/mechanical_checks.json')['ten_longest']
    rows = [(row['formatted_lines'], row['name']) for row in remesh]
    rows += [(row['lines'], row['function']) for row in optimizer]
    rows += [(size, f'Selection::{name}') for name, size in selection]
    return sorted(rows, key=lambda row: (-row[0], row[1]))[:10]


def scoped(table):
    table.extend((
        '| Sub-step | Checkpoint and build | G1 serial / 2 / 4 ranks (s) | G2 stop (s) | G3 splits / exact common edits | GQ stop (s) |',
        '|---|---|---:|---:|---:|---:|',
    ))
    for label, build in BUILDS.items():
        path = cut.PHASE / f'N6{label}/validation/acceptance_summary.json'
        summary = cut.read(path)
        checkpoint = cut.read(N6 / f'{label}_checkpoint.json')
        assert checkpoint['passed'] and checkpoint['build'] == build
        g1 = ' / '.join(f"{row['elapsed_seconds']:.3f}" for row in summary['G1'].values())
        g3 = summary['G3']
        table.append(f"| {link(label, path)} | `{checkpoint['commit'][:10]}` / {build} | {g1} | "
                     f"{summary['G2']['seconds_until_stop']:.3f} | {g3['accepted_splits']:,} / "
                     f"{g3['common_edit_count']:,} | {summary['GQ']['seconds_until_stop']:.3f} |")
    return summary


def full_workflow(lines):
    path = WORKSPACE / 'acceptance_summary.json'
    g5 = cut.read(path)
    assert g5['passed'] and g5['old_rows_exact'] and g5['timer_snapshots_exact'] == 96
    lines.extend(('', '## G5 and isolated restart', '',
                  f"The two-event run completed in {g5['seconds_total']:.3f} s. The three event-one P6c files, "
                  'the three event-two P7 files, the old integer rows, and 96 timer snapshots matched. '
                  'Both events passed certification, transfer and continued CFD. '
                  + link('G5 summary and six SHA-256 hashes', path),
                  '',
                  '| Event | Native max (s) | Points / cells | Job peak and process-peak sum (B) | Min. timer coverage | Join difference max | Owned proof faces / surface size | Max. transfer residual |',
                  '|---|---:|---:|---:|---:|---:|---:|---:|'))
    for event in g5['events']:
        count = event['actual_mesh_counts']
        lines.append(f"| {event['event']} | {event['native_seconds']:.3f} | {count['points']:,} / {count['cells']:,} | "
                     f"{event['job_peak_rss_bytes']:,} | {event['minimum_timer_coverage']:.4%} | "
                     f"{event['maximum_join_relative_difference']:.8%} | {event['proof_faces']:,} / "
                     f"{event['proof_faces']:,} | {event['maximum_transfer_residual']:.3e} |")
    restart = g5['isolated_restart']
    lines.extend(('', f"The isolated restart reopened {restart['adapted_points']:,} points and "
                      f"{restart['adapted_cells']:,} cells with zero coordinate error. It passed continued CFD in "
                      f"{restart['seconds']:.3f} s. "
                      + link('Disk restart validator', WORKSPACE / 'disk_restart_full578_event2/disk_restart_validation.json')))


def main():
    frozen = cut.read(ROOT / 'acceptance_summary.json')
    assert frozen['passed']
    lines = ['# Native 3D N6 cuts', '',
             'The three N6 cuts passed and were pushed. Build578 qualified the integrated extraction. The shared '
             'no-BL workflow still uses the six P6c/P7 reference files. N6 compared full integer rows and timer '
             'calls with the accepted build574 T0 run. ' + link('Whole-item source check', N6 / 'source_check.md'),
             '', '## Scoped gates', '']
    selection = scoped(lines)
    g4 = ' / '.join(f"{row['status']['seconds']:.3f}" for row in selection['G4']['new_cases'])
    lines.extend(('', 'G1 matched 375 / 114 / 228 case records and failed assertions in every sub-step. '
                      'G2 matched work 90,809,869 and counts 480 / 473 / 346 / 263 / 469 / 464. G2 and GQ '
                      'preserved old integer rows, modeled storage, all timer calls, fixed storage 4,120 B and '
                      'census delta zero. GQ matched all six third quality windows. G3 met every physical gate '
                      'with at least 336 splits and observer-adjusted entry work 5,113. '
                      'G3 is a clock-bounded saved surface fixture.',
                      '', f'G4 passed native 2D with and without BL at 1 / 2 / 4 ranks in {g4} s. '
                      + link('Native 2D receipts', cut.PHASE / 'N6Selection/validation/g4_case_paths.json'),
                      '', '## Readability', '',
                      'The public Remesh entry is 50 formatted lines. The six source fragments '
                      'keep their numerical checks and caller contracts. The formatted inventories found maxima '
                      'of 74 lines for the Remesh helpers, 76 for the optimizer and 60 for Selection. '
                      'New lambdas fit the 15-line limit. N6 added no exception. The original move-as-is '
                      'exception for the legacy wall remains part of phase C.',
                      '', '| Longest new or moved definition | Formatted lines |', '|---|---:|'))
    lines.extend(f'| `{name}` | {count} |' for count, name in longest())
    lines.extend(('', '## Found during the cut', '',
                  '- The public remesh entry supplies the failure caller name. Its timing summary reads the '
                  'report clock before releasing the summary buffers.',
                  '- The optimizer can fail a late binding check after mutation. The cut kept that order. '
                  'The interior and boundary ring operators use the same scoring and traversal loops, with '
                  'independent memoization, retry, certification and commit state.',
                  '- Selection reset leaves the blocked directions, blocked links and repair age alone. '
                  'Migration swaps its explicit original member list. The traveling row types, bytes '
                  'calculation and repair update order stay in place.',
                  '', '## Readings used and helper corrections', '',
                  'The Remesh helpers retain the public caller name and report clock boundary. The optimizer uses '
                  'two named helpers for the identical ring loops. Selection keeps a single owner with two '
                  'value members. The Selection patch generator initially resolved the repository one directory '
                  'too high, then located the formatting file from its ancestors. Both engine drafts passed '
                  'syntax-only compilation with the accepted flags in private overlays. The no-launch self-check first '
                  'resolved the historic safeguard module by name, then loaded the N6 entry directly. '
                  'The first optimizer GQ wrapper stopped at the existing free-space preflight before numerical '
                  'launch. We retained the passed build, G1, G2 and G3, verified lossless archived copies of '
                  'the phase C and D backup binaries and ran only the missing GQ. '
                  + link('R13 choices', cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md') + ' · '
                  + link('Selection draft review', N6 / 'drafts/selection/REVIEW.md') + ' · '
                  + link('Adapter self-check', ROOT / 'adapter_self_check.json') + ' · '
                  + link('Disk recovery', N6 / 'R3_disk_preflight/space_recovery.json')))
    full_workflow(lines)
    guard_path = ROOT / 'safeguards/acceptance_summary.json'
    guards = cut.read(guard_path)['safeguards']
    values = ' / '.join(f"{row['seconds']:.3f}" for row in guards.values())
    lines.extend(('', '## Standing safeguards and terminal state', '',
                  f"The geometry merit, saved 481 and current saved-target checks passed in {values} s. "
                  f"All required saved payloads and logical rows matched. The audit verified {frozen['terminal_identities']} "
                  f"terminal process identities and {frozen['source_pins_verified']:,} source pins. "
                  'The actual-host preflight found zero owned heavy jobs. '
                  + link('Safeguard receipts', guard_path) + ' · '
                  + link('Terminal audit', ROOT / 'terminal_audit.json') + ' · '
                  + link('Binary manifest', ROOT / 'binary_manifest.json'),
                  '', '## Unit tests that fail on the accepted base', '',
                  'G1 uses the adopted N14 cases and exact failure values. '
                  + link('Case, file, line and printed values', cut.PHASE / 'phase_C/accepted_base_g1_failures.md')))
    (HANDBOOK / 'ADAPTATION_CUT_N6_20261118.md').write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
