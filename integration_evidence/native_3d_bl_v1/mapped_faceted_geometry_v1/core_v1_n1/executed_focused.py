#!/usr/bin/env python3
"""Bounded geometry-only build/check; qualified support archives reused, integrated binary untouched."""
import gzip,hashlib,json,os,shlex,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent;BUILD=ROOT/'integration_evidence/build-integrated-v2'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  while b:=f.read(4*1024**2):h.update(b)
 return h.hexdigest()
def main():
 folder=HERE/'core_v1_n1';folder.mkdir(exist_ok=True)
 assert not (folder/'result.json').exists(),'Inspect existing terminal receipt; preserve failed case and choose a new version before rebuilding'
 files=('Common/include/adaptation/CNativePointConstruction3D.hpp','Common/src/adaptation/CNativePredicates3D.cpp','Common/include/adaptation/CNativeAffineReference.hpp','Common/src/adaptation/CNativeAffineReference.cpp','UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp')
 pins={p:sha(ROOT/p) for p in files};pins[str(Path(__file__).resolve().relative_to(ROOT))]=sha(Path(__file__).resolve())
 commands=json.loads((BUILD/'compile_commands.json').read_text());steps=[]
 for file in (files[1],files[3],files[4]):
  row=next(r for r in commands if (Path(r['directory'])/r['file']).resolve()==ROOT/file)
  a=row.get('arguments') or shlex.split(row['command']);out=folder/(Path(file).name+'.o')
  for flag,value in (('-o',str(out)),('-MF',str(out)+'.d'),('-MQ',str(out))):
   if flag in a:a[a.index(flag)+1]=value
  steps.append(('compile_'+Path(file).name,a))
 link=shlex.split(subprocess.check_output(['/home/rausa/Software/su2-develop/ninja','-C',str(BUILD),'-t','commands','UnitTests/test_driver'],text=True).splitlines()[-1]);tail=link.index('-Wl,--as-needed')
 executable=folder/'geometry_driver';main=BUILD/'UnitTests/test_driver.p/test_driver.cpp.o'
 steps.append(('link',[link[0],'-o',str(executable)]+[s[1][s[1].index('-o')+1] for s in steps]+[str(main)]+link[tail:]))
 archives=[(BUILD/a).resolve() for a in link[tail:] if a.endswith(('.a','.so')) and (BUILD/a).is_file()]+[main]
 archive_pins={str(p):sha(p) for p in archives};pins['integration_evidence/build-integrated-v2/compile_commands.json']=sha(BUILD/'compile_commands.json')
 capsule={p:(ROOT/p).read_text() for p in files};(folder/'source.json.gz').write_bytes(gzip.compress(json.dumps(capsule,sort_keys=True).encode(),mtime=0))
 env=os.environ.copy();env.update(TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',MALLOC_ARENA_MAX='2')
 receipt=dict(status='RUNNING',source_sha256=pins,support_archive_sha256=archive_pins,steps=[],scope='New geometry code/test objects plus reused qualified support archives; not a current integrated CFD binary or MPI lifecycle qualification.')
 (folder/'process.json').write_text(json.dumps(receipt,indent=2)+'\n')
 steps.append(('geometry_controls',[str(executable),'[NativeAffineGeometry],[NativeAffineSourceGeometry],[NativeMappedFacetGeometry],[NativeParametricGeometry]','--abort','--reporter','compact']))
 for name,command in steps:
  start=time.monotonic();cwd=folder if name=='geometry_controls' else BUILD
  wrapped=['nice','-n','19','timeout','--kill-after=5s','120s']+command
  with (folder/(name+'.log')).open('w') as out:
   p=subprocess.Popen(wrapped,cwd=cwd,env=env,stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
   receipt.update(pid=p.pid,current_step=name);(folder/'process.json').write_text(json.dumps(receipt,indent=2)+'\n');code=p.wait()
  receipt['steps'].append(dict(name=name,command=wrapped,exit_code=code,seconds=time.monotonic()-start,log_sha256=sha(folder/(name+'.log'))))
  print(name,code,receipt['steps'][-1]['seconds'],flush=True)
  if code:receipt['status']='FAIL';break
 else:receipt['status']='PASS'
 assert all(sha(ROOT/p)==h for p,h in pins.items()),'Pinned build sources changed'
 receipt['binary_sha256']=sha(executable) if executable.exists() else None
 (folder/'process.json').write_text(json.dumps(receipt,indent=2)+'\n');(folder/'result.json').write_text(json.dumps(receipt,indent=2)+'\n')
 if receipt['status']!='PASS':raise SystemExit(1)
if __name__=='__main__':main()
