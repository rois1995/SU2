/*!
 * \file CNativePredicates3D.cpp
 * \brief Strict tetrahedral predicates and scale-independent metric measures.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeMesh3D.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
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
long double Determinant(Point a, Point b, Point c, Point d, KernelStats* stats, long double relative_error) {
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
bool Clip(const Cell& cell, Point a, Point b, WorkPiece& result, KernelStats* stats) {
  const Tetrahedron t{cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p};
  const auto volume = ExactVolume(t, stats);
  if (Sign(volume) <= 0) throw std::invalid_argument("Nonpositive native 3D segment donor.");
  std::array<Integer<198>, 4> va, vb;
  Root lo(false), hi(true);
  for (size_t i = 0; i < 4; ++i) {
    auto ta = t, tb = t;
    ta[i] = a;
    tb[i] = b;
    va[i] = ExactVolume(ta, stats);
    vb[i] = ExactVolume(tb, stats);
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
    if (CompareRoot(lo, hi) >= 0) return false;
  }
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
std::vector<SegmentPiece> TraceSegment(const std::vector<const Cell*>& donors, Point a, Point b, KernelStats* stats) {
  if (donors.size() > MaximumSegmentDonors) throw std::invalid_argument("Native 3D segment donor budget exceeded.");
  if (!(Finite(a) && Finite(b)) || (a.x == b.x && a.y == b.y && a.z == b.z))
    throw std::invalid_argument("Native 3D source segment must be finite and nonzero.");
  std::vector<WorkPiece> work;
  work.reserve(donors.size());
  for (size_t i = 0; i < donors.size(); ++i) {
    if (!donors[i]) throw std::invalid_argument("Null native 3D segment donor.");
    WorkPiece piece;
    if (Clip(*donors[i], a, b, piece, stats)) {
      piece.piece.donor = i;
      work.push_back(std::move(piece));
    }
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
    if (order != 0)
      throw std::runtime_error(order > 0 ? "Overlapping native 3D source segment intervals."
                                         : "Uncovered native 3D source segment interval.");
    result.push_back(piece.piece);
    cursor = piece.end;
    previous = &piece;
  }
  if (CompareRoot(cursor, Root(true)) != 0) throw std::runtime_error("Uncovered native 3D source segment endpoint.");
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
  const auto pivot11 = std::fma(-l10, l10, e);
  if (!(pivot11 > 0)) throw std::runtime_error("Unresolved native 3D SPD factorization.");
  const auto l11 = std::sqrt(pivot11), l21 = std::fma(-l20, l10, f) / l11;
  const auto pivot22 = std::fma(-l21, l21, std::fma(-l20, l20, g));
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
