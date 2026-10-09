"""Sequential correctness checks for selectable PREDICT history; never a timing benchmark."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,signal,subprocess,sys
root=Path(__file__).resolve().parents[2]
os.chdir(root);sys.path.insert(0,str(root/'integration_evidence'))
from audit_native_unsteady import audit,read_mesh,state
from collect_native_cluster_results import TOOLS
from native_cluster_metric_comparison_v1.prepare_correctness import verify
import numpy as np
import h5py  # CGNS audit dependency: fail before allocating solver work if unavailable.
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,default=root/'build-native/SU2_CFD/src/SU2_CFD')
parser.add_argument('--test-binary',type=Path,default=root/'build-native/UnitTests/test_driver')
parser.add_argument('--machinefile',type=Path,required=True)
parser.add_argument('--timeout',type=int,default=7200)
parser.add_argument('--checkpoint',type=Path,default=root/'build-native/native_correctness_checkpoint.json')
args=parser.parse_args()
def closed(path):
 path=path.resolve();path.relative_to(root);assert path.is_file();return path
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
binary,test_binary=closed(args.binary),closed(args.test_binary)
assert args.timeout>0 and int(os.environ.get('NSLOTS','0'))>=4,'Request at least four SGE slots'
job=os.environ['JOB_ID'];assert re.fullmatch(r'[A-Za-z0-9_-]+',job)
base=root/'integration_evidence/native_post_rebase_metric_v1'
prepared=base/'predict_history_v1'
checkpoint_file=closed(args.checkpoint)
checkpoint_data=json.loads(checkpoint_file.read_text());verify(root,checkpoint_data)
assert str(binary.relative_to(root))==checkpoint_data['binaries']['SU2_CFD']['path']
assert str(test_binary.relative_to(root))==checkpoint_data['binaries']['test_driver']['path']
pins=checkpoint_data['files_sha256']
out=root/'ClusterResults'/('predict_correctness_'+job);out.mkdir(parents=True,exist_ok=False)
machinefile=out/'machinefile';shutil.copy2(args.machinefile,machinefile)
shutil.copy2(checkpoint_file,out/'checkpoint.json')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
record=dict(status='RUNNING',scope=__doc__,stages=[],binary_sha256=sha(binary),test_binary_sha256=sha(test_binary),checkpoint_sha256=sha(out/'checkpoint.json'),source_and_suite_files=len(pins),scheduler={key:os.environ.get(key) for key in ('JOB_ID','NSLOTS','QUEUE','PE')},skipped='MMG controls and archived workstation bitwise-output comparison; native-only cluster build')
for name in TOOLS:
 target=out/'tools'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(closed(root/name),target)
rows=[]
for row in json.loads((prepared/'prepared_cases.json').read_text()):
 if row['workers']==0:continue
 source=closed(root/row['case']/'run.cfg').parent
 case=out/row['name'];case.mkdir()
 for name in ('run.cfg','input.su2'):shutil.copy2(closed(source/name),case/name)
 row=dict(row,case=str(case.relative_to(root)));rows.append(row)
def save(): (out/'validation.json').write_text(json.dumps(record,indent=2)+'\n')
def run(label,command,cwd,negative=None):
 cwd.mkdir(parents=True,exist_ok=True)
 assert all(sha(closed(root/p))==v for p,v in pins.items())
 row=dict(label=label,command=command,working_directory=str(cwd.relative_to(root)),status='RUNNING');record['stages'].append(row);save()
 logpath=cwd/(label+'.log')
 with logpath.open('x') as log:
  print('Working folder:',str(cwd),flush=True)
  child=subprocess.Popen(command,cwd=cwd,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
  record['child_pid']=child.pid;save()
  try:code=child.wait(timeout=args.timeout)
  except subprocess.TimeoutExpired:
   os.killpg(child.pid,signal.SIGTERM)
   try:child.wait(timeout=10)
   except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
   code=124
 record.pop('child_pid',None);text=logpath.read_text()
 okay=code==0 if negative is None else code not in (0,124) and negative in text
 row.update(status='PASS' if okay else 'FAIL',exit_code=code,log_sha256=sha(logpath),test_summaries=re.findall(r'All tests passed \(([^\n]+)\)',text));save()
 print(label,code,row['test_summaries'],flush=True)
 assert okay,(label,code)
 return text
def mpi(n,program,*tail):return ['mpirun','-machinefile',str(machinefile),'-np',str(n),str(program),*tail]
def check_case(row):
 case=root/row['case'];text=run('solver',mpi(row['ranks'],binary,'run.cfg'),case)
 assert 'Exit Success' in text
 actual=list(map(int,re.findall(r'Metric snapshot of time step (\d+)',text)))
 assert actual==row['snapshots'], f"{row['name']}: expected snapshot steps {row['snapshots']}, got {actual}; check TIME_ITER, MAX_TIME and the solver stop reason"
 if row['count']>2:
  assert text.count(f"Temporal feature fit: {row['count']} snapshots")==len(row['snapshots'])//row['count']
 else:assert 'Temporal feature fit:' not in text
 result=audit(case,require_native_reference=row['workers']!=0);assert len(result['windows'])==row['adaptations']
 (case/'independent_unsteady_audit.json').write_text(json.dumps(result,indent=2)+'\n')
 row['audit_sha256']=sha(case/'independent_unsteady_audit.json')
 (case/'run_evidence.json').write_text(json.dumps(dict(status='PASS',solver_exit=0,binary_sha256=record['binary_sha256'],**row),indent=2)+'\n')
 print('INDEPENDENT CASE PASS',row['name'],flush=True)
serial='[Adaptation],[Gradients],[HessianReliability],[Hessian3DStress],[RestartReliability],[MetricRobustness],[NativeComposite2D]'
parallel='[HessianReliability],[Hessian3DStress],[RestartReliability],[MetricRobustness],[NativeComposite2D],[CustomSensorsMPI],[GoalMPI],Metric intersection,[SteadyMetricReuse],[MetricPredictionMPI]'
native='[NativeDistributed2D],[NativeField2D],[NativeEngine2D],[NativeSupport2D],[DistributedTransfer],[NativeCGNS2D],[NativeRejectedOutput],[NativeBL2D],[NativeProducedBL2D],[NativeRemesher],[MetricRobustness],[NativeComposite2D],[PassiveComm],[DistributedSearch]'
try:
 for n in (1,2,4):
  wd=out/f'units-mpi{n}'
  for label,selection in [('metric',serial if n==1 else parallel),('native',native)]:
   text=run(label,mpi(n,test_binary,selection,'--use-colour','no'),wd)
   assert text.count('All tests passed')==n
 run('fixed_point_guard',mpi(4,test_binary,'[NativeUnsupportedWindowMetric]','--use-colour','no'),out/'native-fixed-point-guard',negative='FIXED_POINT')
 template=(out/'four_n1m1/run.cfg').read_text()
 for name,key,value,diagnostic in [('count_low','ADAP_PREDICT_SNAPSHOTS','1','ADAP_PREDICT_SNAPSHOTS must'),('count_high','ADAP_PREDICT_SNAPSHOTS','6','ADAP_PREDICT_SNAPSHOTS must'),('cadence_span','ADAP_PREDICT_SEPARATION','2','ADAP_PREDICT_SEPARATION *'),('negative_filter','ADAP_PREDICT_TEMPORAL_FILTER','-1','ADAP_PREDICT_TEMPORAL_FILTER must')]:
  case=out/('invalid_'+name);case.mkdir(exist_ok=False)
  (case/'run.cfg').write_text(re.sub(r'^'+key+r'\s*=.*$',key+'= '+value,template,flags=re.M))
  shutil.copy2(out/'four_n1m1/input.su2',case/'input.su2')
  run('solver',mpi(1,binary,'run.cfg'),case,negative=diagnostic)
 for row in rows:check_case(row)
 # Use the actual adapted mesh, immutable native reference, and matching two BDF states from the donor fixture.
 parent=out/'four_fixture_n1m1'
 for start in (7,8,9):
  case=out/f'partial_restart_{start}_n1m1';case.mkdir(exist_ok=False)
  shutil.copy2(parent/'mesh_00005.su2',case/'input.su2');shutil.copy2(parent/'mesh_00005.su2.native_ref',case/'input.su2.native_ref')
  for step in (start-2,start-1):
   for p in parent.glob(f'solution_{step:05d}.*'):shutil.copy2(p,case/p.name)
  cfg=(parent/'run.cfg').read_text()+'\nRESTART_SOL= YES\nRESTART_ITER= '+str(start)+'\n'
  (case/'run.cfg').write_text(cfg)
  text=run('solver',mpi(1,binary,'run.cfg'),case)
  samples=list(range(start,10));assert list(map(int,re.findall(r'Metric snapshot of time step (\d+)',text)))==samples
  if start==7:assert 'Temporal feature fit: 3 snapshots' in text
  if start==8:assert 'Temporal feature fit:' not in text and 'motion of the metric features from time step 8 to 9' in text
  if start==9:assert 'only one available snapshot' in text
  assert text.count('Native adaptation:')==0 and 'Exit Success' in text
  mesh=read_mesh(case/'input.su2');_,admissibility=state(mesh,case/'solution_00009.dat')
  from audit_native_unsteady import capcheck
  metric,precision=capcheck.metric_of(mesh,case/'flow_00009.vtu')
  eigen=np.linalg.eigvalsh(metric);assert precision=='Float64' and np.isfinite(eigen).all() and (eigen>0).all()
  report=dict(status='PASS',scope='Partial-window snapshot selection and matching restart geometry/state; no new remesh or history transfer in these short restarts',samples=samples,final_admissibility=admissibility,min_metric_eigenvalue=float(eigen.min()),input_sha256={p.name:sha(p) for p in case.iterdir() if p.name in ('input.su2','input.su2.native_ref','run.cfg')})
  (case/'independent_partial_restart_audit.json').write_text(json.dumps(report,indent=2)+'\n');print('PARTIAL RESTART PASS',start,flush=True)
 verify(root,checkpoint_data)
 assert sha(out/'checkpoint.json')==record['checkpoint_sha256'],'Saved checkpoint changed during job'
 record.update(status='PASS',phase='terminal',cases=rows);save();print('PREDICT HISTORY VALIDATION PASS',len(record['stages']),'sequential stages',flush=True)
except BaseException as error:
 record.update(status='FAIL',phase='terminal',error=str(error));save();raise
