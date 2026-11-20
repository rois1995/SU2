"""Run the Part S G5 ON pair against the sealed Resume 5 OFF proof."""

from argparse import ArgumentParser
from pathlib import Path
import runpy

HERE = Path(__file__).resolve().parent
FP1 = HERE.parent
PRIOR = FP1 / 'resume5'
RESUME5 = runpy.run_path(str(PRIOR / 'run_g5.py'))
workflow = RESUME5['workflow']
cut, sha = RESUME5['cut'], RESUME5['sha']
ORDER = FP1 / 'inputs/FP1_RESUME_6.md'
CHECKSUM = ORDER.with_suffix('.sha256')
ORDER_SHA = '490873ec2fe8da82406e6b541592a49ec610bb5243ed2876bbfda353d8449714'
BINDING = HERE / 'G5/build615_off_binding.json'
OLD_BUILD = PRIOR / 'build615'
OLD_OFF = PRIOR / 'G5/OFF'


def check_binding(build_dir, payloads, reference):
    old_receipt = cut.read(OLD_BUILD / 'build_status.json')
    new_receipt = cut.read(build_dir / 'build_status.json')
    assert old_receipt['build'] == 615 and new_receipt['build'] == 616
    assert all(receipt['passed'] and not receipt['new_warnings'] for receipt in (old_receipt, new_receipt))
    assert old_receipt['binary_sha256'] != new_receipt['binary_sha256']

    old_pins, new_pins = (build / 'source_pins.json' for build in (OLD_BUILD, build_dir))
    for pins, receipt in ((old_pins, old_receipt), (new_pins, new_receipt)):
        assert sha(pins) == receipt['source_pins_sha256']
    before, after = map(cut.read, (old_pins, new_pins))
    assert before.keys() == after.keys()
    changed = sorted(str(Path(path).relative_to(cut.REPO)) for path in before if before[path] != after[path])
    assert changed == [f'Common/src/adaptation/native_engine3d/{name}.cpp' for name in
                       ('EngineCensus', 'EngineControl')], changed

    proof = RESUME5['verified_off'](
        OLD_OFF.parent, 615, old_receipt['binary_sha256'], old_receipt['source_pins_sha256'], payloads, reference)
    assert [row['native_seconds'] for row in proof['events']] == [1778.627487361, 1029.081453324]
    return proof, {'passed': True, 'historical_off_build': 615, 'on_build': 616,
                   'off_build_binary_sha256': old_receipt['binary_sha256'],
                   'on_build_binary_sha256': new_receipt['binary_sha256'],
                   'off_source_pins_sha256': sha(old_pins), 'on_source_pins_sha256': sha(new_pins),
                   'off_proof_sha256': sha(OLD_OFF / RESUME5['COVERAGE']),
                   'off_run_receipt_sha256': sha(OLD_OFF / 'run_receipt.json'),
                   'off_case': proof['case'], 'changed_source_paths': changed,
                   'native_maxima': [row['native_seconds'] for row in proof['events']]}


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    assert sha(ORDER) == ORDER_SHA and CHECKSUM.read_text().split() == [ORDER_SHA, ORDER.name]
    build = args.build.resolve()
    scoped, safeguards = (HERE / f'scoped/{name}.json' for name in ('scoped', 'safeguards'))
    payloads = runpy.run_path(str(workflow.PAYLOADS))
    reference = workflow.off_reference(payloads)
    off, binding = check_binding(build, payloads, reference)
    env = RESUME5['bind_campaign']('on', [Path(__file__).resolve(), ORDER, CHECKSUM, scoped, safeguards, BINDING,
                                        OLD_OFF / 'run_receipt.json', OLD_OFF / RESUME5['COVERAGE'],
                                        OLD_BUILD / 'build_status.json', OLD_BUILD / 'source_pins.json'])

    def bound_off(output, on_build, binary_sha, source_sha, payload_tools, adopted):
        assert output == BINDING.parent and on_build == binding['on_build']
        assert binary_sha == binding['on_build_binary_sha256'] and source_sha == binding['on_source_pins_sha256']
        assert adopted == reference and payload_tools['payload_hashes'](Path(off['case'])) == reference
        assert cut.read(BINDING) == binding
        return off

    env['verified_off'] = bound_off
    if args.self_check:
        assert env['ONLY'] == 'on' and env['verified_off'] is bound_off
        assert set(workflow.timings(off['events'], off['events'])[0]) == {'event', 'native_seconds', 'limit_seconds', 'passed'}
        print('Resume 6 G5 no-launch check passed with the sealed build615 OFF binding.')
        return
    BINDING.parent.mkdir(exist_ok=True)
    workflow.sealed_write(BINDING, binding)
    env['campaign'](BINDING.parent, build, scoped, safeguards)


if __name__ == '__main__':
    main()
