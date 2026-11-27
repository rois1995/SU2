"""Run and seal one FP3-D full workflow with the inherited standing gates."""

from argparse import ArgumentParser
from contextlib import contextmanager
from decimal import Decimal
from pathlib import Path
import json
import re
import runpy
import subprocess
import sys
from tempfile import TemporaryDirectory
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
A = HERE.parent / 'FP3_A'
B = HERE.parent / 'FP3_B'
P3 = HERE.parent / 'FP2_P3'
sys.path.insert(0, str(P3))
import run_full as p3

SELF = Path(__file__).resolve()
ORDER = HERE / 'inputs/FP3_D_EXCHANGE_ON_CELL_IMBALANCE.md'
ORDER_SHA = '57482de37bd4f8a13f7299a0dcef6e9a5b4ba30da3826dc007da2b66c9da8f15'
A_ORDER = A / 'inputs/FP3_A_NO_ENGINE_COLLAPSE.md'
B_ORDER = B / 'inputs/FP3_B_NO_EXCHANGE_AT_EMPTY_EXIT.md'
B_REFERENCE = B / 'accepted_reference.json'
P3_REFERENCE = P3 / 'accepted_reference.json'
BASE_COMMIT = '186f9ab790'
PROBE = 'SU2_NATIVE_FP3_PROBE'
PARTS = {'S1': ('S', 0), 'S2': ('S', 0), 'probe3': ('S', 3)}
PROBE_OBSERVATION = 'the exchange rule of FP3-B'
cut, sha, workflow, c3 = p3.cut, p3.sha, p3.workflow, p3.C3
write = workflow.sealed_write
base = p3.BASE
FINGERPRINTS = p3.FINGERPRINTS
restart_source = p3.restart_source
core_source = p3.core_source


class FailedCheck(RuntimeError):
    """A recorded failure that can gate acceptance or an observation."""

    def __init__(self, kind, first):
        self.kind, self.first = kind, first
        super().__init__(str(first))


def root(label, part):
    return HERE / 'G5' / f'build{label}' / part


def path_pin(path):
    return {'path': str(path), 'sha256': sha(path)}


def check_pins(pins):
    for path, digest in pins.items():
        assert sha(Path(path)) == digest, path


def settings(part):
    return {p3.TRACE: '0', PROBE: str(PARTS[part][1])}


def bound_core(part, retry=False):
    source = core_source(part, retry)
    anchor = '            elapsed = time.monotonic() - start\n            receipts.write('
    observation = '''
            probe_path = case / "fp3_rank_probe_environment.json"
            if not probe_path.is_file():
                probe_by_rank = {}
                for row in sampled:
                    if row["comm"] != "SU2_CFD":
                        continue
                    try:
                        values = dict(entry.split(b"=", 1) for entry in
                                      (Path("/proc") / str(row["pid"]) / "environ").read_bytes().split(b"\\0")
                                      if entry.startswith((b"OMPI_COMM_WORLD_RANK=", b"SU2_NATIVE_FP3_PROBE=")))
                    except OSError:
                        continue
                    rank = values.get(b"OMPI_COMM_WORLD_RANK", b"")
                    if rank.isdigit():
                        value = values.get(b"SU2_NATIVE_FP3_PROBE")
                        probe_by_rank[int(rank)] = {"rank": int(rank), "pid": row["pid"],
                                                  "start_ticks": row["start_ticks"],
                                                  "probe": value.decode() if value is not None else None}
                if set(probe_by_rank) == set(range(6)):
                    save(probe_path, {"ranks": [probe_by_rank[rank] for rank in range(6)]})
'''
    return workflow.replace_once(source, anchor, observation.lstrip('\n') + anchor)


def bind_restart_failure(source):
    anchor = '    exec(compile(source, str(BASE), "exec"), namespace)\n'
    return workflow.replace_once(source, anchor, '''    try:
        exec(compile(source, str(BASE), "exec"), namespace)
    except AssertionError as error:
        entry = error.__traceback__
        while entry and entry.tb_frame.f_code.co_filename != str(BASE):
            entry = entry.tb_next
        if entry:
            statement = source.splitlines()[entry.tb_lineno - 1].strip()
            result_checks = (
                'assert len(mesh.P)', 'assert mesh.P.shape', 'assert coordinate_error',
                'assert np.all(rho', 'assert max(changes.values())',
                'assert sha(D / "continued508_restart.dat")',
            )
            if statement.startswith(result_checks):
                values = {key: namespace[key] for key in ("coordinate_error", "changes") if key in namespace}
                values.update({f"{key}_min": float(namespace[key].min())
                               for key in ("rho", "internal") if key in namespace})
                if "mesh" in namespace:
                    values["points"], values["cells"] = len(namespace["mesh"].P), len(namespace["mesh"].E)
                failure = {"assertion": statement, "line": entry.tb_lineno,
                           "expected": True, "actual": False, "values": values}
                (case / "fp3_restart_result_failure.json").write_text(json.dumps(failure) + "\\n")
        raise
''')


def bound_restart(part, name, label, build_dir):
    template, source = restart_source(part, name, label, build_dir)
    if name == 'validate_restart':
        return template, bind_restart_failure(source)
    # Use the qualified P3 observation correction before launching any restart.
    inherited = '''            except (OSError,RuntimeError,ValueError) as exc:
                # Preserve the failed observation while continuing to own/reap the bounded CFD job.
                runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})'''
    corrected = r'''            except OSError as exc:
                if exc.errno == __import__("errno").EIO:
                    with (FOLDER / "runtime_sample_eio.jsonl").open("a") as observations:
                        observations.write(json.dumps({"time": time.time(), "error": str(exc)}) + "\n")
                else:
                    runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})
            except (RuntimeError,ValueError) as exc:
                runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})'''
    anchor = '            compile(source, str(BASE), "exec")\n'
    addition = f'            old_observation, new_observation = {inherited!r}, {corrected!r}\n'
    addition += '            assert source.count(old_observation) == 1, old_observation\n'
    addition += '            source = source.replace(old_observation, new_observation, 1)\n'
    old_probe = '    return {"pid": pid, "start_ticks": proc_stat(pid)["start_ticks"],'
    new_probe = '''    values = (proc / "environ").read_bytes().split(b"\\0")
    probe = next((entry.split(b"=", 1)[1].decode() for entry in values
                  if entry.startswith(b"SU2_NATIVE_FP3_PROBE=")), None)
    return {"pid": pid, "start_ticks": proc_stat(pid)["start_ticks"], "probe": probe,'''
    addition += f'            old_probe, new_probe = {old_probe!r}, {new_probe!r}\n'
    addition += '            assert source.count(old_probe) == 1, old_probe\n'
    addition += '            source = source.replace(old_probe, new_probe, 1)\n'
    return template, workflow.replace_once(source, anchor, addition + anchor)


@contextmanager
def bound():
    with patch.multiple(p3, HERE=HERE, SELF=SELF, settings=settings, core_source=bound_core,
                        restart_source=bound_restart,
                        MODES={part: mode for part, (mode, _) in PARTS.items()}):
        yield


def effective_receipt(part, case, command, *, restart=False):
    record = cut.read(command)
    env, argv = record['environment'], record['argv']
    mpi = argv.index('mpirun')
    assert argv[argv.index('-n', mpi + 1) + 1] == '6', command
    assert p3.SWITCH not in env and p3.SWITCH not in argv, command
    for key, value in settings(part).items():
        assert env.get(key) == value and argv.count(key) == 1, (command, key)
        assert argv[argv.index(key) - 1] == '-x' and argv.index(key) > mpi, (command, key)
    logs = {int(path.parent.name.split('.')[-1]): path
            for path in case.glob('rank_output/*/rank.*/stderr')}
    assert set(logs) == set(range(6)), case
    output = [path for log in logs.values() for path in (log, log.with_name('stdout'))]
    for path in output:
        assert p3.MARKER not in path.read_text(errors='replace'), path
    live = case / ('loaded_runtime_startup.json' if restart else 'fp3_rank_probe_environment.json')
    live_ranks = cut.read(live)['ranks']
    assert [row['rank'] for row in live_ranks] == list(range(6)), live
    assert all(row['probe'] == settings(part)[PROBE] for row in live_ranks), live_ranks
    if restart:
        assert re.search(r'(?m)^ADAP_LOOP\s*=\s*NO\s*$', (case / 'run.cfg').read_text()), case
    return {'effective_environment': {p3.SWITCH: None, **settings(part)}, 'command': path_pin(command),
            'live_rank_environment': path_pin(live),
            'rank_count': len(logs), 'mpi_exports': list(settings(part)),
            'logs': {str(path): sha(path) for path in output}}


def verify_order():
    assert sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    for order in (A_ORDER, B_ORDER):
        assert order.read_bytes() == committed_bytes(order), order
        checksum = order.with_suffix('.sha256')
        assert checksum.read_bytes() == committed_bytes(checksum), checksum
        assert checksum.read_text().split() == [sha(order), order.name], checksum
    p3.verify_order()


def committed_bytes(path):
    return subprocess.check_output(
        ['git', 'show', f'{BASE_COMMIT}:{path.relative_to(cut.REPO)}'], cwd=cut.REPO)


def reference_runs(reference, payloads):
    """Authenticate both accepted S runs and their common six-file reference."""
    label = {B_REFERENCE: 676, P3_REFERENCE: 673}[reference]
    assert reference.read_bytes() == committed_bytes(reference), reference
    adopted = cut.read(reference)
    prior = reference.parent / f'build{label}/build_status.json'
    assert prior.read_bytes() == committed_bytes(prior), prior
    build = cut.read(prior)
    assert adopted['accepted'] and adopted['build'] == build['build'] == label and build['passed']
    assert {key: adopted[key] for key in FINGERPRINTS} == {key: build[key] for key in FINGERPRINTS}
    runs = adopted['workflows']['S']['runs']
    assert len(runs) == 2 and runs[0]['payload_hashes'] == runs[1]['payload_hashes']
    pins = [reference, prior]
    for index, run in enumerate(runs, 1):
        if reference == B_REFERENCE:
            pointer = adopted['gate_receipts'][f'S{index}']
            path = Path(pointer['path'])
            assert path_pin(path) == pointer and path.read_bytes() == committed_bytes(path), path
            gate = cut.read(path)
            assert gate['status'] == 'passed' and gate['run'] == run['full_run_receipt'], path
            if index == 2:
                assert gate['comparison']['equal'], path
            pins.append(path)
        case = Path(run['case'])
        for key in ('effective_config', 'full_run_receipt', 'isolated_restart_receipt'):
            pointer = run[key]
            path = Path(pointer['path'])
            assert sha(path) == pointer['sha256'], path
            pins.append(path)
        assert sha(p3.config_for('S')) == run['effective_config']['sha256']
        receipt_path = Path(run['full_run_receipt']['path'])
        receipt = p3.reference_run(receipt_path, 'S', build, payloads)
        assert receipt['hashes'] == run['payload_hashes'] and receipt['case'] == str(case)
        assert Path(receipt['restart']) == Path(run['isolated_restart_receipt']['path'])
        assert run['native_maximum_seconds'] == {str(row['event']): row['native_seconds'] for row in receipt['events']}
        restart = cut.read(Path(run['isolated_restart_receipt']['path']))
        assert restart['disk_restart_verified'] and restart['all_inputs_and_pins_unchanged']
        host_resources = case / 'host_resources.jsonl'
        if str(host_resources.relative_to(receipt_path.parent)) not in receipt['artifacts']:
            assert host_resources.read_bytes() == committed_bytes(host_resources), host_resources
            pins.append(host_resources)
        pins += [receipt_path.parent / artifact for artifact in receipt['artifacts']]
    return runs, pins


def require(path, build):
    receipt = cut.read(path)
    assert all(receipt[key] == build[key] for key in FINGERPRINTS), path
    assert receipt['status'] == 'passed', path
    check_pins(receipt['pins'])
    return receipt


def previous(label, part, build, payloads):
    step = HERE / f'build{label}' / 'step1.json'
    require(step, build)
    if part == 'S1':
        return [step], None

    predecessor = 'S1' if part == 'S2' else 'S2'
    summary = root(label, predecessor) / 'case_summary.json'
    row = require(summary, build)
    if part == 'probe3':
        assert row['comparison']['equal'], summary
    run = Path(row['run']['path'])
    assert sha(run) == row['run']['sha256']
    old = authenticate(run.parent, label, predecessor, build, payloads)
    pins = [step, summary, run, *(run.parent / name for name in old['artifacts'])]
    return pins, old['hashes']


def authenticate(work, label, part, build, payloads):
    mode, probe = PARTS[part]
    if not probe:
        with patch.dict(base['verify_run'].__globals__, {'config_for': p3.config_for}):
            row = base['verify_run'](work, mode, build, payloads)
    else:
        row = cut.read(work / 'run_receipt.json')
        workflow.verify_helpers(work)
        c3.terminal_wait(work, 'full', 0)
        case = Path(row['case'])
        base['status_ok'](case, build, p3.config_for(mode))
        assert row['hashes'] == workflow.workflow_payloads(case, build['binary_sha256'], payloads)
        check_pins({str(work / name): digest for name, digest in row['artifacts'].items()})
    assert all(row[key] == build[key] for key in FINGERPRINTS)
    assert row['probe'] == probe and row['mode'] == mode
    assert row['case'] == str(work / f'full{label}_run1')
    assert set(row['switches']) == ({'full'} if probe else {'full', 'restart'}), row['switches']
    if not probe:
        c3.terminal_wait(work, 'restart', 0)
        post = cut.read(Path(row['restart']).parent / 'post_terminal_handles.json')
        assert post['actual_host_proc'] and post['all_owned_handles_terminal'], post
        assert all(cut.terminal(entry['identity']) for entry in post['owned_identity_checks']), post
    for switch in row['switches'].values():
        assert switch['effective_environment'] == {p3.SWITCH: None, **settings(part)}, switch
        assert switch['mpi_exports'] == list(settings(part)) and switch['rank_count'] == 6, switch
    return row


def collect(work, case, part, *, partial=False, events=None):
    from collect_fp3_full import collect as collect_case

    probe = PARTS[part][1]
    result = collect_case(case, probe, partial=partial, event_receipts=events)
    destination = work / ('partial_collection.json' if partial else 'collection.json')
    write(destination, json.loads(json.dumps(result)))
    return destination, result


def envelope(work, label, part, build, events, hashes, restart, collected):
    mode, probe = PARTS[part]
    case = work / f'full{label}_run1'
    launch = case / 'provenance.json'
    config = p3.config_for(mode)
    switches = {'full': effective_receipt(part, case, launch)}
    required = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                        'full_host_preflight.json')]
    required += [case / name for name in ('provenance.json', 'status.json', 'run.cfg', 'full_workflow_receipt.json',
                                          'n8_payload_identity.json', 'accepted_reference_payloads.json',
                                          'final_source_first_collapse_gate.json', 'host_resources.jsonl')]
    required += [case / f'{prefix}{event}{suffix}.json' for event in (1, 2)
                 for prefix, suffix in (('p7p8_event', '_gate'), ('f3b_event', '_timer_gate'))]
    required += sorted(case.glob('rank_output/*/rank.*/std*'))
    if restart:
        restart_case = restart.parent
        switches['restart'] = effective_receipt(part, restart_case, restart_case / 'command.json', restart=True)
        required += [work / name for name in ('restart_launch.json', 'restart_wait.json',
                                             'restart_host_preflight.json')]
        required += [restart, restart_case / 'command.json', restart_case / 'run.cfg',
                     restart_case / 'post_terminal_handles.json', restart_case / 'status.json',
                     *sorted(restart_case.glob('rank_output/*/rank.*/std*')),
                     *restart_case.glob('runtime_sample_eio.jsonl')]
        manifest = work / 'restart_attempt1_watchdog/manifest.json'
        if manifest.is_file():
            required += [manifest, work / f'run_restart{label}_1800.py',
                         work / 'restart_retry_start_load.json', work / 'restart_retry_timing.json']
    required.extend(Path(row['live_rank_environment']['path']) for row in switches.values())
    if collected:
        required.append(collected)
    workflow.verify_helpers(work)
    for binary in ('binary', 'test_binary'):
        assert sha(Path(build[binary])) == build[f'{binary}_sha256']
    row = {**{key: build[key] for key in FINGERPRINTS}, 'passed': True,
           'mode': mode, 'probe': probe, 'observation_only': bool(probe), 'case': str(case),
           'closure_variable': None, 'config': str(config), 'config_sha256': sha(config),
           'hashes': hashes, 'restart': str(restart) if restart else None, 'events': events,
           'switches': switches, 'collection': path_pin(collected) if collected else None,
           'artifacts': {str(path.relative_to(work)): sha(path) for path in required}}
    destination = work / 'run_receipt.json'
    write(destination, json.loads(json.dumps(row)))
    return destination


def full_problem(work, case, error):
    status = cut.read(case / 'status.json') if (case / 'status.json').is_file() else {}
    if (work / 'contact_failure.json').exists():
        return 'result', cut.read(work / 'contact_failure.json')
    for name in ('first_rejected_event.json', 'whole_job_peak_RSS_failure.json'):
        if (case / name).is_file():
            return 'result', cut.read(case / name)
    for path in sorted(case.glob('*event*_gate.json')):
        row = cut.read(path)
        if not row['passed']:
            return 'result', row
    reasons = status.get('full_workflow_errors') or []
    if status.get('native_case_started') and status.get('exit_code') not in (None, 0, 124):
        return 'result', {'field': 'solver_exit_code', 'expected': 0, 'actual': status['exit_code'],
                          'diagnostic': reasons[0] if reasons else str(error)}
    if reasons and status.get('exit_code') == 0:
        first = reasons[0]
        if any(text in first for text in ('exceeds', 'q/J/geometry/size', 'completion flags failed',
                                          'mesh/restart gate failed', 'transfer residual gate failed',
                                          'projected inadmissible states were not zero')):
            return 'result', first
        return 'measurement', first
    return 'measurement', str(error)


def restart_problem(work, case, error):
    restart = work / f'disk_restart_{case.name}_event2'
    status = restart / 'status.json'
    if (work / 'contact_failure.json').exists():
        return 'result', cut.read(work / 'contact_failure.json')
    proof = restart / 'fp3_restart_result_failure.json'
    if proof.is_file():
        return 'result', cut.read(proof)
    if status.is_file():
        row = cut.read(status)
        resource = row.get('resource_gate')
        if resource and not resource['passed']:
            return 'result', resource
        if row.get('exit_code') not in (None, 0, 124):
            return 'result', {'field': 'isolated_restart_exit_code', 'expected': 0, 'actual': row['exit_code']}
    return 'measurement', str(error)


def attempt(work, label, part, build, sources, payloads, retry=False):
    mode, probe = PARTS[part]
    config = p3.config_for(mode)
    case = work / f'full{label}_run1'
    receipt = work / 'run_receipt.json'
    if receipt.is_file():
        authenticate(work, label, part, build, payloads)
        return receipt
    p3.campaign(part, sources, retry)['prepare'](work, label, HERE / f'build{label}', config)
    stage = 'full event gates'
    try:
        command = [cut.PYTHON, '-B', str(SELF), str(label), part, '--worker']
        if retry:
            command.append('--retry-full')
        try:
            workflow.run_stage(work, 'full', command)
        finally:
            c3.require_clean_contact_logs(work, case)
        base['status_ok'](case, build, config)
        stage = 'sealing workflow'
        effective_receipt(part, case, case / 'provenance.json')
        hashes = workflow.seal_run(case, build['binary_sha256'], config, payloads, True)
        events = workflow.event_collector()(case)
        assert [row['event'] for row in events] == [1, 2]
        stage = 'suppression and exchange' if not probe else 'lab observation'
        collected, row = collect(work, case, part, events=events)
        if not probe and row['status'] != 'passed':
            raise FailedCheck(row.get('failure_kind') or 'measurement', row['first_difference'])
        restart = None
        if not probe:
            stage = 'isolated restart'
            restart = c3.isolate_with_retry(work, label, case)
        stage = 'sealing provenance'
        return envelope(work, label, part, build, events, hashes, restart, collected)
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        base['failure'](work, case, mode, stage, error)
        if isinstance(error, (FailedCheck, c3.WatchdogOpen)):
            raise
        if stage == 'full event gates':
            kind, first = full_problem(work, case, error)
        elif stage == 'isolated restart':
            kind, first = restart_problem(work, case, error)
        else:
            kind, first = 'measurement', str(error) or type(error).__name__
        raise FailedCheck(kind, first) from error


def helper_cutoff(work, label, part, error, seconds):
    if error.kind == 'result' or not c3.full_watchdog(work, label, seconds):
        return False
    case = work / f'full{label}_run1'
    strip = runpy.run_path(str(p3.inherited.EXTRACT))['strip_mpi_prefixes']
    for path in case.glob('rank_output/*/rank.*/stdout'):
        rank = path.parent.name.split('.')[-1]
        for number, raw in enumerate(path.open(errors='replace'), 1):
            line = strip(raw)
            if 'NATIVE3D rank=' not in line and 'NATIVE3D_DETAIL stage=refine ' not in line:
                continue
            fields = dict(re.findall(r'(\w+)=([^\s]+)', line))
            if fields.get('rank') != rank or 'total' not in fields:
                continue
            try:
                elapsed = Decimal(fields['total'])
            except ArithmeticError:
                continue
            if elapsed.is_finite() and elapsed > 5400:
                raise FailedCheck('result', {'field': 'native_event_seconds', 'limit': 5400,
                                             'actual': str(elapsed), 'path': str(path), 'line': number})
    _, partial = collect(work, case, part, partial=True)
    if not PARTS[part][1] and partial['failure_kind'] == 'result':
        raise FailedCheck('result', partial['first_difference'])
    return True


def run_trial(label, part, build, sources, payloads):
    work = root(label, part) / 'trial1'
    try:
        return attempt(work, label, part, build, sources, payloads)
    except FailedCheck as error:
        if not helper_cutoff(work, label, part, error, c3.FULL_WATCHDOG):
            raise
    extra = [work / name for name in ('full_watchdog.json', 'failed_attempt.json', 'full_host_preflight.json',
                                      'partial_collection.json') if (work / name).is_file()]
    retry = work / c3.FULL_RETRY
    try:
        return attempt(retry, label, part, build, [*sources, *extra], payloads, retry=True)
    except FailedCheck as error:
        if helper_cutoff(retry, label, part, error, 3 * c3.FULL_WATCHDOG):
            raise c3.WatchdogOpen('Full workflow reached both helper watchdogs.') from error
        raise


def run(label, part):
    from build_fp3 import verify_build

    verify_order()
    mode, probe = PARTS[part]
    build = verify_build(label)
    build_dir = HERE / f'build{label}'
    work = root(label, part)
    work.mkdir(parents=True, exist_ok=True)
    summary_path = work / 'case_summary.json'
    if summary_path.is_file():
        row = cut.read(summary_path)
        assert all(row[key] == build[key] for key in FINGERPRINTS), summary_path
        check_pins(row['pins'])
        return row['status']
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    prerequisites, expected = previous(label, part, build, payloads)
    references, pins = reference_runs(B_REFERENCE, payloads)
    if probe:
        expected = references[0]['payload_hashes']
    sources = list(dict.fromkeys([
        SELF, Path(p3.__file__).resolve(), ORDER, ORDER.with_suffix('.sha256'),
        HERE / 'reports/source_check.md', A_ORDER, A_ORDER.with_suffix('.sha256'),
        B_ORDER, B_ORDER.with_suffix('.sha256'),
        p3.ORDER, p3.ORDER.with_suffix('.sha256'),
        p3.inherited.SELF, c3.DRIVER, workflow.WRAPPER, p3.config_for(mode),
        p3.C1 / 'retry_c1_restart.py', p3.NIGHT / 'FP2_C3/resume_restart_observation.py',
        build_dir / 'build_status.json', build_dir / 'source_pins.json',
        HERE / 'collect_fp3_full.py', *prerequisites, *pins,
    ]))
    summary = {**{key: build[key] for key in FINGERPRINTS}, 'part': part, 'mode': mode, 'probe': probe,
               'settings': settings(part), 'observation_only': bool(probe), 'status': 'passed',
               'failure_kind': None, 'first_difference': None}
    if probe:
        summary['observation'] = PROBE_OBSERVATION
    evidence = {}
    try:
        path = run_trial(label, part, build, sources, payloads)
        receipt = authenticate(path.parent, label, part, build, payloads)
        summary['run'] = path_pin(path)
        summary['collection'] = receipt['collection']
        evidence = full_pins(path, receipt)
        if not probe:
            summary['historical_comparison'] = p3.compare_hashes(
                references[0]['payload_hashes'], receipt['hashes'])
        if expected is not None:
            comparison = p3.compare_hashes(expected, receipt['hashes'])
            summary['comparison'] = comparison
            if not probe and not comparison['equal']:
                raise FailedCheck('result', comparison['first_difference'])
        if probe:
            collected = cut.read(receipt['collection']['path'])
            summary['probe_status'] = collected['probe_status']
            if collected['probe_status'] != 'complete' or collected['status'] != 'passed':
                kind = collected['failure_kind'] or 'measurement'
                summary.update(status='FAILED' if kind == 'result' else 'incomplete', failure_kind=kind,
                               first_difference=collected['first_difference'])
            elif not comparison['equal']:
                summary.update(status='mismatch', first_difference=comparison['first_difference'])
    except c3.WatchdogOpen as error:
        summary.update(status='OPEN', failure_kind='helper_watchdog', first_difference=str(error))
    except FailedCheck as error:
        summary.update(status='FAILED', failure_kind=error.kind, first_difference=error.first)
    observed = ('failed_attempt.json', 'full_watchdog.json', 'partial_collection.json', 'collection.json',
                'contact_failure.json', 'restart_watchdog.json', 'fp3_restart_result_failure.json',
                'restart_post_only.log')
    for name in observed:
        evidence.update({str(path): sha(path) for path in work.glob(f'**/{name}')})
    for path in work.glob('**/failed_attempt.json'):
        evidence.update({str(path.parent / name): digest
                         for name, digest in cut.read(path)['artifacts'].items()})
    summary['pins'] = {str(path): sha(path) for path in sources} | evidence
    write(summary_path, json.loads(json.dumps(summary)))
    print(f'{part} {summary["status"]}: {summary_path}', flush=True)
    return summary['status']


def full_pins(path, receipt):
    return {str(path): sha(path)} | {str(path.parent / name): digest
                                    for name, digest in receipt['artifacts'].items()}


def self_check(label):
    verify_order()
    build = HERE / f'build{label}'
    if not (build / 'build_status.json').is_file():
        build = P3 / 'build673'
    for part, (mode, _) in PARTS.items():
        with TemporaryDirectory(prefix='fp3-d-full-') as temp:
            for retry in (False, True):
                work = Path(temp) / str(retry)
                p3.campaign(part, [SELF], retry)['prepare'](work, label, build, p3.config_for(mode))
                p3.verify_generated(work, part, label, retry)
                restart = runpy.run_path(str(work / f'run_restart{label}.py'))
                source = restart['runner_context']().load_base().retargeted_source()
                assert all(value in source for _, value in p3.restart_edits(part))
                assert 'runtime_sample_eio.jsonl' in source and '"probe": probe' in source
                full = (work / 'full_controller.py').read_text()
                assert 'fp3_rank_probe_environment.json' in full
                compile(source, '<restart self-check>', 'exec')
        print(part, 'generated full and restart scripts verified', flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('part', choices=PARTS, nargs='?')
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--retry-full', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    with bound():
        if args.self_check:
            self_check(args.label)
        elif not args.part:
            parser.error('choose a workflow')
        elif args.worker:
            p3.launch_child(args.label, args.part, 1, args.retry_full)
        else:
            outcome = run(args.label, args.part)
            raise SystemExit({'passed': 0, 'incomplete': 0, 'mismatch': 0, 'FAILED': 1, 'OPEN': 75}[outcome])
