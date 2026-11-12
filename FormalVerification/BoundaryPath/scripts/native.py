#!/usr/bin/env python3
"""Supplementary native execution; never substitutes for Lean or certified C++ semantics."""
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path, help='Private build_baseline.py output')
    parser.add_argument('output', type=Path, help='Fresh result directory')
    args = parser.parse_args()
    limits()
    baseline = args.baseline.resolve()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    source = baseline / 'SU2'
    build = source / 'build'
    commands = json.loads((build / 'compile_commands.json').read_text())
    command, = [c for c in commands if c['file'].endswith('edge_residual_blocks_tests.cpp')]
    flags = shlex.split(command['command'])
    flags = flags[:flags.index('-MD')]
    flags += ['-I', str(source)]
    obj = out / 'boundary_native.o'
    binary = out / 'boundary_native'
    before_work()
    run(flags + ['-c', ROOT / 'tests/boundary_native.cpp', '-o', obj], build, out / 'full-build.log', timeout=300)
    before_work()
    run(['c++', obj, '-Wl,--start-group', build / 'SU2_CFD/src/libSU2core.a',
         build / 'Common/src/libSU2Common.a', '-Wl,--end-group', '-o', binary], build,
        out / 'full-build.log', timeout=180)
    shutil.copytree(ROOT.parent / 'EulerPath/tests/integration', out / 'case')
    before_work()
    run([binary, 'euler.cfg'], out / 'case', out / 'full-native.log', timeout=90)
    require('PASS checks=' in (out / 'full-native.log').read_text(), 'Missing actual-method checks')
    generate(out / 'generated')
    variants = {'original': {}, 'refactored': {'points': 'refactored', 'components': 'refactored'},
                'wallOldSign': {'wallOld': 'wrongSign'}, 'wallResidualSign': {'wallRes': 'wrongSign'},
                'wallTruncationSign': {'wallTrunc': 'wrongSign'}, 'farSign': {'farAdd': 'wrongSign'},
                'skipPoint': {'points': 'skipLast'}, 'skipEnergy': {'components': 'skipLast'},
                'wrongTimestep': {'timestep': 'wrongField'}}
    results = {}
    for name, choices in variants.items():
        selected = out / name
        selected.mkdir()
        for part in ['wallOld', 'wallRes', 'wallTrunc', 'farAdd', 'points', 'components', 'volume', 'periodic', 'timestep']:
            variant = choices.get(part, 'original')
            shutil.copyfile(out / 'generated/fixtures' / (part + '_' + variant + '.cpp'), selected / (part + '.inc'))
        for part in ['apply', 'call', 'scale', 'update']:
            shutil.copyfile(ROOT.parent / 'EulerPath/fixtures' / (part + '_original.cpp'), selected / (part + '.inc'))
        executable = selected / 'test'
        before_work()
        run(['g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-I', selected,
             ROOT / 'tests/fragments.cpp', '-o', executable], ROOT, out / (name + '-build.log'), timeout=60)
        expected = 0 if name in {'original', 'refactored'} else 1
        before_work()
        log = out / (name + '.log')
        run([executable], ROOT, log, timeout=30, expected=expected)
        require(('PASS' if expected == 0 else 'contract failed:') in log.read_text(), 'Unrelated variant failure')
        results[name] = {'expected_exit': expected, 'log_sha256': sha(log), 'binary_sha256': sha(executable)}
    record = {'status': 'PASS supplementary native boundary and full-loop evidence',
              'source_commit': 'd162016382f0c122433000c114e88d808b48f7a7',
              'actual_method_binary_sha256': sha(binary), 'actual_method_log': (out / 'full-native.log').read_text(),
              'baseline_record_sha256': sha(baseline / 'result.json'), 'variants': results,
              'limitations': 'Actual methods linked from pinned release build (fast-math retained). Mutants/refactor are selected fragments in mock object context, -O2 without fast-math. No certified compiler, object-layout refinement or floating-point proof.'}
    (out / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
    print('Actual-method checks and nine source-fragment controls passed.', flush=True)


if __name__ == '__main__':
    main()
