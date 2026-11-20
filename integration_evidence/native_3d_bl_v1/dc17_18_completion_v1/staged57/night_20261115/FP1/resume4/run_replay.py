"""Run the single conditional six-rank H6 frozen event-one replay."""

from argparse import ArgumentParser
from collections import Counter
from decimal import Decimal
import json
import os
from pathlib import Path
import re
import runpy
import signal
import subprocess
import sys
import time

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
CASE = HERE / 'event1'
LAB = HERE / 'lab/lab_summary.json'
PREVIOUS = HERE.parent / 'resume2/run_replay.py'
REPORT = HERE.parent / 'resume2/report_replay.py'
HOUSE = HERE.parents[2] / 'p7p8_r3_20261115/run_full_workflow.py'
LAB_RUNNER = HERE / 'run_lab.py'
ORDERS = {
    str(HERE.parent / 'inputs/FP1_RESUME_3.md'): '0db5cccf8b48cc100e3eacf3b381bf7ef779c216b03e6d1919d1f2da03473b7d',
    str(HERE.parent / 'inputs/FP1_RESUME_4.md'): '742c936b94e08cb02badd7f3b77757395f2123075ebcffe442cbd064ab1f363f',
}
old = runpy.run_path(str(PREVIOUS))['run'].__globals__
report = runpy.run_path(str(REPORT))
house = runpy.run_path(str(HOUSE))
cut, sha = old['cut'], old['sha']
RANKS = report['RANKS']
WALL = 5400
SHIFT_DEADLINE = 600
STAGE0_LIMIT = Decimal(180)
POLL_SECONDS = 1
MARKER = re.compile(r'\bNATIVE3D_(PARTITION|LOCAL_SWEEP|LOCAL_CORE|LOCAL_SHIFT)(?=\s)')


def fields(line, rank):
    line = report['STRIP'](report['ANSI'].sub('', line))
    matches = list(MARKER.finditer(line))
    for match, end in zip(matches, [*matches[1:], None]):
        text = line[match.start():end.start() if end else len(line)].rstrip()
        values = dict(report['FIELD'].findall(text))
        if values.get('rank') == str(rank):
            yield match[1], values, text


def seconds(row):
    value = Decimal(row['seconds'])
    assert value.is_finite() and value >= 0, row
    return value


class CoreWatch:
    def __init__(self):
        self.partition = {}
        self.first_sweep_begin = {}
        self.higher = {}
        self.offsets = {}

    def observe(self, rank, line, elapsed):
        for marker, row, _ in fields(line, rank):
            if marker == 'PARTITION' and row.get('parts') == '6':
                self.partition.setdefault(rank, elapsed)
            if row.get('stage') != '0':
                continue
            if marker == 'LOCAL_SWEEP' and row.get('pass') == '1':
                self.first_sweep_begin.setdefault(rank, elapsed - float(seconds(row)))
            if marker == 'LOCAL_SHIFT' and row.get('direction') == 'higher':
                self.higher.setdefault(rank, elapsed)

    def poll(self, case, started):
        for rank, path in report['streams'](case).items():
            with path.open('rb') as stream:
                stream.seek(self.offsets.get(path, 0))
                while line := stream.readline():
                    if not line.endswith(b'\n'):
                        break
                    self.observe(rank, line.decode(errors='replace'), time.monotonic() - started)
                    self.offsets[path] = stream.tell()

    def core_start(self):
        return max(self.partition.values()) if self.partition.keys() == RANKS else None

    def stop(self, elapsed):
        start = self.core_start()
        if start is not None and elapsed >= start + SHIFT_DEADLINE:
            if any(self.higher.get(rank, float('inf')) > start + SHIFT_DEADLINE for rank in RANKS):
                return 'stage0_higher_shift_missing_at_600_seconds'
        if elapsed >= WALL:
            return 'event_wall_limit'
        return None


def kill_owned(identity):
    # The pidfd keeps a recycled PID from being confused with a witnessed replay member.
    try:
        handle = os.pidfd_open(identity['pid'])
        try:
            if not cut.terminal(identity):
                signal.pidfd_send_signal(handle, signal.SIGKILL)
                return True
        finally:
            os.close(handle)
    except ProcessLookupError:
        pass
    return False


def sample_peaks(live, peaks):
    for entry in live:
        if entry.get('comm') != 'test_driver':
            continue
        pid = entry['pid']
        try:
            raw = Path(f'/proc/{pid}/status').read_text()
            match = re.search(r'^VmHWM:\s+(\d+)\s+kB$', raw, re.M)
            if match and not cut.terminal(entry):
                key = (pid, entry['start_ticks'])
                peaks[key] = max(peaks.get(key, 0), int(match[1]) * 1024)
        except FileNotFoundError:
            continue


def guarded_gate(case, name, argv):
    assert case == CASE and name == 'test'
    recipe = cut.read(case / 'command.json')
    assert recipe['argv'] == argv and recipe['ranks'] == 6 and recipe['seconds_limit'] == WALL
    assert recipe['selector'] == '[.][NativeFrozenVolume3D]'
    assert hasattr(os, 'pidfd_open') and hasattr(signal, 'pidfd_send_signal')
    subprocess.run([cut.PYTHON, str(cut.PREFLIGHT), str(case / 'test_host_preflight.json')],
                   cwd=cut.REPO, check=True)
    print('H6 frozen event 1 starting', flush=True)
    watch, owned, killed, peaks = CoreWatch(), {}, [], {}
    stop, observed_stop = None, None
    with (case / 'test_outer.log').open('x') as logfile:
        started = time.monotonic()
        child = subprocess.Popen(argv, cwd=cut.REPO, stdout=logfile, stderr=subprocess.STDOUT,
                                 start_new_session=True)
        root = cut.actual_identity(child.pid)
        owned[root['pid'], root['start_ticks']] = root
        cut.save(case / 'test_launch.json', {'actual_host_proc': True, 'controller': root,
                                             'supervisor': cut.actual_identity(os.getpid()), 'unix': time.time()})
        while True:
            watch.poll(case, started)
            rows = house['read_processes']()
            live = {entry['pid'] for entry in owned.values() if house['identity_alive'](entry, rows)}
            # mpirun may change its process group, so retain session members and witnessed descendants.
            session_pid = rows.get(root['pid'])
            if session_pid is None or session_pid['start_ticks'] == root['start_ticks']:
                live.update(pid for pid, row in rows.items() if row['sid'] == root['pid'])
            for pid in house['descendants'](rows, live):
                row = rows[pid]
                owned.setdefault((pid, row['start_ticks']), row)
            code = child.poll()
            elapsed = time.monotonic() - started
            live = [entry for entry in owned.values() if house['identity_alive'](entry, rows)]
            sample_peaks(live, peaks)
            stop = stop or watch.stop(elapsed)
            if stop and observed_stop is None:
                observed_stop = elapsed
                print(f'H6 replay stop at {elapsed:.3f} s ({stop})', flush=True)
            if stop:
                for entry in live:
                    if kill_owned(entry):
                        killed.append({'pid': entry['pid'], 'start_ticks': entry['start_ticks']})
            if code is not None and not live:
                break
            time.sleep(POLL_SECONDS)
    watch.poll(case, started)
    terminal = all(cut.terminal(entry) for entry in owned.values())
    wait = {'actual_host_proc': True, 'controller': root, 'returncode': code,
            'elapsed_seconds': time.monotonic() - started, 'terminal': terminal}
    cut.save(case / 'core_watch.json', {
        'elapsed_seconds_origin': 'watch monotonic clock immediately before spawning the pinned wrapper',
        'core_start_basis': 'latest first initial partition row observed across six rank stdout files',
        'poll_interval_seconds': POLL_SECONDS, 'first_partition_observed_seconds': watch.partition,
        'stage0_core_start_proxy_seconds': watch.core_start(),
        'first_stage0_sweep_begin_estimate_seconds': watch.first_sweep_begin,
        'stage0_higher_observed_seconds': watch.higher, 'stop': stop, 'stop_observed_seconds': observed_stop,
        'owned_processes': list(owned.values()), 'sigkill_sent_to': killed, 'all_owned_terminal': terminal,
        'sampled_rank_peak_lower_bounds': [{'pid': pid, 'start_ticks': ticks, 'peak_rss_bytes': peak}
                                           for (pid, ticks), peak in peaks.items()],
        'sampled_job_peak_lower_bound_bytes': sum(peaks.values()) if peaks else None,
    })
    cut.save(case / 'test_wait.json', wait)
    assert terminal
    return wait


def stage0(on):
    ranks, failures = {}, []
    for rank in sorted(RANKS):
        record = on['rank_data'].get(rank, {})
        core = [row['fields'] for row in record.get('core_rows', ()) if row['fields'].get('stage') == '0']
        shift = [row['fields'] for row in record.get('shift_rows', ()) if row['fields'].get('stage') == '0']
        complete = (Counter(row.get('pass') for row in core) == {'1': 1, '2': 1} and
                    all(row.get('applicable') == '1' for row in core) and
                    Counter(row.get('direction') for row in shift) == {'lower': 1, 'higher': 1})
        total = sum((seconds(row) for row in [*core, *shift]), Decimal(0))
        ranks[rank] = {'completed_rows_seconds': str(total), 'inclusive_core_seconds': str(total) if complete else None,
                       'complete': complete}
        if not complete or total > STAGE0_LIMIT:
            failures.append(rank)
    return {'ranks': ranks, 'failed_ranks': failures}


def lab_gate(build):
    lab = runpy.run_path(str(LAB_RUNNER))
    summary = cut.read(LAB)
    status = cut.read(build / 'build_status.json')
    _, source_sha = old['verify_build'](status, build / 'source_pins.json')
    assert (summary['build'], summary['test_binary_sha256'], summary['source_pins_sha256']) == (
        status['build'], status['test_binary_sha256'], source_sha)
    results = summary['results']
    assert tuple((row['dataset'], row['variant']) for row in results) == lab['PAIRS']
    held = lab['v2_holds'](results)
    assert held == summary['v2_hold']
    pins = {str(LAB): sha(LAB)}
    for result in results:
        case = LAB.parent / f"{result['dataset']}_{result['variant']}"
        receipt_path = case / 'lab_receipt.json'
        receipt = cut.read(receipt_path)
        assert all(receipt[key] == value for key, value in result.items()), receipt_path
        old['verify']({str(case / name): digest for name, digest in receipt['artifacts'].items()})
        wait = result['wait']
        assert wait['terminal'] and wait == cut.read(case / 'test_wait.json')
        processes = cut.read(case / 'owned_processes.json')
        assert processes['all_observed_terminal']
        if Path('/proc/1/comm').read_text().strip() == 'systemd':
            assert cut.terminal(wait['controller'])
            assert all(cut.terminal(entry) for entry in processes['identities'])
        pins[str(receipt_path)] = sha(receipt_path)
    return held, pins


def run(build, verify_only):
    old['verify'](ORDERS)
    held, pins = lab_gate(build)
    if held:
        return {'launched': False, 'stop': 'resume4_v2_lab_hold', 'datasets': held}
    prepare = old['prepare_input']
    support = (Path(__file__).resolve(), REPORT, HOUSE, LAB_RUNNER)
    pins.update({**ORDERS, **{str(path): sha(path) for path in support}})

    def prepare_pinned(event):
        config, sensor, copies, previous_pins, cells = prepare(event)
        return config, sensor, copies, {**previous_pins, **pins}, cells

    if not verify_only and (CASE / 'command.json').exists() and not any((CASE / name).exists() for name in (
            'test_launch.json', 'test_wait.json', 'test_outer.log', 'rank_output')):
        assert Path('/proc/1/comm').read_text().strip() == 'systemd'
        prior = CASE / f'preflight_{time.time_ns()}'
        prior.mkdir()
        for name in ('command.json', 'test_host_preflight.json'):
            if (CASE / name).exists():
                (CASE / name).rename(prior / name)

    original_gate, original_here = cut.run_gate, old['HERE']
    old['HERE'], old['prepare_input'], cut.run_gate = HERE, prepare_pinned, guarded_gate
    try:
        result = old['run'](build, 1, verify_only)
    finally:
        old['HERE'], old['prepare_input'], cut.run_gate = original_here, prepare, original_gate
    if verify_only:
        return result

    watch_path = CASE / 'core_watch.json'
    watch = cut.read(watch_path)
    assert watch['all_owned_terminal'] and all(cut.terminal(entry) for entry in watch['owned_processes'])
    comparison = report['compare'](CASE, 1, None)
    on = comparison['fp1_on_replay']
    for rank, path in report['streams'](CASE).items():
        on['rank_data'][rank]['sweep_rows'] = [
            {'line': number, 'text': text, 'fields': row}
            for number, line in enumerate(report['read_log'](path).splitlines(keepends=True), 1)
            if line.endswith('\n')
            for marker, row, text in fields(line, rank) if marker == 'LOCAL_SWEEP']
    comparison['notes'][4] = ('Resume 4 core, shift, and sweep rows preserve explicit normalization stage. '
                              'Stage 0 inclusive seconds use CORE and SHIFT totals without nested sweep time.')
    gate = stage0(on)
    start = watch['stage0_core_start_proxy_seconds']
    observed = watch['stage0_higher_observed_seconds']
    timely = start is not None and all(observed.get(str(rank), float('inf')) <= start + SHIFT_DEADLINE
                                       for rank in RANKS)
    gate.update(higher_shift_timely=timely, first_result_message=result['first_message'], accepted=result['accepted'],
                watch=watch, watch_sha256=sha(watch_path),
                proceed=bool(not watch['stop'] and timely and not gate['failed_ranks'] and result['accepted']))
    comparison['resume4_stop_rule'] = gate
    path = CASE / 'replay_comparison.json'
    if path.is_file():
        assert cut.read(path) == json.loads(json.dumps(comparison))
    else:
        cut.save(path, comparison)
    return {'build': result['build'], 'accepted': result['accepted'], 'proceed': gate['proceed'],
            'watch_stop': watch['stop'], 'failed_core_ranks': gate['failed_ranks'],
            'comparison': str(path)}


def self_check():
    watch = CoreWatch()
    for rank in range(6):
        watch.observe(rank, f'NATIVE3D_PARTITION rank={rank} parts=6\n', 10 + rank)
    watch.observe(5, 'NATIVE3D_LOCAL_SWEEP rank=5 stage=0 pass=1 seconds=2\n', 22)
    for rank in range(6):
        stage = int(rank == 5)
        watch.observe(rank, f'NATIVE3D_LOCAL_SHIFT rank={rank} stage={stage} direction=higher seconds=.25\n', 600)
    assert watch.core_start() == 15 and watch.first_sweep_begin == {5: 20}
    assert watch.stop(614.999) is None and watch.stop(615).startswith('stage0_higher_shift')
    watch.observe(5, 'NATIVE3D_LOCAL_SHIFT rank=5 stage=0 direction=higher seconds=.25\n', 615)
    assert watch.stop(615) is None and watch.stop(5400) == 'event_wall_limit'
    data = {'core_rows': [{'fields': {'stage': '0', 'pass': str(i), 'applicable': '1', 'seconds': '89.9'}}
                          for i in (1, 2)],
            'shift_rows': [{'fields': {'stage': '0', 'direction': direction, 'seconds': '.1'}}
                           for direction in ('lower', 'higher')]}
    on = {'rank_data': dict.fromkeys(RANKS, data)}
    gate = stage0(on)
    assert not gate['failed_ranks'] and gate['ranks'][0]['inclusive_core_seconds'] == '180.0'
    data['shift_rows'][1]['fields']['seconds'] = '.10000000001'
    assert stage0(on)['failed_ranks'] == sorted(RANKS)
    data['shift_rows'][1]['fields']['stage'] = '1'
    assert stage0(on)['ranks'][0]['inclusive_core_seconds'] is None


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--verify-only', action='store_true')
    group.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        print('Replay parsing and timing stop self-checks passed.')
    elif args.build:
        outcome = run(args.build.resolve(), args.verify_only)
        print(json.dumps(outcome, indent=2))
        raise SystemExit(not outcome.get('verified' if args.verify_only else 'proceed', False))
    else:
        parser.error('supply the H6 build directory or --self-check')
