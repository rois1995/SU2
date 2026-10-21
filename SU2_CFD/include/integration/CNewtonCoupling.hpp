/*!
 * \file CNewtonCoupling.hpp
 * \brief Assembled flow/turbulence cross Jacobians of a coupled assembled Newton group.
 * \author SU2 Contributors
 */
#pragma once

#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/linear_algebra/CSysVector.hpp"
#include "../solvers/CSolver.hpp"
#include <vector>

/*!
 * \class CNewtonCoupling
 * \brief Off-diagonal blocks J_tf = dR_turb/dU_flow and J_ft = dR_flow/dU_turb on the edge graph.
 * \details The native solvers assemble only the diagonal blocks. These blocks add the dominant local couplings of
 * the strongly coupled linearization (Clerici, Spalart, Alauzet, JCP 514, 2024, App. A):
 * - J_tf, convection: first-order upwind turbulence flux through the face velocity (and the density weighting of
 *   conservative models), at frozen specific turbulence variables, matching the Newton coordinates.
 * - J_ft, diffusion: flow viscous and heat fluxes through the eddy viscosity, with frozen gradients.
 * - J_tf, SST k row: the source is linear in density at frozen specific k, omega and strain (dS_k/drho = S_k/rho).
 *   The omega row's density column is left out: it is orders of magnitude above the omega diagonal near walls.
 * - J_ft, SST: k is part of the static energy, e = E - u^2/2 - k, so rho k acts as -rho E (native flow Jacobian
 *   column) except for the explicit rho E of the energy convection.
 * - J_tf, sources through the vorticity and strain magnitudes, chained through Green-Gauss gradient weights to the
 *   velocities of the point and its neighbours (the native least-squares gradient uses the same stencil); the SST
 *   omega row is left out, as its density column. Below |omega| = 1e-18 no vorticity chain is used.
 * - J_tf, SA: the source dependence on density through nu = mu/rho.
 * - Incompressible flow: the same terms in the unknowns (p, u, h), without density columns, the SST energy terms
 *   and the viscous work.
 * - Periodic points keep their partial rows, as the native assembly: products are P C P (see
 *   CNewtonEquationSet::ApplyCoupling), and the source chain of each copy carries the summed source sensitivities.
 * Laminar-viscosity (temperature) terms, boundary closures and MUSCL face values are not included.
 * Rows of prescribed equations are zero; symmetry and Euler-wall momentum rows are projected as the native rows.
 * Blocks are stored per point (diagonal) and per edge side, and applied by gathering over the neighbours of each
 * owned point, so assembly and products need no edge colouring.
 */
class CNewtonCoupling {
 public:
  using Vector = CSysVector<passivedouble>;
  static constexpr unsigned short MaxDim = 3;
  /*! \brief Largest flow (nDim+2) or turbulence block size handled by the products. */
  static constexpr unsigned short MaxVariables = MaxDim + 2;

  CNewtonCoupling(CGeometry* geometry, CConfig* config, CSolver** solvers, CNumerics** turbNumerics,
                  unsigned short flowOffset, unsigned short turbOffset, unsigned short nTurb);

  /*! \brief Build the blocks from the live flow and turbulence states (call inside the parallel region). */
  void Assemble();

  /*!
   * \brief Set the cross rows of y to C x on owned points, in the group's unscaled coordinates.
   * \details Halo rows and the rows of other equations are left unchanged.
   * \param[in] lowerOnly - Apply only J_tf (block Gauss-Seidel coupling); otherwise J_tf and J_ft.
   * \param[in] isFree - isFree(point, group variable); other columns do not contribute.
   */
  template <class IsFree>
  void Apply(const Vector& x, Vector& y, bool lowerOnly, const IsFree& isFree) const {
    /*--- Mask the columns once per point instead of once per edge. ---*/
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
      for (auto c = 0u; c < nFlow; ++c)
        maskedFlow[p * nFlow + c] = isFree(p, flowOffset + c) ? x(p, flowOffset + c) : 0.0;
      if (!lowerOnly)
        for (auto c = 0u; c < nTurb; ++c)
          maskedTurb[p * nTurb + c] = isFree(p, turbOffset + c) ? x(p, turbOffset + c) : 0.0;
    }
    END_SU2_OMP_FOR
    SU2_OMP_FOR_DYN(256)
    for (auto i = 0ul; i < geometry->GetnPointDomain(); ++i) {
      double turb[MaxVariables] = {}, flow[MaxVariables] = {};
      auto add = [&](unsigned long point, const passivedouble* tf, const passivedouble* ft) {
        const auto* flowValue = &maskedFlow[point * nFlow];
        for (auto v = 0u; v < nTurb; ++v)
          for (auto c = 0u; c < nFlow; ++c) turb[v] += tf[v * nFlow + c] * flowValue[c];
        if (lowerOnly) return;
        const auto* turbValue = &maskedTurb[point * nTurb];
        for (auto r = 0u; r < nFlow; ++r)
          for (auto c = 0u; c < nTurb; ++c) flow[r] += ft[r * nTurb + c] * turbValue[c];
      };
      add(i, &tfDiagonal[i * nTurb * nFlow], &ftDiagonal[i * nFlow * nTurb]);
      for (auto k = rowStart[i]; k < rowStart[i + 1]; ++k) {
        const auto slot = neighbourSlot[k];
        add(neighbourPoint[k], &tfEdge[slot * nTurb * nFlow], &ftEdge[slot * nFlow * nTurb]);
      }
      for (auto v = 0u; v < nTurb; ++v) y(i, turbOffset + v) = turb[v];
      if (!lowerOnly)
        for (auto r = 0u; r < nFlow; ++r) y(i, flowOffset + r) = flow[r];
    }
    END_SU2_OMP_FOR
  }

 private:
  CGeometry* geometry;
  CConfig* config;
  CSolver** solvers;
  CNumerics** turbNumerics;
  unsigned short nDim, nFlow, nTurb, flowOffset, turbOffset;
  bool sst, negativeSA, incompressible;
  /*--- Row-major blocks: tf is nTurb x nFlow, ft is nFlow x nTurb; edge slot 0 is row node 0, slot 1 row node 1. ---*/
  std::vector<passivedouble> tfDiagonal, tfEdge, ftDiagonal, ftEdge;
  std::vector<passivedouble> eddyDerivative; /*!< \brief d mu_t / d (turbulence unknowns) per point. */
  /*! \brief Point-local source sensitivities dR_turb/d|omega|, dR_turb/dS (strain) and, for SA, dR_turb/drho. */
  std::vector<passivedouble> vorticityDerivative, strainDerivative, densityDerivative;
  std::vector<su2double> savedResidual, savedValue;
  su2activematrix periodicWork;
  std::vector<passivedouble> savedResidualBase;
  /*--- Neighbours of the owned points (CSR) with the edge slot of the block in that row. ---*/
  std::vector<unsigned long> rowStart, neighbourPoint, neighbourSlot;
  mutable std::vector<passivedouble> maskedFlow, maskedTurb; /*!< \brief Free columns of the input of Apply. */

  /*! \brief Coupling block of a point (diagonal) or of an edge side (2 * edge + side), see ZeroPrescribedRows. */
  passivedouble* TF(bool diagonal, unsigned long index) {
    return (diagonal ? tfDiagonal.data() : tfEdge.data()) + index * nTurb * nFlow;
  }
  passivedouble* FT(bool diagonal, unsigned long index) {
    return (diagonal ? ftDiagonal.data() : ftEdge.data()) + index * nFlow * nTurb;
  }
  void EddyViscosityDerivatives();
  /*!
   * \brief Differentiate the native turbulence source with respect to the vorticity and strain magnitudes (and the
   *        SA density) by perturbing them at all points at once: each point's source depends only on its own values.
   * \details Covers every SA variant (negative, BCM, corrections) and SST without repeating the model. Runs the
   *          source explicitly; the turbulence residual is restored.
   */
  void SourceSensitivities();
  void ProjectSymmetryRows();
  void ZeroPrescribedRows();
};
