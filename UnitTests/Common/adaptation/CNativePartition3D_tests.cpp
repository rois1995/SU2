/*!
 * \file CNativePartition3D_tests.cpp
 * \brief Weighted tetrahedral dual graphs, explicit subsets, byte preservation and rejection.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativePartition3D.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
using namespace SU2Native3D;
namespace {
std::vector<char> Bytes(OwnedCell row) { std::vector<char> b;RecordStream s(b);s(row);return b; }
std::vector<char> Bytes(const Owned& rows) { std::vector<char> b;RecordStream s(b);for(auto r:rows) s(r.second);return b; }
std::vector<char> Bytes(const std::vector<DonorCell>& rows) { std::vector<char> b;RecordStream s(b);for(auto r:rows) s(r);return b; }
void Log(World& world,const std::string& text) {
  const auto b=CPassiveComm::Allgatherv(std::vector<char>(text.begin(),text.end()),nullptr,world.comm);
  if (!world.rank && !b.empty()) std::cout.write(b.data(),b.size())<<std::flush;
}
std::array<Face,4> Faces(Cell c) {auto v=c.v;return {{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},
    {v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};}
Owned Grid(int n) {
  Owned result;const std::array<unsigned,6> ring{1,2,3,7,4,5};
  auto node=[&](int x,int y,int z) { return Node{(Id(1)<<56)+Id((z*(n+1)+y)*(n+1)+x),{double(x)/n,double(y)/n,double(z)/n}}; };
  for (int z=0;z<n;++z) for (int y=0;y<n;++y) for (int x=0;x<n;++x) {
    const std::array<Node,8> v{{node(x,y,z),node(x+1,y,z),node(x+1,y+1,z),node(x,y+1,z),
        node(x,y,z+1),node(x+1,y,z+1),node(x+1,y+1,z+1),node(x,y+1,z+1)}};
    for (size_t k=0;k<6;++k) {
      const Id index=6*Id((z*n+y)*n+x)+k,id=(Id(1)<<60)+13*index;
      OwnedCell row;row.cell={id,{v[0],v[6],v[ring[k]],v[ring[(k+1)%6]]}};row.version=(Id(1)<<55)+index%49;
      auto& t=row.cell.v;
      if (Orientation(t[0].p,t[1].p,t[2].p,t[3].p)<0) std::swap(t[0],t[1]);
      const auto faces=Faces(row.cell);std::map<Id,Point> p;for(auto v:t)p.emplace(v.id,v.p);
      for(size_t i=0;i<4;++i) {
        row.physical[i].facet=900000+index*4+i;  // Preserve even unused payload fields.
        const auto f=faces[i];
        for(size_t axis=0;axis<3;++axis) for(double side:{0.,1.}) {
          auto coord=[&](Id id) {auto q=p.at(id);return axis==0?q.x:axis==1?q.y:q.z;};
          if(coord(f[0])==side && coord(f[1])==side && coord(f[2])==side)
            row.physical[i]={true,1000+2*axis+Id(side)};
        }
      }
      result.emplace(id,row);
    }
  }
  return result;
}
Owned Layout(const Owned& all,World& world,int layout) {
  const int owners=layout==0?1:layout==1?std::max(1,world.size-1):world.size;
  Owned result;size_t i=0;for(auto r:all) {if(i++%owners==unsigned(world.rank))result.emplace(r);}
  return result;
}
uint32_t Prescribed(OwnedCell row) {return Centroid(row.cell).x<.5 ? 4 : 1;}
void Rows(const std::string& label,const char* stage,World& world,const Owned& rows) {
  std::ostringstream s;
  for(auto r:rows) {
    s<<"PART_ROW "<<label<<' '<<stage<<' '<<world.rank<<' '<<Prescribed(r.second)<<' ';
    for(unsigned char c:Bytes(r.second))s<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c);
    s<<std::dec<<'\n';
  }
  Log(world,s.str());
}
}
TEST_CASE("Native tetrahedral weights preserve sensor volume and compose every actual sample", "[NativeMesh3D][NativePartition3D]") {
  const auto all=Grid(1);std::vector<DonorCell> source;std::vector<Cell> cells;
  for(auto r:all) {DonorCell d;d.cell=r.second.cell;d.sensor.fill({1,0,0,1,0,1});source.push_back(d);cells.push_back(d.cell);}
  FrozenField plain(source);const auto weights=WorkWeights(cells,plain);CHECK(weights.size()==6);
  uint64_t calls=0;
  FrozenField thin(source,[&](Point p,Tensor m) {++calls;if(p.z==0)m.zz=1e12;return m;});
  const auto first=WorkWeights(cells,thin);const auto second=WorkWeights(cells,thin);
  CHECK(calls==60);CHECK(thin.Statistics().requests==60);CHECK(thin.Statistics().hits>=30);CHECK(first==second);
  for(size_t i=0;i<6;++i) {CHECK(first[i]>weights[i]);CHECK(first[i]<100);}
  CHECK(plain.Statistics().compositions==0);
  FrozenField strong(source,[&](Point,Tensor) {return Tensor{NAN,0,0,1,0,1};});
  CHECK_THROWS(WorkWeights(cells,strong));CHECK_THROWS(WorkWeights(std::vector<Cell>(129,cells[0]),plain));
  auto bad=cells;std::swap(bad[0].v[0],bad[0].v[1]);CHECK_THROWS(WorkWeights(bad,plain));
  // Sensor refinement strength is retained in volume demand, unlike a sampled thin geometric constraint.
  for(auto& d:source)d.sensor.fill({1,0,0,1,0,1e12});FrozenField fine(source);
  for(auto weight:WorkWeights(cells,fine))CHECK(weight==MaximumPartitionWeight);
}
TEST_CASE("Native tetrahedral partition migrates two constraints with atomic records and explicit worlds", "[NativeDistributed3D][NativePartition3D]") {
  World world;const auto all=Grid(4);
  for(int layout:{0,1,2}) for(int parts:std::set<int>{1,world.size,std::max(1,world.size-1),std::max(1,world.size/2)}) {
    auto input=Layout(all,world,layout);const auto saved=Bytes(input);std::vector<uint32_t> weights;
    for(auto r:input)weights.push_back(Prescribed(r.second));
    PartitionStats rejected;std::string reason;
    CHECK_FALSE(Repartition(world,input,weights,parts,rejected,reason,world.rank==0?1:SIZE_MAX));
    CHECK(Bytes(input)==saved);CHECK_FALSE(reason.empty());
    PartitionStats stats;
    REQUIRE(Repartition(world,input,weights,parts,stats,reason));CHECK(reason.empty());
    CHECK(std::accumulate(stats.cells_after.begin(),stats.cells_after.end(),uint64_t(0))==384);
    CHECK(std::accumulate(stats.work_after.begin(),stats.work_after.end(),uint64_t(0))==960);
    CHECK(stats.destinations.size()==weights.size());CHECK(stats.maximum_native_bytes>0);
    CHECK(stats.total_seconds>=stats.graph_seconds+stats.partition_seconds+stats.migration_seconds-1e-12);
    for(auto r:input)CHECK(Bytes(r.second)==Bytes(all.at(r.first)));
    if(world.rank>=parts)CHECK(input.empty());
    CHECK(*std::max_element(stats.cells_after.begin(),stats.cells_after.end())<=1.30*384/parts);
    CHECK(*std::max_element(stats.work_after.begin(),stats.work_after.end())<=1.30*960/parts);
    const std::string name="grid4_l"+std::to_string(layout)+"_m"+std::to_string(parts);
    Rows(name,"input",world,Layout(all,world,layout));Rows(name,"output",world,input);
    std::ostringstream log;
    log<<std::setprecision(17)<<"PART_PROFILE case="<<name<<" rank="<<world.rank<<" ranks="<<world.size<<" parts="<<parts
        <<" total_s="<<stats.total_seconds<<" graph_s="<<stats.graph_seconds<<" partition_s="<<stats.partition_seconds
        <<" migration_s="<<stats.migration_seconds<<" other_s="<<stats.other_seconds<<" directory_nested_s="<<stats.directory_seconds
        <<" native_bytes="<<stats.maximum_native_bytes<<" arcs="<<stats.graph_arcs<<" edgecut="<<stats.edgecut
        <<" moved="<<stats.moved_cells<<" cells_after="<<stats.cells_after[world.rank]<<" work_after="<<stats.work_after[world.rank]<<'\n';
    Log(world,log.str());
  }
}
TEST_CASE("Native tetrahedral partition rejects invalid input without ownership changes", "[NativeDistributed3D][NativePartition3D]") {
  World world;const auto initial=Layout(Grid(1),world,0);
  for(int problem=0;problem<7;++problem) {
    auto input=initial;std::vector<uint32_t> weights(input.size(),1);int parts=1;
    if(world.rank==0) {
      if(problem==0)parts=0;
      if(problem==1)weights.pop_back();
      if(problem==2)weights[0]=0;
      if(problem==3)input.begin()->second.physical[0].present=!input.begin()->second.physical[0].present;
      if(problem==4)input.begin()->second.cell.v[0].p.x+=.125;
      if(problem==5) {auto row=input.begin()->second;row.cell.id+=10000;input.emplace(row.cell.id,row);weights.push_back(1);}
    }
    const auto saved=Bytes(input);PartitionStats stats;std::string reason;
    CHECK_FALSE(Repartition(world,input,weights,parts,stats,reason,SIZE_MAX,problem==6&&world.rank==0?SIZE_MAX:0));
    CHECK(Bytes(input)==saved);CHECK_FALSE(reason.empty());
  }
}
#ifdef HAVE_MPI
TEST_CASE("Native weighted tetrahedral worker migration preserves original sensors and returns adapted records", "[NativeDistributed3D][NativePartition3D]") {
  World parent;const auto all=Grid(1);const auto initial=Layout(all,parent,2);const auto initial_bytes=Bytes(initial);
  std::vector<DonorCell> global,local;
  for(auto r:all) {DonorCell d;d.cell=r.second.cell;d.sensor.fill({4,0,0,4,0,4});global.push_back(d);}
  for(auto d:global)if(initial.count(d.cell.id))local.push_back(d);
  const auto source_bytes=Bytes(local);std::vector<Facet> facets;
  std::map<Id,Node> nodes;for(auto r:all)for(auto n:r.second.cell.v)nodes.emplace(n.id,n);
  for(auto face:Incidence([&]{std::vector<Cell> c;for(auto r:all)c.push_back(r.second.cell);return c;}()).Boundary()) {
    const Id id=facets.size();facets.push_back({id,int(id)+10,{nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])}});
  }
  // Bind the small adaptation fixture to its actual immutable original triangle authority.
  Owned fixture=initial;
  for(auto& r:fixture) {const auto f=Faces(r.second.cell);for(size_t i=0;i<4;++i)if(r.second.physical[i].present)
    for(auto ref:facets){auto key=Face{ref.v[0].id,ref.v[1].id,ref.v[2].id},actual=f[i];std::sort(key.begin(),key.end());std::sort(actual.begin(),actual.end());
      if(key==actual)r.second.physical[i].facet=ref.id;}}
  const PlanarReference geometry(facets);
  for(int m:std::set<int>{1,parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
    auto working=fixture;std::vector<Cell> cells;for(auto r:working)cells.push_back(r.second.cell);
    std::vector<uint32_t> weights;{FrozenField oracle(global);weights=WorkWeights(cells,oracle);}
    PartitionStats stats;std::string reason;REQUIRE(Repartition(parent,working,weights,m,stats,reason));
    std::vector<std::vector<DonorCell>> send(parent.size);
    for(size_t i=0;i<local.size();++i)send[stats.destinations[i]].push_back(local[i]);
    const auto migrated=parent.exchange(send);MPI_Comm comm=MPI_COMM_NULL;
    MPI_Comm_split(parent.comm,parent.rank<m?0:MPI_UNDEFINED,parent.rank,&comm);
    Owned refined;bool valid=true;
    if(comm!=MPI_COMM_NULL) {
      World workers(comm);
      // This second partition proves graph collectives use this subset, never the parent/global communicator.
      std::vector<uint32_t> ones(working.size(),1);PartitionStats subset;
      CHECK(Repartition(workers,working,ones,m,subset,reason));
      EngineOptions options;options.timings=true;
      auto result=Refine(workers,working,migrated,geometry,{},options);valid=result.valid;refined.swap(result.cells);
      MPI_Comm_free(&comm);
    }
    CHECK_FALSE(parent.maximum(!valid));std::vector<std::vector<OwnedCell>> to(parent.size);
    for(auto r:refined)to[r.first%parent.size].push_back(r.second);
    Owned returned;for(auto r:parent.exchange(to))returned.emplace(r.cell.id,r);
    const std::string label="engine_m"+std::to_string(m);
    Rows(label,"input",parent,fixture);Rows(label,"output",parent,returned);
    std::vector<std::vector<OwnedCell>> gather(parent.size);for(auto r:returned)gather[0].push_back(r.second);
    const auto complete=parent.exchange(gather);bool checked=true;
    if(!parent.rank) {
      std::vector<Cell> fresh;for(auto r:complete)fresh.push_back(r.cell);
      FrozenField target(global);PatchMetric measured;checked=ValidateMetricPatch(fresh,target,measured,reason);
      CHECK(complete.size()>6);CHECK(complete.size()<=128);
    }
    CHECK_FALSE(parent.maximum(!checked));CHECK(Bytes(initial)==initial_bytes);CHECK(Bytes(local)==source_bytes);
    std::ostringstream report;report<<"PART_ENGINE rank="<<parent.rank<<" ranks="<<parent.size<<" workers="<<m<<" valid="<<valid
        <<" returned="<<returned.size()<<" source_preserved="<<(Bytes(local)==source_bytes)<<'\n';Log(parent,report.str());
  }
}
#endif
