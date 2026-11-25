"""Execute inherited G3, retaining physical checks and projecting its edit evidence."""
import os
from pathlib import Path
import re
import runpy
import sys
from types import SimpleNamespace
from unittest.mock import patch

from build_p3 import HERE, cut, verify, verify_build
from run_prefix_p3 import MARKER, ORDER, SOURCE_CHECK, SWITCH, TRACE, bind_once

N = HERE.parent
SCOPE = cut.PHASE / 'N22C/validation'
G3 = SCOPE / 'run_g3_591.py'
END = SCOPE / 'validate_end_b.py'
N11_PHYSICAL = N / 'N11/validation/validate_n11.py'
WATCHDOG = 900
BL_GUARD = N.parent / 'purpose_query_fix_20261114/check_bl_regression.py'
C492 = N.parent / 'cases/curved492_saved_production_surface_serial_20261113T163756_3724702'
CASE = 'FP2P3_G3'


def rejoin(case, closure, required):
    raw = case / 'run.log'
    octets = raw.read_bytes()
    marker = (MARKER + '\n').encode()
    count = octets.count(marker)
    assert count in ({1} if closure == 0 and required else {0, 1} if closure == 0 else {0}), (
        case, closure, count)
    if closure == 0 and count:
        i = octets.index(marker)
        if i and octets[i - 1:i] != b'\n':
            derived = case / 'stdout_without_interleaved_marker.log'
            projection = octets[:i] + octets[i + len(marker):]
            if derived.is_file():
                assert derived.read_bytes() == projection
            else:
                derived.write_bytes(projection)
            cut.save(case / 'stdout_rejoined.json', {'source': str(raw), 'source_sha256': cut.sha(raw),
                     'path': str(derived), 'sha256': cut.sha(derived), 'marker_byte_offset': i})
            return derived, count
    return raw, count


def switch(case, closure, timeout, required):
    command = cut.read(case / 'command.json')
    env = command['environment']
    assert env.get(SWITCH) == ('0' if closure == 0 else None) and env.get(TRACE) == '0', env
    if closure:
        assert SWITCH not in env
    assert command['ranks'] == 1 and command['timeout_seconds'] == timeout
    argv = command['argv']
    assert argv[argv.index('timeout') + 1] == str(timeout) and 'mpirun' not in argv
    parsed, count = rejoin(case, closure, required)
    return parsed, {'environment': {SWITCH: env.get(SWITCH), TRACE: env[TRACE]}, 'rank_marker_counts': {0: count},
                    'log': str(parsed), 'raw': str(case / 'run.log')}


def physical(case, closure, timeout, partial=False):
    parsed, _ = switch(case, closure, timeout, not partial)
    physical_ns = runpy.run_path(str(N11_PHYSICAL))
    guard_source = physical_ns['guard_source']
    load_guard = physical_ns['load_physical_guard']

    def guard():
        return bind_once(guard_source(), "k!='SU2_NATIVE_FROZEN_CONFIG'",
                         f'k not in {("SU2_NATIVE_FROZEN_CONFIG", SWITCH, TRACE)!r}')

    def load():
        namespace = load_guard()
        prior = namespace['prior']
        original = prior.conditions

        def conditions(status, command, row, identical):
            gates = original(status, command, row, identical)
            argv = command['argv']
            gates['serial_bounded_command'] = (command.get('ranks') == 1
                and command.get('timeout_seconds') == timeout and argv[argv.index('timeout') + 1] == str(timeout))
            return gates

        prior.conditions = conditions
        return namespace

    original_read = Path.read_text

    def projected_read(path, *args, **kwargs):
        return original_read(parsed if path == case / 'run.log' else path, *args, **kwargs)

    ns = physical_ns['physical_case'].__globals__
    with patch.dict(ns, guard_source=guard, load_physical_guard=load), patch.object(Path, 'read_text', projected_read):
        return physical_ns['physical_case'](case)


def evaluate(case, terminal, closure, timeout):
    parsed, marker = switch(case, closure, timeout, True)
    namespace = {'__name__': 'p3_endb_g3', '__file__': str(END)}
    source = END.read_text()
    source = bind_once(source, '    assert mismatch is None, mismatch\n', '')
    source = bind_once(source, '    assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]\n', '')
    source = bind_once(source, '        "passed": True,\n        "case": str(case),',
                       '        "passed": True,\n        "case": str(case),'
                       '\n        "first_mismatch": None if mismatch is None else '
                       '{"index": mismatch, "expected": ref_edits[mismatch], "actual": new_edits[mismatch]},')
    exec(compile(source, str(END), 'exec'), namespace)
    original_read = Path.read_text

    def projected_read(path, *args, **kwargs):
        return original_read(parsed if path == case / 'run.log' else path, *args, **kwargs)

    with patch.object(Path, 'read_text', projected_read):
        report = namespace['g3_result'](case, terminal)
    first = report['first_mismatch']
    work = report['observer_adjusted_entry_work']
    if first is None and work['reference'] != work['candidate']:
        first = {'field': 'observer_adjusted_entry_work', 'expected': work['reference'], 'actual': work['candidate']}
    report.update(passed=closure == 1 or first is None, first_difference=first,
                  identity_is_gate=closure == 0, switch_receipt=marker)
    return report


def owned(build, root, closure, timeout, supporting):
    label = int(build.name.removeprefix('build'))
    archived = {'__file__': str(G3), '__name__': 'p3_g3_inherited'}
    source = G3.read_text().replace('build591', f'build{label}')
    exec(compile(source, str(G3), 'exec'), archived)
    inherited = archived['run_or_stub'].__globals__
    receipt = verify_build(label)
    pins = cut.read(build / 'source_pins.json')
    identities = [{'alive': False, **receipt['wait']['controller']}]
    terminal = {'actual_host_proc': True, 'all_owned_handles_terminal': True, 'identities': identities}
    local = {'status': receipt, 'pins': pins, 'handles': terminal, 'primary_terminal': {**terminal, 'passed': True}}
    identity = cut.actual_identity(os.getpid())
    prereq_receipts = (ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK, G3, END, Path(__file__).resolve(), *supporting)
    cut.save(root / 'g3_primary_launch_identity.json', {'actual_host_proc': True, 'controller': identity,
             'argv': sys.argv, 'closure': 'absent' if closure else '0', 'trace': '0', 'seconds_limit': timeout,
             'host_preflight_sha256': cut.sha(root / 'G3_host_preflight.json')})
    (root / 'launch_g3.py').symlink_to(Path(__file__).resolve())
    (root / 'validate_end_b.py').symlink_to(END)
    for filename, value in (
        ('aliases/terminal_handles.json', terminal), ('aliases/primary_terminal_receipt.json', local['primary_terminal']),
        ('aliases/generated_build_graph_after.json', {
            'build_status_sha256': cut.sha(build / 'build_status.json'),
            'source_pins_sha256': receipt['source_pins_sha256'], 'after': receipt['graph_after']}),
    ):
        target = root / filename
        target.parent.mkdir(exist_ok=True)
        cut.save(target, value)
    data = root / 'aliases'
    for name in ('build_status.json', 'source_pins.json'):
        (data / name).symlink_to(build / name)

    original_house = inherited['load_house']
    original_module = inherited['load_module']
    original_environment = inherited['apply_reference_environment']
    original_config = inherited['configure_house']

    def house(workspace, status):
        env = original_house(workspace, status)
        if timeout != WATCHDOG:
            original = env['main']
            text = str(inherited['HOUSE'])
            source = Path(text).read_text()
            source = bind_once(source, 'timeout_seconds = 2400 if case == "m6_stage" else 600',
                               f'timeout_seconds = 2400 if case == "m6_stage" else {timeout}')
            old = {key: value for key, value in env.items() if not key.startswith('__')}
            exec(compile(source, text, 'exec'), env)
            env.update({key: old[key] for key in ('ROOT', 'REPO', 'BINARY', 'CASES')})
            assert original is not env['main']
        return env

    def environment():
        for key in tuple(os.environ):
            if key.startswith('SU2_NATIVE_'):
                os.environ.pop(key)
        original_environment()
        os.environ.pop(SWITCH, None)
        if closure == 0:
            os.environ[SWITCH] = '0'
        os.environ[TRACE] = '0'

    def configure(env, paths):
        return original_config(env, [*paths, *prereq_receipts])

    def module(name, path):
        if name == 'endb_g3_physical':
            return SimpleNamespace(physical_case=lambda case: physical(case, closure, timeout))
        if name == 'endb_g3_gate':
            return SimpleNamespace(g3_result=lambda case, term: evaluate(case, term, closure, timeout))
        return original_module(name, path)

    inherited.update(HERE=root, BUILD=data, CASE=CASE, verify_build=lambda: local,
                     load_house=house, load_module=module, apply_reference_environment=environment,
                     configure_house=configure)
    result = archived['run_or_stub'](host_path=root / 'G3_host_preflight.json', stub=False)
    print(result, flush=True)
    return 0 if result['passed'] else 1


def read_attempt(root, closure, timeout):
    case, = (root / 'cases').glob(CASE + '_*')
    status = cut.read(case / 'status.json')
    wait = cut.read(root / 'G3_wait.json')
    terminal = cut.read(case / 'g3_terminal_receipt.json')
    assert wait['terminal'] and cut.terminal(wait['controller'])
    assert terminal['all_owned_handles_terminal'] and all(cut.terminal(row) for row in terminal['identities'])
    assert status['state'] == 'terminal' and status['pins_verified'] and not status['changed']
    verify(cut.read(case / 'pins.json'))
    paths = [case / name for name in ('command.json', 'status.json', 'g3_terminal_receipt.json', 'pins.json', 'run.log')]
    paths += [root / 'G3_wait.json', root / 'G3_host_preflight.json', root / 'g3_primary_launch_identity.json']
    parsed, switches = switch(case, closure, timeout, status['exit_code'] == 0)
    def finish(state, first=None, result=None):
        for name in ('stdout_rejoined.json', 'stdout_without_interleaved_marker.log',
                     'cheap_bl_receipt.json', 'g3_end_b_gate.json', 'nose_target_observation.jsonl',
                     'actual_saved_surface_stage_rank0.json', 'resources.jsonl'):
            if (case / name).is_file():
                paths.append(case / name)
        return {'status': state, 'case': str(case), 'seconds_limit': timeout, 'switch': switches,
                'first_difference': first, 'result': result, 'pins': {str(path): cut.sha(path) for path in paths}}

    if not terminal['sampled_process_tree_within_cap']:
        return finish('FAILED', {'field': 'sampled_process_tree_peak_rss_bytes',
                                 'observed': terminal['sampled_process_tree_peak_rss_bytes'],
                                 'limit': terminal['sampled_process_tree_cap_bytes']})
    gate = case / 'g3_end_b_gate.json'
    if gate.is_file():
        result = cut.read(gate)
        return finish('passed' if result['passed'] else 'FAILED',
                      result['first_difference'] if not result['passed'] else None, result)
    cheap = case / 'cheap_bl_receipt.json'
    if cheap.is_file():
        data = cut.read(cheap)
        failed = {key: value for key, value in data['gates'].items() if not value}
        if failed:
            return finish('FAILED', {'physical_gates': failed}, data)
        if data['actual_splits'] < 336:
            return finish('FAILED', {'field': 'accepted_splits', 'expected_minimum': 336,
                                     'observed': data['actual_splits']}, data)
    if status['exit_code'] != 124:
        raise AssertionError(f'missing G3 evaluator receipt after exit {status["exit_code"]}: {case}')
    log = parsed.read_text(errors='replace')
    for line in log.splitlines():
        if 'NATIVE3D_STAGE ' not in line or 'stage=exit ' not in line:
            continue
        row = dict(re.findall(r'(\w+)=([^\s]+)', line))
        if int(row.get('upper_refinements', 0)) or int(row.get('accepted', 0)) < 336:
            return finish('FAILED', {'visible_stage_exit': row})
    # Catch can attribute a final failure to the wrapper SIGTERM. Keep earlier assertions.
    signal_only = re.search(r'FAILED:\n\s+\{Unknown expression after the reported line\}\n'
                            r'due to a fatal error condition:\n\s+SIGTERM - Termination request signal\n', log)
    if signal_only and 'Command exited with non-zero status 124' in log[signal_only.end():]:
        log = log[:signal_only.start()] + re.sub(
            r'^(?:test cases|assertions):[^\n]*\| +1 failed[ \t]*$', '',
            log[signal_only.end():], flags=re.MULTILINE)
    if re.search(r'\bFAILED:|(?:test cases|assertions):.*failed', log):
        return finish('FAILED', {'field': 'Catch assertion'})
    row_path = case / 'actual_saved_surface_stage_rank0.json'
    if row_path.is_file():
        validator = runpy.run_path(str(BL_GUARD))
        command = cut.read(case / 'command.json')
        row = cut.read(row_path)
        checks = validator['conditions'](status, command, row, {})
        retained = ('runtime_pins_verified', 'exact_input_and_target', 'unchanged_stage_limits',
                    'all_private_cells_positive_with_sensors', 'modeled_storage_within_12GiB')
        excluded = (SWITCH, TRACE, 'SU2_NATIVE_FROZEN_CONFIG')
        def frozen(env):
            return {k: v for k, v in env.items() if k.startswith('SU2_NATIVE_') and k not in excluded}
        checks = {k: checks[k] for k in retained}
        checks.update(at_least_336_splits=int(row['splits']) >= 336,
                      same_frozen_input_contract=frozen(command['environment']) ==
                      frozen(cut.read(C492 / 'command.json')['environment']),
                      same_frozen_configuration=(case / 'run.cfg').read_bytes() == (C492 / 'run.cfg').read_bytes())
        if not all(checks.values()):
            return finish('FAILED', {'visible_stage_gates': checks})
    return finish('OPEN', {'field': 'outer watchdog', 'seconds': timeout})


def run(build, root, closure, supporting):
    attempt_results = []
    for i, timeout in enumerate((WATCHDOG, 3 * WATCHDOG), 1):
        attempt = root / f'attempt{i}'
        attempt.mkdir(parents=True, exist_ok=True)
        if not (attempt / 'G3_wait.json').is_file():
            from checkpoint import host_preflight
            host_preflight()
            try:
                cut.run_gate(attempt, 'G3', [cut.PYTHON, '-B', str(Path(__file__).resolve()), str(build),
                    str(attempt), str(closure), str(timeout), *map(str, supporting)])
            except RuntimeError:
                if not (attempt / 'G3_wait.json').is_file():
                    raise
        result = read_attempt(attempt, closure, timeout)
        attempt_results.append(result)
        if result['status'] != 'OPEN':
            break
    return {**attempt_results[-1], 'attempts': attempt_results}


if __name__ == '__main__':
    build, root, closure, timeout, *supporting = sys.argv[1:]
    raise SystemExit(owned(Path(build), Path(root), int(closure), int(timeout), list(map(Path, supporting))))
