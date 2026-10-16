/*!
 * \file CNativePredicates2D.cpp
 * \brief Filtered 2D orientation with an exact integer fallback for finite binary64 coordinates.
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 * See the SU2 license for redistribution terms and warranty limitations.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace SU2Native2D {
namespace {

/*--- Every binary64 coordinate is an integer multiple of 2^-1074. Its magnitude needs at most 2098 bits;
 *    coordinate differences need 2099, and a difference of two products at most 4199. Arrays of 66/132
 *    32-bit limbs therefore suffice for the complete finite binary64 range, without heap allocation. ---*/
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
    if (carry) throw std::logic_error("Native orientation integer bound exceeded.");
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
                "Native predicates require IEEE binary64.");
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

Integer<132> Product(const Integer<66>& a, const Integer<66>& b) {
  Integer<132> result;
  result.negative = a.negative != b.negative;
  for (size_t i = 0; i < 66; ++i) {
    if (!a.limb[i]) continue;
    uint64_t carry = 0;
    for (size_t j = 0; j < 66; ++j) {
      const uint64_t sum = uint64_t(a.limb[i]) * b.limb[j] + result.limb[i + j] + carry;
      result.limb[i + j] = static_cast<uint32_t>(sum);
      carry = sum >> 32;
    }
    result.limb[i + 66] = static_cast<uint32_t>(carry);
  }
  return result;
}

long double RoundedDeterminant(const Integer<132>& value) {
  int top = 131;
  while (top >= 0 && value.limb[top] == 0) --top;
  if (top < 0) return 0;
  const int bottom = std::max(0, top - 2);
  long double mantissa = 0;
  for (int i = top; i >= bottom; --i) mantissa = std::ldexp(mantissa, 32) + value.limb[i];
  /*--- Products of the scaled coordinate integers carry scale 2^-2148. Retain sign even if the platform's
   *    long double cannot represent this nonzero magnitude; callers must separately admit usable scales. ---*/
  long double result = std::ldexp(mantissa, bottom * 32 - 2148);
  if (result == 0) result = std::numeric_limits<long double>::denorm_min();
  return value.negative ? -result : result;
}

}  // namespace

long double NormalizedDeterminant(double xx, double xy, double yy) {
  if (!(std::isfinite(xx) && std::isfinite(xy) && std::isfinite(yy) && xx > 0 && yy > 0)) return -1;
  const double scale = std::max({std::abs(xx), std::abs(xy), std::abs(yy)});
  const long double a = static_cast<long double>(xx) / scale, b = static_cast<long double>(xy) / scale,
                    c = static_cast<long double>(yy) / scale;
  return a * c - b * b;
}

/*--- Compile this unit without fast math or contraction, like CAccurateSum. This filter includes the rounding
 *    of coordinate differences, products and subtraction. Tiny/uncertain products use the integer fallback. ---*/
long double Orientation(double ax, double ay, double bx, double by, double cx, double cy) {
  if (!(std::isfinite(ax) && std::isfinite(ay) && std::isfinite(bx) && std::isfinite(by) && std::isfinite(cx) &&
        std::isfinite(cy)))
    throw std::invalid_argument("Native orientation requires finite coordinates.");
  const long double abx = static_cast<long double>(bx) - ax, aby = static_cast<long double>(by) - ay;
  const long double acx = static_cast<long double>(cx) - ax, acy = static_cast<long double>(cy) - ay;
  const long double left = abx * acy, right = aby * acx;
  const long double determinant = left - right, permanent = std::abs(left) + std::abs(right);
  const auto epsilon = std::numeric_limits<long double>::epsilon();
  if (std::isfinite(determinant) && permanent > std::numeric_limits<long double>::min() / epsilon &&
      std::abs(determinant) > 8 * epsilon * permanent)
    return determinant;
  const auto x = Coordinate(ax), y = Coordinate(ay);
  const auto exact = Difference(Product(Difference(Coordinate(bx), x), Difference(Coordinate(cy), y)),
                                Product(Difference(Coordinate(by), y), Difference(Coordinate(cx), x)));
  return RoundedDeterminant(exact);
}

}  // namespace SU2Native2D
