/*!
 * \file p1_boundary_ghost.hpp
 * \brief Ghost states of the far field, inlet and outlet of the compressible solver, shared by the weak boundary
 *        residuals of CEulerSolver (BC_Far_Field_Residual, BC_Inlet_Residual, BC_Outlet_Residual) and closure C of
 *        BC_ENTROPY_P1, which builds them at the face points from the entropy projected state.
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

/*--- The formulas and the order of the operations are those of the weak boundary residuals they were taken from, so
 * those give the same states as before. Inputs: the boundary state (density, velocity, pressure and, for the inlet,
 * the enthalpy h = (rho E + p)/rho of the primitive vector), the outward unit normal and the marker data. Closure C
 * (P1_BOUNDARY.md, Thm B9; P1_NOSLIP.md, 11) passes the entropy projected state of a face point and the marker data
 * interpolated to it; Lemma B7 holds for any admissible ghost, which Admissible checks. ---*/

namespace P1BoundaryGhost {

/*! \brief max(0, x) and min(1, x) with the semantics of std::max(0.0, x), std::min(1.0, x). */
inline su2double MaxZero(const su2double& x) { return (0.0 < x) ? x : su2double(0.0); }
inline su2double MinOne(const su2double& x) { return (x < 1.0) ? x : su2double(1.0); }

/*! \brief rho > 0 and p > 0, both finite. */
inline bool Admissible(const su2double& rho, const su2double& p) {
  return std::isfinite(SU2_TYPE::GetValue(rho)) && std::isfinite(SU2_TYPE::GetValue(p)) && (rho > 0.0) && (p > 0.0);
}

/*!
 * \brief Characteristic far-field ghost (Riemann invariants), the state of BC_Far_Field_Residual and of closure C.
 */
struct FarFieldState {
  su2double Qn_Infty = 0.0;          /*!< \brief Free stream normal velocity relative to the grid. */
  su2double SoundSpeed_Infty = 0.0;  /*!< \brief Free stream speed of sound. */
  su2double SoundSpeed = 0.0;        /*!< \brief Speed of sound of the ghost. */
  su2double Entropy = 0.0;           /*!< \brief rho^gamma/p of the ghost. */
  su2double Density = 0.0, Pressure = 0.0, Velocity[3] = {0.0, 0.0, 0.0};
};

/*!
 * \brief Far-field ghost. Outflow (Qn_Infty > 0) keeps the boundary entropy and tangential velocity, inflow those of
 *        the free stream; the normal velocity and the speed of sound come from the Riemann invariants.
 * \param[in] gridVel - Grid velocity of the boundary node, nullptr for a static grid.
 */
inline void FarField(unsigned short nDim, const su2double& gamma, const su2double& densityB, const su2double* velB,
                     const su2double& pressureB, const su2double& densityInf, const su2double* velInf,
                     const su2double& pressureInf, const su2double* unitNormal, const su2double* gridVel,
                     FarFieldState& G) {
  const su2double gm1 = gamma - 1.0;

  su2double Vn_Bound = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) Vn_Bound += velB[iDim] * unitNormal[iDim];
  const su2double SoundSpeed_Bound = sqrt(gamma * pressureB / densityB);
  const su2double Entropy_Bound = pow(densityB, gamma) / pressureB;

  su2double Vn_Infty = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) Vn_Infty += velInf[iDim] * unitNormal[iDim];
  G.SoundSpeed_Infty = sqrt(gamma * pressureInf / densityInf);
  const su2double Entropy_Infty = pow(densityInf, gamma) / pressureInf;

  G.Qn_Infty = Vn_Infty;
  if (gridVel != nullptr) {
    for (unsigned short iDim = 0; iDim < nDim; iDim++) G.Qn_Infty -= gridVel[iDim] * unitNormal[iDim];
  }

  /*--- if/else rather than ?: so that the AD expression types need not match. ---*/
  su2double RiemannPlus, RiemannMinus;
  if (G.Qn_Infty > -G.SoundSpeed_Infty) RiemannPlus = Vn_Bound + 2.0 * SoundSpeed_Bound / gm1;
  else RiemannPlus = Vn_Infty + 2.0 * G.SoundSpeed_Infty / gm1;
  if (G.Qn_Infty > G.SoundSpeed_Infty) RiemannMinus = Vn_Bound - 2.0 * SoundSpeed_Bound / gm1;
  else RiemannMinus = Vn_Infty - 2.0 * G.SoundSpeed_Infty / gm1;

  const su2double Vn = 0.5 * (RiemannPlus + RiemannMinus);
  G.SoundSpeed = 0.25 * (RiemannPlus - RiemannMinus) * gm1;

  if (G.Qn_Infty > 0.0) {
    for (unsigned short iDim = 0; iDim < nDim; iDim++) G.Velocity[iDim] = velB[iDim] + (Vn - Vn_Bound) * unitNormal[iDim];
    G.Entropy = Entropy_Bound;
  } else {
    for (unsigned short iDim = 0; iDim < nDim; iDim++) G.Velocity[iDim] = velInf[iDim] + (Vn - Vn_Infty) * unitNormal[iDim];
    G.Entropy = Entropy_Infty;
  }
  G.Density = pow(G.Entropy * G.SoundSpeed * G.SoundSpeed / gamma, 1.0 / gm1);
  G.Pressure = G.Density * G.SoundSpeed * G.SoundSpeed / gamma;
}

/*!
 * \brief Subsonic inlet ghost of BC_Inlet_Residual.
 */
struct InletState {
  su2double Temperature = 0.0, Pressure = 0.0, Density = 0.0, Velocity[3] = {0.0, 0.0, 0.0};
  su2double Velocity2 = 0.0;  /*!< \brief |v|^2 of the energy (total conditions). */
  su2double Vel_Mag = 0.0;    /*!< \brief |v| of the energy (mass flow). */
};

/*!
 * \brief Inlet ghost: total conditions (P_t, T_t, flow direction) or mass flow (density, velocity magnitude, flow
 *        direction), closed with the outgoing Riemann invariant of the boundary state.
 * \param[in] massFlow - INLET_TYPE::MASS_FLOW, otherwise TOTAL_CONDITIONS.
 * \param[in] enthalpyB - h = (rho E + p)/rho of the boundary state without k (SST: primitive nDim+3 minus k, as
 *            BC_Inlet; the imposed total enthalpy has no k).
 * \param[in] dataT - Inlet_Ttotal: total temperature, or the density for mass flow (dimensional).
 * \param[in] dataP - Inlet_Ptotal: total pressure, or the velocity magnitude for mass flow (dimensional).
 * \param[in] flowDir - Flow direction (not necessarily unit), unused with useNormal.
 * \param[in] refT - Temperature reference, or the density reference for mass flow.
 * \param[in] refP - Pressure reference, or the velocity reference for mass flow.
 */
inline void Inlet(unsigned short nDim, bool massFlow, const su2double& gamma, const su2double& gasConst,
                  const su2double& densityB, const su2double* velB, const su2double& pressureB,
                  const su2double& enthalpyB, const su2double& dataT, const su2double& dataP, const su2double* flowDir,
                  bool useNormal, const su2double* unitNormal, const su2double& refT, const su2double& refP,
                  InletState& G) {
  const su2double gm1 = gamma - 1.0;
  const su2double Two_Gamma_M1 = 2.0 / gm1;

  /*--- Unit flow direction, or the local boundary normal (negative). ---*/
  su2double mag = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) mag += flowDir[iDim] * flowDir[iDim];
  mag = sqrt(mag);
  su2double Flow_Dir[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; iDim++) {
    if (useNormal) Flow_Dir[iDim] = -unitNormal[iDim];
    else Flow_Dir[iDim] = flowDir[iDim] / mag;
  }

  if (!massFlow) {
    const su2double P_Total = dataP / refP;
    const su2double T_Total = dataT / refT;

    su2double Velocity2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; iDim++) Velocity2 += velB[iDim] * velB[iDim];
    const su2double Energy = enthalpyB - pressureB / densityB;
    const su2double H_Total = (gamma * gasConst / gm1) * T_Total;
    su2double SoundSpeed2 = gamma * pressureB / densityB;

    /*--- Acoustic Riemann invariant extrapolated from the interior. ---*/
    su2double Riemann = 2.0 * sqrt(SoundSpeed2) / gm1;
    for (unsigned short iDim = 0; iDim < nDim; iDim++) Riemann += velB[iDim] * unitNormal[iDim];

    const su2double SoundSpeed_Total2 = gm1 * (H_Total - (Energy + pressureB / densityB) + 0.5 * Velocity2) + SoundSpeed2;

    su2double alpha = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; iDim++) alpha += unitNormal[iDim] * Flow_Dir[iDim];

    /*--- Quadratic equation for the velocity magnitude, the positive root. ---*/
    const su2double aa = 1.0 + 0.5 * gm1 * alpha * alpha;
    const su2double bb = -1.0 * gm1 * alpha * Riemann;
    const su2double cc = 0.5 * gm1 * Riemann * Riemann - 2.0 * SoundSpeed_Total2 / gm1;
    su2double dd = bb * bb - 4.0 * aa * cc;
    dd = sqrt(MaxZero(dd));
    su2double Vel_Mag = (-bb + dd) / (2.0 * aa);
    Vel_Mag = MaxZero(Vel_Mag);
    Velocity2 = Vel_Mag * Vel_Mag;

    SoundSpeed2 = SoundSpeed_Total2 - 0.5 * gm1 * Velocity2;

    /*--- Mach squared cut at 1. ---*/
    su2double Mach2 = Velocity2 / SoundSpeed2;
    Mach2 = MinOne(Mach2);
    Velocity2 = Mach2 * SoundSpeed2;
    Vel_Mag = sqrt(Velocity2);
    SoundSpeed2 = SoundSpeed_Total2 - 0.5 * gm1 * Velocity2;

    for (unsigned short iDim = 0; iDim < nDim; iDim++) G.Velocity[iDim] = Vel_Mag * Flow_Dir[iDim];
    G.Temperature = SoundSpeed2 / (gamma * gasConst);
    G.Pressure = P_Total * pow((G.Temperature / T_Total), gamma / gm1);
    G.Density = G.Pressure / (gasConst * G.Temperature);
    G.Velocity2 = Velocity2;
    G.Vel_Mag = Vel_Mag;
    return;
  }

  /*--- Mass flow. ---*/
  const su2double Density = dataT / refT;
  const su2double Vel_Mag = dataP / refP;
  su2double SoundSpeed2 = gamma * pressureB / densityB;

  su2double Riemann = Two_Gamma_M1 * sqrt(SoundSpeed2);
  for (unsigned short iDim = 0; iDim < nDim; iDim++) Riemann += velB[iDim] * unitNormal[iDim];

  SoundSpeed2 = Riemann;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) SoundSpeed2 -= Vel_Mag * Flow_Dir[iDim] * unitNormal[iDim];
  const su2double half = 0.5 * gm1 * SoundSpeed2;
  SoundSpeed2 = MaxZero(half);
  SoundSpeed2 = SoundSpeed2 * SoundSpeed2;

  G.Pressure = SoundSpeed2 * Density / gamma;
  G.Density = Density;
  G.Temperature = G.Pressure / (gasConst * Density);
  for (unsigned short iDim = 0; iDim < nDim; iDim++) G.Velocity[iDim] = Vel_Mag * Flow_Dir[iDim];
  G.Vel_Mag = Vel_Mag;
  G.Velocity2 = Vel_Mag * Vel_Mag;
}

/*!
 * \brief Outlet ghost of BC_Outlet_Residual.
 */
struct OutletState {
  bool supersonic = false;  /*!< \brief Mach >= 1: the ghost is the boundary state itself. */
  su2double Density = 0.0, Pressure = 0.0, Velocity[3] = {0.0, 0.0, 0.0}, Velocity2 = 0.0;
};

/*!
 * \brief Outlet ghost: subsonic, the back pressure with the boundary entropy, the outgoing Riemann invariant and the
 *        tangential velocity; supersonic, the boundary state.
 * \param[in] pExit - Non-dimensional back pressure.
 */
inline void Outlet(unsigned short nDim, const su2double& gamma, const su2double& densityB, const su2double* velB,
                   const su2double& pressureB, const su2double& pExit, const su2double* unitNormal, OutletState& G) {
  const su2double gm1 = gamma - 1.0;

  su2double Velocity2 = 0.0, Vn = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) {
    G.Velocity[iDim] = velB[iDim];
    Velocity2 += G.Velocity[iDim] * G.Velocity[iDim];
    Vn += G.Velocity[iDim] * unitNormal[iDim];
  }
  su2double SoundSpeed = sqrt(gamma * pressureB / densityB);
  const su2double Mach_Exit = sqrt(Velocity2) / SoundSpeed;

  G.supersonic = (Mach_Exit >= 1.0);
  if (G.supersonic) {
    G.Density = densityB;
    G.Pressure = pressureB;
    G.Velocity2 = Velocity2;
    return;
  }

  const su2double Entropy = pressureB * pow(1.0 / densityB, gamma);
  const su2double Riemann = Vn + 2.0 * SoundSpeed / gm1;

  G.Density = pow(pExit / Entropy, 1.0 / gamma);
  G.Pressure = pExit;
  SoundSpeed = sqrt(gamma * pExit / G.Density);
  const su2double Vn_Exit = Riemann - 2.0 * SoundSpeed / gm1;
  G.Velocity2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) {
    G.Velocity[iDim] = G.Velocity[iDim] + (Vn_Exit - Vn) * unitNormal[iDim];
    G.Velocity2 += G.Velocity[iDim] * G.Velocity[iDim];
  }
}

/*--- Exact Jacobians of the ghosts above for closure C of BC_ENTROPY_P1 (P1_BOUNDARY.md, 11): dG[a][b] = dP_g[a]/dP_B[b]
 * in the primitive variables P = (rho, u_1..u_nDim, p) of the ghost (rows) and of the boundary state (columns). The
 * branches are those of the builders, evaluated with the same expressions; the marker data are constants. Static grid
 * only (gridVel = nullptr), as closure C uses them. ---*/

constexpr unsigned short JAC_NVAR = 5;

inline void ZeroJacobian(unsigned short nVar, su2double (*dG)[JAC_NVAR]) {
  for (unsigned short a = 0; a < nVar; a++)
    for (unsigned short b = 0; b < nVar; b++) dG[a][b] = 0.0;
}

/*!
 * \brief Jacobian of FarField without grid motion. The switches depend on the free stream only (Qn_Infty = u_inf.n),
 *        so the ghost is smooth in the boundary state: dR+ = dVn_B + 2 dc_B/(gamma-1) where it comes from the boundary
 *        (0 otherwise), likewise dR-, dVn = (dR+ + dR-)/2, dc = (gamma-1)(dR+ - dR-)/4,
 *        d rho = rho/(gamma-1) (dS/S + 2 dc/c), dp = p (d rho/rho + 2 dc/c), dS = dS_B at outflow, 0 at inflow.
 */
inline void FarFieldJacobian(unsigned short nDim, const su2double& gamma, const su2double& densityB,
                             const su2double* velB, const su2double& pressureB, const su2double& densityInf,
                             const su2double* velInf, const su2double& pressureInf, const su2double* unitNormal,
                             su2double (*dG)[JAC_NVAR]) {
  const unsigned short nVar = nDim + 2;
  const su2double gm1 = gamma - 1.0;
  ZeroJacobian(nVar, dG);

  su2double Vn_Bound = 0.0, Vn_Infty = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) {
    Vn_Bound += velB[iDim] * unitNormal[iDim];
    Vn_Infty += velInf[iDim] * unitNormal[iDim];
  }
  const su2double SoundSpeed_Bound = sqrt(gamma * pressureB / densityB);
  const su2double Entropy_Bound = pow(densityB, gamma) / pressureB;
  const su2double SoundSpeed_Infty = sqrt(gamma * pressureInf / densityInf);
  const su2double Entropy_Infty = pow(densityInf, gamma) / pressureInf;
  const su2double Qn_Infty = Vn_Infty;

  su2double dVnB[JAC_NVAR] = {0.0}, dcB[JAC_NVAR] = {0.0}, dSB[JAC_NVAR] = {0.0};
  for (unsigned short iDim = 0; iDim < nDim; iDim++) dVnB[iDim+1] = unitNormal[iDim];
  dcB[0] = -0.5 * SoundSpeed_Bound / densityB;
  dcB[nDim+1] = 0.5 * SoundSpeed_Bound / pressureB;
  dSB[0] = gamma * Entropy_Bound / densityB;
  dSB[nDim+1] = -Entropy_Bound / pressureB;

  su2double RiemannPlus, RiemannMinus, dRp[JAC_NVAR] = {0.0}, dRm[JAC_NVAR] = {0.0};
  if (Qn_Infty > -SoundSpeed_Infty) {
    RiemannPlus = Vn_Bound + 2.0 * SoundSpeed_Bound / gm1;
    for (unsigned short b = 0; b < nVar; b++) dRp[b] = dVnB[b] + 2.0 * dcB[b] / gm1;
  } else {
    RiemannPlus = Vn_Infty + 2.0 * SoundSpeed_Infty / gm1;
  }
  if (Qn_Infty > SoundSpeed_Infty) {
    RiemannMinus = Vn_Bound - 2.0 * SoundSpeed_Bound / gm1;
    for (unsigned short b = 0; b < nVar; b++) dRm[b] = dVnB[b] - 2.0 * dcB[b] / gm1;
  } else {
    RiemannMinus = Vn_Infty - 2.0 * SoundSpeed_Infty / gm1;
  }
  const su2double SoundSpeed = 0.25 * (RiemannPlus - RiemannMinus) * gm1;

  const bool outflow = (Qn_Infty > 0.0);
  su2double Entropy = Entropy_Infty;
  if (outflow) Entropy = Entropy_Bound;
  const su2double Density = pow(Entropy * SoundSpeed * SoundSpeed / gamma, 1.0 / gm1);
  const su2double Pressure = Density * SoundSpeed * SoundSpeed / gamma;

  for (unsigned short b = 0; b < nVar; b++) {
    const su2double dVn = 0.5 * (dRp[b] + dRm[b]);
    const su2double dc = 0.25 * gm1 * (dRp[b] - dRm[b]);
    su2double dLogS = 0.0;
    if (outflow) dLogS = dSB[b] / Entropy;
    dG[0][b] = Density / gm1 * (dLogS + 2.0 * dc / SoundSpeed);
    dG[nDim+1][b] = Pressure * (dG[0][b] / Density + 2.0 * dc / SoundSpeed);
    for (unsigned short iDim = 0; iDim < nDim; iDim++) {
      if (outflow) dG[iDim+1][b] = (dVn - dVnB[b]) * unitNormal[iDim];
      else dG[iDim+1][b] = dVn * unitNormal[iDim];
    }
  }
  if (outflow) {
    for (unsigned short iDim = 0; iDim < nDim; iDim++) dG[iDim+1][iDim+1] += 1.0;
  }
}

/*!
 * \brief Jacobian of Inlet. Total conditions: with h the enthalpy of the boundary state (as closure C passes it),
 *        SoundSpeed_Total2 = (gamma-1) H_Total does not depend on the state, and the velocity magnitude depends on it
 *        through the Riemann invariant R only, dR = dVn_B + 2 dc_B/(gamma-1); the root, the clips MaxZero/MinOne and
 *        the static relations of the builder are differentiated in turn (a clipped value has zero derivative).
 *        Mass flow: density and velocity are data, p = max(0, (gamma-1)(R - |v| dir.n)/2)^2 rho/gamma.
 */
inline void InletJacobian(unsigned short nDim, bool massFlow, const su2double& gamma, const su2double& gasConst,
                          const su2double& densityB, const su2double* velB, const su2double& pressureB,
                          const su2double& enthalpyB, const su2double& dataT, const su2double& dataP,
                          const su2double* flowDir, bool useNormal, const su2double* unitNormal,
                          const su2double& refT, const su2double& refP, su2double (*dG)[JAC_NVAR]) {
  const unsigned short nVar = nDim + 2;
  const su2double gm1 = gamma - 1.0;
  const su2double Two_Gamma_M1 = 2.0 / gm1;
  ZeroJacobian(nVar, dG);

  su2double mag = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) mag += flowDir[iDim] * flowDir[iDim];
  mag = sqrt(mag);
  su2double Flow_Dir[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; iDim++) {
    if (useNormal) Flow_Dir[iDim] = -unitNormal[iDim];
    else Flow_Dir[iDim] = flowDir[iDim] / mag;
  }

  /*--- Riemann invariant of the boundary state and its gradient. ---*/
  const su2double SoundSpeedB2 = gamma * pressureB / densityB;
  const su2double SoundSpeedB = sqrt(SoundSpeedB2);
  su2double Riemann = Two_Gamma_M1 * SoundSpeedB, dR[JAC_NVAR] = {0.0};
  for (unsigned short iDim = 0; iDim < nDim; iDim++) {
    Riemann += velB[iDim] * unitNormal[iDim];
    dR[iDim+1] = unitNormal[iDim];
  }
  dR[0] = -Two_Gamma_M1 * 0.5 * SoundSpeedB / densityB;
  dR[nDim+1] = Two_Gamma_M1 * 0.5 * SoundSpeedB / pressureB;

  if (!massFlow) {
    const su2double P_Total = dataP / refP;
    const su2double T_Total = dataT / refT;

    su2double Velocity2 = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; iDim++) Velocity2 += velB[iDim] * velB[iDim];
    const su2double Energy = enthalpyB - pressureB / densityB;
    const su2double H_Total = (gamma * gasConst / gm1) * T_Total;
    const su2double SoundSpeed_Total2 = gm1 * (H_Total - (Energy + pressureB / densityB) + 0.5 * Velocity2) + SoundSpeedB2;

    su2double alpha = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; iDim++) alpha += unitNormal[iDim] * Flow_Dir[iDim];
    const su2double aa = 1.0 + 0.5 * gm1 * alpha * alpha;
    const su2double bb = -1.0 * gm1 * alpha * Riemann;
    const su2double cc = 0.5 * gm1 * Riemann * Riemann - 2.0 * SoundSpeed_Total2 / gm1;
    const su2double dd = bb * bb - 4.0 * aa * cc;
    su2double ddRoot = 0.0;
    if (0.0 < dd) ddRoot = sqrt(dd);
    const su2double velMagRoot = (-bb + ddRoot) / (2.0 * aa);
    const bool positiveRoot = (0.0 < velMagRoot);
    su2double Vel_Mag = 0.0;
    if (positiveRoot) Vel_Mag = velMagRoot;
    const su2double Velocity2a = Vel_Mag * Vel_Mag;
    const su2double SoundSpeed2a = SoundSpeed_Total2 - 0.5 * gm1 * Velocity2a;
    const su2double Mach2a = Velocity2a / SoundSpeed2a;
    const bool subsonic = (Mach2a < 1.0);
    su2double Mach2 = 1.0;
    if (subsonic) Mach2 = Mach2a;
    const su2double Velocity2b = Mach2 * SoundSpeed2a;
    const su2double Vel_Mag_b = sqrt(Velocity2b);
    const su2double SoundSpeed2 = SoundSpeed_Total2 - 0.5 * gm1 * Velocity2b;
    const su2double Temperature = SoundSpeed2 / (gamma * gasConst);
    const su2double Pressure = P_Total * pow((Temperature / T_Total), gamma / gm1);
    const su2double Density = Pressure / (gasConst * Temperature);

    for (unsigned short b = 0; b < nVar; b++) {
      const su2double dbb = -gm1 * alpha * dR[b];
      const su2double dcc = gm1 * Riemann * dR[b];
      su2double dRoot = 0.0;
      if (0.0 < dd) dRoot = (2.0 * bb * dbb - 4.0 * aa * dcc) / (2.0 * ddRoot);
      su2double dVelMag = 0.0;
      if (positiveRoot) dVelMag = (-dbb + dRoot) / (2.0 * aa);
      const su2double dVelocity2a = 2.0 * Vel_Mag * dVelMag;
      const su2double dSoundSpeed2a = -0.5 * gm1 * dVelocity2a;
      su2double dMach2 = 0.0;
      if (subsonic) dMach2 = (dVelocity2a - Mach2a * dSoundSpeed2a) / SoundSpeed2a;
      const su2double dVelocity2b = dMach2 * SoundSpeed2a + Mach2 * dSoundSpeed2a;
      su2double dVelMagB = 0.0;
      if (Vel_Mag_b > 0.0) dVelMagB = dVelocity2b / (2.0 * Vel_Mag_b);
      const su2double dTemperature = -0.5 * gm1 * dVelocity2b / (gamma * gasConst);
      const su2double dPressure = Pressure * gamma / gm1 * dTemperature / Temperature;
      dG[0][b] = Density * (dPressure / Pressure - dTemperature / Temperature);
      for (unsigned short iDim = 0; iDim < nDim; iDim++) dG[iDim+1][b] = dVelMagB * Flow_Dir[iDim];
      dG[nDim+1][b] = dPressure;
    }
    return;
  }

  /*--- Mass flow. ---*/
  const su2double Density = dataT / refT;
  const su2double Vel_Mag = dataP / refP;
  su2double SoundSpeed = Riemann;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) SoundSpeed -= Vel_Mag * Flow_Dir[iDim] * unitNormal[iDim];
  const su2double half = 0.5 * gm1 * SoundSpeed;
  if (0.0 < half) {
    for (unsigned short b = 0; b < nVar; b++) dG[nDim+1][b] = 2.0 * half * 0.5 * gm1 * dR[b] * Density / gamma;
  }
}

/*!
 * \brief Jacobian of Outlet. Supersonic: the identity. Subsonic: p_g = pExit, rho_g = (pExit/(p rho^-gamma))^(1/gamma),
 *        so d rho_g = rho_g (d rho/rho - dp/(gamma p)), and u_g = u + (Vn_Exit - Vn) n with
 *        Vn_Exit - Vn = 2 (c - c_g)/(gamma-1), dc = c (dp/p - d rho/rho)/2, dc_g = -c_g d rho_g/(2 rho_g).
 */
inline void OutletJacobian(unsigned short nDim, const su2double& gamma, const su2double& densityB,
                           const su2double* velB, const su2double& pressureB, const su2double& pExit,
                           const su2double* unitNormal, su2double (*dG)[JAC_NVAR]) {
  const unsigned short nVar = nDim + 2;
  const su2double gm1 = gamma - 1.0;
  ZeroJacobian(nVar, dG);

  su2double Velocity2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; iDim++) Velocity2 += velB[iDim] * velB[iDim];
  const su2double SoundSpeed = sqrt(gamma * pressureB / densityB);
  const su2double Mach_Exit = sqrt(Velocity2) / SoundSpeed;

  if (Mach_Exit >= 1.0) {
    for (unsigned short a = 0; a < nVar; a++) dG[a][a] = 1.0;
    return;
  }

  const su2double Entropy = pressureB * pow(1.0 / densityB, gamma);
  const su2double Density = pow(pExit / Entropy, 1.0 / gamma);
  const su2double SoundSpeedExit = sqrt(gamma * pExit / Density);

  su2double dc[JAC_NVAR] = {0.0};
  dc[0] = -0.5 * SoundSpeed / densityB;
  dc[nDim+1] = 0.5 * SoundSpeed / pressureB;
  dG[0][0] = Density / densityB;
  dG[0][nDim+1] = -Density / (gamma * pressureB);
  for (unsigned short b = 0; b < nVar; b++) {
    const su2double dcExit = -0.5 * SoundSpeedExit * dG[0][b] / Density;
    for (unsigned short iDim = 0; iDim < nDim; iDim++) dG[iDim+1][b] = 2.0 / gm1 * (dc[b] - dcExit) * unitNormal[iDim];
  }
  for (unsigned short iDim = 0; iDim < nDim; iDim++) dG[iDim+1][iDim+1] += 1.0;
}

}  // namespace P1BoundaryGhost
