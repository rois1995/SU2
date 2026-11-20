"""Run the six FP1 lab pairs in the prescribed order, preserving partial sweeps."""

from argparse import ArgumentParser
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import time

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from run_g1 import cut, sha, verify_build
from run_fp0 import case_input, peak_kib, run_case

FP0 = HERE.parent.parent / 'FP0'
G5 = HERE.parent.parent / 'N4/section6_3_after_N11/round_8_5t_P/G5/full587'
MANIFEST = FP0 / 'case_input_manifest.json'
ORDER = HERE.parent / 'inputs/FP1_RESUME_3.md'
ORDER_SHA = '0db5cccf8b48cc100e3eacf3b381bf7ef779c216b03e6d1919d1f2da03473b7d'
PAIRS = (('LAB2', 'V2'), ('LAB1', 'V2'), ('LAB2', 'V3'), ('LAB1', 'V1'), ('LAB2', 'V1'), ('LAB2', 'V0'))
REPORT = re.compile(r'^FP1_LAB (load|before|pass|after|exact|strict)\s+(.+)$')
FIELDS = re.compile(r'(\w+)=(\S*)')


def verify(pins):
    for path, expected in pins.items():
        assert sha(Path(path)) == expected, path


def load_input(dataset):
    part = 'B' if dataset == 'LAB1' else 'A'
    home, environment = case_input(part)
    info = cut.read(MANIFEST)[part]
    assert info['exact_coordinate_global_id_match'] and info['exact_metric_roundtrip']
    cfg = Path(environment['SU2_NATIVE_FROZEN_CONFIG'])
    sensor = Path(environment['SU2_NATIVE_FROZEN_SENSOR3D'])
    config = cfg.read_bytes()
    mesh = 'm6.su2' if dataset == 'LAB1' else 'adapted_mesh_adap_00001.su2'
    restart = 'restart_flow_adap_00000.dat' if dataset == 'LAB1' else 'restart_flow_adap_00001.dat'
    names = (mesh, mesh + '.native_ref', restart, 'baseline_euler_seed.dat')
    assert set(str(G5 / name) for name in names) == set(info['pinned_sources'])
    assert sha(G5 / 'run.cfg') == info['source_config_sha256']
    expected = (G5 / 'run.cfg').read_bytes()
    if dataset == 'LAB1':
        assert config == expected and config.count(b'ADAP_NATIVE_RANKS= 6\n') == 1
        config = config.replace(b'ADAP_NATIVE_RANKS= 6\n', b'ADAP_NATIVE_RANKS= 1\n', 1)
    else:
        for old, new in ((b'ADAP_NATIVE_RANKS= 6\n', b'ADAP_NATIVE_RANKS= 1\n'),
                         (b'MESH_FILENAME= m6.su2\n', f'MESH_FILENAME= {mesh}\n'.encode()),
                         (b'SOLUTION_FILENAME= baseline_euler_seed\n',
                          f'SOLUTION_FILENAME= {Path(restart).stem}\n'.encode())):
            assert expected.count(old) == 1, old
            expected = expected.replace(old, new, 1)
        assert config == expected
    assert b'ADAP_NATIVE_LOCAL_CORE' not in config
    for line in (b'ADAP_NATIVE_RANKS= 1', b'ADAP_NATIVE_MEMORY_LIMIT= 12288',
                 b'ADAP_NATIVE_JOB_MEMORY_LIMIT= 19073'):
        assert config.splitlines().count(line) == 1, line
    copies = {home / name: info['pinned_sources'][str(G5 / name)] for name in names}
    inputs = {str(cfg): info['config_sha256'], str(sensor): info['sensor_sha256'],
              str(G5 / 'run.cfg'): info['source_config_sha256'], **info['pinned_sources'],
              **{str(path): digest for path, digest in copies.items()}}
    return config, sensor, copies, inputs, info['counts']


def write_once(path, value):
    if not path.exists():
        cut.save(path, value)
    assert cut.read(path) == value, path


def prepare(case, inputs, pins):
    config, sensor, copies, adopted, _ = inputs
    cfg = case / 'run.cfg'
    if not cfg.exists():
        with cfg.open('xb') as output:
            output.write(config)
    assert cfg.read_bytes() == config
    bound = {**pins, **adopted, str(cfg): hashlib.sha256(config).hexdigest()}
    for source, digest in copies.items():
        destination = case / source.name
        if not destination.exists():
            shutil.copyfile(source, destination)
        bound[str(destination)] = digest
    verify(bound)
    write_once(case / 'pins.json', bound)
    return cfg, sensor, bound


def observe(log):
    rows, sweeps = {}, []
    for line in log.splitlines(keepends=True):
        if not line.endswith(('\n', '\r')):
            continue
        if line.startswith('NATIVE3D_LOCAL_SWEEP '):
            sweeps.append(dict(FIELDS.findall(line)))
        elif match := REPORT.match(line):
            assert match[1] not in rows, match[1]
            rows[match[1]] = dict(FIELDS.findall(match[2]))
    return rows, sweeps


def finish(case, dataset, variant, pins, counts):
    wait = cut.read(case / 'test_wait.json')
    assert wait['controller'] == cut.read(case / 'test_launch.json')['controller']
    assert wait['terminal'] and cut.terminal(wait['controller'])
    verify(pins)
    log = (case / 'test_outer.log').read_text(errors='replace')
    rows, sweeps = observe(log)
    if 'before' in rows:
        assert all(int(rows['before'][key]) == counts[key] for key in ('cells', 'points'))
    passed = rows.get('pass')
    sweep_counts_match = None
    if passed is not None:
        assert passed['variant'] == variant and passed['self_check'] == '1'
        by_kind = Counter(row['kind'] for row in sweeps)
        expected = {name: int(passed[f'sweeps_{name}']) for name in ('collapse', 'split', 'quality')
                    if int(passed[f'sweeps_{name}'])}
        sweep_counts_match = dict(by_kind) == expected
    exact = rows.get('exact')
    valid = None if exact is None else exact['nonpositive'] == '0'
    if passed is not None and exact is not None:
        assert passed['cells_out'] == exact['cells']
    peak = peak_kib(case / 'time.txt') if (case / 'time.txt').is_file() else None
    state = ('wall_limit' if wait['returncode'] == 124 else
             'test_failure' if wait['returncode'] else 'completed')
    error = re.search(r'unexpected exception with message:\s*([^\n]+)', log, re.IGNORECASE)
    if state == 'completed':
        assert set(rows) == {'load', 'before', 'pass', 'after', 'exact', 'strict'} and valid
        assert passed['cells_out'] == rows['after']['cells'] == rows['strict']['cells']
    artifacts = ('command.json', 'test_host_preflight.json', 'test_launch.json', 'test_wait.json', 'test_outer.log',
                 'time.txt')
    receipt = {'dataset': dataset, 'variant': variant, 'state': state, 'wait': wait, 'peak_rss_kib': peak,
               'within_job_memory': peak <= 19073 * 1024 if peak is not None else None,
               'pass_seconds': float(passed['seconds']) if passed is not None else None, 'exact_validity': valid,
               'sweep_counts_match': sweep_counts_match, 'first_exception': error[1].strip() if error else None,
               'rows': rows, 'sweeps': sweeps, 'pins': pins,
               'artifacts': {name: sha(case / name) for name in artifacts if (case / name).is_file()}}
    write_once(case / 'lab_receipt.json', receipt)
    return {key: value for key, value in receipt.items() if key not in ('pins', 'artifacts', 'sweeps')}


def launch(case, binary, cfg, sensor, variant):
    assert not any((case / name).exists() for name in ('test_launch.json', 'test_outer.log')), case
    if (case / 'command.json').exists():
        # A preflight can fail before the wrapper creates a log or spawns the test.
        prior = case / f'preflight_{time.time_ns()}'
        prior.mkdir()
        for name in ('command.json', 'test_host_preflight.json'):
            if (case / name).exists():
                (case / name).rename(prior / name)
    run_case(case, binary, 1, '[.][NativeLocalCoreLab]', 1200,
             {'SU2_NATIVE_FROZEN_CONFIG': str(cfg), 'SU2_NATIVE_FROZEN_SENSOR3D': str(sensor),
              'SU2_NATIVE_LOCAL_LAB_VARIANT': variant})


def run(build, verify_only):
    status_path, source_pins = build / 'build_status.json', build / 'source_pins.json'
    status = cut.read(status_path)
    binary, source_sha = verify_build(status, source_pins)
    kept = (MANIFEST, Path(__file__).resolve(), FP0 / 'run_fp0.py', HERE.parent / 'run_g1.py', cut.PREFLIGHT)
    pins = {**{str(path): sha(path) for path in kept}, str(ORDER): ORDER_SHA, str(status_path): sha(status_path),
            str(source_pins): source_sha, str(binary): status['test_binary_sha256'], status['binary']: status['binary_sha256']}
    verify(pins)
    inputs = {name: load_input(name) for name in ('LAB1', 'LAB2')}
    if verify_only:
        return {'verified': True, 'launched': False, 'build': status['build'], 'pairs': PAIRS,
                'config_sha256': {name: hashlib.sha256(rows[0]).hexdigest() for name, rows in inputs.items()}}

    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert status['wait']['terminal'] and cut.terminal(status['wait']['controller'])
    for key in tuple(os.environ):
        if key.startswith('SU2_NATIVE_'):
            del os.environ[key]
    root = HERE / 'lab'
    root.mkdir(exist_ok=True)
    results = []
    for dataset, variant in PAIRS:
        case = root / f'{dataset}_{variant}'
        case.mkdir(exist_ok=True)
        cfg, sensor, bound = prepare(case, inputs[dataset], pins)
        if not (case / 'test_wait.json').exists():
            launch(case, binary, cfg, sensor, variant)
        result = finish(case, dataset, variant, bound, inputs[dataset][-1])
        verify_build(status, source_pins)
        results.append(result)
        print(f"{case.name}: {result['state']}, pass wall {result['pass_seconds']}, "
              f"exact validity {result['exact_validity']}", flush=True)
    hold = [row['dataset'] for row in results if row['variant'] == 'V2' and
            (row['exact_validity'] is not True or row['pass_seconds'] is None or row['pass_seconds'] > 600)]
    summary = {'build': status['build'], 'test_binary_sha256': status['test_binary_sha256'],
               'source_pins_sha256': source_sha, 'v2_hold': hold, 'results': results}
    write_once(root / 'lab_summary.json', summary)
    return summary


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    print(json.dumps(run(args.build.resolve(), args.verify_only), indent=2))
