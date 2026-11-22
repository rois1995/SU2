"""Verify recorded process identities and check for live FP2-B workloads on the host."""

from argparse import ArgumentParser
from datetime import datetime
import os
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE.parent / name) for name in ('FP1', 'N18', 'FP2_B0')]
from archive_fp1 import cut
from run_b0 import host, verify_terminal
from seal_f import identity_rows


def audit():
    host()
    verify_terminal()
    receipts, identities = [], set()
    kinds = ('launch', 'wait', 'status', 'terminal', 'receipt', 'process', 'preflight', 'measurement')
    for path in sorted(HERE.rglob('*.json')):
        if path.name == 'terminal_audit.json' or not any(kind in path.stem for kind in kinds):
            continue
        rows = list(identity_rows(cut.read(path)))
        if rows:
            receipts.append(str(path.relative_to(HERE)))
            identities.update((row['pid'], row['start_ticks']) for row in rows)
    rows = [{'pid': pid, 'start_ticks': ticks} for pid, ticks in sorted(identities)]
    assert rows and all(cut.terminal(row) for row in rows), rows

    ancestors = set()
    pid = os.getpid()
    while pid > 1:
        ancestors.add(pid)
        stat = Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()
        pid = int(stat[1])
    live = []
    for path in Path('/proc').iterdir():
        if not path.name.isdigit() or int(path.name) in ancestors:
            continue
        try:
            command = (path / 'comm').read_text().strip()
            if command not in {'SU2_CFD', 'test_driver', 'mpirun', 'orterun', 'ninja', 'cc1plus', 'time', 'timeout'} \
                    and not command.startswith('python'):
                continue
            argv = (path / 'cmdline').read_bytes().split(b'\0')
            case_arg = any(b'/FP2_B/' in arg and (arg.endswith(b'.py') or b'/G5/build' in arg) for arg in argv)
            cwd = (path / 'cwd').resolve(strict=True)
            if case_arg or cwd.is_relative_to(HERE):
                live.append(int(path.name))
        except OSError:
            continue
    assert not live, live
    return {'verified_at': datetime.now().astimezone().isoformat(), 'actual_host_proc': True,
            'receipts': receipts, 'all_owned_handles_terminal': True,
            'identities': [{**row, 'terminal': True} for row in rows], 'unrecorded_live_matches': live}


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    result = audit()
    if args.record:
        cut.save(HERE / 'terminal_audit.json', result)
    print(f'{len(result["identities"])} owned identities terminal. No live FP2-B workload matched.')
