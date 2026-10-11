
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
