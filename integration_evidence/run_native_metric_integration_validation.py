from pathlib import Path
import json,os,subprocess,sys,time
root=Path(__file__).resolve().parent.parent;os.chdir(root);e=root/'integration_evidence';d=e/'native_metric_integration_v4';state=d/'campaign_frozen.json';assert not state.exists()
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
record={'controller_pid':os.getpid(),'sequential':True,'stages':[]}
def save():state.write_text(json.dumps(record,indent=2)+'\n')
def run(label,args):
 start=time.monotonic();record.update(phase=label,command=args);save()
 with (d/(label+'.controller.log')).open('x') as log:
  child=subprocess.Popen([sys.executable,*args],env=env,stdout=log,stderr=subprocess.STDOUT);record['child_pid']=child.pid;save();code=child.wait()
 record.pop('child_pid',None);record['stages'].append({'name':label,'exit_code':code,'elapsed_seconds':time.monotonic()-start});save();print(label,'exit',code,flush=True)
 if code:record['phase']='terminal_failed';save();raise SystemExit(code)
prepared=json.loads((d/'prepared_cases.json').read_text())
for row in prepared['frozen_cases']:
 if row['seed']=='fine':continue # Immutable successful runs, same final production binary.
 wd=Path(row['folder']);assert (wd/'input.su2').is_file()
 run('verified_frozen_'+row['seed']+'_'+row['method'].lower()+'_np'+str(row['ranks']),['integration_evidence/run_rae2822_trial.py',str(wd),'--ranks',str(row['ranks']),'--timeout','600','--build-evidence',str(e/'native_metric_integration_build_v4/evidence.json')])
record['phase']='terminal_pass';save()
