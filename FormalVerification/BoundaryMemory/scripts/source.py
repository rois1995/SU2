#!/usr/bin/env python3
"""Unverified byte selection. Lean parses each complete declared fragment."""
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('old_selection', ROOT.parent / 'BoundaryPath/scripts/source.py')
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)
base = prior.prior
COMMIT = base.COMMIT


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    (out / 'fixtures').mkdir(exist_ok=True)
    (out / 'SU2BoundaryMemory').mkdir(exist_ok=True)
    paths = ['SU2_CFD/include/solvers/CFVMFlowSolverBase.inl',
             'SU2_CFD/src/solvers/CEulerSolver.cpp',
             'SU2_CFD/include/solvers/CFVMFlowSolverBase.hpp',
             'SU2_CFD/include/variables/CFlowVariable.hpp',
             'Common/include/linear_algebra/CSysVector.hpp',
             'Common/include/containers/C2DContainer.hpp',
             'SU2_CFD/src/variables/CFlowVariable.cpp']
    texts = {p: base.data(p).decode() for p in paths}
    records = {}
    lean = ['import SU2BoundaryMemory.Parser', 'set_option maxRecDepth 100000',
            'set_option maxHeartbeats 8000000', 'namespace SU2BoundaryMemory.Generated']
    identity = lambda x: x
    def add(key, path, span, parser, ty, variants=None):
        start, end, raw = span
        (out / 'fixtures' / (key + '.cpp')).write_text(raw + '\n')
        record = {'path': path, 'span_bytes': [len(texts[path][:start].encode()), len(texts[path][:end].encode())],
                  'sha256': base.sha(raw.encode()), 'variants': {}}
        for variant, transform in (variants or {'original': identity}).items():
            src = transform(raw)
            name = key + '_' + variant
            (out / 'fixtures' / (name + '.cpp')).write_text(src + '\n')
            lean.extend([f'def {name}Source : String := ' + json.dumps(src),
                         f'def {name} : {ty} := ({parser} {name}Source).get (by decide)',
                         f'theorem {name}_parsed : {parser} {name}Source = some {name} := by decide'])
            record['variants'][variant] = {'sha256': base.sha(src.encode()),
                'full_file_sha256': base.sha((texts[path][:start] + src + texts[path][end:]).encode())}
        records[key] = record
    def statement(path, raw, after=''):
        text = texts[path]
        start = text.index(raw, text.index(after) if after else 0)
        return start, start + len(raw), raw
    add('oldBinding', paths[0], statement(paths[0], 'su2double* solutionOld = nodes->GetSolution_Old(iPoint);', '::BC_Sym_Plane('),
        'parseOldBinding', 'Nat', {'original': identity, 'wrongBuffer': lambda x: x.replace('GetSolution_Old', 'GetSolution')})
    add('primitiveExtra', paths[1], statement(paths[1], 'nPrimVar = nDim + 9;'), 'parsePrimitiveExtra', 'Nat',
        {'original': identity, 'shortStride': lambda x: x.replace('+ 9', '+ 4')})
    add('cacheLookup', paths[2], base.method(texts[paths[2]], 'inline su2double* GetCharacPrimVar('),
        'parseCacheLookup', 'CacheMode', {'original': identity,
            'firstMarker': lambda x: x.replace('CharacPrimVar[val_marker]', 'CharacPrimVar[0]'),
            'firstVertex': lambda x: x.replace('[val_vertex]', '[0]')})
    raw = 'V_infty[0] = Pressure/(Gas_Constant*Density);'
    end = 'V_infty[nDim+3] = Energy + Pressure/Density;'
    start = texts[paths[1]].index(raw, texts[paths[1]].index('void CEulerSolver::BC_Far_Field'))
    stop = texts[paths[1]].index(end, start) + len(end)
    add('cacheWrites', paths[1], (start, stop, texts[paths[1]][start:stop]), 'parseCacheWrites', 'Bool',
        {'original': identity, 'eraseTail': lambda x: x + '\nfor (unsigned short j = 6; j < nPrimVar; ++j) V_infty[j] = 0.0;'})
    for key, buffer, binder, target in [('wallOld', 'old', 'vp', 'solutionOld[iVel + iDim]'),
            ('wallRes', 'residual', 'normalRes', 'LinSysRes(iPoint, iVel + iDim)'),
            ('wallTrunc', 'truncation', 'normalRes', 'Res_TruncError[iVel + iDim]')]:
        raw = (ROOT.parent / 'BoundaryPath/fixtures' / (key + '_original.cpp')).read_text().rstrip('\n')
        start = texts[paths[0]].index(raw)
        term = '(solutionOld[iVel + jDim] - gridVel[jDim])' if buffer == 'old' else target.replace('iDim', 'jDim')
        def refresh(x, b=binder, t=target, r=term):
            injection = b + ' = 0.0; for (auto jDim = 0u; jDim < nDim; jDim++) { ' + b + ' += ' + r + ' * UnitNormal[jDim]; }\n      '
            return x.replace(t + ' -=', injection + t + ' -=')
        def reverse(x):
            header = 'for (auto iDim = 0u; iDim < nDim; iDim++) {'
            pos = x.rindex(header)
            return x[:pos] + x[pos:].replace(header, 'for (auto iDim = nDim; iDim-- > 0;) {', 1)
        add(key, paths[0], (start, start + len(raw), raw), 'parseProjectionPlan .' + buffer, 'ProjectionPlan',
            {'original': identity, 'refresh': refresh, 'refactored': reverse})
    add('setBlock', paths[4], base.method(texts[paths[4]], 'template <class VectorType, bool Overwrite = true>'),
        'parseSetBlock', 'SU2Euler.Op')
    exacts = [('primitiveGetter', paths[3], base.method(texts[paths[3]], 'inline su2double* GetPrimitive(unsigned long iPoint)')),
              ('addBlock', paths[4], base.method(texts[paths[4]], 'template <class VectorType>\n  FORCEINLINE void AddBlock(')),
              ('cacheAllocation', paths[0], statement(paths[0], 'AllocVectorOfMatrices(nVertex, nPrimVar, CharacPrimVar);')),
              ('primitiveAllocation', paths[6], statement(paths[6], 'Primitive.resize(nPoint, nPrimVar) = su2double(0.0);')),
              ('rowPointer', paths[5], statement(paths[5], 'return &m_data[k * N];'))]
    for key, path, span in exacts:
        add(key, path, span, 'parse' + key[0].upper() + key[1:], 'Unit')
    lean.append('end SU2BoundaryMemory.Generated')
    (out / 'SU2BoundaryMemory/Generated.lean').write_text('\n\n'.join(lean) + '\n')
    # Entire relevant implementations are pinned for manual translation/branch review.
    prior_manifest = json.loads((ROOT.parent / 'BoundaryPath/source-manifest.json').read_text())
    full = dict(prior_manifest['full_source_sha256'])
    full.update({p: base.sha(base.data(p)) for p in paths})
    record = {'source_commit': COMMIT, 'production_patches': [], 'selections': records,
              'full_source_sha256': full,
              'scope': 'Closed source fragments only; selection, caller enumeration, characteristic/Roe translation, C++ object/pointer semantics and compilation remain unverified.'}
    (out / 'source-manifest.json').write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    import sys
    generate(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT)
