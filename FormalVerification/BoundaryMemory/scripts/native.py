#!/usr/bin/env python3
"""Actual pinned methods plus compiled fragment controls; supplementary evidence only."""
import argparse
import json
from pathlib import Path
import shlex
import shutil
import sys

from source import ROOT, generate
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import before_work, limits, require, run, sha


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('baseline', type=Path)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    limits()
    baseline = args.baseline.resolve()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    source = baseline / 'SU2'
    build = source / 'build'
    commands = json.loads((build / 'compile_commands.json').read_text())
    command, = [c for c in commands if c['file'].endswith('edge_residual_blocks_tests.cpp')]
    flags = shlex.split(command['command'])
    flags = flags[:flags.index('-MD')] + ['-I', str(source)]
    obj = out / 'memory_native.o'
    binary = out / 'memory_native'
    before_work()
    run(flags + ['-c', ROOT / 'tests/boundary_memory_native.cpp', '-o', obj], build, out / 'actual-build.log', timeout=300)
    before_work()
    run(['c++', obj, '-Wl,--start-group', build / 'SU2_CFD/src/libSU2core.a',
         build / 'Common/src/libSU2Common.a', '-Wl,--end-group', '-o', binary], build, out / 'actual-build.log', timeout=180)
    shutil.copytree(ROOT.parent / 'EulerPath/tests/integration', out / 'case')
    before_work()
    run([binary, 'euler.cfg'], out / 'case', out / 'actual.log', timeout=90)
    require('far_passes=2 primitive_stride=11 cache_stride=11' in (out / 'actual.log').read_text(), 'Missing full-memory coverage summary')
    generate(out / 'generated')
    variants = {'original': {}, 'refactored': {k: 'refactored' for k in ['wallOld', 'wallRes', 'wallTrunc']},
        'discard': {}, 'wrongBuffer': {'oldBinding': 'wrongBuffer'},
        'refresh': {k: 'refresh' for k in ['wallOld', 'wallRes', 'wallTrunc']},
        'shortStride': {'primitiveExtra': 'shortStride'}, 'firstMarker': {'cacheLookup': 'firstMarker'},
        'firstVertex': {'cacheLookup': 'firstVertex'},
        'eraseTail': {'cacheWrites': 'eraseTail'}, 'deduplicate': {}}
    results = {}
    for name, choices in variants.items():
        selected = out / name
        selected.mkdir()
        for part in ['oldBinding', 'wallOld', 'wallRes', 'wallTrunc', 'primitiveExtra', 'cacheWrites', 'cacheLookup']:
            shutil.copyfile(out / 'generated/fixtures' / (part + '_' + choices.get(part, 'original') + '.cpp'), selected / (part + '.inc'))
        executable = selected / 'test'
        before_work()
        run(['g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-I', selected,
             ROOT / 'tests/memory_controls.cpp', '-o', executable], ROOT, out / (name + '-build.log'), timeout=60)
        expected = 0 if name in {'original', 'refactored'} else 1
        before_work()
        log = out / (name + '.log')
        run([executable, name], ROOT, log, timeout=30, expected=expected)
        require(('PASS memory contract' if expected == 0 else 'contract failed:') in log.read_text(), 'Unrelated mutation failure')
        results[name] = {'expected_exit': expected, 'log_sha256': sha(log), 'binary_sha256': sha(executable)}
    def numerical_output(name):
        return (out / (name + '.log')).read_text().split('PASS memory contract\n')[1].split('EXIT ')[0]
    require(numerical_output('original') == numerical_output('refactored'), 'Refactor changed fixture bits')
    record = {'status': 'PASS supplementary native memory composition and semantic controls',
              'source_commit': 'd162016382f0c122433000c114e88d808b48f7a7', 'production_patches': [],
              'actual_method_binary_sha256': sha(binary), 'actual_method_log': (out / 'actual.log').read_text(),
              'baseline_record_sha256': sha(baseline / 'result.json'), 'variants': results,
              'refactor': 'All printed scalar hexfloat values identical for original/refactored fixture',
              'limitations': 'Actual methods use the pinned release build, including fast-math. Fragment controls use mock objects and -O2. No C++ object-layout, pointer-lifetime, compiler or floating-point proof. Duplicate-visit driver is a hypothetical mutant, not a discovered production defect.'}
    (out / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
    print('Actual methods and ten compiled memory controls passed.', flush=True)


if __name__ == '__main__':
    main()
