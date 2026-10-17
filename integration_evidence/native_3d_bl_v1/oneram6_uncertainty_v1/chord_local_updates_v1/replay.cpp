#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#define SU2Native3D SU2LegacyChord
#include "legacy_envelope.hpp"
#undef SU2Native3D
#include "Common/include/adaptation/CNativeChordEnvelope.hpp"
using P=std::array<double,3>;
using F=std::array<uint64_t,3>;
using E=std::array<uint64_t,2>;
struct Report {std::vector<double> cuts;std::vector<long double> distance;size_t comparisons=0,pieces=0,features=0;std::string failure;double seconds=0;};
template<class Envelope>Report build(P a,P b,const std::vector<P>& points,const std::vector<F>& faces,const std::vector<size_t>& support,const std::vector<double>& levels) {
  const auto start=std::chrono::steady_clock::now();Envelope envelope(a,b);Report r;
  try {
    std::set<uint64_t> vertices;std::set<E> edges;
    for(size_t face:support) {
      std::array<P,3> tri;const auto ids=faces[face];
      for(unsigned k=0;k<3;++k) {
        tri[k]=points[ids[k]];
        if(vertices.insert(ids[k]).second)envelope.Vertex(tri[k]);
        E edge{ids[k],ids[(k+1)%3]};std::sort(edge.begin(),edge.end());
        if(edges.insert(edge).second)envelope.Segment(points[edge[0]],points[edge[1]]);
      }
      envelope.Face(tri);
    }
    if(!support.empty()) {
      r.cuts=envelope.Cuts(levels);for(unsigned k=0;k<=64;++k)r.distance.push_back(envelope.Distance(k/64.L));
    }
  }catch(const std::exception& e){r.failure=e.what();}
  r.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  r.comparisons=envelope.Comparisons();r.features=envelope.Features();r.pieces=envelope.Pieces();return r;
}
int main(int argc,char** argv) {
  if(argc!=2)return 2;
  std::ifstream input(argv[1],std::ios::binary);uint64_t header[4];input.read(reinterpret_cast<char*>(header),sizeof(header));
  if(!input || header[0]!=0x43484f52444d3655ULL || header[1]>100000 || header[2]>100000 || header[3]>128)return 2;
  std::vector<P> points(header[1]);std::vector<F> faces(header[2]);std::vector<E> edges(header[3]);
  input.read(reinterpret_cast<char*>(points.data()),points.size()*sizeof(P));input.read(reinterpret_cast<char*>(faces.data()),faces.size()*sizeof(F));input.read(reinterpret_cast<char*>(edges.data()),edges.size()*sizeof(E));
  if(!input || input.peek()!=std::char_traits<char>::eof())return 2;
  std::vector<P> low(faces.size()),high(faces.size());double magnitude=0;
  for(auto p:points)for(double x:p)if(!std::isfinite(x))return 2;
  for(size_t i=0;i<faces.size();++i)for(unsigned j=0;j<3;++j) {
    low[i][j]=INFINITY;high[i][j]=-INFINITY;
    for(auto v:faces[i]){if(v>=points.size())return 2;low[i][j]=std::min(low[i][j],points[v][j]);high[i][j]=std::max(high[i][j],points[v][j]);magnitude=std::max(magnitude,std::abs(points[v][j]));}
  }
  std::vector<double> levels{0,1e-6,64*std::numeric_limits<double>::epsilon()*magnitude};
  for(double level=2e-6;level<.009;level*=2)levels.push_back(level);
  levels.push_back(.009);for(unsigned k=1;k<=4;++k)levels.push_back(.009+.001*k/4);
  std::cout<<std::setprecision(17)<<"pass,row,faces,features,pieces,comparisons,cuts,legacy_seconds,candidate_seconds,equal,failure\n";
  for(unsigned pass=0;pass<2;++pass)for(size_t i=0;i<edges.size();++i) {
    const auto edge=edges[i];if(edge[0]>=points.size() || edge[1]>=points.size() || edge[0]==edge[1])return 2;
    const auto a=points[edge[0]],b=points[edge[1]];std::vector<size_t> support;
    for(size_t f=0;f<faces.size();++f) {
      bool overlaps=true;for(unsigned j=0;j<3;++j)overlaps=overlaps && low[f][j]<=std::max(a[j],b[j])+.01 && high[f][j]>=std::min(a[j],b[j])-.01;
      if(overlaps)support.push_back(f);
    }
    Report old,fresh;
    if(!pass){old=build<SU2LegacyChord::ChordEnvelope>(a,b,points,faces,support,levels);fresh=build<SU2Native3D::ChordEnvelope>(a,b,points,faces,support,levels);}
    else {fresh=build<SU2Native3D::ChordEnvelope>(a,b,points,faces,support,levels);old=build<SU2LegacyChord::ChordEnvelope>(a,b,points,faces,support,levels);}
    const bool same=old.cuts==fresh.cuts && old.distance==fresh.distance && old.comparisons==fresh.comparisons && old.pieces==fresh.pieces && old.features==fresh.features && old.failure==fresh.failure;
    std::cout<<pass<<','<<i<<','<<support.size()<<','<<fresh.features<<','<<fresh.pieces<<','<<fresh.comparisons<<','<<fresh.cuts.size()<<','<<old.seconds<<','<<fresh.seconds<<','<<same<<','<<std::quoted(fresh.failure)<<std::endl;
    if(!same)return 1;
  }
}
