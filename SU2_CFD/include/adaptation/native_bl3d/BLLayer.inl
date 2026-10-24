/*!
 * \file BLLayer.inl
 * \brief BL pipeline admissions (D.1), refine/repair proposals and the first-height tolerance test (ex DC-5).
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_layer_2dway.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// BL_REMESH_PLAN rev 3, DC-5: (history: formerly test-only) [NativeLayer2DWay3D] harness. (DC-14: production.)
// Existing operators + D.1 admission + LayerConstruct + a D.3 coordinated first row, run twice per case on the SAME
// deterministic proposal stream: admission A (current absolute shape on every fresh cell) and admission D (D.1:
// first children absolute, LayerConstruct other fresh cells positivity only, other kinds not-worse-than-before).
// Both admissions use the decided T = 0.10 first-height tolerance test-side (CheckFirstHeight report), so the
// comparison isolates D.1. Final gates are unchanged and absolute; the final mesh is re-audited with a fresh field.
// Reuses junction_wp0 (fixtures/native_junction_replay.inc) for state, loading, visibility and patches.
#include <chrono>
namespace layer2d {
using namespace junction_wp0;
constexpr long double HeightTolerance = 0.10L;
enum class Adm { A, D };
enum class Kind { Refine, Repair, Layer };
inline const char* KindName(Kind k){return k==Kind::Refine?"Refine":k==Kind::Repair?"Repair":"LayerConstruct";}

// ---- test-local D1 law = the production expression (per wall: BL ∩ F_1 ∩ ... canonical, fade, ∩ into metric) ----
struct D1Law {
  double h0=0,g=1,T=0,hgrad=1.3,Rf=0;
  struct Seg { V a,b;std::string marker; };
  struct Corner { V c;std::string marker; };
  std::vector<Seg> segments;std::vector<Corner> corners;
  double hnf(long double rho) const {
    if(g>1)return double(std::max<long double>(h0,2*(h0+(g-1)*rho)/(g+1)));
    const long double s=2*(hgrad-1)/(hgrad+1);return double(h0+s*std::max<long double>(rho-T,0));
  }
};
using Mat=su2double[3][3];
inline void Sym(const su2double (&C)[3][3],su2double (&M)[3][3]){for(int i=0;i<3;++i)for(int j=0;j<3;++j)M[i][j]=0.5*(C[i][j]+C[j][i]);}
template<class F> void SpectralT(const su2double (&A)[3][3],su2double (&B)[3][3],F f) {
  su2double vec[3][3],val[3],work[3];CBlasStructure::EigenDecomposition(A,vec,val,3,work);
  for(int i=0;i<3;++i)val[i]=f(val[i]);su2double C[3][3];CBlasStructure::EigenRecomposition(C,vec,val,3);Sym(C,B);
}
inline double MinEig(const su2double (&A)[3][3]){su2double vec[3][3],val[3],work[3];CBlasStructure::EigenDecomposition(A,vec,val,3,work);return std::min({val[0],val[1],val[2]});}
inline MetricComposition ComposeD1(std::shared_ptr<CBoundaryLayerMetric> layers,double core,std::shared_ptr<const D1Law> d1,std::shared_ptr<size_t> applied) {
  return [layers,core,d1,applied](Point p,Tensor s) {
    su2double M[3][3]={{s.xx,s.xy,s.xz},{s.xy,s.yy,s.yz},{s.xz,s.yz,s.zz}};
    const su2double x[3]={p.x,p.y,p.z};
    for(unsigned short w=0;w<layers->GetnWall();++w) {
      const auto sample=layers->Evaluate(w,x,core);
      if(!(sample.weight>0))continue;
      su2double X[3][3];for(int i=0;i<3;++i)for(int j=0;j<3;++j)X[i][j]=sample.full.m[i][j];
      bool changed=false;
      if(d1) {
        const auto& marker=layers->GetLayer(w).marker;const V q=Of(p);
        auto fold=[&](const su2double (&F)[3][3],long double lam){
          if(!(lam>MinEig(X)))return;  // F <= lam I <= X: exactly redundant
          su2double C[3][3];CSolver::IntersectMetrics(3,X,F,C);Sym(C,X);changed=true;};
        for(const auto& seg:d1->segments)if(seg.marker==marker) {
          const V t=seg.b-seg.a;const auto L=Norm(t);const V u=(1/L)*t;const auto sp=std::clamp(Dot(q-seg.a,u),0.L,L);
          const auto rho=Norm(q-(seg.a+sp*u));if(rho>d1->Rf)continue;const long double lam=1/std::pow((long double)d1->hnf(rho),2);
          su2double F[3][3];const long double uu[3]={u.x,u.y,u.z};
          for(int i=0;i<3;++i)for(int j=0;j<3;++j)F[i][j]=double(lam*((i==j?1:0)-uu[i]*uu[j]));
          fold(F,lam);
        }
        for(const auto& c:d1->corners)if(c.marker==marker) {
          const auto rho=Norm(q-c.c);if(rho>d1->Rf)continue;const long double lam=1/std::pow((long double)d1->hnf(rho),2);
          su2double F[3][3]={{double(lam),0,0},{0,double(lam),0},{0,0,double(lam)}};fold(F,lam);
        }
      }
      su2double faded[3][3];
      if(sample.weight<1.0) {
        const su2double w8=sample.weight;
        if(!changed)SpectralT(sample.logarithm.m,faded,[&](su2double v){return exp(w8*v+(1-w8)*log(su2double(core)));});
        else SpectralT(X,faded,[&](su2double v){return exp(w8*log(v)+(1-w8)*log(su2double(core)));});
      } else for(int i=0;i<3;++i)for(int j=0;j<3;++j)faded[i][j]=X[i][j];
      if(changed && applied)++*applied;
      su2double C[3][3];CSolver::IntersectMetrics(3,M,faded,C);Sym(C,M);
    }
    return Tensor{SU2_TYPE::GetValue(M[0][0]),SU2_TYPE::GetValue(M[0][1]),SU2_TYPE::GetValue(M[0][2]),
                  SU2_TYPE::GetValue(M[1][1]),SU2_TYPE::GetValue(M[1][2]),SU2_TYPE::GetValue(M[2][2])};
  };
}

// ---- T-aware first height (production CheckFirstHeight still hard-codes 1e-8 until WP-B) ----------------------------
inline bool HeightT(const State& s,const Patch& p,std::string& why,double* worst=nullptr) {
  if(s.heights.empty())return true;
  FirstHeightReport r;std::string reason;
  if(CheckFirstHeight(p,s.heights,*s.reference,r,reason)){if(worst)*worst=r.maximum_relative_error;return true;}
  if(worst)*worst=r.maximum_relative_error;
  if(reason.find("prescribed height fails")!=std::string::npos && !r.footprint_failures && !r.missing_cells &&
     !r.reference_clearance_failures && (long double)r.maximum_relative_error<=HeightTolerance)return true;
  why=reason;return false;
}
inline bool IsFirstChild(const State& s,const Patch& p,const Cell& c) {
  std::set<Face> walls;for(const auto& f:p.surface)if(s.walls.count(s.reference->Get(f.facet).marker))walls.insert(Sorted(f.v));
  for(auto f:CellFaces(c))if(walls.count(Sorted(f)))return true;return false;
}
struct CellShape { double q=0,J=0; };
inline CellShape ShapeOf(State& s,const Cell& c){const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},s.field->Query(Centroid(c)).target);return {m.mean_ratio,m.minimum_scaled_jacobian};}
inline bool SamePoint(Point a,Point b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline long double ExcessOf(long double l){const auto e=std::max(0.L,l-(long double)MaximumMetricEdge);return e*e;}

struct Decision {
  bool ok=false;int category=0;std::string reason;double q_old=1,J_old=1,q_new=1,J_new=1,first_q=1,first_J=1,other_q=1,other_J=1;
  long double removed=0,added=0;size_t old_cells=0,fresh_cells=0;
};
// One measurement, two admissions (A and D) on the same proposal.
inline std::pair<Decision,Decision> Admit(State& s,const Patch& old,const Patch& fresh,Kind kind) {
  Decision base;base.old_cells=old.cells.size();base.fresh_cells=fresh.cells.size();
  if(old.cells.empty()||fresh.cells.empty()||old.cells.size()>MaximumReplacementCells||fresh.cells.size()>MaximumReplacementCells){
    base.category=5;base.reason="replacement size";return {base,base};}
  std::string why;
  if(!HeightT(s,fresh,why)){base.category=4;base.reason=why;return {base,base};}
  PatchMetric after,before;
  try {
    after=MeasureMetricCells(fresh.cells,*s.field,nullptr,nullptr,false);
    before=MeasureMetricCells(old.cells,*s.field,nullptr,&after.edges,false);
  } catch(const std::exception& e){base.category=5;base.reason=e.what();return {base,base};}
  base.q_old=before.minimum_mean_ratio;base.J_old=before.minimum_scaled_jacobian;base.q_new=after.minimum_mean_ratio;base.J_new=after.minimum_scaled_jacobian;
  for(const auto& c:fresh.cells){const auto sh=ShapeOf(s,c);
    if(IsFirstChild(s,fresh,c)){base.first_q=std::min(base.first_q,sh.q);base.first_J=std::min(base.first_J,sh.J);}
    else{base.other_q=std::min(base.other_q,sh.q);base.other_J=std::min(base.other_J,sh.J);}}
  auto shared=[](const std::map<Edge,EdgeMeasure>& others,const std::pair<const Edge,EdgeMeasure>& e){
    const auto o=others.find(e.first);return o!=others.end()&&SamePoint(o->second.a,e.second.a)&&SamePoint(o->second.b,e.second.b);};
  for(const auto& e:before.edges)if(!shared(after.edges,e))base.removed+=ExcessOf(e.second.length);
  for(const auto& e:after.edges)if(!shared(before.edges,e))base.added+=ExcessOf(e.second.length);
  const bool size_progress=base.removed>0&&base.added<base.removed*(1-1e-12L);
  const bool quality_progress=(base.q_new>base.q_old+1e-7&&base.J_new>=base.J_old*(1-1e-12))||(base.J_new>base.J_old+1e-7&&base.q_new>=base.q_old*(1-1e-12));
  auto progress=[&](Kind k,std::string& r)->bool{
    if(k==Kind::Refine){if(fresh.cells.size()>old.cells.size()&&size_progress)return true;r="refine: no strict changed-edge excess reduction";return false;}
    if(k==Kind::Repair){if(size_progress||(quality_progress&&base.added<=base.removed))return true;r="repair: neither size nor quality progress";return false;}
    // LayerConstruct (D only): all affected faces within T and first children absolute are checked above/below;
    // certificate: added excess <= removed excess (no size regression); the T/first-child count decrease is implied
    // by the absolute D.1 checks on every affected face.
    if(base.added<=base.removed)return true;r="layer: adds frozen excess";return false;};
  Decision a=base,d=base;
  // A: every fresh cell absolute; LayerConstruct is not a kind of A, judged as Repair.
  if(base.q_new<MinimumMeanRatio||base.J_new<MinimumScaledJacobian){a.category=2;a.reason="A shape: q="+std::to_string(base.q_new)+" J="+std::to_string(base.J_new);}
  else if(!progress(kind==Kind::Layer?Kind::Repair:kind,a.reason))a.category=3;else a.ok=true;
  // D: D.1 contract.
  if(base.first_q<MinimumMeanRatio||base.first_J<MinimumScaledJacobian){d.category=2;d.reason="D first child: q="+std::to_string(base.first_q)+" J="+std::to_string(base.first_J);}
  else if(kind==Kind::Layer?!(base.other_q>0&&base.other_J>0):
          (base.other_q<std::min<double>(MinimumMeanRatio,base.q_old)||base.other_J<std::min<double>(MinimumScaledJacobian,base.J_old))){
    d.category=2;d.reason="D floor: q="+std::to_string(base.other_q)+" J="+std::to_string(base.other_J)+" old q="+std::to_string(base.q_old);}
  else if(!progress(kind,d.reason))d.category=3;else d.ok=true;
  return {a,d};
}

// ---- private point insertion inside a patch (visibility bounded by the patch; physical/artificial faces block) ------
struct Work { Patch p;Id next_cell=0; };
inline bool InsertVolumePoint(State& s,Work& w,Node node,std::string& reason) {
  const auto skin=Incidence(w.p.cells).Boundary();std::set<Face> physical,artificial;
  for(const auto& f:w.p.surface)physical.insert(Sorted(f.v));
  for(auto f:skin)if(!physical.count(Sorted(f)))artificial.insert(Sorted(f));
  std::set<Id> in;for(const auto& c:w.p.cells){std::array<long double,4> wt{};if(Barycentric({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},node.p,wt))in.insert(c.id);}
  if(in.empty()){reason="point outside the private patch";return false;}
  for(unsigned pass=0;pass<512;++pass) {
    std::map<Face,int> count;std::map<Face,std::pair<Face,const Cell*>> owner;
    for(const auto& c:w.p.cells)if(in.count(c.id))for(auto f:CellFaces(c)){++count[Sorted(f)];owner[Sorted(f)]={f,&c};}
    bool grew=false;
    for(const auto& row:owner){if(count.at(row.first)!=1)continue;const auto f=row.second.first;std::array<Point,3> q;
      for(unsigned k=0;k<3;++k)for(auto n:row.second.second->v)if(n.id==f[k])q[k]=n.p;
      if(Orientation(node.p,q[0],q[1],q[2])>0)continue;
      if(physical.count(row.first)){reason="private visibility blocked by a physical face";return false;}
      if(artificial.count(row.first)){reason="private visibility leaves the patch";return false;}
      for(const auto& c:w.p.cells)if(!in.count(c.id)){bool has=true;for(Id v:f)has=has&&std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==v;});
        if(has){in.insert(c.id);grew=true;break;}}
      break;}
    if(!grew)break;
  }
  std::vector<Cell> removed,kept;for(const auto& c:w.p.cells)(in.count(c.id)?removed:kept).push_back(c);
  std::vector<Cell> added;if(!Cone(removed,node,w.next_cell,added,reason))return false;
  w.next_cell+=added.size();kept.insert(kept.end(),added.begin(),added.end());
  if(kept.size()>MaximumReplacementCells){reason="private patch exceeds 128 cells";return false;}
  w.p.cells=kept;return true;
}

// ---- engine ---------------------------------------------------------------------------------------------------------
struct Record { size_t work=0;std::string op;Kind kind=Kind::Refine;Decision a,d; };
struct Run {
  std::string outcome;size_t work=0,accepted=0,sweeps=0;double seconds=0;std::map<std::string,size_t> rejections;std::map<std::string,size_t> accepts;
  std::map<std::string,size_t> reasons;  // first 90 characters of each rejection/build-failure reason, per kind
  bool diverged=false;Record divergence;std::string last_witness;
};
struct Limits { size_t work=8192;double seconds=120; };
inline std::vector<std::pair<Id,Face>> WallFaces(const State& s) {
  std::vector<std::pair<Id,Face>> out;
  if(s.wall_indexed){out.reserve(s.wall_index.size());for(const auto& w:s.wall_index)out.push_back({w.second,w.first});return out;}  // DC-14 index
  for(const auto& row:s.mesh){const auto f=CellFaces(row.second.cell);for(unsigned k=0;k<4;++k)
    if(row.second.physical[k].present&&s.walls.count(s.reference->Get(row.second.physical[k].facet).marker))out.push_back({row.first,Sorted(f[k])});}
  std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.second<b.second;});return out;
}
inline std::set<Id> VertexStar(const State& s,const std::vector<Id>& vertices){std::set<Id> out;for(Id v:vertices){const auto f=s.vertex_cells.find(v);if(f!=s.vertex_cells.end())out.insert(f->second.begin(),f->second.end());}return out;}
inline V FaceNormalInward(const State& s,const Patch& p,const SurfaceFace& f){
  std::map<Id,Point> n;for(const auto& c:p.cells)for(auto x:c.v)n[x.id]=x.p;
  // physical skin faces are oriented out of the fluid patch: the fluid normal is opposite
  return -1.L*Unit(Cross(Of(n.at(f.v[1]))-Of(n.at(f.v[0])),Of(n.at(f.v[2]))-Of(n.at(f.v[0]))));
}
// D.3 coordinated first-row LayerConstruct on the closed vertex star of wall face key.
inline bool LayerProposal(State& s,const Face& wall,std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& reason,size_t& points) {
  const auto star=VertexStar(s,{wall[0],wall[1],wall[2]});
  if(star.size()>MaximumCavityCells){reason="LayerConstruct cavity exceeds 64 cells (cap-bound)";return false;}
  ids.assign(star.begin(),star.end());old=PatchOf(s,ids);
  Work w;w.p=old;w.next_cell=s.next_cell+8192;
  // W*: every BL wall face of the cavity; points designed jointly (candidate sets), inserted c_v first then p_f.
  std::vector<SurfaceFace> wstar;for(const auto& f:old.surface)if(s.walls.count(s.reference->Get(f.facet).marker))wstar.push_back(f);
  std::map<Id,Point> nodes;for(const auto& c:old.cells)for(auto x:c.v)nodes[x.id]=x.p;
  std::map<Id,std::vector<V>> vnormals;std::map<Id,size_t> vcount;
  std::set<Face> skinKeys;for(auto f:Incidence(old.cells).Boundary())skinKeys.insert(Sorted(f));
  std::set<Id> artificialNodes;std::set<Face> physKeys;for(const auto& f:old.surface)physKeys.insert(Sorted(f.v));
  for(const auto& f:skinKeys)if(!physKeys.count(f))artificialNodes.insert(f.begin(),f.end());
  std::set<Id> nonwallPhysicalNodes;for(const auto& f:old.surface)if(!s.walls.count(s.reference->Get(f.facet).marker))nonwallPhysicalNodes.insert(f.v.begin(),f.v.end());
  std::vector<Node> pf;Id next=s.next_node;
  for(const auto& f:wstar) {
    const V a=Of(nodes.at(f.v[0])),b=Of(nodes.at(f.v[1])),c=Of(nodes.at(f.v[2]));const V n=FaceNormalInward(s,old,f);
    const auto h=s.heights.at(s.reference->Get(f.facet).marker);
    const std::array<std::array<long double,3>,4> feet{{{1.L/3,1.L/3,1.L/3},{.5L,.25L,.25L},{.25L,.5L,.25L},{.25L,.25L,.5L}}};
    double best=-1;Point chosen{};
    for(const auto& fb:feet){const V x=fb[0]*a+fb[1]*b+fb[2]*c+h*n;const Point p=Pt(x);
      Cell t{0,{Node{f.v[0],nodes.at(f.v[0])},Node{f.v[1],nodes.at(f.v[1])},Node{f.v[2],nodes.at(f.v[2])},Node{0,p}}};
      if(Orientation(t.v[0].p,t.v[1].p,t.v[2].p,t.v[3].p)<0)std::swap(t.v[0],t.v[1]);
      double score=-1;try{const auto sh=ShapeOf(s,t);score=std::min(sh.q/MinimumMeanRatio,sh.J/MinimumScaledJacobian);}catch(const std::exception&){}
      if(score>best){best=score;chosen=p;}}
    pf.push_back({next++,chosen});
    for(Id v:f.v){vnormals[v].push_back(n);++vcount[v];}
  }
  // interior wall vertices of W* (no artificial or non-wall physical incidence) receive c_v along the min-norm normal
  std::vector<Node> cv;
  for(const auto& row:vnormals) {
    const Id v=row.first;if(artificialNodes.count(v)||nonwallPhysicalNodes.count(v))continue;
    V dir;if(!MinNorm(row.second,dir))continue;
    const auto h=s.h0;double best=-1;Point chosen{};
    for(long double beta:{1.6L,2.2L,3.0L}){const Point p=Pt(Of(nodes.at(v))+beta*h*dir);
      // score: ring cells (v, p_fi, p_fj, c) over consecutive wall faces at v
      double worst=1e9;bool any=false;
      for(size_t i=0;i<wstar.size();++i)for(size_t j=0;j<wstar.size();++j)if(i!=j){
        const auto& fi=wstar[i];const auto& fj=wstar[j];if(!std::count(fi.v.begin(),fi.v.end(),v)||!std::count(fj.v.begin(),fj.v.end(),v))continue;
        int shared=0;for(Id x:fi.v)shared+=std::count(fj.v.begin(),fj.v.end(),x);if(shared!=2)continue;
        Cell t{0,{Node{v,nodes.at(v)},pf[i],pf[j],Node{0,p}}};const auto o=Orientation(t.v[0].p,t.v[1].p,t.v[2].p,t.v[3].p);
        if(o<=0)continue;any=true;try{const auto sh=ShapeOf(s,t);worst=std::min(worst,std::min(sh.q/MinimumMeanRatio,sh.J/MinimumScaledJacobian));}catch(const std::exception&){worst=-1;}}
      if(any&&worst>best){best=worst;chosen=p;}}
    if(best>-1)cv.push_back({next++,chosen});
  }
  std::vector<Node> order=cv;order.insert(order.end(),pf.begin(),pf.end());points=order.size();
  std::set<Id> wanted;
  for(const auto& node:order){std::string why;if(!InsertVolumePoint(s,w,node,why)){reason="LayerConstruct insertion: "+why;return false;}wanted.insert(node.id);}
  // enclosed volume points: re-insert once (RestorePatchPoints semantics, test-side)
  {std::set<Id> present;for(const auto& c:w.p.cells)for(auto x:c.v)present.insert(x.id);
   for(const auto& node:order)if(!present.count(node.id)){std::string why;if(!InsertVolumePoint(s,w,node,why)){reason="LayerConstruct restore: "+why;return false;}}}
  // physical faces of the patch are unchanged (volume-only insertion); keep the surface list
  {Patch repaired;ConnectivityRepairReport report;std::string why;const std::set<Id> movable(wanted.begin(),wanted.end());
   try{if(RepairMetricConnectivity(w.p,*s.field,*s.reference,s.heights,w.next_cell,repaired,report,why,64,false,&movable)){w.p=std::move(repaired);}}catch(const std::exception&){}}
  fresh=w.p;for(size_t i=0;i<fresh.cells.size();++i)fresh.cells[i].id=s.next_cell+i;
  // the fixed artificial interface must be unchanged
  {std::map<Face,Face> after;for(auto f:Incidence(fresh.cells).Boundary())after[Sorted(f)]=f;
   for(auto f:Incidence(old.cells).Boundary()){if(physKeys.count(Sorted(f)))continue;if(!after.count(Sorted(f))){reason="LayerConstruct changed the artificial interface";return false;}}}
  try{s.reference->ExportParameters(fresh);}catch(const std::exception& e){reason=e.what();return false;}
  return true;
}
// Production order (CNativeEngine3D.cpp:1704-1724, :2403-2504): balanced metric split point, SplitEdge then
// InsertEdgeCavity, physical central point as second choice; wall edges through the wall reconstructions.
constexpr int RefineVariants=5;
inline bool RefineProposal(State& s,Edge e,std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& op,std::string& reason,int variant) {
  const auto star=Star(s,e);if(star.empty()||star.size()>MaximumCavityCells){reason="edge star exceeds 64";return false;}
  const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);
  const bool physical=PhysicalEdge(s,e);bool wall=false;
  for(Id id:star){const auto& row=s.mesh.at(id);const auto f=CellFaces(row.cell);for(unsigned k=0;k<4;++k)
    if(row.physical[k].present&&std::count(f[k].begin(),f[k].end(),e[0])&&std::count(f[k].begin(),f[k].end(),e[1])&&s.walls.count(s.reference->Get(row.physical[k].facet).marker))wall=true;}
  Point balanced,central{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
  try{balanced=s.field->BalancedSplitPoint(a,b);}catch(const std::exception& x){balanced=central;}
  if(physical){try{central=s.field->SegmentPoint(a,b,.5);}catch(const std::exception&){}}
  ids=star;old=PatchOf(s,ids);
  if(wall) {
    const Node point{s.next_node,central};
    // wall reconstructions need complete stars of the free apices: extend the edge star by the vertex stars of its
    // non-endpoint vertices while the 64-cell cavity bound holds (deterministic: sorted vertex order)
    if(variant>=1) {
      std::set<Id> region(star.begin(),star.end());std::set<Id> verts;for(Id id:star)for(auto n:s.mesh.at(id).cell.v)if(n.id!=e[0]&&n.id!=e[1])verts.insert(n.id);
      for(Id v:verts){auto more=region;for(Id id:s.vertex_cells.at(v))more.insert(id);if(more.size()<=MaximumCavityCells)region=more;}
      ids.assign(region.begin(),region.end());old=PatchOf(s,ids);
    }
    if(variant==0||variant==1) {
      for(unsigned grow=0;grow<4;++grow) {
        Face obstruction{};Feature feature{};op=variant==0?"ReconstructWallSplit":"RefitWallSplit";
        try{if(variant==0?ReconstructWallSplit(old,e,point,s.next_node+1,s.next_cell,s.heights,*s.reference,fresh,obstruction,reason):
                          RefitWallSplit(old,e,point,s.next_node+1,s.next_cell,s.heights,*s.reference,fresh,feature,reason))return true;}catch(const std::exception& x){reason=x.what();}
        if(variant==1)break;
        if(obstruction==Face{})break;
        std::set<Id> more(ids.begin(),ids.end());for(Id id:VertexStar(s,{obstruction[0],obstruction[1],obstruction[2]})){const auto& v=s.mesh.at(id).cell.v;
          unsigned m=0;for(auto n:v)m+=std::count(obstruction.begin(),obstruction.end(),n.id);if(m==3)more.insert(id);}
        if(more.size()==ids.size()||more.size()>MaximumCavityCells)break;ids.assign(more.begin(),more.end());old=PatchOf(s,ids);
      }
      return false;
    }
    if(variant==2) {
      op="PlaceSplitPatch";const auto apices=SplitPatchApices(old,e);if(apices.empty()){reason="no bounded apex cover";return false;}
      std::vector<std::pair<Id,Id>> reserved;Id next=s.next_node+1;for(Id x:apices)reserved.push_back({x,next++});
      JointSplitReport report;try{return PlaceSplitPatch(old,e,point,reserved,s.next_cell,*s.field,*s.reference,s.heights,fresh,report,reason,128);}catch(const std::exception& x){reason=x.what();return false;}
    }
    return false;
  }
  const Point p=variant<2?balanced:central;if(variant>=4||(variant>=2&&!physical)){reason="no further variant";return false;}
  const Node point{s.next_node,p};
  if(variant%2==0){op="SplitEdge";try{return SplitEdge(old,e,point,s.next_cell,*s.reference,fresh,reason);}catch(const std::exception& x){reason=x.what();return false;}}
  op="InsertEdgeCavity";try{return InsertEdgeCavity(old,e,point,s.next_cell,*s.reference,fresh,reason);}catch(const std::exception& x){reason=x.what();return false;}
}
inline bool RepairProposal(State& s,Id cell,std::vector<Id>& ids,Patch& old,Patch& fresh,std::string& reason) {
  std::vector<Id> vs;for(auto n:s.mesh.at(cell).cell.v)vs.push_back(n.id);auto star=VertexStar(s,vs);
  if(star.size()>MaximumCavityCells){star.clear();for(Id id:VertexStar(s,{vs[0],vs[1]}))star.insert(id);}
  if(star.size()>MaximumCavityCells){reason="repair star exceeds 64";return false;}
  ids.assign(star.begin(),star.end());old=PatchOf(s,ids);ConnectivityRepairReport report;
  try{return RepairMetricConnectivity(old,*s.field,*s.reference,s.heights,s.next_cell,fresh,report,reason,64,true);}catch(const std::exception& x){reason=x.what();return false;}
}
struct Gates { size_t cells=0,bad_shape=0,long_edges=0,wall_faces=0,height_failures=0,long_wall=0,long_seam=0,long_other_physical=0,long_interior=0;double q=1,J=1;long double lmax=0;bool complete=false; };
inline Gates FinalGates(State& s) {
  Gates g;g.cells=s.mesh.size();std::set<Edge> seen;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;const auto sh=ShapeOf(s,c);g.q=std::min(g.q,sh.q);g.J=std::min(g.J,sh.J);
    g.bad_shape+=sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      const auto l=Length(s,e[0],e[1]);g.lmax=std::max(g.lmax,l);g.long_edges+=l>MaximumMetricEdge;
      if(l>MaximumMetricEdge){bool w=false,o=false;for(Id id:Star(s,e)){const auto& r=s.mesh.at(id);const auto f=CellFaces(r.cell);for(unsigned k=0;k<4;++k)
          if(r.physical[k].present&&std::count(f[k].begin(),f[k].end(),e[0])&&std::count(f[k].begin(),f[k].end(),e[1]))(s.walls.count(s.reference->Get(r.physical[k].facet).marker)?w:o)=true;}
        if(w&&o)++g.long_seam;else if(w)++g.long_wall;else if(o)++g.long_other_physical;else ++g.long_interior;}}}
  for(const auto& wf:WallFaces(s)){++g.wall_faces;std::string why;if(!HeightT(s,PatchOf(s,{wf.first}),why))++g.height_failures;}
  g.complete=!g.bad_shape&&!g.long_edges&&!g.height_failures;return g;
}
inline Run Engine(State& s,Adm adm,const Limits& lim,std::ostream& log,std::vector<Record>* trace=nullptr) {
  Run run;const auto start=Clock::now();
  auto elapsed=[&]{return std::chrono::duration<double>(Clock::now()-start).count();};
  auto attempt=[&](const std::string& op,Kind kind,const std::vector<Id>& ids,const Patch& old,const Patch& fresh)->bool{
    ++run.work;const auto ad=Admit(s,old,fresh,kind);const auto& mine=adm==Adm::A?ad.first:ad.second;
    if(!run.diverged&&ad.first.ok!=ad.second.ok){run.diverged=true;run.divergence={run.work,op,kind,ad.first,ad.second};}
    if(trace&&trace->size()<4096)trace->push_back({run.work,op,kind,ad.first,ad.second});
    if(!mine.ok){++run.rejections[std::string(KindName(kind))+":"+std::to_string(mine.category)];run.last_witness=op+": "+mine.reason;
      ++run.reasons[std::string(KindName(kind))+" "+op+": "+mine.reason.substr(0,90)];return false;}
    Apply(s,ids,fresh);++run.accepted;++run.accepts[op];return true;};
  for(;;++run.sweeps) {
    if(run.work>=lim.work||elapsed()>=lim.seconds){run.outcome="limit";break;}
    bool changed=false;
    // 1. LayerConstruct (D) / first-row repair (A: same proposal judged as Repair) on faces failing T or q/J.
    for(const auto& wf:WallFaces(s)) {
      if(run.work>=lim.work||elapsed()>=lim.seconds)break;
      if(!s.mesh.count(wf.first))continue;const auto cell=s.mesh.at(wf.first).cell;
      std::string why;bool fails=!HeightT(s,PatchOf(s,{wf.first}),why);
      if(!fails){const auto sh=ShapeOf(s,cell);fails=sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian;}
      if(!fails)continue;
      bool still=false;for(const auto& x:WallFaces(s))still=still||(x.second==wf.second);if(!still)continue;
      std::vector<Id> ids;Patch old,fresh;std::string reason;size_t pts=0;
      if(!LayerProposal(s,wf.second,ids,old,fresh,reason,pts)){++run.work;++run.rejections["Layer:build"];run.last_witness="LayerConstruct build: "+reason;++run.reasons["Layer build: "+reason.substr(0,90)];continue;}
      changed|=attempt("LayerConstruct",Kind::Layer,ids,old,fresh);
    }
    // 2. Refine long edges, longest first.
    std::vector<std::pair<long double,Edge>> longs;std::set<Edge> seen;
    for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
      const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
      const auto l=Length(s,e[0],e[1]);if(l>MaximumMetricEdge)longs.emplace_back(-l,e);}}
    std::sort(longs.begin(),longs.end());
    for(const auto& row:longs) {
      if(run.work>=lim.work||elapsed()>=lim.seconds)break;
      if(Star(s,row.second).empty())continue;
      std::vector<Id> ids;Patch old,fresh;std::string op,reason;
      // ordered candidate list (identical for both admissions): cavity insertion, then edge-star split; each raw and
      // after private repair of the fresh patch (RepairMetricConnectivity, strictly improved or unchanged)
      bool taken=false;
      for(int variant=0;variant<RefineVariants&&!taken;++variant) {
        if(!RefineProposal(s,row.second,ids,old,fresh,op,reason,variant)){if(reason!="no further variant"){++run.work;++run.rejections["Refine:build"];++run.reasons["Refine build: "+reason.substr(0,90)];}continue;}
        Patch repaired;ConnectivityRepairReport report;std::string why;bool better=false;
        try{better=RepairMetricConnectivity(fresh,*s.field,*s.reference,s.heights,s.next_cell+4096,repaired,report,why,128,false);}catch(const std::exception&){better=false;}
        if(better){for(size_t i=0;i<repaired.cells.size();++i)repaired.cells[i].id=s.next_cell+i;try{s.reference->ExportParameters(repaired);}catch(const std::exception&){better=false;}}
        if(better&&attempt(op+"+repair",Kind::Refine,ids,old,repaired)){taken=true;break;}
        if(attempt(op,Kind::Refine,ids,old,fresh)){taken=true;break;}
      }
      changed|=taken;
    }
    // 3. Repair bad cells.
    std::vector<std::pair<double,Id>> bad;for(const auto& row:s.mesh){const auto sh=ShapeOf(s,row.second.cell);if(sh.q<MinimumMeanRatio||sh.J<MinimumScaledJacobian)bad.emplace_back(sh.q,row.first);}
    std::sort(bad.begin(),bad.end());
    for(const auto& row:bad) {
      if(run.work>=lim.work||elapsed()>=lim.seconds)break;if(!s.mesh.count(row.second))continue;
      std::vector<Id> ids;Patch old,fresh;std::string reason;
      if(!RepairProposal(s,row.second,ids,old,fresh,reason)){++run.work;++run.rejections["Repair:build"];run.last_witness="Repair build: "+reason;++run.reasons["Repair build: "+reason.substr(0,90)];continue;}
      changed|=attempt("RepairMetricConnectivity",Kind::Repair,ids,old,fresh);
    }
    const auto g=FinalGates(s);
    std::cout<<"DC5 sweep "<<run.sweeps<<" adm="<<(adm==Adm::A?"A":"D")<<" cells="<<g.cells<<" long="<<g.long_edges<<" bad="<<g.bad_shape<<" work="<<run.work<<std::endl;
    log<<"DC5 sweep "<<run.sweeps<<" cells="<<g.cells<<" q="<<g.q<<" J="<<g.J<<" lmax="<<double(g.lmax)<<" bad="<<g.bad_shape<<" long="<<g.long_edges
       <<" height_fail="<<g.height_failures<<" work="<<run.work<<" accepted="<<run.accepted<<"\n";
    if(g.complete){run.outcome="complete";break;}
    if(run.work>=lim.work||elapsed()>=lim.seconds){run.outcome="limit";break;}
    if(!changed){run.outcome="blocked";break;}
  }
  run.seconds=elapsed();return run;
}
inline void Export(State& s,const std::string& path) {
  std::ofstream out(path);out<<std::setprecision(17);std::map<Id,Point> nodes;for(const auto& row:s.mesh)for(auto n:row.second.cell.v)nodes[n.id]=n.p;
  out<<"nodes "<<nodes.size()<<"\n";for(const auto& n:nodes)out<<n.first<<' '<<n.second.x<<' '<<n.second.y<<' '<<n.second.z<<"\n";
  out<<"cells "<<s.mesh.size()<<"\n";for(const auto& row:s.mesh){out<<row.first;for(auto n:row.second.cell.v)out<<' '<<n.id;out<<"\n";}
  size_t nf=0;for(const auto& row:s.mesh)for(const auto& f:row.second.physical)nf+=f.present;
  out<<"faces "<<nf<<"\n";for(const auto& row:s.mesh){const auto f=CellFaces(row.second.cell);for(unsigned k=0;k<4;++k)if(row.second.physical[k].present){
    const int m=s.reference->Get(row.second.physical[k].facet).marker;out<<row.first<<' '<<f[k][0]<<' '<<f[k][1]<<' '<<f[k][2]<<' '<<m<<' '<<(s.walls.count(m)?1:0)<<"\n";}}
  out<<"h0 "<<s.h0<<"\n";
}
inline std::string Json(const Decision& d){std::ostringstream o;o<<std::setprecision(10)<<"{\"ok\":"<<d.ok<<",\"category\":"<<d.category<<",\"reason\":"<<junction_wp0::Json(d.reason)
  <<",\"q_old\":"<<d.q_old<<",\"q_new\":"<<d.q_new<<",\"first_q\":"<<d.first_q<<",\"first_J\":"<<d.first_J<<",\"other_q\":"<<d.other_q<<",\"other_J\":"<<d.other_J
  <<",\"removed\":"<<double(d.removed)<<",\"added\":"<<double(d.added)<<",\"cells\":["<<d.old_cells<<','<<d.fresh_cells<<"]}";return o.str();}
inline std::string Json(const Run& r,const Gates& g){std::ostringstream o;o<<std::setprecision(12)<<"{\"outcome\":\""<<r.outcome<<"\",\"work\":"<<r.work<<",\"accepted\":"<<r.accepted<<",\"sweeps\":"<<r.sweeps
  <<",\"seconds\":"<<r.seconds<<",\"final\":{\"cells\":"<<g.cells<<",\"q_min\":"<<g.q<<",\"J_min\":"<<g.J<<",\"edge_max\":"<<double(g.lmax)<<",\"bad_shape\":"<<g.bad_shape
  <<",\"long_edges\":"<<g.long_edges<<",\"long_by_class\":{\"wall\":"<<g.long_wall<<",\"seam\":"<<g.long_seam<<",\"other_physical\":"<<g.long_other_physical<<",\"interior\":"<<g.long_interior<<"},\"wall_faces\":"<<g.wall_faces<<",\"height_failures\":"<<g.height_failures<<",\"complete\":"<<g.complete<<"},\"rejections\":{";
  bool c=false;for(const auto& x:r.rejections){o<<(c?",":"")<<'"'<<x.first<<"\":"<<x.second;c=true;}o<<"},\"accepts\":{";c=false;for(const auto& x:r.accepts){o<<(c?",":"")<<'"'<<x.first<<"\":"<<x.second;c=true;}
  o<<"},\"reasons_top\":{";{std::vector<std::pair<size_t,std::string>> top;for(const auto& x:r.reasons)top.emplace_back(x.second,x.first);std::sort(top.rbegin(),top.rend());
    for(size_t i=0;i<top.size()&&i<25;++i)o<<(i?",":"")<<junction_wp0::Json(top[i].second)<<':'<<top[i].first;}
  o<<"},\"last_witness\":"<<junction_wp0::Json(r.last_witness)<<",\"diverged\":"<<r.diverged;
  if(r.diverged)o<<",\"divergence\":{\"work\":"<<r.divergence.work<<",\"op\":\""<<r.divergence.op<<"\",\"kind\":\""<<KindName(r.divergence.kind)<<"\",\"A\":"<<Json(r.divergence.a)<<",\"D\":"<<Json(r.divergence.d)<<'}';
  o<<'}';return o.str();}
}  // namespace layer2d
