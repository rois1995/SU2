#!/usr/bin/env python3
"""Extract pinned upstream text; Lean consumes raw text, not a Python-produced AST."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
COMMIT = 'd162016382f0c122433000c114e88d808b48f7a7'
HEADER = 'SU2_CFD/include/variables/CVariable.hpp'
VECTOR = 'Common/include/linear_algebra/CSysVector.hpp'
CALLER = 'SU2_CFD/include/solvers/CFVMFlowSolverBase.hpp'
spec = importlib.util.spec_from_file_location('accepted_source', ROOT.parent / 'SourceGate/scripts/source.py')
accepted = importlib.util.module_from_spec(spec)
spec.loader.exec_module(accepted)


def data(path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', COMMIT + ':' + path])


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def method(text, anchor):
    # Unverified selection only. The bytes and surrounding complete source are frozen for review.
    if text.count(anchor) != 1:
        raise ValueError('Ambiguous or missing source anchor: ' + anchor)
    start = text.index(anchor)
    masked = re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: ' ' * len(m.group()), text, flags=re.S)
    opening = masked.index('{', start)
    depth = 1
    end = opening + 1
    while end < len(masked) and depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    if depth:
        raise ValueError('Unclosed method')
    return start, end, text[start:end]


def span(text, first, last):
    if text.count(first) != 1:
        raise ValueError('Ambiguous or missing fragment')
    start = text.index(first)
    end = text.index(last, start) + len(last)
    return start, end, text[start:end]


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    fixtures = out / 'fixtures'
    fixtures.mkdir(exist_ok=True)
    lean = out / 'SU2Euler'
    lean.mkdir(exist_ok=True)
    texts = {p: data(p).decode() for p in [HEADER, VECTOR, CALLER]}
    caller_start, _, caller_text = method(texts[CALLER], 'void Explicit_Iteration_impl(ResidualPrecond& preconditioner')
    def caller_span(first, last):
        start, end, text = span(caller_text, first, last)
        return caller_start + start, caller_start + end, text
    selections = {
        'update': (HEADER, accepted.extract(texts[HEADER])),
        'scatter': (VECTOR, method(texts[VECTOR], 'template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>\n  FORCEINLINE void UpdateBlocks')),
        'unpack': (VECTOR, method(texts[VECTOR], 'template <size_t N, size_t nVar, class VecTypeSIMD, class F>\n  FORCEINLINE static void UnpackBlock')),
        'apply': (CALLER, method(texts[CALLER], 'FORCEINLINE su2double apply(unsigned short iVar, const su2double* res, const su2double* resTrunc)')),
        'scale': (CALLER, caller_span('su2double Vol = geometry->nodes->GetVolume(iPoint) + geometry->nodes->GetPeriodicVolume(iPoint);', 'su2double Delta = nodes->GetDelta_Time(iPoint) / Vol;')),
        'call': (CALLER, caller_span('case EULER_EXPLICIT:', 'break;')),
    }
    records = {}
    statements = ['import SU2Euler.Parser', '', 'set_option maxRecDepth 100000',
                  'set_option maxHeartbeats 2000000', '', 'namespace SU2Euler.Generated', '']
    for name, (path, (start, end, text)) in selections.items():
        if '#' in text:
            raise ValueError('Preprocessor directive inside selected fragment')
        raw = texts[path].encode()
        before, after = len(texts[path][:start].encode()), len(texts[path][:end].encode())
        records[name] = {'path': path, 'span_bytes': [before, after], 'sha256': sha(text.encode())}
        (fixtures / (name + '.cpp')).write_text(text + '\n')
        if name == 'update':
            variants = accepted.variants(text)
            parser, ty = 'SU2Source.parse', 'SU2Source.Program'
        elif name == 'scatter':
            variants = {'original': text,
                        'refactored': re.sub(r'\bvec\b', 'unpacked', text),
                        'wrongSign': text.replace('] -= vec', '] += vec'),
                        'wrongMask': text.replace('mask[k] == 0', 'mask[k] != 0')}
            parser, ty = 'parseScatter', 'ScatterCode'
        elif name == 'apply':
            variants = {'original': text, 'subtractTrunc': text.replace(' + resTrunc', ' - resTrunc')}
            parser, ty = 'parseApply', 'Op'
        elif name == 'call':
            variants = {'original': text, 'wrongSign': text.replace('-Res*Delta', '+Res*Delta')}
            parser, ty = 'parseCall', 'Bool'
        elif name == 'scale':
            variants = {'original': text}
            parser, ty = 'parseScale', 'Unit'
        else:
            # UnpackBlock remains a separately disclosed manual interpretation, tested natively.
            continue
        records[name]['variants'] = {}
        for variant, source in variants.items():
            key = name + '_' + variant
            (fixtures / (key + '.cpp')).write_text(source + '\n')
            statements += [f'def {key}Source : String := ' + json.dumps(source),
                           f'def {key}Code : {ty} := ({parser} {key}Source).get (by decide)',
                           f'theorem {key}_parsed : {parser} {key}Source = some {key}Code := by decide', '']
            records[name]['variants'][variant] = {'sha256': sha(source.encode()),
                'full_file_sha256': sha(raw[:before] + source.encode() + raw[after:])}
        if len({v['sha256'] for v in records[name]['variants'].values()}) != len(variants):
            raise ValueError('Mutation did not change source')
    statements.append('end SU2Euler.Generated')
    (lean / 'Generated.lean').write_text('\n'.join(statements) + '\n')
    paths = list(texts) + ['Common/include/containers/C2DContainer.hpp', 'Common/include/code_config.hpp',
             'Common/include/parallelization/vectorization.hpp', 'Common/include/parallelization/omp_structure.hpp',
             'Common/src/linear_algebra/CSysVector.cpp', 'SU2_CFD/src/variables/CVariable.cpp',
             'SU2_CFD/src/solvers/CEulerSolver.cpp', 'SU2_CFD/include/solvers/CFVMFlowSolverBase.inl',
             'SU2_CFD/include/numerics/util.hpp', 'SU2_CFD/include/numerics_simd/CNumericsSIMD.cpp',
             'SU2_CFD/include/numerics_simd/flow/convection/upwind.hpp',
             'SU2_CFD/src/integration/CSingleGridIntegration.cpp',
             'SU2_CFD/src/integration/CMultiGridIntegration.cpp',
             'SU2_CFD/src/integration/CIntegration.cpp', 'SU2_CFD/src/integration/CIntegrationFactory.cpp',
             'SU2_CFD/src/solvers/CSolverFactory.cpp', 'SU2_CFD/src/iteration/CFluidIteration.cpp']
    manifest = {'source_commit': COMMIT, 'production_patches': [], 'selections': records,
                'source_files': {p: {'sha256': sha(data(p)), 'git_blob': subprocess.check_output(
                    ['git', '-C', str(REPO), 'rev-parse', COMMIT + ':' + p], text=True).strip()} for p in paths},
                'scope': 'Restricted syntax and manually assigned serial exact-real semantics; no certified C++ frontend, heap refinement or IEEE-754 proof.'}
    (out / 'source-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    sample = selections['scatter'][1][2]
    for invalid in [sample + sample, sample[:-1]]:
        try:
            method(invalid, 'template <size_t N, class T, class VecTypeSIMD, class F = ScalarType>')
        except ValueError:
            pass
        else:
            raise AssertionError('Unsafe source selection accepted')
    assert accepted.extract(texts[HEADER])[2] == (ROOT.parent / 'SourceGate/fixtures/original.cpp').read_text().removesuffix('\n')
    print('Regenerated upstream scalar gate and actual SIMD/Euler caller source fragments.', flush=True)


if __name__ == '__main__':
    generate(ROOT)
