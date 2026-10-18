#!/usr/bin/env python3
"""Bounded local replay of exact captured volume stars using the existing native APIs."""
import hashlib,json,os,shlex,subprocess,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
BASE=HERE.parent
ROOT=BASE.parents[3]
BUILD=ROOT/'integration_evidence/build-integrated-v2'
SOURCE=BASE.parent/'concurrent_composed_proposals_v1/frozen_m6_np4'
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
sys.path.insert(0,str(BASE))
from diagnose_contexts import patch

def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()

def main():
 # Preserve the previous source and all its receipts; derive only this new diagnostic.
 cpp=(BASE/'replay.cpp').read_text().replace('../../../../','../../../../../')
 def replace(old,new):
  nonlocal cpp
  assert cpp.count(old)==1,old
  cpp=cpp.replace(old,new)
 replace('// Diagnostic only: original input cavity, not captured late connectivity or a global mesh certificate.', '// Diagnostic only: actual captured volume cavities, not a whole mesh certificate.')
 replace('}require(old.cells.size()==5&&donors.size()==169,"Changed witness support");', '''}
  std::ifstream working("working.dat");size_t nc,nf;working>>tag>>nc>>nf;
  require(tag=="NATIVE_PRIVATE_PATCH_V1"&&nc>0&&nc<=MaximumCavityCells&&nf==0,"Expected captured interior cavity");
  old={};nodes.clear();for(size_t k=0;k<nc;++k){Cell c;working>>c.id;for(auto& n:c.v){working>>n.id>>n.p.x>>n.p.y>>n.p.z;nodes.emplace(n.id,n);}old.cells.push_back(c);}
  require(bool(working)&&nodes.rbegin()->first<1000000000,"Invalid captured node range");
  Edge edge;std::ifstream edgeFile("edge.dat");edgeFile>>edge[0]>>edge[1];require(bool(edgeFile),"Missing captured feature");''')
 replace('FrozenField field(std::move(donors),compose,true,{},bands);const Edge edge{25389,53318};', 'FrozenField field(std::move(donors),compose,true,{},bands);')
 begin=cpp.index('  require(std::abs(length-2.6088200816802036L)')
 end=cpp.index('  std::ofstream report',begin)
 cpp=cpp[:begin]+cpp[end:]
 replace('valid=ValidateMetricChange(old.cells,p.cells,MetricChange::Refine,field,m,reason,nullptr,nullptr,&diag);', 'm=MeasureMetricCells(p.cells,field,nullptr,&before.edges);valid=ValidateMetricChange(old.cells.size(),before,p.cells.size(),m,MetricChange::Refine,reason,&diag);')
 replace('<<joint.flips<<"}\\n";', '<<joint.flips<<",\\"connectivity_trials\\":"<<joint.connectivity_trials<<",\\"best_shape_loss\\":"<<joint.best_shape_loss<<",\\"blocked_vertex\\":"<<joint.blocked_interior_vertex<<"}\\n";')
 begin=cpp.index('  {const Node point{1000000000,field.BalancedSplitPoint(a,b)},interior')
 end=cpp.index('  const auto& s=field.Statistics()',begin)
 cpp=cpp[:begin]+cpp[end:]
 (HERE/'replay.cpp').write_text(cpp)
 r=json.loads((BASE/'receipt.json').read_text());compile_cmd,link=r['commands']
 for flag,p in [('-c',HERE/'replay.cpp'),('-o',HERE/'replay.o'),('-MF',HERE/'replay.d'),('-MQ',HERE/'replay.o')]:
  if flag in compile_cmd:compile_cmd[compile_cmd.index(flag)+1]=str(p)
 link=[str(HERE/'replay.o') if t==str(BASE/'replay.o') else t for t in link];link[link.index('-o')+1]=str(HERE/'replay')
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 receipt={'scope':'Ten exact captured volume stars; no physical boundary edits or global acceptance claim','commands':[compile_cmd,link],'hashes':{},'cases':[]}
 for p in [Path(__file__).resolve(),BASE/'replay.cpp',BASE/'context_summary.json',SOURCE/'selected_sensor.bin',SOURCE/'Coarse.su2',SOURCE/'Coarse.su2.native_ref',HERE/'replay.cpp',BUILD/'build.ninja',BUILD/'compile_commands.json']:
  receipt['hashes'][str(p.relative_to(ROOT))]=digest(p)
 receipt['linked_archives']=r['linked_archives']
 for p,expected in receipt['linked_archives'].items():assert digest(Path(p))==expected,p
 for name,args in [('compile',compile_cmd),('link',link)]:
  t=time.monotonic()
  with (HERE/(name+'.log')).open('w') as f:ret=subprocess.run(['nice','-n','19',*args],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':ret.returncode,'seconds':time.monotonic()-t};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if ret.returncode:return ret.returncode
 receipt['binary_sha256']=digest(HERE/'replay')
 mesh=read_su2(SOURCE/'Coarse.su2');points=mesh.P[mesh.E];a=points.min(1);b=points.max(1)
 dtype=np.dtype([('id','<u8'),('x','<f8',(3,)),('m','<f8',(9,))]);data=np.fromfile(SOURCE/'selected_sensor.bin',dtype=dtype,offset=8)
 assert np.array_equal(data['x'],mesh.P) and np.array_equal(data['id'],np.arange(len(mesh.P)))
 rows=json.loads((BASE/'context_summary.json').read_text())['contexts'];rows=[r for r in rows if r['stem'].startswith('rank0_')]
 assert len(rows)==10
 for row in rows:
  folder=HERE/row['stem'];folder.mkdir(exist_ok=True);original=BASE/'actual_context_np4'/(row['stem']+'_old.dat');cells,nodes,faces=patch(original);assert not faces
  xyz=np.array(list(nodes.values()));lo=xyz.min(0);hi=xyz.max(0);donors=np.flatnonzero(np.all((a<=hi)&(b>=lo),axis=1));assert 0<len(donors)<=256
  (folder/'working.dat').write_bytes(original.read_bytes());(folder/'edge.dat').write_text(' '.join(map(str,row['feature'][:2]))+'\n');(folder/'serial.cfg').write_bytes((BASE/'serial.cfg').read_bytes())
  with (folder/'donors.dat').open('w') as f:
   f.write(f'ORIGINAL_CAVITY_V1 {len(donors)}\n')
   for i in donors:
    f.write(f'{i} 0\n')
    for n in mesh.E[i]:f.write(str(n)+' '+' '.join(format(float(x),'.17g') for x in [*data['x'][n],*data['m'][n][[0,1,2,4,5,8]]])+'\n')
  pins={p.name:digest(p) for p in folder.iterdir() if p.suffix in ('.dat','.cfg')}
  cmd=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','90s','taskset','-c','0',str(HERE/'replay'),str(SOURCE)]
  t=time.monotonic()
  with (folder/'run.log').open('w') as f:ret=subprocess.run(cmd,cwd=folder,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=100)
  case={'stem':row['stem'],'edge':row['feature'][:2],'donors':len(donors),'input_hashes':pins,'exit':ret.returncode,'seconds':time.monotonic()-t,'command':cmd}
  if ret.returncode==0:
   trials=[json.loads(s) for s in (folder/'trials.jsonl').read_text().splitlines()];assert trials[-1]['kind']=='work'
   case['initial']=trials[0];case['work']=trials[-1];case['joint_work']=next(t for t in trials if t['kind']=='joint_work');case['joint']=next(t for t in trials if t.get('name')=='joint');case['any_local_progress']=any(t.get('local_progress') for t in trials)
   for trial in trials:
    if trial.get('local_progress'):assert trial['q']>=.2 and trial['J']>=.05 and trial['added_excess']<trial['removed_excess']
  receipt['cases'].append(case);(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(case),flush=True)
  if ret.returncode:return ret.returncode
 assert len(receipt['cases'])==10
 return 0
if __name__=='__main__':sys.exit(main())
