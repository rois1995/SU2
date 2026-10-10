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
  if (a.negative != b.negative) {
    uint64_t carry = 0;
    for (size_t i = 0; i < N; ++i) {
      const uint64_t sum = uint64_t(a.limb[i]) + b.limb[i] + carry;
      result.limb[i] = static_cast<uint32_t>(sum);
      carry = sum >> 32;
    }
    if (carry) throw std::logic_error("Native 3D predicate integer bound exceeded.");
    result.negative = a.negative;
  } else {
    const bool less = Compare(a, b) < 0;
    const auto& larger = less ? b : a;
    const auto& smaller = less ? a : b;
    uint64_t borrow = 0;
    for (size_t i = 0; i < N; ++i) {
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
  for (size_t i = 0; i < A; ++i) {
    if (!a.limb[i]) continue;
    uint64_t carry = 0;
    for (size_t j = 0; j < B; ++j) {
      const uint64_t sum = uint64_t(a.limb[i]) * b.limb[j] + result.limb[i + j] + carry;
      result.limb[i + j] = static_cast<uint32_t>(sum);
      carry = sum >> 32;
    }
    result.limb[i + B] = static_cast<uint32_t>(carry);
  }
  return result;
}
long double Rounded(const Integer<198>& value) {
  int top = 197;
  while (top >= 0 && value.limb[top] == 0) --top;
  if (top < 0) return 0;
  const int bottom = std::max(0, top - 2);
  long double mantissa = 0;
  for (int i = top; i >= bottom; --i) mantissa = std::ldexp(mantissa, 32) + value.limb[i];
  long double result = std::ldexp(mantissa, bottom * 32 - 3222);
  if (result == 0) result = std::numeric_limits<long double>::denorm_min();
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
  return Rounded(Difference(Difference(first, second), negativeThird));
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
