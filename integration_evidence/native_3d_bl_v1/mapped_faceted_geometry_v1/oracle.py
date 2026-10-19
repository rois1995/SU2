#!/usr/bin/env python3
"""Exact-rational oracle for compiled unrounded-parameter bounds and orientations."""
import hashlib,json,os,shlex,struct,subprocess,time
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 folder=HERE/'oracle_v1';folder.mkdir(exist_ok=True)
 assert not (folder/'result.json').exists(),'Inspect/reuse existing receipt before rebuilding'
 old=json.loads((HERE/'core_v1_n1/result.json').read_text());assert old['status']=='PASS'
 for p,h in old['source_sha256'].items():assert sha(ROOT/p)==h
 row=next(r for r in json.loads((BUILD/'compile_commands.json').read_text()) if r['file'].endswith('CNativePredicates3D.cpp'))
 cmd=row.get('arguments') or shlex.split(row['command']);obj=folder/'probe.o'
 for flag,value in (('-o',str(obj)),('-MF',str(obj)+'.d'),('-MQ',str(obj))):
  if flag in cmd:cmd[cmd.index(flag)+1]=value
 cmd[cmd.index('-c')+1]=str(HERE/'probe.cpp');cmd.insert(1,'-I'+str(ROOT))
 link=next(s['command'] for s in old['steps'] if s['name']=='link');tail=link.index('-Wl,--as-needed');executable=folder/'probe'
 link=['c++','-o',str(executable),str(obj),str(HERE/'core_v1_n1/CNativePredicates3D.cpp.o'),str(HERE/'core_v1_n1/CNativeAffineReference.cpp.o')]+link[tail:]
 env=os.environ.copy();env.update(TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1')
 steps=[]
 for name,args,cwd in [('compile',cmd,BUILD),('link',link,BUILD),('probe',[str(executable)],folder)]:
  start=time.monotonic()
  with (folder/(name+'.log')).open('w') as out:
   code=subprocess.run(['nice','-n','19','timeout','120s']+args,cwd=cwd,env=env,stdout=out,stderr=subprocess.STDOUT).returncode
  steps.append(dict(name=name,exit_code=code,seconds=time.monotonic()-start,command=args))
  (folder/'process.json').write_text(json.dumps(dict(steps=steps),indent=2)+'\n')
  assert code==0,(name,code)
 rows=json.loads((folder/'probe.log').read_text());assert len(rows)==40
 nonzero=collinear=0
 def next_up(x):
  if x==0:return struct.unpack('<d',struct.pack('<Q',1))[0]
  bits=struct.unpack('<Q',struct.pack('<d',x))[0];return struct.unpack('<d',struct.pack('<Q',bits+(-1 if x<0 else 1)))[0]
 for row in rows:
  a,b,c=(list(map(F,row[k])) for k in ('origin','first','second'));u,v=F(row['u']),F(row['v']);p=[a[k]+u*(b[k]-a[k])+v*(c[k]-a[k]) for k in range(3)]
  for k in range(3):
   lo,hi=(r[k] for r in row['bounds']);assert F(lo)<=p[k]<=F(hi)
   assert lo==hi and F(lo)==p[k] or next_up(lo)==hi
  for axis,(i,j) in enumerate(((1,2),(0,2),(0,1))):
   det=(b[i]-a[i])*(p[j]-a[j])-(b[j]-a[j])*(p[i]-a[i]);sign=(det>0)-(det<0)
   assert row['orientation'][axis]==sign
   collinear+=sign==0;nonzero+=sign!=0
 assert nonzero and collinear
 result=dict(status='EXACT_RATIONAL_PARAMETRIC_ORACLE_PASS',samples=len(rows),coordinate_bounds=3*len(rows),nonzero_orientation=nonzero,exact_zero_orientation=collinear,steps=steps,
  new_source_sha256=old['source_sha256'],support_archive_sha256=old['support_archive_sha256'],probe_sha256=sha(HERE/'probe.cpp'),oracle_sha256=sha(Path(__file__)),binary_sha256=sha(executable),data_sha256=sha(folder/'probe.log'),
  scope='New exact-parameter arithmetic at five scales and both edge/facet parameters; not a new mapped mesh, integration, transfer, MPI or restart acceptance.')
 (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],len(rows),nonzero,collinear)
if __name__=='__main__':main()
