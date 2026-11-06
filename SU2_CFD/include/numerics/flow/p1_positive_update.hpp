/*!
 * \file p1_positive_update.hpp
 * \brief Step lengths of POSITIVE_UPDATE_FLOW: the largest relaxation factor of an implicit update that keeps the
 *        density and the internal energy (minus rho k with SST) above a fixed fraction of their old values.
 *        Scalar code, header only, so that the unit tests can check it without a solver.
 * \version 8.6.0 "Harrier"
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

#include <cmath>
#include "../../../../Common/include/basic_types/datatype_structure.hpp"

/*--- Conservative state U = (rho, m, E), E = rho E, the update U(a) = U0 + a dU, a in [0, a0], a0 the factor of
 * ComputeUnderRelaxationFactor. Q = E - |m|^2/(2 rho) - rho k is the internal energy per volume, k the specific
 * turbulent kinetic energy of SST (k = 0 otherwise), held fixed during the flow step. p = (gamma-1) Q for an ideal gas.
 *
 * Flow step (FlowStepLimit), from a finite admissible start (rho0 > 0, Q0 > 0) and a finite dU:
 *  1. a_r = largest a in [0, a0] with rho(a) >= eps_r rho0 (linear).
 *  2. On [0, a_r] rho > 0, so Q(a) >= eps_e Q0 <=> P(a) = rho(a) [E(a) - eps_e Q0] - |m(a)|^2/2 - k rho(a)^2 >= 0,
 *     P(a) = c0 + c1 a + c2 a^2, c0 = (1 - eps_e) rho0 Q0 > 0. Q is concave in U on rho > 0, so the feasible set
 *     that contains 0 is an interval [0, a*]: a* = smallest positive root of P if it is below a_r, else a_r.
 *  3. The rounded state U0 + a dU is checked again; a is halved until it passes (at most MAX_HALVINGS times),
 *     then a = 0.
 * A nonfinite dU gives a = 0; the caller must also zero dU, since 0 * NaN = NaN. An inadmissible start keeps a0
 * (no claim is made there).
 *
 * Turbulence step of SST (TkeStepLimit): rho k_new = rho k0 + b dK must stay <= (1 - eps_e) q, q the flow internal
 * energy per volume without k, unless rho k does not grow. Linear in the factor b. ---*/

namespace P1PositiveUpdate {

constexpr unsigned short MAXNVAR = 5;  /*!< \brief rho, m (up to 3), E. */
constexpr passivedouble EPS_RHO = 0.1; /*!< \brief Admissibility margin eps_r of the density. */
constexpr passivedouble EPS_Q = 0.1;   /*!< \brief Admissibility margin eps_e of the internal energy. */
constexpr int MAX_HALVINGS = 30;       /*!< \brief Halvings of the rounded check before the update is cancelled. */

/*! \brief Finite test on the passive value (also for AD types). */
inline bool IsFinite(const su2double& x) { return std::isfinite(SU2_TYPE::GetValue(x)); }

/*! \brief Q = E - |m|^2/(2 rho) - rho k, the internal energy per volume (k = 0 without SST). Needs rho != 0. */
inline su2double InternalEnergy(unsigned short nDim, const su2double* U, const su2double& k) {
  su2double m2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) m2 += U[iDim + 1] * U[iDim + 1];
  const su2double q = U[nDim + 1] - 0.5 * m2 / U[0] - U[0] * k;
  return q;
}

/*! \brief Finite, rho > 0, rho >= rhoMin, Q > 0 and Q >= qMin. */
inline bool Admissible(unsigned short nDim, const su2double* U, const su2double& k, const su2double& rhoMin,
                       const su2double& qMin) {
  for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
    if (!IsFinite(U[iVar])) return false;
  }
  if (!IsFinite(k)) return false;
  if (!(U[0] > 0.0) || !(U[0] >= rhoMin)) return false;
  const su2double q = InternalEnergy(nDim, U, k);
  return IsFinite(q) && (q > 0.0) && (q >= qMin);
}

/*!
 * \brief Smallest root a > 0 of P(a) = c0 + c1 a + c2 a^2 with c0 > 0, or -1 if P has no positive root.
 *        Constant (c1 = c2 = 0) and linear (c2 = 0) cases are explicit, the quadratic uses the stable form
 *        t = -(c1 + sign(c1) sqrt(D))/2 with the roots t/c2 and c0/t.
 */
inline su2double SmallestPositiveRoot(const su2double& c0, const su2double& c1, const su2double& c2) {
  if (c2 == 0.0) {
    if (c1 < 0.0) {
      const su2double root = c0 / (-c1);
      return root;
    }
    return -1.0;
  }
  const su2double disc = c1 * c1 - 4.0 * c2 * c0;
  if (disc < 0.0) return -1.0;  // only for c2 > 0, then P > 0 everywhere
  const su2double sq = sqrt(disc);
  su2double t = 0.0;
  if (c1 >= 0.0) {
    t = -0.5 * (c1 + sq);
  } else {
    t = -0.5 * (c1 - sq);
  }
  if (t == 0.0) return -1.0;
  su2double best = -1.0;
  const su2double r1 = t / c2;
  const su2double r2 = c0 / t;
  if (IsFinite(r1) && (r1 > 0.0)) best = r1;
  if (IsFinite(r2) && (r2 > 0.0) && ((best < 0.0) || (r2 < best))) best = r2;
  return best;
}

/*!
 * \brief Largest factor a in [0, alpha0] of U0 + a dU that keeps rho >= epsRho rho0 and Q >= epsQ Q0 on [0, a],
 *        with k fixed (k = 0 without SST). 0 for a nonfinite dU, alpha0 for an inadmissible start.
 * \param[in] nDim - Number of dimensions, U0 and dU have nDim + 2 entries.
 * \param[in] U0 - Old conservative state.
 * \param[in] dU - Increment of the linear solver.
 * \param[in] k - Specific turbulent kinetic energy (SST), else 0.
 * \param[in] alpha0 - Factor of ComputeUnderRelaxationFactor.
 */
inline su2double FlowStepLimit(unsigned short nDim, const su2double* U0, const su2double* dU, const su2double& k,
                               const su2double& alpha0, passivedouble epsRho = EPS_RHO, passivedouble epsQ = EPS_Q) {
  const unsigned short nVar = nDim + 2;

  for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
    if (!IsFinite(dU[iVar])) return 0.0;
  }
  if (!(alpha0 > 0.0)) return alpha0;

  const su2double zero = 0.0;
  if (!Admissible(nDim, U0, k, zero, zero)) return alpha0;

  const su2double rho0 = U0[0];
  const su2double Q0 = InternalEnergy(nDim, U0, k);

  /*--- Step 1: density, linear. ---*/

  su2double alpha = alpha0;
  if (dU[0] < 0.0) {
    const su2double aRho = (1.0 - epsRho) * rho0 / (-dU[0]);
    if (aRho < alpha) alpha = aRho;
  }

  /*--- Step 2: internal energy, P(a) = c0 + c1 a + c2 a^2. ---*/

  su2double m0dm = 0.0, dm2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    m0dm += U0[iDim + 1] * dU[iDim + 1];
    dm2 += dU[iDim + 1] * dU[iDim + 1];
  }
  const su2double E0 = U0[nDim + 1], dE = dU[nDim + 1], dRho = dU[0];
  const su2double c0 = (1.0 - epsQ) * rho0 * Q0;
  const su2double c1 = rho0 * dE + dRho * (E0 - epsQ * Q0) - m0dm - 2.0 * k * rho0 * dRho;
  const su2double c2 = dRho * dE - 0.5 * dm2 - k * dRho * dRho;

  const su2double root = SmallestPositiveRoot(c0, c1, c2);
  if ((root > 0.0) && (root < alpha)) alpha = root;

  /*--- Step 3: check the rounded state that the update produces, halve if it fails. ---*/

  const su2double rhoMin = epsRho * rho0;
  const su2double qMin = epsQ * Q0;
  su2double U[MAXNVAR] = {0.0};
  for (int iHalf = 0; iHalf <= MAX_HALVINGS; ++iHalf) {
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) U[iVar] = U0[iVar] + alpha * dU[iVar];
    if (Admissible(nDim, U, k, rhoMin, qMin)) return alpha;
    alpha *= 0.5;
  }
  return 0.0;
}

/*!
 * \brief k after the clipped conservative update of CScalarSolver: clip((k0 rho + beta dK)/rho, kLower, kUpper).
 */
inline su2double ClippedTke(const su2double& rho, const su2double& k0, const su2double& dK, const su2double& beta,
                            const su2double& kLower, const su2double& kUpper) {
  su2double kNew = (k0 * rho + beta * dK) / rho;
  if (kNew < kLower) kNew = kLower;
  if (kNew > kUpper) kNew = kUpper;
  return kNew;
}

/*!
 * \brief SST turbulence step of POSITIVE_UPDATE_FLOW: largest factor b in [0, beta0] such that the clipped k_new
 *        keeps rho k_new <= (1 - epsQ) q, or does not raise k. 0 for a nonfinite dK; beta0 if the flow state is
 *        not usable (nonfinite or rho <= 0).
 * \param[in] rhoNew - Density of the updated flow solution, the one SetPrimVar uses in q - rho k.
 * \param[in] rho - Density of the conservative k update (flow primitive density, AddClippedSolution).
 * \param[in] q - Internal energy per volume of the updated flow solution, E - |m|^2/(2 rho), without k.
 * \param[in] k0 - Old specific turbulent kinetic energy.
 * \param[in] dK - Increment of rho k from the linear solver.
 * \param[in] beta0 - Factor of ComputeUnderRelaxationFactorHelper.
 * \param[in] kLower, kUpper - Clipping limits of k.
 */
inline su2double TkeStepLimit(const su2double& rhoNew, const su2double& rho, const su2double& q, const su2double& k0, const su2double& dK,
                              const su2double& beta0, const su2double& kLower, const su2double& kUpper,
                              passivedouble epsQ = EPS_Q) {
  if (!IsFinite(dK)) return 0.0;
  if (!(beta0 > 0.0)) return beta0;
  if (!IsFinite(rho) || !IsFinite(q) || !IsFinite(k0) || !(rho > 0.0)) return beta0;
  if (!IsFinite(rhoNew) || !(rhoNew > 0.0)) return beta0;

  const su2double rhokMax = (1.0 - epsQ) * q;

  su2double beta = beta0;
  if (dK > 0.0) {
    /*--- rhoNew k_new = rhoNew k0 + b dK rhoNew / rho, with k_new from the update density rho. ---*/
    const su2double room = rhokMax - rhoNew * k0;
    if (!(room > 0.0)) return 0.0;
    const su2double bK = room * rho / (rhoNew * dK);
    if (bK < beta) beta = bK;
  }

  /*--- Check the rounded clipped k, halve if it fails. ---*/

  for (int iHalf = 0; iHalf <= MAX_HALVINGS; ++iHalf) {
    const su2double kNew = ClippedTke(rho, k0, dK, beta, kLower, kUpper);
    const su2double rhokNew = rhoNew * kNew;
    if (IsFinite(kNew) && ((rhokNew <= rhokMax) || (kNew <= k0))) return beta;
    beta *= 0.5;
  }
  return 0.0;
}

}  // namespace P1PositiveUpdate
