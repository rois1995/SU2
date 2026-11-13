// Actual pinned SU2 methods; source-model/native evidence, not C++ or IEEE proof.
#include "UnitTests/UnitQuadTestCase.hpp"
#include "SU2_CFD/include/integration/CIntegration.hpp"
#include "SU2_CFD/include/numerics_simd/CNumericsSIMD.hpp"
#include "SU2_CFD/include/numerics/flow/convection/roe.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
struct Integration : CIntegration { using CIntegration::Space_Integration; };
using U=std::array<double,4>;
using P=std::array<double,11>;
using V=std::array<double,2>;
static unsigned long checks=0;
static bool finiteValue(double x){uint64_t bits;std::memcpy(&bits,&x,8);return (bits&0x7ff0000000000000ULL)!=0x7ff0000000000000ULL;}
static void require(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
static void close(double a,double b,const char* s){require(finiteValue(a)&&finiteValue(b)&&std::abs(a-b)<2e-9*(1+std::abs(b)),s);}
static double pressure(U u,double g){return (g-1)*(u[3]-(u[1]*u[1]+u[2]*u[2])/(2*u[0]));}
static U read(const su2double* x){return {x[0],x[1],x[2],x[3]};}
static bool admissible(U x,double g){for(double v:x)if(!finiteValue(v))return false;return x[0]>0&&pressure(x,g)>0;}
static U conservative(double rho,double vx,double vy,double p,double g){return {rho,rho*vx,rho*vy,p/(g-1)+rho*(vx*vx+vy*vy)/2};}
// Literal SIMD arithmetic model: no scalar-Roe abs-ratio or c2<=0 shortcut.
static U referenceFlux(P L,P R,V normal,double gamma){
 double area=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]);V n{normal[0]/area,normal[1]/area};
 double ratio=std::sqrt(R[4]/L[4]),inverse=1/(ratio+1),rho=ratio*L[4];
 V v{(ratio*R[1]+L[1])*inverse,(ratio*R[2]+L[2])*inverse};
 double H=(ratio*R[5]+L[5])*inverse,k=(v[0]*v[0]+v[1]*v[1])/2;
 double c=std::sqrt((gamma-1)*(H-k)),q=v[0]*n[0]+v[1]*n[1],r2=rho/2,rc=r2/c;
 double pm[4][4]={{1,0,rc,rc},{v[0],rho*n[1],rc*v[0]+r2*n[0],rc*v[0]-r2*n[0]},
 {v[1],-rho*n[0],rc*v[1]+r2*n[1],rc*v[1]-r2*n[1]},
 {k,rho*(v[0]*n[1]-v[1]*n[0]),rc*k+r2*(q+c/(gamma-1)),rc*k-r2*(q-c/(gamma-1))}};
 double tmp=(gamma-1)/(c*c),ir=1/rho,gc=(gamma-1)/(rho*c);
 double qm[4][4]={{1-tmp*k,tmp*v[0],tmp*v[1],-tmp},{(n[0]*v[1]-n[1]*v[0])*ir,n[1]*ir,-n[0]*ir,0},
 {-q*ir+gc*k,n[0]*ir-gc*v[0],n[1]*ir-gc*v[1],gc},
 {q*ir+gc*k,-n[0]*ir-gc*v[0],-n[1]*ir-gc*v[1],gc}};
 U ul{L[4],L[4]*L[1],L[4]*L[2],L[4]*L[5]-L[3]},ur{R[4],R[4]*R[1],R[4]*R[2],R[4]*R[5]-R[3]};
 auto physical=[&](P p,U u){double m=u[1]*normal[0]+u[2]*normal[1];return U{m,m*p[1]+normal[0]*p[3],m*p[2]+normal[1]*p[3],m*p[5]};};
 U fi=physical(L,ul),fj=physical(R,ur),f{},lambda{std::abs(q),std::abs(q),std::abs(q+c),std::abs(q-c)};
 for(unsigned i=0;i<4;++i){f[i]=(fi[i]+fj[i])/2;for(unsigned j=0;j<4;++j){double a=0;for(unsigned h=0;h<4;++h)a+=pm[i][h]*lambda[h]*qm[h][j];f[i]-=a*(1-0.5)*area*(ur[j]-ul[j]);}}
 return f;
}
int main(int argc,char** argv){
 SU2_MPI::Init(&argc,&argv);
 try{
  require(argc>=2,"configuration required");bool robustness=argc>2&&std::string(argv[2])=="robustness";
  UnitQuadTestCase test;std::ifstream file(argv[1]);require(bool(file),"read configuration");
  test.config_options=std::string(std::istreambuf_iterator<char>(file),{});test.InitConfig();test.InitGeometry();test.InitSolver();
  auto* cfg=test.config.get();auto* geo=test.geometry.get();auto* solver=dynamic_cast<CEulerSolver*>(test.solver[FLOW_SOL]);require(solver,"Euler solver");auto* nodes=solver->GetNodes();
  cfg->SetGlobalParam(MAIN_SOLVER::EULER,RUNTIME_FLOW_SYS);geo->ComputeModifiedSymmetryNormals(cfg);
  const auto np=geo->GetnPoint(),ne=geo->GetnEdge();double gamma=cfg->GetGamma();
  require(geo->GetnDim()==2&&solver->GetnVar()==4&&solver->GetnPrimVar()==11,"selected layout");
  require(np==20&&np==geo->GetnPointDomain()&&ne==43,"pinned all-owned triangle fixture");
  require(!cfg->GetViscous()&&!cfg->GetMUSCL_Flow()&&!cfg->GetDynamic_Grid(),"selected inviscid stationary first order");
  require(cfg->GetKind_TimeIntScheme()==EULER_EXPLICIT&&cfg->GetTime_Marching()==TIME_MARCHING::STEADY,"steady explicit pseudo-time");
  require(cfg->GetRoe_Kappa()==0.5&&cfg->GetEntropyFix_Coeff()==0,"selected Roe parameters");
  for(unsigned long p=0;p<np;++p){require(admissible(read(nodes->GetSolution(p)),gamma),"admissible actual initialization");close(nodes->GetLocalCFL(p),cfg->GetCFL(0),"executed initial CFL stores");}
  auto color=geo->GetEdgeColoring();require(color.getOuterSize()==1&&color.getNumNonZeros(0)==ne,"one serial natural color");
  for(unsigned long e=0;e<ne;++e)require(color.innerIdx(0)[e]==e,"iota edge identity");
  std::vector<unsigned> incidents(ne,0);double totalVolume=0;
  for(unsigned long p=0;p<np;++p){require(geo->nodes->GetVolume(p)>0,"positive actual dual volume");totalVolume+=geo->nodes->GetVolume(p);
   for(unsigned k=0;k<geo->nodes->GetnPoint(p);++k){auto q=geo->nodes->GetPoint(p,k);auto e=geo->nodes->GetEdge(p,k);require(e>=0&&static_cast<unsigned long>(e)<ne&&q<np,"adjacency bounds");require((geo->edges->GetNode(e,0)==p&&geo->edges->GetNode(e,1)==q)||(geo->edges->GetNode(e,1)==p&&geo->edges->GetNode(e,0)==q),"opposite endpoint incidence");++incidents[e];}}
  close(totalVolume,1,"actual assembled dual volume");
  for(unsigned long e=0;e<ne;++e){require(incidents[e]==2,"twice-adjacent edge coverage");require(geo->edges->GetNode(e,0)<geo->edges->GetNode(e,1),"ordered endpoints");auto n=geo->edges->GetNormal(e);require(n[0]*n[0]+n[1]*n[1]>0,"actual nonzero edge normal");}
  {CEdge edge(1,2);edge.SetZeroValues();double a[2]={0.5,0},b[2]={1.0/3,1.0/3},p[2]={0,0};edge.SetNodes_Coord(0,a,b);auto n=edge.GetNormal(0);close(n[0],b[1]-a[1],"median normal x");close(n[1],a[0]-b[0],"median normal y");close(CEdge::GetVolume(p,a,b),1.0/12,"actual dual triangle helper");}
  require(!cfg->GetBody_Force()&&!cfg->GetRotating_Frame()&&!cfg->GetAxisymmetric()&&!cfg->GetGravityForce()&&!cfg->GetVorticityConfinement()&&!cfg->GetPyCustomSource(),"disabled source guards");
  require(cfg->GetVerification_Solution()==VERIFICATION_SOLUTION::NONE&&!cfg->GetBoolGiles()&&cfg->GetnMarker_Periodic()==0,"disabled verification Giles periodic branches");
  for(unsigned marker=0;marker<cfg->GetnMarker_All();++marker)require(cfg->GetMarker_All_KindBC(marker)==FAR_FIELD||cfg->GetMarker_All_KindBC(marker)==EULER_WALL,"selected marker dispatch");
  if(robustness){
   CUpwRoe_Flow farNumerics(2,4,cfg,false);unsigned firstBadOld=0,badTrials=0;bool failed=false;
   Integration integration;CNumerics* numerics[MAX_TERMS]={};numerics[CONV_BOUND_TERM]=&farNumerics;
   auto inspectOld=[&](unsigned iteration,const char* stage){for(unsigned long p=0;p<np;++p){U u=read(nodes->GetSolution_Old(p));if(admissible(u,gamma))continue;if(!firstBadOld)firstBadOld=iteration+1;std::cout<<"OLD_WITNESS iteration="<<iteration+1<<" stage="<<stage<<" point="<<p<<" bits=";for(double v:u){uint64_t bits;std::memcpy(&bits,&v,8);std::cout<<std::hex<<bits<<',';}std::cout<<std::dec<<'\n';failed=true;return;}};
   for(unsigned iteration=0;iteration<100;++iteration){
    inspectOld(iteration,"before_preprocessing");if(failed)break;
    solver->Preprocessing(geo,test.solver,cfg,0,0,RUNTIME_FLOW_SYS,false);
    badTrials+=cfg->GetNonphysical_Points();solver->Set_OldSolution();
    inspectOld(iteration,"after_snapshot");
    if(failed)break;
    solver->SetTime_Step(geo,test.solver,cfg,0,iteration);
    integration.Space_Integration(geo,test.solver,numerics,cfg,0,0,RUNTIME_FLOW_SYS);
    inspectOld(iteration,"after_actual_space_integration");
    if(failed)break;
    solver->ExplicitEuler_Iteration(geo,test.solver,cfg);
   }
   std::cout<<"ROBUSTNESS cfl="<<cfg->GetCFL(0)<<" first_inadmissible_old_iteration="<<firstBadOld<<" rejected_trials="<<badTrials<<" checks="<<checks<<'\n';
   SU2_MPI::Finalize();return 0;
  }
  for(unsigned long p=0;p<np;++p){U u=conservative(1+0.01*p,0.7+0.005*p,0.12,1+0.03*p,gamma);for(unsigned c=0;c<4;++c){nodes->GetSolution(p)[c]=u[c];nodes->GetSolution_Old(p)[c]=u[c];nodes->GetResTruncError(p)[c]=0.0125*(p+c+1);}}
  solver->Preprocessing(geo,test.solver,cfg,0,0,RUNTIME_FLOW_SYS,false);solver->Set_OldSolution();
  std::vector<P> primitive(np);std::vector<U> current(np),old(np),trunc(np);std::vector<double> volume(np),dt(np),cfl(np),expectedLambda(np);
  for(unsigned long p=0;p<np;++p){for(unsigned c=0;c<11;++c)primitive[p][c]=nodes->GetPrimitive(p)[c];current[p]=read(nodes->GetSolution(p));old[p]=read(nodes->GetSolution_Old(p));trunc[p]=read(nodes->GetResTruncError(p));volume[p]=geo->nodes->GetVolume(p);cfl[p]=nodes->GetLocalCFL(p);nodes->SetMax_Lambda_Inv(p,91);nodes->SetDelta_Time(p,92);}
  auto projected=[&](unsigned long p,const double* n){return primitive[p][1]*n[0]+primitive[p][2]*n[1];};
  for(unsigned long p=0;p<np;++p)for(unsigned k=0;k<geo->nodes->GetnPoint(p);++k){auto q=geo->nodes->GetPoint(p,k);auto e=geo->nodes->GetEdge(p,k);auto n=geo->edges->GetNormal(e);expectedLambda[p]+=std::abs((projected(p,n)+projected(q,n))/2)+(primitive[p][6]+primitive[q][6])/2*std::hypot(n[0],n[1]);}
  unsigned farVertices=0;
  for(unsigned marker=0;marker<cfg->GetnMarker_All();++marker)if(cfg->GetMarker_All_KindBC(marker)==FAR_FIELD)for(unsigned long v=0;v<geo->nVertex[marker];++v){auto p=geo->vertex[marker][v]->GetNode();if(!geo->nodes->GetDomain(p))continue;auto n=geo->vertex[marker][v]->GetNormal();expectedLambda[p]+=std::abs(projected(p,n))+primitive[p][6]*std::hypot(n[0],n[1]);++farVertices;}
  double physical=cfg->GetDelta_UnstTimeND();solver->SetTime_Step(geo,test.solver,cfg,0,0);close(cfg->GetDelta_UnstTimeND(),physical,"physical timestep unchanged in steady path");
  double minRaw=1e30,maxRaw=0;
  for(unsigned long p=0;p<np;++p){double raw=cfl[p]*volume[p]/expectedLambda[p];minRaw=std::min(minRaw,raw);maxRaw=std::max(maxRaw,raw);dt[p]=std::min(raw,cfg->GetMax_DeltaTime());close(nodes->GetMax_Lambda_Inv(p),expectedLambda[p],"computed spectral sum");close(nodes->GetDelta_Time(p),dt[p],"computed and retrieved local timestep");}
  close(solver->GetMin_Delta_Time(),minRaw,"uncapped minimum diagnostic");close(solver->GetMax_Delta_Time(),maxRaw,"uncapped maximum diagnostic");
  if(cfg->GetMax_DeltaTime()<1e-6)require(minRaw>cfg->GetMax_DeltaTime(),"cap branch exercised separately from diagnostics");
  geo->nodes->SetVolume(np-1,0);solver->SetTime_Step(geo,test.solver,cfg,0,0);close(nodes->GetDelta_Time(np-1),0,"zero-volume branch");geo->nodes->SetVolume(np-1,volume.back());solver->SetTime_Step(geo,test.solver,cfg,0,0);
  std::unique_ptr<CNumericsSIMD> kernel(CNumericsSIMD::CreateNumerics(*cfg,2,0));require(bool(kernel),"actual SIMD Roe factory");
  std::vector<U> expected(np);SparseMatrixType matrix;
  for(unsigned long e=0;e<ne;++e){auto p=geo->edges->GetNode(e,0),q=geo->edges->GetNode(e,1);auto n=geo->edges->GetNormal(e);auto f=referenceFlux(primitive[p],primitive[q],{n[0],n[1]},gamma);
   solver->LinSysRes.SetValZero();Int ids=e;Double mask=0.0;mask[0]=1;kernel->ComputeFlux(ids,*cfg,*geo,*nodes,UpdateType::COLORING,mask,solver->LinSysRes,matrix,nullptr);
   for(unsigned c=0;c<4;++c){close(solver->LinSysRes(p,c),f[c],"actual SIMD flux vs provisional arithmetic model");close(solver->LinSysRes(q,c),-f[c],"oriented endpoint subtraction");expected[p][c]+=f[c];expected[q][c]-=f[c];}
   kernel->ComputeFlux(ids,*cfg,*geo,*nodes,UpdateType::COLORING,Double(0.0),solver->LinSysRes,matrix,nullptr);
   for(unsigned c=0;c<4;++c)close(solver->LinSysRes(p,c),f[c],"inactive lane performs no residual write");
  }
  solver->LinSysRes.SetValZero();solver->Upwind_Residual(geo,test.solver,nullptr,cfg,0);
  CNumerics* noSources[MAX_TERMS]={};
  static_cast<CSolver*>(solver)->Viscous_Residual(geo,test.solver,noSources,cfg,0,0);
  solver->Source_Residual(geo,test.solver,noSources,cfg,0);
  solver->BC_Fluid_Interface(geo,test.solver,nullptr,nullptr,cfg);
  for(unsigned c=0;c<4;++c){double total=0;for(unsigned long p=0;p<np;++p){close(solver->LinSysRes(p,c),expected[p][c],"complete interior residual from actual traversal");total+=solver->LinSysRes(p,c);}close(total,0,"all-allocated-point interior conservation");}
  for(unsigned long p=0;p<np;++p){for(unsigned c=0;c<4;++c){close(nodes->GetSolution(p)[c],current[p][c],"current frame");close(nodes->GetSolution_Old(p)[c],old[p][c],"old frame");close(nodes->GetResTruncError(p)[c],trunc[p][c],"truncation frame");}for(unsigned c=0;c<11;++c)close(nodes->GetPrimitive(p)[c],primitive[p][c],"primitive row frame");close(nodes->GetDelta_Time(p),dt[p],"interior timestep frame");close(geo->nodes->GetVolume(p),volume[p],"volume frame");close(nodes->GetLocalCFL(p),cfl[p],"CFL frame");}
  require(cfg->GetNonphysical_Reconstr()==0,"first-order edge counter remains zero");
  std::cout<<"PASS actual interior checks="<<checks<<" points="<<np<<" edges="<<ne<<" simd_width="<<Double::Size<<" far_vertices="<<farVertices<<" cap="<<cfg->GetMax_DeltaTime()<<'\n';
  SU2_MPI::Finalize();return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';SU2_MPI::Finalize();return 1;}
}
