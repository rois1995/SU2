"""Reuse only exact-command Ninja objects with unchanged source dependencies."""

import json
from pathlib import Path
import shutil
import subprocess

from source_view import ITEM, REPO, SOURCE, sha


BUILD = SOURCE / 'integration_evidence/build_fp0'
ACTIVE = REPO / 'integration_evidence/build-integrated-v3-p5-20261114'
NINJA = REPO / 'integration_evidence/build-integrated-v2/tools/ninja'
TEST_FILES = tuple(Path(name) for name in (
    'UnitTests/meson.build',
    'UnitTests/SU2_CFD/adaptation/CNativeFastPathStage0_tests.cpp',
    'UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
    'UnitTests/SU2_CFD/adaptation/fixtures/native_fast_path_edits.inc',
    'UnitTests/SU2_CFD/adaptation/fixtures/native_fast_path_proofs.inc',
    'UnitTests/SU2_CFD/adaptation/fixtures/native_fp0_exit_refine.inc',
    'UnitTests/SU2_CFD/adaptation/fixtures/native_transaction_capture.inc',
))


def commands(build):
    return {row['output']: row for row in json.loads((build / 'compile_commands.json').read_text())
            if 'output' in row}


def dependencies():
    lines = subprocess.check_output([NINJA, '-C', ACTIVE, '-t', 'deps'], text=True).splitlines()
    result = {}
    for line in lines:
        if ': #deps ' in line:
            target = line.split(': #deps ', 1)[0]
            result[target] = []
        elif line.startswith('    '):
            result[target].append(line.strip())
    return result


def reuse():
    receipt = json.loads((ITEM / 'n21_source_view.json').read_text())
    changed = {(REPO / path).resolve() for path in [*receipt['restored'], *TEST_FILES]}
    old, current, deps = commands(ACTIVE), commands(BUILD), dependencies()
    reused, skipped = [], []
    for output, row in current.items():
        previous = old.get(output)
        source = ACTIVE / output
        if not previous or output not in deps or not source.is_file():
            continue
        paths = [previous['file'], *deps[output]]
        if previous['command'] != row['command'] or any((ACTIVE / path).resolve() in changed for path in paths):
            skipped.append(output)
            continue
        target = BUILD / output
        shutil.copy2(source, target)
        reused.append(output)
    for name in ('.ninja_deps', '.ninja_log'):
        shutil.copy2(ACTIVE / name, BUILD / name)
    info = {'compiler_commands_identical': len(set(old) & set(current)) - sum(
                old[name]['command'] != current[name]['command'] for name in set(old) & set(current)),
            'active_compile_commands_sha256': sha(ACTIVE / 'compile_commands.json'),
            'isolated_compile_commands_sha256': sha(BUILD / 'compile_commands.json'),
            'changed_sources': sorted(map(str, changed)), 'reused_objects': reused, 'withheld_objects': skipped}
    (ITEM / 'object_reuse.json').write_text(json.dumps(info, indent=2) + '\n')
    print(f'Reused {len(reused)} objects. Withheld {len(skipped)} objects with changed inputs or commands.')


if __name__ == '__main__':
    reuse()
