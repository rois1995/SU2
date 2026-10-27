/*!
 * \file CNativeAtlas3D.cpp
 * \brief Checked 32-bit-limb rational arithmetic for curved boundary correspondence.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeAtlas3D.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace SU2NativeGeometry {
namespace {
using Limbs=std::vector<uint32_t>;
size_t AddSize(size_t a,size_t b) {
  if(b>SIZE_MAX-a)throw std::length_error("Atlas size counter overflow.");
  return a+b;
}
size_t MulSize(size_t a,size_t b) {
  if(a && b>SIZE_MAX/a)throw std::length_error("Atlas size counter overflow.");
  return a*b;
}
size_t Bits(const Limbs& a) {
  if(a.empty())return 0;
  size_t bits=MulSize(a.size()-1,32);uint32_t top=a.back();
  while(top){bits=AddSize(bits,1);top>>=1;}
  return bits;
}
size_t Storage(const ExactRatio& a) {
  return AddSize(sizeof(a),MulSize(AddSize(a.numerator.capacity(),a.denominator.capacity()),sizeof(uint32_t)));
}
struct Context {
  AtlasBudget& budget;
  size_t retained=0,live=0;
  Context(AtlasBudget& b,size_t inputs):budget(b),retained(AddSize(b.caller_bytes,inputs)) {
    if(b.work>b.maximum_work)throw std::length_error("Invalid atlas budget counters.");
    Admit(0);
  }
  void Work(size_t n) {
    if(n>budget.maximum_work-budget.work)throw std::length_error("Atlas arithmetic work budget exhausted.");
    budget.work+=n;
  }
  void Admit(size_t extra) {
    const auto total=AddSize(retained,AddSize(live,extra));
    if(total>budget.maximum_bytes)throw std::length_error("Atlas arithmetic storage budget exhausted.");
    budget.peak_bytes=std::max(budget.peak_bytes,total);
  }
  void Acquire(size_t bytes,size_t bits) {
    if(bits>budget.maximum_integer_bits)throw std::length_error("Atlas integer bit budget exhausted.");
    Admit(bytes);live=AddSize(live,bytes);
  }
  void Release(size_t bytes){live-=bytes;}
};
// Same unsigned carry/borrow/product scheme as the fixed-size predicate Integer<N>.
// Ownership here accounts every dynamic limb array before its allocation.
struct Natural {
  Context* context;
  Limbs limb;
  size_t bytes=0;
  Natural(Context& c,size_t count=0,size_t bits=0):context(&c) {
    c.Work(count);bytes=MulSize(count,sizeof(uint32_t));c.Acquire(bytes,bits);
    try{limb.resize(count);}catch(...){c.Release(bytes);throw;}
  }
  Natural(const Natural&)=delete;
  Natural& operator=(const Natural&)=delete;
  Natural(Natural&& a) noexcept:context(a.context),limb(std::move(a.limb)),bytes(a.bytes){a.bytes=0;}
  Natural& operator=(Natural&& a) noexcept {
    if(this!=&a){context->Release(bytes);context=a.context;limb=std::move(a.limb);bytes=a.bytes;a.bytes=0;}
    return *this;
  }
  ~Natural(){context->Release(bytes);}
  void Trim(){while(!limb.empty() && !limb.back()){context->Work(1);limb.pop_back();}}
};
Natural Copy(Context& c,const Limbs& a) {
  Natural result(c,a.size(),Bits(a));std::copy(a.begin(),a.end(),result.limb.begin());return result;
}
Natural Small(Context& c,uint64_t value) {
  size_t bits=0;for(auto v=value;v;v>>=1)++bits;
  Natural result(c,(bits+31)/32,bits);
  if(bits)result.limb[0]=uint32_t(value);
  if(bits>32)result.limb[1]=uint32_t(value>>32);
  return result;
}
int Compare(Context& c,const Limbs& a,const Limbs& b) {
  c.Work(1);
  if(a.size()!=b.size())return a.size()>b.size()?1:-1;
  for(size_t i=a.size();i--;) {c.Work(1);if(a[i]!=b[i])return a[i]>b[i]?1:-1;}
  return 0;
}
bool One(const Limbs& a){return a.size()==1 && a[0]==1;}
Natural Sum(Context& c,const Limbs& a,const Limbs& b) {
  const auto count=std::max(a.size(),b.size());
  if(!count)return Natural(c);
  Natural result(c,AddSize(count,1),AddSize(std::max(Bits(a),Bits(b)),1));
  uint64_t carry=0;
  for(size_t i=0;i<count;++i){c.Work(1);const uint64_t s=carry+(i<a.size()?a[i]:0)+(i<b.size()?uint64_t(b[i]):0);
    result.limb[i]=uint32_t(s);carry=s>>32;}
  result.limb[count]=uint32_t(carry);result.Trim();return result;
}
void SubtractInPlace(Context& c,Natural& a,const Limbs& b) {
  uint64_t borrow=0;
  for(size_t i=0;i<a.limb.size();++i){c.Work(1);const uint64_t rhs=(i<b.size()?uint64_t(b[i]):0)+borrow;
    const uint64_t lhs=a.limb[i];a.limb[i]=uint32_t(lhs-rhs);borrow=lhs<rhs;}
  if(borrow)throw std::logic_error("Negative unsigned atlas subtraction.");
  a.Trim();
}
Natural Difference(Context& c,const Limbs& a,const Limbs& b) {
  auto result=Copy(c,a);SubtractInPlace(c,result,b);return result;
}
Natural Product(Context& c,const Limbs& a,const Limbs& b) {
  if(a.empty() || b.empty())return Natural(c);
  if(One(a))return Copy(c,b);
  if(One(b))return Copy(c,a);
  const auto bits=AddSize(Bits(a),Bits(b));
  if(bits>c.budget.maximum_integer_bits)throw std::length_error("Atlas product bit budget exhausted.");
  c.Work(MulSize(a.size(),b.size()));
  Natural result(c,AddSize(a.size(),b.size()),bits);
  for(size_t i=0;i<a.size();++i){uint64_t carry=0;
    for(size_t j=0;j<b.size();++j){const uint64_t s=uint64_t(a[i])*b[j]+result.limb[i+j]+carry;
      result.limb[i+j]=uint32_t(s);carry=s>>32;}
    result.limb[i+b.size()]=uint32_t(carry);}
  result.Trim();return result;
}
Natural Shift(Context& c,const Limbs& a,size_t shift) {
  if(a.empty())return Natural(c);
  const auto bits=AddSize(Bits(a),shift),count=AddSize(bits,31)/32;
  Natural result(c,count,bits);const size_t whole=shift/32;const unsigned part=shift%32;
  for(size_t i=0;i<a.size();++i){c.Work(1);const uint64_t v=uint64_t(a[i])<<part;
    result.limb[i+whole]|=uint32_t(v);if((v>>32) && i+whole+1<count)result.limb[i+whole+1]|=uint32_t(v>>32);}
  return result;
}
void Half(Context& c,Natural& a) {
  uint32_t carry=0;
  for(size_t i=a.limb.size();i--;){c.Work(1);const auto next=a.limb[i]&1;a.limb[i]=(a.limb[i]>>1)|(carry<<31);carry=next;}
  a.Trim();
}
Natural Gcd(Context& c,const Limbs& first,const Limbs& second) {
  auto a=Copy(c,first),b=Copy(c,second);
  if(a.limb.empty())return b;
  if(b.limb.empty())return a;
  size_t twos=0;
  while(!(a.limb[0]&1) && !(b.limb[0]&1)){Half(c,a);Half(c,b);twos=AddSize(twos,1);}
  while(!(a.limb[0]&1))Half(c,a);
  while(!b.limb.empty()) {
    while(!(b.limb[0]&1))Half(c,b);
    if(Compare(c,a.limb,b.limb)>0)std::swap(a,b);
    SubtractInPlace(c,b,a.limb);
  }
  return Shift(c,a.limb,twos);
}
Natural Quotient(Context& c,const Limbs& a,const Limbs& divisor) {
  if(divisor.empty())throw std::invalid_argument("Zero atlas integer divisor.");
  if(One(divisor))return Copy(c,a);
  const auto bits=Bits(a);
  Natural quotient(c,a.size(),bits),remainder(c,AddSize(divisor.size(),1),AddSize(Bits(divisor),1));
  for(size_t bit=bits;bit--;) {
    uint32_t carry=(a[bit/32]>>(bit%32))&1;
    for(size_t i=0;i<remainder.limb.size();++i){c.Work(1);const auto next=remainder.limb[i]>>31;
      remainder.limb[i]=(remainder.limb[i]<<1)|carry;carry=next;}
    // The reserved remainder has a fixed width; compare only its significant prefix.
    size_t used=remainder.limb.size();while(used && !remainder.limb[used-1]){c.Work(1);--used;}
    int comparison=used==divisor.size()?0:used>divisor.size()?1:-1;
    if(!comparison)for(size_t i=used;i--;){c.Work(1);if(remainder.limb[i]!=divisor[i]){comparison=remainder.limb[i]>divisor[i]?1:-1;break;}}
    if(comparison>=0){uint64_t borrow=0;for(size_t i=0;i<remainder.limb.size();++i){c.Work(1);
        const uint64_t rhs=(i<divisor.size()?uint64_t(divisor[i]):0)+borrow,lhs=remainder.limb[i];
        remainder.limb[i]=uint32_t(lhs-rhs);borrow=lhs<rhs;}
      if(borrow)throw std::logic_error("Atlas division borrow.");quotient.limb[bit/32]|=uint32_t(1)<<(bit%32);}
  }
  for(auto limb:remainder.limb){c.Work(1);if(limb)throw std::logic_error("Inexact atlas integer division.");}
  quotient.Trim();return quotient;
}
struct Rational {
  Natural numerator,denominator;bool negative=false;
  Rational(Natural n,Natural d,bool sign=false):numerator(std::move(n)),denominator(std::move(d)),negative(sign){}
};
void Shape(Context& c,const ExactRatio& a) {
  c.Work(AddSize(a.numerator.size(),a.denominator.size()));
  if(a.denominator.empty() || !a.denominator.back() || (!a.numerator.empty() && !a.numerator.back()) ||
     (a.numerator.empty() && (a.negative || !One(a.denominator))))
    throw std::invalid_argument("Noncanonical atlas rational shape.");
  if(std::max(Bits(a.numerator),Bits(a.denominator))>c.budget.maximum_integer_bits)
    throw std::length_error("Atlas input integer bit budget exhausted.");
}
Rational Load(Context& c,const ExactRatio& a) {
  Shape(c,a);
  const auto gcd=Gcd(c,a.numerator,a.denominator);
  if(!One(gcd.limb))throw std::invalid_argument("Unreduced atlas rational.");
  return {Copy(c,a.numerator),Copy(c,a.denominator),a.negative};
}
Rational Reduced(Context& c,Natural n,Natural d,bool negative) {
  if(d.limb.empty())throw std::invalid_argument("Zero atlas rational denominator.");
  if(n.limb.empty())return {std::move(n),Small(c,1)};
  auto gcd=Gcd(c,n.limb,d.limb);
  if(!One(gcd.limb)){n=Quotient(c,n.limb,gcd.limb);d=Quotient(c,d.limb,gcd.limb);}
  return {std::move(n),std::move(d),negative};
}
ExactRatio Finish(Rational value) {
  return {value.negative,std::move(value.numerator.limb),std::move(value.denominator.limb)};
}
Rational Binary(Context& c,double value) {
  static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits==53,
                "Atlas rationals require IEEE binary64.");
  if(!std::isfinite(value))throw std::invalid_argument("Nonfinite atlas binary64 coordinate.");
  uint64_t bits;std::memcpy(&bits,&value,8);const unsigned exponent=(bits>>52)&2047;
  uint64_t mantissa=(bits&((uint64_t(1)<<52)-1))|(exponent?uint64_t(1)<<52:0);
  if(!mantissa)return {Natural(c),Small(c,1)};
  int power=exponent?int(exponent)-1023-52:-1074;
  while(!(mantissa&1)){c.Work(1);mantissa>>=1;++power;}
  auto n=Small(c,mantissa),d=Small(c,1);
  if(power>=0)n=Shift(c,n.limb,size_t(power));else d=Shift(c,d.limb,size_t(-power));
  return {std::move(n),std::move(d),bool(bits>>63)};
}
int Order(Context& c,const Rational& a,const Rational& b) {
  c.Work(1);
  if(a.negative!=b.negative)return a.negative?-1:1;
  if(a.numerator.limb.empty() || b.numerator.limb.empty())
    return a.numerator.limb.empty()?(b.numerator.limb.empty()?0:-1):1;
  if(!Compare(c,a.numerator.limb,b.numerator.limb) && !Compare(c,a.denominator.limb,b.denominator.limb))return 0;
  auto left=Product(c,a.numerator.limb,b.denominator.limb),right=Product(c,b.numerator.limb,a.denominator.limb);
  const int order=Compare(c,left.limb,right.limb);return a.negative?-order:order;
}
template<class F>bool Attempt(std::string& reason,F operation) {
  try{operation();reason.clear();return true;}catch(const std::exception& e){reason=e.what();return false;}
}
size_t Inputs(const ExactRatio& a,const ExactRatio& b,const ExactRatio& out) {
  return AddSize(AddSize(Storage(a),Storage(b)),Storage(out));
}
bool Arithmetic(const ExactRatio& a,const ExactRatio& b,ExactRatio& out,AtlasBudget& budget,std::string& reason,char kind) {
  return Attempt(reason,[&]{Context c(budget,Inputs(a,b,out));auto x=Load(c,a),y=Load(c,b);
    Rational result{Natural(c),Small(c,1)};
    if(kind=='+' || kind=='-') {
      const bool sign=y.negative!=(kind=='-');
      auto left=Product(c,x.numerator.limb,y.denominator.limb),right=Product(c,y.numerator.limb,x.denominator.limb);
      auto denominator=Product(c,x.denominator.limb,y.denominator.limb);
      if(x.negative==sign)result=Reduced(c,Sum(c,left.limb,right.limb),std::move(denominator),x.negative);
      else {const int order=Compare(c,left.limb,right.limb);
        result=Reduced(c,order>=0?Difference(c,left.limb,right.limb):Difference(c,right.limb,left.limb),
                       std::move(denominator),order>=0?x.negative:sign);}
    } else {
      if(kind=='/' && y.numerator.limb.empty())throw std::invalid_argument("Atlas rational division by zero.");
      const auto& yn=kind=='/'?y.denominator.limb:y.numerator.limb;
      const auto& yd=kind=='/'?y.numerator.limb:y.denominator.limb;
      result=Reduced(c,Product(c,x.numerator.limb,yn),Product(c,x.denominator.limb,yd),x.negative!=y.negative);
    }
    out=Finish(std::move(result));});
}
}  // namespace

bool ExactInteger(int64_t value,ExactRatio& out,AtlasBudget& budget,std::string& reason) {
  return Attempt(reason,[&]{Context c(budget,Storage(out));const auto magnitude=value<0?uint64_t(-(value+1))+1:uint64_t(value);
    out=Finish({Small(c,magnitude),Small(c,1),value<0});});
}
bool ExactBinary64(double value,ExactRatio& out,AtlasBudget& budget,std::string& reason) {
  return Attempt(reason,[&]{Context c(budget,Storage(out));out=Finish(Binary(c,value));});
}
bool ExactValidate(const ExactRatio& a,AtlasBudget& budget,std::string& reason) {
  return Attempt(reason,[&]{Context c(budget,Storage(a));Load(c,a);});
}
bool ExactAdd(const ExactRatio& a,const ExactRatio& b,ExactRatio& out,AtlasBudget& budget,std::string& reason){return Arithmetic(a,b,out,budget,reason,'+');}
bool ExactSubtract(const ExactRatio& a,const ExactRatio& b,ExactRatio& out,AtlasBudget& budget,std::string& reason){return Arithmetic(a,b,out,budget,reason,'-');}
bool ExactMultiply(const ExactRatio& a,const ExactRatio& b,ExactRatio& out,AtlasBudget& budget,std::string& reason){return Arithmetic(a,b,out,budget,reason,'*');}
bool ExactDivide(const ExactRatio& a,const ExactRatio& b,ExactRatio& out,AtlasBudget& budget,std::string& reason){return Arithmetic(a,b,out,budget,reason,'/');}
bool ExactCompare(const ExactRatio& a,const ExactRatio& b,int& out,AtlasBudget& budget,std::string& reason) {
  return Attempt(reason,[&]{Context c(budget,AddSize(Storage(a),Storage(b)));auto x=Load(c,a),y=Load(c,b);const int order=Order(c,x,y);out=order;});
}
bool ExactBounds(const ExactRatio& a,std::array<double,2>& out,AtlasBudget& budget,std::string& reason) {
  return Attempt(reason,[&]{Context c(budget,Storage(a));auto x=Load(c,a);const bool negative=x.negative;x.negative=false;
    if(x.numerator.limb.empty()){out={0.,0.};return;}
    constexpr uint64_t maximum=UINT64_C(0x7fefffffffffffff);
    auto value=[](uint64_t bits){double v;std::memcpy(&v,&bits,8);return v;};
    uint64_t low=0,high=maximum;
    while(low<high){c.Work(1);const uint64_t middle=low+(high-low)/2+1;
      auto point=Binary(c,value(middle));if(Order(c,point,x)<=0)low=middle;else high=middle-1;}
    const double lower=value(low);auto point=Binary(c,lower);const bool equal=Order(c,point,x)==0;
    const double upper=equal?lower:low==maximum?std::numeric_limits<double>::infinity():value(low+1);
    out=negative?std::array<double,2>{-upper,-lower}:std::array<double,2>{lower,upper};});
}
bool ExactEncode(const ExactRatio& a,std::vector<uint64_t>& out,AtlasBudget& budget,std::string& reason) {
  return Attempt(reason,[&]{Context c(budget,AddSize(Storage(a),MulSize(out.capacity(),sizeof(uint64_t))));Load(c,a);
    const auto words=AddSize(3,AddSize(a.numerator.size(),a.denominator.size()));c.Work(words);c.Acquire(MulSize(words,sizeof(uint64_t)),0);
    std::vector<uint64_t> encoded;encoded.reserve(words);encoded.push_back(a.negative);encoded.push_back(a.numerator.size());encoded.push_back(a.denominator.size());
    for(auto limb:a.numerator)encoded.push_back(limb);for(auto limb:a.denominator)encoded.push_back(limb);out=std::move(encoded);});
}
bool ExactDecode(const std::vector<uint64_t>& words,size_t& offset,ExactRatio& out,AtlasBudget& budget,std::string& reason) {
  return Attempt(reason,[&]{Context c(budget,AddSize(Storage(out),MulSize(words.capacity(),sizeof(uint64_t))));
    if(offset>words.size() || words.size()-offset<3)throw std::invalid_argument("Truncated atlas rational header.");
    c.Work(3);const auto sign=words[offset],nc=words[offset+1],dc=words[offset+2];
    if(sign>1 || uint64_t(size_t(nc))!=nc || uint64_t(size_t(dc))!=dc || !dc)
      throw std::invalid_argument("Invalid atlas rational header.");
    const size_t count=AddSize(size_t(nc),size_t(dc)),first=AddSize(offset,3);
    if(count>words.size()-first)throw std::invalid_argument("Truncated atlas rational limbs.");
    // Check values and actual top bits before any limb allocation.
    c.Work(count);for(size_t i=0;i<count;++i)if(words[first+i]>UINT32_MAX)throw std::invalid_argument("Oversized atlas limb word.");
    auto width=[&](size_t start,size_t length){if(!length)return size_t(0);uint64_t top=words[start+length-1];
      if(!top)throw std::invalid_argument("Atlas rational has a leading zero limb.");
      size_t bits=MulSize(length-1,32);while(top){bits=AddSize(bits,1);top>>=1;}return bits;};
    const auto nb=width(first,size_t(nc)),db=width(first+size_t(nc),size_t(dc));
    Natural n(c,size_t(nc),nb),d(c,size_t(dc),db);
    for(size_t i=0;i<nc;++i)n.limb[i]=uint32_t(words[first+i]);for(size_t i=0;i<dc;++i)d.limb[i]=uint32_t(words[first+size_t(nc)+i]);
    if(n.limb.empty() && (sign || !One(d.limb)))throw std::invalid_argument("Noncanonical atlas zero.");
    const auto gcd=Gcd(c,n.limb,d.limb);if(!One(gcd.limb))throw std::invalid_argument("Unreduced atlas rational payload.");
    auto result=Finish({std::move(n),std::move(d),bool(sign)});out=std::move(result);offset=first+count;});
}
}  // namespace SU2NativeGeometry
