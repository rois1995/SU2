#!/usr/bin/env python3
"""Link the pinned baseline libraries and execute actual SU2 timestep, geometry and interior methods."""
import argparse
import json
from pathlib import Path
import shlex
import shutil
import sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'EvaluationKit'))
from common import before_work,limits,require,run,sha

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('baseline',type=Path)
    p.add_argument('output',type=Path)
    args = p.parse_args()
    limits()
    baseline = args.baseline.resolve()
    record = json.loads((baseline/'result.json').read_text())
    require(record['source_commit']=='d162016382f0c122433000c114e88d808b48f7a7' and record['production_patches']==[],'Wrong native baseline')
    source = baseline/'SU2'
    build = source/'build'
    out = args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    entry = next(x for x in json.loads((build/'compile_commands.json').read_text()) if x['file'].endswith('edge_residual_blocks_tests.cpp'))
    flags = shlex.split(entry['command'])
    flags = flags[:flags.index('-MD')]
    before_work()
    run(flags+['-I',source,'-c',ROOT/'tests/interior_native.cpp','-o',out/'actual.o'],build,out/'build.log',timeout=120)
    before_work()
    run(['c++',out/'actual.o','-Wl,--start-group',build/'SU2_CFD/src/libSU2core.a',
        build/'Common/src/libSU2Common.a','-Wl,--end-group','-o',out/'actual'],build,out/'build.log',timeout=120)
    shutil.copytree(ROOT.parent/'EulerPath/tests/integration',out/'case')
    before_work()
    run([out/'actual','euler.cfg'],out/'case',out/'actual.log',timeout=120)
    require('PASS actual interior checks=' in (out/'actual.log').read_text(),'Missing actual-method result')
    config = (out/'case/euler.cfg').read_text()
    (out/'case/capped.cfg').write_text(config+'\nMAX_DELTA_TIME= 1e-9\n')
    before_work()
    run([out/'actual','capped.cfg'],out/'case',out/'capped.log',timeout=120)
    require('PASS actual interior checks=' in (out/'capped.log').read_text(),'Capped timestep result missing')
    for name,cfl in [('selected','0.1'),('stress10','10'),('stress100','100')]:
        (out/'case'/(name+'.cfg')).write_text(config.replace('CFL_NUMBER= 0.1','CFL_NUMBER= '+cfl))
        before_work()
        run([out/'actual',name+'.cfg','robustness'],out/'case',out/(name+'.log'),timeout=120,expected=1 if name=='stress100' else 0)
        expected = 'SU2 has diverged (Residual > 10^20 detected).' if name=='stress100' else 'ROBUSTNESS cfl='
        require(expected in (out/(name+'.log')).read_text(),'Unexpected robustness observation')
    result = dict(status='PASS actual pinned SU2 timestep, geometry, interior and old-buffer observations',source_commit=record['source_commit'],
        source_sha256=sha(ROOT/'tests/interior_native.cpp'),binary_sha256=sha(out/'actual'),
        baseline_result_sha256=sha(baseline/'result.json'),compiler_flags=flags,
        limitations='Finite actual-method observations and manually controlled caller replay; stress CFL values differ from the selected profile; unmodified baseline release flags. No C++ heap, IEEE, libm or compiler proof. No production bug demonstrated.')
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],flush=True)

if __name__ == '__main__': main()
