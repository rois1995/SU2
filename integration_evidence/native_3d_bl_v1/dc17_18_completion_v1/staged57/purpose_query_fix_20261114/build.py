"""Build a frozen P5 checkpoint while retaining the build494 comparison binaries."""
import hashlib,json,os,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent
REPO=ROOT.parents[4]
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(name,value):(ROOT/name).write_text(json.dumps(value,indent=2)+'\n')
def identity(pid):
 try:return {'pid':pid,'start_ticks':int((Path('/proc')/str(pid)/'stat').read_text().rsplit(')',1)[1].split()[19])}
 except OSError:return None
pins=json.loads((ROOT/'source_pins.json').read_text());assert all(sha(p)==h for p,h in pins.items())
assert not (ROOT/'build_status.json').exists()
active=[]
for p in Path('/proc').glob('[0-9]*'):
 try:
  name=(p/'comm').read_text().strip()
  if name in ('SU2_CFD','test_driver','cc1plus','lto1','ninja'):active.append((p.name,name))
 except OSError:pass
assert not active,active
build=REPO/'integration_evidence/build-integrated-v3-p5-20261114'
env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(REPO/'integration_evidence/build-integrated-v2/compile-tmp'))
argv=['nice','-n','19',str(REPO/'integration_evidence/build-integrated-v2/tools/ninja'),'-C',str(build),'-j4','UnitTests/test_driver','SU2_CFD/src/SU2_CFD']
start=time.time();save('build_plan.json',{'argv':argv,'source_pins_sha256':sha(ROOT/'source_pins.json'),'environment':{k:env[k] for k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS','TMPDIR')},'started_unix':start,'loadavg':Path('/proc/loadavg').read_text(),'MemAvailable':next(l for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:'))})
with (ROOT/'build.log').open('x') as log,(ROOT/'build_resources.jsonl').open('x') as resources:
 proc=subprocess.Popen(argv,cwd=REPO,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
 save('build_status.json',{'state':'running','controller':identity(os.getpid()),'identity':identity(proc.pid)})
 while proc.poll() is None:
  resources.write(json.dumps({'unix':time.time(),'loadavg':Path('/proc/loadavg').read_text(),'meminfo':Path('/proc/meminfo').read_text()})+'\n');resources.flush();time.sleep(10)
 code=proc.wait()
changed=[p for p,h in pins.items() if sha(p)!=h]
result={'state':'terminal','exit_code':code,'seconds':time.time()-start,'pins_verified':not changed,'changed':changed,'binary':str(build/'SU2_CFD/src/SU2_CFD'),'test_binary':str(build/'UnitTests/test_driver')}
if code==0:
 result.update(binary_sha256=sha(result['binary']),test_binary_sha256=sha(result['test_binary']))
save('build_status.json',result);print(json.dumps(result),flush=True)
raise SystemExit(code or bool(changed))
