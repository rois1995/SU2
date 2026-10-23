// BL_FIRST_ROW_DESIGN rev 2.1, DC-8: TEST-ONLY [NativeFirstRowDC8] harness. Extends DC-7 (W -> R1 -> R2 -> V) by
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
// and absolute; the final mesh is re-audited with a fresh field and no caches. Nothing here is production code.
#include <sys/resource.h>
#include <unordered_map>
namespace dc8 {
using namespace layer2d;
using Clock=std::chrono::steady_clock;
constexpr long double Kappa=1.2L,Beta=1e-3L,LowBand=1.3L,HighBand=2.2L,BandMargin=0.05L,StationTarget=1.45L,ShortWallEdge=0.25L;
double Seconds(Clock::time_point t){return std::chrono::duration<double>(Clock::now()-t).count();}
long double Det(V a,V b,V c,V d){return Dot(b-a,Cross(c-a,d-a));}
long double Sigma(){static const long double s=Orientation({0,0,0},{1,0,0},{0,1,0},{0,0,1})>0?1.L:-1.L;return s;}
long double SDet(V a,V b,V c,V d){return Sigma()*Det(a,b,c,d);}
V Outward(V a,V b,V c){return Sigma()*Unit(Cross(b-a,c-a));}
bool Positive(const std::array<Node,4>& v){return Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0;}

// ---- performance accounting --------------------------------------------------------------------------------------
struct Perf {
  std::map<std::string,std::pair<size_t,double>> ops;
  size_t length_hits=0,length_misses=0,length_failures=0,shape_hits=0,shape_misses=0,height_hits=0,height_misses=0,cache_skips=0;
};
Perf perf;
struct Scope {
  const char* key;Clock::time_point t0;
  explicit Scope(const char* k):key(k),t0(Clock::now()){}
  ~Scope(){auto& r=perf.ops[key];++r.first;r.second+=Seconds(t0);}
};
long PeakKB(){struct rusage u{};getrusage(RUSAGE_SELF,&u);return u.ru_maxrss;}
struct Counters { uint64_t queries=0,misses=0,compositions=0,edges=0,edge_compositions=0,edge_pieces=0; };
Counters CountersOf(State& s){const auto& t=s.field->Statistics();return {t.requests,t.misses,t.compositions,t.edge_requests,t.edge_compositions,t.edge_pieces};}
std::string JsonCounters(const Counters& a,const Counters& b){std::ostringstream o;o<<"{\"queries\":"<<b.queries-a.queries<<",\"query_misses\":"<<b.misses-a.misses
  <<",\"compositions\":"<<b.compositions-a.compositions<<",\"edge_integrations\":"<<b.edges-a.edges<<",\"edge_compositions\":"<<b.edge_compositions-a.edge_compositions
  <<",\"edge_pieces\":"<<b.edge_pieces-a.edge_pieces<<'}';return o.str();}

// ---- caches keyed by immutable identities --------------------------------------------------------------------------
std::unordered_map<Id,CellShape> shapeCache;
std::unordered_map<Id,std::pair<bool,double>> heightCache;
bool cachesOn=true;
void ResetCaches(){shapeCache.clear();heightCache.clear();}
CellShape Shape(State& s,const Cell& c){try{return ShapeOf(s,c);}catch(const std::exception&){return {-1,-1};}}
bool Bad(const CellShape& sh){return sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian;}
CellShape MeshShape(State& s,Id id) {
  if(cachesOn){const auto f=shapeCache.find(id);if(f!=shapeCache.end()){++perf.shape_hits;return f->second;}}
  ++perf.shape_misses;const auto sh=Shape(s,s.mesh.at(id).cell);if(cachesOn)shapeCache.emplace(id,sh);return sh;
}
// Composed edge length; an unmeasurable edge is cached as -1 and rethrown (no repeated failing integrations).
long double Len(State& s,Id a,Id b) {
  const auto key=std::minmax(a,b);const auto pa=NodePoint(s,key.first),pb=NodePoint(s,key.second);
  const auto found=s.lengths.find({key.first,key.second});
  if(found!=s.lengths.end()&&SamePoint(found->second.first[0],pa)&&SamePoint(found->second.first[1],pb)) {
    ++perf.length_hits;if(found->second.second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return found->second.second;}
  ++perf.length_misses;
  try{const auto l=s.field->TargetEdgeLength(pa,pb);s.lengths[{key.first,key.second}]={{pa,pb},l};return l;}
  catch(const std::exception&){++perf.length_failures;s.lengths[{key.first,key.second}]={{pa,pb},-1.L};throw;}
}
bool HeightOk(State& s,Id cell,double* worst=nullptr) {
  if(cachesOn){const auto f=heightCache.find(cell);if(f!=heightCache.end()){++perf.height_hits;if(worst)*worst=f->second.second;return f->second.first;}}
  ++perf.height_misses;std::string why;double w=0;bool ok=false;
  try{ok=HeightT(s,PatchOf(s,{cell}),why,&w);}catch(const std::exception&){ok=false;}
  if(cachesOn)heightCache.emplace(cell,std::make_pair(ok,w));if(worst)*worst=w;return ok;
}

// ---- mesh helpers (as DC-7) ----------------------------------------------------------------------------------------
struct Rows {
  std::map<Id,Id> lift;
  std::map<Face,Id> apex;
  std::set<Id> Apices() const {std::set<Id> a;for(const auto& r:apex)a.insert(r.second);return a;}
};
struct WF { Face key{},out{};Id facet=0,owner=0;int marker=0; };
std::vector<WF> PhysicalOf(const State& s,const std::set<Id>& cells,bool wall) {
  std::vector<WF> out;std::set<Face> seen;
  for(Id id:cells){const auto found=s.mesh.find(id);if(found==s.mesh.end())continue;const auto& row=found->second;const auto f=CellFaces(row.cell);
    for(unsigned k=0;k<4;++k)if(row.physical[k].present){const int m=s.reference->Get(row.physical[k].facet).marker;
      if(bool(s.walls.count(m))!=wall)continue;const auto key=Sorted(f[k]);if(!seen.insert(key).second)continue;
      out.push_back({key,f[k],row.physical[k].facet,id,m});}}
  return out;
}
std::set<Id> StarOf(const State& s,Id v){const auto f=s.vertex_cells.find(v);return f==s.vertex_cells.end()?std::set<Id>{}:f->second;}
std::set<Id> WallVertices(const State& s){std::set<Id> out;for(const auto& f:WallFaces(s))out.insert(f.second.begin(),f.second.end());return out;}
std::set<Id> ColumnCells(const State& s,const Rows& rows){std::set<Id> out;for(const auto& r:rows.apex){const auto st=StarOf(s,r.second);out.insert(st.begin(),st.end());}return out;}
bool Has(const Face& f,Id v){return f[0]==v||f[1]==v||f[2]==v;}
bool Rim(const State& s,Id v) {
  for(Id id:StarOf(s,v)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
    if(r.physical[k].present&&Has(f[k],v)&&s.markers.at(s.reference->Get(r.physical[k].facet).marker)=="CUT")return true;}
  return false;
}
bool RimFace(const State& s,const Face& f){for(Id x:f)if(Rim(s,x))return true;return false;}
bool CellHas(const Cell& c,Id v){for(auto n:c.v)if(n.id==v)return true;return false;}

// ---- (b) D1 crease geometry, test-side detection with the production rule (BuildFeatures, CBoundaryLayerMetric:1386)
struct Crease { V a,b,t;long double L=0; };
struct D1Geometry { std::vector<Crease> creases;long double slope=0,radius=0,h0=0,angle=0,scale=1; };
D1Geometry d1;
void DetectCreases(const State& s) {
  d1=D1Geometry{};const auto& F=s.layers->GetFeatures();d1.slope=F.slope;d1.radius=F.maximumRadius;d1.h0=s.h0;d1.angle=F.angle;
  for(const auto& f:s.facets)for(auto x:f.v)d1.scale=std::max({d1.scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});
  if(!(d1.slope>0))return;
  auto kind=[&](const Facet& f){if(s.walls.count(f.marker))return 1;std::string m=s.markers.at(f.marker);for(auto& c:m)c=char(std::tolower(c));
    if(m=="cut"||m.find("sym")!=std::string::npos)return 0;return 2;};  // reflective images are smooth continuations here
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
long double Rho(V x){long double best=INFINITY;for(const auto& c:d1.creases){const auto sp=std::clamp(Dot(x-c.a,c.t),0.L,c.L);best=std::min(best,Norm(x-(c.a+sp*c.t)));}return best;}
long double Phi(long double rho){return std::log1p(d1.slope*rho/d1.h0)/d1.slope;}
long double RhoOf(long double phi){return d1.h0/d1.slope*std::expm1(phi*d1.slope);}
bool OnCrease(V p,V q) {
  const long double tol=1e-9L*d1.scale;
  for(const auto& c:d1.creases){auto on=[&](V x){const auto sp=Dot(x-c.a,c.t);return sp>=-tol&&sp<=c.L+tol&&Norm(x-(c.a+sp*c.t))<=tol;};if(on(p)&&on(q))return true;}
  return false;
}

// ---- lift direction (as DC-7) --------------------------------------------------------------------------------------
bool MinNormAny(const std::vector<V>& n,V& out) {
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
Dir DirectionAt(State& s,Id v) {
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
Kernel KernelBound(const std::vector<Cell>& cells,Id v,V d) {
  Kernel k;
  for(const auto& c:cells){std::array<V,4> p;int at=-1;for(unsigned i=0;i<4;++i){p[i]=Of(c.v[i].p);if(c.v[i].id==v)at=int(i);}if(at<0)continue;
    const auto o0=SDet(p[0],p[1],p[2],p[3]);auto q=p;q[at]=q[at]+d;const auto slope=SDet(q[0],q[1],q[2],q[3])-o0;
    if(slope<0){const auto t=o0/(-slope);if(t<k.delta){k.delta=t;k.cell=c.id;}}}
  return k;
}
std::vector<Cell> MovedStar(const State& s,const std::set<Id>& apices,Id v) {
  std::vector<Cell> out;
  for(Id id:StarOf(s,v)){const auto& c=s.mesh.at(id).cell;bool col=false;for(auto n:c.v)col=col||apices.count(n.id);if(!col)out.push_back(c);}
  return out;
}
long double Need(const State& s){return (1+HeightTolerance)*s.h0*Kappa;}
long double MinimumBand(const State& s){return (1-HeightTolerance)*s.h0*(1+BandMargin);}
struct Plan { bool ok=false;Dir dir;Kernel kernel;long double H=0,delta=0;V p;std::string why; };
// (c) band height: preferred clamp(0.9 delta_K s*, [1.3,2.2]h0); below 1.3h0 the clearance value itself, floored at the
// height-window top (1+T)h0 (DC-7) or, in the low-band retry, at the feasible LP window (1-T)h0(1+5%). The metric cap
// (vertical column edge <= 1.5) needs edge integrations and is evaluated only for an actual lift (metricCap).
Plan PlanLift(State& s,const std::set<Id>& apices,Id v,bool metricCap,bool low) {
  Scope scope("PlanLift");
  Plan pl;pl.dir=DirectionAt(s,v);if(!pl.dir.ok){pl.why=pl.dir.why;return pl;}
  pl.kernel=KernelBound(MovedStar(s,apices,v),v,pl.dir.d);
  const long double h0=s.h0;pl.H=std::min(0.9L*pl.kernel.delta*pl.dir.s,HighBand*h0);
  if(low)pl.H=std::min(pl.H,(1+HeightTolerance)*h0);
  if(pl.H<LowBand*h0)pl.H=std::max(pl.H,low?MinimumBand(s):(1+HeightTolerance)*h0);
  const V x=Of(NodePoint(s,v));
  if(metricCap)for(unsigned k=0;k<12&&pl.H>LowBand*h0;++k){long double l=0;try{l=s.field->TargetEdgeLength(Pt(x),Pt(x+(pl.H/pl.dir.s)*pl.dir.d));}catch(const std::exception&){break;}
    if(l<=1.5L)break;pl.H=std::max(LowBand*h0,0.9L*pl.H);}
  pl.delta=pl.H/pl.dir.s;pl.p=x+pl.delta*pl.dir.d;pl.ok=true;return pl;
}
long double Deficit(const State& s,const Plan& p){return p.dir.ok?std::max(0.L,Need(s)-p.kernel.delta*p.dir.s):Need(s);}
Node Constrain(const State& s,const PlanarReference* priv,Node node,const std::set<Id>& carriers,const std::set<Id>& bind) {
  if(carriers.empty())return node;
  if(s.mapped){if(!priv)throw std::runtime_error("mapped lift without a private context");return priv->BindNewPoint(node,bind.empty()?carriers:bind);}
  std::set<Id> comps;for(Id f:carriers)comps.insert(s.component.at(f));
  V p=Of(node.p);bool all=true;for(Id c:comps)all=all&&s.snap.count(c);
  if(all){node.p=Pt(junction_wp0::Snap(s,comps,p));return node;}
  for(unsigned pass=0;pass<4;++pass)for(Id f:carriers)node.p=s.reference->Project(f,node.p);
  return node;
}

// ---- L-B' apex LP (as DC-7) and column skins -----------------------------------------------------------------------
struct LP { bool ok=false;long double slack=-INFINITY,l1=0,l2=0,eta=0; };
LP ApexLP(const std::vector<std::array<V,3>>& tris,V a,V b,V c,V n,long double h0,long double T,bool fixH) {
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
void Lateral(Id x0,Id x1,Id y0,Id y1,std::vector<Face>& out) {
  if(y0==x0&&y1==x1)return;
  if(x0<x1){if(y1!=x1)out.push_back({x1,x0,y1});if(y0!=x0)out.push_back({x0,y0,y1});}
  else{if(y0!=x0)out.push_back({x1,x0,y0});if(y1!=x1)out.push_back({x1,y0,y1});}
}
struct Skin { std::vector<Face> faces;std::array<std::vector<Face>,3> lateral; };
// Outward column skin of f_out; optional (a) override: on the junction edge {ovV,ovU} the vertex ovV lifts to ovP.
Skin ColumnSkin(const Face& f,const std::function<Id(Id)>& L,Id ovV=UINT64_MAX,Id ovU=UINT64_MAX,Id ovP=UINT64_MAX) {
  Skin out;out.faces.push_back(f);bool any=false;for(Id x:f)any=any||L(x)!=x;if(!any)return out;
  out.faces.push_back({L(f[2]),L(f[1]),L(f[0])});
  for(unsigned i=0;i<3;++i){const Id x0=f[i],x1=f[(i+1)%3];Id y0=L(x0),y1=L(x1);
    if(ovV!=UINT64_MAX&&((x0==ovV&&x1==ovU)||(x1==ovV&&x0==ovU))){if(x0==ovV)y0=ovP;else y1=ovP;}
    Lateral(x0,x1,y0,y1,out.lateral[i]);out.faces.insert(out.faces.end(),out.lateral[i].begin(),out.lateral[i].end());}
  return out;
}
// Cone of one column skin (skin[0] = wall face, outward) from an apex of the exact LP; best first child, then column q.
// spread: also a fixed grid of feet x heights inside the T window (ApexMove, and R2 when the LP points fail U1 shape).
// D.2 parent/child invariant (Prepare kind and unrebuilt faces): a fresh wall face whose first child fails T or the
// absolute first-child q/J is admitted only if every old wall face it overlaps (shares >= 2 vertices) failed as well.
bool FirstChildPasses(State& s,const Patch& p,const SurfaceFace& sf) {
  Patch one;const auto key=Sorted(sf.v);for(const auto& c:p.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)one.cells.push_back(c);}
  if(one.cells.size()!=1)return false;one.surface={sf};one.parameters=p.parameters;std::string why;
  bool ok=false;try{ok=HeightT(s,one,why);}catch(const std::exception&){ok=false;}
  return ok&&!Bad(Shape(s,one.cells.front()));
}
bool D2Ok(State& s,const Patch& old,const Patch& fresh,std::string& why) {
  Scope scope("D.2 invariant");
  std::vector<std::pair<Face,bool>> parents;std::set<Face> oldKeys;
  for(const auto& sf:old.surface)if(s.walls.count(s.reference->Get(sf.facet).marker)){parents.push_back({Sorted(sf.v),FirstChildPasses(s,old,sf)});oldKeys.insert(Sorted(sf.v));}
  for(const auto& sf:fresh.surface){if(!s.walls.count(s.reference->Get(sf.facet).marker))continue;const auto key=Sorted(sf.v);
    bool anyPass=false;for(const auto& p:parents){unsigned k=0;for(Id x:p.first)k+=Has(key,x);if(k>=2&&p.second)anyPass=true;}
    if(!anyPass)continue;
    if(!FirstChildPasses(s,fresh,sf)){why="D.2: wall face "+std::to_string(key[0])+","+std::to_string(key[1])+","+std::to_string(key[2])+" loses a passing first child";return false;}}
  return true;
}
size_t firstChildDiagnostics=0;
struct ConeOut { bool ok=false;std::string cls,witness;std::vector<Cell> cells;Node apex;double fq=0,fJ=0,colq=0;long double slack=-INFINITY; };
ConeOut ConeColumn(State& s,const std::vector<Face>& skin,const std::map<Id,Node>& nodes,Id apexId,bool spread) {
  Scope scope("ConeColumn");
  ConeOut out;const long double h0=s.h0,T=HeightTolerance;const Face& f=skin[0];
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
  double bestScore=-1,bestCol=-1;
  auto evaluate=[&](const std::vector<V>& list){
    for(const auto& p:list){const Node apex{apexId,Pt(p)};std::vector<Cell> cone;bool pos=true;double colq=1;CellShape first{1,1};
      for(size_t k=0;k<skin.size()&&pos;++k){Cell cell{0,{apex,nodes.at(skin[k][0]),nodes.at(skin[k][1]),nodes.at(skin[k][2])}};
        pos=Positive(cell.v);if(!pos)break;const auto sh=Shape(s,cell);if(k==0)first=sh;colq=std::min(colq,sh.q);cone.push_back(cell);}
      if(!pos)continue;const double score=std::min(first.q/MinimumMeanRatio,first.J/MinimumScaledJacobian);
      if(score>bestScore+1e-12||(score>=1&&bestScore>=1&&colq>bestCol)){bestScore=score;bestCol=colq;out.cells=cone;out.fq=first.q;out.fJ=first.J;out.apex=apex;out.colq=colq;}}};
  evaluate(cand);
  if(spread||bestScore<1) {
    std::vector<V> grid;const std::array<std::array<long double,3>,7> feet{{{1.L/3,1.L/3,1.L/3},{.6L,.2L,.2L},{.2L,.6L,.2L},{.2L,.2L,.6L},{.45L,.45L,.1L},{.45L,.1L,.45L},{.1L,.45L,.45L}}};
    for(long double eta:{.92L,1.L,1.08L})for(const auto& w:feet)grid.push_back(w[0]*a+w[1]*b+w[2]*c+(eta*h0)*n);
    evaluate(grid);
  }
  if(out.cells.empty()){out.cls="partial-state";out.witness="LP apex rounds outside its column";return out;}
  if(bestScore<1){out.cls="first-child";std::ostringstream w;w<<"first child q="<<out.fq<<" J="<<out.fJ;
    // diagnostic (first 400 witnesses per case): composed metric lengths of the base edges and the apex edges
    if(firstChildDiagnostics<400){++firstChildDiagnostics;const auto& c0=out.cells.front();w<<"; metric edges base/apex:";
      for(unsigned i=1;i<4;++i)for(unsigned j=i+1;j<4;++j){try{w<<" "<<double(s.field->TargetEdgeLength(c0.v[i].p,c0.v[j].p));}catch(const std::exception&){w<<" ?";}}w<<" /";
      for(unsigned j=1;j<4;++j){try{w<<" "<<double(s.field->TargetEdgeLength(c0.v[0].p,c0.v[j].p));}catch(const std::exception&){w<<" ?";}}}
    out.witness=w.str();return out;}
  out.ok=true;return out;
}

// ---- (a) carrier sector plan --------------------------------------------------------------------------------------
struct SectorPlan {
  bool ok=true,needed=false;std::string why;Id corner=UINT64_MAX,cornerOpposite[2]={0,0};Edge splitEdge{0,0};
  std::map<Id,Node> cellLift;                    // old star cell -> lift of v used by that cell
  std::vector<std::array<Node,4>> gaps;          // positive gap tetrahedra
  std::map<Face,std::array<Id,3>> junction;      // wall face key -> {v,u,P_1}: lateral override on the junction edge
  std::map<Face,std::vector<Face>> extras;       // wall face key -> extra outward skin faces (rims, lid extras)
  std::map<Face,int> carrierFaces;               // new carrier faces (sorted) -> marker
  std::vector<Node> points;std::set<Id> seamPoints;
};
SectorPlan PlanSectors(State& s,const Patch& old,const PlanarReference& ref,Id v,const Node& vstar,const Dir& dir,long double H,Id& next,
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
    long double dist=H/c;if(dist>.45L*Norm(xw-x0)){dist=.45L*Norm(xw-x0);if(dist*c<MinimumBand(s)){sp.why="sector: seam edge shorter than the band";return false;}}
    P=Node{next++,Pt(x0+dist*e)};
    try{P=Constrain(s,&ref,P,{left.facet,right.facet},{left.facet,right.facet});}catch(const std::exception& x){sp.why=std::string("sector: seam binding: ")+x.what();return false;}
    return true;};
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
    for(size_t j=0;j<m;++j) {
      std::map<Id,Id> parent;std::deque<Id> queue{seam[j]};parent[seam[j]]=seam[j];
      while(!queue.empty()&&!parent.count(u)){const Id x=queue.front();queue.pop_front();for(Id y:graph[x]){if(parent.count(y))continue;if(y!=u&&boundaryVertex.count(y))continue;parent[y]=x;queue.push_back(y);}}
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
      const Face rim=orientCarrier({v,loP,hiP},like);extra.push_back(rim);sp.carrierFaces[Sorted(rim)]=like.marker;
      const Face gap=orientCarrier({hiP,loP,seam[j]},like);sp.carrierFaces[Sorted(gap)]=like.marker;
      if(lastA[j]==UINT64_MAX){sp.ok=false;sp.why="sector: cut path misses the junction";return sp;}
      // lid extra: closes the column skin over the rim's top edge, traversed opposite to the rim
      bool loHi=false;for(int i=0;i<3;++i)loHi=loHi||(rim[i]==loP&&rim[(i+1)%3]==hiP);
      const Face lid=loHi?Face{hiP,loP,L(u)}:Face{loP,hiP,L(u)};
      {std::array<Node,4> t{Ln(lastA[j]),Node{lid[0],lid[0]==hiP?lifts[j+1].p:lifts[j].p},Node{lid[1],lid[1]==hiP?lifts[j+1].p:lifts[j].p},Ln(u)};
       if(Positive(t)){sp.ok=false;sp.why="sector: gap tetrahedron on the column side of its lid face";return sp;}}
      extra.push_back(lid);(void)seamMarker;}
  }
  return sp;
}

// D.1 admission (layer2d::Admit, decision D) with exact reuse of already integrated composed edges: the old cavity's
// edges and unchanged fresh edges come from the harness length cache (same frozen field identity, same endpoint
// coordinates, MetricEdgeRequests rule); newly integrated edges are stored back. Results are identical.
std::map<Edge,EdgeMeasure> ReuseOf(State& s,const std::vector<Cell>& a,const std::vector<Cell>* b=nullptr) {
  std::map<Edge,EdgeMeasure> out;const auto& id=s.field->Identity();if(!id)return out;
  auto add=[&](const std::vector<Cell>& cells){for(const auto& c:cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){Node x=c.v[i],y=c.v[j];if(x.id>y.id)std::swap(x,y);
    const auto f=s.lengths.find({x.id,y.id});if(f!=s.lengths.end()&&f->second.second>0&&SamePoint(f->second.first[0],x.p)&&SamePoint(f->second.first[1],y.p)){
      EdgeMeasure m;m.a=x.p;m.b=y.p;m.length=f->second.second;m.identity=id;out.insert_or_assign(Edge{x.id,y.id},m);++perf.length_hits;}}};
  add(a);if(b)add(*b);return out;
}
void StoreLengths(State& s,const PatchMetric& m){for(const auto& e:m.edges)if(e.second.length>0)s.lengths[{e.first[0],e.first[1]}]={{e.second.a,e.second.b},e.second.length};}
// LayerConstruct size certificate (D.1): changed-edge excess of the fresh patch must not exceed the removed excess.
// Changed edges only (same identities and coordinates cancel); composed lengths through the harness cache. An
// unmeasurable added edge rejects; an unmeasurable removed edge counts as removed excess (a defect leaves the mesh).
long double LenAt(State& s,Id a,Point pa,Id b,Point pb) {
  if(a>b){std::swap(a,b);std::swap(pa,pb);}
  const auto f=s.lengths.find({a,b});
  if(f!=s.lengths.end()&&SamePoint(f->second.first[0],pa)&&SamePoint(f->second.first[1],pb)){++perf.length_hits;if(f->second.second<0)throw std::runtime_error("unmeasurable composed edge (cached)");return f->second.second;}
  ++perf.length_misses;
  try{const auto l=s.field->TargetEdgeLength(pa,pb);s.lengths[{a,b}]={{pa,pb},l};return l;}catch(const std::exception&){++perf.length_failures;s.lengths[{a,b}]={{pa,pb},-1.L};throw;}
}
bool SizeCertificate(State& s,const Patch& old,const Patch& fresh,long double& removed,long double& added,std::string& why) {
  Scope scope("Excess certificate");
  auto edges=[](const std::vector<Cell>& cells){std::map<Edge,std::array<Point,2>> out;for(const auto& c:cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){Node x=c.v[i],y=c.v[j];if(x.id>y.id)std::swap(x,y);out[{x.id,y.id}]={x.p,y.p};}return out;};
  const auto before=edges(old.cells),after=edges(fresh.cells);
  auto same=[](const std::map<Edge,std::array<Point,2>>& m,const std::pair<const Edge,std::array<Point,2>>& e){const auto o=m.find(e.first);return o!=m.end()&&SamePoint(o->second[0],e.second[0])&&SamePoint(o->second[1],e.second[1]);};
  removed=added=0;
  for(const auto& e:before)if(!same(after,e)){try{removed+=layer2d::ExcessOf(LenAt(s,e.first[0],e.second[0],e.first[1],e.second[1]));}catch(const std::exception&){removed+=1e30L;}}
  for(const auto& e:after)if(!same(before,e)){try{added+=layer2d::ExcessOf(LenAt(s,e.first[0],e.second[0],e.first[1],e.second[1]));}catch(const std::exception& x){why=std::string("adds an unmeasurable edge: ")+x.what();return false;}}
  return true;
}
// ---- R2 closed row transaction with sector lifts and band-height window --------------------------------------------
struct Tx {
  bool ok=false;std::string cls,witness;std::vector<Face> failing;std::vector<Id> ids;Patch old,fresh;
  std::map<Id,Id> lifts;std::map<Face,Id> apices;std::set<Id> seam,blockers;std::set<Face> rebuilt;size_t old_cells=0,fresh_cells=0,gaps=0,sectors=0;
  double first_q=1,first_J=1,height_error=0;long double min_slack=INFINITY;Id corner=UINT64_MAX,sectorVertex=UINT64_MAX;Edge cornerEdge{0,0},splitEdge{0,0};
};
Tx Transaction(State& s,const Rows& rows,const std::set<Id>& V0,bool low) {
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
    const Face f=outFace.at(key);Skin skin;Id ovV=UINT64_MAX,ovU=UINT64_MAX,ovP=UINT64_MAX;const std::vector<Face>* extra=nullptr;
    for(const auto& sp:sectors){const auto j=sp.second.junction.find(key);if(j!=sp.second.junction.end()){ovV=j->second[0];ovU=j->second[1];ovP=j->second[2];extra=&sp.second.extras.at(key);}}
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
      const bool was=before.cells.size()==1&&CheckFirstHeight(before,s.heights,*s.reference,r0,w0)&&!Bad(Shape(s,before.cells.front()));
      if(was&&!(after.cells.size()==1&&CheckFirstHeight(after,s.heights,*s.reference,r1,w1)&&!Bad(Shape(s,after.cells.front())))){tx.failing.push_back(key);tx.cls="invariant";tx.witness="unrebuilt wall face loses its passing first child (T or q/J): "+w1;}
      continue;}
    Patch one;for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)one.cells.push_back(c);}
    one.surface={sf};for(const auto& c:one.cells)for(auto n:c.v){const auto pr=out.parameters.find(n.id);if(pr!=out.parameters.end())one.parameters.insert(*pr);}
    FirstHeightReport r;if(one.cells.size()!=1||!CheckFirstHeight(one,s.heights,*s.reference,r,reason)){tx.failing.push_back(key);tx.cls="height";tx.witness="face "+std::to_string(key[0])+": "+reason;continue;}
    tx.height_error=std::max(tx.height_error,r.maximum_relative_error);
    const auto sh=Shape(s,one.cells.front());if(Bad(sh)){tx.failing.push_back(key);tx.cls="first-child";tx.witness="rowed face first child q="+std::to_string(sh.q);}}
  if(!tx.failing.empty())return tx;
  {long double removed=0,added=0;std::string why;if(!SizeCertificate(s,tx.old,out,removed,added,why)){tx.cls="measure";tx.witness=why;return tx;}
   if(!(added<=removed)){tx.cls="excess";std::ostringstream w;w<<"LayerConstruct adds frozen excess: added "<<double(added)<<" > removed "<<double(removed);tx.witness=w.str();return tx;}}
  tx.fresh=std::move(out);tx.ok=true;return tx;
}
void ApplyTx(State& s,Rows& rows,const Tx& tx) {
  Apply(s,tx.ids,tx.fresh);for(const auto& r:tx.lifts)rows.lift[r.first]=r.second;
  for(const auto& f:tx.rebuilt)rows.apex.erase(f);for(const auto& r:tx.apices)rows.apex[r.first]=r.second;
}

// ---- reporting -----------------------------------------------------------------------------------------------------
struct Stage { size_t work=0,accepted=0,rejected=0;double seconds=0;std::string outcome;std::map<std::string,size_t> reasons;std::string last;long rss=0;Counters c0,c1; };
std::string Key(const std::string& r){return swbi_dc2::Category(r).substr(0,110);}
std::string JsonStage(const Stage& g){std::ostringstream o;o<<std::setprecision(10)<<"{\"outcome\":\""<<g.outcome<<"\",\"work\":"<<g.work<<",\"accepted\":"<<g.accepted
  <<",\"rejected\":"<<g.rejected<<",\"seconds\":"<<g.seconds<<",\"peak_rss_kB\":"<<g.rss<<",\"field\":"<<JsonCounters(g.c0,g.c1)<<",\"reasons\":{";
  std::vector<std::pair<size_t,std::string>> top;for(const auto& r:g.reasons)top.emplace_back(r.second,r.first);
  std::sort(top.rbegin(),top.rend());for(size_t i=0;i<top.size()&&i<24;++i)o<<(i?",":"")<<junction_wp0::Json(top[i].second)<<':'<<top[i].first;
  o<<"},\"last_witness\":"<<junction_wp0::Json(g.last)<<'}';return o.str();}
void Begin(State& s,Stage& g){g.c0=CountersOf(s);}
void End(State& s,Stage& g,Clock::time_point t0){g.seconds=Seconds(t0);g.c1=CountersOf(s);g.rss=PeakKB();}

// ---- phase W (b): crease stations first, then level-line crossings, then metric bisection -------------------------
struct WStage : Stage { size_t stations=0,levels=0,bisections=0,unmeasurable=0,collapses=0; };
// Parameter t on (a,b) with Rho = rho_k (bracketed on 64 samples, then bisection); <0 when none in [lo,hi].
long double LevelCrossing(V a,V b,long double rhoK,long double lo,long double hi) {
  long double best=-1,bestDist=INFINITY;const unsigned n=64;long double t0=0,f0=Rho(a)-rhoK;
  for(unsigned i=1;i<=n;++i){const long double t1=(long double)i/n,f1=Rho(a+t1*(b-a))-rhoK;
    if((f0<0)!=(f1<0)){long double x0=t0,x1=t1,g0=f0;for(unsigned it=0;it<60;++it){const auto xm=(x0+x1)/2,gm=Rho(a+xm*(b-a))-rhoK;if((gm<0)==(g0<0)){x0=xm;g0=gm;}else x1=xm;}
      const auto t=(x0+x1)/2;if(t>=lo&&t<=hi&&std::abs(t-.5L)<bestDist){bestDist=std::abs(t-.5L);best=t;}}
    t0=t1;f0=f1;}
  return best;
}
WStage PhaseW(State& s,size_t work,double seconds,std::ostream& log) {
  Scope scope("PhaseW");
  WStage g;Begin(s,g);const auto t0=Clock::now();
  for(unsigned sweep=0;;++sweep) {
    std::vector<std::tuple<int,long double,Edge>> longs;std::set<Edge> seen;g.unmeasurable=0;
    for(const auto& wf:WallFaces(s))for(int i=0;i<3;++i){Edge e{std::min(wf.second[i],wf.second[(i+1)%3]),std::max(wf.second[i],wf.second[(i+1)%3])};
      if(!seen.insert(e).second)continue;long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception& x){++g.reasons["length: "+Key(x.what())];++g.unmeasurable;continue;}
      const bool crease=!d1.creases.empty()&&OnCrease(Of(NodePoint(s,e[0])),Of(NodePoint(s,e[1])));
      if(crease?l>StationTarget:l>MaximumMetricEdge)longs.emplace_back(crease?0:1,-l,e);}
    log<<"DC8 W sweep "<<sweep<<" wall_faces="<<WallFaces(s).size()<<" long_wall_edges="<<longs.size()<<" work="<<g.work<<"\n";
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
      bool done=false;std::string why;
      for(int op=0;op<2&&!done;++op)for(const auto& p:points){Patch fresh;
        try{Scope sc(op==0?"W SplitEdge":"W InsertEdgeCavity");done=op==0?SplitEdge(old,e,{s.next_node,p.first},s.next_cell,*s.reference,fresh,why):InsertEdgeCavity(old,e,{s.next_node,p.first},s.next_cell,*s.reference,fresh,why);}
        catch(const std::exception& x){why=x.what();done=false;}
        if(done){std::string d2;if(!D2Ok(s,old,fresh,d2)){done=false;why=d2;continue;}Apply(s,star,fresh);++g.accepted;++g.reasons[op?"accepted InsertEdgeCavity":"accepted SplitEdge"];(p.second==0?g.stations:p.second==1?g.levels:g.bisections)++;changed=true;break;}}
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
  End(s,g,t0);return g;
}

// ---- R1 (c): clearance conditioning with the finite candidate set of §8.3 ------------------------------------------
long double LocalDeficit(State& s,const std::set<Id>& walls,const std::vector<Id>& removed,const std::vector<Cell>* added,std::map<Id,Dir>& dirs) {
  std::set<Id> gone,touched;if(added)gone.insert(removed.begin(),removed.end());for(Id id:removed)for(auto n:s.mesh.at(id).cell.v)if(walls.count(n.id))touched.insert(n.id);
  long double sum=0;
  for(Id w:touched){auto d=dirs.find(w);if(d==dirs.end())d=dirs.emplace(w,DirectionAt(s,w)).first;if(!d->second.ok){sum+=Need(s);continue;}
    std::vector<Cell> cells;for(Id id:StarOf(s,w))if(!gone.count(id))cells.push_back(s.mesh.at(id).cell);
    if(added)for(const auto& c:*added)for(auto n:c.v)if(n.id==w)cells.push_back(c);
    const auto k=KernelBound(cells,w,d->second.d);sum+=std::max(0.L,Need(s)-k.delta*d->second.s);}
  return sum;
}
struct R1Stage : Stage { size_t deficient_before=0,deficient_after=0,moves=0,flips=0,steiner=0,collapses=0,exhausted=0; };
R1Stage PhaseR1(State& s,const Rows& rows,size_t work,double seconds,std::ostream& log) {
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
          for(long double k:{2.L,3.L}){const Point p=Pt(in+(k*Need(s)/Kappa)*n);std::string why;const auto vis=dc7::Visible(s,p,{});
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
  End(s,g,t0);log<<"DC8 R1 deficient "<<g.deficient_before<<" -> "<<g.deficient_after<<" accepted="<<g.accepted<<" outcome="<<g.outcome<<"\n";return g;
}

// ---- census (as DC-7, deficits without metric-cap integrations) ----------------------------------------------------
struct Census { size_t faces=0,vertices=0,no_direction=0,deficient=0,lp_empty=0;std::map<std::string,size_t> why; };
Census FullStateCensus(State& s,const Rows& rows) {
  Scope scope("Census");
  Census c;std::map<Id,Plan> plans;const auto apices=rows.Apices();
  for(Id v:WallVertices(s)){if(Rim(s,v))continue;++c.vertices;auto pl=PlanLift(s,apices,v,false,false);if(!pl.ok){++c.no_direction;++c.why[pl.why.substr(0,40)];}else if(Deficit(s,pl)>0)++c.deficient;plans.emplace(v,pl);}
  for(const auto& wf:WallFaces(s)) {
    if(RimFace(s,wf.second))continue;++c.faces;const auto& row=s.mesh.at(wf.first);const auto faces=CellFaces(row.cell);Face out{};for(auto f:faces)if(Sorted(f)==wf.second)out=f;
    bool ok=true;for(Id x:out)ok=ok&&plans.at(x).ok;if(!ok){++c.lp_empty;continue;}
    std::map<Id,V> P;for(Id x:out){P[x]=Of(NodePoint(s,x));P[x+(Id(1)<<60)]=plans.at(x).p;}
    const auto skin=ColumnSkin(out,[](Id x){return x+(Id(1)<<60);}).faces;
    const V a=P[out[0]],b=P[out[1]],cc=P[out[2]];std::vector<std::array<V,3>> tris;for(size_t k=1;k<skin.size();++k)tris.push_back({P[skin[k][0]],P[skin[k][1]],P[skin[k][2]]});
    if(!ApexLP(tris,a,b,cc,-1.L*Outward(a,b,cc),s.h0,HeightTolerance,false).ok)++c.lp_empty;
  }
  return c;
}
std::string JsonCensus(const Census& c){std::ostringstream o;o<<"{\"wall_faces\":"<<c.faces<<",\"wall_vertices\":"<<c.vertices<<",\"no_direction\":"<<c.no_direction
  <<",\"clearance_deficient_vertices\":"<<c.deficient<<",\"faces_full_state_lp_empty\":"<<c.lp_empty<<",\"direction_failures\":{";bool k=false;
  for(const auto& r:c.why){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}o<<"}}";return o.str();}

// ---- R2: closed row transactions; low band before growth; corner separation (Prepare) for sector lifts ------------
struct R2Stage : Stage { size_t rim_faces=0,transactions=0,lifted=0,unlifted=0,rowed=0,unrowed=0,max_closure=0,max_old=0,max_fresh=0,low_band=0,sector_tx=0,gaps=0,separations=0;
  double first_q=1,first_J=1,height_error=0;std::map<std::string,size_t> classes; };
// Corner / link separation (Prepare kind): a point at the centroid of the named star cell cones that cell, so a corner
// cell no longer owns a wall face and a foreign carrier face and the link of v gains an interior vertex. Positivity,
// the D.2 invariant on its wall faces; no column cell touched.
bool SplitCell(State& s,const Rows& rows,Id cell,std::string& why) {
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
  Apply(s,{cell},fresh);return true;
}
// Link separation (Prepare kind): cone the edge star of an interior edge from its midpoint (InsertEdgeCavity).
bool SplitLinkEdge(State& s,const Rows& rows,Edge e,std::string& why,Id wall=UINT64_MAX) {
  Scope scope("R2 SplitLinkEdge");
  const auto star=Star(s,e);if(star.empty()||star.size()>MaximumCavityCells){why="link edge star";return false;}
  const auto cols=ColumnCells(s,rows);for(Id id:star)if(cols.count(id)){why="link edge star touches a column";return false;}
  Patch old,fresh;try{old=PatchOf(s,star);}catch(const std::exception& x){why=x.what();return false;}
  const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);Point mid{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
  if(wall!=UINT64_MAX){const auto dir=DirectionAt(s,wall);if(dir.ok){V nbar{};for(const auto& n:dir.normals)nbar=nbar+n;nbar=Unit(nbar);const V w=Of(NodePoint(s,wall));
    auto h=[&](V p){return Dot(p-w,nbar);};const V A=Of(a),B=Of(b);if(h(Of(mid))<2*Need(s)){long double best=-INFINITY;
      for(long double t:{.3L,.4L,.5L,.6L,.7L}){const V p=A+t*(B-A);if(h(p)>best){best=h(p);mid=Pt(p);}}}}}
  bool ok=false;try{ok=InsertEdgeCavity(old,e,{s.next_node,mid},s.next_cell,*s.reference,fresh,why);}catch(const std::exception& x){why=x.what();ok=false;}
  if(!ok)return false;
  {std::string d2;if(!D2Ok(s,old,fresh,d2)){why=d2;return false;}}
  Apply(s,star,fresh);return true;
}
R2Stage PhaseR2(State& s,Rows& rows,double seconds,std::ostream& log) {
  Scope scope("PhaseR2");
  R2Stage g;Begin(s,g);const auto t0=Clock::now();
  for(unsigned pass=0;pass<2;++pass) {
    // (c) canonical order by lift-direction quality: smooth vertices (s* = 1) first, crease and corner vertices last,
    // so a crease vertex is lifted when its neighbours already are (full instead of skewed partial column states)
    std::vector<std::pair<long double,Id>> order;for(Id v:WallVertices(s)){if(rows.lift.count(v)||Rim(s,v))continue;const auto d=DirectionAt(s,v);order.emplace_back(d.ok?-std::round(d.s*1e6L)/1e6L:1.L,v);}
    std::sort(order.begin(),order.end());
    for(const auto& ov:order) {
      const Id v=ov.second;
      if(rows.lift.count(v)||Rim(s,v))continue;if(Seconds(t0)>=seconds){g.outcome="limit:time";break;}
      std::set<Id> V0{v};Tx tx;bool low=false;unsigned separated=0;
      for(unsigned grow=0;grow<20;++grow) {
        if(Seconds(t0)>=seconds)break;
        ++g.work;tx=Transaction(s,rows,V0,low);
        log<<"DC8 R2 tx v="<<v<<" |V|="<<V0.size()<<" low="<<low<<" ok="<<tx.ok<<" old="<<tx.old_cells<<" fresh="<<tx.fresh_cells<<" sectors="<<tx.sectors<<" gaps="<<tx.gaps<<" cls="<<tx.cls<<" "<<tx.witness<<"\n";
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
  for(Id v:WallVertices(s))if(!Rim(s,v))(rows.lift.count(v)?g.lifted:g.unlifted)++;
  for(const auto& wf:WallFaces(s)){if(RimFace(s,wf.second)){++g.rim_faces;continue;}const bool r=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});(r?g.rowed:g.unrowed)++;}
  if(g.outcome.empty())g.outcome=g.unrowed?"incomplete":"complete";End(s,g,t0);
  log<<"DC8 R2 transactions="<<g.transactions<<" lifted="<<g.lifted<<" unlifted="<<g.unlifted<<" rowed="<<g.rowed<<" unrowed="<<g.unrowed<<"\n";return g;
}

// ---- final gates with cached measures --------------------------------------------------------------------------------
struct Gates8 { size_t cells=0,bad_shape=0,long_edges=0,wall_faces=0,height_failures=0,long_wall=0,long_seam=0,long_other_physical=0,long_interior=0,unmeasurable=0;double q=1,J=1;long double lmax=0;bool complete=false; };
int EdgeClass(State& s,Edge e) {
  bool w=false,o=false;for(Id id:Star(s,e)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
    if(r.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1]))(s.walls.count(s.reference->Get(r.physical[k].facet).marker)?w:o)=true;}
  return w&&o?2:w?1:o?3:0;
}
Gates8 Final(State& s) {
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
std::string JsonFinal(State& s,const Rows& rows) {
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

Decision Admit8(State& s,const Patch& old,const Patch& fresh,Kind kind) {
  Scope scope("Admit (D.1)");
  Decision base;base.old_cells=old.cells.size();base.fresh_cells=fresh.cells.size();
  if(old.cells.empty()||fresh.cells.empty()||old.cells.size()>MaximumReplacementCells||fresh.cells.size()>MaximumReplacementCells){base.category=5;base.reason="replacement size";return base;}
  std::string why;if(!HeightT(s,fresh,why)){base.category=4;base.reason=why;return base;}
  PatchMetric after,before;
  try{auto reuse=ReuseOf(s,old.cells,&fresh.cells);
    after=MeasureMetricCells(fresh.cells,*s.field,nullptr,&reuse,false);StoreLengths(s,after);
    for(const auto& e:after.edges)reuse.insert_or_assign(e.first,e.second);
    before=MeasureMetricCells(old.cells,*s.field,nullptr,&reuse,false);StoreLengths(s,before);}
  catch(const std::exception& e){base.category=5;base.reason=e.what();return base;}
  base.q_old=before.minimum_mean_ratio;base.J_old=before.minimum_scaled_jacobian;base.q_new=after.minimum_mean_ratio;base.J_new=after.minimum_scaled_jacobian;
  for(const auto& c:fresh.cells){const auto sh=ShapeOf(s,c);
    if(IsFirstChild(s,fresh,c)){base.first_q=std::min(base.first_q,sh.q);base.first_J=std::min(base.first_J,sh.J);}
    else{base.other_q=std::min(base.other_q,sh.q);base.other_J=std::min(base.other_J,sh.J);}}
  auto shared=[](const std::map<Edge,EdgeMeasure>& others,const std::pair<const Edge,EdgeMeasure>& e){
    const auto o=others.find(e.first);return o!=others.end()&&SamePoint(o->second.a,e.second.a)&&SamePoint(o->second.b,e.second.b);};
  for(const auto& e:before.edges)if(!shared(after.edges,e))base.removed+=layer2d::ExcessOf(e.second.length);
  for(const auto& e:after.edges)if(!shared(before.edges,e))base.added+=layer2d::ExcessOf(e.second.length);
  const bool size_progress=base.removed>0&&base.added<base.removed*(1-1e-12L);
  const bool quality_progress=(base.q_new>base.q_old+1e-7&&base.J_new>=base.J_old*(1-1e-12))||(base.J_new>base.J_old+1e-7&&base.q_new>=base.q_old*(1-1e-12));
  Decision d=base;
  if(base.first_q<MinimumMeanRatio||base.first_J<MinimumScaledJacobian){d.category=2;d.reason="D first child: q="+std::to_string(base.first_q)+" J="+std::to_string(base.first_J);return d;}
  if(kind==Kind::Layer?!(base.other_q>0&&base.other_J>0):(base.other_q<std::min<double>(MinimumMeanRatio,base.q_old)||base.other_J<std::min<double>(MinimumScaledJacobian,base.J_old))){
    d.category=2;d.reason="D floor: q="+std::to_string(base.other_q)+" J="+std::to_string(base.other_J)+" old q="+std::to_string(base.q_old);return d;}
  if(kind==Kind::Refine){if(fresh.cells.size()>old.cells.size()&&size_progress){d.ok=true;return d;}d.category=3;d.reason="refine: no strict changed-edge excess reduction";return d;}
  if(kind==Kind::Repair){if(size_progress||(quality_progress&&base.added<=base.removed)){d.ok=true;return d;}d.category=3;d.reason="repair: neither size nor quality progress";return d;}
  if(base.added<=base.removed){d.ok=true;return d;}d.category=3;d.reason="layer: adds frozen excess";return d;
}
// ---- V (d): existing volume operators outside columns; ApexMove / ColumnSplit inside columns --------------------------
// Exact on-edge binary64 point near the requested one (production OnEdge semantics: zero projected orientation on all
// three axes and inside the bounding box); none -> no SplitEdge trial for a non-physical edge.
bool OnEdgeExact(Point a,Point b,Point p) {
  auto pr=[](Point x,int k){return k==0?Point{x.y,x.z,0}:k==1?Point{x.x,x.z,0}:Point{x.x,x.y,0};};
  for(int k=0;k<3;++k)if(Orientation(pr(a,k),pr(b,k),pr(p,k),{0,0,1})!=0)return false;
  return p.x>=std::min(a.x,b.x)&&p.x<=std::max(a.x,b.x)&&p.y>=std::min(a.y,b.y)&&p.y<=std::max(a.y,b.y)&&p.z>=std::min(a.z,b.z)&&p.z<=std::max(a.z,b.z);
}
bool ExactOnEdgePoint(Point a,Point b,Point near,Point& out) {
  const double c[3]={near.x,near.y,near.z};
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)for(int dz=-1;dz<=1;++dz) {
    double p[3];const int d[3]={dx,dy,dz};
    for(int k=0;k<3;++k)p[k]=d[k]==0?c[k]:std::nextafter(c[k],d[k]>0?INFINITY:-INFINITY);
    const Point q{p[0],p[1],p[2]};if(SamePoint(q,a)||SamePoint(q,b))continue;if(OnEdgeExact(a,b,q)){out=q;return true;}}
  return false;
}
struct VStage : Stage { size_t column_demand=0,sweeps=0,apex_moves=0,column_splits=0,cache_skips=0,exact_splits=0,no_exact_point=0,junction_skips=0;std::map<std::string,size_t> demand; };
struct ColumnIndex {
  std::set<Id> cells;std::map<Id,Face> apexFace;std::map<Id,Id> unlift;
  void Build(const State& s,const Rows& rows){cells=ColumnCells(s,rows);apexFace.clear();unlift.clear();for(const auto& r:rows.apex)apexFace[r.second]=r.first;for(const auto& r:rows.lift)unlift[r.second]=r.first;}
};
// ApexMove: Cone(star(p_f), p') with p' from the LP and a fixed grid inside the T window; Repair admission (D.1).
bool ApexMove(State& s,Rows& rows,const Face& key,VStage& g,std::string& why) {
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
  const auto d=Admit8(s,old,fresh,Kind::Repair);
  if(!d.ok){++g.rejected;++g.reasons["ApexMove ["+std::to_string(d.category)+"] "+Key(d.reason)];why="ApexMove: "+d.reason;return false;}
  Apply(s,ids,fresh);rows.apex[key]=cone.apex.id;++g.accepted;++g.apex_moves;return true;
}
// ColumnSplit(a,b): cavity col(f) U col(g) U edge star of the lid edge (a*,b*); wall point m on (a,b) bound to both wall
// charts, m* on the lid edge; upper cells split at m*, four columns coned from LP apices; D.1 Refine admission.
bool ColumnSplit(State& s,Rows& rows,Edge ab,VStage& g,std::string& why,std::set<std::vector<Id>>& rejected) {
  Scope scope("V ColumnSplit");
  const Id a=ab[0],b=ab[1];const auto la=rows.lift.find(a),lb=rows.lift.find(b);if(la==rows.lift.end()||lb==rows.lift.end()){why="unlifted wall edge";return false;}
  std::vector<Face> faces;for(const auto& r:rows.apex)if(Has(r.first,a)&&Has(r.first,b))faces.push_back(r.first);
  if(faces.size()!=2){why="ColumnSplit needs two rowed faces on the wall edge (junction edge or unrowed neighbour)";return false;}
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
    for(const Face& h:{f1,f2}){const auto skin=ColumnSkin(h,Lf);auto cone=ConeColumn(s,skin.faces,nodes,next,false);
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
std::map<std::array<double,8>,Point> balancedCache;  // (edge ids, endpoint coordinates) -> balanced point
bool SameChartJunction(State& s,Edge e,bool& junction) {
  std::set<Id> charts;size_t n=0;
  for(Id id:Star(s,e)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)if(r.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1])){++n;charts.insert(s.reference->Canonical(r.physical[k].facet));}}
  junction=n>0&&charts.size()>1;return n>0;
}
std::vector<std::pair<std::string,std::function<bool(std::vector<Id>&,Patch&,Patch&,std::string&)>>> RefineVariantsOf(State& s,Edge e,VStage& g) {
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
VStage PhaseV(State& s,Rows& rows,size_t work,double seconds,std::ostream& log) {
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
    log<<"DC8 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" height_fail="<<gates.height_failures<<" column_demand="<<g.column_demand<<" work="<<g.work<<" cache_skips="<<g.cache_skips<<" t="<<Seconds(t0)<<"\n";
    std::cout<<"DC8 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" work="<<g.work<<" t="<<Seconds(t0)<<std::endl;
    if(gates.complete){g.outcome="complete";break;}
    if(limited()){g.outcome="limit";break;}
    if(!changed){g.outcome="blocked";break;}
  }
  End(s,g,t0);return g;
}
// Final mesh with the native gates (as DC-7) from the cached measures.
void ExportGates(State& s,const Rows& rows,const std::string& path) {
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
// ---- loaders: DC-7 copies whose frozen field carries a metric identity (enables exact edge-length reuse) and timings
std::unique_ptr<State> LoadA1(const std::string& wp0b) {
  const std::string annulus=wp0b+"/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1";
  ReferenceState rejected;if(!LoadReference(annulus+"_rejected_rank0.native_ref",rejected)||!rejected.original||!rejected.original->Mapped())throw std::runtime_error("DC7 annulus reference");
  std::ifstream cfgFile(annulus+".cfg");if(!cfgFile)throw std::runtime_error("DC7 annulus cfg");
  std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(cfgFile,SU2_COMPONENT::SU2_CFD,false);}
  std::ifstream in(wp0b+"/annulus_case.txt");std::string tag;int version;in>>tag>>version;if(tag!="SU2_JUNCTION_ANNULUS")throw std::runtime_error("DC7 annulus case");
  std::string key;double core=0;Id recordedCell=0;double recordedQ=0,recordedJ=0;in>>key>>core;in>>key>>recordedCell>>recordedQ>>recordedJ;
  size_t count=0;in>>key>>count;std::vector<DonorCell> donors;
  for(size_t i=0;i<count;++i){DonorCell d;int extension=0;in>>d.cell.id>>extension;for(unsigned k=0;k<4;++k){auto& n=d.cell.v[k];auto& m=d.sensor[k];in>>n.id>>n.p.x>>n.p.y>>n.p.z>>m.xx>>m.xy>>m.xz>>m.yy>>m.yz>>m.zz;}
    if(extension)d.extension_component=0;donors.push_back(d);}
  std::string name;size_t n;in>>name>>n;if(name!="input")throw std::runtime_error("DC7 annulus block");std::vector<OwnedCell> input;
  for(size_t i=0;i<n;++i){OwnedCell row;in>>row.cell.id;for(unsigned k=0;k<4;++k)in>>row.cell.v[k].id>>row.cell.v[k].p.x>>row.cell.v[k].p.y>>row.cell.v[k].p.z;
    for(unsigned k=0;k<4;++k){int present;in>>present>>row.physical[k].facet;row.physical[k].present=present;}input.push_back(row);}
  auto layers=CBoundaryLayerMetric::FromNativeReference(rejected,*config);
  if(config->GetAdap_BL_Budget()){layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config->GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config->GetAdap_BL_OuterGrowthMax()));layers->FreezeOuterNormal();}
  auto authority=rejected.original->Parameters()->Authority();BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:rejected.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto x:f.v)points.emplace(x.id,BoundaryPoint{x,authority->Vertex(x.id)});}
  auto s=std::make_unique<State>();s->name="annulus_A1";
  s->reference=PlanarReference::WithParameters(rejected.original,std::make_shared<BoundaryParameterState>(authority,points,skin));
  s->facets=rejected.facets;s->markers=rejected.marker_names;s->mapped=true;s->core=core;
  for(unsigned short b=0;b<config->GetnAdap_BL();++b)for(size_t m=0;m<rejected.marker_names.size();++m)if(rejected.marker_names[m]==config->GetAdap_BL(b).marker) {
    s->walls.insert(int(m));s->heights[int(m)]=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);
    s->h0=SU2_TYPE::GetValue(config->GetAdap_BL(b).firstHeight);s->growth=SU2_TYPE::GetValue(config->GetAdap_BL(b).growth);s->thickness=SU2_TYPE::GetValue(config->GetAdap_BL(b).thickness);}
  s->heights.tolerance=HeightTolerance;
  for(const auto& row:input)s->mesh.emplace(row.cell.id,row);
  s->layers=layers;s->field=std::make_unique<FrozenField>(donors,SU2NativeBL3D::Composition(layers,core),true,std::vector<Id>{0},SU2NativeBL3D::EdgeGeometry(layers,core),std::make_shared<const FrozenMetricIdentity>());
  Prepare(*s);return s;
}
// Faceted SU2_JUNCTION_CASE fixture with the production D1 law (ADAP_HGRAD 1.3, default ADAP_ANGLE 45, partners are
// the non-wall facets, "symmetry" reflective) and the production composition.
std::unique_ptr<State> LoadTube(const std::string& path) {
  Id center=0;
  auto s=LoadCaseWith(path,center,[](std::shared_ptr<CBoundaryLayerMetric> layers,double core){return Compose(layers,core);});
  std::vector<CBoundaryLayerMetric::Wall> walls;CBoundaryLayerMetric::FeatureOptions options;options.hgrad=1.3;
  for(int m:s->walls){CBoundaryLayerMetric::Wall w;w.layer.marker=s->markers.at(m);w.layer.firstHeight=s->h0;w.layer.growth=s->growth;w.layer.thickness=s->thickness;
    std::map<Id,unsigned long> index;
    for(const auto& f:s->facets)if(f.marker==m){for(auto x:f.v){const auto ins=index.emplace(x.id,index.size());if(ins.second){w.coord.push_back(x.p.x);w.coord.push_back(x.p.y);w.coord.push_back(x.p.z);}w.conn.push_back(ins.first->second);}w.conn.push_back(w.conn.back());}
    walls.push_back(std::move(w));}
  for(const auto& f:s->facets)if(!s->walls.count(f.marker)){CBoundaryLayerMetric::FeatureFace p;p.reflective=s->markers.at(f.marker).find("sym")!=std::string::npos;
    for(auto x:f.v){p.coord.push_back(x.p.x);p.coord.push_back(x.p.y);p.coord.push_back(x.p.z);}options.faces.push_back(std::move(p));}
  s->layers=std::make_shared<CBoundaryLayerMetric>(3,std::move(walls),45.0,std::move(options));
  // same six covering uniform-sensor donors as LoadCaseWith
  Point lo=s->mesh.begin()->second.cell.v[0].p,hi=lo;for(const auto& r:s->mesh)for(auto x:r.second.cell.v){lo={std::min(lo.x,x.p.x),std::min(lo.y,x.p.y),std::min(lo.z,x.p.z)};hi={std::max(hi.x,x.p.x),std::max(hi.y,x.p.y),std::max(hi.z,x.p.z)};}
  const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
  const Tensor sensor{s->core,0,0,s->core,0,s->core};std::vector<DonorCell> donors;
  auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
  const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
  for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
    if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),true,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core),std::make_shared<const FrozenMetricIdentity>());
  s->heights.tolerance=HeightTolerance;s->lengths.clear();dc7::MakeMapped(*s);return s;
}
// M6 neighbourhood (dc7_m6_prepare.py): fresh adopted Y=0 faceted authority of the cut, real selected sensor on the
// cut's own original cells (<= 256 donors), production layers from the saved-state reference/config (ADAP_ANGLE 60).
struct M6Field { std::shared_ptr<CBoundaryLayerMetric> layers;double h0=0,growth=0,thickness=0;std::string wall; };
std::map<std::string,M6Field> m6cache;
std::unique_ptr<State> LoadM6Cut(const std::string& path) {
  std::ifstream in(path);if(!in)throw std::runtime_error("DC7 missing "+path);std::string tag;int version=0;in>>tag>>version;
  if(tag!="SU2_DC7_M6"||version!=1)throw std::runtime_error("DC7 bad M6 cut header");
  auto s=std::make_unique<State>();std::string cfg,ref;double parameter=-1;std::map<Id,std::pair<Point,Tensor>> nodes;std::vector<std::pair<Id,std::array<Id,4>>> cells;
  std::vector<std::tuple<Id,int,std::array<Id,3>>> facets;std::vector<std::pair<std::array<Id,3>,std::pair<Point,Tensor>>> collar;Id centre=UINT64_MAX;
  for(std::string key;in>>key;) {
    if(key=="end")break;
    if(key=="centre")in>>centre;else if(key=="extension"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;std::array<Id,3> f;Point p;Tensor t;in>>id>>f[0]>>f[1]>>f[2]>>p.x>>p.y>>p.z>>t.xx>>t.xy>>t.xz>>t.yy>>t.yz>>t.zz;collar.push_back({f,{p,t}});}}
    else if(key=="name")in>>s->name;else if(key=="cfg")in>>cfg;else if(key=="ref")in>>ref;else if(key=="parameter")in>>parameter;else if(key=="core")in>>s->core;
    else if(key=="markers"){size_t n;in>>n;s->markers.resize(n);for(auto& m:s->markers)in>>m;}
    else if(key=="walls"){size_t n;in>>n;for(size_t k=0;k<n;++k){int m;in>>m;s->walls.insert(m);}}
    else if(key=="nodes"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;Point p;Tensor t;in>>id>>p.x>>p.y>>p.z>>t.xx>>t.xy>>t.xz>>t.yy>>t.yz>>t.zz;nodes[id]={p,t};}}
    else if(key=="cells"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;std::array<Id,4> v;in>>id>>v[0]>>v[1]>>v[2]>>v[3];cells.push_back({id,v});}}
    else if(key=="facets"){size_t n;in>>n;for(size_t k=0;k<n;++k){Id id;int m;std::array<Id,3> v;in>>id>>m>>v[0]>>v[1]>>v[2];facets.emplace_back(id,m,v);}}
    else throw std::runtime_error("DC7 unknown M6 key "+key);
    if(!in)throw std::runtime_error("DC7 truncated M6 cut");
  }
  auto& field=m6cache[cfg+"|"+ref+"|"+std::to_string(parameter)];
  if(!field.layers) {
    std::ifstream cfgFile(cfg);if(!cfgFile)throw std::runtime_error("DC7 M6 cfg");std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(cfgFile,SU2_COMPONENT::SU2_CFD,false);}
    ReferenceState reference;if(!LoadReference(ref,reference)||!reference.original)throw std::runtime_error("DC7 M6 reference");
    field.layers=CBoundaryLayerMetric::FromNativeReference(reference,*config);
    field.layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config->GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config->GetAdap_BL_OuterGrowthMax()));
    field.layers->SelectOuterNormal(parameter);field.layers->FreezeOuterNormal();
    field.h0=SU2_TYPE::GetValue(config->GetAdap_BL(0).firstHeight);field.growth=SU2_TYPE::GetValue(config->GetAdap_BL(0).growth);
    field.thickness=SU2_TYPE::GetValue(config->GetAdap_BL(0).thickness);field.wall=config->GetAdap_BL(0).marker;
  }
  s->h0=field.h0;s->growth=field.growth;s->thickness=field.thickness;s->layers=field.layers;
  std::vector<Cell> mesh;std::vector<DonorCell> donors;
  for(const auto& c:cells){Cell cell{c.first,{}};for(unsigned k=0;k<4;++k)cell.v[k]={c.second[k],nodes.at(c.second[k]).first};
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);mesh.push_back(cell);}
  {// the real selected sensor of the cut centre node, frozen uniform on six covering donors (the cut exceeds the
   // 256-donor FrozenField bound; within a cut of a few cm the P1 sensor variation is neglected and reported)
   Point lo=mesh.front().v[0].p,hi=lo;for(const auto& c:mesh)for(auto x:c.v){lo={std::min(lo.x,x.p.x),std::min(lo.y,x.p.y),std::min(lo.z,x.p.z)};hi={std::max(hi.x,x.p.x),std::max(hi.y,x.p.y),std::max(hi.z,x.p.z)};}
   const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
   const Tensor sensor=nodes.at(centre).second;
   auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
   const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
   for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
     if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}}
  std::map<Face,Face> skin;for(const auto& c:mesh)for(auto f:CellFaces(c))skin[Sorted(f)]=f;
  std::map<Face,int> uses;for(const auto& c:mesh)for(auto f:CellFaces(c))++uses[Sorted(f)];
  std::map<Face,Id> physical;
  for(const auto& f:facets){const auto key=Sorted(std::get<2>(f));if(uses.at(key)!=1)throw std::runtime_error("DC7 M6 facet is not a skin face.");
    const auto oriented=skin.at(key);Facet facet;facet.id=std::get<0>(f);facet.marker=std::get<1>(f);
    for(unsigned k=0;k<3;++k)facet.v[k]={oriented[k],nodes.at(oriented[k]).first};s->facets.push_back(facet);physical[key]=facet.id;}
  s->reference=std::make_shared<PlanarReference>(s->facets);
  for(const auto& c:mesh){OwnedCell row;row.cell=c;const auto f=CellFaces(c);for(unsigned k=0;k<4;++k){const auto found=physical.find(Sorted(f[k]));if(found!=physical.end())row.physical[k]={true,found->second};}s->mesh.emplace(c.id,row);}
  for(int m:s->walls)s->heights[m]=s->h0;s->heights.tolerance=HeightTolerance;
  if(!collar.empty())throw std::runtime_error("DC7 collar donors are not used with the covering sensor");
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),true,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core),std::make_shared<const FrozenMetricIdentity>());
  Prepare(*s);dc7::MakeMapped(*s);return s;
}
std::string JsonPerf(){std::ostringstream o;o<<std::setprecision(8)<<"{\"ops\":{";bool k=false;
  std::vector<std::pair<double,std::string>> top;for(const auto& r:perf.ops)top.emplace_back(r.second.second,r.first);std::sort(top.rbegin(),top.rend());
  for(const auto& t:top){const auto& r=perf.ops.at(t.second);o<<(k?",":"")<<junction_wp0::Json(t.second)<<":{\"count\":"<<r.first<<",\"seconds\":"<<r.second<<",\"ms_per_op\":"<<1e3*r.second/std::max<size_t>(1,r.first)<<'}';k=true;}
  o<<"},\"caches\":{\"length_hits\":"<<perf.length_hits<<",\"length_misses\":"<<perf.length_misses<<",\"length_failures\":"<<perf.length_failures<<",\"shape_hits\":"<<perf.shape_hits
   <<",\"shape_misses\":"<<perf.shape_misses<<",\"height_hits\":"<<perf.height_hits<<",\"height_misses\":"<<perf.height_misses<<",\"rejection_cache_skips\":"<<perf.cache_skips<<"}}";return o.str();}
}  // namespace dc8

TEST_CASE("DC-8 W(crease strips) -> R1 -> R2(sector lifts, band window) -> V(ApexMove, ColumnSplit)", "[.][NativeFirstRowDC8]") {
  using namespace dc8;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC8_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC8_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC8_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC8_CASES");REQUIRE(only);
  const char* binary=std::getenv("SU2_DC8_BINARY_SHA");const std::string sha=binary?binary:"unknown";
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty())continue;
    // resume only a complete result of the same binary (provenance in the JSON); anything else is recomputed
    {std::ifstream old(root+"/dc8_"+name+".json");std::stringstream text;text<<old.rdbuf();const auto saved=text.str();
     if(old&&sha!="unknown"&&saved.find("\"binary_sha256\":\""+sha+"\"")!=std::string::npos&&saved.find("\"total_seconds\":")!=std::string::npos)continue;}
    perf=Perf{};ResetCaches();cachesOn=true;balancedCache.clear();firstChildDiagnostics=0;
    const auto start=Clock::now();std::ostringstream json,log;
    json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\",\"binary_sha256\":\""<<sha<<"\",\"limits\":{\"W\":[8192,120],\"R1\":[4096,120],\"R2_seconds\":600,\"V\":[8192,120]}";
    auto load=[&]()->std::unique_ptr<State>{
      if(name=="annulus")return LoadA1(fixtures);
      if(name.rfind("m6_",0)==0)return LoadM6Cut(cuts+"/"+name+".txt");
      return LoadTube(fixtures+"/"+name+".txt");};
    std::unique_ptr<State> s;{Scope sc("Load");s=load();}Rows rows;DetectCreases(*s);
    json<<",\"input\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<",\"h0\":"<<s->h0<<",\"core\":"<<s->core<<",\"mapped\":"<<s->mapped
        <<",\"d1\":{\"segments\":"<<s->layers->GetFeatures().segments<<",\"images\":"<<s->layers->GetFeatures().images<<",\"corners\":"<<s->layers->GetFeatures().corners
        <<",\"angle\":"<<s->layers->GetFeatures().angle<<",\"slope\":"<<s->layers->GetFeatures().slope<<",\"R_f\":"<<s->layers->GetFeatures().maximumRadius
        <<",\"creases_in_cut_test_side\":"<<d1.creases.size()<<"}}";
    std::cout<<"DC8 "<<name<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<" creases="<<d1.creases.size()<<std::endl;
    const auto w=PhaseW(*s,8192,120,log);
    json<<",\"W\":"<<JsonStage(w)<<",\"W_counts\":{\"stations\":"<<w.stations<<",\"level_splits\":"<<w.levels<<",\"bisections\":"<<w.bisections<<",\"short_edge_collapses\":"<<w.collapses<<",\"unmeasurable\":"<<w.unmeasurable<<"}"
        <<",\"after_W\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<'}';
    std::cout<<"DC8 "<<name<<" W "<<w.outcome<<" wall_faces="<<WallFaces(*s).size()<<" stations="<<w.stations<<" levels="<<w.levels<<" bisections="<<w.bisections<<" collapses="<<w.collapses<<" t="<<w.seconds<<std::endl;
    const auto c0=FullStateCensus(*s,rows);json<<",\"census_after_W\":"<<JsonCensus(c0);
    const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1)<<",\"R1_counts\":{\"deficient_before\":"<<r1.deficient_before<<",\"deficient_after\":"<<r1.deficient_after
        <<",\"moves\":"<<r1.moves<<",\"flips\":"<<r1.flips<<",\"collapses\":"<<r1.collapses<<",\"steiner\":"<<r1.steiner<<",\"exhausted_vertices\":"<<r1.exhausted<<'}';
    const auto c1=FullStateCensus(*s,rows);json<<",\"census_after_R1\":"<<JsonCensus(c1);
    std::cout<<"DC8 "<<name<<" R1 "<<r1.outcome<<" deficient "<<r1.deficient_before<<"->"<<r1.deficient_after<<" lp_empty="<<c1.lp_empty<<" t="<<r1.seconds<<std::endl;
    const auto r2=PhaseR2(*s,rows,600,log);
    json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"transactions\":"<<r2.transactions<<",\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed
        <<",\"unrowed\":"<<r2.unrowed<<",\"rim_faces_excluded\":"<<r2.rim_faces<<",\"max_closure_vertices\":"<<r2.max_closure<<",\"max_old_cells\":"<<r2.max_old<<",\"max_fresh_cells\":"<<r2.max_fresh
        <<",\"low_band_transactions\":"<<r2.low_band<<",\"sector_transactions\":"<<r2.sector_tx<<",\"gap_cells\":"<<r2.gaps<<",\"corner_separations\":"<<r2.separations
        <<",\"first_q_min\":"<<r2.first_q<<",\"first_J_min\":"<<r2.first_J<<",\"max_height_error\":"<<r2.height_error<<",\"witness_classes\":{";
    {bool k=false;for(const auto& c:r2.classes){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}";
    std::cout<<"DC8 "<<name<<" R2 rowed="<<r2.rowed<<" unrowed="<<r2.unrowed<<" t="<<r2.seconds<<" last="<<r2.last.substr(0,300)<<std::endl;
    {Scope sc("JsonFinal after_R");json<<",\"after_R\":"<<JsonFinal(*s,rows);}
    // Optional V limits (default 8192 work / 120 s): a work-limited V makes before/after runs comparable.
    const size_t vWork=std::getenv("SU2_DC8_V_WORK")?std::stoul(std::getenv("SU2_DC8_V_WORK")):8192;
    const double vSeconds=std::getenv("SU2_DC8_V_SECONDS")?std::stod(std::getenv("SU2_DC8_V_SECONDS")):120;
    const auto v=PhaseV(*s,rows,vWork,vSeconds,log);
    json<<",\"V\":"<<JsonStage(v)<<",\"V_counts\":{\"sweeps\":"<<v.sweeps<<",\"column_demand_unrouted\":"<<v.column_demand<<",\"apex_moves\":"<<v.apex_moves<<",\"column_splits\":"<<v.column_splits
        <<",\"rejection_cache_skips\":"<<v.cache_skips<<",\"exact_on_edge_splits\":"<<v.exact_splits<<",\"no_exact_on_edge_point\":"<<v.no_exact_point<<",\"junction_insert_skips\":"<<v.junction_skips
        <<",\"with_complete_rows\":"<<(r2.unrowed==0)<<",\"demand\":{";
    {bool k=false;for(const auto& d:v.demand){json<<(k?",":"")<<junction_wp0::Json(d.first)<<':'<<d.second;k=true;}}json<<"}}";
    {Scope sc("JsonFinal final");json<<",\"final\":"<<JsonFinal(*s,rows);}
    ExportGates(*s,rows,root+"/dc8_"+name+"_final.mesh");
    const auto before=Seconds(start);
    {Scope sc("Reaudit");const auto gc=Final(*s);ResetCaches();cachesOn=false;auto fresh=load();fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);cachesOn=true;
     json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max_measured\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"bad_shape\":"<<ga.bad_shape
         <<",\"height_failures\":"<<ga.height_failures<<",\"unmeasurable\":"<<ga.unmeasurable<<",\"complete\":"<<ga.complete<<",\"seconds\":"<<Seconds(start)-before;
     json<<",\"agrees_with_cached_final\":"<<(gc.long_edges==ga.long_edges&&gc.bad_shape==ga.bad_shape&&gc.height_failures==ga.height_failures&&gc.unmeasurable==ga.unmeasurable)<<'}';}
    json<<",\"perf\":"<<JsonPerf()<<",\"peak_rss_kB\":"<<PeakKB()<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
    std::ofstream(root+"/dc8_"+name+".json")<<json.str();std::ofstream(root+"/dc8_"+name+".log")<<log.str();
    std::cout<<"DC8 "<<name<<" done in "<<Seconds(start)<<" s"<<std::endl;
  }
}
