#!/usr/bin/env python3
"""Affected source/import and construction checks; reuse the existing pinned replay launcher."""
import functools,gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
helper=HERE.with_name('oneram6_uncertainty_v1')/'run.py'
spec=importlib.util.spec_from_file_location('replay',helper);replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)
replay.sha=functools.lru_cache(maxsize=None)(replay.sha)
FILTERS={
 'publication':'[NativeMappedPublication3D]',
 'components':'[NativeMappedComponent3D]',
 'domains':'Mapped components release*,Mapped finite components*,Exact ideal finite-component*',
 'reader':'[NativeMappedReader3D]',
 'realized':'[NativeMappedRealized3D]',
 'runtime':'[NativeMappedRuntime3D]',
 'runtime_all':'[NativeMappedRuntime3D]',
 'annulus':'[NativeSymmetryRuntime3D]',
 'rejection':'[NativeRejectedState3D]',
 'affected':'[NativeMappedComponent3D],[NativeMappedFacetGeometry],[NativeParametricGeometry],[NativeAffineGeometry],[NativeAffineImport3D]',
 'compatibility':'[NativeSymmetryRuntime3D]'}
import os,subprocess,time
def sampled_cpus(count):
 def sample():
  return {int(s[3:]):list(map(int,v)) for s,*v in (line.split() for line in Path('/proc/stat').read_text().splitlines()) if s.startswith('cpu') and s[3:].isdigit()}
 before=sample();time.sleep(.25);after=sample()
 def busy(cpu):
  d=[y-x for x,y in zip(before[cpu],after[cpu])];total=sum(d[:8]);return 1-(sum(d[3:5])/total) if total else 0
 allowed=os.sched_getaffinity(0);assert len(allowed)>=count
 return sorted(sorted(allowed,key=busy)[:count])
def invoke(folder,env,fixture,ranks=1,test_filter='[NativeFacetedVolumeReplay3D]',expected_failure=False):
 assert ranks in (1,2,4)
 folder.mkdir(parents=True,exist_ok=True)
 identity={'pins':replay.pins(),'fixture':replay.sha(fixture),'parameters':env,'ranks':ranks,'test_filter':test_filter,'expected_failure':expected_failure}
 proof=folder/'provenance.json'
 if proof.exists():assert json.loads(proof.read_text())==identity,'Provenance changed: '+str(folder)
 else:proof.write_text(json.dumps(identity,indent=2)+'\n')
 result=folder/'result.json'
 if result.exists():
  saved=json.loads(result.read_text())
  assert saved['provenance_sha256']==replay.sha(proof) and saved['log_sha256']==replay.sha(folder/'run.log')
  assert (0<saved['exit_code']<124) if expected_failure else saved['exit_code']==0
  if 'normalization_sha256' in saved:assert saved['normalization_sha256']==replay.sha(folder/'normalization.json')
  print('REUSE',folder,flush=True);return saved
 if (folder/'run.log').exists():raise RuntimeError('Unfinished run: inspect its process handle and preserve evidence before retrying '+str(folder))
 environment=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
                  TMPDIR=str(replay.BUILD/'compile-tmp'),SU2_NATIVE_BL_REPLAY=str(fixture),**env)
 base=[str(replay.BINARY),test_filter,'--abort','--reporter','compact']
 if ranks>1:base=['mpirun','-np',str(ranks),'--bind-to','none']+base
 command=['taskset','-c',','.join(str(cpu) for cpu in sampled_cpus(ranks)),'nice','-n','19','timeout','--kill-after=5s','180s']+base
 start=time.monotonic()
 with (folder/'run.log').open('w') as log:
  process=subprocess.Popen(command,cwd=str(folder),env=environment,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
  (folder/'process.json').write_text(json.dumps(dict(pid=process.pid,status='RUNNING',command=command),indent=2)+'\n')
  code=process.wait()
  run=subprocess.CompletedProcess(command,code)
 (folder/'process.json').write_text(json.dumps(dict(pid=process.pid,status='TERMINAL',exit_code=code,command=command),indent=2)+'\n')
 report={'exit_code':run.returncode,'elapsed_seconds':time.monotonic()-start,'command':command,
         'provenance_sha256':replay.sha(proof),'log_sha256':replay.sha(folder/'run.log')}
 if (folder/'complexity_estimate.json').exists():report['estimate']=json.loads((folder/'complexity_estimate.json').read_text())
 if (folder/'normalization.json').exists():
  report['normalization']=json.loads((folder/'normalization.json').read_text());report['normalization_sha256']=replay.sha(folder/'normalization.json')
 (folder/'result.json').write_text(json.dumps(report,indent=2)+'\n')
 print(folder.name,json.dumps(report.get('estimate',report)),flush=True)
 if not ((0<run.returncode<124) if expected_failure else run.returncode==0):raise RuntimeError('Failed '+str(folder))
 return report
def main():
 receipt=json.loads((HERE/'build_process.json').read_text())
 assert receipt['status']=='PASS' and receipt['exit_code']==0 and receipt['source_unchanged'],'Linked build must finish before checks'
 assert receipt['binary_sha256']==replay.sha(replay.BINARY),'Executable changed'
 assert all(replay.sha(ROOT/p)==h for p,h in receipt['source_sha256'].items()),'Compiled sources changed'
 mode=sys.argv[1] if len(sys.argv)>1 else 'publication';ranks=int(sys.argv[2]) if len(sys.argv)>2 else 1
 assert mode in FILTERS and ranks in (1,2,4)
 version=sys.argv[3] if len(sys.argv)>3 else ''
 assert not version or (version.startswith('v') and version[1:].isdigit())
 folder=HERE/(mode+('_'+version if version else '')+'_n'+str(ranks));folder.mkdir(exist_ok=True)
 sources=[ROOT/p for p in receipt['source_sha256']]+[Path(__file__).resolve(),helper,
  ROOT/'UnitTests/SU2_CFD/adaptation/TransferTestCase.hpp',ROOT/'Common/include/adaptation/CNativeField3D.hpp',
  ROOT/'Common/src/adaptation/CNativeField3D.cpp',ROOT/'Common/include/adaptation/CNativeDonor3D.hpp',ROOT/'Common/src/adaptation/CNativeDonor3D.cpp',
  ROOT/'Common/include/adaptation/CNativePartition3D.hpp',ROOT/'Common/src/adaptation/CNativePartition3D.cpp']
 sources+=list((ROOT/'UnitTests/SU2_CFD/adaptation/fixtures').glob('*.inc'))
 sources+=[ROOT/p for p in ('Common/include/adaptation/CNativeMesh3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp',
  'Common/include/adaptation/CNativeCavity3D.hpp','Common/src/adaptation/CNativeCavity3D.cpp',
  'Common/include/adaptation/CNativeTopology3D.hpp','Common/src/adaptation/CNativeTopology3D.cpp')]
 source_pins={str(p.relative_to(ROOT)):replay.sha(p) for p in sorted(set(sources))}
 previous=replay.pins;replay.pins=lambda:dict(previous(),**source_pins)
 payload=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sorted(set(sources))},sort_keys=True).encode(),mtime=0)
 capsule=folder/'executed_source.json.gz'
 if capsule.exists():assert capsule.read_bytes()==payload,'Source changed: preserve this folder and select a new version'
 else:capsule.write_bytes(payload)
 fixture=HERE.with_name('affine_initial_adoption_v1')/'initial.planes'
 parameters={'SU2_NATIVE_MAPPED_COMPONENTS':'1'} if mode not in ('compatibility','realized','rejection') else {}
 if mode=='compatibility':parameters['SU2_NATIVE_SYMMETRY_ANNULUS']='1'
 if mode=='runtime_all':parameters['SU2_NATIVE_MAPPED_ALL_WORKERS']='1'
 if mode=='annulus':parameters.update(SU2_NATIVE_SYMMETRY_ANNULUS='1',SU2_NATIVE_SYMMETRY_OBLIQUE='1',SU2_NATIVE_SYMMETRY_MAPPED='1')
 result=invoke(folder,parameters,fixture,ranks,test_filter=FILTERS[mode])
 replay.sha.cache_clear()
 unchanged=receipt['binary_sha256']==replay.sha(replay.BINARY) and all(replay.sha(ROOT/p)==h for p,h in receipt['source_sha256'].items())
 text=(folder/'run.log').read_text(errors='replace')
 matched=not any(message in text for message in ('No test cases matched','No tests ran'))
 result.update(compiled_sources_unchanged=unchanged,test_filter_matched=matched)
 (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 assert unchanged and matched,'Current-source mismatch or empty test selection'
 artifacts={p.name:replay.sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('result.json','run.log','provenance.json','executed_source.json.gz')}
 if 'artifacts' in result:assert result['artifacts']==artifacts,'Saved case artifacts changed'
 else:
  result['artifacts']=artifacts;(folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS',mode,ranks,folder,flush=True)
if __name__=='__main__':main()
