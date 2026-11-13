// Unchanged pinned method bodies in mock storage: native evidence, not C++ correspondence.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using su2double = double;
#define SU2_OMP_FOR_STAT(x)
#define END_SU2_OMP_FOR
#include "copyLoop.inc"
struct SysVector {
  using ScalarType=double;
  std::array<double,12> x;
  explicit SysVector(double v) { x.fill(v); }
  SysVector& operator=(double v) { x.fill(v); return *this; }
#include "residualZero.inc"
};
struct Matrix {
  unsigned long rows,cols,addressStride;
  std::vector<double> x;
  Matrix(unsigned long r,unsigned long c): rows(r),cols(c),addressStride(c),x(r*c,90) {}
  double& operator()(unsigned long r,unsigned long c) { return x.at(r*addressStride+c); }
  double operator()(unsigned long r,unsigned long c) const { return x.at(r*addressStride+c); }
  size_t size() const { return x.size(); }
  double* data() { return x.data(); }
};
struct CVariable {
  unsigned long nPoint=3,nDim=2,nVar=4;
  Matrix Solution{3,4},Solution_Old{3,4};
  void Set_OldSolution();
};
#include "snapshot.inc"
struct CFluidModel {
  double Gamma=2, Gamma_Minus_One=1, Gas_Constant=1, Cv=1;
  double Density=0,StaticEnergy=0,dPdrho_e=0,dPde_rho=0,Pressure=0,Temperature=0,SoundSpeed2=0,Entropy=0;
  double dTde_rho=1;
  bool ComputeEntropy=true;
  void SetTDState_rhoe(double rho,double e)
#include "idealGas_body.inc"
  double GetPressure() const { return Pressure; }
  double GetSoundSpeed2() const { return SoundSpeed2; }
  double GetTemperature() const { return Temperature; }
  double GetExtrapolation() const { return 0; }
  double GetEntropy() const { return Entropy; }
};
struct Indices {
  unsigned long Velocity()const{return 1;} unsigned long Pressure()const{return 3;}
  unsigned long Enthalpy()const{return 5;} unsigned long Temperature()const{return 0;}
  unsigned long Density()const{return 4;}
};
struct CEulerVariable: CVariable {
  Matrix Primitive{3,11};
  Indices indices;
  std::array<double,3> velocity2{};
  bool DataDrivenFluid=false;
  double& Velocity2(unsigned long p) { return velocity2.at(p); }
  void constructor(double density,const double* velocity,double energy) {
#include "constructor.inc"
  }
  void SetVelocity(unsigned long iPoint)
#include "velocity_body.inc"
  bool SetDensity(unsigned long iPoint)
#include "density_body.inc"
  bool SetPressure(unsigned long iPoint,double pressure)
#include "pressure_body.inc"
  bool SetSoundSpeed(unsigned long iPoint,double soundspeed2)
#include "sound_body.inc"
  bool SetTemperature(unsigned long iPoint,double temperature)
#include "temperature_body.inc"
  void SetEnthalpy(unsigned long iPoint)
#include "enthalpy_body.inc"
  double GetDensity(unsigned long iPoint)const
#include "getDensity_body.inc"
  double GetEnergy(unsigned long iPoint)const
#include "getEnergy_body.inc"
  void SetSecondaryVar(unsigned long,CFluidModel*){}
  void SetDataExtrapolation(unsigned long,double){}
  void SetEntropy(unsigned long,double){}
  bool SetPrimVar(unsigned long iPoint,CFluidModel* FluidModel);
};
#include "setPrim.inc"
#define SU2_ZONE_SCOPED
namespace AD { static void StartNoSharedReading(){} static void EndNoSharedReading(){} }
struct CConfig {};
struct CSolver {};
struct CEulerSolver {
 CEulerVariable* nodes;
 CFluidModel* fluid;
 unsigned long nPoint=3;
 CFluidModel* GetFluidModel(){return fluid;}
 unsigned long SetPrimitive_Variables(CSolver**,const CConfig*);
};
#include "primitiveLoop.inc"
static unsigned checks=0;
static void require(bool ok,const char* s) { ++checks; if(!ok) throw std::runtime_error(s); }
static void close(double a,double b,const char* s) {
  require(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<1e-11*(1+std::abs(b)),s);
}
using U=std::array<double,4>;
static double pressure(U u) { return u[3]-(u[1]*u[1]+u[2]*u[2])/(2*u[0]); }
int main(int argc,char** argv) {
 try {
  require(argc==2,"variant argument");
  const std::string variant=argv[1];
  SysVector residual(91),truncation(92);
  if(variant=="wrongResidual")truncation.SetValZero();else residual.SetValZero();
  for(unsigned i=0;i<12;++i) {
    close(residual.x[i],0,"residual clearing");
    close(truncation.x[i],92,"truncation frame");
  }
  CEulerVariable v;
  double vel[3]={3,4,0};
  v.constructor(2,vel,15);
  U seed{2,6,8,30};
  for(unsigned p=0;p<3;++p)for(unsigned c=0;c<4;++c) {
    close(v.Solution(p,c),seed[c],"conservative initialization");
    close(v.Solution_Old(p,c),seed[c],"constructor old copy");
  }
  const std::array<U,6> states{{U{1,2,0,3},U{1,2,0,1},U{1,0,0,0},U{-1,0,0,2},U{0,0,0,2},U{2,6,8,30}}};
  CFluidModel gas;
  for(U u:states) {
   for(unsigned p=0;p<3;++p) {
    for(unsigned c=0;c<4;++c) { v.Solution(p,c)=u[c]; v.Solution_Old(p,c)=seed[c]; }
    for(unsigned c=0;c<11;++c)v.Primitive(p,c)=100+c+20*p;
    bool good=u[0]>0 && pressure(u)>0;
    U selected=good?u:seed;
    double pExpected=pressure(selected);
    bool actual=v.SetPrimVar(p,&gas);
    require(actual==good,"physical/fallback flag");
    for(unsigned c=0;c<4;++c) {
      close(v.Solution(p,c),selected[c],"selected conservative state");
      close(v.Solution_Old(p,c),seed[c],"backup preserved by conversion");
    }
    close(v.Primitive(p,0),pExpected/selected[0],"temperature");
    close(v.Primitive(p,1),selected[1]/selected[0],"velocity x");
    close(v.Primitive(p,2),selected[2]/selected[0],"velocity y");
    close(v.Primitive(p,3),pExpected,"pressure");
    close(v.Primitive(p,4),selected[0],"density");
    close(v.Primitive(p,5),(selected[3]+pExpected)/selected[0],"enthalpy");
    close(v.Primitive(p,6),std::sqrt(2*pExpected/selected[0]),"sound speed");
    close(v.Velocity2(p),(selected[1]*selected[1]+selected[2]*selected[2])/(selected[0]*selected[0]),"velocity scratch");
    for(unsigned c=7;c<11;++c)close(v.Primitive(p,c),100+c+20*p,"untouched primitive tail");
   }
   // Production snapshots the corrected current state AFTER SetPrimVar fallback.
   v.Set_OldSolution();
   for(unsigned p=0;p<3;++p)for(unsigned c=0;c<4;++c)close(v.Solution_Old(p,c),v.Solution(p,c),"complete old snapshot");
  }
  // Final-row witness distinguishes incomplete and wrong-buffer copies.
  v.Solution(2,3)=123; v.Solution_Old(2,3)=90;
  v.Set_OldSolution();
  close(v.Solution_Old(2,3),123,"last snapshot component");
  // Complete caller test: rows 0,1 are owned and row 2 is a halo witness.
  for(unsigned p=0;p<3;++p) {
   U trial=p==1?U{1,2,0,3}:U{1,2,0,1};
   for(unsigned c=0;c<4;++c){v.Solution(p,c)=trial[c];v.Solution_Old(p,c)=seed[c];}
   for(unsigned c=0;c<11;++c)v.Primitive(p,c)=100+20*p+c;
   v.velocity2[p]=9;
  }
  if(variant=="badStride")v.Primitive.addressStride=6;
  CEulerSolver solver{&v,&gas};
  if(variant=="earlySnapshot")v.Set_OldSolution();
  unsigned long nonphysical=solver.SetPrimitive_Variables(nullptr,nullptr);
  require(nonphysical==2,"owned and halo counter");
  for(unsigned p=0;p<3;++p) {
   U expected=p==1?U{1,2,0,3}:seed;
   for(unsigned c=0;c<4;++c)close(v.Solution(p,c),expected[c],"complete-loop fallback");
   double pr=pressure(expected);
   close(v.Primitive.x.at(p*11+3),pr,"eleven-column physical stride");
   for(unsigned c=7;c<11;++c)close(v.Primitive.x.at(p*11+c),100+20*p+c,"complete-loop tail frame");
  }
  close(gas.Entropy,std::log(2.5)-std::log(2.0),"enabled entropy scratch");
  v.Set_OldSolution();
  for(unsigned p=0;p<3;++p)for(unsigned c=0;c<4;++c)close(v.Solution_Old(p,c),v.Solution(p,c),"post-correction snapshot");
  std::cout<<"PASS native source fragments checks="<<checks<<" variants="<<variant<<" rows=3 primitive_stride=11 cases=6\n";
  return 0;
 } catch(const std::exception& e) { std::cerr<<"contract failed: "<<e.what()<<'\n'; return 1; }
}
