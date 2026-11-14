"""One unprofiled case, intentionally stopped after all six first-collapse records."""
import hashlib,json,os,re,shutil,signal,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent;R=ROOT.parent;REPO=R.parents[3];CASE=ROOT/'run'
sys.path.insert(0,str(R/'nobl_clean_check_20261114'));from extract_markers import strip_mpi_prefixes
assert not (CASE/'status.json').exists() and not CASE.exists(),'Exactly one native launch'
def save(path,data):path.write_text(json.dumps(data,indent=2)+'\n')
def sha(path):
    with Path(path).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def processes():
    rows={}
    for p in Path('/proc').iterdir():
        if not p.name.isdigit():continue
        try:
            f=(p/'stat').read_text().rsplit(')',1)[1].split();rows[int(p.name)]={'pid':int(p.name),'state':f[0],'ppid':int(f[1]),'pgid':int(f[2]),'sid':int(f[3]),'start_ticks':int(f[19]),'comm':(p/'comm').read_text().strip()}
        except(OSError,ValueError,IndexError):pass
    return rows
def same(stamp,rows):
    r=rows.get(stamp['pid']);return bool(r and r['start_ticks']==stamp['start_ticks'] and r['state'] not in ('Z','X'))
def counters():
    out={}
    for f in CASE.glob('rank_output/*/rank.*/stdout'):
        for number,raw in enumerate(f.open(errors='replace'),1):
            if not raw.endswith('\n'):continue
            line=strip_mpi_prefixes(raw)
            if 'NATIVE3D_STAGE ' not in line or 'stage=length_collapse_exit ' not in line:continue
            row=dict(re.findall(r'(\w+)=([^\s]+)',line));required=['rank','elapsed','upper_work','upper_calls','upper_seconds','accepted','length_collapses']
            if not all(k in row for k in required):continue
            try:
                assert float(row['elapsed'])>0
                for k in ['rank','upper_work','upper_calls','accepted','length_collapses']:int(row[k])
            except(ValueError,AssertionError):continue
            row.update(source_file=str(f),source_line=number);out[int(row['rank'])]=row;break
    return out
build=json.loads((ROOT/'build_status.json').read_text());assert build['state']=='terminal' and build['exit_code']==0
plan=json.loads((ROOT/'build_plan.json').read_text());assert sha(plan['new_binary'])==build['binary_sha256']
assert sha(plan['source_path'])==plan['diagnostic_BP_sha256']
base=json.loads((R/'nobl_clean_check_20261114/run/provenance.json').read_text());existing=processes();assert not [r for r in existing.values() if r['comm'] in ('SU2_CFD','test_driver','cc1plus','lto1')]
launch_host={'actual_host_proc':True,'unix':time.time(),'loadavg':Path('/proc/loadavg').read_text().strip(),'MemAvailable':next(l for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')),'disk_free_bytes':shutil.disk_usage(ROOT).free};save(ROOT/'launch_host.json',launch_host);assert launch_host['disk_free_bytes']>2_000_000_000
CASE.mkdir();(CASE/'mpi_tmp').mkdir();cfg=R/'m6_inputs/euler_adapted293_event1_v1/run.cfg';assert sha(cfg)==base['config_sha256'];shutil.copyfile(cfg,CASE/'run.cfg')
inputs={str(CASE/'run.cfg'):sha(CASE/'run.cfg'),plan['new_binary']:sha(plan['new_binary']),plan['source_path']:sha(plan['source_path'])}
for name in ['m6.su2','m6.su2.native_ref','baseline_euler_seed.dat']:
    src=(R/'m6_inputs/euler_original_force_seed_v1'/name).resolve();assert sha(src)==base['input_hashes'][str(src)];(CASE/name).symlink_to(src);inputs[str(src)]=sha(src)
old_case=R/'nobl_clean_check_20261114/run';argv=[a.replace(str(old_case),str(CASE)) for a in base['argv']];assert argv.count(str(REPO/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD'))==1;argv=[plan['new_binary'] if a==str(REPO/'integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD') else a for a in argv]
env=os.environ.copy()
for key in list(env):
    if key.startswith('SU2_NATIVE_'):del env[key]
env.update(base['environment']);env['OMPI_MCA_orte_tmpdir_base']=str(CASE/'mpi_tmp')
assert 'perf' not in argv and 'gdb' not in argv and argv[-1]=='run.cfg'
save(CASE/'provenance.json',{'argv':argv,'environment':{k:v for k,v in env.items() if k in base['environment']},'input_hashes':inputs,'config_sha256':sha(CASE/'run.cfg'),'diagnostic_binary_sha256':build['binary_sha256'],'baseline_binary_sha256':plan['baseline_binary_sha256'],'difference_from_clean_command':'New output/temp paths and diagnostic binary only; intentional external stop after all first-collapse counters or1500s from launch','stop_deadline_seconds_since_launch':1500,'controller_sha256':sha(__file__)})
seen={};start=time.monotonic();started_unix=time.time();reason=None;captured={}
with (CASE/'run.log').open('x') as log,(CASE/'host_resources.jsonl').open('x') as receipts:
    proc=subprocess.Popen(argv,cwd=CASE,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True);root=processes()[proc.pid];seen[(root['pid'],root['start_ticks'])]=root
    save(CASE/'status.json',{'state':'running','started_unix':started_unix,'identity':root});print(json.dumps({'launched':root,'case':str(CASE)}),flush=True)
    while True:
        rows=processes();owned={pid for pid,r in rows.items() if (pid,r['start_ticks']) in seen};changed=True
        while changed:
            added={pid for pid,r in rows.items() if r['ppid'] in owned};changed=not added.issubset(owned);owned|=added
        resource=[]
        for pid in owned:
            r=rows[pid];seen[(pid,r['start_ticks'])]=r
            try:
                fields=(Path('/proc')/str(pid)/'status').read_text().splitlines();memory={k:int(v.split()[0]) for line in fields if ':' in line for k,v in [line.split(':',1)] if k in ['VmRSS','VmHWM']};resource.append({**r,**memory})
            except OSError:pass
        elapsed=time.monotonic()-start;receipts.write(json.dumps({'unix':time.time(),'elapsed':elapsed,'loadavg':Path('/proc/loadavg').read_text().strip(),'MemAvailable':next(l for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')),'owned_processes':resource})+'\n');receipts.flush();captured=counters()
        if set(captured)==set(range(6)):
            reason='all_six_first_collapse_records';save(CASE/'collapse_records.json',captured);break
        if proc.poll() is not None:reason='run_exited_before_complete_collapse_records';break
        if elapsed>=1500:reason='1500s_deadline_without_all_collapse_records';save(CASE/'partial_collapse_records.json',captured);break
        time.sleep(min(10,max(.05,1500-elapsed)))
    stop_elapsed=time.monotonic()-start
    if proc.poll() is None and same(root,processes()) and os.getpgid(proc.pid)==root['pgid']:
        os.killpg(root['pgid'],signal.SIGTERM)
    for attempt in range(12):
        current=processes();live=[r for r in seen.values() if same(r,current)]
        if not live:break
        if attempt in (3,8):
            for r in live:
                if same(r,processes()):
                    try:os.kill(r['pid'],signal.SIGTERM if attempt==3 else signal.SIGKILL)
                    except ProcessLookupError:pass
        time.sleep(1)
    code=proc.wait(timeout=10)
current=processes();live=[r for r in seen.values() if same(r,current)];altered=[f for f,h in inputs.items() if sha(f)!=h]
assert not live,live
result={'state':'terminal','exit_code':code,'intentional_stop':reason!='run_exited_before_complete_collapse_records','stop_reason':reason,'seconds_until_stop':stop_elapsed,'seconds_total':time.monotonic()-start,'captured_ranks':sorted(captured),'all_owned_handles_terminal':not live,'live':live,'identities':list(seen.values()),'inputs_unchanged':not altered,'changed':altered,'native_case_started':bool(captured) or 'Begin Solver' in (CASE/'run.log').read_text(errors='replace')};save(CASE/'status.json',result);print(json.dumps(result),flush=True)
raise SystemExit(bool(live) or bool(altered))
