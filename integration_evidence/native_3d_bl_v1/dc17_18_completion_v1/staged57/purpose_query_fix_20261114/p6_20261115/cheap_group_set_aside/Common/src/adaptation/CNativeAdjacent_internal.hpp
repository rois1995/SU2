/* Private exact adjacent-value stepping used by outward interval arithmetic. */
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace SU2NativeGeometry::detail {

inline long double Adjacent(long double value, bool upward) {
#if defined(__x86_64__) && defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__ == 64 && \
    defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  static_assert(std::numeric_limits<long double>::radix == 2 &&
                std::numeric_limits<long double>::max_exponent == 16384,
                "x87 stepping requires the 80-bit extended format");
  struct X87 { uint64_t significand; uint16_t sign_exponent; };
  X87 bits{};
  constexpr std::size_t kX87PayloadBytes = sizeof(uint64_t) + sizeof(uint16_t);
  std::memcpy(&bits, &value, kX87PayloadBytes);
  const unsigned exponent = bits.sign_exponent & 0x7fff;
  const bool negative = (bits.sign_exponent & 0x8000) != 0;

  const uint64_t integer_bit = UINT64_C(1) << 63;
  const uint64_t fraction_mask = integer_bit - 1;
  const bool canonical_subnormal = exponent == 0 && (bits.significand & integer_bit) == 0;
  const bool canonical_normal = exponent >= 1 && exponent < 0x7ffe &&
                                (bits.significand & integer_bit) != 0;
  // Keep the existing fast normal range (exp 2..0x7ffd) and add canonical
  // zero/subnormal and min-normal-binade stepping. Noncanonical and exceptional
  // payloads retain the implementation's library behavior.
  if (canonical_subnormal || canonical_normal) {
    if (bits.significand == 0) {
      bits.sign_exponent = upward ? 0 : 0x8000;
      bits.significand = 1;
    } else if (upward != negative) {  // increase magnitude
      if (bits.significand == UINT64_MAX) {
        bits.significand = integer_bit; ++bits.sign_exponent;
      } else {
        ++bits.significand;
        if (exponent == 0 && bits.significand == integer_bit) ++bits.sign_exponent;
      }
    } else {  // decrease magnitude
      if (exponent > 0 && bits.significand == integer_bit) {
        bits.significand = exponent == 1 ? fraction_mask : UINT64_MAX;
        --bits.sign_exponent;
      } else {
        --bits.significand;
      }
    }
    std::memcpy(&value, &bits, kX87PayloadBytes);
    return value;
  }
#endif
  return std::nextafter(value, upward ? INFINITY : -INFINITY);
}

}  // namespace SU2NativeGeometry::detail
