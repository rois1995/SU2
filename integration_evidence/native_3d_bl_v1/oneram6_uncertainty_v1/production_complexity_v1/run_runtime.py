"""One current-binary M6 metric initializer from saved flow; no adaptation or convergence campaign."""
import json,os,re,subprocess,sys,time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from run import ROOT,BUILD,sha,pins
BASE=Path(__file__).resolve().parent;CASE=BASE/'m6_runtime_metric';BINARY=BUILD/'SU2_CFD/src/SU2_CFD'
paths=[CASE/p for p in ['run.cfg','Coarse.su2','Coarse.su2.native_ref','donor_flow.dat']]
paths += [BINARY,ROOT/'SU2_CFD/src/adaptation/CNativeComplexity3D.cpp',ROOT/'SU2_CFD/include/adaptation/CNativeComplexity3D.hpp',Path(__file__).resolve()]
proof={'scope':'One saved-state iteration, real RANS/SA Hessian and bounded/graded metric normalization; no remeshing.',
       'pins':dict(pins(),**{str(p.relative_to(ROOT)):sha(p) for p in paths})}
p=CASE/'provenance.json'
if p.exists():assert json.loads(p.read_text())==proof
else:p.write_text(json.dumps(proof,indent=2)+'\n')
result=CASE/'result.json'
if result.exists():
 saved=json.loads(result.read_text());assert saved['exit_code']==0 and saved['provenance_sha256']==sha(p) and saved['log_sha256']==sha(CASE/'run.log')
 print('REUSE_M6_METRIC_RUNTIME',json.dumps(saved),flush=True);sys.exit(0)
command=['taskset','-c','0-3','nice','-n','19','/usr/bin/time','-v','-o','resources.txt','timeout','--kill-after=10s','600s',
         'mpirun','-np','4','--bind-to','none',str(BINARY),'run.cfg']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(BUILD/'compile-tmp'))
start=time.monotonic()
with (CASE/'run.log').open('w') as log:code=subprocess.run(command,cwd=str(CASE),env=env,stdout=log,stderr=subprocess.STDOUT).returncode
text=(CASE/'run.log').read_text();normal=re.findall(r'^Native3D complexity normalization: (.*)$',text,re.M)
correction=re.findall(r'^Native3D support correction: (.*)$',text,re.M)
saved={'exit_code':code,'seconds':time.monotonic()-start,'command':command,'provenance_sha256':sha(p),'log_sha256':sha(CASE/'run.log'),
       'normalization':normal,'support_correction':correction,'construction_attempted':False,'accepted_mesh':False,'CFD_steps_requested':1}
result.write_text(json.dumps(saved,indent=2)+'\n');print(json.dumps(saved),flush=True)
assert code==0 and normal and correction,'Actual metric initialization did not complete; preserve failure evidence.'
assert 'Native3D bounded complexity switch:' in text
assert 'proven_error_bound=0' in text
