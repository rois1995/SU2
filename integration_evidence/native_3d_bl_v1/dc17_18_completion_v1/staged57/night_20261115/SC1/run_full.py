"""Run one SC1 mode-S workflow with its standing gates and isolated restart."""

from argparse import ArgumentParser
from contextlib import contextmanager
from importlib.util import module_from_spec, spec_from_file_location
import inspect
from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
SELF = Path(__file__).resolve()
NIGHT = HERE.parent
DEF1 = NIGHT / 'DEF1'
ORDER = HERE / 'inputs/SC1_GEOMETRY_CERTIFICATES.md'
ORDER_SUM = ORDER.with_suffix('.sha256')
ORDER_SHA = '10280d0d70d04ed4bb7d4721d295055035bdceb2b8a3fe4149be1847704e5eb2'
PARTS = {'count': 6, 'count_repeat': 6, 'S1': 0, 'S2': 0, 'probe5': 5}


def load_def1():
    path = DEF1 / 'run_full.py'
    spec = spec_from_file_location('sc1_inherited_def1_full', path)
    module = module_from_spec(spec)
    sys.path.insert(0, str(DEF1))
    original = sys.modules.get('build_fp3')
    try:
        # DEF1's import is for its own verifier. SC1 binds its own verifier at launch.
        sys.modules.pop('build_fp3', None)
        spec.loader.exec_module(module)
    finally:
        sys.path.remove(str(DEF1))
        sys.modules.pop('build_fp3', None)
        if original is not None:
            sys.modules['build_fp3'] = original
        sys.path.insert(0, str(HERE))
    return module


prior = load_def1()
E, p3, cut, workflow = prior.E, prior.p3, prior.cut, prior.workflow
sha, path_pin = E.sha, E.path_pin


def verify_build(label):
    from build_fp3 import verify_build as verify_sc1

    return verify_sc1(label)


def check_order():
    prior.check_order()
    assert sha(ORDER) == ORDER_SHA
    assert ORDER_SUM.read_text().split() == [ORDER_SHA, ORDER.name]


def modify(function, replacements):
    """Bind the sealed FP3-E observer variants to standing SC1 runs."""
    source = inspect.getsource(function)
    for old, new in replacements:
        source = workflow.replace_once(source, old, new)
    exec(compile(source, E.__file__, 'exec'), E.__dict__)


@contextmanager
def bound(label):
    with patch.dict(E.__dict__, {
        'HERE': HERE, 'SELF': SELF, 'bound_restart': prior.bound_restart,
        'PARTS': {part: ('S', probe) for part, probe in PARTS.items()},
    }):
        modify(E.attempt, (("stage = 'suppression, exchange and stall' if not probe else 'lab observation'",
                            "stage = 'suppression, exchange and stall'"),
                           ("if not probe and row['status'] != 'passed':", "if row['status'] != 'passed':"),
                           ("restart = None\n        if not probe:\n            stage = 'isolated restart'\n"
                            "            restart = c3.isolate_with_retry(work, label, case)",
                            "stage = 'isolated restart'\n        restart = c3.isolate_with_retry(work, label, case)")))
        modify(E.collect, (('probe = PARTS[part][1]', 'probe = 0'),))
        modify(E.envelope, (("'observation_only': bool(probe)", "'observation_only': False"),))
        # FP3-E's probe-only authenticator omits the restart and full gate seal.
        auth = inspect.getsource(E.authenticate)
        legacy = auth[auth.index('    if not probe:'):auth.index('    assert all(row[key]')]
        standing = """    with patch.dict(base['verify_run'].__globals__, {'config_for': p3.config_for}):
        row = base['verify_run'](work, mode, build, payloads)
"""
        modify(E.authenticate, (
            (legacy, standing),
            ("({'full'} if probe else {'full', 'restart'})", "{'full', 'restart'}"),
            ("    if not probe:\n        c3.terminal_wait(work, 'restart', 0)\n",
             "    c3.terminal_wait(work, 'restart', 0)\n"),
            ("        post = cut.read(Path(row['restart']).parent / 'post_terminal_handles.json')\n"
             "        assert post['actual_host_proc'] and post['all_owned_handles_terminal'], post\n"
             "        assert all(cut.terminal(entry['identity']) for entry in post['owned_identity_checks']), post",
             "    post = cut.read(Path(row['restart']).parent / 'post_terminal_handles.json')\n"
             "    assert post['actual_host_proc'] and post['all_owned_handles_terminal'], post\n"
             "    assert all(cut.terminal(entry['identity']) for entry in post['owned_identity_checks']), post"),
        ))
        modify(E.helper_cutoff, (("not PARTS[part][1] and partial['failure_kind']", "partial['failure_kind']"),))
        modify(E.run_trial, (("        if PARTS[part][1]:\n"
                              "            raise c3.WatchdogOpen('The quality step of FP3-D reached its helper watchdog.') from error\n",
                              ""),))
        with E.bound(), patch.object(p3, 'config_for', lambda mode: config_for(mode, label)):
            yield


def config_for(mode, label):
    assert mode == 'S'
    target = HERE / f'build{label}/legacy_S.cfg'
    receipt = cut.read(target.with_name('legacy_config.json'))
    assert sha(target) == receipt['derived_sha256']
    return target


def root(label, part):
    return HERE / 'G5' / f'build{label}' / part


def read_run(label, part, build):
    result = root(label, part) / 'case_summary.json'
    summary = E.require(result, build)
    path = Path(summary['run']['path'])
    assert path_pin(path) == summary['run'], path
    row = cut.read(path)
    assert row['passed'] and row['probe'] == PARTS[part] and not row['observation_only'], path
    return result, summary, row


def prerequisites(label, part, build, count_from):
    scoped = HERE / 'controls' / f'build{label}' / 'step1_scoped.json'
    E.require(scoped, build)
    inputs = [scoped]
    if part != 'count':
        step = HERE / f'build{label}/step2.json'
        if part == 'count_repeat':
            from collect_sc1 import count_measurements

            assert count_from is not None and count_from != label
            original_build = cut.read(HERE / f'build{count_from}/build_status.json')
            assert original_build['passed'] and original_build['binary_sha256'] != build['binary_sha256']
            receipt_path, _, preceding = read_run(count_from, 'count', original_build)
            assert count_measurements(Path(preceding['case']))['repeat_required'], receipt_path
            inputs.append(receipt_path)
        else:
            E.require(step, build)
            inputs.append(step)
    if part in ('S2', 'probe5'):
        first, _, run1 = read_run(label, 'S1', build)
        inputs.append(first)
        if part == 'probe5':
            second, _, run2 = read_run(label, 'S2', build)
            assert run1['hashes'] == run2['hashes'], second
            inputs.append(second)
    return inputs


def run(label, part, count_from=None):
    check_order()
    build = verify_build(label)
    build_path = HERE / f'build{label}'
    work = root(label, part)
    result = work / 'case_summary.json'
    if result.is_file():
        receipt = cut.read(result)
        assert all(receipt[key] == build[key] for key in E.FINGERPRINTS)
        assert receipt['part'] == part and receipt['probe'] == PARTS[part]
        E.check_pins(receipt['pins'])
        return receipt['status']
    required = prerequisites(label, part, build, count_from)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    references, historical = prior.inherited_reference(payloads)
    reference = references[0]['payload_hashes']
    if part == 'probe5':
        assert not p3.compare_hashes(reference, read_run(label, 'S1', build)[2]['hashes'])['equal']
    full, restart_settings = prior.reference_configuration()
    prior.prepared(*full, build_path / 'legacy_S.cfg', launch_source=prior.p3_original_config('S'))
    inputs = [SELF, ORDER, ORDER_SUM, HERE / 'reports/source_check.md', *required,
              *(path for path in prior.sources(DEF1 / 'build681', DEF1 / 'controls/build681/step1_scoped.json', historical)
                if DEF1 / 'build681' not in path.parents)]
    inputs += [build_path / name for name in ('build_status.json', 'source_pins.json',
                                              'legacy_S.cfg', 'legacy_config.json')]
    work.mkdir(parents=True, exist_ok=True)
    summary = {key: build[key] for key in E.FINGERPRINTS}
    summary.update(status='passed', part=part, mode='S', probe=PARTS[part],
                   count_from=count_from, failure_kind=None, first_difference=None)
    evidence = {}
    try:
        with bound(label):
            path = E.run_trial(label, part, build, inputs, payloads)
            receipt = E.authenticate(path.parent, label, part, build, payloads)
        summary['run'] = path_pin(path)
        summary['collection'] = receipt['collection']
        evidence = E.full_pins(path, receipt)
        restart = Path(receipt['restart'])
        full_proof, full_paths = prior.verify_case(Path(receipt['case']), *full)
        restart_proof, restart_paths = prior.verify_case(restart.parent, *restart_settings)
        evidence.update({str(path): sha(path) for path in (*full_paths, *restart_paths)})
        assert cut.read(restart)['artifact_sha256']['run.cfg'] == restart_proof['derived_sha256'], restart
        summary['legacy_configs'] = {'full': full_proof, 'isolated_restart': restart_proof}
        summary['comparison'] = p3.compare_hashes(reference, receipt['hashes'])
        if part in ('count', 'count_repeat', 'probe5') and not summary['comparison']['equal']:
            raise E.FailedCheck('result', summary['comparison']['first_difference'])
        if part == 'S2':
            expected = read_run(label, 'S1', build)[2]['hashes']
            summary['reproducibility'] = p3.compare_hashes(expected, receipt['hashes'])
            if not summary['reproducibility']['equal']:
                raise E.FailedCheck('result', summary['reproducibility']['first_difference'])
    except E.c3.WatchdogOpen as error:
        summary.update(status='OPEN', failure_kind='helper_watchdog', first_difference=str(error))
    except E.FailedCheck as error:
        summary.update(status='FAILED', failure_kind=error.kind, first_difference=error.first)

    for name in ('failed_attempt.json', 'full_watchdog.json', 'partial_collection.json', 'collection.json',
                 'contact_failure.json', 'restart_watchdog.json', 'fp3_restart_result_failure.json'):
        evidence.update({str(path): sha(path) for path in work.glob(f'**/{name}')})
    for path in work.glob('**/failed_attempt.json'):
        evidence.update({str(path.parent / name): digest for name, digest in cut.read(path)['artifacts'].items()})
    summary['pins'] = {str(path): sha(path) for path in inputs} | evidence
    workflow.sealed_write(result, summary)
    print(f'SC1 {part} {summary["status"]}: {result}', flush=True)
    return summary['status']


def self_check(label):
    check_order()
    with TemporaryDirectory(prefix='sc1-full-') as temp:
        build = DEF1 / 'build681'
        for part in PARTS:
            with bound(label):
                for retry in (False, True):
                    work = Path(temp) / f'{part}-{retry}'
                    work.mkdir()
                    config = build / 'legacy_S.cfg'
                    p3.campaign(part, [SELF], retry)['prepare'](work, label, build, config)
                    p3.verify_generated(work, part, label, retry)
                    restart = runpy.run_path(str(work / f'run_restart{label}.py'))
                    source = restart['runner_context']().load_base().retargeted_source()
                    assert 'expected_restart' in source and '"probe": probe' in source
                    compile(source, '<SC1 restart self-check>', 'exec')
    print('SC1 full and restart controller bindings checked for probes 0, 5 and 6.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('part', nargs='?', choices=PARTS)
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--retry-full', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--count-from', type=int, help='build of the first count run when repeating after attribution')
    args = parser.parse_args()
    if args.self_check:
        self_check(args.label)
    elif not args.part:
        parser.error('choose a workflow')
    elif args.worker:
        with bound(args.label):
            p3.launch_child(args.label, args.part, 1, args.retry_full)
    else:
        raise SystemExit({'passed': 0, 'FAILED': 1, 'OPEN': 75}[run(args.label, args.part, args.count_from)])
