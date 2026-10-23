/*!
 * \file CNativeFma.hpp
 * \brief Correctly rounded long-double fused multiply-add without floating-point environment traffic.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
namespace SU2Native3D {
/*! RN(x*y+z) in x87 extended precision, bitwise equal to std::fma(long double...) (glibc fmal) for every input.
 *  Normal operands well inside the exponent range use Dekker's exact product, Knuth's exact sum and an arithmetic
 *  round-to-odd (Boldo-Melquiond, the algorithm of glibc's ldbl-96 fmal) instead of fmal's rounding-mode switches;
 *  zeros, subnormals, extreme exponents and non-finite operands call std::fma. Compiled in the strict-arithmetic
 *  library (no fast math): callers in fast-math translation units must not inline their own copy.
 *  Assumes the default round-to-nearest mode and x87 extended precision control (the SU2 runtime state). */
#if defined(__GNUC__)
__attribute__((const, leaf, nothrow))  // A pure leaf function of its arguments, like the fmal builtin it replaces.
#endif
long double FusedMultiplyAdd(long double x, long double y, long double z) noexcept;
}  // namespace SU2Native3D
