"""Run one retained-mesh correctness audit with bounded host resources and provenance."""
from pathlib import Path
import hashlib,json,os,sys,time
folder=Path(__file__).resolve().parent
root=folder.parents[2]
sys.path.insert(0,str(root/'integration_evidence/native_cluster_3d_core_v1'))
from run import command
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
stages=[];status='FAIL';reason=''
try:
 for args,label in [(['uptime'],'host_load'),(['free','-m'],'host_memory'),(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')]:
  row=command(args,folder,label,env,30);stages.append(row);assert row['exit_code']==0
 allowed=sorted(os.sched_getaffinity(0))
 def sample():return {line.split()[0]:list(map(int,line.split()[1:])) for line in Path('/proc/stat').read_text().splitlines() if line.startswith('cpu')}
 a=sample();time.sleep(1);b=sample();idle={}
 for cpu in allowed:
  d=[y-x for x,y in zip(a['cpu'+str(cpu)],b['cpu'+str(cpu)])];idle[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
 chosen=max(allowed,key=lambda c:idle[c])
 memory={k:int(v.split()[0]) for k,v in (line.split(':',1) for line in Path('/proc/meminfo').read_text().splitlines())}
 assert memory['MemAvailable']>=2*1024*1024
 (folder/'host_gate.json').write_text(json.dumps(dict(cpu=chosen,idle_sample=idle,available_bytes=1024*memory['MemAvailable'],load=os.getloadavg(),owned_maximum_compute_cpus=1,nice=19,threads=1,scope='Contention-authorized independent mesh correctness audit; no CFD/MPI or timing campaign.'),indent=2)+'\n')
 print('Working and retained case folder:',folder,flush=True)
 row=command(['taskset','-c',str(chosen),'nice','-n','19',sys.executable,str(folder/'audit.py')],folder,'audit',env,900);stages.append(row)
 assert row['exit_code']==0 and not row.get('timeout')
 assert json.loads((folder/'independent_audit.json').read_text())['status']=='PASS'
 status='PASS'
except Exception as e:reason=str(e);print('FAIL',reason,flush=True)
finally:
 result=dict(status=status,reason=reason,stages=stages,script_sha256=sha(Path(__file__).resolve()),command_helper_sha256=sha(root/'integration_evidence/native_cluster_3d_core_v1/run.py'),scope='Terminal one-CPU/nice19/thread1 independent audit of retained historical private rejected mesh; no production change, new SU2/MPI or calibrated timing claim.')
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
raise SystemExit(status!='PASS')
