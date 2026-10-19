#!/usr/bin/env python3
"""One pinned low-priority connected build; no test execution or source mutation."""
import gzip,hashlib,json,os,resource,signal,subprocess,time,shutil
from pathlib import Path
ROOT=Path.cwd();HERE=ROOT/'integration_evidence/native_3d_bl_v1/joint_affine_source_v1';BUILD=ROOT/'integration_evidence/build-integrated-v2'
files=json.loads((HERE/'source_v1.json').read_text())['sha256'];files=list(files)+['Common/src/adaptation/CNativeRemesher3D.cpp']
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  while b:=f.read(4*1024**2):h.update(b)
 return h.hexdigest()
pins={p:sha(ROOT/p) for p in files};(HERE/'source_v2.json.gz').write_bytes(gzip.compress(json.dumps({p:(ROOT/p).read_text() for p in files}).encode()))
(HERE/'source_v2.json').write_text(json.dumps(dict(base=subprocess.check_output(['git','rev-parse','HEAD']).decode().strip(),sha256=pins),indent=2)+'\n')
old=BUILD/'UnitTests/test_driver';archive=ROOT/'integration_evidence/native_3d_bl_v1/affine_initial_adoption_v1/test_driver_prototype_v2.gz';expected='957e2bab5df11db23e622d67aa5f9519ead09e40e52a2130f41a941a877c28a8'
assert sha(old)==expected
assert Path('/home/rausa/Software/su2-develop/ninja').is_file()
if not archive.exists():
 with old.open('rb') as source,archive.open('xb') as raw:
  with gzip.GzipFile(filename='',mode='wb',fileobj=raw,compresslevel=1,mtime=0) as dest:
   while b:=source.read(4*1024**2):dest.write(b)
prior=json.loads((HERE/'prior_binary_archive.json').read_text())
assert prior['sha256']==expected and prior['original_bytes']==old.stat().st_size
assert prior['compressed_sha256']==sha(archive)
print('REUSED_VERIFIED_PRIOR_ARCHIVE',flush=True)
def cpus():
 return {int(s[3:]):list(map(int,v)) for s,*v in (line.split() for line in Path('/proc/stat').read_text().splitlines()) if s.startswith('cpu') and s[3:].isdigit()}
a=cpus();time.sleep(.25);b=cpus();allowed=os.sched_getaffinity(0)
def busy(cpu):
 delta=[y-x for x,y in zip(a[cpu],b[cpu])];total=sum(delta[:8]);return 1-(sum(delta[3:5])/total) if total else 0
cpu=min(allowed,key=busy);command=['taskset','-c',str(cpu),'nice','-n','19','/home/rausa/Software/su2-develop/ninja','-C',str(BUILD),'-j1','UnitTests/test_driver']
env=os.environ.copy();env.update(TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',NUMEXPR_NUM_THREADS='1',MALLOC_ARENA_MAX='2')
started=time.time();log=(HERE/'build.log').open('w');process=subprocess.Popen(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
receipt=dict(status='RUNNING',pid=process.pid,command=command,source_sha256=pins,started_unix=started,compiler_tmp=env['TMPDIR'],threads=1,max_seconds=5400)
(HERE/'build_process.json').write_text(json.dumps(receipt,indent=2)+'\n');print('BUILD_PID',process.pid,'CPU',cpu,flush=True)
def children(pid):
 try:return [int(v) for v in Path('/proc/%d/task/%d/children'%(pid,pid)).read_text().split()]
 except OSError:return []
maximum_rss=0;minimum_available=2**64;reason=None
with (HERE/'build_resources.jsonl').open('w') as samples:
 while process.poll() is None:
  pending=[process.pid];seen=set();rss=0
  while pending:
   pid=pending.pop()
   if pid in seen:continue
   seen.add(pid);pending.extend(children(pid))
   try:
    values={line.split(':',1)[0]:line.split(':',1)[1].strip() for line in Path('/proc/%d/status'%pid).read_text().splitlines() if ':' in line}
    rss+=int(values.get('VmRSS','0').split()[0])
   except (OSError,ValueError):pass
  mem={line.split(':')[0]:int(line.split()[1]) for line in Path('/proc/meminfo').read_text().splitlines()}
  maximum_rss=max(maximum_rss,rss);minimum_available=min(minimum_available,mem['MemAvailable'])
  samples.write(json.dumps(dict(elapsed_seconds=time.time()-started,descendant_rss_sum_KiB=rss,mem_available_KiB=mem['MemAvailable'],processes=len(seen)))+'\n');samples.flush()
  if time.time()-started>5400:reason='Build elapsed ceiling'
  if mem['MemAvailable']<8*1024**2:reason='Host available-memory reserve8GiB'
  if reason:os.killpg(process.pid,signal.SIGTERM);process.wait();break
  time.sleep(15)
code=process.wait();log.close();same=all(sha(ROOT/p)==h for p,h in pins.items());assert same
receipt.update(status='PASS' if code==0 else 'FAILED',exit_code=code,resource_stop=reason,elapsed_seconds=time.time()-started,source_unchanged=same,
 maximum_sampled_descendant_rss_sum_KiB=maximum_rss,minimum_sampled_host_MemAvailable_KiB=minimum_available,
 memory_scope='15s sampled descendant RSS sum, not PSS or guaranteed peak; host available-memory reserve observed.',
 binary_sha256=sha(old) if code==0 else None,binary_bytes=old.stat().st_size if code==0 else None,compile_commands_sha256=sha(BUILD/'compile_commands.json'))
(HERE/'build_process.json').write_text(json.dumps(receipt,indent=2)+'\n');print('BUILD_TERMINAL',code,receipt['elapsed_seconds'],flush=True)
raise SystemExit(code)
