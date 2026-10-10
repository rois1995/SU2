/* Bit-for-bit comparison with the original schoolbook product; private arithmetic stays out of the public API. */
#include "Common/src/adaptation/CNativePredicates3D.cpp"
#include <cassert>
#include <iostream>
#include <random>
using namespace SU2Native3D;
template <size_t A, size_t B>
Integer<A + B> Reference(const Integer<A>& a, const Integer<B>& b) {
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
template <size_t N>
Integer<N> Sample(std::mt19937_64& random, size_t pattern) {
  Integer<N> value;
  value.negative = random() & 1;
  if (pattern == 0) return value;
  if (pattern == 1) value.limb[random() % N] = uint32_t(1) << (random() % 32);
  if (pattern == 2) value.limb[random() % N] = uint32_t(random());
  if (pattern == 3) {
    const auto begin = random() % N, end = begin + random() % (N - begin);
    for (size_t i = begin; i <= end; ++i) value.limb[i] = uint32_t(random());
  }
  if (pattern == 4)
    for (auto& limb : value.limb) limb = std::numeric_limits<uint32_t>::max();
  if (pattern == 5)
    for (auto& limb : value.limb) limb = random() % 4 ? 0 : uint32_t(random());
  if (pattern == 6) {
    value.limb[0] = std::numeric_limits<uint32_t>::max();
    value.limb[N - 1] = std::numeric_limits<uint32_t>::max();
  }
  return value;
}
template <size_t A, size_t B>
size_t Check(std::mt19937_64& random) {
  size_t count = 0;
  for (size_t pa = 0; pa < 7; ++pa)
    for (size_t pb = 0; pb < 7; ++pb)
      for (size_t repeat = 0; repeat < 16; ++repeat) {
        const auto a = Sample<A>(random, pa), b = Sample<B>(random, pb);
        const auto expected = Reference(a, b), actual = Product(a, b);
        assert(actual.limb == expected.limb && actual.negative == expected.negative);
        ++count;
      }
  return count;
}
template <size_t N>
Integer<N> ReferenceDifference(const Integer<N>& a, const Integer<N>& b) {
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
template <size_t N>
size_t CheckDifference(std::mt19937_64& random) {
  size_t count = 0;
  for (size_t pa = 0; pa < 7; ++pa)
    for (size_t pb = 0; pb < 7; ++pb)
      for (size_t repeat = 0; repeat < 16; ++repeat) {
        const auto a = Sample<N>(random, pa), b = Sample<N>(random, pb);
        Integer<N> expected, actual;
        bool reference_overflow = false, overflow = false;
        try { expected = ReferenceDifference(a, b); } catch (const std::logic_error&) { reference_overflow = true; }
        try { actual = Difference(a, b); } catch (const std::logic_error&) { overflow = true; }
        assert(overflow == reference_overflow);
        if (!overflow) assert(actual.limb == expected.limb && actual.negative == expected.negative);
        ++count;
      }
  return count;
}
int main() {
  std::mt19937_64 random(3102026);
  const auto products = Check<66, 66>(random) + Check<66, 132>(random) + Check<198, 198>(random) +
                     Check<396, 396>(random);
  const auto differences = CheckDifference<66>(random) + CheckDifference<132>(random) +
                           CheckDifference<198>(random) + CheckDifference<396>(random);
  std::cout << "PASS " << products << " exact products and " << differences
            << " exact differences (zero/signed/sparse/dense/carry/borrow/overflow/extreme limbs)\n";
}
