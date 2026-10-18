#!/usr/bin/env python3
"""Build once and run resumable serial/MPI2/MPI4 exact admitted-feature diagnosis checks."""
import hashlib,json,os,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];BUILD=ROOT/'integration_evidence/build-integrated-v2'
OLD=HERE.parent/'cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1'
SOURCE=HERE.parent/'concurrent_composed_proposals_v1/frozen_m6_np4'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def main():
 assert not (HERE/'probe_build.json').exists(),'Preserve this completed/failed build; inspect it or use a fresh versioned folder.'
 r=json.loads((OLD/'receipt.json').read_text());compile_cmd,link=r['commands']
 for flag,p in [('-c',HERE/'probe.cpp'),('-o',HERE/'probe.o'),('-MF',HERE/'probe.d'),('-MQ',HERE/'probe.o')]:
  if flag in compile_cmd:compile_cmd[compile_cmd.index(flag)+1]=str(p)
 link=[str(HERE/'probe.o') if t==str(OLD/'replay.o') else t for t in link];link[link.index('-o')+1]=str(HERE/'probe')
 pins=[Path(__file__).resolve(),HERE/'prepare_probe.py',HERE/'probe.cpp',SOURCE/'selected_sensor.bin',SOURCE/'Coarse.su2.native_ref',ROOT/'Common/src/adaptation/CNativePredicates3D.cpp',ROOT/'Common/src/adaptation/CNativeEngine3D.cpp',ROOT/'Common/include/adaptation/CNativePointConstruction3D.hpp',BUILD/'build.ninja',BUILD/'compile_commands.json']+[HERE/n for n in ['working.dat','edge.dat','serial.cfg','donors.dat']]
 receipt={'commands':[compile_cmd,link],'hashes':{str(p.relative_to(ROOT)):digest(p) for p in pins},'linked_archives':{}}
 for t in link:
  if t.endswith('.a'):
   p=Path(t);p=p if p.is_absolute() else BUILD/p;receipt['linked_archives'][str(p)]=digest(p)
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 for name,args in [('compile',compile_cmd),('link',link)]:
  start=time.monotonic()
  with (HERE/(name+'.log')).open('w') as f:r=subprocess.run(['nice','-n','19',*args],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':r.returncode,'seconds':time.monotonic()-start};(HERE/'probe_build.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if r.returncode:return r.returncode
 receipt['binary_sha256']=digest(HERE/'probe');(HERE/'probe_build.json').write_text(json.dumps(receipt,indent=2)+'\n')
 for ranks in [1,2,4]:
  folder=HERE/f'probe_np{ranks}';folder.mkdir(exist_ok=True)
  assert not (folder/'result.json').exists(),'Existing completed result must be verified before reuse.'
  for name in ['working.dat','edge.dat','serial.cfg','donors.dat']:
   dest=folder/name
   if not dest.exists():dest.symlink_to('../'+name)
  cmd=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','60s']
  if ranks>1:cmd+=['mpirun','--bind-to','none','-np',str(ranks)]
  cmd+=['taskset','-c',f'0-{ranks-1}' if ranks>1 else '0',str(HERE/'probe'),str(SOURCE)]
  start=time.monotonic()
  with (folder/'run.log').open('w') as f:r=subprocess.run(cmd,cwd=folder,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=70)
  result={'exit':r.returncode,'seconds':time.monotonic()-start,'command':cmd,'build_sha256':digest(HERE/'probe_build.json'),'log_sha256':digest(folder/'run.log')}
  result['ranks']=[json.loads(p.read_text()) for p in sorted(folder.glob('result_rank*.json'))]
  (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
  if r.returncode:return r.returncode
  assert len(result['ranks'])==ranks and all(x['immutable_input'] and 'Native3D exact-facet resolution conflict' in x['reason'] for x in result['ranks'])
 return 0
if __name__=='__main__':raise SystemExit(main())
