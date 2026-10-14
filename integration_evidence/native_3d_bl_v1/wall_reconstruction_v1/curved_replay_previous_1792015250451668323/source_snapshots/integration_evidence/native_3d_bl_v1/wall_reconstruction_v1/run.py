#!/usr/bin/env python3
"""Reuse the production provenance collector for bounded serial wall-reconstruction stages."""
import hashlib,json,os,re,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2';BINARY=BUILD/'UnitTests/test_driver'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def main():
 stage=sys.argv[1];assert stage in ("geometry","sensor","sensor_mpi2","sensor_mpi4","curved_field","curved_volume","curved_cold","curved_replay","interacting_volume")
 ranks=2 if stage=="sensor_mpi2" else 4 if stage=="sensor_mpi4" else 1;selection="[NativeWallReconstruction3D],[NativeJointGeometry3D]" if stage=="geometry" else "[NativeFinerSensorBL3D]"
 expected=3 if stage=="geometry" else 1
 if stage.startswith("curved_") or stage=="interacting_volume":
  selection={"curved_field":"[NativeCurvedBL3D]","curved_volume":"[NativeCurvedBLVolume3D]","curved_cold":"[NativeCurvedBLConstruction3D]","curved_replay":"[NativeCurvedBLConstruction3D]","interacting_volume":"[NativeInteractingBLVolume3D]"}[stage]
  expected=4 if stage=="curved_field" else 1
 check=subprocess.run(['/tmp/native-build-tools/ninja','-C',str(BUILD),'-n','UnitTests/test_driver','SU2_CFD/src/SU2_CFD'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,universal_newlines=True,timeout=30)
 assert check.returncode==0 and 'ninja: no work to do.' in check.stdout,check.stdout
 sources=['UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp','Common/include/adaptation/CNativeMetric3D.hpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','Common/src/adaptation/CNativeDistributed3D.cpp','Common/include/adaptation/CNativeField3D.hpp','Common/src/adaptation/CNativeField3D.cpp','Common/src/adaptation/CNativeMetric3D.cpp','Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','Common/include/adaptation/CNativeRemesher3D.hpp','Common/src/adaptation/CNativeRemesher3D.cpp','Common/include/adaptation/CNativeEngine3D.hpp','Common/src/adaptation/CNativeEngine3D.cpp','Common/src/adaptation/CNativeImport3D.cpp','Common/src/adaptation/CNativeReferenceIO3D.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/src/solvers/CSolver.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp',str(Path(__file__).resolve().relative_to(ROOT))]
 sources.append('UnitTests/SU2_CFD/adaptation/fixtures/native_curved_wall_blockage.inc')
 pins={p:digest(ROOT/p) for p in sources}
 pins['build.ninja']=digest(BUILD/'build.ninja');pins['compile_commands.json']=digest(BUILD/'compile_commands.json');pins['test_binary']=digest(BINARY);pins['solver_binary']=digest(BUILD/'SU2_CFD/src/SU2_CFD')
 for library in ['Common/src/libSU2Common.a','Common/src/libSU2AccurateSum.a','SU2_CFD/src/libSU2core.a']:
  path=BUILD/library;pins[library]=digest(path)
  with path.open('rb') as f:thin=f.read(8)==b'!<thin>\n'
  if thin:
   for member in subprocess.check_output(['ar','t',str(path)],cwd=ROOT,universal_newlines=True).splitlines():
    f=Path(member);f=f if f.is_absolute() else ROOT/f;pins['object:'+str(f)]=digest(f)
 command=['taskset','-c','5' if ranks==1 else '7,6,3,2','nice','-n','19']
 if ranks>1:command+=['mpirun','--oversubscribe','--bind-to','none','-np',str(ranks)]
 command+=[str(BINARY),selection,'--reporter','compact']
 env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',OMPI_MCA_mpi_yield_when_idle='1',PYTHONDONTWRITEBYTECODE='1',SU2_NATIVE_CURVED_EVIDENCE='1')
 if stage=='curved_replay':env['SU2_NATIVE_CURVED_REPLAY']='1'
 contract={'pins':pins,'command':command,'MPI_ranks':ranks,'environment':{k:env[k] for k in ['OMP_NUM_THREADS','OPENBLAS_NUM_THREADS','MKL_NUM_THREADS','OMPI_MCA_mpi_yield_when_idle','SU2_NATIVE_CURVED_EVIDENCE']+(['SU2_NATIVE_CURVED_REPLAY'] if stage=='curved_replay' else [])}}
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
 record.write_text(json.dumps({'status':'RUNNING','contract':contract,'collector_pid':os.getpid(),'scope':'Execution intent only; verify actual live process before restart; not a passing receipt.'},indent=2)+'\n')
 with log.open('w') as out:
  try:code=subprocess.run(command,cwd=folder,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=420).returncode
  except subprocess.TimeoutExpired:code=124
 text=log.read_text();counts=re.findall(r'Passed (?:(?:all|both) )?(\d+) test cases? with ',text);passed=code==0 and counts==[str(expected)]*ranks
 assert all(digest(ROOT/p)==pins[p] for p in sources),'Sources changed during stage'
 assert digest(BINARY)==pins['test_binary'],'Binary changed during stage'
 result={'status':'PASS' if passed else 'FAIL','returncode':code,'contract':contract,'elapsed_seconds':time.monotonic()-start,'artifacts':{str(f.relative_to(folder)):digest(f) for f in folder.rglob('*') if f.is_file()},'stage':stage,'scope':('Exact curved cavity shape/reconnection diagnosis only, not full construction/removal or accepted replacement. ' if stage=='curved_replay' else '')+'Selected focused geometry/field/integration control or complete declared cold construction/removal probe. Curved stages use actual smooth reference and original sensors; cold case imports initialized RANS CFD geometry but does not run replacement/solution transfer or CFD iterations. Independent audit and full-goal qualification remain required; no performance claim.'}
 record.write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True);return 0 if passed else 1
if __name__=='__main__':sys.exit(main())
