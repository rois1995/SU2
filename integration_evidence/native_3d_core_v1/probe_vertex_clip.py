"""Test vertex-identity exact clipping on saved n4 wedge chords without production changes."""
from pathlib import Path
import hashlib,itertools,json,os,subprocess,sys,time
root=Path(__file__).resolve().parents[2];where=Path(__file__).resolve().parent
folder=Path(sys.argv[1]).resolve();folder.mkdir(exist_ok=False)
print('Case working and retained folder:',folder,flush=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base='7bc3869babb19171fe402fdfb0e96f745b769703'
paths={'header':'Common/include/adaptation/CNativeMesh3D.hpp','source':'Common/src/adaptation/CNativePredicates3D.cpp'}
pins={p:sha(root/p) for p in paths.values()};pins[str(Path(__file__).resolve().relative_to(root))]=sha(Path(__file__))
for label,namespace in [('baseline','Baseline3D'),('candidate','SU2Native3D')]:
 for kind,path in paths.items():
  original=subprocess.check_output(['git','show',base+':'+path],cwd=root) if label=='baseline' else (root/path).read_bytes()
  if label=='candidate' and kind=='source':
   text=original.decode()
   start=text.index('bool ClipInterval(');end=text.index('\nbool Clip(',start)
   part=text[start:end]
   needle='  for (size_t i = 0; i < 4; ++i) {\n    auto ta = t, tb = t;'
   assert part.count(needle)==1
   replacement='''  // Exact donor vertices have unit barycentric numerators; crossing endpoints retain exact determinants.
  auto vertex = [&](Point p) {
    for (size_t i = 0; i < 4; ++i)
      if (p.x == t[i].x && p.y == t[i].y && p.z == t[i].z) return i;
    return size_t(4);
  };
  const auto first = vertex(a), last = vertex(b);
  for (size_t i = 0; i < 4; ++i) {
    auto ta = t, tb = t;'''
   part=part.replace(needle,replacement)
   part=part.replace('    va[i] = ExactVolume(ta, stats);','    va[i] = first < 4 ? (first == i ? volume : Integer<198>{}) : ExactVolume(ta, stats);')
   part=part.replace('    vb[i] = ExactVolume(tb, stats);','    vb[i] = last < 4 ? (last == i ? volume : Integer<198>{}) : ExactVolume(tb, stats);')
   original=(text[:start]+part+text[end:]).encode()
  (folder/(label+'_original.'+('hpp' if kind=='header' else 'cpp'))).write_bytes(original)
  text=original.decode().replace('SU2Native3D',namespace)
  if kind=='source':text=text.replace('../../include/adaptation/CNativeMesh3D.hpp',label+'.hpp')
  (folder/(label+('.hpp' if kind=='header' else '.cpp'))).write_text(text)
source=where/'larger_wedge_n4_n1_m1_v1';source_receipt=json.loads((source/'validation.json').read_text());assert source_receipt['status']=='PASS'
inputs={};sets=[]
for cycle in range(3):
 initial=source/f'envelope_s{cycle}_initial.json';final=source/f'envelope_s{cycle}.json'
 for p in (initial,final):assert source_receipt['artifact_sha256'][p.name]==sha(p);inputs[str(p)]=sha(p)
 a=json.loads(initial.read_text());b=json.loads(final.read_text())
 donors=sorted(a['original'],key=lambda row:sorted(v[0] for v in row['vertices']))
 boxes=[([min(v[axis] for v in d['vertices']) for axis in (1,2,3)],[max(v[axis] for v in d['vertices']) for axis in (1,2,3)]) for d in donors]
 chords=set()
 for mesh in (a,b):
  nodes={row[0]:tuple(row[1:]) for row in mesh['nodes']}
  for cell in mesh['cells']:
   for i,j in itertools.combinations(cell['nodes'],2):chords.add(tuple(sorted((nodes[i],nodes[j]))))
 queries=[];skipped=0
 for chord in sorted(chords):
  lo=[min(chord[0][i],chord[1][i]) for i in range(3)];hi=[max(chord[0][i],chord[1][i]) for i in range(3)]
  candidates=[i for i,(low,high) in enumerate(boxes) if all(lo[a]<=high[a] and hi[a]>=low[a] for a in range(3))]
  if len(candidates)>256:skipped+=1;continue
  assert candidates
  queries.append((chord,candidates))
 assert queries;sets.append(dict(cycle=cycle,donors=donors,queries=queries,skipped=skipped))
with (folder/'geometry.txt').open('w') as out:
 out.write(str(len(sets))+'\n')
 for data in sets:
  out.write(f"{data['cycle']} {len(data['donors'])} {len(data['queries'])}\n")
  for i,donor in enumerate(data['donors']):
   out.write(str(i)+' '+' '.join(str(v[0])+' '+' '.join(format(x,'.17g') for x in v[1:4]) for v in donor['vertices'])+'\n')
  for chord,candidates in data['queries']:
   out.write(' '.join(format(x,'.17g') for p in chord for x in p)+' '+str(len(candidates))+' '+' '.join(map(str,candidates))+'\n')
(folder/'driver.cpp').write_text(r'''
#include "baseline.hpp"
#include "candidate.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
struct Query {SU2Native3D::Point a,b;std::vector<size_t> candidates;};
struct Data {int cycle;std::vector<SU2Native3D::Cell> current;std::vector<Baseline3D::Cell> baseline;std::vector<Query> queries;};
int main(int argc,char** argv) {
 assert(argc==2);std::ifstream in(argv[1]);size_t count;in>>count;std::vector<Data> data(count);
 for(auto& d:data) {
  size_t n,q;in>>d.cycle>>n>>q;d.current.resize(n);d.baseline.resize(n);d.queries.resize(q);
  for(size_t i=0;i<n;++i) {
   auto& c=d.current[i];auto& b=d.baseline[i];in>>c.id;b.id=c.id;
   for(size_t j=0;j<4;++j) {auto& v=c.v[j];in>>v.id>>v.p.x>>v.p.y>>v.p.z;b.v[j]={v.id,{v.p.x,v.p.y,v.p.z}};}
  }
  for(auto& query:d.queries) {in>>query.a.x>>query.a.y>>query.a.z>>query.b.x>>query.b.y>>query.b.z>>n;query.candidates.resize(n);for(auto& i:query.candidates)in>>i;}
 }
 assert(in.good());
 for(auto& d:data) {
  SU2Native3D::KernelStats current;Baseline3D::KernelStats baseline;size_t pieces=0,candidates=0;
  for(auto& q:d.queries) {
   std::vector<const SU2Native3D::Cell*> c;std::vector<const Baseline3D::Cell*> b;
   for(auto i:q.candidates){c.push_back(&d.current.at(i));b.push_back(&d.baseline.at(i));}
   const auto x=SU2Native3D::TraceSegment(c,q.a,q.b,&current);
   const auto y=Baseline3D::TraceSegment(b,{q.a.x,q.a.y,q.a.z},{q.b.x,q.b.y,q.b.z},&baseline);
   assert(x.size()==y.size());pieces+=x.size();candidates+=c.size();
   for(size_t i=0;i<x.size();++i) {assert(x[i].donor==y[i].donor && x[i].begin==y[i].begin && x[i].end==y[i].end && x[i].width==y[i].width);assert(x[i].begin_weights==y[i].begin_weights && x[i].end_weights==y[i].end_weights);}
  }
  std::cout<<"EQUALITY cycle="<<d.cycle<<" chords="<<d.queries.size()<<" candidates="<<candidates<<" pieces="<<pieces<<" separated="<<current.segment_separated<<" baseline_exact="<<baseline.exact_orientations<<" candidate_exact="<<current.exact_orientations<<'\n';
  for(int repeat=0;repeat<3;++repeat) for(int order=0;order<2;++order) {
   const bool candidate=bool(order^(repeat%2));long double checksum=0;const auto start=std::chrono::steady_clock::now();
   for(auto& q:d.queries) {
    if(candidate) {std::vector<const SU2Native3D::Cell*> c;for(auto i:q.candidates)c.push_back(&d.current.at(i));for(auto p:SU2Native3D::TraceSegment(c,q.a,q.b))checksum+=p.width;}
    else {std::vector<const Baseline3D::Cell*> b;for(auto i:q.candidates)b.push_back(&d.baseline.at(i));for(auto p:Baseline3D::TraceSegment(b,{q.a.x,q.a.y,q.a.z},{q.b.x,q.b.y,q.b.z}))checksum+=p.width;}
   }
   const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
   assert(checksum>0);std::cout<<std::setprecision(17)<<"TIMING cycle="<<d.cycle<<" repeat="<<repeat<<" candidate="<<candidate<<" seconds="<<seconds<<" checksum="<<double(checksum)<<'\n';
  }
 }
}
''')
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1');stages=[];status='FAIL'
def run(command,label):
 print('Starting',label,flush=True);start=time.monotonic();p=subprocess.run(['timeout','--signal=TERM','--kill-after=10s','300s']+list(map(str,command)),cwd=folder,env=env,capture_output=True,text=True)
 (folder/(label+'.stdout')).write_text(p.stdout);(folder/(label+'.stderr')).write_text(p.stderr);stages.append(dict(stage=label,seconds=time.monotonic()-start,exit_code=p.returncode,command=list(map(str,command))));assert p.returncode==0,label
try:
 run(['uptime'],'host_load');run(['ps','-eo','pid,ppid,ni,pcpu,comm','--sort=-pcpu'],'host_processes')
 for label in ('baseline','candidate'):run(['nice','-n','19','g++','-std=c++17','-O2','-g','-c',label+'.cpp','-o',label+'.o'],'compile_'+label)
 run(['nice','-n','19','g++','-std=c++17','-O2','-g','driver.cpp','baseline.o','candidate.o','-o','probe'],'link')
 run(['nice','-n','19','./probe','geometry.txt'],'probe')
 assert all(sha(root/p)==h for p,h in pins.items());status='PASS'
finally:
 result=dict(status=status,baseline_revision=base,pins=pins,input_sha256=inputs,stages=stages,sets=[dict(cycle=d['cycle'],donors=len(d['donors']),chords=len(d['queries']),over256_candidate_chords_skipped=d['skipped']) for d in sets],scope='Isolated saved baseline and vertex-identity exact-clip candidate compiled O2; candidate modification only in its private source copy, namespace/include changes for comparison; actual initial/final mesh chord unions and closed AABB candidates, skipping >256 candidates. All returned donor/interval/weight values compared exactly; three alternating baseline/candidate timing samples. No actual import/ADT/CFD/scaling timing or uniform speedup claim.')
 result['artifact_sha256']={str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file() and p.suffix not in ('.o','.pyc') and p.name!='probe'}
 (folder/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print(status,folder,flush=True)
