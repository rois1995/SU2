"""One sequential MPI/case campaign; stop on a failed check and retain evidence."""
from pathlib import Path
import json,os,subprocess,sys,time
root=Path(__file__).resolve().parent
os.chdir(root.parent)
state=root/'native_joint_surface_validation_v1.json'
assert not state.exists(),'Use a new campaign rather than replacing evidence'
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
record={'controller_pid':os.getpid(),'sequential':True,'phase':'prepared','stages':[]}
def save():state.write_text(json.dumps(record,indent=2)+'\n')
def run(name,args):
 start=time.monotonic();record.update(phase=name,command=args);save()
 with (root/(name+'.controller.log')).open('x') as log:
  child=subprocess.Popen([sys.executable,*args],env=env,stdout=log,stderr=subprocess.STDOUT)
  record['child_pid']=child.pid;save();code=child.wait()
 record.pop('child_pid',None)
 record['stages'].append({'name':name,'exit_code':code,'elapsed_seconds':time.monotonic()-start});save()
 print(name,'exit',code,flush=True)
 if code:
  record['phase']='terminal_failed';save();raise SystemExit(code)
save()
run('native_joint_surface_core_v2',['integration_evidence/run_native_driver_checks.py','--label','native_joint_surface_core_v2','--build','build-integrated-v2','--ranks','1','2','4','--filter','[NativeMesh2D],[NativeField2D],[NativeEngine2D],[NativeComposite2D],[BoundaryLayerMetric]','--timeout','180'])
for kind,case in [('euler','nativefix_euler_rans_seed_v13'),('rans','nativefix_rans_euler_seed_v22')]:
 folder=root/'rae2822_transonic_v1'/case
 run('native_joint_surface_'+kind+'_v1',['integration_evidence/run_rae2822_trial.py',str(folder),'--ranks','4','--timeout','700','--build-evidence',str(root/'native_cross_grid_build_v30/evidence.json')])
 run('native_joint_surface_'+kind+'_inspection_v1',['integration_evidence/inspect_rae2822_solution.py',str(folder)])
 if kind=='euler':run('native_joint_surface_euler_metric_v1',[str(folder/'audit_metric_source.py')])
 else:run('native_joint_surface_rans_metric_v1',['integration_evidence/audit_native_composite_rae.py',str(folder)])
run('native_joint_surface_adapter_captured_v1',['integration_evidence/run_native_driver_checks.py','--label','native_joint_surface_adapter_captured_v1','--build','build-integrated-v2','--ranks','1','2','4','--filter','[NativeRemesher],[NativeProducedBL2D],[NativeRAEJointMPI]','--save-audit','--cavity-replay-folder',str(root/'native_joint_captured_mpi_v1/captured_fixture'),'--timeout','180'])
record['phase']='terminal_pass';save()
