// Actual pinned SU2 classes/methods; finite observations, not a C++ semantics proof.
#include "UnitTests/UnitQuadTestCase.hpp"
#include "SU2_CFD/include/fluid/CIdealGas.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using U=std::array<double,4>;
using P=std::array<double,11>;
static unsigned long checks=0;
static bool finiteValue(double x) { uint64_t bits; std::memcpy(&bits,&x,sizeof x); return (bits&0x7ff0000000000000ULL)!=0x7ff0000000000000ULL; }
static void require(bool b,const char* s) { ++checks; if(!b)throw std::runtime_error(s); }
static void close(double a,double b,const char* s) { require(finiteValue(a)&&finiteValue(b)&&std::abs(a-b)<3e-10*(1+std::abs(b)),s); }
static double press(U u,double g) { return (g-1)*(u[3]-(u[1]*u[1]+u[2]*u[2])/(2*u[0])); }
static U cons(double r,double vx,double vy,double p,double g) { return {r,r*vx,r*vy,p/(g-1)+r*(vx*vx+vy*vy)/2}; }
int main(int argc,char** argv) {
 SU2_MPI::Init(&argc,&argv);
 try {
  require(argc==2,"configuration required");
  UnitQuadTestCase test;
  std::ifstream cfg(argv[1]); require(bool(cfg),"config readable");
  test.config_options=std::string(std::istreambuf_iterator<char>(cfg),{});
  test.InitConfig();test.InitGeometry();test.InitSolver();
  auto* conf=test.config.get();auto* geo=test.geometry.get();
  auto* solver=dynamic_cast<CEulerSolver*>(test.solver[FLOW_SOL]);require(solver!=nullptr,"Euler solver");
  conf->SetGlobalParam(MAIN_SOLVER::EULER,RUNTIME_FLOW_SYS);
  auto* nodes=solver->GetNodes();auto* fluid=solver->GetFluidModel();
  const auto count=geo->GetnPoint();
  require(count>1 && count==geo->GetnPointDomain(),"serial all-owned actual fixture");
  require(geo->GetnDim()==2 && solver->GetnVar()==4 && solver->GetnPrimVar()==11,"selected strides");
  require(!conf->GetMUSCL_Flow(),"first order");
  require(conf->GetComm_Level()==COMM_FULL,"selected counter publication");
  const double gamma=conf->GetGamma(),gas=conf->GetGas_ConstantND();
  // Direct actual constructors establish the inputs modeled by the initialization lemmas.
  const double velocity[3]={3,4,0};
  CEulerVariable ctor(2,velocity,15,3,2,4,conf);
  U seed{2,6,8,30};
  for(unsigned long p=0;p<3;++p) {
   for(unsigned c=0;c<4;++c) {
    close(ctor.GetSolution(p)[c],seed[c],"constructor current");
    close(ctor.GetSolution_Old(p)[c],seed[c],"constructor old");
    close(ctor.GetResTruncError(p)[c],0,"constructor truncation");
   }
   for(unsigned c=0;c<11;++c)close(ctor.GetPrimitive(p)[c],0,"constructor primitive initialization");
   close(ctor.GetVelocity2(p),0,"constructor velocity scratch");
   close(ctor.GetDelta_Time(p),0,"constructor timestep initialization");
  }
  CIdealGas defaultEntropy(2,1);defaultEntropy.SetTDState_rhoe(1,2);
  close(defaultEntropy.GetEntropy(),std::log(2.0),"default entropy enabled");
  close(defaultEntropy.GetCp(),2,"Cp constructor");close(defaultEntropy.GetCv(),1,"Cv constructor");
  close(defaultEntropy.GetdTde_rho(),1,"temperature derivative constructor");
  std::vector<U> backup(count),selected(count),trunc(count);
  std::vector<P> prior(count);
  std::vector<double> volume(count),periodic(count),dt(count);
  unsigned long rejected=0;
  for(unsigned long p=0;p<count;++p) {
   backup[p]=cons(1+0.02*p,0.3+0.01*p,-0.15,1+0.03*p,gamma);
   U trial=cons(1.1+0.01*p,0.8+0.02*p,0.2,1.4+0.04*p,gamma);
   bool bad=(p%3==0)||(p+1==count);
   if(bad) { trial[3]=0; ++rejected; }
   selected[p]=bad?backup[p]:trial;
   // Give Velocity2 a nonzero prior value before changing the trial: reset must occur.
   auto scratchSeed=cons(1,7,8,1,gamma);
   for(unsigned c=0;c<4;++c)nodes->GetSolution(p)[c]=scratchSeed[c];
   nodes->SetVelocity(p);
   for(unsigned c=0;c<4;++c) {
    nodes->GetSolution(p)[c]=trial[c];nodes->GetSolution_Old(p)[c]=backup[p][c];
    solver->LinSysRes(p,c)=91+p+c;
    trunc[p][c]=0.125*(p+c+1);nodes->GetResTruncError(p)[c]=trunc[p][c];
   }
   for(unsigned c=0;c<11;++c) { prior[p][c]=1000+100*p+c;nodes->SetPrimitive(p,c,prior[p][c]); }
   volume[p]=geo->nodes->GetVolume(p);periodic[p]=geo->nodes->GetPeriodicVolume(p);dt[p]=nodes->GetDelta_Time(p);
  }
  std::vector<std::vector<P>> caches(conf->GetnMarker_All());
  for(unsigned k=0;k<caches.size();++k) {
   caches[k].resize(geo->GetnVertex(k));
   for(unsigned long v=0;v<caches[k].size();++v)for(unsigned c=0;c<11;++c) {
    caches[k][v][c]=20000+1000*k+20*v+c;solver->GetCharacPrimVar(k,v)[c]=caches[k][v][c];
   }
  }
  double cp=fluid->GetCp(),cv=fluid->GetCv(),derivative=fluid->GetdTde_rho();
  conf->SetNonphysical_Points(999);
  solver->Preprocessing(geo,test.solver,conf,MESH_0,0,RUNTIME_FLOW_SYS,false);
  require(conf->GetNonphysical_Points()==rejected,"all-point nonphysical counter");
  for(unsigned long p=0;p<count;++p) {
   U u=selected[p];double pr=press(u,gamma),temperature=pr/(u[0]*gas);
   for(unsigned c=0;c<4;++c) {
    close(nodes->GetSolution(p)[c],u[c],"selected current");
    close(nodes->GetSolution_Old(p)[c],backup[p][c],"old preserved before snapshot");
    close(solver->LinSysRes(p,c),0,"residual cleared");
    close(nodes->GetResTruncError(p)[c],trunc[p][c],"truncation frame");
   }
   const double expected[7]={temperature,u[1]/u[0],u[2]/u[0],pr,u[0],(u[3]+pr)/u[0],std::sqrt(gamma*pr/u[0])};
   for(unsigned c=0;c<7;++c)close(nodes->GetPrimitive(p)[c],expected[c],"primitive fields");
   for(unsigned c=7;c<11;++c)close(nodes->GetPrimitive(p)[c],prior[p][c],"primitive tail frame");
   close(nodes->GetVelocity2(p),(u[1]*u[1]+u[2]*u[2])/(u[0]*u[0]),"velocity scratch");
   close(geo->nodes->GetVolume(p),volume[p],"volume frame");
   close(geo->nodes->GetPeriodicVolume(p),periodic[p],"periodic volume frame");
   close(nodes->GetDelta_Time(p),dt[p],"timestep frame before computation");
  }
  U last=selected.back();double e=last[3]/last[0]-(last[1]*last[1]+last[2]*last[2])/(2*last[0]*last[0]);
  double temperature=(gamma-1)/gas*e;
  close(fluid->GetDensity(),last[0],"fluid density");close(fluid->GetStaticEnergy(),e,"fluid static energy");
  close(fluid->GetPressure(),(gamma-1)*last[0]*e,"fluid pressure");
  close(fluid->GetTemperature(),temperature,"fluid temperature");
  close(fluid->GetSoundSpeed2(),gamma*gas*temperature,"fluid sound2");
  close(fluid->GetdPdrho_e(),(gamma-1)*e,"fluid density derivative");
  close(fluid->GetdPde_rho(),(gamma-1)*last[0],"fluid energy derivative");
  close(fluid->GetEntropy(),cv*std::log(temperature)-gas*std::log(last[0]),"fluid entropy enabled and stored");
  close(fluid->GetCp(),cp,"Cp frame");close(fluid->GetCv(),cv,"Cv frame");close(fluid->GetdTde_rho(),derivative,"temperature derivative frame");
  solver->Set_OldSolution();
  for(unsigned long p=0;p<count;++p)for(unsigned c=0;c<4;++c)close(nodes->GetSolution_Old(p)[c],selected[p][c],"snapshot after correction");
  for(unsigned k=0;k<caches.size();++k)for(unsigned long v=0;v<caches[k].size();++v)for(unsigned c=0;c<11;++c)
   close(solver->GetCharacPrimVar(k,v)[c],caches[k][v][c],"entire characteristic cache frame");
  solver->Preprocessing(geo,test.solver,conf,MESH_0,0,RUNTIME_FLOW_SYS,false);
  require(conf->GetNonphysical_Points()==0,"repeated preprocessing resets count");
  // An injected invalid backup demonstrates the explicit missing caller invariant.
  // This is not a reachable production counterexample or real-log/IEEE equivalence claim.
  for(unsigned c=0;c<4;++c) { ctor.GetSolution(0)[c]=U{1,2,0,1}[c];ctor.GetSolution_Old(0)[c]=U{1,2,0,1}[c]; }
  ctor.SetPrimitive(0,6,123);
  bool flag=ctor.SetPrimVar(0,&defaultEntropy);
  require(!flag,"second-invalid return flag");
  close(ctor.GetPrimitive(0)[3],-1,"invalid backup remains invalid");
  close(ctor.GetPrimitive(0)[6],123,"negative sound preserves old slot");
  require(!finiteValue(defaultEntropy.GetEntropy()),"observed invalid logarithm is nonfinite");
  std::cout<<"PASS actual preprocessing checks="<<checks<<" points="<<count<<" rejected="<<rejected
           <<" primitive_stride=11 entropy_default=true invalid_backup_observed=true\n";
  SU2_MPI::Finalize();return 0;
 }catch(const std::exception& e) { std::cerr<<"FAIL "<<e.what()<<'\n';SU2_MPI::Finalize();return 1; }
}
