"""Run correction 3 fixtures or the single authorized S1 workflow."""
from argparse import ArgumentParser
from pathlib import Path
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True

import run_correction2_validation as c2
import run_correction3_sheet as sheet

HERE = sheet.HERE
DRIVER = Path(__file__).resolve()
GATE_CASES = ('N1', 'N1Invariant', 'N6', 'N6P')
PIN_RECEIPT = HERE / 'inputs/FP2_B_CORRECTION_3_sheet_pins.json'


def fixtures(build):
    sheet.run(build, PIN_RECEIPT)
    receipt = sheet.a.cut.read(build / 'build_status.json')
    case = HERE / 'scoped' / build.name / 'sheet' / 'fixture_status.json'
    c2.prior.scope.verify_fixture(case)
    assert sheet.a.cut.read(case)['passed']
    original = c2.prior.prepare

    def prepare(directory, extra=()):
        return original(directory, (*extra, sheet.ORDER, sheet.ORDER.with_suffix('.sha256'),
                                     sheet.R4, sheet.ORDER.parent / 'FP2_B_CORRECTION_3_sheet_pins.json',
                                     case, DRIVER, Path(sheet.__file__).resolve()))

    assert receipt['build'] == int(build.name.removeprefix('build'))
    with patch.object(c2.prior, 'prepare', prepare):
        c2.scoped(build, 'fixtures')


def s1(build):
    label = sheet.a.cut.read(build / 'build_status.json')['build']
    case = HERE / f'cases/build{label}/N6P/measurement.json'
    n6p = sheet.a.cut.read(case)
    assert n6p['passed'] and n6p['measurement']['valid'] and n6p['measurement']['gate_applies']
    support = [DRIVER, sheet.ORDER, sheet.ORDER.with_suffix('.sha256'), sheet.R4, PIN_RECEIPT,
               HERE / 'run_correction3_lab.py']
    invoke = c2.full.run_case

    def run_case(work, mode, build_dir, receipt, config, sources, expected, runs, payloads):
        return invoke(work, mode, build_dir, receipt, config, [*sources, *support], expected, runs, payloads)

    with patch.object(c2, 'GATE_CASES', GATE_CASES), patch.object(c2.full, 'run_case', run_case):
        c2.g5(build, 'S1')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('mode', choices=('fixtures', 'S1'))
    args = parser.parse_args()
    if args.mode == 'fixtures':
        fixtures(args.build.resolve())
    else:
        s1(args.build.resolve())


if __name__ == '__main__':
    main()
