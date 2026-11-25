"""Recover an interleaved process marker and stdout projection from a terminal serial trial."""
from argparse import ArgumentParser
from pathlib import Path
from unittest.mock import patch

import run_p1 as p


def seal(build, mode):
    summary = p.selected_summary(build, mode)
    row = p.cut.read(summary)
    if 'mixed_marker_seal' in row:
        p.verify_pins(row['pins'])
        return row
    failure = row.get('first_difference')
    assert failure == {'rank': 0, 'field': 'switch marker count', 'observed': 0, 'expected': 1}, failure
    receipt = p.cut.read(p.HERE / f'build{build}' / 'build_status.json')
    p.verify_summary(receipt, mode, summary)
    original_measurement = Path(row['used'])
    case = original_measurement.parent
    raw = p.cut.read(original_measurement)
    assert raw['case'] == p.TRIALS[mode][1] == 'N1' and raw['ranks'] == 1
    assert raw['passed'] and raw['wait']['returncode'] == 0 and raw['supervisor_stop'] is None
    assert all(p.cut.terminal(handle) for handle in p.cut.read(case / 'owned_processes.json')['identities'])
    log = case / 'test_outer.log'
    content = log.read_bytes()
    marker = (p.MARKER + '\n').encode()
    assert content.count(marker) == 1 and row['switches']['log_pins'][str(log)] == p.cut.sha(log)
    offset = content.index(marker)
    assert offset > 0 and content[offset - 1] != 10, 'The marker was not interleaved with stdout'
    normalized = case / 'stdout_without_interleaved_marker.log'
    clean = content[:offset] + content[offset + len(marker):]
    if normalized.is_file():
        assert normalized.read_bytes() == clean
    else:
        normalized.write_bytes(clean)
    with patch.object(p.lab, 'LINE', p.m2.LINE):
        rows = p.lab.parse_rows(normalized)
    host = p.cut.read(case / 'test_host_preflight.json')
    rechecked = p.inherited.evaluate(raw['case'], {0: rows}, {}, host['loadavg'])
    assert rechecked == raw['measurement']
    catch = p.a.summarize_box([normalized])
    assert [[list(a) for a in group] for group in catch['catch_summaries']] == raw['catch_summaries']
    corrected = case / 'measurement_stdout_rejoined.json'
    content = {**raw, 'rows_by_rank': {'0': rows}, 'stdout_rejoined_sha256': p.cut.sha(normalized)}
    p.a.sealed(corrected, content)
    archive = summary.with_name('case_summary_before_mixed_marker.json')
    if archive.exists():
        assert archive.read_bytes() == summary.read_bytes()
    else:
        archive.write_bytes(summary.read_bytes())
    row['switches'].update(rank_marker_counts={'0': 1}, ranks_with_no_marker=[], first_failure=None, passed=True)
    row.update(status='passed', result_status='passed', first_difference=None, used=str(corrected),
               mixed_marker_seal={'marker_byte_offset': offset, 'original_summary': str(archive),
                                  'raw_log': str(log), 'normalized_log': str(normalized)})
    row['attempts'][str(corrected)] = p.cut.sha(corrected)
    row['pins'].update(p.hashes((archive, original_measurement, normalized, corrected, Path(__file__))))
    path = summary.with_suffix('.tmp')
    p.cut.save(path, row)
    path.replace(summary)
    return row


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('mode', choices=('campaign-N1-1', 'campaign-N1-2'))
    args = parser.parse_args()
    print(args.mode, seal(args.build, args.mode)['status'])
