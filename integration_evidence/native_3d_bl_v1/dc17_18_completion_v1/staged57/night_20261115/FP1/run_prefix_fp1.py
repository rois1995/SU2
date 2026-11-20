"""Run FP1 off-path prefixes with the frozen N22 controller and Work order 3 comparisons."""

from argparse import ArgumentParser
from importlib.util import module_from_spec, spec_from_file_location
import os
from pathlib import Path
import runpy
import sys

from run_g1 import ITEM, cut, sha
from build_fp1 import verify

N22 = ITEM.parent / 'N22'
sys.path.insert(0, str(N22))
from compare_prefix import C_FIELDS, N4, compare_cases, parse_case, split_communication

ORIGINAL = cut.PHASE / 'N22C/validation'
CAPTURE = ORIGINAL / 'run_g2_gq_prefix_591.py'
G2_REFERENCE = ITEM.parent / 'N14/section6_12/validation/cases/N14_G2_552'
G2_PREPARATION = Path(cut.read(N22 / 'C/scoped_acceptance.json')['G2']['case'])
GQ_REFERENCE = Path(cut.read(N22 / 'gq_reference.json')['case'])


def report_counters(reference, candidate, gate):
    before, after = (parse_case(path, gate) for path in (reference, candidate))
    reports = {}
    for stream in sorted(before):
        rows = []
        for index, (old, new) in enumerate(zip(before[stream]['plain'], after[stream]['plain']), 1):
            keys = sorted(C_FIELDS & (old.fields.keys() | new.fields.keys()))
            if keys:
                rows.append({'row': index, 'marker': old.marker,
                             'reference': {key: old.fields.get(key) for key in keys},
                             'candidate': {key: new.fields.get(key) for key in keys}})
        reports[stream] = rows
    return reports


def check_g2(case):
    """Keep N14's exact states and counters, substituting N22 C preparation communication only."""
    common = compare_cases('C', 'G2', G2_REFERENCE, case)
    before, candidate, preparation = (parse_case(path, 'G2')
                                      for path in (G2_REFERENCE, case, G2_PREPARATION))
    assert set(before) == set(candidate) == set(preparation)
    plain = N4.compare_streams({name: row['plain'] for name, row in before.items()},
                               {name: row['plain'] for name, row in candidate.items()}, 'G2')
    counters = []
    for name in sorted(before):
        for n, (old, new, n22) in enumerate(zip(before[name]['timers'], candidate[name]['timers'],
                                                preparation[name]['timers'], strict=True)):
            old_calls, new_calls, prep_calls = [split_communication(snap, True)[1] for snap in (old, new, n22)]
            phases = set(old_calls) | set(new_calls) | set(prep_calls)
            for phase in sorted(phases):
                source = prep_calls if phase == 'preparation' else old_calls
                expected = source.get(phase, {}).get('calls')
                observed = new_calls.get(phase, {}).get('calls')
                counters.append({'stream': name, 'snapshot': n, 'phase': phase, 'expected': expected,
                                 'actual': observed, 'passed': expected == observed})
    first = plain.get('first_difference') or common.get('first_difference') or next(
        (entry for entry in counters if not entry['passed']), None)
    return {'passed': first is None, 'first_difference': first, 'strict_integer_rows': plain,
            'common_rows': common, 'communication_calls': counters,
            'preparation_reference': str(G2_PREPARATION)}


def bind_controller(build, root, corrected=False):
    """Bind the qualified prefix stopping conditions to this build and fresh evidence root."""
    source = runpy.run_path(str(CAPTURE))['load_runner']()
    old_patch = source['patched_controller']
    status_file, pins_file = build / 'build_status.json', build / 'source_pins.json'
    receipt = cut.read(status_file)
    solver = Path(receipt['binary'])
    extra = source['A2'] / 'validation/unchanged_build499_source_pins.json'

    def check_pins(strict=True):
        live = cut.read(pins_file)
        runtime = cut.read(extra)
        assert receipt['passed'] and receipt['state'] == 'terminal'
        assert receipt['source_pins_sha256'] == sha(pins_file)
        assert cut.terminal(receipt['wait']['controller'])
        assert not live.keys() & runtime.keys()
        pins = dict(live, **runtime)
        verify(pins)
        assert sha(solver) == receipt['binary_sha256']
        return {'binary': str(solver), 'source_build_binary': str(solver), 'source_pins': pins,
                'binary_sha256': receipt['binary_sha256'], 'binary_matches_build_status': True,
                'source_pin_mismatches': {}, 'build_status': str(status_file),
                'source_pins_sha256': sha(pins_file), 'additional_source_pins_sha256': sha(extra)}

    def patch():
        text = old_patch()
        check = ITEM / 'reports' / ('FP1_resume_source_check.md' if corrected else 'FP1_source_check.md')
        replacements = {
            'GROUP / "source_check.md"': f'Path({str(check)!r})',
            'PURPOSE / "validate_g2_gq.py"': f'Path({str(ORIGINAL / "validate_g2_gq.py")!r})',
            'PURPOSE / "run_g2_gq_prefix.py"': f'Path({str(CAPTURE)!r})',
            'BUILD / "terminal_handles.json"': 'BUILD / "build_wait.json"',
            'BUILD / "generated_build_graph_after.json"': 'BUILD / "build_plan.json"',
        }
        for old, new in replacements.items():
            assert old in text, old
            text = text.replace(old, new)
        if corrected:
            order = ITEM / 'inputs/FP1_RESUME.md'
            line = '    input_hashes.update(runtime_pins)'
            assert text.count(line) == 1
            text = text.replace(line, line + f'\n    input_hashes[{str(order)!r}] = sha(Path({str(order)!r}))')
        return text

    spec = spec_from_file_location('validate_g2_gq', ORIGINAL / 'validate_g2_gq.py')
    validation = module_from_spec(spec)
    sys.modules[spec.name] = validation
    spec.loader.exec_module(validation)
    source.update(HERE=root, BUILD=build, CASES=root / 'cases', patched_controller=patch)
    env, _ = source['namespace']()
    env.update(SCRIPT=Path(__file__).resolve(), verify_pins=check_pins)
    return env


def controller(build, root, gate, case, corrected):
    env = bind_controller(build, root, corrected)
    env['verify_pins']()
    os.environ['OMPI_MCA_osc'] = 'pt2pt'
    sys.argv = [str(env['N4_SCRIPT']), str(case), '--gate', gate]
    env['main']()


def run(build, gate, root, corrected):
    root.mkdir(parents=True, exist_ok=True)
    case = root / 'cases' / gate
    if not (case / 'status.json').is_file():
        flags = ['--corrected'] if corrected else []
        cut.run_gate(root, gate, [cut.PYTHON, '-B', str(Path(__file__).resolve()),
                                 str(build), gate, '--output', str(root), '--capture', *flags])
    status = cut.read(case / 'status.json')
    assert status['all_owned_handles_terminal'] and all(cut.terminal(row) for row in status['owned_identities'])
    assert status['inputs_unchanged'] and status['source_and_binary_pins_unchanged']
    assert status['gate_passed'], status
    current = cut.read(case / 'provenance.json')
    reference = cut.read((G2_REFERENCE if gate == 'G2' else GQ_REFERENCE) / 'provenance.json')
    assert current['config_sha256'] == reference['config_sha256']
    assert current['binary_sha256'] == cut.read(build / 'build_status.json')['binary_sha256']
    assert current['source_pins_sha256'] == sha(build / 'source_pins.json')
    for basename in ('m6.su2', 'm6.su2.native_ref', 'baseline_euler_seed.dat'):
        hashes = [{Path(path).name: digest for path, digest in provenance['input_hashes'].items()}
                  for provenance in (current, reference)]
        assert hashes[0][basename] == hashes[1][basename], basename
    reference_case = G2_REFERENCE if gate == 'G2' else GQ_REFERENCE
    if gate == 'G2' and not corrected:
        result = check_g2(case)
    else:
        result = compare_cases('C', gate, reference_case, case)
        result['report_only_integer_fields'] = report_counters(reference_case, case, gate)
    cut.save(root / f'{gate}_comparison.json', result)
    print(f"{gate} {'PASSED' if result['passed'] else 'FAILED'}; comparison at {root}", flush=True)
    return result['passed']


def self_check():
    build = ITEM / 'B/build599'
    root = ITEM / 'C/prefix'
    env = bind_controller(build, root)
    assert env['PURPOSE'] == root and env['BUILD_STATUS'] == build / 'build_status.json'
    bind_controller(build, root, corrected=True)
    assert env['SOURCE_PINS'] == build / 'source_pins.json'
    assert env['N4_SCRIPT'] == ITEM.parent / 'N4/run_prefix_controller.py'
    assert env['EXPECTED_G2_WORK'] == 90_809_869
    assert (env['N1_FULL'] / 'run.cfg').is_file()
    assert compare_cases('C', 'G2', G2_REFERENCE, G2_PREPARATION)['passed']
    result = check_g2(G2_PREPARATION)
    witness = result['first_difference']
    print(witness['key'])
    assert witness['key'] == 'integer.collective_calls: 120046 != 114332'
    assert all(row['passed'] for row in result['communication_calls'] if row['phase'] == 'preparation')
    construction = [row for row in result['communication_calls'] if row['phase'] == 'construction']
    assert len(construction) == 6 and all((r['expected'], r['actual']) == (340843, 333799) for r in construction)
    rows = report_counters(G2_REFERENCE, G2_PREPARATION, 'G2')
    first = rows[sorted(rows)[0]][0]
    assert first['reference']['collective_calls'] == '120046' and first['candidate']['collective_calls'] == '114332'


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, nargs='?')
    parser.add_argument('gate', choices=('G2', 'GQ'), nargs='?')
    parser.add_argument('--capture', action='store_true')
    parser.add_argument('--output', type=Path, default=ITEM / 'C/prefix')
    parser.add_argument('--corrected', action='store_true')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        sys.exit(0)
    root = args.output.resolve()
    if args.capture:
        controller(args.build.resolve(), root, args.gate, root / 'cases' / args.gate, args.corrected)
    else:
        sys.exit(not run(args.build.resolve(), args.gate, root, args.corrected))
