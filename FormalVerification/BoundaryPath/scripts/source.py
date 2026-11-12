#!/usr/bin/env python3
"""Unverified byte selector; the Lean gate parses the complete declared fragments."""
import importlib.util
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('prior', ROOT.parent / 'EulerPath/scripts/source.py')
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)
COMMIT = prior.COMMIT


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    (out / 'fixtures').mkdir(exist_ok=True)
    (out / 'SU2Boundary').mkdir(exist_ok=True)
    paths = ['SU2_CFD/include/solvers/CFVMFlowSolverBase.inl',
             'SU2_CFD/include/solvers/CFVMFlowSolverBase.hpp',
             'SU2_CFD/src/solvers/CEulerSolver.cpp',
             'Common/include/geometry/dual_grid/CPoint.hpp',
             'SU2_CFD/include/variables/CVariable.hpp']
    texts = {p: prior.data(p).decode() for p in paths}
    records = {}
    lean = ['import SU2Boundary.Parser', 'set_option maxRecDepth 100000',
            'set_option maxHeartbeats 4000000', 'namespace SU2Boundary.Generated']
    def add(key, path, span, parser, ty, variants):
        start, end, raw = span
        (out / 'fixtures' / (key + '.cpp')).write_text(raw + '\n')
        record = {'path': path, 'span_bytes': [len(texts[path][:start].encode()), len(texts[path][:end].encode())],
                  'sha256': prior.sha(raw.encode()), 'variants': {}}
        for variant, transform in variants.items():
            src = transform(raw)
            name = key + '_' + variant
            (out / 'fixtures' / (name + '.cpp')).write_text(src + '\n')
            lean.extend([f'def {name}Source : String := ' + json.dumps(src),
                         f'def {name} : {ty} := ({parser} {name}Source).get (by decide)',
                         f'theorem {name}_parsed : {parser} {name}Source = some {name} := by decide'])
            record['variants'][variant] = {'sha256': prior.sha(src.encode()),
                'full_file_sha256': prior.sha((texts[path][:start] + src + texts[path][end:]).encode())}
        records[key] = record
    identity = lambda x: x
    wall = texts[paths[0]]
    for key, first, target, buffer in [
        ('wallOld', 'su2double vp = 0.0;', 'solutionOld[iVel + iDim]', 'old'),
        ('wallRes', 'su2double normalRes = 0.0;', 'LinSysRes(iPoint, iVel + iDim)', 'residual'),
        ('wallTrunc', 'normalRes = 0.0;\n    su2double* Res_TruncError', 'Res_TruncError[iVel + iDim]', 'truncation')]:
        start = wall.index(first, wall.index('::BC_Sym_Plane('))
        end = wall.index('}', wall.index(target + ' -=', start)) + 1
        add(key, paths[0], (start, end, wall[start:end]), 'parseProjection .' + buffer,
            'SU2Euler.Op', {'original': identity, 'wrongSign': lambda x: x.replace(' -= ', ' += ')})
    text = texts[paths[2]]
    start = text.index('LinSysRes.AddBlock(iPoint, residual);', text.index('void CEulerSolver::BC_Far_Field'))
    raw = 'LinSysRes.AddBlock(iPoint, residual);'
    add('farAdd', paths[2], (start, start + len(raw), raw), 'parseBoundaryAdd', 'SU2Euler.Op',
        {'original': identity, 'wrongSign': lambda x: x.replace('AddBlock', 'SubtractBlock')})
    text = texts[paths[1]]
    begin = text.index('void Explicit_Iteration_impl(')
    for key, point, ty, var, bound in [('points', True, 'unsigned long', 'iPoint', 'nPointDomain'),
                                     ('components', False, 'unsigned short', 'iVar', 'nVar')]:
        raw = f'for ({ty} {var} = 0; {var} < {bound}; {var}++) {{'
        start = text.index(raw, begin)
        add(key, paths[1], (start, start + len(raw), raw), 'parseLoop ' + str(point).lower(), 'LoopCode',
            {'original': identity, 'refactored': lambda x, v=var: x.replace(v + '++', '++' + v),
             'skipLast': lambda x, b=bound: x.replace('< ' + b + ';', '< ' + b + ' - 1;')})
    for key, path, anchor, method in [
        ('volume', paths[3], 'inline su2double& GetVolume(', 'GetVolume'),
        ('volumeConst', paths[3], 'inline const su2double& GetVolume(', 'GetVolume'),
        ('periodic', paths[3], 'inline su2double& GetPeriodicVolume(', 'GetPeriodicVolume'),
        ('periodicConst', paths[3], 'inline const su2double& GetPeriodicVolume(', 'GetPeriodicVolume'),
        ('timestep', paths[4], 'inline su2double GetDelta_Time(', 'GetDelta_Time')]:
        variants = {'original': identity}
        if key == 'timestep': variants['wrongField'] = lambda x: x.replace('return Delta_Time', 'return LocalCFL')
        add(key, path, prior.method(texts[path], anchor), 'parseGetter ' + json.dumps(method), 'Getter', variants)
    lean.append('end SU2Boundary.Generated')
    (out / 'SU2Boundary/Generated.lean').write_text('\n\n'.join(lean) + '\n')
    extra = ['SU2_CFD/src/numerics/flow/convection/roe.cpp', 'SU2_CFD/include/numerics/CNumerics.hpp',
             'SU2_CFD/src/numerics/CNumerics.cpp', 'SU2_CFD/src/drivers/CDriver.cpp',
             'SU2_CFD/src/integration/CIntegration.cpp', 'SU2_CFD/src/integration/CMultiGridIntegration.cpp',
             'Common/src/geometry/dual_grid/CPoint.cpp', 'SU2_CFD/src/variables/CVariable.cpp',
             'SU2_CFD/src/variables/CFlowVariable.cpp']
    manifest = {'source_commit': COMMIT, 'production_patches': [], 'selections': records,
                'full_source_sha256': {p: prior.sha(prior.data(p)) for p in paths + extra},
                'scope': 'Closed fragments only; caller selection, characteristic/Roe translation, object layout and compilation are not certified C++ semantics.'}
    (out / 'source-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')


if __name__ == '__main__':
    import sys
    generate(Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT)
