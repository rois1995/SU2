#!/usr/bin/env python3
"""Compile one small native replay against the existing qualified build; no CFD run."""
import hashlib,json,os,re,shlex,subprocess,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
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
 for flag,value in [('-c',str(HERE/'replay.cpp')),('-o',str(HERE/'replay.o')),('-MF',str(HERE/'replay.d')),('-MQ',str(HERE/'replay.o'))]:
  if flag in cmd:cmd[cmd.index(flag)+1]=value
 ninja='/home/rausa/Software/su2-develop/ninja'
 lines=subprocess.check_output([ninja,'-C',str(BUILD),'-t','commands','UnitTests/test_driver'],text=True).splitlines()
 link=[x for x in shlex.split(lines[-1]) if not x.endswith('.o')]
 link[link.index('-o')+1]=str(HERE/'replay');link.insert(1,str(HERE/'replay.o'))
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 receipt={'scope':'Original five-cell witness, not late rejected cavity; fixed artificial boundary',
          'commands':[cmd,link],'input_hashes':{},'linked_archives':{}}
 for p in [Path(__file__).resolve(),HERE/'replay.cpp',HERE/'donors.dat',HERE/'serial.cfg',SOURCE/'Coarse.su2',SOURCE/'Coarse.su2.native_ref',SOURCE/'selected_sensor.bin',SOURCE/'run.cfg',BUILD/'compile_commands.json',BUILD/'build.ninja']:
  receipt['input_hashes'][str(p.relative_to(ROOT))]=digest(p)
 for token in link:
  if token.endswith('.a'):
   p=Path(token);p=p if p.is_absolute() else BUILD/p;receipt['linked_archives'][str(p)]=digest(p)
 for name,args in [('compile',cmd),('link',link)]:
  t=time.monotonic()
  with (HERE/(name+'.log')).open('w') as f:
   r=subprocess.run(['nice','-n','19',*args],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':r.returncode,'seconds':time.monotonic()-t}
  (HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if r.returncode:return r.returncode
 receipt['binary_sha256']=digest(HERE/'replay')
 t=time.monotonic()
 with (HERE/'run.log').open('w') as f:
  r=subprocess.run(['/usr/bin/time','-v','nice','-n','19','taskset','-c','0',str(HERE/'replay'),str(SOURCE)],cwd=HERE,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=120)
 receipt['run']={'exit':r.returncode,'seconds':time.monotonic()-t}
 (HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 if r.returncode:return r.returncode
 trials=[json.loads(x) for x in (HERE/'trials.jsonl').read_text().splitlines()]
 assert trials[0]['donors']==169 and abs(trials[0]['edge_length']-2.6088200816802036)<1e-12
 assert trials[-1]['kind']=='work'
 print(json.dumps({'trials':trials,'run':receipt['run']},indent=2));return 0
if __name__=='__main__':sys.exit(main())
