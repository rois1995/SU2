/*!
 * \file BLDeform16.inl
 * \brief DC-16 room step of the BL growth layers (closed advancing layer, Alauzet-Loseille-Marcum 2017 Alg. 2): before the
 *        certified lifts of layer k, the volume above the active front is deformed. The planned displacement of every
 *        active top is prescribed, the free vertices near the front are solved by the shared dimension-generic
 *        deformation module (CNativeDeformation.hpp: stiffened P1 Laplacian, symmetry planes sliding) and moved in
 *        pseudo-time with validity freezing; the changed cells are committed as one unit. The layer itself is then
 *        inserted by the unchanged LayerTransaction (old top kept, the cells above re-attached to the new top, fill
 *        cells over the stack) in the room made. Serial, not reentrant: enter through PipelineScope.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <malloc.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include "../../../../Common/include/adaptation/CNativeDeformation.hpp"
namespace dc16 {
using namespace dc12;
using junction_wp0::V;

// switches (production defaults; tests may change them inside a scope)
inline bool roomStep=true;            // the deformation before each layer of G
inline long double roomNeed=1.5L;     // a top needs room when its above-star kernel bound is below roomNeed x its step
inline size_t roomRings=2;            // free vertices of the needy above stars plus roomRings-1 further rings
inline size_t roomMaxVertices=400000; // bound of one region (larger regions are cut at the bound, in ring order)
inline double roomEuclidRatio=.25;    // pseudo-time kernel: Euclidean mean ratio may not drop below this x its old value

inline long RssKB(){long pages=0,resident=0;std::ifstream in("/proc/self/statm");in>>pages>>resident;return resident*(sysconf(_SC_PAGESIZE)/1024);}
inline long rssCeilingKB=long(16)<<20;  // G stops planning above this resident set (machine safety; reported as limit:memory)
/*! Current RSS and malloc in-use bytes (diagnostics of the DC-15 G memory jump). */
inline std::string MemoryLine(const State& s) {
  long pages=0,resident=0;{std::ifstream in("/proc/self/statm");in>>pages>>resident;}
  const struct mallinfo mi=mallinfo();  // glibc 2.31: 32-bit fields (modulo 4 GiB; read together with rss_now)
  std::ostringstream o;o<<"rss_now_kB="<<resident*(sysconf(_SC_PAGESIZE)/1024)<<" heap_inuse_kB_mod4G="<<unsigned(mi.uordblks)/1024<<" heap_free_kB_mod4G="<<unsigned(mi.fordblks)/1024
   <<" mmap_kB="<<unsigned(mi.hblkhd)/1024<<" cells="<<s.mesh.size()<<" vertices="<<s.vertex_cells.size()<<" lengths="<<s.lengths.size()<<" geometry="<<dc9::geometryLengths.size()
   <<" shape="<<dc9::shapeCache.size()<<" height="<<dc9::heightCache.size()<<" balanced="<<dc9::balancedCache.size()<<" overlay="<<s.overlay.size()<<" boxes="<<s.box_grid.size();
  return o.str();
}

inline long double EuclidQuality(const std::array<SU2NativeDeform::Vec<3>,4>& p) {
  auto sub=[](const SU2NativeDeform::Vec<3>& a,const SU2NativeDeform::Vec<3>& b){return V{(long double)a[0]-b[0],(long double)a[1]-b[1],(long double)a[2]-b[2]};};
  const V a=sub(p[1],p[0]),b=sub(p[2],p[0]),c=sub(p[3],p[0]);const long double vol=std::abs(Dot(a,Cross(b,c)))/6;
  long double l2=0;for(int i=0;i<4;++i)for(int j=i+1;j<4;++j){const V e=sub(p[j],p[i]);l2+=Dot(e,e);}
  return l2>0?12*std::pow(3*vol,2.L/3)/l2:0;
}

struct RoomStats { size_t calls=0,needy=0,region=0,cells=0,moved=0,frozen=0,metric_frozen=0,slide=0,slide_reverted=0,iterations=0,unconverged=0,commits=0,refused=0;
  double seconds=0,solve=0,move=0,metric=0,commit=0,batch=0,slides=0,problem=0;std::map<std::string,size_t> why; };
inline RoomStats roomStats;

/*! Room step of one layer. `plan`: planned top -> target; `fixed`: registry identities (never moved). */
inline void MakeRoomImpl(State& s,const std::function<bool(Id)>& below,const std::set<Id>& fixed,const std::map<Id,V>& plan,std::ostream& log);
/*! Review 2: an exception before the commit (planning, solve, checks) leaves the mesh unchanged and is only counted; an
 *  exception during the commit (Apply) propagates and fails the event (Apply is not exception safe). */
inline bool roomCommitting=false;
inline void MakeRoom(State& s,const std::function<bool(Id)>& below,const std::set<Id>& fixed,const std::map<Id,V>& plan,std::ostream& log) {
  roomCommitting=false;
  try{MakeRoomImpl(s,below,fixed,plan,log);}
  catch(const std::exception& e){if(roomCommitting)throw;++roomStats.why[std::string("exception: ")+e.what()];}
}
inline void MakeRoomImpl(State& s,const std::function<bool(Id)>& below,const std::set<Id>& fixed,const std::map<Id,V>& plan,std::ostream& log) {
  Scope scope("G MakeRoom");
  using namespace SU2NativeDeform;
  const auto t0=Clock::now();auto& st=roomStats;++st.calls;
  // 1 needy tops: the kernel bound of the above star along the step is below roomNeed x the step
  std::vector<Id> needy;
  for(const auto& p:plan){if(!s.vertex_cells.count(p.first))continue;const V x=Of(NodePoint(s,p.first)),d=p.second-x;const long double L=Norm(d);if(!(L>0))continue;
    std::vector<Cell> above;for(Id id:StarOf(s,p.first))if(!below(id))above.push_back(s.mesh.at(id).cell);
    const auto k=dc9::KernelBound(above,p.first,(1/L)*d);if(k.delta<roomNeed*L)needy.push_back(p.first);}
  st.needy+=needy.size();
  if(needy.empty()){st.seconds+=Seconds(t0);return;}
  // 2 vertex classes: 0 free, 1 slide (one role-0 continuation plane), 2 pinned
  std::unordered_map<Id,int> cls;std::unordered_map<Id,V> slideNormal;
  auto classify=[&](Id v)->int{
    const auto f=cls.find(v);if(f!=cls.end())return f->second;int c=0;
    if(fixed.count(v)||plan.count(v))c=2;
    std::vector<V> normals;
    if(!c)for(Id id:StarOf(s,v)){if(below(id)||WallChild(s,id)){c=2;break;}
      const auto& r=s.mesh.at(id);const auto faces=CellFaces(r.cell);
      for(unsigned k=0;k<4&&!c;++k){if(!r.physical[k].present||!Has(faces[k],v))continue;const int m=s.reference->Get(r.physical[k].facet).marker;
        const auto role=dc9::markerRoles.find(m);
        if(s.walls.count(m)||dc9::artificialMarkers.count(m)||role==dc9::markerRoles.end()||role->second!=0){c=2;break;}
        const V n=Outward(Of(NodePoint(s,faces[k][0])),Of(NodePoint(s,faces[k][1])),Of(NodePoint(s,faces[k][2])));
        bool dup=false;for(const auto& x:normals)dup=dup||std::abs(Dot(x,n))>1-1e-9L;if(!dup)normals.push_back(n);}
      if(c)break;}
    if(!c&&!normals.empty()){if(normals.size()==1){c=1;slideNormal[v]=normals.front();}else c=2;}
    cls[v]=c;return c;};
  // 3 region: free vertices of the needy above stars, then further rings through non-below cells
  std::vector<Id> region;std::unordered_set<Id> inRegion;
  auto add=[&](Id v){if(region.size()>=roomMaxVertices||inRegion.count(v))return;if(classify(v)<2){inRegion.insert(v);region.push_back(v);}};
  for(Id t:needy)for(Id id:StarOf(s,t))if(!below(id))for(auto n:s.mesh.at(id).cell.v)add(n.id);
  for(size_t ring=1,from=0;ring<roomRings;++ring){const size_t to=region.size();
    for(size_t i=from;i<to;++i)for(Id id:StarOf(s,region[i]))for(auto n:s.mesh.at(id).cell.v)add(n.id);from=to;}
  if(region.empty()){++st.why["no free vertex above the needy tops"];st.seconds+=Seconds(t0);return;}
  // 4 problem: cells touching region vertices (never below or first-child cells: their vertices are pinned)
  Problem<3> pb;std::unordered_map<Id,uint32_t> local;std::vector<Id> global;
  auto index=[&](Id v)->uint32_t{const auto f=local.find(v);if(f!=local.end())return f->second;const uint32_t k=uint32_t(global.size());local.emplace(v,k);global.push_back(v);return k;};
  for(Id v:region)index(v);
  std::vector<Id> cellIds;{std::unordered_set<Id> seen;for(Id v:region)for(Id id:StarOf(s,v))if(seen.insert(id).second)cellIds.push_back(id);}
  std::sort(cellIds.begin(),cellIds.end());
  for(Id id:cellIds){const auto& c=s.mesh.at(id).cell;std::array<uint32_t,4> q;for(unsigned k=0;k<4;++k)q[k]=index(c.v[k].id);pb.cells.push_back(q);}
  const size_t n=global.size();pb.x.resize(n);pb.kind.assign(n,Constraint::Fixed);pb.value.assign(n,Vec<3>{0,0,0});pb.normals.assign(n,{});
  for(size_t i=0;i<n;++i){const auto p=NodePoint(s,global[i]);pb.x[i]={p.x,p.y,p.z};}
  for(size_t i=0;i<region.size();++i){const int c=classify(region[i]);pb.kind[i]=c==1?Constraint::Slide:Constraint::Free;
    if(c==1){const V m=slideNormal.at(region[i]);pb.normals[i]={Vec<3>{double(m.x),double(m.y),double(m.z)}};++st.slide;}}
  for(size_t i=region.size();i<n;++i){const auto f=plan.find(global[i]);if(f==plan.end())continue;const V d=f->second-Of(NodePoint(s,global[i]));pb.value[i]={double(d.x),double(d.y),double(d.z)};}
  st.region+=region.size();st.cells+=cellIds.size();
  st.problem+=Seconds(t0);
  // 5 solve
  std::vector<Vec<3>> d;SolveOptions so;SolveReport sr;
  try{if(!SolveDisplacement<3>(pb,d,so,sr))++st.unconverged;}catch(const std::exception& e){++st.why[std::string("solve: ")+e.what()];st.seconds+=Seconds(t0);return;}
  st.iterations+=sr.iterations;st.solve+=sr.seconds;
  // 6 pseudo-time move: exact orientation and a Euclidean shape floor relative to the old cell
  std::vector<long double> q0(pb.cells.size());
  for(size_t c=0;c<pb.cells.size();++c){std::array<Vec<3>,4> p;for(int k=0;k<4;++k)p[k]=pb.x[pb.cells[c][k]];q0[c]=EuclidQuality(p);}
  auto pt=[](const Vec<3>& a){return Point{a[0],a[1],a[2]};};
  const CellCheck<3> kernel=[&](size_t c,const std::array<Vec<3>,4>& p){
    if(!(Orientation(pt(p[0]),pt(p[1]),pt(p[2]),pt(p[3]))>0))return false;return EuclidQuality(p)>=roomEuclidRatio*q0[c];};
  std::vector<Vec<3>> y;std::vector<char> frozen;MoveOptions mo;MoveReport mr;
  PseudoTimeMove<3>(pb,d,kernel,y,frozen,mo,mr);st.move+=mr.seconds;st.frozen+=mr.frozen;
  // 7 metric validity of every changed cell (q > 0, J >= 1e-3 at the actual centroid, as Admit12); a failing cell
  // returns its moved vertices to the original position, re-checked until stable (the original state is valid)
  std::vector<std::vector<uint32_t>> star(n);for(size_t c=0;c<pb.cells.size();++c)for(auto v:pb.cells[c])star[v].push_back(uint32_t(c));
  auto cellOf=[&](size_t c,const std::vector<Vec<3>>& at){Cell x;x.id=0;for(int k=0;k<4;++k)x.v[k]=Node{global[pb.cells[c][k]],pt(at[pb.cells[c][k]])};return x;};
  std::vector<char> check(pb.cells.size(),0);
  // review 1: a settle that does not close within its bound cancels the whole move (the original state is valid)
  auto settle=[&]{const auto tm=Clock::now();bool closed=false;
   for(unsigned round=0;round<256;++round){std::vector<uint32_t> reverted;
     for(size_t c=0;c<pb.cells.size();++c){if(!check[c])continue;check[c]=0;bool changed=false;for(auto v:pb.cells[c])changed=changed||y[v]!=pb.x[v];if(!changed)continue;
       const Cell x=cellOf(c,y);const bool ok=Orientation(x.v[0].p,x.v[1].p,x.v[2].p,x.v[3].p)>0&&Valid(Shape(s,x));
       if(!ok)for(auto v:pb.cells[c])if(y[v]!=pb.x[v]){y[v]=pb.x[v];reverted.push_back(v);++st.metric_frozen;}}
     if(reverted.empty()){closed=true;break;}for(auto v:reverted)for(auto c:star[v])check[c]=1;}
   if(!closed){y=pb.x;std::fill(check.begin(),check.end(),0);++st.why["settle bound: move cancelled"];}
   st.metric+=Seconds(tm);};
  for(size_t c=0;c<pb.cells.size();++c)for(auto v:pb.cells[c])if(y[v]!=pb.x[v])check[c]=1;
  settle();
  // 8 commit. (a) interior batch: every moved free vertex in ONE Apply over the complete union of their stars (its skin
  // holds no moved vertex, so physical faces and bindings are unchanged; no private parameter context is needed). The
  // sliding vertices are held at their old positions for (a), every cell around them re-validated first. (b) each
  // sliding vertex then moves alone through the certified MoveVertex kernel (binding on its plane, chart containment,
  // positivity), its fresh cells re-validated; a refused one stays. Moved vertices get new identities (pinned
  // registry, below-cell and first-child vertices never move).
  const auto tc=Clock::now();
  std::vector<std::pair<uint32_t,Vec<3>>> slides;
  for(size_t i=0;i<region.size();++i)if(pb.kind[i]==Constraint::Slide&&y[i]!=pb.x[i]){slides.push_back({uint32_t(i),y[i]});y[i]=pb.x[i];for(auto c:star[i])check[c]=1;}
  if(!slides.empty())settle();
  {std::vector<uint32_t> moved;for(size_t i=0;i<region.size();++i)if(y[i]!=pb.x[i])moved.push_back(uint32_t(i));
   if(!moved.empty()){
     std::vector<Id> ids;{std::unordered_set<Id> seen;for(auto v:moved)for(auto c:star[v])if(seen.insert(cellIds[c]).second)ids.push_back(cellIds[c]);}
     std::sort(ids.begin(),ids.end());
     std::unordered_map<Id,Node> to;Id next=s.next_node;for(auto v:moved){if(fixed.count(global[v]))throw std::logic_error("DC16 room: a registry identity would move");to[global[v]]=Node{next++,pt(y[v])};}
     // the fresh patch is built from the cell rows directly: physical faces keep their facets (none holds a moved
     // vertex) and the boundary bindings in the overlay stay valid (no boundary point moves), so no parameter export
     const auto tb=Clock::now();Patch fresh;bool ok=true;
     for(size_t i=0;i<ids.size()&&ok;++i){const auto& row=s.mesh.at(ids[i]);const auto faces=CellFaces(row.cell);
       for(unsigned k=0;k<4;++k)if(row.physical[k].present){for(Id x:faces[k])ok=ok&&!to.count(x);fresh.surface.push_back({faces[k],row.physical[k].facet});}
       Cell c=row.cell;for(auto& x:c.v){const auto f=to.find(x.id);if(f!=to.end())x=f->second;}c.id=s.next_cell+i;
       ok=ok&&Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0;fresh.cells.push_back(c);}
     if(ok){roomCommitting=true;Apply(s,ids,fresh);roomCommitting=false;s.next_node=std::max(s.next_node,next);++st.commits;st.moved+=moved.size();}
     else{++st.refused;++st.why["interior commit: skin or positivity"];}
     st.batch+=Seconds(tb);}}
  const auto ts=Clock::now();
  for(const auto& sl:slides){const Id g=global[sl.first];if(!s.vertex_cells.count(g))continue;
    const auto starIds=StarOf(s,g);if(starIds.size()>MaximumCavityCells){++st.slide_reverted;++st.why["slide: star size"];continue;}
    std::vector<Id> ids(starIds.begin(),starIds.end());Patch old,fresh;std::string why;bool ok=false;
    try{old=PatchOf(s,ids);ok=MoveVertex(old,g,Node{s.next_node,pt(sl.second)},s.next_cell,*s.reference,fresh,why);}catch(const std::exception& e){ok=false;why=e.what();}
    if(ok)for(const auto& c:fresh.cells)ok=ok&&Valid(Shape(s,c));
    if(!ok){++st.slide_reverted;++st.why["slide: "+swbi_dc2::Category(why).substr(0,60)];continue;}
    roomCommitting=true;Apply(s,ids,fresh);roomCommitting=false;++st.moved;}
  st.slides+=Seconds(ts);
  st.commit+=Seconds(tc);st.seconds+=Seconds(t0);
  log<<"DC16 room needy="<<needy.size()<<" region="<<region.size()<<" cells="<<cellIds.size()<<" cg="<<sr.iterations<<(sr.converged?"":" (unconverged)")
     <<" pseudo_frozen="<<mr.frozen<<" stations="<<mr.reached<<" metric_frozen_total="<<st.metric_frozen<<" commits="<<st.commits<<" t="<<Seconds(t0)<<"\n";
}
inline std::string JsonRoom(const RoomStats& r) {
  std::ostringstream o;o<<std::setprecision(8)<<"{\"calls\":"<<r.calls<<",\"needy_tops\":"<<r.needy<<",\"region_vertices\":"<<r.region<<",\"cells\":"<<r.cells<<",\"moved\":"<<r.moved
   <<",\"pseudo_frozen\":"<<r.frozen<<",\"metric_reverted\":"<<r.metric_frozen<<",\"sliding\":"<<r.slide<<",\"sliding_reverted\":"<<r.slide_reverted<<",\"cg_iterations\":"<<r.iterations
   <<",\"unconverged\":"<<r.unconverged<<",\"commits\":"<<r.commits<<",\"refused\":"<<r.refused<<",\"seconds\":{\"total\":"<<r.seconds<<",\"solve\":"<<r.solve<<",\"move\":"<<r.move
   <<",\"metric\":"<<r.metric<<",\"commit\":"<<r.commit<<",\"batch\":"<<r.batch<<",\"slides\":"<<r.slides<<",\"problem\":"<<r.problem<<"},\"why\":{";
  bool k=false;for(const auto& w:r.why){o<<(k?",":"")<<junction_wp0::Json(w.first)<<':'<<w.second;k=true;}o<<"}}";return o.str();
}
}  // namespace dc16
