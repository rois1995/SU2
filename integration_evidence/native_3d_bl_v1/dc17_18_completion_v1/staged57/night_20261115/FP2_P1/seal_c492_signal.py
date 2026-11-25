"""Classify the C492 terminal signal emitted by the recorded wrapper."""
import re
from pathlib import Path

from retry_c492_watchdog import shared, wrapper_timeout


def seal(label):
    cut = shared.cut
    base = shared.HERE / 'shared' / f'build{label}'
    group = base / 'closure1/exact-c492'
    path = group / 'case_summary.json'
    record = cut.read(path)
    run = record['subruns']['bl']
    if run['status'] == 'OPEN':
        return record
    assert run['first_failure'] == 'test exit'
    wrapper = group / 'watchdog_retry'
    case, = (wrapper / 'cases').iterdir()
    status = cut.read(case / 'status.json')
    command = cut.read(case / 'command.json')
    monitor = cut.read(case / 'safeguard_process_receipt.json')
    assert status['state'] == 'terminal' and status['exit_code'] == 124 and status['pins_verified']
    assert monitor['state'] == 'terminal' and not monitor['live_owned_descendants']
    assert all(cut.terminal(item) for item in monitor['identities'])
    assert run['wait']['terminal'] and cut.terminal(run['wait']['controller'])
    assert command['timeout_seconds'] == 600 and command['argv'][command['argv'].index('timeout') + 1] == '600'
    assert monitor['peak_sampled_tree_rss_bytes'] <= shared.LIMIT
    log = case / 'run.log'
    text = log.read_text()
    match, = re.finditer(r'FAILED:\n\s+\{Unknown expression after the reported line\}\n'
                         r'due to a fatal error condition:\n\s+SIGTERM - Termination request signal\n', text)
    prior = text[:match.start()]
    assert not re.search(r'FAILED:|Curved atlas (?:work|storage) exhausted', prior)
    assert 'Command exited with non-zero status 124' in text[match.end():] and 'Exit status: 124' in text
    assert not (case / 'actual_saved_surface_stage_rank0.json').is_file()
    stage, = (line for line in prior.splitlines() if 'NATIVE3D_STAGE ' in line and 'stage=exit ' in line)
    row = dict(re.findall(r'(\w+)=(\S+)', stage))
    assert int(row['upper_refinements']) == 0 and int(row['length_splits']) >= 300
    off, = (base / 'closure0/exact-c492/bl/cases').iterdir()
    name = 'stage_resumed_surface_rank0_manifest.json'
    baseline, candidate = [cut.read(root / name) for root in (off, case)]
    assert baseline['work']['stop'] == candidate['work']['stop'] == 'time_budget'
    assert baseline['policies']['maximum_seconds'] == candidate['policies']['maximum_seconds'] == 388.8
    first = wrapper_timeout(group / 'bl')
    assert first is not None
    archive = group / 'case_summary_before_signal_seal.json'
    if archive.exists():
        assert archive.read_bytes() == path.read_bytes()
    else:
        archive.write_bytes(path.read_bytes())
    second = {'seconds': 600, 'exit_code': 124, 'status': str(case / 'status.json'), 'log': str(log),
              'sigterm_from_wrapper': True, 'stage_splits_before_cutoff': int(row['length_splits']),
              'stage_upper_refinements': int(row['upper_refinements'])}
    run['watchdog'].update(second=second, raw_signal_classification=str(archive))
    run.update(status='OPEN', first_failure='wrapper-only timeout')
    record['status'] = 'OPEN'
    files = [archive, log, off / name, case / name, Path(__file__)]
    record['pins'].update({str(f): cut.sha(f) for f in files})
    pending = path.with_suffix('.tmp')
    cut.save(pending, record)
    pending.replace(path)
    return record


if __name__ == '__main__':
    import sys

    result = seal(int(sys.argv[1]))
    print('Exact C492', result['status'], result['subruns']['bl']['watchdog']['second'])
