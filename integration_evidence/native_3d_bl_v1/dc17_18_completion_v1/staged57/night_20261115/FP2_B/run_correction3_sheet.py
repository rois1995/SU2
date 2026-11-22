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


def run(build):
    assert a.sha(ORDER) == SHA
    assert ORDER.with_suffix('.sha256').read_text().split() == [SHA, ORDER.name]
    receipt, binary, pins = lab.prepare(build)
    original, _, inherited, in1, _, _ = a.evidence_inputs()
    helpers = (Path(__file__).resolve(), ORDER, ORDER.with_suffix('.sha256'), R4)
    pins.update({**inherited, **{str(path): a.sha(path) for path in helpers}})
    original['verify'](pins)
    name = 'SU2_FP2_IN'
    environment = {name: in1[name]['path']}
    root = HERE / 'scoped' / build.name / 'sheet'
    provenance = {key: receipt[key] for key in lab.FINGERPRINTS}
    provenance.update(pins=pins)
    result = a.fixture(root, binary, 1, SELECTOR, environment, provenance)
    original['verify'](pins)
    assert result['passed'], root
    return result


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    args = parser.parse_args()
    result = run(args.build.resolve())
    print(json.dumps({key: result[key] for key in ('passed', 'catch_summaries', 'wait')}, indent=2))


if __name__ == '__main__':
    main()
