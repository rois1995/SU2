/*!
 * \file BLFirstRow7.inl
 * \brief BL first-row transactions of DC-7 (lift directions, apex LP, closed row transactions, visibility).
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc7.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// BL_FIRST_ROW_DESIGN rev 2.1, DC-7: (history: formerly test-only) [NativeFirstRowDC7] harness of the WP-D construction order
// W (wall/seam/junction subdivision to the composed D1 surface target, no row present) -> R1 (clearance conditioning)
// -> R2 (closed row transactions T(V) = LiftCone + FaceApex, apex window decided by the exact three-variable LP over
// the whole height interval, persistent columns col(f) = star(p_f)) -> V (existing volume operators, columns
// protected). (DC-14: production.) Production primitives only: SplitEdge,
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
inline double Seconds(Clock::time_point t){return std::chrono::duration<double>(Clock::now()-t).count();}
// ---- orientation convention: SDet(a,b,c,d) > 0 iff the production Orientation of the cell (a,b,c,d) is positive ----
inline long double Det(V a,V b,V c,V d){return Dot(b-a,Cross(c-a,d-a));}
inline long double Sigma() {
  static const long double s=Orientation({0,0,0},{1,0,0},{0,1,0},{0,0,1})>0?1.L:-1.L;return s;
}
inline long double SDet(V a,V b,V c,V d){return Sigma()*Det(a,b,c,d);}
// Outward unit normal of an outward-oriented skin triangle (the cell interior lies on the negative side).
inline V Outward(V a,V b,V c){return Sigma()*Unit(Cross(b-a,c-a));}

struct Rows {
  std::map<Id,Id> lift;            // wall vertex -> lifted vertex v*
  std::map<Face,Id> apex;          // sorted wall face -> current column apex p_f
  std::set<Id> Apices() const {std::set<Id> a;for(const auto& r:apex)a.insert(r.second);return a;}
};
struct WF { Face key{},out{};Id facet=0,owner=0;int marker=0; };
// Physical faces of the given cells (wall==true: wall markers only; false: carriers only).
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
// Neighbourhood cut fixtures declare their artificial skin as marker CUT: its wall vertices are the fixed rim.
inline bool Rim(const State& s,Id v) {
  for(Id id:StarOf(s,v)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
    if(r.physical[k].present&&Has(f[k],v)&&s.markers.at(s.reference->Get(r.physical[k].facet).marker)=="CUT")return true;}
  return false;
}
inline bool RimFace(const State& s,const Face& f){for(Id x:f)if(Rim(s,x))return true;return false;}

// ---- lift direction: maximize min n_f.d over unit d with d.nu = 0 on every carrier plane at v (rank test) ----------
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
struct Dir { bool ok=false;V d;long double s=0;std::set<Id> carriers,bind;int rank=0;Id seam=UINT64_MAX;std::string why; };
inline Dir DirectionAt(State& s,Id v) {
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
inline Kernel KernelBound(const std::vector<Cell>& cells,Id v,V d) {
  Kernel k;
  for(const auto& c:cells){std::array<V,4> p;int at=-1;for(unsigned i=0;i<4;++i){p[i]=Of(c.v[i].p);if(c.v[i].id==v)at=int(i);}if(at<0)continue;
    const auto o0=SDet(p[0],p[1],p[2],p[3]);auto q=p;q[at]=q[at]+d;const auto slope=SDet(q[0],q[1],q[2],q[3])-o0;
    if(slope<0){const auto t=o0/(-slope);if(t<k.delta){k.delta=t;k.cell=c.id;}}}
  return k;
}
inline std::vector<Cell> MovedStar(const State& s,const Rows& rows,Id v) {
  const auto apices=rows.Apices();std::vector<Cell> out;
  for(Id id:StarOf(s,v)){const auto& c=s.mesh.at(id).cell;bool col=false;for(auto n:c.v)col=col||apices.count(n.id);if(!col)out.push_back(c);}
  return out;
}
inline long double Need(const State& s){return (1+HeightTolerance)*s.h0*Kappa;}
struct Plan { bool ok=false;Dir dir;Kernel kernel;long double H=0,delta=0;V p;std::string why; };
inline Plan PlanLift(State& s,const Rows& rows,Id v) {
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
inline long double Deficit(const State& s,const Plan& p){return p.dir.ok?std::max(0.L,Need(s)-p.kernel.delta*p.dir.s):Need(s);}
// v* on its carriers: mapped -> BindNewPoint (private context); faceted axis planes -> exact snap; else projection.
inline Node Constrain(const State& s,const PlanarReference* priv,Node node,const std::set<Id>& carriers,const std::set<Id>& bind) {
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
inline LP ApexLP(const std::vector<std::array<V,3>>& tris,V a,V b,V c,V n,long double h0,long double T,bool fixH) {
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
inline std::vector<Face> ColumnSkin(const Face& f,const std::function<Id(Id)>& L) {
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
inline CellShape Shape(State& s,const Cell& c){try{return ShapeOf(s,c);}catch(const std::exception&){return {-1,-1};}}
inline Tx Transaction(State& s,const Rows& rows,const std::set<Id>& V0) {
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
inline void ApplyTx(State& s,Rows& rows,const Tx& tx) {
  Apply(s,tx.ids,tx.fresh);for(const auto& r:tx.lifts)rows.lift[r.first]=r.second;
  for(const auto& f:tx.rebuilt)rows.apex.erase(f);for(const auto& r:tx.apices)rows.apex[r.first]=r.second;
}

// ---- reporting -------------------------------------------------------------------------------------------------
struct Stage { size_t work=0,accepted=0,rejected=0;double seconds=0;std::string outcome;std::map<std::string,size_t> reasons;std::string last; };
inline std::string Key(const std::string& r){return swbi_dc2::Category(r).substr(0,110);}
inline std::string JsonStage(const Stage& g){std::ostringstream o;o<<std::setprecision(10)<<"{\"outcome\":\""<<g.outcome<<"\",\"work\":"<<g.work<<",\"accepted\":"<<g.accepted
  <<",\"rejected\":"<<g.rejected<<",\"seconds\":"<<g.seconds<<",\"reasons\":{";std::vector<std::pair<size_t,std::string>> top;for(const auto& r:g.reasons)top.emplace_back(r.second,r.first);
  std::sort(top.rbegin(),top.rend());for(size_t i=0;i<top.size()&&i<20;++i)o<<(i?",":"")<<junction_wp0::Json(top[i].second)<<':'<<top[i].first;
  o<<"},\"last_witness\":"<<junction_wp0::Json(g.last)<<'}';return o.str();}

// ---- phase W: wall/seam/junction SplitEdge to the composed surface target (no row present; Prepare kind) ---------
inline Stage PhaseW(State& s,size_t work,double seconds,std::ostream& log) {
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
inline VisibleResult Visible(const State& s,Point p,const std::set<Id>& forbid) {
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
inline long double LocalDeficit(State& s,const std::set<Id>& walls,const std::vector<Id>& removed,const std::vector<Cell>* added,
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
inline R1Stage PhaseR1(State& s,const Rows& rows,size_t work,double seconds,std::ostream& log) {
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
inline Census FullStateCensus(State& s,const Rows& rows) {
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
inline std::string JsonCensus(const Census& c){std::ostringstream o;o<<"{\"wall_faces\":"<<c.faces<<",\"wall_vertices\":"<<c.vertices<<",\"no_direction\":"<<c.no_direction
  <<",\"clearance_deficient_vertices\":"<<c.deficient<<",\"faces_full_state_lp_empty\":"<<c.lp_empty<<",\"direction_failures\":{";bool k=false;
  for(const auto& r:c.why){o<<(k?",":"")<<junction_wp0::Json(r.first)<<':'<<r.second;k=true;}o<<"}}";return o.str();}

// ---- R2: closed row transactions in canonical vertex order with bounded closure growth (§8.2) ----------------------
struct R2Stage : Stage { size_t rim_faces=0,transactions=0,lifted=0,unlifted=0,rowed=0,unrowed=0,max_closure=0,max_old=0,max_fresh=0;double first_q=1,first_J=1,height_error=0;
  std::map<std::string,size_t> classes; };
inline R2Stage PhaseR2(State& s,Rows& rows,double seconds,std::ostream& log) {
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
inline Gates Final(State& s);
struct VStage : Stage { size_t column_demand=0,sweeps=0; };
inline VStage PhaseV(State& s,const Rows& rows,size_t work,double seconds,std::ostream& log) {
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
inline Gates Final(State& s) {
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
inline size_t Unmeasurable(State& s){std::set<Edge> seen;size_t bad=0;for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
  const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;try{Length(s,e[0],e[1]);}catch(const std::exception&){++bad;}}}return bad;}
// Final classification: long edges (column / wall / seam / carrier / interior), bad cells (first child / column / other).
inline std::string JsonFinal(State& s,const Rows& rows) {
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

// Final mesh with the native gates (appended to layer2d::Export): per cell metric q, J, bad flag, first-child flag,
// column id (apex) and class (0 outside columns, 1 first child, 2 other column cell); per metric long edge (> 1.8)
// its composed length (-1 unmeasurable), class (0 interior, 1 wall, 2 seam, 3 other physical, 4 unmeasurable) and
// whether every cell of its star is a column cell. Runs once at the end of a case.
inline void ExportGates(State& s,const Rows& rows,const std::string& path) {
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
}  // namespace dc7
