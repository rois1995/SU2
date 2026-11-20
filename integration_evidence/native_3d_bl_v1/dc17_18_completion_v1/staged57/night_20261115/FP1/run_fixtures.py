"""Run the scoped FP1 fixture set with the sealed binary and host receipts."""

from argparse import ArgumentParser
from pathlib import Path
import re

from run_g1 import ITEM, cut, run_case, sha, verify_build

FIXTURES = {
    'A': ((1, '[NativeLocalCore3D]'),),
    'B': ((2, '[NativeLocalCore3DShift]'), (4, '[NativeLocalCore3DShift]')),
    'C': ((1, '[NativeLocalCore3DOption]'), (2, '[NativeLocalCore3DOption]'),
          (4, '[NativeLocalCore3DOption]')),
    'FA': ((1, '[NativeLocalCore3DResumeA]'),),
    'FR': ((1, '[NativeLocalCore3DResumeRing]'),),
    'FB': tuple((ranks, '[NativeLocalCore3DResumeShift]') for ranks in (2, 4)),
    'FC': tuple((ranks, '[NativeLocalCore3DResumeOption]') for ranks in (1, 2, 4)),
}
ANSI = re.compile(r"\x1b\[[0-9;]*m")
PASS = re.compile(r'All tests passed \((\d+) assertions? in (\d+) test cases?\)')


def fixtures(part, build, root):
    receipt = cut.read(build / 'build_status.json')
    pins = build / 'source_pins.json'
    binary, manifest = verify_build(receipt, pins)
    results = []
    for ranks, selector in FIXTURES[part]:
        case = root / f'rank{ranks}'
        wait = run_case(case, binary, ranks, selector, 5400)
        if ranks == 1:
            logs = [case / 'test_outer.log']
        else:
            logs = sorted(case.glob('rank_output/*/rank.*/stdout'))
        summaries = [PASS.findall(ANSI.sub('', log.read_text(errors='replace'))) for log in logs]
        passed = (wait['returncode'] == 0 and len(logs) == ranks
                  and all(len(summary) == 1 for summary in summaries))
        data = {'passed': passed, 'ranks': ranks, 'selector': selector,
                'test_binary_sha256': receipt['test_binary_sha256'], 'source_pins_sha256': manifest,
                'logs': {str(log): sha(log) for log in logs}, 'counts': summaries, 'wait': wait}
        cut.save(case / 'fixture_status.json', data)
        results.append(data)
        if not passed:
            raise RuntimeError(f'Part {part} fixture failed at {ranks} rank(s). See {case}.')
        print(f'FP1 {part} fixtures at {ranks} rank(s): {summaries}', flush=True)
    verify_build(receipt, pins)
    cut.save(root / 'fixtures.json', {'passed': True, 'part': part, 'receipts': results})


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('part', choices=FIXTURES)
    parser.add_argument('build', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    fixtures(args.part, args.build.resolve(), (args.output or ITEM / args.part / 'fixtures').resolve())
