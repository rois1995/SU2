"""Run the hidden IN1 sheet-placement fixture on its pinned test driver."""
from argparse import ArgumentParser
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True

import run_lab as lab

a = lab.a
HERE = lab.HERE
ORDER = HERE / 'inputs/FP2_B_CORRECTION_3.md'
SHA = '2b305d1c1fcd00aa5f4c59c1ca437068aee33a21095843fc344ac73d09e9adc4'
R4 = HERE / 'source_check_correction_3.md'
SELECTOR = '[.][NativeFastPathStage2SheetPlacement]'
SHEET_PIN_SHA = 'cb872f38f3efdb8cdf4de216b24a0b042cea7616956c135f8111119b9c595348'

def measure_sheet(log, pins):
    rows = {kind: [] for kind in ('ordinary', 'tolerance', 'far', 'counts')}
    for line in log.splitlines():
        if line.startswith('FP2_B_C3 '):
            _, kind, data = line.split(' ', 2)
            rows[kind].append(json.loads(data))
    counts = rows['counts'][0] if len(rows['counts']) == 1 else {}
    ordinary, tolerance, far = (rows[kind] for kind in ('ordinary', 'tolerance', 'far'))
    old_walk = {tuple(edge) for edge in pins['walk_refusals']}
    outside_old_walk = [row['edge'] for row in tolerance if tuple(row['edge']) not in old_walk]
    walk_keys = ('walk_none', 'walk_tol8', 'walk_far', 'walk_no_start', 'walk_exception')
    checks = {
        'ordinary_bit_pins': ordinary == pins['ordinary'],
        'tolerance_edges_previously_refused_walk': not outside_old_walk,
        'all_eligible_edges_reported': sum(counts.get(key, 0) for key in walk_keys) == pins['baseline']['tested'],
        'first_tolerance_rows_reported': counts.get('accepted_tolerance', -1) >= 0
        and len(tolerance) == min(3, counts['accepted_tolerance']),
        'all_far_rows_reported': len(far) == counts.get('walk_far'),
    }
    return {'passed': all(checks.values()), 'checks': checks, 'counts': counts,
            'ordinary': ordinary, 'tolerance': tolerance, 'far': far, 'outside_old_walk': outside_old_walk}


def run(build, sheet_pins=None):
    assert a.sha(ORDER) == SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [SHA, ORDER.name]
    receipt, binary, pins = lab.prepare(build)
    original, _, inherited, in1, _, _ = a.evidence_inputs()
    helpers = (Path(__file__).resolve(), ORDER, ORDER.with_suffix('.sha256'), R4)
    pins.update({**inherited, **{str(path): a.sha(path) for path in helpers}})
    if sheet_pins is not None:
        assert a.sha(sheet_pins) == SHEET_PIN_SHA
        pins[str(sheet_pins)] = SHEET_PIN_SHA
        baseline = a.cut.read(sheet_pins)
    original['verify'](pins)
    name = 'SU2_FP2_IN'
    environment = {name: in1[name]['path']}
    root = HERE / 'scoped' / build.name / 'sheet'
    provenance = {key: receipt[key] for key in lab.FINGERPRINTS}
    provenance.update(pins=pins, output=str(root.parent))
    measure = (lambda log: measure_sheet(log, baseline)) if sheet_pins is not None else None
    result = a.fixture(root, binary, 1, SELECTOR, environment, provenance, measure=measure)
    original['verify'](pins)
    assert result['passed'], root
    return result


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--pins', type=Path)
    args = parser.parse_args()
    result = run(args.build.resolve(), args.pins.resolve() if args.pins else None)
    print(json.dumps({key: result[key] for key in ('passed', 'catch_summaries', 'wait', 'measurement')}, indent=2))


if __name__ == '__main__':
    main()
