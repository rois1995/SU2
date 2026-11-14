"""Run exactly one requested case under external per-rank profiling; no native/config edits."""
import hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent;R=ROOT.parent;REPO=R.parents[3]
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
def ident(pid):
    try:return dict(pid=pid,start_ticks=int(Path('/proc',str(pid),'stat').read_text().rsplit(')',1)[1].split()[19]))
    except FileNotFoundError:return None
mode=sys.argv[1];assert mode in ('A','B');case=ROOT/('run_'+mode);assert not (case/'status.json').exists()
CFD=REPO/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD';TEST=REPO/'integration_evidence/build-integrated-v3-currenthost/UnitTests/test_driver'
assert sha(CFD)=='0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6'
assert sha(TEST)=='e6c25520db86bdd212b500249420a7cf3ceddf4af5c17c40e02b1b0b743cd365'
assert shutil.disk_usage(REPO).free>2_000_000_000
if mode=='B':
    prior=json.loads((ROOT/'run_A/status.json').read_text());assert prior['all_owned_handles_terminal'] and not prior['profiler_failed'] and prior['native_started']
env=os.environ.copy()
for key in list(env):
    if key.startswith('SU2_NATIVE_'):del env[key]
env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_osc='pt2pt',TMPDIR=str(REPO/'integration_evidence/build-integrated-v2/compile-tmp'))
mpi_tmp=case/'mpi_tmp';mpi_tmp.mkdir();env['OMPI_MCA_orte_tmpdir_base']=str(mpi_tmp)
inputs={};source_case=None
if mode=='A':
    source_case=R/'cases/curved492_saved_production_surface_serial_20261113T163756_3724702'
    old=json.loads((source_case/'command.json').read_text());env.update(old['environment'])
    env['OMPI_MCA_orte_tmpdir_base']=str(mpi_tmp);env['SU2_NATIVE_FROZEN_CONFIG']=str(case/'run.cfg')
    for name in ('m6.su2','m6.su2.native_ref','m6.su2.native_rows','baseline_sa_seed.dat'):
        src=(source_case/name).resolve();(case/name).symlink_to(src);inputs[str(src)]=sha(src)
    shutil.copyfile(source_case/'run.cfg',case/'run.cfg');assert sha(case/'run.cfg')==sha(source_case/'run.cfg')
    for key,value in env.items():
        if key.startswith('SU2_NATIVE_'):
            p=Path(value)
            if p.is_file():inputs[str(p)]=sha(p)
    state=json.loads((R/'saved_surface485_checkpoint.json').read_text())
    for key in ('owned','parameters','prepared_reference'):
        item=state[key];assert sha(item['path'])==item['sha256'];inputs[item['path']]=item['sha256']
    ranks=1;timeout='600';program=TEST;args=['[NativeActualSavedSurfaceStage3D]','--durations','yes']
else:
    source_case=R/'m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k'
    cfg=R/'m6_inputs/euler_adapted293_event1_v1/run.cfg';assert sha(cfg)==sha(source_case/'run.cfg')
    shutil.copyfile(cfg,case/'run.cfg');seed=R/'m6_inputs/euler_original_force_seed_v1'
    qualification=json.loads((R/'m6_inputs/euler_adapted293_event1_v1/seed_input.json').read_text())['qualification']['output_sha256']
    for name in ('m6.su2','m6.su2.native_ref','baseline_euler_seed.dat'):
        src=(seed/name).resolve();assert sha(src)==qualification[name];(case/name).symlink_to(src);inputs[str(src)]=sha(src)
    ranks=6;timeout='14400';program=CFD;args=['run.cfg']
inputs[str(case/'run.cfg')]=sha(case/'run.cfg');inputs[str(CFD)]=sha(CFD);inputs[str(TEST)]=sha(TEST)
for p in (ROOT/'rank_perf.py',Path(__file__).resolve(),ROOT/'brief.md'):inputs[str(p)]=sha(p)
argv=['nice','-n','19','/usr/bin/time','-v','-o',str(case/'outer.time.txt'),'timeout',timeout]
if ranks>1:argv+=['mpirun','--bind-to','none','-n',str(ranks)]
argv+=[sys.executable,str(ROOT/'rank_perf.py'),str(program),*args]
receipt=dict(case=mode,ranks=ranks,source_case=str(source_case),argv=argv,environment={k:v for k,v in env.items() if k.startswith(('SU2_NATIVE_','OMP_','OPENBLAS_','MKL_','OMPI_')) or k=='TMPDIR'},config_sha256=sha(case/'run.cfg'),input_hashes=inputs,free_space=shutil.disk_usage(REPO)._asdict(),meminfo=Path('/proc/meminfo').read_text(),loadavg=Path('/proc/loadavg').read_text(),wrapper=ident(os.getpid()),started_monotonic_ns=time.monotonic_ns(),started_unix=time.time())
save(case/'provenance.json',receipt);seen={};started=time.time()
with (case/'launcher.log').open('x') as log,(case/'host_resources.jsonl').open('x') as resources:
    proc=subprocess.Popen(argv,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    save(case/'status.json',dict(state='running',pid=proc.pid,identity=ident(proc.pid),started_unix=started))
    print(json.dumps(dict(case=str(case),pid=proc.pid)),flush=True)
    while proc.poll() is None:
        rows=[]
        for p in Path('/proc').iterdir():
            if not p.name.isdigit():continue
            try:
                cwd=(p/'cwd').resolve();cmd=(p/'cmdline').read_bytes().replace(b'\0',b' ').decode(errors='replace')
                if cwd!=case and str(case) not in cmd:continue
                stamp=ident(int(p.name))
                if not stamp:continue
                seen[(stamp['pid'],stamp['start_ticks'])]=stamp
                status=(p/'status').read_text();rss=next((int(x.split()[1])*1024 for x in status.splitlines() if x.startswith('VmRSS:')),0)
                rows.append(dict(**stamp,rss_bytes=rss,command=cmd[:300]))
            except (FileNotFoundError,ProcessLookupError,PermissionError):pass
        resources.write(json.dumps(dict(elapsed=time.time()-started,monotonic_ns=time.monotonic_ns(),loadavg=Path('/proc/loadavg').read_text(),processes=rows))+'\n');resources.flush();time.sleep(1)
    code=proc.wait()
profiles=list((case/'profile').glob('rank*.status.json'));native_started=bool(list((case/'profile').glob('rank*.exec.json')))
profiler_failed=len(profiles)!=ranks or any(json.loads(p.read_text())['record_exit'] or json.loads(p.read_text())['script_exit'] or json.loads(p.read_text())['failures'] for p in profiles)
changed=[p for p,h in inputs.items() if sha(p)!=h];live=[]
for stamp in seen.values():
    if stamp['pid']==os.getpid():continue
    if ident(stamp['pid'])==stamp:live.append(stamp)
result=dict(state='terminal',exit_code=code,seconds=time.time()-started,profiler_failed=bool(profiler_failed),native_started=native_started,inputs_unchanged=not changed,changed=changed,all_owned_handles_terminal=not live,live=live,identities=list(seen.values()),wrapper_excluded=ident(os.getpid()))
save(case/'status.json',result);print(json.dumps(result),flush=True)
raise SystemExit(code or profiler_failed or bool(changed) or bool(live))
