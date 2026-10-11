#include "Common/include/adaptation/CNativeEngine3D.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
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
void Transform(std::vector<Cell>& cells,const std::string& mode) {
 for(auto& c:cells)for(auto& n:c.v) {
  if(mode=="thin8")n.p.z/=8;
  if(mode=="shear4")n.p.x+=4*n.p.z;
 }
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

namespace {
void Export(const std::string& path,const std::vector<OwnedCell>& rows,const std::vector<Facet>& facets,
            const std::vector<Cell>& original,double strength) {
 std::ofstream f(path);assert(f);f<<std::setprecision(17);
 std::map<Id,Node> nodes;for(const auto& row:rows)for(auto n:row.cell.v)nodes.emplace(n.id,n);
 auto vertex=[&](Node n){f<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<']';};
 f<<"{\"nodes\":[";bool first=true;for(auto entry:nodes){if(!first){f<<',';}first=false;vertex(entry.second);}
 f<<"],\"cells\":[";first=true;for(auto row:rows){if(!first){f<<',';}first=false;f<<"{\"id\":"<<row.cell.id<<",\"nodes\":[";
  for(unsigned i=0;i<4;++i){if(i)f<<',';f<<row.cell.v[i].id;}f<<"]}";}
 f<<"],\"surface\":[";first=true;for(auto row:rows){const auto faces=Faces(row.cell);for(unsigned i=0;i<4;++i)if(row.physical[i].present){
  if(!first){f<<',';}first=false;f<<"{\"facet\":"<<row.physical[i].facet<<",\"nodes\":[";
  for(unsigned j=0;j<3;++j){if(j)f<<',';f<<faces[i][j];}f<<"]}";}}
 f<<"],\"facets\":[";first=true;for(auto facet:facets){if(!first){f<<',';}first=false;
  f<<"{\"id\":"<<facet.id<<",\"marker\":"<<facet.marker<<",\"vertices\":[";
  for(unsigned i=0;i<3;++i){if(i)f<<',';vertex(facet.v[i]);}f<<"]}";}
 f<<"],\"original\":[";first=true;for(auto cell:original){if(!first){f<<',';}first=false;f<<"{\"vertices\":[";
  for(unsigned i=0;i<4;++i){if(i)f<<',';const auto n=cell.v[i];const auto t=strength*(1+.1*n.p.x);
   f<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<",["<<t<<",0,0,"<<t<<",0,"<<t<<"]]";}f<<"]}";}
 f<<"]}\n";assert(f.good());
}
std::vector<DonorCell> Source(World& world,const std::vector<Cell>& grid,double strength) {
 std::vector<DonorCell> source;
 for(auto cell:grid)if(cell.id%world.size==unsigned(world.rank)) {
  DonorCell d;d.cell=cell;
  for(unsigned i=0;i<4;++i){const auto t=strength*(1+.1*cell.v[i].p.x);d.sensor[i]={t,0,0,t,0,t};}
  source.push_back(d);
 }
 return source;
}
}
int main(int argc,char** argv) {
 SU2_MPI::Init(&argc,&argv);
 {
  World parent;assert(argc==2);const std::string folder=argv[1];
  for(unsigned n:{3u,4u,5u,6u}) {
   const auto grid=Grid(n);const auto source=Source(parent,grid,1.);
   DonorImport service(parent,true);std::string reason;assert(service.Build(source,reason));
   auto box=service.Import({{0,0,0,1,1,1}});assert(box.valid==(n==3));
   const auto resident=service.ResidentBytes();
   for(unsigned i=0;i<6;++i) {
    const auto coarse=Grid(1)[i];auto imported=service.ImportCells(parent.rank ? std::vector<Cell>{} : std::vector<Cell>{coarse});
    assert(imported.valid==(n<5));assert(service.ResidentBytes()==resident);
    std::ostringstream line;line<<std::setprecision(17)<<"LARGE_IMPORT n="<<n<<" cell="<<i<<" rank="<<parent.rank
     <<" ranks="<<parent.size<<" valid="<<imported.valid<<" count="<<imported.cells.size()<<" ids=";
    for(auto d:imported.cells){line<<d.cell.id<<',';}line<<'\n';Ordered(parent,line.str());
    if(imported.valid && !parent.rank) {
     assert(imported.cells.size()==(n==3 ? 162u : 170u));
     uint64_t compositions=0;FrozenField field(imported.cells,[&](Point p,Tensor m){++compositions;m.xx+=p.z;return m;},true);
     for(unsigned q=0;q<4;++q) {
      Point p{};for(unsigned j=0;j<4;++j){const double w=j==q ? .625 : .125;
       p.x+=w*coarse.v[j].p.x;p.y+=w*coarse.v[j].p.y;p.z+=w*coarse.v[j].p.z;}
      auto value=field.Query(p);const auto expected=1+.1*p.x;
      assert(std::abs(value.sensor.xx-expected)<1e-14 && std::abs(value.target.xx-expected-p.z)<1e-14);
      field.Query(p);assert(compositions==2*(q+1));
      std::ostringstream sample;sample<<std::setprecision(17)<<"LARGE_QUERY n="<<n<<" cell="<<i<<" x="<<p.x
       <<" y="<<p.y<<" z="<<p.z<<" sensor="<<value.sensor.xx<<" target="<<value.target.xx<<" donor=";
      for(auto id:value.donor){sample<<id<<',';}sample<<'\n';Ordered(parent,sample.str());
     }
    } else if(imported.valid) {
     for(unsigned q=0;q<4;++q)Ordered(parent,"");
    }
   }
   for(const std::string mode:{"box","thin8","shear4"}) {
    if(n!=4 && mode!="box")continue;
    auto query_grid=grid;Transform(query_grid,mode);auto coarse_grid=Grid(1);Transform(coarse_grid,mode);
    DonorImport query_service(parent,true);assert(query_service.Build(Source(parent,query_grid,1),reason));
    const auto query_resident=query_service.ResidentBytes();
    for(unsigned i=0;i<6;++i) {
     const auto coarse=coarse_grid[i];auto imported=query_service.ImportMetricCells(parent.rank ? std::vector<Cell>{} : std::vector<Cell>{coarse});
     assert(imported.valid && query_service.ResidentBytes()==query_resident);
     std::ostringstream line;line<<"LARGE_SUPPORT n="<<n<<" cell="<<i<<" rank="<<parent.rank
      <<" ranks="<<parent.size<<" mode="<<mode<<" count="<<imported.cells.size()<<" ids=";
     for(auto d:imported.cells){line<<d.cell.id<<',';}line<<'\n';Ordered(parent,line.str());
     std::ostringstream query;
     if(!parent.rank) {
      uint64_t compositions=0;FrozenField field(imported.cells,[&](Point p,Tensor m){++compositions;m.xx+=p.z;return m;},true);
      const auto center=Centroid(coarse);const auto value=field.Query(center);field.Query(center);assert(compositions==2);
      FrozenField sensor(imported.cells);query<<std::setprecision(20)<<"LARGE_SUPPORT_QUERY n="<<n<<" cell="<<i<<" mode="<<mode
       <<" x="<<center.x<<" y="<<center.y<<" z="<<center.z<<" sensor="<<value.sensor.xx<<" target="<<value.target.xx<<" donor=";
      for(auto id:value.donor){query<<id<<',';}query<<" lengths=";
      for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)query<<sensor.SensorEdgeLength(coarse.v[a].p,coarse.v[b].p)<<',';
      query<<'\n';
     }
     Ordered(parent,query.str());
    }
    const auto& q=query_service.Statistics();std::ostringstream cost;cost<<std::setprecision(17)
     <<"LARGE_SUPPORT_COST n="<<n<<" mode="<<mode<<" rank="<<parent.rank<<" ranks="<<parent.size
     <<" candidates="<<q.index_candidates<<" filtered="<<q.filtered<<" payloads="<<q.payloads<<" max_patch="<<q.maximum_patch
     <<" build_s="<<q.build_seconds<<" import_s="<<q.import_seconds<<" search_nested_s="<<q.search_seconds
     <<" filter_nested_s="<<q.filter_seconds<<" work_bytes="<<q.maximum_work_bytes<<'\n';Ordered(parent,cost.str());
   }
   const auto& st=service.Statistics();assert((st.filter_candidates>0)==(n>3));std::ostringstream report;report<<std::setprecision(17)
    <<"LARGE_IMPORT_COST n="<<n<<" rank="<<parent.rank<<" ranks="<<parent.size<<" candidates="<<st.index_candidates
    <<" filtered="<<st.filtered<<" payloads="<<st.payloads<<" max_patch="<<st.maximum_patch
    <<" imports="<<st.imports<<" rejected="<<st.rejected<<" build_s="<<st.build_seconds<<" import_s="<<st.import_seconds
    <<" search_nested_s="<<st.search_seconds<<" filter_nested_s="<<st.filter_seconds<<" work_bytes="<<st.maximum_work_bytes<<'\n';
   Ordered(parent,report.str());
  }
  {
   const auto grid=Grid(7);DonorImport service(parent,true);std::string reason;
   assert(service.Build(Source(parent,grid,1),reason));const auto resident=service.ResidentBytes();
   const auto imported=service.ImportMetricCells(parent.rank ? std::vector<Cell>{} : std::vector<Cell>{Grid(1)[0]});
   assert(!imported.valid && imported.reason.find("patch budget")!=std::string::npos && service.ResidentBytes()==resident);
   if(!parent.rank)std::cout<<"LARGE_SUPPORT_LIMIT n=7 valid=0 source=2058\n";
  }
  for(const std::string kind:{"coarse","wedge","dense","dense_wedge","dense_coarsen","dense_repair","dense_reject"}) {
   const bool wedge=kind!="coarse" && kind!="dense";
   const bool coarsen=kind=="dense_coarsen",repair=kind=="dense_repair",reject=kind=="dense_reject";
   const unsigned n=kind.rfind("dense",0)==0 ? 5 : 4;
   const double strength=coarsen ? .25 : (repair || !wedge ? 1. : 4.);
   const auto grid=Grid(n);const auto source=Source(parent,grid,strength);
   std::vector<Facet> facets;auto input=Coarse(parent,n,facets,wedge);
   if(coarsen || repair) {
    std::vector<std::vector<OwnedCell>> distribute(parent.size);
    for(const auto& row:input) {
     Patch old,fresh;old.cells={row.second.cell};const auto faces=Faces(row.second.cell);
     for(unsigned i=0;i<4;++i)if(row.second.physical[i].present)old.surface.push_back({faces[i],row.second.physical[i].facet});
     std::string reason;const Id next=Id((n+1)*(n+1)*(n+1));
     if(coarsen) {
      const auto a=old.cells[0].v[0],b=old.cells[0].v[1];
      assert(SplitEdge(old,{a.id,b.id},{next,{(a.p.x+b.p.x)/2,(a.p.y+b.p.y)/2,(a.p.z+b.p.z)/2}},
          20000,PlanarReference(facets),fresh,reason));
     } else {
      Point p;for(unsigned i=1;i<4;++i){const auto v=old.cells[0].v[i].p;p.x+=v.x/32;p.y+=v.y/32;p.z+=v.z/32;}
      assert(Cone(old.cells,{next,p},20000,fresh.cells,reason));fresh.surface=old.surface;
     }
     std::map<Face,Id> physical;for(auto face:fresh.surface)physical.emplace(Key(face.v),face.facet);
     for(auto cell:fresh.cells) {
      OwnedCell child;child.cell=cell;child.version=23;const auto ff=Faces(cell);
      for(unsigned i=0;i<4;++i){auto f=physical.find(Key(ff[i]));if(f!=physical.end())child.physical[i]={true,f->second};}
      distribute[cell.id%parent.size].push_back(child);
     }
    }
    input.clear();for(auto row:parent.exchange(distribute))input.emplace(row.cell.id,row);
   }
   const auto saved=Bytes(input);
   std::vector<std::vector<OwnedCell>> initial_to(parent.size);
   for(auto row:input)initial_to[0].push_back(row.second);
   const auto initial=parent.exchange(initial_to);
   if(!parent.rank)Export(folder+"/"+kind+"_initial.json",initial,facets,grid,strength);
   for(int m:std::set<int>{parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
    const std::string label=kind+"_m"+std::to_string(m);const auto start=std::chrono::steady_clock::now();
    auto elapsed=[](auto t){return std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();};
    auto phase=std::chrono::steady_clock::now();std::vector<std::vector<OwnedCell>> to(parent.size);
    std::vector<std::vector<DonorCell>> sensor_to(parent.size);
    for(auto row:input)to[row.first%m].push_back(row.second);
    for(auto row:source)sensor_to[row.cell.id%m].push_back(row);
    const auto migrated=parent.exchange(to);const auto source_rows=parent.exchange(sensor_to);const double migration_s=elapsed(phase);
    phase=std::chrono::steady_clock::now();MPI_Comm subset=MPI_COMM_NULL;
    MPI_Comm_split(parent.comm,parent.rank<m ? 0 : MPI_UNDEFINED,parent.rank,&subset);const double comm_s=elapsed(phase);
    phase=std::chrono::steady_clock::now();Owned adapted;bool valid=true;
    if(subset!=MPI_COMM_NULL) {
     World worker(subset);Owned owned;for(auto row:migrated)owned.emplace(row.cell.id,row);
     const auto before=Bytes(owned);EngineOptions options;options.timings=true;if(reject)options.maximum_rounds=1;
     const auto result=coarsen ? Coarsen(worker,owned,source_rows,PlanarReference(facets),{},options) :
         (repair ? Repair(worker,owned,source_rows,PlanarReference(facets),{},options) :
         Refine(worker,owned,source_rows,PlanarReference(facets),{},options));const auto& s=result.stats;
     valid=result.valid;assert(valid==!reject);
     if(kind=="dense" || kind=="dense_wedge" || reject)assert(s.query_import_retries>0);
     assert(Bytes(owned)==before);assert(s.other_seconds>=-1e-12);
     if(!wedge)assert(Bytes(result.cells)==before && s.accepted==0);
     if(kind=="dense_wedge")assert(s.initial_cells==owned.size() && worker.sum(int(s.accepted))>0 && s.private_query_retries>0);
     if(wedge)assert(worker.sum(int(s.accepted))>0);
     if(n==5 && wedge)assert(s.private_query_retries>0);
     if(n==4 || !wedge)assert(s.private_query_retries==0);
     if(reject)assert(result.reason.find("round limit")!=std::string::npos);
     std::ostringstream report;report<<std::setprecision(17)<<"LARGE_ENGINE case="<<label<<" rank="<<worker.rank
      <<" workers="<<m<<" valid="<<valid<<" source="<<grid.size()<<" initial="<<s.initial_cells
      <<" final_local="<<result.cells.size()<<" moved="<<s.moved<<" reconnected="<<s.reconnected<<" rounds="<<s.rounds<<" accepted="<<s.accepted<<" rejected="<<s.rejected
      <<" batches="<<s.donor_batches<<" retries="<<s.batch_retries<<" query_import_retries="<<s.query_import_retries<<" private_query_retries="<<s.private_query_retries<<" candidates="<<s.donor_candidates
      <<" filtered="<<s.donor_filtered<<" payloads="<<s.donor_payloads<<" max_patch="<<s.donor_maximum_patch
      <<" total_s="<<s.total_seconds<<" init_s="<<s.initialization_seconds<<" seed_s="<<s.seed_seconds
      <<" selection_s="<<s.selection_seconds<<" dependency_s="<<s.dependency_seconds<<" reservation_s="<<s.reservation_seconds
      <<" donor_s="<<s.donor_seconds<<" reconstruction_s="<<s.reconstruction_seconds<<" metric_s="<<s.metric_seconds
      <<" routing_s="<<s.routing_seconds<<" publication_s="<<s.publication_seconds<<" update_s="<<s.update_seconds
      <<" completion_s="<<s.completion_seconds<<" other_s="<<s.other_seconds<<" cpu_s="<<s.process_cpu_seconds
      <<" filter_nested_s="<<s.donor_filter_seconds<<" search_nested_s="<<s.donor_search_seconds
      <<" trace_nested_s="<<s.edge_trace_seconds<<" work_bytes="<<s.maximum_work_bytes<<'\n';Ordered(worker,report.str());
     adapted=result.cells;MPI_Comm_free(&subset);
    }
    const auto failed=parent.maximum(!valid);const double worker_s=elapsed(phase);assert(bool(failed)==reject);
    phase=std::chrono::steady_clock::now();to.assign(parent.size,{});
    for(auto row:adapted)to[row.first%parent.size].push_back(row.second);
    const auto returned=parent.exchange(to);Owned output;for(auto row:returned)output.emplace(row.cell.id,row);
    const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);const double return_s=elapsed(phase);
    assert(Bytes(input)==saved);if(coarsen)assert(count==1);else if(wedge)assert(count>1 && count<=128);
    const double total_s=elapsed(start),other_s=total_s-migration_s-comm_s-worker_s-return_s;assert(other_s>=-1e-12);
    std::ostringstream event;event<<std::setprecision(17)<<"LARGE_EVENT case="<<label<<" rank="<<parent.rank<<" ranks="<<parent.size
     <<" workers="<<m<<" valid="<<!failed<<" final="<<count<<" parent_s="<<total_s<<" migration_s="<<migration_s
     <<" worker_comm_s="<<comm_s<<" worker_s="<<worker_s<<" return_s="<<return_s<<" other_s="<<other_s<<'\n';Ordered(parent,event.str());
    to.assign(parent.size,{});for(auto row:output)to[0].push_back(row.second);const auto all=parent.exchange(to);
    if(!parent.rank)Export(folder+"/"+label+(failed ? "_rejected" : "")+".json",all,facets,grid,strength);
   }
  }
  if(!parent.rank)std::cout<<"PASS large-source exact-filter controls\n";
 }
 SU2_MPI::Finalize();return 0;
}
