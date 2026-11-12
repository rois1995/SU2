#!/usr/bin/env python3
"""Clean serial production unit build at the frozen source commit, then parent negative control."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from common import (ARCHIVE, COMMIT, PARENT, WALL, before_work, git, limits, payload_manifest,
                    require, restore_repo, run, sha, source_provenance, unpack)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('payload', type=Path)
    p.add_argument('output', type=Path, help='Must not exist; all build and fixture edits stay here')
    args = p.parse_args()
    limits()
    payload, out = args.payload.resolve(), args.output.resolve()
    manifest = payload_manifest(payload)
    out.mkdir(parents=True, exist_ok=False)
    log = out / 'build.log'
    records = {r['name']: r for r in manifest['repositories']}
    repo = out / 'SU2'
    restore_repo(payload, records['SU2'], repo, out / 'repositories')
    unpack(payload / ARCHIVE, out)
    project = out / 'SU2_Lean'
    source_provenance(repo, project)
    for name in ['meson', 'eigen', 'ninja', 'mel']:
        restore_repo(payload, records[name], repo / 'externals' / name, out / 'repositories')
    # This is a standalone repository with private Git metadata, never a shared worktree.
    require((repo / '.git').is_dir(), 'Private Git directory required')
    before_work()
    run(['cmake', '-S', repo / 'externals/ninja', '-B', out / 'ninja-build',
         '-DBUILD_TESTING=OFF', '-DCMAKE_BUILD_TYPE=Release'], out, log)
    before_work()
    run(['cmake', '--build', out / 'ninja-build', '--parallel', '2'], out, log)
    ninja = out / 'ninja-build/ninja'
    os.environ['NINJA'] = str(ninja)
    before_work()
    run([sys.executable, 'preconfigure.py', '--no-codi', '--no-medi', '--no-opdi', '--no-mpp',
         '--no-coolprop', '--no-mel', '--no-fado', '--no-mlpcpp', '--no-eigen'], repo, log)
    before_work()
    run([sys.executable, 'externals/meson/meson.py', 'setup', 'build', '--wrap-mode=nodownload',
         '-Denable-tests=true', '-Dwith-mpi=disabled', '-Dwith-omp=false',
         '-Denable-cgns=false', '-Denable-tecio=false', '-Dcpu-arch='], repo, log)
    before_work()
    run([ninja, '-C', 'build', '-j2', 'UnitTests/test_driver'], repo, log)
    binary = repo / 'build/UnitTests/test_driver'
    result = {'status': 'local supplementary production C++ test; not formal certification',
              'source_commit': COMMIT, 'parent': PARENT,
              'fixed_binary_sha256': sha(binary), 'compiler': subprocess.check_output(['c++', '--version'], text=True),
              'test_filter': '[HLLC-wall]', 'configuration': 'serial, double, release; MPI/OpenMP/AD/CGNS/TecIO disabled',
              'fast_math': 'SU2 default -ffast-math -fno-finite-math-only retained'}
    shutil.copyfile(repo / 'build/compile_commands.json', out / 'compile_commands.json')
    shutil.copyfile(repo / 'build/meson-info/intro-buildoptions.json', out / 'build-options.json')
    before_work()
    run([binary, '[HLLC-wall]'], repo, out / 'fixed-test.log', timeout=300)
    require('All tests passed' in (out / 'fixed-test.log').read_text(), 'Missing successful Catch2 test summary')
    require(git(repo, 'diff', '--name-only', '--ignore-submodules=all') == '', 'Production tree changed')
    # The parent differs only in this production file and the two test registration files.
    changed = set(git(repo, 'diff', '--name-only', PARENT, COMMIT).splitlines())
    require(changed == {WALL, 'UnitTests/meson.build', 'UnitTests/SU2_CFD/solvers/moving_wall_tests.cpp'},
            'Parent negative-control recipe needs review')
    fixed = (repo / WALL).read_bytes()
    try:
        (repo / WALL).write_bytes(subprocess.check_output(['git', '-C', str(repo), 'show', PARENT + ':' + WALL]))
        require(git(repo, 'diff', '--name-only', PARENT, '--ignore-submodules=all') ==
                'UnitTests/SU2_CFD/solvers/moving_wall_tests.cpp\nUnitTests/meson.build',
                'Negative control must have exactly parent production code and repaired test fixture')
        before_work()
        run([ninja, '-C', 'build', '-j2', 'UnitTests/test_driver'], repo, out / 'parent-build.log')
        result['parent_binary_sha256'] = sha(binary)
        before_work()
        run([binary, '[HLLC-wall]', '--abort'], repo, out / 'parent-test.log', timeout=300, expected=1)
        require('failed' in (out / 'parent-test.log').read_text(), 'Missing Catch2 failure summary')
    finally:
        (repo / WALL).write_bytes(fixed)
    require(git(repo, 'diff', '--name-only', '--ignore-submodules=all') == '', 'Source restoration failed')
    result['fixed_test'] = 'pass'
    result['parent_with_identical_test_fixture'] = 'fail (expected)'
    result['retained_binary'] = 'parent negative-control binary; source restored to fixed commit'
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2), flush=True)


if __name__ == '__main__':
    main()
