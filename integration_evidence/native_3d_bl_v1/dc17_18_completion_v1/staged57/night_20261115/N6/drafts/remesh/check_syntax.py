"""Check this private source overlay with the accepted Ninja compile flags."""
from pathlib import Path
import json
import os
import shlex
import shutil
import subprocess
import tempfile

draft = Path(__file__).resolve().parent
root = Path.cwd()
source = Path('Common/src/adaptation')
files = root / source
build = root / 'integration_evidence/build-integrated-v3-p5-20261114'
commands = json.loads((build / 'compile_commands.json').read_text())
entry, = (item for item in commands if item['file'].endswith('/adaptation/CNativeRemesher3D.cpp'))
assert Path(entry['directory']) == build
args = iter(shlex.split(entry['command']))
compiler = []
for arg in args:
    if arg in ('-MQ','-MF','-o'):
        next(args)
    elif arg not in ('-c','-MD',entry['file']):
        compiler.append(arg)

with tempfile.TemporaryDirectory(prefix='su2_n6_remesh_syntax_') as directory:
    overlay = Path(directory)
    target = overlay / source
    target.mkdir(parents=True)
    (overlay/'Common/include').symlink_to(root/'Common/include',target_is_directory=True)
    private = target / 'native_engine3d'
    private.mkdir()
    for child in files.iterdir():
        if child.name not in ('CNativeRemesher3D.cpp','native_engine3d'):
            (target/child.name).symlink_to(child)
    for child in (files/'native_engine3d').iterdir():
        if child.suffix in ('.hpp','.cpp'):
            (private/child.name).symlink_to(child)
    for name in ('NativeRemeshRun.inc','NativeRemeshReporting.inc'):
        shutil.copyfile(draft/name,private/name)
    shutil.copyfile(draft/'CNativeRemesher3D.cpp',target/'CNativeRemesher3D.cpp')
    env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
    env['TMPDIR']=str(root/'integration_evidence/build-integrated-v2/compile-tmp')
    result=subprocess.run(['timeout','-k','2s','37s','nice','-n','19',*compiler,'-fsyntax-only',
                           str(target/'CNativeRemesher3D.cpp')],cwd=build,env=env,check=False)
    raise SystemExit(result.returncode)
