/*!
 * \file CNativeBLTarget3D.hpp
 * \brief Shared FIRST_ROW target for normalization, construction and reassessment.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CNativeBLVector3D.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "CNativeBLComposition3D.hpp"
#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CTransferMemory.hpp"
#include <deque>
#include <unordered_map>
namespace dc10 {
using namespace junction_wp0;
// ---- (A) option-A target ------------------------------------------------------------------------------------------
using M3=std::array<std::array<long double,3>,3>;
inline M3 FromTensor(const Tensor& t){return M3{{{t.xx,t.xy,t.xz},{t.xy,t.yy,t.yz},{t.xz,t.yz,t.zz}}};}
inline Tensor ToTensor(const M3& m){return Tensor{double(m[0][0]),double((m[0][1]+m[1][0])/2),double((m[0][2]+m[2][0])/2),double(m[1][1]),double((m[1][2]+m[2][1])/2),double(m[2][2])};}
// Cyclic Jacobi eigen-decomposition of a symmetric 3x3 matrix: A = Q diag(w) Q^T.
inline void Eig3(const M3& A,std::array<long double,3>& w,M3& Q) {
  M3 a=A;Q=M3{{{1,0,0},{0,1,0},{0,0,1}}};
  for(unsigned sweep=0;sweep<64;++sweep) {
    const long double off=a[0][1]*a[0][1]+a[0][2]*a[0][2]+a[1][2]*a[1][2],diag=a[0][0]*a[0][0]+a[1][1]*a[1][1]+a[2][2]*a[2][2];
    if(!(off>1e-40L*diag))break;
    for(int p=0;p<2;++p)for(int q=p+1;q<3;++q) {
      if(a[p][q]==0)continue;
      const long double theta=(a[q][q]-a[p][p])/(2*a[p][q]);
      const long double t=(theta>=0?1.L:-1.L)/(std::abs(theta)+std::sqrt(theta*theta+1)),c=1/std::sqrt(t*t+1),s=t*c;
      for(int k=0;k<3;++k){const auto akp=a[k][p],akq=a[k][q];a[k][p]=c*akp-s*akq;a[k][q]=s*akp+c*akq;}
      for(int k=0;k<3;++k){const auto apk=a[p][k],aqk=a[q][k];a[p][k]=c*apk-s*aqk;a[q][k]=s*apk+c*aqk;}
      for(int k=0;k<3;++k){const auto qkp=Q[k][p],qkq=Q[k][q];Q[k][p]=c*qkp-s*qkq;Q[k][q]=s*qkp+c*qkq;}
    }
  }
  for(int i=0;i<3;++i)w[i]=a[i][i];
}
inline M3 Function3(const M3& A,long double (*f)(long double)) {
  std::array<long double,3> w;M3 Q;Eig3(A,w,Q);M3 out{};
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){long double x=0;for(int k=0;k<3;++k)x+=Q[i][k]*f(w[k])*Q[j][k];out[i][j]=x;}
  return out;
}
inline long double LogL(long double x){if(!(x>0))throw std::runtime_error("DC10 option A: non-positive metric eigenvalue");return std::log(x);}
inline long double ExpL(long double x){return std::exp(x);}
inline long double Smooth(long double u){u=std::clamp(u,0.L,1.L);return u*u*(3-2*u);}
struct WedgeCrease { V a,b,t,n1,n2,m1,m2;long double L=0,turn=0;Id f1=0,f2=0; };
struct StripPair { size_t i=0,j=0;V nb; };
// Crease chains: segments joined at endpoints of crease degree 2; degree >= 3 is a corner, degree 1 a free end (or a
// carrier end). Side labels (n1/m1 = side 1) are made consistent along a chain. Fan count per chain from the median
// turn: k = ceil(turn / 49.5 deg) - 1 (theta_max = 45 deg with 10% hysteresis), at least 0.
struct Chain { std::vector<size_t> segments;long double median=0;int k=0; };
using PKey=std::array<double,3>;
inline PKey KeyOf(V x){return {double(x.x),double(x.y),double(x.z)};}
struct OptionA {
  std::vector<WedgeCrease> creases;std::vector<StripPair> strips;long double radius=0,onLine=0,r0=0;
  std::vector<Chain> chains;std::vector<size_t> chainOf;std::map<PKey,std::vector<size_t>> atPoint;
  // [rev 3] w(x) = max_c S(r_c/r0) (1 - S(2 r_c/R_f - 1)) A_c F(d/r_c): r_c the distance to crease c, d the BL wall
  // distance (min over walls of the base layers' exact nearest-face distance), F(u) = S((u - cos 22.5)/(1 - cos 22.5)),
  // A_c = 1 - S((a - 45)/45) with a the angle (deg) of the direction outside the crease's polar cone (A = 1 inside the
  // wedge and up to 45 deg outside, 0 beyond 90 deg). d = 0 on every wall, so w = 0 on every wall; d = r_c and A = 1 in
  // the wedge, so w = 1 there (production D1, isotropic about the crease); continuous everywhere (no switch, cutoff or
  // prefilter jump); d is evaluated only where some term is not already 0 by its geometric factors.
  std::shared_ptr<CBoundaryLayerMetric> distanceLayers;double core=0;mutable uint64_t distanceEvaluations=0;
  long double WallDistance(V x) const {
    const su2double X[3]={double(x.x),double(x.y),double(x.z)};long double best=INFINITY;
    for(unsigned short w=0;w<distanceLayers->GetnWall();++w)best=std::min(best,(long double)SU2_TYPE::GetValue(distanceLayers->Evaluate(w,X,core).distance));
    ++distanceEvaluations;return best;
  }
  // DC-14 exact broad phase: a crease farther than R_f (radius) has radial factor 0 (1 - S(2 r/R_f - 1) = 0 for r >= R_f),
  // so only the segments whose box grown by R_f contains x can contribute; they are visited in increasing index order
  // (identical candidate list, w is a maximum). Built by IndexCreases (BuildOptionA); empty grid = the full loop.
  using GridKey=std::array<long long,3>;
  struct GridHash{size_t operator()(const GridKey& k)const{return size_t(k[0]*73856093LL^k[1]*19349663LL^k[2]*83492791LL);}};
  std::unordered_map<GridKey,std::vector<size_t>,GridHash> grid;long double cell=0;
  GridKey KeyAt(V x)const{return {(long long)std::floor(x.x/cell),(long long)std::floor(x.y/cell),(long long)std::floor(x.z/cell)};}
  void IndexCreases() {
    grid.clear();cell=0;if(creases.empty()||!(radius>0)||!std::isfinite((double)radius))return;
    // DC-15: buckets of radius/k (k = 8, 4, 2, 1, the finest within 2^23 registrations): every segment is registered in
    // every bucket its box grown by R_f meets, so the candidate list of a point still holds every segment closer than
    // R_f, in increasing index order (exact, fewer far segments per query; the profile's top cost centre)
    for(long double k:{8.L,4.L,2.L,1.L}) {
      cell=radius/k;std::vector<std::array<GridKey,2>> boxes;long double buckets=0;
      for(const auto& c:creases){V lo{std::min(c.a.x,c.b.x)-radius,std::min(c.a.y,c.b.y)-radius,std::min(c.a.z,c.b.z)-radius};
        V hi{std::max(c.a.x,c.b.x)+radius,std::max(c.a.y,c.b.y)+radius,std::max(c.a.z,c.b.z)+radius};
        boxes.push_back({KeyAt(lo),KeyAt(hi)});const auto& b=boxes.back();
        buckets+=(long double)(b[1][0]-b[0][0]+1)*(long double)(b[1][1]-b[0][1]+1)*(long double)(b[1][2]-b[0][2]+1);}
      if(buckets>(long double)(size_t(1)<<23))continue;
      for(size_t i=0;i<creases.size();++i){const auto& b=boxes[i];
        for(long long u=b[0][0];u<=b[1][0];++u)for(long long v=b[0][1];v<=b[1][1];++v)for(long long w=b[0][2];w<=b[1][2];++w)grid[{u,v,w}].push_back(i);}
      return;
    }
    grid.clear();cell=0;  // bounded index; otherwise the exact full loop
  }
  long double Weight(V x) const {
    const long double pi=std::acos(-1.L),c0=std::cos(pi/8);
    thread_local std::vector<std::pair<long double,long double>> cand;cand.clear();  // (radial factor, r_c)
    static const std::vector<size_t> none;const std::vector<size_t>* visit=nullptr;
    if(cell>0){const auto f=grid.find(KeyAt(x));visit=f==grid.end()?&none:&f->second;}
    const size_t count=visit?visit->size():creases.size();
    for(size_t ci=0;ci<count;++ci) {const auto& c=creases[visit?(*visit)[ci]:ci];
      const auto s=std::clamp(Dot(x-c.a,c.t),0.L,c.L);const V d=x-(c.a+s*c.t);const auto r=Norm(d);if(!(r>0))continue;
      const auto radial=Smooth(r/r0)*(1-Smooth(2*r/radius-1));if(!(radial>0))continue;
      V u=d-Dot(d,c.t)*c.t;const auto ru=Norm(u);if(ru>0){u=(1/ru)*u;
        long double out=0;const auto u1=-Dot(u,c.m1),u2=-Dot(u,c.m2);
        if(u1<0)out=std::max(out,std::atan2(-u1,Dot(u,c.n1)));if(u2<0)out=std::max(out,std::atan2(-u2,Dot(u,c.n2)));
        const auto axial=(ru/r)*(ru/r);  // beyond a segment end the transverse direction degenerates: fade weighted by it
        const auto angular=1-Smooth((out-pi/4)/(pi/4))*axial;if(!(angular>0))continue;cand.emplace_back(radial*angular,r);continue;}
      cand.emplace_back(radial,r);
    }
    if(cand.empty())return 0;
    const auto d=WallDistance(x);long double w=0;
    for(const auto& c:cand)w=std::max(w,c.first*Smooth((std::min(d/c.second,1.L)-c0)/(1-c0)));
    return w;
  }
  // Thin strip (W rule, not part of the target): x on the closed strip of pair p, its width and unit across direction.
  bool InStrip(const StripPair& p,V x,long double& width,V& e) const {
    const auto& ci=creases[p.i];const auto& cj=creases[p.j];
    const auto si=Dot(x-ci.a,ci.t),sj=Dot(x-cj.a,cj.t);
    if(si<-onLine||si>ci.L+onLine||sj<-onLine||sj>cj.L+onLine)return false;
    const V qi=ci.a+si*ci.t,qj=cj.a+sj*cj.t;e=qj-qi;width=Norm(e);if(!(width>0))return false;e=(1/width)*e;
    if(Dot(x-qi,e)<-onLine||Dot(x-qj,e)>onLine)return false;
    return std::abs(Dot(p.nb,x-qi))<=1e-6L*width;  // on the strip face plane
  }
  Tensor Compose(CBoundaryLayerMetric& base,CBoundaryLayerMetric& full,double core,Point p,Tensor sensor,const CBoundaryLayerMetric::ChordOwners* owners,long double t) const {
    const V x=Of(p);const auto w=Weight(x);
    const Tensor mb=SU2NativeBL3D::Compose(base,core,p,sensor,owners,t);
    if(!(w>0))return mb;
    const Tensor mf=SU2NativeBL3D::Compose(full,core,p,sensor,owners,t);
    if(w>=1)return mf;
    return Blend(mb,mf,w,sensor);
  }
  static Tensor Blend(Tensor mb,Tensor mf,long double w,Tensor sensor) {
    if(!(w>0))return mb;
    if(w>=1)return mf;
    const auto lb=Function3(FromTensor(mb),LogL),lf=Function3(FromTensor(mf),LogL);M3 l;
    for(int a=0;a<3;++a)for(int b=0;b<3;++b)l[a][b]=(1-w)*lb[a][b]+w*lf[a][b];
    return SU2NativeBL3D::PreserveSensor(ToTensor(Function3(l,ExpL)),sensor);
  }
};
// Crease detection: the production rule of dc9::DetectCreases (wall-wall and wall-partner edges, exact convexity,
// face-normal angle > ADAP_ANGLE; symmetry and cut facets are continuations), keeping the two facets.
// Marker roles from the loader (configuration kinds / fixture metadata): 1 BL wall, 2 non-reflective physical partner,
// 0 continuation (reflective carrier or artificial cut boundary). No marker name is read here.
inline std::shared_ptr<OptionA> BuildOptionA(const std::vector<Facet>& facets,const std::map<int,int>& roles,double angle,double radius,double h0) {
  auto A=std::make_shared<OptionA>();A->radius=radius;A->r0=h0/10;
  {long double scale=1;for(const auto& f:facets)for(auto x:f.v)scale=std::max({scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});A->onLine=1e-9L*scale;}
  auto kind=[&](const Facet& f){return roles.at(f.marker);};
  std::map<std::pair<Id,Id>,std::vector<const Facet*>> edges;
  for(const auto& f:facets)if(kind(f)==1)for(int k=0;k<3;++k){const Id a=f.v[k].id,b=f.v[(k+1)%3].id;edges[{std::min(a,b),std::max(a,b)}].push_back(&f);}
  for(const auto& f:facets)if(kind(f)==2)for(int k=0;k<3;++k){const Id a=f.v[k].id,b=f.v[(k+1)%3].id;const auto e=edges.find({std::min(a,b),std::max(a,b)});if(e!=edges.end())e->second.push_back(&f);}
  auto normal=[](const Facet& f) {
    const long double sign=Orientation({0,0,0},{1,0,0},{0,1,0},{0,0,1})>0?1.L:-1.L;
    return sign*Unit(Cross(Of(f.v[1].p)-Of(f.v[0].p),Of(f.v[2].p)-Of(f.v[0].p)));
  };
  const long double pi=std::acos(-1.L);
  for(const auto& row:edges) {
    if(row.second.size()!=2)continue;const Facet& f1=*row.second[0];const Facet& f2=*row.second[1];
    unsigned k=0;while(k<3&&!(std::minmax(f1.v[k].id,f1.v[(k+1)%3].id)==std::minmax(row.first.first,row.first.second)))++k;if(k==3)continue;
    const Point a=f1.v[k].p,b=f1.v[(k+1)%3].p,o1=f1.v[(k+2)%3].p;Point o2{};for(auto x:f2.v)if(x.id!=row.first.first&&x.id!=row.first.second)o2=x.p;
    if(!(Orientation(a,b,o1,o2)>0&&Orientation(f2.v[0].p,f2.v[1].p,f2.v[2].p,o1)>0))continue;
    WedgeCrease c;c.a=Of(a);c.b=Of(b);c.L=Norm(c.b-c.a);c.t=(1/c.L)*(c.b-c.a);c.f1=f1.id;c.f2=f2.id;
    c.n1=-1.L*normal(f1);c.n2=-1.L*normal(f2);
    auto inFace=[&](Point o){V d=Of(o)-c.a;d=d-Dot(d,c.t)*c.t;return Unit(d);};c.m1=inFace(o1);c.m2=inFace(o2);
    c.turn=std::acos(std::clamp(Dot(c.n1,c.n2),-1.L,1.L));
    if(!(c.turn*180/pi>angle))continue;
    A->creases.push_back(c);
  }
  // chains
  for(size_t i=0;i<A->creases.size();++i){A->atPoint[KeyOf(A->creases[i].a)].push_back(i);A->atPoint[KeyOf(A->creases[i].b)].push_back(i);}
  A->chainOf.assign(A->creases.size(),SIZE_MAX);
  for(size_t i=0;i<A->creases.size();++i) {
    if(A->chainOf[i]!=SIZE_MAX)continue;Chain ch;std::deque<size_t> queue{i};A->chainOf[i]=A->chains.size();
    while(!queue.empty()){const size_t c=queue.front();queue.pop_front();ch.segments.push_back(c);
      for(const V& end:{A->creases[c].a,A->creases[c].b}){const auto& at=A->atPoint.at(KeyOf(end));if(at.size()!=2)continue;
        for(size_t o:at)if(A->chainOf[o]==SIZE_MAX){
          auto& B=A->creases[o];const auto& P=A->creases[c];
          if(Dot(P.n1,B.n1)<Dot(P.n1,B.n2)){std::swap(B.n1,B.n2);std::swap(B.m1,B.m2);std::swap(B.f1,B.f2);}
          A->chainOf[o]=A->chains.size();queue.push_back(o);}}}
    std::vector<long double> turns;for(size_t c:ch.segments)turns.push_back(A->creases[c].turn);std::sort(turns.begin(),turns.end());
    ch.median=turns[turns.size()/2];ch.k=std::max(0,int(std::ceil(ch.median*180/pi/49.5L-1e-12L))-1);A->chains.push_back(ch);
  }
  // thin strips: parallel creases (|t_i.t_j| >= cos 30) with a coplanar face each (normals within 10 deg) bounding the
  // strip from both sides, width <= the D1 feature radius
  for(size_t i=0;i<A->creases.size();++i)for(size_t j=i+1;j<A->creases.size();++j) {
    const auto& ci=A->creases[i];const auto& cj=A->creases[j];if(std::abs(Dot(ci.t,cj.t))<std::cos(pi/6))continue;
    for(int si=0;si<2;++si)for(int sj=0;sj<2;++sj) {
      const V ni=si?ci.n2:ci.n1,nj=sj?cj.n2:cj.n1,mi=si?ci.m2:ci.m1,mj=sj?cj.m2:cj.m1;if(Dot(ni,nj)<std::cos(pi/18))continue;
      const V mid=ci.a+(ci.L/2)*ci.t,q=cj.a+Dot(mid-cj.a,cj.t)*cj.t,e=q-mid;const auto width=Norm(e);
      if(!(width>0)||width>radius||std::abs(Dot(ni,e))>.1L*width||!(Dot(mi,e)>0)||!(Dot(mj,-1.L*e)>0))continue;
      A->strips.push_back({i,j,Unit(ni+nj)});
    }
  }
  A->IndexCreases();
  return A;
}
inline MetricComposition CompositionA(std::shared_ptr<CBoundaryLayerMetric> base,std::shared_ptr<CBoundaryLayerMetric> full,double core,std::shared_ptr<OptionA> A) {
  return [base,full,core,A](Point p,Tensor sensor){return A->Compose(*base,*full,core,p,sensor,nullptr,0);};
}
inline MetricEdgeBreakpoints EdgeGeometryA(std::shared_ptr<CBoundaryLayerMetric> base,std::shared_ptr<CBoundaryLayerMetric> full,double core,std::shared_ptr<OptionA> A) {
  return [base,full,core,A](Point a,Point b) {
    auto owners=std::make_shared<CBoundaryLayerMetric::ChordOwners>();
    SU2Native3D::EdgeGeometry geometry(full->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z},1.5,16,owners.get()));
    geometry.pinned=[base,full,core,A,owners](long double t,Point p,Tensor sensor){return A->Compose(*base,*full,core,p,sensor,owners.get(),t);};
    return geometry;
  };
}

}  // namespace dc10
namespace SU2NativeBLPipeline3D {
using namespace SU2Native3D;
/*! One immutable per-event target: full layers (production D1), base layers (no D1 feature), option A blending them
 *  (D1 kept off the wall), the frozen sensor core. The same object gives the composition of the remesher's
 *  reassessment and of the construction's frozen field. */
struct TargetPolicy {
  std::shared_ptr<CBoundaryLayerMetric> full, base;
  std::shared_ptr<dc10::OptionA> A;
  double core = 0, h0 = 0, outer_parameter = -1;
  std::map<int, int> roles;  // marker -> 1 BL wall, 0 reflective continuation (MARKER_SYM), 2 other physical partner
  MetricComposition Composition() const { return dc10::CompositionA(base, full, core, A); }
  MetricEdgeBreakpoints Edges() const { return dc10::EdgeGeometryA(base, full, core, A); }
  size_t StorageBytes() const {
    size_t bytes=transfer_memory::Add(sizeof(*this),full->StorageBytes(),base->StorageBytes(),sizeof(*A),
        transfer_memory::Bytes(A->creases),transfer_memory::Bytes(A->strips),transfer_memory::Bytes(A->chains),transfer_memory::Bytes(A->chainOf));
    for(const auto& c:A->chains)bytes=transfer_memory::Add(bytes,transfer_memory::Bytes(c.segments));
    for(const auto& c:A->atPoint)bytes=transfer_memory::Add(bytes,sizeof(c)+4*sizeof(void*),transfer_memory::Bytes(c.second));
    bytes=transfer_memory::Add(bytes,transfer_memory::Mul(A->grid.bucket_count(),sizeof(void*)));
    for(const auto& c:A->grid)bytes=transfer_memory::Add(bytes,sizeof(c)+2*sizeof(void*),transfer_memory::Bytes(c.second));
    return bytes;
  }
  void Update(double trialCore,double hgrad) {
    const auto& profile=full->GetOuterNormal();
    if(profile.version) {
      if(!base->GetOuterNormal().version)base->ConfigureOuterNormal(profile.protected_distance,profile.maximum_growth);
      if(base->GetOuterNormal().parameter!=profile.parameter)base->SelectOuterNormal(profile.parameter);
      if(profile.frozen)base->FreezeOuterNormal();
      outer_parameter=profile.parameter;
    }
    full->ConfigureOuterTransition(trialCore,hgrad);base->ConfigureOuterTransition(trialCore,hgrad);
    core=trialCore;A->core=trialCore;
  }
  static Tensor Pack(const CBoundaryLayerMetric::Tensor& m) {
    return {SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
            SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  }
  static void Unpack(Tensor result,CBoundaryLayerMetric::Tensor& m) {
    m.m[0][0]=result.xx;m.m[0][1]=m.m[1][0]=result.xy;m.m[0][2]=m.m[2][0]=result.xz;
    m.m[1][1]=result.yy;m.m[1][2]=m.m[2][1]=result.yz;m.m[2][2]=result.zz;
  }
  void ApplyPoint(Point p,CBoundaryLayerMetric::Tensor& m) const {
    Unpack(A->Compose(*base,*full,core,p,Pack(m),nullptr,0),m);
  }
  void ApplySamples(const std::vector<CBoundaryLayerMetric::PointSample>& fullSamples,
                    const std::vector<CBoundaryLayerMetric::PointSample>& baseSamples,long double weight,
                    CBoundaryLayerMetric::Tensor& m) const {
    const auto sensor=Pack(m);auto b=m;
    for(const auto& g:baseSamples)base->ApplySample(g.wall,g.sample,b,core,nullptr,false);
    if(!(weight>0)){m=b;return;}
    for(const auto& g:fullSamples)full->ApplySample(g.wall,g.sample,m,core,nullptr,false);
    if(weight<1)Unpack(dc10::OptionA::Blend(Pack(b),Pack(m),weight,sensor),m);
  }
};

/*! Marker roles from the configuration kinds only (no marker names): BL markers 1, symmetry planes 0, others 2. */
inline std::map<int, int> MarkerRoles(const CConfig& config, const ReferenceState& state) {
  std::map<int, int> roles;
  for (size_t m = 0; m < state.marker_names.size(); ++m) {
    const auto& name = state.marker_names[m];
    bool bl = false;
    for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) bl = bl || config.GetAdap_BL(b).marker == name;
    roles[int(m)] = bl ? 1 : config.GetMarker_CfgFile_KindBC(name) == SYMMETRY_PLANE ? 0 : 2;
  }
  return roles;
}

/*! outerParameter < 0: no budget outer-normal profile (ADAP_BL_BUDGET= NO); otherwise the selected frozen parameter. */
inline std::shared_ptr<TargetPolicy> MakeTargetPolicy(const CConfig& config, const ReferenceState& state, double core,
                                                      double outerParameter, std::shared_ptr<CBoundaryLayerMetric> retained = {}) {
  auto p = std::make_shared<TargetPolicy>();
  p->core = core;
  p->outer_parameter = outerParameter;
  p->full = retained ? retained : CBoundaryLayerMetric::FromNativeReference(state, config);
  p->base = CBoundaryLayerMetric::FromNativeReference(state, config, 179.999);
  if(config.GetAdap_BL_Budget() && !retained) {
    if(!(outerParameter>=0))throw std::invalid_argument("FIRST_ROW with ADAP_BL_BUDGET needs the selected outer-normal parameter.");
    p->full->ConfigureOuterNormal(SU2_TYPE::GetValue(config.GetAdap_BL_ProtectDistance()),
                                  SU2_TYPE::GetValue(config.GetAdap_BL_OuterGrowthMax()));
    p->full->SelectOuterNormal(outerParameter);p->full->FreezeOuterNormal();
  }
  if (p->base->GetFeatures().segments != 0) throw std::runtime_error("FIRST_ROW base layers still carry a D1 feature.");
  p->roles = MarkerRoles(config, state);
  p->h0 = std::numeric_limits<double>::infinity();
  for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) p->h0 = std::min(p->h0, SU2_TYPE::GetValue(config.GetAdap_BL(b).firstHeight));
  p->A = dc10::BuildOptionA(state.facets, p->roles, p->full->GetFeatures().angle, p->full->GetFeatures().maximumRadius, p->h0);
  p->A->distanceLayers = p->base;
  p->Update(core,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
  return p;
}

}
