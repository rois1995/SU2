// Source fragments in a small, explicit object context; not certified C++ translation.
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using su2double=double;
using U=std::array<double,4>;
struct Nodes {
  U old{2,3,4,40}, current{9,8,7,60}, trunc{1,1,0,2};
  double* GetSolution_Old(unsigned long){return old.data();}
  double* GetSolution(unsigned long){return current.data();}
  double* GetResTruncError(unsigned long){return trunc.data();}
};
struct Residual {
  U value{1,5,12,3};
  double& operator()(unsigned long,unsigned short c){return value[c];}
  void AddBlock(unsigned long,const U& f){for(unsigned c=0;c<4;++c)value[c]+=f[c];}
};
static bool equal(double a,double b){return std::abs(a-b)<1e-12*(1+std::abs(b));}
static void require(bool p,const char* label){if(!p)throw std::runtime_error(label);}
static U projection(U u){double d=u[1]*0.6+u[2]*0.8;u[1]-=d*0.6;u[2]-=d*0.8;return u;}
static void wall(Nodes* nodes,Residual& LinSysRes) {
  const unsigned short nDim=2,iVel=1;
  const unsigned long iPoint=0;
  const double UnitNormal[2]={0.6,0.8},gridVel[2]={0,0};
#include "oldBinding.inc"
#include "wallOld.inc"
#include "wallRes.inc"
#include "wallTrunc.inc"
}
struct CacheBase { virtual double* GetCharacPrimVar(unsigned short,unsigned long)=0; };
struct CacheAccess : CacheBase {
  struct Matrix { double* base;unsigned stride;double* operator[](unsigned long v){return base+v*stride;} };
  Matrix CharacPrimVar[2];
  CacheAccess(std::array<std::array<double,22>,2>& rows,unsigned stride)
    : CharacPrimVar{{rows[0].data(),stride},{rows[1].data(),stride}} {}
#include "cacheLookup.inc"
};
static void cache(std::array<std::array<double,22>,2>& rows,unsigned marker,unsigned vertex) {
  const unsigned short nDim=2;
  unsigned short nPrimVar=0,iDim=0;
#include "primitiveExtra.inc"
  (void)nPrimVar;
  CacheAccess access(rows,nPrimVar);
  double* V_infty=access.GetCharacPrimVar(marker,vertex);
  const double Pressure=0.5,Gas_Constant=1,Density=1,Energy=0.625,Velocity[2]={0.5,0};
#include "cacheWrites.inc"
}
int main(int argc,char** argv) {
  try {
    require(argc==2,"control mode");std::string mode=argv[1];
    Nodes nodes; Residual res;
    const auto before=nodes; const auto oldRes=res.value;
    auto old=projection(nodes.old), rr=projection(res.value), tt=projection(nodes.trunc);
    wall(&nodes,res);
    if(mode=="discard") { nodes=before;res.value=oldRes; }
    for(unsigned c=0;c<4;++c) {
      require(equal(nodes.old[c],old[c]),"old projection memory");
      require(equal(nodes.trunc[c],tt[c]),"truncation projection memory");
      require(equal(res.value[c],rr[c]),"residual projection memory");
      require(nodes.current[c]==before.current[c],"current frame");
      double actual=nodes.old[c]-0.1*(res.value[c]+nodes.trunc[c]);
      require(equal(actual,old[c]-0.1*(rr[c]+tt[c])),"boundary to update readiness");
    }
    std::array<std::array<double,22>,2> rows{};
    for(auto& r:rows)r.fill(99);
    cache(rows,1,1);
    const std::array<double,6> expected{0.5,0.5,0,0.5,1,1.125};
    for(unsigned marker=0;marker<2;++marker)for(unsigned vertex=0;vertex<2;++vertex)
      for(unsigned c=0;c<11;++c)
        require(equal(rows[marker][vertex*11+c],marker==1&&vertex==1&&c<6?expected[c]:99),"cache numerical value or frame");
    // Uniform primitive states give physical Roe mass flux 1/2. Both visits are required.
    double mass=0;
    for(unsigned visit=0;visit<(mode=="deduplicate"?1u:2u);++visit)mass+=rows[1][11+4]*rows[1][11+1];
    require(equal(mass,1),"repeated boundary residual accumulation");
    std::cout<<"PASS memory contract\n"<<std::hexfloat;
    for(auto u:{nodes.old,nodes.current,nodes.trunc,res.value})for(auto x:u)std::cout<<x<<' ';
    for(auto r:rows)for(auto x:r)std::cout<<x<<' ';
    std::cout<<mass<<'\n';
  }catch(const std::exception& e){std::cerr<<"contract failed: "<<e.what()<<'\n';return 1;}
}
