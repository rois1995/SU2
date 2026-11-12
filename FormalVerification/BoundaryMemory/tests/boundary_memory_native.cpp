// Native evidence for the manually represented interfaces; not a proof of C++ semantics.
#include "UnitTests/UnitQuadTestCase.hpp"
#include "SU2_CFD/include/numerics/flow/convection/roe.hpp"
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using V = std::array<double, 2>;
using U = std::array<double, 4>;
using P = std::array<double, 6>; // temperature, vx, vy, pressure, density, enthalpy
static unsigned long checks = 0;
static void require(bool ok, const char* label) { if (!ok) throw std::runtime_error(label); ++checks; }
static void close(double a, double b, const char* label) {
  require(std::isfinite(a) && std::isfinite(b) && std::abs(a-b) <= 2e-10*(1+std::abs(b)), label);
}
static double dot(V a, V b) { return a[0]*b[0]+a[1]*b[1]; }
static P primitive(double r, V v, double p, double g, double gas) {
  return {p/(gas*r), v[0], v[1], p, r, p/((g-1)*r)+dot(v,v)/2+p/r};
}
static U conservative(P w) { return {w[4], w[4]*w[1], w[4]*w[2], w[4]*w[5]-w[3]}; }
static P farState(P in, P free, V n, double g, double gas) {
  V vi{in[1],in[2]}, vf{free[1],free[2]};
  double qi=dot(vi,n), q=dot(vf,n), ci=std::sqrt(g*in[3]/in[4]), cf=std::sqrt(g*free[3]/free[4]);
  double rp=q > -cf ? qi+2*ci/(g-1) : q+2*cf/(g-1);
  double rm=q > cf ? qi-2*ci/(g-1) : q-2*cf/(g-1);
  double vn=(rp+rm)/2, c=(rp-rm)*(g-1)/4;
  P w=q > 0 ? in : free;
  V v{w[1],w[2]}; double old=dot(v,n);
  for (unsigned j=0;j<2;++j) v[j]+=(vn-old)*n[j];
  double r=std::pow(std::pow(w[4],g)/w[3]*c*c/g,1/(g-1));
  return primitive(r,v,r*c*c/g,g,gas);
}
static U flux(P w, V normal) {
  double q=dot({w[1],w[2]},normal);
  return {w[4]*q,w[4]*q*w[1]+w[3]*normal[0],w[4]*q*w[2]+w[3]*normal[1],w[4]*w[5]*q};
}
static U roe(P l, P rr, V n, double area, double g) {
  double ratio=std::sqrt(std::abs(rr[4]/l[4])), r=ratio*l[4];
  V v{(ratio*rr[1]+l[1])/(ratio+1),(ratio*rr[2]+l[2])/(ratio+1)};
  double H=(ratio*rr[5]+l[5])/(ratio+1), k=dot(v,v)/2, c2=(g-1)*(H-k);
  if (c2<=0) return {};
  double c=std::sqrt(c2), q=dot(v,n), a=(g-1)/(r*c), b=(g-1)/c2;
  double p[4][4]={{1,0,r/c/2,r/c/2},
    {v[0],r*n[1],(v[0]*r/c+n[0]*r)/2,(v[0]*r/c-n[0]*r)/2},
    {v[1],-r*n[0],(v[1]*r/c+n[1]*r)/2,(v[1]*r/c-n[1]*r)/2},
    {k,r*(v[0]*n[1]-v[1]*n[0]),(k*r/c+r*q+r*c/(g-1))/2,(k*r/c-r*q+r*c/(g-1))/2}};
  double B[4][4]={{1-b*k,b*v[0],b*v[1],-b},
    {-n[1]/r*v[0]+n[0]/r*v[1],n[1]/r,-n[0]/r,0},
    {-q/r+a*k,n[0]/r-a*v[0],n[1]/r-a*v[1],a},
    {q/r+a*k,-n[0]/r-a*v[0],-n[1]/r-a*v[1],a}};
  U lambda{std::abs(q),std::abs(q),std::abs(q+c),std::abs(q-c)};
  auto ul=conservative(l), ur=conservative(rr), fl=flux(l,{area*n[0],area*n[1]}), fr=flux(rr,{area*n[0],area*n[1]});
  U f{};
  for(unsigned i=0;i<4;++i) {
    double diss=0;
    for(unsigned j=0;j<4;++j) {
      double A=0;
      for(unsigned h=0;h<4;++h) A+=p[i][h]*lambda[h]*B[h][j];
      diss+=A*(ur[j]-ul[j]);
    }
    f[i]=(fl[i]+fr[i])/2-area*diss/2;
  }
  return f;
}
static void project(U& u,V n) { double q=u[1]*n[0]+u[2]*n[1]; u[1]-=q*n[0];u[2]-=q*n[1]; }
static U read(const su2double* u) { return {u[0],u[1],u[2],u[3]}; }

int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);
  try {
    require(argc==2,"configuration argument");
    UnitQuadTestCase test;
    std::ifstream cfg(argv[1]); require(bool(cfg),"configuration readable");
    test.config_options=std::string(std::istreambuf_iterator<char>(cfg),{});
    test.InitConfig();test.InitGeometry();test.InitSolver();
    auto* conf=test.config.get(); auto* geo=test.geometry.get();
    auto* solver=dynamic_cast<CEulerSolver*>(test.solver[FLOW_SOL]);
    require(solver!=nullptr,"Euler solver");
    auto* nodes=solver->GetNodes();
    conf->SetGlobalParam(MAIN_SOLVER::EULER,RUNTIME_FLOW_SYS);
    geo->ComputeModifiedSymmetryNormals(conf);
    require(conf->GetKind_TimeIntScheme()==EULER_EXPLICIT,"explicit scheme selected");
    require(geo->GetnDim()==2 && solver->GetnVar()==4,"2D conservative layout");
    require(geo->GetnPoint()==geo->GetnPointDomain(),"serial all-owned fixture");
    require(solver->GetnPrimVar()==11,"eleven primitive/cache columns");
    const auto count=geo->GetnPoint();
    double g=conf->GetGamma(),gas=conf->GetGas_ConstantND();
    CUpwRoe_Flow numerics(2,4,conf,false);
    std::vector<U> current(count),old(count),res(count),trunc(count);
    std::vector<P> inside(count);
    std::vector<double> volume(count),timestep(count);
    unsigned farVisits=0,wallVisits=0;
    double cf=std::sqrt(g*solver->GetPressure_Inf()/solver->GetDensity_Inf());
    const std::array<double,7> regimes{-2,-1,-0.4,0,0.4,1,2};
    for(double speed:regimes) {
      solver->GetVelocity_Inf()[0]=speed*cf;
      solver->GetVelocity_Inf()[1]=0.1*cf;
      P free=primitive(solver->GetDensity_Inf(),{speed*cf,0.1*cf},solver->GetPressure_Inf(),g,gas);
      for(unsigned long p=0;p<count;++p) {
        inside[p]=primitive(free[4]*(1+0.01*p),{0.15*cf+0.01*p,0.08*cf},free[3]*(1+0.02*p),g,gas);
        auto u=conservative(inside[p]);
        for(unsigned c=0;c<6;++c)nodes->SetPrimitive(p,c,inside[p][c]);
        for(unsigned c=6;c<11;++c)nodes->SetPrimitive(p,c,1000+100*p+c);
        for(unsigned c=0;c<4;++c) {
          current[p][c]=u[c]+0.125*(c+1);
          old[p][c]=u[c];res[p][c]=0.1*(p+1)*(c+1);trunc[p][c]=0.03*(p+2)*(c+1);
          nodes->GetSolution(p)[c]=current[p][c];nodes->GetSolution_Old(p)[c]=old[p][c];
          nodes->GetResTruncError(p)[c]=trunc[p][c];solver->LinSysRes(p,c)=res[p][c];
        }
        timestep[p]=0.0001*(p+1);volume[p]=geo->nodes->GetVolume(p);
        nodes->SetDelta_Time(p,timestep[p]);
        close(geo->nodes->GetPeriodicVolume(p),0,"periodic volume initialized zero");
        require(geo->nodes->GetVolume(p)>0,"positive physical volume");
      }
      // One explicit ownership-mask experiment; this does not claim MPI coverage.
      geo->nodes->SetDomain(0,speed!=0);
      std::vector<std::vector<std::array<double,11>>> cache(conf->GetnMarker_All());
      for(unsigned marker=0;marker<conf->GetnMarker_All();++marker) {
        cache[marker].resize(geo->nVertex[marker]);
        for(unsigned long vertex=0;vertex<geo->nVertex[marker];++vertex) {
          auto row=solver->GetCharacPrimVar(marker,vertex);
          require(row==solver->GetCharacPrimVar(marker,0)+vertex*11,"actual cache row stride");
          for(unsigned c=0;c<11;++c)row[c]=cache[marker][vertex][c]=-1000.0-100.0*marker-10.0*vertex-c;
        }
      }
      auto checkCache=[&]() {
        for(unsigned marker=0;marker<conf->GetnMarker_All();++marker)
          for(unsigned long vertex=0;vertex<geo->nVertex[marker];++vertex)
            for(unsigned c=0;c<11;++c)
              close(solver->GetCharacPrimVar(marker,vertex)[c],cache[marker][vertex][c],"all cache rows/fields frame");
      };
      auto checkPrimitive=[&]() {
        for(unsigned long p=0;p<count;++p) {
          require(nodes->GetPrimitive(p)==nodes->GetPrimitive(0)+p*11,"actual primitive row stride");
          close(nodes->GetDelta_Time(p),timestep[p],"timestep getter frame");
          close(geo->nodes->GetVolume(p),volume[p],"volume getter frame");
          close(geo->nodes->GetPeriodicVolume(p),0,"periodic getter frame");
          for(unsigned c=0;c<11;++c)
            close(nodes->GetPrimitive(p)[c],c<6?inside[p][c]:1000+100*p+c,"all primitive fields frame");
        }
      };
      // Two visits through each far marker test accumulation in evolving memory.
      for(unsigned pass=0;pass<2;++pass)
      for(unsigned marker=0;marker<conf->GetnMarker_All();++marker) {
        if(conf->GetMarker_All_KindBC(marker)!=FAR_FIELD) continue;
        for(unsigned long vertex=0;vertex<geo->nVertex[marker];++vertex) {
          auto p=geo->vertex[marker][vertex]->GetNode();
          if(!geo->nodes->GetDomain(p))continue;
          auto normal=geo->vertex[marker][vertex]->GetNormal();
          double area=std::hypot(normal[0],normal[1]); V n{-normal[0]/area,-normal[1]/area};
          auto w=farState(inside[p],free,n,g,gas);auto f=roe(inside[p],w,n,area,g);
          for(unsigned c=0;c<6;++c)cache[marker][vertex][c]=w[c];
          for(unsigned c=0;c<4;++c)res[p][c]+=f[c];
          ++farVisits;
        }
        solver->BC_Far_Field(geo,test.solver,&numerics,nullptr,conf,marker);
        checkCache();checkPrimitive();
      }
      for(unsigned long p=0;p<count;++p)for(unsigned c=0;c<4;++c) {
        close(solver->LinSysRes(p,c),res[p][c],"far-field flux residual");
        close(nodes->GetSolution(p)[c],current[p][c],"far-field current frame");
        close(nodes->GetSolution_Old(p)[c],old[p][c],"far-field old frame");
        close(nodes->GetResTruncError(p)[c],trunc[p][c],"far-field truncation frame");
      }
      for(unsigned marker=0;marker<conf->GetnMarker_All();++marker) {
        if(conf->GetMarker_All_KindBC(marker)!=EULER_WALL)continue;
        for(unsigned long vertex=0;vertex<geo->nVertex[marker];++vertex) {
          auto p=geo->vertex[marker][vertex]->GetNode();auto normal=geo->vertex[marker][vertex]->GetNormal();
          if(!geo->nodes->GetDomain(p))continue;
          double area=std::hypot(normal[0],normal[1]); V n{normal[0]/area,normal[1]/area};
          auto it=geo->symmetryNormals[marker].find(vertex);
          if(it!=geo->symmetryNormals[marker].end())n={it->second[0],it->second[1]};
          project(old[p],n);project(res[p],n);project(trunc[p],n);++wallVisits;
        }
        solver->BC_Euler_Wall(geo,test.solver,&numerics,nullptr,conf,marker);
        checkCache();checkPrimitive();
      }
      for(unsigned long p=0;p<count;++p)for(unsigned c=0;c<4;++c) {
        close(solver->LinSysRes(p,c),res[p][c],"wall residual projection");
        close(nodes->GetSolution_Old(p)[c],old[p][c],"wall old projection");
        close(nodes->GetResTruncError(p)[c],trunc[p][c],"wall truncation projection");
        close(nodes->GetSolution(p)[c],current[p][c],"wall current frame");
      }
      geo->nodes->SetDomain(0,true);
      solver->ExplicitEuler_Iteration(geo,test.solver,conf);
      checkCache();checkPrimitive();
      for(unsigned long p=0;p<count;++p)for(unsigned c=0;c<4;++c) {
        double expected=old[p][c]-timestep[p]/volume[p]*(res[p][c]+trunc[p][c]);
        close(nodes->GetSolution(p)[c],expected,"complete explicit Euler update");
        close(nodes->GetSolution_Old(p)[c],old[p][c],"update old frame");
        close(nodes->GetResTruncError(p)[c],trunc[p][c],"update truncation frame");
        close(solver->LinSysRes(p,c),res[p][c],"update residual frame");
      }
    }
    require(farVisits>0 && wallVisits>0,"both boundary operations exercised");
    // Exercise matrix entries that axis-aligned mesh normals cannot distinguish.
    const std::array<V,4> directions{{{1,0},{0,1},{0.6,0.8},{-0.8,0.6}}};
    for(auto n:directions)for(unsigned sample=0;sample<25;++sample){
      P l=primitive(1+0.03*sample,{0.2+0.01*sample,-0.3},2+0.04*sample,g,gas);
      P r=primitive(0.9+0.02*sample,{-0.1,0.4-0.01*sample},1.7+0.03*sample,g,gas);
      double area=0.3+0.01*sample,normal[2]={area*n[0],area*n[1]};
      numerics.SetNormal(normal);numerics.SetPrimitive(l.data(),r.data());
      auto actual=numerics.ComputeResidual(conf);auto expected=roe(l,r,n,area,g);
      for(unsigned c=0;c<4;++c)close(actual.residual[c],expected[c],"oblique scalar Roe model");
    }
    // Exercise the scalar Roe nonpositive-sound-speed-squared return explicitly.
    P bad{1,0,0,1,1,-1};double normal[2]={1,0};
    numerics.SetNormal(normal);numerics.SetPrimitive(bad.data(),bad.data());
    auto rejected=numerics.ComputeResidual(conf);
    for(unsigned c=0;c<4;++c)close(rejected.residual[c],0,"nonpositive Roe sound speed branch");
    std::cout<<"PASS checks="<<checks<<" points="<<count<<" far_visits="<<farVisits<<" wall_visits="<<wallVisits<<" regimes=7 far_passes=2 primitive_stride=11 cache_stride=11\n";
  }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
  SU2_MPI::Finalize();return 0;
}
