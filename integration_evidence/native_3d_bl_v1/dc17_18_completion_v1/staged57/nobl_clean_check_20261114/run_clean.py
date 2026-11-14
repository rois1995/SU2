"""One unprofiled frozen M6 run, gated on actual host quietness; /proc every 10s."""
import hashlib,json,os,shutil,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent; R=ROOT.parent; REPO=R.parents[3]
def save(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
def sha(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def processes():
    out={}
    for p in Path('/proc').iterdir():
        if not p.name.isdigit():continue
        try:
            v=(p/'stat').read_text().rsplit(')',1)[1].split()
            out[int(p.name)]={'ticks':int(v[11])+int(v[12]),'start_ticks':int(v[19]),'comm':(p/'comm').read_text().strip(),'command':(p/'cmdline').read_bytes().replace(b'\0',b' ').decode(errors='replace')[:500]}
        except (OSError,ValueError,IndexError):pass
    return out
assert not (ROOT/'run/status.json').exists(), 'Never launch this case twice'
previous=json.loads((R/'measurement_20261114_v2/final_host.json').read_text())
assert previous['actual_host_proc'] and previous['all_owned_run_handles_terminal']
load_override=(ROOT/'launch_override.json').exists()
last=processes(); tick=os.sysconf('SC_CLK_TCK'); prior=time.monotonic()
with (ROOT/'quiet_rechecks.jsonl').open('a') as receipt:
    while True:
        time.sleep(10);now=time.monotonic();current=processes();busy=[]
        for pid,v in current.items():
            if pid in last and v['start_ticks']==last[pid]['start_ticks']:
                cpu=100*(v['ticks']-last[pid]['ticks'])/tick/(now-prior)
                if cpu>5:busy.append({'pid':pid,'cpu_percent':cpu,**v})
        load=Path('/proc/loadavg').read_text().strip()
        quiet_load_pass=float(load.split()[0])<1
        cpu_pass=not any(v['cpu_percent']>50 for v in busy)
        gate=(quiet_load_pass or load_override) and cpu_pass
        # A separate native workload must not be overlapped even when temporarily idle.
        native=[{'pid':pid,**v} for pid,v in current.items() if v['comm'] in ('SU2_CFD','test_driver')]
        gate=gate and not native
        d={'unix':time.time(),'actual_host_proc':True,'loadavg':load,'sample_seconds':now-prior,'processes':busy,'other_native_processes':native,'quiet_load_pass':quiet_load_pass,'cpu_gate_pass':cpu_pass,'user_authorized_load_override':load_override,'gate_pass':gate}
        receipt.write(json.dumps(d)+'\n');receipt.flush();print(json.dumps({'waiting_for_quiet_host':not gate,'loadavg':load,'busy_over_50':[v['pid'] for v in busy if v['cpu_percent']>50]}),flush=True)
        if gate:save(ROOT/'launch_gate.json',d);break
        last=current;prior=now
case=ROOT/'run';case.mkdir();(case/'mpi_tmp').mkdir()
old=json.loads((R/'measurement_20261114_v2/run_B/provenance.json').read_text())
program=REPO/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD'
assert sha(program)=='0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6'
assert shutil.disk_usage(REPO).free>2_000_000_000
cfg=R/'m6_inputs/euler_adapted293_event1_v1/run.cfg';assert sha(cfg)==old['config_sha256'];shutil.copyfile(cfg,case/'run.cfg')
inputs={str(program):sha(program),str(case/'run.cfg'):sha(case/'run.cfg')}
for name in ('m6.su2','m6.su2.native_ref','baseline_euler_seed.dat'):
    src=(R/'m6_inputs/euler_original_force_seed_v1'/name).resolve();assert sha(src)==old['input_hashes'][str(src)]
    (case/name).symlink_to(src);inputs[str(src)]=sha(src)
env=os.environ.copy()
for key in list(env):
    if key.startswith('SU2_NATIVE_'):del env[key]
env.update(old['environment']);env['OMPI_MCA_orte_tmpdir_base']=str(case/'mpi_tmp')
# MPI output flags only preserve rank-separated logs and wall-clock emission times.
argv=['nice','-n','19','/usr/bin/time','-v','-o',str(case/'outer.time.txt'),'timeout','14400','mpirun','--bind-to','none','-n','6','--timestamp-output','--output-filename',str(case/'rank_output'),str(program),'run.cfg']
assert not any(x in argv for x in ('perf','gdb'))
save(case/'provenance.json',{'argv':argv,'logging_only_additions':['--timestamp-output','--output-filename'],'environment':{k:v for k,v in env.items() if k.startswith(('OMP_','OPENBLAS_','MKL_','OMPI_')) or k=='TMPDIR'},'input_hashes':inputs,'config_sha256':sha(case/'run.cfg'),'started_unix':time.time(),'free_space':shutil.disk_usage(REPO)._asdict(),'launch_gate':'../launch_gate.json','launch_override':'../launch_override.json' if load_override else None,'controller_sha256':sha(__file__)})
seen={};start=time.time()
with (case/'run.log').open('x') as log,(case/'host_resources.jsonl').open('x') as resource:
    proc=subprocess.Popen(argv,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    save(case/'status.json',{'state':'running','pid':proc.pid,'started_unix':start})
    print(json.dumps({'launched':str(case),'pid':proc.pid}),flush=True)
    while True:
        rows=[]
        for pid,v in processes().items():
            try:
                if (Path('/proc')/str(pid)/'cwd').resolve()!=case:continue
                seen[(pid,v['start_ticks'])]={'pid':pid,'start_ticks':v['start_ticks']}
                status=(Path('/proc')/str(pid)/'status').read_text()
                memory={k:int(value.split()[0]) for line in status.splitlines() if ':' in line for k,value in [line.split(':',1)] if k in ('VmRSS','VmHWM')}
                rows.append({'pid':pid,**v,**memory})
            except (OSError,ValueError):pass
        resource.write(json.dumps({'unix':time.time(),'elapsed':time.time()-start,'loadavg':Path('/proc/loadavg').read_text().strip(),'mem_available':next(l for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')),'processes':rows})+'\n');resource.flush()
        if proc.poll() is not None:break
        time.sleep(10)
    code=proc.wait()
current=processes();live=[v for (pid,stamp),v in seen.items() if pid in current and current[pid]['start_ticks']==stamp]
changed=[p for p,h in inputs.items() if sha(p)!=h]
result={'state':'terminal','exit_code':code,'seconds':time.time()-start,'inputs_unchanged':not changed,'changed':changed,'all_owned_handles_terminal':not live,'live':live,'identities':list(seen.values())}
save(case/'status.json',result);print(json.dumps(result),flush=True)
raise SystemExit(code or bool(changed) or bool(live))
