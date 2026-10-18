#!/usr/bin/env python3
"""Compile one small native replay against the existing qualified build; no CFD run."""
import hashlib,json,os,re,shlex,subprocess,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
CONTEXT="--context" in sys.argv
CPP=HERE/("context.cpp" if CONTEXT else "replay.cpp")
EXE=HERE/("context" if CONTEXT else "replay")
OUT=HERE/"actual_context_np4" if CONTEXT else HERE
ROOT=HERE.parents[3]
BUILD=ROOT/'integration_evidence/build-integrated-v2'
SOURCE=HERE.parent/'concurrent_composed_proposals_v1/frozen_m6_np4'
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2

def digest(path):
 h=hashlib.sha256()
 with Path(path).open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()

def main():
 cfg=(SOURCE/'run.cfg').read_text()
 cfg,n=re.subn(r'(?m)^ADAP_NATIVE_RANKS\s*=.*$', 'ADAP_NATIVE_RANKS= 1',cfg)
 assert n==1
 (HERE/'serial.cfg').write_text(cfg)
 mesh=read_su2(SOURCE/'Coarse.su2')
 w=json.loads((SOURCE.parent/'original_edge_witness/witness.json').read_text())
 p=mesh.P[w['original_nodes']];lo=p.min(0);hi=p.max(0);tet=mesh.P[mesh.E]
 ids=np.flatnonzero(np.all((tet.min(1)<=hi)&(tet.max(1)>=lo),axis=1))
 assert len(ids)==169 and set(w['original_cells'])<=set(ids)
 dtype=np.dtype([('id','<u8'),('x','<f8',(3,)),('m','<f8',(9,))])
 data=np.fromfile(SOURCE/'selected_sensor.bin',dtype=dtype,offset=8)
 assert np.array_equal(data['id'],np.arange(len(mesh.P))) and np.array_equal(data['x'],mesh.P)
 with (HERE/'donors.dat').open('w') as f:
  f.write(f'ORIGINAL_CAVITY_V1 {len(ids)}\n')
  for i in ids:
   f.write(f'{i} {int(i in w["original_cells"])}\n')
   for n in mesh.E[i]:
    vals=list(data['x'][n])+list(data['m'][n][[0,1,2,4,5,8]])
    f.write(str(n)+' '+' '.join(format(float(x),'.17g') for x in vals)+'\n')
 commands=json.loads((BUILD/'compile_commands.json').read_text())
 entry=next(c for c in commands if c['file'].endswith('CNativeRemesher3D_tests.cpp'))
 cmd=shlex.split(entry['command'])
 for flag,value in [('-c',str(CPP)),('-o',str(CPP.with_suffix('.o'))),('-MF',str(CPP.with_suffix('.d'))),('-MQ',str(CPP.with_suffix('.o')))]:
  if flag in cmd:cmd[cmd.index(flag)+1]=value
 ninja='/home/rausa/Software/su2-develop/ninja'
 lines=subprocess.check_output([ninja,'-C',str(BUILD),'-t','commands','UnitTests/test_driver'],text=True).splitlines()
 link=[x for x in shlex.split(lines[-1]) if not x.endswith('.o')]
 link[link.index('-o')+1]=str(EXE);link.insert(1,str(CPP.with_suffix('.o')))
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 receipt={'scope':('Bounded actual M6 private proposal contexts; no complete mesh certificate' if CONTEXT else 'Original five-cell witness, not late rejected cavity; fixed artificial boundary'),
          'commands':[cmd,link],'input_hashes':{},'linked_archives':{}}
 for p in [Path(__file__).resolve(),CPP,HERE/'donors.dat',HERE/'serial.cfg',SOURCE/'Coarse.su2',SOURCE/'Coarse.su2.native_ref',SOURCE/'selected_sensor.bin',SOURCE/'run.cfg',BUILD/'compile_commands.json',BUILD/'build.ninja']:
  receipt['input_hashes'][str(p.relative_to(ROOT))]=digest(p)
 for token in link:
  if token.endswith('.a'):
   p=Path(token);p=p if p.is_absolute() else BUILD/p;receipt['linked_archives'][str(p)]=digest(p)
 OUT.mkdir(exist_ok=True)
 for name,args in [('compile',cmd),('link',link)]:
  t=time.monotonic()
  with (OUT/(name+'.log')).open('w') as f:
   r=subprocess.run(['nice','-n','19',*args],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':r.returncode,'seconds':time.monotonic()-t}
  (OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if r.returncode:return r.returncode
 receipt['binary_sha256']=digest(EXE)
 t=time.monotonic()
 if CONTEXT:
  for name in ['Coarse.su2','Coarse.su2.native_ref','donor_flow.dat','selected_sensor.bin','run.cfg']:
   dest=OUT/name
   if not dest.exists():dest.symlink_to(os.path.relpath(SOURCE/name,OUT))
  args=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','420s','mpirun','--bind-to','none','-np','4','taskset','-c','0-3',str(EXE),str(OUT/'run.cfg'),str(SOURCE/'selected_sensor.bin')]
 else:args=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','110s','taskset','-c','0',str(EXE),str(SOURCE)]
 receipt['run_command']=args
 with (OUT/'run.log').open('w') as f:
  r=subprocess.run(args,cwd=OUT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=450 if CONTEXT else 120)
 receipt['run']={'exit':r.returncode,'seconds':time.monotonic()-t}
 (OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 if r.returncode:return r.returncode
 if CONTEXT:
  print(json.dumps({'run':receipt['run'],'folder':str(OUT)},indent=2));return 0
 trials=[json.loads(x) for x in (HERE/'trials.jsonl').read_text().splitlines()]
 assert trials[0]['donors']==169 and abs(trials[0]['edge_length']-2.6088200816802036)<1e-12
 assert trials[-1]['kind']=='work'
 print(json.dumps({'trials':trials,'run':receipt['run']},indent=2));return 0
if __name__=='__main__':sys.exit(main())
