// BL_REMESH_PLAN rev 3, DC-6: TEST-ONLY [NativeWPEDC6] microbenchmark of the WP-E enclosures (screen, not a
// certificate: long double without the outward-rounded primitives of WP-E (a); exact-arithmetic formulas of WP-E).
// Field: full M6 WING3D wall (Coarse.su2 facets, h0 2e-6, g 1.2, T 0.01), isotropic proxy sensor/core 1/0.02^2,
// production composition plus the test-local D1 law of DC-5 (DC-1 feature export). Authoritative length:
// FrozenField::TargetEdgeLength (native integrator). Reports wrong decisions, decided fraction and us/evaluation.
#include <random>
namespace wpe_dc6 {
using namespace layer2d;
struct WallTri { std::array<V,3> p;V n;long double Lmin=0,Lmax=0;V lo,hi; };
struct Model {
  std::vector<WallTri> tri;std::vector<std::vector<size_t>> grid;V glo,ghi;long double cell=0;size_t nx=0,ny=0,nz=0;
  double h0=2e-6,g=1.2,T=0.01,core=2500;std::shared_ptr<const D1Law> d1;
  double full() const {return std::max(h0,0.9*T);} double width() const {return T-full();}
  long double hn(long double d) const {return std::max<long double>(h0,2*(h0+(g-1)*d)/(g+1));}
  long double weight(long double d) const {const long double s=std::clamp((d-full())/width(),0.L,1.L);return 1-s*s*(3-2*s);}
  size_t Index(long double x,long double y,long double z) const {
    auto c=[&](long double v,long double lo,size_t n){return std::min(n-1,size_t(std::max(0.L,(v-lo)/cell)));};
    return (c(z,glo.z,nz)*ny+c(y,glo.y,ny))*nx+c(x,glo.x,nx);}
  template<class F> void Near(V p,long double r,F f) const {
    std::set<size_t> seen;
    auto c=[&](long double v,long double lo,size_t n){return std::min<long>(long(n)-1,std::max<long>(0,long((v-lo)/cell)));};
    for(long k=c(p.z-r,glo.z,nz);k<=c(p.z+r,glo.z,nz);++k)for(long j=c(p.y-r,glo.y,ny);j<=c(p.y+r,glo.y,ny);++j)for(long i=c(p.x-r,glo.x,nx);i<=c(p.x+r,glo.x,nx);++i)
      for(size_t t:grid[(size_t(k)*ny+size_t(j))*nx+size_t(i)])if(seen.insert(t).second){const auto& w=tri[t];
        if(w.lo.x<=p.x+r&&w.hi.x>=p.x-r&&w.lo.y<=p.y+r&&w.hi.y>=p.y-r&&w.lo.z<=p.z+r&&w.hi.z>=p.z-r)f(t);}
  }
  // exact nearest distance and the fluid-side sign (normals oriented into the fluid)
  std::pair<long double,long double> Distance(V p) const {
    long double best=INFINITY,side=0;
    for(long double r=cell;best==INFINITY||r<2*best;r*=2){Near(p,r,[&](size_t t){const auto& w=tri[t];const V q=Closest(p,w.p[0],w.p[1],w.p[2]);const auto d=Norm(p-q);
      if(d<best){best=d;side=Dot(p-q,w.n);}});if(r>1e3)break;}
    return {best,side};
  }
};
// Coarse.su2 reader: WING3D triangles oriented into the fluid by their volume cell, SYMMETRY nodes snapped to y = 0.
std::shared_ptr<Model> LoadM6(const std::string& path,std::vector<CBoundaryLayerMetric::Wall>& walls) {
  std::ifstream in(path);if(!in)throw std::runtime_error("DC6 missing M6 mesh");
  std::vector<std::array<unsigned long,4>> cells;std::vector<std::array<double,3>> P;std::map<std::string,std::vector<std::array<unsigned long,3>>> mk;
  std::string line;
  while(std::getline(in,line)) {
    if(line.rfind("NELEM=",0)==0){const size_t n=std::stoul(line.substr(6));cells.resize(n);for(auto& c:cells){int type;in>>type>>c[0]>>c[1]>>c[2]>>c[3];std::getline(in,line);}}
    else if(line.rfind("NPOIN=",0)==0){const size_t n=std::stoul(line.substr(6));P.resize(n);for(auto& p:P){in>>p[0]>>p[1]>>p[2];std::getline(in,line);}}
    else if(line.rfind("MARKER_TAG=",0)==0){std::string name=line.substr(11);name.erase(0,name.find_first_not_of(' '));name.erase(name.find_last_not_of(" \r")+1);
      std::getline(in,line);const size_t n=std::stoul(line.substr(line.find('=')+1));auto& v=mk[name];v.resize(n);for(auto& f:v){int type;in>>type>>f[0]>>f[1]>>f[2];std::getline(in,line);}}
  }
  for(const auto& f:mk.at("SYMMETRY"))for(auto v:f)P[v][1]=0.0;
  std::map<std::array<unsigned long,3>,unsigned long> opposite;
  std::set<std::array<unsigned long,3>> wallKeys;for(auto f:mk.at("WING3D")){std::sort(f.begin(),f.end());wallKeys.insert(f);}
  for(const auto& c:cells)for(unsigned k=0;k<4;++k){std::array<unsigned long,3> f;unsigned j=0;for(unsigned m=0;m<4;++m)if(m!=k)f[j++]=c[m];std::sort(f.begin(),f.end());
    if(wallKeys.count(f))opposite[f]=c[k];}
  auto model=std::make_shared<Model>();CBoundaryLayerMetric::Wall w;w.layer.marker="WING3D";w.layer.firstHeight=model->h0;w.layer.growth=model->g;w.layer.thickness=model->T;
  std::map<unsigned long,unsigned long> index;
  model->glo={INFINITY,INFINITY,INFINITY};model->ghi={-INFINITY,-INFINITY,-INFINITY};
  for(auto f:mk.at("WING3D")) {
    auto key=f;std::sort(key.begin(),key.end());const auto o=P[opposite.at(key)];
    V a{P[f[0]][0],P[f[0]][1],P[f[0]][2]},b{P[f[1]][0],P[f[1]][1],P[f[1]][2]},c{P[f[2]][0],P[f[2]][1],P[f[2]][2]};
    V n=Unit(Cross(b-a,c-a));if(Dot(V{o[0],o[1],o[2]}-a,n)<0){std::swap(f[1],f[2]);std::swap(b,c);n=-n;}
    WallTri t;t.p={a,b,c};t.n=n;
    // face covariance 2/3 sum e e^T projected: tangential eigenvalues (size^2) -> log bracket of the vertex tensors
    long double C[3][3]={{0}};const V e[3]={b-a,c-b,a-c};
    for(const auto& x:e){const long double v[3]={x.x,x.y,x.z};for(int i=0;i<3;++i)for(int j=0;j<3;++j)C[i][j]+=2.L/3*v[i]*v[j];}
    V u=Unit(b-a),v2=Unit(Cross(n,u));const long double U[3]={u.x,u.y,u.z},W2[3]={v2.x,v2.y,v2.z};
    long double aa=0,bb=0,cc=0;for(int i=0;i<3;++i)for(int j=0;j<3;++j){aa+=U[i]*C[i][j]*U[j];bb+=U[i]*C[i][j]*W2[j];cc+=W2[i]*C[i][j]*W2[j];}
    const auto tr=aa+cc,det=aa*cc-bb*bb,disc=std::sqrt(std::max(0.L,tr*tr/4-det));
    t.Lmin=std::log(tr/2-disc);t.Lmax=std::log(tr/2+disc);
    t.lo={std::min({a.x,b.x,c.x}),std::min({a.y,b.y,c.y}),std::min({a.z,b.z,c.z})};t.hi={std::max({a.x,b.x,c.x}),std::max({a.y,b.y,c.y}),std::max({a.z,b.z,c.z})};
    model->glo={std::min(model->glo.x,t.lo.x),std::min(model->glo.y,t.lo.y),std::min(model->glo.z,t.lo.z)};
    model->ghi={std::max(model->ghi.x,t.hi.x),std::max(model->ghi.y,t.hi.y),std::max(model->ghi.z,t.hi.z)};
    model->tri.push_back(t);
    for(auto id:f){const auto ins=index.emplace(id,index.size());if(ins.second){w.coord.push_back(P[id][0]);w.coord.push_back(P[id][1]);w.coord.push_back(P[id][2]);}w.conn.push_back(ins.first->second);}
    w.conn.push_back(w.conn.back());
  }
  // vertex-tensor bracket: the eigenvalues of each vertex log-mean lie in the hull of its incident faces' log
  // eigenvalues (rotations keep spectra; the normal eigenvalue equals the face's largest tangential one)
  std::map<unsigned long,std::pair<long double,long double>> vb;
  {size_t k=0;for(auto f:mk.at("WING3D")){for(auto id:f){auto& r=vb.emplace(id,std::make_pair(INFINITY,-INFINITY)).first->second;r.first=std::min(r.first,model->tri[k].Lmin);r.second=std::max(r.second,model->tri[k].Lmax);}++k;}}
  {size_t k=0;for(auto f:mk.at("WING3D")){long double lo=INFINITY,hi=-INFINITY;for(auto id:f){lo=std::min(lo,vb.at(id).first);hi=std::max(hi,vb.at(id).second);}model->tri[k].Lmin=lo;model->tri[k].Lmax=hi;++k;}}
  model->cell=0.02;model->glo=model->glo-V{0.05,0.05,0.05};model->ghi=model->ghi+V{0.05,0.05,0.05};
  model->nx=size_t((model->ghi.x-model->glo.x)/model->cell)+1;model->ny=size_t((model->ghi.y-model->glo.y)/model->cell)+1;model->nz=size_t((model->ghi.z-model->glo.z)/model->cell)+1;
  model->grid.assign(model->nx*model->ny*model->nz,{});
  for(size_t t=0;t<model->tri.size();++t){const auto& x=model->tri[t];
    for(size_t k=model->Index(0,0,x.lo.z)/(model->nx*model->ny);k<=model->Index(0,0,x.hi.z)/(model->nx*model->ny);++k)
      for(size_t j=(model->Index(0,x.lo.y,0)/model->nx)%model->ny;j<=(model->Index(0,x.hi.y,0)/model->nx)%model->ny;++j)
        for(size_t i=model->Index(x.lo.x,0,0)%model->nx;i<=model->Index(x.hi.x,0,0)%model->nx;++i)model->grid[(k*model->ny+j)*model->nx+i].push_back(t);}
  walls.push_back(std::move(w));return model;
}
// Directional bound of one piece: e^T M_final e in [lo, hi] (exact-arithmetic WP-E formulas, long double).
struct Bound { long double lo=0,hi=0;bool ok=true; };
Bound PieceBound(const Model& m,V a,V e,long double t0,long double t1) {
  Bound out;const auto len2=Dot(e,e);const V mid=a+(0.5L*(t0+t1))*e;const auto half=std::sqrt(len2)*(t1-t0)/2;const V eh=(1/std::sqrt(len2))*e;
  const auto dm=m.Distance(mid).first;const long double dlo=std::max(0.L,dm-half),dhi=dm+half;
  const long double c=m.core;
  if(dlo>=m.T){out.lo=out.hi=c*len2;return out;}
  // candidate foot triangles: within dhi of every point => within dhi+half of the midpoint
  long double Lmin=INFINITY,Lmax=-INFINITY;std::vector<V> normals;
  m.Near(mid,dhi+half,[&](size_t t){const auto& w=m.tri[t];const auto d=Norm(mid-Closest(mid,w.p[0],w.p[1],w.p[2]));if(d<=dhi+half){Lmin=std::min(Lmin,w.Lmin);Lmax=std::max(Lmax,w.Lmax);normals.push_back(w.n);}});
  if(normals.empty()){out.ok=false;return out;}
  V n0;for(const auto& n:normals)n0=n0+n;if(!(Norm(n0)>1e-12L)){out.ok=false;return out;}n0=Unit(n0);
  long double theta=0;for(const auto& n:normals)theta=std::max(theta,std::acos(std::clamp(Dot(n,n0),-1.L,1.L)));
  const auto a0=std::acos(std::clamp(Dot(eh,n0),-1.L,1.L));
  long double dlo_=std::cos(std::min<long double>(M_PI,a0+theta)),dhi_=std::cos(std::max<long double>(0,a0-theta));
  long double en2lo=(dlo_<=0&&dhi_>=0)?0:std::min(dlo_*dlo_,dhi_*dhi_),en2hi=std::max(dlo_*dlo_,dhi_*dhi_);   // (ê·n)^2
  const long double mn_lo=1/std::pow(m.hn(dhi),2),mn_hi=1/std::pow(m.hn(dlo),2),mt_lo=std::exp(-Lmax),mt_hi=std::exp(-Lmin);
  const long double wlo=m.weight(dhi),whi=m.weight(dlo);
  auto fade_lo=[&](long double mu){return std::min(c*std::pow(mu/c,wlo),c*std::pow(mu/c,whi));};
  auto fade_hi=[&](long double mu){return std::max(c*std::pow(mu/c,wlo),c*std::pow(mu/c,whi));};
  // M_BL^w directional (frame-exact): sum over normal + tangential spectral parts
  long double bl_lo=fade_lo(mn_lo)*en2lo+fade_lo(mt_lo)*(1-en2hi),bl_hi=fade_hi(mn_hi)*en2hi+fade_hi(mt_hi)*(1-en2lo);
  long double lo=bl_lo,hi=bl_hi;
  if(m.d1) {
    // D1: lower max(M_BL^w, F_i^w) over features active on the whole piece; upper c^(1-w)(ê(M_BL+sum F)ê)^w
    long double sumF=0;long double xb_hi=mn_hi*en2hi+mt_hi*(1-en2lo);
    for(const auto& s:m.d1->segments) {
      const V t=s.b-s.a;const auto L=Norm(t);const V u=(1/L)*t;const auto sp=std::clamp(Dot(mid-s.a,u),0.L,L);const auto rho=Norm(mid-(s.a+sp*u));
      const long double rlo=std::max(0.L,rho-half),rhi=rho+half;if(rlo>m.d1->Rf)continue;
      const long double lam_hi=1/std::pow((long double)m.d1->hnf(rlo),2),lam_lo=1/std::pow((long double)m.d1->hnf(rhi),2);
      const auto et=Dot(eh,u);const long double perp=std::max(0.L,1-et*et);
      // perp is exact for a straight segment (fixed t); sum F upper
      sumF+=lam_hi*perp;
      if(rhi<=m.d1->Rf)lo=std::max(lo,fade_lo(lam_lo)*perp);
    }
    for(const auto& k:m.d1->corners){const auto rho=Norm(mid-k.c);const long double rlo=std::max(0.L,rho-half),rhi=rho+half;if(rlo>m.d1->Rf)continue;
      sumF+=1/std::pow((long double)m.d1->hnf(rlo),2);if(rhi<=m.d1->Rf)lo=std::max(lo,fade_lo(1/std::pow((long double)m.d1->hnf(rhi),2)));}
    if(sumF>0)hi=std::max(fade_hi(xb_hi+sumF),bl_hi);
  }
  // intersection with the isotropic proxy sensor c I: A,B <= A∩B <= A+B
  out.lo=std::max(lo,c)*len2;out.hi=(hi+c)*len2;return out;
}
struct Cert { long double lo=0,hi=0;size_t pieces=0;bool decided=false,ok=true; };
Cert Certify(const Model& m,V a,V b,size_t budget) {
  const V e=b-a;std::vector<std::pair<long double,long double>> pieces{{0.L,1.L}};std::vector<Bound> bounds{PieceBound(m,a,e,0,1)};Cert c;c.pieces=1;
  for(;;) {
    long double lo=0,hi=0;size_t worst=0;long double gap=-1;bool ok=true;
    for(size_t i=0;i<pieces.size();++i){const auto dt=pieces[i].second-pieces[i].first;ok=ok&&bounds[i].ok;
      const auto l=dt*std::sqrt(bounds[i].lo),h=dt*std::sqrt(bounds[i].hi);lo+=l;hi+=h;if(h-l>gap){gap=h-l;worst=i;}}
    c.lo=lo;c.hi=hi;c.ok=ok;
    if(ok&&(hi<=MaximumMetricEdge||lo>MaximumMetricEdge)){c.decided=true;return c;}
    if(c.pieces>=budget)return c;
    const auto p=pieces[worst];const auto mid=0.5L*(p.first+p.second);
    pieces[worst]={p.first,mid};bounds[worst]=PieceBound(m,a,e,p.first,mid);pieces.push_back({mid,p.second});bounds.push_back(PieceBound(m,a,e,mid,p.second));c.pieces+=2;
  }
}
}  // namespace wpe_dc6

TEST_CASE("DC-6 WP-E enclosure screen on native M6 edges", "[.][NativeWPEDC6]") {
  using namespace wpe_dc6;using namespace layer2d;
  const char* mesh=std::getenv("SU2_DC6_MESH");REQUIRE(mesh);const char* d1file=std::getenv("SU2_DC6_D1");
  const char* outdir=std::getenv("SU2_DC6_OUT");REQUIRE(outdir);
  const size_t count=std::getenv("SU2_DC6_EDGES")?std::stoul(std::getenv("SU2_DC6_EDGES")):20000;
  const size_t budget=std::getenv("SU2_DC6_BUDGET")?std::stoul(std::getenv("SU2_DC6_BUDGET")):64;
  std::vector<CBoundaryLayerMetric::Wall> walls;auto model=LoadM6(mesh,walls);
  if(d1file)model->d1=LoadD1(d1file);
  auto layers=std::make_shared<CBoundaryLayerMetric>(3,std::move(walls));
  // proxy field: six covering donors with the uniform isotropic core sensor (as junction_wp0::LoadCase)
  std::vector<DonorCell> donors;{V lo=model->glo-V{1,1,1},hi=model->ghi+V{1,1,1};const Tensor sensor{model->core,0,0,model->core,0,model->core};
    auto corner=[&](unsigned b){return Node{Id(1)<<40|b,{double(b&1?hi.x:lo.x),double(b&2?hi.y:lo.y),double(b&4?hi.z:lo.z)}};};
    const std::array<std::array<unsigned,3>,6> paths{{{1,2,4},{1,4,2},{2,1,4},{2,4,1},{4,1,2},{4,2,1}}};
    for(size_t k=0;k<6;++k){unsigned at=0;DonorCell d;d.cell.id=k;d.cell.v[0]=corner(0);for(unsigned j=0;j<3;++j){at|=paths[k][j];d.cell.v[j+1]=corner(at);}
      if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0)std::swap(d.cell.v[0],d.cell.v[1]);d.sensor.fill(sensor);donors.push_back(d);}}
  FrozenField field(donors,model->d1?ComposeD1(layers,model->core,model->d1,nullptr):Compose(layers,model->core),false,std::vector<Id>{},Bands(layers));
  // deterministic edge sample in BL support: area-weighted wall point, log-uniform offset in [h0/10, T], random
  // direction, length scaled so that its local metric length is uniform in [0.3, 3]; both ends in the fluid.
  std::mt19937_64 rng(20261022);std::uniform_real_distribution<double> U(0,1);
  std::vector<long double> area;long double total=0;for(const auto& t:model->tri){total+=Norm(Cross(t.p[1]-t.p[0],t.p[2]-t.p[0]))/2;area.push_back(total);}
  std::ofstream out(std::string(outdir)+"/dc6_edges.csv");out<<std::setprecision(17)<<"ax,ay,az,bx,by,bz,native,lo,hi,pieces,decided,cert_us,native_us\n";
  size_t wrong=0,decided=0,done=0,enclosure_miss=0,undecided_ok=0;double cert_us=0,native_us=0;
  while(done<count) {
    const auto r=U(rng)*total;const size_t t=std::lower_bound(area.begin(),area.end(),r)-area.begin();const auto& w=model->tri[t];
    long double s1=U(rng),s2=U(rng);if(s1+s2>1){s1=1-s1;s2=1-s2;}const V foot=w.p[0]+s1*(w.p[1]-w.p[0])+s2*(w.p[2]-w.p[0]);
    const long double d=model->h0/10*std::exp(U(rng)*std::log(model->T*10/model->h0));const V a=foot+d*w.n;
    V dir{U(rng)-.5,U(rng)-.5,U(rng)-.5};if(!(Norm(dir)>1e-6))continue;dir=Unit(dir);
    Tensor M;try{M=field.Query(Pt(a)).target;}catch(const std::exception&){continue;}
    const auto q=dir.x*(M.xx*dir.x+M.xy*dir.y+M.xz*dir.z)+dir.y*(M.xy*dir.x+M.yy*dir.y+M.yz*dir.z)+dir.z*(M.xz*dir.x+M.yz*dir.y+M.zz*dir.z);
    const long double L=(0.3+2.7*U(rng))/std::sqrt(q);const V b=a+L*dir;
    // domain: fluid side of the wing and the y >= 0 half-space of the SYMMETRY plane
    bool fluid=a.y>=0&&b.y>=0;for(int k=0;k<=10&&fluid;++k){const auto ds=model->Distance(a+(k/10.L)*(b-a));fluid=ds.second>0;}
    if(!fluid)continue;
    auto t0=std::chrono::steady_clock::now();long double native=0;try{native=field.TargetEdgeLength(Pt(a),Pt(b));}catch(const std::exception&){continue;}
    auto t1=std::chrono::steady_clock::now();const auto c=Certify(*model,a,b,budget);auto t2=std::chrono::steady_clock::now();
    const double nu=std::chrono::duration<double,std::micro>(t1-t0).count(),cu=std::chrono::duration<double,std::micro>(t2-t1).count();
    native_us+=nu;cert_us+=cu;++done;
    if(c.decided){++decided;const bool certLE=c.hi<=MaximumMetricEdge;if(certLE!=(native<=MaximumMetricEdge))++wrong;}
    if(c.ok&&(native<c.lo*(1-1e-9L)||native>c.hi*(1+1e-9L)))++enclosure_miss;
    out<<double(a.x)<<','<<double(a.y)<<','<<double(a.z)<<','<<double(b.x)<<','<<double(b.y)<<','<<double(b.z)<<','<<double(native)<<','<<double(c.lo)<<','<<double(c.hi)<<','<<c.pieces<<','<<c.decided<<','<<cu<<','<<nu<<'\n';
  }
  std::ofstream summary(std::string(outdir)+"/dc6_summary.json");summary<<std::setprecision(12)<<"{\"edges\":"<<done<<",\"d1\":"<<(model->d1?"true":"false")<<",\"piece_budget\":"<<budget
    <<",\"decided\":"<<decided<<",\"decided_fraction\":"<<double(decided)/done<<",\"wrong_decisions\":"<<wrong<<",\"enclosure_misses\":"<<enclosure_miss
    <<",\"certificate_us_per_edge\":"<<cert_us/done<<",\"native_us_per_edge\":"<<native_us/done<<",\"screen_not_certificate\":true}\n";
  CHECK(wrong==0);
}
