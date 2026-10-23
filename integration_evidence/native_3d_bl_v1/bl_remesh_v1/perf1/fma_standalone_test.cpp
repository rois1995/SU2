#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <random>
namespace SU2Native3D { long double FusedMultiplyAdd(long double,long double,long double); }
using SU2Native3D::FusedMultiplyAdd;
static std::mt19937_64 rng(12345);
struct B{uint64_t m;uint16_t e;};
static long double make(uint64_t m,int e,bool neg){B b{m|(uint64_t(1)<<63),uint16_t((e&0x7fff)|(neg?0x8000:0))};long double v=0;std::memcpy(&v,&b,10);return v;}
static long double rnd(int lo,int hi){std::uniform_int_distribution<int> d(lo,hi);return make(rng(),16383+d(rng),rng()&1);}
static bool same(long double a,long double b){B x{},y{};std::memcpy(&x,&a,10);std::memcpy(&y,&b,10);
  if(a!=a && b!=b)return true; return x.m==y.m && x.e==y.e;}
static long double ulpstep(long double v,long k){B b{};std::memcpy(&b,&v,10);b.m+=k;long double r=v;std::memcpy(&r,&b,10);return r;}
int main(int argc,char**argv){
  long n=argc>1?atol(argv[1]):1000000;long bad=0,count=0;
  auto check=[&](long double x,long double y,long double z){++count;const auto r=FusedMultiplyAdd(x,y,z),s=std::fma(x,y,z);
    if(!same(r,s)){if(bad<10)printf("MISMATCH %La %La %La -> %La vs %La\n",x,y,z,r,s);++bad;}};
  for(long i=0;i<n;++i){
    // 1 random wide
    check(rnd(-60,60),rnd(-60,60),rnd(-120,120));
    // 2 cancellation against the rounded product, shifted and perturbed
    {auto x=rnd(-30,30),y=rnd(-30,30);long double m1=x*y;long k=long(rng()%2001)-1000;
     check(x,y,-ulpstep(m1,k));check(x,y,-ldexpl(m1,int(rng()%141)-70));check(x,y,-m1);
     long double z=-m1;B b{};std::memcpy(&b,&z,10);b.m^=rng()&0xffffffffull;std::memcpy(&z,&b,10);check(x,y,z);}
    // 3 ties: short significands make the product exact; add a half ulp of it (plus or minus tiny)
    {uint64_t mx=(rng()|(uint64_t(1)<<31))&0xffffffffull, my=(rng()|(uint64_t(1)<<31))&0xffffffffull;
     long double x=ldexpl((long double)mx,int(rng()%20)-10),y=ldexpl((long double)my,int(rng()%20)-10);
     if(rng()&1)x=-x;
     long double p=x*y;int e;frexpl(p,&e);long double half=ldexpl(1.0L,e-65);
     check(x,y,half);check(x,y,-half);check(x,y,half+ldexpl(half,-70));check(x,y,half*3);check(x,y,-half*3);
     check(x,y,ldexpl(half,-1));check(x,y,ldexpl(half,1));}
    // 4 squares as used in a*c-b*b
    {auto a=rnd(-4,0),c=rnd(-4,0),bb=std::sqrt(std::fabs(a*c))*(1+ldexpl((long double)(rng()%1000),-60));check(a,c,-bb*bb);check(a,std::fabs(c),-bb*bb);}
  }
  // 5 special and extreme operands
  const long double sp[]={0.0L,-0.0L,1.0L,-1.0L,INFINITY,-INFINITY,NAN,ldexpl(1.0L,-16400),ldexpl(1.0L,-16382),ldexpl(1.0L,16380),
    ldexpl(1.0L,-8200),ldexpl(1.0L,8100),ldexpl(1.0L,-8000),3.0L,ldexpl(1.0L,16000),ldexpl(1.0L,-16000),ldexpl(1.0L,-16128),ldexpl(1.0L,16128)};
  for(auto x:sp)for(auto y:sp)for(auto z:sp)check(x,y,z);
  for(long i=0;i<n;++i){check(rnd(-16500,16500),rnd(-16500,16500),rnd(-16500,16500));check(rnd(-8300,8300),rnd(-8300,8300),rnd(-16500,16500));}
  printf("checked %ld mismatches %ld\n",count,bad);return bad!=0;}
