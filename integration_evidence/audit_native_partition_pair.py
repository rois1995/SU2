"""Compare two audited actual RAE runs without treating different meshes as fixed work."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import numpy as np
from audit_native_unsteady import capcheck, state


def total_adaptation(costs):
    # transfer is nested in replace; never add it a second time.
    return sum(costs[key] for key in ('metric', 'remesh', 'replace', 'adapted_output'))


def window_costs(log):
    rows = []
    for line in log.splitlines():
        if not re.match(r'^\|\s*\d+\|\s*\d+-\d+\|', line):
            continue
        values = [value.strip() for value in line.split('|')[1:-1]]
        assert len(values) == 12, values
        costs = {key: None if value == '-' else float(value)
                 for key, value in zip(('CFD', 'metric', 'remesh', 'replace', 'transfer', 'adapted_output'), values[3:9])}
        rows.append(dict(cycle=int(values[0]), steps=values[1], points=int(values[2]), phase_seconds=costs,
                         applied_adaptation_seconds=total_adaptation(costs) if costs['remesh'] is not None else None))
    assert rows, 'Missing per-window summary'
    return rows


def inspect(left, right):
    cases = [left.resolve(), right.resolve()]
    configs = [(case / 'run.cfg').read_text() for case in cases]
    normalize = lambda cfg: re.sub(r'^ADAP_NATIVE_RANKS\s*=.*$', 'ADAP_NATIVE_RANKS=<varies>', cfg, flags=re.M)
    assert normalize(configs[0]) == normalize(configs[1]), 'Other configuration differs'
    runs = [json.loads((case / 'run_evidence.json').read_text()) for case in cases]
    for key in ('binary_sha256', 'source_evidence_sha256', 'mesh_sha256', 'ranks', 'environment'):
        assert runs[0][key] == runs[1][key], key
    assert all(run['phase'] == 'terminal' and run['solver_exit'] == 0 for run in runs)
    audits = [json.loads((case / 'independent_rae_unsteady_audit.json').read_text()) for case in cases]
    assert all(audit['status'] == 'PASS' for audit in audits)
    for case in cases:
        accounting = json.loads((case / 'independent_profile_accounting.json').read_text())
        assert accounting['status'] == 'PASS', accounting
    freq = int(re.search(r'^ADAP_FREQ\s*=\s*(\d+)', configs[0], re.M)[1])
    mesh = capcheck.read_su2(cases[0] / 'input.su2')
    original = []
    for case in cases:
        path = case / f'flow_{freq-1:05d}.vtu'
        values, _ = state(mesh, path)
        metric, precision = capcheck.metric_of(mesh, path)
        assert precision == 'Float64'
        original.append((values, metric))
    equality = dict(first_donor_conserved_states_exact=np.array_equal(original[0][0], original[1][0]),
                    first_donor_metric_exact=np.array_equal(original[0][1], original[1][1]),
                    first_donor_max_state_difference=float(np.max(np.abs(original[0][0]-original[1][0]))),
                    first_donor_max_metric_difference=float(np.max(np.abs(original[0][1]-original[1][1]))))
    assert equality['first_donor_conserved_states_exact'] and equality['first_donor_metric_exact'], equality
    results = []
    for case, cfg, run, audit in zip(cases, configs, runs, audits):
        history = [dict(first_step=window['first_step'], donor_step=row['step'],
                        raw_domain_integral_defect=row['relative_integral_defect'],
                        closed_policy_residual=row['closed_policy_integral_residual'])
                   for window in audit['windows'] for row in window['history']]
        assert all(row['closed_policy_residual'] is not None and row['closed_policy_residual'] < 1e-10 for row in history)
        costs = audit['phase_seconds']
        windows = window_costs((case / 'solver.log').read_text())
        assert abs(sum(window['phase_seconds']['metric'] for window in windows)-costs['metric']) <= 2e-5*max(1,costs['metric'])
        contention_path = case / 'contention_receipt.json'
        qualification = json.loads(contention_path.read_text())['status'] if contention_path.exists() else 'SINGLE_TRAJECTORY_NO_SPEEDUP_CLAIM'
        samples = run['machine_samples']
        active = [sample for sample in samples if 'elapsed_seconds' in sample]
        results.append(dict(case=str(case), adaptation_ranks=int(re.search(r'^ADAP_NATIVE_RANKS\s*=\s*(\d+)', cfg, re.M)[1]),
                            whole_seconds=run['elapsed_seconds'], phase_seconds=costs, window_costs=windows,
                            applied_windows_adaptation_seconds=sum(window['applied_adaptation_seconds'] or 0 for window in windows),
                            metric_in_windows_without_remeshing_seconds=sum(window['phase_seconds']['metric'] for window in windows if window['applied_adaptation_seconds'] is None),
                            total_adaptation_seconds=total_adaptation(costs), adaptation_over_CFD=audit['adaptation_over_CFD'],
                            points=[window['points'] for window in audit['windows']],
                            history_checks=history, maximum_closed_policy_residual=max(row['closed_policy_residual'] for row in history),
                            peak_active_cpu_pressure_avg10=max(sample['cpu_pressure_avg10'] for sample in active),
                            timing_qualification=qualification,
                            scope='Both saved donor histories at each event independently checked; CLOSED-policy conservation includes independently measured open FARFIELD area change.'))
    comparison = dict(left_relative_to_right_applied_windows_change=results[0]['applied_windows_adaptation_seconds']/results[1]['applied_windows_adaptation_seconds']-1,
                      left_relative_to_right_total_adaptation_change=results[0]['total_adaptation_seconds']/results[1]['total_adaptation_seconds']-1,
                      left_relative_to_right_remesh_change=results[0]['phase_seconds']['remesh']/results[1]['phase_seconds']['remesh']-1,
                      left_relative_to_right_whole_change=results[0]['whole_seconds']/results[1]['whole_seconds']-1)
    return dict(status='PASS_MATCHED_FIRST_TARGET_AND_INDEPENDENT_GATES', first_target=equality,
                results=results, comparison=comparison,
                scope='One run per mode. Different remeshing operations, meshes and subsequent CFD trajectories. Host contention recorded, not controlled away. No fixed-work speedup or recommended-worker-count claim.',
                checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())


if __name__ == '__main__':
    assert total_adaptation(dict(metric=1, remesh=2, replace=3, transfer=99, adapted_output=4)) == 10
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('left', type=Path); parser.add_argument('right', type=Path); parser.add_argument('output', type=Path)
    args = parser.parse_args()
    assert not args.output.exists(), 'Preserve previous comparison evidence'
    result = inspect(args.left, args.right)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(result['status'], result['comparison'])
