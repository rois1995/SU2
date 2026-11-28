"""Execute the TOL1 scoped standing checks with the authenticated DEF1 adapters and existing exact comparators."""

from argparse import ArgumentParser
from pathlib import Path
import sys

from build_fp3 import DEF1, HERE, ORDER, ORDER_SHA, SOURCE_CHECK, baseline, cut, from_base, verify_order

sys.path.insert(1, str(DEF1))
import legacy_config  # noqa: E402

SELF = Path(__file__).resolve()
ADAPTER = DEF1 / 'run_controls.py'
EXTERNAL_CONFIGS = ('G2', 'GQ', 'G3', 'first-collapse', 'exact-c492', 'saved-current')


def gate_reference(name):
    """Keep the original derivation and verify its bytes against the accepted DEF1 launch."""
    reference, prior_receipts, expected = legacy_config.gate_reference(name)
    entry = from_base(DEF1 / 'controls' / 'build681' / name / 'case_summary.json')
    summary = cut.read(entry)
    assert summary['status'] == 'passed' and summary['build'] == 681, entry
    row = summary
    if 'subruns' in row:
        row, = row['subruns'].values()
    config, metadata, recorded = legacy_config.metadata_config(Path(row['case']))
    assert recorded == row['legacy_config']['derived_sha256'] == summary['pins'][str(config)]
    assert cut.sha(metadata) == summary['pins'][str(metadata)], metadata
    assert legacy_config.derived(reference.read_bytes())[0] == config.read_bytes(), config
    return reference, [*prior_receipts, entry, metadata, config], expected


def support(build):
    verify_order()
    return (SELF, ORDER, ORDER.with_suffix('.sha256'), SOURCE_CHECK,
            scoped['ACCEPTED'], scoped['P3_SCOPED'], ADAPTER, DEF1 / 'legacy_config.py',
            HERE / 'build_fp3.py', build / 'build_status.json', build / 'source_pins.json')


def replace(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def load_controls():
    source = from_base(ADAPTER).read_text()
    source = replace(source, "CASE='DEF1_G3'", "CASE='TOL1_G3'")
    source = replace(source, "HERE/'build681'", "HERE/'build683'")
    source = replace(source, "            verify_pins(existing['pins'])\n            continue",
                     "            verify_pins(existing['pins'])\n"
                     "            if existing['status'] != 'passed':\n"
                     "                return existing['status']\n"
                     "            continue")
    source = replace(source, "            if not finalize_measurement:\n                continue\n",
                     "            if not finalize_measurement:\n"
                     "                raise RuntimeError(f'Unsealed TOL1 {name} measurement at {log}.') from error\n")
    source = replace(source, "        with (HERE.parent/'progress.md').open('a') as out:\n"
                     '            out.write(f"\\nDEF1 build{label} Step 1 {name} {row[\'status\']}, receipt {summary}.\\n")\n', '')
    source = replace(source, '        print(f"DEF1 {name}: {row[\'status\']} at {summary}", flush=True)\n',
                     '        print(f"TOL1 {name}: {row[\'status\']} at {summary}", flush=True)\n'
                     "        if row['status'] != 'passed':\n"
                     "            return row['status']\n")
    source = source.replace('DEF1 ', 'TOL1 ')
    scope = {'__name__': 'tol1_def1_controls', '__file__': str(ADAPTER)}
    exec(compile(source, str(ADAPTER), 'exec'), scope)
    scope.update(SELF=SELF, ORDER=ORDER, ORDER_SHA=ORDER_SHA, SOURCE_CHECK=SOURCE_CHECK,
                 verify_order=verify_order, support=support, gate_reference=gate_reference)
    scope['CHECKS'] = tuple(name for name in scope['CHECKS'] if name != 'hidden')
    return scope


scoped = load_controls()


def self_check():
    assert Path(legacy_config.__file__).resolve() == from_base(DEF1 / 'legacy_config.py')
    baseline()
    for gate in EXTERNAL_CONFIGS:
        gate_reference(gate)
    assert scoped['CHECKS'] == ('G1-serial', 'G1-mpi2', 'G1-mpi4', 'G2', 'GQ', 'first-collapse',
                               'exact-c492', 'merit481', 'native2d', 'saved-current', 'G3', 'G4')
    scoped['self_check']()


if __name__ == '__main__':
    if sys.argv[1:2] == ['--worker']:
        raise SystemExit(scoped['worker'](sys.argv[2], sys.argv[3:]))
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('control', choices=scoped['CHECKS'], nargs='?')
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--collect-only', action='store_true')
    parser.add_argument('--finalize-measurement-failure', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    else:
        assert args.build is not None
        assert not (args.collect_only and args.control)
        names = () if args.collect_only else (args.control,) if args.control else scoped['CHECKS']
        for name in names:
            if name in EXTERNAL_CONFIGS:
                gate_reference(name)
        status = scoped['run'](args.build, names, args.finalize_measurement_failure)
        raise SystemExit(scoped['EXIT'][status])
