from pathlib import Path
import hashlib,json,os,subprocess,time,re,shlex,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
pins={str(p.relative_to(ROOT)):sha(p) for d in ('Common','SU2_CFD','UnitTests') for p in (ROOT/d).rglob('*') if p.is_file() and p.suffix in ('.cpp','.hpp','.h','.inl','.tpp')}
pins['config_template.cfg']=sha(ROOT/'config_template.cfg');pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
assert not (HERE/'validation.json').exists()
assert int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024>=3*1024**3
assert __import__('shutil').disk_usage(HERE).free>=3*1024**3
(HERE/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-m'],text=True)+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,pmem,comm','--sort=-pcpu'],text=True).splitlines()[:15])+'\n')
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for cpu in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(cpu)],b['cpu'+str(cpu)])];idle[cpu]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
(HERE/'host_gate.json').write_text(json.dumps(dict(cpu=cpu,idle=idle,nice=19,maximum_compute_cpus=1,threads=1,scope='User-authorized correctness only, no performance campaign'),indent=2)+'\n')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1')
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'));from run import command
stages=[];status='FAIL';reason='';objects={};libraries={};members={};binary=BUILD/'UnitTests/test_driver'
def run(args,label,cwd,timeout=18000):
 print(label,flush=True);row=command(prefix+list(map(str,args)),cwd,label,env,timeout);stages.append(row)
 if cwd!=HERE:
  for ext in ('stdout','stderr'):__import__('shutil').move(cwd/(label+'.'+ext),HERE/(label+'.'+ext))
 if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; logs retained')
 return (HERE/(label+'.stdout')).read_text()
try:
 qualified=ROOT/'integration_evidence/native_3d_core_v1/execution_policy_veto_v2'
 control=json.loads((qualified/'validation.json').read_text())
 assert control['status']=='PASS' and not control['source_drift'] and not control['object_drift'] and not control['library_drift']
 assert all(sha(ROOT/n)==h for n,h in control['source_sha256'].items() if n.startswith(('Common/','SU2_CFD/','UnitTests/')) or n=='config_template.cfg')
 assert sha(binary)==control['binary_sha256']
 lines=(BUILD/'build.ninja').read_text().splitlines();link=next(l for l in lines if l.startswith('build UnitTests/test_driver: cpp_LINKER '))
 objects={n:sha(BUILD/n) for n in shlex.split(link.split('cpp_LINKER ',1)[1].split(' | ',1)[0]) if n.endswith('.o')};assert len(objects)==68
 args=shlex.split(next(l.split('=',1)[1] for l in lines[lines.index(link)+1:] if l.startswith(' LINK_ARGS =')))
 for n in args:
  if not n.endswith('.a'):continue
  p=Path(n);p=p if p.is_absolute() else BUILD/p;libraries[str(p)]=sha(p)
  with p.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for n in subprocess.check_output(['ar','t',p],cwd=ROOT,text=True).splitlines():
    q=Path(n);q=q if q.is_absolute() else ROOT/q;members[str(q)]=sha(q)
 binary_pin=sha(binary)
 assert objects==control['actual_objects_sha256']
 assert libraries==control['library_sha256'] and members==control['thin_archive_member_sha256']
 cases=[
  ('n8_n1_m1_batch1',dict(ranks=1,workers=1,size=8,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=0,candidate_batch=1,repeat=0,detail=1,restart_format='none')),
  ('n8_n1_m1_batch8',dict(ranks=1,workers=1,size=8,anisotropy=1,varying=1,direction=1,height_power=0,wedge=True,aligned_markers=True,diagnostic_rounds=0,candidate_batch=8,repeat=0,detail=1,restart_format='none')),
  ('n4_n4_m2_thin_cgns',dict(ranks=4,workers=2,size=4,anisotropy=2,varying=1,direction=1,height_power=20,wedge=True,aligned_markers=True,diagnostic_rounds=0,candidate_batch=8,repeat=0,detail=1,restart_format='cgns'))]
 for name,p in cases:
  work=HERE/name;work.mkdir();print('CASE_FOLDER='+str(work),flush=True)
  (work/'parameters.json').write_text(json.dumps(p,indent=2)+'\n')
  settings=dict(size=p['size'],workers=p['workers'],anisotropy=p['anisotropy'],varying=1,direction=1,height_power=p['height_power'],wedge=1,aligned=1,rounds=0,repeat=0,detail=1,restart=2 if p['restart_format']=='cgns' else 0,batch=p['candidate_batch'])
  local=dict(env,**{'SU2_NATIVE3D_ENVELOPE_'+k.upper():str(v) for k,v in settings.items()})
  args=prefix+['mpirun','--oversubscribe','--bind-to','none','-np',str(p['ranks']),str(binary),'[NativeEnvelope3D]','--use-colour','no']
  row=command(args,work,'case',local,3600);stages.append(row)
  if row['exit_code'] or row.get('timeout'):raise RuntimeError(name+' failed; actual default-policy driver/state outputs preserved')
  raw=(work/'case.stdout').read_text()
  assert len(re.findall(r'All tests passed \(\d+ assertions in 1 test case\)',raw))==p['ranks']
  if p['candidate_batch']==1:
   rows=[dict(x.split('=',1) for x in l.split()[1:]) for l in raw.splitlines() if l.startswith('NATIVE3D_DETAIL ') and ('stage=refine ' in l or 'stage=coarsen ' in l)]
   assert any(int(row['rounds'])>512 for row in rows),'Need actual productive completion beyond the old round cutoff'
  run(['python3',ROOT/'integration_evidence/native_3d_core_v1/audit_envelope.py',work],'audit_'+name,work,3600)
  assert json.loads((work/'independent_audit.json').read_text())['status']=='PASS'
  run(['python3',ROOT/'integration_evidence/native_3d_core_v1/export_envelope_vtu.py',work],'vtu_'+name,work,300)
 assert sha(binary)==binary_pin;status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(ROOT/n)!=h];object_drift=[n for n,h in objects.items() if sha(BUILD/n)!=h];library_drift=[n for n,h in {**libraries,**members}.items() if sha(Path(n))!=h]
 if drift or object_drift or library_drift:status='FAIL'
 receipt=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,object_drift=object_drift,library_drift=library_drift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,scope='Local actual default work-policy driver qualification: productive serial size8batch1 completion beyond512, default batch8size8 and rotated/varying aspect2/H20thinN4M2CGNS restart, each3actual driver events with fresh independent geometry/original-P1/current-BDF/output/readback certificate. Full configured68-object driver and production archive pins. One local correctness pass per case, no performance/cluster work and no fullGoal1 claim until broad regression/completion audit.')
 if binary.exists():receipt['binary_sha256']=sha(binary)
 receipt['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json'}
 (HERE/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(status,flush=True)
raise SystemExit(status!='PASS')
