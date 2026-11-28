"""Keep the archived diagnostic and authenticate its full-run artifacts at their original paths."""

from argparse import ArgumentParser
from pathlib import Path

from resume_equal_restart import ARCHIVE, E, cut, full, verify_build, workflow


def seal(label):
    verify_build(label)
    folder = full.root(label, 'equal')
    work = folder / 'trial1'
    archive = work / ARCHIVE
    path = folder / 'case_summary.json'
    backup = archive / 'case_summary_before_pin_resolution.json'
    record = cut.read(backup if backup.is_file() else path)
    assert record['part'] == 'equal' and record['passed'] and record['comparison']['equal']
    archived = cut.read(archive / 'failed_attempt.json')['artifacts']
    resolved, relocations = {}, {}
    for name, expected in record['pins'].items():
        file = Path(name)
        if not file.is_file():
            relative = file.relative_to(archive)
            assert archived[str(relative)] == expected
            actual = work / relative
            relocations[name] = str(actual)
            file = actual
        assert full.sha(file) == expected, file
        assert str(file) not in resolved or resolved[str(file)] == expected
        resolved[str(file)] = expected
    assert relocations
    if not backup.is_file():
        path.rename(backup)
    record['archive_pin_resolution'] = relocations
    record['pins'] = resolved | {
        str(file): full.sha(file) for file in (backup, Path(__file__).resolve())
    }
    workflow.sealed_write(path, record)
    E.check_pins(record['pins'])
    print(f'Checked all {len(record["pins"])} pins and resolved {len(relocations)} archived references.',
          flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int)
    seal(parser.parse_args().build)
