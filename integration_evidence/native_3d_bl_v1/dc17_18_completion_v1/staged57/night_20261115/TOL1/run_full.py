"""Run and authenticate one TOL1 scalar or equal-list standing S workflow."""

from argparse import ArgumentParser
from contextlib import contextmanager
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory
from unittest.mock import patch

import build_fp3 as tol1_build

HERE, DEF1 = tol1_build.HERE, tol1_build.DEF1
SELF = Path(__file__).resolve()
PARTS = ('scalar', 'equal')
SCALAR = b'ADAP_HAUSD= 1E-4'
EQUAL = b'ADAP_HAUSD= (WING3D, 1E-4, SYMMETRY, 1E-4, FARFIELD, 1E-4)'


def load_def1():
    path = tol1_build.from_base(DEF1 / 'run_full.py')
    spec = spec_from_file_location('tol1_inherited_def1_full', path)
    module = module_from_spec(spec)
    sys.path.insert(0, str(DEF1))
    try:
        sys.modules.pop('build_fp3')
        spec.loader.exec_module(module)
    finally:
        sys.path.remove(str(DEF1))
        sys.modules['build_fp3'] = tol1_build
        sys.path.insert(0, str(HERE))
    return module


prior = load_def1()
E, p3, cut, workflow = prior.E, prior.p3, prior.cut, prior.workflow
sha, path_pin = E.sha, E.path_pin
legacy = sys.modules['legacy_config']


def derive(original, part):
    data, inserted = prior.derived(original)
    assert len(re.findall(rb'(?m)^' + SCALAR + rb'(?=\r?$)', data)) == 1
    if part == 'equal':
        data = re.sub(rb'(?m)^' + SCALAR + rb'(?=\r?$)', EQUAL, data, count=1)
    return data, inserted


def authenticate_cfg(settings, part, target, launch=None):
    source, receipts, original = settings
    assert sha(source) == original, source
    data, inserted = derive(source.read_bytes(), part)
    if launch is None:
        if not target.exists():
            with target.open('xb') as output:
                output.write(data)
    assert target.read_bytes() == data, target
    if launch:
        case = target.parent
        cfg, metadata, recorded = legacy.metadata_config(case)
        assert cfg == target and recorded == legacy.digest(data), metadata
        command = case / 'command.json'
        if command.is_file():
            env = cut.read(command)['environment']
            if 'SU2_NATIVE_FROZEN_CONFIG' in env:
                assert Path(env['SU2_NATIVE_FROZEN_CONFIG']).resolve() == cfg.resolve(), command
    record = {'sealed_config': path_pin(source), 'sealed_receipts': [path_pin(path) for path in receipts],
              'inserted_lines': inserted, 'replacement': [SCALAR.decode(), EQUAL.decode()] if part == 'equal' else None,
              'derived_config': path_pin(target)}
    if launch:
        record['launch_metadata'] = path_pin(metadata)
    legacy.write_json(target.with_suffix('.derivation.json'), record)
    return record


def full_config(label, part):
    return HERE / f'build{label}' / f'{part}_S.cfg'


def bound_restart(part, name, label, build_dir):
    template, source = prior.bound_restart(part, name, label, build_dir)
    if part == 'scalar' or name != 'run_restart':
        return template, source
    reference = prior.reference_configuration()[1][0]
    data, _ = derive(reference.read_bytes(), part)
    # DEF1's independently authenticated scalar guard runs before this single change.
    guard = (f'config = config.replace({SCALAR.decode()!r}, {EQUAL.decode()!r}, 1)\n'
             f'assert config.encode() == {data!r}\n')
    anchor = '            compile(source, str(BASE), "exec")\n'
    extra = (f'            tolerance_guard = {guard!r}\n'
             '            assert source.count(old) == 1, old\n'
             '            source = source.replace(old, tolerance_guard + old, 1)\n')
    return template, workflow.replace_once(source, anchor, extra + anchor)


@contextmanager
def bound(label, part, config=None):
    target = config or full_config(label, part)
    with patch.dict(E.__dict__, {'HERE': HERE, 'SELF': SELF, 'bound_restart': bound_restart,
                                  'PARTS': {name: ('S', 0) for name in PARTS}}):
        with E.bound(), patch.object(p3, 'config_for', lambda mode: target):
            yield


def root(label, part):
    return HERE / 'G5' / f'build{label}' / part


def reference():
    tol1_build.verify_order()
    prior.check_order()
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    references, historical = prior.inherited_reference(payloads)
    full, restart = prior.reference_configuration()
    assert prior.p3_original_config('S').read_bytes() == full[0].read_bytes()
    return payloads, references[0]['payload_hashes'], historical, full, restart


def mobility_and_tolerance(case):
    from collect_fp3_full import PREFIX

    paths = sorted(case.glob('rank_output/*/rank.0/stdout'))
    assert len(paths) == 1, (case, paths)
    mobility, tolerance = [], []
    for line in paths[0].open(errors='replace'):
        line = PREFIX.sub('', line)
        if 'Native3D curved mobility:' in line:
            match = re.search(r'Native3D curved mobility: protected_edges=(\d+) corners=(\d+)', line)
            assert match, line
            mobility.append({'protected_edges': int(match[1]), 'corners': int(match[2])})
        if line.startswith('NATIVE3D_TOLERANCE '):
            tolerance.append(dict(re.findall(r'(\w+)=([^\s]+)', line)))
    return mobility, tolerance


def require_same(field, actual, expected):
    if actual != expected:
        raise E.FailedCheck('result', {'field': field, 'actual': actual, 'expected': expected})


def tolerance_gate(mobility, rows, scalar_mobility):
    if len(rows) != 2 or not mobility or not scalar_mobility:
        raise E.FailedCheck('measurement', {'field': 'tolerance or mobility rows', 'rows': rows, 'mobility': mobility,
                                           'scalar_mobility': scalar_mobility})
    require_same('curved mobility', mobility, scalar_mobility)
    for event, row in enumerate(rows, 1):
        prefix = f'event {event}'
        for field, expected in (('rank', 0), ('seam_corners', 0), ('seam_edges', 0)):
            require_same(f'{prefix} {field}', int(row[field]), expected)
        require_same(f'{prefix} strict', float(row['strict']), 1e-4)
        markers = {}
        for marker in row['markers'].split(','):
            name, index, value, group = marker.split(':')
            assert name not in markers, row
            markers[name] = (int(index), float(value), int(group))
        require_same(f'{prefix} marker names', sorted(markers), ['FARFIELD', 'SYMMETRY', 'WING3D'])
        require_same(f'{prefix} marker indices', sorted(entry[0] for entry in markers.values()), [0, 1, 2])
        for name, (_, value, group) in markers.items():
            require_same(f'{prefix} {name} tolerance', value, 1e-4)
            require_same(f'{prefix} {name} class', group, 0)


def run(label, part):
    tol1_build.verify_order()
    build = tol1_build.verify_build(label)
    work = root(label, part)
    result = work / 'case_summary.json'
    if result.is_file():
        saved = cut.read(result)
        assert all(saved[key] == build[key] for key in E.FINGERPRINTS)
        assert saved['part'] == part
        E.check_pins(saved['pins'])
        return saved['status']
    scoped = HERE / 'controls' / f'build{label}' / 'step1_scoped.json'
    E.require(scoped, build)
    required = [scoped]
    if part == 'equal':
        scalar_path = root(label, 'scalar') / 'case_summary.json'
        scalar = E.require(scalar_path, build)
        E.check_pins(scalar['pins'])
        required.append(scalar_path)

    payloads, expected, historical, full, restart_settings = reference()
    target = full_config(label, part)
    authenticate_cfg(full, part, target)
    inputs = [SELF, tol1_build.ORDER, tol1_build.ORDER.with_suffix('.sha256'), tol1_build.SOURCE_CHECK,
              target, target.with_suffix('.derivation.json'), *required,
              *(path for path in prior.sources(DEF1 / 'build681', DEF1 / 'controls/build681/step1_scoped.json', historical)
                if DEF1 / 'build681' not in path.parents),
              *(target.parent / name for name in ('build_status.json', 'source_pins.json'))]
    work.mkdir(parents=True, exist_ok=True)
    summary = {key: build[key] for key in E.FINGERPRINTS}
    summary.update(status='passed', passed=True, part=part, mode='S', probe=0,
                   failure_kind=None, first_difference=None)
    evidence = {}
    try:
        with bound(label, part):
            path = E.run_trial(label, part, build, inputs, payloads)
            receipt = E.authenticate(path.parent, label, part, build, payloads)
        summary['run'] = path_pin(path)
        summary['collection'] = receipt['collection']
        evidence = E.full_pins(path, receipt)
        restart = Path(receipt['restart'])
        case = Path(receipt['case'])
        full_proof = authenticate_cfg(full, part, case / 'run.cfg', launch=True)
        restart_proof = authenticate_cfg(restart_settings, part, restart.parent / 'run.cfg', launch=True)
        for cfg in (case / 'run.cfg', restart.parent / 'run.cfg'):
            proof = cfg.with_suffix('.derivation.json')
            evidence[str(proof)] = sha(proof)
        assert cut.read(restart)['artifact_sha256']['run.cfg'] == restart_proof['derived_config']['sha256'], restart
        summary['legacy_configs'] = {'full': full_proof, 'isolated_restart': restart_proof}
        summary['comparison'] = p3.compare_hashes(expected, receipt['hashes'])
        if not summary['comparison']['equal']:
            raise E.FailedCheck('result', summary['comparison']['first_difference'])
        mobility, rows = mobility_and_tolerance(case)
        summary['mobility'] = mobility
        summary['tolerance_rows'] = rows
        if part == 'equal':
            tolerance_gate(mobility, rows, scalar['mobility'])
            _, restart_rows = mobility_and_tolerance(restart.parent)
            require_same('restart tolerance rows', restart_rows, [])
    except E.c3.WatchdogOpen as error:
        summary.update(status='OPEN', passed=False, failure_kind='helper_watchdog', first_difference=str(error))
    except E.FailedCheck as error:
        summary.update(status='FAILED', passed=False, failure_kind=error.kind, first_difference=error.first)

    for name in ('failed_attempt.json', 'full_watchdog.json', 'partial_collection.json', 'collection.json',
                 'contact_failure.json', 'restart_watchdog.json', 'fp3_restart_result_failure.json'):
        evidence.update({str(path): sha(path) for path in work.glob(f'**/{name}')})
    for path in work.glob('**/failed_attempt.json'):
        evidence.update({str(path.parent / name): digest for name, digest in cut.read(path)['artifacts'].items()})
    summary['pins'] = {str(path): sha(path) for path in inputs} | evidence
    workflow.sealed_write(result, summary)
    print(f'TOL1 {part} {summary["status"]}: {result}', flush=True)
    return summary['status']


def self_check(label):
    _, _, _, full, restart_settings = reference()
    with TemporaryDirectory(prefix='tol1-full-') as temp:
        base = Path(temp)
        for part in PARTS:
            config = base / f'{part}.cfg'
            record = authenticate_cfg(full, part, config)
            assert bool(record['replacement']) == (part == 'equal')
            restart, _ = derive(restart_settings[0].read_bytes(), part)
            assert (EQUAL if part == 'equal' else SCALAR) in restart
            for retry in (False, True):
                work = base / f'{part}-{retry}'
                work.mkdir()
                with bound(label, part, config):
                    p3.campaign(part, [SELF], retry)['prepare'](work, label, tol1_build.BASE_BUILD, config)
                    p3.verify_generated(work, part, label, retry)
                module = runpy.run_path(str(work / f'run_restart{label}.py'))
                source = module['runner_context']().load_base().retargeted_source()
                assert 'expected_restart' in source and (part == 'equal') == (f'config = config.replace({SCALAR.decode()!r}' in source)
                compile(source, '<TOL1 restart worker>', 'exec')
    row = dict(rank='0', strict='0.0001', seam_corners='0', seam_edges='0',
               markers='FARFIELD:0:0.0001:0,WING3D:1:0.0001:0,SYMMETRY:2:0.0001:0')
    mobility = [{'protected_edges': 64, 'corners': 2}]
    tolerance_gate(mobility, [row, row], mobility)
    try:
        tolerance_gate(mobility, [row, row | {'seam_edges': '1'}], mobility)
    except E.FailedCheck as error:
        assert error.first == {'field': 'event 2 seam_edges', 'actual': 1, 'expected': 0}
    else:
        raise AssertionError('The equal-list seam check did not refuse a changed seam count.')
    print('TOL1 scalar and equal-list full/restart derivations and tolerance checks authenticated offline.')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int)
    parser.add_argument('part', choices=PARTS, nargs='?')
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--retry-full', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check(args.label)
    elif not args.part:
        parser.error('choose a workflow')
    elif args.worker:
        with bound(args.label, args.part):
            p3.launch_child(args.label, args.part, 1, args.retry_full)
    else:
        raise SystemExit({'passed': 0, 'FAILED': 1, 'OPEN': 75}[run(args.label, args.part)])
