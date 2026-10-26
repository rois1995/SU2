"""Run the saved-state serial experiment with immutable inputs; no new MPI prefix."""
import json,os,subprocess,time,sys,signal,re,resource
from pathlib import Path
from run import ROOT,REPO,PREVIOUS,sha,save

def main():
    name=sys.argv[1] if len(sys.argv)>1 else 'offline';assert re.fullmatch(r'offline[0-9_]*',name)
    folder=ROOT/name;assert not (folder/'status.json').exists(),'Inspect terminal status before retrying'
    folder.mkdir(exist_ok=True)
    inputs=ROOT/'replay_input';pins={str(inputs/name):h for name,h in json.loads((inputs/'pins.json').read_text()).items()}
    pins.update(json.loads((ROOT/'build_source_pins.json').read_text()))
    pins.update(json.loads((ROOT.parent/'validation54/m6_euler/loaded_runtime_pins.json').read_text()))
    for name,h in pins.items():assert sha(name)==h,name
    binary=REPO/'integration_evidence/build-integrated-v2/UnitTests/test_driver'
    authority=PREVIOUS/'observed/m6.su2.native_ref'
    assert sha(authority)==json.loads((PREVIOUS/'input_manifest.json').read_text())['reference_sha256']
    for p in [binary,authority,Path(__file__)]:pins[str(p)]=sha(p)
    save(folder/'pins.json',pins)
    env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
        SU2_NATIVE_TRANSACTION_REPLAY=str(inputs),SU2_NATIVE_TRANSACTION_REFERENCE=str(authority),SU2_NATIVE_TRANSACTION_HAUSD='0.0001')
    command=['nice','-n','19','/usr/bin/time','-v','timeout','90',str(binary),'[NativeTransactionReplay3D]']
    save(folder/'command.json',dict(argv=command,environment={k:v for k,v in env.items() if k.startswith(('OMP_','OPENBLAS_','MKL_','SU2_NATIVE_'))}))
    start=time.time()
    with (folder/'run.log').open('w') as log:
        p=subprocess.Popen(command,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True,preexec_fn=lambda:resource.setrlimit(resource.RLIMIT_CPU,(60,61)))
        save(folder/'status.json',dict(state='running',pid=p.pid,start=start));rss_cap=False;time_cap=False;largest_sample=0;repair_started=None
        while p.poll() is None:
            pending=[p.pid];seen=set()
            while pending:
                pid=pending.pop()
                if pid in seen:continue
                seen.add(pid)
                try:
                    proc=Path('/proc')/str(pid)
                    pending.extend(int(x) for x in (proc/'task'/str(pid)/'children').read_text().split())
                    rss=int((proc/'statm').read_text().split()[1])*os.sysconf('SC_PAGE_SIZE')
                    largest_sample=max(largest_sample,rss)
                    if rss>1024**3:rss_cap=True
                except (FileNotFoundError,ProcessLookupError):pass
            if repair_started is None and 'TRANSACTION_REPAIR_BEGIN' in (folder/'run.log').read_text():repair_started=time.monotonic()
            if repair_started is not None and time.monotonic()-repair_started>60:time_cap=True
            if rss_cap or time_cap:
                os.killpg(p.pid,signal.SIGTERM);break
            time.sleep(.25)
        code=p.wait()
    changed=[name for name,h in pins.items() if sha(name)!=h]
    result=dict(state='terminal',exit_code=code,seconds=time.time()-start,pins_verified=not changed,changed=changed,rss_cap_reached=rss_cap,branch_wall_cap_reached=time_cap,whole_process_cpu_limit_seconds=60,largest_sampled_process_rss=largest_sample)
    save(folder/'status.json',result);print(json.dumps(result));assert not changed,changed
    raise SystemExit(code or int(rss_cap or time_cap))

if __name__=='__main__':main()
