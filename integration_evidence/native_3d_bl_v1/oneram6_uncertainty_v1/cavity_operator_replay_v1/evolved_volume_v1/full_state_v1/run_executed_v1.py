#!/usr/bin/env python3
"""One bounded MPI4 frozen event using the existing backend, with final-state retention."""
import hashlib,json,os,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.parent.parent;ROOT=BASE.parents[3];BUILD=ROOT/'integration_evidence/build-integrated-v2';SOURCE=BASE.parent/'concurrent_composed_proposals_v1/frozen_m6_np4'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def main():
 r=json.loads((BASE/'actual_context_np4/receipt.json').read_text());compile_cmd,link=r['commands']
 for flag,p in [('-c',HERE/'context.cpp'),('-o',HERE/'context.o'),('-MF',HERE/'context.d'),('-MQ',HERE/'context.o')]:
  if flag in compile_cmd:compile_cmd[compile_cmd.index(flag)+1]=str(p)
 link=[str(HERE/'context.o') if t==str(BASE/'context.o') else t for t in link];link[link.index('-o')+1]=str(HERE/'context')
 for name in ['Coarse.su2','Coarse.su2.native_ref','donor_flow.dat','selected_sensor.bin','run.cfg']:
  p=HERE/name
  if not p.exists():p.symlink_to(os.path.relpath(SOURCE/name,HERE))
 receipt={'scope':'Frozen MPI4 M6 event; rejected-state snapshot is diagnostic, never an accepted CFD mesh','commands':[compile_cmd,link],'hashes':{},'linked_archives':{}}
 pins=[Path(__file__).resolve(),HERE/'prepare.py',HERE/'context.cpp',BASE/'context.cpp',BUILD/'build.ninja',BUILD/'compile_commands.json',ROOT/'Common/include/adaptation/CNativeEngine3D.hpp',ROOT/'Common/src/adaptation/CNativeEngine3D.cpp']+[HERE/n for n in ['Coarse.su2','Coarse.su2.native_ref','selected_sensor.bin','run.cfg','donor_flow.dat']]
 receipt['hashes']={str(p.relative_to(ROOT)):digest(p) for p in pins}
 for token in link:
  if token.endswith('.a'):
   p=Path(token);p=p if p.is_absolute() else BUILD/p;receipt['linked_archives'][str(p)]=digest(p)
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 for name,args in [('compile',compile_cmd),('link',link)]:
  t=time.monotonic()
  with (HERE/(name+'.log')).open('w') as f:ret=subprocess.run(['nice','-n','19',*args],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':ret.returncode,'seconds':time.monotonic()-t};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if ret.returncode:return ret.returncode
 receipt['binary_sha256']=digest(HERE/'context');cmd=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','420s','mpirun','--bind-to','none','-np','4','taskset','-c','0-3',str(HERE/'context'),str(HERE/'run.cfg'),str(SOURCE/'selected_sensor.bin')]
 t=time.monotonic()
 with (HERE/'run.log').open('w') as f:ret=subprocess.run(cmd,cwd=HERE,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=450)
 receipt['run']={'exit':ret.returncode,'seconds':time.monotonic()-t,'command':cmd};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print(json.dumps(receipt['run'],indent=2));return ret.returncode
if __name__=='__main__':raise SystemExit(main())
