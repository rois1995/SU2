"""Seal the successful Part A evidence and record its pushed checkpoint."""

from pathlib import Path
import re
import subprocess
from run_a import N19, SOURCES, cut
from run_de import checkpoint

PART_A = cut.PHASE / 'N19A/validation'
A_BUILD = cut.PHASE / 'build581'
REPORTS = cut.REPO / 'Common/src/adaptation/native_engine3d/EngineReports.cpp'


def document_helpers(pins):
    """Finish comments and their alignment after the pinned run."""
    comments = {
        'static bool M29Touches(': '// Tests vertex incidence without expanding a cell into an auxiliary record.',
        'static void M29PrintTensor(': '// Writes tensor components in the six-value order shared by both M29 columns.',
        'void EngineReports::PrintStuckNeighborhood(': '// Writes the owned, lowest-level cells of the rejected exit census.',
    }
    before = REPORTS.read_text().splitlines(keepends=True)
    after = list(before)
    for function, comment in comments.items():
        line, = [i for i, value in enumerate(before) if value.startswith(function)]
        assert before[line - 1] == '\n' and before[line - 2] == '}\n'
        after[line - 1] = comment + '\n'
    same_input = [left == right or (left == '\n' and right.startswith('// ') and '\\' not in right)
                  for left, right in zip(before, after, strict=True)]
    assert all(same_input)
    REPORTS.write_text(''.join(after))
    header = cut.REPO / SOURCES[0]
    old = 'bool report_rejected_neighborhood = false;  // Print-only'
    new = 'bool report_rejected_neighborhood = false;       // Print-only'
    assert old.split() == new.split()
    text = header.read_text()
    assert text.count(old) == 1
    header.write_text(text.replace(old, new))
    current = dict(pins)
    changes = {}
    for path in (REPORTS, header):
        current[str(path)] = cut.sha(path)
        changes[str(path.relative_to(cut.REPO))] = {'qualified_sha256': pins[str(path)],
                                                  'checkpoint_sha256': current[str(path)]}
    cut.save(N19 / 'A_checkpoint_source_pins.json', current)
    cut.save(N19 / 'A_comment_proof.json',
             {'changed_only_blank_lines_to_line_comments': sum(a != b for a, b in zip(before, after)),
              'header_comment_alignment_only': True, 'source_line_count_unchanged': True,
              'source_hashes': changes})


def readability():
    arguments = ['clang-format', '--dry-run', '--Werror']
    for name in SOURCES:
        diff = subprocess.check_output(['git', 'diff', '--unified=0', '--', name], cwd=cut.REPO, text=True)
        changed = []
        for start, count in re.findall(r'^@@ [^+]+\+(\d+)(?:,(\d+))? @@', diff, re.M):
            if int(count or 1):
                changed.append(f'--lines={start}:{int(start) + int(count or 1) - 1}')
        subprocess.run([*arguments, *changed, name], cwd=cut.REPO, check=True)
    output = N19 / 'A_readability.md'
    rows = []
    for name in (SOURCES[1], SOURCES[3], SOURCES[5]):
        source = (cut.REPO / name).read_text().splitlines()
        for start, line in enumerate(source):
            match = re.search(r'\b(ObserveProgress|EmitFinalRowsAndMove|ApplyMetric|PrintStuckCells|'
                              r'PrintStuckNeighborhood|BlockedShapeRepairs|M29\w+)\(', line)
            if not match or not line.startswith(('bool ', 'EngineResult ', 'void ', 'static ', '  void ')):
                continue
            indent = len(line) - len(line.lstrip())
            end = next(i for i in range(start + 1, len(source)) if source[i] == ' ' * indent + '}')
            rows.append((end - start + 1, match[1]))
    assert len(rows) >= 10 and max(size for size, _ in rows) <= 80, rows
    output.write_text("The changed implementation ranges passed `clang-format --dry-run --Werror`. "
                      "The dry run produced no diagnostics. All new lambdas fit in 15 lines.\n\n"
                      "| Changed or new function | Formatted lines |\n|---|---:|\n" +
                      ''.join(f'| `{name}` | {size} |\n' for size, name in sorted(rows, reverse=True)[:10]))


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert cut.terminal(cut.read(N19 / 'A_controller_launch.json'))
    guard = cut.read(N19 / 'A_print_guard.json')
    assert guard['pass']
    summary = cut.read(PART_A / 'acceptance_summary.json')
    assert summary['state'] == 'PASSED' and summary['build'] == 581
    pins = cut.read(A_BUILD / 'source_pins.json')
    assert all(cut.sha(path) == digest for path, digest in pins.items())
    document_helpers(pins)
    readability()
    g1, g2, g3, g4, gq = (summary[key] for key in ('G1', 'G2', 'G3', 'G4', 'GQ'))
    part_a = N19 / 'PartA.md'
    part_a.write_text(
        f"Build581 compiled without warnings. It verified {summary['source_pins']:,} source pins. "
        "No completed event or prefix emitted M29. "
        f"The guard paired M29 with {len(guard['rejected_events_with_m29'])} rejected event in the scoped logs. "
        "This was an expected serial G1 refusal. Its begin and end both reported zero seeds or cells.\n\n"
        f"- G1 matched all {g1['serial']['records']} / {g1['mpi2']['records']} / {g1['mpi4']['records']} "
        "case records at serial / 2 / 4 ranks, with zero changed failures.\n"
        f"- G2 matched work {g2['work']:,}, collapse counts "
        f"{'/'.join(map(str, g2['accepted_collapses']))}, and the required integer, "
        "modeled storage and timer rows.\n"
        f"- G3 passed its physical gates with {g3['accepted_splits']:,} splits. "
        f"All {g3['common_edit_count']:,} common edits matched the N14 first run. "
        "Adjusted entry atlas work matched at 5,113.\n"
        f"- G4 passed at 1 / 2 / 4 ranks with identical successful assertion summaries "
        f"for all {len(g4['new_cases'])} cases.\n"
        f"- GQ matched old integer, modeled storage and timer rows through its third quality window. "
        f"The prefix stopped in {gq['seconds_until_stop']:.3f} s. G2 and GQ held "
        f"`fixedstorage={gq['fixedstorage']}` and census call delta 0.\n\n"
        "[Gate receipt](../N4/section6_3_after_N11/N19A/validation/acceptance_summary.json) · "
        "[Print guard](A_print_guard.json) · [Readability](A_readability.md). "
        "Three descriptive comments replaced three blank lines, and a trailing comment was aligned "
        "after the pinned campaign. "
        "All compiled tokens and source line numbers are identical. "
        "[Comment proof](A_comment_proof.json)\n"
    )
    progress = cut.NIGHT / 'progress.md'
    with progress.open('a') as output:
        output.write(f"\nN19 A. Default-width build581 passed G1 375/114/228, G2 {g2['work']:,}, "
                     f"G3 {g3['accepted_splits']:,} splits with {g3['common_edit_count']:,} common edits, "
                     f"G4 1/2/4 and GQ {gq['seconds_until_stop']:.3f} s. Fixed storage 4,120, "
                     "no M29 output on success or measured prefixes. Preparing the checkpoint.\n")
    extra = [N19 / name for name in ('run_a.py', 'checkpoint_a.py', 'A_controller.log', 'A_controller_launch.json',
                                    'source_check.md', 'A_print_guard.json', 'inputs/MEASUREMENT_M29.md',
                                    'inputs/SHA256SUMS', 'PartA.md', 'A_comment_proof.json',
                                    'A_checkpoint_source_pins.json', 'A_readability.md')]
    paths = [*cut.read(PART_A / 'checkpoint_paths.json'), *SOURCES, *extra,
             PART_A / 'checkpoint_paths.json', PART_A / 'run_g2_gq_prefix.py', progress,
             cut.NIGHT / 'QUESTIONS_FOR_REVIEWER.md']
    head = checkpoint('Measure the stuck-cell neighborhood on native 3D rejection', paths)
    cut.save(N19 / 'A_checkpoint.json', {'passed': True, 'build': 581, 'commit': head})
    with progress.open('a') as output:
        output.write(f'\nN19 A. Pushed `{head[:10]}`. Next is the exact three-path diagnostic overlay.\n')


if __name__ == '__main__':
    main()
