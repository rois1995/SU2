#!/usr/bin/env python3
"""Compile actual selected fragments and numerical mutants against one unchanged contract."""
import argparse
import json
from pathlib import Path
import sys
from source import ROOT,generate
sys.path.insert(0,str(ROOT.parent/'EvaluationKit'))
from common import before_work,limits,run,require,sha
VARIANTS = ['original','refactored','uncapped','wrongLambda','noReset','wrongSign','skipEnergy','wrongBase','activePadding','skipLast','normalSign']
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path)
    args = p.parse_args()
    limits()
    out = args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    generate(out/'generated')
    sources = {f.stem:f.read_text() for f in (out/'generated/fixtures').glob('*.cpp')}
    results = {}
    for name in VARIANTS:
        dest = out/name
        dest.mkdir()
        raw = {key:sources[key] for key in ['unpack','scatter','dualNormal','resetLambda','localTime','timeStore','packetTraversal']}
        if name=='refactored':
            raw['timeStore'] = sources['timeStore_refactored']
            raw['scatter'] = sources['scatter_refactored']
        if name=='uncapped': raw['timeStore'] = sources['timeStore_uncapped']
        if name=='wrongLambda': raw['localTime'] = raw['localTime'].replace('GetMax_Lambda_Inv','GetMax_Lambda_Visc')
        if name=='noReset': raw['resetLambda'] = raw['resetLambda'].replace('0.0','3.0')
        if name=='wrongSign': raw['scatter'] = sources['scatter_wrongSign']
        if name=='skipEnergy': raw['scatter'] = raw['scatter'].replace('i < nVar','i + 1 < nVar')
        if name=='wrongBase': raw['packetTraversal'] = raw['packetTraversal'].replace('color.indices[k+j*in]','color.indices[k]')
        if name=='activePadding': raw['packetTraversal'] = raw['packetTraversal'].replace('mask[j] = in;','mask[j] = true;')
        if name=='skipLast': raw['packetTraversal'] = raw['packetTraversal'].replace('k < color.size','k+1 < color.size')
        if name=='normalSign': raw['dualNormal'] = raw['dualNormal'].replace('Normal(iEdge, 1) -=','Normal(iEdge, 1) +=')
        require(name in ['original','refactored'] or any(raw[k]!=sources[k] for k in raw),'Mutation did not change source')
        for key,body in raw.items(): (dest/(key+'.inc')).write_text(body)
        before_work()
        run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-I',dest,ROOT/'tests/fragments.cpp','-o',dest/'test'],ROOT,out/(name+'-build.log'),timeout=60)
        before_work()
        expected = 0 if name in ['original','refactored'] else 1
        log = out/(name+'.log')
        run([dest/'test'],ROOT,log,timeout=30,expected=expected)
        require(('PASS numerical source-fragment contract' if expected==0 else 'contract failed:') in log.read_text(),'Unrelated control failure')
        results[name] = dict(compiled=True,exit=expected,log_sha256=sha(log),source_fragments={k:sha(dest/(k+'.inc')) for k in raw},binary_sha256=sha(dest/'test'))
    result = dict(status='PASS compiled semantic source-fragment controls',variants=results,production_patches=[],source_commit='d162016382f0c122433000c114e88d808b48f7a7',test_sha256=sha(ROOT/'tests/fragments.cpp'),limitations='Finite double observations in explicit mock interfaces. No full C++ translation, heap, libm or compiler theorem. Mutants are hypothetical, not production defects.')
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],flush=True)
if __name__=='__main__': main()
