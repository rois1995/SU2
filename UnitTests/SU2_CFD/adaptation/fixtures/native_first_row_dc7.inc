// BL_FIRST_ROW_DESIGN rev 2.1, DC-7: TEST-ONLY [NativeFirstRowDC7] harness of the WP-D construction order
// W (wall/seam/junction subdivision to the composed D1 surface target, no row present) -> R1 (clearance conditioning)
// -> R2 (closed row transactions T(V) = LiftCone + FaceApex, apex window decided by the exact three-variable LP over
// the whole height interval, persistent columns col(f) = star(p_f)) -> V (existing volume operators, columns
// protected). Nothing here is production code or promoted. Production primitives only: SplitEdge,
// InsertEdgeCavity, MoveVertex, Reconnect, Cone, ValidatePrescribedInterface, CheckFirstHeight, FindFace,
// BindNewPoint, RepairMetricConnectivity, FrozenField with the production composition (SU2NativeBL3D) and the
// production D1 law of CBoundaryLayerMetric. Admission (user decision U1): first children absolute q >= .20,
// J >= .05 and |h-h0|/h0 <= T; layer-construct fresh cells positivity only; other kinds not worse; Prepare
// transactions (W, R1, before any row) geometry/positivity only. Final gates are unchanged and absolute.
#include "../../../../SU2_CFD/include/adaptation/CNativeMetricFamily3D.hpp"
namespace dc7 {
using namespace layer2d;
using Clock=std::chrono::steady_clock;
constexpr long double Kappa=1.2L,Beta=1e-3L,LowBand=1.3L,HighBand=2.2L;
double Seconds(Clock::time_point t){return std::chrono::duration<double>(Clock::now()-t).count();}
// ---- orientation convention: SDet(a,b,c,d) > 0 iff the production Orientation of the cell (a,b,c,d) is positive ----
long double Det(V a,V b,V c,V d){return Dot(b-a,Cross(c-a,d-a));}
long double Sigma() {
  static const long double s=Orientation({0,0,0},{1,0,0},{0,1,0},{0,0,1})>0?1.L:-1.L;return s;
}
long double SDet(V a,V b,V c,V d){return Sigma()*Det(a,b,c,d);}
// Outward unit normal of an outward-oriented skin triangle (the cell interior lies on the negative side).
V Outward(V a,V b,V c){return Sigma()*Unit(Cross(b-a,c-a));}

struct Rows {
  std::map<Id,Id> lift;            // wall vertex -> lifted vertex v*
  std::map<Face,Id> apex;          // sorted wall face -> current column apex p_f
  std::set<Id> Apices() const {std::set<Id> a;for(const auto& r:apex)a.insert(r.second);return a;}
};
struct WF { Face key{},out{};Id facet=0,owner=0;int marker=0; };
// Physical faces of the given cells (wall==true: wall markers only; false: carriers only).
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
// Neighbourhood cut fixtures declare their artificial skin as marker CUT: its wall vertices are the fixed rim.
bool Rim(const State& s,Id v) {
  for(Id id:StarOf(s,v)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
    if(r.physical[k].present&&Has(f[k],v)&&s.markers.at(s.reference->Get(r.physical[k].facet).marker)=="CUT")return true;}
  return false;
}
bool RimFace(const State& s,const Face& f){for(Id x:f)if(Rim(s,x))return true;return false;}

// ---- lift direction: maximize min n_f.d over unit d with d.nu = 0 on every carrier plane at v (rank test) ----------
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
struct Dir { bool ok=false;V d;long double s=0;std::set<Id> carriers,bind;int rank=0;Id seam=UINT64_MAX;std::string why; };
Dir DirectionAt(State& s,Id v) {
  Dir out;const auto star=StarOf(s,v);std::vector<V> normals;std::vector<V> cn;
  for(const auto& f:PhysicalOf(s,star,true))if(Has(f.key,v)){const V a=Of(NodePoint(s,f.out[0])),b=Of(NodePoint(s,f.out[1])),c=Of(NodePoint(s,f.out[2]));
    const V n=-1.L*Outward(a,b,c);bool dup=false;for(const auto& x:normals)dup=dup||Dot(x,n)>1-1e-14L;if(!dup)normals.push_back(n);}
  for(const auto& f:PhysicalOf(s,star,false))if(Has(f.key,v)){const V a=Of(NodePoint(s,f.out[0])),b=Of(NodePoint(s,f.out[1])),c=Of(NodePoint(s,f.out[2]));
    cn.push_back(Outward(a,b,c));out.carriers.insert(f.facet);}
  if(normals.empty()){out.why="no wall face";return out;}
  std::vector<V> basis;for(const auto& x:cn){V r=x;for(const auto& b:basis)r=r-Dot(r,b)*b;if(Norm(r)>1e-9L)basis.push_back(Unit(r));}
  out.rank=int(basis.size());
  if(basis.size()>=3){out.why="carrier: constraint rank 3 leaves no direction";return out;}
  if(basis.size()==2) {
    V t=Unit(Cross(basis[0],basis[1]));long double sum=0;for(const auto& n:normals)sum+=Dot(n,t);
    out.d=sum<0?-1.L*t:t;
  } else {
    std::vector<V> projected;for(auto n:normals){for(const auto& b:basis)n=n-Dot(n,b)*b;if(Norm(n)<1e-12L){out.why="carrier: wall normal parallel to the carrier normal";return out;}projected.push_back(n);}
    if(!MinNormAny(projected,out.d)){out.why="hemisphere: no open hemisphere";return out;}
  }
  out.s=INFINITY;for(const auto& n:normals)out.s=std::min(out.s,Dot(n,out.d));
  if(!(out.s>1e-9L)){out.why="hemisphere: s*="+std::to_string(double(out.s));return out;}
  // binding support of v*: rank 1 -> the carrier facet whose sector at v contains d; rank 2 -> the carrier facets of
  // the seam edge (v,w) along d (the existing seam-split support, ProjectSplitPoint semantics).
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
      const int plane=std::abs(Dot(n,basis[0]))>1-1e-9L?0:1;
      for(Id x:f.key)if(x!=v){planes[x].insert(plane);facets[x].insert(f.facet);}}
    long double best=-INFINITY;for(const auto& r:planes)if(r.second.size()==2){const auto c=Dot(Unit(Of(NodePoint(s,r.first))-x0),out.d);if(c>best){best=c;out.seam=r.first;out.bind=facets[r.first];}}
    if(out.seam==UINT64_MAX){out.ok=false;out.why="carrier: no seam edge along the corner direction";return out;}
    out.d=Unit(Of(NodePoint(s,out.seam))-x0);out.s=INFINITY;for(const auto& n:normals)out.s=std::min(out.s,Dot(n,out.d));
    if(!(out.s>1e-9L)){out.why="hemisphere: seam s*="+std::to_string(double(out.s));return out;}
  }
  out.ok=true;return out;
}
// Single-move affine kernel bound of the moved (non-column) cells of star(v) along d (L-A, one moving vertex).
struct Kernel { long double delta=INFINITY;Id cell=0; };
Kernel KernelBound(const std::vector<Cell>& cells,Id v,V d) {
  Kernel k;
  for(const auto& c:cells){std::array<V,4> p;int at=-1;for(unsigned i=0;i<4;++i){p[i]=Of(c.v[i].p);if(c.v[i].id==v)at=int(i);}if(at<0)continue;
    const auto o0=SDet(p[0],p[1],p[2],p[3]);auto q=p;q[at]=q[at]+d;const auto slope=SDet(q[0],q[1],q[2],q[3])-o0;
    if(slope<0){const auto t=o0/(-slope);if(t<k.delta){k.delta=t;k.cell=c.id;}}}
  return k;
}
std::vector<Cell> MovedStar(const State& s,const Rows& rows,Id v) {
  const auto apices=rows.Apices();std::vector<Cell> out;
  for(Id id:StarOf(s,v)){const auto& c=s.mesh.at(id).cell;bool col=false;for(auto n:c.v)col=col||apices.count(n.id);if(!col)out.push_back(c);}
  return out;
}
long double Need(const State& s){return (1+HeightTolerance)*s.h0*Kappa;}
struct Plan { bool ok=false;Dir dir;Kernel kernel;long double H=0,delta=0;V p;std::string why; };
Plan PlanLift(State& s,const Rows& rows,Id v) {
  Plan pl;pl.dir=DirectionAt(s,v);if(!pl.dir.ok){pl.why=pl.dir.why;return pl;}
  pl.kernel=KernelBound(MovedStar(s,rows,v),v,pl.dir.d);
  // band height: 0.9 of the clearance bound, capped at (1+g)h0; the floor is the height window top (1+T)h0 (the
  // LP decides the apex window); the 1.3h0 preference of rev 2.1 is kept only where the clearance admits it.
  const long double h0=s.h0;pl.H=std::min(0.9L*pl.kernel.delta*pl.dir.s,HighBand*h0);
  if(pl.H<LowBand*h0)pl.H=std::max(pl.H,(1+HeightTolerance)*h0);
  const V x=Of(NodePoint(s,v));
  // metric cap: the vertical column edge (v,v*) stays well inside the 1.8 edge gate (actual composed field)
  for(unsigned k=0;k<12&&pl.H>LowBand*h0;++k){long double l=0;try{l=s.field->TargetEdgeLength(Pt(x),Pt(x+(pl.H/pl.dir.s)*pl.dir.d));}catch(const std::exception&){break;}
    if(l<=1.5L)break;pl.H=std::max(LowBand*h0,0.9L*pl.H);}
  pl.delta=pl.H/pl.dir.s;pl.p=x+pl.delta*pl.dir.d;pl.ok=true;return pl;
}
long double Deficit(const State& s,const Plan& p){return p.dir.ok?std::max(0.L,Need(s)-p.kernel.delta*p.dir.s):Need(s);}
// v* on its carriers: mapped -> BindNewPoint (private context); faceted axis planes -> exact snap; else projection.
Node Constrain(const State& s,const PlanarReference* priv,Node node,const std::set<Id>& carriers,const std::set<Id>& bind) {
  if(carriers.empty())return node;
  if(s.mapped){if(!priv)throw std::runtime_error("mapped lift without a private context");return priv->BindNewPoint(node,bind.empty()?carriers:bind);}
  std::set<Id> comps;for(Id f:carriers)comps.insert(s.component.at(f));
  V p=Of(node.p);bool all=true;for(Id c:comps)all=all&&s.snap.count(c);
  if(all){node.p=Pt(Snap(s,comps,p));return node;}
  for(unsigned pass=0;pass<4;++pass)for(Id f:carriers)node.p=s.reference->Project(f,node.p);
  return node;
}

// ---- L-B' decision: exact LP over (l1,l2,eta,s) for the apex window of one column skin --------------------------
struct LP { bool ok=false;long double slack=-INFINITY,l1=0,l2=0,eta=0; };
LP ApexLP(const std::vector<std::array<V,3>>& tris,V a,V b,V c,V n,long double h0,long double T,bool fixH) {
  std::vector<std::array<long double,5>> A;  // A x <= rhs, x = (l1,l2,eta,s)
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
    bool feasible=true;for(const auto& row:A){const auto lhs=row[0]*x[0]+row[1]*x[1]+row[2]*x[2]+row[3]*x[3];
      feasible=feasible&&lhs<=row[4]+1e-11L*(1+std::abs(row[4]));}
    if(feasible&&x[3]>best.slack){best.slack=x[3];best.l1=x[0];best.l2=x[1];best.eta=x[2];}
  }
  best.ok=best.slack>1e-9L;return best;
}
// Outward column skin of wall face f_out (outward of the domain) in the lift state L (identity when unlifted):
// bottom f, top reversed L(f), lateral quads split by the lower-ID-bottom to higher-ID-top diagonal rule.
std::vector<Face> ColumnSkin(const Face& f,const std::function<Id(Id)>& L) {
  std::vector<Face> out{f};bool any=false;for(Id x:f)any=any||L(x)!=x;if(!any)return out;
  out.push_back({L(f[2]),L(f[1]),L(f[0])});
  for(unsigned i=0;i<3;++i){const Id x0=f[i],x1=f[(i+1)%3];const Id y0=L(x0),y1=L(x1);
    if(y0==x0&&y1==x1)continue;
    if(x0<x1){if(y1!=x1)out.push_back({x1,x0,y1});if(y0!=x0)out.push_back({x0,y0,y1});}
    else{if(y0!=x0)out.push_back({x1,x0,y0});if(y1!=x1)out.push_back({x1,y0,y1});}}
  return out;
}

// ---- R2 closed row transaction -------------------------------------------------------------------------------
struct Tx {
  bool ok=false;std::string cls,witness;std::vector<Face> failing;std::vector<Id> ids;Patch old,fresh;
  std::map<Id,Id> lifts;std::map<Face,Id> apices;std::set<Id> seam,blockers;std::set<Face> rebuilt;size_t old_cells=0,fresh_cells=0;
  double first_q=1,first_J=1,height_error=0;long double min_slack=INFINITY;
};
CellShape Shape(State& s,const Cell& c){try{return ShapeOf(s,c);}catch(const std::exception&){return {-1,-1};}}
Tx Transaction(State& s,const Rows& rows,const std::set<Id>& V0) {
  Tx tx;const long double h0=s.h0,T=HeightTolerance;
  // 1. cavity closure (§8.1): stars of moved vertices, every touched apex star, fixpoint; rebuilt faces.
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
  // 2. lifts
  std::map<Id,Node> nodes;for(const auto& c:tx.old.cells)for(auto n:c.v)nodes[n.id]=n;
  Id next=s.next_node;std::map<Id,Id> L=rows.lift;
  for(Id v:V0) {
    const auto pl=PlanLift(s,rows,v);if(!pl.ok){tx.cls=pl.why.substr(0,pl.why.find(':'));tx.witness="vertex "+std::to_string(v)+": "+pl.why;return tx;}
    Node node{next++,Pt(pl.p)};
    try{node=Constrain(s,priv.get(),node,pl.dir.carriers,pl.dir.bind);}catch(const std::exception& e){tx.cls="carrier";tx.witness="vertex "+std::to_string(v)+" binding: "+e.what();return tx;}
    nodes[node.id]=node;L[v]=node.id;tx.lifts[v]=node.id;if(pl.dir.rank==2)tx.seam.insert(node.id);}
  auto Lf=[&](Id x){const auto f=L.find(x);return f==L.end()?x:f->second;};
  // 3. moved cells: every non-column cell, v -> v* (all moved vertices at once; direct final orientation, §8.4)
  std::set<Id> oldApices;for(const auto& f:tx.rebuilt){const auto a=rows.apex.find(f);if(a!=rows.apex.end())oldApices.insert(a->second);}
  std::vector<Cell> fresh;
  for(const auto& c:tx.old.cells){bool col=false;for(auto n:c.v)col=col||oldApices.count(n.id);if(col)continue;
    Cell m=c;for(auto& n:m.v){const auto f=tx.lifts.find(n.id);if(f!=tx.lifts.end())n=nodes.at(f->second);}
    if(!(Orientation(m.v[0].p,m.v[1].p,m.v[2].p,m.v[3].p)>0)){tx.cls="clearance";std::ostringstream w;w<<"moved cell "<<c.id<<" inverts; |V|="<<V0.size()<<" moved in cell:";
      for(auto x:c.v)if(tx.lifts.count(x.id)){const auto pl=PlanLift(s,rows,x.id);w<<" v"<<x.id<<"(delta/deltaK="<<double(pl.delta/pl.kernel.delta)<<",kernel cell "<<pl.kernel.cell<<",rank "<<pl.dir.rank<<",d=("<<double(pl.dir.d.x)<<","<<double(pl.dir.d.y)<<","<<double(pl.dir.d.z)<<"),s*="<<double(pl.dir.s)<<")";}
      const auto wall=WallVertices(s);std::set<Id> liftedPts;for(const auto& r:rows.lift)liftedPts.insert(r.second);const auto apx=rows.Apices();
      w<<"; cell:";for(auto x:c.v)w<<" "<<x.id<<(wall.count(x.id)?"W":liftedPts.count(x.id)?"L":apx.count(x.id)?"A":"")<<"("<<x.p.x<<","<<x.p.y<<","<<x.p.z<<")";
      for(auto x:c.v)if(wall.count(x.id)&&!rows.lift.count(x.id)&&!V0.count(x.id))tx.blockers.insert(x.id);
      tx.witness=w.str();return tx;}
    fresh.push_back(m);}
  // 4. columns of every rebuilt face: apex from the exact LP, best first-child shape among the LP points
  std::map<Face,Face> outFace;for(const auto& f:PhysicalOf(s,cavity,true))outFace[f.key]=f.out;
  std::map<Face,int> lateralMarker;  // lateral triangles over junction edges -> carrier marker
  auto carrierMarker=[&](Id x0,Id x1)->int{for(const auto& sf:tx.old.surface){const int m=s.reference->Get(sf.facet).marker;if(s.walls.count(m))continue;
      if(Has(sf.v,x0)&&Has(sf.v,x1))return m;}return -1;};
  for(const auto& key:tx.rebuilt) {
    const Face f=outFace.at(key);const auto skin=ColumnSkin(f,Lf);
    const V a=Of(nodes.at(f[0]).p),b=Of(nodes.at(f[1]).p),c=Of(nodes.at(f[2]).p);const V n=-1.L*Outward(a,b,c);
    std::vector<std::array<V,3>> tris;for(size_t k=1;k<skin.size();++k)tris.push_back({Of(nodes.at(skin[k][0]).p),Of(nodes.at(skin[k][1]).p),Of(nodes.at(skin[k][2]).p)});
    for(unsigned i=0;i<3;++i){const Id x0=f[i],x1=f[(i+1)%3];const int m=carrierMarker(x0,x1);
      if(m>=0)for(size_t k=1;k<skin.size();++k){int hit=0;for(Id x:skin[k])hit+=x==x0||x==x1||x==Lf(x0)||x==Lf(x1);
        bool top=Has(skin[k],Lf(f[0]))&&Has(skin[k],Lf(f[1]))&&Has(skin[k],Lf(f[2]));if(hit==3&&!top&&skin[k]!=f)lateralMarker[Sorted(skin[k])]=m;}}
    std::vector<V> candidates;
    for(bool fixH:{false,true}){const auto lp=ApexLP(tris,a,b,c,n,h0,T,fixH);if(lp.ok){candidates.push_back(c+lp.l1*(a-c)+lp.l2*(b-c)+(lp.eta*h0)*n);tx.min_slack=std::min(tx.min_slack,lp.slack);}}
    if(candidates.empty()){tx.failing.push_back(key);if(tx.cls.empty()){tx.cls="partial-state";std::ostringstream w;
        w<<"face "<<key[0]<<","<<key[1]<<","<<key[2]<<": A_f empty (exact LP) with "<<skin.size()<<" skin faces; lifted:";
        const auto la=Norm(b-c),lb=Norm(c-a),lc=Norm(a-b),area=Norm(Cross(b-a,c-a))/2,r=2*area/(la+lb+lc);
        for(Id x:f)if(Lf(x)!=x){const V q=Of(nodes.at(Lf(x)).p)-Of(nodes.at(x).p);const auto hn=Dot(q,n);w<<" v"<<x<<"(H_n/h0="<<double(hn/h0)<<",tangential/inradius="<<double(Norm(q-hn*n)/r)<<")";}
        w<<"; inradius/h0="<<double(r/h0);
        {std::vector<std::array<V,3>> top;for(size_t k=1;k<skin.size();++k){bool lateral=false;for(Id x:skin[k])lateral=lateral||Has(f,x);if(!lateral)top.push_back(tris[k-1]);}
         w<<"; free-height LP slack="<<double(ApexLP(tris,a,b,c,n,h0,0.9L,false).slack);}
        tx.witness=w.str();}continue;}
    candidates.push_back((1.L/3)*(a+b+c)+h0*n);
    double bestScore=-1,bestCol=-1;std::vector<Cell> chosen;double fq=0,fJ=0;
    for(const auto& p:candidates){const Node apex{next,Pt(p)};std::vector<Cell> cone;bool pos=true;double colq=1;CellShape first{1,1};
      for(size_t k=0;k<skin.size()&&pos;++k){Cell cell{0,{apex,nodes.at(skin[k][0]),nodes.at(skin[k][1]),nodes.at(skin[k][2])}};
        pos=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0;if(!pos)break;const auto sh=Shape(s,cell);if(k==0)first=sh;colq=std::min(colq,sh.q);cone.push_back(cell);}
      if(!pos)continue;const double score=std::min(first.q/MinimumMeanRatio,first.J/MinimumScaledJacobian);
      if(score>bestScore+1e-12||(score>=1&&bestScore>=1&&colq>bestCol)){bestScore=score;bestCol=colq;chosen=cone;fq=first.q;fJ=first.J;}}
    if(chosen.empty()){tx.failing.push_back(key);if(tx.cls.empty()){tx.cls="partial-state";tx.witness="face: LP apex rounds outside its column";}continue;}
    if(bestScore<1){tx.failing.push_back(key);if(tx.cls.empty()){tx.cls="first-child";tx.witness="face "+std::to_string(key[0])+","+std::to_string(key[1])+","+std::to_string(key[2])+": first child q="+std::to_string(fq)+" J="+std::to_string(fJ);}continue;}
    tx.first_q=std::min(tx.first_q,fq);tx.first_J=std::min(tx.first_J,fJ);
    tx.apices[key]=next;nodes[next]=chosen.front().v[0];++next;fresh.insert(fresh.end(),chosen.begin(),chosen.end());
  }
  if(!tx.failing.empty())return tx;
  if(fresh.size()>MaximumReplacementCells){tx.cls="cap";tx.witness="cap-bound closure: "+std::to_string(fresh.size())+" fresh cells";tx.fresh_cells=fresh.size();return tx;}
  for(size_t i=0;i<fresh.size();++i)fresh[i].id=s.next_cell+i;tx.fresh_cells=fresh.size();
  // 5. surfaces: walls and artificial interface unchanged; carriers moved or lateral, located by FindFace; L-C.
  std::vector<Face> newSkin,oldSkin;
  try{newSkin=Incidence(fresh).Boundary();oldSkin=Incidence(tx.old.cells).Boundary();}catch(const std::exception& e){tx.cls="topology";tx.witness=e.what();return tx;}
  std::map<Face,SurfaceFace> oldPhysical;for(const auto& sf:tx.old.surface)oldPhysical[Sorted(sf.v)]=sf;
  std::set<Face> oldOriented(oldSkin.begin(),oldSkin.end()),newOriented(newSkin.begin(),newSkin.end());
  auto oriented=[&](const std::set<Face>& set,Face f){for(int r=0;r<3;++r){if(set.count(f))return true;std::rotate(f.begin(),f.begin()+1,f.end());}return false;};
  std::map<Id,Id> unlift;for(const auto& r:tx.lifts)unlift[r.second]=r.first;
  Patch out;out.cells=fresh;
  std::map<int,std::map<std::pair<Id,Id>,int>> oldEdges,newEdges;
  auto edges=[](std::map<std::pair<Id,Id>,int>& m,const Face& f){for(int i=0;i<3;++i){const Id a=f[i],b=f[(i+1)%3];const auto r=m.find({b,a});if(r!=m.end()){if(--r->second==0)m.erase(r);}else ++m[{a,b}];}};
  for(const auto& f:oldSkin){const auto p=oldPhysical.find(Sorted(f));if(p==oldPhysical.end()){if(!oriented(newOriented,f)){tx.cls="topology";tx.witness="artificial interface changed";return tx;}continue;}
    const int m=s.reference->Get(p->second.facet).marker;if(s.walls.count(m)){if(!oriented(newOriented,f)){tx.cls="topology";tx.witness="wall face lost";return tx;}continue;}
    edges(oldEdges[m],f);}
  for(const auto& f:newSkin) {
    const auto key=Sorted(f);const auto p=oldPhysical.find(key);
    if(p!=oldPhysical.end()&&oriented(oldOriented,f)){out.surface.push_back({f,p->second.facet});const int m=s.reference->Get(p->second.facet).marker;if(!s.walls.count(m))edges(newEdges[m],f);continue;}
    if(oriented(oldOriented,f))continue;  // unchanged artificial face
    int marker=-1;{Face back=f;for(auto& x:back){const auto u=unlift.find(x);if(u!=unlift.end())x=u->second;}const auto q=oldPhysical.find(Sorted(back));
      if(q!=oldPhysical.end())marker=s.reference->Get(q->second.facet).marker;}
    if(marker<0){const auto lm=lateralMarker.find(key);if(lm!=lateralMarker.end())marker=lm->second;}
    if(marker<0||s.walls.count(marker)){tx.cls="topology";tx.witness="unexpected new skin face";return tx;}
    std::array<Point,3> tri{nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p};Id facet=0;bool found=false;
    try{found=ref.FindFace(marker,tri,facet);}catch(const std::exception&){found=false;}
    if(!found){tx.cls="carrier";std::ostringstream w;w<<"new carrier face ("<<f[0]<<","<<f[1]<<","<<f[2]<<") leaves marker "<<marker<<" authority; containing old facets per vertex:";
      std::set<Id> fs;for(const auto& sf:tx.old.surface)if(s.reference->Get(sf.facet).marker==marker)fs.insert(s.reference->Canonical(sf.facet));
      for(Id x:f){w<<" "<<x<<"{";for(Id fc:fs){bool in=false;try{in=ref.Contains(fc,nodes.at(x).p);}catch(const std::exception&){}if(in)w<<fc<<" ";}w<<"}";}
      tx.witness=w.str();return tx;}
    out.surface.push_back({f,facet});edges(newEdges[marker],f);}
  // a corner lift v* bound on its carrier seam subdivides the seam edge of both carrier loops: merge a->v*->b
  for(auto& m:newEdges)for(Id p:tx.seam){std::pair<Id,Id> in{0,0},outE{0,0};int nin=0,nout=0;
    for(const auto& e:m.second){if(e.first.second==p){in=e.first;++nin;}if(e.first.first==p){outE=e.first;++nout;}}
    if(nin==1&&nout==1){m.second.erase(in);m.second.erase(outE);const std::pair<Id,Id> joined{in.first,outE.second},back{outE.second,in.first};
      const auto r=m.second.find(back);if(r!=m.second.end()){if(--r->second==0)m.second.erase(r);}else ++m.second[joined];}}
  for(auto& m:oldEdges){auto& n=newEdges[m.first];if(m.second!=n){tx.cls="carrier";tx.witness="L-C: carrier boundary loop changed on marker "+std::to_string(m.first);return tx;}}
  for(auto& m:newEdges)if(!oldEdges.count(m.first)&&!m.second.empty()){tx.cls="carrier";tx.witness="L-C: new carrier patch";return tx;}
  std::string reason;
  if(!ValidatePrescribedInterface(tx.old.cells,fresh,newSkin,reason,nullptr,MaximumCavityCells)){tx.cls="topology";tx.witness=reason;return tx;}
  try{ref.ExportParameters(out);}catch(const std::exception& e){tx.cls="carrier";tx.witness=std::string("parameters: ")+e.what();return tx;}
  {FirstHeightReport r;WallHeights none;none.tolerance=HeightTolerance;
   if(!CheckFirstHeight(out,none,*s.reference,r,reason)){tx.cls="carrier";tx.witness="surface: "+reason;return tx;}}
  // 6. U1 on every rebuilt face (T, foot, absolute first child) and every previously rowed face in the patch.
  for(const auto& sf:out.surface){const auto key=Sorted(sf.v);if(!s.walls.count(s.reference->Get(sf.facet).marker))continue;
    const bool rowed=rows.apex.count(key)&&std::all_of(key.begin(),key.end(),[&](Id x){return rows.lift.count(x);});
    if(!tx.rebuilt.count(key)&&!rowed) {
      // D.2 Prepare invariant: a face outside the rebuilt set may keep failing T only if it failed before
      Patch before,after;for(const auto& c:tx.old.cells){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)before.cells.push_back(c);}
      for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)after.cells.push_back(c);}
      before.surface=after.surface={sf};before.parameters=tx.old.parameters;after.parameters=out.parameters;
      FirstHeightReport r0,r1;std::string w0,w1;
      const bool was=before.cells.size()==1&&CheckFirstHeight(before,s.heights,*s.reference,r0,w0);
      if(was&&!(after.cells.size()==1&&CheckFirstHeight(after,s.heights,*s.reference,r1,w1))){tx.failing.push_back(key);tx.cls="invariant";tx.witness="unrebuilt wall face loses its passing first child: "+w1;}
      continue;}
    Patch one;for(const auto& c:fresh){unsigned k=0;for(auto n:c.v)k+=Has(key,n.id);if(k==3)one.cells.push_back(c);}
    one.surface={sf};for(const auto& c:one.cells)for(auto n:c.v){const auto pr=out.parameters.find(n.id);if(pr!=out.parameters.end())one.parameters.insert(*pr);}
    FirstHeightReport r;if(one.cells.size()!=1||!CheckFirstHeight(one,s.heights,*s.reference,r,reason)){
      tx.failing.push_back(key);tx.cls="height";tx.witness="face "+std::to_string(key[0])+": "+reason;continue;}
    tx.height_error=std::max(tx.height_error,r.maximum_relative_error);
    const auto sh=Shape(s,one.cells.front());if(sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian){tx.failing.push_back(key);tx.cls="first-child";tx.witness="rowed face first child q="+std::to_string(sh.q);}}
  if(!tx.failing.empty())return tx;
  tx.fresh=std::move(out);tx.ok=true;return tx;
}
void ApplyTx(State& s,Rows& rows,const Tx& tx) {
  Apply(s,tx.ids,tx.fresh);for(const auto& r:tx.lifts)rows.lift[r.first]=r.second;
  for(const auto& f:tx.rebuilt)rows.apex.erase(f);for(const auto& r:tx.apices)rows.apex[r.first]=r.second;
}

// ---- reporting -------------------------------------------------------------------------------------------------
struct Stage { size_t work=0,accepted=0,rejected=0;double seconds=0;std::string outcome;std::map<std::string,size_t> reasons;std::string last; };
std::string Key(const std::string& r){return swbi_dc2::Category(r).substr(0,110);}
std::string JsonStage(const Stage& g){std::ostringstream o;o<<std::setprecision(10)<<"{\"outcome\":\""<<g.outcome<<"\",\"work\":"<<g.work<<",\"accepted\":"<<g.accepted
  <<",\"rejected\":"<<g.rejected<<",\"seconds\":"<<g.seconds<<",\"reasons\":{";std::vector<std::pair<size_t,std::string>> top;for(const auto& r:g.reasons)top.emplace_back(r.second,r.first);
  std::sort(top.rbegin(),top.rend());for(size_t i=0;i<top.size()&&i<20;++i)o<<(i?",":"")<<junction_wp0::Json(top[i].second)<<':'<<top[i].first;
  o<<"},\"last_witness\":"<<junction_wp0::Json(g.last)<<'}';return o.str();}

// ---- phase W: wall/seam/junction SplitEdge to the composed surface target (no row present; Prepare kind) ---------
Stage PhaseW(State& s,size_t work,double seconds,std::ostream& log) {
  Stage g;const auto t0=Clock::now();size_t unmeasurable=0;
  for(unsigned sweep=0;;++sweep) {
    std::vector<std::pair<long double,Edge>> longs;std::set<Edge> seen;unmeasurable=0;
    for(const auto& wf:WallFaces(s))for(int i=0;i<3;++i){Edge e{std::min(wf.second[i],wf.second[(i+1)%3]),std::max(wf.second[i],wf.second[(i+1)%3])};
      if(!seen.insert(e).second)continue;long double l=0;try{l=Length(s,e[0],e[1]);}catch(const std::exception& x){++g.reasons["length: "+Key(x.what())];++unmeasurable;continue;}
      if(l>MaximumMetricEdge)longs.emplace_back(-l,e);}
    log<<"DC7 W sweep "<<sweep<<" wall_faces="<<WallFaces(s).size()<<" long_wall_edges="<<longs.size()<<" work="<<g.work<<"\n";
    if(longs.empty()){g.outcome=unmeasurable?"unmeasurable wall edges: "+std::to_string(unmeasurable):"complete";break;}
    std::sort(longs.begin(),longs.end());bool changed=false;
    for(const auto& row:longs) {
      if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
      const auto e=row.second;const auto star=Star(s,e);if(star.empty())continue;++g.work;
      if(star.size()>MaximumCavityCells){++g.rejected;++g.reasons["edge star exceeds 64"];continue;}
      Patch old;try{old=PatchOf(s,star);}catch(const std::exception& x){++g.rejected;++g.reasons[Key(x.what())];continue;}
      const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);
      std::vector<Point> points;try{points.push_back(s.field->SegmentPoint(a,b,.5));}catch(const std::exception&){}
      points.push_back({double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)});
      bool done=false;std::string why;
      // production order: exact on-edge SplitEdge, then InsertEdgeCavity (same-chart surface seams) with the same points
      for(int op=0;op<2&&!done;++op)for(const auto& p:points){Patch fresh;
        try{done=op==0?SplitEdge(old,e,{s.next_node,p},s.next_cell,*s.reference,fresh,why):InsertEdgeCavity(old,e,{s.next_node,p},s.next_cell,*s.reference,fresh,why);}
        catch(const std::exception& x){why=x.what();done=false;}
        if(done){Apply(s,star,fresh);++g.accepted;++g.reasons[op?"accepted InsertEdgeCavity":"accepted SplitEdge"];changed=true;break;}}
      if(!done){++g.rejected;++g.reasons[Key(why)];g.last=why;}
    }
    if(!g.outcome.empty())break;
    if(!changed){g.outcome="blocked";break;}
  }
  g.seconds=Seconds(t0);return g;
}

// ---- R1: clearance conditioning (Prepare kind; finite candidate set; strict local deficit decrease) --------------
struct VisibleResult { bool ok=false;std::vector<Id> cavity;std::string why; };
VisibleResult Visible(const State& s,Point p,const std::set<Id>& forbid) {
  VisibleResult r;std::set<Id> in;for(Id id:Locate(s,p))in.insert(id);if(in.empty()){r.why="outside";return r;}
  for(unsigned pass=0;pass<256;++pass){std::map<Face,std::pair<Face,Id>> skin;std::map<Face,int> count;
    for(Id id:in){const auto f=CellFaces(s.mesh.at(id).cell);for(unsigned k=0;k<4;++k){const auto key=Sorted(f[k]);++count[key];skin[key]={f[k],id};}}
    bool grew=false;
    for(const auto& row:skin){if(count.at(row.first)!=1)continue;const auto& f=row.second.first;const auto& owner=s.mesh.at(row.second.second);
      std::array<Point,3> q;for(unsigned k=0;k<3;++k)for(auto n:owner.cell.v)if(n.id==f[k])q[k]=n.p;
      if(Orientation(p,q[0],q[1],q[2])>0)continue;
      const auto faces=CellFaces(owner.cell);for(unsigned k=0;k<4;++k)if(Sorted(faces[k])==row.first&&owner.physical[k].present){r.why="physical face blocks";return r;}
      Id nb=UINT64_MAX;for(Id id:Star(s,{f[0],f[1]}))if(!in.count(id)){const auto& v=s.mesh.at(id).cell.v;if(std::any_of(v.begin(),v.end(),[&](Node n){return n.id==f[2];}))nb=id;}
      if(nb==UINT64_MAX){r.why="no neighbour";return r;}if(forbid.count(nb)){r.why="protected";return r;}
      in.insert(nb);grew=true;if(in.size()>MaximumCavityCells){r.why="cavity exceeds 64";return r;}break;}
    if(!grew){r.cavity.assign(in.begin(),in.end());r.ok=true;return r;}}
  r.why="did not close";return r;
}
long double LocalDeficit(State& s,const std::set<Id>& walls,const std::vector<Id>& removed,const std::vector<Cell>* added,
                         std::map<Id,Dir>& dirs) {
  std::set<Id> gone,touched;if(added)gone.insert(removed.begin(),removed.end());for(Id id:removed)for(auto n:s.mesh.at(id).cell.v)if(walls.count(n.id))touched.insert(n.id);
  long double sum=0;
  for(Id w:touched){auto d=dirs.find(w);if(d==dirs.end())d=dirs.emplace(w,DirectionAt(s,w)).first;if(!d->second.ok){sum+=Need(s);continue;}
    std::vector<Cell> cells;for(Id id:StarOf(s,w))if(!gone.count(id))cells.push_back(s.mesh.at(id).cell);
    if(added)for(const auto& c:*added)for(auto n:c.v)if(n.id==w)cells.push_back(c);
    const auto k=KernelBound(cells,w,d->second.d);sum+=std::max(0.L,Need(s)-k.delta*d->second.s);}
  return sum;
}
struct R1Stage : Stage { size_t deficient_before=0,deficient_after=0,moves=0,flips=0,steiner=0; };
R1Stage PhaseR1(State& s,const Rows& rows,size_t work,double seconds,std::ostream& log) {
  R1Stage g;const auto t0=Clock::now();const auto walls=WallVertices(s);
  auto deficient=[&](){std::vector<std::pair<long double,Id>> out;for(Id v:walls){if(Rim(s,v))continue;const auto pl=PlanLift(s,rows,v);const auto d=Deficit(s,pl);if(d>0)out.emplace_back(-d,v);}std::sort(out.begin(),out.end());return out;};
  auto list=deficient();g.deficient_before=list.size();
  for(unsigned round=0;round<4&&!list.empty();++round) {
    bool any=false;
    for(const auto& row:list) {
      const Id v=row.second;
      for(unsigned edit=0;edit<6;++edit) {
        if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
        const auto pl=PlanLift(s,rows,v);if(!pl.dir.ok||Deficit(s,pl)<=0)break;
        if(!s.mesh.count(pl.kernel.cell))break;
        const auto& blocking=s.mesh.at(pl.kernel.cell).cell;std::vector<Id> F;for(auto n:blocking.v)if(n.id!=v)F.push_back(n.id);
        std::map<Id,Dir> dirs;bool taken=false;
        auto attempt=[&](const std::string& op,const std::vector<Id>& ids,const Patch& fresh)->bool{
          ++g.work;const auto before=LocalDeficit(s,walls,ids,nullptr,dirs),after=LocalDeficit(s,walls,ids,&fresh.cells,dirs);
          if(!(after<before*(1-1e-9L))){++g.rejected;++g.reasons[op+": no clearance gain"];return false;}
          Apply(s,ids,fresh);++g.accepted;return true;};
        // c1 flip/reconnect of the blocking link face: star(v) + the cell across it, coned from v
        {Id across=UINT64_MAX;for(Id id:Star(s,{F[0],F[1]})){if(id==pl.kernel.cell)continue;const auto& c=s.mesh.at(id).cell;if(std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==F[2];}))across=id;}
         if(across!=UINT64_MAX){auto ids=StarOf(s,v);ids.insert(across);if(ids.size()<=MaximumCavityCells){std::vector<Id> list(ids.begin(),ids.end());Patch old,fresh;std::string why;
           try{old=PatchOf(s,list);if(Reconnect(old,v,s.next_cell,*s.reference,fresh,why)){if(attempt("Reconnect",list,fresh)){++g.flips;taken=true;}}else{++g.work;++g.rejected;++g.reasons["Reconnect: "+Key(why)];}}
           catch(const std::exception& x){++g.rejected;++g.reasons["Reconnect: "+Key(x.what())];}}}}
        // c2 MoveVertex of interior link vertices of the blocking face (no wall or carrier vertex moves)
        if(!taken)for(Id x:F){if(taken)break;bool boundary=false;for(Id id:StarOf(s,x)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)boundary=boundary||(r.physical[k].present&&Has(f[k],x));}
          if(boundary)continue;const auto star=StarOf(s,x);if(star.size()>MaximumCavityCells)continue;std::vector<Id> list(star.begin(),star.end());
          const V px=Of(NodePoint(s,x));V away=px-Of(NodePoint(s,v));if(Norm(away)>0)away=Unit(away);
          for(const V& u:{pl.dir.d,away}){if(taken)break;for(long double t:{1.L,2.L,4.L}){Patch old,fresh;std::string why;
            try{old=PatchOf(s,list);if(MoveVertex(old,x,{s.next_node,Pt(px+(t*Need(s))*u)},s.next_cell,*s.reference,fresh,why)){if(attempt("MoveVertex",list,fresh)){++g.moves;taken=true;break;}}
                else{++g.work;++g.rejected;++g.reasons["MoveVertex: "+Key(why)];}}catch(const std::exception& e){++g.rejected;++g.reasons["MoveVertex: "+Key(e.what())];}}}}
        // c3 Steiner point above an incident wall face at {2,3} times the clearance need
        if(!taken)for(const auto& f:PhysicalOf(s,StarOf(s,v),true)){if(taken)break;if(!Has(f.key,v))continue;
          const V a=Of(NodePoint(s,f.out[0])),b=Of(NodePoint(s,f.out[1])),c=Of(NodePoint(s,f.out[2]));const auto la=Norm(b-c),lb=Norm(c-a),lc=Norm(a-b);
          const V in=(1/(la+lb+lc))*(la*a+lb*b+lc*c);const V n=-1.L*Outward(a,b,c);
          for(long double k:{2.L,3.L}){const Point p=Pt(in+(k*Need(s))*n);const auto vis=Visible(s,p,{});if(!vis.ok){++g.work;++g.rejected;++g.reasons["Steiner: "+vis.why];continue;}
            Patch old,fresh;std::string why;try{old=PatchOf(s,vis.cavity);fresh.surface=old.surface;fresh.parameters=old.parameters;
              if(Cone(old.cells,{s.next_node,p},s.next_cell,fresh.cells,why)){s.reference->ExportParameters(fresh);if(attempt("Steiner",vis.cavity,fresh)){++g.steiner;taken=true;break;}}
              else{++g.work;++g.rejected;++g.reasons["Steiner: "+Key(why)];}}catch(const std::exception& e){++g.rejected;++g.reasons["Steiner: "+Key(e.what())];}}}
        if(!taken)break;any=true;
      }
      if(!g.outcome.empty())break;
    }
    if(!g.outcome.empty()||!any)break;
    list=deficient();
  }
  list=deficient();g.deficient_after=list.size();if(g.outcome.empty())g.outcome=list.empty()?"complete":"exhausted";
  if(!list.empty()){std::ostringstream w;w<<"clearance (R1 candidate set exhausted): vertex "<<list.front().second<<" deficit/h0="<<double(-list.front().first/s.h0);g.last=w.str();}
  g.seconds=Seconds(t0);log<<"DC7 R1 deficient "<<g.deficient_before<<" -> "<<g.deficient_after<<" accepted="<<g.accepted<<"\n";return g;
}

// ---- census: exact-LP feasibility of every wall face with all its vertices at their planned lift points -------------
struct Census { size_t faces=0,vertices=0,no_direction=0,deficient=0,lp_empty=0;std::map<std::string,size_t> why; };
Census FullStateCensus(State& s,const Rows& rows) {
  Census c;std::map<Id,Plan> plans;
  for(Id v:WallVertices(s)){if(Rim(s,v))continue;++c.vertices;auto pl=PlanLift(s,rows,v);if(!pl.ok){++c.no_direction;++c.why[pl.why.substr(0,40)];}else if(Deficit(s,pl)>0)++c.deficient;plans.emplace(v,pl);}
  for(const auto& wf:WallFaces(s)) {
    if(RimFace(s,wf.second))continue;++c.faces;const auto& row=s.mesh.at(wf.first);const auto faces=CellFaces(row.cell);Face out{};for(auto f:faces)if(Sorted(f)==wf.second)out=f;
    bool ok=true;for(Id x:out)ok=ok&&plans.at(x).ok;if(!ok){++c.lp_empty;continue;}
    std::map<Id,V> P;for(Id x:out){P[x]=Of(NodePoint(s,x));P[x+(Id(1)<<60)]=plans.at(x).p;}
    const auto skin=ColumnSkin(out,[](Id x){return x+(Id(1)<<60);});
    const V a=P[out[0]],b=P[out[1]],cc=P[out[2]];std::vector<std::array<V,3>> tris;for(size_t k=1;k<skin.size();++k)tris.push_back({P[skin[k][0]],P[skin[k][1]],P[skin[k][2]]});
    if(!ApexLP(tris,a,b,cc,-1.L*Outward(a,b,cc),s.h0,HeightTolerance,false).ok)++c.lp_empty;
  }
  return c;
}
std::string JsonCensus(const Census& c){std::ostringstream o;o<<"{\"wall_faces\":"<<c.faces<<",\"wall_vertices\":"<<c.vertices<<",\"no_direction\":"<<c.no_direction
  <<",\"clearance_deficient_vertices\":"<<c.deficient<<",\"faces_full_state_lp_empty\":"<<c.lp_empty<<",\"direction_failures\":{";bool k=false;
  for(const auto& r:c.why){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}o<<"}}";return o.str();}

// ---- R2: closed row transactions in canonical vertex order with bounded closure growth (§8.2) ----------------------
struct R2Stage : Stage { size_t rim_faces=0,transactions=0,lifted=0,unlifted=0,rowed=0,unrowed=0,max_closure=0,max_old=0,max_fresh=0;double first_q=1,first_J=1,height_error=0;
  std::map<std::string,size_t> classes; };
R2Stage PhaseR2(State& s,Rows& rows,double seconds,std::ostream& log) {
  R2Stage g;const auto t0=Clock::now();
  for(unsigned pass=0;pass<2;++pass) {
    for(Id v:WallVertices(s)) {
      if(rows.lift.count(v)||Rim(s,v))continue;if(Seconds(t0)>=seconds){g.outcome="limit";break;}
      std::set<Id> V0{v};Tx tx;
      for(unsigned grow=0;grow<16;++grow) {
        ++g.work;tx=Transaction(s,rows,V0);
        log<<"DC7 R2 tx v="<<v<<" |V|="<<V0.size()<<" ok="<<tx.ok<<" old="<<tx.old_cells<<" fresh="<<tx.fresh_cells<<" cls="<<tx.cls<<" "<<tx.witness<<"\n";
        if(tx.ok)break;
        if(tx.cls=="clearance"){bool added=false;for(Id x:tx.blockers)if(!Rim(s,x)&&V0.insert(x).second)added=true;if(added)continue;break;}
        if(tx.cls!="partial-state"&&tx.cls!="first-child"&&tx.cls!="height")break;
        bool added=false;std::sort(tx.failing.begin(),tx.failing.end());
        // §8.2: the canonical first failing face grows V by its unlifted vertices, or rejects at once with its witness
        {const auto& f=tx.failing.front();for(Id x:f)if(!rows.lift.count(x)&&!V0.count(x)&&!Rim(s,x)){V0.insert(x);added=true;}}
        if(!added){tx.witness="no growth: "+tx.witness;break;}
      }
      if(tx.ok){ApplyTx(s,rows,tx);++g.accepted;++g.transactions;g.max_closure=std::max(g.max_closure,V0.size());g.max_old=std::max(g.max_old,tx.old_cells);
        g.max_fresh=std::max(g.max_fresh,tx.fresh_cells);g.first_q=std::min(g.first_q,tx.first_q);g.first_J=std::min(g.first_J,tx.first_J);g.height_error=std::max(g.height_error,tx.height_error);}
      else if(pass==1||true){++g.rejected;++g.classes[tx.cls];++g.reasons[tx.cls+": "+Key(tx.witness)];g.last="vertex "+std::to_string(v)+" ["+tx.cls+"] "+tx.witness;}
    }
    if(!g.outcome.empty())break;
  }
  for(Id v:WallVertices(s))if(!Rim(s,v))(rows.lift.count(v)?g.lifted:g.unlifted)++;
  for(const auto& wf:WallFaces(s)){if(RimFace(s,wf.second)){++g.rim_faces;continue;}const bool r=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});(r?g.rowed:g.unrowed)++;}
  if(g.outcome.empty())g.outcome=g.unrowed?"incomplete":"complete";g.seconds=Seconds(t0);
  log<<"DC7 R2 transactions="<<g.transactions<<" lifted="<<g.lifted<<" unlifted="<<g.unlifted<<" rowed="<<g.rowed<<" unrowed="<<g.unrowed<<"\n";return g;
}

// ---- V: existing volume operators; columns star(p_f) are fixed interfaces (no cavity may contain a column cell) ----
Gates Final(State& s);
struct VStage : Stage { size_t column_demand=0,sweeps=0; };
VStage PhaseV(State& s,const Rows& rows,size_t work,double seconds,std::ostream& log) {
  VStage g;const auto t0=Clock::now();
  auto attempt=[&](const std::string& op,Kind kind,const std::vector<Id>& ids,const Patch& old,const Patch& fresh)->bool{
    ++g.work;const auto d=Admit(s,old,fresh,kind).second;
    if(!d.ok){++g.rejected;++g.reasons[op+" ["+std::to_string(d.category)+"] "+Key(d.reason)];g.last=op+": "+d.reason;return false;}
    Apply(s,ids,fresh);++g.accepted;return true;};
  for(;;++g.sweeps) {
    if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
    const auto columns=ColumnCells(s,rows);bool changed=false;g.column_demand=0;
    auto protectedIds=[&](const std::vector<Id>& ids){for(Id id:ids)if(columns.count(id))return true;return false;};
    std::vector<std::pair<long double,Edge>> longs;std::set<Edge> seen;
    for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
      const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      long double l=0;try{l=Length(s,e[0],e[1]);}catch(const std::exception&){continue;}if(l>MaximumMetricEdge)longs.emplace_back(-l,e);}}
    std::sort(longs.begin(),longs.end());
    for(const auto& row:longs) {
      if(g.work>=work||Seconds(t0)>=seconds)break;const auto star=Star(s,row.second);if(star.empty())continue;
      if(protectedIds(star)){++g.column_demand;continue;}
      bool taken=false;
      for(int variant=0;variant<RefineVariants&&!taken;++variant) {
        std::vector<Id> ids;Patch old,fresh;std::string op,reason;
        if(!RefineProposal(s,row.second,ids,old,fresh,op,reason,variant)){if(reason!="no further variant"){++g.work;++g.rejected;++g.reasons["Refine build: "+Key(reason)];}continue;}
        if(protectedIds(ids)){++g.rejected;++g.reasons["Refine: protected column"];continue;}
        Patch repaired;ConnectivityRepairReport report;std::string why;bool better=false;
        try{better=RepairMetricConnectivity(fresh,*s.field,*s.reference,s.heights,s.next_cell+4096,repaired,report,why,128,false);}catch(const std::exception&){better=false;}
        if(better){for(size_t i=0;i<repaired.cells.size();++i)repaired.cells[i].id=s.next_cell+i;try{s.reference->ExportParameters(repaired);}catch(const std::exception&){better=false;}}
        if(better&&attempt(op+"+repair",Kind::Refine,ids,old,repaired)){taken=true;break;}
        if(attempt(op,Kind::Refine,ids,old,fresh)){taken=true;break;}
      }
      changed|=taken;
    }
    std::vector<std::pair<double,Id>> bad;for(const auto& row:s.mesh){if(columns.count(row.first))continue;const auto sh=Shape(s,row.second.cell);if(sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian)bad.emplace_back(sh.q,row.first);}
    std::sort(bad.begin(),bad.end());
    for(const auto& row:bad) {
      if(g.work>=work||Seconds(t0)>=seconds)break;if(!s.mesh.count(row.second))continue;
      const auto cols=ColumnCells(s,rows);std::set<Id> star;for(auto n:s.mesh.at(row.second).cell.v)for(Id id:StarOf(s,n.id))if(!cols.count(id))star.insert(id);
      if(star.size()>MaximumCavityCells){star.clear();const auto& c=s.mesh.at(row.second).cell;for(Id id:Star(s,{c.v[0].id,c.v[1].id}))if(!cols.count(id))star.insert(id);}
      if(star.empty()||star.size()>MaximumCavityCells){++g.work;++g.rejected;++g.reasons["Repair build: star exceeds 64"];continue;}
      std::vector<Id> ids(star.begin(),star.end());Patch old,fresh;std::string reason;ConnectivityRepairReport report;
      std::set<Id> movable,frozen=rows.Apices();for(const auto& r:rows.lift){frozen.insert(r.first);frozen.insert(r.second);}
      for(Id id:ids)for(auto n:s.mesh.at(id).cell.v)if(!frozen.count(n.id))movable.insert(n.id);
      bool ok=false;try{old=PatchOf(s,ids);ok=RepairMetricConnectivity(old,*s.field,*s.reference,s.heights,s.next_cell,fresh,report,reason,64,true,&movable);}catch(const std::exception& x){reason=x.what();}
      if(!ok){++g.work;++g.rejected;++g.reasons["Repair build: "+Key(reason)];continue;}
      changed|=attempt("RepairMetricConnectivity",Kind::Repair,ids,old,fresh);
    }
    const auto gates=Final(s);
    log<<"DC7 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" height_fail="<<gates.height_failures<<" column_demand="<<g.column_demand<<" work="<<g.work<<"\n";
    std::cout<<"DC7 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" work="<<g.work<<std::endl;
    if(gates.complete){g.outcome="complete";break;}
    if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
    if(!changed){g.outcome="blocked";break;}
  }
  g.seconds=Seconds(t0);return g;
}
// Unchanged absolute gates (as layer2d::FinalGates), with an unmeasurable edge counted as a failing edge, not thrown.
Gates Final(State& s) {
  Gates g;g.cells=s.mesh.size();std::set<Edge> seen;size_t bad=0;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;const auto sh=Shape(s,c);g.q=std::min(g.q,sh.q);g.J=std::min(g.J,sh.J);
    g.bad_shape+=sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      long double l=0;try{l=Length(s,e[0],e[1]);}catch(const std::exception&){++bad;l=INFINITY;}
      if(std::isfinite(l))g.lmax=std::max(g.lmax,l);g.long_edges+=l>MaximumMetricEdge;
      if(l>MaximumMetricEdge){bool w=false,o=false;for(Id id:Star(s,e)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
          if(r.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1]))(s.walls.count(s.reference->Get(r.physical[k].facet).marker)?w:o)=true;}
        if(w&&o)++g.long_seam;else if(w)++g.long_wall;else if(o)++g.long_other_physical;else ++g.long_interior;}}}
  for(const auto& wf:WallFaces(s)){++g.wall_faces;std::string why;if(!HeightT(s,PatchOf(s,{wf.first}),why))++g.height_failures;}
  g.complete=!g.bad_shape&&!g.long_edges&&!g.height_failures&&!bad;return g;
}
size_t Unmeasurable(State& s){std::set<Edge> seen;size_t bad=0;for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
  const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;try{Length(s,e[0],e[1]);}catch(const std::exception&){++bad;}}}return bad;}
// Final classification: long edges (column / wall / seam / carrier / interior), bad cells (first child / column / other).
std::string JsonFinal(State& s,const Rows& rows) {
  const auto g=Final(s);const auto cols=ColumnCells(s,rows);size_t longColumn=0,badFirst=0,badColumn=0,badOther=0;std::set<Edge> seen;
  double fq=1,fJ=1,herr=0;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;const auto sh=Shape(s,c);const bool first=WallChild(s,row.first);
    if(first){fq=std::min(fq,sh.q);fJ=std::min(fJ,sh.J);}
    if(sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian)(first?badFirst:cols.count(row.first)?badColumn:badOther)++;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      long double l=0;try{l=Length(s,e[0],e[1]);}catch(const std::exception&){continue;}
      if(l>MaximumMetricEdge){bool inColumn=true;for(Id id:Star(s,e))inColumn=inColumn&&cols.count(id);longColumn+=inColumn;}}}
  for(const auto& wf:WallFaces(s)){std::string why;double w=0;HeightT(s,PatchOf(s,{wf.first}),why,&w);herr=std::max(herr,w);}
  std::ostringstream o;o<<std::setprecision(10)<<"{\"cells\":"<<g.cells<<",\"q_min\":"<<g.q<<",\"J_min\":"<<g.J<<",\"edge_max\":"<<double(g.lmax)<<",\"bad_shape\":"<<g.bad_shape
   <<",\"bad_first_child\":"<<badFirst<<",\"bad_column_other\":"<<badColumn<<",\"bad_outside_columns\":"<<badOther<<",\"long_edges\":"<<g.long_edges
   <<",\"long_by_class\":{\"wall\":"<<g.long_wall<<",\"seam\":"<<g.long_seam<<",\"other_physical\":"<<g.long_other_physical<<",\"interior\":"<<g.long_interior<<",\"inside_columns_only\":"<<longColumn<<"}"
   <<",\"wall_faces\":"<<g.wall_faces<<",\"height_failures\":"<<g.height_failures<<",\"first_child_q_min\":"<<fq<<",\"first_child_J_min\":"<<fJ<<",\"max_height_error\":"<<herr
   <<",\"unmeasurable_edges\":"<<Unmeasurable(s)<<",\"complete\":"<<g.complete<<'}';return o.str();
}

// Faceted fixture -> v6 mapped-component authority (as the A1 reference: MappedFacetReference with released certified
// planar seams, 1e-13 relative mapping error per facet, every original vertex bound to itself). Crease and junction
// points then bind to their original edge feature (production SplitEdge/BindNewPoint) instead of requiring an
// exactly representable on-edge binary64 point.
void MakeMapped(State& s) {
  long double scale=0;for(const auto& f:s.facets)for(auto x:f.v)scale=std::max({scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});
  std::map<Id,double> errors;for(const auto& f:s.facets)errors.emplace(f.id,double(1e-13L*std::max(1.L,scale)));
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(s.reference,errors,7,SIZE_MAX,true);
  BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:s.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto x:f.v)points.emplace(x.id,BoundaryPoint{x,authority->Vertex(x.id)});}
  s.reference=PlanarReference::WithParameters(s.reference,std::make_shared<BoundaryParameterState>(authority,points,skin));
  s.mapped=true;s.overlay.clear();Prepare(s);
}
// Final mesh with the native gates (appended to layer2d::Export): per cell metric q, J, bad flag, first-child flag,
// column id (apex) and class (0 outside columns, 1 first child, 2 other column cell); per metric long edge (> 1.8)
// its composed length (-1 unmeasurable), class (0 interior, 1 wall, 2 seam, 3 other physical, 4 unmeasurable) and
// whether every cell of its star is a column cell. Runs once at the end of a case.
void ExportGates(State& s,const Rows& rows,const std::string& path) {
  Export(s,path);std::ofstream out(path,std::ios::app);out<<std::setprecision(17);
  std::map<Id,Id> column;for(const auto& r:rows.apex)for(Id id:StarOf(s,r.second))column.emplace(id,r.second);
  out<<"cell_gates "<<s.mesh.size()<<"\n";
  for(const auto& row:s.mesh){const auto sh=Shape(s,row.second.cell);const bool first=WallChild(s,row.first);const auto c=column.find(row.first);
    const bool bad=sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian;
    out<<row.first<<' '<<sh.q<<' '<<sh.J<<' '<<bad<<' '<<first<<' '<<(c==column.end()?-1:(long long)c->second)<<' '<<(first?1:c!=column.end()?2:0)<<"\n";}
  std::vector<std::string> lines;std::set<Edge> seen;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
    const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
    long double l=-1;bool ok=true;try{l=Length(s,e[0],e[1]);}catch(const std::exception&){ok=false;}
    if(ok&&!(l>MaximumMetricEdge))continue;
    bool w=false,o=false,inColumn=true;for(Id id:Star(s,e)){inColumn=inColumn&&column.count(id);const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);
      for(unsigned k=0;k<4;++k)if(r.physical[k].present&&Has(f[k],e[0])&&Has(f[k],e[1]))(s.walls.count(s.reference->Get(r.physical[k].facet).marker)?w:o)=true;}
    const int cls=!ok?4:w&&o?2:w?1:o?3:0;std::ostringstream x;x<<std::setprecision(10)<<e[0]<<' '<<e[1]<<' '<<(ok?double(l):-1.0)<<' '<<cls<<' '<<inColumn;lines.push_back(x.str());}}
  out<<"long_edges "<<lines.size()<<"\n";for(const auto& l:lines)out<<l<<"\n";
}
// ---- loaders with the production composition (SU2NativeBL3D) and the production D1 law -----------------------------
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
  s->layers=layers;s->field=std::make_unique<FrozenField>(donors,SU2NativeBL3D::Composition(layers,core),false,std::vector<Id>{0},SU2NativeBL3D::EdgeGeometry(layers,core));
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
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),false,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core));
  s->heights.tolerance=HeightTolerance;s->lengths.clear();MakeMapped(*s);return s;
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
  s->field=std::make_unique<FrozenField>(std::move(donors),SU2NativeBL3D::Composition(s->layers,s->core),false,std::vector<Id>{},SU2NativeBL3D::EdgeGeometry(s->layers,s->core));
  Prepare(*s);MakeMapped(*s);return s;
}
}  // namespace dc7

TEST_CASE("DC-7 W -> R1 -> R2 closed row transactions -> V with protected columns", "[.][NativeFirstRowDC7]") {
  using namespace dc7;
  World world;REQUIRE(world.size==1);
  const char* folder=std::getenv("SU2_DC7_ROOT");REQUIRE(folder);const std::string root(folder);
  const char* wp0b=std::getenv("SU2_DC7_WP0B");REQUIRE(wp0b);const std::string fixtures(wp0b);
  const char* m6=std::getenv("SU2_DC7_M6");const std::string cuts=m6?m6:"";
  const char* only=std::getenv("SU2_DC7_CASES");REQUIRE(only);
  std::stringstream names(only);
  for(std::string name;std::getline(names,name,',');) {
    if(name.empty()||std::ifstream(root+"/dc7_"+name+".json").good())continue;
    const auto start=Clock::now();std::ostringstream json,log;json<<std::setprecision(12)<<"{\"case\":\""<<name<<"\",\"limits\":{\"work\":8192,\"seconds\":120}";
    auto load=[&]()->std::unique_ptr<State>{
      if(name=="annulus")return LoadA1(fixtures);
      if(name.rfind("m6_",0)==0)return LoadM6Cut(cuts+"/"+name+".txt");
      return LoadTube(fixtures+"/"+name+".txt");};
    auto s=load();Rows rows;
    json<<",\"input\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<",\"h0\":"<<s->h0<<",\"core\":"<<s->core<<",\"mapped\":"<<s->mapped
        <<",\"d1\":{\"segments\":"<<s->layers->GetFeatures().segments<<",\"images\":"<<s->layers->GetFeatures().images<<",\"corners\":"<<s->layers->GetFeatures().corners
        <<",\"angle\":"<<s->layers->GetFeatures().angle<<",\"R_f\":"<<s->layers->GetFeatures().maximumRadius<<"}}";
    std::cout<<"DC7 "<<name<<" loaded: cells="<<s->mesh.size()<<" wall_faces="<<WallFaces(*s).size()<<std::endl;
    const auto w=PhaseW(*s,8192,120,log);json<<",\"W\":"<<JsonStage(w)<<",\"after_W\":{\"cells\":"<<s->mesh.size()<<",\"wall_faces\":"<<WallFaces(*s).size()<<'}';
    std::cout<<"DC7 "<<name<<" W "<<w.outcome<<" wall_faces="<<WallFaces(*s).size()<<std::endl;
    const auto c0=FullStateCensus(*s,rows);json<<",\"census_after_W\":"<<JsonCensus(c0);
    const auto r1=PhaseR1(*s,rows,4096,120,log);json<<",\"R1\":"<<JsonStage(r1)<<",\"R1_counts\":{\"deficient_before\":"<<r1.deficient_before<<",\"deficient_after\":"<<r1.deficient_after
        <<",\"moves\":"<<r1.moves<<",\"flips\":"<<r1.flips<<",\"steiner\":"<<r1.steiner<<'}';
    const auto c1=FullStateCensus(*s,rows);json<<",\"census_after_R1\":"<<JsonCensus(c1);
    std::cout<<"DC7 "<<name<<" R1 "<<r1.outcome<<" deficient "<<r1.deficient_before<<"->"<<r1.deficient_after<<" lp_empty="<<c1.lp_empty<<std::endl;
    const auto r2=PhaseR2(*s,rows,600,log);
    json<<",\"R2\":"<<JsonStage(r2)<<",\"R2_counts\":{\"transactions\":"<<r2.transactions<<",\"lifted\":"<<r2.lifted<<",\"unlifted\":"<<r2.unlifted<<",\"rowed\":"<<r2.rowed
        <<",\"unrowed\":"<<r2.unrowed<<",\"rim_faces_excluded\":"<<r2.rim_faces<<",\"max_closure_vertices\":"<<r2.max_closure<<",\"max_old_cells\":"<<r2.max_old<<",\"max_fresh_cells\":"<<r2.max_fresh
        <<",\"first_q_min\":"<<r2.first_q<<",\"first_J_min\":"<<r2.first_J<<",\"max_height_error\":"<<r2.height_error<<",\"witness_classes\":{";
    {bool k=false;for(const auto& c:r2.classes){json<<(k?",":"")<<junction_wp0::Json(c.first)<<':'<<c.second;k=true;}}json<<"}}";
    std::cout<<"DC7 "<<name<<" R2 rowed="<<r2.rowed<<" unrowed="<<r2.unrowed<<" last="<<r2.last<<std::endl;
    json<<",\"after_R\":"<<JsonFinal(*s,rows);
    VStage v;
    // V runs in every case (with incomplete rows it measures the transition only; the construction has already failed)
    v=PhaseV(*s,rows,8192,120,log);json<<",\"V\":"<<JsonStage(v)<<",\"V_column_demand\":"<<v.column_demand<<",\"V_sweeps\":"<<v.sweeps<<",\"V_with_complete_rows\":"<<(r2.unrowed==0);
    json<<",\"final\":"<<JsonFinal(*s,rows);
    ExportGates(*s,rows,root+"/dc7_"+name+"_final.mesh");
    {auto fresh=load();fresh->mesh=s->mesh;fresh->overlay=s->overlay;Index(*fresh);const auto ga=Final(*fresh);
     json<<",\"reaudit\":{\"q_min\":"<<ga.q<<",\"J_min\":"<<ga.J<<",\"edge_max\":"<<double(ga.lmax)<<",\"long_edges\":"<<ga.long_edges<<",\"height_failures\":"<<ga.height_failures<<",\"complete\":"<<ga.complete<<'}';}
    json<<",\"total_seconds\":"<<Seconds(start)<<"}\n";
    std::ofstream(root+"/dc7_"+name+".json")<<json.str();std::ofstream(root+"/dc7_"+name+".log")<<log.str();
    std::cout<<"DC7 "<<name<<" done in "<<Seconds(start)<<" s"<<std::endl;
  }
}

// M6 field export at the configured target (ADAP_ANGLE as configured): selected sensor per owned node, D1 summary,
// normalization; saved state, no CFD step, no construction. Run on 2 ranks (serial donor admission refuses M6).
TEST_CASE("DC-7 M6 selected field export", "[.][NativeFirstRowDC7Field]") {
  class Driver : public CSinglezoneDriver {
   public:
    using CSinglezoneDriver::CSinglezoneDriver;
    CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
    CSolver& Solver(){return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];}
    CConfig& Config(){return *config_container[ZONE_0];}
  };
  World world;const auto* configFile=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(configFile);
  auto driver=std::make_unique<Driver>(const_cast<char*>(configFile),1,world.comm);driver->Preprocess(0);
  REQUIRE_FALSE(driver->Config().GetTime_Domain());
  const auto begin=world.seconds();driver->ComputeMetric();const auto seconds=world.seconds()-begin;
  const auto family=driver->Solver().GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->layers);
  double core=std::numeric_limits<double>::max();
  {std::ofstream out("selected_rank"+std::to_string(world.rank)+".bin",std::ios::binary);const uint64_t count=driver->Geometry().GetnPointDomain();out.write(reinterpret_cast<const char*>(&count),8);
   for(unsigned long p=0;p<count;++p){const uint64_t id=driver->Geometry().nodes->GetGlobalIndex(p);out.write(reinterpret_cast<const char*>(&id),8);
     for(unsigned k=0;k<3;++k){const double x=SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,k));out.write(reinterpret_cast<const char*>(&x),8);}
     const auto& m=family->selected.at(p);double t[9];for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b){t[3*a+b]=SU2_TYPE::GetValue(m.m[a][b]);out.write(reinterpret_cast<const char*>(&t[3*a+b]),8);}
     su2double mm[3][3]={{m.m[0][0],m.m[0][1],m.m[0][2]},{m.m[1][0],m.m[1][1],m.m[1][2]},{m.m[2][0],m.m[2][1],m.m[2][2]}},vec[3][3],val[3],work[3];
     CBlasStructure::EigenDecomposition(mm,vec,val,3,work);core=std::min(core,SU2_TYPE::GetValue(*std::min_element(val,val+3)));}
   REQUIRE(bool(out));}
  core=CPassiveComm::Allreduce(core,CPassiveComm::Op::MIN,world.comm);
  const auto& f=family->layers->GetFeatures();const auto profile=family->layers->GetOuterNormal();
  if(!world.rank) {
    std::ofstream out("field.json");out<<std::setprecision(17)<<"{\"CFD_steps\":0,\"construction\":false,\"ranks\":"<<world.size<<",\"seconds\":"<<seconds
      <<",\"angle\":"<<f.angle<<",\"segments\":"<<f.segments<<",\"image_segments\":"<<f.images<<",\"corners\":"<<f.corners<<",\"image_corners\":"<<f.imageCorners
      <<",\"carrier_ends\":"<<f.carrierEnds<<",\"carrier_corners\":"<<f.carrierCorners<<",\"free_corners\":"<<f.freeCorners<<",\"planes\":"<<f.planes
      <<",\"slope\":"<<f.slope<<",\"R_f\":"<<f.maximumRadius<<",\"identity\":"<<f.identity<<",\"parameter\":"<<profile.parameter<<",\"version\":"<<profile.version
      <<",\"normalization_status\":"<<unsigned(family->normalization.status)<<",\"estimate\":"<<family->normalization.estimate.value
      <<",\"uncertainty\":";if(std::isfinite(family->normalization.estimate.uncertainty))out<<family->normalization.estimate.uncertainty;else out<<"null";
    out<<",\"family_core\":"<<family->core<<",\"sensor_core_min_eigenvalue\":"<<core<<"}\n";REQUIRE(bool(out));
  }
  {Mute mute;driver->Finalize();driver.reset();}
}
