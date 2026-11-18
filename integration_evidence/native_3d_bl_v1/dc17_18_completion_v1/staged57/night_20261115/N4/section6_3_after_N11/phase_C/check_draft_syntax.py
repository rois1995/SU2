"""Check the drafted phase-C source chain in a temporary overlay with Ninja's compile flags."""
from pathlib import Path
import hashlib
import json
import os
import shlex
import subprocess
import tempfile

phase = Path(__file__).resolve().parents[1]
repo = phase.parents[6]
relative = Path('Common/src/adaptation')
tree = repo / relative
build = repo / 'integration_evidence/build-integrated-v3-p5-20261114'
commands = json.loads((build / 'compile_commands.json').read_text())
entry, = (item for item in commands if item['file'].endswith('/adaptation/CNativeEngine3D.cpp'))
assert Path(entry['directory']) == build
args = iter(shlex.split(entry['command']))
compiler = []
for arg in args:
    if arg in ('-MQ', '-MF', '-o'):
        next(args)
    elif arg not in ('-c', '-MD', entry['file']):
        compiler.append(arg)
original = dict()
with tempfile.TemporaryDirectory(prefix='su2_phase_c_compile_') as temp:
    overlay = Path(temp)
    destination = overlay / relative
    destination.mkdir(parents=True)
    # The overlay supplies only the changed sources. Other headers use their existing paths.
    (overlay / 'Common/include').symlink_to(repo / 'Common/include', target_is_directory=True)
    for child in tree.iterdir():
        if child.name != 'native_engine3d' and child.name != 'CNativeEngine3D.cpp':
            (destination / child.name).symlink_to(child)
    private = destination / 'native_engine3d'
    private.mkdir()
    committed_sources = tree / 'native_engine3d'
    for file in committed_sources.iterdir():
        if file.suffix in ('.cpp', '.hpp'):
            (private / file.name).write_bytes(file.read_bytes())
    (destination / 'CNativeEngine3D.cpp').write_bytes((phase / 'C4_preparation/draft/CNativeEngine3D.cpp').read_bytes())
    (private / 'EngineCommit.cpp').write_bytes((phase / 'C4_preparation/draft/EngineCommit.cpp').read_bytes())
    for step in range(5, 12):
        patch = phase / f'C{step}_preparation/proposed.diff'
        subprocess.run(['git', 'apply', str(patch)], cwd=overlay, check=True)
        original[patch.name + f'.C{step}'] = hashlib.sha256(patch.read_bytes()).hexdigest()
    env = os.environ.copy()
    env.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1')
    env['TMPDIR'] = str(repo / 'integration_evidence/build-integrated-v2/compile-tmp')
    command = ['nice', '-n', '19', *compiler, '-fsyntax-only', str(destination / 'CNativeEngine3D.cpp')]
    status = subprocess.run(command, cwd=build, env=env, check=False).returncode
    print(json.dumps({'draft_pins': original, 'compile_argv': command, 'exit_code': status,
                      'binary_output_written': False}, indent=2), flush=True)
    raise SystemExit(status)
