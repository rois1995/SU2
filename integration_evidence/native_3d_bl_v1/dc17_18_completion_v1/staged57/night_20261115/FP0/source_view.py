"""Bind an isolated source tree to the N21 production state at the FP0 order."""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess


ITEM = Path(__file__).resolve().parent
REPO = ITEM.parents[5]
SOURCE = ITEM / 'local_source'
BASE = '4fe083d460dac20a39825d0e25a98188d3955e55'
PINS = ITEM.parent / 'N4/section6_3_after_N11/build589/source_pins.json'
TREES = ('Common', 'SU2_CFD', 'SU2_DEF', 'SU2_DOT', 'SU2_GEO', 'SU2_SOL', 'SU2_PY', 'UnitTests', 'meson_scripts')
FILES = ('meson.build', 'meson_options.txt', 'su2preconfig.timestamp', 'su2omp.syntax.json',
         'AUTHORS.md', 'LICENSE.md', 'AGENTS.md', '.clang-format', 'config_template.cfg')


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def prepare():
    SOURCE.mkdir(exist_ok=True)
    for part in TREES:
        shutil.copytree(REPO / part, SOURCE / part, symlinks=True, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('.git', '__pycache__', '*.orig'))
    for name in FILES:
        shutil.copy2(REPO / name, SOURCE / name)
    for name in ('externals', 'subprojects'):
        link = SOURCE / name
        if link.is_symlink():
            assert link.resolve(strict=True) == (REPO / name).resolve(strict=True)
        else:
            link.symlink_to(REPO / name, target_is_directory=True)
    expected = json.loads(PINS.read_text())
    checked, restored = {}, []
    for name, digest in expected.items():
        original = Path(name)
        relative = original.relative_to(REPO) if original.is_relative_to(REPO) else None
        if relative is None or relative.parts[0] in ('externals', 'subprojects'):
            target = original
        elif relative.parts[0] not in (*TREES, *FILES):
            continue
        else:
            target = SOURCE / relative
            assert target.is_file(), target
            if sha(target) != digest:
                assert relative.parts[0] in TREES and not target.is_symlink(), relative
                content = subprocess.check_output(['git', 'show', f'{BASE}:{relative}'], cwd=REPO)
                assert hashlib.sha256(content).hexdigest() == digest, relative
                target.write_bytes(content)
                restored.append(str(relative))
        assert sha(target) == digest, (original, target)
        checked[str(target)] = digest
    receipt = {'base': BASE, 'accepted_source_pins_sha256': sha(PINS), 'restored': restored, 'verified': checked}
    (ITEM / 'n21_source_view.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    prepare()
