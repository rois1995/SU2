#!/usr/bin/env python3
"""Build the pinned upstream baseline in a standalone checkout with private Git metadata."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import before_work, git, limits, require, run, sha

UPSTREAM = 'd162016382f0c122433000c114e88d808b48f7a7'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source_repo', type=Path)
    p.add_argument('externals', type=Path, help='Read-only Git sources for meson, ninja, eigen and mel')
    p.add_argument('output', type=Path, help='New directory, never a shared Git worktree')
    args = p.parse_args()
    limits()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    repo = out / 'SU2'
    log = out / 'build.log'
    run(['git', 'clone', '--quiet', '--shared', '--no-checkout', args.source_repo.resolve(), repo], out, log)
    run(['git', 'checkout', '--quiet', '--detach', UPSTREAM], repo, log)
    require((repo / '.git').is_dir(), 'Standalone metadata required')
    require(git(repo, 'rev-parse', 'HEAD') == UPSTREAM, 'Wrong source baseline')
    dependencies = {}
    for name in ['meson', 'ninja', 'eigen', 'mel']:
        revision = git(repo, 'rev-parse', UPSTREAM + ':externals/' + name)
        source = args.externals.resolve() / name
        require(git(source, 'rev-parse', revision + '^{commit}') == revision, 'Missing pinned dependency')
        target = repo / 'externals' / name
        target.rmdir()
        run(['git', 'clone', '--quiet', '--shared', '--no-checkout', source, target], out, log)
        run(['git', 'checkout', '--quiet', '--detach', revision], target, log)
        dependencies[name] = revision
    before_work()
    run(['cmake', '-S', repo / 'externals/ninja', '-B', out / 'ninja-build',
         '-DBUILD_TESTING=OFF', '-DCMAKE_BUILD_TYPE=Release'], out, log)
    before_work()
    run(['cmake', '--build', out / 'ninja-build', '--parallel', '2'], out, log)
    ninja = out / 'ninja-build/ninja'
    os.environ['NINJA'] = str(ninja)
    (repo / 'ninja').symlink_to(ninja)
    # All required dependencies are installed; invoke official setup without acquisition.
    before_work()
    run([sys.executable, '-c', 'import os, sys; sys.path[0] = os.getcwd(); import inspect, preconfigure; preconfigure.run(**{name: False for name in inspect.signature(preconfigure.run).parameters})'], repo, log)
    before_work()
    run([sys.executable, 'externals/meson/meson.py', 'setup', 'build', '--wrap-mode=nodownload',
         '-Denable-tests=true', '-Dwith-mpi=disabled', '-Dwith-omp=false', '-Denable-autodiff=false',
         '-Denable-directdiff=false', '-Denable-cgns=false', '-Denable-tecio=false', '-Dcpu-arch='], repo, log)
    before_work()
    run([ninja, '-C', 'build', '-j2', 'UnitTests/test_driver', 'SU2_CFD/src/SU2_CFD'], repo, log)
    before_work()
    run([repo / 'build/UnitTests/test_driver', '[Vectorization],[LinearAlgebra]'], repo,
        out / 'unit-tests.log', timeout=300)
    require('All tests passed' in (out / 'unit-tests.log').read_text(), 'Missing test success summary')
    require(not git(repo, 'diff', '--name-only', '--ignore-submodules=all'), 'Production source changed')
    for name in ['compile_commands.json', 'meson-info/intro-buildoptions.json']:
        shutil.copyfile(repo / 'build' / name, out / Path(name).name)
    result = {'status': 'PASS build and selected upstream unit tests; integration and proofs recorded separately',
              'source_commit': UPSTREAM, 'source_tree': git(repo, 'rev-parse', 'HEAD^{tree}'),
              'production_patches': [], 'dependencies': dependencies,
              'compiler': subprocess.check_output(['g++', '--version'], text=True).splitlines()[0],
              'configuration': 'serial passive double, MPI/OpenMP/AD/CGNS/TecIO/CUDA disabled; default release fast-math flags retained',
              'binary_sha256': sha(repo / 'build/SU2_CFD/src/SU2_CFD'),
              'unit_binary_sha256': sha(repo / 'build/UnitTests/test_driver')}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Upstream build and selected unit tests passed.', flush=True)


if __name__ == '__main__':
    main()
