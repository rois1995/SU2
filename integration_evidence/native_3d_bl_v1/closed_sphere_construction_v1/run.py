#!/usr/bin/env python3
"""One bounded same-field MPI4 comparison; independent stage receipts survive interruption."""
import gzip,hashlib,json,os,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
OLD=HERE.parent/'oneram6_uncertainty_v1/strict_geometry_preflight_v1'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def main():
 assert not (HERE/'receipt.json').exists(),'Saved attempt exists; preserve and inspect before a changed version.'
 old=json.loads((OLD/'probe_build.json').read_text());compile_cmd,link=old['commands']
 for flag,path in [('-c',HERE/'construct.cpp'),('-o',HERE/'reassess.o'),('-MF',HERE/'reassess.d'),('-MQ',HERE/'reassess.o')]:
  if flag in compile_cmd:compile_cmd[compile_cmd.index(flag)+1]=str(path)
 link=[str(HERE/'reassess.o') if t==str(OLD/'probe.o') else t for t in link];link[link.index('-o')+1]=str(HERE/'reassess')
 paths=[Path(__file__).resolve(),HERE/'construct.cpp',BUILD/'build.ninja',BUILD/'compile_commands.json']+[HERE/name for name in ['sphere.su2','sphere.su2.native_ref','run.cfg']]
 sources=[p for d in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/d).glob('CNative*3D.*')]+[ROOT/'SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',ROOT/'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp']
 (HERE/'executed_source.json.gz').write_bytes(gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0))
 receipt={'scope':'Complete closed-sphere BL construction under unchanged gates; fixed original identity P1 sensor/actual-query geometric BL; no CFD advance','commands':[compile_cmd,link],'hashes':{str(p.relative_to(ROOT)):sha(p) for p in paths+sources},'linked_archives':{}}
 for token in link:
  if token.endswith('.a'):
   p=Path(token);p=p if p.is_absolute() else BUILD/p;receipt['linked_archives'][str(p)]=sha(p)
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 for name,cmd in [('compile',compile_cmd),('link',link)]:
  start=time.monotonic()
  with (HERE/(name+'.log')).open('w') as f:run=subprocess.run(['nice','-n','19',*cmd],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':run.returncode,'seconds':time.monotonic()-start};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if run.returncode:return run.returncode
 receipt['binary_sha256']=sha(HERE/'reassess')
 command=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','330s','mpirun','--bind-to','none','-np','1','taskset','-c','0',str(HERE/'reassess'),'run.cfg']
 start=time.monotonic();receipt['run']={'status':'running','command':command};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 with (HERE/'run.log').open('w') as f:run=subprocess.run(command,cwd=HERE,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=340)
 receipt['run'].update(status='terminal',exit=run.returncode,seconds=time.monotonic()-start,log_sha256=sha(HERE/'run.log'))
 receipt['stage_hashes']={p.name:sha(p) for p in HERE.glob('*_rank*.json')}
 (HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['run']),flush=True);return run.returncode
if __name__=='__main__':raise SystemExit(main())
