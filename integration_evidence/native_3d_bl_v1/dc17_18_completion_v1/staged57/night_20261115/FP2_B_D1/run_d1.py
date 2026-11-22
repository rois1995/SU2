"""Run the focused D1 fixtures and the single unchanged S1 controller."""

from argparse import ArgumentParser
from pathlib import Path
import json
import runpy
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FP2_A = HERE.parent / 'FP2_A'
FP2_B = HERE.parent / 'FP2_B'
sys.path[:0] = [str(FP2_B), str(FP2_A)]
import audit_b
import run_a as a

DRIVER = Path(__file__).resolve()
ORDER = HERE / 'inputs/FP2_B_DIAGNOSIS_1.md'
ORDER_SHA = '78b34a13b5f7050ea8935b93ac023ba7fc6549c481088e38f64dfcf6b001d320'
CONFIG = FP2_A / 'run.cfg'
CONFIG_SHA = '07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382'
REFERENCE = FP2_B / 'G5/build633/S1'
BASE_DRIVER = FP2_A / 'run_g5.py'
CASES = (
    ('paths', '[.][NativeLocalCoreSurfaceB]', 'locked'),
    ('cylinder', 'Cylinder surface length and distance steps repeat and account for every residual face', None),
    ('volume', '[NativeLocalCoreVolumeB]', 'volume'),
)


def audit():
    prior = audit_b.audit()
    try:
        audit_b.HERE = HERE
        current = audit_b.audit()
    finally:
        audit_b.HERE = FP2_B
    current['prior_fp2_b_identities_checked'] = len(prior['identities'])
    return current


def prepare(build):
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    assert a.sha(CONFIG) == CONFIG_SHA
    receipt = a.cut.read(build / 'build_status.json')
    binary, _ = a.verify_build(receipt, build / 'source_pins.json')
    assert a.sha(Path(receipt['binary'])) == receipt['binary_sha256']
    assert receipt['wait']['terminal'] and a.cut.terminal(receipt['wait']['controller'])
    assert not receipt['new_warnings']
    return receipt, binary


def fixtures(build, receipt, binary):
    output = HERE / 'fixtures' / build.name
    provenance = {k: receipt[k] for k in ('build', 'source_pins_sha256', 'binary_sha256', 'test_binary_sha256')}
    provenance['output'] = str(output)
    provenance['inputs'] = {str(path): a.sha(path) for path in (DRIVER, ORDER, HERE / 'source_check.md')}
    records = {}
    for name, selector, previous in CASES:
        case = output / name
        row = a.fixture(case, binary, 1, selector, {}, provenance)
        if previous:
            old = a.cut.read(FP2_B / 'scoped/build633' / previous / 'fixture_status.json')
            assert json.loads(json.dumps(row['catch_summaries'])) == old['catch_summaries'], name
        else:
            marker = 'FP2_B cylinder_invariant checked=27 violations=0'
            assert marker in (case / 'test_outer.log').read_text()
            assert json.loads(json.dumps(row['catch_summaries']))[0][0][1] == '1'
        path = case / 'fixture_status.json'
        records[str(path)] = a.sha(path)
    a.sealed(output / 'fixtures.json', {**provenance, 'passed': True, 'receipts': records})


def launch(build, mode, base):
    work = HERE / 'G5' / build.name / 'S1'
    base['launch_full'].__globals__.update(config_for=lambda _: CONFIG, __file__=str(DRIVER), HERE=HERE)
    if mode == 'full':
        base['launch_full'](work, build, 'S1')
        return
    assert not (work / 'full_launch.json').exists(), 'D1 permits one launched S1.'
    receipt = a.cut.read(HERE / 'fixtures' / build.name / 'fixtures.json')
    assert receipt['passed']
    for path, digest in receipt['receipts'].items():
        assert a.sha(Path(path)) == digest, path

    baseline = a.cut.read(REFERENCE / 'helper_pins.json')
    sources = [DRIVER, BASE_DRIVER, ORDER, ORDER.with_suffix('.sha256'), HERE / 'source_check.md',
               build / 'source_pins.json', build / 'build_status.json',
               HERE / 'fixtures' / build.name / 'fixtures.json', REFERENCE / 'helper_pins.json']
    shared = ('full_controller.py', 'check_complete_events.py', 'check_connected_events.py', 'check_timer_tree.py')
    for name in shared:
        path = REFERENCE / name
        assert a.sha(path) == baseline['bound'][str(path)], path
        sources.append(path)
    campaign = base['PRIOR']['bind_campaign']('on', sources)
    campaign['prepare'](work, receipt['build'], build, CONFIG)
    for name in shared:
        assert a.sha(work / name) == a.sha(REFERENCE / name), name

    workflow = base['workflow']
    try:
        workflow.run_stage(work, 'full', [a.cut.PYTHON, '-B', str(DRIVER), str(build), 'full'])
    except RuntimeError:
        if not (work / 'full_wait.json').is_file():
            raise
    manifest = workflow.verify_helpers(work)
    wait = a.cut.read(work / 'full_wait.json')
    case = work / f'full{receipt["build"]}_run1'
    status_path = case / 'status.json'
    status = a.cut.read(status_path)
    assert wait['terminal'] and a.cut.terminal(wait['controller'])
    assert status['state'] == 'terminal' and status['all_owned_handles_terminal']
    kept = [status_path, case / 'provenance.json', work / 'helper_pins.json', work / 'full_wait.json',
            *(case / 'rank_output').glob('*/rank.*/stdout')]
    a.sealed(work / 'diagnosis_receipt.json', {
        'build': receipt['build'], 'case': str(case), 'wait': wait, 'status': status,
        'config_sha256': manifest['config_sha256'], 'controller_sha256': a.sha(work / 'full_controller.py'),
        'artifacts': {str(path.relative_to(work)): a.sha(path) for path in kept},
    })
    print(f'D1 S1 terminal. Accepted by inherited controller: {status["full_workflow_accepted"]}', flush=True)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('mode', choices=('fixtures', 'S1', 'full', 'audit'))
    args = parser.parse_args()
    build = args.build.resolve()
    if args.mode == 'full':
        launch(build, args.mode, runpy.run_path(str(BASE_DRIVER)))
        return
    terminal = audit()
    if args.mode == 'audit':
        a.cut.save(HERE / 'terminal_audit.json', terminal)
        print(f'{len(terminal["identities"])} D1 process identities terminal.', flush=True)
        return
    receipt, binary = prepare(build)
    if args.mode == 'fixtures':
        fixtures(build, receipt, binary)
    else:
        assert receipt['test_binary_sha256'] == a.sha(Path(receipt['test_binary']))
        launch(build, args.mode, runpy.run_path(str(BASE_DRIVER)))


if __name__ == '__main__':
    main()
