#!/usr/bin/env python3
"""Exact-rational normal-dot oracle against the official compiled predicate object."""
import hashlib,json,math,os,random,shlex,struct,subprocess,time
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
def sha(path):
 h=hashlib.sha256()
 with path.open('rb') as src:
  while chunk:=src.read(4*1024**2):h.update(chunk)
 return h.hexdigest()
def adjacent(x,up):
 if x==0:return float.fromhex('0x0.0000000000001p-1022')*(1 if up else -1)
 bits=struct.unpack('<Q',struct.pack('<d',x))[0]
 return struct.unpack('<d',struct.pack('<Q',bits+(1 if (x>0)==up else -1)))[0]
def main():
 receipt=json.loads((HERE/'build_process.json').read_text());assert receipt['status']=='PASS'
 assert all(sha(ROOT/p)==h for p,h in receipt['source_sha256'].items())
 folder=HERE/'orientation_oracle_v2';folder.mkdir(exist_ok=True)
 assert not (folder/'result.json').exists(),'Preserve/reuse existing independent evidence'
 probe=folder/'probe.cpp';probe.write_text('''#include "Common/src/adaptation/CNativePredicates3D_internal.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
int main(){using namespace SU2Native3D;std::string word;while(std::cin>>word){std::array<Point,3>a{},b{};
for(auto* face:{&a,&b})for(auto& p:*face)for(double* x:{&p.x,&p.y,&p.z}){*x=std::strtod(word.c_str(),nullptr);if(!(face==&b && &p==&b[2] && x==&p.z))if(!(std::cin>>word))return 2;}
KernelStats stats;const int sign=SurfaceOrientationSign(a,b,&stats);std::cout<<sign<<' '<<stats.filtered_orientations<<' '<<stats.exact_orientations<<'\\n';}}
''')
 row=next(r for r in json.loads((BUILD/'compile_commands.json').read_text()) if r['file'].endswith('CNativePredicates3D.cpp'))
 command=row.get('arguments') or shlex.split(row['command']);actual_object=BUILD/command[command.index('-o')+1]
 compile_command=command[:];obj=folder/'probe.o'
 for flag,value in (('-o',str(obj)),('-MF',str(obj)+'.d'),('-MQ',str(obj))):
  if flag in compile_command:compile_command[compile_command.index(flag)+1]=value
 compile_command[compile_command.index('-c')+1]=str(probe);compile_command.insert(1,'-I'+str(ROOT))
 old=json.loads((HERE.with_name('mapped_faceted_geometry_v1')/'core_v1_n1/result.json').read_text())
 prior=next(s['command'] for s in old['steps'] if s['name']=='link');tail=prior[prior.index('-Wl,--as-needed'):]
 executable=folder/'probe';link=['c++','-o',str(executable),str(obj),str(actual_object)]+tail
 pins={str(p):sha(p) for p in [actual_object,ROOT/'Common/src/adaptation/CNativePredicates3D.cpp',ROOT/'Common/src/adaptation/CNativePredicates3D_internal.hpp',probe,Path(__file__).resolve()]+[(Path(t) if Path(t).is_absolute() else BUILD/t) for t in tail if (Path(t) if Path(t).is_absolute() else BUILD/t).is_file()]}
 env=dict(os.environ,TMPDIR=str(BUILD/'compile-tmp'),OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1')
 steps=[]
 for name,args in [('compile',compile_command),('link',link)]:
  start=time.monotonic()
  with (folder/(name+'.log')).open('w') as out:code=subprocess.run(['nice','-n','19','timeout','--kill-after=5s','120s']+args,cwd=BUILD,env=env,stdout=out,stderr=subprocess.STDOUT).returncode
  steps.append(dict(name=name,command=args,exit_code=code,seconds=time.monotonic()-start));assert code==0,(name,code)
 samples=[]
 for s in [float.fromhex('0x0.0000000000001p-1022'),1e-300,1e-100,1.,1e100,1e300,math.ldexp(1.,1022)]:
  a=[[0.,0.,0.],[s,s,0.],[0.,0.,s]]
  for x in [adjacent(-s,False),-s,adjacent(-s,True),s]:
   if math.isfinite(x):samples.append([a,[[0.,0.,0.],[s,x,0.],[0.,0.,s]]])
  samples.extend([[a,a],[a,[a[0],a[2],a[1]]],[a,[a[0],a[1],a[1]]]])
 rng=random.Random(43951)
 for _ in range(160):
  faces=[[[math.ldexp(rng.uniform(-1,1),rng.randint(-1073,1023)) for k in range(3)] for j in range(3)] for i in range(2)]
  samples.append(faces)
 def normal(face):
  a,b,c=[[F(x) for x in p] for p in face];u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
  return [u[(i+1)%3]*v[(i+2)%3]-u[(i+2)%3]*v[(i+1)%3] for i in range(3)]
 data=''.join(' '.join(x.hex() for f in pair for p in f for x in p)+'\n' for pair in samples)
 (folder/'input.txt').write_text(data);start=time.monotonic()
 run=subprocess.run(['nice','-n','19','timeout','30s',str(executable)],input=data,capture_output=True,text=True,env=env)
 (folder/'probe.log').write_text(run.stdout);(folder/'probe.err').write_text(run.stderr)
 assert run.returncode==0,run.stderr
 rows=[list(map(int,line.split())) for line in run.stdout.splitlines()];assert len(rows)==len(samples)
 counts=dict(positive=0,negative=0,zero=0,filtered=0,exact=0)
 for pair,row in zip(samples,rows):
  n,m=map(normal,pair);dot=sum(x*y for x,y in zip(n,m));expected=(dot>0)-(dot<0)
  assert row[0]==expected,(pair,row,expected)
  counts['positive' if expected>0 else 'negative' if expected<0 else 'zero']+=1;counts['filtered']+=row[1];counts['exact']+=row[2]
 assert counts['filtered'] and counts['exact'] and counts['positive'] and counts['negative'] and counts['zero']
 assert all(sha(Path(p))==h for p,h in pins.items()),'Predicate, helpers or support changed during oracle'
 result=dict(status='EXACT_RATIONAL_NORMAL_DOT_PASS',samples=len(samples),counts=counts,probe_seconds=time.monotonic()-start,steps=steps,source_and_object_sha256=pins,input_sha256=sha(folder/'input.txt'),output_sha256=sha(folder/'probe.log'),scope='Realized orientation arithmetic including cancellation, subnormal and exponent extremes; not embedding, construction, MPI, transfer or restart.')
 (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],counts)
if __name__=='__main__':main()
