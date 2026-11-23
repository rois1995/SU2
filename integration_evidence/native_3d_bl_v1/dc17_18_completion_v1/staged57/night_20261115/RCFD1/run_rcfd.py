"""Run the RCFD1 S and OFF workflows and isolated restarts."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import runpy
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
C1 = HERE.parent / 'FP2_C1'
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
SURFACE_CONFIG = C1 / 'run.cfg'
SURFACE_CONFIG_SHA = '07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382'
OFF_CONFIG = HERE.parent / 'FX2/G5/build645/OFF/full645_run1/run.cfg'
OFF_CONFIG_SHA = '5c0fe04144c7bb230ec7f71e26c87bf7d0150cdf5c89caa6e840ba20f3ae2d56'
C1_ORDER = C1 / 'inputs/FAST_PATH_STAGE2_C1.md'
C1_ORDER_SHA = '6029450b746aed4493a4790fe5cc6504835387b70c7fc658e3bd3279fd96f35d'
ORDER = HERE / 'inputs/REAL_CFD_1.md'
ORDER_SHA = 'cf2ff9a225c4cf0d1c220ed8f1d2cdcb99d897e5c9d9e9fa1a033e622c92ab5b'
S0 = '7e6953e721'
S0_S = HERE.parent / 'FX2/G5/build645/S/run_receipt.json'
MODES = ('S', 'OFF')


def config_for(mode):
    assert mode in MODES
    if mode == 'S':
        base, digest = SURFACE_CONFIG, SURFACE_CONFIG_SHA
        assert sha(DEFAULT_CONFIG('S1')) == digest
        checksum = base.with_suffix('.cfg.sha256')
        assert checksum.read_text().split() == [digest, base.name], checksum
    else:
        base, digest = OFF_CONFIG, OFF_CONFIG_SHA
    assert sha(base) == digest, base
    expected = base.read_bytes().splitlines(keepends=True)
    edits = {
        b'CFL_ADAPT= NO\n': b'CFL_ADAPT= YES\n',
        b'ADAP_FLOW_ITER= ( 1 )\n': b'ADAP_FLOW_ITER= ( 500 )\n',
        b'OUTPUT_WRT_FREQ= 1\n': b'OUTPUT_WRT_FREQ= 100000\n',
        b'OUTPUT_FILES= ( RESTART )\n': b'OUTPUT_FILES= ( RESTART, PARAVIEW )\n',
    }
    for old, new in edits.items():
        assert expected.count(old) == 1, (base, old)
        expected[expected.index(old)] = new
    assert expected[-1].endswith(b'\n'), base
    expected.append(b'HISTORY_OUTPUT= ( ITER, RMS_RES, AERO_COEFF, CFL_NUMBER, LINSOL, ITER_TIME )\n')
    config = HERE / f'run_{mode.lower()}.cfg'
    assert config.read_bytes() == b''.join(expected), config
    checksum = config.with_suffix('.sha256')
    assert checksum.read_text().split() == [sha(config), config.name], checksum
    return config


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
    for order, digest in ((ORDER, ORDER_SHA), (C1_ORDER, C1_ORDER_SHA)):
        assert sha(order) == digest, order
        assert order.with_suffix('.sha256').read_text().split() == [digest, order.name], order


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
    fixture = C1 / 'scoped' / f'build{label}/fixtures.json'
    gate = scope.verified(fixture, fingerprints)
    assert gate['G1'] == [375, 114, 228] and gate['fp1_cohorts'] == 18, fixture
    paths = [fixture]
    fx2 = C1 / 'fixtures' / f'build{label}/comparison.json'
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


def bind_campaign(mode, source_paths):
    campaign = BASE['PRIOR']['bind_campaign']('on', source_paths)
    if mode == 'OFF':
        original = campaign['core_source']

        def core_source():
            source = workflow.replace_once(original(), '"report_only": FP1_ON, "records": guard_records',
                                           '"report_only": True, "records": guard_records')
            return workflow.replace_once(
                source, '                    if not FP1_ON:\n'
                        '                        stop_reason="p6c_one_sided_refinement_guard_failure"\n'
                        '                        break\n', '')

        campaign['core_source'] = core_source
    return campaign


def run_case(work, mode, build_dir, build, config, source_paths, payloads):
    label = build['build']
    case = work / f'full{label}_run1'
    campaign = bind_campaign(mode, source_paths)
    campaign['prepare'](work, label, build_dir, config)
    stage = 'full event gates'
    try:
        workflow.run_stage(work, 'full', [cut.PYTHON, '-B', str(Path(__file__).resolve()), str(build_dir),
                                          '--full', mode])
        BASE['status_ok'](case, build, config)
        hashes = workflow.seal_run(case, build['binary_sha256'], config, payloads, mode != 'OFF')
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
    assert build_dir == C1 / 'build647', build_dir
    build, _, _ = lab.prepare(build_dir)
    label = build['build']
    work = HERE / 'G5' / f'build{label}' / mode
    if lab_proof is None:
        lab_proof = C1 / 'cases' / f'build{label}/lab_acceptance.json'
    config = config_for(mode)
    source_paths = [Path(__file__).resolve(), C1 / 'run_fp2_b_g5.py', B / 'run_fp2_b_g5.py', INHERITED,
                    Path(lab.__file__).resolve(), Path(scope.__file__).resolve(),
                    ORDER, ORDER.with_suffix('.sha256'), HERE / 'source_check.md',
                    C1_ORDER, C1_ORDER.with_suffix('.sha256'), C1 / 'source_check.md',
                    build_dir / 'build_status.json', build_dir / 'source_pins.json',
                    config, config.with_suffix('.sha256'), FP2_A / 'run.cfg',
                    SURFACE_CONFIG, SURFACE_CONFIG.with_suffix('.cfg.sha256'), OFF_CONFIG]
    source_paths += [path for order in BASE['ORDERS'] for path in (order, order.with_suffix('.sha256'))]
    source_paths += reference_sources()
    source_paths += prerequisite_sources(build, lab_proof)
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    if (work / 'run_receipt.json').is_file():
        verify_run(work, mode, build, payloads)
    else:
        assert not (work / 'failed_attempt.json').exists(), work
        run_case(work, mode, build_dir, build, config, source_paths, payloads)
    print(f'RCFD1 R-{mode} PASSED.', flush=True)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('--lab-proof', type=Path, help='Override the candidate lab_acceptance.json path.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=MODES)
    group.add_argument('--full', choices=MODES, help='Internal supervised full controller.')
    group.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        verify_order()
        for mode in MODES:
            config_for(mode)
            campaign = bind_campaign(mode, [])
            source = campaign['core_source']()
            compile(source, str(workflow.CORE), 'exec')
            assert ('stop_reason="p6c_one_sided_refinement_guard_failure"' in source) == (mode == 'S')
            assert campaign['prepare'].__globals__['core_source'] is campaign['core_source']
        reference_sources()
        print('RCFD1 configs, inherited inputs and controller bindings verified.')
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
