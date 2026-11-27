"""Run the sole DEF1 standing mode-S workflow and isolated disk restart."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
import inspect
from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory
from unittest.mock import patch

from legacy_config import BASE, FULL_REF, HERE, NIGHT, committed, derived, prepared, verify_case

FP3 = NIGHT / 'FP3_E'
SELF = Path(__file__).resolve()
ORDER = HERE / 'inputs/DEF1_METRIC_DEFAULTS.md'
ORDER_SUM = ORDER.with_suffix('.sha256')
ORDER_SHA = '480c7ddeb2751bc4748b5cd1105011e80dfa188f8bfdd816419084105bb80025'
PART = 'S1'


def load_e():
    inherited = NIGHT / 'FP2_P3'
    sys.path.insert(0, str(inherited))
    p3_spec = spec_from_file_location('def1_inherited_p3_full', inherited / 'run_full.py')
    p3_module = module_from_spec(p3_spec)
    p3_spec.loader.exec_module(p3_module)
    path = FP3 / 'run_fp3_full.py'
    spec = spec_from_file_location('def1_fp3_full', path)
    module = module_from_spec(spec)
    with patch.dict(sys.modules, {'run_full': p3_module}):
        spec.loader.exec_module(module)
    # Leave the sealed FP3-E collector and its imports untouched.
    sys.path.insert(0, str(FP3))
    return module


E = load_e()
p3, cut, workflow = E.p3, E.cut, E.workflow
sha, path_pin = E.sha, E.path_pin
p3_original_config = p3.config_for
restart_original = E.bound_restart


def inherited_reference(payloads):
    """Use the FP3-E resolver on the adopted E seals, never on the derived config."""
    source = inspect.getsource(E.reference_runs)
    assert source.count('label = 678') == 1
    source = source.replace('label = 678', 'label = 680', 1)
    scope = dict(E.__dict__, D_REFERENCE=FULL_REF, BASE_COMMIT=BASE)
    # The commit resolver closes over its own globals.
    def committed_bytes(path):
        return committed(path).read_bytes()
    scope['committed_bytes'] = committed_bytes
    exec(compile(source, E.__file__, 'exec'), scope)
    return scope['reference_runs'](FULL_REF, payloads)


def reference_configuration():
    """The run resolver authenticates these paths before they can seed a launch."""
    accepted = cut.read(committed(FULL_REF))
    primary = accepted['workflows']['S']['runs'][0]
    config = Path(primary['effective_config']['path'])
    digest_full = primary['effective_config']['sha256']
    old_launch = config.parent / 'provenance.json'
    restart = Path(primary['isolated_restart_receipt']['path'])
    run = Path(primary['full_run_receipt']['path'])
    assert sha(run) == primary['full_run_receipt']['sha256']
    assert sha(restart) == primary['isolated_restart_receipt']['sha256']
    restart_meta = cut.read(restart)
    assert restart_meta['disk_restart_verified'] and restart_meta['all_inputs_and_pins_unchanged']
    cfg_restart = restart.parent / 'run.cfg'
    digest_restart = restart_meta['artifact_sha256']['run.cfg']
    assert sha(config) == digest_full == cut.read(old_launch)['config_sha256']
    assert sha(cfg_restart) == digest_restart
    return ((config, [FULL_REF, run, old_launch], digest_full), (cfg_restart, [FULL_REF, run, restart], digest_restart))


def config_for(mode, build):
    assert mode == 'S'
    target = HERE / f'build{build}/legacy_S.cfg'
    receipt = cut.read(target.with_name('legacy_config.json'))
    assert sha(target) == receipt['derived_sha256']
    return target


def bound_restart(part, name, label, build_dir):
    template, source = restart_original(part, name, label, build_dir)
    if name != 'run_restart':
        return template, source
    reference, metadata, sha_expected = reference_configuration()[1]
    prefix = ('from pathlib import Path\n'
              f'expected_restart = Path({str(reference)!r}).read_bytes()\n'
              f'assert __import__("hashlib").sha256(expected_restart).hexdigest() == {sha_expected!r}\n')
    final, inserted = derived(reference.read_bytes())
    assert inserted == ['ADAP_NORM= 1.0']
    # Check the generated restart bytes before its launch. Existing write and pin paths stay intact.
    check = f'assert config.encode() == {final!r}\n'
    anchor = '            compile(source, str(BASE), "exec")\n'
    position = '(FOLDER / "run.cfg").write_text(config)'
    extra = (f'            legacy_guard = {(prefix + check)!r}\n'
             f'            old = {position!r}\n'
             '            assert source.count(old) == 1, old\n'
             '            source = source.replace(old, legacy_guard + old, 1)\n')
    return template, workflow.replace_once(source, anchor, extra + anchor)


def check_order():
    E.verify_order()
    assert sha(ORDER) == ORDER_SHA and ORDER_SUM.read_text().split() == [ORDER_SHA, ORDER.name]


def settings_for(label):
    return patch.object(p3, 'config_for', lambda mode: config_for(mode, label))


def sources(build, scoped, historical):
    config, restart = reference_configuration()
    return list(dict.fromkeys([
        SELF, ORDER, ORDER_SUM, HERE / 'legacy_config.py', HERE / 'reports/source_check.md',
        scoped, build / 'build_status.json', build / 'source_pins.json',
        Path(p3.__file__).resolve(), FP3 / 'run_fp3_full.py', E.ORDER, E.ORDER_SUM,
        *(path for order in E.INHERITED_ORDERS for path in (order, order.with_suffix('.sha256'))),
        p3.ORDER, p3.ORDER.with_suffix('.sha256'),
        p3.inherited.SELF, E.c3.DRIVER, workflow.WRAPPER,
        p3.C1 / 'retry_c1_restart.py', p3.NIGHT / 'FP2_C3/resume_restart_observation.py',
        FP3 / 'collect_fp3_full.py', FP3 / 'collect_stall.py',
        *config[1], *restart[1], config[0], restart[0],
        build / 'legacy_S.cfg', build / 'legacy_config.json', *historical,
    ]))


def run(label):
    from build_fp3 import verify_build

    check_order()
    build = verify_build(label)
    build_path = HERE / f'build{label}'
    scoped = HERE / 'controls' / build_path.name / 'step1_scoped.json'
    E.require(scoped, build)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    references, historical = inherited_reference(payloads)
    settings, restart_settings = reference_configuration()
    assert p3_original_config('S').read_bytes() == settings[0].read_bytes()
    prepared(*settings, build_path / 'legacy_S.cfg')
    prerequisites = sources(build_path, scoped, historical)
    work = E.root(label, PART)
    work.mkdir(parents=True, exist_ok=True)
    result = work / 'case_summary.json'
    if result.is_file():
        receipt = cut.read(result)
        assert all(receipt[key] == build[key] for key in E.FINGERPRINTS)
        E.check_pins(receipt['pins'])
        return receipt['status']

    summary = {key: build[key] for key in E.FINGERPRINTS}
    summary.update(status='passed', part=PART, mode='S', probe=0,
                   failure_kind=None, first_difference=None)
    evidence = {}
    try:
        with E.bound(), settings_for(label):
            path = E.run_trial(label, PART, build, prerequisites, payloads)
            receipt = E.authenticate(path.parent, label, PART, build, payloads)
        summary['run'] = path_pin(path)
        summary['collection'] = receipt['collection']
        evidence = E.full_pins(path, receipt)
        restart = Path(receipt['restart'])
        full_proof, full_paths = verify_case(Path(receipt['case']), *settings)
        restart_proof, restart_paths = verify_case(restart.parent, *restart_settings)
        evidence.update({str(file): sha(file) for file in (*full_paths, *restart_paths)})
        assert cut.read(restart)['artifact_sha256']['run.cfg'] == restart_proof['derived_sha256'], restart
        summary['legacy_configs'] = {'full': full_proof, 'isolated_restart': restart_proof}
        summary['comparison'] = p3.compare_hashes(references[0]['payload_hashes'], receipt['hashes'])
        if not summary['comparison']['equal']:
            raise E.FailedCheck('result', summary['comparison']['first_difference'])
    except E.c3.WatchdogOpen as error:
        summary.update(status='OPEN', failure_kind='helper_watchdog', first_difference=str(error))
    except E.FailedCheck as error:
        summary.update(status='FAILED', failure_kind=error.kind, first_difference=error.first)

    for name in ('failed_attempt.json', 'full_watchdog.json', 'partial_collection.json', 'collection.json',
                 'contact_failure.json', 'restart_watchdog.json', 'fp3_restart_result_failure.json'):
        evidence.update({str(path): sha(path) for path in work.glob(f'**/{name}')})
    for path in work.glob('**/failed_attempt.json'):
        evidence.update({str(path.parent / name): digest
                         for name, digest in cut.read(path)['artifacts'].items()})
    summary['pins'] = {str(path): sha(path) for path in prerequisites} | evidence
    workflow.sealed_write(result, summary)
    print(f'DEF1 {PART} {summary["status"]}: {result}', flush=True)
    return summary['status']


def self_check(label):
    check_order()
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    reference, _ = inherited_reference(payloads)
    full, restart = reference_configuration()
    assert sha(full[0]) == reference[0]['effective_config']['sha256']
    assert derived(full[0].read_bytes())[1] == derived(restart[0].read_bytes())[1] == ['ADAP_NORM= 1.0']
    # Apply both inherited launch rewrites without executing a solver or changing sealed helpers.
    build = HERE / f'build{label}'
    with E.bound(), TemporaryDirectory(prefix='def1-full-') as tmp:
        for retry in (False, True):
            work = Path(tmp) / str(retry)
            work.mkdir()
            config = work / 'legacy_S.cfg'
            prepared(*full, config)
            p3.campaign(PART, [SELF], retry)['prepare'](work, label, build, config)
            p3.verify_generated(work, PART, label, retry)
            for script in (work / 'full_controller.py', work / f'run_restart{label}.py'):
                compile(script.read_text(), str(script), 'exec')
            assert str(restart[0]) in (work / f'run_restart{label}.py').read_text()
    print('DEF1 standalone S and restart launch bindings verified without starting a solver.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('part', choices=(PART,), nargs='?', default=PART)
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--retry-full', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    E.HERE = HERE
    E.SELF = SELF
    E.PARTS = {PART: ('S', 0)}
    E.bound_restart = bound_restart
    if args.self_check:
        self_check(args.label)
    elif args.worker:
        with E.bound(), settings_for(args.label):
            p3.launch_child(args.label, PART, 1, args.retry_full)
    else:
        raise SystemExit({'passed': 0, 'FAILED': 1, 'OPEN': 75}[run(args.label)])
