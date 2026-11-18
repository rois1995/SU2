"""Summarize the executed G5 gates and final exclusive phase clocks."""
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sys


def read(path):
    return json.loads(path.read_text())


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def stage_clocks(records):
    """Use each rank's final exclusive ledger once."""
    per_rank = []
    for row in records:
        stages = defaultdict(lambda: {'seconds': 0.0, 'calls': 0})
        for record in row['ledger']['rows']:
            phase = stages[record['phase']]
            phase['seconds'] += float(record['exclusive_seconds'])
            phase['calls'] += int(record['calls'])
        per_rank.append(stages)
    summary = {}
    for phase in sorted(set().union(*(row.keys() for row in per_rank))):
        rows = [row[phase] for row in per_rank]
        summary[phase] = {
            'seconds_min': min(row['seconds'] for row in rows),
            'seconds_max': max(row['seconds'] for row in rows),
            'timer_calls_sum': sum(row['calls'] for row in rows),
        }
    return summary


def collect_events(case):
    full = read(case / 'full_workflow_receipt.json')
    assert full['workflow_accepted'] and not full['errors']
    assert full['P7_P8_events_checked'] == full['F3b_timer_events_checked'] == [1, 2]
    events = []
    for event in full['events']:
        number = event['event']
        timed = read(case / f'f3b_event{number}_timer_gate.json')
        connected = read(case / f'p7p8_event{number}_gate.json')
        assert timed['passed'] and connected['passed'] and all(connected['gates'].values())
        assert len(timed['ranks']) == 6
        cover = [float(row['ledger']['summary']['coverage']) for row in timed['ranks']]
        join = [float(row['join_relative_difference']) for row in timed['ranks']]
        assert min(cover) >= 0.95 and max(join) <= 0.01
        assert all(row['ledger']['summary']['fixedstorage']=='4120' for row in timed['ranks'])
        counts = event['certification_six_rank_sums']
        assert event['certification_complete']
        assert all(counts[name] == 0 for name in ('q_bad','j_bad','size_bad','geometry_bad'))
        proof = connected['proof']
        assert set(proof['surface_faces_by_rank']) == {proof['owned_faces_sum']}
        memory = connected['memory']
        assert memory['job_peak_rss_bytes'] == memory['process_peak_sum_bytes'] <= 19_999_490_048
        assert event['whole_event_seconds_max_rank'] <= 5400
        events.append({
            'event': number, 'native_seconds': event['whole_event_seconds_max_rank'],
            'remesh_detail_seconds': event['refine_detail_seconds_max_rank'],
            'job_peak_rss_bytes': memory['job_peak_rss_bytes'], 'process_peak_sum_bytes': memory['process_peak_sum_bytes'],
            'minimum_timer_coverage': min(cover), 'maximum_join_relative_difference': max(join),
            'proof_faces': proof['owned_faces_sum'], 'actual_mesh_counts': event['actual_mesh_counts'],
            'certification_six_rank_sums': counts, 'coordinate_maximum_error': event['coordinate_maximum_error'],
            'positive_solution': event['positive_solution'],
            'maximum_transfer_residual': max(map(abs,event['transfer_residuals'].values())),
            'continued_cfd_row': event['continued_cfd_row'],
            'phase_exclusive_clocks': stage_clocks(timed['ranks']),
            'global_work_by_rank': [int(row['global_work']) for row in event['refine_details']],
            'unattributed_seconds_max': max(float(row['ledger']['summary']['unattributed']) for row in timed['ranks']),
        })
    assert [row['event'] for row in events] == [1,2]
    return events


def qualify(case, restart, expected, output):
    identity = read(case / 'n8_payload_identity.json')
    assert identity['passed'] and identity['workflow_gates_passed'] and not identity['mismatches']
    references = read(expected)['hashes']
    assert len(references)==6 and identity['hashes']==references
    assert all(sha(case / name)==digest for name,digest in references.items())
    status = read(case / 'status.json')
    assert status['state']=='terminal' and status['exit_code']==0 and status['all_owned_handles_terminal']
    assert status['source_and_binary_pins_unchanged'] and status['inputs_unchanged']
    assert status['full_workflow_accepted']
    exact = read(case / 'strict_full_identity.json')
    assert exact['passed'] and exact['integer_rows']['passed'] and exact['timer_rows']['passed']
    reopened = read(restart / 'disk_restart_validation.json')
    assert reopened['disk_restart_verified'] and reopened['all_inputs_and_pins_unchanged']
    assert read(restart / 'post_terminal_handles.json')['all_owned_handles_terminal']
    events = collect_events(case)
    result = {
        'passed': True, 'seconds_total': status['seconds_total'], 'events': events,
        'six_reference_sha256': references, 'old_rows_exact': exact['passed'],
        'timer_snapshots_exact': exact['timer_rows']['timer_snapshots'],
        'isolated_restart': {key: reopened[key] for key in ('seconds','adapted_points','adapted_cells',
                                                           'coordinate_maximum_error','continued_cfd')},
    }
    with output.open('x') as file:
        json.dump(result,file,indent=2)
        file.write('\n')
    print(json.dumps(result))


if __name__ == '__main__':
    qualify(*map(Path,sys.argv[1:]))
