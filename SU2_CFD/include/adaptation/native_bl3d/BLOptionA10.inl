/*!
 * \file BLOptionA10.inl
 * \brief BL option-A target (D1 kept off the wall), wall resolve W, crease fans and Rows10 (ex DC-10).
 * Production since DC-14 (moved from the TEST-ONLY fixture UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc10.inc;
 *        the stage namespaces keep their historical names; fixture loaders and TEST_CASEs stay in the fixture).
 *        Serial, not reentrant: enter through SU2NativeBLPipeline3D::PipelineScope. Every function/variable is
 *        inline (shared by the production translation unit and the harness translation unit).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
// DC-10: (history: formerly test-only) [NativeFirstRowDC10] harness (CREASE_FAN_PLAN). It reuses the DC-9 harness (namespace dc9: caches,
// lazy excess certificate, coordinate length memo, ApexLP/ConeColumn, R1, ApexMove, ColumnSplit incl. the junction
// variant, final gates and export) and adds, as one connected step:
//  (A) the option-A target (user decision 2026-10-23): the D1 crease law is kept off the wall. The composed target is
//      the production BL composition without D1 (no detected feature) blended in logarithm towards the production composition
//      with D1 by a weight w(x) that is 1 inside the convex fluid wedge of a crease (the directions whose closest wall
//      point is the crease) and 0 on and above the adjacent wall faces (smoothstep in the angle from the wedge's
//      boundary planes, radial fade at the crease line). One definition for W, R, V, the final gates and the fresh
//      re-audit. Thin strips between two parallel convex creases bounding one plane face (a blunt TE base) get >= 4
//      wall cells across in W (a construction rule with its own census, not a metric term). Geometric only: creases are detected from the
//      authority facets with the production rule (ADAP_ANGLE, convexity), as in dc9::DetectCreases.
//  (W) plain wall resolve to that target (metric bisection of long wall edges); no crease stations, level lines or
//      needle strips.
//  (R) the first row with per-side lifts and an explicit crease fan (SU2_DC10_LIFT=fan), or the DC-9 single-direction
//      lift (SU2_DC10_LIFT=single, the diagnostic of CREASE_FAN_PLAN step 1).
// Final gates are unchanged and absolute. (DC-14: production.)
namespace dc10 {
using namespace dc9;

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
    std::vector<std::array<GridKey,2>> boxes;size_t buckets=0;
    for(const auto& c:creases){V lo{std::min(c.a.x,c.b.x)-radius,std::min(c.a.y,c.b.y)-radius,std::min(c.a.z,c.b.z)-radius};
      V hi{std::max(c.a.x,c.b.x)+radius,std::max(c.a.y,c.b.y)+radius,std::max(c.a.z,c.b.z)+radius};
      cell=radius;boxes.push_back({KeyAt(lo),KeyAt(hi)});const auto& b=boxes.back();
      buckets+=size_t(b[1][0]-b[0][0]+1)*size_t(b[1][1]-b[0][1]+1)*size_t(b[1][2]-b[0][2]+1);}
    if(buckets>(size_t(1)<<24)){grid.clear();cell=0;return;}  // bounded index; otherwise the exact full loop
    for(size_t i=0;i<creases.size();++i){const auto& b=boxes[i];
      for(long long u=b[0][0];u<=b[1][0];++u)for(long long v=b[0][1];v<=b[1][1];++v)for(long long w=b[0][2];w<=b[1][2];++w)grid[{u,v,w}].push_back(i);}
  }
  long double Weight(V x) const {
    const long double pi=std::acos(-1.L),c0=std::cos(pi/8);
    std::vector<std::pair<long double,long double>> cand;  // (radial factor, r_c)
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
    const auto lb=Function3(FromTensor(mb),LogL),lf=Function3(FromTensor(mf),LogL);M3 l;
    for(int a=0;a<3;++a)for(int b=0;b<3;++b)l[a][b]=(1-w)*lb[a][b]+w*lf[a][b];
    return ToTensor(Function3(l,ExpL));
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
  const long double pi=std::acos(-1.L);
  for(const auto& row:edges) {
    if(row.second.size()!=2)continue;const Facet& f1=*row.second[0];const Facet& f2=*row.second[1];
    unsigned k=0;while(k<3&&!(std::minmax(f1.v[k].id,f1.v[(k+1)%3].id)==std::minmax(row.first.first,row.first.second)))++k;if(k==3)continue;
    const Point a=f1.v[k].p,b=f1.v[(k+1)%3].p,o1=f1.v[(k+2)%3].p;Point o2{};for(auto x:f2.v)if(x.id!=row.first.first&&x.id!=row.first.second)o2=x.p;
    if(!(Orientation(a,b,o1,o2)>0&&Orientation(f2.v[0].p,f2.v[1].p,f2.v[2].p,o1)>0))continue;
    WedgeCrease c;c.a=Of(a);c.b=Of(b);c.L=Norm(c.b-c.a);c.t=(1/c.L)*(c.b-c.a);c.f1=f1.id;c.f2=f2.id;
    c.n1=-1.L*Outward(Of(f1.v[0].p),Of(f1.v[1].p),Of(f1.v[2].p));c.n2=-1.L*Outward(Of(f2.v[0].p),Of(f2.v[1].p),Of(f2.v[2].p));
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
inline std::shared_ptr<OptionA> optionA;  // of the current case (diagnostics, fan wedge tests)
// Loader role metadata (DC-12 rim by role): boundary markers of the last loaded case that the configuration does not
// declare, i.e. the artificial cut boundary (empty for the tube/annulus fixtures, which have none).
inline std::set<int> loadedArtificial;
inline std::set<std::string> reflectiveMarkers;inline bool reflectiveByRole=false;  // DC-12 explicit carrier roles (tube fixtures)
inline std::shared_ptr<CBoundaryLayerMetric> probeBase,probeFull;  // diagnostics only (SU2_DC10_PROBE)

// Option-A self-check on the loaded case (diagnostic, recorded in the JSON): at the input mesh vertices and edge
// midpoints, w = 0 composes exactly the no-D1 target and w = 1 exactly the production D1 target; counts of points
// in the wedges and in strips.
inline std::string OptionACheck(State& s,std::shared_ptr<CBoundaryLayerMetric> base) {
  size_t points=0,wedge=0,full=0,strip=0;
  for(const auto& row:s.mesh)for(auto n:row.second.cell.v){++points;const auto w=optionA->Weight(Of(n.p));wedge+=w>0;full+=w>=1;
    bool in=false;for(const auto& p:optionA->strips){long double width=0;V e;in=in||optionA->InStrip(p,Of(n.p),width,e);}strip+=in;}
  size_t ends=0,corners=0;for(const auto& p:optionA->atPoint){ends+=p.second.size()==1;corners+=p.second.size()>=3;}
  std::ostringstream o;o<<"{\"creases\":"<<optionA->creases.size()<<",\"chains\":[";
  for(size_t c=0;c<optionA->chains.size();++c)o<<(c?",":"")<<"{\"segments\":"<<optionA->chains[c].segments.size()<<",\"median_turn_deg\":"<<double(optionA->chains[c].median*180/std::acos(-1.L))<<",\"k\":"<<optionA->chains[c].k<<'}';
  o<<"],\"crease_points_degree1\":"<<ends<<",\"crease_points_degree3plus\":"<<corners<<",\"strip_pairs\":"<<optionA->strips.size()<<",\"radius\":"<<double(optionA->radius)
    <<",\"vertex_samples\":"<<points<<",\"in_wedge\":"<<wedge<<",\"full_weight\":"<<full<<",\"in_strip\":"<<strip<<'}';(void)base;return o.str();
}

// ---- (W) plain wall resolve to the option-A target --------------------------------------------------------------
struct W10 : Stage { size_t splits=0,unmeasurable=0,strip_splits=0,strip_faces=0;double strip_cells_min=-1; };
// Split one wall edge (production SplitEdge, else InsertEdgeCavity) at the given points in order; D.2 invariant.
inline bool SplitWallEdge(State& s,Edge e,const std::vector<Point>& points,W10& g,const char* tag) {
  const auto star=Star(s,e);if(star.empty())return false;++g.work;
  if(star.size()>MaximumCavityCells){++g.rejected;++g.reasons["edge star exceeds 64"];return false;}
  Patch old;try{old=PatchOf(s,star);}catch(const std::exception& x){++g.rejected;++g.reasons[Key(x.what())];return false;}
  std::string why;
  for(int op=0;op<2;++op)for(const auto& p:points){Patch fresh;bool built=false;
    try{Scope sc(op==0?"W SplitEdge":"W InsertEdgeCavity");built=op==0?SplitEdge(old,e,{s.next_node,p},s.next_cell,*s.reference,fresh,why):InsertEdgeCavity(old,e,{s.next_node,p},s.next_cell,*s.reference,fresh,why);}
    catch(const std::exception& x){why=x.what();built=false;}
    if(!built)continue;{std::string d2;if(!D2Ok(s,old,fresh,d2)){why=d2;continue;}}
    if(!FreshValid(s,fresh.cells)){why="validity";continue;}
    Apply(s,star,fresh);++g.accepted;++g.reasons[std::string(tag)+(op?" accepted InsertEdgeCavity":" accepted SplitEdge")];return true;}
  ++g.rejected;++g.reasons[std::string(tag)+" "+Key(why)];g.last=why;return false;
}
inline Point Mid(Point a,Point b){return {double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};}
// Wall faces on a thin strip: (face, width, unit across direction).
inline std::vector<std::tuple<Face,long double,V>> StripFaces(State& s) {
  std::vector<std::tuple<Face,long double,V>> out;if(optionA->strips.empty())return out;
  for(const auto& wf:WallFaces(s)){V c{};for(Id x:wf.second)c=c+(1.L/3)*Of(NodePoint(s,x));
    for(const auto& p:optionA->strips){long double width=0;V e;if(optionA->InStrip(p,c,width,e)){out.emplace_back(wf.second,width,e);break;}}}
  return out;
}
inline W10 PhaseW10(State& s,size_t work,double seconds,std::ostream& log) {
  Scope scope("PhaseW");
  W10 g;Begin(s,g);const auto t0=Clock::now();
  for(unsigned sweep=0;;++sweep) {
    std::vector<std::pair<long double,Edge>> longs;std::set<Edge> seen;g.unmeasurable=0;
    for(const auto& wf:WallFaces(s))for(int i=0;i<3;++i){Edge e{std::min(wf.second[i],wf.second[(i+1)%3]),std::max(wf.second[i],wf.second[(i+1)%3])};
      if(!seen.insert(e).second)continue;long double l=0;try{l=Len(s,e[0],e[1]);}catch(const std::exception& x){++g.reasons["length: "+Key(x.what())];++g.unmeasurable;
        if(g.unmeasurable<=8){const V A=Of(NodePoint(s,e[0])),B=Of(NodePoint(s,e[1]));log<<std::setprecision(17)<<"DC10 W unmeasurable "<<e[0]<<"-"<<e[1]<<" w(a)="<<double(optionA->Weight(A))<<" w(mid)="<<double(optionA->Weight(.5L*(A+B)))
          <<" w(b)="<<double(optionA->Weight(B))<<" crease="<<OnCrease(A,B)<<" : "<<x.what()<<"\n";}
        continue;}
      if(l>MaximumMetricEdge)longs.emplace_back(-l,e);}
    log<<"DC10 W sweep "<<sweep<<" wall_faces="<<WallFaces(s).size()<<" long_wall_edges="<<longs.size()<<" work="<<g.work<<"\n";
    if(longs.empty()){g.outcome=g.unmeasurable?"unmeasurable wall edges: "+std::to_string(g.unmeasurable):"complete";break;}
    std::sort(longs.begin(),longs.end());bool changed=false;
    for(const auto& row:longs) {
      if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
      const auto e=row.second;if(Star(s,e).empty())continue;
      const Point a=NodePoint(s,e[0]),b=NodePoint(s,e[1]);std::vector<Point> points;
      try{points.push_back(s.field->SegmentPoint(a,b,.5));}catch(const std::exception&){}
      points.push_back(Mid(a,b));
      if(SplitWallEdge(s,e,points,g,"metric")){++g.splits;changed=true;}
    }
    if(!g.outcome.empty())break;
    if(!changed){g.outcome="blocked";break;}
  }
  // thin-strip rule (user decision: >= 4 wall cells across a blunt TE base): split strip-face edges whose across-strip
  // extent exceeds width/4 (5% slack), largest first; same W limits and the D.2 invariant.
  for(unsigned sweep=0;sweep<32;++sweep) {
    std::vector<std::pair<long double,Edge>> cand;std::set<Edge> seen;
    for(const auto& sf:StripFaces(s)){const Face& f=std::get<0>(sf);const auto width=std::get<1>(sf);const V e=std::get<2>(sf);
      for(int i=0;i<3;++i){const Edge ed{std::min(f[i],f[(i+1)%3]),std::max(f[i],f[(i+1)%3])};const auto across=std::abs(Dot(Of(NodePoint(s,ed[1]))-Of(NodePoint(s,ed[0])),e));
        if(across>1.05L*width/4&&seen.insert(ed).second)cand.emplace_back(-across/width,ed);}}
    if(cand.empty())break;std::sort(cand.begin(),cand.end());bool changed=false;
    for(const auto& row:cand){if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
      const Edge e=row.second;if(Star(s,e).empty())continue;if(SplitWallEdge(s,e,{Mid(NodePoint(s,e[0]),NodePoint(s,e[1]))},g,"strip")){++g.strip_splits;changed=true;}}
    if(g.outcome=="limit"||!changed)break;
  }
  {const auto faces=StripFaces(s);g.strip_faces=faces.size();
   for(const auto& sf:faces){const Face& f=std::get<0>(sf);long double worst=0;for(int i=0;i<3;++i)worst=std::max(worst,std::abs(Dot(Of(NodePoint(s,f[(i+1)%3]))-Of(NodePoint(s,f[i])),std::get<2>(sf))));
     const double cells=double(std::get<1>(sf)/worst);if(g.strip_cells_min<0||cells<g.strip_cells_min)g.strip_cells_min=cells;}}
  End(s,g,t0);return g;
}

// ---- (R) per-side lifts and an explicit crease fan (CREASE_FAN_PLAN §3-§6) --------------------------------------------
// A ridge vertex (two wall sectors separated by crease edges of one chain; the symmetry root is one) gets k+2 copies:
// side copies along the most-normal direction of each sector and k fan copies by slerp, all at wall distance h0.
// Moved cells take one copy (slot) each; interior faces whose images differ get gap tetrahedra; every crease edge
// incident to the lift set gets its fan prisms. Single vertices are lifted exactly as in DC-9.
struct Copies { std::vector<Id> ids;std::vector<V> dirs;size_t chain=SIZE_MAX;V t,e1,e2;long double turn=0; };
struct Rows10 : Rows { std::map<Id,Copies> copies;std::set<std::array<Id,4>> fan; };
inline std::array<Id,4> CellKey(const Cell& c){std::array<Id,4> k{c.v[0].id,c.v[1].id,c.v[2].id,c.v[3].id};std::sort(k.begin(),k.end());return k;}
// A current wall edge on an option-A crease segment (dc9::OnCrease tolerance); its chain.
inline bool CreaseOf(const State& s,Id a,Id b,size_t& chain) {
  const V A=Of(NodePoint(s,a)),B=Of(NodePoint(s,b));const long double tol=1e-9L*d1.scale;
  for(size_t i=0;i<optionA->creases.size();++i){const auto& c=optionA->creases[i];
    auto on=[&](V x){const auto sp=Dot(x-c.a,c.t);return sp>=-tol&&sp<=c.L+tol&&Norm(x-(c.a+sp*c.t))<=tol;};
    if(on(A)&&on(B)){chain=optionA->chainOf[i];return true;}}
  return false;
}
struct VertexClass { int kind=0;std::string why;size_t chain=SIZE_MAX;std::vector<std::vector<WF>> sectors;std::vector<Id> creaseNbr; };
// Sectors: incident wall faces joined across non-crease wall edges; a crease edge separates only when two wall faces
// share it. kind 0 single (one sector), 1 ridge (two sectors, one chain), 2 corner (witness; not exercised, §3).
inline VertexClass ClassifyVertex(const State& s,Id v) {
  VertexClass r;std::vector<WF> faces;for(const auto& f:PhysicalOf(s,StarOf(s,v),true))if(Has(f.key,v))faces.push_back(f);
  std::vector<size_t> parent(faces.size());for(size_t i=0;i<faces.size();++i)parent[i]=i;
  std::function<size_t(size_t)> root=[&](size_t a){while(parent[a]!=a)a=parent[a]=parent[parent[a]];return a;};
  std::map<Id,std::vector<size_t>> byNbr;for(size_t i=0;i<faces.size();++i)for(Id x:faces[i].key)if(x!=v)byNbr[x].push_back(i);
  std::set<size_t> chains;
  for(const auto& e:byNbr){size_t ch=SIZE_MAX;if(e.second.size()==2&&CreaseOf(s,v,e.first,ch)){chains.insert(ch);r.creaseNbr.push_back(e.first);continue;}
    for(size_t k=1;k<e.second.size();++k)parent[root(e.second[k])]=root(e.second[0]);}
  std::map<size_t,std::vector<WF>> comps;for(size_t i=0;i<faces.size();++i)comps[root(i)].push_back(faces[i]);
  for(auto& c:comps)r.sectors.push_back(std::move(c.second));
  if(r.sectors.size()<=1){r.kind=0;return r;}
  if(r.sectors.size()==2&&chains.size()==1){r.kind=1;r.chain=*chains.begin();return r;}
  r.kind=2;r.why="corner: "+std::to_string(r.sectors.size())+" sectors, "+std::to_string(chains.size())+" chains";return r;
}
inline long double PointTriangleDistance(V p,V a,V b,V c) {
  const V n=Cross(b-a,c-a);const auto nn=Norm(n);
  if(nn>0){const V u=(1/nn)*n;const V q=p-Dot(p-a,u)*u;
    auto inside=[&](V x,V y){return Dot(Cross(y-x,q-x),n)>=0;};if(inside(a,b)&&inside(b,c)&&inside(c,a))return std::abs(Dot(p-a,u));}
  auto seg=[&](V x,V y){const auto l=Dot(y-x,y-x);const auto t=l>0?std::clamp(Dot(p-x,y-x)/l,0.L,1.L):0.L;return Norm(p-(x+t*(y-x)));};
  return std::min({seg(a,b),seg(b,c),seg(c,a)});
}
inline V Slerp(V a,V b,long double t) {
  const auto c=std::clamp(Dot(a,b),-1.L,1.L),w=std::acos(c);if(!(w>1e-12L))return Unit(a);
  return Unit((std::sin((1-t)*w)/std::sin(w))*a+(std::sin(t*w)/std::sin(w))*b);
}
struct RidgePlan { bool ok=false;std::string cls,why;Copies c;std::vector<Node> nodes;std::vector<long double> wallDistance;Dir dir; };
// Ray length with BL wall distance = target (all walls, exact nearest face); -1 when not reached within 4 target.
inline long double RayToWallDistance(V x0,V d,long double target) {
  // [rev 3] first reachable root: scan lambda = .05, .1, ... 4 target for the first distance >= target, then bisect
  // that bracket (the all-wall distance need not grow monotonically along the ray)
  long double lo=0,hi=-1;
  for(int i=1;i<=80;++i){const long double l=i*.05L*target;if(optionA->WallDistance(x0+l*d)>=target){hi=l;break;}lo=l;}
  if(hi<0)return -1;
  for(unsigned it=0;it<48;++it){const auto mid=(lo+hi)/2;(optionA->WallDistance(x0+mid*d)<target?lo:hi)=mid;}
  return (lo+hi)/2;
}
// Copies of a ridge vertex (§4): side directions = MinNormAny of each sector's normals (projected into a rank-1 carrier),
// side 0 = the sector closer to the chain's side-1 normal, fan directions by slerp, every copy at wall distance h0
// (side copies at h0 / min n.d), bound to the vertex's carriers.
// Side directions of a ridge vertex: MinNormAny of each sector's normals projected into a rank-1 carrier; side 0 is the
// sector closer to the chain's side-1 normal. Shared by R1w and PlanRidge.
struct Sides { bool ok=false;std::string cls,why;std::array<V,2> d;std::array<long double,2> slack{};const WedgeCrease* seg=nullptr; };
inline Sides RidgeSides(State& s,Id v,const VertexClass& vc,const Dir& dir) {
  Sides out;if(dir.rank>=2){out.cls="corner";out.why="ridge on a carrier corner (rank "+std::to_string(dir.rank)+")";return out;}
  std::vector<V> basis;
  for(const auto& f:PhysicalOf(s,StarOf(s,v),false))if(Has(f.key,v)){V r=Outward(Of(NodePoint(s,f.out[0])),Of(NodePoint(s,f.out[1])),Of(NodePoint(s,f.out[2])));
    for(const auto& b:basis)r=r-Dot(r,b)*b;if(Norm(r)>1e-9L)basis.push_back(Unit(r));}
  const V x0=Of(NodePoint(s,v));
  for(int k=0;k<2;++k){std::vector<V> proj,normals;
    for(const auto& f:vc.sectors[k]){const V n=-1.L*Outward(Of(NodePoint(s,f.out[0])),Of(NodePoint(s,f.out[1])),Of(NodePoint(s,f.out[2])));normals.push_back(n);
      V p=n;for(const auto& b:basis)p=p-Dot(p,b)*b;if(Norm(p)<1e-12L){out.cls="carrier";out.why="sector normal parallel to the carrier";return out;}proj.push_back(p);}
    if(!MinNormAny(proj,out.d[k])){out.cls="hemisphere";out.why="sector without an open hemisphere";return out;}
    out.slack[k]=INFINITY;for(const auto& n:normals)out.slack[k]=std::min(out.slack[k],Dot(n,out.d[k]));
    if(!(out.slack[k]>.1L)){out.cls="hemisphere";out.why="sector direction slack "+std::to_string(double(out.slack[k]));return out;}}
  const auto& chain=optionA->chains.at(vc.chain);size_t best=SIZE_MAX;long double bestD=INFINITY;
  for(size_t i:chain.segments){const auto& c=optionA->creases[i];const auto sp=std::clamp(Dot(x0-c.a,c.t),0.L,c.L);const auto d=Norm(x0-(c.a+sp*c.t));if(d<bestD){bestD=d;best=i;}}
  out.seg=&optionA->creases.at(best);
  if(Dot(out.d[1],out.seg->n1)>Dot(out.d[0],out.seg->n1)){std::swap(out.d[0],out.d[1]);std::swap(out.slack[0],out.slack[1]);}
  out.ok=true;return out;
}
inline RidgePlan PlanRidge(State& s,const PlanarReference* priv,Id v,const VertexClass& vc,Id& next) {
  RidgePlan rp;rp.dir=DirectionAt(s,v);
  const auto sides=RidgeSides(s,v,vc,rp.dir);if(!sides.ok){rp.cls=sides.cls;rp.why=sides.why;return rp;}
  const V x0=Of(NodePoint(s,v));const auto side=sides.d;const auto& seg=*sides.seg;const auto& chain=optionA->chains.at(vc.chain);
  const int k=chain.k;rp.c.chain=vc.chain;
  {V t{};for(Id w:vc.creaseNbr){V e=Unit(Of(NodePoint(s,w))-x0);if(Dot(e,seg.t)<0)e=-1.L*e;t=t+e;}rp.c.t=Unit(t);}
  const V& t=rp.c.t;rp.c.e1=Unit(side[0]-Dot(side[0],t)*t);const V a1=Unit(side[1]-Dot(side[1],t)*t);rp.c.e2=Unit(a1-Dot(a1,rp.c.e1)*rp.c.e1);
  rp.c.turn=std::atan2(Dot(a1,rp.c.e2),Dot(a1,rp.c.e1));if(!(rp.c.turn>0)){rp.cls="ridge";rp.why="side directions do not turn";return rp;}
  for(int j=0;j<=k+1;++j) {
    const V d=j==0?side[0]:j==k+1?side[1]:Slerp(side[0],side[1],(long double)j/(k+1));
    const long double dist=RayToWallDistance(x0,d,s.h0);
    if(!(dist>0)){rp.cls="fan-height";rp.why="copy "+std::to_string(j)+": no ray length with wall distance h0";return rp;}
    Node node{next++,Pt(x0+dist*d)};
    try{node=Constrain(s,priv,node,rp.dir.carriers,rp.dir.bind);}catch(const std::exception& e){rp.cls="carrier";rp.why=std::string("copy binding: ")+e.what();return rp;}
    // fan rule: every copy within 10% of h0 from the wall (BL wall distance after binding, all walls)
    const long double wd=optionA->WallDistance(Of(node.p));
    if(std::abs(wd/s.h0-1)>HeightTolerance){rp.cls="fan-height";std::ostringstream w;w<<"copy "<<j<<" wall distance/h0="<<double(wd/s.h0);rp.why=w.str();return rp;}
    rp.c.ids.push_back(node.id);rp.c.dirs.push_back(d);rp.nodes.push_back(node);rp.wallDistance.push_back(wd);
  }
  rp.ok=true;return rp;
}
// Slot of a direction about the crease at a ridge copy set: nearest copy angle in the plane orthogonal to t.
inline size_t GeometricSlot(const Copies& c,V x0,V centroid) {
  V d=centroid-x0;d=d-Dot(d,c.t)*c.t;const auto phi=std::atan2(Dot(d,c.e2),Dot(d,c.e1));const long double K=c.ids.size()-1;
  return size_t(std::clamp(std::llround(phi/(c.turn/K)),0LL,(long long)K));
}
// Strict changed-edge excess decrease (D.1 Refine size progress), integrating every changed edge.
inline bool StrictExcessDecrease(State& s,const Patch& old,const Patch& fresh,long double& removed,long double& added) {
  auto edges=[](const std::vector<Cell>& cells){std::map<Edge,std::array<Point,2>> out;for(const auto& c:cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){Node x=c.v[i],y=c.v[j];if(x.id>y.id)std::swap(x,y);out[{x.id,y.id}]={x.p,y.p};}return out;};
  const auto before=edges(old.cells),after=edges(fresh.cells);removed=added=0;
  auto same=[](const std::map<Edge,std::array<Point,2>>& m,const std::pair<const Edge,std::array<Point,2>>& e){const auto o=m.find(e.first);return o!=m.end()&&SamePoint(o->second[0],e.second[0])&&SamePoint(o->second[1],e.second[1]);};
  for(const auto& e:after)if(!same(before,e)){try{added+=layer2d::ExcessOf(LenAt(s,e.first[0],e.second[0],e.first[1],e.second[1]));}catch(const std::exception&){return false;}}
  for(const auto& e:before)if(!same(after,e)){try{removed+=layer2d::ExcessOf(LenAt(s,e.first[0],e.second[0],e.first[1],e.second[1]));}catch(const std::exception&){removed+=1e30L;}if(removed>added)break;}
  return added<removed;
}
// [rev 2] R1w (CREASE_FAN_PLAN §6): wedge conditioning before R2. For each unlifted ridge vertex and direction d of
// {d_0, d_mid, d_{k+1}} whose 22.5-degree cone holds no link vertex within metric length 1.8, a Steiner point at wall
// distance 2.5 h0 along d (DC-7 visibility cavity + Cone). Admission: positivity, D.2, strict excess decrease, caps.
struct R1wStage : Stage { size_t ridges=0,inserted=0,present=0; };
inline R1wStage PhaseR1w(State& s,const Rows10& rows,size_t work,double seconds,std::ostream& log) {
  Scope scope("PhaseR1w");
  R1wStage g;Begin(s,g);const auto t0=Clock::now();const long double cone=std::cos(std::acos(-1.L)/8);
  for(Id v:WallVertices(s)) {
    if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
    if(Rim(s,v)||rows.lift.count(v))continue;const auto vc=ClassifyVertex(s,v);if(vc.kind!=1)continue;++g.ridges;
    const auto sides=RidgeSides(s,v,vc,DirectionAt(s,v));if(!sides.ok){++g.reasons["sides: "+sides.cls];continue;}
    const V x0=Of(NodePoint(s,v));
    for(const V& d:{sides.d[0],Slerp(sides.d[0],sides.d[1],.5L),sides.d[1]}) {
      if(g.work>=work||Seconds(t0)>=seconds){g.outcome="limit";break;}
      bool near=false;long double coneShortest=INFINITY;  // DC-11: the split request of this direction
      for(Id id:StarOf(s,v)){for(auto n:s.mesh.at(id).cell.v){if(n.id==v)continue;const V e=Of(n.p)-x0;if(!(Norm(e)>0)||Dot(Unit(e),d)<cone)continue;
          long double l=INFINITY;try{l=LenAt(s,v,NodePoint(s,v),n.id,n.p);}catch(const std::exception&){}coneShortest=std::min(coneShortest,l);if(l<=MaximumMetricEdge){near=true;break;}}if(near)break;}
      if(near){++g.present;continue;}
      const auto lambda=RayToWallDistance(x0,d,2.5L*s.h0);if(!(lambda>0)){++g.reasons["no ray length at 2.5 h0"];continue;}
      const Point p=Pt(x0+lambda*d);++g.work;
      const auto vis=dc7::Visible(s,p,ColumnCells(s,rows));if(!vis.ok){++g.rejected;++g.reasons["visible: "+vis.why];continue;}
      Patch old,fresh;std::string why;
      try{Scope sc("R1w Steiner");old=PatchOf(s,vis.cavity);fresh.surface=old.surface;fresh.parameters=old.parameters;
        if(!Cone(old.cells,{s.next_node,p},s.next_cell,fresh.cells,why)){++g.rejected;++g.reasons["Cone: "+Key(why)];continue;}
        s.reference->ExportParameters(fresh);}catch(const std::exception& e){++g.rejected;++g.reasons[Key(e.what())];continue;}
      if(fresh.cells.size()>MaximumReplacementCells){++g.rejected;++g.reasons["fresh cells exceed 128"];continue;}
      {std::string d2;if(!D2Ok(s,old,fresh,d2)){++g.rejected;++g.reasons["D.2"];continue;}}
      if(!FreshValid(s,fresh.cells)){++g.rejected;++g.reasons["validity"];continue;}
      if(splitPolicy.rule==1){  // DC-11: the full D.1 Refine admission (height, floors, measurement) with the cone request length
        splitPolicy.requested=coneShortest;const auto d=Admit8(s,old,fresh,Kind::Refine);splitPolicy.requested=-1;
        if(!d.ok){++g.rejected;++g.reasons["split rule: "+Key(d.reason)];continue;}}
      else{long double removed=0,added=0;if(!StrictExcessDecrease(s,old,fresh,removed,added)){++g.rejected;++g.reasons["no strict excess decrease"];continue;}}
      Apply(s,vis.cavity,fresh);++g.accepted;++g.inserted;}
  }
  if(g.outcome.empty())g.outcome="complete";End(s,g,t0);
  log<<"DC10 R1w ridges="<<g.ridges<<" inserted="<<g.inserted<<" present="<<g.present<<" rejected="<<g.rejected<<"\n";return g;
}
struct Tx10 : Tx { std::map<Id,Copies> copies;std::set<std::array<Id,4>> fanKeys,droppedFan;size_t fan_cells=0,gap_cells=0,ridges=0;std::map<std::string,size_t> fanChecks; };
// Fan cell admission (§5.8): validity, Euclidean worst dihedral < 174 deg, node-to-opposite-face distance >= h0/2,
// metric q/J floors. Returns the failing check or "".
inline std::string FanCheck(State& s,const Cell& c) {
  std::array<V,4> p;for(int i=0;i<4;++i)p[i]=Of(c.v[i].p);
  if(!Positive(c.v))return "fan: non-positive volume";
  const auto vol=std::abs(Det(p[0],p[1],p[2],p[3]));
  for(int i=0;i<4;++i){std::array<V,3> f;int k=0;for(int j=0;j<4;++j)if(j!=i)f[k++]=p[j];const auto area2=Norm(Cross(f[1]-f[0],f[2]-f[0]));
    if(!(area2>0)||vol/area2<.5L*s.h0)return "fan: node-to-face distance < h0/2";}
  for(int i=0;i<4;++i)for(int j=i+1;j<4;++j){int o[2],k=0;for(int m=0;m<4;++m)if(m!=i&&m!=j)o[k++]=m;
    const V e=Unit(p[j]-p[i]);auto perp=[&](V x){V d=x-p[i];return Unit(d-Dot(d,e)*e);};
    const auto ang=std::acos(std::clamp(Dot(perp(p[o[0]]),perp(p[o[1]])),-1.L,1.L))*180/std::acos(-1.L);if(ang>=174)return "fan: dihedral >= 174 deg";}
  const auto sh=Shape(s,c);if(Bad(sh))return "fan: metric q/J below the floor";
  for(int i=0;i<4;++i)for(int j=i+1;j<4;++j){long double l=INFINITY;try{l=LenAt(s,c.v[i].id,c.v[i].p,c.v[j].id,c.v[j].p);}catch(const std::exception&){return "fan: unmeasurable edge";}
    if(l>MaximumMetricEdge)return "fan: metric edge > 1.8";}
  return "";
}
inline Tx10 Transaction10(State& s,const Rows10& rows,const std::set<Id>& V0,bool low) {
  Scope scope("Transaction10");
  Tx10 tx;const auto apicesAll=rows.Apices();
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
  Id next=s.next_node;std::map<Id,Id> L=rows.lift;std::map<Id,Plan> plans;std::map<Id,VertexClass> classes;
  for(Id v:V0) {
    auto vc=ClassifyVertex(s,v);if(vc.kind==2){tx.cls="corner";tx.witness="vertex "+std::to_string(v)+": "+vc.why;return tx;}
    if(vc.kind==1) {
      auto rp=PlanRidge(s,priv.get(),v,vc,next);if(!rp.ok){tx.cls=rp.cls;tx.witness="vertex "+std::to_string(v)+": "+rp.why;return tx;}
      for(const auto& n:rp.nodes)nodes[n.id]=n;tx.copies[v]=rp.c;L[v]=rp.c.ids.front();tx.lifts[v]=rp.c.ids.front();++tx.ridges;
      for(const auto& n:rp.nodes)if(!rp.dir.carriers.empty())tx.seam.insert(n.id);  // bound to a carrier
      classes.emplace(v,std::move(vc));continue;}
    auto pl=PlanLift(s,apicesAll,v,true,low);if(!pl.ok){tx.cls=pl.why.substr(0,pl.why.find(':'));tx.witness="vertex "+std::to_string(v)+": "+pl.why;return tx;}
    if(!vc.creaseNbr.empty()){  // [rev 2] a fan endpoint (crease end, carrier end) obeys the fan height rule: wall distance h0
      const V x0=Of(NodePoint(s,v));const auto lambda=RayToWallDistance(x0,pl.dir.d,s.h0);
      if(!(lambda>0)){tx.cls="fan-height";tx.witness="vertex "+std::to_string(v)+": crease endpoint has no ray length with wall distance h0";return tx;}
      pl.delta=lambda;pl.p=x0+lambda*pl.dir.d;pl.H=s.h0;}
    Node node{next++,Pt(pl.p)};
    try{node=Constrain(s,priv.get(),node,pl.dir.carriers,pl.dir.bind);}catch(const std::exception& e){tx.cls="carrier";tx.witness="vertex "+std::to_string(v)+" binding: "+e.what();return tx;}
    if(!vc.creaseNbr.empty()){const auto wd=optionA->WallDistance(Of(node.p));if(std::abs(wd/s.h0-1)>HeightTolerance){tx.cls="fan-height";std::ostringstream w;w<<"vertex "<<v<<": crease endpoint wall distance/h0="<<double(wd/s.h0);tx.witness=w.str();return tx;}}
    nodes[node.id]=node;L[v]=node.id;tx.lifts[v]=node.id;plans[v]=pl;if(pl.dir.rank==2)tx.seam.insert(node.id);classes.emplace(v,std::move(vc));}
  tx.seam.clear();  // ridge copies are interior points of their carrier patch (no seam contraction); DC-9 rank-2 seams below
  for(const auto& r:plans)if(r.second.dir.rank==2)tx.seam.insert(L.at(r.first));
  auto Lf=[&](Id x){const auto f=L.find(x);return f==L.end()?x:f->second;};
  auto copiesOf=[&](Id x)->const Copies*{const auto a=tx.copies.find(x);if(a!=tx.copies.end())return &a->second;const auto b=rows.copies.find(x);return b==rows.copies.end()?nullptr:&b->second;};
  std::map<Id,std::pair<Id,size_t>> copyOwner;for(const auto& r:rows.copies)for(size_t j=0;j<r.second.ids.size();++j)copyOwner[r.second.ids[j]]={r.first,j};
  // side copy of a ridge vertex for a wall face (outward key f)
  auto sideOf=[&](const Copies& c,const Face& f)->size_t{const V n=-1.L*Outward(Of(nodes.at(f[0]).p),Of(nodes.at(f[1]).p),Of(nodes.at(f[2]).p));
    return Dot(n,c.dirs.front())>=Dot(n,c.dirs.back())?0:c.ids.size()-1;};
  // (a) DC-9 carrier sector plans for single vertices; not combined with ridges in one transaction
  std::map<Id,SectorPlan> sectors;std::map<Id,Id> unlift;for(const auto& r:tx.lifts)unlift[r.second]=r.first;
  for(const auto& r:tx.copies)for(Id c:r.second.ids)unlift[c]=r.first;for(const auto& r:rows.copies)for(Id c:r.second.ids)unlift[c]=r.first;
  for(Id v:V0) {
    if(tx.copies.count(v))continue;
    auto sp=PlanSectors(s,tx.old,ref,v,nodes.at(L.at(v)),plans.at(v).dir,plans.at(v).H,next,nodes,Lf);
    if(!sp.ok){tx.cls=sp.why.substr(0,sp.why.find(':'));tx.witness="vertex "+std::to_string(v)+": "+sp.why;
      tx.splitEdge=sp.splitEdge;tx.sectorVertex=v;if(sp.corner!=UINT64_MAX){tx.corner=sp.corner;tx.cornerEdge={std::min(sp.cornerOpposite[0],sp.cornerOpposite[1]),std::max(sp.cornerOpposite[0],sp.cornerOpposite[1])};}return tx;}
    if(!sp.needed)continue;if(!tx.copies.empty()){tx.cls="sector";tx.witness="carrier sector lift and ridge copies in one transaction";return tx;}
    ++tx.sectors;for(const auto& p:sp.points){nodes[p.id]=p;unlift[p.id]=v;tx.seam.insert(p.id);}
    sectors.emplace(v,std::move(sp));}
  for(const auto& sp:sectors)for(Id v2:V0)if(v2!=sp.first)for(const auto& g:sp.second.gaps)for(auto n:g)if(n.id==L.at(v2)&&sectors.count(v2)){tx.cls="sector";tx.witness="adjacent multi-sector lifts";return tx;}
  // old cells: drop columns of rebuilt faces and registered fan cells; move the rest
  std::set<Id> oldApices;for(const auto& f:tx.rebuilt){const auto a=rows.apex.find(f);if(a!=rows.apex.end())oldApices.insert(a->second);}
  std::map<Id,const Cell*> oldCell;for(const auto& c:tx.old.cells)oldCell[c.id]=&c;
  std::map<Face,std::vector<Id>> faceCells;for(const auto& c:tx.old.cells)for(const auto& f:CellFaces(c))faceCells[Sorted(f)].push_back(c.id);
  auto isColumn=[&](const Cell& c){for(auto n:c.v)if(oldApices.count(n.id))return true;return false;};
  std::map<Id,Id> columnApex;for(const auto& c:tx.old.cells)for(auto n:c.v)if(oldApices.count(n.id))columnApex[c.id]=n.id;
  std::map<Id,Face> apexToFace;for(const auto& f:tx.rebuilt){const auto a=rows.apex.find(f);if(a!=rows.apex.end())apexToFace[a->second]=f;}
  std::map<Face,Face> outFace;for(const auto& f:PhysicalOf(s,cavity,true))outFace[f.key]=f.out;
  std::vector<Cell> fresh;std::map<Id,std::map<Id,Node>> image;  // moved old cell id -> old vertex id -> image
  std::map<std::pair<Id,Id>,size_t> slotOf;  // (old cell id, ridge vertex) -> slot
  for(const auto& c:tx.old.cells) {
    if(isColumn(c))continue;if(rows.fan.count(CellKey(c))){tx.droppedFan.insert(CellKey(c));continue;}
    // slots of the ridge vertices of V0 in c (§5.3): column/wall constraints, inheritance from a lifted chain
    // neighbour, one slot per cell and chain (lower id frame), geometric angle
    std::vector<Id> ridgeHere;for(auto n:c.v)if(tx.copies.count(n.id))ridgeHere.push_back(n.id);std::sort(ridgeHere.begin(),ridgeHere.end());
    V centroid{};for(auto n:c.v)centroid=centroid+.25L*Of(n.p);
    std::map<size_t,size_t> chainSlot;
    for(Id v:ridgeHere) {
      const auto& C=tx.copies.at(v);std::set<size_t> forced;
      for(const auto& f:CellFaces(c)){const auto key=Sorted(f);if(!Has(key,v))continue;
        const auto wf=outFace.find(key);if(wf!=outFace.end()){bool own=false;for(Id id:faceCells.at(key))own=own||id==c.id;if(own&&faceCells.at(key).size()==1){forced.insert(sideOf(C,wf->second));continue;}}
        for(Id o:faceCells.at(key))if(o!=c.id){const auto ca=columnApex.find(o);if(ca!=columnApex.end()){const auto af=apexToFace.find(ca->second);if(af!=apexToFace.end())forced.insert(sideOf(C,outFace.at(af->second)));}}}
      std::set<size_t> inherited;Id owner=UINT64_MAX;
      for(auto n:c.v){const auto o=copyOwner.find(n.id);if(o!=copyOwner.end()&&rows.copies.at(o->second.first).chain==C.chain){inherited.insert(o->second.second);owner=o->second.first;}}
      size_t slot=SIZE_MAX;
      if(forced.size()>1){tx.cls="slot";tx.witness="cell "+std::to_string(c.id)+" touches columns of both sides at ridge vertex "+std::to_string(v);return tx;}
      if(forced.size()==1)slot=*forced.begin();
      else if(!inherited.empty())slot=v<owner?*inherited.begin():*inherited.rbegin();
      else if(chainSlot.count(C.chain))slot=chainSlot.at(C.chain);
      else slot=GeometricSlot(C,Of(nodes.at(v).p),centroid);
      if(!chainSlot.count(C.chain))chainSlot[C.chain]=slot;
      slotOf[{c.id,v}]=slot;}
    Cell m=c;
    for(auto& n:m.v) {
      const Id x=n.id;
      if(tx.copies.count(x)){n=nodes.at(tx.copies.at(x).ids.at(slotOf.at({c.id,x})));continue;}
      const auto f=tx.lifts.find(x);if(f==tx.lifts.end())continue;const auto sp=sectors.find(x);
      if(sp!=sectors.end()){const auto cl=sp->second.cellLift.find(c.id);if(cl!=sp->second.cellLift.end()){n=cl->second;continue;}}n=nodes.at(f->second);}
    if(!Positive(m.v)){tx.cls="clearance";std::ostringstream w;w<<"moved cell "<<c.id<<" inverts; |V|="<<V0.size()<<(low?" low band":"")<<" moved in cell:";
      for(auto x:c.v)if(tx.copies.count(x.id))w<<" ridge v"<<x.id<<"(slot "<<slotOf.at({c.id,x.id})<<")";else if(tx.lifts.count(x.id)){const auto& pl=plans.at(x.id);w<<" v"<<x.id<<"(delta/deltaK="<<double(pl.delta/pl.kernel.delta)<<",H/h0="<<double(pl.H/s.h0)<<")";}
      const auto wall=WallVertices(s);for(auto x:c.v)if(wall.count(x.id)&&!rows.lift.count(x.id)&&!V0.count(x.id))tx.blockers.insert(x.id);
      tx.witness=w.str();return tx;}
    for(int i=0;i<4;++i)image[c.id][c.v[i].id]=m.v[i];
    fresh.push_back(m);}
  for(const auto& sp:sectors)for(const auto& g:sp.second.gaps){fresh.push_back(Cell{0,g});++tx.gaps;}
  // [rev 2] band check: for every ridge vertex v and slot j, the moved cells around v with slot <= j and those with
  // slot > j are each face-connected through faces containing v (bands from crease side to crease side, no islands)
  for(const auto& r:tx.copies) {
    const Id v=r.first;const size_t K=r.second.ids.size()-1;std::map<Id,size_t> slot;for(const auto& c:slotOf)if(c.first.second==v)slot[c.first.first]=c.second;
    std::map<Id,std::vector<Id>> adj;for(const auto& fc:faceCells)if(fc.second.size()==2&&Has(fc.first,v)&&slot.count(fc.second[0])&&slot.count(fc.second[1])){adj[fc.second[0]].push_back(fc.second[1]);adj[fc.second[1]].push_back(fc.second[0]);}
    for(size_t j=0;j<K;++j)for(int side=0;side<2;++side) {
      std::set<Id> set;for(const auto& c:slot)if((c.second<=j)==(side==0))set.insert(c.first);if(set.empty())continue;
      std::set<Id> seen{*set.begin()};std::deque<Id> q{*set.begin()};
      while(!q.empty()){const Id c=q.front();q.pop_front();for(Id o:adj[c])if(set.count(o)&&seen.insert(o).second)q.push_back(o);}
      if(seen.size()!=set.size()){tx.cls="slot-bands";std::ostringstream w;w<<"ridge vertex "<<v<<": slots "<<(side?">":"<=")<<j<<" form "<<set.size()-seen.size()<<"+ disconnected cells of "<<set.size();tx.witness=w.str();return tx;}}
  }
  // gap cells (§5.4): interior faces of two moved cells whose images differ; ridge vertices advance one slot at a time,
  // interleaved, higher id first
  for(const auto& fc:faceCells) {
    if(fc.second.size()!=2||!image.count(fc.second[0])||!image.count(fc.second[1]))continue;
    const auto& i1=image.at(fc.second[0]);const auto& i2=image.at(fc.second[1]);const Face& key=fc.first;
    bool same=true;for(Id x:key)same=same&&i1.at(x).id==i2.at(x).id;if(same)continue;
    std::map<Id,std::pair<size_t,size_t>> moves;bool sectorFace=false;
    for(Id x:key)if(i1.at(x).id!=i2.at(x).id){if(sectors.count(x)){sectorFace=true;break;}
      if(!tx.copies.count(x)){tx.cls="gap";tx.witness="image change of a non-ridge vertex "+std::to_string(x);return tx;}
      moves[x]={slotOf.at({fc.second[0],x}),slotOf.at({fc.second[1],x})};}
    if(sectorFace)continue;  // DC-9 sector gaps cover it
    // [rev 2] canonical sweep from the lower-slot image to the higher one (independent of traversal order)
    bool up=false,down=false;for(const auto& m:moves){up=up||m.second.first<m.second.second;down=down||m.second.first>m.second.second;}
    if(up&&down){tx.cls="gap";tx.witness="mixed slot directions on one face";return tx;}
    const auto& start=down?i2:i1;if(down)for(auto& m:moves)std::swap(m.second.first,m.second.second);
    std::map<Id,Node> cur;for(Id x:key)cur[x]=start.at(x);
    // [rev 3] one global order of advancement events (Codex review 2): increasing transition slot, then decreasing
    // vertex id; every face, fan prism and column lateral then shares the diagonal x_j - y_{j+1} (x < y)
    std::vector<std::pair<size_t,Id>> events;for(const auto& m:moves)for(size_t j=m.second.first;j<m.second.second;++j)events.emplace_back(j,m.first);
    std::sort(events.begin(),events.end(),[](const auto& a,const auto& b){return a.first!=b.first?a.first<b.first:a.second>b.second;});
    for(const auto& ev:events) {
      const Id x=ev.second;const auto& C=tx.copies.at(x);
      std::array<Node,4> t;int k=0;for(Id y:key)if(y!=x)t[k++]=cur.at(y);t[2]=nodes.at(C.ids.at(ev.first));t[3]=nodes.at(C.ids.at(ev.first+1));
      if(!Positive(t))std::swap(t[2],t[3]);if(!Positive(t)){tx.cls="gap";tx.witness="degenerate gap tetrahedron at ridge vertex "+std::to_string(x);return tx;}
      fresh.push_back(Cell{0,t});++tx.gap_cells;cur[x]=nodes.at(C.ids.at(ev.first+1));}
  }
  // fan prisms (§5.5) of every crease edge incident to V0 (wall edges of the cavity)
  std::set<Edge> creaseEdges;
  for(const auto& f:PhysicalOf(s,cavity,true))for(int i=0;i<3;++i){const Id a=f.key[i],b=f.key[(i+1)%3];if(!V0.count(a)&&!V0.count(b))continue;size_t ch=0;
    if(CreaseOf(s,a,b,ch))creaseEdges.insert({std::min(a,b),std::max(a,b)});}
  for(const auto& e:creaseEdges) {
    const Copies* ca=copiesOf(e[0]);const Copies* cb=copiesOf(e[1]);if(!ca&&!cb)continue;  // no ridge end: DC-9 shared lateral
    const size_t K=(ca?ca:cb)->ids.size();if(ca&&cb&&ca->ids.size()!=cb->ids.size()){tx.cls="fan";tx.witness="crease edge with different fan counts";return tx;}
    auto slots=[&](Id x,const Copies* c){std::vector<Id> out(K,Lf(x));if(c)out=c->ids;return out;};
    const auto X=slots(e[0],ca),Y=slots(e[1],cb);const Id x=e[0],y=e[1];  // x < y
    for(size_t j=0;j+1<K;++j) {
      const std::array<std::array<Id,4>,3> tets{{{x,y,Y[j],Y[j+1]},{x,X[j],Y[j],Y[j+1]},{x,X[j],X[j+1],Y[j+1]}}};
      for(const auto& t:tets){std::set<Id> u(t.begin(),t.end());if(u.size()<4)continue;
        std::array<Node,4> c4{nodes.at(t[0]),nodes.at(t[1]),nodes.at(t[2]),nodes.at(t[3])};if(!Positive(c4))std::swap(c4[2],c4[3]);
        const Cell cell{0,c4};const auto why=FanCheck(s,cell);if(!why.empty()){++tx.fanChecks[why];tx.failing.push_back(Face{x,y,x});if(tx.cls.empty()){tx.cls="fan";std::ostringstream w;w<<"crease edge "<<x<<"-"<<y<<" slot "<<j<<": "<<why;tx.witness=w.str();}continue;}
        fresh.push_back(cell);tx.fanKeys.insert(CellKey(cell));++tx.fan_cells;}}
  }
  if(!tx.failing.empty())return tx;
  // columns of every rebuilt face (§5.6), lid/lateral copies on the face's side
  std::map<Face,int> lateralMarker;
  auto carrierMarker=[&](Id x0,Id x1)->int{for(const auto& sf:tx.old.surface){const int m=s.reference->Get(sf.facet).marker;if(s.walls.count(m))continue;if(Has(sf.v,x0)&&Has(sf.v,x1))return m;}return -1;};
  for(const auto& sp:sectors)for(const auto& r:sp.second.carrierFaces)lateralMarker[r.first]=r.second;
  for(const auto& key:tx.rebuilt) {
    const Face f=outFace.at(key);Skin skin;Id ovV=UINT64_MAX,ovU=UINT64_MAX,ovP=UINT64_MAX;const std::vector<Face>* extra=nullptr;std::vector<Face> storedExtras;
    for(const auto& sp:sectors){const auto j=sp.second.junction.find(key);if(j!=sp.second.junction.end()){ovV=j->second[0];ovU=j->second[1];ovP=j->second[2];extra=&sp.second.extras.at(key);
      tx.overrides[key]=j->second;auto& stored=tx.extras[key];const auto& flags=sp.second.lidFlags.at(key);
      for(size_t i=0;i<extra->size();++i){Face e=(*extra)[i];if(flags[i])e[2]=ovU;stored.push_back({e,flags[i]});}}}
    if(ovV==UINT64_MAX){const auto o=rows.overrides.find(key);if(o!=rows.overrides.end()){ovV=o->second[0];ovU=o->second[1];ovP=o->second[2];
      for(const auto& e:rows.extras.at(key)){Face x=e.first;if(e.second)x[2]=Lf(x[2]);storedExtras.push_back(x);}extra=&storedExtras;
      tx.overrides[key]=o->second;tx.extras[key]=rows.extras.at(key);++tx.stored_sectors;}}
    auto LfFace=[&](Id x)->Id{const Copies* c=copiesOf(x);if(!c)return Lf(x);return c->ids.at(sideOf(*c,f));};
    skin=ColumnSkin(f,LfFace,ovV,ovU,ovP);if(extra)skin.faces.insert(skin.faces.end(),extra->begin(),extra->end());
    for(unsigned i=0;i<3;++i){const int m=carrierMarker(f[i],f[(i+1)%3]);if(m>=0)for(const auto& t:skin.lateral[i])lateralMarker[Sorted(t)]=m;}
    {std::map<std::pair<Id,Id>,int> dir;for(const auto& t:skin.faces)for(int i=0;i<3;++i)++dir[{t[i],t[(i+1)%3]}];bool closed=true;std::ostringstream w;
     for(const auto& d:dir)if(dir.count({d.first.second,d.first.first})!=1||d.second!=1){closed=false;w<<" ("<<d.first.first<<"->"<<d.first.second<<")";}
     if(!closed){tx.failing.push_back(key);if(tx.cls.empty()){tx.cls="skin";tx.witness="face "+std::to_string(key[0])+": column skin not closed:"+w.str();}continue;}}
    auto cone=ConeColumn(s,skin.faces,nodes,next,false);
    if(!cone.ok){tx.failing.push_back(key);if(tx.cls.empty()){tx.cls=cone.cls;std::ostringstream w;w<<"face "<<key[0]<<","<<key[1]<<","<<key[2]<<(low?" (low band)":"")<<": "<<cone.witness;tx.witness=w.str();}continue;}
    tx.first_q=std::min(tx.first_q,cone.fq);tx.first_J=std::min(tx.first_J,cone.fJ);tx.min_slack=std::min(tx.min_slack,cone.slack);
    tx.apices[key]=next;nodes[next]=cone.apex;++next;fresh.insert(fresh.end(),cone.cells.begin(),cone.cells.end());
  }
  if(!tx.failing.empty())return tx;
  if(fresh.size()>MaximumReplacementCells){tx.cls="cap";tx.witness="cap-bound closure: "+std::to_string(fresh.size())+" fresh cells";tx.fresh_cells=fresh.size();return tx;}
  for(size_t i=0;i<fresh.size();++i)fresh[i].id=s.next_cell+i;tx.fresh_cells=fresh.size();
  {std::set<std::array<Id,4>> keys;for(const auto& c:fresh)if(tx.fanKeys.count(CellKey(c)))keys.insert(CellKey(c));tx.fanKeys=keys;}
  // signed volume closure
  {long double vo=0,vf=0;for(const auto& c:tx.old.cells)vo+=std::abs(Det(Of(c.v[0].p),Of(c.v[1].p),Of(c.v[2].p),Of(c.v[3].p)));
   for(const auto& c:fresh)vf+=Sigma()*Det(Of(c.v[0].p),Of(c.v[1].p),Of(c.v[2].p),Of(c.v[3].p));
   if(!(std::abs(vf-vo)<=1e-9L*vo)){tx.cls="topology";std::ostringstream w;w<<std::setprecision(12)<<"volume closure: old "<<double(vo)<<" fresh "<<double(vf);tx.witness=w.str();return tx;}}
  // surfaces (DC-9) plus carrier faces of ridge copies: a new skin face whose vertices all lie on one carrier
  std::map<Id,std::set<int>> onCarrier;for(const auto& sf:tx.old.surface){const int m=s.reference->Get(sf.facet).marker;if(s.walls.count(m))continue;for(Id x:sf.v)onCarrier[x].insert(m);}
  for(const auto& r:tx.copies)for(Id c:r.second.ids){const auto o=onCarrier.find(r.first);if(o!=onCarrier.end())onCarrier[c]=o->second;}
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
    if(marker<0){std::set<int> common;bool first=true;for(Id x:f){const auto o=onCarrier.find(x);std::set<int> here=o==onCarrier.end()?std::set<int>{}:o->second;
        if(first){common=here;first=false;}else{std::set<int> both;std::set_intersection(common.begin(),common.end(),here.begin(),here.end(),std::inserter(both,both.end()));common=both;}}
      if(common.size()==1)marker=*common.begin();}
    if(marker<0||s.walls.count(marker)){tx.cls="topology";std::ostringstream w;w<<"unexpected new skin face ("<<f[0]<<","<<f[1]<<","<<f[2]<<")";tx.witness=w.str();return tx;}
    std::array<Point,3> tri{nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p};Id facet=0;bool found=false;
    try{found=ref.FindFace(marker,tri,facet);}catch(const std::exception&){found=false;}
    if(!found){tx.cls="carrier";std::ostringstream w;w<<"new carrier face ("<<f[0]<<","<<f[1]<<","<<f[2]<<") leaves marker "<<marker<<" authority";tx.witness=w.str();return tx;}
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
  if(splitPolicy.rule==1){std::set<Id> rel;for(const auto& k:tx.rebuilt){const auto a=rows.apex.find(k);if(a!=rows.apex.end())rel.insert(a->second);}
    if(!RemovesVertex(tx.old.cells,out.cells,&rel)){std::string why;if(!MeasurableChanged(s,tx.old,out,why)){tx.cls="measure";tx.witness=why;return tx;}
      ++splitPolicy.layer_free;tx.fresh=std::move(out);tx.ok=true;return tx;}++splitPolicy.layer_checked;}
  {long double removed=0,added=0;std::string why;if(!SizeCertificate(s,tx.old,out,removed,added,why)){tx.cls="measure";tx.witness=why;return tx;}
   if(!(added<=removed)){tx.cls="excess";std::ostringstream w;w<<"LayerConstruct adds frozen excess: added "<<double(added)<<" > removed "<<double(removed);tx.witness=w.str();return tx;}}
  tx.fresh=std::move(out);tx.ok=true;return tx;
}
inline void ApplyTx10(State& s,Rows10& rows,const Tx10& tx) {
  ApplyTx(s,rows,tx);for(const auto& r:tx.copies)rows.copies[r.first]=r.second;
  for(const auto& k:tx.droppedFan)rows.fan.erase(k);for(const auto& k:tx.fanKeys)rows.fan.insert(k);
}
inline std::set<Id> FanCells(const State& s,const Rows10& rows) {
  std::set<Id> out;std::map<std::array<Id,4>,Id> byKey;
  for(const auto& k:rows.fan){const auto a=s.vertex_cells.find(k[0]);if(a==s.vertex_cells.end())continue;
    for(Id id:a->second)if(CellKey(s.mesh.at(id).cell)==k)out.insert(id);}
  return out;
}
struct R2Stage10 : R2Stage { size_t ridges=0,fan_cells=0,gap_cells=0;std::map<std::string,size_t> fanChecks; };
inline R2Stage10 PhaseR2_10(State& s,Rows10& rows,double seconds,std::ostream& log) {
  Scope scope("PhaseR2");
  R2Stage10 g;Begin(s,g);const auto t0=Clock::now();
  for(unsigned pass=0;pass<2;++pass) {
    std::vector<std::pair<long double,Id>> order;
    for(Id v:WallVertices(s)){if(rows.lift.count(v)||Rim(s,v))continue;const auto d=DirectionAt(s,v);order.emplace_back(d.ok?-std::round(d.s*1e6L)/1e6L:1.L,v);}
    std::sort(order.begin(),order.end());
    for(const auto& ov:order) {
      const Id v=ov.second;
      if(rows.lift.count(v)||Rim(s,v))continue;if(Seconds(t0)>=seconds){g.outcome="limit:time";break;}
      std::set<Id> V0{v};Tx10 tx;bool low=false;unsigned separated=0;
      for(unsigned grow=0;grow<20;++grow) {
        if(Seconds(t0)>=seconds)break;
        ++g.work;tx=Transaction10(s,rows,V0,low);
        for(const auto& c:tx.fanChecks)g.fanChecks[c.first]+=c.second;
        log<<"DC10 R2 tx v="<<v<<" |V|="<<V0.size()<<" low="<<low<<" ok="<<tx.ok<<" old="<<tx.old_cells<<" fresh="<<tx.fresh_cells<<" ridges="<<tx.ridges<<" fan="<<tx.fan_cells<<" gaps="<<tx.gap_cells<<" sectors="<<tx.sectors<<" cls="<<tx.cls<<" "<<tx.witness<<"\n";
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
      if(tx.ok){ApplyTx10(s,rows,tx);++g.accepted;++g.transactions;g.low_band+=low;g.sector_tx+=tx.sectors>0;g.gaps+=tx.gaps;g.max_closure=std::max(g.max_closure,V0.size());
        g.ridges+=tx.ridges;g.fan_cells+=tx.fan_cells;g.gap_cells+=tx.gap_cells;
        g.max_old=std::max(g.max_old,tx.old_cells);g.max_fresh=std::max(g.max_fresh,tx.fresh_cells);g.first_q=std::min(g.first_q,tx.first_q);g.first_J=std::min(g.first_J,tx.first_J);g.height_error=std::max(g.height_error,tx.height_error);}
      else{++g.rejected;++g.classes[tx.cls];++g.reasons[tx.cls+": "+Key(tx.witness)];g.last="vertex "+std::to_string(v)+" ["+tx.cls+"] "+tx.witness;}
    }
    if(!g.outcome.empty())break;
  }
  for(Id v:WallVertices(s))if(!Rim(s,v))(rows.lift.count(v)?g.lifted:g.unlifted)++;
  for(const auto& wf:WallFaces(s)){if(RimFace(s,wf.second)){++g.rim_faces;continue;}const bool r=rows.apex.count(wf.second)&&std::all_of(wf.second.begin(),wf.second.end(),[&](Id x){return rows.lift.count(x);});(r?g.rowed:g.unrowed)++;}
  if(g.outcome.empty())g.outcome=g.unrowed?"incomplete":"complete";End(s,g,t0);
  log<<"DC10 R2 transactions="<<g.transactions<<" lifted="<<g.lifted<<" unlifted="<<g.unlifted<<" rowed="<<g.rowed<<" unrowed="<<g.unrowed<<" fan_cells="<<FanCells(s,rows).size()<<"\n";return g;
}

// V as DC-9 with fan cells protected like column cells and ColumnSplit refused on a wall edge with a ridge endpoint.
inline VStage PhaseV10(State& s,Rows10& rows,size_t work,double seconds,std::ostream& log) {
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
  auto build=[&]{ci.Build(s,rows);for(Id id:FanCells(s,rows))ci.cells.insert(id);for(const auto& r:rows.copies)for(Id c:r.second.ids)ci.unlift[c]=r.first;};
  auto ridgeEdge=[&](Edge w){if(rows.copies.count(w[0])||rows.copies.count(w[1]))return true;
    for(const auto& r:rows.apex)if(Has(r.first,w[0])&&Has(r.first,w[1]))for(Id x:r.first)if(rows.copies.count(x))return true;return false;};
  for(;;++g.sweeps) {
    if(limited()){g.outcome="limit";break;}
    build(); bool changed=false;g.column_demand=0;g.demand.clear();
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
            if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=ColumnSplit(s,rows,w,g,why,rejected);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}}
        else if(b0!=UINT64_MAX&&b1!=UINT64_MAX&&b0!=b1){const Edge w{std::min(b0,b1),std::max(b0,b1)};if(ridgeEdge(w))why="ColumnSplit: ridge vertex on the split faces (fan)";else{try{done=ColumnSplit(s,rows,w,g,why,rejected);}catch(const std::exception& x){done=false;why=std::string("ColumnSplit: ")+x.what();}}}
        else why="column demand without a column route";
        if(done){changed=true;build(); }else{++g.column_demand;++g.demand[Key(why)];g.last="column demand: "+why;}
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
      changed|=taken;if(taken)build(); 
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
        if(ApexMove(s,rows,apexKey,g,why)){changed=true;build(); }else{rejected.insert(k);++g.demand["bad column cell: "+Key(why)];}
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
      if(acc){changed=true;build(); }else rejected.insert(key);
    }
    const auto gates=Final(s);
    log<<"DC10 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" height_fail="<<gates.height_failures<<" column_demand="<<g.column_demand<<" work="<<g.work<<" cache_skips="<<g.cache_skips<<" t="<<Seconds(t0)<<"\n";
    std::cout<<"DC10 V sweep "<<g.sweeps<<" cells="<<gates.cells<<" long="<<gates.long_edges<<" bad="<<gates.bad_shape<<" work="<<g.work<<" t="<<Seconds(t0)<<std::endl;
    if(gates.complete){g.outcome="complete";break;}
    if(limited()){g.outcome="limit";break;}
    if(!changed){g.outcome="blocked";break;}
  }
  End(s,g,t0);return g;
}
// Final mesh with the native gates; cell class 3 = fan cell.
inline void ExportGates10(State& s,const Rows10& rows,const std::string& path) {
  Scope scope("ExportGates");
  Export(s,path);std::ofstream out(path,std::ios::app);out<<std::setprecision(17);
  std::map<Id,Id> column;for(const auto& r:rows.apex)for(Id id:StarOf(s,r.second))column.emplace(id,r.second);const auto fan=FanCells(s,rows);
  out<<"cell_gates "<<s.mesh.size()<<"\n";
  for(const auto& row:s.mesh){const auto sh=MeshShape(s,row.first);const bool first=WallChild(s,row.first);const auto c=column.find(row.first);
    out<<row.first<<' '<<sh.q<<' '<<sh.J<<' '<<Bad(sh)<<' '<<first<<' '<<(c==column.end()?-1:(long long)c->second)<<' '<<(first?1:c!=column.end()?2:fan.count(row.first)?3:0)<<"\n";}
  std::vector<std::string> lines;std::set<Edge> seen;
  for(const auto& row:s.mesh){const auto& c=row.second.cell;for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b){
    const Edge e{std::min(c.v[a].id,c.v[b].id),std::max(c.v[a].id,c.v[b].id)};if(!seen.insert(e).second)continue;
    long double l=-1;bool ok=true;try{l=Len(s,e[0],e[1]);}catch(const std::exception&){ok=false;}
    if(ok&&!(l>MaximumMetricEdge))continue;
    bool inColumn=true;for(Id id:Star(s,e))inColumn=inColumn&&column.count(id);
    const int cls=!ok?4:EdgeClass(s,e);std::ostringstream x;x<<std::setprecision(10)<<e[0]<<' '<<e[1]<<' '<<(ok?double(l):-1.0)<<' '<<cls<<' '<<inColumn;lines.push_back(x.str());}}
  out<<"long_edges "<<lines.size()<<"\n";for(const auto& l:lines)out<<l<<"\n";
}
}  // namespace dc10
