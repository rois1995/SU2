#!/usr/bin/env python3
"""Source-body controls: compilable numerical mutants, never parser/hash failures."""
import argparse
import json
from pathlib import Path
import sys
from source import ROOT,generate
sys.path.insert(0,str(ROOT.parent/'EvaluationKit'))
from common import before_work,limits,run,require,sha

VARIANTS = ['original','refactored','wrongBuffer','incomplete','omitDensity','omitKinetic',
    'staleFallback','eraseTail','wrongResidual','skipLast','earlySnapshot','badStride','skipReset','skipEntropy']

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    args = p.parse_args()
    limits()
    out = args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    generate(out/'generated')
    files = {f.stem:f.read_text() for f in (out/'generated/fixtures').glob('*.cpp')}
    results = {}
    for name in VARIANTS:
        dest = out/name
        dest.mkdir()
        raw = dict(files)
        if name == 'omitDensity': raw['constructor'] = raw['constructor'].replace('density*val_solution[iVar]','val_solution[iVar]')
        if name == 'omitKinetic': raw['setPrim'] = raw['setPrim'].replace('-0.5*Velocity2(iPoint)','')
        if name == 'staleFallback':
            pos = raw['setPrim'].index('/*--- Recompute the primitive variables ---*/')
            end = raw['setPrim'].index('RightVol = false;',pos)
            raw['setPrim'] = raw['setPrim'][:pos]+raw['setPrim'][end:]
        if name == 'eraseTail': raw['setPrim'] = raw['setPrim'].replace('return RightVol;','Primitive(iPoint,10)=0.0; return RightVol;')
        if name == 'skipReset': raw['velocity'] = raw['velocity'].replace('Velocity2(iPoint) = 0.0;','')
        if name == 'skipEntropy': raw['idealGas'] = raw['idealGas'].replace('if (ComputeEntropy)','if (false)')
        key = name if name in ['refactored','wrongBuffer','incomplete'] else 'original'
        for part in ['snapshot','copyLoop']: (dest/(part+'.inc')).write_text(raw[key+'_'+part])
        loopKey = name if name in ['refactored','skipLast'] else 'original'
        (dest/'primitiveLoop.inc').write_text(raw[loopKey+'_primitiveLoop'])
        for part in ['constructor','setPrim','residualZero']: (dest/(part+'.inc')).write_text(raw[part])
        for part in ['idealGas','velocity','density','pressure','sound','temperature','enthalpy','getDensity','getEnergy']:
            (dest/(part+'_body.inc')).write_text(raw[part][raw[part].index('{'):])
        binary = dest/'test'
        before_work()
        run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
            '-I',dest,ROOT/'tests/fragments.cpp','-o',binary],ROOT,out/(name+'-build.log'),timeout=60)
        before_work()
        log = out/(name+'.log')
        expected = 0 if name in ['original','refactored'] else 1
        run([binary,name],ROOT,log,timeout=30,expected=expected)
        require(('PASS native source fragments' if expected==0 else 'contract failed:') in log.read_text(),'Unrelated control failure')
        results[name] = dict(compiled=True,exit=expected,log_sha256=sha(log),binary_sha256=sha(binary))
    result = dict(status='PASS source-body semantic controls',variants=results,production_patches=[],
        source_commit='d162016382f0c122433000c114e88d808b48f7a7',test_sha256=sha(ROOT/'tests/fragments.cpp'),
        limits='Actual method bodies in mock containers, AD/serial macros, fluid storage and selected caller order. Not full C++ heap/FP/compiler verification. earlySnapshot, badStride and wrongResidual mutate the harness interface; other variants alter extracted source bodies.')
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],flush=True)

if __name__ == '__main__': main()
