"""Recover FP2-M1 build666 N1 from archived evidence without starting a workload."""

from datetime import datetime
import json
from pathlib import Path
import re
import subprocess
from unittest.mock import patch

import report_m1 as report
import run_m1 as runner

HERE, a, cut, lab = runner.HERE, runner.a, runner.cut, runner.lab


def recover():
    name = 'N1'
    build = HERE / 'build666'
    case = runner.case_path(666, name)
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    receipt, binary, build_pins = runner.prepare(build)
    fingerprints = {key: receipt[key] for key in lab.FINGERPRINTS}
    pins = cut.read(case / 'pins.json')
    provenance = cut.read(case / 'provenance.json')
    assert pins == provenance['pins'] and all(pins[key] == value for key, value in build_pins.items())
    assert all(provenance[key] == value for key, value in fingerprints.items())
    for path, digest in pins.items():
        assert a.sha(Path(path)) == digest, path
    for mode in (*runner.OFF, 'fixture'):
        path = runner.fixture_result(666, mode)
        fixture = cut.read(path)
        assert fixture['status'] == 'PASS' and pins[str(path)] == a.sha(path)
        assert all(fixture[key] == value for key, value in fingerprints.items())
        for path, digest in fixture['attempts'].items():
            attempt = Path(path)
            assert a.sha(attempt) == digest
            lab.archived.verify_artifacts(attempt.parent, cut.read(attempt)['artifacts'])

    old_path, old = runner.baseline(name)
    assert pins[str(old_path)] == a.sha(old_path)
    assert pins[str(runner.BASE_PROOF)] == a.sha(runner.BASE_PROOF)
    command = cut.read(case / 'command.json')
    seconds_limit = runner.TIMEOUTS[name]
    selector = runner.inherited.selector(name)
    assert all(row['ranks'] == 1 and row['selector'] == selector and row['seconds_limit'] == seconds_limit
               for row in (command, provenance))
    assert command['binary'] == str(binary) and command['sensor'] in pins
    with patch.object(lab, 'SECONDS', seconds_limit), patch.object(lab, 'selector', runner.inherited.selector):
        assert command['argv'] == runner.command(case, binary, case / 'run.cfg', Path(command['sensor']), 1, True)

    missing = ['test_wait.json', 'owned_processes.json']
    assert all(not (case / filename).exists() for filename in missing)
    launch = cut.read(case / 'test_launch.json')
    samples = [json.loads(line) for line in (case / 'resources.jsonl').read_text().splitlines()]
    pids = {launch[key]['pid'] for key in ('controller', 'supervisor')}
    pids.update(pid for sample in samples for pid in sample['live_pids'])
    assert all(not Path(f'/proc/{pid}').exists() for pid in pids), pids
    processes = subprocess.check_output(['ps', '-e', '-o', 'pid=,stat=,comm='], text=True)
    job_names = {'SU2_CFD', 'test_driver', 'ninja', 'cc1plus', 'mpirun', 'mpiexec', 'orted'}
    assert not [line for line in processes.splitlines() if line.split()[-1] in job_names
                and not line.split()[1].startswith('Z')]

    time_path = case / 'time.txt'
    time_text = time_path.read_text()
    assert re.findall(r'^\s*Exit status: (\d+)$', time_text, re.MULTILINE) == ['0']
    stamp, = re.findall(r'^\s*Elapsed \(wall clock\) time \(h:mm:ss or m:ss\): ([0-9:.]+)$',
                       time_text, re.MULTILINE)
    elapsed = sum(float(part) * 60**i for i, part in enumerate(reversed(stamp.split(':'))))
    assert 0 < elapsed < seconds_limit
    log = case / 'test_outer.log'
    catch = a.summarize_box([log])
    assert catch['passed'] and catch['catch_summaries'] == [[('37', '1')]]
    catch['catch_summaries'] = [[list(item) for item in group] for group in catch['catch_summaries']]
    assert not runner.log_problems(case, 1)
    with patch.object(lab, 'LINE', runner.LINE):
        rows = lab.parse_rows(log)
    measurement = runner.inherited.evaluate(name, {0: rows}, {}, cut.read(case / 'test_host_preflight.json')['loadavg'])
    assert not (measurement['problems'] or measurement['failed_limits'] or measurement.get('first_round_difference'))
    rows_by_rank = {'0': rows}
    observer = runner.validate_observer(name, rows_by_rank, measurement['proof'])
    assert observer['status'] == 'PASS', observer
    differences = runner.comparison.repeat_differences(name, old['rows_by_rank'], rows_by_rank)
    assert not differences, differences[:1]

    sampled_peak = max(sample['sampled_peak_lower_bound_bytes'] for sample in samples)
    timed_peak = a.peak_kib(time_path) * 1024
    peak = max(sampled_peak, timed_peak)
    assert 0 < peak <= a.MEMORY * 1024
    evidence = {
        'method': 'completed_child_log_and_gnu_time',
        'absent_supervisor_receipts': missing, 'actual_host_proc': True,
        'recorded_pids_absent_on_host': sorted(pids), 'process_scan_at': datetime.now().astimezone().isoformat(),
        'last_sample_elapsed_seconds': samples[-1]['elapsed_seconds'],
        'last_sample_mtime': datetime.fromtimestamp((case / 'resources.jsonl').stat().st_mtime).astimezone().isoformat(),
        'time_receipt_mtime': datetime.fromtimestamp(time_path.stat().st_mtime).astimezone().isoformat(),
        'sampled_peak_through_last_sample_bytes': sampled_peak, 'timed_serial_process_peak_rss_bytes': timed_peak,
        'full_job_peak_available': False, 'pin_count': len(pins), 'repeat_launched': False,
    }
    names = (*a.ARTIFACTS, 'pins.json', 'provenance.json', 'test_launch.json', 'resources.jsonl', log.name, time_path.name)
    artifacts = {filename: a.sha(case / filename) for filename in sorted(set(names)) if (case / filename).is_file()}
    result = {**fingerprints, **catch, 'passed': True, 'case': name, 'ranks': 1,
              'wait': {'terminal': True, 'returncode': 0, 'elapsed_seconds': elapsed,
                       'source': 'time.txt and retrospective actual-host process check'},
              'supervisor_stop': 'unavailable', 'peak_lower_bound_bytes': peak, 'measurement': measurement,
              'rows_by_rank': rows_by_rank, 'artifacts': artifacts, 'recovery': evidence}
    path = case / 'measurement.json'
    a.sealed(path, result)
    identity = {'passed': True, 'baseline': str(old_path), 'baseline_sha256': a.sha(old_path),
                'first_difference': None, 'differences': []}
    a.sealed(case / 'case_summary.json', {'name': name, 'build': 666, 'status': 'PASS', 'observer': observer,
             'identity': identity, 'first_difference': None, 'watchdog': {}, 'correction_of': None,
             'used': str(path), 'attempts': {str(path): a.sha(path)}, 'recovery': evidence})
    return evidence


def write_report(evidence):
    original_text = report.text

    def annotated(data):
        note = (
            'The N1 child exited 0 at 18:18:22 local time after 2,238.47 s and printed a complete Catch result. '
            'Supervisor sampling ended at 18:08:58, leaving test_wait.json and owned_processes.json absent. '
            'The continuation order authorized reuse after pin and identity checks. All 40 trial pins verified, '
            'all recorded PIDs were absent on the host, and the inherited gates and exact projection passed. '
            f'The last sampling window reached {evidence["sampled_peak_through_last_sample_bytes"]:,} bytes. '
            f'GNU time recorded a serial maximum RSS of {evidence["timed_serial_process_peak_rss_bytes"]:,} bytes '
            'and supplies the reported job peak lower bound. Full-job peak telemetry for the last 564 s is unavailable. '
            'The original logs and partial samples were retained. N1 ran once. '
            '[Recovery evidence](../cases/build666/N1/case_summary.json)', '',
            'The build665 fixture launcher imported a historical adapter and exited before starting a child. '
            'Loading the intended C3 module by absolute path fixed the launcher. Build666 sealed the correction '
            'with identical executable hashes. [Source check](../source_check.md) · [Progress log](../../progress.md)', '',
        )
        text = original_text(data).rstrip()
        return text.replace('## N6P\n', '\n'.join((*note, '## N6P\n')), 1)

    with patch.object(report, 'text', annotated):
        return report.report(HERE / 'build666')


if __name__ == '__main__':
    case = runner.case_path(666, 'N1') / 'case_summary.json'
    evidence = cut.read(case)['recovery'] if case.is_file() else recover()
    print(write_report(evidence)['label'])
