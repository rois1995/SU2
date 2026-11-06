/*!
 * \file common.hpp
 * \brief Common convection-related methods.
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
#include "../../../variables/CNSVariable.hpp"

/*!
 * \brief Retrieve primitive variables for points i/j, reconstructing them if needed.
 * \note Density and enthalpy are recomputed from ideal gas EOS.
 * \param[in] iEdge, iPoint, jPoint - Edge and its nodes.
 * \param[in] gamma - Heat capacity ratio.
 * \param[in] gasConst - Specific gas constant.
 * \param[in] muscl - If true, reconstruct, else simply fetch.
 * \param[in] kappa - Blending coefficient for MUSCL reconstruction.
 * \param[in] umusclRamp - MUSCL 1st-2nd order ramp times Newton-Krylov relaxation.
 * \param[in] limiterType - Type of flux limiter.
 * \param[in] V1st - Pair of compressible flow primitives for nodes i,j.
 * \param[in] vector_ij - Distance vector from i to j.
 * \param[in] solution - Entire solution container (a derived CVariable).
 * \param[out] nonPhysical - Signals that the edge is treated as non-physical.
 * \return Pair of primitive variables.
 */
template<class ReconVarType, class PrimVarType, size_t nDim, class VariableType>
FORCEINLINE CPair<ReconVarType> reconstructPrimitives(const Int& iEdge,
                                                      const Int& iPoint, const Int& jPoint,
                                                      const su2double& gamma,
                                                      const su2double& gasConst,
                                                      const bool muscl,
                                                      const su2double& kappa,
                                                      const su2double& umusclRamp,
                                                      const LIMITER limiterType,
                                                      const CPair<PrimVarType>& V1st,
                                                      const VectorDbl<nDim>& vector_ij,
                                                      const VariableType& solution,
                                                      Double& nonPhysical) {
  static_assert(ReconVarType::nVar <= PrimVarType::nVar);

  const auto& gradients = solution.GetGradient_Reconstruction();
  const auto& limiters = solution.GetLimiter_Primitive();

  CPair<ReconVarType> V;

  for (size_t iVar = 0; iVar < ReconVarType::nVar; ++iVar) {
    V.i.all(iVar) = V1st.i.all(iVar);
    V.j.all(iVar) = V1st.j.all(iVar);
  }

  if (muscl) {
    /*--- Reconstruct density and enthalpy without using their gradients. ---*/
    constexpr auto nVarGrad = ReconVarType::nVar - 2;
    reconstruct<nVarGrad>(iPoint, jPoint, vector_ij, gradients, limiters, limiterType, 0, V, kappa, umusclRamp);
    /*--- Recompute density using the reconstructed pressure and temperature. ---*/
    V.i.density() = V.i.pressure() / (gasConst * V.i.temperature());
    V.j.density() = V.j.pressure() / (gasConst * V.j.temperature());

    /*--- Reconstruct enthalpy using dH/dT = Cp and dH/dv = v. Recomputing enthalpy would cause
     * stability issues because we use rho E = rho H - P, which loses its relation to temperature
     * if both rho and H are recomputed. This only seems to be an issue for wall-function meshes.
     * NOTE: This "one-sided" reconstruction does not lose much of the U-MUSCL benefit, because
     * the static enthalpy is linear, and for the KE term we do the equivalent of using the
     * average dH/dv, which is exact for quadratic functions. ---*/
    const su2double cp = gasConst * gamma / (gamma - 1);
    V.i.enthalpy() += cp * (V.i.temperature() - V1st.i.temperature());
    V.j.enthalpy() += cp * (V.j.temperature() - V1st.j.temperature());
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      V.i.enthalpy() += 0.5 * (pow(V.i.velocity(iDim), 2) - pow(V1st.i.velocity(iDim), 2));
      V.j.enthalpy() += 0.5 * (pow(V.j.velocity(iDim), 2) - pow(V1st.j.velocity(iDim), 2));
    }

    /*--- Detect a non-physical reconstruction based on negative pressure or density. ---*/
    const Double neg_p_or_rho = fmax(fmin(V.i.pressure(), V.j.pressure()) < 0.0,
                                     fmin(V.i.density(), V.j.density()) < 0.0);
    /*--- Test the sign of the Roe-averaged speed of sound. ---*/
    const Double R = sqrt(abs(V.j.density() / V.i.density()));
    /*--- Delay dividing by R+1 until comparing enthalpy and velocity magnitude. ---*/
    const Double enthalpy = R*V.j.enthalpy() + V.i.enthalpy();
    Double v_squared = 0.0;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      v_squared += pow(R*V.j.velocity(iDim) + V.i.velocity(iDim), 2);
    }
    /*--- Multiply enthalpy by R+1 since v^2 was not divided by (R+1)^2.
     * Note: a = sqrt((gamma-1) * (H - 0.5 * v^2)) ---*/
    const Double neg_sound_speed = enthalpy * (R+1) < 0.5 * v_squared;

    /*--- Revert to first order if the state is non-physical. ---*/
    nonPhysical = fmax(neg_p_or_rho, neg_sound_speed);
    /*--- Handle SIMD dimensions 1 by 1. ---*/
    for (size_t k = 0; k < Double::Size; ++k) {
      nonPhysical[k] = solution.UpdateNonPhysicalEdgeCounter(iEdge[k], nonPhysical[k]);
      nonPhysical[k] = fmax(nonPhysical[k],
          fmax(solution.OutlierMitigation(iPoint[k]),
               solution.OutlierMitigation(jPoint[k])) / VariableType::MAX_OUTLIER_MITIGATION);
    }
    for (size_t iVar = 0; iVar < ReconVarType::nVar; ++iVar) {
      V.i.all(iVar) = nonPhysical * V1st.i.all(iVar) + (1-nonPhysical) * V.i.all(iVar);
      V.j.all(iVar) = nonPhysical * V1st.j.all(iVar) + (1-nonPhysical) * V.j.all(iVar);
    }
  } else {
    nonPhysical = 0;
  }
  return V;
}

template<class ReconVarType, size_t nDim, class VariableType>
FORCEINLINE void CorrectFlux(Int iPoint, Int jPoint,
                 const VariableType& solution,
                 const CPair<ReconVarType>& V,
                 VectorDbl<nDim+2>& flux,
                 const VectorDbl<nDim> dirX,
                 const VectorDbl<nDim> dirY,
                 const VectorDbl<nDim> dirZ,
                 const Double gamma,
                 bool limited = false) {

  /*--- Reconstructed primitives. ---*/

  ReconVarType avgV;
  for (size_t iVar = 0; iVar < ReconVarType::nVar; ++iVar) {
    avgV.all(iVar) = 0.5 * (V.i.all(iVar) + V.j.all(iVar));
  }

  constexpr unsigned short index[] = {static_cast<unsigned short>(nDim+2), 1, 2, 3, static_cast<unsigned short>(nDim+1)};

  MatrixDbl<nDim+2> jac_x, jac_y, jac_z;

  jac_x = inviscidProjJacPrim(gamma, avgV.density(),avgV.velocity(),
                              avgV.enthalpy(),dirX,0.5);

  jac_y = inviscidProjJacPrim(gamma, avgV.density(),avgV.velocity(),
                              avgV.enthalpy(),dirY,0.5);

  jac_z = inviscidProjJacPrim(gamma, avgV.density(),avgV.velocity(),
                              avgV.enthalpy(),dirZ,0.5);

  auto grad_i = gatherVariables<ReconVarType::nVar,nDim>(iPoint, solution.GetGradient_Reconstruction());
  auto grad_j = gatherVariables<ReconVarType::nVar,nDim>(jPoint, solution.GetGradient_Reconstruction());

  /*--- FLUX_CORRECTION_LIMITED: the gradients of the correction use the limiter of the reconstruction. ---*/

  if (limited) {
    const auto lim_i = gatherVariables<ReconVarType::nVar>(iPoint, solution.GetLimiter_Primitive());
    const auto lim_j = gatherVariables<ReconVarType::nVar>(jPoint, solution.GetLimiter_Primitive());
    for (size_t jVar = 0; jVar < nDim+2; ++jVar) {
      const auto var = index[jVar];
      for (size_t iDim = 0; iDim < nDim; ++iDim) {
        grad_i[var][iDim] *= lim_i(var);
        grad_j[var][iDim] *= lim_j(var);
      }
    }
  }

  for (size_t iVar = 0; iVar < nDim+2; ++iVar) {
    for (size_t jVar = 0; jVar < nDim+2; ++jVar) {
      flux[iVar] += jac_x[iVar][jVar] * (grad_i[index[jVar]][0] + grad_j[index[jVar]][0]);
      flux[iVar] += jac_y[iVar][jVar] * (grad_i[index[jVar]][1] + grad_j[index[jVar]][1]);
      if (nDim == 3) flux[iVar] += jac_z[iVar][jVar] * (grad_i[index[jVar]][2] + grad_j[index[jVar]][2]);
    }
  }

};

/*!
 * \brief Compute and return the P tensor (compressible flow, ideal gas).
 */
template<size_t nDim, class RandomAccessIterator>
FORCEINLINE MatrixDbl<nDim+2> pMatrix(const Double& gamma, const Double& density, const RandomAccessIterator& velocity,
                                      const Double& projVel, const Double& speedSound, const VectorDbl<nDim>& normal) {
  MatrixDbl<nDim+2> pMat;
  const Double vel2 = 0.5*squaredNorm<nDim>(velocity);

  if (nDim == 2) {
    pMat(0,0) = 1.0;
    pMat(0,1) = 0.0;

    pMat(1,0) = velocity[0];
    pMat(1,1) = density*normal(1);

    pMat(2,0) = velocity[1];
    pMat(2,1) = -density*normal(0);

    pMat(3,0) = vel2;
    pMat(3,1) = density*(velocity[0]*normal(1) - velocity[1]*normal(0));
  }
  else {
    pMat(0,0) = normal(0);
    pMat(0,1) = normal(1);
    pMat(0,2) = normal(2);

    pMat(1,0) = velocity[0]*normal(0);
    pMat(1,1) = velocity[0]*normal(1) - density*normal(2);
    pMat(1,2) = velocity[0]*normal(2) + density*normal(1);

    pMat(2,0) = velocity[1]*normal(0) + density*normal(2);
    pMat(2,1) = velocity[1]*normal(1);
    pMat(2,2) = velocity[1]*normal(2) - density*normal(0);

    pMat(3,0) = velocity[2]*normal(0) - density*normal(1);
    pMat(3,1) = velocity[2]*normal(1) + density*normal(0);
    pMat(3,2) = velocity[2]*normal(2);

    pMat(4,0) = vel2*normal(0) + density*(velocity[1]*normal(2) - velocity[2]*normal(1));
    pMat(4,1) = vel2*normal(1) - density*(velocity[0]*normal(2) - velocity[2]*normal(0));
    pMat(4,2) = vel2*normal(2) + density*(velocity[0]*normal(1) - velocity[1]*normal(0));
  }

  /*--- Last two columns. ---*/

  const Double rhoOn2 = 0.5*density;
  const Double rhoOnTwoC = rhoOn2 / speedSound;
  pMat(0,nDim) = rhoOnTwoC;
  pMat(0,nDim+1) = rhoOnTwoC;

  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    pMat(iDim+1,nDim) = rhoOnTwoC * velocity[iDim] + rhoOn2 * normal(iDim);
    pMat(iDim+1,nDim+1) = rhoOnTwoC * velocity[iDim] - rhoOn2 * normal(iDim);
  }

  pMat(nDim+1,nDim) = rhoOnTwoC * vel2 + rhoOn2 * (projVel + speedSound/(gamma-1));
  pMat(nDim+1,nDim+1) = rhoOnTwoC * vel2 - rhoOn2 * (projVel - speedSound/(gamma-1));

  return pMat;
}

/*!
 * \brief Compute and return the inverse P tensor (compressible flow, ideal gas).
 */
template<size_t nDim, class RandomAccessIterator>
FORCEINLINE MatrixDbl<nDim+2> pMatrixInv(const Double& gamma, const Double& density,
                                         const RandomAccessIterator& velocity, const Double& projVel,
                                         const Double& speedSound, const VectorDbl<nDim>& normal) {
  MatrixDbl<nDim+2> pMatInv;

  const Double c2 = pow(speedSound,2);
  const Double vel2 = 0.5*squaredNorm<nDim>(velocity);
  const Double oneOnRho = 1 / density;

  if (nDim == 2) {
    Double tmp = (gamma-1)/c2;
    pMatInv(0,0) = 1.0 - tmp*vel2;
    pMatInv(0,1) = tmp*velocity[0];
    pMatInv(0,2) = tmp*velocity[1];
    pMatInv(0,3) = -tmp;

    pMatInv(1,0) = (normal(0)*velocity[1]-normal(1)*velocity[0])*oneOnRho;
    pMatInv(1,1) = normal(1)*oneOnRho;
    pMatInv(1,2) = -normal(0)*oneOnRho;
    pMatInv(1,3) = 0.0;
  }
  else {
    Double tmp = (gamma-1)/c2 * normal(0);
    pMatInv(0,0) = normal(0) - tmp*vel2 - (normal(2)*velocity[1]-normal(1)*velocity[2])*oneOnRho;
    pMatInv(0,1) = tmp*velocity[0];
    pMatInv(0,2) = tmp*velocity[1] + normal(2)*oneOnRho;
    pMatInv(0,3) = tmp*velocity[2] - normal(1)*oneOnRho;
    pMatInv(0,4) = -tmp;

    tmp = (gamma-1)/c2 * normal(1);
    pMatInv(1,0) = normal(1) - tmp*vel2 + (normal(2)*velocity[0]-normal(0)*velocity[2])*oneOnRho;
    pMatInv(1,1) = tmp*velocity[0] - normal(2)*oneOnRho;
    pMatInv(1,2) = tmp*velocity[1];
    pMatInv(1,3) = tmp*velocity[2] + normal(0)*oneOnRho;
    pMatInv(1,4) = -tmp;

    tmp = (gamma-1)/c2 * normal(2);
    pMatInv(2,0) = normal(2) - tmp*vel2 - (normal(1)*velocity[0]-normal(0)*velocity[1])*oneOnRho;
    pMatInv(2,1) = tmp*velocity[0] + normal(1)*oneOnRho;
    pMatInv(2,2) = tmp*velocity[1] - normal(0)*oneOnRho;
    pMatInv(2,3) = tmp*velocity[2];
    pMatInv(2,4) = -tmp;
  }

  /*--- Last two rows. ---*/

  const Double gamma_minus_1_on_rho_times_c = (gamma-1) / (density*speedSound);

  for (size_t iVar = nDim; iVar < nDim+2; ++iVar) {
    Double sign = (iVar==nDim)? 1 : -1;
    pMatInv(iVar,0) = -sign*projVel*oneOnRho + gamma_minus_1_on_rho_times_c * vel2;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      pMatInv(iVar,iDim+1) = sign*normal(iDim)*oneOnRho - gamma_minus_1_on_rho_times_c * velocity[iDim];
    }
    pMatInv(iVar,nDim+1) = gamma_minus_1_on_rho_times_c;
  }

  return pMatInv;
}

/*!
 * \brief Convective projected (onto normal) flux (compressible flow).
 */
template<class PrimVarType, class ConsVarType, size_t nDim>
FORCEINLINE VectorDbl<nDim+2> inviscidProjFlux(const PrimVarType& V,
                                               const ConsVarType& U,
                                               const VectorDbl<nDim>& normal) {
  static_assert(ConsVarType::nVar == nDim+2);
  Double mdot = dot(U.momentum(), normal);
  VectorDbl<nDim+2> flux;
  flux(0) = mdot;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    flux(iDim+1) = mdot*V.velocity(iDim) + normal(iDim)*V.pressure();
  }
  flux(nDim+1) = mdot*V.enthalpy();
  return flux;
}

/*!
 * \brief Logarithmic mean (a-b)/(ln a - ln b) of positive a, b, in the stable form of Ismail and Roe (2009),
 * appendix B: f = (a-b)/(a+b), u = f^2, mean = (a+b) / (2 F), F = ln(a/b) / (2f) = 1 + u/3 + u^2/5 + u^3/7 + ...
 * Both branches are evaluated (SIMD), the log one with f = 1 where the series is used, so it stays finite.
 */
FORCEINLINE Double logMean(const Double& a, const Double& b) {
  const Double f = (a - b) / (a + b);
  const Double u = f * f;
  const Double series = 1.0 + u * (1.0/3.0 + u * (1.0/5.0 + u * (1.0/7.0)));
  const Double large = (u >= 1e-2);
  const Double fSafe = large * f + (1.0 - large);
  const Double F = large * log(a / b) / (2.0 * fSafe) + (1.0 - large) * series;
  return (a + b) / (2.0 * F);
}

/*!
 * \brief Entropy conservative flux of Ismail and Roe (2009) projected on "normal" (ideal gas). With z = sqrt(rho/p),
 * y = sqrt(rho p), arithmetic means {.} and logarithmic means ln(.): rho = {z} ln(y), u = {z u}/{z}, p1 = {y}/{z},
 * p2 = [(gamma+1) ln(y)/ln(z) + (gamma-1) {y}/{z}] / (2 gamma), H = gamma p2 / ((gamma-1) rho) + |u|^2/2.
 * It equals the flux of the state when both states are equal.
 */
template<class PrimVarType, size_t nDim>
FORCEINLINE VectorDbl<nDim+2> ismailRoeProjFlux(const Double& gamma,
                                                const PrimVarType& Vi,
                                                const PrimVarType& Vj,
                                                const VectorDbl<nDim>& normal) {
  const Double z_i = sqrt(Vi.density() / Vi.pressure());
  const Double z_j = sqrt(Vj.density() / Vj.pressure());
  const Double y_i = z_i * Vi.pressure();
  const Double y_j = z_j * Vj.pressure();
  const Double zAvg = 0.5 * (z_i + z_j);
  const Double yAvg = 0.5 * (y_i + y_j);
  const Double zLog = logMean(z_i, z_j);
  const Double yLog = logMean(y_i, y_j);

  const Double density = zAvg * yLog;
  VectorDbl<nDim> velocity;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    velocity(iDim) = 0.5 * (z_i * Vi.velocity(iDim) + z_j * Vj.velocity(iDim)) / zAvg;
  }
  const Double pressure1 = yAvg / zAvg;
  const Double pressure2 = ((gamma + 1.0) * yLog / zLog + (gamma - 1.0) * pressure1) / (2.0 * gamma);
  const Double enthalpy = gamma * pressure2 / ((gamma - 1.0) * density) + 0.5 * squaredNorm(velocity);

  const Double mdot = density * dot(velocity, normal);
  VectorDbl<nDim+2> flux;
  flux(0) = mdot;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    flux(iDim+1) = mdot * velocity(iDim) + normal(iDim) * pressure1;
  }
  flux(nDim+1) = mdot * enthalpy;
  return flux;
}

/*!
 * \brief Logarithmic mean of logMean and its partial derivatives, from the derivatives of its two branches:
 * L = (a+b)/(2F(f)), f = (a-b)/(a+b), F the series (u = f^2 < 1e-2) or ln(a/b)/(2f) = atanh(f)/f, whose derivative is
 * (1/(1-u) - F)/f. dL/da = 1/(2F) - b F'/((a+b) F^2), dL/db = 1/(2F) + a F'/((a+b) F^2).
 */
FORCEINLINE void logMeanDerivatives(const Double& a, const Double& b, Double& mean, Double& dMean_a, Double& dMean_b) {
  const Double sum = a + b;
  const Double f = (a - b) / sum;
  const Double u = f * f;
  const Double series = 1.0 + u * (1.0/3.0 + u * (1.0/5.0 + u * (1.0/7.0)));
  const Double dSeries = 2.0 * f * (1.0/3.0 + u * (2.0/5.0 + u * (3.0/7.0)));
  const Double large = (u >= 1e-2);
  const Double fSafe = large * f + (1.0 - large);
  const Double logBranch = log(a / b) / (2.0 * fSafe);
  const Double dLogBranch = (1.0 / (1.0 - u) - logBranch) / fSafe;
  const Double F = large * logBranch + (1.0 - large) * series;
  const Double dF = large * dLogBranch + (1.0 - large) * dSeries;
  mean = sum / (2.0 * F);
  const Double factor = dF / (sum * F * F);
  dMean_a = 0.5 / F - b * factor;
  dMean_b = 0.5 / F + a * factor;
}

/*!
 * \brief Exact Jacobians (EDGE_FORM_EC_EXACT_JACOBIAN) of ismailRoeProjFlux with respect to the conservative variables
 * of the two states, jac_i = dF/dU_i and jac_j = dF/dU_j. Chain rule through z = sqrt(rho/p), y = sqrt(rho p) and the
 * velocity of each state: with zbar = {z}, the logarithmic means zl, yl and their derivatives zl', yl' (one side),
 *   d rho/dz = yl/2, d rho/dy = zbar yl',  d u/dz = (u_s - u)/(2 zbar),  d u/du_s = z_s/(2 zbar),
 *   d p1/dz = -p1/(2 zbar),  d p1/dy = 1/(2 zbar),  d p2 = [(gamma+1) d(yl/zl) + (gamma-1) d p1] / (2 gamma),
 *   d H = gamma/((gamma-1) rho) (d p2 - p2/rho d rho) + u.du,  F = (m, m u + p1 n, m H), m = rho u.n,
 * and dz/dU = z/(2 rho) d rho/dU - z/(2p) dp/dU, dy/dU = y/(2 rho) d rho/dU + y/(2p) dp/dU, dp/dU = (gamma-1)(|u|^2/2, -u, 1).
 * Linear in the normal, like the flux.
 */
template<class PrimVarType, size_t nDim>
FORCEINLINE void ismailRoeProjJac(const Double& gamma,
                                  const PrimVarType& Vi,
                                  const PrimVarType& Vj,
                                  const VectorDbl<nDim>& normal,
                                  MatrixDbl<nDim+2>& jac_i,
                                  MatrixDbl<nDim+2>& jac_j) {
  constexpr size_t nVar = nDim+2;
  const Double z_i = sqrt(Vi.density() / Vi.pressure());
  const Double z_j = sqrt(Vj.density() / Vj.pressure());
  const Double y_i = z_i * Vi.pressure();
  const Double y_j = z_j * Vj.pressure();
  const Double zAvg = 0.5 * (z_i + z_j);
  const Double yAvg = 0.5 * (y_i + y_j);
  Double zLog, dzLog_i, dzLog_j, yLog, dyLog_i, dyLog_j;
  logMeanDerivatives(z_i, z_j, zLog, dzLog_i, dzLog_j);
  logMeanDerivatives(y_i, y_j, yLog, dyLog_i, dyLog_j);

  const Double density = zAvg * yLog;
  VectorDbl<nDim> velocity;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    velocity(iDim) = 0.5 * (z_i * Vi.velocity(iDim) + z_j * Vj.velocity(iDim)) / zAvg;
  }
  const Double pressure1 = yAvg / zAvg;
  const Double pressure2 = ((gamma + 1.0) * yLog / zLog + (gamma - 1.0) * pressure1) / (2.0 * gamma);
  const Double enthalpy = gamma * pressure2 / ((gamma - 1.0) * density) + 0.5 * squaredNorm(velocity);
  const Double projVel = dot(velocity, normal);
  const Double mdot = density * projVel;
  const Double hFactor = gamma / ((gamma - 1.0) * density);

  auto side = [&](const PrimVarType& V, const Double& z, const Double& y, const Double& dzLog, const Double& dyLog,
                  MatrixDbl<nVar>& jac) {
    /*--- Partial derivatives with respect to z, y and the velocity of this state. ---*/
    const Double dRho_z = 0.5 * yLog;
    const Double dRho_y = zAvg * dyLog;
    VectorDbl<nDim> dVel_z;
    for (size_t iDim = 0; iDim < nDim; ++iDim) dVel_z(iDim) = (V.velocity(iDim) - velocity(iDim)) / (2.0 * zAvg);
    const Double dVel_u = z / (2.0 * zAvg);
    const Double dP1_z = -pressure1 / (2.0 * zAvg);
    const Double dP1_y = 1.0 / (2.0 * zAvg);
    const Double dP2_z = (-(gamma + 1.0) * yLog * dzLog / (zLog * zLog) + (gamma - 1.0) * dP1_z) / (2.0 * gamma);
    const Double dP2_y = ((gamma + 1.0) * dyLog / zLog + (gamma - 1.0) * dP1_y) / (2.0 * gamma);
    const Double dH_z = hFactor * (dP2_z - pressure2 / density * dRho_z) + dot(velocity, dVel_z);
    const Double dH_y = hFactor * (dP2_y - pressure2 / density * dRho_y);
    const Double dMdot_z = dRho_z * projVel + density * dot(dVel_z, normal);
    const Double dMdot_y = dRho_y * projVel;

    VectorDbl<nVar> dF_z, dF_y;
    MatrixDbl<nVar, nDim> dF_u;
    dF_z(0) = dMdot_z;
    dF_y(0) = dMdot_y;
    for (size_t iDim = 0; iDim < nDim; ++iDim) {
      dF_z(iDim+1) = dMdot_z * velocity(iDim) + mdot * dVel_z(iDim) + normal(iDim) * dP1_z;
      dF_y(iDim+1) = dMdot_y * velocity(iDim) + normal(iDim) * dP1_y;
    }
    dF_z(nDim+1) = dMdot_z * enthalpy + mdot * dH_z;
    dF_y(nDim+1) = dMdot_y * enthalpy + mdot * dH_y;
    for (size_t jDim = 0; jDim < nDim; ++jDim) {
      const Double dMdot_u = density * normal(jDim) * dVel_u;
      dF_u(0, jDim) = dMdot_u;
      for (size_t iDim = 0; iDim < nDim; ++iDim) {
        dF_u(iDim+1, jDim) = dMdot_u * velocity(iDim);
      }
      dF_u(jDim+1, jDim) += mdot * dVel_u;
      dF_u(nDim+1, jDim) = dMdot_u * enthalpy + mdot * velocity(jDim) * dVel_u;
    }

    /*--- Chain rule to the conservative variables of this state. ---*/
    const Double& rho = V.density();
    const Double& p = V.pressure();
    VectorDbl<nVar> dp;
    dp(0) = 0.5 * (gamma - 1.0) * squaredNorm<nDim>(V.velocity());
    for (size_t iDim = 0; iDim < nDim; ++iDim) dp(iDim+1) = -(gamma - 1.0) * V.velocity(iDim);
    dp(nDim+1) = gamma - 1.0;
    VectorDbl<nVar> dz, dy;
    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      dz(iVar) = -z / (2.0 * p) * dp(iVar);
      dy(iVar) = y / (2.0 * p) * dp(iVar);
    }
    dz(0) += z / (2.0 * rho);
    dy(0) += y / (2.0 * rho);
    const Double invRho = 1.0 / rho;
    for (size_t iVar = 0; iVar < nVar; ++iVar) {
      /*--- du_k/dU = (-u_k/rho, delta_k/rho, 0). ---*/
      Double d0 = dF_z(iVar) * dz(0) + dF_y(iVar) * dy(0);
      for (size_t kDim = 0; kDim < nDim; ++kDim) d0 -= dF_u(iVar, kDim) * V.velocity(kDim) * invRho;
      jac(iVar, 0) = d0;
      for (size_t jVar = 1; jVar < nVar; ++jVar) {
        jac(iVar, jVar) = dF_z(iVar) * dz(jVar) + dF_y(iVar) * dy(jVar);
      }
      for (size_t kDim = 0; kDim < nDim; ++kDim) jac(iVar, kDim+1) += dF_u(iVar, kDim) * invRho;
    }
  };
  side(Vi, z_i, y_i, dzLog_i, dyLog_i, jac_i);
  side(Vj, z_j, y_j, dzLog_j, dyLog_j, jac_j);
}

/*!
 * \brief TeCNO dissipation applied to a vector v of entropy variables (EDGE_FORM_TECNO_ADJOINT):
 * q = scale R |Lambda| T R^T v at the Roe state, eigenvectors of A(n) with the Barth scalings T (acoustic rho/(2 gamma),
 * entropy rho (gamma-1)/gamma, shear p as one tangent vector), the same products as the sigma = 1 dissipation of
 * CUpwindBase::tecnoFlux with dy = R^T v. Symmetric positive semidefinite in v.
 */
template<size_t nDim>
FORCEINLINE VectorDbl<nDim+2> tecnoDissipation(const Double& gamma, const CRoeVariables<nDim>& avg,
                                               const VectorDbl<nDim>& unitNormal, const Double& lamM,
                                               const Double& lam0, const Double& lamP, const Double& scale,
                                               const VectorDbl<nDim+2>& v) {
  const Double& c = avg.speedSound;
  const Double& un = avg.projVel;
  const Double& enthalpy = avg.enthalpy;
  const Double halfSqVel = 0.5 * squaredNorm(avg.velocity);

  Double uv = 0.0, nv = 0.0, nq = 0.0;
  VectorDbl<nDim> yS;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    uv += avg.velocity(iDim) * v(iDim+1);
    nv += unitNormal(iDim) * v(iDim+1);
    yS(iDim) = v(iDim+1) + avg.velocity(iDim) * v(nDim+1);
    nq += unitNormal(iDim) * yS(iDim);
  }
  for (size_t iDim = 0; iDim < nDim; ++iDim) yS(iDim) -= nq * unitNormal(iDim);
  const Double yM = v(0) + uv - c * nv + (enthalpy - un * c) * v(nDim+1);
  const Double y0 = v(0) + uv + halfSqVel * v(nDim+1);
  const Double yP = v(0) + uv + c * nv + (enthalpy + un * c) * v(nDim+1);

  const Double coefM = scale * lamM * avg.density / (2.0 * gamma) * yM;
  const Double coef0 = scale * lam0 * avg.density * (gamma - 1.0) / gamma * y0;
  const Double coefP = scale * lamP * avg.density / (2.0 * gamma) * yP;
  const Double coefS = scale * lam0 * avg.density * c * c / gamma;

  VectorDbl<nDim+2> q;
  q(0) = coefM + coef0 + coefP;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    q(iDim+1) = (coefM + coef0 + coefP) * avg.velocity(iDim) + (coefP - coefM) * c * unitNormal(iDim) +
                coefS * yS(iDim);
  }
  q(nDim+1) = (coefM + coefP) * enthalpy + (coefP - coefM) * un * c + coef0 * halfSqVel +
              coefS * dot(avg.velocity, yS);
  return q;
}

/*!
 * \brief Temporal entropy conservative average of two states (GRID_ENTROPY_P1, P1_ALE.md, Lemma A6), ideal gas:
 * ubar = rho_ln (1, {v}, 1/(2(gamma-1) beta_ln) + |{v}|^2 - {|v|^2}/2), beta = rho/(2p), ln the logarithmic mean.
 * It is symmetric, ubar(U,U) = U, and [[w]].ubar = [[rho]] for the entropy variables w of U = -rho s/(gamma-1),
 * rho being the temporal potential w.U - U. So -ubar g_ik with an antisymmetric grid flux g_ik that satisfies the
 * discrete GCL produces only boundary terms of entropy.
 */
template<class PrimVarType, size_t nDim = PrimVarType::nDim>
FORCEINLINE VectorDbl<nDim+2> ecTemporalAverage(const Double& gamma, const PrimVarType& Vi, const PrimVarType& Vj) {
  const Double rhoLog = logMean(Vi.density(), Vj.density());
  const Double betaLog = logMean(0.5 * Vi.density() / Vi.pressure(), 0.5 * Vj.density() / Vj.pressure());
  Double velAvg2 = 0.0, vel2Avg = 0.0;
  VectorDbl<nDim+2> ubar;
  ubar(0) = rhoLog;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    const Double velAvg = 0.5 * (Vi.velocity(iDim) + Vj.velocity(iDim));
    velAvg2 += velAvg * velAvg;
    vel2Avg += 0.5 * (Vi.velocity(iDim) * Vi.velocity(iDim) + Vj.velocity(iDim) * Vj.velocity(iDim));
    ubar(iDim+1) = rhoLog * velAvg;
  }
  ubar(nDim+1) = rhoLog * (0.5 / ((gamma - 1.0) * betaLog) + velAvg2 - 0.5 * vel2Avg);
  return ubar;
}

/*!
 * \brief Edge projection of a dual normal, nu = (n.e/|e|^2) e (EDGE_DISS_PROJECTED).
 * \note nu.e = n.e, and nu = n when n is parallel to e. Among the normals with the edge projection n.e it has the
 *       least length, hence the least acoustic dissipation c|nu| (Lemma G4 of the optimal dual notes).
 * \param[in] normal - Median normal n of the edge, oriented from i to j.
 * \param[in] vector_ij - Edge vector e = x_j - x_i.
 * \param[out] projNormal - nu.
 * \param[out] projArea - |nu| = n.e/|e|, positive since the geometry checks n.e > 0.
 * \param[out] projUnitNormal - nu/|nu| = e/|e|.
 */
template<size_t nDim>
FORCEINLINE void projectedNormal(const VectorDbl<nDim>& normal, const VectorDbl<nDim>& vector_ij,
                                 VectorDbl<nDim>& projNormal, Double& projArea, VectorDbl<nDim>& projUnitNormal) {
  const Double invLength = 1.0 / norm(vector_ij);
  projArea = dot(normal, vector_ij) * invLength;
  const Double scale = projArea * invLength;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    projNormal(iDim) = scale * vector_ij(iDim);
    projUnitNormal(iDim) = vector_ij(iDim) * invLength;
  }
}

/*!
 * \brief Jacobian of the convective flux (compressible flow, ideal gas).
 */
template<size_t nDim, class RandomAccessIterator>
FORCEINLINE MatrixDbl<nDim+2> inviscidProjJac(const Double& gamma, RandomAccessIterator velocity,
                                              const Double& energy, const VectorDbl<nDim>& normal,
                                              const Double& scale) {
  MatrixDbl<nDim+2> jac;

  Double projVel = dot(velocity, normal);
  Double gamma_m_1 = gamma-1;
  Double phi = 0.5*gamma_m_1*squaredNorm<nDim>(velocity);
  Double a1 = gamma*energy - phi;

  jac(0,0) = 0.0;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    jac(0,iDim+1) = scale * normal(iDim);
  }
  jac(0,nDim+1) = 0.0;

  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    jac(iDim+1,0) = scale * (normal(iDim)*phi - velocity[iDim]*projVel);
    for (size_t jDim = 0; jDim < nDim; ++jDim) {
      jac(iDim+1,jDim+1) = scale * (normal(jDim)*velocity[iDim] - gamma_m_1*normal(iDim)*velocity[jDim]);
    }
    jac(iDim+1,iDim+1) += scale * projVel;
    jac(iDim+1,nDim+1) = scale * gamma_m_1 * normal(iDim);
  }

  jac(nDim+1,0) = scale * projVel * (phi-a1);
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    jac(nDim+1,iDim+1) = scale * (normal(iDim)*a1 - gamma_m_1*velocity[iDim]*projVel);
  }
  jac(nDim+1,nDim+1) = scale * gamma * projVel;

  return jac;
}

template<size_t nDim, class RandomAccessIterator>
FORCEINLINE MatrixDbl<nDim+2> inviscidProjJacPrim(Double gamma, Double density, RandomAccessIterator velocity,
                                                Double enthalpy, const VectorDbl<nDim>& normal,
                                                Double scale) {
  MatrixDbl<nDim+2> jac;

  Double projVel = dot(velocity, normal);
  Double gamma_m_1 = gamma-1;
  Double sqvel = squaredNorm<nDim>(velocity);


  jac(0,0) = scale*projVel;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    jac(0,iDim+1) = scale * density * normal(iDim);
  }
  jac(0,nDim+1) = 0.0;

  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    jac(iDim+1,0) = scale * velocity[iDim] * projVel;
    for (size_t jDim = 0; jDim < nDim; ++jDim) {
      jac(iDim+1,jDim+1) = scale * density * normal(jDim) * velocity[iDim];
    }
    jac(iDim+1,iDim+1) += scale * density * projVel;
    jac(iDim+1,nDim+1) = scale * normal(iDim);
  }

  jac(nDim+1,0) = 0.5 * scale * projVel * sqvel;
  for (size_t iDim = 0; iDim < nDim; ++iDim) {
    jac(nDim+1,iDim+1) = scale * density * (normal(iDim)*enthalpy + velocity[iDim]*projVel);
  }
  jac(nDim+1,nDim+1) = scale * gamma * projVel / (gamma_m_1);

  return jac;
}

/*!
 * \brief (Low) Dissipation coefficient for Roe schemes.
 */
template<class VariableType>
FORCEINLINE Double roeDissipation(const Int& iPoint,
                                  const Int& jPoint,
                                  const ENUM_ROELOWDISS type,
                                  const VariableType& solution) {
  if (type == NO_ROELOWDISS) {
    return 1.0;
  }

  const auto& sol = static_cast<const CNSVariable&>(solution);
  const auto& sensor = sol.GetSensor();
  const auto& dissip = sol.GetRoe_Dissipation();

  const Double si = gatherVariables(iPoint, sensor);
  const Double sj = gatherVariables(jPoint, sensor);
  const Double avgSensor = 0.5 * (si + sj);

  const Double di = gatherVariables(iPoint, dissip);
  const Double dj = gatherVariables(jPoint, dissip);
  const Double avgDissip = 0.5 * (di + dj);

  /*--- A minimum level of upwinding is used to enhance stability. ---*/
  constexpr passivedouble minDissip = 0.05;

  switch (type) {
    case FD:
    case FD_DUCROS: {
      Double d = fmax(minDissip, 1.0 - avgDissip);

      if (type == FD_DUCROS) {
        /*--- See Jonhsen et al. JCP 229 (2010) pag. 1234 ---*/
        d = fmax(d, 0.05 + 0.95*(avgSensor > 0.65));
      }
      return d;
    }
    case NTS:
      return fmax(minDissip, avgDissip);

    case NTS_DUCROS:
      /*--- See Xiao et al. INT J HEAT FLUID FL 51 (2015) pag. 141
       * https://doi.org/10.1016/j.ijheatfluidflow.2014.10.007 ---*/
      return fmax(minDissip, avgSensor+avgDissip - avgSensor*avgDissip);

    default:
      assert(false);
      return 1.0;
  }
}

/*!
 * \brief Correct spectral radius (avgLambda) for stretching.
 */
template<class VariableType, class T>
FORCEINLINE Double correctedSpectralRadius(const Int& iPoint,
                                           const Int& jPoint,
                                           const Double& avgLambda,
                                           const T& stretchParam,
                                           const VariableType& solution) {

  const auto lambda_i = gatherVariables(iPoint, solution.GetLambda());
  const Double phi_i = pow(0.25*lambda_i/avgLambda, stretchParam);

  const auto lambda_j = gatherVariables(jPoint, solution.GetLambda());
  const Double phi_j = pow(0.25*lambda_j/avgLambda, stretchParam);

  return 4*phi_i*phi_j / (phi_i + phi_j) * avgLambda;
}

/*!
 * \brief Update of a flux Jacobian due to a scalar dissipation term.
 */
template<class VariableType, size_t nVar>
FORCEINLINE void scalarDissipationJacobian(const VariableType& V,
                                           const Double& gamma,
                                           Double dissipConst,
                                           MatrixDbl<nVar>& jac) {
  /*--- Diagonal entries. ---*/
  for (size_t iVar = 0; iVar < nVar-1; ++iVar) {
    jac(iVar,iVar) += dissipConst;
  }
  jac(nVar-1,nVar-1) += dissipConst * gamma;

  /*--- N-1 columns of last row. ---*/
  dissipConst *= gamma-1.0;
  for (size_t iDim = 0; iDim < VariableType::nDim; ++iDim) {
    jac(nVar-1,iDim+1) -= dissipConst * V.velocity(iDim);
    jac(nVar-1,0) += dissipConst * pow(V.velocity(iDim), 2);
  }
}
