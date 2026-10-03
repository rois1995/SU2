/*!
 * \file CAccurateSum.hpp
 * \brief Compensated (Neumaier) sums of doubles, kept exact to the stated error bound under SU2's fast-math flags.
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstddef>

/*!
 * \class CAccurateSum
 * \brief Kernels of the accurate global sums of the distributed solution transfer (MPI_TRANSFER_PLAN.md 3.6).
 * \note Algorithm: per rank a Neumaier compensated sum of its terms gives (s_r, c_r) and a_r = sum |x_i|; the triples
 *       of all ranks are merged by a second Neumaier pass over s_0, c_0, s_1, c_1, ... in rank order; the result is
 *       (s + c) of that pass, rounded once. Error bound (Higham 4.3, both passes): |result - exact| <= 2 eps |exact| +
 *       O((n + P) eps^2) A, A = sum of the a_r, eps = 2^-53.
 *
 *       The error terms of Neumaier's method are destroyed by reassociation, which SU2 enables (-ffast-math). These
 *       kernels are therefore compiled in their own translation unit and static library with -fno-fast-math
 *       -ffp-contract=off appended to the SU2 flags, they are not inline, and callers pass arrays across that boundary.
 *       The accuracy tests of the unit-test drivers use them as linked in production.
 *
 *       Supported range: A < DBL_MAX / 4. Every term, every partial (s, c, a), every merge step and the result must be
 *       finite; the functions report it (false), the caller turns it into a (collective) error. Pure doubles: no SU2
 *       types, no MPI (the communication is CAccurateSumBatch in CDistributedSearch.hpp).
 */
class CAccurateSum {
 public:
  /*!
   * \brief Local compensated sum of x[0], x[stride], ..., x[(n-1) stride].
   * \param[out] triple - s, c, a: s + c is the sum, a the sum of the absolute values.
   * \param[out] firstBad - If not null: index (0..n-1) of the first nonfinite term, n if none.
   * \return False if a term, s, c or a is not finite or a >= DBL_MAX / 4 (the triple is then NaN).
   */
  static bool Local(const double* x, size_t n, size_t stride, double* triple, size_t* firstBad = nullptr);

  /*!
   * \brief Merge of the triples of nRank ranks (3 doubles each, in rank order): second Neumaier pass over
   *        s_0, c_0, s_1, c_1, ..., result s + c.
   * \param[out] result - The sum.
   * \param[out] absSum - If not null: A = sum of the a_r.
   * \return False if an input, an intermediate or the result is not finite, or A >= DBL_MAX / 4.
   */
  static bool Merge(const double* triples, size_t nRank, double* result, double* absSum = nullptr);

  /*!
   * \brief Merge of a list of (hi, lo) pairs (2 doubles each, in order): Neumaier pass over hi_0, lo_0, hi_1, ...
   * \return False if not finite.
   */
  static bool MergePairs(const double* pairs, size_t nPair, double* result);
};

/*!
 * \class CNeumaierSum
 * \brief Running compensated sum (protected arithmetic, the methods are not inline).
 */
class CNeumaierSum {
 public:
  /*! \brief Add a term. */
  void Add(double x);

  /*! \brief s + c, rounded once. */
  double Sum() const;

  double s = 0.0; /*!< \brief Running sum. */
  double c = 0.0; /*!< \brief Running compensation. */
  double a = 0.0; /*!< \brief Sum of the absolute values. */
};
