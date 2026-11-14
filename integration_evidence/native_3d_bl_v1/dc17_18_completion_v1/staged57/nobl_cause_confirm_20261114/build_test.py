"""Execute the one prepared diagnostic build, without writing the baseline build."""
import hashlib,json,os,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent;plan=json.loads((ROOT/'build_plan.json').read_text());assert not (ROOT/'build_status.json').exists()
def save(name,data):(ROOT/name).write_text(json.dumps(data,indent=2)+'\n')
def sha(path):
    with Path(path).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def ident(pid):
    try:return {'pid':pid,'start_ticks':int((Path('/proc')/str(pid)/'stat').read_text().rsplit(')',1)[1].split()[19])}
    except OSError:return None
assert sha(plan['source_path'])==plan['diagnostic_BP_sha256']
env=os.environ.copy();tmp=ROOT/'compile_tmp';tmp.mkdir();env.update(TMPDIR=str(tmp),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
start=time.time();results=[];save('build_status.json',{'state':'running','controller':ident(os.getpid()),'started_unix':start})
with (ROOT/'build_resources.jsonl').open('x') as resource:
    for label in ['compile','archive','link']:
        argv=['nice','-n','19',*plan[label+'_argv']]
        with (ROOT/(label+'.log')).open('x') as log:
            proc=subprocess.Popen(argv,cwd=plan['working_directory'],env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True);stamp=ident(proc.pid);begin=time.time()
            print(json.dumps({'build_stage':label,'identity':stamp}),flush=True)
            while True:
                rows=[]
                for p in Path('/proc').iterdir():
                    if not p.name.isdigit():continue
                    try:
                        if (p/'cwd').resolve()!=Path(plan['working_directory']):continue
                        fields=(p/'status').read_text().splitlines();memory={k:int(v.split()[0]) for line in fields if ':' in line for k,v in [line.split(':',1)] if k in ['VmRSS','VmHWM']};rows.append({**ident(int(p.name)),**memory,'comm':(p/'comm').read_text().strip()})
                    except(OSError,TypeError):pass
                resource.write(json.dumps({'unix':time.time(),'stage':label,'loadavg':Path('/proc/loadavg').read_text().strip(),'MemAvailable':next(l for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')),'processes':rows})+'\n');resource.flush()
                if proc.poll() is not None:break
                time.sleep(10)
            code=proc.wait();results.append({'stage':label,'exit_code':code,'seconds':time.time()-begin,'identity':stamp,'handle_terminal':ident(proc.pid)!=stamp})
        if code:
            save('build_status.json',{'state':'terminal','exit_code':code,'stages':results,'seconds':time.time()-start,'native_run_started':False});raise SystemExit(code)
frozen=json.loads((ROOT/'baseline_build_inputs.json').read_text())['files'];changed=[f['path'] for f in frozen if sha(f['path'])!=f['sha256'] or Path(f['path']).stat().st_mtime_ns!=f['mtime_ns']]
assert not changed,changed
pins=json.loads((ROOT.parent/'build494_source_pins.json').read_text());assert all(sha(p)==h for p,h in pins.items());assert sha(plan['source_path'])==plan['diagnostic_BP_sha256']
result={'state':'terminal','exit_code':0,'stages':results,'seconds':time.time()-start,'binary':plan['new_binary'],'binary_sha256':sha(plan['new_binary']),'baseline_build_inputs_and_metadata_unchanged':True,'baseline_frozen_files':len(frozen),'baseline_source_pins_unchanged':len(pins),'diagnostic_source_unchanged_during_build':True};save('build_status.json',result);print(json.dumps(result),flush=True)
