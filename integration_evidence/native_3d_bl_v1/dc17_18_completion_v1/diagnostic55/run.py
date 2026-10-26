"""One bounded replay; immutable pins checked before/after, independently saved stage."""
import json
import os
import subprocess
import sys
import time
from pathlib import Path
from prepare import ROOT, sha

repo=ROOT.parents[3]
assert (repo/'Common').is_dir()
mode=sys.argv[1];assert mode in ('plain','observed')
folder=ROOT/mode
assert not (folder/'status.json').exists(), 'Inspect prior handle/status instead of overwriting a run'
binary=repo/'integration_evidence/build-integrated-v2/UnitTests/test_driver'
baseline=json.loads((ROOT.parent/'validation54/source_input_pins.json').read_text())
changed=[]
for name,expected in baseline.items():
    p=Path(name);p=p if p.is_absolute() else repo/p
    if sha(p)!=expected:changed.append(name)
assert changed==['UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp'], changed
paths=[repo/name for name in baseline]
paths += [binary, ROOT/'mpi_calls.so', ROOT/'sensor_original.bin',folder/'run.cfg',folder/'m6.su2',folder/'m6.su2.native_ref',ROOT/'run.py']
# Existing loaded-plugin pins are a strict baseline; record the current linked runtime too.
loaded=json.loads((ROOT.parent/'validation54/m6_euler/loaded_runtime_pins.json').read_text())
for name,expected in loaded.items():
    assert sha(name)==expected,name
    paths.append(Path(name))
pins={str(p):sha(p) for p in paths}
(folder/'pins.json').write_text(json.dumps(pins,indent=2)+'\n')
env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
    SU2_NATIVE_FROZEN_CONFIG=str(folder/'run.cfg'),SU2_NATIVE_FROZEN_SENSOR3D=str(ROOT/'sensor_original.bin'))
launch=['env']
if mode=='observed':
    launch+=['LD_PRELOAD='+str(ROOT/'mpi_calls.so'),'SU2_NATIVE_FROZEN_WITNESSES=1']
launch += [str(binary),'[NativeFrozenVolume3D]']
command=['nice','-n','19','/usr/bin/time','-v','timeout','240','mpirun','--bind-to','none','-n','6',*launch]
(folder/'command.json').write_text(json.dumps(dict(argv=command,environment={k:env[k] for k in ['OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS','SU2_NATIVE_FROZEN_CONFIG','SU2_NATIVE_FROZEN_SENSOR3D']},changed_sources=changed),indent=2)+'\n')
start=time.time()
with (folder/'run.log').open('w') as out:
    proc=subprocess.Popen(command,cwd=folder,env=env,stdout=out,stderr=subprocess.STDOUT)
    (folder/'status.json').write_text(json.dumps(dict(state='running',pid=proc.pid,start=start))+'\n')
    code=proc.wait()
for name,expected in pins.items():assert sha(name)==expected,name
result=dict(state='terminal',exit_code=code,elapsed_seconds=time.time()-start,pins_verified=True)
if (folder/'construction_result.json').exists():result['native']=json.loads((folder/'construction_result.json').read_text())
(folder/'status.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
sys.exit(code)
