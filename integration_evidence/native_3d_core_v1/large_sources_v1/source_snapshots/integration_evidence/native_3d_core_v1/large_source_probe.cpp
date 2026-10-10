#include "Common/include/adaptation/CNativeEngine3D.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
using namespace SU2Native3D;
namespace {
Face Key(Face face) { std::sort(face.begin(),face.end());return face; }
std::array<Face,4> Faces(const Cell& c) {
 const auto& v=c.v;
 return {{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},
          {v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
}
std::vector<Cell> Grid(unsigned n) {
 std::vector<Cell> cells;
 const std::array<unsigned,6> ring{1,2,3,7,4,5};
 const unsigned xyz[8][3]{{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
 for(unsigned z=0;z<n;++z)for(unsigned y=0;y<n;++y)for(unsigned x=0;x<n;++x) {
  std::array<Node,8> nodes;
  for(unsigned i=0;i<8;++i) {
   const unsigned a=x+xyz[i][0],b=y+xyz[i][1],c=z+xyz[i][2];
   nodes[i]={Id(a+(n+1)*(b+(n+1)*c)),{double(a)/n,double(b)/n,double(c)/n}};
  }
  for(unsigned i=0;i<6;++i) {
   Cell cell{Id(cells.size()),{nodes[0],nodes[6],nodes[ring[i]],nodes[ring[(i+1)%6]]}};
   if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
   cells.push_back(cell);
  }
 }
 return cells;
}
std::vector<char> Bytes(const Owned& owned) {
 std::vector<char> bytes;RecordStream stream(bytes);
 for(auto row:owned){auto id=row.first;stream(id,row.second);}return bytes;
}
void Ordered(World& world,const std::string& text) {
 const std::vector<char> row(text.begin(),text.end());
 const auto all=CPassiveComm::Allgatherv(row,nullptr,world.comm);
 if(!world.rank && !all.empty())std::cout.write(all.data(),all.size())<<std::flush;
}
Owned Coarse(World& world,unsigned n,std::vector<Facet>& facets,bool wedge=false) {
 auto cells=Grid(1);if(wedge)cells.resize(1);
 std::map<Id,Node> nodes;
 for(auto& cell:cells){cell.id+=10000;for(auto& v:cell.v){
  v.id=Id(v.p.x*n+(n+1)*(v.p.y*n+(n+1)*v.p.z*n));nodes.emplace(v.id,v);
 }}
 std::map<Face,Id> physical;
 for(auto face:Incidence(cells).Boundary()) {
  const Id id=facets.size();facets.push_back({id,int(id)+10,{nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])}});
  physical.emplace(Key(face),id);
 }
 Owned owned;
 for(auto cell:cells)if(cell.id%world.size==unsigned(world.rank)) {
  OwnedCell row;row.cell=cell;row.version=17+cell.id;const auto faces=Faces(cell);
  for(unsigned i=0;i<4;++i){auto found=physical.find(Key(faces[i]));if(found!=physical.end())row.physical[i]={true,found->second};}
  owned.emplace(cell.id,row);
 }
 return owned;
}
}
int main(int argc,char** argv) {
 SU2_MPI::Init(&argc,&argv);
 {
  World world;const unsigned n=4;auto grid=Grid(n);
  std::vector<DonorCell> source;
  for(auto cell:grid)if(cell.id%world.size==unsigned(world.rank)) {
   DonorCell d;d.cell=cell;d.sensor.fill({});source.push_back(d);
  }
  std::vector<Facet> facets;const auto owned=Coarse(world,n,facets);const auto saved=Bytes(owned);
  EngineOptions options;options.timings=true;
  auto result=Refine(world,owned,source,PlanarReference(facets),{},options);
  assert(!result.valid && result.stats.accepted==0 && Bytes(owned)==saved);
  assert(result.reason.find("patch budget")!=std::string::npos);
  const auto& s=result.stats;std::ostringstream report;report<<std::setprecision(17)
   <<"LARGE_BASELINE rank="<<world.rank<<" ranks="<<world.size<<" source="<<grid.size()
   <<" valid="<<result.valid<<" batches="<<s.donor_batches<<" retries="<<s.batch_retries
   <<" initial_measured="<<s.initial_cells<<" total_s="<<s.total_seconds<<" seed_s="<<s.seed_seconds
   <<" work_bytes="<<s.maximum_work_bytes<<" reason="<<result.reason<<'\n';Ordered(world,report.str());
  if(!world.rank)std::cout<<"PASS large-source baseline rejection\n";
 }
 SU2_MPI::Finalize();return 0;
}
