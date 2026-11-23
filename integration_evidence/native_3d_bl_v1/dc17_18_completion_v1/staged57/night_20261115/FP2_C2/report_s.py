"""Report FP2-C2 S beside the accepted FP2-C1 build647 S workflow."""

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
COLLAPSE_FIELDS = ('collapses_surface', 'c_sweeps', 'c_candidates', 'c_deferred', 'c_reverse_max',
                   'seconds_c', 'seconds_c_samples', 'seconds_c_entity', 'seconds_c_bound', 'c_bound_queries',
                   'faces_before_c', 'points_before_c')
REASONS = ('protected', 'border', 'ineligible', 'border_keeper', 'feature_chord', 'link', 'near', 'outward',
           'reverse', 'topology', 'validity', 'size', 'shape', 'distance', 'entity', 'bound')
REFUSALS = tuple('c_refused_' + reason for reason in REASONS)
BUDGET_FIELDS = ('overlay_peak', 'pass_admitted_bytes', 'query_scratch_max', 'conservative_sum', 'ceiling')
CONTINUATION_HISTORY = (
    'Build652 N1 failed the lab proof with four unexplained mesh-side faces. '
    'The build653 test-only replay localized all four to step C on sheet 10154 and located the model-side '
    'witness on facet 11045. [Build652 report](FP2_C2_652.md) · '
    '[Continuation diagnosis](FP2_C2_continue1_diag_653.md)')
QUALITY = ('q_measured', 'q_bad', 'q_debt', 'j_measured', 'j_bad', 'j_debt')
C_CLOCK = ('Core surface collapse step', SURFACE, ('seconds_c',))
RANK_TIMING = prior.rank_timing


def report_work(label):
    work = runner.work_path(label)
    retry = work / runner.FULL_RETRY
    assert not all((path / 'run_receipt.json').is_file() for path in (work, retry))
    return retry if retry.is_dir() else work


def collect(path, parse, candidate=False):
    run = prior.collect(path)
    case = Path(cut.read(path)['case'])
    for event in run['events']:
        number = event['summary']['event']
        _, local, _, _ = parse['read_event'](case, number)
        for rank, lines in sorted(local.items()):
            rows = event['ranks'][str(rank)]
            for key, marker in (('surface', SURFACE), ('surface_state', SURFACE_STATE), ('surface_budget', SURFACE_BUDGET)):
                rows[key] = [row for line in lines if (row := parse['fields'](line, marker))
                             and row['rank'] == str(rank) and row.get('applicable') != '0']
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


def markdown(label, runs, cases, watchdog):
    summary, surface, budgets, engine_surface, collapse, phases, repairs, quality, missing = ([] for _ in range(9))
    for name, run in runs.items():
        for event in run['events']:
            row = event['summary']
            number = row['event']
            summary.append((number, name, f'{row["native_seconds"]:.3f}', row['actual_mesh_counts']['cells'],
                            row['actual_mesh_counts']['points'], row['proof_faces'],
                            f'{row["minimum_timer_coverage"]:.3%}', row['job_peak_rss_bytes']))
            removal = event['engine_surface_removal']
            engine_surface.append((number, name, *(removal.get(key, '—')
                                                    for key in ('seed_faces', 'exit_faces', 'removed'))))
            missing.extend({'event': number, 'run': name, **row} for row in event['missing_observations'])
            for rank, rows in event['ranks'].items():
                key = [number, name, rank]
                for row in rows['surface']:
                    refusals = ', '.join(f'{reason}={row["c_refused_" + reason]}' for reason in REASONS
                                         if row.get('c_refused_' + reason, '0') != '0')
                    surface.append((*key, row['stage'], row['pass'], *(row.get(field, '—')
                                    for field in (*COLLAPSE_FIELDS, 'faces_out', 'points_out')), refusals or '—'))
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

    lines = [f'# FP2-C2 build{label} S', '', 'FP2-C2: S passed; default path not yet proven', '',
             'Both remesh events and isolated restart passed the standing gates. The comparison uses the first '
             'accepted FP2-C1 S run on build647.', '',
             CONTINUATION_HISTORY, '', f'[Continuation lab and memory observations](FP2_C2_lab_{label}.json)', '',
             *table(('Event', 'Run', 'Native maximum s', 'Cells', 'Points', 'Proof faces',
                     'Minimum coverage', 'Job peak bytes'), sorted(summary)),
             '## Published payload SHA256', '',
             *table(('File', 'SHA256'), sorted(runs[f'build{label}']['hashes'].items())),
             '## Core surface rows', '',
             f'Full per-rank rows are in [FP2_C2_{label}.json](FP2_C2_{label}.json). Omitted refusal counts in '
             'the summary table are zero where the operation ran.', '',
             *table(('Event', 'Run', 'Rank', 'Stage', 'Pass', *COLLAPSE_FIELDS, 'faces_out', 'points_out',
                     'Nonzero refusals'), sorted(surface)),
             '## Engine collapse pass and phase clocks', '',
             'The removal count is the difference between the first surface-state face count and the length '
             'collapse exit working face count, divided by two. These face counts are global.', '',
             *table(('Event', 'Run', 'Initial faces', 'Collapse exit faces', 'Surface points removed'),
                    sorted(engine_surface)),
             'Collapse seconds use the elapsed engine clock at the collapse pass boundary and include setup.', '',
             *table(('Event', 'Run', 'Rank', 'Pass', 'Collapse boundary s', 'Rounds', 'collapse_accepted'),
                    sorted(collapse)),
             *table(('Event', 'Run', 'Rank', *(f'{phase} s' for phase in prior.ENGINE_PHASES)), sorted(phases)),
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
    reference = collect(runner.S0_S, parse)
    current = collect(work / 'run_receipt.json', parse, candidate=True)
    runs = {'build647': reference, name: current}
    cases = {'build647': Path(cut.read(runner.S0_S)['case']), name: Path(record['case'])}
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
    watchdog = watchdog_observations(work)
    open_for_user = any(row.get('status') == 'OPEN for the user' for row in watchdog.values())
    assert open_for_user or not (work / 'full_watchdog.json').is_file(), 'Authorized full watchdog retry is pending.'
    status = ('FP2-C2: S OPEN for the user (helper watchdog); default path not yet proven' if open_for_user
              else 'FP2-C2: S FAILED; default path not yet proven')
    evaluated = {str(path): cut.read(path)['passed'] for path in sorted(
        (*case.glob('f3b_event*_timer_gate.json'), *case.glob('p7p8_event*_gate.json')))}
    lines = [f'# FP2-C2 build{label} S', '', status, '',
             CONTINUATION_HISTORY, '', f'[Continuation lab and memory observations](FP2_C2_lab_{label}.json)', '',
             f'Step ended during {failure["stage"]}.', '',
             f'First recorded observation. {failure["first_observed_failure"]}', '',
             '## Evaluated event gates', '',
             *table(('Receipt', 'Result'), ((Path(name).name, 'PASS' if passed else 'FAIL')
                                           for name, passed in evaluated.items()))]
    if watchdog:
        lines += ['## Wrapper watchdog receipts', '',
                  *table(('Receipt', 'Status', 'Watchdog s', 'Start load', 'Elapsed s', 'Exit'),
                         watchdog_rows(watchdog))]
    publish(label, '\n'.join(lines), {'failure': failure, 'receipt': str(path), 'receipt_sha256': sha(path),
                                      'evaluated_gates': evaluated, 'watchdog': watchdog})


def publish(label, content, evidence):
    path = HERE / 'reports' / f'FP2_C2_{label}.md'
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
    record = collect(runner.S0_S, parse)
    assert [row['engine_surface_removal']['removed'] for row in record['events']] == [3105, 3139]
    assert not any(row['missing_observations'] for row in record['events'])
    timing = timing_table({'build647': Path(cut.read(runner.S0_S)['case'])})
    assert '| Core surface collapse step | 0.000 | 0.000 |' in timing
    assert 'timing unavailable' not in timing
    print('FP2-C2 report parser, both C1 engine removal counts and two timing tables verified.')


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
