// CONSTRUCTION_STRATEGY_PLAN rev 3, Codex review 2 blocker 3 (D2): TEST-ONLY native replay of the measured
// non-integrable M6 band edges (integration_evidence/native_3d_bl_v1/construction_strategy_v1/d2). Not production code.
// For each witness: the unchanged native FrozenField::TargetEdgeLength; the native EdgeBreakpoints cut list; a replica
// of the composed edge integrator (CNativeField3D.cpp EdgeLength: same pieces, same integrand, GK7/15, depth 20,
// 65,536 evaluations, 1e-9 relative + 1e-12 absolute) that must reproduce the native outcome exactly; a replica of the
// faceted 3D cut logic (CBoundaryLayerMetric.cpp EdgeBreakpointsImpl) that must reproduce the native cut list exactly;
// the residual hot spots, the bisected integrand jump, native samples (distance/normal/hn) on both sides, the
// brute-force nearest wall facets (native facet ids) and one-sided integrands at each cut. Variants of the cut list
// (SU2_D2_VARIANT) are test-only candidate fixes. SU2_D2_LAW=copy evaluates the integrand with the generated
// test-only copy of CBoundaryLayerMetric (native_d2_blmetric_patched.inc); SU2_D2_TIE selects its ownership rule
// (0 = production rule, must reproduce native; 1 = canonical owner among rounding-level distance ties).
// SU2_D2_PIN=1 pins the owner facet of every integrand evaluation to the feature owning that chord parameter in the
// cut logic's envelope (d2env copy with an Owner accessor; vertex/segment -> lowest incident candidate face).
// Budget (65,536) and tolerance (1e-9) are never changed.
#include "../../../../Common/include/adaptation/CNativeChordEnvelope.hpp"
#include "native_d2_blmetric_patched.inc"
#include "native_d2_envelope_copy.inc"
namespace d2_replay {
using namespace SU2Native3D;
using swbi_dc2::Clock;using swbi_dc2::Seconds;
using A3=std::array<double,3>;
struct WallGeometry {  // Exactly the wall handed to CBoundaryLayerMetric by swbi_dc2::Build (face order = facet order).
  std::vector<A3> x;std::vector<std::array<unsigned long,3>> face;std::vector<Id> id;std::vector<std::array<Id,3>> gid;
  double h0=0,growth=0,thickness=0,full=0,tolerance=0;
};
WallGeometry Wall(const swbi_dc2::Case& c) {
  WallGeometry w;std::map<Id,unsigned long> index;w.h0=c.h0;w.growth=c.growth;w.thickness=c.thickness;
  for(size_t i=0;i<c.nfacets;++i)if(int(c.facets[5*i+1])==c.wall) {
    std::array<unsigned long,3> f;std::array<Id,3> g;
    for(unsigned k=0;k<3;++k){const auto v=c.facets[5*i+2+k];const auto ins=index.emplace(Id(v),index.size());
      if(ins.second)w.x.push_back({c.P0[3*v],c.P0[3*v+1],c.P0[3*v+2]});f[k]=ins.first->second;g[k]=Id(v);}
    w.face.push_back(f);w.gid.push_back(g);w.id.push_back(Id(c.facets[5*i]));
  }
  w.full=std::max(w.h0,.9*w.thickness);
  double maxAbs=0,extent=0;A3 lo{INFINITY,INFINITY,INFINITY},hi{-INFINITY,-INFINITY,-INFINITY};
  for(const auto& p:w.x)for(unsigned j=0;j<3;++j){lo[j]=std::min(lo[j],p[j]);hi[j]=std::max(hi[j],p[j]);maxAbs=std::max(maxAbs,std::abs(p[j]));}
  for(unsigned j=0;j<3;++j)extent=std::max(extent,hi[j]-lo[j]);
  w.tolerance=64*std::numeric_limits<double>::epsilon()*std::max(maxAbs,extent);
  return w;
}
std::vector<DonorCell> Donors(const swbi_dc2::Case& c) {  // Same six covering donors as swbi_dc2::Build, canonical order.
  Point lo=swbi_dc2::At(c.P0,0),hi=lo;
  auto grow=[&](const std::vector<double>& x){for(size_t i=0;i<x.size()/3;++i){const auto p=swbi_dc2::At(x,i);
    lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}};
  grow(c.P0);grow(c.pts);
  const double pad=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z});lo={lo.x-pad,lo.y-pad,lo.z-pad};hi={hi.x+pad,hi.y+pad,hi.z+pad};
  Tensor sensor;sensor.xx=sensor.yy=sensor.zz=1/(c.hs*c.hs);sensor.xy=sensor.xz=sensor.yz=0;
  std::vector<DonorCell> donors;
  auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{b&1?hi.x:lo.x,b&2?hi.y:lo.y,b&4?hi.z:lo.z}};};
  const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
  for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);
    for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
    if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);
    d.sensor.fill(sensor);donors.push_back(d);}
  std::sort(donors.begin(),donors.end(),[](const DonorCell& a,const DonorCell& b){return a.Key()<b.Key();});
  return donors;
}
std::shared_ptr<d2patched::CBoundaryLayerMetric> PatchedLayers(const swbi_dc2::Case& c) {  // Same wall as Build.
  d2patched::CBoundaryLayerMetric::Wall w;w.layer.marker=c.markers.at(c.wall);w.layer.firstHeight=c.h0;w.layer.growth=c.growth;w.layer.thickness=c.thickness;
  std::map<Id,unsigned long> index;
  for(size_t i=0;i<c.nfacets;++i)if(int(c.facets[5*i+1])==c.wall){for(unsigned k=0;k<3;++k){const auto v=c.facets[5*i+2+k];
    const auto ins=index.emplace(Id(v),index.size());if(ins.second){w.coord.push_back(c.P0[3*v]);w.coord.push_back(c.P0[3*v+1]);w.coord.push_back(c.P0[3*v+2]);}
    w.conn.push_back(ins.first->second);}w.conn.push_back(w.conn.back());}
  std::vector<d2patched::CBoundaryLayerMetric::Wall> walls;walls.push_back(std::move(w));
  return std::make_shared<d2patched::CBoundaryLayerMetric>(3,std::move(walls));
}
MetricComposition PatchedCompose(std::shared_ptr<d2patched::CBoundaryLayerMetric> layers,double core) {
  return [layers,core](Point p,Tensor s) {
    d2patched::CBoundaryLayerMetric::Tensor m;m.m[0][0]=s.xx;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;
    m.m[1][1]=s.yy;m.m[1][2]=m.m[2][1]=s.yz;m.m[2][2]=s.zz;const su2double x[3]={p.x,p.y,p.z};
    layers->ApplyPoint(x,m,core,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
                  SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
}
// Test-only cut variants. 0 = replica of the native faceted 3D cut logic.
struct Variant {int id=0;};
struct CutDiag {std::vector<unsigned long> faces;std::vector<double> transitions;size_t envelopeCuts=0,classificationCuts=0;
  std::shared_ptr<d2env::SU2Native3D::ChordEnvelope> envelope;std::vector<long> featureFace;  // envelope feature -> owner face (-1: support cap)
  long Owner(long double t)const{if(!envelope)return -1;const auto k=envelope->Owner(t);return k<featureFace.size()?featureFace[k]:-1;}};
std::vector<double> Breakpoints(const WallGeometry& w,const A3& a,const A3& b,const Variant& variant,CutDiag* diag,
                                double bandRatio=1.5,unsigned fadeBands=16) {
  using Vector=std::array<long double,3>;
  Vector chord{};for(unsigned j=0;j<3;++j)chord[j]=(long double)b[j]-a[j];
  auto dot=[](const Vector& u,const Vector& v){return u[0]*v[0]+u[1]*v[1]+u[2]*v[2];};
  std::vector<double> cuts{0,1};std::set<double> retained{0,1};
  const double h0=w.h0,full=w.full,thickness=w.thickness,tolerance=w.tolerance;
  std::vector<double> levels{0,.5*h0};
  levels.push_back(tolerance);
  for(double level=h0;level<full;){levels.push_back(level);const double next=level*bandRatio;level=next;}
  levels.push_back(full);
  for(unsigned k=1;k<=fadeBands;++k)levels.push_back(full+(thickness-full)*k/fadeBands);
  std::vector<unsigned long> faces;
  A3 low,high;for(unsigned j=0;j<3;++j){low[j]=std::min(a[j],b[j])-thickness;high[j]=std::max(a[j],b[j])+thickness;}
  for(unsigned long f=0;f<w.face.size();++f) {
    bool hit=true;
    for(unsigned j=0;j<3 && hit;++j){double l=INFINITY,h=-INFINITY;for(auto v:w.face[f]){l=std::min(l,w.x[v][j]);h=std::max(h,w.x[v][j]);}
      hit=!(h<low[j] || l>high[j]);}
    if(hit)faces.push_back(f);
  }
  if(diag)diag->faces=faces;
  if(faces.empty())return cuts;
  double coordinateScale=thickness;for(unsigned j=0;j<3;++j)coordinateScale=std::max({coordinateScale,std::abs(a[j]),std::abs(b[j])});
  const auto support=std::nextafter(std::max(thickness,tolerance)+128*std::numeric_limits<double>::epsilon()*coordinateScale+4*tolerance,INFINITY);
  auto owned=std::make_shared<d2env::SU2Native3D::ChordEnvelope>(a,b,1u<<20,support);auto& envelope=*owned;
  std::vector<long> featureFace(envelope.Features(),-1);
  auto record=[&](unsigned long face){while(featureFace.size()<envelope.Features())featureFace.push_back(long(face));};
  std::set<unsigned long> vertices;std::set<std::pair<unsigned long,unsigned long>> edges;
  for(const auto face:faces) {
    std::array<A3,3> triangle;const auto& ids=w.face[face];
    for(unsigned k=0;k<3;++k) {
      triangle[k]=w.x[ids[k]];
      if(vertices.insert(ids[k]).second){envelope.Vertex(triangle[k]);record(face);}
      const auto edge=std::minmax(ids[k],ids[(k+1)%3]);
      if(edges.insert(edge).second){envelope.Segment(w.x[edge.first],w.x[edge.second]);record(face);}
    }
    envelope.Face(triangle);record(face);
  }
  for(auto t:envelope.Cuts(levels,true))if(t>0 && t<1 && retained.insert(t).second){cuts.push_back(t);if(diag)++diag->envelopeCuts;}
  if(diag){diag->transitions=envelope.Cuts({},true);diag->envelope=owned;diag->featureFace=featureFace;}
  {
    const auto cross=[](const Vector& a,const Vector& b){return Vector{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};};
    const auto delta=[](const A3& a,const A3& b){return Vector{(long double)a[0]-b[0],(long double)a[1]-b[1],(long double)a[2]-b[2]};};
    long double magnitude=0;for(unsigned j=0;j<3;++j)magnitude=std::max({magnitude,std::abs((long double)a[j]),std::abs((long double)b[j])});
    const auto padding=64*std::numeric_limits<double>::epsilon()*magnitude;
    constexpr double NormalFeatureThreshold3D=1e-10;
    auto classificationCut=[&](long double t,long double distance) {
      if(!(t>0 && t<1) || retained.count(double(t)))return;
      const auto nearest=envelope.Distance(t);
      if(nearest>tolerance+padding || distance>nearest+padding)return;
      retained.insert(double(t));cuts.push_back(double(t));if(diag)++diag->classificationCuts;
    };
    for(const auto face:faces) {
      const auto& ids=w.face[face];const auto p=w.x[ids[0]];
      const auto u=delta(w.x[ids[1]],p),v=delta(w.x[ids[2]],p),r=delta(a,p),n=cross(u,v);
      const auto square=dot(n,n);
      const long double first[3]={0,dot(cross(r,v),n)/square,dot(cross(u,r),n)/square};
      const long double slope[3]={0,dot(cross(chord,v),n)/square,dot(cross(u,chord),n)/square};
      const long double wt[3]={1-first[1]-first[2],first[1],first[2]},dw[3]={-slope[1]-slope[2],slope[1],slope[2]};
      for(unsigned k=0;k<3;++k)if(dw[k]!=0) {
        const auto t=(NormalFeatureThreshold3D-wt[k])/dw[k];if(!(t>0 && t<1))continue;
        long double lo=1,scale=1;for(unsigned j=0;j<3;++j){lo=std::min(lo,std::fma(t,dw[j],wt[j]));scale=std::max({scale,std::abs(wt[j]),std::abs(dw[j])});}
        if(lo < -256*std::numeric_limits<long double>::epsilon()*scale)continue;
        const auto distance=std::abs(dot(r,n)+t*dot(chord,n))/std::sqrt(square);
        classificationCut(t,distance);
      }
    }
    for(const auto& edge:edges) {
      const auto p=w.x[edge.first];const auto e=delta(w.x[edge.second],p),r=delta(a,p);
      const auto square=dot(e,e),slope=dot(chord,e)/square;if(slope==0)continue;
      const auto first=dot(r,e)/square;
      for(long double fraction:{(long double)NormalFeatureThreshold3D,1.L-NormalFeatureThreshold3D}) {
        const auto t=(fraction-first)/slope;if(!(t>0 && t<1))continue;
        Vector residual{};for(unsigned j=0;j<3;++j)residual[j]=std::fma(t,chord[j],r[j])-fraction*e[j];
        classificationCut(t,std::sqrt(dot(residual,residual)));
      }
    }
  }
  (void)variant;
  std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
  return cuts;
}
struct Panel {size_t piece;long double low,high,value,error;unsigned depth;bool operator<(const Panel& o)const{return error<o.error;}};
struct Integrand {  // Replica of the composed EdgeLength integrand of one edge (pieces from the native TraceSegment).
  Point a,b;const std::vector<DonorCell>* donors;MetricComposition composition;std::vector<SegmentPiece> pieces;size_t evaluations=0,limit=65536;
  const CutDiag* pin=nullptr;  // Envelope-pinned owner (test-only copy law only).
  Point At(size_t index,long double t)const{const auto& piece=pieces[index];const auto parameter=piece.begin+t*piece.width;
    return {double((1-parameter)*a.x+parameter*b.x),double((1-parameter)*a.y+parameter*b.y),double((1-parameter)*a.z+parameter*b.z)};}
  long double Value(size_t index,long double t,bool counted=true) {
    if(counted && ++evaluations>limit)throw std::length_error("budget");
    const auto& piece=pieces[index];const auto& donor=(*donors)[piece.donor];
    std::array<long double,4> weights;for(unsigned k=0;k<4;++k)weights[k]=(1-t)*piece.begin_weights[k]+t*piece.end_weights[k];
    d2patched::D2ForcedFace=pin?pin->Owner(piece.begin+t*piece.width):-1;
    struct Reset{~Reset(){d2patched::D2ForcedFace=-1;}} reset;
    const auto target=composition(At(index,t),InterpolateSensor(donor,weights));ValidateTensor(target);return MetricNorm(a,b,target);
  }
  // Global chord parameter -> (piece, local parameter); uncounted diagnostic evaluation.
  std::pair<size_t,long double> Local(long double g)const{
    for(size_t i=0;i<pieces.size();++i)if(g<=pieces[i].end || i+1==pieces.size())return {i,std::clamp((g-pieces[i].begin)/pieces[i].width,0.L,1.L)};
    return {0,0};
  }
  long double F(long double g){const auto l=Local(g);return Value(l.first,l.second,false);}
  Point X(long double g)const{const auto l=Local(g);return At(l.first,l.second);}
};
struct Outcome {bool ok=false;long double total=0,error=0;size_t evaluations=0;std::string failure;size_t failPiece=0;long double failParameter=0;
  std::vector<Panel> residual;};
Outcome Integrate(Integrand& f,const std::vector<double>& cuts) {
  Outcome out;f.evaluations=0;
  constexpr long double node[8]={.991455371120812639206854697526329L,.949107912342758524526189684047851L,
      .864864423359769072789712788640926L,.741531185599394439863864773280788L,.586087235467691130294144838258730L,
      .405845151377397166906606412076961L,.207784955007898467600689403773245L,0};
  constexpr long double kronrod[8]={.022935322010529224963732008058970L,.063092092629978553290700663189204L,
      .104790010322250183839876322541518L,.140653259715525918745189590510238L,.169004726639267902826583426598550L,
      .190350578064785409913256402421014L,.204432940075298892414161999234649L,.209482141084727828012999174891714L};
  constexpr long double gauss[4]={.129484966168869693270611432679082L,.279705391489276667901467771423780L,
      .381830050505118944950369775488975L,.417959183673469387755102040816327L};
  size_t lastPiece=0;long double lastParameter=0;
  auto value=[&](size_t index,long double t){lastPiece=index;lastParameter=t;return f.Value(index,t);};
  auto measure=[&](size_t index,long double low,long double high,unsigned depth) {
    const auto middle=(low+high)/2,half=(high-low)/2;
    if(!(middle>low && middle<high))throw std::runtime_error("Native 3D composed edge panel is unrepresentable.");
    const auto center=value(index,middle);
    long double lower=gauss[3]*center,higher=kronrod[7]*center;
    for(unsigned j=0;j<7;++j){const auto pair=value(index,middle-half*node[j])+value(index,middle+half*node[j]);higher+=kronrod[j]*pair;if(j%2)lower+=gauss[j/2]*pair;}
    const auto scale=f.pieces[index].width*half;
    return Panel{index,low,high,scale*higher,scale*std::abs(higher-lower),depth};
  };
  long double total=0,error=0;std::vector<Panel> pending,stuck;
  auto retain=[&](Panel p){total+=p.value;error+=p.error;pending.push_back(p);std::push_heap(pending.begin(),pending.end());};
  try {
    for(size_t index=0;index<f.pieces.size();++index) {
      const auto& piece=f.pieces[index];long double low=0;
      for(const auto cut:cuts){if(cut<=piece.begin || cut>=piece.end)continue;const auto high=std::clamp((cut-piece.begin)/piece.width,0.L,1.L);
        if(high>low)retain(measure(index,low,high,0));low=high;}
      if(low<1)retain(measure(index,low,1,0));
    }
    while(error>1e-9L*std::abs(total)+1e-12L) {
      if(pending.empty())throw std::runtime_error("did not converge");
      std::pop_heap(pending.begin(),pending.end());auto panel=pending.back();pending.pop_back();
      if(panel.depth>=20){stuck.push_back(panel);continue;}
      const auto middle=(panel.low+panel.high)/2;
      const auto left=measure(panel.piece,panel.low,middle,panel.depth+1),right=measure(panel.piece,middle,panel.high,panel.depth+1);
      total-=panel.value;error=std::max(0.L,error-panel.error);retain(left);retain(right);
    }
    out.ok=true;
  }catch(const std::length_error&){out.failure="budget";out.failPiece=lastPiece;out.failParameter=lastParameter;}
   catch(const std::exception& e){out.failure=e.what();}
  out.total=total;out.error=error;out.evaluations=std::min(f.evaluations,f.limit);
  out.residual=pending;out.residual.insert(out.residual.end(),stuck.begin(),stuck.end());
  return out;
}
using LV=junction_wp0::V;
struct Foot {long double distance;LV foot;};
Foot Nearest(const WallGeometry& w,unsigned long f,Point x) {
  const auto& t=w.face[f];auto P=[&](unsigned k){return LV{w.x[t[k]][0],w.x[t[k]][1],w.x[t[k]][2]};};
  const LV p{x.x,x.y,x.z};const auto c=junction_wp0::Closest(p,P(0),P(1),P(2));return {junction_wp0::Norm(p-c),c};
}
std::string Num(long double v){std::ostringstream s;s<<std::setprecision(17)<<double(v);const auto r=s.str();
  return (r=="nan"||r=="-nan"||r=="inf"||r=="-inf")?"null":r;}
std::string Relation(const WallGeometry& w,unsigned long f,unsigned long g) {
  if(f==g)return "same";int shared=0;for(auto u:w.gid[f])for(auto v:w.gid[g])shared+=u==v;
  return shared==2?"edge":shared==1?"vertex":"disjoint";
}
long double Dihedral(const WallGeometry& w,unsigned long f,unsigned long g) {
  auto normal=[&](unsigned long i){const auto& t=w.face[i];LV p[3];for(unsigned k=0;k<3;++k)p[k]={w.x[t[k]][0],w.x[t[k]][1],w.x[t[k]][2]};
    return junction_wp0::Unit(junction_wp0::Cross(p[1]-p[0],p[2]-p[0]));};
  return std::acos(std::clamp(junction_wp0::Dot(normal(f),normal(g)),-1.L,1.L))*180/M_PIl;
}
// Native sample and native-selected facet at a chord parameter (facet = brute-force nearest with matching foot direction).
template<class Layers> std::string SideJson(Layers& layers,const WallGeometry& w,const std::vector<unsigned long>& faces,Integrand& f,long double g,unsigned long& owner) {
  const auto x=f.X(g);const su2double X[3]={x.x,x.y,x.z};const auto s=layers.Evaluate(0,X,1.0);
  long double best=INFINITY;for(auto face:faces)best=std::min(best,Nearest(w,face,x).distance);
  owner=std::numeric_limits<unsigned long>::max();long double match=INFINITY;std::vector<std::pair<long double,unsigned long>> near;
  for(auto face:faces){const auto n=Nearest(w,face,x);if(n.distance<=best*(1+1e-9L)+1e-300L) {
      near.push_back({n.distance,face});
      const LV dir=n.distance>0?(1/n.distance)*(LV{x.x,x.y,x.z}-n.foot):LV{};
      const auto m=junction_wp0::Norm(dir-LV{s.normal[0],s.normal[1],s.normal[2]});
      if(m<match){match=m;owner=face;}}}
  std::ostringstream j;j<<"{\"t\":"<<Num(g)<<",\"f\":"<<Num(f.F(g))<<",\"d\":"<<Num(s.distance)<<",\"hn\":"<<Num(s.hn)<<",\"weight\":"<<Num(s.weight)
    <<",\"normal\":["<<Num(s.normal[0])<<','<<Num(s.normal[1])<<','<<Num(s.normal[2])<<"],\"brute_d\":"<<Num(best)
    <<",\"owner\":"<<(owner==std::numeric_limits<unsigned long>::max()?std::string("null"):std::to_string(w.id[owner]))<<",\"owner_normal_mismatch\":"<<Num(match)<<",\"near\":[";
  for(size_t i=0;i<near.size();++i)j<<(i?",":"")<<"["<<w.id[near[i].second]<<','<<Num((near[i].first-best)/std::max(best,1e-300L))<<"]";
  j<<"]}";return j.str();
}
}  // namespace d2_replay

TEST_CASE("D2 native replay of the non-integrable M6 band edges", "[.][NativeD2Replay]") {
  using namespace d2_replay;
  World world;REQUIRE(world.size==1);
  const char* root=std::getenv("SU2_D2_CASE");REQUIRE(root);           // dc2/m6/input
  const char* list=std::getenv("SU2_D2_WITNESSES");REQUIRE(list);      // d2/witnesses.i64 (index,a,b)
  const char* output=std::getenv("SU2_D2_OUT");REQUIRE(output);        // JSONL, appended and resumed
  const char* mode=std::getenv("SU2_D2_MODE");const std::string stage=mode?mode:"diagnose";
  const int variantId=std::getenv("SU2_D2_VARIANT")?std::atoi(std::getenv("SU2_D2_VARIANT")):0;
  const std::string law=std::getenv("SU2_D2_LAW")?std::getenv("SU2_D2_LAW"):"native";REQUIRE((law=="native"||law=="copy"));
  const int tie=std::getenv("SU2_D2_TIE")?std::atoi(std::getenv("SU2_D2_TIE")):0;REQUIRE((tie==0||tie==1));
  const bool callNative=!std::getenv("SU2_D2_NATIVE") || std::atoi(std::getenv("SU2_D2_NATIVE"));
  d2patched::D2TieMode=tie;
  const bool pinOwner=std::getenv("SU2_D2_PIN") && std::atoi(std::getenv("SU2_D2_PIN"));REQUIRE((!pinOwner || law=="copy"));
  const auto t0=Clock::now();
  const auto c=swbi_dc2::Load(root);auto n=swbi_dc2::Build(c);
  const auto wall=Wall(c);const auto donors=Donors(c);
  std::vector<const Cell*> cells;for(const auto& d:donors)cells.push_back(&d.cell);
  const auto patched=PatchedLayers(c);
  const auto compose=law=="copy"?PatchedCompose(patched,c.core):junction_wp0::Compose(n.layers,c.core);
  std::ifstream in(list,std::ios::binary|std::ios::ate);REQUIRE(in);const size_t count=size_t(in.tellg())/24;in.seekg(0);
  std::vector<int64_t> w(3*count);in.read(reinterpret_cast<char*>(w.data()),std::streamsize(24*count));REQUIRE(in);
  std::set<int64_t> done;{std::ifstream old(output);for(std::string line;std::getline(old,line);){const auto k=line.find("\"index\":");
    if(k!=std::string::npos)done.insert(std::atoll(line.c_str()+k+8));}}
  std::ofstream out(output,std::ios::app);REQUIRE(out);
  std::cout<<"D2 "<<stage<<" variant "<<variantId<<" law "<<law<<" tie "<<tie<<" pin "<<pinOwner<<": "<<count<<" witnesses, "<<done.size()<<" resumed, wall faces "<<wall.face.size()
           <<" tolerance "<<wall.tolerance<<", built in "<<Seconds(t0)<<" s\n"<<std::flush;
  for(size_t i=0;i<count;++i) {
    if(done.count(w[3*i]))continue;
    const auto ts=Clock::now();
    const Point a=swbi_dc2::At(c.pts,w[3*i+1]),b=swbi_dc2::At(c.pts,w[3*i+2]);
    const A3 A{a.x,a.y,a.z},B{b.x,b.y,b.z};
    std::ostringstream j;j<<std::setprecision(17)<<"{\"index\":"<<w[3*i]<<",\"a\":"<<w[3*i+1]<<",\"b\":"<<w[3*i+2]<<",\"variant\":"<<variantId<<",\"law\":\""<<law<<"\",\"tie\":"<<tie<<",\"pin\":"<<pinOwner;
    Integrand f{a,b,&donors,compose,TraceSegment(cells,a,b,nullptr,cells.size())};
    CutDiag diag;const auto native=n.layers->EdgeBreakpoints(A,B);const auto copy=Breakpoints(wall,A,B,Variant{0},&diag);
    if(pinOwner)f.pin=&diag;
    j<<",\"pieces\":"<<f.pieces.size()<<",\"cuts\":"<<native.size()<<",\"cut_replica_equal\":"<<(native==copy)<<",\"envelope_cuts\":"<<diag.envelopeCuts
     <<",\"classification_cuts\":"<<diag.classificationCuts<<",\"faces\":"<<diag.faces.size()<<",\"transitions\":"<<diag.transitions.size()-2;
    if(stage=="diagnose") {
      std::string nativeWhat;long double nativeLength=NAN;
      if(callNative){try{nativeLength=n.field->TargetEdgeLength(a,b);}catch(const std::exception& e){nativeWhat=e.what();}}
      else nativeWhat="skipped";
      const auto o=Integrate(f,native);
      std::ostringstream param;param<<std::setprecision(21)<<"piece="<<o.failPiece<<" parameter="<<o.failParameter;
      const bool reproduced=(nativeWhat.empty()?o.ok && o.total==nativeLength:(!o.ok && o.failure=="budget" && nativeWhat.find(param.str())!=std::string::npos));
      j<<",\"replica_evaluations\":"<<o.evaluations<<",\"native_called\":"<<callNative<<",\"native_ok\":"<<nativeWhat.empty()<<",\"native_length\":"<<Num(nativeLength)<<",\"replica_ok\":"<<o.ok<<",\"replica_failure\":\""<<o.failure
       <<"\",\"replica_reproduces_native\":"<<reproduced<<",\"replica_total\":"<<Num(o.total)<<",\"replica_error\":"<<Num(o.error)
       <<",\"allowance\":"<<Num(1e-9L*std::abs(o.total)+1e-12L);
      // Median integrand (1025 uniform chord parameters) and the jump across every cut (one-sided at +-1e-12 and +-1e-9).
      std::vector<long double> samples;for(int k=0;k<=1024;++k)samples.push_back(f.F(k/1024.L));
      auto sorted=samples;std::nth_element(sorted.begin(),sorted.begin()+512,sorted.end());const auto fmed=sorted[512];
      j<<",\"fmed\":"<<Num(fmed);
      size_t jumpCuts=0;long double maxCutJump=0;
      for(size_t k=1;k+1<native.size();++k){const long double t=native[k];
        const auto jump=std::abs(f.F(std::min(1.L,t+1e-12L))-f.F(std::max(0.L,t-1e-12L)))/fmed;jumpCuts+=jump>1e-6L;maxCutJump=std::max(maxCutJump,jump);}
      j<<",\"cuts_with_jump\":"<<jumpCuts<<",\"max_cut_jump\":"<<Num(maxCutJump);
      // Residual hot spots: panels carrying 99% of the residual error, clustered by adjacency in the global parameter.
      struct G{long double lo,hi,error;unsigned depth;size_t piece;long double plo,phi;};std::vector<G> res;long double resErr=0;
      for(const auto& p:o.residual){const auto& pc=f.pieces[p.piece];res.push_back({pc.begin+p.low*pc.width,pc.begin+p.high*pc.width,p.error,p.depth,p.piece,p.low,p.high});resErr+=p.error;}
      std::sort(res.begin(),res.end(),[](const G& x,const G& y){return x.error>y.error;});
      long double acc=0;size_t keep=0;while(keep<res.size() && acc<.99L*resErr){acc+=res[keep].error;++keep;}res.resize(keep);
      std::sort(res.begin(),res.end(),[](const G& x,const G& y){return x.lo<y.lo;});
      struct Cluster{long double lo,hi,error;unsigned maxDepth,minDepth;size_t panels;G top;};std::vector<Cluster> cl;
      for(const auto& r:res){const auto width=r.hi-r.lo;
        if(!cl.empty() && r.lo-cl.back().hi<=4*std::max(width,cl.back().top.hi-cl.back().top.lo)){auto& q=cl.back();q.hi=std::max(q.hi,r.hi);q.error+=r.error;
          q.maxDepth=std::max(q.maxDepth,r.depth);q.minDepth=std::min(q.minDepth,r.depth);++q.panels;if(r.error>q.top.error)q.top=r;}
        else cl.push_back({r.lo,r.hi,r.error,r.depth,r.depth,1,r});}
      std::sort(cl.begin(),cl.end(),[](const Cluster& x,const Cluster& y){return x.error>y.error;});
      j<<",\"residual_panels\":"<<o.residual.size()<<",\"residual_error\":"<<Num(resErr)<<",\"clusters\":"<<cl.size()<<",\"spots\":[";
      for(size_t k=0;k<cl.size() && k<4;++k) {
        const auto& q=cl[k];
        // Flip scan over the cluster span.
        size_t flips=0;long double prev=f.F(q.lo),maxStep=0;
        for(int s=1;s<=512;++s){const auto v=f.F(q.lo+(q.hi-q.lo)*s/512);const auto step=std::abs(v-prev)/fmed;flips+=step>1e-6L;maxStep=std::max(maxStep,step);prev=v;}
        // Bisect the jump inside the top panel (larger-difference half at each step).
        long double L=q.top.lo,R=q.top.hi,fL=f.F(L),fR=f.F(R);
        {long double bestStep=0,bl=L,br=R,pv=fL;for(int s=1;s<=64;++s){const auto t=L+(R-L)*s/64;const auto v=f.F(t);if(std::abs(v-pv)>bestStep){bestStep=std::abs(v-pv);bl=L+(R-L)*(s-1)/64;br=t;}pv=v;}
         L=bl;R=br;fL=f.F(L);fR=f.F(R);}
        for(int s=0;s<200 && R-L>4*std::numeric_limits<long double>::epsilon()*std::max(R,1e-300L);++s){
          const auto M=(L+R)/2;const auto fM=f.F(M);if(std::abs(fM-fL)>=std::abs(fR-fM)){R=M;fR=fM;}else{L=M;fL=fM;}}
        const auto tj=(L+R)/2;long double cutGap=INFINITY,transitionGap=INFINITY;double nearestCut=0;
        for(auto t:native)if(std::abs(t-tj)<cutGap){cutGap=std::abs(t-tj);nearestCut=t;}
        for(auto t:diag.transitions)transitionGap=std::min(transitionGap,std::abs((long double)t-tj));
        unsigned long ownerL,ownerR;std::string sl,sr;
        if(law=="copy"){sl=SideJson(*patched,wall,diag.faces,f,L,ownerL);sr=SideJson(*patched,wall,diag.faces,f,R,ownerR);}
        else{sl=SideJson(*n.layers,wall,diag.faces,f,L,ownerL);sr=SideJson(*n.layers,wall,diag.faces,f,R,ownerR);}
        j<<(k?",":"")<<"{\"lo\":"<<Num(q.lo)<<",\"hi\":"<<Num(q.hi)<<",\"error\":"<<Num(q.error)<<",\"panels\":"<<q.panels<<",\"depth\":["<<q.minDepth<<','<<q.maxDepth
         <<"],\"flips\":"<<flips<<",\"max_step\":"<<Num(maxStep)<<",\"jump_t\":"<<Num(tj)<<",\"jump_width\":"<<Num(R-L)<<",\"jump_rel\":"<<Num(std::abs(fR-fL)/fmed)
         <<",\"nearest_cut\":"<<Num(nearestCut)<<",\"cut_gap\":"<<Num(cutGap)<<",\"transition_gap\":"<<Num(transitionGap)
         <<",\"cut_one_sided\":["<<Num(f.F(std::max(0.L,nearestCut-1e-12L)))<<','<<Num(f.F(std::min(1.L,nearestCut+1e-12L)))<<"]"
         <<",\"left\":"<<sl<<",\"right\":"<<sr;
        if(ownerL!=std::numeric_limits<unsigned long>::max() && ownerR!=std::numeric_limits<unsigned long>::max()) {
          j<<",\"relation\":\""<<Relation(wall,ownerL,ownerR)<<"\",\"dihedral\":"<<Num(Dihedral(wall,ownerL,ownerR));
          // Feet on both owners at the jump, and the owner distance gap along the chord (tie persistence).
          const auto xm=f.X(tj);const auto fa=Nearest(wall,ownerL,xm),fb=Nearest(wall,ownerR,xm);
          j<<",\"feet_gap\":"<<Num(junction_wp0::Norm(fa.foot-fb.foot)/std::max(fa.distance,1e-300L))<<",\"tie\":[";
          bool comma=false;
          for(long double delta:{-1e-3L,-1e-6L,-1e-9L,0.L,1e-9L,1e-6L,1e-3L}){const auto t=std::clamp(tj+delta,0.L,1.L);const auto x=f.X(t);
            const auto da=Nearest(wall,ownerL,x).distance,db=Nearest(wall,ownerR,x).distance;
            j<<(comma?",":"")<<"["<<Num(delta)<<','<<Num((da-db)/std::max({da,db,1e-300L}))<<"]";comma=true;}
          j<<"]";
        }
        j<<"}";
      }
      j<<"]";
    } else {  // "variant": the replica integrator with a test-only cut variant (native pointwise law unchanged).
      const auto cuts=Breakpoints(wall,A,B,Variant{variantId},nullptr);
      const auto o=Integrate(f,cuts);
      j<<",\"variant_cuts\":"<<cuts.size()<<",\"ok\":"<<o.ok<<",\"failure\":\""<<o.failure<<"\",\"evaluations\":"<<o.evaluations<<",\"total\":"<<Num(o.total)<<",\"error\":"<<Num(o.error);
    }
    j<<",\"seconds\":"<<Seconds(ts)<<"}\n";out<<j.str()<<std::flush;
  }
  std::cout<<"D2 "<<stage<<" done in "<<Seconds(t0)<<" s\n";
}
