"""Run the sealed FP1 OFF, ON1, and ON2 workflows after Part C's scoped gates."""

from argparse import ArgumentParser
from collections import defaultdict
from decimal import Decimal
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory

sys.dont_write_bytecode = True
from run_g1 import ITEM, cut, sha, verify_build

NIGHT = ITEM.parent
R = NIGHT.parent
N16 = cut.PHASE / 'round_8_5r_C/G5'
N22 = cut.PHASE / 'round_8_5u_C/G5'
CORE = R / 'p7p8_r3_20261115/run_full_workflow.py'
WRAPPER = NIGHT / 'section6_2/run_full_section6_2.py'
PAYLOADS = NIGHT / 'section6_2/compare_reference_payloads.py'
TIMER_REPORT = NIGHT / 'N8/A2_A3_N12/section6_8/validation/validate_n8.py'
F_WORKFLOW = NIGHT / 'N18/drafts/recipes/workflow_f.py'
SUMMARY = cut.PHASE / 'phase_C/collect_g5_summary.py'
N21 = NIGHT / 'N21/accepted_reference.json'
ORDER = ITEM / 'inputs/FAST_PATH_STAGE1.md'
BASE_CONFIG = R / 'm6_inputs/euler_adapted293_event1_v1/run.cfg'
OFF_CONFIG = N22 / 'full591_run1/run.cfg'
OFF_CONFIG_SHA = '5c0fe04144c7bb230ec7f71e26c87bf7d0150cdf5c89caa6e840ba20f3ae2d56'
ON_CONFIG = ITEM / 'run.cfg'
ON_LINE = b'ADAP_NATIVE_LOCAL_CORE= YES\n'
MEMORY_LINE = b'\nADAP_NATIVE_JOB_MEMORY_LIMIT= 19073\n'
MODES = ('OFF', 'ON1', 'ON2')
HELPERS = ('check_connected_events.py', 'check_complete_events.py', 'check_timer_tree.py',
           'unchanged_build499_source_pins.json')
RESTART_SCRIPTS = ('run_restart', 'validate_restart')


def sealed_write(path, value):
    if path.exists():
        assert cut.read(path) == value, path
    else:
        cut.save(path, value)


def replace_once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def config_bytes():
    """Authenticate the existing effective OFF config and construct the exact ON bytes."""
    clean = cut.read(R / 'nobl_clean_check_20261114/run/provenance.json')
    assert sha(BASE_CONFIG) == clean['config_sha256']
    assert sha(OFF_CONFIG) == OFF_CONFIG_SHA
    before = OFF_CONFIG.read_bytes()
    assert before == BASE_CONFIG.read_bytes() + MEMORY_LINE
    assert ON_LINE.split(b'=')[0] not in before
    return before + ON_LINE


def select_config(on):
    expected = config_bytes()
    if not on:
        return OFF_CONFIG
    if not ON_CONFIG.exists():
        with ON_CONFIG.open('xb') as stream:
            stream.write(expected)
    assert ON_CONFIG.read_bytes() == expected
    return ON_CONFIG


def core_source():
    """Bind ON and the normalized-event parser without changing the original controller."""
    source = CORE.read_text()
    edits = [
        ('R = PURPOSE.parent', f'R = Path({str(R)!r})'),
        ('    shutil.copyfile(config, case / "run.cfg")\n'
         '    with (case / "run.cfg").open("a") as stream:\n'
         '        stream.write("\\nADAP_NATIVE_JOB_MEMORY_LIMIT= 19073\\n")',
         '    effective = FP1_CONFIG\n'
         '    assert sha(effective) == FP1_CONFIG_SHA\n'
         '    shutil.copyfile(effective, case / "run.cfg")\n'
         '    assert sha(case / "run.cfg") == FP1_CONFIG_SHA'),
        ('    input_hashes.update(runtime_pins)',
         '    input_hashes.update(runtime_pins)\n'
         '    input_hashes.update({str(path): sha(path) for path in FP1_HELPERS})'),
        ('                if guard_records:\n'
         '                    save(case / "p6c_refinement_guard_failure.json",{"reason":"P6c predicts zero one-sided model refinements; stop first failure without another fix.","records":guard_records})\n'
         '                    stop_reason="p6c_one_sided_refinement_guard_failure"\n'
         '                    break',
         '                if guard_records:\n'
         '                    receipt = case / "p6c_refinement_guard_failure.json"\n'
         '                    save(receipt, {"reason": "P6c predicts zero one-sided model refinements.",\n'
         '                                   "report_only": FP1_ON, "records": guard_records})\n'
         '                    if not FP1_ON:\n'
         '                        stop_reason="p6c_one_sided_refinement_guard_failure"\n'
         '                        break'),
        ('                save(case / \'final_source_first_collapse_gate.json\', p6_collapse)\n'
         "                if not p6_collapse['passed']:",
         "                p6_collapse['report_only'] = FP1_ON\n"
         "                save(case / 'final_source_first_collapse_gate.json', p6_collapse)\n"
         "                if not FP1_ON and not p6_collapse['passed']:"),
        ('        "upper_work_sum": work_sum, "upper_work_limit": MAX_WORK,',
         '        "upper_work_sum": work_sum, "upper_work_limit": MAX_WORK, "report_only": FP1_ON,'),
        ('or not first_collapse_gate["upper_work_pass"] or',
         'or (not FP1_ON and not first_collapse_gate["upper_work_pass"]) or'),
        ('    for path in rank_logs:\n        for number, raw in enumerate(path.open(errors="replace"), 1):',
         '    pending_exits = {}\n'
         '    for path in rank_logs:\n        for number, raw in enumerate(path.open(errors="replace"), 1):'),
        ('                        per_rank[rank]["stages"].append(row)\n'
         '            if "NATIVE3D_DETAIL',
         '                        if FP1_ON:\n'
         '                            pending_exits[rank] = row\n'
         '                        else:\n'
         '                            per_rank[rank]["stages"].append(row)\n'
         '            if FP1_ON and "NATIVE3D_TIMER_SUMMARY " in line:\n'
         '                token = line.split("NATIVE3D_TIMER_SUMMARY ", 1)[1]\n'
         '                timer = dict(re.findall(r"([\\w]+)=([^\\s]+)", token))\n'
         '                rank = int(path.parent.name.rsplit(".", 1)[-1])\n'
         '                if (timer.get("root") == "remesh" and timer.get("boundary") == "final"\n'
         '                        and timer.get("rank") == str(rank) and rank in pending_exits):\n'
         '                    per_rank[rank]["stages"].append(pending_exits.pop(rank))\n'
         '            if "NATIVE3D_DETAIL'),
    ]
    for old, new in edits:
        source = replace_once(source, old, new)
    return source


def restart_source(name, build, build_dir):
    template = N16 / f'{name}575.py'
    source = template.read_text().replace('575', str(build))
    source = replace_once(source, 'N11 = HERE.parents[1]', f'N11 = Path({str(cut.PHASE)!r})')
    source = replace_once(source, f'BUILD = N11 / "build{build}"', f'BUILD = Path({str(build_dir)!r})')
    return template, source


def bind_text(path, source):
    compile(source, str(path), 'exec')
    if not path.exists():
        with path.open('x') as stream:
            stream.write(source)
    assert path.read_text() == source, path


def prepare(work, build, build_dir, config):
    work.mkdir(parents=True, exist_ok=True)
    sources = [CORE, WRAPPER, PAYLOADS, TIMER_REPORT, F_WORKFLOW, SUMMARY, N21, ORDER, BASE_CONFIG,
               OFF_CONFIG, config, Path(__file__).resolve(),
               R / 'f3b_20261115/check_timer_tree.py', NIGHT / 'N16/event_prefix.py']
    bound = []
    for name in HELPERS:
        origin = N22 / name
        target = work / name
        if not target.exists():
            shutil.copyfile(origin, target)
        assert sha(origin) == sha(target), target
        sources.append(origin)
        bound.append(target)
    generated = work / 'full_controller.py'
    bind_text(generated, core_source())
    bound.append(generated)
    for name in RESTART_SCRIPTS:
        origin, source = restart_source(name, build, build_dir)
        target = work / f'{name}{build}.py'
        bind_text(target, source)
        sources.append(origin)
        bound.append(target)
    sealed_write(work / 'helper_pins.json', {
        'build': build, 'source': {str(p): sha(p) for p in sources},
        'bound': {str(p): sha(p) for p in bound}, 'config': str(config), 'config_sha256': sha(config),
    })


def verify_helpers(work):
    manifest = cut.read(work / 'helper_pins.json')
    for name in ('source', 'bound'):
        for path, digest in manifest[name].items():
            assert cut.sha(path) == digest, path
    return manifest


def runner(work, build_dir, on, config, helpers):
    wrapper = runpy.run_path(str(WRAPPER))
    load = wrapper['load_runner']
    load.__globals__.update(CORE=work / 'full_controller.py', HERE=work)
    status = cut.read(build_dir / 'build_status.json')
    namespace = load(status['binary'])
    namespace.update(SCRIPT=Path(__file__).resolve(), BUILD_STATUS=build_dir / 'build_status.json',
                     SOURCE_PINS=build_dir / 'source_pins.json', FP1_ON=on,
                     FP1_CONFIG=config, FP1_CONFIG_SHA=sha(config), FP1_HELPERS=helpers)
    return namespace


def full_case(work, build, build_dir, on):
    manifest = verify_helpers(work)
    assert manifest['build'] == build
    config = Path(manifest['config'])
    expected = config_bytes()
    if not on:
        expected = expected.removesuffix(ON_LINE)
    assert config.read_bytes() == expected and sha(config) == manifest['config_sha256']
    namespace = runner(work, build_dir, on, config, [*manifest['source'], *manifest['bound']])
    assert namespace['PURPOSE'] == work and namespace['R'] == R
    sys.path[0:0] = [str(work), str(R / 'p7p8_r3_20261115')]
    for name in ('check_complete_events', 'check_connected_events'):
        module = __import__(name)
        assert Path(module.__file__).resolve() == work / f'{name}.py'
    sys.argv = [str(Path(__file__).resolve()), str(work / f'full{build}_run1'), '--full']
    namespace['main']()


def run_stage(work, name, argv):
    wait = work / f'{name}_wait.json'
    launch = work / f'{name}_launch.json'
    if not wait.exists():
        assert not launch.exists(), f'Unverified prior {name} process: {launch}'
        cut.run_gate(work, name, argv)
    receipt = cut.read(wait)
    assert receipt['controller'] == cut.read(launch)['controller']
    assert receipt['terminal'] and cut.terminal(receipt['controller'])
    assert receipt['returncode'] == 0, wait


def timer_report(case, require_wall):
    source = replace_once(TIMER_REPORT.read_text(), 'if marker not in raw:', 'if marker + " " not in raw:')
    env = {'__name__': 'fp1_timer_report', '__file__': str(TIMER_REPORT)}
    exec(compile(source, str(TIMER_REPORT), 'exec'), env)
    return env['full_wall_timer_report'](case, require_wall=require_wall)


def workflow_payloads(case, binary_sha, payloads):
    status = cut.read(case / 'status.json')
    assert status['all_owned_handles_terminal'] and all(cut.terminal(row) for row in status['owned_identities'])
    assert cut.read(case / 'provenance.json')['binary_sha256'] == binary_sha
    return payloads['payload_hashes'](case)


def seal_run(case, binary_sha, config, payloads, on):
    """Authenticate this run's six payloads for restart independently of the cross-run rules."""
    assert sha(case / 'run.cfg') == sha(config)
    assert cut.read(case / 'provenance.json')['config_sha256'] == sha(config)
    actual = workflow_payloads(case, binary_sha, payloads)
    timer = timer_report(case, require_wall=not on)
    workflow = sha(case / 'full_workflow_receipt.json')
    reference = case / 'accepted_reference_payloads.json'
    sealed_write(reference, {'accepted': False, 'hashes': actual, 'state': 'standing workflow gates passed',
                             'workflow_receipt_sha256': workflow})
    sealed_write(case / 'n8_wall_timer_report.json', timer)
    sealed_write(case / 'n8_payload_identity.json', {
        'case': str(case), 'reference': str(reference), 'reference_sha256': sha(reference),
        'hashes': actual, 'mismatches': {}, 'passed': True, 'workflow_gates_passed': True,
        'timer_and_wall_report': timer, 'workflow_receipt_sha256': workflow,
        'rank_stdout_sha256': {str(p.relative_to(case)): sha(p)
                               for p in sorted(case.glob('rank_output/*/rank.*/stdout'))},
    })
    return actual


def off_reference(payloads):
    accepted = cut.read(N21)
    assert accepted['accepted']
    for name in ('payloads', 'workflow_acceptance', 'timing_rule'):
        entry = accepted[name]
        path = (NIGHT / 'N21' / entry['path']).resolve()
        assert sha(path) == entry['sha256'], path
    adopted = cut.read((NIGHT / 'N21' / accepted['payloads']['path']).resolve())
    assert adopted['accepted'] and adopted['acceptance_sha256'] == accepted['workflow_acceptance']['sha256']
    assert all(payloads['payload_hashes'](Path(adopted[name])) == adopted['hashes']
               for name in ('first_case', 'repeat_case'))
    return adopted['hashes']


def timings(off, now):
    rows = []
    for old, new in zip(off, now, strict=True):
        assert old['event'] == new['event']
        seconds = Decimal(str(new['native_seconds']))
        limit = Decimal('1.05') * Decimal(str(old['native_seconds']))
        rows.append({'event': new['event'], 'native_seconds': str(seconds),
                     'limit_seconds': str(limit), 'passed': seconds <= limit})
    return rows


def isolate_restart(work, build, case):
    for name in RESTART_SCRIPTS:
        script = work / f'{name}{build}.py'
        subprocess.run([cut.PYTHON, '-B', str(script), '--self-check'], cwd=cut.REPO, check=True)
    run_stage(work, 'restart', [cut.PYTHON, '-B', str(work / f'run_restart{build}.py'),
                                '--source-case', str(case)])
    workflow = runpy.run_path(str(F_WORKFLOW))
    verify = workflow['verify_restart']
    verify.__globals__['WORKSPACE'] = work
    return verify(build, case)


def prefix_report(off, on):
    compare = runpy.run_path(str(F_WORKFLOW))['compare_event_one']
    try:
        return {'report_only': True, **compare(off, on)}
    except ValueError as error:
        return {'report_only': True, 'passed': False, 'error': str(error)}


def report_rows(case, event, parse):
    """Collect the FP1 report rows within each rank's terminal remesh event."""
    _, local, _, final_timers = parse['read_event'](case, event)
    fields = parse['fields']
    strip = parse['timer'].strip_mpi_prefixes
    core, shift = defaultdict(list), defaultdict(list)
    length_rows, engine, entries = {}, {}, {}
    core_clock, shift_clock = {}, {}
    frozen = complexity = None
    for rank, lines in sorted(local.items()):
        last_length = None
        engine[rank], entries[rank] = [], {}
        core_clock[rank] = shift_clock[rank] = 0
        stage = 0
        for line in map(strip, lines):
            if row := fields(line, 'NATIVE3D_LOCAL_CORE'):
                if row.get('rank') != str(rank) or row.get('applicable') != '1':
                    continue
                stage = int(row['stage'])
                core[stage, int(row['pass'])].append(row)
                core_clock[rank] += float(row['seconds'])
                entries[rank][stage] = int(row['cells_out'])
            elif row := fields(line, 'NATIVE3D_LOCAL_SHIFT'):
                if row.get('rank') != str(rank):
                    continue
                shift[stage, row['direction']].append(row)
                shift_clock[rank] += float(row['seconds'])
                entries[rank][stage] = int(row['cells_after'])
            elif row := fields(line, 'NATIVE3D_PARTITION'):
                if row.get('rank') == str(rank):
                    entries[rank][stage] = int(row['cells_after'])
            elif row := fields(line, 'NATIVE3D_LENGTH'):
                if row.get('rank') == str(rank):
                    last_length = row
            elif row := fields(line, 'NATIVE3D_STAGE'):
                if row.get('rank') == str(rank) and row.get('stage') == 'exit':
                    engine[rank].append({key: int(row[key]) for key in ('length_collapses', 'length_splits', 'rounds')})
            elif rank == 0:
                if match := re.search(r'Native3D frozen stages:\s+count=(\d+)', line):
                    frozen = int(match[1])
                if match := re.search(r'Native3D candidate complexity:\s+value=(\S+)', line):
                    complexity = float(match[1])
        if last_length:
            length_rows[rank] = {key: int(last_length[key]) for key in ('local_edges', 'short_local_edges')}

    def sum_fields(rows, labels):
        values = defaultdict(int)
        for row in rows:
            for key, value in row.items():
                if key not in (*labels, 'rank'):
                    values[key] += float(value) if key == 'seconds' else int(value)
        return dict(values)

    construction = {rank: sum(float(row['exclusive_seconds']) for row in item['rows']
                              if row['phase'] == 'construction') for rank, item in final_timers.items()}
    return {
        'local_core_sums': [{'stage': stage, 'pass': index, 'ranks_observed': len(rows),
                             **sum_fields(rows, ('stage', 'pass', 'applicable'))}
                            for (stage, index), rows in sorted(core.items())],
        'local_shift_sums': [{'stage': stage, 'direction': direction, 'ranks_observed': len(rows),
                              **sum_fields(rows, ('direction',))}
                             for (stage, direction), rows in sorted(shift.items())],
        'engine_entry_cells_by_rank_and_stage': {str(rank): {str(stage): cells for stage, cells in by_stage.items()}
                                                  for rank, by_stage in entries.items()},
        'engine_exit_by_rank_and_stage': {str(rank): rows for rank, rows in engine.items()},
        'engine_accepted_collapses': sum(row['length_collapses'] for rows in engine.values() for row in rows),
        'engine_accepted_splits': sum(row['length_splits'] for rows in engine.values() for row in rows),
        'last_length_by_rank': {str(rank): row for rank, row in length_rows.items()},
        'last_length_sums': sum_fields(length_rows.values(), ()),
        'normalization_stages': frozen, 'complexity_value': complexity,
        'core_pass_seconds_by_rank': {str(rank): value for rank, value in core_clock.items()},
        'shift_seconds_by_rank': {str(rank): value for rank, value in shift_clock.items()},
        'construction_excluding_core_passes_and_shifts_seconds_by_rank': {
            str(rank): seconds - core_clock[rank] - shift_clock[rank] for rank, seconds in construction.items()
        },
    }


def campaign(output, build_dir, scoped, safeguards):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    receipt = cut.read(build_dir / 'build_status.json')
    _, source_sha = verify_build(receipt, build_dir / 'source_pins.json')
    wait = receipt['wait']
    assert wait['terminal'] and cut.terminal(wait['controller'])
    build = receipt['build']
    for path in (scoped, safeguards):
        gate = cut.read(path)
        assert (gate.get('passed') or gate.get('state') == 'PASSED') and gate['build'] == build, path
    binary_sha = receipt['binary_sha256']
    assert cut.sha(receipt['binary']) == binary_sha
    payloads = runpy.run_path(str(PAYLOADS))
    collect = runpy.run_path(str(SUMMARY))['collect_events']
    parse = runpy.run_path(str(N22 / 'check_connected_events.py'))
    reference = off_reference(payloads)
    outputs = {}
    for mode in MODES:
        on = mode != 'OFF'
        config = select_config(on)
        work = output / mode
        assert not (work / 'failed_attempt.json').exists(), work
        prepare(work, build, build_dir, config)
        case = work / f'full{build}_run1'
        stage = 'full event gates'
        try:
            run_stage(work, 'full', [cut.PYTHON, '-B', str(Path(__file__).resolve()),
                                      str(build_dir), '--output', str(output), '--full', mode])
            actual = seal_run(case, binary_sha, config, payloads, on)
            baseline = payloads['compare_hashes'](actual, reference)
            sealed_write(work / 'off_payload_comparison.json',
                         {'report_only': on, 'passed': not baseline, 'differences': baseline})
            if not on:
                assert not baseline, next(iter(baseline.items()), None)
            stage = 'isolated restart'
            restart = isolate_restart(work, build, case)
            stage = 'report rows'
            events = collect(case)
            for event in events:
                event.update(report_rows(case, event['event'], parse))
            info = {'case': str(case), 'hashes': actual, 'restart': str(restart),
                    'events': events, 'config_sha256': sha(config)}
            if on:
                report = prefix_report(Path(outputs['OFF']['case']), case)
                sealed_write(work / 'event_one_construction_report.json', report)
                info['time_rule'] = timings(outputs['OFF']['events'], info['events'])
            sealed_write(work / 'run_receipt.json', info)
            outputs[mode] = info
        except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
            sealed_write(work / 'failed_attempt.json', {
                'passed': False, 'mode': mode, 'stage': stage, 'case': str(case), 'error': str(error),
                'status': cut.read(case / 'status.json') if (case / 'status.json').exists() else None,
            })
            raise
        assert cut.sha(receipt['binary']) == binary_sha
    differences = payloads['compare_hashes'](outputs['ON2']['hashes'], outputs['ON1']['hashes'])
    sealed_write(output / 'on_repeat_comparison.json', {'passed': not differences, 'differences': differences})
    assert not differences, next(iter(differences.items()), None)
    opened = any(not row['passed'] for mode in MODES[1:] for row in outputs[mode]['time_rule'])
    outcome = 'OPEN for the user' if opened else 'PASSED'
    final = {'standing_gates_passed': True, 'repeat_payloads_exact': True, 'status': outcome,
             'build': build, 'binary_sha256': binary_sha, 'source_pins_sha256': source_sha, 'runs': outputs}
    sealed_write(output / 'acceptance_summary.json', final)
    sealed_write(ITEM / 'on_reference.json', {
        'accepted': True, 'hashes': outputs['ON1']['hashes'], 'binary_sha256': binary_sha,
        'first_case': outputs['ON1']['case'], 'repeat_case': outputs['ON2']['case'],
        'config_sha256': outputs['ON1']['config_sha256'], 'status': outcome,
        'acceptance_sha256': sha(output / 'acceptance_summary.json'),
    })
    print(f'FP1 ON {outcome}.', flush=True)


def self_check():
    expected = config_bytes()
    assert expected.endswith(MEMORY_LINE + ON_LINE)
    payloads = runpy.run_path(str(PAYLOADS))
    archive = N22 / 'full591_run1'
    status = cut.PHASE / 'build591'
    assert payloads['payload_hashes'](archive) == off_reference(payloads)
    assert timer_report(archive, require_wall=True)['all_fixedstorage_4120']
    parse = runpy.run_path(str(N22 / 'check_connected_events.py'))
    for event, value in ((1, 51328.596781622735), (2, 50588.745036561028)):
        report = report_rows(archive, event, parse)
        assert report['normalization_stages'] == 1 and report['complexity_value'] == value
        assert report['last_length_sums']['local_edges'] > 0
        assert len(report['engine_entry_cells_by_rank_and_stage']) == 6
    assert prefix_report(archive, archive)['passed']
    with TemporaryDirectory(prefix='fp1-g5-') as directory:
        work = Path(directory) / 'G5'
        work.mkdir()
        # The connected helper locates its source root via the staged57 ancestor.
        # Exercise the binding and source anchors here without importing a launch entry point.
        core = work / 'full_controller.py'
        bind_text(core, core_source())
        for on in (False, True):
            namespace = runner(work, status, on, OFF_CONFIG, [])
            assert namespace['FP1_ON'] is on and namespace['PURPOSE'] == work
            assert namespace['assess_p6']({}, True)['passed'] is False
        for name in RESTART_SCRIPTS:
            _, source = restart_source(name, 591, status)
            script = work / f'{name}591.py'
            bind_text(script, source)
        for name in RESTART_SCRIPTS:
            subprocess.run([cut.PYTHON, '-B', str(work / f'{name}591.py'), '--self-check'],
                           cwd=cut.REPO, stdout=subprocess.DEVNULL, check=True)
    old = [{'event': 1, 'native_seconds': 2000}, {'event': 2, 'native_seconds': 1000}]
    assert all(row['passed'] for row in timings(old, [{'event': 1, 'native_seconds': 2100},
                                                    {'event': 2, 'native_seconds': 1050}]))
    assert not timings(old, [{'event': 1, 'native_seconds': 2100.001}, old[1]])[0]['passed']
    print('FP1 G5 no-launch checks passed.')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build_dir', nargs='?', type=Path)
    parser.add_argument('--scoped', type=Path)
    parser.add_argument('--safeguards', type=Path)
    parser.add_argument('--output', type=Path, default=ITEM / 'C/G5')
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--full', choices=MODES, help='internal ordered-stage entry point')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.build_dir is None:
        parser.error('a sealed Part C build directory is required')
    if args.full:
        build = cut.read(args.build_dir / 'build_status.json')['build']
        full_case(args.output.resolve() / args.full, build, args.build_dir.resolve(), args.full != 'OFF')
        return
    if args.scoped is None or args.safeguards is None:
        parser.error('the passed Part C scoped and safeguard receipts are required')
    campaign(args.output.resolve(), args.build_dir.resolve(), args.scoped, args.safeguards)


if __name__ == '__main__':
    main()
