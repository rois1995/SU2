from pathlib import Path
import json,hashlib,subprocess,os,time,signal,sys,shutil
r=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated');e=r/'integration_evidence';case=Path(sys.argv[1]).resolve();manifest=Path(sys.argv[2]);prior=json.loads(manifest.read_text());binary=Path(prior['archived_binary'])
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
assert sha(binary)==prior['binary_sha256']
recovered=[]
for name,digest in prior['source_sha256'].items():
 if sha(r/name)==digest:continue
 found=False
 for archive in ('native_frozen_rae_euler_to_bl_v1','native_unsteady_cache_mpi_v3','native_unsteady_mpi_v3','native_pruning_cost_core_mpi_v2','native_donor_index_core_mpi_v1'):
  p=e/archive/'sources'/name
  if p.is_file() and sha(p)==digest:found=True;recovered.append(str(p));break
 assert found,('Cannot verify original source pin',name)
case.mkdir(parents=True,exist_ok=False);fixture=Path(sys.argv[3]).resolve();shutil.copy2(fixture/'run.cfg',case/'run.cfg');shutil.copy2(fixture/'input.su2',case/'input.su2');shutil.copy2(fixture/'frozen_sensor.csv',case/'frozen_sensor.csv')
record=dict(scope='Frozen original sensor and actual geometric BL/native backend MPI4; contention-aware immutable executable control, setup/gather outside the remeshing timer',binary=str(binary),binary_sha256=prior['binary_sha256'],source_evidence=str(manifest.resolve()),source_pins_checked=len(prior['source_sha256']),recovered_original_sources=recovered,phase='preparing',working_directory=str(case),inputs_sha256={p.name:sha(p) for p in (case/'run.cfg',case/'input.su2')})
report=case/'run_evidence.json';assert not report.exists();shutil.copy2(__file__,case/'runner_source.py')
def save():report.write_text(json.dumps(record,indent=2)+'\n')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',SU2_NATIVE_AIRFOIL_CONFIG=str(case/'run.cfg'),SU2_NATIVE_FROZEN_METRIC=str(case/'frozen_sensor.csv'));record['inputs_sha256']['frozen_sensor.csv']=sha(case/'frozen_sensor.csv');quiet=None
record['machine_samples']=[]
def cpu_ticks():
 a=list(map(int,Path('/proc/stat').read_text().splitlines()[0].split()[1:9]));return sum(a),a[3]+a[4]
def machine_sample(previous):
 now=cpu_ticks();total=now[0]-previous[0];idle=now[1]-previous[1]
 psi=Path('/proc/pressure/cpu').read_text();avg10=float(psi.split('avg10=',1)[1].split()[0])
 sample=dict(monotonic=time.monotonic(),busy_fraction=(1-idle/total if total else 0),cpu_pressure=psi.strip(),cpu_pressure_avg10=avg10,loadavg=Path('/proc/loadavg').read_text().strip())
 record['machine_samples'].append(sample);return now,sample
previous=cpu_ticks();time.sleep(5)
while True:
 previous,sample=machine_sample(previous)
 busy=[s for s in subprocess.check_output(['ps','-eo','pid,stat,comm'],text=True).splitlines()[1:] if len(s.split())==3 and 'Z' not in s.split()[1] and (s.split()[2] in ('ninja','cc1plus','test_driver') or s.split()[2].startswith('SU2_CFD'))]
 record.update(phase='waiting_for_machine',busy=busy);save()
 if busy or sample['busy_fraction']>(os.cpu_count()-5)/os.cpu_count() or sample['cpu_pressure_avg10']>10:quiet=None
 elif quiet is None:quiet=time.monotonic()
 elif time.monotonic()-quiet>=15:break
 time.sleep(5)
command=['mpiexec','-n','4',str(binary),'[NativeFrozenAirfoil2D]','--use-colour','no']
start=time.monotonic();snapshot=[];previous=cpu_ticks();sample_started=start
with (case/'solver.log').open('x') as log:
 child=subprocess.Popen(command,cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True);record.update(phase='running',command=command,child_pid=child.pid);save()
 while True:
  try:code=child.wait(timeout=2);break
  except subprocess.TimeoutExpired:
   if not snapshot:
    for line in subprocess.check_output(['ps','-eo','pid,comm'],text=True).splitlines()[1:]:
     pid,name=line.split(maxsplit=1)
     if name!='test_driver':continue
     p=Path('/proc')/pid
     try:
      if (p/'cwd').resolve()==case:snapshot.append(dict(pid=int(pid),status=[s for s in (p/'status').read_text().splitlines() if s.startswith(('Cpus_allowed_list:','Threads:','VmHWM:'))]))
     except OSError:pass
    record['rank_affinity_snapshot']=snapshot
   if time.monotonic()-sample_started>=5:
    previous,sample=machine_sample(previous);sample_started=time.monotonic()
    record['machine_samples'][-1]['elapsed_seconds']=time.monotonic()-start
   record['elapsed_seconds']=time.monotonic()-start;save()
   if record['elapsed_seconds']>300:
    os.killpg(child.pid,signal.SIGTERM)
    try:child.wait(timeout=10)
    except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
    code=124;break
record.update(phase='terminal',solver_exit=code,elapsed_seconds=time.monotonic()-start);record.pop('child_pid',None);save();assert code==0 and (case/'solver.log').read_text().count('All tests passed')==4;print(case.name,record['elapsed_seconds'],flush=True)
