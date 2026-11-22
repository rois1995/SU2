"""Run correction 2 fixtures and bind the corrected lab evidence to G5."""
from argparse import ArgumentParser
from pathlib import Path
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True

import run_correction2_lab as lab

prior = lab.prior
full = prior.full
a = lab.a
HERE = lab.HERE
DRIVER = Path(__file__).resolve()
VOLUME = ('CNativeLocalCore3DChanges.cpp', 'CNativeLocalCore3DPass.cpp')
GATE_CASES = ('N1', 'N1Invariant', 'N6', 'H6off', 'N6P', 'N7P')


def gate_e(directory):
    manifest = a.cut.read(directory / 'source_pins.json')
    reference = a.cut.read(HERE / 'build633/source_pins.json')
    paths = [str(a.cut.REPO / 'Common/src/adaptation/native_local' / leaf) for leaf in VOLUME]
    rows = {path: manifest[path] for path in paths}
    assert all(sha == reference[path] == a.sha(Path(path)) for path, sha in rows.items())
    return rows


def scoped(build, mode):
    prepare_base = prior.prepare

    def prepare(directory):
        receipt, binary, pins = prepare_base(directory, (DRIVER, lab.ORDER, lab.ORDER.with_suffix('.sha256'),
                                                         lab.SOURCE_CHECK, lab.DRIVER))
        pins.update(gate_e(directory))
        return receipt, binary, pins

    with patch.object(prior, 'prepare', prepare):
        return prior.run_scoped(build, mode)


def g5(build, mode):
    lab.verify_order()
    receipt, _, _ = lab.lab.prepare(build)
    fingerprints = prior.build_fingerprints(receipt)
    label = receipt['build']
    evidence = []
    for name in GATE_CASES:
        path = prior.case_path(label, name) / 'measurement.json'
        if not path.is_file():
            assert name in lab.PARTITIONS
            continue
        row = lab.lab.checked_receipt(path.parent, fingerprints)
        assert row['case'] == name
        if name in ('N1', 'N1Invariant', 'N6') or row['measurement'].get('gate_applies'):
            assert row['passed'], path
        evidence.append(path)
    invoke = full.run_case

    def run_case(work, mode, build_dir, build_receipt, config, sources, expected, runs, payloads):
        sources = [*sources, DRIVER, lab.DRIVER, lab.ORDER, lab.ORDER.with_suffix('.sha256'),
                   lab.SOURCE_CHECK, *evidence]
        return invoke(work, mode, build_dir, build_receipt, config, sources, expected, runs, payloads)

    with patch.object(full, 'run_case', run_case), patch.object(full.scope, 'verify_fixtures', prior.scoped_fixtures):
        full.run(build, mode)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('mode', choices=('fixtures', 'default', *full.MODES))
    args = parser.parse_args()
    build = args.build.resolve()
    if args.mode in ('fixtures', 'default'):
        scoped(build, args.mode)
    else:
        g5(build, args.mode)


if __name__ == '__main__':
    main()
