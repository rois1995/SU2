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
 print('REUSE_M6_METRIC_RUNTIME',json.dumps(saved),flush=True)
else:
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
text=(CASE/'run.log').read_text()
code=saved['exit_code'];normal=saved['normalization'];correction=saved['support_correction']
assert code==0 and normal and correction,'Actual metric initialization did not complete; preserve failure evidence.'
assert 'Native3D bounded complexity strategy: nonplanar faceted wall geometry.' in text
assert 'proven_error_bound=0' in text

baseline=json.loads((BASE.parent/'production_complexity_v1/m6_runtime_metric/result.json').read_text())
def numbers(line):return {k:v for k,v in re.findall(r'(\w+)=([^ ;]+)',line)}
a=numbers(baseline['normalization'][0]);b=numbers(saved['normalization'][0])
assert b['status']==a['status']=='outside_estimated_bounds' and int(b['trials'])==1
for k in ('estimate','uncertainty_indicator'):
 assert abs(float(b[k])/float(a[k])-1)<2e-12,(k,a[k],b[k])
assert '0 strict density evaluations.' in text
previous=BASE.parent/'production_complexity_v1/m6_runtime_metric/metric_check_flow.dat'
assert sha(CASE/'metric_check_flow.dat')==sha(previous),'CFD state changed in a metric-only implementation change.'
print('PASS_IDENTICAL_ENDPOINT_AND_CFD_STATE_WITH_SINGLE_TRIAL',flush=True)
def work(log):
 out={}
 for stage in ('metric','metric_build'):
  rows=[numbers(line) for line in log.splitlines() if line.startswith('NATIVE3D_STAGE stage='+stage+' ')]
  assert len(rows)==4
  out[stage]={'maximum_rank_seconds':max(float(row['total']) for row in rows),
              'sum_rank_CPU_seconds':sum(float(row['cpu']) for row in rows),
              'maximum_process_lifetime_peak_bytes':max(int(row['process_peak_rss']) for row in rows)}
 m=re.search(r'Native metric work: (\d+) complexity trials, (\d+) total gradation sweeps, (\d+) owned-point visits;',log)
 assert m
 out.update(complexity_trials=int(m[1]),gradation_sweeps=int(m[2]),owned_point_visits=int(m[3]))
 out['support']={k:float(v) for k,v in numbers(re.search(r'^Native3D support correction: (.*)$',log,re.M)[1]).items()}
 return out
before=work((BASE.parent/'production_complexity_v1/m6_runtime_metric/run.log').read_text());after=work(text)
verification={'status':'PASS_IDENTICAL_ENDPOINT_AND_CFD_STATE_WITH_SINGLE_TRIAL','runtime_result_sha256':sha(result),
 'baseline_result_sha256':sha(BASE.parent/'production_complexity_v1/m6_runtime_metric/result.json'),
 'flow_sha256':sha(CASE/'metric_check_flow.dat'),'helper_sha256':sha(Path(__file__).resolve()),
 'baseline':before,'candidate':after,
 'metric_elapsed_ratio':before['metric']['maximum_rank_seconds']/after['metric']['maximum_rank_seconds'],
 'metric_CPU_ratio':before['metric']['sum_rank_CPU_seconds']/after['metric']['sum_rank_CPU_seconds'],
 'limits':['Contended local initialization comparison, not full remeshing or scaling.',
           'Maximum process lifetime RSS is not a simultaneous aggregate memory peak.',
           'Outside-budget estimate remains heuristic; no accepted adapted mesh.']}
(CASE/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
