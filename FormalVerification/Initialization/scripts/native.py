#!/usr/bin/env python3
"""Compile unchanged source bodies in mock storage plus semantic mutations."""
import argparse
import json
from pathlib import Path
import sys
from source import ROOT, generate
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import before_work, limits, run, require, sha


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    args = p.parse_args()
    limits()
    out = args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    generated = out/'generated'
    generate(generated)
    files = {f.stem:f.read_text() for f in (generated/'fixtures').glob('*.cpp')}
    variants = ['original','refactored','wrongBuffer','incomplete','omitDensity','omitKinetic','staleFallback','eraseTail','wrongResidual']
    results = {}
    for name in variants:
        dest = out/name
        dest.mkdir()
        raw = dict(files)
        if name == 'omitDensity': raw['constructor'] = raw['constructor'].replace('density*val_solution[iVar]','val_solution[iVar]')
        if name == 'omitKinetic': raw['setPrim'] = raw['setPrim'].replace('-0.5*Velocity2(iPoint)','')
        if name == 'staleFallback':
            # Retain the first attempt but skip the fallback recomputation: valid C++.
            pos = raw['setPrim'].index('/*--- Recompute the primitive variables ---*/')
            end = raw['setPrim'].index('RightVol = false;',pos)
            raw['setPrim'] = raw['setPrim'][:pos] + raw['setPrim'][end:]
        if name == 'eraseTail': raw['setPrim'] = raw['setPrim'].replace('return RightVol;', 'Primitive(iPoint,10)=0.0; return RightVol;')
        key = name if name in ['refactored','wrongBuffer','incomplete'] else 'original'
        for part in ['snapshot','copyLoop']:
            (dest/(part+'.inc')).write_text(raw[key+'_'+part])
        for part in ['constructor','setPrim','residualZero']:
            (dest/(part+'.inc')).write_text(raw[part])
        for part in ['idealGas','velocity','density','pressure','sound','temperature','enthalpy','getDensity','getEnergy']:
            (dest/(part+'_body.inc')).write_text(raw[part][raw[part].index('{'):])
        binary = dest/'test'
        before_work()
        run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-I',dest,ROOT/'tests/fragments.cpp','-o',binary],ROOT,out/(name+'-build.log'),timeout=60)
        before_work()
        log = out/(name+'.log')
        expected = 0 if name in ['original','refactored'] else 1
        run([binary,name],ROOT,log,timeout=30,expected=expected)
        require(('PASS native source fragments' if not expected else 'contract failed:') in log.read_text(),'Unrelated control failure')
        results[name] = dict(exit=expected,log_sha256=sha(log),binary_sha256=sha(binary))
    record = dict(status='PASS supplementary source-body controls',variants=results,
        test_sha256=sha(ROOT/'tests/fragments.cpp'), script_sha256=sha(Path(__file__)),
        source_manifest_sha256=sha(generated/'source-manifest.json'),
        production_patches=[],source_commit='d162016382f0c122433000c114e88d808b48f7a7',
        limits='Unchanged production method bodies compiled in mock classes/containers, not a full native SU2 build or certified C++ semantics. No production defect is claimed.')
    (out/'result.json').write_text(json.dumps(record,indent=2)+'\n')
    print(record['status'],flush=True)


if __name__ == '__main__':
    main()
