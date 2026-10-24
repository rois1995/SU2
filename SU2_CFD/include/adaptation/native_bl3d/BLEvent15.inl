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

/*! Row completion (DC15_PLAN sect. 3): every wall face whose first child fails the first-height rule or the validity
 *  check gets the certified first-child routes of V without V's per-face try limit: a registered column re-places its
 *  apex (ApexPlace, then ApexMove); an unregistered face gets FirstRowInsert (InsertFirstWallPoint / ConstructFirstLayer
 *  / ApexRelocate); a refused face whose cell carries several wall faces or a foreign physical face (a corner cell where
 *  creases meet another wall, symmetry or other plane) is separated first (SplitCell, the R2 Prepare operator) and
 *  retried. Every route is admitted by its own checks (D.2, Admit12, validity); faces are certified for the A3 audit. */
struct Completion { size_t failing_before=0,failing_after=0,apex_placed=0,inserted=0,separated=0,refused=0,passes=0;double seconds=0;std::string outcome;
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
   <<",\"apex_placed\":"<<c.apex_placed<<",\"separated\":"<<c.separated<<",\"passes\":"<<c.passes<<",\"seconds\":"<<c.seconds<<",\"why\":{";
  {bool k=false;for(const auto& r:c.why){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}}
  o<<"},\"witnesses\":[";for(size_t i=0;i<c.witnesses.size();++i)o<<(i?",":"")<<junction_wp0::Json(c.witnesses[i]);o<<"]}";return o.str();
}
}  // namespace dc15
