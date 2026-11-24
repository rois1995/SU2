"""Report FP2-C3 S beside the FP2-D build660 S workflow."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import runpy
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
spec = spec_from_file_location('c2_surface_g5', HERE / 'run_fp2_b_g5.py')
runner = module_from_spec(spec)
spec.loader.exec_module(runner)

spec = spec_from_file_location('c1_surface_report', runner.C1 / 'report_s.py')
prior = module_from_spec(spec)
with patch.dict(sys.modules, run_fp2_b_g5=runner.prior):
    spec.loader.exec_module(prior)
cut, sha, workflow = runner.cut, runner.sha, runner.workflow
table = prior.table
SURFACE = 'NATIVE3D_LOCAL_SURFACE'
SURFACE_STATE = 'NATIVE3D_LOCAL_SURFACE_STATE'
SURFACE_BUDGET = 'NATIVE3D_LOCAL_SURFACE_BUDGET'
from run_c2_lab import (C3_METRIC_FIELDS, CONTACT, CONTACT_CLOCKS, CONTACT_COUNTS, CONTACT_COUNTERS,
                        CONTACT_TIMES, D_FIELDS, NOT_AVAILABLE, first_short_checkpoint)
SHORT = 'NATIVE3D_LOCAL_SHORT'
COLLAPSE_FIELDS = ('splits_distance', 'sweeps_distance', 'path_max', 'path_splits', 'waits_neighbor',
                   'left_distance', *D_FIELDS, 'collapses_surface', 'c_sweeps', 'c_candidates', 'c_deferred', 'c_reverse_max',
                   'seconds_c', 'seconds_c_samples', 'seconds_c_entity', 'seconds_c_bound', 'c_bound_queries',
                   'faces_before_c', 'points_before_c', *CONTACT_COUNTERS, *CONTACT_CLOCKS)
REASONS = ('protected', 'border', 'ineligible', 'border_keeper', 'feature_chord', 'link', 'near', 'outward',
           'reverse', 'topology', 'validity', 'size', 'shape', 'distance', 'entity', 'bound', 'contact_remote', 'contact')
REFUSALS = tuple('c_refused_' + reason for reason in REASONS)
BUDGET_FIELDS = ('overlay_peak', 'pass_admitted_bytes', 'query_scratch_max', 'conservative_sum', 'ceiling')
S0_HISTORY = ('S0 is FP2-D at 0c6860fd3a. Its sealed build660 S workflow provides the comparison. '
              '[S0 report](../../FP2_D/reports/FP2_D_660.md)')
BUILD_663_HISTORY = ('Build663 failed N6P trial 1 at rank 0 in pass 2. Step C created a physical edge with '
                     'metric length 0.0090487726. N1 and N6 passed both trials, and N6P trial 2 was gated out. '
                     '[Failed lab report](FP2_C3_663.md)')
CONTACT_COLUMNS = ('remote_rows', 'remote_bytes', 'owner_rows', 'owner_bytes', 'peak_bytes', 'verify_cells',
                   'verify_candidates', 'verify_calls', 'seconds_gather', 'seconds_verify')
QUALITY = ('q_measured', 'q_bad', 'q_debt', 'j_measured', 'j_bad', 'j_debt')
C_CLOCK = ('Core surface collapse step', SURFACE, ('seconds_c',))
RANK_TIMING = prior.rank_timing


def report_work(label):
    work = runner.work_path(label)
    retry = work / runner.FULL_RETRY
    assert not all((path / 'run_receipt.json').is_file() for path in (work, retry))
    return retry if retry.is_dir() else work


def short_observations(surfaces, short_rows, core=()):
    def census(stage, phase, checkpoint):
        rows = [row for row in short_rows if row.get('stage') == stage and row.get('pass') == phase
                and row.get('checkpoint') == checkpoint]
        count = next((row['lt_1e_2'] for row in rows if 'lt_1e_2' in row), NOT_AVAILABLE)
        smallest = next((row for row in rows if row.get('order') == '1'), NOT_AVAILABLE)
        return {'lt_1e_2': count, 'smallest': smallest}

    result = {}
    stages = {row['stage'] for row in (*surfaces, *short_rows, *core)}
    for stage in sorted(stages, key=int):
        fields = {row['pass']: row for row in surfaces if row['stage'] == stage}
        phases = fields.keys() | {row['pass'] for row in short_rows
                                  if row['stage'] == stage and row['checkpoint'] == 'c1'}
        phases |= {row['pass'] for row in core if row['stage'] == stage and row['pass'] != '0'}
        passes, checkpoints = {}, []
        for phase in sorted(phases, key=int):
            current = {key: fields.get(phase, {}).get(key, NOT_AVAILABLE) for key in C3_METRIC_FIELDS}
            census_c1 = census(stage, phase, 'c1')
            passes[phase] = {**current, 'c1': census_c1, 'out': NOT_AVAILABLE}
            checkpoints.extend((f'pass {phase} {key}', current[key]) for key in C3_METRIC_FIELDS[1:])
            checkpoints.append((f'pass {phase} c1', census_c1['lt_1e_2']))
        shifted = census(stage, '0', 'shifted')
        checkpoints.append(('pass 0 shifted', shifted['lt_1e_2']))
        result[stage] = {'passes': passes, 'shifted': shifted,
                         'first_short_checkpoint': first_short_checkpoint(checkpoints)}
    return result


def short_census_columns(row):
    smallest = row['smallest']
    return (row['lt_1e_2'], *(smallest.get(key, NOT_AVAILABLE) if isinstance(smallest, dict) else NOT_AVAILABLE
                               for key in ('metric', 'space', 'a', 'b')))


def collect(path, parse, candidate=False):
    run = prior.collect(path)
    case = Path(cut.read(path)['case'])
    connected = cut.read(case / 'full_workflow_receipt.json')
    for event in run['events']:
        number = event['summary']['event']
        _, local, _, _ = parse['read_event'](case, number)
        workflow_event, = (row for row in connected['events'] if row['event'] == number)
        event['slowest_rank'] = int(min(workflow_event['native_summaries'],
                                        key=lambda row: (-float(row['total']), int(row['rank'])))['rank'])
        for rank, lines in sorted(local.items()):
            rows = event['ranks'][str(rank)]
            for key, marker in (('surface', SURFACE), ('surface_state', SURFACE_STATE),
                                ('surface_budget', SURFACE_BUDGET), ('contact', CONTACT), ('short', SHORT)):
                rows[key] = [row for line in lines if (row := parse['fields'](line, marker))
                             and row['rank'] == str(rank) and row.get('applicable') != '0']
            rows['hand_over_metric'] = short_observations(rows['surface'], rows['short'], rows['core_rows'])
            rows['clearance'] = [row for line in lines if (row := parse['fields'](line, 'NATIVE3D_TIMER'))
                                 and row['rank'] == str(rank) and row.get('root') == 'remesh'
                                 and row.get('operation') == 'clearance' and row.get('boundary') == 'final']
            if candidate:
                expected = sorted((row['stage'], row['pass'], kind)
                                  for row in rows['surface'] for kind in CONTACT_COUNTS)
                seen = sorted((row['stage'], row['pass'], row['kind']) for row in rows['contact'])
                if expected != seen:
                    event['missing_observations'].append({'rank': rank, 'row': CONTACT,
                                                          'expected': expected, 'observed': seen})
                for row in rows['contact']:
                    kind = row['kind']
                    if missing := set((*CONTACT_COUNTS.get(kind, ()), *CONTACT_TIMES.get(kind, ()))) - row.keys():
                        event['missing_observations'].append({'rank': rank, 'row': CONTACT, 'fields': sorted(missing)})
            for row in rows['surface']:
                if candidate:
                    missing = set((*COLLAPSE_FIELDS, *REFUSALS)) - row.keys()
                    if missing:
                        event['missing_observations'].append({'rank': rank, 'row': SURFACE,
                                                              'fields': sorted(missing)})
            if not rows['surface'] or not rows['surface_state']:
                event['missing_observations'].append({'rank': rank, 'row': 'surface or surface-state'})
            if candidate:
                for row in rows['surface_budget']:
                    if missing := set(BUDGET_FIELDS) - row.keys():
                        event['missing_observations'].append({'rank': rank, 'row': SURFACE_BUDGET, 'fields': sorted(missing)})
        rank0 = event['ranks']['0']
        if rank0['surface_state'] and rank0['engine_length_collapse']:
            before = int(rank0['surface_state'][0]['faces'])
            after = int(rank0['engine_length_collapse'][0]['exit']['working_faces'])
            difference = before - after
            event['engine_surface_removal'] = {'seed_faces': before, 'exit_faces': after,
                                               'removed': difference // 2 if difference >= 0 and difference % 2 == 0
                                               else None}
        else:
            event['engine_surface_removal'] = {}
    return run


def rank_timing(local, stages, ledger, native):
    # C1 has no step C clock. Its contribution to the common table is zero.
    local = {**local, SURFACE: [{'seconds_c': '0', **row} for row in local[SURFACE]]}
    return RANK_TIMING(local, stages, ledger, native)


def timing_table(cases):
    clocks = (*prior.CORE_CLOCKS[:2], C_CLOCK, *prior.CORE_CLOCKS[2:])
    headings = (*(name for name, _, _ in clocks), 'Engine construction', 'Engine quality')
    with patch.multiple(prior, CORE_CLOCKS=clocks, TIMING_HEAD=headings, rank_timing=rank_timing):
        markdown = prior.continuation_markdown(cases)
    heading = '## Where the time is'
    return markdown[markdown.index(heading):]


def watchdog_observations(work):
    names = ('full_watchdog.json', 'restart_watchdog.json', 'restart_retry_timing.json')
    parent = work.parent if work.name == runner.FULL_RETRY else work
    folders = (parent, work) if work != parent else (work,)
    records = {str(path.relative_to(parent)): cut.read(path)
               for folder in folders for name in names if (path := folder / name).is_file()}
    archive = work / 'restart_attempt1_watchdog'
    if (archive / 'manifest.json').is_file():
        manifest = cut.read(archive / 'manifest.json')
        for name, row in manifest['files'].items():
            assert row['old_path'] == str(work / name) and sha(archive / name) == row['sha256'], name
        records[str(archive.relative_to(parent))] = {
            'status': 'retry allowed', 'watchdog_seconds': 600,
            'host_preflight': cut.read(archive / 'restart_host_preflight.json'),
            'wait': cut.read(archive / 'restart_wait.json'),
        }
    return records


def watchdog_rows(observations):
    for path, row in observations.items():
        wait = row.get('wait', row.get('retry_timing', row))
        load = row.get('host_preflight', {}).get('loadavg', '').split()
        yield (path, row.get('status', 'retry'), row['watchdog_seconds'],
               load[0] if load else wait.get('start_load_1m', '—'),
               wait.get('elapsed_seconds', '—'), wait.get('returncode', '—'))


def total_seconds(rows, field):
    values = [prior.Decimal(row[field]) for row in rows if field in row]
    return f'{sum(values, prior.ZERO):.6f}' if values else NOT_AVAILABLE


def peak_bytes(rows):
    return max((int(row['peak_bytes']) for row in rows if 'peak_bytes' in row), default=NOT_AVAILABLE)


def markdown(label, runs, cases, watchdog):
    summary, surface, budgets, engine_surface, collapse, phases, repairs, quality, missing = ([] for _ in range(9))
    contact, slowest, clearance, handover, bad, short, shifted = ([] for _ in range(7))
    for name, run in runs.items():
        for event in run['events']:
            row = event['summary']
            number = row['event']
            summary.append((number, name, f'{row["native_seconds"]:.3f}', row['actual_mesh_counts']['cells'],
                            row['actual_mesh_counts']['points'], row['proof_faces'],
                            f'{row["minimum_timer_coverage"]:.3%}', row['job_peak_rss_bytes']))
            removal = event['engine_surface_removal']
            engine_surface.append((number, name, *(removal.get(key, NOT_AVAILABLE)
                                                    for key in ('seed_faces', 'exit_faces', 'removed'))))
            last_state = event['ranks']['0']['surface_state'][-1:]
            handover.append((number, name, *(last_state[0].get(key, NOT_AVAILABLE) if last_state else NOT_AVAILABLE
                                             for key in ('faces', 'points'))))
            rank = event['slowest_rank']
            work = event['ranks'][str(rank)]
            screens = [row for row in work['contact'] if row['kind'] == 'screen']
            verifies = [row for row in work['contact'] if row['kind'] == 'verify']
            slowest.append((number, name, rank, *(total_seconds(work['surface'], field) for field in
                                                  ('seconds_c', 'seconds_c_contact', 'seconds_c_bound')),
                            total_seconds(screens, 'seconds_gather'), peak_bytes(screens),
                            total_seconds(verifies, 'seconds_gather'), total_seconds(verifies, 'seconds_verify'),
                            peak_bytes(verifies)))
            missing.extend({'event': number, 'run': name, **row} for row in event['missing_observations'])
            for rank, rows in event['ranks'].items():
                key = [number, name, rank]
                for row in rows['surface']:
                    refusals = ', '.join(f'{reason}={row["c_refused_" + reason]}' for reason in REASONS
                                         if row.get('c_refused_' + reason, '0') != '0')
                    surface.append((*key, row['stage'], row['pass'], *(row.get(field, '—')
                                    for field in (*COLLAPSE_FIELDS, 'faces_out', 'points_out')), refusals or '—'))
                for stage, census in rows['hand_over_metric'].items():
                    for phase, row in census['passes'].items():
                        short.append((*key, stage, phase, *(row[field] for field in C3_METRIC_FIELDS),
                                      *short_census_columns(row['c1']), row['out']))
                    shifted.append((*key, stage, *short_census_columns(census['shifted']),
                                    census['first_short_checkpoint']))
                for row in rows['contact']:
                    contact.append((*key, row['stage'], row['pass'], row['kind'],
                                    *(row.get(field, '—') for field in CONTACT_COLUMNS)))
                for row in rows['clearance']:
                    clearance.append((*key, row['phase'], row['calls'], row['inclusive_call_seconds'],
                                      row['exclusive_seconds']))
                for row in rows['core_rows']:
                    total, free = row.get('bad_out'), row.get('bad_out_free')
                    bad.append((*key, row['stage'], row['pass'], row['cells_out'], total or NOT_AVAILABLE,
                                free or NOT_AVAILABLE, int(total) - int(free) if total and free else NOT_AVAILABLE))
                for row in rows['surface_budget']:
                    budgets.append((*key, row['stage'], row['pass'], *(row.get(field, '—') for field in BUDGET_FIELDS)))
                for index, pair in enumerate(rows['engine_length_collapse'], 1):
                    length = pair['length'] or {}
                    elapsed = length.get('elapsed')
                    collapse.append((*key, index, f'{float(elapsed):.6f}' if elapsed else '—',
                                     pair['exit']['rounds'], length.get('collapse_accepted', '—')))
                totals = dict.fromkeys(prior.ENGINE_PHASES, prior.ZERO)
                for span in rows['engine_spans']:
                    phase = span['phase']
                    seconds = prior.Decimal(span['seconds'])
                    totals[phase] += seconds
                    if 'repair' in phase:
                        repairs.append((*key, span['invocation'], phase, span['from'], span['through'], seconds))
                phases.append((*key, *(f'{totals[phase]:.6f}' for phase in prior.ENGINE_PHASES)))
                for index, row in enumerate(rows['engine_quality_entry'], 1):
                    quality.append((*key, index, *(row.get(field, '—') for field in QUALITY)))

    lines = [f'# FP2-C3 build{label} S', '', 'FP2-C3: S passed; default path not yet proven', '',
             'Both remesh events and isolated restart passed the standing gates. The comparison uses the first '
             'FP2-D S run on build660.', '',
             S0_HISTORY, '', BUILD_663_HISTORY, '', f'[Fixture report and contact storage model](FP2_C3_fixture_{label}.md)', '',
             f'[Lab contact, embedding and memory observations](FP2_C3_lab_{label}.json)', '',
             *table(('Event', 'Run', 'Native maximum s', 'Cells', 'Points', 'Proof faces',
                     'Minimum coverage', 'Job peak bytes'), sorted(summary)),
             '## Hand-over mesh', '',
             *table(('Event', 'Run', 'Physical faces', 'Physical points'), sorted(handover)),
             'The production core row provides the total and free bad cells. The constrained count is their '
             'difference. Production rows do not distinguish the lab’s additional classes.', '',
             *table(('Event', 'Run', 'Rank', 'Stage', 'Pass', 'Cells after V', 'Bad total', 'Bad free',
                     'Bad constrained'), sorted(bad)),
             '## Short metric-edge observations', '',
             'The c1 census runs after step C. Its smallest edge has order 1. The production per-pass census '
             'after volume work is unavailable. The first positive count is an observation and can include edges '
             'already present at pass entry.', '',
             *table(('Event', 'Run', 'Rank', 'Stage', 'Pass', *C3_METRIC_FIELDS, 'c1 lt_1e_2',
                     'c1 metric', 'c1 space', 'c1 a', 'c1 b', 'out lt_1e_2'), sorted(short)),
             *table(('Event', 'Run', 'Rank', 'Stage', 'Shifted lt_1e_2', 'Shifted metric', 'Shifted space',
                     'Shifted a', 'Shifted b', 'First observed positive'), sorted(shifted)), '',
             '## Observed payload SHA256', '',
             *table(('File', 'SHA256'), sorted(runs[f'build{label}']['hashes'].items())),
             '## Core surface rows', '',
             f'Full per-rank rows are in [FP2_C3_{label}.json](FP2_C3_{label}.json). Omitted refusal counts in '
             'the summary table are zero where the operation ran.', '',
             *table(('Event', 'Run', 'Rank', 'Stage', 'Pass', *COLLAPSE_FIELDS, 'faces_out', 'points_out',
                     'Nonzero refusals'), sorted(surface)),
             '## Core contact rows', '',
             *table(('Event', 'Run', 'Rank', 'Stage', 'Pass', 'Kind', *CONTACT_COLUMNS), sorted(contact)),
             '### Contact work on the slowest native rank', '',
             'Step C already includes its contact and bound query clocks. The two gathers and the pass '
             'verification clock run outside step C.', '',
             *table(('Event', 'Run', 'Rank', 'All step C s', 'Contact in C s', 'Bound in C s',
                     'Screen gather s', 'Screen peak bytes', 'Verify gather s', 'Verify check s', 'Verify peak bytes'),
                    sorted(slowest)),
             '## Engine collapse pass and phase clocks', '',
             'The removal count is the difference between the first surface-state face count and the length '
             'collapse exit working face count, divided by two. These face counts are global.', '',
             *table(('Event', 'Run', 'Initial faces', 'Collapse exit faces', 'Surface points removed'),
                    sorted(engine_surface)),
             'Collapse seconds use the elapsed engine clock at the collapse pass boundary and include setup.', '',
             *table(('Event', 'Run', 'Rank', 'Pass', 'Collapse boundary s', 'Rounds', 'collapse_accepted'),
                    sorted(collapse)),
             *table(('Event', 'Run', 'Rank', *(f'{phase} s' for phase in prior.ENGINE_PHASES)), sorted(phases)),
             '### Clearance in the final remesh ledger', '',
             'Clearance is part of the containing phase clock. The inclusive call sum includes nested work.', '',
             *table(('Event', 'Run', 'Rank', 'Phase', 'Calls', 'Inclusive call s', 'Exclusive s'), sorted(clearance)),
             '### Individual repair spans', '',
             *table(('Event', 'Run', 'Rank', 'Invocation', 'Phase', 'Start', 'End', 'Seconds'), sorted(repairs)),
             '## Shape at quality entry', '',
             *table(('Event', 'Run', 'Rank', 'Entry', *QUALITY), sorted(quality)), timing_table(cases)]
    if budgets:
        lines += ['## Surface query storage', '',
                  'The scratch is budgeted above the overlay caller baseline. The conservative pass-plus-query '
                  'sum is reported beside its ceiling.', '',
                  *table(('Event', 'Run', 'Rank', 'Stage', 'Pass', *BUDGET_FIELDS), sorted(budgets))]
    if watchdog:
        lines += ['## Wrapper watchdog receipts', '',
                  *table(('Receipt', 'Status', 'Watchdog s', 'Start load', 'Elapsed s', 'Exit'),
                         watchdog_rows(watchdog))]
    if missing:
        lines += ['## Missing observations', '', *(f'- {row}' for row in missing), '']
    return '\n'.join(lines)


def report(build):
    label = build['build']
    name = f'build{label}'
    work = report_work(label)
    if not (work / 'run_receipt.json').is_file():
        failure_report(label, work)
        return
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    runner.reference_sources(payloads)
    with runner.bound():
        record = runner.prior.verify_run(work, 'S', build, payloads)
    parse = runpy.run_path(str(workflow.N22 / 'check_connected_events.py'))
    reference = collect(runner.s0_receipt(), parse)
    current = collect(work / 'run_receipt.json', parse, candidate=True)
    runs = {'build660': reference, name: current}
    cases = {'build660': Path(cut.read(runner.s0_receipt())['case']), name: Path(record['case'])}
    watchdog = watchdog_observations(work)
    publish(label, markdown(label, runs, cases, watchdog), {'runs': runs, 'watchdog': watchdog})


def failure_report(label, work):
    path = work / 'failed_attempt.json'
    failure = cut.read(path)
    case = Path(failure['case'])
    for name in ('full', 'restart'):
        if (work / f'{name}_launch.json').is_file():
            wait = cut.read(work / f'{name}_wait.json')
            runner.terminal_wait(work, name, wait['returncode'])
    for name, digest in failure['artifacts'].items():
        assert sha(work / name) == digest, name
    contact_failure = {}
    if (path := work / 'contact_failure.json').is_file():
        contact_failure = cut.read(path)
        assert all(sha(work / name) == digest for name, digest in contact_failure['logs'].items()), path
    path = work / 'failed_attempt.json'
    watchdog = watchdog_observations(work)
    open_for_user = any(row.get('status') == 'OPEN for the user' for row in watchdog.values())
    assert open_for_user or not (work / 'full_watchdog.json').is_file(), 'Authorized full watchdog retry is pending.'
    status = ('FP2-C3: S OPEN for the user (helper watchdog); default path not yet proven' if open_for_user
              else 'FP2-C3: S FAILED; default path not yet proven')
    evaluated = {str(path): cut.read(path)['passed'] for path in sorted(
        (*case.glob('f3b_event*_timer_gate.json'), *case.glob('p7p8_event*_gate.json')))}
    lines = [f'# FP2-C3 build{label} S', '', status, '',
             S0_HISTORY, '', BUILD_663_HISTORY, '', f'[Fixture report and contact storage model](FP2_C3_fixture_{label}.md)', '',
             f'[Lab contact and embedding observations](FP2_C3_lab_{label}.json)', '',
             f'Step ended during {failure["stage"]}.', '',
             f'First recorded observation. {failure["first_observed_failure"]}', '',
             '## Evaluated event gates', '',
             *table(('Receipt', 'Result'), ((Path(name).name, 'PASS' if passed else 'FAIL')
                                           for name, passed in evaluated.items()))]
    if contact_failure:
        lines += ['## Contact result failure', '', *(f'- {path}' for path in contact_failure['logs']), '']
    if watchdog:
        lines += ['## Wrapper watchdog receipts', '',
                  *table(('Receipt', 'Status', 'Watchdog s', 'Start load', 'Elapsed s', 'Exit'),
                         watchdog_rows(watchdog))]
    publish(label, '\n'.join(lines), {'failure': failure, 'contact_failure': contact_failure, 'receipt': str(path), 'receipt_sha256': sha(path),
                                      'evaluated_gates': evaluated, 'watchdog': watchdog})


def publish(label, content, evidence):
    path = HERE / 'reports' / f'FP2_C3_{label}.md'
    path.parent.mkdir(exist_ok=True)
    workflow.sealed_write(path.with_suffix('.json'), evidence)
    if not path.exists():
        with path.open('x') as stream:
            stream.write(content)
    else:
        assert path.read_text() == content, path
    print(path)


def self_check():
    parse = runpy.run_path(str(workflow.N22 / 'check_connected_events.py'))
    record = collect(runner.s0_receipt(), parse)
    assert len(record['events']) == 2
    assert not any(row['missing_observations'] for row in record['events'])
    for event in record['events']:
        for rows in event['ranks'].values():
            for stage in rows['hand_over_metric'].values():
                assert stage['passes']
                for found in stage['passes'].values():
                    assert found['short_after_c'] == NOT_AVAILABLE
                    assert found['c1']['lt_1e_2'].isdigit()
                    assert found['c1']['smallest']['order'] == '1'
                assert stage['shifted']['lt_1e_2'].isdigit()
    sample = {'stage': '0', 'pass': '1', 'short_after_l': 'unavailable', 'short_after_c': '2'}
    data = short_observations([sample], [])
    assert data['0']['passes']['1']['short_after_l'] == 'unavailable'
    assert data['0']['first_short_checkpoint'] == 'pass 1 short_after_c'
    missing = short_observations([], [], ({'stage': '0', 'pass': '1'},))
    assert missing['0']['passes']['1']['short_after_c'] == NOT_AVAILABLE
    assert missing['0']['first_short_checkpoint'] == NOT_AVAILABLE
    timing = timing_table({'build660': Path(cut.read(runner.s0_receipt())['case'])})
    assert '| Core surface collapse step |' in timing
    assert 'timing unavailable' not in timing
    print('FP2-C3 report parser, S0 engine removal counts and two timing tables verified.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    opts = parser.parse_args()
    if opts.self_check:
        self_check()
    elif opts.build is None:
        parser.error('a build directory is required')
    else:
        report(cut.read(opts.build.resolve() / 'build_status.json'))


if __name__ == '__main__':
    main()
