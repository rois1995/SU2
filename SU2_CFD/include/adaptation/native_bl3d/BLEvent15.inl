/*!
 * \file BLEvent15.inl
 * \brief DC-15 pieces of the FIRST_ROW event: the independent validity decision of intermediate events (exact
 *        orientation, conformity of the cell complex and its physical skin, first heights with no construction cache)
 *        and the row completion pass (every wall face gets a passing first child, also at multi-plane corners).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <unordered_set>
namespace dc15 {
using namespace dc13;

/*! Validity of a returned cell set, computed from the cells, their physical face flags, the authority and the
 *  prescribed heights only (no index, cache or field of the construction). */
struct Validity {
  size_t cells=0,nonpositive=0,faces=0,boundary_faces=0,interior_faces=0,overshared=0,same_orientation=0,interior_physical=0,
         open_unbound=0,wall_faces=0,height_failures=0,unknown_marker=0,vertices=0,inconsistent_points=0,skin_edges=0,nonmanifold_skin_edges=0;
  std::string witness;
  bool Valid()const{return cells>0&&!nonpositive&&!overshared&&!same_orientation&&!interior_physical&&!open_unbound&&!unknown_marker&&!height_failures&&!inconsistent_points&&!nonmanifold_skin_edges;}
};
inline Validity IndependentValidity(const State& s) {
  Scope scope("DC15 IndependentValidity");
  Validity v;v.cells=s.mesh.size();
  struct Row{Face key;Face oriented;Id cell;unsigned k;bool physical;};
  std::vector<Row> rows;rows.reserve(4*s.mesh.size());
  auto note=[&](const std::string& w){if(v.witness.size()<600)v.witness+=(v.witness.empty()?"":"; ")+w;};
  {std::unordered_map<Id,Point> at;at.reserve(s.mesh.size());
   for(const auto& r:s.mesh)for(const auto& n:r.second.cell.v){const auto f=at.emplace(n.id,n.p);
     if(!f.second&&!SamePoint(f.first->second,n.p)){++v.inconsistent_points;note("vertex "+std::to_string(n.id)+" has two coordinates");}}
   v.vertices=at.size();}
  std::map<std::pair<Id,Id>,int> skinEdges;  // directed skin edges: each must appear once in each direction
  for(const auto& r:s.mesh){const auto& c=r.second.cell;
    if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0)){++v.nonpositive;note("cell "+std::to_string(r.first)+" not positive");}
    const auto f=CellFaces(c);for(unsigned k=0;k<4;++k)rows.push_back({Sorted(f[k]),f[k],r.first,k,r.second.physical[k].present});}
  std::sort(rows.begin(),rows.end(),[](const Row& a,const Row& b){return a.key<b.key;});
  auto rotationOf=[](Face a,const Face& b){for(int r=0;r<3;++r){if(a==b)return true;std::rotate(a.begin(),a.begin()+1,a.end());}return false;};
  std::map<Id,bool> heightOf;  // owner cell -> verdict, local to this check
  for(size_t i=0;i<rows.size();) {
    size_t j=i;while(j<rows.size()&&rows[j].key==rows[i].key)++j;++v.faces;
    if(j-i==1){++v.boundary_faces;const auto& r=rows[i];for(int e=0;e<3;++e)++skinEdges[{r.oriented[e],r.oriented[(e+1)%3]}];
      if(!r.physical){++v.open_unbound;note("open face of cell "+std::to_string(r.cell)+" without a physical binding");}
      else{int marker=-1;try{marker=s.reference->Get(s.mesh.at(r.cell).physical[r.k].facet).marker;}catch(const std::exception&){marker=-1;}
        if(marker<0||marker>=int(s.markers.size())){++v.unknown_marker;note("physical face of cell "+std::to_string(r.cell)+" without a known marker");}
        else if(s.walls.count(marker)){++v.wall_faces;auto h=heightOf.find(r.cell);
          if(h==heightOf.end()){bool ok=false;std::string why;try{ok=HeightT(s,PatchOf(s,{r.cell}),why);}catch(const std::exception&){ok=false;}h=heightOf.emplace(r.cell,ok).first;}
          if(!h->second){++v.height_failures;if(v.height_failures<=4)note("wall face of cell "+std::to_string(r.cell)+" fails the first-height rule");}}}}
    else if(j-i==2){++v.interior_faces;
      if(rows[i].physical||rows[i+1].physical){++v.interior_physical;note("interior face carries a physical binding (cell "+std::to_string(rows[i].cell)+")");}
      Face back=rows[i+1].oriented;std::swap(back[1],back[2]);
      if(!rotationOf(rows[i].oriented,back)){++v.same_orientation;note("cells "+std::to_string(rows[i].cell)+","+std::to_string(rows[i+1].cell)+" share a face with the same orientation");}}
    else{++v.overshared;note("face shared by "+std::to_string(j-i)+" cells");}
    i=j;
  }
  for(const auto& e:skinEdges){if(e.first.first<e.first.second)++v.skin_edges;const auto b=skinEdges.find({e.first.second,e.first.first});
    if(e.second!=1||b==skinEdges.end()||b->second!=1){++v.nonmanifold_skin_edges;if(v.nonmanifold_skin_edges<=4)note("skin edge "+std::to_string(e.first.first)+"-"+std::to_string(e.first.second)+" not two-manifold");}}
  return v;
}
inline std::string JsonValidity(const Validity& v) {
  std::ostringstream o;o<<"{\"valid\":"<<v.Valid()<<",\"cells\":"<<v.cells<<",\"nonpositive\":"<<v.nonpositive<<",\"faces\":"<<v.faces<<",\"boundary_faces\":"<<v.boundary_faces
   <<",\"interior_faces\":"<<v.interior_faces<<",\"overshared\":"<<v.overshared<<",\"same_orientation\":"<<v.same_orientation<<",\"interior_physical\":"<<v.interior_physical
   <<",\"open_unbound\":"<<v.open_unbound<<",\"unknown_marker\":"<<v.unknown_marker<<",\"wall_faces\":"<<v.wall_faces<<",\"height_failures\":"<<v.height_failures
   <<",\"vertices\":"<<v.vertices<<",\"inconsistent_points\":"<<v.inconsistent_points<<",\"skin_edges\":"<<v.skin_edges<<",\"nonmanifold_skin_edges\":"<<v.nonmanifold_skin_edges
   <<",\"witness\":"<<junction_wp0::Json(v.witness)<<"}";return o.str();
}

/*! Deficits of an intermediate event without integrating every edge: a measured (cached) length is used when it exists,
 *  otherwise the midpoint estimate sqrt(e^T M(mid) e), counted as `estimated`. Shapes and heights as the cached Final. */
struct Estimated { dc9::Gates8 gates;size_t estimated=0,metric_invalid=0,bad_first=0; };
inline Estimated FinalEstimated(State& s) {
  Scope scope("DC15 FinalEstimated");
  Estimated out;auto& g=out.gates;g.cells=s.mesh.size();std::unordered_set<uint64_t> seen;seen.reserve(8*s.mesh.size());
  std::set<Edge> seenBig;  // ids beyond 2^32 (never expected) fall back to an ordered set
  for(const auto& row:s.mesh){const auto& c=row.second.cell;const auto sh=MeshShape(s,row.first);g.q=std::min(g.q,sh.q);g.J=std::min(g.J,sh.J);
    if(Bad(sh)){++g.bad_shape;if(WallChild(s,row.first))++out.bad_first;}if(!(sh.q>0&&sh.J>=ValiditySliverJacobian))++out.metric_invalid;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};
      if(e[1]<(Id(1)<<32)){if(!seen.insert((uint64_t(e[0])<<32)|uint64_t(e[1])).second)continue;}else if(!seenBig.insert(e).second)continue;
      long double l=0;bool est=false;try{l=LenOrEstimate(s,e[0],e[1],est);}catch(const std::exception&){++g.unmeasurable;continue;}
      out.estimated+=est;if(std::isfinite(l))g.lmax=std::max(g.lmax,l);
      if(l>MaximumMetricEdge){++g.long_edges;const int k=EdgeClass(s,e);(k==2?g.long_seam:k==1?g.long_wall:k==3?g.long_other_physical:g.long_interior)++;}}}
  for(const auto& wf:WallFaces(s)){++g.wall_faces;if(!HeightOk(s,wf.first))++g.height_failures;}
  g.complete=!g.bad_shape&&!g.long_edges&&!g.height_failures&&!g.unmeasurable;return out;
}
inline std::string JsonEstimated(const Estimated& e) {
  const auto& g=e.gates;std::ostringstream o;o<<std::setprecision(10)<<"{\"kind\":\"cached lengths, midpoint estimate for unmeasured edges\",\"cells\":"<<g.cells<<",\"q_min\":"<<g.q<<",\"J_min\":"<<g.J
   <<",\"edge_max\":"<<double(g.lmax)<<",\"bad_shape\":"<<g.bad_shape<<",\"bad_first_child\":"<<e.bad_first<<",\"metric_invalid\":"<<e.metric_invalid<<",\"long_edges\":"<<g.long_edges
   <<",\"long_by_class\":{\"wall\":"<<g.long_wall<<",\"seam\":"<<g.long_seam<<",\"other_physical\":"<<g.long_other_physical<<",\"interior\":"<<g.long_interior<<"}"
   <<",\"estimated_edges\":"<<e.estimated<<",\"wall_faces\":"<<g.wall_faces<<",\"height_failures\":"<<g.height_failures<<",\"unmeasurable\":"<<g.unmeasurable<<"}";return o.str();
}

/*! Cavity first child (completion route 3): for a wall face whose own cell cannot hold a point at the prescribed height
 *  (the cell is thinner than h0: "cone obstructed"), a point p at height h0 over a foot inside the face is coned over
 *  the star-shaped visibility cavity of p seeded with the face's cell; no other first child and no physical face may
 *  enter the cavity; D.2 and Admit12 Layer (validity, every wall face of the patch passing its height) admit it. */
inline bool CavityFirstChild(State& s,const Rows10& rows,Id cell,const Face& key,std::set<Face>& certified,std::string& why) {
  Scope scope("DC15 CavityFirstChild");
  // code review 1: column cells (stars of registered apices) never enter the cavity; every registered identity present
  // in the cavity (lids, apices, copies) must survive the cone
  std::set<Id> registered;for(const auto& r:rows.lift)registered.insert(r.second);for(const auto& r:rows.apex)registered.insert(r.second);
  for(const auto& r:rows.copies)for(Id c:r.second.ids)registered.insert(c);
  std::set<Id> apices;for(const auto& r:rows.apex)apices.insert(r.second);
  auto columnCell=[&](Id id){for(auto n:s.mesh.at(id).cell.v)if(apices.count(n.id))return true;return false;};
  const Cell c=s.mesh.at(cell).cell;Face out{};for(auto f:CellFaces(c))if(Sorted(f)==key)out=f;
  const V a=Of(NodePoint(s,out[0])),b=Of(NodePoint(s,out[1])),cc=Of(NodePoint(s,out[2]));const V n=-1.L*Outward(a,b,cc);const long double h=FaceH0(s,key);
  const std::array<std::array<long double,3>,7> feet{{{1.L/3,1.L/3,1.L/3},{.6L,.2L,.2L},{.2L,.6L,.2L},{.2L,.2L,.6L},{.45L,.45L,.1L},{.45L,.1L,.45L},{.1L,.45L,.45L}}};
  for(long double eta:{1.L,.95L,1.05L})for(const auto& w:feet) {
    const Point p=Pt(w[0]*a+w[1]*b+w[2]*cc+(eta*h)*n);
    std::set<Id> in{cell};bool ok=true;
    if(columnCell(cell)){why="the face cell is a column cell";return false;}
    for(Id id:Locate(s,p)){if(id!=cell&&(WallChild(s,id)||columnCell(id))){ok=false;why="target inside another first child or a column";break;}in.insert(id);}
    // star-shaped visibility cavity of p (hidden faces are crossed)
    auto visible=[&]()->bool{
      for(unsigned pass=0;pass<256;++pass) {
        std::map<Face,std::pair<Face,Id>> skin;std::map<Face,int> count;
        for(Id id:in){const auto f=CellFaces(s.mesh.at(id).cell);for(unsigned k=0;k<4;++k){const auto kk=Sorted(f[k]);++count[kk];skin[kk]={f[k],id};}}
        bool grew=false;
        for(const auto& row:skin){if(count.at(row.first)!=1)continue;const auto& f=row.second.first;const auto& own=s.mesh.at(row.second.second);
          std::array<Point,3> q;for(unsigned k=0;k<3;++k)for(auto nn:own.cell.v)if(nn.id==f[k])q[k]=nn.p;
          if(Orientation(p,q[0],q[1],q[2])>0)continue;
          const auto faces=CellFaces(own.cell);bool phys=false;for(unsigned k=0;k<4;++k)if(Sorted(faces[k])==row.first&&own.physical[k].present)phys=true;
          if(phys){why="target cannot see a physical face";return false;}
          Id nb=UINT64_MAX;for(Id id:Star(s,{f[0],f[1]}))if(!in.count(id)&&CellHas(s.mesh.at(id).cell,f[2]))nb=id;
          if(nb==UINT64_MAX){why="no neighbour across a hidden face";return false;}
          if(WallChild(s,nb)||columnCell(nb)){why="visibility needs another first child or a column cell";return false;}
          in.insert(nb);grew=true;if(in.size()>MaximumCavityCells){why="cavity exceeds 64 cells";return false;}break;}
        if(!grew)return true;
      }
      why="visibility did not close";return false;};
    if(!ok||!visible())continue;
    // DC-16 (B2): a cone cell that is not valid (a metric sliver over a nearly coplanar skin face) widens the cavity
    // across that face (an admissible neighbour), then visibility closes again; at most 8 widenings
    std::vector<Id> ids;Patch old,fresh;bool built=false;
    for(unsigned widen=0;widen<=8;++widen) {
      ids.assign(in.begin(),in.end());try{old=PatchOf(s,ids);}catch(const std::exception& x){why=x.what();break;}
      fresh=Patch{};fresh.surface=old.surface;fresh.parameters=old.parameters;
      try{if(!Cone(old.cells,{s.next_node,p},s.next_cell,fresh.cells,why))break;s.reference->ExportParameters(fresh);}catch(const std::exception& x){why=x.what();break;}
      std::vector<Face> thin;for(const auto& c:fresh.cells)if(!Valid(Shape(s,c))){Face f{};unsigned k=0;for(auto nn:c.v)if(nn.id!=s.next_node&&k<3)f[k++]=nn.id;thin.push_back(f);}
      if(thin.empty()){built=true;break;}
      if(widen==8){why="cone cells stay invalid after widening";break;}
      bool grew=false;
      for(const auto& f:thin){Id nb=UINT64_MAX;for(Id id:Star(s,{f[0],f[1]}))if(!in.count(id)&&CellHas(s.mesh.at(id).cell,f[2]))nb=id;
        if(nb==UINT64_MAX||WallChild(s,nb)||columnCell(nb))continue;in.insert(nb);grew=true;}
      if(!grew){why="an invalid cone cell sits on a face that cannot be crossed";break;}
      if(in.size()>MaximumCavityCells||!visible()){if(why.empty())why="cavity exceeds 64 cells";break;}
    }
    if(!built)continue;
    {std::set<Id> before,after;for(const auto& x:old.cells)for(auto nn:x.v)if(registered.count(nn.id))before.insert(nn.id);
     for(const auto& x:fresh.cells)for(auto nn:x.v)after.insert(nn.id);bool kept=true;for(Id x:before)kept=kept&&after.count(x);
     if(!kept){why="the cone would remove a registered identity";continue;}}
    {std::string d2;if(!D2Ok(s,old,fresh,d2)){why=d2;continue;}}
    const auto d=Admit12(s,old,fresh,Kind::Layer);if(!d.ok){why="admission: "+d.reason;continue;}
    for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))certified.insert(Sorted(sf.v));
    Apply(s,ids,fresh);certified.insert(key);return true;
  }
  return false;
}

/*! Row completion (DC15_PLAN sect. 3): every wall face whose first child fails the first-height rule or the validity
 *  check gets the certified first-child routes of V without V's per-face try limit: a registered column re-places its
 *  apex (ApexPlace, then ApexMove); an unregistered face gets FirstRowInsert (InsertFirstWallPoint / ConstructFirstLayer
 *  / ApexRelocate); a refused face whose cell carries several wall faces or a foreign physical face (a corner cell where
 *  creases meet another wall, symmetry or other plane) is separated first (SplitCell, the R2 Prepare operator) and
 *  retried. Every route is admitted by its own checks (D.2, Admit12, validity); faces are certified for the A3 audit. */
struct Completion { size_t failing_before=0,failing_after=0,apex_placed=0,inserted=0,separated=0,refused=0,passes=0,cavity=0;double seconds=0;std::string outcome;
  std::map<std::string,size_t> why;std::vector<std::string> witnesses; };
inline Completion CompleteFirstRow(State& s,Rows10& rows,double seconds,std::set<Face>& certified,std::ostream& log) {
  Scope scope("DC15 CompleteFirstRow");
  Completion st;const auto t0=Clock::now();
  auto failing=[&]{std::vector<Face> out;for(const auto& wf:WallFaces(s)){if(!s.mesh.count(wf.first))continue;
      if(HeightOk(s,wf.first)&&!Invalid(MeshShape(s,wf.first)))continue;out.push_back(wf.second);}return out;};
  auto owner=[&](const Face& key,Id& cell){const auto w=s.wall_index.find(key);if(w==s.wall_index.end()||!s.mesh.count(w->second))return false;cell=w->second;return true;};
  auto passes=[&](Id cell){return HeightOk(s,cell)&&!Invalid(MeshShape(s,cell));};
  // a completed first child is certified but not a column: its registration stays only when the face is a full column
  // (all three vertices lifted), as the column routes assume (review item 3)
  auto settle=[&](const Face& key){certified.insert(key);if(!std::all_of(key.begin(),key.end(),[&](Id x){return rows.lift.count(x)>0;})){rows.apex.erase(key);rows.overrides.erase(key);rows.extras.erase(key);}};
  auto list=failing();st.failing_before=list.size();
  for(unsigned pass=0;pass<3&&!list.empty();++pass) {
    ++st.passes;bool any=false;
    for(const auto& key:list) {
      if(Seconds(t0)>=seconds){st.outcome="limit:time";break;}
      Id cell=0;if(!owner(key,cell)||passes(cell))continue;
      const auto a=rows.apex.find(key);const bool registered=a!=rows.apex.end()&&s.vertex_cells.count(a->second);
      std::string why;VStage g;
      if(registered){if(ApexPlace(s,rows,key,g,why,certified)||ApexMove12(s,rows,key,g,why,certified)){++st.apex_placed;any=true;continue;}++st.why["apex: "+Key(why)];}
      FirstRowStats fr;
      if(FirstRowInsert(s,rows,cell,key,fr,certified)){settle(key);++st.inserted;any=true;continue;}
      for(const auto& w:fr.why)++st.why[w.first];
      {std::string cw;if(CavityFirstChild(s,rows,cell,key,certified,cw)){settle(key);++st.cavity;any=true;continue;}++st.why["cavity: "+Key(cw)];}
      // corner cell: several wall faces or a foreign physical face -> separation, then the first-child route again
      unsigned walls=0,others=0;{const auto& row=s.mesh.at(cell);for(const auto& f:row.physical)if(f.present)(s.walls.count(s.reference->Get(f.facet).marker)?walls:others)++;}
      if(walls>=2||others>=1){std::string sw;
        std::set<Face> touched;for(const auto& f:CellFaces(s.mesh.at(cell).cell))if(s.wall_index.count(Sorted(f)))touched.insert(Sorted(f));
        if(SplitCell(s,rows,cell,sw)){++st.separated;any=true;Id c2=0;for(const auto& f:touched)certified.insert(f);
          if(owner(key,c2)&&!passes(c2)){FirstRowStats f2;if(FirstRowInsert(s,rows,c2,key,f2,certified)){settle(key);++st.inserted;}else for(const auto& w:f2.why)++st.why["after separation: "+w.first];}
          continue;}
        ++st.why["separation: "+Key(sw)];}
      if(pass==2||st.witnesses.size()<16){std::ostringstream w;w<<"face "<<key[0]<<","<<key[1]<<","<<key[2]<<" cell "<<cell<<" walls="<<walls<<" others="<<others<<" registered="<<registered;
        if(!fr.why.empty())w<<" first-row: "<<fr.why.rbegin()->first;if(st.witnesses.size()<16)st.witnesses.push_back(w.str());log<<"DC15 completion refused "<<w.str()<<"\n";}
    }
    if(!st.outcome.empty()||!any)break;
    list=failing();
  }
  st.failing_after=failing().size();st.refused=st.failing_after;if(st.outcome.empty())st.outcome=st.failing_after?"incomplete":"complete";
  st.seconds=Seconds(t0);
  log<<"DC15 completion failing "<<st.failing_before<<" -> "<<st.failing_after<<" inserted="<<st.inserted<<" apex="<<st.apex_placed<<" separated="<<st.separated<<"\n";
  return st;
}
inline std::string JsonCompletion(const Completion& c) {
  std::ostringstream o;o<<std::setprecision(8)<<"{\"outcome\":\""<<c.outcome<<"\",\"failing_before\":"<<c.failing_before<<",\"failing_after\":"<<c.failing_after<<",\"inserted\":"<<c.inserted
   <<",\"apex_placed\":"<<c.apex_placed<<",\"cavity\":"<<c.cavity<<",\"separated\":"<<c.separated<<",\"passes\":"<<c.passes<<",\"seconds\":"<<c.seconds<<",\"why\":{";
  {bool k=false;for(const auto& r:c.why){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}
  o<<"},\"witnesses\":[";for(size_t i=0;i<c.witnesses.size();++i)o<<(i?",":"")<<junction_wp0::Json(c.witnesses[i]);o<<"]}";return o.str();
}
}  // namespace dc15
