"""Apply the procfs KiB conversion to a terminal first-collapse receipt."""
from argparse import ArgumentParser
from pathlib import Path
import json

from run_shared import HERE, LIMIT, cut


def seal(build, closure):
    group = HERE / 'shared' / f'build{build}' / f'closure{closure}' / 'first-collapse'
    path = group / 'case_summary.json'
    raw = group / 'first-collapse/case/host_resources.jsonl'
    record = cut.read(path)
    result = record['subruns']['first-collapse']
    witness = result['switch_receipt']
    history = group / 'case_summary_before_rss_units.json'
    if 'rss_units_seal' in result:
        assert cut.sha(history) == result['rss_units_seal']['original_sha256']
        assert cut.sha(raw) == result['rss_units_seal']['raw_sha256']
        return record
    assert result['wait']['terminal'] and cut.terminal(result['wait']['controller'])
    kib = witness['observed_solver_peak_sum_bytes']
    assert kib > 0
    with raw.open() as stream:
        maximum = {}
        for line in stream:
            for process in json.loads(line)['owned_processes']:
                if process['comm'] != 'SU2_CFD':
                    continue
                handle = process['pid'], process['start_ticks']
                maximum[handle] = max(maximum.get(handle, 0), process.get('VmHWM', 0), process.get('VmRSS', 0))
    assert len(maximum) == 6 and sum(maximum.values()) == kib
    if history.is_file():
        assert history.read_bytes() == path.read_bytes()
    else:
        history.write_bytes(path.read_bytes())
    witness['observed_solver_peak_sum_bytes'] = kib * 1024
    witness['observed_solver_peak_by_pid_bytes'] = {str(pid): value * 1024 for (pid, _), value in maximum.items()}
    result['rss_units_seal'] = {'original_sha256': cut.sha(history), 'raw_sha256': cut.sha(raw),
                                'observed_procfs_peak_sum_kib': kib}
    record['pins'].update({str(source): cut.sha(source) for source in (history, raw, Path(__file__))})
    if witness['observed_solver_peak_sum_bytes'] > LIMIT:
        result.update(status='FAILED', first_failure='observed solver peak sum')
        record['status'] = 'FAILED'
    updated = path.with_suffix('.tmp')
    cut.save(updated, record)
    updated.replace(path)
    return record


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('closure', type=int, choices=(0, 1))
    args = parser.parse_args()
    report = seal(args.build, args.closure)
    print('First collapse', report['status'],
          report['subruns']['first-collapse']['switch_receipt']['observed_solver_peak_sum_bytes'], 'bytes')
