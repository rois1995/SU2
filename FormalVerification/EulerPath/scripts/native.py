#!/usr/bin/env python3
"""Native controls are supplementary evidence, never a C++ semantics proof."""
import argparse
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile

from source import ROOT, REPO, COMMIT, HEADER, VECTOR, accepted, data, generate
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import before_work, limits, require, run, sha


def check_native(out):
    out.mkdir(parents=True, exist_ok=False)
    generate(out / 'generated')
    manifest = json.loads((out / 'generated/source-manifest.json').read_text())
    tree = out / 'source'
    raw = subprocess.check_output(['git', '-C', str(REPO), 'archive', COMMIT,
                                  'Common', 'SU2_CFD/include', 'SU2_CFD/src/variables/CVariable.cpp'])
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        archive.extractall(tree, filter='data')
    fixtures = out / 'generated/fixtures'
    flags = ['g++', '-std=c++17', '-O2', '-DNDEBUG', '-ffunction-sections', '-fdata-sections',
             '-Wl,--gc-sections', '-I', tree, '-I', out]
    results = {}
    for operation, variants in [('update', ['original', 'refactored', 'subtract', 'currentRead', 'wrongTarget']),
                                ('composed', ['original', 'refactored', 'wrongSign', 'wrongMask', 'subtractTrunc', 'wrongCall'])]:
        for variant in variants:
            name = operation + '-' + variant
            (tree / HEADER).write_bytes(data(HEADER))
            (tree / VECTOR).write_bytes(data(VECTOR))
            fragment = 'update' if operation == 'update' else 'scatter'
            selected_variant = variant if operation == 'update' or variant in {'original', 'refactored', 'wrongSign', 'wrongMask'} else 'original'
            selection = manifest['selections'][fragment]
            path = selection['path']
            original = data(path)
            start, end = selection['span_bytes']
            selected = (fixtures / (fragment + '_' + selected_variant + '.cpp')).read_text().removesuffix('\n')
            (tree / path).write_bytes(original[:start] + selected.encode() + original[end:])
            require(sha(tree / path) == selection['variants'][selected_variant]['full_file_sha256'], 'Wrong variant header')
            sources = [tree / 'SU2_CFD/src/variables/CVariable.cpp']
            if operation == 'update':
                test = ROOT.parent / 'SourceGate/tests/update.cpp'
                success = '20412 bitwise destination/frame checks passed'
                failure = 'contract failed:'
            else:
                test = ROOT / 'tests/composed.cpp'
                sources += [tree / 'Common/src/linear_algebra/CSysVector.cpp',
                            tree / 'Common/include/parallelization/mpi_structure.cpp',
                            tree / 'Common/include/parallelization/omp_structure.cpp']
                for part in ['apply', 'scale', 'call']:
                    choice = 'subtractTrunc' if part == 'apply' and variant == 'subtractTrunc' else (
                        'wrongSign' if part == 'call' and variant == 'wrongCall' else 'original')
                    (out / ('selected-' + part + '.inc')).write_bytes((fixtures / (part + '_' + choice + '.cpp')).read_bytes())
                success = '3312 composed destination/residual/old-state checks passed'
                failure = 'composed contract failed:'
            preprocessed = out / (name + '.ii')
            before_work()
            run(flags[:5] + ['-E', '-P', '-I', tree, '-I', out, test, '-o', preprocessed], ROOT,
                out / (name + '-preprocess.log'), timeout=120)
            # The migrated scalar method is checked against the compiled preprocessing result.
            if operation == 'update':
                compiled = accepted.extract(preprocessed.read_text())[2]
                require(re.findall(r'\w+|[^\s]', compiled) == re.findall(r'\w+|[^\s]', selected),
                        'Compiled scalar tokens differ from Lean input')
                (out / (name + '-preprocessed-method.cpp')).write_text(compiled + '\n')
            binary = out / name
            before_work()
            run(flags + [test] + sources + ['-o', binary], ROOT, out / (name + '-build.log'), timeout=180)
            expected = 0 if variant in {'original', 'refactored'} else 1
            before_work()
            log = out / (name + '.log')
            run([binary], ROOT, log, timeout=30, expected=expected)
            require((success if expected == 0 else failure) in log.read_text(), 'Unrelated native failure')
            results[name] = {'expected_exit': expected, 'binary_sha256': sha(binary),
                             'preprocessed_sha256': sha(preprocessed), 'log_sha256': sha(log)}
    (tree / HEADER).write_bytes(data(HEADER))
    (tree / VECTOR).write_bytes(data(VECTOR))
    record = {'status': 'PASS supplementary native controls', 'source_commit': COMMIT,
              'compiler': subprocess.check_output(['g++', '--version'], text=True).splitlines()[0],
              'configuration': 'C++17 LP64 passive double, serial, -O2 -DNDEBUG; no fast-math in source-slice controls',
              'variants': results,
              'limitation': 'Extracted caller fragments run in a fixture; full execution dispatch is separately inspected and integration-tested. No verified compiler or IEEE arithmetic.'}
    (out / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
    print('All eleven native source variants produced their expected semantic result.', flush=True)
    return record


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    limits()
    check_native(args.output.resolve())
