/*!
 * \file centered.hpp
 * \brief Centered convective schemes.
 * \author P. Gomes, F. Palacios, T. Economon
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

#include "../../CNumericsSIMD.hpp"
#include "../../util.hpp"
#include "../variables.hpp"
#include "common.hpp"
#include "../../../numerics/flow/p1_sst_matched.hpp"
#include "../../../variables/CEulerVariable.hpp"
#include "../../../../../Common/include/geometry/CGeometry.hpp"

/*!
 * \class CCenteredBase
 * \ingroup ConvDiscr
 * \brief Base class for Centered schemes, derived classes implement
 * the dissipation term in a const "finalizeFlux" method.
 * \note See CRoeBase for the role of Base.
 */
template<class Derived, class Base>
class CCenteredBase : public Base {
protected:
  using Base::nDim;
  static constexpr size_t nVar = CCompressibleConservatives<nDim>::nVar;
  static constexpr size_t nPrimVar = Max(Base::nPrimVar, nDim+5);

  const su2double gamma;
  const su2double fixFactor;
  const bool dynamicGrid;
  const bool edgeForm;
  const bool exactFormJac;
  const bool projDiss;  /*!< \brief EDGE_DISS_PROJECTED, declared before ecFlux, which depends on it. */
  const bool ecFlux;
  const bool esDiss;
  const bool esBlend;
  const bool sweptFlux;
  const bool gridEntropy;  /*!< \brief GRID_ENTROPY_P1, ALE terms of the entropy methods (P1_ALE.md, section 10). */
  const bool weakWallDiss; /*!< \brief JST_WEAK_WALL_DISSIPATION, eps4 = 0 on edges that leave a weak no-slip wall. */
  const su2double stretchParam = 0.3;
  const CVariable* const tkeVars; /*!< \brief SST turbulence variables (k is part of the total energy), else nullptr. */
  const bool matchedK; /*!< \brief SST with the P1 options: hat states and matched k transport (P1_SST_COUPLED.md). */

  /*!
   * \brief Constructor, store some constants and forward args to base.
   */
  template<class... Ts>
  CCenteredBase(const CConfig& config, unsigned iMesh, Ts&... args) : Base(config, iMesh, args...),
    gamma(config.GetGamma()),
    fixFactor(config.GetCent_Jac_Fix_Factor()),
    dynamicGrid(config.GetDynamic_Grid()),
    edgeForm(iMesh == MESH_0 && (config.GetEdge_Form_P1() || config.GetSplit_Edge_Flux())),
    exactFormJac(edgeForm && config.GetEdge_Form_Exact_Jacobian()),
    projDiss(iMesh == MESH_0 && config.GetEdge_Diss_Projected()),
    ecFlux((edgeForm || projDiss) && config.GetEdge_Form_EC_Flux()),
    esDiss(edgeForm && config.GetEdge_Form_ES_Diss()),
    esBlend(edgeForm && config.GetEdge_Form_ES_Blend()),
    sweptFlux((edgeForm || projDiss) && config.GetGCL_Exact_P1()),
    gridEntropy((edgeForm || projDiss) && config.GetDynamic_Grid() && config.GetGrid_Entropy_P1()),
    weakWallDiss(iMesh == MESH_0 && config.GetJST_Weak_Wall_Diss()),
    tkeVars(config.GetKind_Turb_Model() == TURB_MODEL::SST ? findTurbVars(args...) : nullptr),
    matchedK(iMesh == MESH_0 && config.GetTke_Matched_P1() && (tkeVars != nullptr)) {
  }

  /*!
   * \brief JST_WEAK_WALL_DISSIPATION: factor of eps4 on the edge (iPoint, jPoint) from the wall flags of the nodes.
   */
  template<class T>
  FORCEINLINE static Double weakWallFactor(const T& iPoint, const T& jPoint, const CEulerVariable& solution) {
    return jstWallFactor(gatherVariables(iPoint, solution.GetJSTWallNode()),
                         gatherVariables(jPoint, solution.GetJSTWallNode()));
  }

  FORCEINLINE Double getTke(const Int& point) const {
    /*--- Matched form: the dissipation acts on the hat state, its Jacobian is taken with k = 0 and transformed. ---*/
    return (tkeVars && !matchedK) ? gatherVariables(point, tkeVars->GetSolution()) : Double(0.0);
  }

  /*!
   * \brief Special treatment needed to fetch integer data.
   */
  template<class T, size_t N>
  FORCEINLINE static Double numNeighbor(const simd::Array<T,N>& idx, const CGeometry& geometry) {
    Double n;
    for (size_t k=0; k<N; ++k) n[k] = geometry.nodes->GetnNeighbor(idx[k]);
    return n;
  }
  FORCEINLINE static Double numNeighbor(unsigned long idx, const CGeometry& geometry) {
    return geometry.nodes->GetnNeighbor(idx);
  }

public:
  /*!
   * \brief Implementation of the base centered flux.
   */
  void ComputeFlux(const Int iEdge,
                   const CConfig& config,
                   const CGeometry& geometry,
                   const CVariable& solution_,
                   const UpdateType updateType,
                   const Double updateMask,
                   CSysVector<su2double>& vector,
                   SparseMatrixType& matrix,
                   su2activevector* edgeMassFluxes) const final {

    /*--- Start preaccumulation, inputs are registered
     *    automatically in "gatherVariables". ---*/
    AD::StartPreacc();

    const bool implicit = (config.GetKind_TimeIntScheme() == EULER_IMPLICIT);
    const auto& solution = static_cast<const CEulerVariable&>(solution_);

    const auto iPoint = geometry.edges->GetNode(iEdge,0);
    const auto jPoint = geometry.edges->GetNode(iEdge,1);

    /*--- Geometric properties. ---*/

    const auto normal = gatherVariables<nDim>(iEdge, geometry.edges->GetNormal());
    const auto area = norm(normal);
    VectorDbl<nDim> unitNormal;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      unitNormal(iDim) = normal(iDim) / area;
    }

    /*--- Primitive variables. ---*/

    CPair<CCompressiblePrimitives<nDim,nPrimVar> > V;
    V.i.all = gatherVariables<nPrimVar>(iPoint, solution.GetPrimitive());
    V.j.all = gatherVariables<nPrimVar>(jPoint, solution.GetPrimitive());

    /*--- Matched k (P1_SST_COUPLED.md, section 7): every P1 part acts on the hat state, H - k, so U, the averages, the
     * E part, the ES dissipation, the blend and the JST jumps carry rhoE - rho k; k moves with the final mass flux below. ---*/
    Double kM_i = 0.0, kM_j = 0.0;
    if (matchedK) {
      kM_i = gatherVariables(iPoint, tkeVars->GetSolution());
      kM_j = gatherVariables(jPoint, tkeVars->GetSolution());
      V.i.enthalpy() -= kM_i;
      V.j.enthalpy() -= kM_j;
    }

    CCompressiblePrimitives<nDim,nPrimVar> avgV;
    for (size_t iVar = 0; iVar < nPrimVar; ++iVar) {
      avgV.all(iVar) = 0.5 * (V.i.all(iVar) + V.j.all(iVar));
    }

    /*--- Compute conservative variables. ---*/

    CPair<CCompressibleConservatives<nDim> > U;
    U.i = compressibleConservatives(V.i);
    U.j = compressibleConservatives(V.j);

    auto avgU = compressibleConservatives(avgV);

    VectorDbl<nVar> diffU;
    for (size_t iVar = 0; iVar < nVar-1; ++iVar) {
      diffU(iVar) = U.i.all(iVar) - U.j.all(iVar);
    }
    diffU(nVar-1) = V.i.density()*V.i.enthalpy() - V.j.density()*V.j.enthalpy();

    /*--- Inviscid fluxes and Jacobians. ---*/

    VectorDbl<nVar> flux;
    if (edgeForm) {
      /*--- 0.5(F_i+F_k).n_ik + (F_k-F_i).E_ik, the dissipation and Jacobians keep the normal of the dual. ---*/
      const auto formNormal = gatherVariables<nDim>(iEdge, geometry.edges->GetFormNormal());
      const auto formE = gatherVariables<nDim>(iEdge, geometry.edges->GetFormE());
      /*--- EDGE_FORM_EC_FLUX: the mean 0.5(F_i+F_k).n_ik is replaced by the Ismail-Roe flux along n_ik. With
       * sum_k n_ik = 0 at interior nodes this part is entropy conservative, F_ec(U,U) = F(U). ---*/
      VectorDbl<nVar> fluxMean;
      if (ecFlux) {
        fluxMean = ismailRoeProjFlux(gamma, V.i, V.j, formNormal);
      } else {
        const auto flux_i = inviscidProjFlux(V.i, U.i, formNormal);
        const auto flux_j = inviscidProjFlux(V.j, U.j, formNormal);
        for (size_t iVar = 0; iVar < nVar; ++iVar) fluxMean(iVar) = 0.5 * (flux_i(iVar) + flux_j(iVar));
      }
      const auto fluxE_i = inviscidProjFlux(V.i, U.i, formE);
      const auto fluxE_j = inviscidProjFlux(V.j, U.j, formE);
      for (size_t iVar = 0; iVar < nVar; ++iVar) {
        flux(iVar) = fluxMean(iVar) + fluxE_j(iVar) - fluxE_i(iVar);
      }
    } else if (ecFlux) {
      /*--- EDGE_DISS_PROJECTED with EDGE_FORM_EC_FLUX: Ismail-Roe along the median normal, entropy conservative at
       * interior nodes because sum_k n_ik = 0 there, the dissipation below is on nu. ---*/
      flux = ismailRoeProjFlux(gamma, V.i, V.j, normal);
    } else {
      flux = inviscidProjFlux(avgV, avgU, normal);
    }

    MatrixDbl<nVar> jac_i, jac_j;
    if (implicit) {
      /*--- With SST the total energy contains k, held fixed in the Jacobian. ---*/
      Double tke_i = 0.0, tke_j = 0.0;
      if (tkeVars && !matchedK) {
        tke_i = gatherVariables(iPoint, tkeVars->GetSolution());
        tke_j = gatherVariables(jPoint, tkeVars->GetSolution());
      }
      if (exactFormJac) {
        const auto formNormal = gatherVariables<nDim>(iEdge, geometry.edges->GetFormNormal());
        const auto formE = gatherVariables<nDim>(iEdge, geometry.edges->GetFormE());
        VectorDbl<nDim> normal_i, normal_j;
        for (size_t iDim = 0; iDim < nDim; ++iDim) {
          normal_i(iDim) = formNormal(iDim) - 2.0 * formE(iDim);
          normal_j(iDim) = formNormal(iDim) + 2.0 * formE(iDim);
        }
        jac_i = inviscidProjJac(gamma, V.i.velocity(), U.i.energy(), normal_i, 0.5, tke_i);
        jac_j = inviscidProjJac(gamma, V.j.velocity(), U.j.energy(), normal_j, 0.5, tke_j);
      } else {
        jac_i = inviscidProjJac(gamma, V.i.velocity(), U.i.energy(), normal, 0.5, tke_i);
        jac_j = inviscidProjJac(gamma, V.j.velocity(), U.j.energy(), normal, 0.5, tke_j);
      }
    }

    /*--- EDGE_FORM_ES_DISS: certified entropy stability of the E part for any jump (Lemma 3 of the method notes).
     * With dw the nodal jump of the entropy variables, u_k - u_i = Hbar dw and F_k.E - F_i.E = Bbar(E) dw exactly
     * (path averages along the straight w path). lam H - A H >= 0 iff lam >= lambda_max(A(E)) = (u.e + c)|E|, and
     * along a straight w path u.e and c are monotone (ratio of linear functions, c^2 = gamma/(2 beta) with beta
     * linear), so their maxima are at the nodes. lam = max(0, max(u_i.e, u_k.e) + max(c_i, c_k)) |E| on the nodal
     * conserved jump makes the E part entropy stable. No average state, no constant. ---*/
    if (esDiss) {
      const auto formE = gatherVariables<nDim>(iEdge, geometry.edges->GetFormE());
      const Double areaE = norm(formE);
      Double proj_i = 0.0, proj_j = 0.0;
      for (size_t iDim = 0; iDim < nDim; ++iDim) {
        proj_i += V.i.velocity(iDim) * formE(iDim);
        proj_j += V.j.velocity(iDim) * formE(iDim);
      }
      /*--- GRID_ENTROPY_P1: Lemma 3 with the relative speed, A - ({{v}}.E) I (P1_ALE.md, Lemma A12). ---*/
      if (gridEntropy) {
        const auto& gridVel = geometry.nodes->GetGridVel();
        const auto gridVel_i = gatherVariables<nDim>(iPoint, gridVel);
        const auto gridVel_j = gatherVariables<nDim>(jPoint, gridVel);
        const Double projGridE = 0.5 * (dot(gridVel_i, formE) + dot(gridVel_j, formE));
        proj_i -= projGridE;
        proj_j -= projGridE;
      }
      const Double lam = fmax(0.0, fmax(proj_i, proj_j) + sqrt(gamma * fmax(V.i.pressure() / V.i.density(), V.j.pressure() / V.j.density())) * areaE);
      for (size_t iVar = 0; iVar < nVar; ++iVar) {
        flux(iVar) -= lam * (U.j.all(iVar) - U.i.all(iVar));
        if (implicit) {
          jac_i(iVar, iVar) += lam;
          jac_j(iVar, iVar) -= lam;
        }
      }
    }

    /*--- EDGE_FORM_ES_BLEND: the same dissipation on the nodal jump, with sum_T theta_T lam^T_ik from the element
     * pass CEulerSolver::SetEdgeBlendLambda, theta_T = P_T^+ / D_T in [0,1]. Theta is frozen in the Jacobian. ---*/
    if (esBlend) {
      const Double lam = gatherVariables(iEdge, solution.GetEdgeBlendLambda());
      for (size_t iVar = 0; iVar < nVar; ++iVar) {
        flux(iVar) -= lam * (U.j.all(iVar) - U.i.all(iVar));
        if (implicit) {
          jac_i(iVar, iVar) += lam;
          jac_j(iVar, iVar) -= lam;
        }
      }
    }

    /*--- Grid motion. ---*/

    Double projGridVel = 0.0;
    if (dynamicGrid) {
      const auto& gridVel = geometry.nodes->GetGridVel();
      const auto gridVel_i = gatherVariables<nDim>(iPoint,gridVel);
      const auto gridVel_j = gatherVariables<nDim>(jPoint,gridVel);
      projGridVel = 0.5*(dot(gridVel_i, normal)+dot(gridVel_j, normal));

      if (gridEntropy) {
        /*--- GRID_ENTROPY_P1 (P1_ALE.md, Definition A9): -ubar^EC(U_i,U_k) g_ik - (U_k - U_i) {{v}}.E_ik, with the pair
         * grid flux of the GCL g_ik = 0.5(v_i+v_k).n_ik + (v_k-v_i).E_ik + omega_ik (CGeometry::SetGridFluxP1), and
         * the E part of the relative flux F - U {{v}}^T. The median dual has E = 0 and the median normal. Production
         * -g_ik [[rho]] telescopes with the GCL source to boundary terms; the static E part is unchanged above. ---*/
        Double gridFlux = projGridVel, projGridE = 0.0;
        if (edgeForm) {
          const auto formNormal = gatherVariables<nDim>(iEdge, geometry.edges->GetFormNormal());
          const auto formE = gatherVariables<nDim>(iEdge, geometry.edges->GetFormE());
          gridFlux = 0.5 * (dot(gridVel_i, formNormal) + dot(gridVel_j, formNormal)) +
                     dot(gridVel_j, formE) - dot(gridVel_i, formE);
          projGridE = 0.5 * (dot(gridVel_i, formE) + dot(gridVel_j, formE));
        }
        if (sweptFlux) gridFlux += gatherVariables(iEdge, geometry.edges->GetFormSwept());

        const auto ubar = ecTemporalAverage(gamma, V.i, V.j);

        /*--- Frozen, like the flux Jacobians: 0.5 g on the dual normal, or 0.5 g -+ {{v}}.E with the exact form. ---*/
        Double jacGrid_i = 0.5 * projGridVel, jacGrid_j = 0.5 * projGridVel;
        if (exactFormJac) {
          jacGrid_i = 0.5 * gridFlux - projGridE;
          jacGrid_j = 0.5 * gridFlux + projGridE;
        }

        for (size_t iVar = 0; iVar < nVar; ++iVar) {
          flux(iVar) -= ubar(iVar) * gridFlux + projGridE * (U.j.all(iVar) - U.i.all(iVar));
          if (implicit) {
            jac_i(iVar,iVar) -= jacGrid_i;
            jac_j(iVar,iVar) -= jacGrid_j;
          }
        }
      } else if (edgeForm) {
        /*--- The nodal flux F - U v with the coefficients of F, as CGeometry::SetGridFluxP1 does for the GCL. ---*/
        const auto formNormal = gatherVariables<nDim>(iEdge, geometry.edges->GetFormNormal());
        const auto formE = gatherVariables<nDim>(iEdge, geometry.edges->GetFormE());
        const Double grid_i = 0.5 * dot(gridVel_i, formNormal) - dot(gridVel_i, formE);
        const Double grid_j = 0.5 * dot(gridVel_j, formNormal) + dot(gridVel_j, formE);

        /*--- Like the flux Jacobians: on the dual normal, or exact with EDGE_FORM_EXACT_JACOBIAN. ---*/
        Double jacGrid_i = 0.5 * projGridVel, jacGrid_j = 0.5 * projGridVel;
        if (exactFormJac) {
          jacGrid_i = grid_i;
          jacGrid_j = grid_j;
        }

        for (size_t iVar = 0; iVar < nVar; ++iVar) {
          flux(iVar) -= grid_i * U.i.all(iVar) + grid_j * U.j.all(iVar);
          if (implicit) {
            jac_i(iVar,iVar) -= jacGrid_i;
            jac_j(iVar,iVar) -= jacGrid_j;
          }
        }

        /*--- GCL_EXACT_P1: -0.5(U_i+U_k) omega_ik, the pair term of the grid flux. ---*/
        if (sweptFlux) {
          const Double omega = gatherVariables(iEdge, geometry.edges->GetFormSwept());
          for (size_t iVar = 0; iVar < nVar; ++iVar) {
            flux(iVar) -= 0.5 * omega * (U.i.all(iVar) + U.j.all(iVar));
          }
        }
      } else {
        for (size_t iVar = 0; iVar < nVar; ++iVar) {
          flux(iVar) -= projGridVel * avgU.all(iVar);
          if (implicit) {
            jac_i(iVar,iVar) -= 0.5 * projGridVel;
            jac_j(iVar,iVar) -= 0.5 * projGridVel;
          }
        }
      }
    }

    /*--- EDGE_DISS_PROJECTED: the dissipation uses nu = (n.e/|e|^2) e, the edge projection of the median normal,
     * so the spectral radius is |u.nu| + c|nu|. The central part and its Jacobian above keep n, the nodal spectral
     * radii of the stretching correction and of the time step stay median. Off, the references are the median
     * normal and nothing else is read. ---*/

    VectorDbl<nDim> projNormal, projUnitNormal;
    Double projArea = 0.0;
    if (projDiss) {
      const auto vector_ij = distanceVector<nDim>(iPoint, jPoint, geometry.nodes->GetCoord());
      projectedNormal(normal, vector_ij, projNormal, projArea, projUnitNormal);
    }
    const VectorDbl<nDim>& rNormal = projDiss ? projNormal : normal;
    const VectorDbl<nDim>& rUnitNormal = projDiss ? projUnitNormal : unitNormal;
    const Double& rArea = projDiss ? projArea : area;

    /*--- GRID_ENTROPY_P1 with EDGE_DISS_PROJECTED: the relative speed along the dissipation normal, (u - {{v}}).nu. ---*/
    Double projGridDiss = projGridVel;
    if (gridEntropy && projDiss) {
      const auto& gridVel = geometry.nodes->GetGridVel();
      const auto gridVel_i = gatherVariables<nDim>(iPoint, gridVel);
      const auto gridVel_j = gatherVariables<nDim>(jPoint, gridVel);
      projGridDiss = 0.5 * (dot(gridVel_i, projNormal) + dot(gridVel_j, projNormal));
    }
    const Double projVel = dot(avgV.velocity(), rNormal) - projGridDiss;

    /*--- Finalize in derived class (static polymorphism). ---*/

    const auto derived = static_cast<const Derived*>(this);

    derived->finalizeFlux(flux, jac_i, jac_j, implicit, rArea, projVel, avgV, V,
                          diffU, iPoint, jPoint, geometry, solution, rUnitNormal, iEdge, updateMask);

    /*--- Matched k: mdot k_up with the final mass flux (stored below for the SST k convection), and the Jacobian
     * L(k_up) Jhat T(k) of the hat Jacobian (D1), before the viscous terms whose Jacobian is already w.r.t. U. ---*/
    if (matchedK) {
      const Double kUp = P1SSTMatched::KUpwind(flux(0), kM_i, kM_j);
      flux(nVar-1) += kUp * flux(0);
      if (implicit) {
        P1SSTMatched::TransformJacobian(nVar, jac_i, kUp, kM_i);
        P1SSTMatched::TransformJacobian(nVar, jac_j, kUp, kM_j);
      }
    }

    /*--- Add the contributions from the base class (static decorator). ---*/

    Base::viscousTerms(iEdge, iPoint, jPoint, avgV, V, solution_, geometry,
                       config, area, unitNormal, implicit, flux, jac_i, jac_j);

    /*--- Stop preaccumulation. ---*/

    stopPreacc(flux);

    /*--- Update the vector and system matrix. ---*/

    updateLinearSystem(iEdge, iPoint, jPoint, implicit, updateType,
                       updateMask, flux, jac_i, jac_j, vector, matrix);

    /*--- Store the mass flux (density-equation flux) for bounded-scalar transport. ---*/

    updateEdgeMassFlux(iEdge, flux(0), edgeMassFluxes);
  }
};

/*!
 * \class CJSTScheme
 * \ingroup ConvDiscr
 * \brief Classical JST scheme with scalar dissipation.
 */
template<class Decorator>
class CJSTScheme : public CCenteredBase<CJSTScheme<Decorator>,Decorator> {
private:
  using Base = CCenteredBase<CJSTScheme<Decorator>,Decorator>;
  using Base::nDim;
  using Base::nVar;
  using Base::gamma;
  using Base::fixFactor;
  using Base::stretchParam;
  using Base::weakWallDiss;
  using Base::matchedK;
  const su2double kappa2;
  const su2double kappa4;
  const bool entropyBudget; /*!< \brief JST_ENTROPY_BUDGET: store (eps2 lam, eps4 lam) of the edge (fine grid). */

public:
  /*!
   * \brief Constructor, forward everything to base.
   */
  template<class... Ts>
  CJSTScheme(const CConfig& config, Ts&... args) : Base(config, args...),
    kappa2(config.GetKappa_2nd_Flow()),
    kappa4(config.GetKappa_4th_Flow()),
    entropyBudget(config.GetJST_Entropy_Budget() && (Base::edgeForm || Base::projDiss)) {
  }

  /*!
   * \brief Updates flux and Jacobians with JST dissipation.
   * \note "Ts" is here just in case other schemes in the family need extra args.
   */
  template<class PrimVarType, class... Ts>
  FORCEINLINE void finalizeFlux(VectorDbl<nVar>& flux,
                                MatrixDbl<nVar>& jac_i,
                                MatrixDbl<nVar>& jac_j,
                                const bool implicit,
                                const Double& area,
                                const Double& projVel,
                                const PrimVarType& avgV,
                                const CPair<PrimVarType>& V,
                                const VectorDbl<nVar>& diffU,
                                const Int& iPoint,
                                const Int& jPoint,
                                const CGeometry& geometry,
                                const CEulerVariable& solution,
                                const VectorDbl<nDim>&,
                                const Int& iEdge,
                                const Double& updateMask,
                                Ts&...) const {

    Double lambda = abs(projVel) + avgV.speedSound()*area;
    lambda = correctedSpectralRadius(iPoint, jPoint, lambda, stretchParam, solution);

    /*--- Compute dissipation coefficients. ---*/

    const auto ni = Base::numNeighbor(iPoint, geometry);
    const auto nj = Base::numNeighbor(jPoint, geometry);
    const Double sc2 = 3 * (ni+nj) / (ni*nj);
    const Double sc4 = 0.25*pow(sc2, 2);

    const auto si = gatherVariables(iPoint, solution.GetSensor());
    const auto sj = gatherVariables(jPoint, solution.GetSensor());
    Double eps2 = kappa2 * 0.5*(si+sj) * sc2;
    Double eps4 = fmax(0.0, kappa4-eps2) * sc4;

    /*--- JST_WEAK_WALL_DISSIPATION (P1_WALL_DISSIPATION.md): no JST dissipation on edges with one end on a weak no-slip
     * wall, neither the fourth-order term nor the second-order one (which drives the wall-node slip), in the flux and in
     * the Jacobian below; eps4 is formed from the unchanged eps2 first. Elsewhere the factor is exactly 1. ---*/
    if (weakWallDiss) {
      const Double keep = Base::weakWallFactor(iPoint, jPoint, solution);
      eps4 *= keep;
      eps2 *= keep;
    }

    /*--- Update flux and Jacobians with dissipation terms. ---*/

    auto lapl_i = gatherVariables<nVar>(iPoint, solution.GetUndivided_Laplacian());
    auto lapl_j = gatherVariables<nVar>(jPoint, solution.GetUndivided_Laplacian());

    /*--- Matched k: the fourth difference of rho Hhat, the energy entry minus the Laplacian of rho k (extra column). ---*/
    if (matchedK) {
      lapl_i(nVar-1) -= gatherVariables(iPoint, solution.GetUndivided_Laplacian(), nVar);
      lapl_j(nVar-1) -= gatherVariables(jPoint, solution.GetUndivided_Laplacian(), nVar);
    }

    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      flux(iVar) += (eps2*diffU(iVar) - eps4*(lapl_i(iVar)-lapl_j(iVar))) * lambda;
    }

    /*--- JST_ENTROPY_BUDGET: the coefficients of this edge for the node budget (P1_JST_ENTROPY.md, J4), unmasked lanes
     * (masked lanes repeat an edge). The flux above is unchanged here. ---*/
    if (entropyBudget) {
      const Double c2 = eps2 * lambda, c4 = eps4 * lambda;
      auto& coeff = solution.JSTEdgeCoeff;
      for (size_t k = 0; k < Double::Size; ++k) {
        if (updateMask[k] == 0.0) continue;
        coeff(iEdge[k], 0) = c2[k];
        coeff(iEdge[k], 1) = c4[k];
      }
    }

    if (implicit) {
      const Double dissip_i = fixFactor * (eps2 + eps4 * (ni + 1)) * lambda;
      const Double dissip_j = -fixFactor * (eps2 + eps4 * (nj + 1)) * lambda;
      scalarDissipationJacobian(V.i, gamma, dissip_i, jac_i, Base::getTke(iPoint));
      scalarDissipationJacobian(V.j, gamma, dissip_j, jac_j, Base::getTke(jPoint));
    }
  }
};

/*!
 * \class CJSTmatScheme
 * \ingroup ConvDiscr
 * \brief JST scheme with matrix dissipation.
 */
template<class Decorator>
class CJSTmatScheme : public CCenteredBase<CJSTmatScheme<Decorator>,Decorator> {
private:
  using Base = CCenteredBase<CJSTmatScheme<Decorator>,Decorator>;
  using Base::nDim;
  using Base::nVar;
  using Base::gamma;
  using Base::fixFactor;
  using Base::weakWallDiss;
  using Base::tkeVars;
  const su2double kappa2;
  const su2double kappa4;
  const su2double entropyFix;

public:
  /*!
   * \brief Constructor, forward everything to base.
   */
  template<class... Ts>
  CJSTmatScheme(const CConfig& config, Ts&... args) : Base(config, args...),
    kappa2(config.GetKappa_2nd_Flow()),
    kappa4(config.GetKappa_4th_Flow()),
    entropyFix(config.GetEntropyFix_Coeff()) {
  }

  /*!
   * \brief Updates flux and Jacobians.
   * \note "Ts" is here just in case other schemes in the family need extra args.
   */
  template<class PrimVarType, class... Ts>
  FORCEINLINE void finalizeFlux(VectorDbl<nVar>& flux,
                                MatrixDbl<nVar>& jac_i,
                                MatrixDbl<nVar>& jac_j,
                                const bool implicit,
                                const Double& area,
                                const Double& projVel,
                                const PrimVarType& avgV,
                                const CPair<PrimVarType>& V,
                                const VectorDbl<nVar>& diffU,
                                const Int& iPoint,
                                const Int& jPoint,
                                const CGeometry& geometry,
                                const CEulerVariable& solution,
                                const VectorDbl<nDim>& unitNormal,
                                Ts&...) const {

    /*--- Compute scalar dissipation. ---*/

    const auto ni = Base::numNeighbor(iPoint, geometry);
    const auto nj = Base::numNeighbor(jPoint, geometry);
    const Double sc2 = 3 * (ni+nj) / (ni*nj);
    const Double sc4 = 0.25*pow(sc2, 2);

    const auto si = gatherVariables(iPoint, solution.GetSensor());
    const auto sj = gatherVariables(jPoint, solution.GetSensor());
    Double eps2 = kappa2 * 0.5*(si+sj) * sc2;
    Double eps4 = fmax(0.0, kappa4-eps2) * sc4;

    /*--- JST_WEAK_WALL_DISSIPATION, as for CJSTScheme: the scalar dissipation, its k part and its Jacobian factor lose
     * eps2 and eps4. ---*/
    if (weakWallDiss) {
      const Double keep = Base::weakWallFactor(iPoint, jPoint, solution);
      eps4 *= keep;
      eps2 *= keep;
    }

    const auto lapl_i = gatherVariables<nVar>(iPoint, solution.GetUndivided_Laplacian());
    const auto lapl_j = gatherVariables<nVar>(jPoint, solution.GetUndivided_Laplacian());

    VectorDbl<nVar> scalarDissip;
    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      scalarDissip(iVar) = eps2 * diffU(iVar) - eps4 * (lapl_i(iVar) - lapl_j(iVar));
    }

    Double tke_i = 0.0, tke_j = 0.0;
    if (tkeVars) {
      tke_i = gatherVariables(iPoint, tkeVars->GetSolution());
      tke_j = gatherVariables(jPoint, tkeVars->GetSolution());
    }
    const Double avgTke = 0.5 * (tke_i + tke_j);

    MatrixDbl<nVar> scalarJac;
    if (implicit) {
      scalarJac = Double(0.0);
      Double factor = fixFactor * (eps2 + 0.5 * eps4 * (ni + nj + 2));
      scalarDissipationJacobian(avgV, gamma, factor, scalarJac, avgTke);
    }

    /*--- Compute matrix dissipation terms. ---*/

    const auto unitProjVel = dot(avgV.velocity(), unitNormal);

    // Remove changes in k before acoustic projection, including the fourth-order term.
    Double rhoDeltaTke = eps2 * avgV.density() * (tke_i - tke_j);
    if (tkeVars) {
      const auto lapK_i = gatherVariables(iPoint, solution.GetUndivided_Laplacian(), nVar);
      const auto lapK_j = gatherVariables(jPoint, solution.GetUndivided_Laplacian(), nVar);
      rhoDeltaTke -= eps4 * (lapK_i - lapK_j - avgTke * (lapl_i(0) - lapl_j(0)));
    }
    scalarDissip(nVar - 1) -= rhoDeltaTke;
    auto pMat = pMatrix(gamma, avgV.density(), avgV.velocity(), unitProjVel, avgV.speedSound(), unitNormal, avgTke);

    auto pMatInv = pMatrixInv(gamma, avgV.density(), avgV.velocity(),
                              unitProjVel, avgV.speedSound(), unitNormal, avgTke);

    /*--- Compute limited absolute eigenvalues (times area). ---*/

    VectorDbl<nVar> lambda;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      lambda(iDim) = projVel;
    }
    lambda(nDim) = projVel + avgV.speedSound()*area;
    lambda(nDim+1) = projVel - avgV.speedSound()*area;

    const Double maxLambda = fmax(lambda(nDim), -lambda(nDim+1));

    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      lambda(iVar) = fmax(abs(lambda(iVar)), entropyFix*maxLambda);
    }

    /*--- Update flux and Jacobians with scaled dissipation terms. ---*/

    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      for (size_t jVar = 0; jVar < nVar; ++jVar) {
        /*--- Matrix scaling, P x |S * Lambda| x P^-1. ---*/

        Double scale = 0.0;
        for (size_t kVar = 0; kVar < nVar; ++kVar) {
          scale += pMat(iVar,kVar) * lambda(kVar) * pMatInv(kVar,jVar);
        }

        /*--- Update flux and Jacobians. ---*/

        flux(iVar) += scale * scalarDissip(jVar);

        if (implicit) {
          for (size_t kVar = 0; kVar < nVar; ++kVar) {
            jac_i(iVar,kVar) += scale * scalarJac(jVar,kVar);
            jac_j(iVar,kVar) -= scale * scalarJac(jVar,kVar);
          }
        }
      }
    }
    flux(nVar-1) += lambda(0) * rhoDeltaTke;
  }
};

/*!
 * \class CJSTkeScheme
 * \ingroup ConvDiscr
 * \brief JST scheme without 4th order dissipation.
 */
template<class Decorator>
class CJSTkeScheme : public CCenteredBase<CJSTkeScheme<Decorator>,Decorator> {
private:
  using Base = CCenteredBase<CJSTkeScheme<Decorator>,Decorator>;
  using Base::nDim;
  using Base::nVar;
  using Base::gamma;
  using Base::fixFactor;
  using Base::stretchParam;
  const su2double kappa2;

public:
  /*!
   * \brief Constructor, forward everything to base.
   */
  template<class... Ts>
  CJSTkeScheme(const CConfig& config, Ts&... args) : Base(config, args...),
    kappa2(config.GetKappa_2nd_Flow()) {
  }

  /*!
   * \brief Updates flux and Jacobians with 2nd order dissipation.
   * \note "Ts" is here just in case other schemes in the family need extra args.
   */
  template<class PrimVarType, class... Ts>
  FORCEINLINE void finalizeFlux(VectorDbl<nVar>& flux,
                                MatrixDbl<nVar>& jac_i,
                                MatrixDbl<nVar>& jac_j,
                                const bool implicit,
                                const Double& area,
                                const Double& projVel,
                                const PrimVarType& avgV,
                                const CPair<PrimVarType>& V,
                                const VectorDbl<nVar>& diffU,
                                const Int& iPoint,
                                const Int& jPoint,
                                const CGeometry& geometry,
                                const CEulerVariable& solution,
                                Ts&...) const {

    Double lambda = abs(projVel) + avgV.speedSound()*area;
    lambda = correctedSpectralRadius(iPoint, jPoint, lambda, stretchParam, solution);

    /*--- Compute dissipation coefficient. ---*/

    const auto ni = Base::numNeighbor(iPoint, geometry);
    const auto nj = Base::numNeighbor(jPoint, geometry);
    const Double sc2 = 3 * (ni+nj) / (ni*nj);

    const auto si = gatherVariables(iPoint, solution.GetSensor());
    const auto sj = gatherVariables(jPoint, solution.GetSensor());
    const Double dissip = kappa2 * 0.5*(si+sj) * sc2 * lambda;

    /*--- Update flux and Jacobians with dissipation term. ---*/

    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      flux(iVar) += dissip * diffU(iVar);
    }

    if (implicit) {
      scalarDissipationJacobian(V.i, gamma, fixFactor * dissip, jac_i, Base::getTke(iPoint));
      scalarDissipationJacobian(V.j, gamma, -fixFactor * dissip, jac_j, Base::getTke(jPoint));
    }
  }
};

/*!
 * \class CLaxScheme
 * \ingroup ConvDiscr
 * \brief Lax–Friedrichs 1st order scheme.
 */
template<class Decorator>
class CLaxScheme : public CCenteredBase<CLaxScheme<Decorator>,Decorator> {
private:
  using Base = CCenteredBase<CLaxScheme<Decorator>,Decorator>;
  using Base::nDim;
  using Base::nVar;
  using Base::gamma;
  using Base::fixFactor;
  using Base::stretchParam;
  const su2double kappa0;

public:
  /*!
   * \brief Constructor, forward everything to base.
   */
  template<class... Ts>
  CLaxScheme(const CConfig& config, Ts&... args) : Base(config, args...),
    kappa0(config.GetKappa_1st_Flow()) {
  }

  /*!
   * \brief Updates flux and Jacobians with 1st order scalar dissipation.
   * \note "Ts" is here just in case other schemes in the family need extra args.
   */
  template<class PrimVarType, class... Ts>
  FORCEINLINE void finalizeFlux(VectorDbl<nVar>& flux,
                                MatrixDbl<nVar>& jac_i,
                                MatrixDbl<nVar>& jac_j,
                                const bool implicit,
                                const Double& area,
                                const Double& projVel,
                                const PrimVarType& avgV,
                                const CPair<PrimVarType>& V,
                                const VectorDbl<nVar>& diffU,
                                const Int& iPoint,
                                const Int& jPoint,
                                const CGeometry& geometry,
                                const CEulerVariable& solution,
                                Ts&...) const {

    Double lambda = abs(projVel) + avgV.speedSound()*area;
    lambda = correctedSpectralRadius(iPoint, jPoint, lambda, stretchParam, solution);

    /*--- Compute dissipation coefficient. ---*/

    const auto ni = Base::numNeighbor(iPoint, geometry);
    const auto nj = Base::numNeighbor(jPoint, geometry);
    const Double dissip = kappa0 * nDim * (ni+nj) / (ni*nj) * lambda;

    /*--- Update flux and Jacobians with dissipation term. ---*/

    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      flux(iVar) += dissip * diffU(iVar);
    }

    if (implicit) {
      scalarDissipationJacobian(V.i, gamma, fixFactor * dissip, jac_i, Base::getTke(iPoint));
      scalarDissipationJacobian(V.j, gamma, -fixFactor * dissip, jac_j, Base::getTke(jPoint));
    }
  }
};
