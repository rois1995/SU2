"""Run one prepared RAE2822 case, preserve outputs and obey workstation limits."""
import argparse, hashlib, json, os, re, shutil, signal, subprocess, time
from pathlib import Path

E = Path(__file__).resolve().parent
source = E.parent

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
    return h.hexdigest()

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('case',type=Path);p.add_argument('--ranks',type=int,choices=(1,2,4),default=4)
p.add_argument('--timeout',type=float,default=1800)
p.add_argument('--build-evidence',type=Path,default=E/'integrated_native_production_v2/evidence.json');args=p.parse_args()
wd=args.case.resolve(strict=True);state=wd/'run_evidence.json'
mesh_name=re.search(r'^MESH_FILENAME\s*=\s*([^%\n]+)',(wd/'run.cfg').read_text(),re.M)[1].strip()
mesh_path=(wd/mesh_name).resolve(strict=True)
if state.exists():raise SystemExit('Preserve existing evidence; use a fresh case folder')
prior=json.loads(args.build_evidence.read_text())
binary=Path(prior.get('archived_binary',E/'integrated_native_production_v1/SU2_CFD'))
if sha(binary)!=prior['binary_sha256']:raise RuntimeError('Archived application changed')
if any(sha(source/name)!=digest for name,digest in prior['source_sha256'].items()):raise RuntimeError('Production sources changed; revalidate binary')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
record=dict(runner_pid=os.getpid(),phase='preparing',working_directory=str(wd),ranks=args.ranks,
            timeout_seconds=args.timeout,binary=str(binary),binary_sha256=sha(binary),
            config_sha256=sha(wd/'run.cfg'),mesh_sha256=sha(mesh_path),mesh_filename=mesh_name,
            source_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip(),
            production_sources_rechecked=len(prior['source_sha256']),source_evidence=str(args.build_evidence.resolve()),
            source_evidence_sha256=sha(args.build_evidence),runner_sha256=sha(__file__),
            environment={k:env[k] for k in ('OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS')})
def save():state.write_text(json.dumps(record,indent=2)+'\n')
shutil.copy2(__file__,wd/'runner_source.py');save();quiet_since=None
while True:
    busy=[]
    for line in subprocess.check_output(['ps','-eo','pid,stat,comm'],text=True).splitlines()[1:]:
        pid,flags,name=line.split(maxsplit=2)
        if pid!='918696' and 'Z' not in flags and (name in ('ninja','cc1plus','test_driver','test_driver_AD','test_memory') or name.startswith('SU2_CFD')):busy.append(int(pid))
    record.update(phase='waiting_for_machine',busy=busy);save()
    if busy:quiet_since=None
    elif quiet_since is None:quiet_since=time.monotonic()
    elif time.monotonic()-quiet_since>=15:break
    time.sleep(5)
command=['mpiexec','-n',str(args.ranks),str(binary),'run.cfg'];start=time.monotonic()
with (wd/'solver.log').open('x') as log:
    child=subprocess.Popen(command,cwd=wd,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
    record.update(phase='running',command=command,child_pid=child.pid,started_unix_seconds=time.time());save()
    while True:
        try:code=child.wait(timeout=5);break
        except subprocess.TimeoutExpired:
            elapsed=time.monotonic()-start;record['elapsed_seconds']=elapsed;save()
            if elapsed>=args.timeout:
                os.killpg(child.pid,signal.SIGTERM)
                try:child.wait(timeout=10)
                except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
                code=124;break
record.update(phase='terminal',solver_exit=code,elapsed_seconds=time.monotonic()-start,
              outputs={f.name:f.stat().st_size for f in wd.iterdir() if f.is_file() and f.suffix in ('.su2','.dat','.vtu','.csv')})
record.pop('child_pid',None);save()
if sha(wd/'run.cfg')!=record['config_sha256'] or sha(mesh_path)!=record['mesh_sha256']:raise RuntimeError('Case input changed during execution')
print(str(wd)+': solver exit '+str(code)+', '+str(round(record['elapsed_seconds'],3))+' seconds')
raise SystemExit(code)
