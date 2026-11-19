"""Build the isolated FP0 test driver and verify its frozen source and build graph."""

from argparse import ArgumentParser
from pathlib import Path
import re
import sys

from reuse_objects import ACTIVE, BUILD, NINJA, TEST_FILES
from source_view import ITEM, REPO, SOURCE, sha

sys.path.insert(0, str(ITEM.parent / 'N22'))
from gq_baseline import cut


def build(label):
    assert Path('/proc/1/comm').read_text().strip() == 'systemd'
    pins = cut.read(ITEM / 'n21_source_view.json')['verified']
    changed = {str(SOURCE / path): sha(REPO / path) for path in TEST_FILES}
    pins.update(changed)
    assert all(sha(Path(path)) == digest for path, digest in pins.items())
    source = ITEM / ('test_source_pins.json' if label == 592 else f'test_source_pins_{label}.json')
    cut.save(source, pins)
    tmp = REPO / 'integration_evidence/build-integrated-v2/compile-tmp'
    environment = ['env', 'OMP_NUM_THREADS=1', 'OPENBLAS_NUM_THREADS=1', 'MKL_NUM_THREADS=1',
                   f'NINJA={NINJA}', f'TMPDIR={tmp}']
    name = f'build{label}'
    wait = cut.run_gate(ITEM, name, [*environment, 'nice', '-n', '19', '/usr/bin/time', '-v', '-o',
                        str(ITEM / f'{name}.time'), 'timeout', '5400', str(NINJA), '-C', str(BUILD),
                        '-j', '4', 'UnitTests/test_driver'])
    binary = BUILD / 'UnitTests/test_driver'
    assert all(sha(Path(path)) == digest for path, digest in pins.items())
    log = ITEM / f'{name}_outer.log'
    warnings = [line for line in log.read_text(errors='replace').splitlines()
                if re.search(r'\bwarning:', line, re.IGNORECASE)]
    graph = {name: sha(BUILD / name) for name in ('build.ninja', 'compile_commands.json')}
    active = ACTIVE / 'SU2_CFD/src/SU2_CFD'
    cut.save(ITEM / f'{name}.json', {'passed': not warnings, 'state': 'terminal', 'build': label,
             'test_binary': str(binary), 'test_binary_sha256': sha(binary), 'warnings': warnings,
             'new_warnings': warnings, 'source_pins': str(source), 'source_pins_sha256': sha(source), 'graph': graph,
             'local_source': str(SOURCE), 'main_solver_sha256': sha(active), 'wait': wait})
    assert not warnings, warnings
    (ITEM / 'active_build.txt').write_text(f'{label}\n')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('label', type=int, default=592, nargs='?')
    build(parser.parse_args().label)
