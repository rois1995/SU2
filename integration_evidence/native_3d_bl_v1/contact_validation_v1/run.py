#!/usr/bin/env python3
"""Bounded contact-only profiling on sealed accepted meshes; no CFD/remeshing."""
import gzip,hashlib,json,os,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
OLD=HERE.parent/'closed_sphere_construction_v1/geodesic_v1'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def main():
 assert not (HERE/'receipt.json').exists(),'Preserve existing experiment before a changed version'
 old=json.loads((OLD/'receipt.json').read_text());compile_cmd,link=old['commands']
 for flag,path in [('-c',HERE/'measure.cpp'),('-o',HERE/'measure.o'),('-MF',HERE/'measure.d'),('-MQ',HERE/'measure.o')]:
  if flag in compile_cmd:compile_cmd[compile_cmd.index(flag)+1]=str(path)
 link=[str(HERE/'measure.o') if t==str(OLD/'reassess.o') else t for t in link];link[link.index('-o')+1]=str(HERE/'measure')
 paths=[Path(__file__).resolve(),HERE/'measure.cpp',ROOT/'Common/src/adaptation/CNativePredicates3D.cpp',ROOT/'Common/include/adaptation/CNativeMesh3D.hpp',BUILD/'build.ninja',BUILD/'compile_commands.json']
 receipt={'hashes':{str(p.relative_to(ROOT)):sha(p) for p in paths},'commands':[compile_cmd,link],'archives':{}}
 (HERE/'predicate_executed.cpp').write_bytes((ROOT/'Common/src/adaptation/CNativePredicates3D.cpp').read_bytes())
 for token in link:
  if token.endswith('.a'):
   p=Path(token);p=p if p.is_absolute() else BUILD/p;receipt['archives'][str(p)]=sha(p)
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 for name,cmd in [('compile',compile_cmd),('link',link)]:
  start=time.monotonic()
  with (HERE/(name+'.log')).open('w') as f:r=subprocess.run(['nice','-n','19',*cmd],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':r.returncode,'seconds':time.monotonic()-start};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if r.returncode:return r.returncode
 receipt['binary_sha256']=sha(HERE/'measure');receipt['cases']=[]
 for name,source,mode in [('released_measure',OLD.parent/'release_regional_v1/accepted.dat','measure'),('BL_classify',OLD/'accepted.dat','classify')]:
  cmd=['/usr/bin/time','-v','nice','-n','19','taskset','-c','0','timeout','--kill-after=5s','100s',str(HERE/'measure'),str(source),mode]
  start=time.monotonic()
  with (HERE/(name+'.json')).open('w') as out,(HERE/(name+'.log')).open('w') as log:r=subprocess.run(cmd,stdout=out,stderr=log,env=env,timeout=110)
  record={'name':name,'input':str(source.relative_to(ROOT)),'input_sha256':sha(source),'exit':r.returncode,'seconds':time.monotonic()-start,'command':cmd,'stdout_sha256':sha(HERE/(name+'.json')),'stderr_sha256':sha(HERE/(name+'.log'))}
  receipt['cases'].append(record);(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(name,r.returncode,flush=True)
  if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
