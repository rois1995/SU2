"""Sequential correctness checks for selectable PREDICT history; never a timing benchmark."""
from pathlib import Path
import hashlib,json,os,re,shutil,signal,subprocess,sys
root=Path(__file__).resolve().parents[2] if Path(__file__).name=='complete_predict_history_validation.py' and 'integration_evidence' in str(Path(__file__)) else Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
os.chdir(root);sys.path.insert(0,str(root/'integration_evidence'))
from audit_native_unsteady import audit,read_mesh,state
from run_cases import point_fields
import numpy as np
base=root/'integration_evidence/native_post_rebase_metric_v1';out=base/'predict_history_v1';build=base/'build_predict_v1'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((build/'evidence.json').read_text());assert receipt['status']=='PASS'
assert sha(build/'SU2_CFD')==receipt['binary_sha256'] and sha(build/'test_driver')==receipt['test_driver_sha256']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',OMPI_MCA_hwloc_base_binding_policy='none')
record=dict(status='RUNNING',scope=__doc__,build_receipt_sha256=sha(build/'evidence.json'),stages=[],test_source_sha256={str(p.relative_to(root)):sha(p) for p in [root/'UnitTests/Common/adaptation/AdaptationLoopOptions_tests.cpp',root/'UnitTests/SU2_CFD/adaptation/CMetricPredictor_tests.cpp',root/'UnitTests/SU2_CFD/adaptation/CNativeSupport2D_tests.cpp']},configuration_template_sha256=sha(root/'config_template.cfg'))
def save(): (out/'validation.json').write_text(json.dumps(record,indent=2)+'\n')
def run(label,command,cwd,timeout=600,negative=None):
 cwd.mkdir(parents=True,exist_ok=True)
 assert all(sha(root/p)==v for p,v in {**receipt['source_sha256'],**record['test_source_sha256']}.items())
 row=dict(label=label,command=command,working_directory=str(cwd.relative_to(root)),status='RUNNING');record['stages'].append(row);save()
 logpath=cwd/(label+'.log')
 with logpath.open('x') as log:
  child=subprocess.Popen(['taskset','-c','4,5,6,7','nice','-n','10',*command],cwd=cwd,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
  record['child_pid']=child.pid;save()
  try:code=child.wait(timeout=timeout)
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
def mpi(n,binary,*args):return ['mpirun','--bind-to','none','--host','localhost:4','-np',str(n),str(binary),*args]
def check_case(row):
 case=root/row['case'];text=run('solver',mpi(row['ranks'],build/'SU2_CFD','run.cfg'),case)
 assert 'Exit Success' in text
 assert list(map(int,re.findall(r'Metric snapshot of time step (\d+)',text)))==row['snapshots']
 if row['count']>2:
  assert text.count('Temporal feature fit: 4 snapshots')==len(row['snapshots'])//4
 else:assert 'Temporal feature fit:' not in text
 result=audit(case,require_native_reference=row['workers']!=0);assert len(result['windows'])==row['adaptations']
 (case/'independent_unsteady_audit.json').write_text(json.dumps(result,indent=2)+'\n')
 row['audit_sha256']=sha(case/'independent_unsteady_audit.json')
 (case/'run_evidence.json').write_text(json.dumps(dict(status='PASS',solver_exit=0,binary_sha256=receipt['binary_sha256'],**row),indent=2)+'\n')
 print('INDEPENDENT CASE PASS',row['name'],flush=True)
serial='[Adaptation],[Gradients],[HessianReliability],[Hessian3DStress],[RestartReliability],[MetricRobustness],[NativeComposite2D]'
parallel='[HessianReliability],[Hessian3DStress],[RestartReliability],[MetricRobustness],[NativeComposite2D],[CustomSensorsMPI],[GoalMPI],Metric intersection,[SteadyMetricReuse],[MetricPredictionMPI]'
native='[NativeDistributed2D],[NativeField2D],[NativeEngine2D],[NativeSupport2D],[DistributedTransfer],[NativeCGNS2D],[NativeRejectedOutput],[NativeBL2D],[NativeProducedBL2D],[NativeRemesher],[MetricRobustness],[NativeComposite2D],[PassiveComm],[DistributedSearch]'
try:
 for n in (1,2,4):
  wd=out/f'units-mpi{n}'
  for label,selection in [('metric',serial if n==1 else parallel),('native',native)]:
   text=run(label,mpi(n,build/'test_driver',selection,'--use-colour','no'),wd)
   assert text.count('All tests passed')==n
 run('fixed_point_guard',mpi(4,build/'test_driver','[NativeUnsupportedWindowMetric]','--use-colour','no'),out/'native-fixed-point-guard',negative='FIXED_POINT')
 template=(out/'four_n1m1/run.cfg').read_text()
 for name,key,value,diagnostic in [('count_low','ADAP_PREDICT_SNAPSHOTS','1','ADAP_PREDICT_SNAPSHOTS must'),('count_high','ADAP_PREDICT_SNAPSHOTS','6','ADAP_PREDICT_SNAPSHOTS must'),('cadence_span','ADAP_PREDICT_SEPARATION','2','ADAP_PREDICT_SEPARATION *'),('negative_filter','ADAP_PREDICT_TEMPORAL_FILTER','-1','ADAP_PREDICT_TEMPORAL_FILTER must')]:
  case=out/('invalid_'+name);case.mkdir(exist_ok=False)
  (case/'run.cfg').write_text(re.sub(r'^'+key+r'\s*=.*$',key+'= '+value,template,flags=re.M))
  shutil.copy2(out/'four_n1m1/input.su2',case/'input.su2')
  run('solver',mpi(1,build/'SU2_CFD','run.cfg'),case,negative=diagnostic)
 rows=json.loads((out/'prepared_cases.json').read_text())
 for row in rows:check_case(row)
 # The default two-frame path must preserve the former MMG control exactly in mesh and saved numerical fields.
 old=base/'vortex-predict-mmg-n4-v2';new=out/'default_two_mmg_n4';comparisons=[]
 for step in (3,6):
  a,b=read_mesh(old/f'mesh_{step:05d}.su2'),read_mesh(new/f'mesh_{step:05d}.su2')
  assert np.array_equal(a.P,b.P) and np.array_equal(a.E,b.E)
  comparisons.append(dict(step=step,points_equal=True,elements_equal=True))
 for step in range(9):
  a,b=point_fields(old/f'flow_{step:05d}.vtu'),point_fields(new/f'flow_{step:05d}.vtu')
  assert a.keys()==b.keys()
  for key in a:assert np.array_equal(np.asarray(a[key]),np.asarray(b[key])),(step,key)
 record['legacy_two_snapshot_comparison']=comparisons
 # Use the actual adapted mesh, immutable native reference, and matching two BDF states from the donor fixture.
 parent=out/'four_fixture_n1m1'
 for start in (7,8,9):
  case=out/f'partial_restart_{start}_n1m1';case.mkdir(exist_ok=False)
  shutil.copy2(parent/'mesh_00005.su2',case/'input.su2');shutil.copy2(parent/'mesh_00005.su2.native_ref',case/'input.su2.native_ref')
  for step in (start-2,start-1):
   for p in parent.glob(f'solution_{step:05d}.*'):shutil.copy2(p,case/p.name)
  cfg=(parent/'run.cfg').read_text()+'\nRESTART_SOL= YES\nRESTART_ITER= '+str(start)+'\n'
  (case/'run.cfg').write_text(cfg)
  text=run('solver',mpi(1,build/'SU2_CFD','run.cfg'),case)
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
 record.update(status='PASS',phase='terminal',cases=rows);save();print('PREDICT HISTORY VALIDATION PASS',len(record['stages']),'sequential stages',flush=True)
except BaseException as error:
 record.update(status='FAIL',phase='terminal',error=str(error));save();raise
