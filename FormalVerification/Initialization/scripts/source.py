#!/usr/bin/env python3
"""Unverified selection of pinned complete methods and declared constructor spans."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
COMMIT = 'd162016382f0c122433000c114e88d808b48f7a7'
spec = importlib.util.spec_from_file_location('prior_selection', ROOT.parent / 'EulerPath/scripts/source.py')
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def generate(out):
    out = Path(out)
    (out / 'fixtures').mkdir(parents=True, exist_ok=True)
    (out / 'SU2Initialization').mkdir(exist_ok=True)
    source_paths = [
        'SU2_CFD/src/variables/CEulerVariable.cpp', 'SU2_CFD/include/variables/CEulerVariable.hpp',
        'SU2_CFD/src/variables/CVariable.cpp', 'SU2_CFD/src/variables/CFlowVariable.cpp',
        'SU2_CFD/src/fluid/CIdealGas.cpp', 'Common/include/parallelization/omp_structure.hpp',
        'Common/include/linear_algebra/CSysVector.hpp', 'SU2_CFD/src/solvers/CEulerSolver.cpp',
        'SU2_CFD/src/integration/CMultiGridIntegration.cpp', 'SU2_CFD/src/integration/CIntegration.cpp',
        'SU2_CFD/include/solvers/CFVMFlowSolverBase.inl',
        'SU2_CFD/include/numerics_simd/flow/convection/upwind.hpp',
        'Common/include/containers/C2DContainer.hpp', 'SU2_CFD/include/solvers/CFVMFlowSolverBase.hpp',
        'SU2_CFD/include/solvers/CSolver.hpp']
    texts = {p: prior.data(p).decode() for p in source_paths}
    records = {}
    def add(key, path, selection, parsed=None):
        start, end, raw = selection
        (out / 'fixtures' / (key + '.cpp')).write_text(raw + '\n')
        records[key] = dict(path=path, span_bytes=[len(texts[path][:start].encode()), len(texts[path][:end].encode())],
            sha256=sha(raw.encode()), formal_status=parsed or 'manual implementation-model correspondence; not a Lean C++ translation')
        return raw
    def method(key, path, anchor, parsed=None):
        return add(key, path, prior.method(texts[path], anchor), parsed)
    def span(key, path, first, last):
        return add(key, path, prior.span(texts[path], first, last))
    p = source_paths[0]
    method('setPrim', p, 'bool CEulerVariable::SetPrimVar(')
    span('constructor', p, 'su2double val_solution[5]', 'Solution_Old = Solution;')
    for key, name in [('velocity','SetVelocity'), ('pressure','SetPressure'), ('sound','SetSoundSpeed'),
                      ('enthalpy','SetEnthalpy'), ('density','SetDensity'), ('temperature','SetTemperature'),
                      ('getDensity','GetDensity'), ('getEnergy','GetEnergy')]:
        prefix = {'sound':'  bool ', 'velocity':'  inline void ', 'enthalpy':'  inline void ',
                  'getDensity':'  inline su2double ', 'getEnergy':'  inline su2double '}.get(key, '  inline bool ')
        method(key, source_paths[1], prefix + name + '(')
    snapshot = method('snapshot', source_paths[2], 'void CVariable::Set_OldSolution()', 'restricted Lean grammar')
    loop = method('copyLoop', source_paths[5], 'template <class T, class U>\nvoid parallelCopy(', 'restricted Lean grammar')
    method('idealGas', source_paths[4], 'void CIdealGas::SetTDState_rhoe(')
    span('truncationZero', source_paths[3], 'Res_TruncError.resize(nPoint, nVar)', 'Res_TruncError.resize(nPoint, nVar) = su2double(0.0);')
    method('residualZero', source_paths[6], '  inline void SetValZero(void)')
    for key,path,anchor in [('preprocessing',source_paths[7],'void CEulerSolver::CommonPreprocessing('),
                            ('primitiveLoop',source_paths[7],'unsigned long CEulerSolver::SetPrimitive_Variables('),
                            ('upwindDispatch',source_paths[7],'void CEulerSolver::Upwind_Residual('),
                            ('spaceIntegration',source_paths[9],'void CIntegration::Space_Integration('),
                            ('simdEdgeLoop',source_paths[10],'void CFVMFlowSolverBase<V, R>::EdgeFluxResidual(')]:
        method(key,path,anchor)
    # The complete integration source is hash-bound even though only trace locators are reported.
    variants = {'original': (snapshot,loop),
        'wrongBuffer': (snapshot.replace('Solution_Old.data()', 'Solution.data()'),loop),
        'incomplete': (snapshot,loop.replace('i < size;', 'i + 1 < size;')),
        'refactored': (snapshot,loop.replace('for (size_t i = 0; i < size; ++i)', 'for (size_t i = size; i-- > 0;)'))}
    lines = ['import SU2Initialization.SourceGate', 'set_option maxRecDepth 100000',
             'set_option maxHeartbeats 4000000', 'namespace SU2Initialization.Generated']
    for key,(snap,copy) in variants.items():
        for kind,raw,parser,ty in [('snapshot',snap,'parseSnapshot','Nat'), ('loop',copy,'parseCopyLoop','CopyPlan')]:
            name = key + '_' + kind
            lines.extend([f'def {name}Source : String := ' + json.dumps(raw),
                f'def {name} : {ty} := ({parser} {name}Source).get (by decide)',
                f'theorem {name}_parsed : {parser} {name}Source = some {name} := by decide'])
        (out/'fixtures'/(key+'_snapshot.cpp')).write_text(snap+'\n')
        (out/'fixtures'/(key+'_copyLoop.cpp')).write_text(copy+'\n')
    lines.append('end SU2Initialization.Generated')
    (out/'SU2Initialization/Generated.lean').write_text('\n\n'.join(lines)+'\n')
    manifest = dict(source_commit=COMMIT,production_patches=[],selections=records,
        full_source_sha256={p:sha(t.encode()) for p,t in texts.items()},
        limits='Only snapshot and copy-loop syntax has restricted Lean execution semantics. Other methods are manually modeled or trace-only. C++ parsing, object semantics and floating-point correspondence are unverified.')
    (out/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    generate(p.parse_args().output)
