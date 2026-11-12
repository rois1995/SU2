"""Supplementary tests of actual pinned CVariable, with only the selected method varied."""
import io
import json
import re
from pathlib import Path
import subprocess
import tarfile
from common import before_work, require, run, sha
from source import COMMIT, HEADER, REPO, ROOT, extract, git_bytes


def check_native(out):
    tree = out / 'native-source'
    tree.mkdir()
    data = subprocess.check_output(['git', '-C', str(REPO), 'archive', COMMIT,
                                   'Common', 'SU2_CFD/include', 'SU2_CFD/src/variables/CVariable.cpp'])
    with tarfile.open(fileobj=io.BytesIO(data)) as tar:
        tar.extractall(tree, filter='data')
    header = git_bytes(COMMIT, HEADER).decode()
    start, end, _ = extract(header)
    manifest = json.loads((ROOT / 'source-manifest.json').read_text())
    compiler = subprocess.check_output(['g++', '--version'], text=True).splitlines()[0]
    records = {}
    for name, identity in manifest['variants'].items():
        selected = (out / 'generated/fixtures' / (name + '.cpp')).read_text().removesuffix('\n')
        (tree / HEADER).write_text(header[:start] + selected + header[end:])
        require(sha(tree / HEADER) == identity['header_sha256'], 'Wrong source variant')
        binary = out / ('native-' + name)
        command = ['g++', '-std=c++17', '-O2', '-DNDEBUG', '-ffunction-sections', '-fdata-sections',
                   '-Wl,--gc-sections', '-I', tree, ROOT / 'tests/update.cpp',
                   tree / 'SU2_CFD/src/variables/CVariable.cpp', '-o', binary]
        preprocessed = out / ('preprocessed-' + name + '.cpp')
        before_work()
        run(['g++', '-std=c++17', '-O2', '-DNDEBUG', '-E', '-P', '-I', tree,
             ROOT / 'tests/update.cpp', '-o', preprocessed], ROOT,
            out / ('preprocess-' + name + '.log'), timeout=120)
        preprocessed_method = extract(preprocessed.read_text())[2]
        require(re.findall(r'\w+|[^\s]', preprocessed_method) == re.findall(r'\w+|[^\s]', selected),
                'Compiled method tokens differ from Lean input')
        (out / ('preprocessed-method-' + name + '.cpp')).write_text(preprocessed_method + '\n')
        before_work()
        run(command, ROOT, out / ('native-' + name + '-build.log'), timeout=120)
        before_work()
        expected = 0 if name in {'original', 'refactored'} else 1
        log = out / ('native-' + name + '.log')
        run([binary], ROOT, log, timeout=30, expected=expected)
        require(('20412 bitwise destination/frame checks passed' if expected == 0 else
                 'contract failed:') in log.read_text(), 'Native check failed for an unrelated reason')
        records[name] = {'expected_exit': expected, 'source_header_sha256': identity['header_sha256'],
                         'preprocessed_sha256': sha(preprocessed),
                         'preprocessed_method_sha256': sha(out / ('preprocessed-method-' + name + '.cpp')),
                         'binary_sha256': sha(binary), 'log_sha256': sha(log)}
    # Leave the disposable tree at the accepted source, never at a mutant.
    (tree / HEADER).write_text(header)
    return {'compiler': compiler, 'configuration': 'C++17 LP64 passive double, -O2 -DNDEBUG, serial',
            'scope': 'Actual inherited AddSolution and C2DContainer; fixtures allocate state through a derived class. No full solver or IEEE semantics proof.',
            'variants': records}
