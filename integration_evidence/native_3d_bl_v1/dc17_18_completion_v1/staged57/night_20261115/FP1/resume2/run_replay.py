"""Run one adopted FP0 frozen-volume replay with the FP1 core enabled."""

from argparse import ArgumentParser
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import shutil
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from run_g1 import cut, sha, verify_build
from run_fp0 import case_input, run_case

FP0 = HERE.parent.parent / 'FP0'
G5 = HERE.parent.parent / 'N4/section6_3_after_N11/round_8_5t_P/G5/full587'
MANIFEST = FP0 / 'case_input_manifest.json'
ORDER = HERE.parent / 'inputs/FP1_RESUME_2.md'
ORDER_SHA = '92b1eaa6a01b3c053b0a877f6cbc5ba9974b03050d6be827f6d4efc56b91750c'
TIMER = HERE.parents[2] / 'f3b_20261115/check_timer_tree.py'
ON = b'ADAP_NATIVE_LOCAL_CORE= YES\n'


def verify(pins):
    for path, expected in pins.items():
        assert sha(Path(path)) == expected, path


def prepare_input(event):
    part = 'B' if event == 1 else 'A'
    home, env = case_input(part)
    info = cut.read(MANIFEST)[part]
    assert info['exact_coordinate_global_id_match'] and info['exact_metric_roundtrip']
    cfg, sensor = (Path(env[key]) for key in ('SU2_NATIVE_FROZEN_CONFIG', 'SU2_NATIVE_FROZEN_SENSOR3D'))
    adopted = (G5 / 'run.cfg').read_bytes()
    assert sha(G5 / 'run.cfg') == info['source_config_sha256']
    base = cfg.read_bytes()
    mesh = 'm6.su2' if event == 1 else 'adapted_mesh_adap_00001.su2'
    restart = f'restart_flow_adap_{event - 1:05}.dat'
    names = {mesh, mesh + '.native_ref', restart, 'baseline_euler_seed.dat'}
    assert set(map(str, (G5 / name for name in names))) == set(info['pinned_sources'])
    copies = {name: (home / name, info['pinned_sources'][str(G5 / name)]) for name in sorted(names)}
    if event == 2:
        for old, new in ((b'MESH_FILENAME= m6.su2\n', f'MESH_FILENAME= {mesh}\n'.encode()),
                         (b'SOLUTION_FILENAME= baseline_euler_seed\n', f'SOLUTION_FILENAME= {Path(restart).stem}\n'.encode())):
            assert adopted.count(old) == 1, old
            adopted = adopted.replace(old, new, 1)
        assert base.count(b'ADAP_NATIVE_RANKS= 1\n') == 1
        base = base.replace(b'ADAP_NATIVE_RANKS= 1\n', b'ADAP_NATIVE_RANKS= 6\n', 1)
    assert base == adopted and base.endswith(b'\n') and b'ADAP_NATIVE_LOCAL_CORE' not in base
    for expected in (b'ADAP_NATIVE_RANKS= 6', b'WRT_PERFORMANCE= YES', b'ADAP_NATIVE_MAX_TIME= 5400',
                     b'ADAP_NATIVE_MEMORY_LIMIT= 12288', b'ADAP_NATIVE_JOB_MEMORY_LIMIT= 19073'):
        assert base.splitlines().count(expected) == 1, expected
    pins = {str(cfg): info['config_sha256'], str(sensor): info['sensor_sha256'],
            str(G5 / 'run.cfg'): info['source_config_sha256'], str(ORDER): ORDER_SHA,
            **info['pinned_sources'], **{str(path): digest for path, digest in copies.values()}}
    return base + ON, sensor, copies, pins, info['counts']['cells']


def observations(case):
    parser = runpy.run_path(str(TIMER))
    coverage, first_message, logs = {}, None, {}
    for path in sorted(case.glob('rank_output/*/rank.*/stdout')):
        rank = path.parent.name.rsplit('.', 1)[1]
        log = path.read_text(errors='replace')
        snapshots = [item for item in parser['snapshots'](log) if item['summary']['root'] == 'remesh'
                     and item['summary']['boundary'] == 'final' and item['summary']['rank'] == rank]
        if snapshots:
            item = parser['assess'](snapshots[-1])
            coverage[rank] = {'coverage': float(item['summary']['coverage']), 'ledger_passed': item['passed'],
                              'fixedstorage': int(item['summary']['fixedstorage'])}
        if rank == '0':
            match = re.search(r'Native3D rejected event: (.*?); accepted CFD mesh/solution retained\.', log)
            if match:
                first_message = match[1]
        logs[str(path.relative_to(case))] = sha(path)
        stderr = path.with_name('stderr')
        if stderr.is_file():
            logs[str(stderr.relative_to(case))] = sha(stderr)
    return first_message, {'report_only': True, 'by_rank': coverage,
                           'minimum_coverage': min((row['coverage'] for row in coverage.values()), default=None)}, logs


def finished_receipt(case):
    result = cut.read(case / 'replay_receipt.json')
    verify(result['pins'])
    wait = result['wait']
    assert wait == cut.read(case / 'test_wait.json')
    assert wait['terminal'] and cut.terminal(wait['controller'])
    verify({str(case / path): digest for path, digest in result['artifacts'].items()})
    return result


def run(build, event, verify_only):
    status_path, source_pins = build / 'build_status.json', build / 'source_pins.json'
    build_status = cut.read(status_path)
    binary, source_sha = verify_build(build_status, source_pins)
    config, sensor, copies, pins, expected_cells = prepare_input(event)
    keep = (MANIFEST, TIMER, Path(__file__).resolve(), FP0 / 'run_fp0.py', HERE.parent / 'run_g1.py', cut.PREFLIGHT)
    pins.update({str(path): sha(path) for path in keep})
    pins.update({str(status_path): sha(status_path), str(source_pins): source_sha,
                 str(binary): build_status['test_binary_sha256'], build_status['binary']: build_status['binary_sha256']})
    verify(pins)
    if verify_only:
        return {'verified': True, 'launched': False, 'event': event, 'build': build_status['build'],
                'config_sha256': hashlib.sha256(config).hexdigest(), 'sensor_sha256': pins[str(sensor)]}

    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    built = build_status['wait']
    assert built['terminal'] and built['returncode'] == 0 and cut.terminal(built['controller'])
    if event == 2:
        first = finished_receipt(HERE / 'event1')
        assert first['event'] == 1 and first['build'] == build_status['build']
        assert first['accepted'] and first['pins'][str(binary)] == pins[str(binary)]
    case = HERE / f'event{event}'
    case.mkdir(exist_ok=True)
    cfg = case / 'run.cfg'
    if not cfg.exists():
        with cfg.open('xb') as output:
            output.write(config)
    assert cfg.read_bytes() == config
    pins[str(cfg)] = sha(cfg)
    for name, (source, digest) in copies.items():
        local = case / name
        if not local.exists():
            shutil.copyfile(source, local)
        assert sha(local) == digest, local
        pins[str(local)] = digest
    if not (case / 'pins.json').exists():
        cut.save(case / 'pins.json', pins)
    assert cut.read(case / 'pins.json') == pins
    if (case / 'replay_receipt.json').exists():
        return finished_receipt(case)

    if not (case / 'test_wait.json').exists():
        assert not any((case / name).exists() for name in ('command.json', 'test_launch.json', 'test_outer.log')), case
        for key in tuple(os.environ):
            if key.startswith('SU2_NATIVE_'):
                del os.environ[key]
        run_case(case, binary, 6, '[.][NativeFrozenVolume3D]', 5400,
                 {'SU2_NATIVE_FROZEN_CONFIG': str(cfg), 'SU2_NATIVE_FROZEN_SENSOR3D': str(sensor)})
    wait = cut.read(case / 'test_wait.json')
    assert wait['controller'] == cut.read(case / 'test_launch.json')['controller']
    assert wait['terminal'] and cut.terminal(wait['controller'])
    verify_build(build_status, source_pins)
    verify(pins)

    native_path = case / 'construction_result.json'
    native = cut.read(native_path) if native_path.is_file() else None
    if native is not None:
        assert native['initial_cells'] == expected_cells and native['workers'] == 6 and native['CFD_steps'] == 0
        assert isinstance(native['accepted'], bool)
    accepted = bool(native and native['accepted'] and wait['returncode'] == 0)
    first_message, timing, artifacts = observations(case)
    for name in ('construction_result.json', 'frozen_geometry_accepted.su2', 'command.json', 'time.txt',
                 'test_host_preflight.json', 'test_launch.json', 'test_wait.json', 'test_outer.log'):
        if (case / name).is_file():
            artifacts[name] = sha(case / name)
    if not accepted and not first_message:
        first_message = (f"Frozen fixture exited {wait['returncode']}; see test_outer.log."
                         if wait['returncode'] else 'Native rejection without a reason row; see the rank logs.')
    result = {'event': event, 'build': build_status['build'], 'accepted': accepted, 'native': native,
              'first_message': first_message, 'config_sha256': pins[str(cfg)], 'wait': wait,
              'timing_observation': timing, 'pins': pins, 'artifacts': artifacts}
    cut.save(case / 'replay_receipt.json', result)
    return result


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--event', type=int, choices=(1, 2), default=1)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    result = run(args.build.resolve(), args.event, args.verify_only)
    print(json.dumps({key: value for key, value in result.items() if key not in ('pins', 'artifacts')}, indent=2))
    raise SystemExit(not (args.verify_only or result['accepted']))
