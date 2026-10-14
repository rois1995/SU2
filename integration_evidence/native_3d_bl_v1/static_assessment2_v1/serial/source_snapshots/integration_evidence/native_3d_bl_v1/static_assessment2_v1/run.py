#!/usr/bin/env python3
"""Resumable focused serial/MPI checks for static assessment fixes; no performance claims."""
import hashlib,json,os,re,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2';BINARY=BUILD/'UnitTests/test_driver'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def main():
 assert len(sys.argv)==2 and sys.argv[1] in ('serial','mpi2','mpi4')
 stage=sys.argv[1];ranks={'serial':1,'mpi2':2,'mpi4':4}[stage]
 selection=('[NativeReference3D],[NativeReferenceSearch3D],[NativeComposite3D],[NativeVolumeBL3D],'
            '[NativeFirstLayerEngine3D],[NativeFirstLayerConcurrency3D],[NativePolicyVeto3D],'
            '[NativeConnectivity3D],[NativeConnectivityReplay3D],[NativeBLPolicyRuntime3D]') if ranks==1 else (
            '[NativeFirstLayerEngine3D],[NativeFirstLayerConcurrency3D],[NativePolicyVeto3D],[NativeReference3D]')
 listed=subprocess.run([str(BINARY),selection,'--list-test-names-only'],stdout=subprocess.PIPE,text=True)
 expected=len(listed.stdout.strip().splitlines());assert expected>0 and listed.returncode==expected%256
 check=subprocess.run(['/tmp/native-build-tools/ninja','-C',str(BUILD),'-n','UnitTests/test_driver','SU2_CFD/src/SU2_CFD'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,universal_newlines=True,timeout=30)
 assert check.returncode==0 and 'ninja: no work to do.' in check.stdout,check.stdout
 sources=['Common/include/adaptation/CNativeCavity3D.hpp','Common/include/adaptation/CNativeReferenceIO3D.hpp','Common/include/adaptation/CRemesher.hpp','SU2_CFD/include/drivers/CSinglezoneDriver.hpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp','UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp','UnitTests/Common/adaptation/fixtures/native_wall_joint.inc','Common/include/adaptation/CNativeCavity3D.hpp','Common/src/adaptation/CNativeCavity3D.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','Common/src/adaptation/CNativeDistributed3D.cpp','Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp','Common/src/adaptation/CNativeMetric3D.cpp','Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','Common/include/adaptation/CNativeRemesher3D.hpp','Common/src/adaptation/CNativeRemesher3D.cpp','Common/include/adaptation/CNativeEngine3D.hpp','Common/src/adaptation/CNativeEngine3D.cpp','Common/src/adaptation/CNativeImport3D.cpp','Common/src/adaptation/CNativeReferenceIO3D.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/src/solvers/CSolver.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp',str(Path(__file__).resolve().relative_to(ROOT))]
 pins={p:digest(ROOT/p) for p in sources}
 pins['build.ninja']=digest(BUILD/'build.ninja');pins['compile_commands.json']=digest(BUILD/'compile_commands.json');pins['test_binary']=digest(BINARY);pins['solver_binary']=digest(BUILD/'SU2_CFD/src/SU2_CFD')
 for library in ['Common/src/libSU2Common.a','Common/src/libSU2AccurateSum.a','SU2_CFD/src/libSU2core.a']:
  path=BUILD/library;pins[library]=digest(path)
  with path.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for member in subprocess.check_output(['ar','t',str(path)],cwd=ROOT,universal_newlines=True).splitlines():
    f=Path(member);f=f if f.is_absolute() else ROOT/f;pins['object:'+str(f)]=digest(f)
 command=['taskset','-c','5','nice','-n','19']
 if ranks>1:command=['nice','-n','19','mpiexec','--bind-to','none','--oversubscribe','-n',str(ranks)]
 command+=[str(BINARY),selection,'--reporter','compact','--use-colour','no']
 env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1',SU2_NATIVE_CURVED_EVIDENCE='1')
 contract={'pins':pins,'command':command,'MPI_ranks':ranks,'environment':{k:env[k] for k in ['OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS','OMPI_MCA_mpi_yield_when_idle','SU2_NATIVE_CURVED_EVIDENCE']}}
 record=HERE/(stage+'.json');folder=HERE/stage;print('Case folder:',folder,flush=True)
 if record.exists():
  old=json.loads(record.read_text())
  if old.get('status')=='PASS' and old.get('contract')==contract and all(digest(folder/f)==sha for f,sha in old['artifacts'].items()):print('Reused PASS',flush=True);return 0
  record.rename(HERE/(stage+'_previous_'+str(time.time_ns())+'.json'))
 if folder.exists():folder.rename(HERE/(folder.name+'_previous_'+str(time.time_ns())))
 folder.mkdir()
 for source in sources:
  out=folder/'source_snapshots'/source;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes((ROOT/source).read_bytes())
 log=folder/'native.log';start=time.monotonic()
 with log.open('w') as out:
  try:code=subprocess.run(command,cwd=folder,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=300).returncode
  except subprocess.TimeoutExpired:code=124
 text=log.read_text();counts=re.findall(r'Passed (?:(?:all|both) )?(\d+) test cases? with ',text);passed=code==0 and counts==[str(expected)]*ranks or (code==0 and expected==1 and 'Passed 1 test case' in text)
 assert all(digest(ROOT/p)==pins[p] for p in sources),'Sources changed during stage'
 assert digest(BINARY)==pins['test_binary'],'Binary changed during stage'
 result={'status':'PASS' if passed else 'FAIL','returncode':code,'contract':contract,'elapsed_seconds':time.monotonic()-start,'artifacts':{str(f.relative_to(folder)):digest(f) for f in folder.rglob('*') if f.is_file()},'stage':stage,'scope':'Focused collective refusal/state preservation, reference admission/chart topology, concurrent first-layer reservations, physical edge bands and density-aware quadrature reuse. Connectivity replay remains private progress only. No full regression, strong-sensor adaptation, CFD accuracy, scaling or affordability claim.'}
 record.write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True);return 0 if passed else 1
if __name__=='__main__':sys.exit(main())
