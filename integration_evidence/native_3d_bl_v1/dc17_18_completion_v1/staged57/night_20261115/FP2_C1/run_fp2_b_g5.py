"""Run the single changed-mesh FP2-C1 S workflow and isolated restart."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
B = HERE.parent / 'FP2_B'
sys.path.insert(0, str(B))
import run_lab as lab

SCOPED = spec_from_file_location('fp2_b_scope', B / 'run_scoped.py')
scope = module_from_spec(SCOPED)
SCOPED.loader.exec_module(scope)
FP2_A = HERE.parent / 'FP2_A'
INHERITED = FP2_A / 'run_g5.py'
BASE = runpy.run_path(str(INHERITED))
workflow, cut, sha = (BASE[key] for key in ('workflow', 'cut', 'sha'))
DEFAULT_CONFIG = BASE['config_for']
SURFACE_CONFIG = HERE / 'run.cfg'
SURFACE_CONFIG_SHA = '07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382'
ORDER = HERE / 'inputs/FAST_PATH_STAGE2_C1.md'
ORDER_SHA = '6029450b746aed4493a4790fe5cc6504835387b70c7fc658e3bd3279fd96f35d'
S0 = '7e6953e721'
S0_S = HERE.parent / 'FX2/G5/build645/S/run_receipt.json'


def config_for(mode):
    assert mode == 'S'
    original = DEFAULT_CONFIG('S1')
    assert sha(original) == SURFACE_CONFIG_SHA
    assert sha(SURFACE_CONFIG) == SURFACE_CONFIG_SHA
    checksum = SURFACE_CONFIG.with_suffix('.cfg.sha256')
    assert checksum.read_text().split() == [SURFACE_CONFIG_SHA, SURFACE_CONFIG.name], checksum
    return SURFACE_CONFIG


def bind_inherited():
    # Keep the inherited controller, receipt verification and standing gates.
    context = BASE['launch_full'].__globals__
    context.update(config_for=config_for, __file__=str(Path(__file__).resolve()), HERE=HERE)


def verify_run(work, mode, build, payloads):
    bind_inherited()
    row = BASE['verify_run'](work, mode, build, payloads)
    assert row['test_binary_sha256'] == build['test_binary_sha256'], work
    return row


def verify_order():
    BASE['verify_orders']()
    assert sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]


def reference_sources():
    # S0 is the accepted FX2 checkpoint, so its S run supplies the report baseline.
    label = subprocess.check_output(['git', 'show', '-s', '--format=%s', S0], cwd=cut.REPO, text=True).strip()
    assert label == 'FX2: accepted', label
    row = cut.read(S0_S)
    assert row['passed'] and row['build'] == 645 and row['mode'] == 'S', S0_S
    assert row['config_sha256'] == SURFACE_CONFIG_SHA and len(row['hashes']) == 6, S0_S
    case = Path(row['case'])
    paths = sorted(case.glob('rank_output/*/rank.*/stdout'))
    assert len(paths) == 6, case
    for path in paths:
        assert sha(path) == row['artifacts'][str(path.relative_to(S0_S.parent))], path
    return [S0_S, *paths]


def prerequisite_sources(build, lab_proof):
    fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
    label = build['build']
    fixture = HERE / 'scoped' / f'build{label}/fixtures.json'
    gate = scope.verified(fixture, fingerprints)
    assert gate['G1'] == [375, 114, 228] and gate['fp1_cohorts'] == 18, fixture
    paths = [fixture]
    fx2 = HERE / 'fixtures' / f'build{label}/comparison.json'
    if fx2.is_file():
        assert cut.read(fx2)['passed'], fx2
        paths.append(fx2)
    result = cut.read(lab_proof)
    assert result['passed'] and all(result[key] == value for key, value in fingerprints.items()), lab_proof
    artifacts = result['artifacts']
    assert artifacts, lab_proof
    for name, digest in artifacts.items():
        assert sha(Path(name)) == digest, name
    return [*paths, lab_proof, *map(Path, artifacts)]


def run_case(work, mode, build_dir, build, config, source_paths, payloads):
    label = build['build']
    case = work / f'full{label}_run1'
    campaign = BASE['PRIOR']['bind_campaign']('on', source_paths)
    campaign['prepare'](work, label, build_dir, config)
    stage = 'full event gates'
    try:
        workflow.run_stage(work, 'full', [cut.PYTHON, '-B', str(Path(__file__).resolve()), str(build_dir),
                                          '--full', mode])
        BASE['status_ok'](case, build, config)
        hashes = workflow.seal_run(case, build['binary_sha256'], config, payloads, True)
        events = workflow.event_collector()(case)
        stage = 'isolated restart'
        restart = workflow.isolate_restart(work, label, case)
        stage = 'sealing provenance'
        workflow.verify_helpers(work)
        assert sha(Path(build['binary'])) == build['binary_sha256']
        assert sha(Path(build['test_binary'])) == build['test_binary_sha256']
        artifacts = [work / name for name in ('helper_pins.json', 'full_launch.json', 'full_wait.json',
                                              'restart_launch.json', 'restart_wait.json')]
        artifacts.extend(case / name for name in ('provenance.json', 'status.json', 'full_workflow_receipt.json',
                                                   'n8_payload_identity.json'))
        artifacts.extend(case.glob('p7p8_event*_gate.json'))
        artifacts.extend(case.glob('f3b_event*_timer_gate.json'))
        artifacts.extend(case.glob('rank_output/*/rank.*/stdout'))
        artifacts.append(restart)
        fingerprints = {key: build[key] for key in lab.FINGERPRINTS}
        record = {**fingerprints, 'passed': True, 'mode': mode, 'case': str(case), 'hashes': hashes,
                  'restart': str(restart), 'events': events, 'config_sha256': sha(config),
                  'report_reference': {'commit': S0, 'receipt': str(S0_S), 'sha256': sha(S0_S)},
                  'artifacts': {str(path.relative_to(work)): sha(path) for path in artifacts}}
        workflow.sealed_write(work / 'run_receipt.json', record)
    except (AssertionError, KeyError, OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        BASE['failure'](work, case, mode, stage, error)
        raise
    return record


def run(build_dir, mode, lab_proof):
    bind_inherited()
    verify_order()
    build, _, _ = lab.prepare(build_dir)
    label = build['build']
    work = HERE / 'G5' / f'build{label}' / mode
    if lab_proof is None:
        lab_proof = HERE / 'cases' / f'build{label}/lab_acceptance.json'
    config = config_for(mode)
    source_paths = [Path(__file__).resolve(), B / 'run_fp2_b_g5.py', INHERITED, Path(lab.__file__).resolve(),
                    Path(scope.__file__).resolve(), ORDER, ORDER.with_suffix('.sha256'), HERE / 'source_check.md',
                    build_dir / 'build_status.json', build_dir / 'source_pins.json', config,
                    FP2_A / 'run.cfg', SURFACE_CONFIG.with_suffix('.cfg.sha256')]
    source_paths += [path for order in BASE['ORDERS'] for path in (order, order.with_suffix('.sha256'))]
    source_paths += reference_sources()
    source_paths += prerequisite_sources(build, lab_proof)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    if (work / 'run_receipt.json').is_file():
        verify_run(work, mode, build, payloads)
    else:
        assert not (work / 'failed_attempt.json').exists(), work
        run_case(work, mode, build_dir, build, config, source_paths, payloads)
    print('FP2-C1 S PASSED. Default path not yet proven.', flush=True)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('--lab-proof', type=Path, help='Override the candidate lab_acceptance.json path.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=('S',))
    group.add_argument('--full', choices=('S',), help='Internal supervised full controller.')
    group.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        verify_order()
        config_for('S')
        reference_sources()
        print('FP2-C1 S inherited inputs and S0 reference verified.')
        return
    if not args.build:
        parser.error('a build directory is required')
    build = args.build.resolve()
    if args.full:
        bind_inherited()
        label = cut.read(build / 'build_status.json')['build']
        BASE['launch_full'](HERE / 'G5' / f'build{label}' / args.full, build, args.full)
    else:
        run(build, args.only, args.lab_proof.resolve() if args.lab_proof else None)


if __name__ == '__main__':
    main()
