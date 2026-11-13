// Compiled source fragments with small explicit interfaces; no SU2 heap/FP proof.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
using su2double=double;
using std::min;
#define FORCEINLINE inline
#define SU2_OMP_SIMD
#define SU2_OMP_SIMD_IF_NOT_AD
namespace simd {
template<class T,size_t N> struct Array {
 static constexpr size_t Size=N;
 std::array<T,N> data{};
 Array()=default;
 Array(T value){data.fill(value);}
 T& operator[](size_t i){return data.at(i);}
 const T& operator[](size_t i)const{return data.at(i);}
};
}
using Double=simd::Array<double,2>;
using Int=simd::Array<unsigned long,2>;
struct Flux {
 static constexpr size_t StaticSize=4;
 Double data[4];
 const Double& operator[](size_t i)const{return data[i];}
};
struct Vector {
 using ScalarType=double;
 size_t nVar=4;
 double vec_val[12]{};
#include "unpack.inc"
#include "scatter.inc"
};
struct Nodes {
 double lambda=91,dt=92;
 unsigned NonPhysicalEdgeCounter[3]={1,0,1};
 void SetMax_Lambda_Inv(unsigned long,double v){lambda=v;}
 double GetLocalCFL(unsigned long)const{return 3;}
 double GetMax_Lambda_Inv(unsigned long)const{return lambda;}
 double GetMax_Lambda_Visc(unsigned long)const{return 6;}
 void SetDelta_Time(unsigned long,double v){dt=v;}
};
struct Config {double GetMax_DeltaTime()const{return 1;}};
struct Normal2 {double values[2]{};double& operator()(unsigned long,unsigned c){return values[c];}};
struct CEdge {Normal2 Normal;void SetNodes_Coord(unsigned long,const double*,const double*);};
#include "dualNormal.inc"
enum class UpdateType {REDUCTION,COLORING};
struct Numerics {
 void ComputeFlux(Int e,const Config&,const int&,const Nodes&,UpdateType,Double mask,Vector& out,int&,void*){
  Flux f;Int p,q;
  for(unsigned lane=0;lane<2;++lane){p[lane]=e[lane]%3;q[lane]=(e[lane]+1)%3;for(unsigned c=0;c<4;++c)f.data[c][lane]=e[lane]+c+1;}
  out.UpdateBlocks(p,q,f,mask);
 }
};
static void require(bool b,const char* name){if(!b)throw std::runtime_error(name);}
int main(){try{
 Config cfg;Config* config=&cfg;Nodes data;Nodes* nodes=&data;unsigned long iPoint=1;
#include "resetLambda.inc"
 require(nodes->lambda==0,"spectral reset");nodes->lambda+=4;double Vol=2;
#include "localTime.inc"
 require(Local_Delta_Time==1.5,"inviscid timestep getter");
#include "timeStore.inc"
 require(nodes->dt==1,"timestep cap");
 CEdge edge;double a[2]={1,2},b[2]={3,5};edge.SetNodes_Coord(0,a,b);
 require(edge.Normal.values[0]==3&&edge.Normal.values[1]==-2,"oriented dual normal");
 int geom=0;int* geometry=&geom;int Jacobian=0;void* massFluxes=nullptr;
 constexpr unsigned MGLevel=0,MESH_0=0;bool ReducerStrategy=false;unsigned long counterLocal=0;
 Vector LinSysRes,EdgeFluxes;Numerics num;Numerics* edgeNumerics=&num;
 unsigned long indices[3]={0,1,2};struct{unsigned long size;unsigned long* indices;}color{3,indices};
#include "packetTraversal.inc"
 double expected[12]{};
 for(unsigned e=0;e<3;++e)for(unsigned c=0;c<4;++c){expected[(e%3)*4+c]+=e+c+1;expected[((e+1)%3)*4+c]-=e+c+1;}
 for(unsigned i=0;i<12;++i)require(LinSysRes.vec_val[i]==expected[i],"packet residual assembly");
 require(counterLocal==3,"inactive gather diagnostic count");
 std::cout<<"PASS numerical source-fragment contract\n";return 0;
 }catch(const std::exception& e){std::cerr<<"contract failed: "<<e.what()<<'\n';return 1;}}
