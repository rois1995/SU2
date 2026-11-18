"""Compile either N6 engine draft against the accepted flags in a private overlay."""
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

drafts = Path(__file__).resolve().parent
root = Path.cwd()
source = Path('Common/src/adaptation')
unit = source / 'CNativeEngine3D.cpp'
build = root / 'integration_evidence/build-integrated-v3-p5-20261114'


def compiler_flags():
    commands = json.loads((build / 'compile_commands.json').read_text())
    entry, = (item for item in commands if item['file'].endswith('/' + str(unit)))
    assert Path(entry['directory']) == build
    arguments = iter(shlex.split(entry['command']))
    flags = []
    for argument in arguments:
        if argument in ('-MQ', '-MF', '-o'):
            next(arguments)
        elif argument not in ('-c', '-MD', entry['file']):
            flags.append(argument)
    return flags


def prepare(overlay, selection):
    target = overlay / source
    target.mkdir(parents=True)
    (overlay / 'Common/include').symlink_to(root / 'Common/include', target_is_directory=True)
    for child in (root / source).iterdir():
        if child.name not in (unit.name, 'native_engine3d'):
            (target / child.name).symlink_to(child)
    private = target / 'native_engine3d'
    shutil.copytree(root / source / private.name, private)
    shutil.copyfile(drafts / 'optimizer' / unit.name, target / unit.name)
    shutil.copyfile(drafts / 'optimizer/MeshOptimizer.cpp', private / 'MeshOptimizer.cpp')
    if selection:
        patch = drafts / 'selection/selection.patch'
        subprocess.run(['git', 'apply', '--check', '--whitespace=error', str(patch)], cwd=overlay, check=True)
        subprocess.run(['git', 'apply', '--whitespace=error', str(patch)], cwd=overlay, check=True)


if __name__ == '__main__':
    assert len(sys.argv) == 2 and sys.argv[1] in ('optimizer', 'selection')
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1')
    env['TMPDIR'] = str(root / 'integration_evidence/build-integrated-v2/compile-tmp')
    with tempfile.TemporaryDirectory(prefix='su2_n6_engine_syntax_') as directory:
        overlay = Path(directory)
        prepare(overlay, sys.argv[1] == 'selection')
        code = subprocess.call(['timeout', '-k', '2s', '37s', 'nice', '-n', '19',
                                *compiler_flags(), '-fsyntax-only', str(overlay / unit)], cwd=build, env=env)
    raise SystemExit(code)
