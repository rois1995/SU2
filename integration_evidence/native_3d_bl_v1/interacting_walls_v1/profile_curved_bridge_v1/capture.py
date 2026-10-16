"""Sample only an already scheduled owned constructor; never launch or signal its solver."""
import gzip,hashlib,json,os,signal,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;CASE=HERE.parents[1]/'region_construction_v1/native_curved_rans_n4_m2_v13';ROOT=HERE.parents[3]
BINARY=ROOT/'integration_evidence/build-integrated-v2/UnitTests/test_driver';LOG=CASE/'native.log'
started=time.monotonic();report={'status':'WAITING_FOR_ALREADY_SCHEDULED_CASE','case':str(CASE),'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
def save(): (HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n')
def marker():
 if not LOG.exists():return None
 return next((line for line in LOG.read_text().splitlines() if line.startswith('NATIVE3D_DETAIL stage=refine rank=0 ')),None)
def process():
 rows=subprocess.run(['ps','-C','test_driver','-o','pid='],text=True,stdout=subprocess.PIPE).stdout.split()
 for value in rows:
  path=Path('/proc')/value
  try:
   if (path/'cwd').resolve()==CASE and (path/'exe').resolve()==BINARY:
    rank=next((v for v in (path/'environ').read_bytes().split(b'\0') if v.startswith(b'OMPI_COMM_WORLD_RANK=')),None)
    if rank==b'OMPI_COMM_WORLD_RANK=0':return int(value)
  except OSError:pass
 return None
save();pid=None
while time.monotonic()-started<600:
 if marker():report['status']='SKIPPED_CONSTRUCTION_ALREADY_TERMINAL';save();raise SystemExit(0)
 pid=process()
 if pid:break
 time.sleep(.2)
if not pid:report['status']='NO_TARGET_PROCESS_WITHIN_OBSERVATION_WINDOW';save();raise SystemExit(0)
stat=Path('/proc/'+str(pid)+'/stat').read_text();ticks=stat[stat.rfind(')')+2:].split()[19]
age=float(Path('/proc/uptime').read_text().split()[0])-int(ticks)/os.sysconf('SC_CLK_TCK')
command=['perf','record','-e','cpu-clock:u','-F','49','--call-graph','dwarf,4096','-p',str(pid),'-o',str(HERE/'perf.data')]
report.update(status='SAMPLING_OWNED_USERSPACE_CONSTRUCTION',pid=pid,process_start_ticks=ticks,attachment_age_seconds=age,record_command=command);save()
with (HERE/'perf.record.log').open('w') as stream:
 sampler=subprocess.Popen(command,stdout=stream,stderr=subprocess.STDOUT)
 try:
  while sampler.poll() is None and time.monotonic()-started<900:
   if marker() or not Path('/proc/'+str(pid)).exists():break
   time.sleep(.2)
 finally:
  if sampler.poll() is None:sampler.send_signal(signal.SIGINT)
  try:sampler.wait(timeout=15)
  except subprocess.TimeoutExpired:sampler.terminate();sampler.wait(timeout=15)
line=marker();report.update(native_marker=line,sampler_returncode=sampler.returncode,elapsed_seconds=time.monotonic()-started)
if sampler.returncode!=0 or not (HERE/'perf.data').exists():
 report['status']='SAMPLING_UNAVAILABLE';save();raise SystemExit(0)
with (HERE/'perf.report.txt').open('w') as stream:
 result=subprocess.run(['perf','report','-i',str(HERE/'perf.data'),'--stdio','--no-children','--sort=symbol','--percent-limit=0.5','--call-graph=none'],stdout=stream,stderr=subprocess.STDOUT)
report['report_returncode']=result.returncode
if result.returncode==0:
 with (HERE/'perf.data').open('rb') as source,gzip.open(str(HERE/'perf.data.gz'),'wb',compresslevel=1) as output:
  while True:
   block=source.read(1024*1024)
   if not block:break
   output.write(block)
 (HERE/'perf.data').unlink()
 report['status']='SAMPLED_UNTIL_FIRST_NATIVE_REFINEMENT_REPORT_DIAGNOSTIC' if line else 'PARTIAL_CPU_SAMPLES_DIAGNOSTIC'
else:report['status']='SAMPLES_SAVED_REPORT_FAILED'
receipt=CASE.with_suffix('.json')
if receipt.exists():report['test_binary_sha256']=json.loads(receipt.read_text()).get('contract',{}).get('pins',{}).get('test_binary')
report['scope']='Rank-zero user-space CPU samples from the already scheduled curved RANS control until its first native refinement report. Includes initialization, CFD and automatic/explicit metric work, then first BL remeshing; it is not isolated validation time. Initial attachment latency is recorded. Function/call-stack diagnosis only: no exclusive/scaling/CFD-ratio or complete MPI/wait claim. Final native and independent receipts qualify correctness separately. The helper never launched, paused or signalled a solver.'
save();print(report['status'],flush=True)
