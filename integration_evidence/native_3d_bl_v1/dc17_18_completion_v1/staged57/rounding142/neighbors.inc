struct Raw {uint64_t m;uint16_t e;};
long double make(uint64_t m,uint16_t e){Raw b{m,e};long double x=0;std::memcpy(&x,&b,10);return x;}
bool same(long double x,long double y){unsigned char a[10],b[10];std::memcpy(a,&x,10);std::memcpy(b,&y,10);return std::memcmp(a,b,10)==0;}
size_t neighbors=0;
void compare(long double x){for(bool up:{false,true}){const auto a=v142::Adjacent(x,up),b=std::nextafter(x,up?INFINITY:-INFINITY);assert((std::isnan(a)&&std::isnan(b))||same(a,b));++neighbors;}}
int main(){
#if defined(__x86_64__) && defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__==64
  const uint64_t mantissas[]={0x8000000000000000ULL,0x8000000000000001ULL,0xaaaaaaaaaaaaaaaaULL,0xfffffffffffffffeULL,0xffffffffffffffffULL};
  for(unsigned exponent=1;exponent<0x7fff;++exponent)for(auto m:mantissas)for(unsigned sign:{0u,0x8000u})compare(make(m,exponent|sign));
  for(auto m:{0ULL,1ULL,0x7fffffffffffffffULL})for(unsigned sign:{0u,0x8000u})compare(make(m,sign));
  std::mt19937_64 random(142);for(unsigned i=0;i<100000;++i){auto exponent=1+random()%0x7ffe;auto m=random()|0x8000000000000000ULL;compare(make(m,exponent|((random()&1)?0x8000:0)));}
#endif
  compare(0.L);compare(-0.L);compare(INFINITY);compare(-INFINITY);compare(std::numeric_limits<long double>::quiet_NaN());
  std::cout<<"adjacent_comparisons="<<neighbors<<"\n";
  v142::interval_check();v141::distance_check();v142::distance_check();
}
