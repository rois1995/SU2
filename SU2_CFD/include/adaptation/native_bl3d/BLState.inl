/*!
 * \file BLState.inl
 * \brief BL pipeline state: mesh/patch/star helpers, visibility cavities, the D2/D3 column prototype operators and the generic first-row construction (ex WP-0(b)).
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_junction_replay.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// ROBUST_JUNCTION_PLAN rev4 WP-0(b): (history: formerly test-only) prototype of D2 column points and D3 single/bounded-compound
// insertion through the existing operators and unchanged gates. (DC-14: production.)
// Outcomes per target: 1 single, 2 bounded compound, 3 blocked (witness kept), 0 not needed (no long edge sees it).
#include "../CNativeBLVector3D.hpp"
#include <chrono>
#include <functional>
#include <numeric>
#include <set>
#include <sstream>
// Reason histogram keys and timing (ex DC-2 swbi_dc2, used by the stage reports).
#include <cctype>
namespace swbi_dc2 {
using Clock=std::chrono::steady_clock;
inline double Seconds(Clock::time_point t){return std::chrono::duration<double>(Clock::now()-t).count();}
inline std::string Category(const std::string& what) {  // Reason histogram key: message with numbers/ids replaced by '#'.
  std::string out;
  for(size_t i=0;i<what.size();++i) {
    const char c=what[i];const bool digit=std::isdigit(static_cast<unsigned char>(c));
    const bool inner=!out.empty()&&out.back()=='#'&&(c=='.'||c=='e'||c=='-'||c=='+')&&i+1<what.size()&&
                     (std::isdigit(static_cast<unsigned char>(what[i+1]))||what[i+1]=='-'||what[i+1]=='+');
    if(digit||inner){if(out.empty()||out.back()!='#')out+='#';}else out+=c;
  }
  return out.substr(0,160);
}
}  // namespace swbi_dc2

namespace junction_wp0 {
using namespace SU2Native3D;
using Clock=std::chrono::steady_clock;
inline std::array<Face,4> CellFaces(const Cell& c) {
  const auto& v=c.v;return {{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
}
inline Face Sorted(Face f){std::sort(f.begin(),f.end());return f;}
// Ericson closest point on a triangle (long double); used only by the test-side distance/nearest-feature probe.
inline V Closest(V p,V a,V b,V c) {
  const V ab=b-a,ac=c-a,ap=p-a;const auto d1=Dot(ab,ap),d2=Dot(ac,ap);if(d1<=0&&d2<=0)return a;
  const V bp=p-b;const auto d3=Dot(ab,bp),d4=Dot(ac,bp);if(d3>=0&&d4<=d3)return b;
  const auto vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0)return a+(d1/(d1-d3))*ab;
  const V cp=p-c;const auto d5=Dot(ab,cp),d6=Dot(ac,cp);if(d6>=0&&d5<=d6)return c;
  const auto vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0)return a+(d2/(d2-d6))*ac;
  const auto va=d3*d6-d5*d4;if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0)return b+((d4-d3)/((d4-d3)+(d5-d6)))*(c-b);
  const auto denom=1/(va+vb+vc);return a+(vb*denom)*ab+(vc*denom)*ac;
}
inline MetricComposition Compose(std::shared_ptr<CBoundaryLayerMetric> layers,double core) {
  return [layers,core](Point p,Tensor s) {
    CBoundaryLayerMetric::Tensor m;m.m[0][0]=s.xx;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;
    m.m[1][1]=s.yy;m.m[1][2]=m.m[2][1]=s.yz;m.m[2][2]=s.zz;const su2double x[3]={p.x,p.y,p.z};
    layers->ApplyPoint(x,m,core,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
                  SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
}
inline MetricEdgeBreakpoints Bands(std::shared_ptr<CBoundaryLayerMetric> layers) {
  return [layers](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
}
struct State {
  std::string name;
  std::shared_ptr<const PlanarReference> reference;
  std::vector<Facet> facets;  // Physical authority facets, oriented like the outward skin.
  std::vector<std::string> markers;
  std::set<int> walls;
  WallHeights heights;
  double h0=0,growth=0,thickness=0,core=0;
  Owned mesh;
  std::map<Id,std::set<Id>> vertex_cells;
  BoundaryPoints overlay;  // Private bindings of edits accepted inside this test (mapped references).
  std::shared_ptr<CBoundaryLayerMetric> layers;
  std::unique_ptr<FrozenField> field;
  Id next_node=1,next_cell=1;
  std::map<Id,size_t> body;     // Wall facet -> connected wall body.
  std::map<Id,Id> component;    // Facet -> canonical reference component.
  std::map<Id,std::pair<int,long double>> snap;  // Canonical partner component -> exact axis plane (faceted).
  std::map<std::pair<Id,Id>,std::pair<std::array<Point,2>,long double>> lengths;
  bool mapped=false;
  // DC-14 scale indices: exact replacements of whole-mesh scans, rebuilt by Index() and maintained by Apply(). Code that
  // changes `mesh` directly must call Index() before using WallFaces()/Locate() (every pipeline path does).
  std::map<Face,Id> wall_index;bool wall_indexed=false;  // sorted wall face -> owner cell
  struct BoxKeyHash{size_t operator()(const std::array<long long,4>& k)const{
    return size_t(k[0]*2654435761LL^k[1]*73856093LL^k[2]*19349663LL^k[3]*83492791LL);}};
  std::unordered_map<std::array<long long,4>,std::vector<Id>,BoxKeyHash> box_grid;std::map<int,size_t> box_levels;
  bool box_indexed=false;  // built lazily by the first Locate
};
inline bool IsWall(const State& s,Id facet){return s.walls.count(s.reference->Get(facet).marker);}
// Box grid of cell bounding boxes: level l has buckets of size 2^l * 2^-40 >= twice the box extent, so a box meets at
// most 2 buckets per axis; a query visits one bucket per populated level (exact broad phase of Locate).
inline int BoxLevel(const Cell& c,std::array<long double,3>& lo,std::array<long double,3>& hi) {
  lo={c.v[0].p.x,c.v[0].p.y,c.v[0].p.z};hi=lo;
  for(const auto& n:c.v){const long double x[3]={n.p.x,n.p.y,n.p.z};for(int k=0;k<3;++k){lo[k]=std::min(lo[k],x[k]);hi[k]=std::max(hi[k],x[k]);}}
  long double e=0;for(int k=0;k<3;++k)e=std::max(e,hi[k]-lo[k]);int l=0;while(std::ldexp(1.L,l-40)<2*e&&l<200)++l;return l;
}
inline std::array<long long,4> BoxKey(int l,long double x,long double y,long double z) {
  const long double h=std::ldexp(1.L,l-40);return {l,(long long)std::floor(x/h),(long long)std::floor(y/h),(long long)std::floor(z/h)};
}
inline void BoxCell(State& s,Id id,const Cell& c,bool add) {
  std::array<long double,3> lo,hi;const int l=BoxLevel(c,lo,hi);const auto a=BoxKey(l,lo[0],lo[1],lo[2]),b=BoxKey(l,hi[0],hi[1],hi[2]);
  for(long long i=a[1];i<=b[1];++i)for(long long j=a[2];j<=b[2];++j)for(long long k=a[3];k<=b[3];++k) {
    auto& v=s.box_grid[{l,i,j,k}];
    if(add)v.push_back(id);else{const auto f=std::find(v.begin(),v.end(),id);if(f!=v.end())v.erase(f);if(v.empty())s.box_grid.erase({l,i,j,k});}}
  if(add)++s.box_levels[l];else if(--s.box_levels[l]==0)s.box_levels.erase(l);
}
inline void IndexRow(State& s,Id id,const OwnedCell& row,bool add) {
  if(s.wall_indexed){const auto f=CellFaces(row.cell);
    for(unsigned k=0;k<4;++k)if(row.physical[k].present&&s.walls.count(s.reference->Get(row.physical[k].facet).marker)){
      if(add)s.wall_index[Sorted(f[k])]=id;else{const auto w=s.wall_index.find(Sorted(f[k]));if(w!=s.wall_index.end()&&w->second==id)s.wall_index.erase(w);}}}
  if(s.box_indexed)BoxCell(s,id,row.cell,add);
}
inline void Index(State& s) {
  s.vertex_cells.clear();
  for(const auto& row:s.mesh)for(auto n:row.second.cell.v){s.vertex_cells[n.id].insert(row.first);s.next_node=std::max(s.next_node,n.id+1);}
  for(const auto& row:s.mesh)s.next_cell=std::max(s.next_cell,row.first+1);
  for(const auto& f:s.facets)for(auto n:f.v)s.next_node=std::max(s.next_node,n.id+1);
  s.wall_index.clear();s.box_grid.clear();s.box_levels.clear();s.box_indexed=false;s.wall_indexed=bool(s.reference);
  if(s.wall_indexed)for(const auto& row:s.mesh)IndexRow(s,row.first,row.second,true);
}
inline void Prepare(State& s) {
  for(const auto& f:s.facets)s.component[f.id]=s.reference->Canonical(f.id);
  // Wall bodies: connected wall facets through shared vertices.
  std::map<Id,Id> parent;std::function<Id(Id)> root=[&](Id a){while(parent.at(a)!=a)a=parent[a]=parent.at(parent.at(a));return a;};
  std::map<Id,std::vector<Id>> by_vertex;
  for(const auto& f:s.facets)if(IsWall(s,f.id)){parent[f.id]=f.id;for(auto n:f.v)by_vertex[n.id].push_back(f.id);}
  for(const auto& row:by_vertex)for(size_t k=1;k<row.second.size();++k)parent[root(row.second[k])]=root(row.second[0]);
  std::map<Id,size_t> label;for(const auto& row:parent){const auto r=root(row.first);label.emplace(r,label.size());s.body[row.first]=label.at(r);}
  // Exactly axis-aligned faceted partner planes are snapped exactly; declared (mapped) planes bind instead.
  std::map<Id,std::vector<const Facet*>> groups;for(const auto& f:s.facets)if(!IsWall(s,f.id))groups[s.component.at(f.id)].push_back(&f);
  for(const auto& g:groups)for(int axis=0;axis<3;++axis) {
    auto value=[&](Point p){return axis==0?p.x:axis==1?p.y:p.z;};const double c=value(g.second.front()->v[0].p);bool same=true;
    for(const auto* f:g.second)for(auto n:f->v)same=same && value(n.p)==c;
    if(same){s.snap[g.first]={axis,c};break;}
  }
  Index(s);
}
inline Patch PatchOf(const State& s,const std::vector<Id>& ids) {
  Patch p;std::set<Id> boundary;
  for(Id id:ids) {
    const auto& row=s.mesh.at(id);p.cells.push_back(row.cell);const auto f=CellFaces(row.cell);
    for(unsigned k=0;k<4;++k)if(row.physical[k].present){p.surface.push_back({f[k],row.physical[k].facet});boundary.insert(f[k].begin(),f[k].end());}
  }
  for(Id n:boundary){const auto o=s.overlay.find(n);if(o!=s.overlay.end())p.parameters.emplace(n,o->second);}
  s.reference->ExportParameters(p);
  return p;
}
inline void Apply(State& s,const std::vector<Id>& old,const Patch& fresh) {
  for(Id id:old){IndexRow(s,id,s.mesh.at(id),false);for(auto n:s.mesh.at(id).cell.v){auto& star=s.vertex_cells[n.id];star.erase(id);if(star.empty())s.vertex_cells.erase(n.id);}s.mesh.erase(id);}
  std::map<Face,Id> physical;for(const auto& f:fresh.surface)physical.emplace(Sorted(f.v),f.facet);
  for(const auto& c:fresh.cells) {
    OwnedCell row;row.cell=c;const auto f=CellFaces(c);
    for(unsigned k=0;k<4;++k){const auto found=physical.find(Sorted(f[k]));if(found!=physical.end())row.physical[k]={true,found->second};}
    if(!s.mesh.emplace(c.id,row).second)throw std::runtime_error("WP0B repeated cell identity.");
    IndexRow(s,c.id,row,true);
    for(auto n:c.v){s.vertex_cells[n.id].insert(c.id);s.next_node=std::max(s.next_node,n.id+1);}
    s.next_cell=std::max(s.next_cell,c.id+1);
  }
  for(const auto& r:fresh.parameters)s.overlay.insert_or_assign(r.first,r.second);
}
inline std::vector<Id> Locate(const State& cs,Point p) {
  std::vector<Id> found;
  if(cs.wall_indexed) {  // exact broad phase (lazy box grid), same result and order as the scan below
    auto& s=const_cast<State&>(cs);
    if(!s.box_indexed){s.box_indexed=true;for(const auto& row:s.mesh)BoxCell(s,row.first,row.second.cell,true);}
    std::vector<Id> cand;
    for(const auto& lv:s.box_levels){const auto f=s.box_grid.find(BoxKey(lv.first,p.x,p.y,p.z));if(f!=s.box_grid.end())cand.insert(cand.end(),f->second.begin(),f->second.end());}
    std::sort(cand.begin(),cand.end());cand.erase(std::unique(cand.begin(),cand.end()),cand.end());
    for(Id id:cand){const auto& v=s.mesh.at(id).cell.v;std::array<long double,4> w{};
      if(Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},p,w))found.push_back(id);}
    return found;
  }
  const auto& s=cs;
  for(const auto& row:s.mesh) {
    const auto& v=row.second.cell.v;std::array<long double,4> w{};
    if(Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},p,w))found.push_back(row.first);
  }
  return found;
}
inline std::vector<Id> Star(const State& s,Edge e) {
  std::vector<Id> out;const auto a=s.vertex_cells.find(e[0]),b=s.vertex_cells.find(e[1]);
  if(a==s.vertex_cells.end()||b==s.vertex_cells.end())return out;
  std::set_intersection(a->second.begin(),a->second.end(),b->second.begin(),b->second.end(),std::back_inserter(out));return out;
}
inline Point NodePoint(const State& s,Id id) {
  for(const auto& n:s.mesh.at(*s.vertex_cells.at(id).begin()).cell.v)if(n.id==id)return n.p;
  throw std::runtime_error("WP0B missing node.");
}
// Distance of a point to the plane of a canonical component (any facet of it), for partner-edge filters.
inline long double PlaneDistance(const State& s,Id component,V p) {
  for(const auto& f:s.facets)if(s.component.at(f.id)==component) {
    const V n=Unit(Cross(Of(f.v[1].p)-Of(f.v[0].p),Of(f.v[2].p)-Of(f.v[0].p)));return std::abs(Dot(p-Of(f.v[0].p),n));
  }
  return INFINITY;
}
inline long double Length(State& s,Id a,Id b) {
  const auto key=std::minmax(a,b);const auto pa=NodePoint(s,key.first),pb=NodePoint(s,key.second);
  const auto found=s.lengths.find({key.first,key.second});
  auto same=[](Point x,Point y){return x.x==y.x&&x.y==y.y&&x.z==y.z;};
  if(found!=s.lengths.end()&&same(found->second.first[0],pa)&&same(found->second.first[1],pb))return found->second.second;
  const auto l=s.field->TargetEdgeLength(pa,pb);s.lengths[{key.first,key.second}]={{pa,pb},l};return l;
}
inline bool PhysicalEdge(const State& s,Edge e) {
  for(Id id:Star(s,e)){const auto& row=s.mesh.at(id);const auto f=CellFaces(row.cell);
    for(unsigned k=0;k<4;++k)if(row.physical[k].present && std::count(f[k].begin(),f[k].end(),e[0]) && std::count(f[k].begin(),f[k].end(),e[1]))return true;}
  return false;
}
inline bool WallChild(const State& s,Id cell) {
  const auto& row=s.mesh.at(cell);for(const auto& f:row.physical)if(f.present && s.walls.count(s.reference->Get(f.facet).marker))return true;return false;
}
// Star-shaped visibility cavity of p; completed first-row wall children are protected, physical faces never crossed.
inline bool VisibleCavity(const State& s,Point p,std::vector<Id>& cavity,std::string& why,const std::set<Id>* within=nullptr) {
  std::set<Id> in;for(Id id:Locate(s,p))if(!within||within->count(id))in.insert(id);
  if(in.empty()){why="target outside the current mesh";return false;}
  for(unsigned pass=0;pass<256;++pass) {
    std::map<Face,std::pair<Face,Id>> skin;std::map<Face,int> count;
    for(Id id:in){const auto f=CellFaces(s.mesh.at(id).cell);for(unsigned k=0;k<4;++k){const auto key=Sorted(f[k]);++count[key];skin[key]={f[k],id};}}
    bool grew=false;
    for(const auto& row:skin) {
      if(count.at(row.first)!=1)continue;
      const auto& f=row.second.first;const auto& owner=s.mesh.at(row.second.second);
      std::array<Point,3> q;for(unsigned k=0;k<3;++k)for(auto n:owner.cell.v)if(n.id==f[k])q[k]=n.p;
      if(Orientation(p,q[0],q[1],q[2])>0)continue;
      const auto faces=CellFaces(owner.cell);
      for(unsigned k=0;k<4;++k)if(Sorted(faces[k])==row.first && owner.physical[k].present){why="target cannot see a physical cavity face";return false;}
      Id neighbor=UINT64_MAX;
      for(Id id:Star(s,{f[0],f[1]}))if(!in.count(id)){const auto& v=s.mesh.at(id).cell.v;if(std::any_of(v.begin(),v.end(),[&](Node n){return n.id==f[2];}))neighbor=id;}
      if(neighbor==UINT64_MAX){why="target cannot see an artificial domain face";return false;}
      if(within && !within->count(neighbor)){why="visibility leaves the bounded compound patch";return false;}
      if(WallChild(s,neighbor)){why="visibility needs a protected first-row wall child";return false;}
      in.insert(neighbor);grew=true;
      if(in.size()>MaximumCavityCells){why="visibility cavity exceeds 64 cells";return false;}
      break;
    }
    if(!grew){cavity.assign(in.begin(),in.end());return true;}
  }
  why="visibility cavity did not close";return false;
}
struct Verdict { bool ok=false;int category=0;std::string reason;double q=0,j=0;long double removed=0,added=0;size_t old_cells=0,fresh_cells=0; };
// category: 1 geometry/operator, 2 shape, 3 Refine progress, 4 first height, 5 measurement/cap
inline Verdict Validate(State& s,const Patch& old,const Patch& fresh) {
  Verdict v;v.old_cells=old.cells.size();v.fresh_cells=fresh.cells.size();PatchMetric measured;MetricDiagnosis d;
  if(old.cells.size()>MaximumReplacementCells||fresh.cells.size()>MaximumReplacementCells){v.category=5;v.reason="replacement exceeds 128 cells";return v;}
  const bool metric=ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,*s.field,measured,v.reason,nullptr,nullptr,&d);
  v.q=d.minimum_mean_ratio;v.j=d.minimum_scaled_jacobian;v.removed=d.removed_excess;v.added=d.added_excess;
  if(!metric){v.category=d.failure==MetricFailure::Shape?2:d.failure==MetricFailure::Progress?3:5;return v;}
  if(!s.heights.empty()){FirstHeightReport h;if(!CheckFirstHeight(fresh,s.heights,*s.reference,h,v.reason)){v.category=4;return v;}}
  v.ok=true;v.reason.clear();return v;
}
struct Target {
  Point p;size_t column=0;unsigned row=0;int kind=0;  // 0 interior, 1 partner chart, 2 partner junction line
  std::set<Id> partners;long double distance=0;int outcome=-1;bool needed=false;
  std::string witness;std::map<int,size_t> histogram;double best_q=0;size_t cavity=0;std::string op;
};
struct Column { Id root=0;V direction;int kind=0;std::set<Id> partners;size_t body=0;std::string cause;std::vector<Point> rows;std::vector<long double> distance,step;V inward; };
// Bounded strictly convex direction problem: min |w|^2, n_i.w >= 1 over active sets <= 3 (residual-checked).
inline bool MinNorm(const std::vector<V>& n,V& out) {
  long double best=INFINITY;const size_t m=n.size();if(!m||m>16)return false;
  std::vector<size_t> pick;
  std::function<void(size_t)> visit=[&](size_t from) {
    if(!pick.empty()) {
      const size_t k=pick.size();long double g[3][3],rhs[3]={1,1,1},lambda[3]={0,0,0};
      for(size_t a=0;a<k;++a)for(size_t b=0;b<k;++b)g[a][b]=Dot(n[pick[a]],n[pick[b]]);
      bool ok=true;  // Gaussian elimination with partial pivoting (k <= 3).
      for(size_t c=0;c<k&&ok;++c){size_t r=c;for(size_t t=c+1;t<k;++t)if(std::abs(g[t][c])>std::abs(g[r][c]))r=t;
        if(!(std::abs(g[r][c])>1e-14L)){ok=false;break;}
        for(size_t t=0;t<k;++t)std::swap(g[c][t],g[r][t]);std::swap(rhs[c],rhs[r]);
        for(size_t t=c+1;t<k;++t){const auto f=g[t][c]/g[c][c];for(size_t u=c;u<k;++u)g[t][u]-=f*g[c][u];rhs[t]-=f*rhs[c];}}
      if(ok){for(size_t c=k;c-->0;){auto x=rhs[c];for(size_t u=c+1;u<k;++u)x-=g[c][u]*lambda[u];lambda[c]=x/g[c][c];}
        V w;bool valid=true;for(size_t a=0;a<k;++a){valid=valid&&lambda[a]>=-1e-12L;w=w+lambda[a]*n[pick[a]];}
        for(const auto& x:n)valid=valid&&Dot(x,w)>=1-1e-12L;
        if(valid&&Dot(w,w)<best){best=Dot(w,w);out=Unit(w);}}
    }
    if(pick.size()==3)return;
    for(size_t i=from;i<m;++i){pick.push_back(i);visit(i+1);pick.pop_back();}
  };
  visit(0);return std::isfinite(best);
}
inline V InwardNormal(const Facet& f){return -1.L*Unit(Cross(Of(f.v[1].p)-Of(f.v[0].p),Of(f.v[2].p)-Of(f.v[0].p)));}
inline V operator-(V a){return {-a.x,-a.y,-a.z};}
struct Nearest { long double distance=INFINITY;Id facet=UINT64_MAX; };
inline Nearest NearestFacet(const State& s,V p,const std::function<bool(const Facet&)>& use) {
  Nearest best;for(const auto& f:s.facets)if(use(f)){const auto d=Norm(p-Closest(p,Of(f.v[0].p),Of(f.v[1].p),Of(f.v[2].p)));if(d<best.distance){best.distance=d;best.facet=f.id;}}
  return best;
}
inline V Snap(const State& s,const std::set<Id>& partners,V p) {
  for(Id c:partners){const auto found=s.snap.find(c);if(found==s.snap.end())continue;const auto value=found->second.second;
    if(found->second.first==0)p.x=value;else if(found->second.first==1)p.y=value;else p.z=value;}
  return p;
}
// Declared (mapped) partner planes are not exactly representable: a carrier point rounded just outside the
// closed source is moved inward by a relative 1e-12; insertion binds/projects it onto the feature anyway.
inline bool Inside(State& s,const Column& c,V& p) {
  auto ok=[&](V q){try{s.field->Query(Pt(q));}catch(const std::exception&){return false;}return !Locate(s,Pt(q)).empty();};
  if(ok(p))return true;
  if(!s.mapped||!(Norm(c.inward)>0))return false;
  const V q=p+(1e-12L*(1+Norm(p)))*c.inward;if(!ok(q))return false;p=q;return true;
}
// D2 march: first-root unit-metric steps with collision, medial, sensor and domain stops; H is read, never added.
inline void March(State& s,Column& c) {
  auto own=[&](const Facet& f){return IsWall(s,f.id)&&s.body.at(f.id)==c.body;};
  auto wall=[&](const Facet& f){return IsWall(s,f.id);};
  auto competitor=[&](const Facet& f) {
    if(IsWall(s,f.id))return s.body.at(f.id)!=c.body;
    if(c.partners.count(s.component.at(f.id)))return false;
    return std::none_of(f.v.begin(),f.v.end(),[&](Node n){return n.id==c.root;});
  };
  V root;for(const auto& f:s.facets)for(auto n:f.v)if(n.id==c.root)root=Of(n.p);
  // First row: wall distance h0 along the column direction (monotone bracket near the wall).
  long double lo=0,hi=s.h0;
  for(unsigned k=0;k<64 && NearestFacet(s,root+hi*c.direction,own).distance<s.h0;++k)hi*=2;
  for(unsigned k=0;k<80;++k){const auto mid=(lo+hi)/2;(NearestFacet(s,root+mid*c.direction,own).distance<s.h0?lo:hi)=mid;}
  V p=Snap(s,c.partners,root+hi*c.direction);
  if(!Inside(s,c,p)){c.cause="domain";return;}
  c.rows.push_back(Pt(p));c.distance.push_back(NearestFacet(s,p,own).distance);c.step.push_back(hi);
  for(unsigned row=1;row<48;++row) {
    Tensor m;try{m=s.field->Query(Pt(p)).target;}catch(const std::exception&){c.cause="domain";return;}
    auto size=[&](V e){const auto q=e.x*(m.xx*e.x+m.xy*e.y+m.xz*e.z)+e.y*(m.xy*e.x+m.yy*e.y+m.yz*e.z)+e.z*(m.xz*e.x+m.yz*e.y+m.zz*e.z);return 1/std::sqrt(q);};
    V e1=Cross(c.direction,{1,0,0});if(Norm(e1)<.5L)e1=Cross(c.direction,{0,1,0});e1=Unit(e1);const V e2=Unit(Cross(c.direction,e1));
    const auto tangential=std::min(size(e1),size(e2));
    long double a=0,b=size(c.direction);bool bracket=false;
    auto probe=[&](long double step){V q=Snap(s,c.partners,p+step*c.direction);if(s.mapped)q=q+(1e-12L*(1+Norm(q)))*c.inward;return q;};
    for(unsigned k=0;k<60;++k){long double l=0;try{l=s.field->TargetEdgeLength(Pt(p),Pt(probe(b)));}catch(const std::exception&){c.cause="domain";return;}
      if(l>=1){bracket=true;break;}a=b;b*=2;}
    if(!bracket){c.cause="unbracketed";return;}
    for(unsigned k=0;k<60;++k){const auto mid=(a+b)/2;long double l=0;
      try{l=s.field->TargetEdgeLength(Pt(p),Pt(probe(mid)));}catch(const std::exception&){c.cause="domain";return;}(l<1?a:b)=mid;}
    const auto t=b;
    if(t>=tangential){c.cause="sensor";return;}
    V next=Snap(s,c.partners,p+t*c.direction);
    if(!Inside(s,c,next)){c.cause="domain";return;}
    const auto nearest=NearestFacet(s,next,wall);const auto self=NearestFacet(s,next,own);
    if(nearest.facet==UINT64_MAX||!own(*std::find_if(s.facets.begin(),s.facets.end(),[&](const Facet& f){return f.id==nearest.facet;}))||
       Dot(InwardNormal(*std::find_if(s.facets.begin(),s.facets.end(),[&](const Facet& f){return f.id==nearest.facet;})),c.direction)<=0||
       !(self.distance>c.distance.back())){c.cause="collision";return;}
    const auto other=NearestFacet(s,next,competitor).distance;
    if(!(self.distance+t<=other)){c.cause="medial";return;}
    p=next;c.rows.push_back(Pt(p));c.distance.push_back(self.distance);c.step.push_back(t);
  }
  c.cause="row_bound";
}
// D1/D2 sectors at one wall vertex: interior sectors (fan rule), or partner carriers at junctions/triple corners.
inline std::vector<Column> Columns(State& s,Id vertex) {
  std::vector<Column> out;std::vector<const Facet*> walls,partners;
  for(const auto& f:s.facets)if(std::any_of(f.v.begin(),f.v.end(),[&](Node n){return n.id==vertex;}))(IsWall(s,f.id)?walls:partners).push_back(&f);
  if(walls.empty())return out;
  V root;for(auto n:walls.front()->v)if(n.id==vertex)root=Of(n.p);
  std::map<Id,V> planes;for(const auto* f:walls)planes.emplace(s.component.at(f->id),InwardNormal(*f));
  std::vector<V> normals;for(const auto& row:planes)normals.push_back(row.second);
  // Fan rule on the actual metric: normals whose first-row points are farther than one unit start new sectors.
  std::vector<int> label(normals.size(),-1);int sectors=0;
  for(size_t i=0;i<normals.size();++i)if(label[i]<0){label[i]=sectors;std::vector<size_t> stack{i};
    while(!stack.empty()){const auto a=stack.back();stack.pop_back();
      for(size_t b=0;b<normals.size();++b)if(label[b]<0){long double l=INFINITY;
        try{l=s.field->TargetEdgeLength(Pt(root+s.h0*normals[a]),Pt(root+s.h0*normals[b]));}catch(const std::exception&){}
        if(l<=1){label[b]=sectors;stack.push_back(b);}}}
    ++sectors;}
  std::vector<V> directions;
  for(int k=0;k<sectors;++k){std::vector<V> group;for(size_t i=0;i<normals.size();++i)if(label[i]==k)group.push_back(normals[i]);
    V d;if(MinNorm(group,d))directions.push_back(d);}
  std::map<Id,V> partner_planes;for(const auto* f:partners)partner_planes.emplace(s.component.at(f->id),Unit(Cross(Of(f->v[1].p)-Of(f->v[0].p),Of(f->v[2].p)-Of(f->v[0].p))));
  const size_t body=s.body.at(walls.front()->id);
  std::vector<V> carriers;int kind=0;std::set<Id> incident;
  for(const auto& row:partner_planes)incident.insert(row.first);
  if(partner_planes.empty())carriers=directions;
  else if(partner_planes.size()==1) {
    kind=1;const V m=partner_planes.begin()->second;
    for(const auto& n:directions){const V t=Cross(n,m);if(Norm(t)<1e-9L)continue;V c=Unit(Cross(m,t));if(Dot(c,n)<0)c=-c;carriers.push_back(c);}
  } else {
    kind=2;auto it=partner_planes.begin();const V m1=it->second;++it;const V m2=it->second;V c=Cross(m1,m2);
    V n;if(!MinNorm(normals,n))n=normals.front();if(Norm(c)>1e-9L){c=Unit(c);if(Dot(c,n)<0)c=-c;carriers.push_back(c);}
  }
  // Fans between sector/carrier directions whose first-row points are more than one metric unit apart.
  std::vector<V> all=carriers;
  for(size_t i=0;i<carriers.size();++i)for(size_t j=i+1;j<carriers.size();++j) {
    long double l=0;try{l=s.field->TargetEdgeLength(Pt(root+s.h0*carriers[i]),Pt(root+s.h0*carriers[j]));}catch(const std::exception&){continue;}
    const auto angle=std::acos(std::clamp(Dot(carriers[i],carriers[j]),-1.L,1.L));if(!(l>1)||!(angle>1e-9L)||angle>3.1L)continue;
    const unsigned fans=unsigned(std::ceil(double(l)))-1;
    for(unsigned f=1;f<=fans;++f){const long double t=(long double)f/(fans+1);
      all.push_back(Unit((std::sin((1-t)*angle)/std::sin(angle))*carriers[i]+(std::sin(t*angle)/std::sin(angle))*carriers[j]));}
  }
  V inward;for(const auto& row:partner_planes)inward=inward-row.second;if(Norm(inward)>0)inward=Unit(inward);
  for(const auto& d:all){Column c;c.root=vertex;c.direction=d;c.kind=kind;c.partners=incident;c.body=body;c.inward=inward;out.push_back(c);}
  return out;
}
struct Summary { size_t targets=0,single=0,compound=0,blocked=0,not_needed=0,unattempted=0;std::map<std::string,size_t> causes;std::map<int,size_t> failures; };
// Candidate private replacements for one target in the current mesh (or inside a bounded private patch).
inline void Candidates(State& s,const Target& t,std::vector<std::pair<std::vector<Id>,std::function<bool(const Patch&,Patch&,std::string&)>>>& out,bool& needed) {
  needed=false;const auto cells=Locate(s,t.p);if(cells.empty())return;
  std::set<Edge> edges;
  for(Id c:cells)for(auto n:s.mesh.at(c).cell.v)for(Id near:s.vertex_cells.at(n.id)){const auto& v=s.mesh.at(near).cell.v;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)edges.insert({std::min(v[a].id,v[b].id),std::max(v[a].id,v[b].id)});}
  std::vector<std::tuple<int,long double,Edge>> ranked;
  for(const auto& e:edges) {
    const auto star=Star(s,e);if(star.empty()||star.size()>MaximumCavityCells)continue;
    bool contains=false;for(Id c:star){const auto& v=s.mesh.at(c).cell.v;std::array<long double,4> w{};contains=contains||Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},t.p,w);}
    long double l=0;try{l=Length(s,e[0],e[1]);}catch(const std::exception&){continue;}
    if(contains&&l>MaximumMetricEdge)needed=true;
    ranked.emplace_back(contains?0:1,-l,e);
  }
  std::sort(ranked.begin(),ranked.end());
  const Id id=s.next_node;const Node node{id,t.p};
  size_t tried=0;
  for(const auto& row:ranked) {
    const Edge e=std::get<2>(row);const bool physical=PhysicalEdge(s,e);
    if(t.kind==0 && physical)continue;
    if(t.kind!=0 && !physical)continue;
    if(t.kind!=0) {
      // Partner targets: both endpoints on every incident partner plane; a junction target lies on the segment.
      const V a=Of(NodePoint(s,e[0])),b=Of(NodePoint(s,e[1])),x=Of(t.p);const auto scale=std::max<long double>(Norm(b-a),1e-300L);
      bool on=true;for(Id c:t.partners)if(t.kind==2||c==*t.partners.begin())on=on&&PlaneDistance(s,c,a)<=1e-9L*scale&&PlaneDistance(s,c,b)<=1e-9L*scale;
      if(t.kind==1){bool any=false;for(Id c:t.partners)any=any||(PlaneDistance(s,c,a)<=1e-9L*scale&&PlaneDistance(s,c,b)<=1e-9L*scale);on=any;}
      if(t.kind==2){const auto u=Dot(x-a,b-a)/Dot(b-a,b-a);on=on&&u>0&&u<1&&Norm(x-(a+u*(b-a)))<=1e-9L*scale;}
      if(!on)continue;
    }
    if(++tried>8)break;
    const auto star=Star(s,e);
    if(t.kind==2) out.push_back({star,[&s,e,node](const Patch& old,Patch& fresh,std::string& why){return SplitEdge(old,e,node,s.next_cell,*s.reference,fresh,why);}});
    else out.push_back({star,[&s,e,node](const Patch& old,Patch& fresh,std::string& why){return InsertEdgeCavity(old,e,node,s.next_cell,*s.reference,fresh,why);}});
  }
  if(t.kind==0) {
    std::vector<Id> cavity;std::string why;
    if(VisibleCavity(s,t.p,cavity,why))out.push_back({cavity,[&s,node](const Patch& old,Patch& fresh,std::string& why){
      fresh={};if(!Cone(old.cells,node,s.next_cell,fresh.cells,why))return false;fresh.surface=old.surface;fresh.parameters=old.parameters;
      s.reference->ExportParameters(fresh);return true;}});
  }
}
inline bool Single(State& s,Target& t) {
  std::vector<std::pair<std::vector<Id>,std::function<bool(const Patch&,Patch&,std::string&)>>> candidates;
  Candidates(s,t,candidates,t.needed);
  if(!t.needed){t.outcome=0;t.witness="no admitted edge longer than 1.8 sees the target";return true;}
  for(auto& c:candidates) {
    Patch old;try{old=PatchOf(s,c.first);}catch(const std::exception& e){++t.histogram[1];continue;}
    if(!s.heights.empty()){FirstHeightReport h;std::string why;if(!CheckFirstHeight(old,s.heights,*s.reference,h,why)){++t.histogram[4];continue;}}
    Patch fresh;std::string why;bool built=false;
    try{built=c.second(old,fresh,why);}catch(const std::exception& e){why=e.what();}
    if(!built){++t.histogram[1];t.witness=why;continue;}
    const auto v=Validate(s,old,fresh);t.best_q=std::max(t.best_q,v.q);
    if(!v.ok){++t.histogram[v.category];t.witness=v.reason;continue;}
    Apply(s,c.first,fresh);t.outcome=1;t.cavity=old.cells.size();return true;
  }
  return false;
}
// D3(2) prototype InsertTargetSet: sequential visibility/edge insertion of <=8 targets in one private patch,
// bounded actual-field connectivity repair of the inserted points, then one Refine/height/interface validation.
inline bool Compound(State& s,std::vector<Target>& targets,size_t at,const std::vector<Column>& columns) {
  auto& t=targets[at];std::vector<size_t> group{at};
  std::vector<std::pair<long double,size_t>> near;
  for(size_t i=0;i<targets.size();++i)if(i!=at&&targets[i].outcome<0) {
    const bool related=targets[i].column==t.column||targets[i].row==t.row||targets[i].row+1==t.row||targets[i].row==t.row+1;
    if(related)near.emplace_back(Norm(Of(targets[i].p)-Of(t.p)),i);
  }
  std::sort(near.begin(),near.end());for(const auto& row:near){if(group.size()==8)break;group.push_back(row.second);}
  for(;!group.empty();group.pop_back()) {
    std::set<Id> region;bool ok=true;std::string why;
    for(size_t g:group) {
      std::vector<Id> cells;
      if(targets[g].kind==0){if(!VisibleCavity(s,targets[g].p,cells,why)){if(g==at){ok=false;break;}continue;}}
      else {for(Id c:Locate(s,targets[g].p))for(auto n:s.mesh.at(c).cell.v)for(Id x:s.vertex_cells.at(n.id))cells.push_back(x);}
      region.insert(cells.begin(),cells.end());
    }
    if(!ok){t.witness="compound: "+why;break;}
    if(region.size()>MaximumCavityCells){t.witness="compound region of "+std::to_string(region.size())+" cells exceeds the 64-cell cavity bound";continue;}
    const std::vector<Id> ids(region.begin(),region.end());Patch old;
    try{old=PatchOf(s,ids);}catch(const std::exception& e){t.witness=std::string("compound patch: ")+e.what();continue;}
    if(!s.heights.empty()){FirstHeightReport h;if(!CheckFirstHeight(old,s.heights,*s.reference,h,why)){t.witness="compound precondition: "+why;continue;}}
    // Private sequential insertion; artificial patch faces stay fixed.
    State& base=s;Patch work=old;std::vector<Id> inserted;Id next_node=base.next_node,next_cell=base.next_cell+4096;
    std::vector<size_t> order=group;std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return targets[a].distance>targets[b].distance;});
    bool target_inserted=false;
    for(size_t g:order) {
      const auto& x=targets[g];const Node node{next_node,x.p};Patch fresh;std::string reason;bool built=false;
      const auto skin=Incidence(work.cells).Boundary();std::set<Face> artificial;
      std::set<Face> physical;for(const auto& f:work.surface)physical.insert(Sorted(f.v));
      for(auto f:skin)if(!physical.count(Sorted(f)))artificial.insert(Sorted(f));
      auto complete=[&](Edge e){for(const auto& f:artificial)if(std::count(f.begin(),f.end(),e[0])&&std::count(f.begin(),f.end(),e[1]))return false;return true;};
      auto sub=[&](const std::vector<Cell>& cells){Patch p;p.cells=cells;const auto b=Incidence(cells).Boundary();std::set<Face> keys;for(auto f:b)keys.insert(Sorted(f));
        for(const auto& f:work.surface)if(keys.count(Sorted(f.v)))p.surface.push_back(f);p.parameters=work.parameters;return p;};
      auto replace=[&](const std::vector<Cell>& removed,const Patch& added) {
        std::set<Id> gone;for(const auto& c:removed)gone.insert(c.id);
        std::vector<Cell> cells;for(const auto& c:work.cells)if(!gone.count(c.id))cells.push_back(c);
        cells.insert(cells.end(),added.cells.begin(),added.cells.end());
        std::set<Face> removedFaces;for(const auto& c:removed)for(auto f:CellFaces(c))removedFaces.insert(Sorted(f));
        std::vector<SurfaceFace> surface;for(const auto& f:work.surface)if(!std::any_of(added.surface.begin(),added.surface.end(),[&](const SurfaceFace& a){return Sorted(a.v)==Sorted(f.v);})) {
          bool covered=false;for(const auto& c:cells){const auto faces=CellFaces(c);for(auto q:faces)covered=covered||Sorted(q)==Sorted(f.v);}if(covered)surface.push_back(f);}
        surface.insert(surface.end(),added.surface.begin(),added.surface.end());
        work.cells=cells;work.surface=surface;for(const auto& r:added.parameters)work.parameters.insert_or_assign(r.first,r.second);
      };
      try {
        if(x.kind==0) {
          // Visibility inside the private patch only.
          std::set<Id> in;for(const auto& c:work.cells){std::array<long double,4> w{};if(Barycentric({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},x.p,w))in.insert(c.id);}
          for(unsigned pass=0;pass<256&&!in.empty();++pass) {
            std::map<Face,int> count;std::map<Face,std::pair<Face,const Cell*>> owner;
            for(const auto& c:work.cells)if(in.count(c.id))for(auto f:CellFaces(c)){++count[Sorted(f)];owner[Sorted(f)]={f,&c};}
            bool grew=false,blocked=false;
            for(const auto& row:owner){if(count.at(row.first)!=1)continue;const auto f=row.second.first;std::array<Point,3> q;
              for(unsigned k=0;k<3;++k)for(auto n:row.second.second->v)if(n.id==f[k])q[k]=n.p;
              if(Orientation(x.p,q[0],q[1],q[2])>0)continue;
              if(physical.count(row.first)||artificial.count(row.first)){blocked=true;break;}
              for(const auto& c:work.cells)if(!in.count(c.id)){bool has=true;for(Id v:f)has=has&&std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==v;});if(has){in.insert(c.id);grew=true;break;}}
              break;}
            if(blocked){in.clear();break;}
            if(!grew)break;
          }
          if(!in.empty()) {
            std::vector<Cell> removed;for(const auto& c:work.cells)if(in.count(c.id))removed.push_back(c);
            Patch added;added.surface=sub(removed).surface;
            if(Cone(removed,node,next_cell,added.cells,reason)){next_cell+=added.cells.size();replace(removed,added);built=true;}
          } else reason="private visibility blocked";
        } else {
          std::vector<std::pair<long double,Edge>> edges;
          for(const auto& c:work.cells)for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
            const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};
            bool onface=false;for(const auto& f:work.surface)onface=onface||(std::count(f.v.begin(),f.v.end(),e[0])&&std::count(f.v.begin(),f.v.end(),e[1]));
            if(onface&&complete(e))edges.emplace_back(Norm(Of(c.v[a].p)+Of(c.v[b].p)-2*Of(x.p)),e);}
          std::sort(edges.begin(),edges.end());edges.erase(std::unique(edges.begin(),edges.end()),edges.end());
          for(size_t k=0;k<edges.size()&&k<6&&!built;++k) {
            const auto e=edges[k].second;std::vector<Cell> star;for(const auto& c:work.cells){unsigned m=0;for(auto n:c.v)m+=n.id==e[0]||n.id==e[1];if(m==2)star.push_back(c);}
            const auto local=sub(star);
            built=x.kind==2?SplitEdge(local,e,node,next_cell,*s.reference,fresh,reason):InsertEdgeCavity(local,e,node,next_cell,*s.reference,fresh,reason);
            if(built){next_cell+=fresh.cells.size();replace(star,fresh);}
          }
        }
      }catch(const std::exception& e){reason=e.what();built=false;}
      if(!built){if(g==at){t.witness="compound insertion: "+reason;target_inserted=false;break;}continue;}
      if(g==at)target_inserted=true;
      inserted.push_back(next_node++);
      if(work.cells.size()>MaximumReplacementCells)break;
    }
    if(!target_inserted)continue;
    if(work.cells.size()>MaximumReplacementCells){t.witness="compound exceeds 128 new cells";continue;}
    // Only the final state is validated. Inserted points may move/reconnect under the actual frozen field.
    {
      FirstHeightReport h;std::string reason;
      if(s.heights.empty()||CheckFirstHeight(work,s.heights,*s.reference,h,reason)) {
        Patch repaired;ConnectivityRepairReport report;const std::set<Id> movable(inserted.begin(),inserted.end());
        try{if(RepairMetricConnectivity(work,*s.field,*s.reference,s.heights,next_cell,repaired,report,reason,64,false,&movable)){work=std::move(repaired);next_cell+=work.cells.size();}}
        catch(const std::exception&){}
      }
    }
    for(size_t i=0;i<work.cells.size();++i)work.cells[i].id=next_cell+i;
    std::string reason;
    {
      // Artificial patch faces must survive unchanged and oriented; physical faces are certified by height/surface checks.
      std::set<Face> physical;for(const auto& f:old.surface)physical.insert(Sorted(f.v));
      std::map<Face,Face> after;for(auto f:Incidence(work.cells).Boundary())after[Sorted(f)]=f;
      bool fixed=true;for(auto f:Incidence(old.cells).Boundary())if(!physical.count(Sorted(f))){const auto found=after.find(Sorted(f));
        fixed=fixed&&found!=after.end()&&Sorted(found->second)==Sorted(f);
        if(fixed){Face a=found->second,b=f;while(a[0]!=b[0])std::rotate(a.begin(),a.begin()+1,a.end());fixed=a==b;}}
      bool positive=true;for(const auto& c:work.cells)positive=positive&&Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0;
      if(!fixed||!positive){t.witness="compound changed its artificial interface or inverted a cell";continue;}
    }
    try{s.reference->ExportParameters(work);}catch(const std::exception& e){t.witness=std::string("compound parameters: ")+e.what();continue;}
    const auto v=Validate(s,old,work);t.best_q=std::max(t.best_q,v.q);
    if(!v.ok){++t.histogram[10+v.category];t.witness="compound: "+v.reason;t.cavity=old.cells.size();continue;}
    Apply(s,ids,work);s.next_node=std::max(s.next_node,next_node);
    for(size_t g:group)if(targets[g].outcome<0){targets[g].outcome=2;targets[g].cavity=old.cells.size();}
    return true;
  }
  (void)columns;return false;
}
inline std::string Json(const std::string& text){std::ostringstream o;o<<std::quoted(text);return o.str();}
// Engine-like existing first-row construction (centroid apices) for wall faces lacking a valid first child.
inline std::pair<size_t,size_t> FirstRow(State& s,std::ostringstream& log) {
  size_t built=0,failed=0;std::set<Id> refused;
  for(unsigned pass=0;pass<8;++pass) {
    std::vector<Id> pending;
    for(const auto& row:s.mesh)if(!refused.count(row.first)) {
      bool wall=false;for(const auto& f:row.second.physical)wall=wall||(f.present&&s.walls.count(s.reference->Get(f.facet).marker));
      if(!wall)continue;FirstHeightReport h;std::string why;
      if(!CheckFirstHeight(PatchOf(s,{row.first}),s.heights,*s.reference,h,why))pending.push_back(row.first);
    }
    if(pending.empty())break;
    for(Id id:pending) {
      if(!s.mesh.count(id))continue;const auto old=PatchOf(s,{id});unsigned walls=0;
      for(const auto& f:old.surface)walls+=s.heights.count(s.reference->Get(f.facet).marker);
      Patch fresh;FirstHeightReport report;std::string why;bool ok=false;
      if(walls==1)ok=InsertFirstWallPoint(old,s.next_node,s.next_cell,s.heights,*s.reference,fresh,report,why);
      else {
        std::vector<Point> backgrounds{Centroid(old.cells.front())};double best=-1;Patch chosen;
        for(const auto& f:old.surface){const auto h=s.heights.find(s.reference->Get(f.facet).marker);if(h==s.heights.end())continue;
          std::array<V,3> q;for(unsigned k=0;k<3;++k)for(auto n:old.cells.front().v)if(n.id==f.v[k])q[k]=Of(n.p);
          const V n=Unit(Cross(q[1]-q[0],q[2]-q[0]));const V c=(1.L/3)*(q[0]+q[1]+q[2]);
          for(double factor:{1.25,1.5,2.})backgrounds.push_back(Pt(c-(factor*h->second)*n));}
        for(const auto& b:backgrounds){Patch candidate;std::string reason;
          if(!ConstructFirstLayer(old,{s.next_node,b},s.next_node+1,s.next_cell,s.heights,*s.reference,candidate,report,reason)){why=reason;continue;}
          const double q=MetricShapeQuality(candidate.cells,*s.field);if(q>best){best=q;chosen=std::move(candidate);ok=true;}}
        if(ok)fresh=std::move(chosen);
      }
      if(ok){Apply(s,{id},fresh);++built;}else{++failed;refused.insert(id);log<<"FIRST_ROW_REFUSED cell="<<id<<" walls="<<walls<<" reason="<<why<<"\n";}
    }
  }
  return {built,failed};
}
inline Summary Sequence(State& s,const std::vector<Id>& roots,double seconds,std::ostream& json,std::ostringstream& log,bool skip_interior_first=true) {
  Summary sum;const auto started=Clock::now();std::vector<Column> columns;
  for(Id v:roots)for(auto& c:Columns(s,v)){March(s,c);columns.push_back(c);}
  std::vector<Target> targets;
  for(size_t k=0;k<columns.size();++k){++sum.causes[columns[k].cause];
    for(size_t r=0;r<columns[k].rows.size();++r){if(skip_interior_first&&columns[k].kind==0&&r==0)continue;
      Target t;t.p=columns[k].rows[r];t.column=k;t.row=unsigned(r+1);t.kind=columns[k].kind;t.partners=columns[k].partners;t.distance=columns[k].distance[r];targets.push_back(t);}}
  sum.targets=targets.size();
  std::vector<size_t> order(targets.size());std::iota(order.begin(),order.end(),0);
  std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){return targets[a].distance>targets[b].distance;});
  for(size_t i:order) {
    auto& t=targets[i];if(t.outcome>=0)continue;
    if(std::chrono::duration<double>(Clock::now()-started).count()>seconds){++sum.unattempted;continue;}
    try {
      if(!Single(s,t)&&!Compound(s,targets,i,columns)){t.outcome=3;}
    }catch(const std::exception& e){t.outcome=3;t.witness=std::string("exception: ")+e.what();}
  }
  json<<"\"columns\":[";
  for(size_t k=0;k<columns.size();++k){const auto& c=columns[k];json<<(k?",":"")<<"{\"root\":"<<c.root<<",\"kind\":"<<c.kind<<",\"direction\":["<<double(c.direction.x)<<','<<double(c.direction.y)<<','<<double(c.direction.z)
    <<"],\"rows\":"<<c.rows.size()<<",\"cause\":"<<Json(c.cause)<<",\"distances\":[";for(size_t r=0;r<c.distance.size();++r)json<<(r?",":"")<<double(c.distance[r]);json<<"]}";}
  json<<"],\"targets\":[";
  for(size_t i=0;i<targets.size();++i) {
    const auto& t=targets[i];
    if(t.outcome==1)++sum.single;else if(t.outcome==2)++sum.compound;else if(t.outcome==3)++sum.blocked;else if(t.outcome==0)++sum.not_needed;
    for(const auto& h:t.histogram)sum.failures[h.first]+=h.second;
    json<<(i?",":"")<<"{\"column\":"<<t.column<<",\"row\":"<<t.row<<",\"kind\":"<<t.kind<<",\"distance\":"<<double(t.distance)<<",\"point\":["<<t.p.x<<','<<t.p.y<<','<<t.p.z
        <<"],\"outcome\":"<<t.outcome<<",\"cavity\":"<<t.cavity<<",\"best_q\":"<<t.best_q<<",\"histogram\":{";
    bool comma=false;for(const auto& h:t.histogram){json<<(comma?",":"")<<'"'<<h.first<<"\":"<<h.second;comma=true;}
    json<<"},\"witness\":"<<Json(t.witness)<<'}';
  }
  json<<"],\"summary\":{\"targets\":"<<sum.targets<<",\"single\":"<<sum.single<<",\"compound\":"<<sum.compound<<",\"blocked\":"<<sum.blocked
      <<",\"not_needed\":"<<sum.not_needed<<",\"unattempted\":"<<sum.unattempted<<",\"column_stops\":{";
  bool comma=false;for(const auto& c:sum.causes){json<<(comma?",":"")<<Json(c.first)<<':'<<c.second;comma=true;}
  json<<"},\"failure_categories\":{";comma=false;for(const auto& f:sum.failures){json<<(comma?",":"")<<'"'<<f.first<<"\":"<<f.second;comma=true;}
  json<<"},\"seconds\":"<<std::chrono::duration<double>(Clock::now()-started).count()<<'}';
  log<<"WP0B_SEQUENCE case="<<s.name<<" targets="<<sum.targets<<" single="<<sum.single<<" compound="<<sum.compound<<" blocked="<<sum.blocked
     <<" not_needed="<<sum.not_needed<<" unattempted="<<sum.unattempted<<"\n";
  return sum;
}
inline void Quality(State& s,std::ostream& json) {
  double q=1,j=1;long double l=0;size_t over=0;
  for(const auto& row:s.mesh) {
    const auto& c=row.second.cell;const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},s.field->Query(Centroid(c)).target);
    q=std::min(q,m.mean_ratio);j=std::min(j,m.minimum_scaled_jacobian);
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const auto x=Length(s,c.v[a].id,c.v[b].id);l=std::max(l,x);over+=x>MaximumMetricEdge;}
  }
  json<<"{\"cells\":"<<s.mesh.size()<<",\"q_min\":"<<q<<",\"J_min\":"<<j<<",\"edge_max\":"<<double(l)<<",\"edge_uses_over_1.8\":"<<over<<'}';
}
// Generic SU2_JUNCTION_CASE fixtures (tubes, M6 neighbourhoods): faceted authority, uniform proxy sensor.
}  // namespace junction_wp0
