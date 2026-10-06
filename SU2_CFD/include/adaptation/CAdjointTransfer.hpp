/*!
 * \file CAdjointTransfer.hpp
 * \brief Transfer of a discrete adjoint problem (primal and adjoint solutions) to a new mesh (goal-oriented adaptation).
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

#include <memory>
#include <vector>

#include "CSolutionTransfer.hpp"

/*!
 * \class CDiscAdjTransfer
 * \brief Solution transfer of a discrete adjoint problem: the primal solution by the transfer of ADAP_TRANSFER (which
 *        sees the flow and turbulence solvers only), then the adjoint solution psi of every adjoint solver.
 * \note Adjoint (stage G2 of STAGE_G_PLAN.md, G2G3_PLAN.md 1.3):
 *       - warm start: barycentric (P1) interpolation of psi at the owned points of the new mesh, a plain weighted sum (no
 *         admissibility, no conservation: psi is only the start of the adjoint fixed point on the new mesh), with the
 *         stencil rules and the distance limit of the distributed barycentric transfer (CDistributedLocator over the
 *         donor, absolute limit 2 ADAP_HAUSD; points of a marker take the closest point of the donor faces of the same
 *         marker, the others the canonical containing element or the nearest donor face; points beyond the limit stop
 *         the run on all ranks). The donor values come from the owned donor rows (CPointDirectory), so the result does
 *         not depend on the partitions. Then the halo values (communication), the old solution (= new) and the
 *         restriction to the coarse levels.
 *       - cold start: psi keeps the initial state of a new adjoint solver (the constructor value).
 *       The interpolated psi depends on the CFL and the Jacobian of the recording on the donor mesh: an approximate start
 *       only. Extra adjoint variables of the direct solver (CSolver::RegisterSolutionExtra) are not supported.
 */
class CDiscAdjTransfer final : public CSolutionTransfer {
 public:
  /*!
   * \brief Statistics of the adjoint part of the last transfer (global).
   */
  struct Summary {
    unsigned long nPoint = 0;           /*!< \brief Points of the new mesh. */
    unsigned long nOutside = 0;         /*!< \brief Of them, outside the donor mesh. */
    passivedouble maxDistance = 0.0;    /*!< \brief Largest distance to the donor (face stencils). */
    unsigned short nSolver = 0;         /*!< \brief Adjoint solvers transferred. */
    std::vector<passivedouble> donorMax, newMax; /*!< \brief max |psi| per variable (adjoint solvers in order). */
    passivedouble time = 0.0;           /*!< \brief Wall time of the adjoint part (seconds). */
  };

  /*!
   * \param[in] primal - Transfer of the primal solution (barycentric, conservative or free stream).
   * \param[in] warmStart - Interpolate psi (ADAP_ADJ_WARM_START), else keep the initial adjoint state.
   */
  CDiscAdjTransfer(std::unique_ptr<CSolutionTransfer> primal, bool warmStart)
      : primal(std::move(primal)), warmStart(warmStart) {}

  void Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver) override;

  Report GetReport() const override { return primal ? primal->GetReport() : Report(); }

  const Summary& GetSummary() const { return summary; }

  /*!
   * \brief The adjoint part alone (also for the tests): psi of every adjoint solver of the new mesh from the donor.
   * \note Collective. The donor and the new mesh must have the same adjoint solvers (same indices and variables).
   */
  void TransferAdjoint(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver);

 private:
  /*--- Collective: the adjoint solvers (indices) of the problem, checked per slot against the donor and over the ranks
   *    (errors on all ranks). ---*/
  static std::vector<unsigned short> AdjointSolvers(const CMeshDonor& donor, CSolver*** solver);

  std::unique_ptr<CSolutionTransfer> primal;
  bool warmStart;
  Summary summary;
};
