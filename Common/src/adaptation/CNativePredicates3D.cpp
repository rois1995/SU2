/*!
 * \file CNativePredicates3D.cpp
 * \brief Strict tetrahedral predicates and scale-independent metric measures.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeMesh3D.hpp"
#include "../../include/adaptation/CNativeFma.hpp"
#include "CNativePredicates3D_internal.hpp"
#include "../../include/adaptation/CNativePointConstruction3D.hpp"
#include "../../include/adaptation/CNativeNearContainment.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace SU2Native3D {
namespace {
// Adapt the bounded binary64 integer scheme of CNativePredicates2D without changing that validated unit.
// Differences need 2099 bits; the six triple products fit within 6301 bits (198 32-bit limbs).
template <size_t N>
struct Integer {
  std::array<uint32_t, N> limb{};
  bool negative = false;
};
template <size_t N>
int Compare(const Integer<N>& a, const Integer<N>& b) {
  for (size_t i = N; i-- > 0;)
    if (a.limb[i] != b.limb[i]) return a.limb[i] > b.limb[i] ? 1 : -1;
  return 0;
}
template <size_t N>
Integer<N> Difference(const Integer<N>& a, const Integer<N>& b) {
  Integer<N> result;
  size_t first = 0, last = N;
  while (last && !a.limb[last - 1] && !b.limb[last - 1]) --last;
  while (first < last && !a.limb[first] && !b.limb[first]) ++first;
  if (a.negative != b.negative) {
    uint64_t carry = 0;
    for (size_t i = first; i < last; ++i) {
      const uint64_t sum = uint64_t(a.limb[i]) + b.limb[i] + carry;
      result.limb[i] = static_cast<uint32_t>(sum);
      carry = sum >> 32;
    }
    if (carry) {
      if (last == N) throw std::logic_error("Native 3D predicate integer bound exceeded.");
      result.limb[last] = static_cast<uint32_t>(carry);
    }
    result.negative = a.negative;
  } else {
    const bool less = Compare(a, b) < 0;
    const auto& larger = less ? b : a;
    const auto& smaller = less ? a : b;
    uint64_t borrow = 0;
    for (size_t i = first; i < last; ++i) {
      const uint64_t rhs = uint64_t(smaller.limb[i]) + borrow;
      result.limb[i] = static_cast<uint32_t>(uint64_t(larger.limb[i]) - rhs);
      borrow = uint64_t(larger.limb[i]) < rhs;
    }
    result.negative = less ? !a.negative : a.negative;
  }
  return result;
}
Integer<66> Coordinate(double value) {
  static_assert(sizeof(double) == sizeof(uint64_t) && std::numeric_limits<double>::is_iec559 &&
                    std::numeric_limits<double>::digits == 53,
                "Native 3D predicates require IEEE binary64.");
  uint64_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  const unsigned exponent = (bits >> 52) & 2047;
  const uint64_t mantissa = (bits & ((uint64_t(1) << 52) - 1)) | (exponent ? uint64_t(1) << 52 : 0);
  const unsigned shift = exponent ? exponent - 1 : 0;
  Integer<66> result;
  result.negative = bits >> 63;
  for (unsigned bit = 0; bit <= 52; ++bit)
    if ((mantissa >> bit) & 1) result.limb[(shift + bit) / 32] |= uint32_t(1) << ((shift + bit) % 32);
  return result;
}
template <size_t A, size_t B>
Integer<A + B> Product(const Integer<A>& a, const Integer<B>& b) {
  Integer<A + B> result;
  result.negative = a.negative != b.negative;
  size_t first = 0, last = B;
  while (first < last && !b.limb[first]) ++first;
  while (last > first && !b.limb[last - 1]) --last;
  if (first == last) return result;
  // Outside this span the original product only copies existing limbs and propagates zero carry.
  for (size_t i = 0; i < A; ++i) {
    if (!a.limb[i]) continue;
    uint64_t carry = 0;
    for (size_t j = first; j < last; ++j) {
      const uint64_t sum = uint64_t(a.limb[i]) * b.limb[j] + result.limb[i + j] + carry;
      result.limb[i + j] = static_cast<uint32_t>(sum);
      carry = sum >> 32;
    }
    result.limb[i + last] = static_cast<uint32_t>(carry);
  }
  return result;
}
long double Rounded(const Integer<198>& value, bool preserve_sign = true) {
  int top = 197;
  while (top >= 0 && value.limb[top] == 0) --top;
  if (top < 0) return 0;
  const int bottom = std::max(0, top - 2);
  long double mantissa = 0;
  for (int i = top; i >= bottom; --i) mantissa = std::ldexp(mantissa, 32) + value.limb[i];
  long double result = std::ldexp(mantissa, bottom * 32 - 3222);
  if (result == 0) {
    if (!preserve_sign) throw std::runtime_error("Unrepresentable native 3D exact magnitude.");
    result = std::numeric_limits<long double>::denorm_min();
  }
  return value.negative ? -result : result;
}
bool Finite(Point p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
using Vector = std::array<long double, 3>;
Vector Delta(Point a, Point b) {
  return {
      {static_cast<long double>(a.x) - b.x, static_cast<long double>(a.y) - b.y, static_cast<long double>(a.z) - b.z}};
}
long double Norm2(const Vector& v) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; }
}  // namespace

namespace {
Integer<198> ExactDeterminant(Point a, Point b, Point c, Point d) {
  const std::array<Integer<66>, 3> origin{{Coordinate(a.x), Coordinate(a.y), Coordinate(a.z)}};
  auto difference = [&](Point p) {
    return std::array<Integer<66>, 3>{{Difference(Coordinate(p.x), origin[0]), Difference(Coordinate(p.y), origin[1]),
                                       Difference(Coordinate(p.z), origin[2])}};
  };
  const auto x = difference(b), y = difference(c), z = difference(d);
  const auto first = Product(x[0], Difference(Product(y[1], z[2]), Product(y[2], z[1])));
  const auto second = Product(x[1], Difference(Product(y[0], z[2]), Product(y[2], z[0])));
  const auto third = Product(x[2], Difference(Product(y[0], z[1]), Product(y[1], z[0])));
  auto negativeThird = third;
  negativeThird.negative = !negativeThird.negative;
  return Difference(Difference(first, second), negativeThird);
}
long double Determinant(Point a, Point b, Point c, Point d, KernelStats* stats, long double relative_error, bool exact = true) {
  if (!(Finite(a) && Finite(b) && Finite(c) && Finite(d)))
    throw std::invalid_argument("Native 3D orientation requires finite coordinates.");
  const auto u = Delta(b, a), v = Delta(c, a), w = Delta(d, a);
  const std::array<long double, 6> terms{{u[0] * v[1] * w[2], u[1] * v[2] * w[0], u[2] * v[0] * w[1],
                                          -u[2] * v[1] * w[0], -u[1] * v[0] * w[2], -u[0] * v[2] * w[1]}};
  long double determinant = 0, permanent = 0;
  for (const auto value : terms) {
    determinant += value;
    permanent += std::abs(value);
  }
  const auto epsilon = std::numeric_limits<long double>::epsilon();
  if (std::isfinite(determinant) && permanent > std::numeric_limits<long double>::min() / epsilon &&
      std::abs(determinant) * relative_error > 64 * epsilon * permanent) {
    if (stats) ++stats->filtered_orientations;
    return determinant;
  }
  if (!exact) return 0;  // Unresolved filters defer to the caller's exact clip, without duplicating exact arithmetic.
  if (stats) ++stats->exact_orientations;
  return Rounded(ExactDeterminant(a, b, c, d));
}

template <size_t N>
int Sign(const Integer<N>& value) {
  const Integer<N> zero;
  return Compare(value, zero) == 0 ? 0 : value.negative ? -1 : 1;
}
template <size_t N>
std::pair<long double, int> Mantissa(const Integer<N>& value) {
  int top = int(N) - 1;
  while (top >= 0 && !value.limb[top]) --top;
  if (top < 0) return {0, 0};
  const int bottom = std::max(0, top - 2);
  long double result = 0;
  for (int i = top; i >= bottom; --i) result = std::ldexp(result, 32) + value.limb[i];
  return {result, 32 * bottom};
}
template <size_t A, size_t B>
long double Ratio(const Integer<A>& a, const Integer<B>& b) {
  const auto x = Mantissa(a), y = Mantissa(b);
  if (y.first == 0) throw std::runtime_error("Zero exact native 3D segment denominator.");
  const auto result = std::ldexp(x.first / y.first, x.second - y.second);
  if (!std::isfinite(result) || (result == 0 && x.first != 0))
    throw std::runtime_error("Unrepresentable exact native 3D segment ratio.");
  return a.negative != b.negative ? -result : result;
}
struct Root {
  Integer<198> numerator, denominator;
  explicit Root(bool one = false) {
    denominator.limb[0] = 1;
    numerator.limb[0] = one ? 1 : 0;
  }
  Root(Integer<198> n, Integer<198> d) : numerator(n), denominator(d) {
    if (Sign(denominator) < 0) {
      denominator.negative = false;
      numerator.negative = !numerator.negative;
    }
  }
};
int CompareRoot(const Root& a, const Root& b) {
  return Compare(Product(a.numerator, b.denominator), Product(b.numerator, a.denominator));
}
struct WorkPiece {
  Root begin, end;
  SegmentPiece piece;
  std::array<Id, 4> support{};
  size_t support_size = 0;
};
static_assert(sizeof(WorkPiece)+2*sizeof(Root)+2*sizeof(SegmentPiece)<=8192,
              "Native3D layered segment storage exceeds the frozen-field byte model.");
Integer<198> ExactVolume(const Tetrahedron& t, KernelStats* stats) {
  for (auto p : t)
    if (!Finite(p)) throw std::invalid_argument("Nonfinite native 3D source segment.");
  if (stats) ++stats->exact_orientations;
  return ExactDeterminant(t[0], t[1], t[2], t[3]);
}
std::array<long double, 4> AtRoot(const Root& root, const std::array<Integer<198>, 4>& a,
                                  const std::array<Integer<198>, 4>& b, const Integer<198>& volume) {
  std::array<long double, 4> weights;
  const auto denominator = Product(root.denominator, volume);
  long double sum = 0;
  for (size_t i = 0; i < 4; ++i) {
    // a_i + t (b_i-a_i), evaluated as an exact rational before rounding.
    auto negative_delta = Difference(b[i], a[i]);
    negative_delta.negative = !negative_delta.negative;
    const auto numerator = Difference(Product(a[i], root.denominator), Product(root.numerator, negative_delta));
    if (Sign(numerator) < 0) throw std::logic_error("Negative clipped native 3D source weight.");
    weights[i] = Ratio(numerator, denominator);
    sum += weights[i];
  }
  if (!(sum > 0)) throw std::logic_error("Empty clipped native 3D source weights.");
  for (auto& w : weights) w /= sum;
  return weights;
}
bool ClipInterval(const Tetrahedron& t, Point a, Point b, Integer<198>& volume,
                  std::array<Integer<198>, 4>& va, std::array<Integer<198>, 4>& vb,
                  Root& lo, Root& hi, bool closed, KernelStats* stats) {
  volume = ExactVolume(t, stats);
  if (Sign(volume) <= 0) throw std::invalid_argument("Nonpositive native 3D segment donor.");
  // Exact donor vertices have unit barycentric numerators; crossing endpoints retain exact determinants.
  auto vertex = [&](Point p) {
    for (size_t i = 0; i < 4; ++i)
      if (p.x == t[i].x && p.y == t[i].y && p.z == t[i].z) return i;
    return size_t(4);
  };
  const auto first = vertex(a), last = vertex(b);
  for (size_t i = 0; i < 4; ++i) {
    auto ta = t, tb = t;
    ta[i] = a;
    tb[i] = b;
    va[i] = first < 4 ? (first == i ? volume : Integer<198>{}) : ExactVolume(ta, stats);
    vb[i] = last < 4 ? (last == i ? volume : Integer<198>{}) : ExactVolume(tb, stats);
    const auto sa = Sign(va[i]), sb = Sign(vb[i]);
    if (sa < 0 && sb < 0) return false;
    if (sa < 0) {
      Root cut(va[i], Difference(va[i], vb[i]));
      if (CompareRoot(lo, cut) < 0) lo = cut;
    }
    if (sb < 0) {
      Root cut(va[i], Difference(va[i], vb[i]));
      if (CompareRoot(cut, hi) < 0) hi = cut;
    }
    const auto order = CompareRoot(lo, hi);
    if (order > 0 || (!closed && order == 0)) return false;
  }
  return true;
}
bool Clip(const Cell& cell, Point a, Point b, WorkPiece& result, KernelStats* stats) {
  const Tetrahedron t{cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p};
  Integer<198> volume;
  std::array<Integer<198>, 4> va, vb;
  Root lo(false), hi(true);
  if (!ClipInterval(t, a, b, volume, va, vb, lo, hi, false, stats)) return false;
  result.begin = lo;
  result.end = hi;
  result.piece.begin = Ratio(lo.numerator, lo.denominator);
  result.piece.end = Ratio(hi.numerator, hi.denominator);
  result.piece.width = Ratio(Difference(Product(hi.numerator, lo.denominator), Product(lo.numerator, hi.denominator)),
                             Product(hi.denominator, lo.denominator));
  result.piece.begin_weights = AtRoot(lo, va, vb, volume);
  result.piece.end_weights = AtRoot(hi, va, vb, volume);
  for (size_t i = 0; i < 4; ++i)
    if (result.piece.begin_weights[i] != 0 || result.piece.end_weights[i] != 0)
      result.support[result.support_size++] = cell.v[i].id;
  std::sort(result.support.begin(), result.support.begin() + result.support_size);
  return true;
}

}  // namespace

long double Orientation(Point a, Point b, Point c, Point d, KernelStats* stats) {
  return Determinant(a, b, c, d, stats, 1);
}
int SurfaceNormalSign(Point normal,const std::array<Point,3>& actual,KernelStats* stats) {
  if(!Finite(normal))throw std::invalid_argument("Nonfinite mapped component normal.");
  for(Point p:actual)if(!Finite(p))throw std::invalid_argument("Nonfinite mapped component face.");
  const double n[3]={normal.x,normal.y,normal.z};
  auto delta=[&](Point p){return std::array<Integer<66>,3>{Difference(Coordinate(p.x),Coordinate(actual[0].x)),
      Difference(Coordinate(p.y),Coordinate(actual[0].y)),Difference(Coordinate(p.z),Coordinate(actual[0].z))};};
  const auto a=delta(actual[1]),b=delta(actual[2]);Integer<198> total;
  for(unsigned k=0;k<3;++k){const unsigned i=(k+1)%3,j=(k+2)%3;auto term=Product(Coordinate(n[k]),Difference(Product(a[i],b[j]),Product(a[j],b[i])));
    term.negative=!term.negative;total=Difference(total,term);}
  if(stats)++stats->exact_orientations;return Sign(total);
}
int SurfaceOrientationSign(const std::array<Point,3>& reference,const std::array<Point,3>& actual,KernelStats* stats) {
  for(const auto& face:{reference,actual})for(Point p:face)
    if(!Finite(p))throw std::invalid_argument("Nonfinite native surface orientation.");
  // The filter needs the complete degree-four binary64 exponent range; otherwise use exact integers.
  if constexpr(std::numeric_limits<long double>::max_exponent>4*std::numeric_limits<double>::max_exponent+8 &&
      std::numeric_limits<long double>::min_exponent-std::numeric_limits<long double>::digits<
          4*(std::numeric_limits<double>::min_exponent-std::numeric_limits<double>::digits)-8) {
    auto normal=[](const std::array<Point,3>& face,Vector& magnitude) {
      const auto u=Delta(face[1],face[0]),v=Delta(face[2],face[0]);Vector n{};
      for(unsigned i=0;i<3;++i){const unsigned j=(i+1)%3,k=(i+2)%3;
        const auto a=u[j]*v[k],b=u[k]*v[j];n[i]=a-b;magnitude[i]=std::abs(a)+std::abs(b);}
      return n;
    };
    Vector a{},b{};const auto n=normal(reference,a),m=normal(actual,b);
    long double value=0,permanent=0;
    for(unsigned i=0;i<3;++i){value+=n[i]*m[i];permanent+=a[i]*b[i];}
    if(std::isfinite(value)&&std::isfinite(permanent)&&permanent>0 &&
        std::abs(value)>64*std::numeric_limits<long double>::epsilon()*permanent) {
      if(stats)++stats->filtered_orientations;return value>0?1:-1;
    }
  }
  if(stats)++stats->exact_orientations;
  auto normal=[](const std::array<Point,3>& face) {
    auto difference=[&](Point p){return std::array<Integer<66>,3>{
        Difference(Coordinate(p.x),Coordinate(face[0].x)),Difference(Coordinate(p.y),Coordinate(face[0].y)),
        Difference(Coordinate(p.z),Coordinate(face[0].z))};};
    const auto u=difference(face[1]),v=difference(face[2]);std::array<Integer<132>,3> n{};
    for(unsigned i=0;i<3;++i){const unsigned j=(i+1)%3,k=(i+2)%3;n[i]=Difference(Product(u[j],v[k]),Product(u[k],v[j]));}
    return n;
  };
  const auto a=normal(reference),b=normal(actual);auto value=Product(a[0],b[0]);
  // Four coordinate factors plus three signed terms fit in264 32-bit limbs (binary64 needs at most8401bits).
  for(unsigned i=1;i<3;++i){auto term=Product(a[i],b[i]);term.negative=!term.negative;value=Difference(value,term);}
  return Sign(value);
}
int OrientationCentroidSign(Point a,Point b,const std::array<Point,3>& triangle,Point d,KernelStats* stats) {
  if(!(Finite(a)&&Finite(b)&&Finite(d)))throw std::invalid_argument("Nonfinite native centroid orientation.");
  const auto u=Delta(b,a),w=Delta(d,a);long double total=0,permanent=0;
  for(auto p:triangle) {
    if(!Finite(p))throw std::invalid_argument("Nonfinite native centroid orientation.");
    const auto v=Delta(p,a);
    const long double terms[]={u[0]*v[1]*w[2],u[1]*v[2]*w[0],u[2]*v[0]*w[1],
        -u[2]*v[1]*w[0],-u[1]*v[0]*w[2],-u[0]*v[2]*w[1]};
    for(auto term:terms){total+=term;permanent+=std::abs(term);}
  }
  const auto epsilon=std::numeric_limits<long double>::epsilon();
  if(std::isfinite(total) && permanent>std::numeric_limits<long double>::min()/epsilon && std::abs(total)>128*epsilon*permanent) {
    if(stats)++stats->filtered_orientations;return total>0?1:-1;
  }
  if(stats)++stats->exact_orientations;
  Integer<198> sum;
  for(auto p:triangle){auto value=ExactDeterminant(a,b,p,d);value.negative=!value.negative;sum=Difference(sum,value);}
  return Sign(sum);
}
Point Centroid(const Cell& cell) {
  long double x = 0, y = 0, z = 0;
  for (auto n : cell.v) {
    x += static_cast<long double>(n.p.x) / 4;
    y += static_cast<long double>(n.p.y) / 4;
    z += static_cast<long double>(n.p.z) / 4;
  }
  return {double(x), double(y), double(z)};
}
namespace {
bool SegmentIntersection(const Tetrahedron& donor, Point a, Point b, KernelStats* stats, bool closed) {
  if (!(Finite(a) && Finite(b))) throw std::invalid_argument("Nonfinite native 3D query segment.");
  if (!(Orientation(donor[0], donor[1], donor[2], donor[3], stats) > 0))
    throw std::invalid_argument("Nonpositive native 3D segment donor.");
  bool inside_a = true, inside_b = true;
  const bool point = a.x == b.x && a.y == b.y && a.z == b.z;
  for (size_t i = 0; i < 4; ++i) {
    auto ta = donor, tb = donor; ta[i] = a; tb[i] = b;
    const auto va = Orientation(ta[0], ta[1], ta[2], ta[3], stats);
    const auto vb = point ? va : Orientation(tb[0], tb[1], tb[2], tb[3], stats);
    if (va < 0 && vb < 0) return false;
    // A zero/outside face pair excludes every interior segment parameter, without rational roots.
    if (!closed && ((va==0 && vb<0) || (va<0 && vb==0))) return false;
    inside_a = inside_a && va >= 0; inside_b = inside_b && vb >= 0;
  }
  if (inside_a && inside_b) return closed || !point;
  if (closed && (inside_a || inside_b)) return true;
  // Exact signs settle separation/endpoint containment; unresolved crossing still needs rational roots.
  Integer<198> volume;
  std::array<Integer<198>, 4> va, vb;
  Root lo(false), hi(true);
  return !point && ClipInterval(donor, a, b, volume, va, vb, lo, hi, closed, stats);
}
}
bool IntersectsSegment(const Tetrahedron& donor, Point a, Point b, KernelStats* stats) {
  return SegmentIntersection(donor,a,b,stats,true);
}
bool IntersectsSegmentPositiveLength(const Tetrahedron& donor, Point a, Point b, KernelStats* stats) {
  return SegmentIntersection(donor,a,b,stats,false);
}
bool ConformingDisjoint(const Cell& a, const Cell& b, KernelStats* stats) {
  const Tetrahedron ta{a.v[0].p,a.v[1].p,a.v[2].p,a.v[3].p}, tb{b.v[0].p,b.v[1].p,b.v[2].p,b.v[3].p};
  for (const auto& t : {ta,tb})
    if (!(Orientation(t[0],t[1],t[2],t[3],stats)>0))
      throw std::invalid_argument("Native3D contact requires positive finite tetrahedra.");
  std::array<bool,4> shared_a{},shared_b{};
  for(size_t i=0;i<4;++i) for(size_t j=0;j<4;++j) if(a.v[i].id==b.v[j].id) {
    if(a.v[i].p.x!=b.v[j].p.x || a.v[i].p.y!=b.v[j].p.y || a.v[i].p.z!=b.v[j].p.z)
      throw std::invalid_argument("Native3D contact has inconsistent shared coordinates.");
    shared_a[i]=shared_b[j]=true;
  }
  auto coordinate=[](Point p,size_t axis) {return axis==0 ? p.x : axis==1 ? p.y : p.z;};
  for(size_t axis=0;axis<3;++axis) {
    double amin=coordinate(ta[0],axis),amax=amin,bmin=coordinate(tb[0],axis),bmax=bmin;
    for(size_t i=1;i<4;++i) {
      amin=std::min(amin,coordinate(ta[i],axis));amax=std::max(amax,coordinate(ta[i],axis));
      bmin=std::min(bmin,coordinate(tb[i],axis));bmax=std::max(bmax,coordinate(tb[i],axis));
    }
    if(amax<bmin || bmax<amin)return true;
    if(amax==bmin || bmax==amin) {
      const double plane=amax==bmin ? amax : bmax;
      bool common_a=true,common_b=true;
      for(size_t i=0;i<4;++i) {
        if(coordinate(ta[i],axis)==plane && !shared_a[i])common_a=false;
        if(coordinate(tb[i],axis)==plane && !shared_b[i])common_b=false;
      }
      // Contact lies in this supporting plane; a wholly shared extremal simplex proves conformity.
      if(common_a || common_b)return true;
    }
  }
  auto delta=[](Point p,Point q) {
    return std::array<Integer<66>,3>{Difference(Coordinate(p.x),Coordinate(q.x)),
      Difference(Coordinate(p.y),Coordinate(q.y)),Difference(Coordinate(p.z),Coordinate(q.z))};
  };
  bool tangent=false;
  auto separated=[&](const std::array<Integer<66>,3>& u,const std::array<Integer<66>,3>& v) {
    const std::array<Integer<132>,3> axis{
      Difference(Product(u[1],v[2]),Product(u[2],v[1])),
      Difference(Product(u[2],v[0]),Product(u[0],v[2])),
      Difference(Product(u[0],v[1]),Product(u[1],v[0]))};
    if(!Sign(axis[0]) && !Sign(axis[1]) && !Sign(axis[2]))return false;
    auto projection=[&](Point p) {
      const auto d=delta(p,ta[0]);
      auto first=Product(axis[0],d[0]),second=Product(axis[1],d[1]),third=Product(axis[2],d[2]);
      second.negative=!second.negative;third.negative=!third.negative;
      return Difference(Difference(first,second),third);
    };
    auto range=[&](const Tetrahedron& t) {
      auto lo=projection(t[0]),hi=lo;
      for(size_t i=1;i<4;++i) {
        auto p=projection(t[i]);
        if(Sign(Difference(p,lo))<0)lo=p;
        if(Sign(Difference(p,hi))>0)hi=p;
      }
      return std::make_pair(lo,hi);
    };
    const auto x=range(ta),y=range(tb);
    const int xy=Sign(Difference(x.second,y.first)),yx=Sign(Difference(y.second,x.first));
    tangent=tangent || xy==0 || yx==0;
    return xy<0 || yx<0;
  };
  for(const auto& t:{ta,tb})for(size_t i=0;i<4;++i) {
    std::array<size_t,3> face{};size_t n=0;
    for(size_t j=0;j<4;++j)if(i!=j)face[n++]=j;
    if(separated(delta(t[face[1]],t[face[0]]),delta(t[face[2]],t[face[0]])))return true;
  }
  for(size_t i=0;i<4;++i)for(size_t j=i+1;j<4;++j)
    for(size_t k=0;k<4;++k)for(size_t l=k+1;l<4;++l)
      if(separated(delta(ta[j],ta[i]),delta(tb[l],tb[k])))return true;
  if(!tangent)return false; // Strict overlap on every nonzero separating axis has nonempty interior.
  auto contacts=[&](const Tetrahedron& t,const Tetrahedron& other,const std::array<bool,4>& shared) {
    for(size_t i=0;i<4;++i)for(size_t j=i+1;j<4;++j) {
      Integer<198> volume;std::array<Integer<198>,4> va,vb;Root lo(false),hi(true);
      if(!ClipInterval(other,t[i],t[j],volume,va,vb,lo,hi,true,stats))continue;
      if(shared[i] && shared[j])continue;
      if(CompareRoot(lo,hi)==0 && ((shared[i] && CompareRoot(lo,Root(false))==0) ||
                                 (shared[j] && CompareRoot(lo,Root(true))==0)))continue;
      return false;
    }
    return true;
  };
  return contacts(ta,tb,shared_a) && contacts(tb,ta,shared_b);
}
namespace {
Integer<132> ParametricCoordinate(const ParametricPoint& p,unsigned axis) {
  const double a[3]={p.origin.x,p.origin.y,p.origin.z},b[3]={p.first.x,p.first.y,p.first.z},c[3]={p.second.x,p.second.y,p.second.z};
  auto sum=Product(Coordinate(a[axis]),Coordinate(1.));
  for(const auto& term:std::array<std::pair<double,double>,2>{{{b[axis],p.u},{c[axis],p.v}}}) {
    auto value=Product(Difference(Coordinate(term.first),Coordinate(a[axis])),Coordinate(term.second));
    value.negative=!value.negative;sum=Difference(sum,value);
  }
  return sum;
}
void CheckParametric(const ParametricPoint& p) {
  if(!(Finite(p.origin)&&Finite(p.first)&&Finite(p.second)&&std::isfinite(p.u)&&std::isfinite(p.v)))
    throw std::invalid_argument("Nonfinite native3D parametric point.");
}
std::array<double,2> ParametricBounds(const Integer<132>& value) {
    if(!Sign(value))return std::array<double,2>{0.,0.};
    size_t top=value.limb.size();while(top && !value.limb[top-1])--top;
    unsigned bit=31;while(!(value.limb[top-1]&(uint32_t(1)<<bit)))--bit;
    const size_t highest=32*(top-1)+bit;
    // Each product uses units 2^-2148; normals retain53 bits, subnormals use units 2^-1074.
    const size_t shift=std::max(size_t(1074),highest>52 ? highest-52 : size_t(0));
    uint64_t mantissa=0;
    for(size_t i=shift;i<=highest;++i)
      if(value.limb[i/32]&(uint32_t(1)<<(i%32)))mantissa|=uint64_t(1)<<(i-shift);
    bool remainder=false;
    for(size_t i=0;i<shift/32;++i)remainder=remainder || value.limb[i]!=0;
    if(shift%32)remainder=remainder || (value.limb[shift/32]&((uint32_t(1)<<(shift%32))-1))!=0;
    const double lower=std::ldexp(double(mantissa),int(shift)-2148);
    const double upper=remainder ? std::nextafter(lower,INFINITY) : lower;
    if(!std::isfinite(upper))throw std::runtime_error("Unrepresentable native3D affine point bounds.");
    return value.negative ? std::array<double,2>{-upper,-lower} : std::array<double,2>{lower,upper};
}
} // namespace
std::array<Point,2> ParametricPointBounds(const ParametricPoint& p) {
  CheckParametric(p);
  const auto x=ParametricBounds(ParametricCoordinate(p,0)),y=ParametricBounds(ParametricCoordinate(p,1)),z=ParametricBounds(ParametricCoordinate(p,2));
  return {{{x[0],y[0],z[0]},{x[1],y[1],z[1]}}};
}
namespace {
Integer<264> ParametricDeterminant(const ParametricPoint& a,const ParametricPoint& b,const ParametricPoint& c,unsigned axis) {
  CheckParametric(a);CheckParametric(b);CheckParametric(c);
  if(axis>2)throw std::invalid_argument("Invalid native3D parametric projection axis.");
  const unsigned x=axis==0?1:0,y=axis==2?1:2;
  const auto ax=ParametricCoordinate(a,x),ay=ParametricCoordinate(a,y);
  const auto first=Product(Difference(ParametricCoordinate(b,x),ax),Difference(ParametricCoordinate(c,y),ay));
  const auto second=Product(Difference(ParametricCoordinate(b,y),ay),Difference(ParametricCoordinate(c,x),ax));
  return Difference(first,second);
}
}
int ParametricOrientation(const ParametricPoint& a,const ParametricPoint& b,const ParametricPoint& c,
    unsigned axis,KernelStats* stats) {
  if(stats)++stats->exact_orientations;
  return Sign(ParametricDeterminant(a,b,c,axis));
}
int ParametricPlaneSide(const ParametricPoint& point,Point origin,Point normal) {
  CheckParametric(point);if(!(Finite(origin)&&Finite(normal)))throw std::invalid_argument("Nonfinite ideal affine plane.");
  const double o[3]={origin.x,origin.y,origin.z},n[3]={normal.x,normal.y,normal.z};Integer<198> total;
  for(unsigned k=0;k<3;++k){auto term=Product(Coordinate(n[k]),Difference(ParametricCoordinate(point,k),Product(Coordinate(o[k]),Coordinate(1.))));
    term.negative=!term.negative;total=Difference(total,term);}
  return Sign(total);
}
int ParametricCentroidSide(const ParametricPoint& a,const ParametricPoint& b,
    const std::array<ParametricPoint,3>& triangle,unsigned axis,KernelStats* stats) {
  Integer<264> total;
  for(const auto& p:triangle){auto term=ParametricDeterminant(a,b,p,axis);term.negative=!term.negative;total=Difference(total,term);}
  if(stats)++stats->exact_orientations;
  return Sign(total);
}
bool ParametricOnProjectedSegment(const ParametricPoint& a,const ParametricPoint& b,const ParametricPoint& p,
    unsigned axis,KernelStats* stats) {
  if(ParametricOrientation(a,b,p,axis,stats))return false;
  const unsigned x=axis==0?1:0,y=axis==2?1:2;
  for(unsigned k:{x,y}) {
    const auto q=ParametricCoordinate(p,k);
    if(Sign(Difference(q,ParametricCoordinate(a,k)))*Sign(Difference(q,ParametricCoordinate(b,k)))>0)return false;
  }
  return true;
}
bool ParametricSegmentEntersTriangle(const ParametricPoint& a,const ParametricPoint& b,
    const std::array<ParametricPoint,3>& triangle,unsigned axis,KernelStats* stats) {
  const int orientation=ParametricOrientation(triangle[0],triangle[1],triangle[2],axis,stats);
  if(!orientation)return false;
  struct Fraction {Integer<264> numerator{},denominator{};explicit Fraction(bool one=false){numerator.limb[0]=one;denominator.limb[0]=1;}};
  auto compare=[](const Fraction& a,const Fraction& b){return Sign(Difference(Product(a.numerator,b.denominator),Product(b.numerator,a.denominator)));};
  Fraction low,high(true);
  for(unsigned k=0;k<3;++k) {
    auto first=ParametricDeterminant(triangle[k],triangle[(k+1)%3],a,axis),last=ParametricDeterminant(triangle[k],triangle[(k+1)%3],b,axis);
    if(orientation<0){first.negative=!first.negative;last.negative=!last.negative;}
    if(stats)stats->exact_orientations+=2;
    const int sa=Sign(first),sb=Sign(last);
    if(sa<=0&&sb<=0)return false;
    if(sa<=0){Fraction root;root.numerator=first;root.numerator.negative=!root.numerator.negative;root.denominator=Difference(last,first);if(compare(low,root)<0)low=root;}
    if(sb<=0){Fraction root;root.numerator=first;root.denominator=Difference(first,last);if(compare(root,high)<0)high=root;}
    if(compare(low,high)>=0)return false;
  }
  return true;
}
bool ParametricOnSegment(const ParametricPoint& a,const ParametricPoint& b,const ParametricPoint& p,KernelStats* stats) {
  for(unsigned axis=0;axis<3;++axis)if(ParametricOrientation(a,b,p,axis,stats))return false;
  for(unsigned k=0;k<3;++k) {
    const auto q=ParametricCoordinate(p,k);
    if(Sign(Difference(q,ParametricCoordinate(a,k)))*Sign(Difference(q,ParametricCoordinate(b,k)))>0)return false;
  }
  return true;
}
std::array<Point,2> AffinePointBounds(Point a,Point b,double t) {
  if(!(Finite(a)&&Finite(b)&&std::isfinite(t)&&t>=0&&t<=1))
    throw std::invalid_argument("Invalid native3D affine point construction.");
  return ParametricPointBounds({a,b,a,t,0});
}
SegmentFreedom Binary64SegmentFreedom(Point a,Point b,Point& midpoint) {
  if(!(Finite(a)&&Finite(b)) || (a.x==b.x && a.y==b.y && a.z==b.z))
    throw std::invalid_argument("Native3D segment freedom needs finite distinct endpoints.");
  const double first[3]={a.x,a.y,a.z},last[3]={b.x,b.y,b.z};uint64_t divisor=0;
  for(unsigned axis=0;axis<3;++axis) {
    if(first[axis]==last[axis])continue;
    double spacing=std::numeric_limits<double>::denorm_min();
    if((first[axis]>0 && last[axis]>0) || (first[axis]<0 && last[axis]<0)) {
      const auto magnitude=std::min(std::abs(first[axis]),std::abs(last[axis]));
      spacing=std::nextafter(magnitude,INFINITY)-magnitude;
    }
    int exponent;std::frexp(spacing,&exponent);const unsigned shift=unsigned(exponent+1073);
    const auto difference=Difference(Coordinate(last[axis]),Coordinate(first[axis]));uint64_t chord=0;
    for(size_t bit=0;bit<difference.limb.size()*32;++bit)if(difference.limb[bit/32]&(uint32_t(1)<<(bit%32))) {
      if(bit<shift)throw std::logic_error("Native3D segment lattice divisibility failed.");
      if(bit-shift>=64)return SegmentFreedom::Unresolved;
      chord|=uint64_t(1)<<(bit-shift);
    }
    divisor=std::gcd(divisor,chord);
  }
  if(divisor==1)return SegmentFreedom::None;
  if(divisor!=2)return SegmentFreedom::Unresolved;
  const auto bounds=AffinePointBounds(a,b,.5);
  if(bounds[0].x!=bounds[1].x || bounds[0].y!=bounds[1].y || bounds[0].z!=bounds[1].z)return SegmentFreedom::None;
  midpoint=bounds[0];return SegmentFreedom::MidpointOnly;
}
bool Barycentric(const Tetrahedron& donor, Point point, std::array<long double, 4>& weights, KernelStats* stats) {
  if (!(Orientation(donor[0], donor[1], donor[2], donor[3], stats) > 0))
    throw std::invalid_argument("Native 3D P1 donor must have positive volume.");
  std::array<long double, 4> values;
  for (size_t i = 0; i < 4; ++i) {
    auto simplex = donor;
    simplex[i] = point;
    values[i] = Determinant(simplex[0], simplex[1], simplex[2], simplex[3], stats, 1e-13L);
    if (values[i] < 0) return false;
  }
  long double sum = 0;
  for (auto v : values) sum += v;
  if (!(sum > 0 && std::isfinite(sum))) throw std::runtime_error("Unusable native 3D P1 weights.");
  for (auto& v : values) v /= sum;
  weights = values;
  return true;
}
bool BarycentricMidpoint(const Tetrahedron& donor,Point a,Point b,std::array<long double,4>& weights,KernelStats* stats) {
  if(!(Finite(a)&&Finite(b)))throw std::invalid_argument("Nonfinite native3D midpoint endpoints.");
  if(!(Orientation(donor[0],donor[1],donor[2],donor[3],stats)>0))
    throw std::invalid_argument("Native3D midpoint donor must have positive volume.");
  auto vertex=[&](Point p){for(size_t i=0;i<4;++i)if(p.x==donor[i].x && p.y==donor[i].y && p.z==donor[i].z)return i;return size_t(4);};
  const auto first=vertex(a),last=vertex(b);
  if(first<4 && last<4){weights={};weights[first]+=.5L;weights[last]+=.5L;return true;}
  const auto volume=ExactVolume(donor,stats);auto negative=volume;negative.negative=!negative.negative;
  const auto denominator=Difference(volume,negative);long double sum=0;
  for(size_t i=0;i<4;++i) {
    auto ta=donor,tb=donor;ta[i]=a;tb[i]=b;
    const auto va=first<4 ? (first==i ? volume:Integer<198>{}) : ExactVolume(ta,stats);
    auto vb=last<4 ? (last==i ? volume:Integer<198>{}) : ExactVolume(tb,stats);vb.negative=!vb.negative;
    const auto numerator=Difference(va,vb);if(Sign(numerator)<0)return false;
    weights[i]=Ratio(numerator,denominator);sum+=weights[i];
  }
  if(!(sum>0))throw std::runtime_error("Unusable native3D exact-midpoint weights.");
  for(auto& w:weights)w/=sum;return true;
}
std::vector<SegmentPiece> TraceSegment(const std::vector<const Cell*>& donors, Point a, Point b, KernelStats* stats,
                                       size_t primary) {
  if (donors.size() > MaximumSegmentDonors) throw std::invalid_argument("Native 3D segment donor budget exceeded.");
  if(primary!=SIZE_MAX && primary>donors.size())throw std::invalid_argument("Invalid native3D source priority prefix.");
  if (!(Finite(a) && Finite(b)) || (a.x == b.x && a.y == b.y && a.z == b.z))
    throw std::invalid_argument("Native 3D source segment must be finite and nonzero.");
  std::vector<WorkPiece> work;
  work.reserve(donors.size());
  for (size_t i = 0; i < donors.size(); ++i) {
    if (!donors[i]) throw std::invalid_argument("Null native 3D segment donor.");
    const auto& v = donors[i]->v;
    const Tetrahedron t{v[0].p, v[1].p, v[2].p, v[3].p};
    if (!(Orientation(t[0], t[1], t[2], t[3], stats) > 0))
      throw std::invalid_argument("Nonpositive native 3D segment donor.");
    bool separated = false;
    // Certified strict negatives discard outside candidates; unresolved/touching/crossing intervals keep exact clipping.
    for (size_t face = 0; face < 4; ++face) {
      auto trial = t; trial[face] = a;
      if (Determinant(trial[0], trial[1], trial[2], trial[3], stats, 1, false) >= 0) continue;
      trial[face] = b;
      if (Determinant(trial[0], trial[1], trial[2], trial[3], stats, 1, false) < 0) { separated = true; break; }
    }
    if (separated) { if (stats) ++stats->segment_separated; continue; }
    WorkPiece piece;
    if (Clip(*donors[i], a, b, piece, stats)) {
      piece.piece.donor = i;
      work.push_back(std::move(piece));
    }
  }
  if (primary<donors.size()) {
    // Original and extension meshes may overlap. Exact cuts select the original field, never a rounded midpoint.
    std::vector<Root> cuts{Root(false),Root(true)};
    cuts.reserve(2+2*work.size());
    for(const auto& p:work){cuts.push_back(p.begin);cuts.push_back(p.end);}
    std::sort(cuts.begin(),cuts.end(),[](const Root& a,const Root& b){return CompareRoot(a,b)<0;});
    cuts.erase(std::unique(cuts.begin(),cuts.end(),[](const Root& a,const Root& b){return CompareRoot(a,b)==0;}),cuts.end());
    std::vector<SegmentPiece> result;
    result.reserve(cuts.size()-1);
    for(size_t k=0;k+1<cuts.size();++k) {
      const WorkPiece* chosen=nullptr;
      for(const auto& p:work) {
        if(CompareRoot(p.begin,cuts[k])>0 || CompareRoot(cuts[k+1],p.end)>0)continue;
        const bool original=p.piece.donor<primary;
        if(chosen && (chosen->piece.donor<primary)==original) {
          if (chosen->support_size>=4 || chosen->support_size!=p.support_size || chosen->support!=p.support ||
              CompareRoot(chosen->begin,p.begin)!=0 || CompareRoot(chosen->end,p.end)!=0)
            throw std::runtime_error("Overlapping/nonconformal native3D same-priority source intervals.");
          if(p.piece.donor<chosen->piece.donor)chosen=&p;
        } else if (!chosen || original) chosen=&p;
      }
      if(!chosen)throw std::runtime_error("Uncovered native3D original/extension source interval.");
      const auto& v=donors[chosen->piece.donor]->v;
      Integer<198> volume;std::array<Integer<198>,4> va,vb;Root lo(false),hi(true);
      if(!ClipInterval({v[0].p,v[1].p,v[2].p,v[3].p},a,b,volume,va,vb,lo,hi,false,stats))
        throw std::logic_error("Selected native3D layered source disappeared.");
      SegmentPiece p;
      p.donor=chosen->piece.donor;
      p.begin=Ratio(cuts[k].numerator,cuts[k].denominator);
      p.end=Ratio(cuts[k+1].numerator,cuts[k+1].denominator);
      p.width=Ratio(Difference(Product(cuts[k+1].numerator,cuts[k].denominator),
                               Product(cuts[k].numerator,cuts[k+1].denominator)),
                    Product(cuts[k+1].denominator,cuts[k].denominator));
      p.begin_weights=AtRoot(cuts[k],va,vb,volume);
      p.end_weights=AtRoot(cuts[k+1],va,vb,volume);
      result.push_back(p);
    }
    return result;
  }
  std::sort(work.begin(), work.end(), [](const WorkPiece& a, const WorkPiece& b) {
    const int start = CompareRoot(a.begin, b.begin);
    if (start) return start < 0;
    const int end = CompareRoot(a.end, b.end);
    return end ? end < 0 : a.piece.donor < b.piece.donor;
  });
  std::vector<SegmentPiece> result;
  result.reserve(work.size());
  Root cursor(false);
  const WorkPiece* previous = nullptr;
  for (const auto& piece : work) {
    if (previous && CompareRoot(previous->begin, piece.begin) == 0 && CompareRoot(previous->end, piece.end) == 0) {
      if (previous->support_size >= 4 || previous->support_size != piece.support_size ||
          previous->support != piece.support)
        throw std::runtime_error("Overlapping/nonconformal native 3D source segment donors.");
      continue;
    }
    const int order = CompareRoot(cursor, piece.begin);
    // DC-16: an uncovered start of length <= the near-coverage tolerance (an endpoint realized just outside a boundary
    // donor) is covered by the first piece (its start weights stand for the endpoint)
    const long double length=std::sqrt(((long double)b.x-a.x)*((long double)b.x-a.x)+((long double)b.y-a.y)*((long double)b.y-a.y)+((long double)b.z-a.z)*((long double)b.z-a.z));
    const long double slack=NearCoverageTolerance(a,b)/std::max(length,std::numeric_limits<long double>::min());
    if (order < 0 && result.empty() && piece.piece.begin <= slack) {
      auto p = piece.piece; p.end = p.begin + p.width; p.begin = 0; p.width = p.end;
      result.push_back(p); cursor = piece.end; previous = &piece; continue;
    }
    if (order != 0)
      throw std::runtime_error(order > 0 ? "Overlapping native 3D source segment intervals."
                                         : "Uncovered native 3D source segment interval.");
    result.push_back(piece.piece);
    cursor = piece.end;
    previous = &piece;
  }
  if (CompareRoot(cursor, Root(true)) != 0) {
    // DC-16: the same tolerance at the far endpoint
    const long double length=std::sqrt(((long double)b.x-a.x)*((long double)b.x-a.x)+((long double)b.y-a.y)*((long double)b.y-a.y)+((long double)b.z-a.z)*((long double)b.z-a.z));
    const long double slack=NearCoverageTolerance(a,b)/std::max(length,std::numeric_limits<long double>::min());
    if (!result.empty() && 1 - (result.back().begin + result.back().width) <= slack) {
      result.back().end = 1; result.back().width = 1 - result.back().begin; return result;
    }
    throw std::runtime_error("Uncovered native 3D source segment endpoint.");
  }
  return result;
}
void ValidateTensor(const Tensor& tensor, KernelStats* stats) {
  const std::array<double, 6> values{{tensor.xx, tensor.xy, tensor.xz, tensor.yy, tensor.yz, tensor.zz}};
  for (const auto value : values)
    if (!std::isfinite(value)) throw std::invalid_argument("Native 3D metric requires finite components.");
  // Sylvester signs distinguish an invalid tensor from an unresolved floating-point factorization.
  if (!(tensor.xx > 0 && Orientation({}, {tensor.xx, tensor.xy, 0}, {tensor.xy, tensor.yy, 0}, {0, 0, 1}, stats) > 0 &&
        Orientation({}, {tensor.xx, tensor.xy, tensor.xz}, {tensor.xy, tensor.yy, tensor.yz},
                    {tensor.xz, tensor.yz, tensor.zz}, stats) > 0))
    throw std::invalid_argument("Native 3D metric is not positive definite.");
}
long double TetrahedronVolume(const Tetrahedron& tet,KernelStats* stats) {
  const auto volume=Determinant(tet[0],tet[1],tet[2],tet[3],stats,1e-13L)/6;
  if(!(volume>0) || !std::isfinite(volume))throw std::invalid_argument("Nonpositive/unrepresentable native 3D integration cell.");
  return volume;
}
double MetricDensity(const Tensor& m,KernelStats* stats,long double* condition) {
  ValidateTensor(m,stats);
  const auto determinant=Determinant({}, {m.xx,m.xy,m.xz},{m.xy,m.yy,m.yz},{m.xz,m.yz,m.zz},stats,1e-13L);
  if(condition) {
    const auto a=Determinant({}, {m.yy,m.yz,0},{m.yz,m.zz,0},{0,0,1},stats,1e-13L);
    const auto b=Determinant({}, {m.xx,m.xz,0},{m.xz,m.zz,0},{0,0,1},stats,1e-13L);
    const auto c=Determinant({}, {m.xx,m.xy,0},{m.xy,m.yy,0},{0,0,1},stats,1e-13L);
    const auto norm=std::max({std::abs((long double)m.xx)+std::abs((long double)m.xy)+std::abs((long double)m.xz),
        std::abs((long double)m.xy)+std::abs((long double)m.yy)+std::abs((long double)m.yz),
        std::abs((long double)m.xz)+std::abs((long double)m.yz)+std::abs((long double)m.zz)});
    *condition=norm*(a+b+c)/determinant; // Row-sum norm times inverse trace; a conditioning indicator.
  }
  const double density=double(std::sqrt(determinant));
  if(!(density>0) || !std::isfinite(density))throw std::runtime_error("Unrepresentable native 3D metric density.");
  return density;
}
long double MetricNorm(Point a, Point b, const Tensor& metric, KernelStats* stats) {
  ValidateTensor(metric, stats);
  if (!(Finite(a) && Finite(b))) throw std::invalid_argument("Nonfinite native 3D metric chord.");
  const auto e = Delta(b, a);
  const std::array<long double, 6> terms{metric.xx * e[0] * e[0],       metric.yy * e[1] * e[1],
                                         metric.zz * e[2] * e[2],       2.L * metric.xy * e[0] * e[1],
                                         2.L * metric.xz * e[0] * e[2], 2.L * metric.yz * e[1] * e[2]};
  long double q = 0, permanent = 0;
  for (auto term : terms) {
    q += term;
    permanent += std::abs(term);
  }
  const auto epsilon = std::numeric_limits<long double>::epsilon();
  if (!(std::isfinite(q) && q > 0 && permanent > std::numeric_limits<long double>::min() / epsilon &&
        q * 1e-13L > 64 * epsilon * permanent)) {
    if (stats) ++stats->exact_norms;
    const std::array<Integer<66>, 3> delta{Difference(Coordinate(b.x), Coordinate(a.x)),
                                           Difference(Coordinate(b.y), Coordinate(a.y)),
                                           Difference(Coordinate(b.z), Coordinate(a.z))};
    Integer<198> total;
    auto add = [&](size_t i, size_t j, double coefficient, bool doubled) {
      auto term = Product(Coordinate(coefficient), Product(delta[i], delta[j]));
      if (doubled) {
        auto negative = term;
        negative.negative = !negative.negative;
        term = Difference(term, negative);
      }
      term.negative = !term.negative;
      total = Difference(total, term);
    };
    add(0, 0, metric.xx, false);
    add(1, 1, metric.yy, false);
    add(2, 2, metric.zz, false);
    add(0, 1, metric.xy, true);
    add(0, 2, metric.xz, true);
    add(1, 2, metric.yz, true);
    if (Sign(total) < 0) throw std::logic_error("Negative exact native 3D SPD chord norm.");
    q = Rounded(total, false);
  } else if (stats)
    ++stats->filtered_norms;
  if (!(std::isfinite(q) && q >= 0)) throw std::runtime_error("Unrepresentable native 3D metric chord norm.");
  return std::sqrt(q);
}
MetricMeasures Measure(const Tetrahedron& tet, const Tensor& metric, KernelStats* stats) {
  ValidateTensor(metric, stats);
  const std::array<double, 6> values{{metric.xx, metric.xy, metric.xz, metric.yy, metric.yz, metric.zz}};
  long double metricScale = 0;
  for (const auto value : values) metricScale = std::max(metricScale, std::abs(static_cast<long double>(value)));
  const long double a = metric.xx / metricScale, b = metric.xy / metricScale, c = metric.xz / metricScale,
                    e = metric.yy / metricScale, f = metric.yz / metricScale, g = metric.zz / metricScale;
  const auto l00 = std::sqrt(a), l10 = b / l00, l20 = c / l00;
  const auto pivot11 = SU2Native3D::FusedMultiplyAdd(-l10, l10, e);
  if (!(pivot11 > 0)) throw std::runtime_error("Unresolved native 3D SPD factorization.");
  const auto l11 = std::sqrt(pivot11), l21 = SU2Native3D::FusedMultiplyAdd(-l20, l10, f) / l11;
  const auto pivot22 = SU2Native3D::FusedMultiplyAdd(-l21, l21, SU2Native3D::FusedMultiplyAdd(-l20, l20, g));
  if (!(pivot22 > 0)) throw std::runtime_error("Unresolved native 3D SPD factorization.");
  const auto l22 = std::sqrt(pivot22);
  const auto determinant = Orientation(tet[0], tet[1], tet[2], tet[3], stats);
  long double coordinateScale = 0;
  for (const auto p : tet)
    for (const auto delta : Delta(p, tet[0])) coordinateScale = std::max(coordinateScale, std::abs(delta));
  if (stats) ++stats->measured_cells;
  if (coordinateScale == 0) return {};
  if (!std::isfinite(coordinateScale) || !std::isfinite(determinant))
    throw std::runtime_error("Native 3D cell scale is not representable.");
  std::array<Vector, 4> transformed{};
  for (size_t k = 0; k < 4; ++k) {
    auto delta = Delta(tet[k], tet[0]);
    for (auto& v : delta) v /= coordinateScale;
    transformed[k] = {
        {l00 * delta[0] + l10 * delta[1] + l20 * delta[2], l11 * delta[1] + l21 * delta[2], l22 * delta[2]}};
  }
  auto edge = [&](size_t i, size_t j) {
    Vector result{};
    for (size_t k = 0; k < 3; ++k) result[k] = transformed[j][k] - transformed[i][k];
    return result;
  };
  long double squared = 0, longest = 0;
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = i + 1; j < 4; ++j) {
      const auto length2 = Norm2(edge(i, j));
      squared += length2;
      longest = std::max(longest, length2);
    }
  const auto volumeFrame = ((determinant / coordinateScale) / coordinateScale) / coordinateScale * l00 * l11 * l22;
  const auto root = std::cbrt(std::abs(volumeFrame) / 2);
  const auto sign = determinant < 0 ? -1.L : 1.L;
  MetricMeasures result;
  result.mean_ratio = squared > 0 ? static_cast<double>(sign * 12 * root * root / squared) : 0;
  long double jacobian = 1;
  for (size_t k = 0; k < 4; ++k) {
    long double denominator = 1;
    for (size_t j = 0; j < 4; ++j)
      if (j != k) denominator *= std::sqrt(Norm2(edge(k, j)));
    const auto local = denominator > 0 ? std::sqrt(2.L) * std::abs(volumeFrame) / denominator : 0;
    jacobian = std::min(jacobian, local);
  }
  result.minimum_scaled_jacobian = static_cast<double>(sign * jacobian);
  const auto sizeScale = coordinateScale * std::sqrt(metricScale);
  result.rms_edge = static_cast<double>(sizeScale * std::sqrt(squared / 6));
  result.maximum_edge = static_cast<double>(sizeScale * std::sqrt(longest));
  if (!(std::isfinite(result.mean_ratio) && std::isfinite(result.minimum_scaled_jacobian) &&
        std::isfinite(result.rms_edge) && std::isfinite(result.maximum_edge)))
    throw std::runtime_error("Native 3D metric measure is not representable.");
  return result;
}
}  // namespace SU2Native3D
