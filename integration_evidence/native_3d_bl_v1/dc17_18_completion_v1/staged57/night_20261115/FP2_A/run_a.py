"""Run the FP2-A Part A box, IN1, LAB-S, inherited FP1 fixtures and G1 on one pinned build."""

from argparse import ArgumentParser
from collections import defaultdict
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys
import time

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FP1 = HERE.parent / 'FP1'
sys.path.insert(0, str(FP1))
from run_fixtures import ANSI, FIXTURES, PASS
from run_fp0 import peak_kib
from run_g1 import cut, sha, run_case, verify_build

ORDER = HERE / 'inputs/FAST_PATH_STAGE2_A.md'
ORDER_SHA = 'c499613b6960740e5666800f1cf75a37f2d1611c99d9a23846f8eab8774f76b3'
BASE = 'abc9450b86'
NRR = 'Common/src/adaptation/native_engine3d/NativeRemeshRun.inc'
LAB_RUNNER = FP1 / 'resume4/run_lab.py'
LAB_ORIGINAL = FP1 / 'resume3/run_lab.py'
NAME_IN1, NAME_LAB = '[.][NativeFastPathStage2Surface]', '[.][NativeLocalCoreSurfaceLab]'
NAME_BOX = '[.][NativeLocalCoreSurfaceA]'
MEASUREMENT = HERE.parent / 'FP2_0/run_measurements.py'
V2_PRIOR = FP1 / 'resume4/lab/LAB1_V2/lab_receipt.json'
RUN_A_PRIOR = HERE.parent / 'FP2_0B/verified_measurements.json'
PARITY = FP1 / 'resume3/g_baseline/baseline.json'
WALL = 5400
MEMORY = 19073 * 1024
ROW = re.compile(r'^(FP2_A (?:IN1_STATE|LAB_S(?:_PROOF|_ENTITY|_EXACT)?)|'
                 r'NATIVE3D_LOCAL_SURFACE(?:_STATE)?|FP1_LAB exact)\s+(.+)$')
FIELD = re.compile(r'(\w+)=("(?:[^"\\]|\\.)*"|\S*)')
ARTIFACTS = ('command.json', 'test_host_preflight.json', 'test_launch.json', 'test_wait.json', 'test_outer.log',
             'time.txt', 'resources.jsonl', 'owned_processes.json')


def setup_lab(output):
    runner = runpy.run_path(str(LAB_RUNNER))['launch'].__globals__
    runner.update(HERE=output, PAIRS=(('LAB1', 'V2'),), SELECTOR=NAME_LAB, WALL=WALL,
                  __file__=str(Path(__file__).resolve()))
    return runner


def evidence_inputs():
    original = runpy.run_path(str(LAB_ORIGINAL))
    measurements = runpy.run_path(str(MEASUREMENT))
    inputs = measurements['inputs']()
    in1 = {key: inputs[key] for key in ('SU2_FP2_IN', 'SU2_FP2_SENSOR')}
    pinned = {
        str(ORDER): ORDER_SHA,
        str(PARITY): 'afdd3beceb87eb113ec2fdc6c6efb1e67652d4dc89353d49585f709a3f260023',
        str(V2_PRIOR): 'dcc7251b88253da4fb39362bf8ebad3f2ba2600e561cd8a4b399667c78005b82',
        str(RUN_A_PRIOR): '3d69e3e4c2554f9f593f794dd3bb0c29b476f6db12c746634d4a66d73e3494e9',
    }
    original['verify'](pinned)
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    parity = cut.read(PARITY)
    assert parity['passed'] and Path(parity['path']).stat().st_size == parity['bytes']
    pinned[parity['path']] = parity['sha256']
    pinned.update({row['path']: row['sha256'] for row in in1.values()})
    lab1 = original['load_input']('LAB1')
    pinned.update(lab1[3])
    support = (Path(__file__).resolve(), LAB_RUNNER, LAB_ORIGINAL, MEASUREMENT, FP1 / 'run_fixtures.py',
               FP1 / 'run_g1.py', HERE.parent / 'FP0/run_fp0.py', original['MANIFEST'], cut.PREFLIGHT,
               FP1.parents[1] / 'run_cases.py', ORDER.with_suffix('.sha256'))
    pinned.update({str(path): sha(path) for path in support})
    original['verify'](pinned)
    return original, measurements, pinned, in1, lab1, parity['path']


def sealed(path, value):
    if not path.exists():
        cut.save(path, value)
    assert cut.read(path) == value, path


def resumed(case):
    launch, wait = (case / f'test_{name}.json' for name in ('launch', 'wait'))
    if launch.exists():
        if not wait.is_file():
            raise RuntimeError(f'Inspect the existing owned process before continuing: {launch}')
        result = cut.read(wait)
        assert result['terminal'] and result['controller'] == cut.read(launch)['controller']
        assert cut.terminal(result['controller'])
        return result
    assert not wait.exists() and not (case / 'test_outer.log').exists(), case
    if (case / 'command.json').exists():
        prior = case / f'preflight_{time.time_ns()}'
        prior.mkdir()
        for name in ('command.json', 'test_host_preflight.json'):
            if (case / name).exists():
                (case / name).rename(prior / name)
    return None


def observed_rows(lines):
    rows = defaultdict(list)
    for number, raw in enumerate(lines, 1):
        if raw.endswith('\n') and (match := ROW.fullmatch(raw.rstrip('\r\n'))):
            rows[match[1]].append({'line': number, 'text': match[0], 'fields': dict(FIELD.findall(match[2]))})
    return dict(rows)


def expect_one(rows, name):
    selected = rows.get(name, ())
    return selected[0]['fields'] if len(selected) == 1 else {}


def summarize_box(logs):
    counts = [PASS.findall(ANSI.sub('', path.read_text(errors='replace'))) for path in logs]
    return {'passed': all(len(row) == 1 for row in counts), 'catch_summaries': counts}


def summarize_in1(log, measurements):
    lines = log.splitlines(keepends=True)
    rows, malformed = measurements['measurements'](lines)
    native = observed_rows(lines)
    state = expect_one(native, 'FP2_A IN1_STATE')
    needed = {'IN1_load', 'IN1_setup', 'IN1_sensor', 'P_L', 'P_D'}
    missing = sorted(needed - {row['name'] for row in rows})
    fields = {'faces', 'points', 'traces', 'revision', 'splits', 'seconds_build', 'modeled_peak'}
    return {'passed': not missing and not malformed and fields <= state.keys(),
            'json_rows': rows, 'rows': native, 'missing_json_rows': missing, 'malformed_json_rows': malformed}


def summarize_lab(log, counts):
    native = observed_rows(log.splitlines(keepends=True))
    surface = expect_one(native, 'NATIVE3D_LOCAL_SURFACE')
    state = expect_one(native, 'NATIVE3D_LOCAL_SURFACE_STATE')
    volume = expect_one(native, 'FP2_A LAB_S')
    exact = expect_one(native, 'FP1_LAB exact')
    proof = expect_one(native, 'FP2_A LAB_S_PROOF')
    entities = native.get('FP2_A LAB_S_ENTITY', ())
    keys = {tuple(row['fields'].get(key) for key in ('body', 'sheet', 'marker')) for row in entities}
    accepted = (len(entities) == len(keys) == 4 and all(None not in key for key in keys) and
                all(row['fields'].get('accepted') == '1' for row in entities))
    labels = {'rank': '0', 'stage': '0', 'pass': '1', 'applicable': '1'}
    return {'passed': (all(surface.get(key) == value for key, value in labels.items()) and
                       state.get('faces') is not None and
                       volume.get('cells_in') == str(counts['cells']) and exact.get('nonpositive') == '0' and
                       exact.get('cells') == volume.get('cells_out') and
                       proof.get('accepted') == '1' and proof.get('entities') == str(len(entities)) and
                       proof.get('classified') == state['faces'] and accepted),
            'rows': native}


def fixture(case, binary, ranks, selector, environment, provenance, measure=None, lab=None):
    case.mkdir(parents=True, exist_ok=True)
    source = case / 'provenance.json'
    sealed(source, {**provenance, 'selector': selector, 'ranks': ranks, 'environment': environment,
                    'seconds_limit': WALL})
    path = case / 'fixture_status.json'
    if path.exists():
        result = cut.read(path)
        assert result['provenance_sha256'] == sha(source)
        assert result['wait'] == cut.read(case / 'test_wait.json') and cut.terminal(result['wait']['controller'])
        for name, digest in result['artifacts'].items():
            assert sha(case / name) == digest, name
    else:
        wait = resumed(case)
        if not wait:
            if lab:
                lab['launch'](case, binary, Path(environment['SU2_NATIVE_FROZEN_CONFIG']),
                              Path(environment['SU2_NATIVE_FROZEN_SENSOR3D']), 'V2')
                wait = resumed(case)
            else:
                run_case(case, binary, ranks, selector, WALL, environment)
                wait = resumed(case)
        logs = [case / 'test_outer.log'] if ranks == 1 else sorted(case.glob('rank_output/*/rank.*/stdout'))
        catch = summarize_box(logs)
        passed = wait['returncode'] == 0 and len(logs) == ranks and catch['passed']
        data = measure(logs[0].read_text(errors='replace')) if measure else {}
        peak = peak_kib(case / 'time.txt') if (case / 'time.txt').is_file() else None
        if lab:
            processes = cut.read(case / 'owned_processes.json')
            assert processes['all_observed_terminal'] and all(cut.terminal(row) for row in processes['identities'])
            passed = passed and 0 < max(peak or 0, processes['sampled_job_peak_lower_bound_kib']) <= MEMORY
        if measure and ranks == 1:
            passed = passed and bool(peak and peak <= MEMORY) and data['passed']
        artifacts = set(logs) | {case / name for name in ARTIFACTS if (case / name).is_file()}
        result = {**catch, 'passed': passed, 'wait': wait, 'measurement': data, 'peak_rss_kib': peak,
                  'provenance_sha256': sha(source),
                  'artifacts': {str(name.relative_to(case)): sha(name) for name in sorted(artifacts)}}
        cut.save(path, result)
    if not result['passed']:
        raise RuntimeError(f'Fixture did not pass: {path}')
    print(f'{case.relative_to(Path(provenance["output"]))}: passed in {result["wait"]["elapsed_seconds"]:.3f} s.',
          flush=True)
    return result


def run(build, output, verify_only):
    status_path, manifest = build / 'build_status.json', build / 'source_pins.json'
    status = cut.read(status_path)
    binary, source_hash = verify_build(status, manifest)
    assert not status['new_warnings']
    source_pins = cut.read(manifest)
    header = cut.REPO / 'Common/include/adaptation/CNativeLocalSurface3D.hpp'
    assert source_pins[str(header)] == sha(header)
    baseline = subprocess.check_output(['git', 'show', f'{BASE}:{NRR}'], cwd=cut.REPO)
    assert (cut.REPO / NRR).read_bytes() == baseline, 'Part A changed the NRR call path.'
    original, measurements, pins, in1, lab1, parity = evidence_inputs()
    pins.update({str(status_path): sha(status_path), str(manifest): source_hash,
                 str(binary): status['test_binary_sha256']})
    output = output or HERE / 'part_a' / f"build{status['build']}"
    original['verify'](pins)
    if verify_only:
        return {'verified': True, 'launched': False, 'build': status['build'], 'output': str(output),
                'selectors': (NAME_BOX, NAME_IN1, NAME_LAB)}

    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    assert status['wait']['terminal'] and cut.terminal(status['wait']['controller'])
    for key in tuple(os.environ):
        if key.startswith(('SU2_NATIVE_', 'SU2_FP2_', 'SU2_FP1_')):
            del os.environ[key]
    prov = {'build': status['build'], 'test_binary_sha256': status['test_binary_sha256'],
            'source_pins_sha256': source_hash, 'output': str(output), 'pins': pins}
    fixture(output / 'box', binary, 1, NAME_BOX, {}, prov)
    in1_environment = {key: row['path'] for key, row in in1.items()}
    in1_result = fixture(output / 'IN1', binary, 1, NAME_IN1, in1_environment, prov,
                         measure=lambda log: summarize_in1(log, measurements))
    lab_case = output / 'lab/LAB1_V2'
    lab_case.mkdir(parents=True, exist_ok=True)
    cfg, sensor, bound = original['prepare'](lab_case, lab1, pins)
    env = {'SU2_NATIVE_FROZEN_CONFIG': str(cfg), 'SU2_NATIVE_FROZEN_SENSOR3D': str(sensor),
           'SU2_NATIVE_LOCAL_LAB_VARIANT': 'V2'}
    lab_result = fixture(lab_case, binary, 1, NAME_LAB, env, {**prov, 'pins': bound},
                         measure=lambda log: summarize_lab(log, lab1[-1]), lab=setup_lab(output))
    for tag, cohorts in FIXTURES.items():
        for ranks, selector in cohorts:
            env = {'SU2_FP1_G_OWNED_BASELINE': parity} if tag == 'H' else {}
            fixture(output / f'FP1/{tag}/rank{ranks}', binary, ranks, selector, env, prov)
    g1 = output / 'G1'
    subprocess.run([cut.PYTHON, '-B', str(FP1 / 'run_g1.py'), str(g1), str(status_path), str(manifest)],
                   cwd=cut.REPO, check=True)
    g1_counts = {cohort: cut.read(g1 / cohort / 'comparison.json') for cohort in ('serial', 'mpi2', 'mpi4')}
    assert [row['records'] for row in g1_counts.values()] == [375, 114, 228]
    assert all(row['identity_pass'] for row in g1_counts.values())
    original['verify'](pins)
    assert verify_build(status, manifest) == (binary, source_hash)
    prior = cut.read(RUN_A_PRIOR)['cases'][0]
    assert prior['case'] == 'A' and not prior['missing_rows'] and not prior['malformed_rows']
    reference = {name: [row['value'] for row in prior['rows'] if row['name'] == name]
                 for name in ('P_L', 'P_D', 'P_entities', 'P3', 'P4')}
    summary = {**prov, 'passed': True, 'NRR_matches_base': BASE, 'IN1': in1_result['measurement'],
               'LAB_S': lab_result['measurement'], 'FP2_0b_run_A': reference,
               'FP1_LAB1_V2': cut.read(V2_PRIOR)['rows'],
               'G1': {key: row['records'] for key, row in g1_counts.items()}}
    sealed(output / 'part_a_receipt.json', summary)
    return {'passed': True, 'build': status['build'], 'receipt': str(output / 'part_a_receipt.json'),
            'G1': summary['G1']}


def self_check():
    raw = ('FP2_A LAB_S cells_in=10 cells_out=12\nFP1_LAB exact cells=12 nonpositive=0\n'
           'NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1\nNATIVE3D_LOCAL_SURFACE_STATE faces=8\n'
           'FP2_A LAB_S_PROOF entities=4 classified=8 classify_above_tol=1 accepted=1\n' +
           ''.join(f'FP2_A LAB_S_ENTITY body=0 sheet={i} marker=0 accepted=1\n' for i in range(4)))
    assert summarize_lab(raw, {'cells': 10})['passed']
    assert not summarize_lab(raw.rstrip(), {'cells': 10})['passed']
    assert not summarize_lab(raw.replace('nonpositive=0', 'nonpositive=1'), {'cells': 10})['passed']
    assert not summarize_lab(raw.replace('classified=8', 'classified=7'), {'cells': 10})['passed']
    assert setup_lab(HERE)['WALL'] == WALL
    _, _, _, in1, lab, parity = evidence_inputs()
    assert Path(parity).exists() and in1['SU2_FP2_SENSOR']['path'] == str(lab[1])


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', nargs='?', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--owned', type=Path, help='internal LAB-S worker')
    args = parser.parse_args()
    if args.owned:
        case = args.owned.resolve()
        raise SystemExit(setup_lab(case.parents[1])['owned'](case))
    if args.self_check:
        self_check()
        print('Part A input, parser and hard lab wrapper binding checks passed.')
    elif args.build:
        print(json.dumps(run(args.build.resolve(), args.output.resolve() if args.output else None, args.verify_only),
                         indent=2))
    else:
        parser.error('supply the pinned build directory or --self-check')
