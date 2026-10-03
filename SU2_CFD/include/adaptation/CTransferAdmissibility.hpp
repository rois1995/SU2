/*!
 * \file CTransferAdmissibility.hpp
 * \brief The admissibility predicate of the solution transfers (flow with the solver's internal energy, finite values,
 *        turbulence within the solver bounds), shared by the barycentric and the conservative transfer.
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

#include "../../../Common/include/basic_types/datatype_structure.hpp"

class CFluidModel;
class CTurbSolver;

/*!
 * \class CTransferAdmissibility
 * \brief Admissibility of the complete state of a point (flow and turbulence of one time level) for the solution
 *        transfers (MPI_TRANSFER_PLAN.md 4.6, 5.11).
 * \note - Admissible (full predicate): every flow and turbulence value finite (std::isfinite, so NaN is caught before
 *         any bounding comparison), the turbulence within the solver bounds (GetLowerLimit/GetUpperLimit; the negative
 *         SA variant has no lower bound), and the flow admissible with the solver's internal energy (SST subtracts k):
 *         CBarycentricTransfer::AdmissibleState.
 *       - Representations: raw solver turbulence (nu_tilde; k, omega), used by the barycentric transfer, or conservative
 *         (nu_tilde; rho k, rho omega), used by the conservative transfer: the adapter divides by rho (rho > 0 needed,
 *         else not admissible) before the predicate.
 *       - Stage-1 predicate P1 of the conservative recovery (conservative representation): every value finite, rho > 0,
 *         and the flow admissible with rho e = rho E - |rho u|^2 / (2 rho) - max(rho k, k_lo rho) (SST; SA and laminar:
 *         no k term). Convex for an ideal gas (rho e is concave), so patch means of P1 states are P1; omega and nu_tilde
 *         only need to be finite. After the pointwise bounds of stage 2 a P1 state satisfies the full predicate.
 *       The fluid model's state is changed by the evaluation (not thread safe).
 */
class CTransferAdmissibility {
 public:
  /*!
   * \param[in] fluidModel - Fluid model of the flow solver.
   * \param[in] nDim - Number of dimensions (nDim + 2 flow variables).
   * \param[in] turbSolver - Turbulence solver (bounds), nullptr for laminar or Euler flow.
   * \param[in] sst - The turbulence model is SST (k enters the internal energy; conservative variables rho k, rho omega).
   */
  CTransferAdmissibility(CFluidModel& fluidModel, unsigned short nDim, const CTurbSolver* turbSolver, bool sst);

  unsigned short GetnVarFlow() const { return nVarFlow; }
  unsigned short GetnVarTurb() const { return nVarTurb; }
  bool IsSST() const { return sst; }

  /*! \brief Full predicate, raw turbulence (flow: nDim + 2 values, turbulence: nVarTurb raw values). */
  bool Admissible(const su2double* flow, const su2double* turbulence) const;

  /*! \brief Full predicate on the fields of one level in the conservative representation (flow, then turbulence). */
  bool AdmissibleConservative(const su2double* fields) const;

  /*! \brief Stage-1 predicate P1 on the fields of one level in the conservative representation. */
  bool AdmissibleStage1(const su2double* fields) const;

  /*! \brief Lower and upper bound of a raw turbulence variable. */
  su2double Lower(unsigned short iVar) const;
  su2double Upper(unsigned short iVar) const;

  /*!
   * \brief A raw turbulence value within the bounds: finite values clipped, NaN left as NaN.
   * \param[in,out] counter - If not null, incremented when the value moves by more than 1e-10 of the bound.
   */
  su2double Bounded(unsigned short iVar, su2double value, unsigned long* counter = nullptr) const;

 private:
  CFluidModel* fluidModel;
  unsigned short nDim, nVarFlow, nVarTurb;
  const CTurbSolver* turbSolver;
  bool sst;
};
