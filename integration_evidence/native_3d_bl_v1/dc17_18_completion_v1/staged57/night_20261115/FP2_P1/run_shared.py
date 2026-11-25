"""Run the five FP2-P1 shared-engine safeguards using their existing validators."""
from argparse import ArgumentParser
from pathlib import Path
import json
import os
import runpy
import sys
import subprocess

from build_p1 import HERE, cut, verify_build

NIGHT = HERE.parent
R = NIGHT.parent
PURPOSE = R / 'purpose_query_fix_20261114'
LEGACY = PURPOSE / 'p6c_20261115'
FIRST = PURPOSE / 'run_p5_first_collapse_gate.py'
REMAINING = LEGACY / 'run_remaining_checks.py'
MONITOR = NIGHT / 'FP2_C1/scoped/build647/safeguards/run_build647_safeguards.py'
SHARED = ('first-collapse', 'exact-c492', 'merit481', 'saved-current', 'native2d')
SWITCH = 'SU2_NATIVE_CONFORMING_WITNESS'
TRACE = 'SU2_NATIVE_GEOMETRY_WORK_TRACE'
MARKER = 'NATIVE_SURFACE_CONFORMING_WITNESS enabled=1'
LIMIT = 19073 * 1024**2
SELF = Path(__file__).resolve()
SAVED_REFERENCE = NIGHT / 'N4/section6_3_after_N11/phase_N6/safeguards/acceptance_summary.json'


def replace(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def environment(closure):
    return {SWITCH: str(closure), TRACE: '0'}


def exports(settings):
    return [item for name in settings for item in ('-x', name)]


def first(root, build, closure):
    source = FIRST.read_text()
    source = replace(source, 'assert case != PURPOSE and PURPOSE in case.parents, "case_dir must be a child of purpose_query_fix_20261114"',
                     'assert case == OUTPUT')
    source = replace(source, '    env.update(clean_provenance["environment"])',
                     '    env.update(clean_provenance["environment"])\n    env.update(SETTINGS)\n'
                     '    i = argv.index("mpirun") + 1\n    argv[i:i] = EXPORTS')
    source = replace(source, 'if key in clean_provenance["environment"]', 'if key in clean_provenance["environment"] or key in SETTINGS')
    sys.argv = [str(FIRST), str(root / 'case')]
    additional = root / 'additional_source_pins.json'
    inherited = cut.read(LEGACY / 'unchanged_build494_source_pins.json')
    selected = cut.read(build / 'source_pins.json')
    # The historical extra manifest can pin an older build of the touched source.
    cut.save(additional, {**{name: digest for name, digest in inherited.items() if name not in selected},
                          str(SELF): cut.sha(SELF)})
    ns = {'__name__': 'p1_first', '__file__': str(FIRST), 'OUTPUT': root / 'case',
          'SETTINGS': environment(closure), 'EXPORTS': exports(environment(closure))}
    exec(compile(source, str(FIRST), 'exec'), ns)
    ns.update(BUILD_STATUS=build / 'build_status.json', SOURCE_PINS=build / 'source_pins.json', ADDITIONAL_PINS=additional)
    ns['main']()


def other(root, build, mode, closure):
    sys.path.insert(0, str(R))
    import run_cases as checks
    settings = environment(closure)
    status = cut.read(build / 'build_status.json')
    monitor = runpy.run_path(str(MONITOR))
    monitor['install_run_monitor'].__globals__['HERE'] = root
    original = monitor['install_run_monitor'](status['test_binary'])
    monitored = subprocess.Popen
    save = checks.save

    def command(path, record):
        if path.name == 'command.json':
            record = {**record, 'argv': list(record['argv']), 'environment': {**record['environment'], **settings}}
            if 'mpirun' in record['argv']:
                record['argv'][record['argv'].index('mpirun') + 1:record['argv'].index('mpirun') + 1] = ['--tag-output', *exports(settings)]
        if path.name == 'pins.json':
            record.update({str(item): cut.sha(item) for item in (SELF, MONITOR, REMAINING)})
        save(path, record)

    def popen(argv, *args, **kwargs):
        if isinstance(argv, (tuple, list)) and status['test_binary'] in argv and argv[0] != 'ldd':
            argv = list(argv)
            kwargs['env'] = {**kwargs.get('env', os.environ), **settings}
            if 'mpirun' in argv:
                pos = argv.index('mpirun') + 1
                argv[pos:pos] = ['--tag-output', *exports(settings)]
        return monitored(argv, *args, **kwargs)

    source = REMAINING.read_text()
    source = replace(source, 'HERE=Path(__file__).resolve().parent;R=HERE.parent.parent;REPO=R.parents[3]',
                     'HERE=OUTPUT;R=STAGED;REPO=R.parents[3]')
    source = replace(source, 'for p in Path(\'/proc\').glob(\'[0-9]*\'):', 'for p in ():')
    source = replace(source, "gate_path=HERE/'check_p6_bl.py'", "gate_path=LEGACY/'check_p6_bl.py'")
    source = source.replace("HERE.parent/'check_bl_regression.py'", "LEGACY.parent/'check_bl_regression.py'")
    source = source.replace("HERE.parent/'bl_output_comparison.json'", "LEGACY.parent/'bl_output_comparison.json'")
    source = source.replace("R/'build498_", "HERE/'build498_")
    source = source.replace("'build498'", repr(build.name))
    source = source.replace("build498_", f"{build.name}_")
    source = replace(source, "module.RESULT=HERE/'geometry_merit496_serial_assessment.json'",
                     "module.RESULT=HERE/'geometry_merit496_serial_assessment.json'\n"
                     " import inspect\n"
                     " definition=inspect.getsource(module.main).replace(\"(R / 'cases')\",\"CASE_ROOT\")\n"
                     " module.CASE_ROOT=HERE/'cases';exec(compile(definition,str(spec.origin),'exec'),module.__dict__)")
    source = replace(source, "exec(compile(text,str(path),'exec'),{'__name__':'__main__','__file__':str(path)})",
                     "text=text.replace(\"(R/'cases').glob(case+'_*')\",\"CASE_ROOT.glob(case+'_*')\")\n"
                     " text=text.replace(\"(R/'" + build.name + "_terminal_handles.json')\",\"(OUTPUT/'" + build.name + "_terminal_handles.json')\")\n"
                     " exec(compile(text,str(path),'exec'),{'__name__':'__main__','__file__':str(path),"
                     "'CASE_ROOT':HERE/'cases','OUTPUT':HERE})")
    source = replace(source, "gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)",
                     "gate_source=gate_path.read_text();gate=importlib.util.module_from_spec(spec)\n"
                     "   gate_source=gate_source.replace(\"k!='SU2_NATIVE_FROZEN_CONFIG'\","
                     "\"k not in ('SU2_NATIVE_FROZEN_CONFIG','SU2_NATIVE_CONFORMING_WITNESS','SU2_NATIVE_GEOMETRY_WORK_TRACE')\")\n"
                     "   exec(compile(gate_source,str(gate_path),'exec'),gate.__dict__)")
    # Legacy helpers expect their build aliases beside the fresh output.
    cut.save(root / 'build_status.json', status)
    cut.save(root / 'source_pins.json', cut.read(build / 'source_pins.json'))
    checks.ROOT = root
    checks.save = command
    sys.argv = [str(REMAINING), mode]
    subprocess.Popen = popen
    try:
        exec(compile(source, str(REMAINING), 'exec'), {'__name__': '__main__', '__file__': str(REMAINING),
             'OUTPUT': root, 'STAGED': R, 'LEGACY': LEGACY})
    finally:
        subprocess.Popen = original
        checks.save = save


def markers(logs, closure):
    entered, absent, mismatches = [], [], []
    for rank, path in sorted(logs.items()):
        count = 0
        with path.open(errors='replace') as stream:
            for line in stream:
                count += line.count(MARKER)
        (entered if count else absent).append(rank)
        if count not in ((0, 1) if closure else (0,)):
            mismatches.append({'rank': rank, 'count': count})
    return {'entered': entered, 'no_entry_marker': absent, 'mismatches': mismatches,
            'paths': {rank: str(path) for rank, path in logs.items()}}


def caught(log, ranks, closure):
    import re
    counts = dict.fromkeys(range(ranks), 0)
    assertions = []
    with log.open(errors='replace') as stream:
        for line in stream:
            tag = re.match(r'\[\d+,(\d+)\]<[^>]+>:', line)
            if MARKER in line:
                counts[int(tag[1]) if tag else 0] += line.count(MARKER)
            if 'All tests passed' in line:
                assertions.append(line.strip())
    return {'entered': [r for r, n in counts.items() if n], 'no_entry_marker': [r for r, n in counts.items() if not n],
            'mismatches': [{'rank': r, 'count': n} for r, n in counts.items() if n not in ((0, 1) if closure else (0,))],
            'catch': assertions, 'path': str(log)}


def evaluate(root, mode, closure, wait):
    if mode == 'first-collapse':
        case = root / 'case'
        raw = case / 'status.json'
        r = cut.read(raw)
        g = r['first_collapse_gate']
        files = {int(p.parent.name.split('.')[-1]): p for p in case.glob('rank_output/*/rank.*/stderr')}
        receipt = markers(files, closure)
        expected = g['accepted_by_rank_expected']
        edits = next(({'field': 'accepted_by_rank', 'rank': row['rank'], 'expected': expected[int(row['rank'])],
                       'actual': int(row['accepted'])} for row in g['per_rank']
                      if int(row['accepted']) != expected[int(row['rank'])]), None)
        failed = next((k for k in ('all_six_captured', 'upper_work_pass', 'upper_refinements_pass') if not g[k]), None)
        failed = failed or next((k for k in ('all_owned_handles_terminal', 'inputs_unchanged', 'source_and_binary_pins_unchanged',
                                             'native_case_started') if not r[k]), None)
        rank_peaks = {}
        with (case / 'host_resources.jsonl').open() as stream:
            for line in stream:
                for row in json.loads(line)['owned_processes']:
                    if row['comm'] == 'SU2_CFD':
                        ident = (row['pid'], row['start_ticks'])
                        rank_peaks[ident] = max(rank_peaks.get(ident, 0), row.get('VmHWM', 0), row.get('VmRSS', 0))
        receipt['observed_solver_peak_sum_bytes'] = sum(rank_peaks.values())
        failed = failed or ('observed solver peak sum' if receipt['observed_solver_peak_sum_bytes'] > LIMIT else None)
        failed = failed or ('switch markers' if (len(files) != 6 or receipt['mismatches']) else None)
        if not failed and not (edits and closure) and wait['returncode']:
            failed = edits or 'controller exit'
        return raw, receipt, failed, edits if closure else None
    case, = (root / 'cases').glob('*')
    raw = case / 'status.json'
    r = cut.read(raw)
    command = cut.read(case / 'command.json')
    receipt = caught(case / 'run.log', command['ranks'], closure)
    assert all(command['environment'][k] == v for k, v in environment(closure).items())
    monitor = cut.read(case / 'safeguard_process_receipt.json')
    failed = next((k for k, ok in (
        ('test exit', r['exit_code'] == 0), ('pins_verified', r['pins_verified']),
        ('owned handles terminal', monitor['state'] == 'terminal' and not monitor['live_owned_descendants']),
        ('job peak', monitor['peak_sampled_tree_rss_bytes'] <= LIMIT),
        ('switch markers', not receipt['mismatches'])) if not ok), None)
    missed = None
    if mode == 'bl':
        raw = case / 'p6_bl_receipt.json'
        if raw.is_file():
            bl = cut.read(raw)
            failed = failed or next((k for k, ok in bl['gates'].items() if not ok), None)
        else:
            raw = case / 'status.json'
            failed = failed or 'missing exact C492 validator result'
    if mode in ('merit481', 'saved_current') and not failed and not wait['returncode']:
        old = cut.read(SAVED_REFERENCE)['safeguards'][mode]
        reference = Path(old['case'])
        sys.path.insert(0, str(NIGHT / 'N16'))
        from run_c import compare_safeguard
        compared = compare_safeguard(mode, reference, case)
        assert all(row['reference'] == old['payloads'][name]['candidate'] for name, row in compared['payloads'].items())
        raw = root / 'historical_comparison.json'
        cut.save(raw, compared)
        if not compared['exact'] and (closure or compared['require_identity']):
            first = next(({'field': name, **row} for name, row in compared['payloads'].items()
                          if row['reference'] != row['candidate']), {'field': 'actual_bl_cavity_planning.jsonl'})
            if closure:
                missed = first
            else:
                failed = first
    if not failed and wait['returncode']:
        failed = 'controller exit'
    return raw, receipt, failed, missed


def run(label, closure, group):
    verify_build(label)
    build = HERE / f'build{label}'
    root = HERE / 'shared' / build.name / f'closure{closure}' / group
    root.mkdir(parents=True, exist_ok=True)
    plan = {'first-collapse': ['first-collapse'], 'exact-c492': ['bl'], 'merit481': ['merit481'],
            'saved-current': ['saved_current'], 'native2d': ['2d1', '2d2', '2d4']}
    results = {}
    for mode in plan[group]:
        attempt = root / mode
        attempt.mkdir()
        try:
            wait = cut.run_gate(attempt, 'controller', [sys.executable, '-B', str(SELF), str(label), str(closure), group,
                                                        '--worker', mode])
        except RuntimeError:
            wait = cut.read(attempt / 'controller_wait.json')
        try:
            raw, receipt, failed, missed = evaluate(attempt, mode, closure, wait)
        except (OSError, KeyError, ValueError) as exc:
            raw, receipt, failed, missed = attempt / 'controller_outer.log', {}, f'missing evaluation: {exc}', None
        part = 'FAILED' if failed else 'OPEN' if missed else 'passed'
        results[mode] = {'status': part, 'raw': str(raw), 'raw_sha256': cut.sha(raw), 'first_failure': failed,
                         'missed_comparison': missed, 'switch_receipt': receipt, 'wait': wait}
    verify_build(label)
    fingerprint = {key: cut.read(build / 'build_status.json')[key] for key in
                   ('build', 'binary_sha256', 'test_binary_sha256', 'source_pins_sha256')}
    files = {Path(part['raw']) for part in results.values()}
    files.update((SELF, FIRST, REMAINING, MONITOR, SAVED_REFERENCE, NIGHT / 'N16/run_c.py', build / 'build_status.json', build / 'source_pins.json'))
    files.update(root.glob('*/controller_*.json'))
    files.update(root.glob('*/cases/*/status.json'))
    files.update(root.glob('*/cases/*/pins.json'))
    files.update(root.glob('*/cases/*/command.json'))
    files.update(root.glob('*/cases/*/safeguard_process_receipt.json'))
    files.update(root.glob('*/case/provenance.json'))
    pins = {str(path): cut.sha(path) for path in sorted(files)}
    status = 'FAILED' if any(p['status'] == 'FAILED' for p in results.values()) else (
        'OPEN' if any(p['status'] == 'OPEN' for p in results.values()) else 'passed')
    cut.save(root / 'case_summary.json', {**fingerprint, 'status': status, 'environment': environment(closure), 'pins': pins,
                                         'subruns': results})
    print(group, status, flush=True)
    return 1 if status == 'FAILED' else 0


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    parser.add_argument('closure', type=int, choices=(0, 1))
    parser.add_argument('group', choices=SHARED)
    parser.add_argument('--worker')
    args = parser.parse_args()
    if args.worker:
        build = HERE / f'build{args.build}'
        root = HERE / 'shared' / build.name / f'closure{args.closure}' / args.group / args.worker
        if args.worker == 'first-collapse':
            first(root, build, args.closure)
        else:
            other(root, build, args.worker, args.closure)
    else:
        raise SystemExit(run(args.build, args.closure, args.group))
