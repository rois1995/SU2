#!/usr/bin/env python3
"""Unverified source extraction; only the complete primitive caller has a new Lean grammar."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
COMMIT = 'd162016382f0c122433000c114e88d808b48f7a7'
spec = importlib.util.spec_from_file_location('old_source',ROOT.parent/'Initialization/scripts/source.py')
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

def generate(out):
    out = Path(out)
    old.generate(out)
    # Preserve old selected bodies as evidence, without modifying the protected parent.
    manifest = json.loads((out/'source-manifest.json').read_text())
    paths = ['SU2_CFD/include/fluid/CIdealGas.hpp','SU2_CFD/src/fluid/CIdealGas.cpp',
        'SU2_CFD/src/variables/CEulerVariable.cpp','SU2_CFD/src/variables/CFlowVariable.cpp',
        'SU2_CFD/src/variables/CVariable.cpp','SU2_CFD/src/solvers/CEulerSolver.cpp',
        'SU2_CFD/src/integration/CMultiGridIntegration.cpp']
    for path in paths:
        raw = old.prior.data(path)
        manifest['full_source_sha256'][path] = digest(raw)
    for key,path,anchor in [
        ('eulerConstructor','SU2_CFD/src/variables/CEulerVariable.cpp','CEulerVariable::CEulerVariable(su2double density'),
        ('flowConstructor','SU2_CFD/src/variables/CFlowVariable.cpp','CFlowVariable::CFlowVariable(unsigned long npoint'),
        ('baseConstructor','SU2_CFD/src/variables/CVariable.cpp','CVariable::CVariable(unsigned long npoint, unsigned long ndim'),
        ('secondary','SU2_CFD/src/variables/CEulerVariable.cpp','void CEulerVariable::SetSecondaryVar('),
        ('idealConstructor','SU2_CFD/src/fluid/CIdealGas.cpp','CIdealGas::CIdealGas(su2double gamma'),
        ('preprocessDispatch','SU2_CFD/src/solvers/CEulerSolver.cpp','void CEulerSolver::Preprocessing('),
        ('preSmoothing','SU2_CFD/src/integration/CMultiGridIntegration.cpp','void CMultiGridIntegration::PreSmoothing(')]:
        text = old.prior.data(path).decode()
        start,end,raw = old.prior.method(text,anchor)
        (out/'fixtures'/(key+'.cpp')).write_text(raw+'\n')
        manifest['selections'][key] = dict(path=path,span_bytes=[len(text[:start].encode()),len(text[:end].encode())],
            sha256=digest(raw.encode()),formal_status='manual implementation-model mapping or caller trace, not verified C++ translation')
    raw = (out/'fixtures/primitiveLoop.cpp').read_text().rstrip()
    variants = {'original':raw,'refactored':raw.replace('iPoint ++','++iPoint'),
        'skipLast':raw.replace('iPoint < nPoint;','iPoint + 1 < nPoint;')}
    (out/'SU2Preprocessing').mkdir(exist_ok=True)
    lines = ['import SU2Preprocessing.SourceGate','set_option maxRecDepth 100000','set_option maxHeartbeats 4000000','namespace SU2Preprocessing.Generated']
    for name,text in variants.items():
        assert text != raw or name == 'original'
        (out/'fixtures'/(name+'_primitiveLoop.cpp')).write_text(text+'\n')
        lines += ['def '+name+'Source : String := '+json.dumps(text),
            'def '+name+' : PrimitivePlan := ⟨'+('true' if name=='skipLast' else 'false')+'⟩',
            'theorem '+name+'_parsed : parsePrimitiveLoop '+name+'Source = some '+name+' := by decide']
    lines.append('end SU2Preprocessing.Generated')
    (out/'SU2Preprocessing/Generated.lean').write_text('\n\n'.join(lines)+'\n')
    manifest['new_restricted_syntax'] = 'complete SetPrimitive_Variables method; original, increment refactor, skipped-last semantic mutant'
    manifest['limits'] = 'Restricted caller grammar has exact-real execution semantics. SetPrimVar, setters, fluid fields and caller macros are manually represented. Extraction, C++ heap/ABI/lifetimes, floating point and compiler semantics are unverified.'
    (out/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    generate(p.parse_args().output)
