"""Run the FP2-B correction 2 lab without changing the inherited work limits."""

from argparse import ArgumentParser
from decimal import Decimal
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import tempfile
from unittest.mock import patch

import run_correction1 as prior
import run_lab as lab

HERE = lab.HERE
a = lab.a
b0 = lab.b0
DRIVER = Path(__file__).resolve()
ORDER = HERE / 'inputs/FP2_B_CORRECTION_2.md'
ORDER_SHA = '933df9ee2224b908ca952cc80a7a3f7951f71ec2622818d12c841755721ed1df'
SOURCE_CHECK = HERE / 'source_check_correction_2.md'
D1 = HERE.parent / 'FP2_B_D1/G5/build635/S1'
D1_CASE = D1 / 'full635_run1'
D1_REPORTER = HERE.parent / 'FP2_B_D1/report_d1.py'
EVENT2_INPUTS = HERE / 'inputs/N7P'
EVENT2_CONFIG_SOURCE = HERE.parent / 'FP2_A/run.cfg'
EVENT2_CONFIG_SHA = '07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382'
EVENT2_MESH = 'adapted_mesh_adap_00001.su2'
EVENT2_RESTART = 'restart_flow_adap_00001.dat'
EVENT2_PAYLOADS = {
    EVENT2_MESH: 'f6fd75c71a6a42b222aa2d56e9de8a789e357b6ebd5d9b1f370ca4d428313cbd',
    EVENT2_MESH + '.native_ref': 'c59dabc30c39e2691bcedcef477fa4b3aaac66f601a175dbfa584d6e97c658a1',
    EVENT2_RESTART: '5edce653ec7ac205d7679781c33a40cdd4f49ab5a1e9bc93c50a46f455dc61f9',
    'baseline_euler_seed.dat': '1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880',
}
EVENT2_MARKERS = {'FARFIELD': 2829, 'SYMMETRY': 12183, 'WING3D': 61114}
PARTITIONS = {
    'N6P': ((48316, 51155, 56223, 51202, 50531, 45932), (9423, 9031, 6040, 6694, 9634, 8132)),
    'N7P': ((95408, 93521, 94879, 87449, 86552, 95137), (8490, 15690, 14277, 12452, 14459, 10758)),
}
REFERENCE_RANKS = {'N6P': (2, 3), 'N7P': (0,)}
S1_OUTPUT = {'N6P': {'handover_bad': 4126, 'faces': [73962, 76940]},
             'N7P': {'handover_bad': 1704, 'faces': [85180, 85422]}}
BINS = ('lt_1e_6', 'lt_1e_4', 'lt_1e_2', 'lt_01')
EDGE_FIELDS = ('metric', 'a', 'b', *BINS)
COUNTERS = ('left_long_neighbor', 'waits_length', 'refused_near')
RUNS = {name: prior.RUNS[name] for name in ('N1', 'N1Invariant', 'N6')}
RUNS.update({'H6off': (6, 'HandoverOff'), 'N6P': (6, 'HandoverProduction1'),
             'N7P': (6, 'HandoverProduction2')})
LINE = re.compile(r'^(FP2_B (?:pass|end|bad|state|proof|invariant|partition|donor_scan|donor_reject)|'
                  r'FP2_A LAB_S_PROOF|NATIVE3D_LOCAL_SURFACE) (.+)$')
BASE_COMMAND = lab.command
BASE_INPUT = b0.input_data


def verify_order():
    assert a.sha(ORDER) == ORDER_SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [ORDER_SHA, ORDER.name]
    assert SOURCE_CHECK.is_file(), SOURCE_CHECK


def verify_pins(pins):
    for path, digest in pins.items():
        assert a.sha(Path(path)) == digest, path


def committed(path, commit='bea11c37f7'):
    data = subprocess.check_output(['git', 'show', f'{commit}:{path.relative_to(a.cut.REPO)}'],
                                   cwd=a.cut.REPO)
    assert data == path.read_bytes(), path


def event2_input():
    cfg, sensor = (EVENT2_INPUTS / name for name in ('run.cfg', 'sensor.bin'))
    manifest_path = EVENT2_INPUTS / 'pins.json'
    manifest = a.cut.read(manifest_path)
    pins = manifest['pins']
    verify_pins(pins)
    assert manifest['counts'] == {'points': 112400, 'cells': 552946, 'surface_faces': 76126}
    assert manifest['markers'] == EVENT2_MARKERS
    assert pins[str(EVENT2_CONFIG_SOURCE)] == EVENT2_CONFIG_SHA
    copies = {D1_CASE / name: digest for name, digest in EVENT2_PAYLOADS.items()}
    assert all(pins[str(path)] == digest for path, digest in copies.items())
    return cfg.read_bytes(), sensor, copies, {**pins, str(manifest_path): a.sha(manifest_path)}, manifest['counts']


def prepare_event2():
    verify_order()
    if (EVENT2_INPUTS / 'pins.json').is_file():
        return event2_input()[-1]
    converter = HERE.parent / 'FP0/prepare_inputs.py'
    fp0 = runpy.run_path(str(converter))
    mesh_reader = fp0['CONVERTER']
    pins = {str(D1_CASE / name): digest for name, digest in EVENT2_PAYLOADS.items()}
    pins.update({str(EVENT2_CONFIG_SOURCE): EVENT2_CONFIG_SHA,
                 str(converter): a.sha(converter), str(mesh_reader): a.sha(mesh_reader)})
    verify_pins(pins)
    points, cells, markers = runpy.run_path(str(mesh_reader))['mesh'](D1_CASE / EVENT2_MESH)
    markers = {name: len(faces) for name, faces in markers.items()}
    assert (len(points), len(cells), markers) == (112400, 552946, EVENT2_MARKERS)
    cfg = EVENT2_CONFIG_SOURCE.read_bytes()
    for old, new in ((b'MESH_FILENAME= m6.su2\n', f'MESH_FILENAME= {EVENT2_MESH}\n'.encode()),
                     (b'SOLUTION_FILENAME= baseline_euler_seed\n',
                      f'SOLUTION_FILENAME= {Path(EVENT2_RESTART).stem}\n'.encode())):
        assert cfg.count(old) == 1, old
        cfg = cfg.replace(old, new, 1)
    for line in (b'ADAP_NATIVE_RANKS= 6', b'ADAP_NATIVE_MEMORY_LIMIT= 12288',
                 b'ADAP_NATIVE_JOB_MEMORY_LIMIT= 19073'):
        assert cfg.splitlines().count(line) == 1, line
    EVENT2_INPUTS.mkdir(exist_ok=True)
    config = EVENT2_INPUTS / 'run.cfg'
    if not config.is_file():
        with config.open('xb') as stream:
            stream.write(cfg)
    assert config.read_bytes() == cfg
    sensor = EVENT2_INPUTS / 'sensor.bin'
    with tempfile.TemporaryDirectory(dir=EVENT2_INPUTS) as work:
        generated = Path(work) / sensor.name
        fp0['sensor'](D1_CASE / EVENT2_RESTART, points, generated)
        if not sensor.is_file():
            os.link(generated, sensor)
        assert a.sha(sensor) == a.sha(generated)
    pins.update({str(path): a.sha(path) for path in (config, sensor)})
    a.sealed(EVENT2_INPUTS / 'pins.json', {'pins': pins, 'markers': markers,
             'counts': {'points': len(points), 'cells': len(cells), 'surface_faces': sum(markers.values())}})
    return event2_input()[-1]


def baseline_reference(name):
    """Read exact core and seed rows from D1's authenticated per-rank logs."""
    receipt_path = D1 / 'diagnosis_receipt.json'
    committed(receipt_path)
    receipt = a.cut.read(receipt_path)
    committed(D1_REPORTER)
    logs = {relpath: digest for relpath, digest in receipt['artifacts'].items() if relpath.endswith('/stdout')}
    assert len(logs) == 6
    lab.archived.verify_artifacts(D1, logs)
    pins = {str(receipt_path): a.sha(receipt_path), str(D1_REPORTER): a.sha(D1_REPORTER)}
    for relpath, digest in logs.items():
        path = D1 / relpath
        if path.is_file():
            pins[str(path)] = digest
        else:
            archived = lab.archived.archived(path)
            committed(archived)
            pins[str(archived)] = a.sha(archived)
    reader = runpy.run_path(str(D1_REPORTER))
    rows, ranks = reader['read_case'](D1_CASE)
    assert sorted(ranks) == list(range(6))
    event = 1 if name == 'N6P' else 2
    result = {}
    for rank in range(6):
        surface = next(row for row in rows['NATIVE3D_LOCAL_SURFACE']
                       if row.event == event and row.stage == 0 and row.rank == rank and row.fields['pass'] == '1')
        assert int(surface.fields['faces_in']) == PARTITIONS[name][1][rank]
        if rank not in REFERENCE_RANKS[name]:
            continue
        core = next(row for row in rows['NATIVE3D_LOCAL_SHORT']
                    if row.event == event and row.stage == 0 and row.rank == rank
                    and row.fields.get('checkpoint') == 'shifted' and row.fields.get('order') == '1')
        seed = next(row for row in rows['NATIVE3D_SEED_SHORT'] if row.event == event and row.rank == rank)
        result[str(rank)] = {
            'shifted': {key: core.fields[key] for key in EDGE_FIELDS},
            'seed': {'metric': seed.fields['e0_metric'], 'a': seed.fields['e0_a'], 'b': seed.fields['e0_b'],
                     **{key: seed.fields['lt_0_1' if key == 'lt_01' else key] for key in BINS}},
        }
    return result, pins


def command(case, binary, config, sensor, ranks, surface):
    recipe = BASE_COMMAND(case, binary, config, sensor, ranks, surface)
    if case.name == 'H6off':
        recipe[3:3] = ['SU2_FP2_B0_RUN=H6off', 'SU2_FP2_B0_EXTRA=none']
    return recipe


def one(rows):
    return rows[0] if len(rows) == 1 else {}


def fields_for_pass(log, kind, phase):
    return one([row for row in log.get(kind, ()) if row.get('pass') == str(phase)])


def shifted(log):
    bins = one([row for row in log.get('edge_bins', ()) if row['checkpoint'] == 'shifted'])
    edges = [row for row in log.get('edge', ()) if row['checkpoint'] == 'shifted']
    if not bins or not edges:
        return None
    edge = min(edges, key=lambda row: Decimal(row['metric']))
    lo, hi = sorted((int(edge['a']), int(edge['b'])))
    return {'metric': edge['metric'], 'a': str(lo), 'b': str(hi), **{key: bins[key] for key in BINS}}


def edge_matches(actual, expected):
    return (actual is not None and Decimal(actual['metric']) == Decimal(expected['metric'])
            and all(actual[field] == expected[field] for field in EDGE_FIELDS if field != 'metric'))


def partition_pairs(name, data):
    pairs = []
    for rank, (cells, faces) in enumerate(zip(*PARTITIONS[name])):
        log = data.get(rank, {})
        after = one(log.get('partition', ())).get('cells_after')
        core = fields_for_pass(log, 'pass', 1).get('cells_in')
        face = fields_for_pass(log, 'NATIVE3D_LOCAL_SURFACE', 1).get('faces_in')
        pairs.append({'rank': rank, 'cells_expected': cells, 'cells_after': int(after) if after else None,
                      'pass1_cells_in': int(core) if core else None, 'faces_expected': faces,
                      'pass1_faces_in': int(face) if face else None})
    return pairs


def donor_scan(data, ranks, result, gate):
    tests = [one(data.get(rank, {}).get('donor_scan', ())) for rank in range(ranks)]
    end = [one(data.get(rank, {}).get('end', ())) for rank in range(ranks)]
    result['donor_scan'] = tests
    result['donor_rejected'] = sum(int(row['rejected']) for row in tests) if all(tests) else None
    if not gate:
        return
    for rank, (scan, final) in enumerate(zip(tests, end)):
        if (not scan or scan['rank'] != str(rank) or int(scan['cells']) <= 0
                or scan['cells'] != final.get('cells_out')):
            result['problems'].append({'rank': rank, 'row': 'donor_scan', 'observed': scan})
        elif int(scan['rejected']):
            result['failed_limits'].append({'rank': rank, 'field': 'donor_rejected',
                                            'observed': int(scan['rejected']), 'maximum': 0})


def evaluate(name, data, expected, reference, baseline, baseline_receipt, invariant_path):
    if name == 'H6off':
        result = {'problems': [], 'failed_limits': []}
        donor_scan(data, 6, result, False)
        if result['donor_rejected'] is None:
            result['problems'].append({'row': 'donor_scan', 'expected_ranks': 6})
        return result
    if name not in PARTITIONS:
        result = prior.evaluate(name, data, expected, invariant_path)
        if name != 'N1Invariant':
            donor_scan(data, RUNS[name][0], result, True)
        for rank, log in data.items():
            for row in log.get('NATIVE3D_LOCAL_SURFACE', ()):
                if any(not row.get(key, '').isdigit() for key in COUNTERS):
                    result['problems'].append({'rank': rank, 'pass': int(row['pass']), 'row': 'correction2_counters'})
        return result
    pairs = partition_pairs(name, data)
    valid = all(row['cells_after'] == row['pass1_cells_in'] == row['cells_expected']
                and row['pass1_faces_in'] == row['faces_expected'] for row in pairs)
    comparisons = {}
    for rank, rows in reference.items():
        actual = shifted(data.get(int(rank), {}))
        comparisons[rank] = {**rows, 'observed': actual, 'equal_shifted': edge_matches(actual, rows['shifted'])}
    reproduced = valid and all(row['equal_shifted'] for row in comparisons.values())
    applies = not baseline and valid and baseline_receipt is not None and baseline_receipt['measurement']['reproduced']
    evidence = {'valid': valid, 'partition_pairs': pairs, 'shifted_pairs': comparisons,
                'reproduced': reproduced, 'gate_applies': applies}
    if baseline:
        return {**evidence, 'problems': [], 'failed_limits': []}
    pass_rows = {rank: {**log, 'edge_bins': [row for row in log.get('edge_bins', ())
                                            if row['checkpoint'] != 'shifted']}
                 for rank, log in data.items()}
    result = prior.BASELINE_EVALUATE(name, pass_rows, expected)
    result['failed_limits'] = [row for row in result['failed_limits']
                               if row['field'] in ('star_max', 'seconds_pass', 'v_limit', 'lt_1e_2')]
    for rank, log in data.items():
        after = shifted(log)
        if after is None:
            result['problems'].append({'rank': rank, 'row': 'shifted_edges'})
        elif int(after['lt_1e_2']):
            result['failed_limits'].append({'rank': rank, 'field': 'shifted_lt_1e_2',
                                            'observed': int(after['lt_1e_2']), 'maximum': 0})
    donor_scan(data, 6, result, True)
    if not applies:
        result['report_only_findings'] = {key: result.pop(key) for key in ('problems', 'failed_limits')}
        result.update(problems=[], failed_limits=[])
    return {**result, **evidence, 's1_report_reference': S1_OUTPUT[name],
            'baseline_reproduced': baseline_receipt['measurement']['reproduced'] if baseline_receipt else None}


def run(build, name, baseline, baseline_build, verify_only):
    verify_order()
    reference, pins = baseline_reference(name) if name in PARTITIONS else ({}, {})
    build_number = a.cut.read(build / 'build_status.json')['build']
    baseline_receipt = None
    if baseline_build is not None and name in PARTITIONS:
        path = prior.case_path(baseline_build, name) / 'measurement.json'
        if path.is_file():
            baseline_receipt = a.cut.read(path)
            assert baseline_receipt['case'] == name and baseline_receipt['build'] == baseline_build
            assert not baseline_receipt['measurement']['gate_applies']
            lab.archived.verify_artifacts(path.parent, baseline_receipt['artifacts'])
            pins[str(path)] = a.sha(path)
    invariant_path = prior.case_path(build_number, 'N1') / 'measurement.json' if name == 'N1Invariant' else None

    def prepare(directory):
        receipt, binary, original_pins = prior.BASELINE_PREPARE(directory)
        files = (DRIVER, prior.DRIVER, prior.OLD_LAB, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK)
        if invariant_path:
            assert lab.checked_receipt(invariant_path.parent, prior.build_fingerprints(receipt))['passed']
            pins[str(invariant_path)] = a.sha(invariant_path)
        return receipt, binary, {**original_pins, **pins, **{str(path): a.sha(path) for path in files}}

    def measurement(case, data, expected):
        return evaluate(case, data, expected, reference, baseline, baseline_receipt, invariant_path)

    with patch.multiple(lab, RUNS=RUNS, LINE=LINE, __file__=str(DRIVER), prepare=prepare, command=command,
                        evaluate=measurement), patch.object(b0, 'input_data',
                        event2_input_for_ranks if name == 'N7P' else BASE_INPUT):
        return lab.run(build, name, verify_only)


def event2_input_for_ranks(ranks):
    assert ranks == 6
    return event2_input()


def self_check():
    event1, _ = baseline_reference('N6P')
    event2, _ = baseline_reference('N7P')
    assert event1['2']['shifted']['metric'] == '3.9070055295542924e-10'
    assert event1['2']['shifted']['lt_1e_2'] == '362' and event1['2']['shifted']['lt_1e_6'] == '138'
    assert event1['3']['shifted']['lt_1e_2'] == '0'
    assert event2['0']['shifted']['lt_1e_2'] == '23' and event2['0']['shifted']['lt_1e_6'] == '10'
    assert (event2['0']['shifted']['a'], event2['0']['shifted']['b']) == ('115118', '115130')
    assert not edge_matches(event2['0']['seed'], event2['0']['shifted'])
    assert edge_matches(event2['0']['shifted'], event2['0']['shifted'])


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--only', choices=tuple(RUNS))
    group.add_argument('--owned', type=Path)
    group.add_argument('--prepare-event2', action='store_true')
    group.add_argument('--self-check', action='store_true')
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--baseline-build', type=int)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    if args.owned:
        with patch.multiple(lab, RUNS=RUNS, command=command):
            raise SystemExit(lab.owned(args.owned))
    if args.prepare_event2:
        print(json.dumps(prepare_event2(), indent=2))
        return
    if args.self_check:
        self_check()
        return
    if not args.build:
        parser.error('a build directory is required')
    if args.baseline and (args.only not in PARTITIONS or args.baseline_build is not None):
        parser.error('--baseline selects N6P or N7P on the D1 code')
    result = run(args.build.resolve(), args.only, args.baseline, args.baseline_build, args.verify_only)
    print(json.dumps({key: result[key] for key in ('case', 'passed', 'measurement')}, indent=2))
    if not result['passed']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
