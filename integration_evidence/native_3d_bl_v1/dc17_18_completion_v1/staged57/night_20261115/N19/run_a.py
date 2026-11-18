"""Qualify the default-width M29 instrument without launching a full workflow."""

import gzip
from pathlib import Path
import re
import subprocess
import sys

N19 = Path(__file__).resolve().parent
sys.path.insert(0, str(N19.parent / 'N18'))
from run_de import bind_build, bind_identity, check_build, cut, git, seal, verify_g4

BUILD = 581
SOURCES = (
    'Common/include/adaptation/CNativeEngine3D.hpp',
    'Common/src/adaptation/native_engine3d/EngineControl.cpp',
    'Common/src/adaptation/native_engine3d/EngineControl.hpp',
    'Common/src/adaptation/native_engine3d/EngineReports.cpp',
    'Common/src/adaptation/native_engine3d/EngineReports.hpp',
    'Common/src/adaptation/native_engine3d/NativeRemeshRun.inc',
)


def report_paths(summary, workspace):
    for cohort in summary['G1']:
        receipt = cut.read(workspace / f'G1_candidate_{cohort}_summary.json')
        yield Path(receipt['candidate_attempt']) / 'run.log'
    for row in summary['G4']['new_cases']:
        yield Path(row['case']) / 'run.log'
    for case in (workspace / 'cases').iterdir():
        if case.name.startswith('N19A_G3_'):
            yield case / 'run.log'
        elif case.name.startswith(('C11_G2_', 'C11_GQ_')):
            yield from sorted(case.glob('rank_output/*/rank.*/stdout'))


def check_print_guard(workspace, summary):
    logs = list(report_paths(summary, workspace))
    assert len(logs) == 19, logs
    dump = re.compile(rb'NATIVE3D_M29_(BEGIN|CELL|END) rank=(\d+)\b')
    completion = re.compile(rb'\bNATIVE3D rank=(\d+)\b.*?\bcomplete=([01])\b')
    rejected = []
    for path in logs:
        if path.is_file():
            reader = path.open('rb')
        else:
            reader = gzip.open(path.with_name(path.name + '.gz'), 'rb')
        pending = {}
        with reader as source:
            for number, line in enumerate(source, 1):
                if match := dump.search(line):
                    tag, rank = match.groups()
                    pending.setdefault(rank, []).append((tag, number))
                if (match := completion.search(line)) and match[1] in pending:
                    rank, complete = match.groups()
                    records = pending.pop(rank)
                    assert complete == b'0', (path, number, records)
                    assert records[0][0] == b'BEGIN' and records[-1][0] == b'END', (path, records)
                    rejected.append({'path': str(path), 'rank': int(rank), 'm29_rows': len(records),
                                     'completion_line': number, 'complete': 0})
        assert not pending, (path, pending)
    cut.save(N19 / 'A_print_guard.json', {'pass': True, 'logs': [str(path) for path in logs],
                                         'rejected_events_with_m29': rejected})


def main():
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert git('rev-parse', '--short=10', 'HEAD') == 'e1d9db1c17'
    assert git('diff', '--name-only', '--', 'Common', 'SU2_CFD', 'UnitTests').splitlines() == list(SOURCES)
    subprocess.run(['sha256sum', '-c', 'SHA256SUMS'], cwd=N19 / 'inputs', check=True)
    target = bind_build(579, BUILD, SOURCES, cut.PHASE / 'build579/source_pins.json')
    workspace = bind_identity('N19A', BUILD)
    cut.qualify(11, BUILD, workspace, target)
    check_build(target)
    verify_g4(workspace, cut.PHASE / 'N18DE/validation')
    seal('N19A', 11, BUILD, workspace, target)
    summary = cut.read(workspace / 'acceptance_summary.json')
    assert [row['records'] for row in summary['G1'].values()] == [375, 114, 228]
    check_print_guard(workspace, summary)
    print('N19 A passed the scoped gates and emit guard.', flush=True)


if __name__ == '__main__':
    main()
