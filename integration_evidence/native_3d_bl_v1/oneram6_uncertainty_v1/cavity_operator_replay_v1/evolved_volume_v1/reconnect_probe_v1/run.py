#!/usr/bin/env python3
"""Test existing whole-cavity reconnection on the captured seven-tet obstruction."""
import hashlib,json,os,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;BASE=HERE.parent;ROOT=BASE.parents[4];BUILD=ROOT/'integration_evidence/build-integrated-v2'
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
cpp=(BASE/'replay.cpp').read_text().replace('../../../../../','../../../../../../')
a=cpp.index('  for(unsigned trial=0;trial<4;');b=cpp.index('  const auto& s=field.Statistics()',a)
cpp=cpp[:a]+'''  for(const auto& row:nodes){
    if(row.first==edge[0] || row.first==edge[1])continue;
    Patch fresh;std::string reason;const bool geometric=Reconnect(old,row.first,2000000000,*state.original,fresh,reason);
    audit("reconnect_"+std::to_string(row.first),geometric,fresh,reason);
  }
'''+cpp[b:]
(HERE/'replay.cpp').write_text(cpp)
for name in ['donors.dat','edge.dat','working.dat','serial.cfg']:
 p=HERE/name
 if not p.exists():p.symlink_to(os.path.relpath(BASE/'rank0_trial472'/name,HERE))
r=json.loads((BASE/'receipt.json').read_text());compile_cmd,link=r['commands']
for flag,p in [('-c',HERE/'replay.cpp'),('-o',HERE/'replay.o'),('-MF',HERE/'replay.d'),('-MQ',HERE/'replay.o')]:
 if flag in compile_cmd:compile_cmd[compile_cmd.index(flag)+1]=str(p)
link=[str(HERE/'replay.o') if t==str(BASE/'replay.o') else t for t in link];link[link.index('-o')+1]=str(HERE/'replay')
receipt={'hashes':{str(p.relative_to(ROOT)):digest(p) for p in [Path(__file__).resolve(),HERE/'replay.cpp',BASE/'replay.cpp',*[HERE/n for n in ['working.dat','edge.dat','serial.cfg','donors.dat']]]},'commands':[compile_cmd,link],'linked_archives':r['linked_archives']}
env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
for name,args in [('compile',compile_cmd),('link',link)]:
 t=time.monotonic()
 with (HERE/(name+'.log')).open('w') as f:ret=subprocess.run(['nice','-n','19',*args],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
 receipt[name]={'exit':ret.returncode,'seconds':time.monotonic()-t};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 if ret.returncode:raise SystemExit(ret.returncode)
receipt['binary_sha256']=digest(HERE/'replay');source=BASE.parent.parent/'concurrent_composed_proposals_v1/frozen_m6_np4';cmd=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','60s','taskset','-c','0',str(HERE/'replay'),str(source)]
t=time.monotonic()
with (HERE/'run.log').open('w') as f:ret=subprocess.run(cmd,cwd=HERE,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=70)
receipt['run']={'exit':ret.returncode,'seconds':time.monotonic()-t,'command':cmd};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
assert ret.returncode==0
rows=[json.loads(x) for x in (HERE/'trials.jsonl').read_text().splitlines()];assert len([x for x in rows if x['kind']=='trial'])==7
print(json.dumps(rows,indent=2))
