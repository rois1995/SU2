/*!
 * \file BLFirstRow9.inl
 * \brief BL pipeline core of DC-9: caches, crease geometry, lift directions, apex LP, sector plans, split policy, R1, R2, final gates, ApexMove, ColumnSplit, V.
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc9.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// BL_FIRST_ROW_DESIGN rev 2.1, DC-9: (history: formerly test-only) [NativeFirstRowDC9] harness, a copy of the DC-8 harness with the DC-9
// construction changes marked "DC-9" below: (a) W crease-strip spacing (no metric-short wall edge when another split
// point avoids it) and chain collapses/moves of metric-short wall edges along their feature chain (CollapseVertex /
// MoveVertex keep every incident reference facet, so a point only moves along its crease line; original corners are
// never collapsed or moved); (b) lazy R2 excess certificate (added excess first; the removed side is integrated only
// while it can still decide) and a coordinate-keyed composed-length memo; (c) sector lifts whose junction neighbour is
// already lifted (cut path through a link vertex above the lid plane, otherwise a Prepare-kind link split above the
// band); (d) ColumnSplit of a one-column wall/carrier junction edge. Limits and gates are unchanged.
// DC-8 text: extends DC-7 (W -> R1 -> R2 -> V) by
// the four routes DC-7 named as missing, as one connected step, with production primitives only:
//  (a) carrier sector lifts: a wall vertex whose carrier fan spans several canonical charts that one lift point cannot
//      serve gets one bound lift point per chart seam (BindNewPoint on both seam charts); the vertex star is split along
//      shortest link paths from each seam to its junction (gap tetrahedra), the corner column takes the rim triangles.
//      A corner cell owning a wall face and a foreign carrier face is first separated (InsertEdgeCavity, Prepare kind).
//  (b) structured crease strips in W: crease edges (convex D1 edges detected from the authority facets with the
//      production rule) are split first to the along-crease station target; every other long wall edge is split at its
//      crossing with a D1 level line rho_k (unit metric spacing of the production law h0 + ln(HGRAD) rho) when it
//      crosses one, otherwise at the metric midpoint, so wall edges end parallel to the crease, graded from h0;
//      metric-short wall edges (< 0.25) left by converging level points are then collapsed (CollapseVertex).
//  (c) bounded clearance route: band height down to the feasible LP window (1-T)h0(1+5%) before any closure growth;
//      R1 with the finite candidate set of §8.3 (Reconnect of each link edge, MoveVertex kernel direction + halvings,
//      CollapseVertex coarsening, Steiner at {2,3}H) and explicit limit labels. Caps unchanged (64/128).
//  (d) V: ApexMove and ColumnSplit for column demand, exact on-edge split points (exact OnEdge predicate, otherwise no
//      SplitEdge trial), no InsertEdgeCavity trial on chart/marker junctions, topology-keyed rejection cache (no
//      repeated identical rejected trials), balanced split point computed once per edge, wall-first edge order.
// Performance: per phase seconds, peak RSS, FrozenField counters; per-operation counts/seconds; cached cell shapes,
// edge lengths and first-height verdicts (cells and vertices are immutable by identity). Final gates are unchanged
// and absolute; the final mesh is re-audited with a fresh field and no caches. (DC-14: production.)
#include <sys/resource.h>
#include <unordered_map>
namespace dc9 {
using namespace layer2d;
using Clock=std::chrono::steady_clock;
constexpr long double Kappa=1.2L,Beta=1e-3L,LowBand=1.3L,HighBand=2.2L,BandMargin=0.05L,StationTarget=1.45L,ShortWallEdge=0.25L;
inline double Seconds(Clock::time_point t){return std::chrono::duration<double>(Clock::now()-t).count();}
inline long double Det(V a,V b,V c,V d){return Dot(b-a,Cross(c-a,d-a));}
inline long double Sigma(){static const long double s=Orientation({0,0,0},{1,0,0},{0,1,0},{0,0,1})>0?1.L:-1.L;return s;}
inline long double SDet(V a,V b,V c,V d){return Sigma()*Det(a,b,c,d);}
inline V Outward(V a,V b,V c){return Sigma()*Unit(Cross(b-a,c-a));}
inline bool Positive(const std::array<Node,4>& v){return Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0;}

// ---- performance accounting --------------------------------------------------------------------------------------
struct Perf {
  std::map<std::string,std::pair<size_t,double>> ops;
  size_t length_hits=0,length_misses=0,length_failures=0,shape_hits=0,shape_misses=0,height_hits=0,height_misses=0,cache_skips=0;
};
inline Perf perf;
struct Scope {
  const char* key;Clock::time_point t0;
  explicit Scope(const char* k):key(k),t0(Clock::now()){}
  ~Scope(){auto& r=perf.ops[key];++r.first;r.second+=Seconds(t0);}
};
inline long PeakKB(){struct rusage u{};getrusage(RUSAGE_SELF,&u);return u.ru_maxrss;}
struct Counters { uint64_t queries=0,misses=0,compositions=0,edges=0,edge_compositions=0,edge_pieces=0; };
inline Counters CountersOf(State& s){const auto& t=s.field->Statistics();return {t.requests,t.misses,t.compositions,t.edge_requests,t.edge_compositions,t.edge_pieces};}
inline std::string JsonCounters(const Counters& a,const Counters& b){std::ostringstream o;o<<"{\"queries\":"<<b.queries-a.queries<<",\"query_misses\":"<<b.misses-a.misses
  <<",\"compositions\":"<<b.compositions-a.compositions<<",\"edge_integrations\":"<<b.edges-a.edges<<",\"edge_compositions\":"<<b.edge_compositions-a.edge_compositions
  <<",\"edge_pieces\":"<<b.edge_pieces-a.edge_pieces<<'}';return o.str();}

// ---- caches keyed by immutable identities --------------------------------------------------------------------------
inline std::unordered_map<Id,CellShape> shapeCache;
inline std::unordered_map<Id,std::pair<bool,double>> heightCache;
inline bool cachesOn=true;
inline std::map<std::array<double,6>,long double> geometryLengths;  // DC-14: defined here (inline), used by ResetCaches
inline void ResetCaches(){shapeCache.clear();heightCache.clear();geometryLengths.clear();}
inline std::map<std::array<double,8>,Point> balancedCache;  // (edge ids, endpoint coordinates) -> balanced point
// DC-14: bounded memos (cleared at a fixed entry count; values are recomputed identically, only speed changes).
inline size_t cacheEntryLimit=size_t(1)<<22;
inline void BoundCaches(State& s) {
  if(shapeCache.size()>cacheEntryLimit)shapeCache.clear();if(heightCache.size()>cacheEntryLimit)heightCache.clear();
  if(geometryLengths.size()>cacheEntryLimit)geometryLengths.clear();if(balancedCache.size()>cacheEntryLimit)balancedCache.clear();
  if(s.lengths.size()>cacheEntryLimit)s.lengths.clear();
}
inline CellShape Shape(State& s,const Cell& c){try{return ShapeOf(s,c);}catch(const std::exception&){return {-1,-1};}}
inline CellShape ShapeOrInvalid(State& s,const Cell& c){return Shape(s,c);}
inline bool Bad(const CellShape& sh){return sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian;}
// ShapePolicy remains for harness compatibility; production construction checks exact positive orientation. Final gates use Bad.
struct ShapePolicy { bool validity=false; };
inline ShapePolicy shapePolicy;
// DC-16 (validity-only intermediate events, user decision): set by the production entry on intermediate (ramped-h0)
// events only. R2 then admits a lift that removes no vertex by its validity checks (cell validity, heights, carriers,
// interface, D.2) without integrating its changed edges, and Admit12 admits such a LayerConstruct without measuring
// them. Final cycles and every harness keep the certificates (default false).
inline bool validityOnlyConstruction=false;
// Construction may carry a measured requested-height deficit forward, but never accepts missing geometry or
// failed reference clearance. Final validation continues to use strict CheckFirstHeight/HeightT.
inline bool HeightConstruct(const Patch& p,const WallHeights& h,const PlanarReference& ref,FirstHeightReport* report=nullptr,std::string* witness=nullptr) {
  FirstHeightReport r;std::string why;if(CheckFirstHeight(p,h,ref,r,why)){if(report)*report=r;return true;}
  if(report)*report=r;if(witness)*witness=why;
  return validityOnlyConstruction&&r.measured&&!r.missing_cells&&!r.reference_clearance_failures;
}

inline bool Invalid(const CellShape& sh){return Bad(sh);}
inline CellShape ShapeOrInvalid(State& s,const Cell& c);
// Every fresh construction cell is checked for exact positive orientation.
inline bool FreshValid(State& s,const std::vector<Cell>& cells){if(!shapePolicy.validity)return true;
  for(const auto& c:cells)if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0))return false;return true;}
inline CellShape MeshShape(State& s,Id id) {
  if(cachesOn){const auto f=shapeCache.find(id);if(f!=shapeCache.end()){++perf.shape_hits;return f->second;}}
  ++perf.shape_misses;const auto sh=Shape(s,s.mesh.at(id).cell);if(cachesOn)shapeCache.emplace(id,sh);return sh;
}
inline long double LenAt(State& s,Id a,Point pa,Id b,Point pb);
inline long double GeometryLength(State& s,Point pa,Point pb);  // DC-9 (b): coordinate-keyed memo of TargetEdgeLength
// Composed edge length; an unmeasurable edge is cached as -1 and rethrown (no repeated failing integrations).
inline long double Len(State& s,Id a,Id b) {
  const auto key=std::minmax(a,b);const auto pa=NodePoint(s,key.first),pb=NodePoint(s,key.second);
  const auto found=s.lengths.find({key.first,key.second});
  if(found!=s.lengths.end()&&SamePoint(found->second.first[0],pa)&&SamePoint(found->second.first[1],pb)) {
    ++perf.length_hits;if(found->second.second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return found->second.second;}
  return LenAt(s,key.first,pa,key.second,pb);
}
inline bool HeightOk(State& s,Id cell,double* worst=nullptr) {
  if(cachesOn){const auto f=heightCache.find(cell);if(f!=heightCache.end()){++perf.height_hits;if(worst)*worst=f->second.second;return f->second.first;}}
  ++perf.height_misses;std::string why;double w=0;bool ok=false;
  try{ok=HeightT(s,PatchOf(s,{cell}),why,&w);}catch(const std::exception&){ok=false;}
  if(cachesOn)heightCache.emplace(cell,std::make_pair(ok,w));if(worst)*worst=w;return ok;
}

// ---- mesh helpers (as DC-7) ----------------------------------------------------------------------------------------
struct Rows {
  std::map<Id,Id> lift;
  std::map<Face,Id> apex;
  // DC-9 (c): sector columns keep their junction override {v,u,P_1} and extra skin faces (rims; lids whose third
  // vertex is the junction wall vertex u, realized as its current lift), so a later lift of u rebuilds the same column.
  std::map<Face,std::array<Id,3>> overrides;
  std::map<Face,std::vector<std::pair<Face,bool>>> extras;
  std::set<Id> Apices() const {std::set<Id> a;for(const auto& r:apex)a.insert(r.second);return a;}
};
struct WF { Face key{},out{};Id facet=0,owner=0;int marker=0; };
inline std::vector<WF> PhysicalOf(const State& s,const std::set<Id>& cells,bool wall) {
  std::vector<WF> out;std::set<Face> seen;
  for(Id id:cells){const auto found=s.mesh.find(id);if(found==s.mesh.end())continue;const auto& row=found->second;const auto f=CellFaces(row.cell);
    for(unsigned k=0;k<4;++k)if(row.physical[k].present){const int m=s.reference->Get(row.physical[k].facet).marker;
      if(bool(s.walls.count(m))!=wall)continue;const auto key=Sorted(f[k]);if(!seen.insert(key).second)continue;
      out.push_back({key,f[k],row.physical[k].facet,id,m});}}
  return out;
}
inline std::set<Id> StarOf(const State& s,Id v){const auto f=s.vertex_cells.find(v);return f==s.vertex_cells.end()?std::set<Id>{}:f->second;}
inline std::set<Id> WallVertices(const State& s){std::set<Id> out;for(const auto& f:WallFaces(s))out.insert(f.second.begin(),f.second.end());return out;}
inline std::set<Id> ColumnCells(const State& s,const Rows& rows){std::set<Id> out;for(const auto& r:rows.apex){const auto st=StarOf(s,r.second);out.insert(st.begin(),st.end());}return out;}
inline bool Has(const Face& f,Id v){return f[0]==v||f[1]==v||f[2]==v;}
// DC-12: the artificial (cut) boundary by loader role metadata when rimByRole is set; otherwise the inherited marker
// name test (default, every earlier harness unchanged).
inline std::set<int> artificialMarkers;inline bool rimByRole=false;
inline bool Rim(const State& s,Id v) {
  for(Id id:StarOf(s,v)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
    if(r.physical[k].present&&Has(f[k],v)){const int m=s.reference->Get(r.physical[k].facet).marker;if(rimByRole?artificialMarkers.count(m)>0:s.markers.at(m)=="CUT")return true;}}
  return false;
}
// DC-12 per-wall first heights: the prescribed height of a wall face / the smallest over a vertex's wall faces (s.h0
// when none is found); equal to s.h0 in every loader where all walls share one height.
inline long double FaceH0(const State& s,const Face& f) {
  const auto a=s.vertex_cells.find(f[0]);if(a==s.vertex_cells.end())return s.h0;const auto key=Sorted(f);
  for(Id id:a->second){const auto& r=s.mesh.at(id);const auto faces=CellFaces(r.cell);
    for(unsigned k=0;k<4;++k)if(r.physical[k].present&&Sorted(faces[k])==key){const auto h=s.heights.find(s.reference->Get(r.physical[k].facet).marker);if(h!=s.heights.end())return h->second;}}
  return s.h0;
}
inline long double VertexH0(const State& s,Id v) {
  long double h=INFINITY;const auto a=s.vertex_cells.find(v);if(a==s.vertex_cells.end())return s.h0;
  for(Id id:a->second){const auto& r=s.mesh.at(id);const auto faces=CellFaces(r.cell);
    for(unsigned k=0;k<4;++k)if(r.physical[k].present&&Has(faces[k],v)){const auto x=s.heights.find(s.reference->Get(r.physical[k].facet).marker);if(x!=s.heights.end())h=std::min<long double>(h,x->second);}}
  return std::isfinite(h)?h:s.h0;
}
inline bool RimFace(const State& s,const Face& f){for(Id x:f)if(Rim(s,x))return true;return false;}
inline bool CellHas(const Cell& c,Id v){for(auto n:c.v)if(n.id==v)return true;return false;}

// ---- (b) D1 crease geometry, test-side detection with the production rule (BuildFeatures, CBoundaryLayerMetric:1386)
struct Crease { V a,b,t;long double L=0; };
struct D1Geometry { std::vector<Crease> creases;long double slope=0,radius=0,h0=0,angle=0,scale=1; };
inline D1Geometry d1;
// DC-15: marker roles from the caller (production: CConfig marker kinds, 1 BL wall, 0 symmetry continuation, 2 other);
// no marker name is read here. Test loaders build their roles themselves.
// DC-16: the roles of the last DetectCreases call (the room step of G slides only role-0 continuation planes; empty:
// no vertex slides). Production: CConfig marker kinds; test loaders: their own role rule.
inline std::map<int,int> markerRoles;
inline void DetectCreases(const State& s,const std::map<int,int>& roles) {
  markerRoles=roles;d1=D1Geometry{};const auto& F=s.layers->GetFeatures();d1.slope=F.slope;d1.radius=F.maximumRadius;d1.h0=s.h0;d1.angle=F.angle;
  for(const auto& f:s.facets)for(auto x:f.v)d1.scale=std::max({d1.scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});
  if(!(d1.slope>0))return;
  auto kind=[&](const Facet& f){if(s.walls.count(f.marker))return 1;const auto r=roles.find(f.marker);
    return r!=roles.end()&&r->second==0?0:2;};  // reflective images (role 0) are smooth continuations here
  std::map<std::pair<Id,Id>,std::vector<const Facet*>> edges;
  for(const auto& f:s.facets)if(kind(f)==1)for(int k=0;k<3;++k){const Id a=f.v[k].id,b=f.v[(k+1)%3].id;edges[{std::min(a,b),std::max(a,b)}].push_back(&f);}
  for(const auto& f:s.facets)if(kind(f)==2)for(int k=0;k<3;++k){const Id a=f.v[k].id,b=f.v[(k+1)%3].id;const auto e=edges.find({std::min(a,b),std::max(a,b)});if(e!=edges.end())e->second.push_back(&f);}
  const long double pi=std::acos(-1.L);
  for(const auto& row:edges) {
    if(row.second.size()!=2)continue;const Facet& f1=*row.second[0];const Facet& f2=*row.second[1];
    unsigned k=0;while(k<3&&!(std::minmax(f1.v[k].id,f1.v[(k+1)%3].id)==std::minmax(row.first.first,row.first.second)))++k;if(k==3)continue;
    const Point a=f1.v[k].p,b=f1.v[(k+1)%3].p,o1=f1.v[(k+2)%3].p;Point o2{};for(auto x:f2.v)if(x.id!=row.first.first&&x.id!=row.first.second)o2=x.p;
    const auto s1=Orientation(a,b,o1,o2),s2=Orientation(f2.v[0].p,f2.v[1].p,f2.v[2].p,o1);
    if(!(s1>0&&s2>0))continue;
    const V n1=Unit(Cross(Of(f1.v[1].p)-Of(f1.v[0].p),Of(f1.v[2].p)-Of(f1.v[0].p))),n2=Unit(Cross(Of(f2.v[1].p)-Of(f2.v[0].p),Of(f2.v[2].p)-Of(f2.v[0].p)));
    if(!(std::acos(std::clamp(Dot(n1,n2),-1.L,1.L))*180/pi>d1.angle))continue;
    Crease c;c.a=Of(a);c.b=Of(b);c.L=Norm(c.b-c.a);c.t=(1/c.L)*(c.b-c.a);d1.creases.push_back(c);
  }
}
inline long double Rho(V x){long double best=INFINITY;for(const auto& c:d1.creases){const auto sp=std::clamp(Dot(x-c.a,c.t),0.L,c.L);best=std::min(best,Norm(x-(c.a+sp*c.t)));}return best;}
inline long double Phi(long double rho){return std::log1p(d1.slope*rho/d1.h0)/d1.slope;}
inline long double RhoOf(long double phi){return d1.h0/d1.slope*std::expm1(phi*d1.slope);}
inline bool OnCrease(V p,V q) {
  const long double tol=1e-9L*d1.scale;
  for(const auto& c:d1.creases){auto on=[&](V x){const auto sp=Dot(x-c.a,c.t);return sp>=-tol&&sp<=c.L+tol&&Norm(x-(c.a+sp*c.t))<=tol;};if(on(p)&&on(q))return true;}
  return false;
}

// ---- lift direction (as DC-7) --------------------------------------------------------------------------------------
inline bool MinNormAny(const std::vector<V>& n,V& out) {
  long double best=INFINITY;const size_t m=n.size();if(!m)return false;
  auto test=[&](const std::vector<size_t>& pick) {
    const size_t k=pick.size();long double g[3][3],rhs[3]={1,1,1},lambda[3]={0,0,0};
    for(size_t a=0;a<k;++a)for(size_t b=0;b<k;++b)g[a][b]=Dot(n[pick[a]],n[pick[b]]);
    for(size_t c=0;c<k;++c){size_t r=c;for(size_t t=c+1;t<k;++t)if(std::abs(g[t][c])>std::abs(g[r][c]))r=t;
      if(!(std::abs(g[r][c])>1e-14L))return;for(size_t t=0;t<k;++t)std::swap(g[c][t],g[r][t]);std::swap(rhs[c],rhs[r]);
      for(size_t t=c+1;t<k;++t){const auto f=g[t][c]/g[c][c];for(size_t u=c;u<k;++u)g[t][u]-=f*g[c][u];rhs[t]-=f*rhs[c];}}
    for(size_t c=k;c-->0;){auto x=rhs[c];for(size_t u=c+1;u<k;++u)x-=g[c][u]*lambda[u];lambda[c]=x/g[c][c];}
    V w;for(size_t a=0;a<k;++a){if(lambda[a]<-1e-12L)return;w=w+lambda[a]*n[pick[a]];}
    for(const auto& x:n)if(Dot(x,w)<1-1e-10L)return;
    if(Dot(w,w)<best){best=Dot(w,w);out=Unit(w);}
  };
  for(size_t i=0;i<m;++i){test({i});for(size_t j=i+1;j<m;++j){test({i,j});for(size_t k=j+1;k<m;++k)test({i,j,k});}}
  return std::isfinite(best);
}
struct Dir { bool ok=false;V d;long double s=0;std::set<Id> carriers,bind;int rank=0;Id seam=UINT64_MAX;std::string why;std::vector<V> normals; };
// DC-12 (OPTIMIZE_PLAN rev 5): precomputed column directions (boundary tangency + smooth blend); null = unchanged.
inline std::map<Id,V>* directionField=nullptr;inline size_t directionFieldUses=0;
inline Dir DirectionAt(State& s,Id v) {
  Dir out;const auto star=StarOf(s,v);std::vector<V> cn;
  for(const auto& f:PhysicalOf(s,star,true))if(Has(f.key,v)){const V n=-1.L*Outward(Of(NodePoint(s,f.out[0])),Of(NodePoint(s,f.out[1])),Of(NodePoint(s,f.out[2])));
    bool dup=false;for(const auto& x:out.normals)dup=dup||Dot(x,n)>1-1e-14L;if(!dup)out.normals.push_back(n);}
  for(const auto& f:PhysicalOf(s,star,false))if(Has(f.key,v)){cn.push_back(Outward(Of(NodePoint(s,f.out[0])),Of(NodePoint(s,f.out[1])),Of(NodePoint(s,f.out[2]))));out.carriers.insert(f.facet);}
  if(out.normals.empty()){out.why="no wall face";return out;}
  std::vector<V> basis;for(const auto& x:cn){V r=x;for(const auto& b:basis)r=r-Dot(r,b)*b;if(Norm(r)>1e-9L)basis.push_back(Unit(r));}
  out.rank=int(basis.size());
  if(basis.size()>=3){out.why="carrier: constraint rank 3 leaves no direction";return out;}
  if(basis.size()==2){V t=Unit(Cross(basis[0],basis[1]));long double sum=0;for(const auto& n:out.normals)sum+=Dot(n,t);out.d=sum<0?-1.L*t:t;}
  else{std::vector<V> projected;for(auto n:out.normals){for(const auto& b:basis)n=n-Dot(n,b)*b;if(Norm(n)<1e-12L){out.why="carrier: wall normal parallel to the carrier normal";return out;}projected.push_back(n);}
    if(!MinNormAny(projected,out.d)){out.why="hemisphere: no open hemisphere";return out;}}
  if(directionField&&basis.size()<2){const auto f=directionField->find(v);
    if(f!=directionField->end()){bool inPlane=true;for(const auto& b:basis)inPlane=inPlane&&std::abs(Dot(f->second,b))<=1e-9L;
      long double sd=INFINITY;for(const auto& n:out.normals)sd=std::min(sd,Dot(n,f->second));
      if(inPlane&&sd>=.2L){out.d=f->second;++directionFieldUses;}}}  // the field's visibility margin
  out.s=INFINITY;for(const auto& n:out.normals)out.s=std::min(out.s,Dot(n,out.d));
  if(!(out.s>1e-9L)){out.why="hemisphere: s*="+std::to_string(double(out.s));return out;}
  const V x0=Of(NodePoint(s,v));const auto carrierFaces=PhysicalOf(s,star,false);
  if(out.rank==1) {
    long double best=-INFINITY;
    for(const auto& f:carrierFaces)if(Has(f.key,v)){std::vector<Id> o;for(Id x:f.key)if(x!=v)o.push_back(x);
      const V a=Of(NodePoint(s,o[0]))-x0,b=Of(NodePoint(s,o[1]))-x0;const auto aa=Dot(a,a),ab=Dot(a,b),bb=Dot(b,b),ad=Dot(a,out.d),bd=Dot(b,out.d),det=aa*bb-ab*ab;
      if(!(det>0))continue;const auto al=(ad*bb-bd*ab)/det,be=(bd*aa-ad*ab)/det;const auto score=std::min(al*std::sqrt(aa),be*std::sqrt(bb));
      if(score>best){best=score;out.bind={f.facet};}}
  } else if(out.rank==2) {
    std::map<Id,std::set<int>> planes;std::map<Id,std::set<Id>> facets;
    for(const auto& f:carrierFaces)if(Has(f.key,v)){const V n=Outward(Of(NodePoint(s,f.out[0])),Of(NodePoint(s,f.out[1])),Of(NodePoint(s,f.out[2])));
      const int plane=std::abs(Dot(n,basis[0]))>1-1e-9L?0:1;for(Id x:f.key)if(x!=v){planes[x].insert(plane);facets[x].insert(f.facet);}}
    long double best=-INFINITY;for(const auto& r:planes)if(r.second.size()==2){const auto c=Dot(Unit(Of(NodePoint(s,r.first))-x0),out.d);if(c>best){best=c;out.seam=r.first;out.bind=facets[r.first];}}
    if(out.seam==UINT64_MAX){out.why="carrier: no seam edge along the corner direction";return out;}
    out.d=Unit(Of(NodePoint(s,out.seam))-x0);out.s=INFINITY;for(const auto& n:out.normals)out.s=std::min(out.s,Dot(n,out.d));
    if(!(out.s>1e-9L)){out.why="hemisphere: seam s*="+std::to_string(double(out.s));return out;}
  }
  out.ok=true;return out;
}
struct Kernel { long double delta=INFINITY;Id cell=0; };
inline Kernel KernelBound(const std::vector<Cell>& cells,Id v,V d) {
  Kernel k;
  for(const auto& c:cells){std::array<V,4> p;int at=-1;for(unsigned i=0;i<4;++i){p[i]=Of(c.v[i].p);if(c.v[i].id==v)at=int(i);}if(at<0)continue;
    const auto o0=SDet(p[0],p[1],p[2],p[3]);auto q=p;q[at]=q[at]+d;const auto slope=SDet(q[0],q[1],q[2],q[3])-o0;
    if(slope<0){const auto t=o0/(-slope);if(t<k.delta){k.delta=t;k.cell=c.id;}}}
  return k;
}
inline std::vector<Cell> MovedStar(const State& s,const std::set<Id>& apices,Id v) {
  std::vector<Cell> out;
  for(Id id:StarOf(s,v)){const auto& c=s.mesh.at(id).cell;bool col=false;for(auto n:c.v)col=col||apices.count(n.id);if(!col)out.push_back(c);}
  return out;
}
inline long double Need(long double h0){return (1+HeightTolerance)*h0*Kappa;}
inline long double MinimumBand(long double h0){return (1-HeightTolerance)*h0*(1+BandMargin);}
struct Plan { bool ok=false;Dir dir;Kernel kernel;long double H=0,delta=0,h0=0;V p;std::string why; };
// (c) band height: preferred clamp(0.9 delta_K s*, [1.3,2.2]h0); below 1.3h0 the clearance value itself, floored at the
// height-window top (1+T)h0 (DC-7) or, in the low-band retry, at the feasible LP window (1-T)h0(1+5%). The metric cap
// (vertical column edge <= 1.5) needs edge integrations and is evaluated only for an actual lift (metricCap).
inline Plan PlanLift(State& s,const std::set<Id>& apices,Id v,bool metricCap,bool low) {
  Scope scope("PlanLift");
  Plan pl;pl.h0=VertexH0(s,v);pl.dir=DirectionAt(s,v);if(!pl.dir.ok){pl.why=pl.dir.why;return pl;}
  pl.kernel=KernelBound(MovedStar(s,apices,v),v,pl.dir.d);
  const long double h0=pl.h0;pl.H=std::min(0.9L*pl.kernel.delta*pl.dir.s,HighBand*h0);
  if(low)pl.H=std::min(pl.H,(1+HeightTolerance)*h0);
  if(pl.H<LowBand*h0)pl.H=std::max(pl.H,low?MinimumBand(h0):(1+HeightTolerance)*h0);
  const V x=Of(NodePoint(s,v));
  if(metricCap)for(unsigned k=0;k<12&&pl.H>LowBand*h0;++k){long double l=0;try{l=GeometryLength(s,Pt(x),Pt(x+(pl.H/pl.dir.s)*pl.dir.d));}catch(const std::exception&){break;}
    if(l<=1.5L)break;pl.H=std::max(LowBand*h0,0.9L*pl.H);}
  pl.delta=pl.H/pl.dir.s;pl.p=x+pl.delta*pl.dir.d;pl.ok=true;return pl;
}
inline long double Deficit(const State& s,const Plan& p){const long double h0=p.h0>0?p.h0:s.h0;return p.dir.ok?std::max(0.L,Need(h0)-p.kernel.delta*p.dir.s):Need(h0);}
inline Node Constrain(const State& s,const PlanarReference* priv,Node node,const std::set<Id>& carriers,const std::set<Id>& bind) {
  if(carriers.empty())return node;
  std::set<Id> comps;for(Id f:carriers)comps.insert(s.component.at(f));
  V p=Of(node.p);bool all=true;for(Id c:comps)all=all&&s.snap.count(c);
  if(s.mapped){if(!priv)throw std::runtime_error("mapped lift without a private context");
    // DC-16: a point on exactly axis-aligned carrier planes is snapped to them before (and, if the binding moved it,
    // after) its binding: a realization off the plane by rounding (1e-18 on the M6 symmetry plane) lies outside the
    // original donors and makes every edge through it unmeasurable
    if(all)node.p=Pt(junction_wp0::Snap(s,comps,p));
    Node bound=priv->BindNewPoint(node,bind.empty()?carriers:bind);
    if(all){const Point q=Pt(junction_wp0::Snap(s,comps,Of(bound.p)));if(!(q.x==bound.p.x&&q.y==bound.p.y&&q.z==bound.p.z)){Node again=node;again.p=q;try{bound=priv->BindNewPoint(again,bind.empty()?carriers:bind);}catch(const std::exception&){}}}
    return bound;}
  if(all){node.p=Pt(junction_wp0::Snap(s,comps,p));return node;}
  for(unsigned pass=0;pass<4;++pass)for(Id f:carriers)node.p=s.reference->Project(f,node.p);
  return node;
}

// ---- L-B' apex LP (as DC-7) and column skins -----------------------------------------------------------------------
struct LP { bool ok=false;long double slack=-INFINITY,l1=0,l2=0,eta=0; };
inline LP ApexLP(const std::vector<std::array<V,3>>& tris,V a,V b,V c,V n,long double h0,long double T,bool fixH) {
  std::vector<std::array<long double,5>> A;
  for(const auto& t:tris){const V N=Cross(t[1]-t[0],t[2]-t[0]);const auto nn=Norm(N);if(!(nn>0))continue;const V Nh=(Sigma()/nn)*N;
    A.push_back({Dot(Nh,a-c)/h0,Dot(Nh,b-c)/h0,Dot(Nh,n),1,-Dot(Nh,c-t[0])/h0});}
  const long double eps=1e-6L;
  A.push_back({-1,0,0,0,-Beta});A.push_back({0,-1,0,0,-Beta});A.push_back({1,1,0,0,1-Beta});
  if(fixH){A.push_back({0,0,-1,0,-1});A.push_back({0,0,1,0,1});}
  else{A.push_back({0,0,-1,0,-(1-T+eps)});A.push_back({0,0,1,0,1+T-eps});}
  A.push_back({0,0,0,1,1});
  LP best;const size_t m=A.size();
  for(size_t i=0;i<m;++i)for(size_t j=i+1;j<m;++j)for(size_t k=j+1;k<m;++k)for(size_t l=k+1;l<m;++l) {
    long double M[4][5];const size_t pick[4]={i,j,k,l};for(int r=0;r<4;++r)for(int q=0;q<5;++q)M[r][q]=A[pick[r]][q];
    bool ok=true;for(int col=0;col<4&&ok;++col){int r=col;for(int t=col+1;t<4;++t)if(std::abs(M[t][col])>std::abs(M[r][col]))r=t;
      if(!(std::abs(M[r][col])>1e-13L)){ok=false;break;}for(int q=0;q<5;++q)std::swap(M[col][q],M[r][q]);
      for(int t=0;t<4;++t)if(t!=col){const auto f=M[t][col]/M[col][col];for(int q=col;q<5;++q)M[t][q]-=f*M[col][q];}}
    if(!ok)continue;long double x[4];for(int r=0;r<4;++r)x[r]=M[r][4]/M[r][r];
    bool feasible=true;for(const auto& row:A){const auto lhs=row[0]*x[0]+row[1]*x[1]+row[2]*x[2]+row[3]*x[3];feasible=feasible&&lhs<=row[4]+1e-11L*(1+std::abs(row[4]));}
    if(feasible&&x[3]>best.slack){best.slack=x[3];best.l1=x[0];best.l2=x[1];best.eta=x[2];}
  }
  best.ok=best.slack>1e-9L;return best;
}
// Lateral triangles over wall edge (x0,x1) with lifted (y0,y1): lower-ID-bottom to higher-ID-top diagonal rule.
inline void Lateral(Id x0,Id x1,Id y0,Id y1,std::vector<Face>& out) {
  if(y0==x0&&y1==x1)return;
  if(x0<x1){if(y1!=x1)out.push_back({x1,x0,y1});if(y0!=x0)out.push_back({x0,y0,y1});}
  else{if(y0!=x0)out.push_back({x1,x0,y0});if(y1!=x1)out.push_back({x1,y0,y1});}
}
struct Skin { std::vector<Face> faces;std::array<std::vector<Face>,3> lateral; };
// Outward column skin of f_out; optional (a) override: on the junction edge {ovV,ovU} the vertex ovV lifts to ovP.
inline Skin ColumnSkin(const Face& f,const std::function<Id(Id)>& L,Id ovV=UINT64_MAX,Id ovU=UINT64_MAX,Id ovP=UINT64_MAX) {
  Skin out;out.faces.push_back(f);bool any=false;for(Id x:f)any=any||L(x)!=x;if(!any)return out;
  out.faces.push_back({L(f[2]),L(f[1]),L(f[0])});
  for(unsigned i=0;i<3;++i){const Id x0=f[i],x1=f[(i+1)%3];Id y0=L(x0),y1=L(x1);
    if(ovV!=UINT64_MAX&&((x0==ovV&&x1==ovU)||(x1==ovV&&x0==ovU))){if(x0==ovV)y0=ovP;else y1=ovP;}
    Lateral(x0,x1,y0,y1,out.lateral[i]);out.faces.insert(out.faces.end(),out.lateral[i].begin(),out.lateral[i].end());}
  return out;
}
// Cone of one column skin (skin[0] = wall face, outward) from an apex of the exact LP; best first child, then column q.
// spread: fixed grid of feet x heights inside the T window when the LP points fail geometric validity.
// D.2 parent/child invariant (Prepare kind and unrebuilt faces): a fresh wall face may not lose an already passing geometric first child.
inline bool FirstChildPasses(State& s,const Patch& p,const SurfaceFace& sf) {
  Patch one;const auto key=Sorted(sf.v);for(const auto& c:p.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)one.cells.push_back(c);}
  if(one.cells.size()!=1)return false;one.surface={sf};one.parameters=p.parameters;std::string why;
  bool ok=false;try{FirstHeightReport h;ok=validityOnlyConstruction?HeightConstruct(one,s.heights,*s.reference,&h,&why):HeightT(s,one,why);}catch(const std::exception&){ok=false;}
  return ok&&Positive(one.cells.front().v);
}
inline bool D2Ok(State& s,const Patch& old,const Patch& fresh,std::string& why) {
  Scope scope("D.2 invariant");
  std::vector<std::pair<Face,bool>> parents;std::set<Face> oldKeys;
  for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker)){parents.push_back({Sorted(sf.v),FirstChildPasses(s,old,sf)});oldKeys.insert(Sorted(sf.v));}
  for(const auto& sf:fresh.surface){if(!s.walls.count(s.reference->Get(sf.facet).marker))continue;const auto key=Sorted(sf.v);
    bool anyPass=false;for(const auto& p:parents){unsigned k=0;for(Id x:p.first)k+=Has(key,x);if(k>=2&&p.second)anyPass=true;}
    if(!anyPass)continue;
    if(!FirstChildPasses(s,fresh,sf)){why="D.2: wall face "+std::to_string(key[0])+","+std::to_string(key[1])+","+std::to_string(key[2])+" loses a passing first child";return false;}}
  return true;
}
inline size_t firstChildDiagnostics=0;
struct ConeOut { bool ok=false;std::string cls,witness;std::vector<Cell> cells;Node apex;double fq=0,fJ=0,colq=0;long double slack=-INFINITY; };
// h0: the prescribed first height of the column's wall face (FaceH0 of skin[0]; a caller passes it for a face not yet in
// the mesh). DC-12 shapePolicy.validity: every cone cell must be valid and the best first child valid (not absolute).
inline ConeOut ConeColumn(State& s,const std::vector<Face>& skin,const std::map<Id,Node>& nodes,Id apexId,bool spread,long double h0Override=-1) {
  Scope scope("ConeColumn");
  ConeOut out;const long double h0=h0Override>0?h0Override:FaceH0(s,skin[0]),T=HeightTolerance;const Face& f=skin[0];
  const V a=Of(nodes.at(f[0]).p),b=Of(nodes.at(f[1]).p),c=Of(nodes.at(f[2]).p);const V n=-1.L*Outward(a,b,c);
  std::vector<std::array<V,3>> tris;for(size_t k=1;k<skin.size();++k)tris.push_back({Of(nodes.at(skin[k][0]).p),Of(nodes.at(skin[k][1]).p),Of(nodes.at(skin[k][2]).p)});
  std::vector<V> cand;
  for(bool fixH:{false,true}){const auto lp=ApexLP(tris,a,b,c,n,h0,T,fixH);if(lp.ok){cand.push_back(c+lp.l1*(a-c)+lp.l2*(b-c)+(lp.eta*h0)*n);out.slack=std::max(out.slack,lp.slack);}}
  if(cand.empty()){out.cls="partial-state";std::ostringstream w;const auto la=Norm(b-c),lb=Norm(c-a),lc=Norm(a-b),area=Norm(Cross(b-a,c-a))/2,r=2*area/(la+lb+lc);
    w<<"A_f empty (exact LP) with "<<skin.size()<<" skin faces; inradius/h0="<<double(r/h0)<<"; free-height LP slack="<<double(ApexLP(tris,a,b,c,n,h0,0.9L,false).slack);
    {const V p0=(1.L/3)*(a+b+c)+h0*n;w<<"; violated at centroid+h0:";for(size_t k=0;k<tris.size();++k){const V N=Cross(tris[k][1]-tris[k][0],tris[k][2]-tris[k][0]);const auto nn=Norm(N);if(!(nn>0))continue;
       const auto d=Sigma()*Dot(N,p0-tris[k][0])/nn/h0;if(d>-1e-9L)w<<" ("<<skin[k+1][0]<<","<<skin[k+1][1]<<","<<skin[k+1][2]<<")="<<double(d);}}
    out.witness=w.str();return out;}
  cand.push_back((1.L/3)*(a+b+c)+h0*n);
  bool found=false;
  auto evaluate=[&](const std::vector<V>& list){
    for(const auto& p:list){const Node apex{apexId,Pt(p)};std::vector<Cell> cone;bool pos=true;
      for(size_t k=0;k<skin.size()&&pos;++k){Cell cell{0,{apex,nodes.at(skin[k][0]),nodes.at(skin[k][1]),nodes.at(skin[k][2])}};
        pos=Positive(cell.v);if(pos)cone.push_back(cell);}
      if(!pos)continue;out.cells=cone;out.apex=apex;found=true;return;}};
  evaluate(cand);
  if(spread||!found) {
    std::vector<V> grid;const std::array<std::array<long double,3>,7> feet{{{1.L/3,1.L/3,1.L/3},{.6L,.2L,.2L},{.2L,.6L,.2L},{.2L,.2L,.6L},{.45L,.45L,.1L},{.45L,.1L,.45L},{.1L,.45L,.45L}}};
    for(long double eta:{.92L,1.L,1.08L})for(const auto& w:feet)grid.push_back(w[0]*a+w[1]*b+w[2]*c+(eta*h0)*n);
    evaluate(grid);
  }
  if(out.cells.empty()){out.cls="partial-state";out.witness="LP apex rounds outside its column";return out;}
  out.ok=true;return out;
}

// ---- (a) carrier sector plan --------------------------------------------------------------------------------------
struct SectorPlan {
  bool ok=true,needed=false;std::string why;Id corner=UINT64_MAX,cornerOpposite[2]={0,0};Edge splitEdge{0,0};
  std::map<Id,Node> cellLift;                    // old star cell -> lift of v used by that cell
  std::vector<std::array<Node,4>> gaps;          // positive gap tetrahedra
  std::map<Face,std::array<Id,3>> junction;      // wall face key -> {v,u,P_1}: lateral override on the junction edge
  std::map<Face,std::vector<Face>> extras;       // wall face key -> extra outward skin faces (rims, lid extras)
  std::map<Face,std::vector<bool>> lidFlags;     // DC-9 (c): extras entry is a lid (third vertex = lift of u)
  std::map<Face,int> carrierFaces;               // new carrier faces (sorted) -> marker
  std::vector<Node> points;std::set<Id> seamPoints;
  Edge gapEdge{0,0};  // DC-9 (c): link edge (a,u) of a gap tetrahedron that falls on the column side of its lid face
};
inline SectorPlan PlanSectors(State& s,const Patch& old,const PlanarReference& ref,Id v,const Node& vstar,const Dir& dir,long double H,Id& next,
                       const std::map<Id,Node>& nodes,const std::function<Id(Id)>& L) {
  SectorPlan sp;
  struct FaceInfo { Face out;Id facet=0,chart=0,owner=0;int marker=0;bool wall=false; };
  std::vector<FaceInfo> carriers,walls;
  std::map<Id,const Cell*> star;for(const auto& c:old.cells)if(CellHas(c,v))star[c.id]=&c;
  auto owner=[&](const Face& f)->Id{for(const auto& r:star){unsigned k=0;for(auto n:r.second->v)k+=Has(f,n.id);if(k==3)return r.first;}return UINT64_MAX;};
  for(const auto& sf:old.surface)if(Has(sf.v,v)){FaceInfo fi;fi.out=sf.v;fi.facet=sf.facet;fi.chart=ref.Canonical(sf.facet);fi.marker=s.reference->Get(sf.facet).marker;
    fi.wall=s.walls.count(fi.marker);fi.owner=owner(sf.v);if(fi.owner==UINT64_MAX){sp.ok=false;sp.why="sector: incomplete star";return sp;}(fi.wall?walls:carriers).push_back(fi);}
  if(carriers.empty())return sp;
  std::set<Id> charts;for(const auto& c:carriers)charts.insert(c.chart);if(charts.size()<2)return sp;
  // fan order from one junction to the other
  std::map<Id,std::vector<size_t>> byEdge;for(size_t i=0;i<carriers.size();++i)for(Id x:carriers[i].out)if(x!=v)byEdge[x].push_back(i);
  std::set<Id> wallNbr;for(const auto& w:walls)for(Id x:w.out)if(x!=v)wallNbr.insert(x);
  std::vector<Id> ends;for(const auto& r:byEdge)if(r.second.size()==1&&wallNbr.count(r.first))ends.push_back(r.first);
  if(ends.size()!=2){sp.ok=false;sp.why="sector: carrier fan at v is not one junction-to-junction fan ("+std::to_string(ends.size())+" junctions)";return sp;}
  std::vector<size_t> fan;std::vector<Id> spokes{ends[0]};{Id w=ends[0];size_t prev=SIZE_MAX;
    for(size_t step=0;step<carriers.size();++step){size_t f=SIZE_MAX;for(size_t i:byEdge[w])if(i!=prev)f=i;if(f==SIZE_MAX)break;fan.push_back(f);prev=f;
      Id nx=UINT64_MAX;for(Id x:carriers[f].out)if(x!=v&&x!=w)nx=x;w=nx;spokes.push_back(w);if(w==ends[1])break;}}
  if(fan.size()!=carriers.size()||spokes.back()!=ends[1]){sp.ok=false;sp.why="sector: carrier fan walk incomplete";return sp;}
  // sectors = maximal chart runs; spokes[k] separates fan[k-1] and fan[k]
  std::vector<std::pair<size_t,size_t>> runs;for(size_t k=0;k<fan.size();++k){if(k==0||carriers[fan[k]].chart!=carriers[fan[k-1]].chart)runs.push_back({k,k});runs.back().second=k;}
  std::vector<bool> main(runs.size(),false);
  for(size_t r=0;r<runs.size();++r){bool in=false;try{in=ref.Contains(carriers[fan[runs[r].first]].facet,vstar.p);}catch(const std::exception&){in=false;}main[r]=in;}
  size_t lo=runs.size(),hi=0;for(size_t r=0;r<runs.size();++r)if(main[r]){lo=std::min(lo,r);hi=std::max(hi,r);}
  if(lo==runs.size()){sp.ok=false;sp.why="sector: the lift point lies in no carrier chart at v";return sp;}
  for(size_t r=lo;r<=hi;++r)if(!main[r]){sp.ok=false;sp.why="sector: main charts not contiguous";return sp;}
  if(lo==0&&hi+1==runs.size())return sp;
  sp.needed=true;
  // link of v: cell adjacency through faces (v,a,b); boundary faces are physical
  std::map<std::pair<Id,Id>,std::vector<Id>> link;
  for(const auto& r:star){std::vector<Id> o;for(auto n:r.second->v)if(n.id!=v)o.push_back(n.id);
    for(int i=0;i<3;++i)for(int j=i+1;j<3;++j)link[std::minmax(o[i],o[j])].push_back(r.first);}
  std::map<Id,std::set<Id>> graph;std::set<Id> boundaryVertex;
  for(const auto& e:link){if(e.second.size()==2){graph[e.first.first].insert(e.first.second);graph[e.first.second].insert(e.first.first);}
    else{boundaryVertex.insert(e.first.first);boundaryVertex.insert(e.first.second);}}
  std::set<Id> wallOwners;for(const auto& w:walls)wallOwners.insert(w.owner);
  auto chartPoint=[&](Id w,const FaceInfo& left,const FaceInfo& right,Node& P)->bool{
    const V x0=Of(nodes.at(v).p),xw=Of(nodes.at(w).p);const V e=Unit(xw-x0);V nbar{};for(const auto& n:dir.normals)nbar=nbar+n;nbar=Unit(nbar);
    const auto c=Dot(nbar,e);if(c<.25L){sp.why="sector: seam "+std::to_string(v)+"-"+std::to_string(w)+" too tangential (n.e="+std::to_string(double(c))+")";return false;}
    long double dist=H/c;if(dist>.45L*Norm(xw-x0)){dist=.45L*Norm(xw-x0);if(dist*c<MinimumBand(VertexH0(s,v))){sp.why="sector: seam edge shorter than the band";return false;}}
    P=Node{next++,Pt(x0+dist*e)};
    try{P=Constrain(s,&ref,P,{left.facet,right.facet},{left.facet,right.facet});}catch(const std::exception& x){sp.why=std::string("sector: seam binding: ")+x.what();return false;}
    return true;};
  const size_t lo_=lo,hi_=hi;
  for(int side=0;side<2;++side) {
    // foreign runs from the junction toward the main run
    std::vector<size_t> foreign;if(side==0){for(size_t r=0;r<lo;++r)foreign.push_back(r);}else{for(size_t r=runs.size();r-->hi+1;)foreign.push_back(r);}
    if(foreign.empty())continue;
    const Id u=side==0?ends[0]:ends[1];const size_t m=foreign.size();
    std::vector<Node> P(m);std::vector<Id> seam(m);std::vector<int> seamMarker(m);
    for(size_t j=0;j<m;++j){const size_t r=foreign[j];const size_t k=side==0?runs[r].second+1:runs[r].first;   // spoke index between run r and its main-ward neighbour
      seam[j]=spokes[k];const auto& left=carriers[fan[k-1]];const auto& right=carriers[fan[k]];
      if(!chartPoint(seam[j],left,right,P[j])){sp.ok=false;return sp;}
      const auto& toward=carriers[fan[side==0?k:k-1]];seamMarker[j]=toward.marker;sp.points.push_back(P[j]);sp.seamPoints.insert(P[j].id);}
    // nested cut paths: shortest link path seam[j] -> u through interior link vertices; side A_j flooded from sigma_1
    std::vector<std::set<Id>> A(m);std::vector<Id> lastA(m,UINT64_MAX);
    // a corner cell owning the junction wall face and the first foreign carrier face admits no cut: separate it first
    {Id wallOwner=UINT64_MAX;for(const auto& w:walls)if(Has(w.out,u))wallOwner=w.owner;
     const auto& first=carriers[fan[side==0?0:fan.size()-1]];
     if(wallOwner!=UINT64_MAX&&wallOwner==first.owner){sp.ok=false;sp.corner=wallOwner;std::vector<Id> o;for(auto n:star.at(wallOwner)->v)if(n.id!=v&&n.id!=u)o.push_back(n.id);
       if(o.size()==2){sp.cornerOpposite[0]=o[0];sp.cornerOpposite[1]=o[1];}sp.why="sector-corner: cell "+std::to_string(wallOwner)+" owns the junction wall face and the foreign carrier face";return sp;}}
    // DC-9 (c) iteration 2: the junction gap tetrahedron (P_{j+1},P_j,Ln(a),Ln(u)) must lie on the far side of the
    // corner column's lid face; the cut path may enter u only from a link vertex a satisfying it (same lid
    // orientation rule as below). A path without such a vertex falls back to the existing witnesses.
    auto lidSideOk=[&](size_t j,Id a)->bool{
      const Node hi=j+1<m?P[j+1]:vstar,lo=P[j];const size_t r=j+1<m?foreign[j+1]:(side==0?lo_:hi_);const auto& like=carriers[fan[runs[r].first]];
      const V o=Outward(Of(nodes.at(like.out[0]).p),Of(nodes.at(like.out[1]).p),Of(nodes.at(like.out[2]).p));
      Face rim{v,lo.id,hi.id};if(Dot(Outward(Of(nodes.at(v).p),Of(lo.p),Of(hi.p)),o)<0)std::swap(rim[1],rim[2]);
      bool loHi=false;for(int i=0;i<3;++i)loHi=loHi||(rim[i]==lo.id&&rim[(i+1)%3]==hi.id);
      const std::array<Node,4> t{nodes.at(L(a)),loHi?hi:lo,loHi?lo:hi,nodes.at(L(u))};return !Positive(t);};
    for(size_t j=0;j<m;++j) {
      std::map<Id,Id> parent;std::deque<Id> queue{seam[j]};parent[seam[j]]=seam[j];
      while(!queue.empty()&&!parent.count(u)){const Id x=queue.front();queue.pop_front();for(Id y:graph[x]){if(parent.count(y))continue;if(y!=u&&boundaryVertex.count(y))continue;
          if(y==u&&!lidSideOk(j,x))continue;parent[y]=x;queue.push_back(y);}}
      if(!parent.count(u)) {
        // relaxed path through boundary link vertices: the first boundary vertex z on it names the cell to split
        std::map<Id,Id> rp;std::deque<Id> rq{seam[j]};rp[seam[j]]=seam[j];
        while(!rq.empty()&&!rp.count(u)){const Id x=rq.front();rq.pop_front();for(Id y:graph[x])if(!rp.count(y)){rp[y]=x;rq.push_back(y);}}
        if(rp.count(u)){std::vector<Id> path;for(Id x=u;;x=rp.at(x)){path.push_back(x);if(x==seam[j])break;}std::reverse(path.begin(),path.end());
          // split the edge from the boundary vertex z to an inner seam w_i (i<j) it borders, else to its path predecessor
          for(size_t i=1;i+1<path.size();++i)if(boundaryVertex.count(path[i])){const Id z=path[i];Id y=path[i-1];
            for(size_t k=0;k<j;++k)if(graph[z].count(seam[k]))y=seam[k];
            sp.ok=false;sp.splitEdge={std::min(y,z),std::max(y,z)};sp.why="sector-link: link path from seam "+std::to_string(seam[j])+" to junction "+std::to_string(u)+" passes boundary vertex "+std::to_string(z)+"; split edge "+std::to_string(y)+"-"+std::to_string(z);return sp;}}
        sp.ok=false;std::ostringstream w;w<<"sector: no interior link path from seam "<<seam[j]<<" to junction "<<u<<"; star:";
        for(const auto& r:star){w<<" [";for(auto n:r.second->v)w<<n.id<<' ';w<<"]";}w<<" boundary faces:";for(const auto& c:carriers)w<<" c"<<c.chart<<"("<<c.out[0]<<','<<c.out[1]<<','<<c.out[2]<<")";
        for(const auto& c:walls)w<<" w("<<c.out[0]<<','<<c.out[1]<<','<<c.out[2]<<")";sp.why=w.str();return sp;}
      std::set<std::pair<Id,Id>> cut;for(Id x=u;x!=seam[j];x=parent.at(x))cut.insert(std::minmax(x,parent.at(x)));
      const Id seed=carriers[fan[runs[foreign[0]].first]].owner;std::deque<Id> q{seed};A[j].insert(seed);
      while(!q.empty()){const Id c=q.front();q.pop_front();std::vector<Id> o;for(auto n:star.at(c)->v)if(n.id!=v)o.push_back(n.id);
        for(int i=0;i<3;++i)for(int k=i+1;k<3;++k){const auto key=std::minmax(o[i],o[k]);if(cut.count(key))continue;for(Id d:link[key])if(A[j].insert(d).second)q.push_back(d);}}
      for(size_t i=0;i<=j;++i)for(size_t k=runs[foreign[i]].first;k<=runs[foreign[i]].second;++k)if(!A[j].count(carriers[fan[k]].owner)){sp.ok=false;sp.why="sector: cut leaves a sector owner outside";return sp;}
      for(Id c:A[j])if(wallOwners.count(c)){sp.ok=false;sp.why="sector-corner: cell "+std::to_string(c)+" owns a wall face and a foreign carrier face";sp.corner=c;
          const auto* cell=star.at(c);std::vector<Id> o;for(auto n:cell->v)if(n.id!=v&&n.id!=u)o.push_back(n.id);if(o.size()==2){sp.cornerOpposite[0]=o[0];sp.cornerOpposite[1]=o[1];}return sp;}
      for(size_t k=0;k<fan.size();++k){bool mine=false;for(size_t i=0;i<=j;++i)mine=mine||(k>=runs[foreign[i]].first&&k<=runs[foreign[i]].second);
        if(!mine&&A[j].count(carriers[fan[k]].owner)){sp.ok=false;sp.why="sector: cut captures another sector";return sp;}}
      if(j&&!std::includes(A[j].begin(),A[j].end(),A[j-1].begin(),A[j-1].end())){sp.ok=false;sp.why="sector: cut paths not nested";return sp;}
      // the cut face at the junction: (v,a,u)
      for(const auto& e:cut)if(e.first==u||e.second==u)lastA[j]=e.first==u?e.second:e.first;
    }
    // lift index per cell and gap tetrahedra (P_{m} -> v*)
    auto index=[&](Id c){for(size_t j=0;j<m;++j)if(A[j].count(c))return j;return m;};
    std::vector<Node> lifts(P);lifts.push_back(vstar);
    for(const auto& r:star){const size_t j=index(r.first);if(j<m)sp.cellLift[r.first]=P[j];}
    auto Ln=[&](Id x){return nodes.at(L(x));};
    for(const auto& e:link){if(e.second.size()!=2)continue;size_t i1=index(e.second[0]),i2=index(e.second[1]);if(i1==i2)continue;if(i1>i2)std::swap(i1,i2);
      for(size_t j=i1;j<i2;++j){std::array<Node,4> t{lifts[j+1],lifts[j],Ln(e.first.first),Ln(e.first.second)};if(!Positive(t))std::swap(t[2],t[3]);
        if(!Positive(t)){sp.ok=false;sp.why="sector: degenerate gap tetrahedron";return sp;}sp.gaps.push_back(t);}}
    // corner column: wall face on the junction (v,u)
    Face fc{};bool found=false;for(const auto& w:walls)if(Has(w.out,u)){fc=w.out;found=true;}
    if(!found){sp.ok=false;sp.why="sector: no wall face on the junction";return sp;}
    const Face key=Sorted(fc);sp.junction[key]={v,u,P[0].id};
    auto& extra=sp.extras[key];
    auto orientCarrier=[&](Face f,const FaceInfo& like)->Face{const V o=Outward(Of(nodes.at(like.out[0]).p),Of(nodes.at(like.out[1]).p),Of(nodes.at(like.out[2]).p));
      auto pos=[&](Id x)->V{for(const auto& p:sp.points)if(p.id==x)return Of(p.p);if(x==vstar.id)return Of(vstar.p);return Of(nodes.at(x).p);};
      if(Dot(Outward(pos(f[0]),pos(f[1]),pos(f[2])),o)<0)std::swap(f[1],f[2]);return f;};
    for(size_t j=0;j<m;++j){const Id hiP=lifts[j+1].id,loP=lifts[j].id;
      const size_t r=j+1<m?foreign[j+1]:(side==0?lo:hi);const auto& like=carriers[fan[runs[r].first]];
      const Face rim=orientCarrier({v,loP,hiP},like);extra.push_back(rim);sp.lidFlags[key].push_back(false);sp.carrierFaces[Sorted(rim)]=like.marker;
      const Face gap=orientCarrier({hiP,loP,seam[j]},like);sp.carrierFaces[Sorted(gap)]=like.marker;
      if(lastA[j]==UINT64_MAX){sp.ok=false;sp.why="sector: cut path misses the junction";return sp;}
      // lid extra: closes the column skin over the rim's top edge, traversed opposite to the rim
      bool loHi=false;for(int i=0;i<3;++i)loHi=loHi||(rim[i]==loP&&rim[(i+1)%3]==hiP);
      const Face lid=loHi?Face{hiP,loP,L(u)}:Face{loP,hiP,L(u)};
      {std::array<Node,4> t{Ln(lastA[j]),Node{lid[0],lid[0]==hiP?lifts[j+1].p:lifts[j].p},Node{lid[1],lid[1]==hiP?lifts[j+1].p:lifts[j].p},Ln(u)};
       if(Positive(t)){sp.ok=false;std::ostringstream w;V nbar{};for(const auto& n:dir.normals)nbar=nbar+n;nbar=Unit(nbar);const V x0=Of(nodes.at(v).p);
         auto h=[&](const Node& n){return double(Dot(Of(n.p)-x0,nbar)/s.h0);};
         w<<"sector: gap tetrahedron on the column side of its lid face (j="<<j<<"/"<<m<<", side "<<side<<", a="<<lastA[j]<<" L(a)="<<L(lastA[j])<<" u="<<u<<" L(u)="<<L(u)
          <<"; heights/h0: Ln(a)="<<h(Ln(lastA[j]))<<" lid=("<<h(t[1])<<","<<h(t[2])<<") Ln(u)="<<h(Ln(u))<<"; seam "<<seam[j]<<")";sp.why=w.str();sp.gapEdge={std::min(lastA[j],u),std::max(lastA[j],u)};return sp;}}
      extra.push_back(lid);sp.lidFlags[key].push_back(true);(void)seamMarker;}
  }
  return sp;
}

// D.1 admission (layer2d::Admit, decision D) with exact reuse of already integrated composed edges: the old cavity's
// edges and unchanged fresh edges come from the harness length cache (same frozen field identity, same endpoint
// coordinates, MetricEdgeRequests rule); newly integrated edges are stored back. Results are identical.
inline std::map<Edge,EdgeMeasure> ReuseOf(State& s,const std::vector<Cell>& a,const std::vector<Cell>* b=nullptr) {
  std::map<Edge,EdgeMeasure> out;const auto& id=s.field->Identity();if(!id)return out;
  auto add=[&](const std::vector<Cell>& cells){for(const auto& c:cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){Node x=c.v[i],y=c.v[j];if(x.id>y.id)std::swap(x,y);
    const auto f=s.lengths.find({x.id,y.id});if(f!=s.lengths.end()&&f->second.second>0&&SamePoint(f->second.first[0],x.p)&&SamePoint(f->second.first[1],y.p)){
      EdgeMeasure m;m.a=x.p;m.b=y.p;m.length=f->second.second;m.identity=id;out.insert_or_assign(Edge{x.id,y.id},m);++perf.length_hits;}}};
  add(a);if(b)add(*b);return out;
}
inline void StoreLengths(State& s,const PatchMetric& m){for(const auto& e:m.edges)if(e.second.length>0)s.lengths[{e.first[0],e.first[1]}]={{e.second.a,e.second.b},e.second.length};}
// LayerConstruct size certificate (D.1): changed-edge excess of the fresh patch must not exceed the removed excess.
// Changed edges only (same identities and coordinates cancel); composed lengths through the harness cache. An
// unmeasurable added edge rejects; an unmeasurable removed edge counts as removed excess (a defect leaves the mesh).
// DC-9 (b): composed lengths are a function of the ordered endpoint coordinates under the frozen field of the case;
// a coordinate-keyed memo reuses them across new node identities (retried transactions, pass 2, rebuilt columns).
// (geometryLengths: defined with the other caches above)
inline size_t geometryHits=0;
inline long double GeometryLength(State& s,Point pa,Point pb) {
  const std::array<double,6> key{pa.x,pa.y,pa.z,pb.x,pb.y,pb.z};
  if(cachesOn){const auto g=geometryLengths.find(key);if(g!=geometryLengths.end()){++geometryHits;if(g->second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return g->second;}}
  try{const auto l=s.field->TargetEdgeLength(pa,pb);if(cachesOn)geometryLengths.emplace(key,l);return l;}
  catch(const std::exception&){if(cachesOn)geometryLengths.emplace(key,-1.L);throw;}
}
inline long double LenAt(State& s,Id a,Point pa,Id b,Point pb) {
  if(a>b){std::swap(a,b);std::swap(pa,pb);}
  const auto f=s.lengths.find({a,b});
  if(f!=s.lengths.end()&&SamePoint(f->second.first[0],pa)&&SamePoint(f->second.first[1],pb)){++perf.length_hits;if(f->second.second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return f->second.second;}
  const std::array<double,6> key{pa.x,pa.y,pa.z,pb.x,pb.y,pb.z};
  const auto g=cachesOn?geometryLengths.find(key):geometryLengths.end();  // never in the fresh re-audit
  if(g!=geometryLengths.end()){++geometryHits;s.lengths[{a,b}]={{pa,pb},g->second};if(g->second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return g->second;}
  ++perf.length_misses;
  try{const auto l=s.field->TargetEdgeLength(pa,pb);s.lengths[{a,b}]={{pa,pb},l};if(cachesOn)geometryLengths.emplace(key,l);return l;}
  catch(const std::exception&){++perf.length_failures;s.lengths[{a,b}]={{pa,pb},-1.L};if(cachesOn)geometryLengths.emplace(key,-1.L);throw;}
}
inline bool SizeCertificate(State& s,const Patch& old,const Patch& fresh,long double& removed,long double& added,std::string& why) {
  Scope scope("Excess certificate");
  auto edges=[](const std::vector<Cell>& cells){std::map<Edge,std::array<Point,2>> out;for(const auto& c:cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){Node x=c.v[i],y=c.v[j];if(x.id>y.id)std::swap(x,y);out[{x.id,y.id}]={x.p,y.p};}return out;};
  const auto before=edges(old.cells),after=edges(fresh.cells);
  auto same=[](const std::map<Edge,std::array<Point,2>>& m,const std::pair<const Edge,std::array<Point,2>>& e){const auto o=m.find(e.first);return o!=m.end()&&SamePoint(o->second[0],e.second[0])&&SamePoint(o->second[1],e.second[1]);};
  // DC-9 (b): the decision is added <= removed. Added excess first (every changed fresh edge stays in the mesh and is
  // needed later anyway); removed edges leave the mesh, so they are integrated only while they can decide: none when
  // added == 0, and the removed partial sum (nonnegative terms, same order) stops once it reaches `added`. A rejection
  // integrates every removed edge, so its witness values equal the DC-8 ones.
  removed=added=0;
  for(const auto& e:after)if(!same(before,e)){try{added+=layer2d::ExcessOf(LenAt(s,e.first[0],e.second[0],e.first[1],e.second[1]));}catch(const std::exception& x){why=std::string("adds an unmeasurable edge: ")+x.what();return false;}}
  if(added==0)return true;
  for(const auto& e:before)if(!same(after,e)){try{removed+=layer2d::ExcessOf(LenAt(s,e.first[0],e.second[0],e.first[1],e.second[1]));}catch(const std::exception&){removed+=1e30L;}
    if(removed>=added)break;}
  return true;
}
// ---- DC-11 split-acceptance policy (SPLIT_RULE_PLAN). rule 0: the DC-9/DC-10 certificates, unchanged (default).
// rule 1: the 2D split rule (CNativeMesh2D.hpp SplitPatch sizeProgress): a refinement that removes no vertex needs no
// changed-edge excess progress when the requested split edge (metric length `requested`; < 0 = unknown -> the longest
// removed changed edge) exceeds 1.6, otherwise it may add no oversized changed edge; one that removes a vertex keeps
// the strict decrease; a LayerConstruct that removes no vertex needs no size certificate (2D HEIGHT has none), one that
// does keeps added <= removed. `relocated`: column apices rebuilt by the operation (a move, not a removal).
struct SplitPolicy {
  int rule=0;const std::set<Id>* relocated=nullptr;size_t relocatedCount=0;long double requested=-1;
  size_t size_insertions=0,vertex_removal_checks=0,vertex_removal_refusals=0,nonsize_checks=0,nonsize_refusals=0,layer_free=0,layer_checked=0;
};
inline SplitPolicy splitPolicy;
// DC-14: the former SU2_DC9_SECTOR_FIRST switch (option of the DC-9..11 harnesses; never set in production).
inline bool sectorFirst=false;
// A removal: an old vertex (not a relocated apex) absent from the fresh cells, or no net vertex gain (the production
// criterion, which cannot see relocations).
// `certified`: further one-for-one relocations certified by the caller (production refit identities).
inline bool RemovesVertex(const std::vector<Cell>& old,const std::vector<Cell>& fresh,const std::set<Id>* relocated,size_t certified=0) {
  std::set<Id> now,before;for(const auto& c:fresh)for(auto n:c.v)now.insert(n.id);size_t gone=0;
  for(const auto& c:old)for(auto n:c.v)if(before.insert(n.id).second&&!now.count(n.id)&&!(relocated&&relocated->count(n.id)))++gone;
  return gone>certified||now.size()<before.size()+1;
}
// Edges of the fresh cells not on their boundary skin (2D: interior edges, perimeter excluded).
inline std::set<Edge> InteriorEdges(const std::vector<Cell>& cells) {
  std::set<Edge> skin,all;for(const auto& f:Incidence(cells).Boundary())for(int i=0;i<3;++i)skin.insert({std::min(f[i],f[(i+1)%3]),std::max(f[i],f[(i+1)%3])});
  for(const auto& c:cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){const Edge e{std::min(c.v[i].id,c.v[j].id),std::max(c.v[i].id,c.v[j].id)};if(!skin.count(e))all.insert(e);}
  return all;
}
// Every changed fresh edge must be measurable (the certificates' measurement validity, kept when the excess
// comparison is exempted). Returns false with the reason on an unmeasurable changed edge.
inline bool MeasurableChanged(State& s,const Patch& old,const Patch& fresh,std::string& why) {
  long double removed=0,added=0;std::string w;
  // SizeCertificate integrates every changed fresh edge first and fails only on an unmeasurable one
  if(!SizeCertificate(s,old,fresh,removed,added,w)){why=w;return false;}return true;
}
// ---- R2 closed row transaction with sector lifts and band-height window --------------------------------------------
struct Tx {
  bool ok=false;std::string cls,witness;std::vector<Face> failing;std::vector<Id> ids;Patch old,fresh;
  std::map<Id,Id> lifts;std::map<Face,Id> apices;std::set<Id> seam,blockers;std::set<Face> rebuilt;size_t old_cells=0,fresh_cells=0,gaps=0,sectors=0,stored_sectors=0;
  std::map<Face,std::array<Id,3>> overrides;std::map<Face,std::vector<std::pair<Face,bool>>> extras;  // DC-9 (c)
  double first_q=1,first_J=1,height_error=0;long double min_slack=INFINITY;Id corner=UINT64_MAX,sectorVertex=UINT64_MAX;Edge cornerEdge{0,0},splitEdge{0,0};
};
inline Tx Transaction(State& s,const Rows& rows,const std::set<Id>& V0,bool low) {
  Scope scope("Transaction");
  Tx tx;const auto apicesAll=rows.Apices();
  std::set<Id> cavity;for(Id v:V0){const auto st=StarOf(s,v);cavity.insert(st.begin(),st.end());}
  std::map<Id,Face> apexFace;for(const auto& r:rows.apex)apexFace[r.second]=r.first;
  for(bool grew=true;grew;){grew=false;std::set<Id> add;
    for(Id id:cavity)for(auto n:s.mesh.at(id).cell.v){const auto a=apexFace.find(n.id);if(a!=apexFace.end()){tx.rebuilt.insert(a->second);for(Id x:StarOf(s,n.id))if(!cavity.count(x))add.insert(x);}}
    if(!add.empty()){cavity.insert(add.begin(),add.end());grew=true;}
    if(cavity.size()>MaximumCavityCells){tx.cls="cap";tx.witness="cap-bound closure: "+std::to_string(cavity.size())+" old cells";tx.old_cells=cavity.size();return tx;}}
  for(const auto& f:PhysicalOf(s,cavity,true))for(Id v:V0)if(Has(f.key,v))tx.rebuilt.insert(f.key);
  tx.ids.assign(cavity.begin(),cavity.end());tx.old_cells=cavity.size();
  try{tx.old=PatchOf(s,tx.ids);}catch(const std::exception& e){tx.cls="patch";tx.witness=e.what();return tx;}
  auto priv=s.reference->PrivateParameters(tx.old);const PlanarReference& ref=priv?*priv:*s.reference;
  std::map<Id,Node> nodes;for(const auto& c:tx.old.cells)for(auto n:c.v)nodes[n.id]=n;
  Id next=s.next_node;std::map<Id,Id> L=rows.lift;std::map<Id,Plan> plans;
  for(Id v:V0) {
    const auto pl=PlanLift(s,apicesAll,v,true,low);if(!pl.ok){tx.cls=pl.why.substr(0,pl.why.find(':'));tx.witness="vertex "+std::to_string(v)+": "+pl.why;return tx;}
    Node node{next++,Pt(pl.p)};
    try{node=Constrain(s,priv.get(),node,pl.dir.carriers,pl.dir.bind);}catch(const std::exception& e){tx.cls="carrier";tx.witness="vertex "+std::to_string(v)+" binding: "+e.what();return tx;}
    nodes[node.id]=node;L[v]=node.id;tx.lifts[v]=node.id;plans[v]=pl;if(pl.dir.rank==2)tx.seam.insert(node.id);}
  auto Lf=[&](Id x){const auto f=L.find(x);return f==L.end()?x:f->second;};
  // (a) sector plans
  std::map<Id,SectorPlan> sectors;std::map<Id,Id> unlift;for(const auto& r:tx.lifts)unlift[r.second]=r.first;
  for(Id v:V0) {
    auto sp=PlanSectors(s,tx.old,ref,v,nodes.at(L.at(v)),plans.at(v).dir,plans.at(v).H,next,nodes,Lf);
    if(!sp.ok){tx.cls=sp.why.substr(0,sp.why.find(':'));tx.witness="vertex "+std::to_string(v)+": "+sp.why;
      tx.splitEdge=sp.splitEdge;tx.sectorVertex=v;if(sp.corner!=UINT64_MAX){tx.corner=sp.corner;tx.cornerEdge={std::min(sp.cornerOpposite[0],sp.cornerOpposite[1]),std::max(sp.cornerOpposite[0],sp.cornerOpposite[1])};}return tx;}
    if(!sp.needed)continue;++tx.sectors;
    for(const auto& p:sp.points){nodes[p.id]=p;unlift[p.id]=v;tx.seam.insert(p.id);}
    sectors.emplace(v,std::move(sp));}
  for(const auto& sp:sectors)for(Id v2:V0)if(v2!=sp.first)for(const auto& g:sp.second.gaps)for(auto n:g)if(n.id==L.at(v2)&&sectors.count(v2)){tx.cls="sector";tx.witness="adjacent multi-sector lifts";return tx;}
  // moved cells (direct final orientation), per-cell sector lift of v
  std::set<Id> oldApices;for(const auto& f:tx.rebuilt){const auto a=rows.apex.find(f);if(a!=rows.apex.end())oldApices.insert(a->second);}
  std::vector<Cell> fresh;
  for(const auto& c:tx.old.cells){bool col=false;for(auto n:c.v)col=col||oldApices.count(n.id);if(col)continue;
    Cell m=c;for(auto& n:m.v){const auto f=tx.lifts.find(n.id);if(f==tx.lifts.end())continue;const auto sp=sectors.find(n.id);
      if(sp!=sectors.end()){const auto cl=sp->second.cellLift.find(c.id);if(cl!=sp->second.cellLift.end()){n=cl->second;continue;}}n=nodes.at(f->second);}
    if(!Positive(m.v)){tx.cls="clearance";std::ostringstream w;w<<"moved cell "<<c.id<<" inverts; |V|="<<V0.size()<<(low?" low band":"")<<" moved in cell:";
      for(auto x:c.v)if(tx.lifts.count(x.id)){const auto& pl=plans.at(x.id);w<<" v"<<x.id<<"(delta/deltaK="<<double(pl.delta/pl.kernel.delta)<<",H/h0="<<double(pl.H/s.h0)<<",rank "<<pl.dir.rank<<",s*="<<double(pl.dir.s)<<")";}
      const auto wall=WallVertices(s);
      for(auto x:c.v)if(wall.count(x.id)&&!rows.lift.count(x.id)&&!V0.count(x.id))tx.blockers.insert(x.id);
      tx.witness=w.str();return tx;}
    fresh.push_back(m);}
  for(const auto& sp:sectors)for(const auto& g:sp.second.gaps){fresh.push_back(Cell{0,g});++tx.gaps;}
  // columns of every rebuilt face
  std::map<Face,Face> outFace;for(const auto& f:PhysicalOf(s,cavity,true))outFace[f.key]=f.out;
  std::map<Face,int> lateralMarker;
  auto carrierMarker=[&](Id x0,Id x1)->int{for(const auto& sf:tx.old.surface){const int m=s.reference->Get(sf.facet).marker;if(s.walls.count(m))continue;if(Has(sf.v,x0)&&Has(sf.v,x1))return m;}return -1;};
  for(const auto& sp:sectors)for(const auto& r:sp.second.carrierFaces)lateralMarker[r.first]=r.second;
  for(const auto& key:tx.rebuilt) {
    const Face f=outFace.at(key);Skin skin;Id ovV=UINT64_MAX,ovU=UINT64_MAX,ovP=UINT64_MAX;const std::vector<Face>* extra=nullptr;std::vector<Face> storedExtras;
    for(const auto& sp:sectors){const auto j=sp.second.junction.find(key);if(j!=sp.second.junction.end()){ovV=j->second[0];ovU=j->second[1];ovP=j->second[2];extra=&sp.second.extras.at(key);
      tx.overrides[key]=j->second;auto& stored=tx.extras[key];const auto& flags=sp.second.lidFlags.at(key);
      for(size_t i=0;i<extra->size();++i){Face e=(*extra)[i];if(flags[i])e[2]=ovU;stored.push_back({e,flags[i]});}}}
    // DC-9 (c): a column built by an earlier sector transaction keeps its override and extras (lid realized at L(u))
    if(ovV==UINT64_MAX){const auto o=rows.overrides.find(key);if(o!=rows.overrides.end()){ovV=o->second[0];ovU=o->second[1];ovP=o->second[2];
      for(const auto& e:rows.extras.at(key)){Face x=e.first;if(e.second)x[2]=Lf(x[2]);storedExtras.push_back(x);}extra=&storedExtras;
      tx.overrides[key]=o->second;tx.extras[key]=rows.extras.at(key);++tx.stored_sectors;}}
    skin=ColumnSkin(f,Lf,ovV,ovU,ovP);if(extra)skin.faces.insert(skin.faces.end(),extra->begin(),extra->end());
    for(unsigned i=0;i<3;++i){const int m=carrierMarker(f[i],f[(i+1)%3]);if(m>=0)for(const auto& t:skin.lateral[i])lateralMarker[Sorted(t)]=m;}
    {std::map<std::pair<Id,Id>,int> dir;for(const auto& t:skin.faces)for(int i=0;i<3;++i)++dir[{t[i],t[(i+1)%3]}];bool closed=true;std::ostringstream w;
     for(const auto& d:dir)if(dir.count({d.first.second,d.first.first})!=1||d.second!=1){closed=false;w<<" ("<<d.first.first<<"->"<<d.first.second<<")";}
     if(!closed){tx.failing.push_back(key);if(tx.cls.empty()){tx.cls="skin";std::ostringstream z;z<<"face "<<key[0]<<","<<key[1]<<","<<key[2]<<": column skin not closed:"<<w.str()<<"; faces:";
        for(const auto& t:skin.faces)z<<" ("<<t[0]<<","<<t[1]<<","<<t[2]<<")";tx.witness=z.str();}continue;}}
    auto cone=ConeColumn(s,skin.faces,nodes,next,false);
    if(!cone.ok){tx.failing.push_back(key);if(tx.cls.empty()){tx.cls=cone.cls;std::ostringstream w;w<<"face "<<key[0]<<","<<key[1]<<","<<key[2]<<(low?" (low band)":"")<<": "<<cone.witness<<"; lifted:";
        const V a=Of(nodes.at(f[0]).p),b=Of(nodes.at(f[1]).p),c=Of(nodes.at(f[2]).p),n=-1.L*Outward(a,b,c);const auto la=Norm(b-c),lb=Norm(c-a),lc=Norm(a-b),r=Norm(Cross(b-a,c-a))/(la+lb+lc);
        for(Id x:f)if(Lf(x)!=x){const V q=Of(nodes.at(Lf(x)).p)-Of(nodes.at(x).p);const auto hn=Dot(q,n);w<<" v"<<x<<"(H_n/h0="<<double(hn/s.h0)<<",tangential/inradius="<<double(Norm(q-hn*n)/r)<<")";}
        tx.witness=w.str();}continue;}
    tx.first_q=std::min(tx.first_q,cone.fq);tx.first_J=std::min(tx.first_J,cone.fJ);tx.min_slack=std::min(tx.min_slack,cone.slack);
    tx.apices[key]=next;nodes[next]=cone.apex;++next;fresh.insert(fresh.end(),cone.cells.begin(),cone.cells.end());
  }
  if(!tx.failing.empty())return tx;
  if(fresh.size()>MaximumReplacementCells){tx.cls="cap";tx.witness="cap-bound closure: "+std::to_string(fresh.size())+" fresh cells";tx.fresh_cells=fresh.size();return tx;}
  for(size_t i=0;i<fresh.size();++i)fresh[i].id=s.next_cell+i;tx.fresh_cells=fresh.size();
  // surfaces: walls and artificial interface unchanged; carriers moved, lateral, rims or gap faces; L-C loops
  std::vector<Face> newSkin,oldSkin;
  try{newSkin=Incidence(fresh).Boundary();oldSkin=Incidence(tx.old.cells).Boundary();}catch(const std::exception& e){tx.cls="topology";tx.witness=e.what();return tx;}
  std::map<Face,SurfaceFace> oldPhysical;for(const auto& sf:tx.old.surface)oldPhysical[Sorted(sf.v)]=sf;
  std::set<Face> oldOriented(oldSkin.begin(),oldSkin.end()),newOriented(newSkin.begin(),newSkin.end());
  auto oriented=[&](const std::set<Face>& set,Face f){for(int r=0;r<3;++r){if(set.count(f))return true;std::rotate(f.begin(),f.begin()+1,f.end());}return false;};
  Patch out;out.cells=fresh;
  std::map<int,std::map<std::pair<Id,Id>,int>> oldEdges,newEdges;
  auto edges=[](std::map<std::pair<Id,Id>,int>& m,const Face& f){for(int i=0;i<3;++i){const Id a=f[i],b=f[(i+1)%3];const auto r=m.find({b,a});if(r!=m.end()){if(--r->second==0)m.erase(r);}else ++m[{a,b}];}};
  for(const auto& f:oldSkin){const auto p=oldPhysical.find(Sorted(f));if(p==oldPhysical.end()){if(!oriented(newOriented,f)){tx.cls="topology";tx.witness="artificial interface changed";return tx;}continue;}
    const int m=s.reference->Get(p->second.facet).marker;if(s.walls.count(m)){if(!oriented(newOriented,f)){tx.cls="topology";tx.witness="wall face lost";return tx;}continue;}
    edges(oldEdges[m],f);}
  for(const auto& f:newSkin) {
    const auto key=Sorted(f);const auto p=oldPhysical.find(key);
    if(p!=oldPhysical.end()&&oriented(oldOriented,f)){out.surface.push_back({f,p->second.facet});const int m=s.reference->Get(p->second.facet).marker;if(!s.walls.count(m))edges(newEdges[m],f);continue;}
    if(oriented(oldOriented,f))continue;
    int marker=-1;{Face back=f;for(auto& x:back){const auto u=unlift.find(x);if(u!=unlift.end())x=u->second;}const auto q=oldPhysical.find(Sorted(back));
      if(q!=oldPhysical.end())marker=s.reference->Get(q->second.facet).marker;}
    if(marker<0){const auto lm=lateralMarker.find(key);if(lm!=lateralMarker.end())marker=lm->second;}
    if(marker<0||s.walls.count(marker)){tx.cls="topology";std::ostringstream w;w<<"unexpected new skin face ("<<f[0]<<","<<f[1]<<","<<f[2]<<")";tx.witness=w.str();return tx;}
    std::array<Point,3> tri{nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p};Id facet=0;bool found=false;
    try{found=ref.FindFace(marker,tri,facet);}catch(const std::exception&){found=false;}
    if(!found){tx.cls="carrier";std::ostringstream w;w<<"new carrier face ("<<f[0]<<","<<f[1]<<","<<f[2]<<") leaves marker "<<marker<<" authority; containing old facets per vertex:";
      std::set<Id> fs;for(const auto& sf:tx.old.surface)if(s.reference->Get(sf.facet).marker==marker)fs.insert(s.reference->Canonical(sf.facet));
      for(Id x:f){w<<" "<<x<<"{";for(Id fc:fs){bool in=false;try{in=ref.Contains(fc,nodes.at(x).p);}catch(const std::exception&){}if(in)w<<fc<<" ";}w<<"}";}
      tx.witness=w.str();return tx;}
    out.surface.push_back({f,facet});edges(newEdges[marker],f);}
  for(auto& m:newEdges)for(Id p:tx.seam){std::pair<Id,Id> in{0,0},outE{0,0};int nin=0,nout=0;
    for(const auto& e:m.second){if(e.first.second==p){in=e.first;++nin;}if(e.first.first==p){outE=e.first;++nout;}}
    if(nin==1&&nout==1){m.second.erase(in);m.second.erase(outE);const std::pair<Id,Id> joined{in.first,outE.second},back{outE.second,in.first};
      const auto r=m.second.find(back);if(r!=m.second.end()){if(--r->second==0)m.second.erase(r);}else ++m.second[joined];}}
  for(auto& m:oldEdges){auto& n=newEdges[m.first];if(m.second!=n){tx.cls="carrier";tx.witness="L-C: carrier boundary loop changed on marker "+std::to_string(m.first);return tx;}}
  for(auto& m:newEdges)if(!oldEdges.count(m.first)&&!m.second.empty()){tx.cls="carrier";tx.witness="L-C: new carrier patch";return tx;}
  std::string reason;
  if(!ValidatePrescribedInterface(tx.old.cells,fresh,newSkin,reason,nullptr,MaximumCavityCells)){tx.cls="topology";tx.witness=reason;return tx;}
  try{ref.ExportParameters(out);}catch(const std::exception& e){tx.cls="carrier";tx.witness=std::string("parameters: ")+e.what();return tx;}
  {FirstHeightReport r;WallHeights none;none.tolerance=HeightTolerance;if(!CheckFirstHeight(out,none,*s.reference,r,reason)){tx.cls="carrier";tx.witness="surface: "+reason;return tx;}}
  for(const auto& sf:out.surface){const auto key=Sorted(sf.v);if(!s.walls.count(s.reference->Get(sf.facet).marker))continue;
    const bool rowed=rows.apex.count(key)&&std::all_of(key.begin(),key.end(),[&](Id x){return rows.lift.count(x);});
    if(!tx.rebuilt.count(key)&&!rowed) {
      Patch before,after;for(const auto& c:tx.old.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)before.cells.push_back(c);}
      for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)after.cells.push_back(c);}
      before.surface=after.surface={sf};before.parameters=tx.old.parameters;after.parameters=out.parameters;
      FirstHeightReport r0,r1;std::string w0,w1;
      const bool was=before.cells.size()==1&&HeightConstruct(before,s.heights,*s.reference,&r0)&&Positive(before.cells.front().v);
      if(was&&!(after.cells.size()==1&&HeightConstruct(after,s.heights,*s.reference,&r1)&&Positive(after.cells.front().v))){tx.failing.push_back(key);tx.cls="invariant";tx.witness="unrebuilt wall face loses its passing geometric first child: "+w1;}
      continue;}
    Patch one;for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)one.cells.push_back(c);}
    one.surface={sf};for(const auto& c:one.cells)for(auto n:c.v){const auto pr=out.parameters.find(n.id);if(pr!=out.parameters.end())one.parameters.insert(*pr);}
    FirstHeightReport r;if(one.cells.size()!=1||!(validityOnlyConstruction?HeightConstruct(one,s.heights,*s.reference,&r,&reason):CheckFirstHeight(one,s.heights,*s.reference,r,reason))){tx.failing.push_back(key);tx.cls="height";tx.witness="face "+std::to_string(key[0])+": "+reason;continue;}
    tx.height_error=std::max(tx.height_error,r.maximum_relative_error);
    if(!Positive(one.cells.front().v)){tx.failing.push_back(key);tx.cls="first-child";tx.witness="rowed face first child has nonpositive orientation";}}
  if(!tx.failing.empty())return tx;
  for(const auto& c:out.cells)if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0)){tx.cls="validity";tx.witness="fresh cell has nonpositive orientation";return tx;}
  if(splitPolicy.rule==1){std::set<Id> rel;for(const auto& k:tx.rebuilt){const auto a=rows.apex.find(k);if(a!=rows.apex.end())rel.insert(a->second);}
    if(!RemovesVertex(tx.old.cells,out.cells,&rel)){std::string why;if(!validityOnlyConstruction&&!MeasurableChanged(s,tx.old,out,why)){tx.cls="measure";tx.witness=why;return tx;}
      ++splitPolicy.layer_free;tx.fresh=std::move(out);tx.ok=true;return tx;}++splitPolicy.layer_checked;}
  {long double removed=0,added=0;std::string why;if(!SizeCertificate(s,tx.old,out,removed,added,why)){tx.cls="measure";tx.witness=why;return tx;}
   if(!(added<=removed)){tx.cls="excess";std::ostringstream w;w<<"LayerConstruct adds frozen excess: added "<<double(added)<<" > removed "<<double(removed);tx.witness=w.str();return tx;}}
  tx.fresh=std::move(out);tx.ok=true;return tx;
}
inline void ApplyTx(State& s,Rows& rows,const Tx& tx) {
  Apply(s,tx.ids,tx.fresh);for(const auto& r:tx.lifts)rows.lift[r.first]=r.second;
  for(const auto& f:tx.rebuilt){rows.apex.erase(f);rows.overrides.erase(f);rows.extras.erase(f);}for(const auto& r:tx.apices)rows.apex[r.first]=r.second;
  for(const auto& r:tx.overrides)rows.overrides[r.first]=r.second;for(const auto& r:tx.extras)rows.extras[r.first]=r.second;
}

// ---- reporting -----------------------------------------------------------------------------------------------------
struct Stage { size_t work=0,accepted=0,rejected=0;double seconds=0;std::string outcome;std::map<std::string,size_t> reasons;std::string last;long rss=0;Counters c0,c1; };
inline std::string Key(const std::string& r){return swbi_dc2::Category(r).substr(0,110);}
inline std::string JsonStage(const Stage& g){std::ostringstream o;o<<std::setprecision(10)<<"{\"outcome\":\""<<g.outcome<<"\",\"work\":"<<g.work<<",\"accepted\":"<<g.accepted
  <<",\"rejected\":"<<g.rejected<<",\"seconds\":"<<g.seconds<<",\"peak_rss_kB\":"<<g.rss<<",\"field\":"<<JsonCounters(g.c0,g.c1)<<",\"reasons\":{";
  std::vector<std::pair<size_t,std::string>> top;for(const auto& r:g.reasons)top.emplace_back(r.second,r.first);
  std::sort(top.rbegin(),top.rend());for(size_t i=0;i<top.size()&&i<24;++i)o<<(i?",":"")<<junction_wp0::Json(top[i].second)<<':'<<top[i].first;
  o<<"},\"last_witness\":"<<junction_wp0::Json(g.last)<<'}';return o.str();}
inline void Begin(State& s,Stage& g){g.c0=CountersOf(s);}
inline void End(State& s,Stage& g,Clock::time_point t0){g.seconds=Seconds(t0);g.c1=CountersOf(s);g.rss=PeakKB();}

// ---- phase W (b): crease stations first, then level-line crossings, then metric bisection -------------------------
struct WStage : Stage { size_t stations=0,levels=0,bisections=0,unmeasurable=0,collapses=0,spacing_alternatives=0,spacing_fallbacks=0,chain_collapses=0,chain_moves=0,short_after=0;
  std::map<std::string,size_t> short_census;std::vector<std::string> short_witness; };
// DC-9 (a) helpers: binding dimension of a boundary point (0 original vertex, 1 original-edge chain, 2 facet interior;
// -1 none), minimum composed length of the wall edges of `fresh` incident to `point`, wall neighbours of a vertex.
inline int BindingDim(State& s,Id v) {
  const auto st=StarOf(s,v);if(st.empty())return -1;std::vector<Id> ids(st.begin(),st.end());
  try{const auto p=PatchOf(s,ids);const auto f=p.parameters.find(v);if(f!=p.parameters.end())return int(f->second.binding.dimension);}catch(const std::exception&){}
  return -1;
}
inline long double MinWallEdgeAt(State& s,const Patch& fresh,Id point) {
  std::map<Id,Point> at;for(const auto& c:fresh.cells)for(auto n:c.v)at[n.id]=n.p;
  long double best=INFINITY;
  for(const auto& sf:fresh.surface){if(!s.walls.count(s.reference->Get(sf.facet).marker)||!Has(sf.v,point))continue;
    for(Id x:sf.v)if(x!=point){long double l=-1;try{l=LenAt(s,point,at.at(point),x,at.at(x));}catch(const std::exception&){l=-1;}best=std::min(best,l);}}
  return best;
}
inline std::set<Id> WallNeighbours(const State& s,Id v) {
  std::set<Id> out;for(const auto& f:PhysicalOf(s,StarOf(s,v),true))if(Has(f.key,v))for(Id x:f.key)if(x!=v)out.insert(x);return out;
}
// Parameter t on (a,b) with Rho = rho_k (bracketed on 64 samples, then bisection); <0 when none in [lo,hi].
inline long double LevelCrossing(V a,V b,long double rhoK,long double lo,long double hi) {
  long double best=-1,bestDist=INFINITY;const unsigned n=64;long double t0=0,f0=Rho(a)-rhoK;
  for(unsigned i=1;i<=n;++i){const long double t1=(long double)i/n,f1=Rho(a+t1*(b-a))-rhoK;
    if((f0<0)!=(f1<0)){long double x0=t0,x1=t1,g0=f0;for(unsigned it=0;it<60;++it){const auto xm=(x0+x1)/2,gm=Rho(a+xm*(b-a))-rhoK;if((gm<0)==(g0<0)){x0=xm;g0=gm;}else x1=xm;}
      const auto t=(x0+x1)/2;if(t>=lo&&t<=hi&&std::abs(t-.5L)<bestDist){bestDist=std::abs(t-.5L);best=t;}}
    t0=t1;f0=f1;}
  return best;
}
inline WStage PhaseW(State& s,size_t work,double seconds,std::ostream& log) {
  Scope scope("PhaseW");
  WStage g;Begin(s,g);const auto t0=Clock::now();
  std::map<Id,int> createdBy;  // DC-9 diagnostics: 0 station, 1 level crossing, 2 bisection, 3 chain move
  std::set<Id> original;for(const auto& f:s.facets)for(auto x:f.v)original.insert(x.id);
  for(unsigned sweep=0;;++sweep) {
    std::vector<std::tuple<int,long double,Edge>> longs;std::set<Edge> seen;g.unmeasurable=0;
    for(const auto& wf:WallFaces(s))for(int i=0;i<3;++i){Edge e{std::min(wf.second[i],wf.second[(i+1)%3]),std::max(wf.second[i],wf.second[(i+1)%3])};
      if(!seen.insert(e).second)continue;long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception& x){++g.reasons["length: "+Key(x.what())];++g.unmeasurable;continue;}
      const bool crease=!d1.creases.empty()&&OnCrease(Of(NodePoint(s,e[0])),Of(NodePoint(s,e[1])));
      if(crease?l>StationTarget:l>MaximumMetricEdge)longs.emplace_back(crease?0:1,-l,e);}
    log<<"DC9 W sweep "<<sweep<<" wall_faces="<<WallFaces(s).size()<<" long_wall_edges="<<longs.size()<<" work="<<g.work<<"\n";
    if(longs.empty()){g.outcome=g.unmeasurable?"unmeasurable wall edges: "+std::to_string(g.unmeasurable):"complete";break;}
    std::sort(longs.begin(),longs.end());bool changed=false;
    for(const auto& row:longs) {
      if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
      const auto e=std::get<2>(row);const auto star=Star(s,e);if(star.empty())continue;++g.work;
      if(star.size()>MaximumCavityCells){++g.rejected;++g.reasons["edge star exceeds 64"];continue;}
      Patch old;try{old=PatchOf(s,star);}catch(const std::exception& x){++g.rejected;++g.reasons[Key(x.what())];continue;}
      const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);
      std::vector<std::pair<Point,int>> points;  // kind 0 station, 1 level, 2 bisection
      if(std::get<0>(row)==1&&!d1.creases.empty()) {
        const V A=Of(a),B=Of(b);const auto pa=Phi(Rho(A)),pb=Phi(Rho(B));const auto lo=std::min(pa,pb),hi=std::max(pa,pb);
        long double bestT=-1,bestD=INFINITY;
        for(long double k=std::ceil(lo+.25L);k<=hi-.25L&&k<512;k+=1){const auto rk=RhoOf(k);if(rk>d1.radius)break;
          const auto t=LevelCrossing(A,B,rk,.15L,.85L);if(t>=0&&std::abs(t-.5L)<bestD){bestD=std::abs(t-.5L);bestT=t;}}
        if(bestT>=0)points.push_back({Pt(A+bestT*(B-A)),1});
      }
      try{points.push_back({s.field->SegmentPoint(a,b,.5),std::get<0>(row)==0?0:2});}catch(const std::exception&){}
      points.push_back({{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)},std::get<0>(row)==0?0:2});
      // DC-9 (a) spacing law: the strip point (station / level crossing) is kept unless it makes a metric-short wall
      // edge (< ShortWallEdge) to the opposite wall vertices while a later candidate (metric, then Euclidean
      // midpoint) does not; when every candidate is short, the one with the longest shortest new wall edge is used.
      bool done=false;std::string why;struct Pick{bool ok=false;Patch fresh;int kind=0,op=0;long double shortest=-INFINITY;size_t rank=0;} best;size_t rank=0;
      for(int op=0;op<2&&!done&&!(op==1&&best.ok);++op)for(const auto& p:points){Patch fresh;bool built=false;
        try{Scope sc(op==0?"W SplitEdge":"W InsertEdgeCavity");built=op==0?SplitEdge(old,e,{s.next_node,p.first},s.next_cell,*s.reference,fresh,why):InsertEdgeCavity(old,e,{s.next_node,p.first},s.next_cell,*s.reference,fresh,why);}
        catch(const std::exception& x){why=x.what();built=false;}
        if(!built)continue;
        {std::string d2;if(!D2Ok(s,old,fresh,d2)){why=d2;continue;}}
        const auto shortest=MinWallEdgeAt(s,fresh,s.next_node);++rank;
        if(shortest>best.shortest){best.ok=true;best.fresh=fresh;best.kind=p.second;best.op=op;best.shortest=shortest;best.rank=rank;}
        if(shortest>=ShortWallEdge){done=true;break;}}
      if(best.ok){done=true;if(best.rank>1)++g.spacing_alternatives;if(best.shortest<ShortWallEdge)++g.spacing_fallbacks;
        createdBy[s.next_node]=best.kind;Apply(s,star,best.fresh);++g.accepted;++g.reasons[best.op?"accepted InsertEdgeCavity":"accepted SplitEdge"];(best.kind==0?g.stations:best.kind==1?g.levels:g.bisections)++;changed=true;}
      if(!done){++g.rejected;++g.reasons[Key(why)];g.last=why;}
    }
    if(!g.outcome.empty())break;
    if(!changed){g.outcome="blocked";break;}
  }
  // (b) iteration 2: metric-short wall edges (< ShortWallEdge) from converging level points near crease vertices make
  // wall slivers whose first children fail U1 shape (DC-8 diagnostic: base edges 1.0/1.0/0.04). Collapse the newer
  // endpoint onto the older one (production CollapseVertex: positivity, facet/feature preservation), when no wall edge
  // of the result exceeds the surface target and the D.2 invariant holds; bounded by the same W limits.
  if(g.outcome=="complete") {
    for(unsigned sweep=0;sweep<4;++sweep) {
      std::vector<std::pair<long double,Edge>> shorts;std::set<Edge> seen;
      for(const auto& wf:WallFaces(s))for(int i=0;i<3;++i){Edge e{std::min(wf.second[i],wf.second[(i+1)%3]),std::max(wf.second[i],wf.second[(i+1)%3])};
        if(!seen.insert(e).second)continue;long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}if(l<ShortWallEdge)shorts.emplace_back(l,e);}
      std::sort(shorts.begin(),shorts.end());bool any=false;
      for(const auto& row:shorts) {
        if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
        const Edge e=row.second;if(Star(s,e).empty())continue;const Id x=e[1],y=e[0];  // newer (larger id) onto older
        const auto star=StarOf(s,x);if(star.empty()||star.size()>MaximumCavityCells){++g.reasons["collapse: star exceeds 64"];continue;}
        ++g.work;std::vector<Id> ids(star.begin(),star.end());Patch old,fresh;std::string why;bool ok=false;
        try{Scope sc("W CollapseVertex");old=PatchOf(s,ids);ok=CollapseVertex(old,x,y,s.next_cell,*s.reference,fresh,why);}catch(const std::exception& z){why=z.what();ok=false;}
        if(ok){for(const auto& c:fresh.cells)for(unsigned i=0;i<4&&ok;++i)for(unsigned j=i+1;j<4&&ok;++j){const Id a=c.v[i].id,b=c.v[j].id;
            bool wallEdge=false;for(const auto& sf:fresh.surface)if(s.walls.count(s.reference->Get(sf.facet).marker)&&Has(sf.v,a)&&Has(sf.v,b))wallEdge=true;
            if(!wallEdge)continue;try{if(LenAt(s,a,c.v[i].p,b,c.v[j].p)>MaximumMetricEdge){ok=false;why="collapse: wall edge exceeds the surface target";}}catch(const std::exception&){ok=false;why="collapse: unmeasurable wall edge";}}}
        if(ok){std::string d2;if(!D2Ok(s,old,fresh,d2)){ok=false;why=d2;}}
        if(!ok){++g.rejected;++g.reasons["collapse: "+Key(why)];continue;}
        Apply(s,ids,fresh);++g.accepted;++g.collapses;any=true;
      }
      if(!any||g.outcome=="limit")break;
    }
  }
  // DC-9 (a) chain pass: a remaining metric-short wall edge whose endpoints are pinned to different feature chains
  // (CollapseVertex keeps every incident reference facet) is removed by collapsing one non-original endpoint onto a
  // wall neighbour on its own chain (any neighbour CollapseVertex admits, shortest first), else lengthened by moving
  // that endpoint along its chain (MoveVertex binds the point to its incident facets; away from the short partner).
  // Original facet vertices (corners) are never collapsed or moved. Same acceptance as above (no wall edge above the
  // surface target, D.2 invariant), same W limits.
  auto wallEdgesOk=[&](const Patch& fresh,std::string& why)->bool{
    for(const auto& c:fresh.cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){const Id a=c.v[i].id,b=c.v[j].id;
      bool wallEdge=false;for(const auto& sf:fresh.surface)if(s.walls.count(s.reference->Get(sf.facet).marker)&&Has(sf.v,a)&&Has(sf.v,b))wallEdge=true;
      if(!wallEdge)continue;try{if(LenAt(s,a,c.v[i].p,b,c.v[j].p)>MaximumMetricEdge){why="wall edge exceeds the surface target";return false;}}catch(const std::exception&){why="unmeasurable wall edge";return false;}}
    return true;};
  if(g.outcome=="complete")for(unsigned sweep=0;sweep<4;++sweep) {
    std::vector<std::pair<long double,Edge>> shorts;std::set<Edge> seen;
    for(const auto& wf:WallFaces(s))for(int i=0;i<3;++i){Edge e{std::min(wf.second[i],wf.second[(i+1)%3]),std::max(wf.second[i],wf.second[(i+1)%3])};
      if(!seen.insert(e).second)continue;long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}if(l<ShortWallEdge)shorts.emplace_back(l,e);}
    std::sort(shorts.begin(),shorts.end());bool any=false;
    for(const auto& row:shorts) {
      if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
      const Edge e=row.second;if(Star(s,e).empty())continue;bool fixed=false;std::string last;
      for(Id v:{e[1],e[0]}){if(fixed)break;if(original.count(v))continue;const Id other=v==e[1]?e[0]:e[1];
        const auto star=StarOf(s,v);if(star.empty()||star.size()>MaximumCavityCells)continue;std::vector<Id> ids(star.begin(),star.end());
        std::vector<std::pair<long double,Id>> targets;for(Id z:WallNeighbours(s,v)){long double l=INFINITY;try{l=Len(s,v,z);}catch(const std::exception&){}targets.emplace_back(z==other?-1:l,z);}
        std::sort(targets.begin(),targets.end());
        for(const auto& t:targets){if(fixed||g.work>=work||Seconds(t0)>=seconds)break;++g.work;Patch old,fresh;std::string why;bool ok=false;
          try{Scope sc("W chain CollapseVertex");old=PatchOf(s,ids);ok=CollapseVertex(old,v,t.second,s.next_cell,*s.reference,fresh,why);}catch(const std::exception& z){why=z.what();ok=false;}
          if(ok)ok=wallEdgesOk(fresh,why);
          if(ok){std::string d2;if(!D2Ok(s,old,fresh,d2)){ok=false;why=d2;}}
          if(!ok){++g.reasons["chain collapse: "+Key(why)];last=why;continue;}
          Apply(s,ids,fresh);++g.accepted;++g.chain_collapses;fixed=true;}
      }
      for(Id v:{e[1],e[0]}){if(fixed)break;if(original.count(v))continue;const Id other=v==e[1]?e[0]:e[1];
        const auto star=StarOf(s,v);if(star.empty()||star.size()>MaximumCavityCells)continue;std::vector<Id> ids(star.begin(),star.end());
        const V pv=Of(NodePoint(s,v)),po=Of(NodePoint(s,other));
        for(long double t:{1.L,.5L,.25L}){if(fixed||g.work>=work||Seconds(t0)>=seconds)break;++g.work;Patch old,fresh;std::string why;bool ok=false;const Id moved=s.next_node;
          try{Scope sc("W chain MoveVertex");old=PatchOf(s,ids);ok=MoveVertex(old,v,{moved,Pt(pv+t*(pv-po))},s.next_cell,*s.reference,fresh,why);}catch(const std::exception& z){why=z.what();ok=false;}
          if(ok&&MinWallEdgeAt(s,fresh,moved)<ShortWallEdge){ok=false;why="move keeps a metric-short wall edge";}
          if(ok)ok=wallEdgesOk(fresh,why);
          if(ok){std::string d2;if(!D2Ok(s,old,fresh,d2)){ok=false;why=d2;}}
          if(!ok){++g.reasons["chain move: "+Key(why)];last=why;continue;}
          createdBy[moved]=3;Apply(s,ids,fresh);++g.accepted;++g.chain_moves;fixed=true;}
      }
      if(fixed)any=true;else ++g.rejected;
      if(Seconds(t0)>=seconds){g.outcome="limit";break;}
    }
    if(!any||g.outcome=="limit")break;
  }
  End(s,g,t0);
  // DC-9 diagnostics (after W's limits and timing are final; read-only): census of the remaining metric-short wall
  // edges by endpoint origin and binding dimension.
  {std::set<Edge> seen;
   for(const auto& wf:WallFaces(s))for(int i=0;i<3;++i){Edge e{std::min(wf.second[i],wf.second[(i+1)%3]),std::max(wf.second[i],wf.second[(i+1)%3])};
     if(!seen.insert(e).second)continue;long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}if(!(l<ShortWallEdge))continue;
     ++g.short_after;auto kind=[&](Id x){if(original.count(x))return std::string("orig");const auto k=createdBy.find(x);return k==createdBy.end()?std::string("input"):k->second==0?std::string("station"):k->second==1?std::string("level"):k->second==2?std::string("bisect"):std::string("moved");};
     std::array<std::string,2> d{kind(e[0])+"/d"+std::to_string(BindingDim(s,e[0])),kind(e[1])+"/d"+std::to_string(BindingDim(s,e[1]))};std::sort(d.begin(),d.end());
     ++g.short_census[d[0]+"-"+d[1]];
     if(g.short_witness.size()<40){std::ostringstream w;w<<std::setprecision(6)<<"e="<<e[0]<<"-"<<e[1]<<" l="<<double(l)<<" "<<d[0]<<" "<<d[1]<<" rho="<<double(Rho(Of(NodePoint(s,e[0]))))<<","<<double(Rho(Of(NodePoint(s,e[1]))));g.short_witness.push_back(w.str());}}}
  for(const auto& w:g.short_witness)log<<"DC9 W short "<<w<<"\n";
  return g;
}

// ---- R1 (c): clearance conditioning with the finite candidate set of §8.3 ------------------------------------------
inline long double LocalDeficit(State& s,const std::set<Id>& walls,const std::vector<Id>& removed,const std::vector<Cell>* added,std::map<Id,Dir>& dirs) {
  std::set<Id> gone,touched;if(added)gone.insert(removed.begin(),removed.end());for(Id id:removed)for(auto n:s.mesh.at(id).cell.v)if(walls.count(n.id))touched.insert(n.id);
  long double sum=0;
  for(Id w:touched){auto d=dirs.find(w);if(d==dirs.end())d=dirs.emplace(w,DirectionAt(s,w)).first;if(!d->second.ok){sum+=Need(VertexH0(s,w));continue;}
    std::vector<Cell> cells;for(Id id:StarOf(s,w))if(!gone.count(id))cells.push_back(s.mesh.at(id).cell);
    if(added)for(const auto& c:*added)for(auto n:c.v)if(n.id==w)cells.push_back(c);
    const auto k=KernelBound(cells,w,d->second.d);sum+=std::max(0.L,Need(VertexH0(s,w))-k.delta*d->second.s);}
  return sum;
}
struct R1Stage : Stage { size_t deficient_before=0,deficient_after=0,moves=0,flips=0,steiner=0,collapses=0,exhausted=0; };
inline R1Stage PhaseR1(State& s,const Rows& rows,size_t work,double seconds,std::ostream& log) {
  Scope scope("PhaseR1");
  R1Stage g;Begin(s,g);const auto t0=Clock::now();const auto walls=WallVertices(s);const auto apices=rows.Apices();
  auto limited=[&]()->bool{if(g.work>=work){g.outcome="limit:work";return true;}if(Seconds(t0)>=seconds){g.outcome="limit:time";return true;}return false;};
  auto deficient=[&](){std::vector<std::pair<long double,Id>> out;for(Id v:walls){if(Rim(s,v))continue;const auto pl=PlanLift(s,apices,v,false,false);const auto d=Deficit(s,pl);if(d>0)out.emplace_back(-d,v);}std::sort(out.begin(),out.end());return out;};
  auto list=deficient();g.deficient_before=list.size();
  auto boundaryVertex=[&](Id x){for(Id id:StarOf(s,x)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)if(r.physical[k].present&&Has(f[k],x))return true;}return false;};
  unsigned round=0;
  for(;round<4&&!list.empty();++round) {
    bool any=false;
    for(const auto& row:list) {
      const Id v=row.second;
      for(unsigned edit=0;edit<6;++edit) {
        if(limited())break;
        const auto pl=PlanLift(s,apices,v,false,false);if(!pl.dir.ok||Deficit(s,pl)<=0)break;if(!s.mesh.count(pl.kernel.cell))break;
        const auto blocking=s.mesh.at(pl.kernel.cell).cell;std::vector<Id> F;for(auto n:blocking.v)if(n.id!=v)F.push_back(n.id);
        std::map<Id,Dir> dirs;bool taken=false;
        auto attempt=[&](const std::string& op,const std::vector<Id>& ids,const Patch& fresh)->bool{
          ++g.work;const auto before=LocalDeficit(s,walls,ids,nullptr,dirs),after=LocalDeficit(s,walls,ids,&fresh.cells,dirs);
          if(!(after<before*(1-1e-9L))){++g.rejected;++g.reasons[op+": no clearance gain"];return false;}
          {Patch old=PatchOf(s,ids);std::string d2;if(!D2Ok(s,old,fresh,d2)){++g.rejected;++g.reasons[op+": "+d2.substr(0,40)];return false;}}
          if(!FreshValid(s,fresh.cells)){++g.rejected;++g.reasons[op+": validity"];return false;}
          Apply(s,ids,fresh);++g.accepted;return true;};
        // c1 Reconnect of star(v) with the edge star of each link edge of the blocking face, coned from v
        for(int i=0;i<3&&!taken&&!limited();++i){auto ids=StarOf(s,v);for(Id id:Star(s,{std::min(F[i],F[(i+1)%3]),std::max(F[i],F[(i+1)%3])}))ids.insert(id);
          if(ids.size()>MaximumCavityCells){++g.reasons["Reconnect: cavity exceeds 64"];continue;}std::vector<Id> list(ids.begin(),ids.end());Patch old,fresh;std::string why;
          try{Scope sc("R1 Reconnect");old=PatchOf(s,list);if(Reconnect(old,v,s.next_cell,*s.reference,fresh,why)){if(attempt("Reconnect",list,fresh)){++g.flips;taken=true;}}else{++g.work;++g.rejected;++g.reasons["Reconnect: "+Key(why)];}}
          catch(const std::exception& x){++g.rejected;++g.reasons["Reconnect: "+Key(x.what())];}}
        // c2 MoveVertex of interior link vertices: kernel direction (lift direction, then away from v), deficit x {2,1,1/2,1/4}
        if(!taken)for(Id x:F){if(taken||limited())break;if(boundaryVertex(x))continue;const auto star=StarOf(s,x);if(star.size()>MaximumCavityCells)continue;std::vector<Id> list(star.begin(),star.end());
          const V px=Of(NodePoint(s,x));V away=px-Of(NodePoint(s,v));if(Norm(away)>0)away=Unit(away);const long double D=Deficit(s,pl)/std::max(pl.dir.s,.1L);
          for(const V& u:{pl.dir.d,away}){if(taken)break;for(long double t:{2.L,1.L,.5L,.25L}){if(limited())break;Patch old,fresh;std::string why;
            try{Scope sc("R1 MoveVertex");old=PatchOf(s,list);if(MoveVertex(old,x,{s.next_node,Pt(px+(t*D)*u)},s.next_cell,*s.reference,fresh,why)){if(attempt("MoveVertex",list,fresh)){++g.moves;taken=true;break;}}
                else{++g.work;++g.rejected;++g.reasons["MoveVertex: "+Key(why)];}}catch(const std::exception& e){++g.rejected;++g.reasons["MoveVertex: "+Key(e.what())];}}}}
        // c3 CollapseVertex (coarsening) of an interior link vertex onto its neighbour farthest along the lift direction
        if(!taken)for(Id x:F){if(taken||limited())break;if(boundaryVertex(x))continue;const auto star=StarOf(s,x);if(star.size()>MaximumCavityCells)continue;
          Id y=UINT64_MAX;long double best=-INFINITY;const V pv=Of(NodePoint(s,v));for(Id id:star)for(auto n:s.mesh.at(id).cell.v)if(n.id!=x&&n.id!=v){const auto h=Dot(Of(n.p)-pv,pl.dir.d);if(h>best){best=h;y=n.id;}}
          if(y==UINT64_MAX)continue;std::vector<Id> list(star.begin(),star.end());Patch old,fresh;std::string why;
          try{Scope sc("R1 CollapseVertex");old=PatchOf(s,list);if(CollapseVertex(old,x,y,s.next_cell,*s.reference,fresh,why)){if(attempt("CollapseVertex",list,fresh)){++g.collapses;taken=true;}}
              else{++g.work;++g.rejected;++g.reasons["CollapseVertex: "+Key(why)];}}catch(const std::exception& e){++g.rejected;++g.reasons["CollapseVertex: "+Key(e.what())];}}
        // c4 Steiner point above an incident wall face at {2,3} x the preferred band height
        if(!taken)for(const auto& f:PhysicalOf(s,StarOf(s,v),true)){if(taken||limited())break;if(!Has(f.key,v))continue;
          const V a=Of(NodePoint(s,f.out[0])),b=Of(NodePoint(s,f.out[1])),c=Of(NodePoint(s,f.out[2]));const auto la=Norm(b-c),lb=Norm(c-a),lc=Norm(a-b);
          const V in=(1/(la+lb+lc))*(la*a+lb*b+lc*c);const V n=-1.L*Outward(a,b,c);
          for(long double k:{2.L,3.L}){const Point p=Pt(in+(k*Need(VertexH0(s,v))/Kappa)*n);std::string why;const auto vis=dc7::Visible(s,p,{});
            if(!vis.ok){++g.work;++g.rejected;++g.reasons["Steiner: "+vis.why];continue;}const auto& cavity=vis.cavity;
            Patch old,fresh;try{Scope sc("R1 Steiner");old=PatchOf(s,cavity);fresh.surface=old.surface;fresh.parameters=old.parameters;
              if(Cone(old.cells,{s.next_node,p},s.next_cell,fresh.cells,why)){s.reference->ExportParameters(fresh);if(attempt("Steiner",cavity,fresh)){++g.steiner;taken=true;break;}}
              else{++g.work;++g.rejected;++g.reasons["Steiner: "+Key(why)];}}catch(const std::exception& e){++g.rejected;++g.reasons["Steiner: "+Key(e.what())];}}}
        if(!taken){++g.exhausted;break;}any=true;
      }
      if(!g.outcome.empty())break;
    }
    if(!g.outcome.empty()||!any)break;
    list=deficient();
  }
  list=deficient();g.deficient_after=list.size();
  if(g.outcome.empty())g.outcome=list.empty()?"complete":round>=4?"limit:rounds":"exhausted";
  if(!list.empty()){std::ostringstream w;w<<"clearance (R1 "<<g.outcome<<"): vertex "<<list.front().second<<" deficit/h0="<<double(-list.front().first/s.h0);g.last=w.str();}
  End(s,g,t0);log<<"DC9 R1 deficient "<<g.deficient_before<<" -> "<<g.deficient_after<<" accepted="<<g.accepted<<" outcome="<<g.outcome<<"\n";return g;
}

// ---- census (as DC-7, deficits without metric-cap integrations) ----------------------------------------------------
struct Census { size_t faces=0,vertices=0,no_direction=0,deficient=0,lp_empty=0;std::map<std::string,size_t> why; };
inline Census FullStateCensus(State& s,const Rows& rows) {
  Scope scope("Census");
  Census c;std::map<Id,Plan> plans;const auto apices=rows.Apices();
  for(Id v:WallVertices(s)){if(Rim(s,v))continue;++c.vertices;auto pl=PlanLift(s,apices,v,false,false);if(!pl.ok){++c.no_direction;++c.why[pl.why.substr(0,40)];}else if(Deficit(s,pl)>0)++c.deficient;plans.emplace(v,pl);}
  for(const auto& wf:WallFaces(s)) {
    if(RimFace(s,wf.second))continue;++c.faces;const auto& row=s.mesh.at(wf.first);const auto faces=CellFaces(row.cell);Face out{};for(auto f:faces)if(Sorted(f)==wf.second)out=f;
    bool ok=true;for(Id x:out)ok=ok&&plans.at(x).ok;if(!ok){++c.lp_empty;continue;}
    std::map<Id,V> P;for(Id x:out){P[x]=Of(NodePoint(s,x));P[x+(Id(1)<<60)]=plans.at(x).p;}
    const auto skin=ColumnSkin(out,[](Id x){return x+(Id(1)<<60);}).faces;
    const V a=P[out[0]],b=P[out[1]],cc=P[out[2]];std::vector<std::array<V,3>> tris;for(size_t k=1;k<skin.size();++k)tris.push_back({P[skin[k][0]],P[skin[k][1]],P[skin[k][2]]});
    if(!ApexLP(tris,a,b,cc,-1.L*Outward(a,b,cc),FaceH0(s,wf.second),HeightTolerance,false).ok)++c.lp_empty;
  }
  return c;
}
inline std::string JsonCensus(const Census& c){std::ostringstream o;o<<"{\"wall_faces\":"<<c.faces<<",\"wall_vertices\":"<<c.vertices<<",\"no_direction\":"<<c.no_direction
  <<",\"clearance_deficient_vertices\":"<<c.deficient<<",\"faces_full_state_lp_empty\":"<<c.lp_empty<<",\"direction_failures\":{";bool k=false;
  for(const auto& r:c.why){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}o<<"}}";return o.str();}

// ---- R2: closed row transactions; low band before growth; corner separation (Prepare) for sector lifts ------------
struct R2Stage : Stage { size_t rim_faces=0,transactions=0,lifted=0,unlifted=0,rowed=0,unrowed=0,max_closure=0,max_old=0,max_fresh=0,low_band=0,sector_tx=0,gaps=0,separations=0,untried=0;
  double first_q=1,first_J=1,height_error=0;std::map<std::string,size_t> classes; };
// Corner / link separation (Prepare kind): a point at the centroid of the named star cell cones that cell, so a corner
// cell no longer owns a wall face and a foreign carrier face and the link of v gains an interior vertex. Positivity,
// the D.2 invariant on its wall faces; no column cell touched.
inline bool SplitCell(State& s,const Rows& rows,Id cell,std::string& why) {
  Scope scope("R2 SplitCell");
  if(!s.mesh.count(cell)){why="cell gone";return false;}if(ColumnCells(s,rows).count(cell)){why="cell is a column cell";return false;}
  Patch old;try{old=PatchOf(s,{cell});}catch(const std::exception& x){why=x.what();return false;}
  const auto& c=old.cells.front();V m{};for(auto n:c.v)m=m+.25L*Of(n.p);
  // above the band: with a wall face, the point lies halfway from its centroid to the opposite vertex
  for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker)){V g{};for(Id x:sf.v)for(auto n:c.v)if(n.id==x)g=g+(1.L/3)*Of(n.p);
    for(auto n:c.v)if(!Has(sf.v,n.id))m=.5L*(g+Of(n.p));break;}
  Patch fresh;fresh.surface=old.surface;fresh.parameters=old.parameters;
  try{if(!Cone(old.cells,{s.next_node,Pt(m)},s.next_cell,fresh.cells,why))return false;s.reference->ExportParameters(fresh);}catch(const std::exception& x){why=x.what();return false;}
  {std::string d2;if(!D2Ok(s,old,fresh,d2)){why=d2;return false;}}
  if(!FreshValid(s,fresh.cells)){why="validity";return false;}
  Apply(s,{cell},fresh);return true;
}
// Link separation (Prepare kind): cone the edge star of an interior edge from its midpoint (InsertEdgeCavity).
inline bool SplitLinkEdge(State& s,const Rows& rows,Edge e,std::string& why,Id wall=UINT64_MAX) {
  Scope scope("R2 SplitLinkEdge");
  const auto star=Star(s,e);if(star.empty()||star.size()>MaximumCavityCells){why="link edge star";return false;}
  const auto cols=ColumnCells(s,rows);for(Id id:star)if(cols.count(id)){why="link edge star touches a column";return false;}
  Patch old,fresh;try{old=PatchOf(s,star);}catch(const std::exception& x){why=x.what();return false;}
  const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);Point mid{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
  if(wall!=UINT64_MAX){const auto dir=DirectionAt(s,wall);if(dir.ok){V nbar{};for(const auto& n:dir.normals)nbar=nbar+n;nbar=Unit(nbar);const V w=Of(NodePoint(s,wall));
    auto h=[&](V p){return Dot(p-w,nbar);};const V A=Of(a),B=Of(b);if(h(Of(mid))<2*Need(VertexH0(s,wall))){long double best=-INFINITY;
      for(long double t:{.3L,.4L,.5L,.6L,.7L}){const V p=A+t*(B-A);if(h(p)>best){best=h(p);mid=Pt(p);}}}}}
  bool ok=false;try{ok=InsertEdgeCavity(old,e,{s.next_node,mid},s.next_cell,*s.reference,fresh,why);}catch(const std::exception& x){why=x.what();ok=false;}
  if(!ok)return false;
  {std::string d2;if(!D2Ok(s,old,fresh,d2)){why=d2;return false;}}
  if(!FreshValid(s,fresh.cells)){why="validity";return false;}
  Apply(s,star,fresh);return true;
}
inline R2Stage PhaseR2(State& s,Rows& rows,double seconds,std::ostream& log,size_t maximum_work=std::numeric_limits<size_t>::max()) {
  Scope scope("PhaseR2");
  R2Stage g;Begin(s,g);const auto t0=Clock::now();std::set<Id> tried;  // DC-15: vertices with at least one transaction
  for(unsigned pass=0;pass<2;++pass) {
    // (c) canonical order by lift-direction quality: smooth vertices (s* = 1) first, crease and corner vertices last,
    // so a crease vertex is lifted when its neighbours already are (full instead of skewed partial column states)
    std::vector<std::pair<long double,Id>> order;std::set<Id> sectorVertices;
    for(Id v:WallVertices(s)){if(rows.lift.count(v)||Rim(s,v))continue;const auto d=DirectionAt(s,v);order.emplace_back(d.ok?-std::round(d.s*1e6L)/1e6L:1.L,v);
      std::set<Id> charts;for(Id f:d.carriers)charts.insert(s.reference->Canonical(f));if(charts.size()>=2)sectorVertices.insert(v);}
    std::sort(order.begin(),order.end());
    // DC-9 (c) iteration 1 (sector vertices before their wall neighbours) is kept as an option only: on A1 it turned the
    // gap-side witness into an LP-empty corner column (lid faces down to the unlifted junction) and rowed fewer faces.
    if(sectorFirst){std::vector<std::pair<long double,Id>> sequenced;std::set<Id> placed;
     for(const auto& ov:order){for(Id w:WallNeighbours(s,ov.second))if(sectorVertices.count(w)&&!rows.lift.count(w)&&!Rim(s,w)&&placed.insert(w).second)sequenced.push_back({0,w});
       if(placed.insert(ov.second).second)sequenced.push_back(ov);}
     order.swap(sequenced);}
    for(const auto& ov:order) {
      const Id v=ov.second;
      if(rows.lift.count(v)||Rim(s,v))continue;if(Seconds(t0)>=seconds){g.outcome="limit:time";break;}if(g.work>=maximum_work){g.outcome="limit:work";break;}
      std::set<Id> V0{v};Tx tx;bool low=false;unsigned separated=0;
      for(unsigned grow=0;grow<20;++grow) {
        if(Seconds(t0)>=seconds)break;if(g.work>=maximum_work){g.outcome="limit:work";break;}
        ++g.work;tried.insert(v);tx=Transaction(s,rows,V0,low);
        log<<"DC9 R2 tx v="<<v<<" |V|="<<V0.size()<<" low="<<low<<" ok="<<tx.ok<<" old="<<tx.old_cells<<" fresh="<<tx.fresh_cells<<" sectors="<<tx.sectors<<" gaps="<<tx.gaps<<" cls="<<tx.cls<<" "<<tx.witness<<"\n";
        if(tx.ok)break;
        if((tx.cls=="sector-corner"||tx.cls=="sector-link")&&separated<4&&(tx.corner!=UINT64_MAX||tx.splitEdge[0]!=tx.splitEdge[1])){std::string why;
          if(tx.corner!=UINT64_MAX?SplitCell(s,rows,tx.corner,why):SplitLinkEdge(s,rows,tx.splitEdge,why,tx.sectorVertex)){++separated;++g.separations;continue;}
          tx.witness+="; separation: "+why;break;}
        if(!low&&(tx.cls=="partial-state"||tx.cls=="first-child"||tx.cls=="clearance"||tx.cls=="height")){low=true;continue;}
        if(tx.cls=="clearance"){bool added=false;for(Id x:tx.blockers)if(!Rim(s,x)&&V0.insert(x).second)added=true;if(added)continue;break;}
        if(tx.cls!="partial-state"&&tx.cls!="first-child"&&tx.cls!="height")break;
        bool added=false;std::sort(tx.failing.begin(),tx.failing.end());
        {const auto& f=tx.failing.front();for(Id x:f)if(!rows.lift.count(x)&&!V0.count(x)&&!Rim(s,x)){V0.insert(x);added=true;}}
        if(!added){tx.witness="no growth: "+tx.witness;break;}
      }
      if(tx.ok){ApplyTx(s,rows,tx);++g.accepted;++g.transactions;g.low_band+=low;g.sector_tx+=tx.sectors>0;g.gaps+=tx.gaps;g.max_closure=std::max(g.max_closure,V0.size());
        g.max_old=std::max(g.max_old,tx.old_cells);g.max_fresh=std::max(g.max_fresh,tx.fresh_cells);g.first_q=std::min(g.first_q,tx.first_q);g.first_J=std::min(g.first_J,tx.first_J);g.height_error=std::max(g.height_error,tx.height_error);}
      else{++g.rejected;++g.classes[tx.cls];++g.reasons[tx.cls+": "+Key(tx.witness)];g.last="vertex "+std::to_string(v)+" ["+tx.cls+"] "+tx.witness;}
    }
    if(!g.outcome.empty())break;
  }
  for(Id v:WallVertices(s))if(!Rim(s,v)){(rows.lift.count(v)?g.lifted:g.unlifted)++;if(!rows.lift.count(v)&&!tried.count(v))++g.untried;}
  for(const auto& wf:WallFaces(s)){if(RimFace(s,wf.second)){++g.rim_faces;continue;}const bool r=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});(r?g.rowed:g.unrowed)++;}
  if(g.outcome.empty())g.outcome=g.unrowed?"incomplete":"complete";End(s,g,t0);
  log<<"DC9 R2 transactions="<<g.transactions<<" lifted="<<g.lifted<<" unlifted="<<g.unlifted<<" untried="<<g.untried<<" rowed="<<g.rowed<<" unrowed="<<g.unrowed<<" outcome="<<g.outcome<<"\n";return g;
}

// ---- final gates with cached measures --------------------------------------------------------------------------------
struct Gates8 { size_t cells=0,bad_shape=0,long_edges=0,wall_faces=0,height_failures=0,long_wall=0,long_seam=0,long_other_physical=0,long_interior=0,unmeasurable=0;double q=1,J=1;long double lmax=0;bool complete=false; };
inline int EdgeClass(State& s,Edge e) {
  bool w=false,o=false;for(Id id:Star(s,e)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
    if(r.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1]))(s.walls.count(s.reference->Get(r.physical[k].facet).marker)?w:o)=true;}
  return w&&o?2:w?1:o?3:0;
}
inline Gates8 Final(State& s) {
  Scope scope("Final gates");
  Gates8 g;g.cells=s.mesh.size();std::set<Edge> seen;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;const auto sh=MeshShape(s,row.first);g.q=std::min(g.q,sh.q);g.J=std::min(g.J,sh.J);g.bad_shape+=Bad(sh);
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){++g.unmeasurable;l=INFINITY;}
      if(std::isfinite(l))g.lmax=std::max(g.lmax,l);
      if(l>MaximumMetricEdge){++g.long_edges;const int k=EdgeClass(s,e);(k==2?g.long_seam:k==1?g.long_wall:k==3?g.long_other_physical:g.long_interior)++;}}}
  for(const auto& wf:WallFaces(s)){++g.wall_faces;if(!HeightOk(s,wf.first))++g.height_failures;}
  g.complete=!g.bad_shape&&!g.long_edges&&!g.height_failures&&!g.unmeasurable;return g;
}
inline std::string JsonFinal(State& s,const Rows& rows) {
  const auto g=Final(s);const auto cols=ColumnCells(s,rows);size_t longColumn=0,badFirst=0,badColumn=0,badOther=0;std::set<Edge> seen;double fq=1,fJ=1,herr=0;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;const auto sh=MeshShape(s,row.first);const bool first=WallChild(s,row.first);
    if(first){fq=std::min(fq,sh.q);fJ=std::min(fJ,sh.J);}
    if(Bad(sh))(first?badFirst:cols.count(row.first)?badColumn:badOther)++;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}
      if(l>MaximumMetricEdge){bool inColumn=true;for(Id id:Star(s,e))inColumn=inColumn&&cols.count(id);longColumn+=inColumn;}}}
  for(const auto& wf:WallFaces(s)){double w=0;HeightOk(s,wf.first,&w);herr=std::max(herr,w);}
  std::ostringstream o;o<<std::setprecision(10)<<"{\"cells\":"<<g.cells<<",\"q_min\":"<<g.q<<",\"J_min\":"<<g.J<<",\"edge_max_measured\":"<<double(g.lmax)<<",\"bad_shape\":"<<g.bad_shape
   <<",\"bad_first_child\":"<<badFirst<<",\"bad_column_other\":"<<badColumn<<",\"bad_outside_columns\":"<<badOther<<",\"long_edges\":"<<g.long_edges
   <<",\"long_by_class\":{\"wall\":"<<g.long_wall<<",\"seam\":"<<g.long_seam<<",\"other_physical\":"<<g.long_other_physical<<",\"interior\":"<<g.long_interior<<",\"inside_columns_only\":"<<longColumn<<"}"
   <<",\"wall_faces\":"<<g.wall_faces<<",\"height_failures\":"<<g.height_failures<<",\"first_child_q_min\":"<<fq<<",\"first_child_J_min\":"<<fJ<<",\"max_height_error\":"<<herr
   <<",\"unmeasurable_edges\":"<<g.unmeasurable<<",\"complete\":"<<g.complete<<'}';return o.str();
}

// DC-12: when set, every Admit8 caller (ApexMove, ColumnSplit, R1w, V) is admitted by this rule instead (Admit12).
inline std::function<Decision(State&,const Patch&,const Patch&,Kind)> admitOverride;
inline Decision Admit8(State& s,const Patch& old,const Patch& fresh,Kind kind) {
  if(admitOverride)return admitOverride(s,old,fresh,kind);
  Scope scope("Admit (D.1)");
  Decision base;base.old_cells=old.cells.size();base.fresh_cells=fresh.cells.size();
  if(old.cells.empty()||fresh.cells.empty()||old.cells.size()>MaximumReplacementCells||fresh.cells.size()>MaximumReplacementCells){base.category=5;base.reason="replacement size";return base;}
  std::string why;FirstHeightReport heightReport;if(!(validityOnlyConstruction?HeightConstruct(fresh,s.heights,*s.reference,&heightReport,&why):HeightT(s,fresh,why))){base.category=4;base.reason=why;return base;}
  for(const auto& c:fresh.cells)if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0)){base.category=2;base.reason="fresh cell has nonpositive exact orientation";return base;}
  PatchMetric after,before;
  try{auto reuse=ReuseOf(s,old.cells,&fresh.cells);
    after=MeasureMetricEdges(fresh.cells,*s.field,nullptr,&reuse);StoreLengths(s,after);
    for(const auto& e:after.edges)reuse.insert_or_assign(e.first,e.second);
    before=MeasureMetricEdges(old.cells,*s.field,nullptr,&reuse);StoreLengths(s,before);}
  catch(const std::exception& e){base.category=5;base.reason=e.what();return base;}
  auto shared=[](const std::map<Edge,EdgeMeasure>& others,const std::pair<const Edge,EdgeMeasure>& e){
    const auto o=others.find(e.first);return o!=others.end()&&SamePoint(o->second.a,e.second.a)&&SamePoint(o->second.b,e.second.b);};
  for(const auto& e:before.edges)if(!shared(after.edges,e))base.removed+=layer2d::ExcessOf(e.second.length);
  for(const auto& e:after.edges)if(!shared(before.edges,e))base.added+=layer2d::ExcessOf(e.second.length);
  const bool size_progress=base.removed>0&&base.added<base.removed*(1-1e-12L);

  Decision d=base;

  if(splitPolicy.rule==1&&(kind==Kind::Refine||kind==Kind::Layer)) {  // DC-11 2D split rule (SplitPolicy)
    const bool removes=RemovesVertex(old.cells,fresh.cells,splitPolicy.relocated,splitPolicy.relocatedCount);
    if(kind==Kind::Layer){if(!removes){++splitPolicy.layer_free;d.ok=true;return d;}++splitPolicy.layer_checked;
      if(base.added<=base.removed){d.ok=true;return d;}d.category=3;d.reason="layer: removes a vertex and adds frozen excess";return d;}
    if(removes){++splitPolicy.vertex_removal_checks;if(size_progress){d.ok=true;return d;}
      ++splitPolicy.vertex_removal_refusals;d.category=3;d.reason="refine: removes a vertex without strict changed-edge excess reduction";return d;}
    long double longest=splitPolicy.requested;
    if(longest<0){longest=0;for(const auto& e:before.edges)if(!shared(after.edges,e))longest=std::max(longest,e.second.length);}
    if(longest>(long double)SplitInsertionLength){++splitPolicy.size_insertions;d.ok=true;return d;}
    // 2D non-size insertion: shape improves and no interior (non-skin) fresh edge, changed or retained, exceeds 1.8
    ++splitPolicy.nonsize_checks;bool interiorOk=base.added==0;
    if(interiorOk){std::set<Edge> in;try{in=InteriorEdges(fresh.cells);}catch(const std::exception&){interiorOk=false;}
      for(const auto& e:after.edges)if(in.count(e.first)&&e.second.length>MaximumMetricEdge)interiorOk=false;}
    if(interiorOk){d.ok=true;return d;}
    ++splitPolicy.nonsize_refusals;d.category=3;d.reason="refine: non-size insertion adds an oversized interior edge or ";return d;
  }
  if(kind==Kind::Refine){if(fresh.cells.size()>old.cells.size()&&size_progress){d.ok=true;return d;}d.category=3;d.reason="refine: no strict changed-edge excess reduction";return d;}
  if(kind==Kind::Repair){d.ok=true;return d;}
  if(base.added<=base.removed){d.ok=true;return d;}d.category=3;d.reason="layer: adds frozen excess";return d;
}
// ---- V (d): existing volume operators outside columns; ApexMove / ColumnSplit inside columns --------------------------
// Exact on-edge binary64 point near the requested one (production OnEdge semantics: zero projected orientation on all
// three axes and inside the bounding box); none -> no SplitEdge trial for a non-physical edge.
inline bool OnEdgeExact(Point a,Point b,Point p) {
  auto pr=[](Point x,int k){return k==0?Point{x.y,x.z,0}:k==1?Point{x.x,x.z,0}:Point{x.x,x.y,0};};
  for(int k=0;k<3;++k)if(Orientation(pr(a,k),pr(b,k),pr(p,k),{0,0,1})!=0)return false;
  return p.x>=std::min(a.x,b.x)&&p.x<=std::max(a.x,b.x)&&p.y>=std::min(a.y,b.y)&&p.y<=std::max(a.y,b.y)&&p.z>=std::min(a.z,b.z)&&p.z<=std::max(a.z,b.z);
}
inline bool ExactOnEdgePoint(Point a,Point b,Point near,Point& out) {
  const double c[3]={near.x,near.y,near.z};
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)for(int dz=-1;dz<=1;++dz) {
    double p[3];const int d[3]={dx,dy,dz};
    for(int k=0;k<3;++k)p[k]=d[k]==0?c[k]:std::nextafter(c[k],d[k]>0?INFINITY:-INFINITY);
    const Point q{p[0],p[1],p[2]};if(SamePoint(q,a)||SamePoint(q,b))continue;if(OnEdgeExact(a,b,q)){out=q;return true;}}
  return false;
}
struct VStage : Stage { size_t column_demand=0,sweeps=0,apex_moves=0,column_splits=0,junction_splits=0,cache_skips=0,exact_splits=0,no_exact_point=0,junction_skips=0;std::map<std::string,size_t> demand; };
// DC-14: the column cells are evaluated live (a cell is a column cell iff it contains a registered apex: exactly
// ColumnCells = union of the apex stars) plus the inserted extra cells (fans); Build() refreshes the apex/lift maps only
// when the registry changed (the callers rebuild after every accepted operation, so live and snapshot views agree).
struct ColumnCellSet {
  const State* s=nullptr;std::set<Id> apices,extra;
  size_t count(Id id) const {
    if(extra.count(id))return 1;if(!s)return 0;const auto f=s->mesh.find(id);if(f==s->mesh.end())return 0;
    for(const auto& n:f->second.cell.v)if(apices.count(n.id))return 1;return 0;}
  void insert(Id id){extra.insert(id);}
};
struct ColumnIndex {
  ColumnCellSet cells;std::map<Id,Face> apexFace;std::map<Id,Id> unlift;
  std::map<Face,Id> lastApex;std::map<Id,Id> lastLift;bool built=false;
  bool Build(const State& s,const Rows& rows) {
    cells.s=&s;cells.extra.clear();
    if(built&&rows.apex==lastApex&&rows.lift==lastLift)return false;
    apexFace.clear();unlift.clear();cells.apices.clear();
    for(const auto& r:rows.apex){apexFace[r.second]=r.first;cells.apices.insert(r.second);}for(const auto& r:rows.lift)unlift[r.second]=r.first;
    lastApex=rows.apex;lastLift=rows.lift;built=true;return true;}
};
// ApexMove: Cone(star(p_f), p') with p' from the LP and a fixed grid inside the T window; Repair admission (D.1).
// A requested height correction is layer construction; an already compliant row is a quality repair.
inline Kind FirstRowKind(State& s,const Patch& old) {
  std::string why;
  return HeightT(s,old,why) ? Kind::Repair : Kind::Layer;
}

inline bool ApexMove(State& s,Rows& rows,const Face& key,VStage& g,std::string& why) {
  Scope scope("V ApexMove");
  const auto it=rows.apex.find(key);if(it==rows.apex.end()){why="not rowed";return false;}
  const auto star=StarOf(s,it->second);if(star.empty()||star.size()>MaximumCavityCells){why="column star";return false;}
  ++g.work;std::vector<Id> ids(star.begin(),star.end());Patch old;try{old=PatchOf(s,ids);}catch(const std::exception& x){why=x.what();return false;}
  std::vector<Face> skin;try{skin=Incidence(old.cells).Boundary();}catch(const std::exception& x){why=x.what();return false;}
  std::vector<Face> ordered;for(const auto& f:skin)if(Sorted(f)==key)ordered.push_back(f);if(ordered.size()!=1){why="column skin has no unique wall face";return false;}
  for(const auto& f:skin)if(Sorted(f)!=key)ordered.push_back(f);
  std::map<Id,Node> nodes;for(const auto& c:old.cells)for(auto n:c.v)nodes[n.id]=n;
  auto cone=ConeColumn(s,ordered,nodes,s.next_node,true);if(!cone.ok){why="ApexMove: "+cone.cls+" "+cone.witness;return false;}
  Patch fresh;fresh.cells=cone.cells;for(size_t i=0;i<fresh.cells.size();++i)fresh.cells[i].id=s.next_cell+i;fresh.surface=old.surface;fresh.parameters=old.parameters;
  std::string reason;if(!ValidatePrescribedInterface(old.cells,fresh.cells,skin,reason,nullptr,MaximumCavityCells)){why="ApexMove: "+reason;return false;}
  try{s.reference->ExportParameters(fresh);}catch(const std::exception& x){why=x.what();return false;}
  const auto d=Admit8(s,old,fresh,FirstRowKind(s,old));
  if(!d.ok){++g.rejected;++g.reasons["ApexMove ["+std::to_string(d.category)+"] "+Key(d.reason)];why="ApexMove: "+d.reason;return false;}
  Apply(s,ids,fresh);rows.apex[key]=cone.apex.id;++g.accepted;++g.apex_moves;return true;
}
// DC-9 (d): one-column ColumnSplit of a wall/carrier junction edge (a,b) (Codex DC-8 finding 6). Cavity: col(f) U edge
// star of the lid edge (a*,b*), which must contain every cell of (a,b). m on (a,b) is bound to the wall facet and the
// carrier facet(s) of (a,b); m* on (a*,b*) to the carrier facet(s) of the lid edge; the upper cells split at m*; f's
// column becomes two columns over the halves of f, coned from LP apices. Validation as an R2 transaction: artificial
// interface and the other wall faces unchanged, f replaced by its halves, every new carrier face inside its marker's
// authority (FindFace), the carrier boundary loop unchanged up to the inserted junction point m (L-C), prescribed
// ball topology, first height of both halves; D.1 Refine admission.
inline bool ColumnSplitJunction(State& s,Rows& rows,Edge ab,const Face& f,VStage& g,std::string& why,std::set<std::vector<Id>>& rejected) {
  Scope scope("V ColumnSplit junction");
  const Id a=ab[0],b=ab[1];const Id as=rows.lift.at(a),bs=rows.lift.at(b);const Id oldApex=rows.apex.at(f);
  std::set<Id> cavity;{const auto st=StarOf(s,oldApex);cavity.insert(st.begin(),st.end());}
  for(Id id:Star(s,{std::min(as,bs),std::max(as,bs)}))cavity.insert(id);
  for(Id id:Star(s,{a,b}))if(!cavity.count(id)){why="ColumnSplit junction: a cell of the wall edge lies outside the column";return false;}
  if(cavity.size()>MaximumCavityCells){why="ColumnSplit junction cavity exceeds 64";return false;}
  std::vector<Id> key{5,a,b,UINT64_MAX};key.insert(key.end(),cavity.begin(),cavity.end());
  if(rejected.count(key)){++g.cache_skips;++perf.cache_skips;why="cached rejection (unchanged cavity)";return false;}
  struct Reject{std::set<std::vector<Id>>& set;std::vector<Id> key;bool keep=true;~Reject(){if(keep)set.insert(key);}} guard{rejected,key};
  ++g.work;std::vector<Id> ids(cavity.begin(),cavity.end());Patch old;try{old=PatchOf(s,ids);}catch(const std::exception& x){why=x.what();return false;}
  auto priv=s.reference->PrivateParameters(old);const PlanarReference& ref=priv?*priv:*s.reference;
  std::map<Id,Node> nodes;for(const auto& c:old.cells)for(auto n:c.v)nodes[n.id]=n;
  std::map<Face,SurfaceFace> oldPhysical;for(const auto& sf:old.surface)oldPhysical[Sorted(sf.v)]=sf;
  auto markerOf=[&](Id facet){return s.reference->Get(facet).marker;};
  const auto wallIt=oldPhysical.find(f);if(wallIt==oldPhysical.end()){why="ColumnSplit junction: wall face outside the cavity";return false;}
  std::set<Id> carrierFacets,lidFacets;int carrier=-1;
  for(const auto& p:oldPhysical){const int mk=markerOf(p.second.facet);
    if(s.walls.count(mk)){if(p.first!=f&&Has(p.first,a)&&Has(p.first,b)){why="ColumnSplit junction: second wall face on the edge";return false;}continue;}
    const bool onWallEdge=Has(p.first,a)&&Has(p.first,b),onLid=Has(p.first,as)&&Has(p.first,bs);if(!onWallEdge&&!onLid)continue;
    if(carrier>=0&&carrier!=mk){why="ColumnSplit junction: several carrier markers on the junction";return false;}carrier=mk;
    (onWallEdge?carrierFacets:lidFacets).insert(p.second.facet);}
  if(carrierFacets.empty()){why="ColumnSplit junction: no carrier face on the wall edge";return false;}
  if(lidFacets.empty()){why="ColumnSplit junction: lid edge not on the carrier";return false;}
  Id next=s.next_node;Node m{next++,Point{}};
  try{m.p=s.field->SegmentPoint(nodes.at(a).p,nodes.at(b).p,.5);}catch(const std::exception&){const auto pa=nodes.at(a).p,pb=nodes.at(b).p;m.p={double(((long double)pa.x+pb.x)/2),double(((long double)pa.y+pb.y)/2),double(((long double)pa.z+pb.z)/2)};}
  {std::set<Id> inc=carrierFacets;inc.insert(wallIt->second.facet);try{m=Constrain(s,&ref,m,inc,inc);}catch(const std::exception& x){why=std::string("ColumnSplit junction binding: ")+x.what();return false;}}
  const Point pa=nodes.at(as).p,pb=nodes.at(bs).p;Node ms{next++,{double(((long double)pa.x+pb.x)/2),double(((long double)pa.y+pb.y)/2),double(((long double)pa.z+pb.z)/2)}};
  try{ms=Constrain(s,&ref,ms,lidFacets,lidFacets);}catch(const std::exception& x){why=std::string("ColumnSplit junction lid binding: ")+x.what();return false;}
  nodes[m.id]=m;nodes[ms.id]=ms;
  std::vector<Cell> fresh;
  for(const auto& c:old.cells){if(CellHas(c,oldApex))continue;
    if(!(CellHas(c,as)&&CellHas(c,bs))){why="ColumnSplit junction: unexpected non-column cell";return false;}
    Cell x=c,y=c;for(auto& n:x.v)if(n.id==bs)n=ms;for(auto& n:y.v)if(n.id==as)n=ms;
    if(!Positive(x.v)||!Positive(y.v)){why="ColumnSplit junction: lid edge split inverts an upper cell";return false;}fresh.push_back(x);fresh.push_back(y);}
  std::map<Id,Id> L=rows.lift;L[m.id]=ms.id;auto Lf=[&](Id x){const auto it=L.find(x);return it==L.end()?x:it->second;};
  const Face out=wallIt->second.v;Face f1=out,f2=out;for(auto& x:f1)if(x==b)x=m.id;for(auto& x:f2)if(x==a)x=m.id;
  std::map<Face,Id> newApex;const long double parentH0=FaceH0(s,f);  // the halves are not in the mesh yet: the parent's wall height
  for(const Face& h:{f1,f2}){const auto skin=ColumnSkin(h,Lf);auto cone=ConeColumn(s,skin.faces,nodes,next,false,parentH0);
    if(!cone.ok){why="ColumnSplit junction: "+cone.cls+" "+cone.witness;return false;}nodes[next]=cone.apex;newApex[Sorted(h)]=next++;fresh.insert(fresh.end(),cone.cells.begin(),cone.cells.end());}
  if(fresh.size()>MaximumReplacementCells){why="ColumnSplit junction exceeds 128 fresh cells";return false;}
  for(size_t i=0;i<fresh.size();++i)fresh[i].id=s.next_cell+i;
  std::vector<Face> newSkin,oldSkin;try{newSkin=Incidence(fresh).Boundary();oldSkin=Incidence(old.cells).Boundary();}catch(const std::exception& x){why=x.what();return false;}
  std::set<Face> oldOriented(oldSkin.begin(),oldSkin.end()),newOriented(newSkin.begin(),newSkin.end());
  auto oriented=[&](const std::set<Face>& set,Face t){for(int r=0;r<3;++r){if(set.count(t))return true;std::rotate(t.begin(),t.begin()+1,t.end());}return false;};
  std::map<std::pair<Id,Id>,int> oldLoop,newLoop;
  auto loop=[](std::map<std::pair<Id,Id>,int>& e,const Face& t){for(int i=0;i<3;++i){const Id x=t[i],y=t[(i+1)%3];const auto r=e.find({y,x});if(r!=e.end()){if(--r->second==0)e.erase(r);}else ++e[{x,y}];}};
  for(const auto& t:oldSkin){const auto p=oldPhysical.find(Sorted(t));
    if(p==oldPhysical.end()){if(!oriented(newOriented,t)){why="ColumnSplit junction: artificial interface changed";return false;}continue;}
    const int mk=markerOf(p->second.facet);
    if(s.walls.count(mk)){if(Sorted(t)!=f&&!oriented(newOriented,t)){why="ColumnSplit junction: wall face lost";return false;}continue;}
    if(mk==carrier)loop(oldLoop,t);}
  Patch outPatch;outPatch.cells=fresh;
  for(const auto& t:newSkin){const auto k=Sorted(t);const auto p=oldPhysical.find(k);
    if(p!=oldPhysical.end()&&oriented(oldOriented,t)){outPatch.surface.push_back({t,p->second.facet});if(markerOf(p->second.facet)==carrier)loop(newLoop,t);continue;}
    if(oriented(oldOriented,t))continue;
    const bool half=k==Sorted(f1)||k==Sorted(f2);const int mk=half?markerOf(wallIt->second.facet):carrier;Id facet=0;bool found=false;
    try{found=ref.FindFace(mk,{nodes.at(t[0]).p,nodes.at(t[1]).p,nodes.at(t[2]).p},facet);}catch(const std::exception&){found=false;}
    if(!found){std::ostringstream w;w<<"ColumnSplit junction: new "<<(half?"wall":"carrier")<<" face ("<<t[0]<<","<<t[1]<<","<<t[2]<<") leaves its chart";why=w.str();return false;}
    outPatch.surface.push_back({t,facet});if(!half)loop(newLoop,t);}
  for(Id p:{m.id,ms.id}){std::pair<Id,Id> in{0,0},o{0,0};int nin=0,nout=0;
    for(const auto& e:newLoop){if(e.first.second==p){in=e.first;++nin;}if(e.first.first==p){o=e.first;++nout;}}
    if(nin==1&&nout==1){newLoop.erase(in);newLoop.erase(o);const std::pair<Id,Id> joined{in.first,o.second},back{o.second,in.first};
      const auto r=newLoop.find(back);if(r!=newLoop.end()){if(--r->second==0)newLoop.erase(r);}else ++newLoop[joined];}}
  if(oldLoop!=newLoop){why="ColumnSplit junction: L-C carrier boundary loop changed";return false;}
  std::string reason;
  if(!ValidatePrescribedInterface(old.cells,fresh,newSkin,reason,nullptr,MaximumCavityCells)){why="ColumnSplit junction: "+reason;return false;}
  try{ref.ExportParameters(outPatch);}catch(const std::exception& x){why=x.what();return false;}
  for(const Face& h:{f1,f2}){Patch one;const auto k=Sorted(h);for(const auto& c:fresh){unsigned n=0;for(auto x:c.v)n+=Has(k,x.id);if(n==3)one.cells.push_back(c);}
    for(const auto& sf:outPatch.surface)if(Sorted(sf.v)==k)one.surface={sf};for(const auto& c:one.cells)for(auto x:c.v){const auto pr=outPatch.parameters.find(x.id);if(pr!=outPatch.parameters.end())one.parameters.insert(*pr);}
    FirstHeightReport r;if(one.cells.size()!=1||!CheckFirstHeight(one,s.heights,*s.reference,r,reason)){why="ColumnSplit junction height: "+reason;return false;}}
  const auto d=Admit8(s,old,outPatch,Kind::Refine);
  if(!d.ok){++g.rejected;++g.reasons["ColumnSplit junction ["+std::to_string(d.category)+"] "+Key(d.reason)];why="ColumnSplit junction: "+d.reason;return false;}
  guard.keep=false;Apply(s,ids,outPatch);rows.lift[m.id]=ms.id;rows.apex.erase(f);for(const auto& r:newApex)rows.apex[r.first]=r.second;
  ++g.accepted;++g.column_splits;++g.junction_splits;return true;
}
// ColumnSplit(a,b): cavity col(f) U col(g) U edge star of the lid edge (a*,b*); wall point m on (a,b) bound to both wall
// charts, m* on the lid edge; upper cells split at m*, four columns coned from LP apices; D.1 Refine admission.
inline bool ColumnSplit(State& s,Rows& rows,Edge ab,VStage& g,std::string& why,std::set<std::vector<Id>>& rejected) {
  Scope scope("V ColumnSplit");
  const Id a=ab[0],b=ab[1];const auto la=rows.lift.find(a),lb=rows.lift.find(b);if(la==rows.lift.end()||lb==rows.lift.end()){why="unlifted wall edge";return false;}
  std::vector<Face> faces;for(const auto& r:rows.apex)if(Has(r.first,a)&&Has(r.first,b))faces.push_back(r.first);
  for(const auto& f:faces)if(rows.overrides.count(f)){why="ColumnSplit: sector column (junction override)";return false;}
  if(faces.size()==1)return ColumnSplitJunction(s,rows,ab,faces.front(),g,why,rejected);
  if(faces.size()!=2){why="ColumnSplit needs two rowed faces on the wall edge (unrowed neighbour)";return false;}
  std::set<Id> cavity;for(const auto& f:faces){const auto st=StarOf(s,rows.apex.at(f));cavity.insert(st.begin(),st.end());}
  for(Id id:Star(s,{std::min(la->second,lb->second),std::max(la->second,lb->second)}))cavity.insert(id);
  if(cavity.size()>MaximumCavityCells){why="ColumnSplit cavity exceeds 64";return false;}
  std::vector<Id> key{4,a,b,UINT64_MAX};key.insert(key.end(),cavity.begin(),cavity.end());
  if(rejected.count(key)){++g.cache_skips;++perf.cache_skips;why="cached rejection (unchanged cavity)";return false;}
  struct Reject{std::set<std::vector<Id>>& set;std::vector<Id> key;bool keep=true;~Reject(){if(keep)set.insert(key);}} guard{rejected,key};
  ++g.work;std::vector<Id> ids(cavity.begin(),cavity.end());Patch old;try{old=PatchOf(s,ids);}catch(const std::exception& x){why=x.what();return false;}
  auto priv=s.reference->PrivateParameters(old);const PlanarReference& ref=priv?*priv:*s.reference;
  std::map<Id,Node> nodes;for(const auto& c:old.cells)for(auto n:c.v)nodes[n.id]=n;
  std::map<Face,SurfaceFace> wallOf;for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker))wallOf[Sorted(sf.v)]=sf;
  std::set<Id> facets;for(const auto& f:faces){const auto w=wallOf.find(f);if(w==wallOf.end()){why="wall face outside the cavity";return false;}facets.insert(w->second.facet);}
  Id next=s.next_node;Node m{next++,Point{}};
  try{m.p=s.field->SegmentPoint(nodes.at(a).p,nodes.at(b).p,.5);}catch(const std::exception&){const auto pa=nodes.at(a).p,pb=nodes.at(b).p;m.p={double(((long double)pa.x+pb.x)/2),double(((long double)pa.y+pb.y)/2),double(((long double)pa.z+pb.z)/2)};}
  try{m=Constrain(s,&ref,m,facets,facets);}catch(const std::exception& x){why=std::string("ColumnSplit binding: ")+x.what();return false;}
  const Point pa=nodes.at(la->second).p,pb=nodes.at(lb->second).p;const Node ms{next++,{double(((long double)pa.x+pb.x)/2),double(((long double)pa.y+pb.y)/2),double(((long double)pa.z+pb.z)/2)}};
  nodes[m.id]=m;nodes[ms.id]=ms;
  std::set<Id> oldApices;for(const auto& f:faces)oldApices.insert(rows.apex.at(f));
  std::vector<Cell> fresh;
  for(const auto& c:old.cells){bool col=false;for(auto n:c.v)col=col||oldApices.count(n.id);if(col)continue;
    if(!(CellHas(c,la->second)&&CellHas(c,lb->second))){why="ColumnSplit: unexpected non-column cell";return false;}
    Cell x=c,y=c;for(auto& n:x.v)if(n.id==lb->second)n=ms;for(auto& n:y.v)if(n.id==la->second)n=ms;
    if(!Positive(x.v)||!Positive(y.v)){why="ColumnSplit: lid edge split inverts an upper cell";return false;}fresh.push_back(x);fresh.push_back(y);}
  std::map<Id,Id> L=rows.lift;L[m.id]=ms.id;auto Lf=[&](Id x){const auto f=L.find(x);return f==L.end()?x:f->second;};
  std::vector<Face> newWall;std::map<Face,Id> newApex;std::map<Face,int> childMarker;
  for(const auto& f:faces){const Face out=wallOf.at(f).v;Face f1=out,f2=out;for(auto& x:f1)if(x==b)x=m.id;for(auto& x:f2)if(x==a)x=m.id;
    const long double parentH0=FaceH0(s,f);  // the halves are not in the mesh yet: the parent's wall height
    for(const Face& h:{f1,f2}){const auto skin=ColumnSkin(h,Lf);auto cone=ConeColumn(s,skin.faces,nodes,next,false,parentH0);
      if(!cone.ok){why="ColumnSplit: "+cone.cls+" "+cone.witness;return false;}nodes[next]=cone.apex;newApex[Sorted(h)]=next++;newWall.push_back(h);
      childMarker[Sorted(h)]=s.reference->Get(wallOf.at(f).facet).marker;fresh.insert(fresh.end(),cone.cells.begin(),cone.cells.end());}}
  if(fresh.size()>MaximumReplacementCells){why="ColumnSplit exceeds 128 fresh cells";return false;}
  for(size_t i=0;i<fresh.size();++i)fresh[i].id=s.next_cell+i;
  std::vector<Face> newSkin;try{newSkin=Incidence(fresh).Boundary();}catch(const std::exception& x){why=x.what();return false;}
  Patch out;out.cells=fresh;std::map<Face,SurfaceFace> oldPhysical;for(const auto& sf:old.surface)oldPhysical[Sorted(sf.v)]=sf;
  for(const auto& f:newSkin){const auto key=Sorted(f);const auto p=oldPhysical.find(key);if(p!=oldPhysical.end()){out.surface.push_back({f,p->second.facet});continue;}
    if(std::find_if(newWall.begin(),newWall.end(),[&](const Face& w){return Sorted(w)==key;})!=newWall.end()){Id facet=0;bool found=false;
      const int marker=childMarker.at(key);try{found=ref.FindFace(marker,{nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p},facet);}catch(const std::exception&){found=false;}
      if(!found){why="ColumnSplit: split wall face leaves its chart";return false;}out.surface.push_back({f,facet});}}
  // prescribed skin: the old skin with f,g replaced by their four halves (oriented, rotation-normalized)
  {auto canon=[](Face f){std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());return f;};std::set<Face> expected,got;
   try{for(const auto& f:Incidence(old.cells).Boundary())if(std::find(faces.begin(),faces.end(),Sorted(f))==faces.end())expected.insert(canon(f));}catch(const std::exception& x){why=x.what();return false;}
   for(const auto& w:newWall)expected.insert(canon(w));for(const auto& f:newSkin)got.insert(canon(f));if(expected!=got){why="ColumnSplit: changed interface";return false;}}
  std::string reason;
  if(!ValidatePrescribedInterface(old.cells,fresh,newSkin,reason,nullptr,MaximumCavityCells)){why="ColumnSplit: "+reason;return false;}
  try{ref.ExportParameters(out);}catch(const std::exception& x){why=x.what();return false;}
  for(const auto& w:newWall){Patch one;const auto key=Sorted(w);for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)one.cells.push_back(c);}
    for(const auto& sf:out.surface)if(Sorted(sf.v)==key)one.surface={sf};for(const auto& c:one.cells)for(auto n:c.v){const auto pr=out.parameters.find(n.id);if(pr!=out.parameters.end())one.parameters.insert(*pr);}
    FirstHeightReport r;if(one.cells.size()!=1||!CheckFirstHeight(one,s.heights,*s.reference,r,reason)){why="ColumnSplit height: "+reason;return false;}}
  const auto d=Admit8(s,old,out,Kind::Refine);
  if(!d.ok){++g.rejected;++g.reasons["ColumnSplit ["+std::to_string(d.category)+"] "+Key(d.reason)];why="ColumnSplit: "+d.reason;return false;}
  guard.keep=false;Apply(s,ids,out);rows.lift[m.id]=ms.id;for(const auto& f:faces)rows.apex.erase(f);for(const auto& r:newApex)rows.apex[r.first]=r.second;
  ++g.accepted;++g.column_splits;return true;
}
// Refine candidates of one edge outside columns (production order; balanced point once per edge, see header).
struct Candidate { std::string op;std::vector<Id> ids;Patch old,fresh; };
// (balancedCache: defined with the other caches above)
inline bool SameChartJunction(State& s,Edge e,bool& junction) {
  std::set<Id> charts;size_t n=0;
  for(Id id:Star(s,e)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)if(r.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1])){++n;charts.insert(s.reference->Canonical(r.physical[k].facet));}}
  junction=n>0&&charts.size()>1;return n>0;
}
inline std::vector<std::pair<std::string,std::function<bool(std::vector<Id>&,Patch&,Patch&,std::string&)>>> RefineVariantsOf(State& s,Edge e,VStage& g) {
  std::vector<std::pair<std::string,std::function<bool(std::vector<Id>&,Patch&,Patch&,std::string&)>>> out;
  bool junction=false;const bool physical=SameChartJunction(s,e,junction);bool wall=false;
  for(Id id:Star(s,e)){const auto& row=s.mesh.at(id);const auto f=CellFaces(row.cell);for(unsigned k=0;k<4;++k)
    if(row.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1])&&s.walls.count(s.reference->Get(row.physical[k].facet).marker))wall=true;}
  if(wall){for(int variant=0;variant<3;++variant)out.push_back({"wall",[&s,e,variant](std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why){std::string op;return RefineProposal(s,e,ids,old,fresh,op,why,variant);}});return out;}
  const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);
  auto balanced=[&s,e,a,b]()->Point{const std::array<double,8> key{double(e[0]),double(e[1]),a.x,a.y,a.z,b.x,b.y,b.z};const auto f=balancedCache.find(key);if(f!=balancedCache.end())return f->second;
    Point p{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
    try{Scope sc("V BalancedSplitPoint");p=s.field->BalancedSplitPoint(a,b);}catch(const std::exception&){}balancedCache[key]=p;return p;};
  auto star=[&s,e](std::vector<Id>& ids,Patch& old){ids=Star(s,e);if(ids.empty()||ids.size()>MaximumCavityCells)return false;old=PatchOf(s,ids);return true;};
  if(!physical) {
    const Point p=balanced();Point exact;
    if(ExactOnEdgePoint(a,b,p,exact)){++g.exact_splits;out.push_back({"SplitEdge",[&s,e,exact,star](std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why){
      if(!star(ids,old)){why="edge star exceeds 64";return false;}Scope sc("V SplitEdge");return SplitEdge(old,e,{s.next_node,exact},s.next_cell,*s.reference,fresh,why);}});}
    else ++g.no_exact_point;
    out.push_back({"InsertEdgeCavity",[&s,e,p,star](std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why){
      if(!star(ids,old)){why="edge star exceeds 64";return false;}Scope sc("V InsertEdgeCavity");return InsertEdgeCavity(old,e,{s.next_node,p},s.next_cell,*s.reference,fresh,why);}});
    return out;
  }
  Point central{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};try{central=s.field->SegmentPoint(a,b,.5);}catch(const std::exception&){}
  const Point p=balanced();
  for(const Point& q:{p,central})out.push_back({"SplitEdge",[&s,e,q,star](std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why){
    if(!star(ids,old)){why="edge star exceeds 64";return false;}Scope sc("V SplitEdge");return SplitEdge(old,e,{s.next_node,q},s.next_cell,*s.reference,fresh,why);}});
  if(junction)++g.junction_skips;
  else for(const Point& q:{p,central})out.push_back({"InsertEdgeCavity",[&s,e,q,star](std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& why){
    if(!star(ids,old)){why="edge star exceeds 64";return false;}Scope sc("V InsertEdgeCavity");return InsertEdgeCavity(old,e,{s.next_node,q},s.next_cell,*s.reference,fresh,why);}});
  return out;
}
inline VStage PhaseV(State& s,Rows& rows,size_t work,double seconds,std::ostream& log) {
  Scope scope("PhaseV");
  VStage g;Begin(s,g);const auto t0=Clock::now();
  auto limited=[&]{return g.work>=work||Seconds(t0)>=seconds;};
  std::set<std::vector<Id>> rejected;
  auto keyOf=[&](Id tag,const std::vector<Id>& head,const std::vector<Id>& ids){std::vector<Id> k{tag};k.insert(k.end(),head.begin(),head.end());k.push_back(UINT64_MAX);k.insert(k.end(),ids.begin(),ids.end());std::sort(k.begin()+head.size()+2,k.end());return k;};
  auto attempt=[&](const std::string& op,Kind kind,const std::vector<Id>& ids,const Patch& old,const Patch& fresh)->bool{
    ++g.work;const auto d=Admit8(s,old,fresh,kind);
    if(!d.ok){++g.rejected;++g.reasons[op+" ["+std::to_string(d.category)+"] "+Key(d.reason)];g.last=op+": "+d.reason;return false;}
    Apply(s,ids,fresh);++g.accepted;return true;};
  ColumnIndex ci;
  for(;;++g.sweeps) {
    if(limited()){g.outcome="limit";break;}
    ci.Build(s,rows);bool changed=false;g.column_demand=0;g.demand.clear();
    auto touchesColumn=[&](const std::vector<Id>& ids){for(Id id:ids)if(ci.cells.count(id))return true;return false;};
    // wall-first: wall/seam edges, other physical, interior; longest first in each class
    std::vector<std::tuple<int,long double,Edge>> longs;std::set<Edge> seen;
    for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
      const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){continue;}if(l>MaximumMetricEdge){const int k=EdgeClass(s,e);longs.emplace_back(k==1||k==2?0:k==3?1:2,-l,e);}}}
    std::sort(longs.begin(),longs.end());
    std::set<Face> movedThisSweep;
    for(const auto& row:longs) {
      if(limited())break;const Edge e=std::get<2>(row);const auto star=Star(s,e);if(star.empty())continue;
      if(touchesColumn(star)) {
        // route column demand: lid/lateral edge of a wall edge (a,b) -> ColumnSplit; apex edge -> ApexMove, then ColumnSplit
        auto base=[&](Id x)->Id{const auto u=ci.unlift.find(x);if(u!=ci.unlift.end())return u->second;return rows.lift.count(x)?x:UINT64_MAX;};
        const Id b0=base(e[0]),b1=base(e[1]);bool done=false;std::string why;
        Face apexKey{};bool apex=false;for(Id x:e){const auto f=ci.apexFace.find(x);if(f!=ci.apexFace.end()){apexKey=f->second;apex=true;}}
        if(apex){if(!movedThisSweep.count(apexKey)){movedThisSweep.insert(apexKey);const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
            if(rejected.count(k)){++g.cache_skips;++perf.cache_skips;why="cached rejection (unchanged cavity)";}else{done=ApexMove(s,rows,apexKey,g,why);if(!done)rejected.insert(k);}}
          if(!done){long double best=-1;Edge w{0,0};for(int i=0;i<3;++i){const Id x=apexKey[i],y=apexKey[(i+1)%3];long double l=0;try{l=Len(s,x,y);}catch(const std::exception&){l=INFINITY;}if(l>best){best=l;w={std::min(x,y),std::max(x,y)};}}
            done=ColumnSplit(s,rows,w,g,why,rejected);}}
        else if(b0!=UINT64_MAX&&b1!=UINT64_MAX&&b0!=b1){const Edge w{std::min(b0,b1),std::max(b0,b1)};done=ColumnSplit(s,rows,w,g,why,rejected);}
        else why="column demand without a column route";
        if(done){changed=true;ci.Build(s,rows);}else{++g.column_demand;++g.demand[Key(why)];g.last="column demand: "+why;}
        continue;
      }
      const auto key=keyOf(1,{e[0],e[1]},star);if(rejected.count(key)){++g.cache_skips;++perf.cache_skips;continue;}
      bool taken=false;
      for(auto& variant:RefineVariantsOf(s,e,g)) {
        if(limited())break;std::vector<Id> ids;Patch old,fresh;std::string reason;bool built=false;
        try{built=variant.second(ids,old,fresh,reason);}catch(const std::exception& x){reason=x.what();built=false;}
        if(!built){++g.work;++g.rejected;++g.reasons["Refine build: "+Key(reason)];continue;}
        if(touchesColumn(ids)){++g.rejected;++g.reasons["Refine: protected column"];continue;}
        Patch repaired;ConnectivityRepairReport report;std::string why;bool better=false;
        {Scope sc("V RepairMetricConnectivity(fresh)");try{better=RepairMetricConnectivity(fresh,*s.field,*s.reference,s.heights,s.next_cell+4096,repaired,report,why,128,false);}catch(const std::exception&){better=false;}}
        if(better){for(size_t i=0;i<repaired.cells.size();++i)repaired.cells[i].id=s.next_cell+i;try{s.reference->ExportParameters(repaired);}catch(const std::exception&){better=false;}}
        {Scope sc("V Admit");if(better&&attempt(variant.first+"+repair",Kind::Refine,ids,old,repaired)){taken=true;break;}
         if(attempt(variant.first,Kind::Refine,ids,old,fresh)){taken=true;break;}}
      }
      if(!taken)rejected.insert(key);
      changed|=taken;if(taken)ci.Build(s,rows);
    }
    // bad cells: column cells -> ApexMove; others -> RepairMetricConnectivity outside columns (as DC-7)
    std::vector<std::pair<double,Id>> bad;for(const auto& row:s.mesh){const auto sh=MeshShape(s,row.first);if(Bad(sh))bad.emplace_back(sh.q,row.first);}
    std::sort(bad.begin(),bad.end());
    for(const auto& row:bad) {
      if(limited())break;if(!s.mesh.count(row.second))continue;
      if(ci.cells.count(row.second)){Face apexKey{};bool found=false;for(auto n:s.mesh.at(row.second).cell.v){const auto f=ci.apexFace.find(n.id);if(f!=ci.apexFace.end()){apexKey=f->second;found=true;}}
        if(!found||movedThisSweep.count(apexKey))continue;movedThisSweep.insert(apexKey);
        const auto st=StarOf(s,rows.apex.at(apexKey));const auto k=keyOf(3,{apexKey[0],apexKey[1],apexKey[2]},std::vector<Id>(st.begin(),st.end()));
        if(rejected.count(k)){++g.cache_skips;++perf.cache_skips;continue;}std::string why;
        if(ApexMove(s,rows,apexKey,g,why)){changed=true;ci.Build(s,rows);}else{rejected.insert(k);++g.demand["bad column cell: "+Key(why)];}
        continue;}
      std::set<Id> star;for(auto n:s.mesh.at(row.second).cell.v)for(Id id:StarOf(s,n.id))if(!ci.cells.count(id))star.insert(id);
      if(star.size()>MaximumCavityCells){star.clear();const auto& c=s.mesh.at(row.second).cell;for(Id id:Star(s,{std::min(c.v[0].id,c.v[1].id),std::max(c.v[0].id,c.v[1].id)}))if(!ci.cells.count(id))star.insert(id);}
      if(star.empty()||star.size()>MaximumCavityCells){++g.work;++g.rejected;++g.reasons["Repair build: star exceeds 64"];continue;}
      std::vector<Id> ids(star.begin(),star.end());const auto key=keyOf(2,{row.second},ids);if(rejected.count(key)){++g.cache_skips;++perf.cache_skips;continue;}
      Patch old,fresh;std::string reason;ConnectivityRepairReport report;
      std::set<Id> movable,frozen=rows.Apices();for(const auto& r:rows.lift){frozen.insert(r.first);frozen.insert(r.second);}
      for(Id id:ids)for(auto n:s.mesh.at(id).cell.v)if(!frozen.count(n.id))movable.insert(n.id);
      bool ok=false;{Scope sc("V RepairMetricConnectivity(bad)");try{old=PatchOf(s,ids);ok=RepairMetricConnectivity(old,*s.field,*s.reference,s.heights,s.next_cell,fresh,report,reason,64,true,&movable);}catch(const std::exception& x){reason=x.what();}}
      if(!ok){++g.work;++g.rejected;++g.reasons["Repair build: "+Key(reason)];rejected.insert(key);continue;}
      // production repair moves vertices in place and may keep cell identities: fresh identities keep every cache valid
      for(size_t i=0;i<fresh.cells.size();++i)fresh.cells[i].id=s.next_cell+i;try{s.reference->ExportParameters(fresh);}catch(const std::exception&){}
      bool acc=false;{Scope sc("V Admit");acc=attempt("RepairMetricConnectivity",Kind::Repair,ids,old,fresh);}
      if(acc){changed=true;ci.Build(s,rows);}else rejected.insert(key);
    }
    const auto gates=Final(s);
    log<<"DC9 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" height_fail="<<gates.height_failures<<" column_demand="<<g.column_demand<<" work="<<g.work<<" cache_skips="<<g.cache_skips<<" t="<<Seconds(t0)<<"\n";
    std::cout<<"DC9 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" work="<<g.work<<" t="<<Seconds(t0)<<std::endl;
    if(gates.complete){g.outcome="complete";break;}
    if(limited()){g.outcome="limit";break;}
    if(!changed){g.outcome="blocked";break;}
  }
  End(s,g,t0);return g;
}
// Final mesh with the native gates (as DC-7) from the cached measures.
inline void ExportGates(State& s,const Rows& rows,const std::string& path) {
  Scope scope("ExportGates");
  Export(s,path);std::ofstream out(path,std::ios::app);out<<std::setprecision(17);
  std::map<Id,Id> column;for(const auto& r:rows.apex)for(Id id:StarOf(s,r.second))column.emplace(id,r.second);
  out<<"cell_gates "<<s.mesh.size()<<"\n";
  for(const auto& row:s.mesh){const auto sh=MeshShape(s,row.first);const bool first=WallChild(s,row.first);const auto c=column.find(row.first);
    out<<row.first<<' '<<sh.q<<' '<<sh.J<<' '<<Bad(sh)<<' '<<first<<' '<<(c==column.end()?-1:(long long)c->second)<<' '<<(first?1:c!=column.end()?2:0)<<"\n";}
  std::vector<std::string> lines;std::set<Edge> seen;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
    const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
    long double l=-1;bool ok=true;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){ok=false;}
    if(ok&&!(l>MaximumMetricEdge))continue;
    bool inColumn=true;for(Id id:Star(s,e))inColumn=inColumn&&column.count(id);
    const int cls=!ok?4:EdgeClass(s,e);std::ostringstream x;x<<std::setprecision(10)<<e[0]<<' '<<e[1]<<' '<<(ok?double(l):-1.0)<<' '<<cls<<' '<<inColumn;lines.push_back(x.str());}}
  out<<"long_edges "<<lines.size()<<"\n";for(const auto& l:lines)out<<l<<"\n";
}
inline std::string JsonPerf(){std::ostringstream o;o<<std::setprecision(8)<<"{\"ops\":{";bool k=false;
  std::vector<std::pair<double,std::string>> top;for(const auto& r:perf.ops)top.emplace_back(r.second.second,r.first);std::sort(top.rbegin(),top.rend());
  for(const auto& t:top){const auto& r=perf.ops.at(t.second);o<<(k?",":"")<<junction_wp0::Json(t.second)<<":{\"count\":"<<r.first<<",\"seconds\":"<<r.second<<",\"ms_per_op\":"<<1e3*r.second/std::max<size_t>(1,r.first)<<'}';k=true;}
  o<<"},\"caches\":{\"length_hits\":"<<perf.length_hits<<",\"length_misses\":"<<perf.length_misses<<",\"length_failures\":"<<perf.length_failures<<",\"shape_hits\":"<<perf.shape_hits
   <<",\"shape_misses\":"<<perf.shape_misses<<",\"height_hits\":"<<perf.height_hits<<",\"height_misses\":"<<perf.height_misses<<",\"rejection_cache_skips\":"<<perf.cache_skips<<",\"geometry_length_hits\":"<<geometryHits<<"}}";return o.str();}
}  // namespace dc9
