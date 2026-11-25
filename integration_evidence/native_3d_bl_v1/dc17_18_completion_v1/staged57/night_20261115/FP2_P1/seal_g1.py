"""Seal P1 G1 comparisons saved under the inherited helper's original filename."""
from argparse import ArgumentParser
from pathlib import Path

from run_p1 import G1, cut, run_fixture, verify_pins


def recover(build):
    for mode in G1:
        case = build.parent / 'fixtures' / build.name / mode
        original = case / 'case_summary.json'
        inherited = case / 'inherited_summary.json'
        if not inherited.is_file():
            summary = cut.read(original)
            assert summary['status'] == 'done', original
            assert summary['build'] == int(build.name.removeprefix('build')), original
            verify_pins(summary['pins'])
            (cohort,) = summary['cohorts'].values()
            assert cohort['status'] == 'done' and cohort['wait']['terminal'] and cut.terminal(cohort['wait']['controller'])
            assert cut.sha(cohort['used']) == cohort['comparison_sha256']
            original.rename(inherited)
        path, record = run_fixture(build, mode, None)
        print(mode, record['status'], path, flush=True)
        assert record['status'] == 'passed'


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    recover(parser.parse_args().build.resolve())
