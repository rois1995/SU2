/*!
 * \file BLVolume13.inl
 * \brief BL volume stage V13, ApexPlace, the halving step and the column redistribution (ex DC-13).
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc13.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// DC-13: (history: formerly test-only) [NativeFirstRowDC13] harness (DC13_PLAN rev 2). DC-12's W -> R1 -> R2 -> G -> V12 pipeline with the
// DC-13 production optimizer (O4m min-max smoothing, O7/O8/O7L vertex insertion) and two diagnostics requested by the
// coordinator: the per-marker first-height multiplier (dc10::h0Scale, every wall marker's own h0 -> max(h0, m h0), no
// case switch) and the halving chain (each step starts from the previous output mesh in the same frozen sensor field
// with the next multiplier: the first children are split in the wall-normal direction at the new height, then V).
// Final gates are unchanged and absolute. (DC-14: production.)
namespace dc13 {
using namespace dc12;
inline std::string JsonOpt13(const OptimizeReport& r) {
  std::string base=JsonOpt(r);base.pop_back();std::ostringstream o;o<<base<<",\"O7_shell_insertions\":"<<r.shell_insertions<<",\"O8_neighbourhood_insertions\":"<<r.neighbourhood_insertions
    <<",\"O7L_length_insertions\":"<<r.length_insertions<<",\"placements\":"<<r.placements<<",\"placement_refusals\":"<<r.placement_refusals<<",\"placement_evaluations\":"<<r.placement_evaluations<<"}";
  return o.str();
}
// Halving step (wall-normal split of the near-wall layer): every wall face's first child (face + apex p) is coned from
// a point q on the segment from p to its normal foot, at the face's (new) prescribed height: the new first child is
// (face, q) with the same foot, the three other cells (q, wall edge, p). Certified first-row route (A3); registrations
// follow the new apex. q lies strictly inside the old first child when the old altitude exceeds the new height.
struct HalveStats { size_t faces=0,split=0,skipped_low=0,refused=0,placed=0;std::map<std::string,size_t> why; };
// DC-17: one face of the halving (the cone split of its first child), shared by HalveFirstRow and the fallback of the
// first-row rebuild (dc17::RebuildFirstRow); `false` when the face was not split (counted in st).
inline bool HalveFace(State& s,Rows10& rows,std::set<Face>& certified,const std::pair<Id,Face>& wf,HalveStats& st) {
  {if(!s.mesh.count(wf.first))return false;
    const Cell c=s.mesh.at(wf.first).cell;Face out{};for(auto f:CellFaces(c))if(Sorted(f)==wf.second)out=f;
    Node apex{};for(auto n:c.v)if(!Has(wf.second,n.id))apex=n;
    const V a=Of(NodePoint(s,out[0])),b=Of(NodePoint(s,out[1])),cc=Of(NodePoint(s,out[2]));const V n=-1.L*Outward(a,b,cc);
    const long double alt=Dot(Of(apex.p)-a,n),h=FaceH0(s,wf.second);
    if(!(alt>h*(1+HeightTolerance))){++st.skipped_low;return false;}  // already within the new window (or lower)
    Patch old;try{old=PatchOf(s,{wf.first});}catch(const std::exception&){++st.refused;++st.why["patch"];return false;}
    Node q{s.next_node,Pt(Of(apex.p)-(alt-h)*n)};
    Patch fresh;fresh.surface=old.surface;fresh.parameters=old.parameters;std::string why;
    // the point is placed by the min-max engine inside the old first child, restricted to the new first-child window
    // (altitude (1 -+ .98 T) h, foot barycentrics >= .02); the straight-down point is the start and the fallback
    {const long double area=Dot(Cross(b-a,cc-a),n);const long double T=.98L*HeightTolerance;
     auto region=[a,b,cc,n,h,area,T](Point p)->bool{const V x=Of(p);const auto t=Dot(x-a,n);if(t<(1-T)*h||t>(1+T)*h)return false;const V f=x-t*n;const std::array<V,3> q3{a,b,cc};
       for(unsigned j=0;j<3;++j)if(!(Dot(Cross(q3[(j+1)%3]-f,q3[(j+2)%3]-f),n)/area>=.02L))return false;return true;};
     std::vector<Cell> placed;double score=0;
     if(std::abs(area)>0&&PlaceConeInsertion(old.cells,q.p,q.id,s.next_cell,RegionMetric(*s.field),placed,score,nullptr,.6,160,UINT64_MAX,region)){
       for(const auto& c:placed)for(auto x:c.v)if(x.id==q.id)q=x;fresh.cells=placed;++st.placed;}}
    if(fresh.cells.empty())try{if(!Cone(old.cells,q,s.next_cell,fresh.cells,why)){++st.refused;++st.why["cone: "+Key(why)];return false;}}
    catch(const std::exception& x){++st.refused;++st.why[std::string("cone: ")+x.what()];return false;}
    try{s.reference->ExportParameters(fresh);}catch(const std::exception& x){++st.refused;++st.why[std::string("parameters: ")+x.what()];return false;}
    bool ok=true;for(const auto& x:fresh.cells)ok=ok&&Positive(x.v)&&Valid(Shape(s,x));
    if(!ok){++st.refused;++st.why["validity"];return false;}
    std::string hw;if(!HeightT(s,fresh,hw)){++st.refused;++st.why["height: "+Key(hw)];return false;}
    // only a registered column face follows its new apex (a face without a column - rim, unrowed - stays unregistered:
    // the column routes assume registered faces have lifted vertices)
    Apply(s,{wf.first},fresh);if(rows.apex.count(wf.second))rows.apex[wf.second]=q.id;certified.insert(wf.second);++st.split;
  }
  return true;
}
inline HalveStats HalveFirstRow(State& s,Rows10& rows,std::set<Face>& certified) {
  Scope scope("DC13 HalveFirstRow");HalveStats st;
  std::vector<std::pair<Id,Face>> faces;for(const auto& wf:WallFaces(s))faces.push_back(wf);
  for(const auto& wf:faces){++st.faces;HalveFace(s,rows,certified,wf,st);}
  return st;
}
// Column redistribution of a halving step (coordinator proposal 2026-10-24): along every first-row column (wall vertex
// w, lid L, direction d = unit(L - w)) the missing near-wall law interfaces of the new first height are inserted ON the
// column edge: normal heights z_k = h0' (1 + g + ... + g^(k-1)), k >= 1, below .85 of the lid height (the lid and the
// G stack above it sit near the shifted law positions already: h0'/... = the old law, ~log 2 / log g more layers near
// the wall). Each insertion splits the current bottom column edge (w, m) by production SplitEdge at an exact on-edge
// point (InsertEdgeCavity when none is representable / for physical carriers the bound point), from the highest
// interface down; admitted when every fresh cell is valid and no first child changes (topology stays column-aligned).
// DC-14: former environment switches of the DC-13 harness (SU2_DC13_UNLIFTED_COLUMNS, SU2_DC13_CUT_SLIDE); the cut
// slide is a TEST-ONLY diagnostic and never set in production.
inline bool unliftedColumns=false,diagnosticCutSlide=false;
struct ColumnStats { size_t columns=0,unlifted=0,inserted=0,refused=0;std::map<std::string,size_t> why; };
// DC-17: one column-edge insertion of ColumnInsert (shared with dc17::RebuildFirstRow): splits the bottom column edge
// (w, top) at normal height z (direction d, d.n = dn) by production SplitEdge at an exact on-edge point
// (InsertEdgeCavity when none is representable; the bound point on physical carriers); admitted when every fresh cell
// is valid and no first child changes. Returns the new vertex, UINT64_MAX on refusal (counted in st).
inline Id SplitColumnEdge(State& s,Id w,Id top,V x,V d,long double dn,long double z,ColumnStats& st) {
  auto firstChildOf=[&](const Patch& p){std::map<Face,std::array<Id,4>> out;std::set<Face> walls;for(const auto& sf:p.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))walls.insert(Sorted(sf.v));
    for(const auto& c:p.cells)for(auto f:CellFaces(c))if(walls.count(Sorted(f)))out[Sorted(f)]=CellKey(c);return out;};
  const Edge e{std::min(w,top),std::max(w,top)};const auto ids=Star(s,e);if(ids.empty()||ids.size()>MaximumCavityCells){++st.refused;++st.why["edge star"];return UINT64_MAX;}
  Patch old;try{old=PatchOf(s,ids);}catch(const std::exception&){++st.refused;++st.why["patch"];return UINT64_MAX;}
  const Point want=Pt(x+(z/dn)*d);Patch fresh;std::string why;bool ok=false;Point exact;
  bool physical=false;for(const auto& sf:old.surface)physical=physical||(Has(sf.v,w)&&Has(sf.v,top));
  try{if(physical)ok=SplitEdge(old,e,{s.next_node,want},s.next_cell,*s.reference,fresh,why);
      else if(ExactOnEdgePoint(NodePoint(s,e[0]),NodePoint(s,e[1]),want,exact))ok=SplitEdge(old,e,{s.next_node,exact},s.next_cell,*s.reference,fresh,why);
      if(!ok&&!physical){fresh=Patch{};ok=InsertEdgeCavity(old,e,{s.next_node,want},s.next_cell,*s.reference,fresh,why);}}
  catch(const std::exception& x2){ok=false;why=x2.what();}
  if(!ok){++st.refused;++st.why["split: "+Key(why)];return UINT64_MAX;}
  for(const auto& pr:old.parameters)fresh.parameters.emplace(pr.first,pr.second);
  try{s.reference->ExportParameters(fresh);}catch(const std::exception&){}
  if(firstChildOf(old)!=firstChildOf(fresh)){++st.refused;++st.why["would change a first child"];return UINT64_MAX;}
  bool valid=true;for(const auto& c:fresh.cells)valid=valid&&Positive(c.v)&&Valid(Shape(s,c));
  if(!valid){++st.refused;++st.why["validity"];return UINT64_MAX;}
  const Id m=s.next_node;Apply(s,ids,fresh);++st.inserted;return m;
}
// DC-17: the current bottom column edge (w, top) of a first-row column (shared by ColumnInsert and
// dc17::RebuildFirstRow): for a lifted vertex the adjacent vertex on the column ray (alignment 1e-6) closest to the
// wall; otherwise (only with `unlifted`) the adjacent non-wall vertex best aligned with its constrained lift direction
// (cos >= .9). Fan ridges (copies) have none. False with the reason counted in st.why.
struct ColumnEdge { Id top=UINT64_MAX;V x,d,nb;long double dn=0;bool lifted=false; };
inline bool ColumnEdgeOf(State& s,const Rows10& rows,Id w,const std::set<Id>& wallSet,const std::set<Id>& apices,bool unlifted,ColumnEdge& out,ColumnStats& st) {
  if(!s.vertex_cells.count(w))return false;const auto lf=rows.lift.find(w);const bool lifted=lf!=rows.lift.end()&&s.vertex_cells.count(lf->second);
  if(rows.copies.count(w))return false;  // fan ridges keep their copies
  // unlifted wall vertices (cut boundary) only on request: on m6_root_te x16 they gave 9 bad cells instead of 6
  if(!lifted&&!unlifted)return false;
  const V x=Of(NodePoint(s,w));const auto dir=DirectionAt(s,w);if(dir.normals.empty()){++st.why["no wall normal"];return false;}
  // column direction: the first-row lift; a vertex without a lift (e.g. on the artificial cut boundary) uses its
  // constrained lift direction (DirectionAt: in-plane on a carrier) and its nearest-aligned non-wall edge
  V d;if(lifted)d=Unit(Of(NodePoint(s,lf->second))-x);else if(dir.ok)d=dir.d;else{++st.why["no direction"];return false;}
  V nb{};for(const auto& n:dir.normals)nb=nb+n;nb=Unit(nb);
  Id top=UINT64_MAX;long double best=INFINITY,align=.9L;
  for(Id c:StarOf(s,w))for(auto n:s.mesh.at(c).cell.v){if(n.id==w||wallSet.count(n.id)||apices.count(n.id))continue;const V y=Of(n.p)-x;const auto t=Dot(y,d);if(!(t>0))continue;
    if(lifted){if(Norm(y-t*d)<=1e-6L*t&&t<best){best=t;top=n.id;}}
    else{const auto c2=t/Norm(y);if(c2>align){align=c2;top=n.id;}}}
  if(top==UINT64_MAX){++st.why[lifted?"no column edge at the wall vertex":"no aligned edge (unlifted)"];return false;}
  ++st.columns;if(!lifted)++st.unlifted;
  d=Unit(Of(NodePoint(s,top))-x);const long double dn=Dot(d,nb);if(!(dn>.1L)){++st.why["column too flat"];return false;}
  out=ColumnEdge{top,x,d,nb,dn,lifted};return true;
}
inline ColumnStats ColumnInsert(State& s,Rows10& rows) {
  Scope scope("DC13 ColumnInsert");ColumnStats st;MapLayers(s);
  const auto wallSet=WallVertices(s);std::set<Id> apices;for(const auto& r:rows.apex)apices.insert(r.second);
  for(Id w:wallSet) {
    ColumnEdge ce;if(!ColumnEdgeOf(s,rows,w,wallSet,apices,unliftedColumns,ce,st))continue;
    Id top=ce.top;const V x=ce.x,d=ce.d,nb=ce.nb;const long double dn=ce.dn;
    const long double hL=Dot(Of(NodePoint(s,top))-x,nb),h0=VertexH0(s,w);
    long double g=-1;for(const auto& f:PhysicalOf(s,StarOf(s,w),true))if(Has(f.key,w)){const auto it=layerOfMarker.find(f.marker);if(it!=layerOfMarker.end()){g=GrowthOf(s,it->second);break;}}
    if(!(g>1)){++st.why["no wall growth policy"];continue;}  // DC-15: per-marker growth only (no global fallback)
    std::vector<long double> z;for(long double zk=h0,step=h0;zk<.85L*hL;step*=g,zk+=step)z.push_back(zk);
    for(size_t k=z.size();k-->0;){const Id m=SplitColumnEdge(s,w,top,x,d,dn,z[k],st);if(m==UINT64_MAX)break;top=m;}
  }
  return st;
}
// Witness of the remaining bad cells: per vertex its role W wall, L lid, A apex, C carrier/cut skin, I interior.
inline std::string BadWitness(State& s,const Rows10& rows,size_t limit=24) {
  std::set<Id> wall=WallVertices(s),lids,apices,skin;for(const auto& r:rows.lift)lids.insert(r.second);for(const auto& r:rows.apex)apices.insert(r.second);
  for(const auto& row:s.mesh)for(unsigned k=0;k<4;++k)if(row.second.physical[k].present&&!s.walls.count(s.reference->Get(row.second.physical[k].facet).marker)){const auto f=CellFaces(row.second.cell);skin.insert(f[k].begin(),f[k].end());}
  std::vector<std::pair<double,Id>> bad;for(const auto& row:s.mesh){const auto sh=MeshShape(s,row.first);if(Bad(sh))bad.emplace_back(std::min(sh.q/MinimumMeanRatio,sh.J/MinimumScaledJacobian),row.first);}
  std::sort(bad.begin(),bad.end());std::ostringstream o;o<<"[";
  for(size_t i=0;i<bad.size()&&i<limit;++i){const auto& c=s.mesh.at(bad[i].second).cell;const auto sh=MeshShape(s,bad[i].second);std::string roles;
    for(auto n:c.v)roles+=wall.count(n.id)?'W':apices.count(n.id)?'A':lids.count(n.id)?'L':skin.count(n.id)?'C':'I';
    o<<(i?",":"")<<"[\""<<roles<<"\","<<Fmt(sh.q)<<","<<Fmt(sh.J)<<","<<WallChild(s,bad[i].second)<<"]";}
  o<<"]";return o.str();
}
// Census of the cells of the input that survive unchanged (cell keys) and of the vertices kept/added/removed.
struct Change { size_t cells_before=0,cells_after=0,cells_unchanged=0,vertices_before=0,vertices_after=0,vertices_kept=0;long double max_kept_displacement=0; };
inline Change Compare(const Owned& before,const State& s) {
  Change c;c.cells_before=before.size();c.cells_after=s.mesh.size();std::set<std::array<Id,4>> keys;std::map<Id,Point> vb;
  for(const auto& r:before){keys.insert(CellKey(r.second.cell));for(auto n:r.second.cell.v)vb[n.id]=n.p;}
  std::map<Id,Point> va;for(const auto& r:s.mesh){c.cells_unchanged+=keys.count(CellKey(r.second.cell));for(auto n:r.second.cell.v)va[n.id]=n.p;}
  c.vertices_before=vb.size();c.vertices_after=va.size();
  for(const auto& v:vb){const auto f=va.find(v.first);if(f==va.end())continue;++c.vertices_kept;c.max_kept_displacement=std::max(c.max_kept_displacement,Norm(Of(f->second)-Of(v.second)));}
  return c;
}
}  // namespace dc13


// ---- V13 helpers ----------------------------------------------------------------------------------------------------
namespace dc13 {
struct ClassCensus { size_t attempts=0;std::map<std::string,size_t> accepted,reasons; };
// R2 retry in V (sweeps 0 and 1) builds missing first-row columns; a halving chain step does not construct the first
// row (its registered apices cover only the subdivided first children, so column rebuilds would read lids outside the
// cavity): off in chain steps.
inline bool v13R2Retry=true;
// DC-14: refinement attempts per V13 sweep (DC-13: 1024; the production entry scales it with the long-edge demand).
inline size_t v13AttemptsPerSweep=1024;
// DC-15: 0 = per-sweep attempt cap from the current demand (max(1024, long edges / 4)); production sets 0. Near-wall first:
// after wall/seam edges, the long edges whose midpoint wall distance is in [h0, 300 h0] (BL band, TE base, near wake).
inline bool v13NearWallFirst=false;
// DC-15 (intermediate events): the long-edge scan uses the measured length when it is cached and the midpoint estimate
// sqrt(e^T M(mid) e) otherwise (selection only: every attempted edge is integrated again before its admission); the
// sweep-end gate scan is skipped when V stops by its limit (the event reports its deficits once afterwards).
inline bool v13EstimateScan=false,v13SkipFinalOnLimit=false;
// DC-16 (B3/B4): demand queue of V. Selection only: one target tensor per vertex (coordinate-checked cache), the geometric
// edge estimate 2/3 (la^2 + la lb + lb^2)/(la + lb) of the endpoint lengths, a full arithmetic scan once, then only the
// edges of cells created since the previous scan plus the not yet attempted queue; every attempted edge is integrated
// exactly before its admission (unchanged). Intermediate events skip the full sweep-end gate scan (deficits are reported
// once after V). Production sets it; the DC-13 harness keeps its exact scan unless SU2_DC16_PRODUCTION_V=1.
inline bool v13DemandQueue=false;inline double v13DemandThreshold=1.6;
struct VertexTensors {
  std::unordered_map<Id,std::pair<Point,Tensor>> at;size_t queries=0;
  const Tensor& Get(State& s,Id v){const auto p=NodePoint(s,v);auto f=at.find(v);
    if(f!=at.end()&&SamePoint(f->second.first,p))return f->second.second;++queries;const Tensor t=s.field->Query(p).target;
    auto& slot=at[v];slot={p,t};return slot.second;}
};
// DC-16: one event-wide vertex tensor cache (reset by the pipeline scope) for the V queue, the estimated deficits and
// the diagnostic export; LenOrEstimate then estimates an unmeasured edge from its endpoint tensors instead of a
// midpoint query
inline VertexTensors eventTensors;
inline long double EstimateLength(State& s,VertexTensors& vt,Id a,Id b);
inline long double LenOrEstimate(State& s,Id a,Id b,bool& estimated) {
  estimated=false;const auto key=std::minmax(a,b);const auto pa=NodePoint(s,key.first),pb=NodePoint(s,key.second);
  const auto f=s.lengths.find({key.first,key.second});
  if(f!=s.lengths.end()&&SamePoint(f->second.first[0],pa)&&SamePoint(f->second.first[1],pb)){if(f->second.second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return f->second.second;}
  const std::array<double,6> k6{pa.x,pa.y,pa.z,pb.x,pb.y,pb.z};const auto g=geometryLengths.find(k6);
  if(g!=geometryLengths.end()){if(g->second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return g->second;}
  estimated=true;if(v13DemandQueue)return EstimateLength(s,eventTensors,a,b);
  const V A=Of(pa),E=Of(pb)-A;const Tensor m=s.field->Query(Pt(A+.5L*E)).target;
  const long double q=E.x*(m.xx*E.x+m.xy*E.y+m.xz*E.z)+E.y*(m.xy*E.x+m.yy*E.y+m.yz*E.z)+E.z*(m.xz*E.x+m.yz*E.y+m.zz*E.z);
  if(!(q>=0))throw std::runtime_error("non-positive midpoint metric");return std::sqrt(q);
}
// DC-14: optimizer boundary operations O5/O6 on a mapped authority (they change skin faces without a parameter
// transaction). Production: off (as RefineOptimize). The DC-13 harness sets it to reproduce its cut results.
inline bool v13MappedBoundaryOps=false;
inline long double EstimateLength(State& s,VertexTensors& vt,Id a,Id b) {
  const auto pa=NodePoint(s,a),pb=NodePoint(s,b);const long double e[3]={(long double)pb.x-pa.x,(long double)pb.y-pa.y,(long double)pb.z-pa.z};
  auto len=[&](const Tensor& m){const long double q=e[0]*(m.xx*e[0]+m.xy*e[1]+m.xz*e[2])+e[1]*(m.xy*e[0]+m.yy*e[1]+m.yz*e[2])+e[2]*(m.xz*e[0]+m.yz*e[1]+m.zz*e[2]);
    if(!(q>=0))throw std::runtime_error("non-positive vertex metric");return std::sqrt(q);};
  const long double la=len(vt.Get(s,a)),lb=len(vt.Get(s,b));return la+lb>0?(2.L/3)*(la*la+la*lb+lb*lb)/(la+lb):0;
}
struct VStage13 : VStage12 { std::map<int,ClassCensus> byClass;size_t apex_place=0,transactional_repairs=0,transactional_rollbacks=0;size_t global_skipped=0; };
// edge class for the census: 0 interior, 1 wall, 2 seam (wall + other physical), 3 carrier (non-wall, non-artificial
// physical), 4 artificial (cut) boundary
inline int EdgeClass13(State& s,Edge e) {
  bool w=false,c=false,a=false;for(Id id:Star(s,e)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
    if(r.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1])){const int m=s.reference->Get(r.physical[k].facet).marker;if(s.walls.count(m))w=true;else if(artificialMarkers.count(m))a=true;else c=true;}}
  return w&&(c||a)?2:w?1:a?4:c?3:0;
}
// ApexPlace (DC13_PLAN sect. 3.1): the apex of a registered column re-placed by the min-max placement over its star,
// restricted to the first-child window of every face registered with it (altitude in (1 -+ .98 T) h0, normal foot
// strictly inside the face, barycentrics >= .02); Admit12 Repair, D.2; registrations follow; certified route (A3).
inline bool ApexPlace(State& s,Rows10& rows,const Face& key,VStage& g,std::string& why,std::set<Face>& certified) {
  Scope scope("V13 ApexPlace");
  const auto it=rows.apex.find(key);if(it==rows.apex.end()){why="not rowed";return false;}const Id v=it->second;
  if(!s.vertex_cells.count(v)){why="apex gone";return false;}
  for(const auto& f:PhysicalOf(s,StarOf(s,v),false))if(Has(f.key,v)){why="carrier apex";return false;}
  const auto star=StarOf(s,v);if(star.empty()||star.size()>MaximumCavityCells){why="star size";return false;}
  std::vector<Id> ids(star.begin(),star.end());Patch old;try{old=PatchOf(s,ids);}catch(const std::exception& x){why=x.what();return false;}
  struct Window{V a,b,c,n;long double h0=0,area=0;};std::vector<Window> windows;
  for(const auto& f:PhysicalOf(s,star,true)){bool registered=false;for(const auto& r:rows.apex)registered=registered||(r.first==f.key&&r.second==v);
    bool child=false;for(const auto& c:old.cells){unsigned k=0;bool hasV=false;for(auto n:c.v){k+=Has(f.key,n.id);hasV=hasV||n.id==v;}child=child||(k==3&&hasV);}
    if(!registered&&!child)continue;
    Window w;w.a=Of(NodePoint(s,f.out[0]));w.b=Of(NodePoint(s,f.out[1]));w.c=Of(NodePoint(s,f.out[2]));w.n=-1.L*Outward(w.a,w.b,w.c);w.h0=FaceH0(s,f.key);w.area=Dot(Cross(w.b-w.a,w.c-w.a),w.n);if(!(std::abs(w.area)>0))continue;windows.push_back(w);}  // signed: barycentrics in the n orientation
  if(windows.empty()){why="no first child at the apex";return false;}
  const long double T=.98L*HeightTolerance;
  auto region=[windows,T](Point p)->bool{const V x=Of(p);
    for(const auto& w:windows){const auto alt=Dot(x-w.a,w.n);if(alt<(1-T)*w.h0||alt>(1+T)*w.h0)return false;
      const V foot=x-alt*w.n;const std::array<V,3> q{w.a,w.b,w.c};
      for(unsigned j=0;j<3;++j){const auto wt=Dot(Cross(q[(j+1)%3]-foot,q[(j+2)%3]-foot),w.n)/w.area;if(!(wt>=.02L))return false;}}
    return true;};
  std::vector<Cell> cells;double score=0;
  if(!PlaceConeInsertion(old.cells,NodePoint(s,v),s.next_node,s.next_cell,RegionMetric(*s.field),cells,score,nullptr,.6,240,v,region)){why="no valid placement in the window";return false;}
  Patch fresh;fresh.cells=cells;fresh.surface=old.surface;fresh.parameters=old.parameters;
  try{s.reference->ExportParameters(fresh);}catch(const std::exception& x){why=x.what();return false;}
  {std::string d2;if(!D2Ok(s,old,fresh,d2)){why=d2;return false;}}
  ++g.work;const auto d=Admit12(s,old,fresh,Kind::Repair);if(!d.ok){++g.rejected;why="admission: "+d.reason;return false;}
  for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))certified.insert(Sorted(sf.v));
  const Id moved=s.next_node;Apply(s,ids,fresh);MoveRegistrations(rows,v,moved,certified);certified.insert(key);++g.accepted;return true;
}
}  // namespace dc13

// ---- V13 (DC13_PLAN sect. 2-4): V12 + best admitted variant + ShellCone + ApexPlace + per-class census --------
namespace dc13 {
// DC-17 (target 3): the production optimizer on a LOCAL region (the cells a refinement slice created plus one vertex
// ring), as one transaction: the region is copied into its own mesh, whose skin (region border, physical faces) the
// optimizer keeps fixed; first children, fan cells, apices and copies are frozen, lids kept (as the mesh-wide call); the
// changed cells are committed back by Apply with their physical faces, and moved registry vertices are relabelled.
// Ready for the 2D ownership pattern (a region is an owned cavity; disjoint regions commute).
// freezeRegistry: every registry identity (lids, G stack tops) is frozen too, so nothing is relabelled and the caller
// can roll the region back from a snapshot (refine + repair transaction below).
inline bool LocalOptimize(State& s,Rows10& rows,Growth& G,const std::set<Id>& region,double seconds,OptimizeReport& rep,bool freezeRegistry=false,size_t work=SIZE_MAX) {
  if(region.empty()||!(seconds>0))return false;
  Owned local;for(Id id:region){const auto f=s.mesh.find(id);if(f!=s.mesh.end())local.emplace(id,f->second);}
  OptimizeOptions opt;opt.maximum_sweeps=4;opt.maximum_seconds=seconds;opt.maximum_work=work;opt.boundary=false;opt.cache_entries=std::max<size_t>(size_t(1)<<16,8*local.size());
  if(freezeRegistry){for(const auto& r:rows.lift)opt.frozen_vertices.insert(r.second);for(const auto& c:G.current)opt.frozen_vertices.insert(c.second);
    for(const auto& c:G.columns){opt.frozen_vertices.insert(c.lid);opt.frozen_vertices.insert(c.top);}}
  for(const auto& row:local)if(WallChild(s,row.first)){opt.frozen_cells.insert(row.first);for(const auto& n:row.second.cell.v)opt.frozen_vertices.insert(n.id);}
  for(Id id:FanCells(s,rows))if(local.count(id)){opt.frozen_cells.insert(id);for(const auto& n:s.mesh.at(id).cell.v)opt.frozen_vertices.insert(n.id);}
  for(const auto& r:rows.lift)opt.keep_vertices.insert(r.second);
  for(const auto& r:rows.copies)for(Id c:r.second.ids)opt.frozen_vertices.insert(c);
  for(const auto& r:rows.apex)opt.frozen_vertices.insert(r.second);
  opt.mesh_quality=[&s](Id id,double& q,double& J){if(!s.mesh.count(id))return false;const auto sh=MeshShape(s,id);q=sh.q;J=sh.J;return true;};
  const RegionMetric target([&s](Point p){return s.field->Query(p).target;},[&s](Point a,Point b){return GeometryLength(s,a,b);});
  Id nn=s.next_node,nc=s.next_cell;bool moved=false;
  {Scope sc("DC17 LocalOptimize");try{moved=OptimizeMetricMesh(local,target,nn,nc,opt,rep);}catch(const std::exception& e){rep.stop=std::string("exception: ")+e.what();return false;}}
  if(!moved)return false;
  std::vector<Id> removed;for(Id id:region)if(s.mesh.count(id)&&!local.count(id))removed.push_back(id);
  Patch fresh;
  for(const auto& row:local){if(region.count(row.first))continue;fresh.cells.push_back(row.second.cell);const auto f=CellFaces(row.second.cell);
    for(unsigned k=0;k<4;++k)if(row.second.physical[k].present)fresh.surface.push_back({f[k],row.second.physical[k].facet});}
  if(removed.empty()&&fresh.cells.empty())return false;
  s.next_node=std::max(s.next_node,nn);s.next_cell=std::max(s.next_cell,nc);
  Apply(s,removed,fresh);
  for(auto& c:G.current){const auto r=rep.relabel.find(c.second);if(r!=rep.relabel.end())c.second=r->second;}
  for(auto& l:rows.lift){const auto r=rep.relabel.find(l.second);if(r!=rep.relabel.end())l.second=r->second;}
  for(auto& c:G.columns){for(Id* x:{&c.lid,&c.top}){const auto r=rep.relabel.find(*x);if(r!=rep.relabel.end())*x=r->second;}}
  return true;
}
inline VStage13 PhaseV13(State& s,Rows10& rows,Growth& G,size_t work,double seconds,size_t maxSweeps,std::set<Face>& certified,std::ostream& log) {
  Scope scope("PhaseV13");
  VStage13 g;std::set<Id> apexIds;Begin(s,g);const auto t0=Clock::now();
  auto remaining=[&]{return seconds-Seconds(t0);};
  auto limited=[&]{return g.work>=work||Seconds(t0)>=seconds;};
  struct Reset{~Reset(){splitPolicy.requested=-1;splitPolicy.relocated=nullptr;splitPolicy.relocatedCount=0;}} resetPolicy;
  std::set<std::vector<Id>> rejected;
  auto keyOf=[&](Id tag,const std::vector<Id>& head,const std::vector<Id>& ids){std::vector<Id> k{tag};k.insert(k.end(),head.begin(),head.end());k.push_back(UINT64_MAX);k.insert(k.end(),ids.begin(),ids.end());std::sort(k.begin()+head.size()+2,k.end());return k;};
  // a first child (the cell on a wall face) may change only through the certified first-row routes (A3)
  auto firstChildKept=[&](const Patch& old,const Patch& fresh){
    for(const auto& sf:old.surface){if(!s.walls.count(s.reference->Get(sf.facet).marker))continue;const auto key=Sorted(sf.v);std::array<Id,4> a{},b{};bool fa=false,fb=false;
      for(const auto& c:old.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3){a=CellKey(c);fa=true;}}
      for(const auto& c:fresh.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3){b=CellKey(c);fb=true;}}
      if(fa!=fb||a!=b)return false;}
    return true;};
  auto attempt=[&](const std::string& op,Kind kind,const std::vector<Id>& ids,const Patch& old,const Patch& fresh)->bool{
    if(op!="wall"&&!firstChildKept(old,fresh)){++g.work;++g.rejected;++g.reasons[op+" [6] changes a first child outside the certified routes"];return false;}
    ++g.work;const auto d=Admit12(s,old,fresh,kind);
    if(!d.ok){++g.rejected;++g.reasons[op+" ["+std::to_string(d.category)+"] "+Key(d.reason)];g.last=op+": "+d.reason;return false;}
    // only the production wall reconstructions (variant "wall": height-certified first-row rebuilds) may change first
    // children here; every other refinement must leave them unchanged (the A3 audit counts any other change)
    if(op=="wall"){std::set<Face> before,after;for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))before.insert(Sorted(sf.v));
     for(const auto& sf:fresh.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))after.insert(Sorted(sf.v));
     for(const auto& f:before)certified.insert(f);for(const auto& f:after)certified.insert(f);}
    Apply(s,ids,fresh);++g.accepted;++g.acceptedBy[op];return true;};
  ColumnIndex ci;
  auto build=[&]{const bool fresh=ci.Build(s,rows);if(!rows.fan.empty())for(Id id:FanCells(s,rows))ci.cells.insert(id);for(const auto& r:rows.copies)for(Id c:r.second.ids)ci.unlift[c]=r.first;
    if(fresh){apexIds.clear();for(const auto& r:ci.apexFace)apexIds.insert(r.first);}splitPolicy.relocated=&apexIds;};
  auto apexMove=[&](const Face& key,std::string& why)->bool{
    if(ApexPlace(s,rows,key,g,why,certified)){++g.acceptedBy["ApexPlace"];return true;}++g.demand["ApexPlace: "+Key(why)];
    const bool ok=ApexMove12(s,rows,key,g,why,certified);if(ok)++g.acceptedBy["ApexMove"];return ok;};
  auto columnSplit=[&](Edge w,std::string& why)->bool{
    long double lw=INFINITY,ll=INFINITY;try{lw=Len(s,w[0],w[1]);}catch(const std::exception&){}
    const auto la=rows.lift.find(w[0]),lb=rows.lift.find(w[1]);
    if(la!=rows.lift.end()&&lb!=rows.lift.end()){try{ll=Len(s,std::min(la->second,lb->second),std::max(la->second,lb->second));}catch(const std::exception&){}}else ll=0;
    const long double l=std::max(lw,ll);if(l<2*HalfGuard){++g.halves_guard;why="ColumnSplit: halves guard (split edges < 1.4)";return false;}
    splitPolicy.requested=l;struct Clear{~Clear(){splitPolicy.requested=-1;}} clear;
    std::set<Face> faces;for(const auto& r:rows.apex)if(Has(r.first,w[0])&&Has(r.first,w[1]))faces.insert(r.first);
    const bool ok=ColumnSplit(s,rows,w,g,why,rejected);
    if(ok){for(const auto& f:faces)certified.insert(f);for(const auto& wf:WallFaces(s))if(Has(wf.second,w[0])||Has(wf.second,w[1]))certified.insert(wf.second);++g.acceptedBy["ColumnSplit"];}
    return ok;};
  auto ridgeEdge=[&](Edge w){if(rows.copies.count(w[0])||rows.copies.count(w[1]))return true;
    for(const auto& r:rows.apex)if(Has(r.first,w[0])&&Has(r.first,w[1]))for(Id x:r.first)if(rows.copies.count(x))return true;return false;};
  std::map<Face,unsigned> firstTried;
  bool globalStalled=false;  // DC-17: the last mesh-wide optimizer pass made no progress
  size_t measureWitnesses=0;VertexTensors& vt=eventTensors;Id scanned=0;std::set<Edge> queued;size_t lastDemand=0;  // DC-16 demand queue state
  for(;;++g.sweeps) {
    if(limited()){g.outcome="limit";break;}
    if(g.sweeps>=maxSweeps){g.outcome="sweep cap";break;}
    // DC-17 (target 3): with the demand queue the sweep budget is a slice (refine -> repairs -> optimizer alternate at
    // least four times instead of one refine block and one optimizer block)
    const double R=v13DemandQueue?std::min(remaining(),std::max(300.,seconds/4)):remaining();bool changed=false;const auto sweepStart=Clock::now();BoundCaches(s);
    if(vt.at.size()>(size_t(1)<<22))vt.at.clear();
    // (a) R2 retry (sweeps 0 and 1) and the certified first-child route
    if(g.sweeps<2&&v13R2Retry){const auto t=Clock::now();size_t rowed0=0;for(const auto& wf:WallFaces(s))rowed0+=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});
      const auto apex0=rows.apex;
      const auto r2=PhaseR2(s,static_cast<Rows&>(rows),std::min(60.,.1*R),log);++g.r2_retries;
      for(const auto& r:rows.apex){const auto o=apex0.find(r.first);if(o==apex0.end()||o->second!=r.second)certified.insert(r.first);}
      size_t rowed1=0;for(const auto& wf:WallFaces(s))rowed1+=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});
      g.r2_new_rowed+=rowed1>rowed0?rowed1-rowed0:0;changed|=r2.transactions>0;g.r2_seconds+=Seconds(t);}
    {const auto t=Clock::now();
     for(const auto& wf:WallFaces(s)){if(limited()||Seconds(t)>=.1*R)break;if(!s.mesh.count(wf.first))continue;
       const auto a=rows.apex.find(wf.second);const bool registered=a!=rows.apex.end()&&s.vertex_cells.count(a->second);
       double w=0;const bool hOk=HeightOk(s,wf.first,&w);if(hOk&&!Bad(MeshShape(s,wf.first)))continue;
       if(firstTried[wf.second]++>=2)continue;
       // DC-13: a registered first child failing its final check (e.g. after a halving step) -> certified apex placement
       if(registered){std::string why;if(apexMove(wf.second,why)){changed=true;build();}else ++g.demand["first child failing: "+Key(why)];continue;}
       if(FirstRowInsert(s,rows,wf.first,wf.second,g.first,certified))changed=true;}
     g.first_seconds+=Seconds(t);}
    build();
    // (b) bounded refinement of long edges (wall-first, longest first), at most 40 % of R and 1024 attempts per sweep
    {const auto t=Clock::now();const double slice=.4*R;size_t attempts=0;g.column_demand=0;g.demand.clear();const Id refineMark=s.next_cell;
     size_t attemptCap=v13AttemptsPerSweep?v13AttemptsPerSweep:SIZE_MAX;
     auto sliceOver=[&]{return limited()||Seconds(t)>=slice||attempts>=attemptCap;};
     auto touchesColumn=[&](const std::vector<Id>& ids){for(Id id:ids)if(ci.cells.count(id))return true;return false;};
     std::vector<std::tuple<int,long double,Edge>> longs;std::set<Edge> seen;
     auto classify=[&](const Edge& e,long double l){const int k=EdgeClass(s,e);int pri=k==1||k==2?0:k==3?1:2;
         if(v13NearWallFirst&&pri>0){long double d=INFINITY;try{d=MinWallDistance(s,.5L*(Of(NodePoint(s,e[0]))+Of(NodePoint(s,e[1]))));}catch(const std::exception&){}
           pri=d>=s.h0&&d<=300*s.h0?1:pri+1;}
         longs.emplace_back(pri,-l,e);};
     if(v13DemandQueue&&v13EstimateScan) {  // review 8: the final cycle keeps the exact scan (no estimate-only selection)
       // DC-16: edges of the cells created since the previous scan (all cells at the first scan) plus the queue rest
       const Id from=scanned;scanned=s.next_cell;std::set<Edge> cand;
       for(const auto& e:queued)if(s.vertex_cells.count(e[0])&&s.vertex_cells.count(e[1])&&!Star(s,e).empty())cand.insert(e);
       for(auto it=s.mesh.lower_bound(from);it!=s.mesh.end();++it){const auto& c=it->second.cell;
         for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)cand.insert(Edge{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)});}
       queued.clear();
       for(const auto& e:cand){long double l=0;try{l=EstimateLength(s,vt,e[0],e[1]);}catch(const std::exception&){l=INFINITY;}
         if(l>v13DemandThreshold){queued.insert(e);classify(e,std::isfinite(l)?l:1e30L);}}
       lastDemand=longs.size();
     } else
     for(const auto& row:s.mesh){if(sliceOver())break;const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
       const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
       long double l=0;try{bool est=false;l=v13EstimateScan?LenOrEstimate(s,e[0],e[1],est):Len(s,e[0],e[1]);}catch(const std::exception&){continue;}if(l>MaximumMetricEdge)classify(e,l);}}
     std::sort(longs.begin(),longs.end());std::set<Face> movedThisSweep;
     if(!v13AttemptsPerSweep)attemptCap=std::max<size_t>(1024,longs.size()/4);
     for(const auto& row:longs) {
       if(sliceOver())break;const Edge e=std::get<2>(row);queued.erase(e);const auto star=Star(s,e);if(star.empty())continue;
       if(touchesColumn(star)) {
         ++attempts;auto base=[&](Id x)->Id{const auto u=ci.unlift.find(x);if(u!=ci.unlift.end())return u->second;return rows.lift.count(x)?x:UINT64_MAX;};
         const Id b0=base(e[0]),b1=base(e[1]);bool done=false;std::string why;
         Face apexKey{};bool apex=false;for(Id x:e){const auto f=ci.apexFace.find(x);if(f!=ci.apexFace.end()){apexKey=f->second;apex=true;}}
         if(apex){if(!movedThisSweep.count(apexKey)){movedThisSweep.insert(apexKey);const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
             if(rejected.count(k)){++g.cache_skips;why="cached rejection (unchanged cavity)";}else{done=apexMove(apexKey,why);if(!done)rejected.insert(k);}}
           if(!done){long double best=-1;Edge w{0,0};for(int i=0;i<3;++i){const Id x=apexKey[i],y=apexKey[(i+1)%3];long double l=0;try{l=Len(s,x,y);}catch(const std::exception&){l=INFINITY;}if(l>best){best=l;w={std::min(x,y),std::max(x,y)};}}
             if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=columnSplit(w,why);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}}
         else if(b0!=UINT64_MAX&&b1!=UINT64_MAX&&b0!=b1){const Edge w{std::min(b0,b1),std::max(b0,b1)};if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=columnSplit(w,why);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}
         else why="column demand without a column route";
         if(done){changed=true;build();continue;}
         ++g.column_demand;++g.demand[Key(why)];g.last="column demand: "+why;
         // DC-13: a column edge off the wall (lateral diagonal, lid or apex edge) without a column route falls through to
         // the generic variants; first children stay protected by firstChildKept, apex/lid/wall vertices are never moved
         {const int k=EdgeClass13(s,e);if(k==1||k==2)continue;}
         ++g.demand["column edge -> generic refinement"];
       }
       const auto key=keyOf(1,{e[0],e[1]},star);if(rejected.count(key)){++g.cache_skips;continue;}
       long double now=-std::get<1>(row);try{now=Len(s,e[0],e[1]);}catch(const std::exception&){now=INFINITY;}
       if(!(now>MaximumMetricEdge)){++g.stale_skips;continue;}
       if(now<2*HalfGuard){++g.halves_guard;continue;}
       ++attempts;++g.refine_attempts;splitPolicy.requested=now;bool taken=false;size_t vi=0;
       const int cls=EdgeClass13(s,e);auto& census=g.byClass[cls];++census.attempts;
       // DC-13 (DC13_PLAN sect. 2): every admitted variant is compared, the one with the largest exact fresh minimum
       // score is applied (a choice among admissible operations); interior edges also get the placement variant
       struct Best{std::string op;std::vector<Id> ids;Patch old,fresh;double score=-INFINITY;size_t relocated=0;} best;
       auto variants=RefineVariantsOf(s,e,g);
       if(cls==0)variants.push_back({"ShellCone",[&](std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why)->bool{
         ids=Star(s,e);if(ids.empty()||ids.size()>MaximumCavityCells){why="shell size";return false;}old=PatchOf(s,ids);
         double score=0;std::vector<Cell> cells;
         if(!PlaceConeInsertion(old.cells,Pt((1.L/2)*(Of(NodePoint(s,e[0]))+Of(NodePoint(s,e[1])))),s.next_node,s.next_cell,RegionMetric(*s.field),cells,score,&e,.6,240)){why="no valid placement";return false;}
         fresh.cells=cells;fresh.surface=old.surface;return true;}});
       for(auto& variant:variants) {
         const size_t index=vi++;splitPolicy.relocatedCount=0;if(sliceOver())break;std::vector<Id> ids;Patch old,fresh;std::string reason;bool built=false;
         try{built=variant.second(ids,old,fresh,reason);}catch(const std::exception& x){reason=x.what();built=false;}
         if(built&&variant.first=="wall"&&(index==1||index==2)){std::set<Id> before;for(const auto& c:old.cells)for(auto n:c.v)before.insert(n.id);
           std::set<Id> moved;for(const auto& c:fresh.cells)for(auto n:c.v)if(n.id>=s.next_node+1&&!before.count(n.id))moved.insert(n.id);splitPolicy.relocatedCount=moved.size();}
         if(!built){++g.work;++g.rejected;++g.reasons["Refine build: "+Key(reason)];++census.reasons[variant.first+" build: "+Key(reason)];continue;}
         if(touchesColumn(ids)&&variant.first=="wall"){++g.rejected;++g.reasons["Refine: protected column"];++census.reasons["protected column"];continue;}
         for(const auto& pr:old.parameters)fresh.parameters.emplace(pr.first,pr.second);
         try{s.reference->ExportParameters(fresh);}catch(const std::exception&){}
         if(variant.first!="wall"&&!firstChildKept(old,fresh)){++g.work;++g.rejected;++census.reasons[variant.first+" changes a first child"];continue;}
         ++g.work;const auto d=Admit12(s,old,fresh,Kind::Refine);
         if(!d.ok){++g.rejected;++g.reasons[variant.first+" ["+std::to_string(d.category)+"] "+Key(d.reason)];++census.reasons[variant.first+" ["+std::to_string(d.category)+"] "+Key(d.reason)];g.last=variant.first+": "+d.reason;
           if(d.category==5&&measureWitnesses<16){++measureWitnesses;log<<"DC16 V measure witness "<<variant.first<<" edge "<<e[0]<<"-"<<e[1]<<": "<<d.reason.substr(0,600)<<"\n";}continue;}
         const double sc=std::min({d.first_q/MinimumMeanRatio,d.first_J/MinimumScaledJacobian,d.other_q/MinimumMeanRatio,d.other_J/MinimumScaledJacobian});
         if(variant.first=="wall"){best=Best{variant.first,ids,old,fresh,INFINITY,splitPolicy.relocatedCount};break;}  // certified wall rebuild first
         if(sc>best.score)best=Best{variant.first,ids,old,fresh,sc,splitPolicy.relocatedCount};
       }
       if(!best.op.empty()) {
         if(best.op=="wall"){std::set<Face> before,after;for(const auto& sf:best.old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))before.insert(Sorted(sf.v));
           for(const auto& sf:best.fresh.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))after.insert(Sorted(sf.v));
           for(const auto& f:before)certified.insert(f);for(const auto& f:after)certified.insert(f);}
         if(v13DemandQueue&&cls==0&&best.op!="wall") {
           // DC-17 (target 3): an interior refinement and the local repair of its region are ONE transaction, kept only
           // when the region holds no more cells below the final shape gate than before (V converges in bad cells
           // instead of trading long edges for unrepairable slivers); otherwise the region is restored from its snapshot
           std::set<Id> region(best.ids.begin(),best.ids.end());
           for(Id id:best.ids)for(const auto& n:s.mesh.at(id).cell.v){const auto st=s.vertex_cells.find(n.id);if(st!=s.vertex_cells.end())region.insert(st->second.begin(),st->second.end());}
           Owned snapshot;size_t bad0=0;for(Id id:region){snapshot.emplace(id,s.mesh.at(id));bad0+=Bad(MeshShape(s,id));}
           const Id mark=s.next_cell;Apply(s,best.ids,best.fresh);
           std::set<Id> area;for(Id id:region)if(s.mesh.count(id))area.insert(id);for(auto it=s.mesh.lower_bound(mark);it!=s.mesh.end();++it)area.insert(it->first);
           OptimizeReport lr;LocalOptimize(s,rows,G,area,std::min(.5,std::max(.05,remaining())),lr,true,256);++g.transactional_repairs;
           area.clear();for(Id id:region)if(s.mesh.count(id))area.insert(id);for(auto it=s.mesh.lower_bound(mark);it!=s.mesh.end();++it)area.insert(it->first);
           size_t bad1=0;for(Id id:area)bad1+=Bad(MeshShape(s,id));
           if(bad1>bad0) {
             std::vector<Id> drop;for(Id id:area)if(!snapshot.count(id))drop.push_back(id);
             Patch back;for(const auto& row:snapshot){if(s.mesh.count(row.first))continue;back.cells.push_back(row.second.cell);const auto f=CellFaces(row.second.cell);
               for(unsigned k=0;k<4;++k)if(row.second.physical[k].present)back.surface.push_back({f[k],row.second.physical[k].facet});}
             Apply(s,drop,back);++g.transactional_rollbacks;++g.rejected;++g.reasons["Refine+repair: adds cells below the final shape gate"];
             ++census.reasons["refine+repair adds bad cells"];
           } else {++g.accepted;++g.acceptedBy[best.op+"+repair"];++census.accepted[best.op];taken=true;}
         } else {
         Apply(s,best.ids,best.fresh);++g.accepted;++g.acceptedBy[best.op];++census.accepted[best.op];taken=true;}}
       splitPolicy.relocatedCount=0;splitPolicy.requested=-1;
       if(!taken)rejected.insert(key);
       changed|=taken;if(taken)build();
     }
     g.refine_seconds+=Seconds(t);
     // DC-17 (target 3): the region this refinement slice created is optimized at once (local transaction), so a
     // refinement does not leave its new cells for a later mesh-wide pass
     if(v13DemandQueue&&!limited()&&s.next_cell>refineMark){const auto tl=Clock::now();std::set<Id> region;
       for(auto it=s.mesh.lower_bound(refineMark);it!=s.mesh.end();++it)for(const auto& n:it->second.cell.v){const auto st=s.vertex_cells.find(n.id);if(st!=s.vertex_cells.end())region.insert(st->second.begin(),st->second.end());}
       OptimizeReport rep;const bool moved=LocalOptimize(s,rows,G,region,std::min(remaining(),.15*R),rep);changed|=moved;if(moved)build();
       g.opt.push_back(rep);g.optimize_seconds+=Seconds(tl);
       log<<"DC17 V local optimize sweep "<<g.sweeps<<" region="<<region.size()<<" bad "<<rep.bad_before<<"->"<<rep.bad_after<<" O1="<<rep.edge_removals<<" O2="<<rep.face_swaps<<" O3="<<rep.collapses<<" O4="<<rep.moves
          <<" stop="<<rep.stop<<" t="<<rep.seconds<<"\n";}}
    // (c) bad first-row column cells -> ApexMove (certified, Admit12 Repair through the override)
    {std::vector<std::pair<double,Id>> badColumn;for(const auto& row:s.mesh)if(ci.cells.count(row.first)){const auto sh=MeshShape(s,row.first);if(Bad(sh))badColumn.emplace_back(sh.q,row.first);}
     std::sort(badColumn.begin(),badColumn.end());std::set<Face> moved;
     for(const auto& b:badColumn){if(limited()||Seconds(sweepStart)>=.6*R)break;if(!s.mesh.count(b.second))continue;  // keep the optimizer's 40 % share
       Face apexKey{};bool found=false;for(auto n:s.mesh.at(b.second).cell.v){const auto f=ci.apexFace.find(n.id);if(f!=ci.apexFace.end()&&rows.apex.count(f->second)&&rows.apex.at(f->second)==n.id){apexKey=f->second;found=true;}}
       if(!found||!moved.insert(apexKey).second)continue;
       const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
       if(rejected.count(k))continue;std::string why;if(apexMove(apexKey,why)){changed=true;build();}else{rejected.insert(k);++g.demand["bad column cell: "+Key(why)];}}
    }
    // (c2) DC-13 boundary-constrained repair (DC13_PLAN sect. 4, review 2 #7): a bad cell with an edge on a non-wall
    // physical face (carrier or artificial cut boundary) gets that boundary edge split by the production physical
    // variants (bound point on the reference); the best Admit12 Repair candidate (validity + quality/size progress)
    // is applied; first children are protected (firstChildKept)
    {std::vector<std::pair<double,Id>> badB;
     std::set<Id> skin;for(const auto& row:s.mesh)for(unsigned k=0;k<4;++k)if(row.second.physical[k].present&&!s.walls.count(s.reference->Get(row.second.physical[k].facet).marker)){const auto f=CellFaces(row.second.cell);skin.insert(f[k].begin(),f[k].end());}
     for(const auto& row:s.mesh){if(WallChild(s,row.first))continue;unsigned on=0;for(const auto& n:row.second.cell.v)on+=skin.count(n.id);if(on<2)continue;
       const auto sh=MeshShape(s,row.first);if(Bad(sh))badB.emplace_back(std::min(sh.q/MinimumMeanRatio,sh.J/MinimumScaledJacobian),row.first);}
     std::sort(badB.begin(),badB.end());
     for(const auto& b:badB){if(limited()||Seconds(sweepStart)>=.65*R)break;if(!s.mesh.count(b.second))continue;
       const auto row=s.mesh.at(b.second);std::vector<std::pair<long double,Edge>> edges;
       for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){const Edge e{std::min(row.cell.v[i].id,row.cell.v[j].id),std::max(row.cell.v[i].id,row.cell.v[j].id)};
         if(!skin.count(e[0])||!skin.count(e[1]))continue;const int cls=EdgeClass13(s,e);if(cls!=3&&cls!=4)continue;
         long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}edges.push_back({-l,e});}
       std::sort(edges.begin(),edges.end());bool done=false;
       for(const auto& ed:edges){if(done||limited())break;const Edge e=ed.second;const auto star=Star(s,e);if(star.empty())continue;
         const auto key=keyOf(4,{e[0],e[1]},star);if(rejected.count(key))continue;
         struct Best{std::string op;std::vector<Id> ids;Patch old,fresh;double score=-INFINITY;} best;
         for(auto& variant:RefineVariantsOf(s,e,g)){if(variant.first=="wall")continue;std::vector<Id> ids;Patch old,fresh;std::string reason;bool built=false;
           try{built=variant.second(ids,old,fresh,reason);}catch(const std::exception& x){reason=x.what();built=false;}
           if(!built){++g.reasons["boundary repair build: "+Key(reason)];continue;}
           for(const auto& pr:old.parameters)fresh.parameters.emplace(pr.first,pr.second);try{s.reference->ExportParameters(fresh);}catch(const std::exception&){}
           if(!firstChildKept(old,fresh))continue;
           ++g.work;const auto d=Admit12(s,old,fresh,Kind::Repair);if(!d.ok){++g.reasons["boundary repair ["+std::to_string(d.category)+"] "+Key(d.reason)];continue;}
           const double sc=std::min({d.first_q/MinimumMeanRatio,d.first_J/MinimumScaledJacobian,d.other_q/MinimumMeanRatio,d.other_J/MinimumScaledJacobian});
           if(sc>best.score)best=Best{variant.first,ids,old,fresh,sc};}
         if(!best.op.empty()){Apply(s,best.ids,best.fresh);++g.accepted;++g.acceptedBy["boundary repair "+best.op];changed=true;done=true;build();}
         else rejected.insert(key);}
       if(!done)++g.demand["bad boundary cell without repair"];}
    }
    // (d) mesh-wide optimization with a reserved share of the sweep budget (DC-17: with the demand queue, skipped after a
    // pass that removed almost nothing - its time goes to refinement and the local repairs)
    if(v13DemandQueue&&globalStalled&&remaining()>2*R){++g.global_skipped;}  // the last slice always optimizes
    else if(!limited()){const auto t=Clock::now();
      OptimizeOptions opt;opt.maximum_sweeps=32;  // DC-13: inner sweeps until no progress (time-bounded)
     opt.maximum_seconds=v13DemandQueue?std::max(1.,std::min(remaining(),R-Seconds(sweepStart))):std::max(1.,std::min(remaining(),std::max(.4*R,.5*remaining())));
      opt.cache_entries=size_t(1)<<19;if(s.mapped&&!v13MappedBoundaryOps)opt.boundary=false;
      // DC-17 (target 3): the whole-mesh scans of the optimizer read the exact cell-shape memo of the state (same
      // measure, cells immutable by identity) and its tensor/length memos are sized to the mesh, so a sweep no longer
      // re-measures 1.5M cells after its 2^19-entry scratch overflows
      if(v13DemandQueue){opt.cache_entries=std::max(opt.cache_entries,4*s.mesh.size());
        opt.mesh_quality=[&s](Id id,double& q,double& J){if(!s.mesh.count(id))return false;const auto sh=MeshShape(s,id);q=sh.q;J=sh.J;return true;};}
      for(const auto& row:s.mesh)if(WallChild(s,row.first)){opt.frozen_cells.insert(row.first);for(auto n:row.second.cell.v)opt.frozen_vertices.insert(n.id);}
      for(Id id:FanCells(s,rows)){opt.frozen_cells.insert(id);for(auto n:s.mesh.at(id).cell.v)opt.frozen_vertices.insert(n.id);}
      // DC-13: lids are not frozen (no first child contains a lid); a moved lid is relabelled in rows.lift and G below
      for(const auto& r:rows.lift)opt.keep_vertices.insert(r.second);  // movable, never collapsed (registry identity)
      for(const auto& r:rows.copies)for(Id c:r.second.ids)opt.frozen_vertices.insert(c);
      for(const auto& r:rows.apex)opt.frozen_vertices.insert(r.second);
      // TEST-ONLY cut-artifact diagnostic (coordinator 2026-10-24, SU2_DC13_CUT_SLIDE=1, never the main result): skin
      // vertices whose every boundary face is on the artificial cut slide in the plane of their cut faces (in a full
      // mesh they are interior vertices); geometry of the cut is then not certified
      if(diagnosticCutSlide) {
        std::map<Id,V> normal;std::set<Id> other;
        for(const auto& row:s.mesh){const auto f=CellFaces(row.second.cell);for(unsigned k=0;k<4;++k){if(!row.second.physical[k].present)continue;
          const int m=s.reference->Get(row.second.physical[k].facet).marker;if(!artificialMarkers.count(m)){for(Id x:f[k])other.insert(x);continue;}
          const V a=Of(NodePoint(s,f[k][0])),b=Of(NodePoint(s,f[k][1])),c=Of(NodePoint(s,f[k][2]));const V n=Cross(b-a,c-a);for(Id x:f[k])normal[x]=normal[x]+n;}}
        for(const auto& n:normal)if(!other.count(n.first)&&!opt.frozen_vertices.count(n.first)&&Norm(n.second)>0){const V u=Unit(n.second);opt.diagnostic_sliding[n.first]={double(u.x),double(u.y),double(u.z)};}
        g.demand["DIAGNOSTIC cut vertices allowed to slide"]+=opt.diagnostic_sliding.size();
      }
      const RegionMetric target([&s](Point p){return s.field->Query(p).target;},[&s](Point a,Point b){return GeometryLength(s,a,b);});
      OptimizeReport rep;Id nn=s.next_node,nc=s.next_cell;bool moved=false;
      {Scope sc("V12 OptimizeMetricMesh");try{moved=OptimizeMetricMesh(s.mesh,target,nn,nc,opt,rep);}catch(const std::exception& e){rep.stop=std::string("exception: ")+e.what();}}
      s.next_node=std::max(s.next_node,nn);s.next_cell=std::max(s.next_cell,nc);Index(s);
      for(auto& c:G.current){const auto r=rep.relabel.find(c.second);if(r!=rep.relabel.end())c.second=r->second;}
      for(auto& l:rows.lift){const auto r=rep.relabel.find(l.second);if(r!=rep.relabel.end())l.second=r->second;}
      for(auto& c:G.columns){for(Id* x:{&c.lid,&c.top}){const auto r=rep.relabel.find(*x);if(r!=rep.relabel.end())*x=r->second;}}
      g.opt.push_back(rep);changed|=moved;g.optimize_seconds+=Seconds(t);
      globalStalled=v13DemandQueue&&rep.bad_before<rep.bad_after+std::max<size_t>(1,rep.bad_before/500);  // removed < 0.2 % (at least 1)
      log<<"DC12 V optimize sweep "<<g.sweeps<<" bad "<<rep.bad_before<<"->"<<rep.bad_after<<" min "<<rep.minimum_before<<"->"<<rep.minimum_after<<" O1="<<rep.edge_removals
         <<" O2="<<rep.face_swaps<<" O3="<<rep.collapses<<" O4="<<rep.moves<<" stop="<<rep.stop<<" t="<<rep.seconds<<"\n";}
    if(v13SkipFinalOnLimit&&limited()){g.outcome="limit";log<<"DC12 V sweep "<<g.sweeps<<" stopped by its limit (gate scan skipped) t="<<Seconds(t0)<<"\n";break;}
    if(v13DemandQueue&&v13EstimateScan){  // DC-16: intermediate event, no full gate scan per sweep
      log<<"DC16 V sweep "<<g.sweeps<<" cells="<<s.mesh.size()<<" estimated_demand="<<lastDemand<<" queued="<<queued.size()<<" vertex_queries="<<vt.queries<<" work="<<g.work<<" t="<<Seconds(t0)<<"\n";
      std::cout<<"DC16 V sweep "<<g.sweeps<<" cells="<<s.mesh.size()<<" estimated_demand="<<lastDemand<<" accepted="<<g.accepted<<" t="<<Seconds(t0)<<std::endl;
      g.per_sweep.push_back({s.mesh.size(),lastDemand,0,0,g.work,Seconds(t0)});
      if(limited()){g.outcome="limit";break;}
      if(!changed){g.outcome="blocked";break;}
      continue;}
    const auto gates=Final(s);
    log<<"DC12 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" height_fail="<<gates.height_failures<<" unmeasurable="<<gates.unmeasurable<<" work="<<g.work<<" t="<<Seconds(t0)<<"\n";
    std::cout<<"DC12 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" h="<<gates.height_failures<<" t="<<Seconds(t0)<<std::endl;
    g.per_sweep.push_back({gates.cells,gates.long_edges,gates.bad_shape,gates.height_failures,g.work,Seconds(t0)});
    if(gates.complete){g.outcome="complete";break;}
    if(limited()){g.outcome="limit";break;}
    if(!changed){g.outcome="blocked";break;}
  }
  End(s,g,t0);return g;
}
}  // namespace dc13
