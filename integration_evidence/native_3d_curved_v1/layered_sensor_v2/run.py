from pathlib import Path
import hashlib,json,os,subprocess,sys,time,shlex,re,math
from collections import defaultdict
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for chunk in iter(lambda:f.read(1048576),b''):h.update(chunk)
 return h.hexdigest()
def det(a,b,c):return a[0]*(b[1]*c[2]-b[2]*c[1])-a[1]*(b[0]*c[2]-b[2]*c[0])+a[2]*(b[0]*c[1]-b[1]*c[0])
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mesh_audit(work,name,row):
 nodes={n[0]:n[1:] for n in row['nodes']};faces=defaultdict(list);volume=0
 for c in row['cells']:
  q=[nodes[n] for n in c];v=det(sub(q[1],q[0]),sub(q[2],q[0]),sub(q[3],q[0]))/6
  assert v>0;volume+=v
  for f in ((c[1],c[2],c[3]),(c[0],c[3],c[2]),(c[0],c[1],c[3]),(c[0],c[2],c[1])):faces[tuple(sorted(f))].append(f)
 def canonical(f):i=f.index(min(f));return tuple(f[i:]+f[:i])
 for owners in faces.values():
  assert len(owners)<=2
  if len(owners)==2:assert canonical(list(owners[0]))==canonical(list(reversed(owners[1])))
 skin={canonical(list(o[0])) for o in faces.values() if len(o)==1}
 observed={canonical(f[:3]) for f in row['surface']}
 assert skin==observed and len(observed)==len(row['surface'])
 signed=sum(det(nodes[f[0]],nodes[f[1]],nodes[f[2]])/6 for f in row['surface'])
 assert abs(signed-volume)<1e-14
 ids=sorted(nodes);renumber={n:i for i,n in enumerate(ids)}
 lines=['NDIME= 3','NELEM= '+str(len(row['cells']))]
 lines+=['10 '+' '.join(str(renumber[n]) for n in c)+' '+str(i) for i,c in enumerate(row['cells'])]
 lines+=['NPOIN= '+str(len(ids))]+[' '.join(format(x,'.17g') for x in nodes[n])+' '+str(renumber[n]) for n in ids]
 markers=sorted(set(f[4] for f in row['surface']));lines+=['NMARK= '+str(len(markers))]
 for m in markers:
  fs=[f for f in row['surface'] if f[4]==m];lines+=['MARKER_TAG= reference_marker_'+str(m),'MARKER_ELEMS= '+str(len(fs))]
  lines+=['5 '+' '.join(str(renumber[n]) for n in f[:3]) for f in fs]
 (work/(name+'.su2')).write_text('\n'.join(lines)+'\n')
 return volume
def audit(work):
 summary={}
 for shape in ('sphere','bump'):
  records={};volumes={}
  for phase in ('original','split','coarsened'):
   name=shape+'_'+phase;p=work/(name+'.json');row=json.loads(p.read_text());records[phase]=row
   volumes[phase]=mesh_audit(work,name,row)
  old={n[0]:n[1:] for n in records['original']['nodes']};split={n[0]:n[1:] for n in records['split']['nodes']}
  assert all(split[n]==p for n,p in old.items())
  assert {n[0]:n[1:] for n in records['coarsened']['nodes']}==old
  assert {tuple(sorted(c)) for c in records['coarsened']['cells']}=={tuple(sorted(c)) for c in records['original']['cells']}
  assert sorted(records['coarsened']['surface'])==sorted(records['original']['surface'])
  assert volumes['split']>volumes['original'] and volumes['coarsened']==volumes['original']
  p=split[4]
  if shape=='sphere':residual=abs(math.sqrt(sum(x*x for x in p))-1)
  else:residual=abs(p[2]-p[0]*(1-p[0])*p[1]*(1-p[1]))
  assert residual<=1e-12
  summary[shape]={'volumes':volumes,'projected_point':p,'geometry_residual':residual,'skin_equals_oriented_volume_boundary':True,'old_coordinates_exact':True,'reverse_connectivity_exact':True}
 joint={}
 for phase in ('original','inserted','moved','collapsed'):
  name='joint_bump_'+phase;row=json.loads((work/(name+'.json')).read_text())
  joint[phase]={'volume':mesh_audit(work,name,row),'nodes':{str(n[0]):n[1:] for n in row['nodes']},'cells':sorted(sorted(c) for c in row['cells'])}
 assert joint['collapsed']['volume']==joint['original']['volume']
 assert joint['collapsed']['cells']!=joint['original']['cells']
 for n,p in joint['original']['nodes'].items():assert joint['inserted']['nodes'][n]==joint['moved']['nodes'][n]==joint['collapsed']['nodes'][n]==p
 for phase,node in (('inserted','4'),('moved','7')):
  p=joint[phase]['nodes'][node];assert abs(p[2]-p[0]*(1-p[0])*p[1]*(1-p[1]))<=1e-12
 assert joint['inserted']['volume']>joint['original']['volume'] and joint['moved']['volume']>joint['original']['volume']
 summary['joint_bump']=joint
 row=json.loads((work/'layered_sphere_split.json').read_text());summary['layered_sphere_split_volume']=mesh_audit(work,'layered_sphere_split',row)
 (work/'geometry_audit.json').write_text(json.dumps(summary,indent=2)+'\n');return summary
assert not (HERE/'validation.json').exists()
control=json.loads((HERE.parent/'joint_bump_geometry_v1/validation.json').read_text());assert control['status']=='PASS'
first=json.loads((HERE.parent/'layered_sensor_v1/validation.json').read_text());assert first['status']=='FAIL'
changed={'Common/include/adaptation/CNativeMesh3D.hpp','Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp','Common/src/adaptation/CNativePredicates3D.cpp','Common/src/adaptation/CNativeEngine3D.cpp','Common/src/adaptation/CNativeImport3D.cpp','UnitTests/Common/adaptation/CNativeField3D_tests.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp'}
assert all(sha(ROOT/n)==h for n,h in control['source_sha256'].items() if n.startswith(('Common/','SU2_CFD/','UnitTests/')) and n not in changed)
pins={n:sha(ROOT/n) for n in control['source_sha256'] if n.startswith(('Common/','SU2_CFD/','UnitTests/'))}
pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
assert int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024>=3*1024**3
(HERE/'host.txt').write_text(subprocess.check_output(['uptime'],text=True)+subprocess.check_output(['free','-m'],text=True)+'\n'.join(subprocess.check_output(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],text=True).splitlines()[:12])+'\n')
def ticks():return {l.split()[0]:list(map(int,l.split()[1:])) for l in Path('/proc/stat').read_text().splitlines() if l.startswith('cpu')}
a=ticks();time.sleep(1);b=ticks();idle={}
for c in os.sched_getaffinity(0):
 d=[y-x for x,y in zip(a['cpu'+str(c)],b['cpu'+str(c)])];idle[c]=(d[3]+d[4])/sum(d) if sum(d) else 0
cpu=max(idle,key=idle.get);prefix=['taskset','-c',str(cpu),'nice','-n','19']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',PYTHONDONTWRITEBYTECODE='1',OMPI_MCA_mpi_yield_when_idle='1',SU2_NATIVE_CURVED_EVIDENCE='1')
sys.path.insert(0,str(ROOT/'integration_evidence/native_cluster_3d_core_v1'));from run import command
stages=[];objects={};libraries={};members={};audits={};counts=[];status='FAIL';reason='';binary=BUILD/'UnitTests/test_driver'
def run(args,label,work,timeout=1800):
 print('CASE_FOLDER='+str(work),flush=True);row=command(prefix+list(map(str,args)),work,label,env,timeout);stages.append(row)
 if row['exit_code'] or row.get('timeout'):raise RuntimeError(label+' failed; original logs retained')
 return (work/(label+'.stdout')).read_text()
try:
 assert sha(binary)==first['binary_sha256']
 run(['/tmp/native-build-tools/ninja','-C',BUILD,'-j1','UnitTests/test_driver'],'build',HERE,3600)
 line=next(l for l in (BUILD/'build.ninja').read_text().splitlines() if l.startswith('build UnitTests/test_driver: cpp_LINKER '))
 objects={n:sha(BUILD/n) for n in shlex.split(line.split('cpp_LINKER ',1)[1].split(' | ',1)[0]) if n.endswith('.o')};assert len(objects)==68
 assert set(objects)==set(control['actual_objects_sha256'])
 for n in control['library_sha256']:libraries[n]=sha(Path(n))
 for n in control['thin_archive_member_sha256']:members[n]=sha(Path(n))
 binary_pin=sha(binary)
 for n in (1,2,4):
  work=HERE/('mpi'+str(n));work.mkdir()
  out=run(['mpirun','--oversubscribe','--bind-to','none','-np',n,binary,'[NativeLayeredField3D],[NativeLayeredDistributed3D],[NativeCurvedLayered3D],[NativeCurvedChart3D],[NativeCurvedGeometry3D],[NativeCurvedReference3D],[NativeMesh3D],[NativeReference3D],[NativeCurvedCoverage3D],[NativeBridge3D],[NativeRuntime3D],[NativeDefault3D],[NativeField2D]','--use-colour','no'],'case',work)
  found=re.findall(r'All tests passed \((\d+) assertions in (\d+) test cases\)',out)
  assert len(found)==n and len(set(c for a,c in found))==1 and int(found[0][1])==95
  counts.append({'ranks':n,'cases_per_rank':int(found[0][1]),'assertions':[int(a) for a,c in found]})
  audits[str(n)]=audit(work)
 assert audits['1']==audits['2']==audits['4']
 assert sha(binary)==binary_pin;status='PASS'
except Exception as e:reason=str(e);print(reason,flush=True)
finally:
 drift=[n for n,h in pins.items() if sha(ROOT/n)!=h]
 if drift:status='FAIL'
 receipt=dict(status=status,reason=reason,source_sha256=pins,source_drift=drift,actual_objects_sha256=objects,library_sha256=libraries,thin_archive_member_sha256=members,stages=stages,counts=counts,geometry_audits=audits,scope='Goal2 explicit associated frozen exterior sensor coverage; original P1 priority at queries and exact layered edge intervals; thin original demand and coincident-cut widths; actual-query composition and same-priority overlap/wrong-component rejection; real MPI source-tag transport; coupled actual curved replacement vertex/centroid/shape/sensor-length checks with declared shell. Automatic exterior generation/admission and curved production runtime remain unimplemented. Existing compatible convex bump-chart union; actual off-edge cavity insertion, projected cross-seam movement, changed-diagonal coarsening/repeated edits; component/marker/definition separation and collective sidecar reconstruction. Previous analytic sphere/bump finite geometry authority, projected complete-edge-star split/inverse, inherited-connectivity inversion rollback, unchanged interface coordinates, indexed finite oriented binding, version2 persistence/v1 regression, immutable-authority serialization checks, unsupported curved/planar junction rollback, bounded physical distance at acute finite chart corners and collective runtime refusal. Existing planar3D bridge/default/runtime/output/history and2D field regressions also selected. Geometry operator studies run identically on each rank; NOT distributed curved remeshing, metric extension, final Hausdorff/clearance proof or actual CFD. Independent signed volume/skin/node/geometry/connectivity checks, inspectable private proposal SU2 files. Full configured68-object driver. OneCPU/nice19, threads1, build-j1, MPI<=4; no performance or cluster qualification.')
 if binary.exists():receipt['binary_sha256']=sha(binary)
 receipt['artifact_sha256']={str(p.relative_to(HERE)):sha(p) for p in HERE.rglob('*') if p.is_file() and p.name!='validation.json'}
 (HERE/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(status,flush=True)
raise SystemExit(status!='PASS')
