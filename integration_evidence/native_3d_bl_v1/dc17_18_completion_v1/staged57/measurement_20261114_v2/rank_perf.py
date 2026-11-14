"""External per-rank perf recorder; native program/configuration are unchanged."""
import hashlib,json,os,re,signal,subprocess,sys,threading,time
from pathlib import Path
HERE=Path(__file__).resolve().parent

def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
def identity(pid):
    try:
        fields=Path('/proc',str(pid),'stat').read_text().rsplit(')',1)[1].split()
        return dict(pid=pid,start_ticks=int(fields[19]))
    except FileNotFoundError:return None

def stop(proc,stamp):
    if proc and proc.poll() is None and identity(proc.pid)==stamp:
        os.killpg(proc.pid,signal.SIGTERM)

if sys.argv[1]=='--exec':
    log=Path(sys.argv[2]);program=sys.argv[3];rank=log.name.split('.')[0]
    save(log.parent/(rank+'.exec.json'),dict(**identity(os.getpid()),monotonic_ns=time.monotonic_ns(),wall_ns=time.time_ns(),program=program))
    fd=os.open(log,os.O_WRONLY|os.O_CREAT|os.O_APPEND,0o600);os.dup2(fd,1);os.dup2(fd,2);os.close(fd)
    if Path(program).name=='test_driver':
        os.execv('/usr/bin/gdb',['gdb','--batch','-nx','-x',str(HERE/'engine_clock.gdb'),'--args',program,*sys.argv[4:]])
    os.execv(program,[program,*sys.argv[4:]])

program=str(Path(sys.argv[1]).resolve());args=sys.argv[2:];rank=int(os.environ.get('OMPI_COMM_WORLD_RANK','0'))
profile=Path.cwd()/'profile';profile.mkdir(exist_ok=True);stem=f'rank{rank}'
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
expected={'SU2_CFD':'0311b5a2a4f871bd82f48cd7cef3eac49a932cd1d90012bbc7e3c0d887e74ee6','test_driver':'e6c25520db86bdd212b500249420a7cf3ceddf4af5c17c40e02b1b0b743cd365'}
assert sha(program)==expected[Path(program).name]
cmd=['/usr/bin/perf','record','--clockid','mono','-e','cycles:u','-F','99','-o','-','--','/usr/bin/time','-v','-o',str(profile/(stem+'.time.txt')),sys.executable,str(Path(__file__).resolve()),'--exec',str(profile/(stem+'.program.log')),program,*args]
parser_cmd=['/usr/bin/perf','script','-i','-','--ns','-F','pid,tid,time,ip,sym,dso']
record=parser=None;stamp=None;failure=[];lbr=None;lbr_stamp=None;lbr_phase=None;finished_lbr=set()

def interrupted(sig,frame):
    stop(lbr,lbr_stamp);stop(record,stamp)
for sig in (signal.SIGTERM,signal.SIGINT):signal.signal(sig,interrupted)
start=time.monotonic();own=identity(os.getpid())
with (profile/(stem+'.perf.stderr')).open('wb') as error,(profile/(stem+'.script.stderr')).open('wb') as parser_error,(profile/(stem+'.samples.txt')).open('wb') as samples,(profile/(stem+'.perf.data')).open('xb') as raw,(profile/(stem+'.resources.jsonl')).open('x') as resources:
    record=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=error,start_new_session=True);stamp=identity(record.pid)
    parser=subprocess.Popen(parser_cmd,stdin=subprocess.PIPE,stdout=samples,stderr=parser_error)
    save(profile/(stem+'.launch.json'),dict(rank=rank,wrapper=own,record=stamp,parser=identity(parser.pid),argv=cmd,parser_argv=parser_cmd,binary_sha256=sha(program),monotonic_ns=time.monotonic_ns(),wall_ns=time.time_ns(),sample_clock='CLOCK_MONOTONIC',profile_limit_total_bytes=2_000_000_000))
    def relay():
        try:
            while True:
                block=os.read(record.stdout.fileno(),65536)
                if not block:break
                raw.write(block);raw.flush();parser.stdin.write(block);parser.stdin.flush()
        except Exception as e:failure.append(str(e));stop(record,stamp)
        finally:
            try:parser.stdin.close()
            except BrokenPipeError:pass
    thread=threading.Thread(target=relay);thread.start();peak_rss=peak_hwm=0
    while record.poll() is None:
        now=time.monotonic_ns();entry=dict(monotonic_ns=now,elapsed_seconds=time.monotonic()-start)
        exe=profile/(stem+'.exec.json')
        if exe.exists():
            main=json.loads(exe.read_text())
            if identity(main['pid'])==dict(pid=main['pid'],start_ticks=main['start_ticks']):
                try:
                    text=Path('/proc',str(main['pid']),'status').read_text();values={k:int(v) for k,v in re.findall(r'^(VmRSS|VmHWM):\s+(\d+) kB',text,re.M)}
                    entry.update(main=main,**values);peak_rss=max(peak_rss,values.get('VmRSS',0));peak_hwm=max(peak_hwm,values.get('VmHWM',0))
                except FileNotFoundError:pass
            request=profile/(stem+'.lbr_request.json')
            if request.exists() and lbr is None:
                req=json.loads(request.read_text());phase=req['phase'];assert re.fullmatch('[A-Za-z0-9_-]{1,50}',phase)
                if phase not in finished_lbr and identity(main['pid'])==dict(pid=main['pid'],start_ticks=main['start_ticks']):
                    lbr_phase=phase;dcmd=['/usr/bin/perf','record','--clockid','mono','-e','cycles:u','-F','99','--call-graph','lbr','-p',str(main['pid']),'-o',str(profile/(stem+'.'+phase+'.lbr.data')),'--','/bin/sleep','60']
                    df=(profile/(stem+'.'+phase+'.lbr.stderr')).open('wb');lbr=subprocess.Popen(dcmd,stdout=df,stderr=df,start_new_session=True);lbr_stamp=identity(lbr.pid);save(profile/(stem+'.'+phase+'.lbr_command.json'),dict(argv=dcmd,attached=main,started_monotonic_ns=now))
        if lbr is not None and lbr.poll() is not None:
            code=lbr.wait();save(profile/(stem+'.'+lbr_phase+'.lbr_status.json'),dict(exit_code=code,finished_monotonic_ns=time.monotonic_ns()))
            if code:print('Optional LBR capture failed; native run continues',file=sys.stderr)
            finished_lbr.add(lbr_phase);lbr=None
        size=sum(p.stat().st_size for root in HERE.parent.glob('measurement_20261114_v*') for base in root.glob('run_*/profile') for p in base.iterdir() if p.is_file())
        entry['all_profile_bytes']=size;resources.write(json.dumps(entry)+'\n');resources.flush()
        if size>=1_900_000_000:failure.append('profiler data approaches2GB cap');stop(record,stamp)
        if parser.poll() is not None and record.poll() is None:failure.append('perf script ended prematurely');stop(record,stamp)
        time.sleep(.5)
    code=record.wait();thread.join();parser_code=parser.wait()
    if lbr is not None:
        dc=lbr.wait();save(profile/(stem+'.'+lbr_phase+'.lbr_status.json'),dict(exit_code=dc,finished_monotonic_ns=time.monotonic_ns()))
        if dc:print('Optional LBR capture failed; native run continues',file=sys.stderr)
    save(profile/(stem+'.status.json'),dict(rank=rank,record_exit=code,script_exit=parser_code,failures=failure,seconds=time.monotonic()-start,sampled_main_peak_rss_kib=peak_rss,sampled_main_peak_hwm_kib=peak_hwm,wrapper=own,record_terminal=record.poll() is not None,parser_terminal=parser.poll() is not None))
raise SystemExit(code or parser_code or bool(failure))
