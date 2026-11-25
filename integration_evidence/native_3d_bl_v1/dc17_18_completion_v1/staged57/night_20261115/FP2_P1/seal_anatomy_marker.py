"""Rejoin ANATOMY stdout when the separately collected startup marker interrupts a row."""
from argparse import ArgumentParser
from pathlib import Path
from unittest.mock import patch

import run_p1 as p


def seal(build):
    mode = 'anatomy-N1'
    summary = p.selected_summary(build, mode)
    receipt = p.cut.read(p.HERE / f'build{build}' / 'build_status.json')
    row = p.verify_summary(receipt, mode, summary)
    if 'mixed_marker_seal' in row:
        return row
    source = Path(row['used'])
    case = source.parent
    raw = p.cut.read(source)
    assert raw['case'] == 'N1' and raw['ranks'] == 1 and raw['passed']
    assert raw['wait']['returncode'] == 0 and raw['supervisor_stop'] is None
    assert all(p.cut.terminal(handle) for handle in p.cut.read(case / 'owned_processes.json')['identities'])
    stdout = case / 'test_outer.log'
    content = stdout.read_bytes()
    marker = (p.MARKER + '\n').encode()
    assert content.count(marker) == 1 and row['switches']['log_pins'][str(stdout)] == p.cut.sha(stdout)
    start = content.index(marker)
    assert start > 0 and content[start - 1] != 10
    derived = case / 'stdout_without_interleaved_marker.log'
    normalized = content[:start] + content[start + len(marker):]
    if derived.is_file():
        assert derived.read_bytes() == normalized
    else:
        derived.write_bytes(normalized)
    with patch.object(p.lab, 'LINE', p.m2.LINE):
        joined = p.lab.parse_rows(derived)
    host = p.cut.read(case / 'test_host_preflight.json')
    assert p.inherited.evaluate('N1', {0: joined}, {}, host['loadavg']) == raw['measurement']
    catch = p.a.summarize_box([derived])
    assert [[list(result) for result in group] for group in catch['catch_summaries']] == raw['catch_summaries']
    measurement = {**raw, 'rows_by_rank': {'0': joined}, 'stdout_rejoined_sha256': p.cut.sha(derived)}
    first = p.cut.read(p.selected_summary(build, 'campaign-N1-1'))
    assert first['status'] == 'passed'
    reference = Path(first['used'])
    exact = p.identity('N1', (reference, p.cut.read(reference)), measurement)
    resolved = case / 'measurement_stdout_rejoined.json'
    p.a.sealed(resolved, measurement)
    archive = summary.with_name('case_summary_before_mixed_marker.json')
    if archive.exists():
        assert archive.read_bytes() == summary.read_bytes()
    else:
        archive.write_bytes(summary.read_bytes())
    row['switches'].update(rank_marker_counts={'0': 1}, ranks_with_no_marker=[], first_failure=None, passed=True)
    result_status = 'passed' if exact['status'] == 'equal' else 'FAILED'
    observed = row['observer']
    ok = result_status == 'passed' and observed['completion'] == 'complete'
    first_difference = (exact['first_difference'] if result_status == 'FAILED' else None if ok else
                        {'field': 'anatomy completion', 'observed': observed['completion'], 'reason': observed.get('reason')})
    row.update(used=str(resolved), identity=exact, result_status=result_status, status='passed' if ok else 'FAILED',
               first_difference=first_difference,
               mixed_marker_seal={'marker_byte_offset': start, 'original_summary': str(archive),
                                  'raw_log': str(stdout), 'normalized_log': str(derived)})
    row['attempts'][str(resolved)] = p.cut.sha(resolved)
    row['pins'].update(p.hashes((archive, source, derived, resolved, reference, Path(__file__))))
    pending = summary.with_suffix('.tmp')
    p.cut.save(pending, row)
    pending.replace(summary)
    return row


if __name__ == '__main__':
    args = ArgumentParser(description=__doc__)
    args.add_argument('build', type=int)
    parsed = args.parse_args()
    result = seal(parsed.build)
    print('ANATOMY', result['status'], result['identity']['status'], result['observer']['completion'])
