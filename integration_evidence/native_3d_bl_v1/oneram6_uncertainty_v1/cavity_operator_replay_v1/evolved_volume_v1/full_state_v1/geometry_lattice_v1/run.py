#!/usr/bin/env python3
"""Measure the actual frozen field on a persistent exact-facet symmetry edge."""
import hashlib,json,os,subprocess,sys,time
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;STATE=HERE.parent;EVOLVED=STATE.parent;BASE=EVOLVED.parent;ROOT=BASE.parents[3];BUILD=ROOT/'integration_evidence/build-integrated-v2'
sys.path.insert(0,str(BASE));from diagnose_contexts import patch
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2

def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def main():
 cpp=(EVOLVED/'replay.cpp').read_text().replace('../../../../../','../../../../../../../')
 old='nc<=MaximumCavityCells&&nf==0';assert cpp.count(old)==1;cpp=cpp.replace(old,'nc<=MaximumCavityCells&&nf<=4*MaximumCavityCells')
 old='  require(bool(working)&&nodes.rbegin()->first<1000000000';assert cpp.count(old)==1;cpp=cpp.replace(old,'  for(size_t k=0;k<nf;++k){SurfaceFace f;working>>f.v[0]>>f.v[1]>>f.v[2]>>f.facet;old.surface.push_back(f);}\n'+old)
 a=cpp.index('  auto audit=');b=cpp.index('int main(',a)
 cpp=cpp[:a]+'''  const Point mid=field.SegmentPoint(a,b,.5);
  report<<"{\\"kind\\":\\"crease\\",\\"length\\":"<<double(length)<<",\\"left\\":"<<double(field.TargetEdgeLength(a,mid))<<",\\"right\\":"<<double(field.TargetEdgeLength(mid,b))<<",\\"midpoint\\":["<<mid.x<<','<<mid.y<<','<<mid.z<<"],\\"edge_limit\\":1.8,\\"canonical_facets\\":[";
  for(size_t k=0;k<old.surface.size();++k){if(k)report<<',';report<<state.original->Canonical(old.surface[k].facet);}report<<"],\\"analytic_authorities\\":"<<state.original->Analytic().size()<<"}\\n";
  require(bool(report),"Cannot save measurement");return 0;
}
'''+cpp[b:]
 (HERE/'replay.cpp').write_text(cpp)
 (HERE/'working.dat').write_bytes((STATE/'current_stars/rank3_edge.dat').read_bytes());(HERE/'edge.dat').write_text('211 3481\n');(HERE/'serial.cfg').write_bytes((BASE/'serial.cfg').read_bytes())
 source=BASE.parent/'concurrent_composed_proposals_v1/frozen_m6_np4';mesh=read_su2(source/'Coarse.su2');cells,nodes,faces=patch(HERE/'working.dat');xyz=np.array(list(nodes.values()));points=mesh.P[mesh.E];donors=np.flatnonzero(np.all((points.min(1)<=xyz.max(0))&(points.max(1)>=xyz.min(0)),axis=1));assert len(donors)==37
 dtype=np.dtype([('id','<u8'),('x','<f8',(3,)),('m','<f8',(9,))]);data=np.fromfile(source/'selected_sensor.bin',dtype=dtype,offset=8);assert np.array_equal(data['x'],mesh.P)
 with (HERE/'donors.dat').open('w') as f:
  f.write(f'ORIGINAL_CAVITY_V1 {len(donors)}\n')
  for i in donors:
   f.write(f'{i} 0\n')
   for n in mesh.E[i]:f.write(str(n)+' '+' '.join(format(float(x),'.17g') for x in [*data['x'][n],*data['m'][n][[0,1,2,4,5,8]]])+'\n')
 r=json.loads((EVOLVED/'receipt.json').read_text());compile_cmd,link=r['commands']
 for flag,p in [('-c',HERE/'replay.cpp'),('-o',HERE/'replay.o'),('-MF',HERE/'replay.d'),('-MQ',HERE/'replay.o')]:
  if flag in compile_cmd:compile_cmd[compile_cmd.index(flag)+1]=str(p)
 link=[str(HERE/'replay.o') if t==str(EVOLVED/'replay.o') else t for t in link];link[link.index('-o')+1]=str(HERE/'replay')
 receipt={'scope':'Measurement only; no geometry/mesh acceptance or reconstruction','commands':[compile_cmd,link],'hashes':{},'linked_archives':{}}
 pins=[Path(__file__).resolve(),HERE/'replay.cpp',EVOLVED/'replay.cpp',source/'selected_sensor.bin',source/'Coarse.su2.native_ref',*[HERE/n for n in ['working.dat','edge.dat','serial.cfg','donors.dat']]];receipt['hashes']={str(p.relative_to(ROOT)):digest(p) for p in pins}
 for t in link:
  if t.endswith('.a'):
   p=Path(t);p=p if p.is_absolute() else BUILD/p;receipt['linked_archives'][str(p)]=digest(p)
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
 for name,args in [('compile',compile_cmd),('link',link)]:
  t=time.monotonic()
  with (HERE/(name+'.log')).open('w') as f:ret=subprocess.run(['nice','-n','19',*args],cwd=BUILD,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  receipt[name]={'exit':ret.returncode,'seconds':time.monotonic()-t};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
  if ret.returncode:return ret.returncode
 receipt['binary_sha256']=digest(HERE/'replay');cmd=['/usr/bin/time','-v','nice','-n','19','timeout','--kill-after=5s','60s','taskset','-c','0',str(HERE/'replay'),str(source)];t=time.monotonic()
 with (HERE/'run.log').open('w') as f:ret=subprocess.run(cmd,cwd=HERE,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=70)
 receipt['run']={'exit':ret.returncode,'seconds':time.monotonic()-t,'command':cmd};(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');assert ret.returncode==0
 print((HERE/'trials.jsonl').read_text());return 0
if __name__=='__main__':raise SystemExit(main())
