// Exact selected fragments in a small fixture. Object layout and dispatch are mocked;
// the separate boundary_native test calls the actual production implementations.
#include <array>
#include <cmath>
#include <iostream>
using su2double=double;
constexpr unsigned nDim=2,nVar=4,nPointDomain=3,iVel=1;
struct Matrix {
  double v[3][4]{};
  double& operator()(unsigned long i,unsigned long j){return v[i][j];}
  double operator()(unsigned long i,unsigned long j)const{return v[i][j];}
};
struct Vector {
  double v[3]{};
  double& operator()(unsigned long i){return v[i];}
  double operator()(unsigned long i)const{return v[i];}
};
struct Points {
  Vector Volume,Periodic_Volume;
#include "volume.inc"
#include "periodic.inc"
};
struct Variables {
  Vector Delta_Time,LocalCFL;
  Matrix Solution,Solution_Old,Trunc;
  double* GetResTruncError(unsigned long p){return Trunc.v[p];}
#include "timestep.inc"
#include "update.inc"
};
struct Residual : Matrix {
  void AddBlock(unsigned long p,const std::array<double,4>& r){for(unsigned c=0;c<4;++c)v[p][c]+=r[c];}
  void SubtractBlock(unsigned long p,const std::array<double,4>& r){for(unsigned c=0;c<4;++c)v[p][c]-=r[c];}
};
struct Geometry {Points* nodes;};
struct Identity {
#define FORCEINLINE inline
#include "apply.inc"
};
enum {EULER_EXPLICIT};
int main(){
  Variables variables;auto* nodes=&variables;
  Points points;Geometry geom{&points};auto* geometry=&geom;
  Residual LinSysRes;
  for(unsigned long p=0;p<nPointDomain;++p)for(unsigned c=0;c<nVar;++c){
    variables.Solution(p,c)=99;variables.Solution_Old(p,c)=2+p+c;
    variables.Trunc(p,c)=0.125*(c+1);LinSysRes(p,c)=0.25*(p+c+1);
  }
  for(unsigned p=0;p<nPointDomain;++p){points.Volume(p)=1+p;variables.Delta_Time(p)=0.1*(p+1);variables.LocalCFL(p)=9;}
  unsigned long iPoint=1;
  std::array<double,4> residual{1,2,3,4};
  auto prior=LinSysRes;
#include "farAdd.inc"
  for(unsigned c=0;c<4;++c)if(LinSysRes(iPoint,c)!=prior(iPoint,c)+residual[c]){
    std::cerr<<"contract failed: far-field addition\n";return 1;
  }
  double UnitNormal[2]={0.6,0.8},gridVel[2]={};
  auto expectedOld=variables.Solution_Old,expectedTrunc=variables.Trunc;
  auto expectedRes=LinSysRes;
  auto project=[&](Matrix& a){double q=a(iPoint,1)*0.6+a(iPoint,2)*0.8;a(iPoint,1)-=q*0.6;a(iPoint,2)-=q*0.8;};
  project(expectedOld);project(expectedRes);project(expectedTrunc);
  auto* solutionOld=variables.Solution_Old.v[iPoint];
#include "wallOld.inc"
#include "wallRes.inc"
#include "wallTrunc.inc"
  auto same=[](double a,double b){return std::abs(a-b)<1e-12;};
  for(unsigned p=0;p<nPointDomain;++p)for(unsigned c=0;c<4;++c){
    if(!same(variables.Solution_Old(p,c),expectedOld(p,c)) || !same(LinSysRes(p,c),expectedRes(p,c)) ||
       !same(variables.Trunc(p,c),expectedTrunc(p,c)) || variables.Solution(p,c)!=99){
      std::cerr<<"contract failed: three-buffer projection/frame\n";return 1;
    }
  }
  Matrix expected=variables.Solution;
  for(unsigned p=0;p<nPointDomain;++p)for(unsigned c=0;c<4;++c)
    expected(p,c)=variables.Solution_Old(p,c)-variables.Delta_Time(p)/points.Volume(p)*(LinSysRes(p,c)+variables.Trunc(p,c));
  Identity preconditioner;
#include "points.inc"
#include "scale.inc"
    const auto* Residual=LinSysRes.v[iPoint];
    const auto* Res_TruncError=nodes->GetResTruncError(iPoint);
#include "components.inc"
      double Res=preconditioner.apply(iVar,Residual,Res_TruncError);
      switch(EULER_EXPLICIT){
#include "call.inc"
      }
    }
  }
  for(unsigned p=0;p<nPointDomain;++p)for(unsigned c=0;c<4;++c){
    if(!same(variables.Solution(p,c),expected(p,c)) || !same(variables.Solution_Old(p,c),expectedOld(p,c)) ||
       !same(LinSysRes(p,c),expectedRes(p,c)) || !same(variables.Trunc(p,c),expectedTrunc(p,c))){
      std::cerr<<"contract failed: complete update/frame\n";return 1;
    }
  }
  std::cout<<"PASS selected boundary/getter/loop fragments, 3 points x 4 components\n";
}
