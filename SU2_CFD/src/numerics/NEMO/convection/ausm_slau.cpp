/*!
 * \file ausm_slau.cpp
 * \brief Implementations of the AUSM-family of schemes in NEMO.
 * \author F. Palacios, S.R. Copeland, W. Maier, C. Garbacz
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

#include "../../../../include/numerics/NEMO/convection/ausm_slau.hpp"

#include "../../../../../Common/include/toolboxes/geometry_toolbox.hpp"

CUpwAUSM_SLAU_Base_NEMO::CUpwAUSM_SLAU_Base_NEMO(unsigned short val_nDim, unsigned short val_nVar,
                                                 unsigned short val_nPrimVar, unsigned short val_nPrimVarGrad,
                                                 const CConfig* config)
    : CNEMONumerics(val_nDim, val_nVar, val_nPrimVar, val_nPrimVarGrad, config) {
  if (config->GetDynamic_Grid() && (SU2_MPI::GetRank() == MASTER_NODE))
    cout << "WARNING: Grid velocities are NOT yet considered in AUSM-type schemes." << endl;

  implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);

  rhos_i = new su2double[nSpecies];
  rhos_j = new su2double[nSpecies];

  Fc_L = new su2double[nVar];
  Fc_R = new su2double[nVar];
  dM_LP = new su2double[nVar];
  dM_RM = new su2double[nVar];
  dP_LP = new su2double[nVar];
  dP_RM = new su2double[nVar];
  da_L = new su2double[nVar];
  da_R = new su2double[nVar];

  Flux = new su2double[nVar];
  Jacobian_i = new su2double*[nVar];
  Jacobian_j = new su2double*[nVar];
  for (auto iVar = 0ul; iVar < nVar; iVar++) {
    Jacobian_i[iVar] = new su2double[nVar];
    Jacobian_j[iVar] = new su2double[nVar];
  }
}

CUpwAUSM_SLAU_Base_NEMO::~CUpwAUSM_SLAU_Base_NEMO() {
  delete[] rhos_i;
  delete[] rhos_j;

  delete[] Fc_L;
  delete[] Fc_R;
  delete[] dM_LP;
  delete[] dM_RM;
  delete[] dP_LP;
  delete[] dP_RM;
  delete[] da_L;
  delete[] da_R;

  delete[] Flux;
  for (auto iVar = 0ul; iVar < nVar; iVar++) {
    delete[] Jacobian_i[iVar];
    delete[] Jacobian_j[iVar];
  }
  delete[] Jacobian_i;
  delete[] Jacobian_j;
}

void CUpwAUSM_SLAU_Base_NEMO::ComputeInterfaceQuantities(const CConfig* config, su2double* pressure,
                                                         su2double& interface_mach, su2double* interface_soundspeed) {
  /*--- For schemes that fit in the general form of Advection Upstream Splitting Method (AUSM) schemes you can inherit
  from this class and implement only the specifics, which should be the face pressure flux(es), interface Mach number
  and the interface soundspeed(s). For implicit solution methods this class uses the analytic AUSM Jacobians as an
  approximation, unless the scheme sets HasAnalyticalDerivatives and also computes the derivatives of the convective
  velocity (interface Mach number times interface soundspeed) and of the pressure flux, see AccurateJacobian.
  ---*/
}

void CUpwAUSM_SLAU_Base_NEMO::SplitDerivatives(su2double mach, su2double alpha, su2double beta, su2double& dMP_dM,
                                               su2double& dPP_dM, su2double& dPP_dalpha) {
  if (fabs(mach) <= 1.0) {
    const su2double tmp = mach * mach - 1.0;
    dMP_dM = 0.5 * (mach + 1.0) + 4.0 * beta * mach * tmp;
    dPP_dM = tmp * (alpha * (5.0 * tmp + 4.0) - 0.75);
    dPP_dalpha = mach * tmp * tmp;
  } else {
    dMP_dM = (mach > 0.0) ? 1.0 : 0.0;
    dPP_dM = 0.0;
    dPP_dalpha = 0.0;
  }
}

void CUpwAUSM_SLAU_Base_NEMO::ApproximateJacobian(su2double** val_Jacobian_i, su2double** val_Jacobian_j) {
  const auto& Ms = fluidmodel->GetSpeciesMolarMass();
  const auto& Cvtr = fluidmodel->GetSpeciesCvTraRot();
  const su2double Ru = 1000.0 * UNIVERSAL_GAS_CONSTANT;
  rhoCvtr_i = V_i[RHOCVTR_INDEX];
  rhoCvtr_j = V_j[RHOCVTR_INDEX];

  /*--- Initialize the Jacobians ---*/
  for (auto iVar = 0ul; iVar < nVar; iVar++) {
    for (auto jVar = 0ul; jVar < nVar; jVar++) {
      Jacobian_i[iVar][jVar] = 0.0;
      Jacobian_j[iVar][jVar] = 0.0;
    }
  }

  /*--- Determine proper flux and soundspeed ---*/
  if (M_F >= 0.0)
    Fc_LR = Fc_L;
  else
    Fc_LR = Fc_R;

  su2double A_LR = 0;
  if (M_F >= 0.0)
    A_LR = A_F[0];
  else
    A_LR = A_F[1];

  /*--- Sound speed derivatives: Species density ---*/

  // Electrons only
  for (auto iSpecies = 0ul; iSpecies < nEl; iSpecies++) {
    da_L[iSpecies] = 1.0 / (2.0 * SoundSpeed_i * Density_i) * (1 + dPdU_i[nSpecies + nDim]) *
                     (dPdU_i[iSpecies] - Pressure_i / Density_i);
    da_R[iSpecies] = 1.0 / (2.0 * SoundSpeed_j * Density_j) * (1 + dPdU_j[nSpecies + nDim]) *
                     (dPdU_j[iSpecies] - Pressure_j / Density_j);
  }

  // Heavy species
  for (auto iSpecies = nEl; iSpecies < nSpecies; iSpecies++) {
    da_L[iSpecies] =
        1.0 / (2.0 * SoundSpeed_i) *
        (1 / rhoCvtr_i * (Ru / Ms[iSpecies] - Cvtr[iSpecies] * dPdU_i[nSpecies + nDim]) * Pressure_i / Density_i +
         1.0 / Density_i * (1.0 + dPdU_i[nSpecies + nDim]) * (dPdU_i[iSpecies] - Pressure_i / Density_i));
    da_R[iSpecies] =
        1.0 / (2.0 * SoundSpeed_j) *
        (1 / rhoCvtr_j * (Ru / Ms[iSpecies] - Cvtr[iSpecies] * dPdU_j[nSpecies + nDim]) * Pressure_j / Density_j +
         1.0 / Density_j * (1.0 + dPdU_j[nSpecies + nDim]) * (dPdU_j[iSpecies] - Pressure_j / Density_j));
  }

  /*--- Sound speed derivatives: Momentum ---*/
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    da_L[nSpecies + iDim] = -1.0 / (2.0 * Density_i * SoundSpeed_i) *
                            ((1.0 + dPdU_i[nSpecies + nDim]) * dPdU_i[nSpecies + nDim]) * Velocity_i[iDim];
    da_R[nSpecies + iDim] = -1.0 / (2.0 * Density_j * SoundSpeed_j) *
                            ((1.0 + dPdU_j[nSpecies + nDim]) * dPdU_j[nSpecies + nDim]) * Velocity_j[iDim];
  }

  /*--- Sound speed derivatives: Energy ---*/
  da_L[nSpecies + nDim] =
      1.0 / (2.0 * Density_i * SoundSpeed_i) * ((1.0 + dPdU_i[nSpecies + nDim]) * dPdU_i[nSpecies + nDim]);
  da_R[nSpecies + nDim] =
      1.0 / (2.0 * Density_j * SoundSpeed_j) * ((1.0 + dPdU_j[nSpecies + nDim]) * dPdU_j[nSpecies + nDim]);

  /*--- Sound speed derivatives: Vib-el energy ---*/
  da_L[nSpecies + nDim + 1] =
      1.0 / (2.0 * Density_i * SoundSpeed_i) * ((1.0 + dPdU_i[nSpecies + nDim]) * dPdU_i[nSpecies + nDim + 1]);
  da_R[nSpecies + nDim + 1] =
      1.0 / (2.0 * Density_j * SoundSpeed_j) * ((1.0 + dPdU_j[nSpecies + nDim]) * dPdU_j[nSpecies + nDim + 1]);

  /*--- Left state Jacobian ---*/
  if (M_F >= 0) {
    /*--- Jacobian contribution: dFc terms ---*/
    for (auto iVar = 0ul; iVar < nSpecies + nDim; iVar++) {
      for (auto jVar = 0ul; jVar < nVar; jVar++) {
        Jacobian_i[iVar][jVar] += M_F * Fc_L[iVar] * da_L[jVar];
      }
      Jacobian_i[iVar][iVar] += M_F * SoundSpeed_i;
    }
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++) {
      Jacobian_i[nSpecies + nDim][iSpecies] +=
          M_F * (dPdU_i[iSpecies] * SoundSpeed_i + Density_i * Enthalpy_i * da_L[iSpecies]);
    }
    for (auto iDim = 0ul; iDim < nDim; iDim++) {
      Jacobian_i[nSpecies + nDim][nSpecies + iDim] +=
          M_F *
          (-dPdU_i[nSpecies + nDim] * Velocity_i[iDim] * SoundSpeed_i + Density_i * Enthalpy_i * da_L[nSpecies + iDim]);
    }
    Jacobian_i[nSpecies + nDim][nSpecies + nDim] +=
        M_F * ((1.0 + dPdU_i[nSpecies + nDim]) * SoundSpeed_i + Density_i * Enthalpy_i * da_L[nSpecies + nDim]);
    Jacobian_i[nSpecies + nDim][nSpecies + nDim + 1] +=
        M_F * (dPdU_i[nSpecies + nDim + 1] * SoundSpeed_i + Density_i * Enthalpy_i * da_L[nSpecies + nDim + 1]);
    for (auto jVar = 0ul; jVar < nVar; jVar++) {
      Jacobian_i[nSpecies + nDim + 1][jVar] += M_F * Fc_L[nSpecies + nDim + 1] * da_L[jVar];
    }
    Jacobian_i[nSpecies + nDim + 1][nSpecies + nDim + 1] += M_F * SoundSpeed_i;
  }

  /*--- Calculate derivatives of the split Mach number and pressure of the left state. They are
   needed for every interface Mach number: at a shock face the interface Mach number can be
   above 1 in size while this state is subsonic. For a supersonic state the branches below
   give the correct limits. ---*/
  if (fabs(M_L) <= 1.0) {
    /*--- Mach number ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++)
      dM_LP[iSpecies] =
          0.5 * (M_L + 1.0) *
          (-ProjVelocity_i / (Density_i * SoundSpeed_i) - ProjVelocity_i * da_L[iSpecies] / (pow(SoundSpeed_i, 2)));
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dM_LP[nSpecies + iDim] = 0.5 * (M_L + 1.0) *
                               (-ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + iDim] +
                                UnitNormal[iDim] / (Density_i * SoundSpeed_i));
    dM_LP[nSpecies + nDim] = 0.5 * (M_L + 1.0) * (-ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + nDim]);
    dM_LP[nSpecies + nDim + 1] =
        0.5 * (M_L + 1.0) * (-ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + nDim + 1]);

    /*--- Pressure ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++)
      dP_LP[iSpecies] = 0.25 * (M_L + 1.0) *
                        (dPdU_i[iSpecies] * (M_L + 1.0) * (2.0 - M_L) +
                         Pressure_i *
                             (-ProjVelocity_i / (Density_i * SoundSpeed_i) -
                              ProjVelocity_i * da_L[iSpecies] / (pow(SoundSpeed_i, 2))) *
                             (3.0 - 3.0 * M_L));
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dP_LP[nSpecies + iDim] = 0.25 * (M_L + 1.0) *
                               (-Velocity_i[iDim] * dPdU_i[nSpecies + nDim] * (M_L + 1.0) * (2.0 - M_L) +
                                Pressure_i *
                                    (-ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + iDim] +
                                     UnitNormal[iDim] / (Density_i * SoundSpeed_i)) *
                                    (3.0 - 3.0 * M_L));
    dP_LP[nSpecies + nDim] =
        0.25 * (M_L + 1.0) *
        (dPdU_i[nSpecies + nDim] * (M_L + 1.0) * (2.0 - M_L) +
         Pressure_i * (-ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + nDim]) * (3.0 - 3.0 * M_L));
    dP_LP[nSpecies + nDim + 1] =
        0.25 * (M_L + 1.0) *
        (dPdU_i[nSpecies + nDim + 1] * (M_L + 1.0) * (2.0 - M_L) +
         Pressure_i * (-ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + nDim + 1]) * (3.0 - 3.0 * M_L));
  } else if (M_L > 1.0) {
    /*--- Mach number ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++)
      dM_LP[iSpecies] =
          -ProjVelocity_i / (Density_i * SoundSpeed_i) - ProjVelocity_i * da_L[iSpecies] / (pow(SoundSpeed_i, 2));
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dM_LP[nSpecies + iDim] = -ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + iDim] +
                               UnitNormal[iDim] / (Density_i * SoundSpeed_i);
    dM_LP[nSpecies + nDim] = -ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + nDim];
    dM_LP[nSpecies + nDim + 1] = -ProjVelocity_i / (pow(SoundSpeed_i, 2)) * da_L[nSpecies + nDim + 1];

    /*--- Pressure ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++) dP_LP[iSpecies] = dPdU_i[iSpecies];
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dP_LP[nSpecies + iDim] = (-Velocity_i[iDim] * dPdU_i[nSpecies + nDim]);
    dP_LP[nSpecies + nDim] = dPdU_i[nSpecies + nDim];
    dP_LP[nSpecies + nDim + 1] = dPdU_i[nSpecies + nDim + 1];
  } else {
    /*--- Supersonic flow towards the left state, the split Mach number and pressure are zero ---*/
    for (auto iVar = 0ul; iVar < nVar; iVar++) {
      dM_LP[iVar] = 0.0;
      dP_LP[iVar] = 0.0;
    }
  }

  /*--- dM contribution ---*/
  for (auto iVar = 0ul; iVar < nVar; iVar++) {
    for (auto jVar = 0ul; jVar < nVar; jVar++) {
      Jacobian_i[iVar][jVar] += dM_LP[jVar] * Fc_LR[iVar] * A_LR;
    }
  }

  /*--- Jacobian contribution: dP terms ---*/
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    for (auto iVar = 0ul; iVar < nVar; iVar++) {
      Jacobian_i[nSpecies + iDim][iVar] += dP_LP[iVar] * UnitNormal[iDim];
    }
  }

  /*--- Right state Jacobian ---*/
  if (M_F < 0) {
    /*--- Jacobian contribution: dFc terms ---*/
    for (auto iVar = 0ul; iVar < nSpecies + nDim; iVar++) {
      for (auto jVar = 0ul; jVar < nVar; jVar++) {
        Jacobian_j[iVar][jVar] += M_F * Fc_R[iVar] * da_R[jVar];
      }
      Jacobian_j[iVar][iVar] += M_F * SoundSpeed_j;
    }
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++) {
      Jacobian_j[nSpecies + nDim][iSpecies] +=
          M_F * (dPdU_j[iSpecies] * SoundSpeed_j + Density_j * Enthalpy_j * da_R[iSpecies]);
    }
    for (auto iDim = 0ul; iDim < nDim; iDim++) {
      Jacobian_j[nSpecies + nDim][nSpecies + iDim] +=
          M_F *
          (-dPdU_j[nSpecies + nDim] * Velocity_j[iDim] * SoundSpeed_j + Density_j * Enthalpy_j * da_R[nSpecies + iDim]);
    }
    Jacobian_j[nSpecies + nDim][nSpecies + nDim] +=
        M_F * ((1.0 + dPdU_j[nSpecies + nDim]) * SoundSpeed_j + Density_j * Enthalpy_j * da_R[nSpecies + nDim]);
    Jacobian_j[nSpecies + nDim][nSpecies + nDim + 1] +=
        M_F * (dPdU_j[nSpecies + nDim + 1] * SoundSpeed_j + Density_j * Enthalpy_j * da_R[nSpecies + nDim + 1]);
    for (auto jVar = 0ul; jVar < nVar; jVar++) {
      Jacobian_j[nSpecies + nDim + 1][jVar] += M_F * Fc_R[nSpecies + nDim + 1] * da_R[jVar];
    }
    Jacobian_j[nSpecies + nDim + 1][nSpecies + nDim + 1] += M_F * SoundSpeed_j;
  }

  /*--- Calculate derivatives of the split Mach number and pressure of the right state, see
   the note for the left state. ---*/
  if (fabs(M_R) <= 1.0) {
    /*--- Mach ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++)
      dM_RM[iSpecies] =
          -0.5 * (M_R - 1.0) *
          (-ProjVelocity_j / (Density_j * SoundSpeed_j) - ProjVelocity_j * da_R[iSpecies] / (pow(SoundSpeed_j, 2)));
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dM_RM[nSpecies + iDim] = -0.5 * (M_R - 1.0) *
                               (-ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + iDim] +
                                UnitNormal[iDim] / (Density_j * SoundSpeed_j));
    dM_RM[nSpecies + nDim] = -0.5 * (M_R - 1.0) * (-ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + nDim]);
    dM_RM[nSpecies + nDim + 1] =
        -0.5 * (M_R - 1.0) * (-ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + nDim + 1]);

    /*--- Pressure ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++)
      dP_RM[iSpecies] = 0.25 * (M_R - 1.0) *
                        (dPdU_j[iSpecies] * (M_R - 1.0) * (2.0 + M_R) +
                         Pressure_j *
                             (-ProjVelocity_j / (Density_j * SoundSpeed_j) -
                              ProjVelocity_j * da_R[iSpecies] / (pow(SoundSpeed_j, 2))) *
                             (3.0 + 3.0 * M_R));
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dP_RM[nSpecies + iDim] = 0.25 * (M_R - 1.0) *
                               ((-Velocity_j[iDim] * dPdU_j[nSpecies + nDim]) * (M_R - 1.0) * (2.0 + M_R) +
                                Pressure_j *
                                    (-ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + iDim] +
                                     UnitNormal[iDim] / (Density_j * SoundSpeed_j)) *
                                    (3.0 + 3.0 * M_R));
    dP_RM[nSpecies + nDim] =
        0.25 * (M_R - 1.0) *
        (dPdU_j[nSpecies + nDim] * (M_R - 1.0) * (2.0 + M_R) +
         Pressure_j * (-ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + nDim]) * (3.0 + 3.0 * M_R));
    dP_RM[nSpecies + nDim + 1] =
        0.25 * (M_R - 1.0) *
        (dPdU_j[nSpecies + nDim + 1] * (M_R - 1.0) * (2.0 + M_R) +
         Pressure_j * (-ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + nDim + 1]) * (3.0 + 3.0 * M_R));

  } else if (M_R < -1.0) {
    /*--- Mach ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++)
      dM_RM[iSpecies] =
          -ProjVelocity_j / (Density_j * SoundSpeed_j) - ProjVelocity_j * da_R[iSpecies] / (pow(SoundSpeed_j, 2));
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dM_RM[nSpecies + iDim] = -ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + iDim] +
                               UnitNormal[iDim] / (Density_j * SoundSpeed_j);
    dM_RM[nSpecies + nDim] = -ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + nDim];
    dM_RM[nSpecies + nDim + 1] = -ProjVelocity_j / (pow(SoundSpeed_j, 2)) * da_R[nSpecies + nDim + 1];

    /*--- Pressure ---*/
    for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++) dP_RM[iSpecies] = dPdU_j[iSpecies];
    for (auto iDim = 0ul; iDim < nDim; iDim++)
      dP_RM[nSpecies + iDim] = -Velocity_j[iDim] * dPdU_j[nSpecies + nDim];
    dP_RM[nSpecies + nDim] = dPdU_j[nSpecies + nDim];
    dP_RM[nSpecies + nDim + 1] = dPdU_j[nSpecies + nDim + 1];
  } else {
    /*--- Supersonic flow towards the right state, the split Mach number and pressure are zero ---*/
    for (auto iVar = 0ul; iVar < nVar; iVar++) {
      dM_RM[iVar] = 0.0;
      dP_RM[iVar] = 0.0;
    }
  }

  /*--- Jacobian contribution: dM terms ---*/
  for (auto iVar = 0ul; iVar < nVar; iVar++) {
    for (auto jVar = 0ul; jVar < nVar; jVar++) {
      Jacobian_j[iVar][jVar] += dM_RM[jVar] * Fc_LR[iVar] * A_LR;
    }
  }

  /*--- Jacobian contribution: dP terms ---*/
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    for (auto iVar = 0ul; iVar < nVar; iVar++) {
      Jacobian_j[nSpecies + iDim][iVar] += dP_RM[iVar] * UnitNormal[iDim];
    }
  }

  /*--- Integrate over dual-face area ---*/
  for (auto iVar = 0ul; iVar < nVar; iVar++) {
    for (auto jVar = 0ul; jVar < nVar; jVar++) {
      Jacobian_i[iVar][jVar] *= Area;
      Jacobian_j[iVar][jVar] *= Area;
    }
  }
}

void CUpwAUSM_SLAU_Base_NEMO::AccurateJacobian(su2double** val_Jacobian_i, su2double** val_Jacobian_j) {
  /*--- The flux is F = ConvVel * Fc + PressureFlux. ConvVel = M_F * A_F is the convective velocity at the
   interface, Fc are the convected variables of the upwind state (species densities, momentum, rho*H and
   rho*e_ve), and the pressure flux goes in the momentum equations. The scheme gives the derivatives of
   ConvVel and PressureFlux with respect to [velocity, P, rho, H, gamma] of the two states, here they are
   converted to derivatives with respect to the conservative variables to build the Jacobians. ---*/

  const auto& Ms = fluidmodel->GetSpeciesMolarMass();
  const auto& Cvtr = fluidmodel->GetSpeciesCvTraRot();
  const su2double Ru = 1000.0 * UNIVERSAL_GAS_CONSTANT;

  const bool upwind_i = (M_F >= 0.0);
  const su2double* Fc_upwind = upwind_i ? Fc_L : Fc_R;
  const su2double ConvVel = M_F * (upwind_i ? A_F[0] : A_F[1]);

  auto StateJacobian = [&](const su2double* V, const su2double* dPdU, const su2double Gamma, const bool upwind,
                           const su2double* dConvVel_dV, const auto& dPressureFlux_dV, su2double** Jacobian) {
    const su2double Density = V[RHO_INDEX];

    for (auto jVar = 0ul; jVar < nVar; jVar++) {
      /*--- Derivatives of [velocity, P, rho, H, gamma] with respect to the conservative variable jVar.
       The enthalpy is H = (rho*E + P) / rho, and gamma = 1 + (rho*R) / (rho*Cv_tr) only depends on
       the species densities. ---*/
      su2double dV_dU[MAXNVAR_DERIV] = {0.0};

      dV_dU[nDim] = dPdU[jVar];
      dV_dU[nDim + 2] = dPdU[jVar] / Density;

      if (jVar < nSpecies) {
        for (auto iDim = 0ul; iDim < nDim; iDim++) dV_dU[iDim] = -V[VEL_INDEX + iDim] / Density;
        dV_dU[nDim + 1] = 1.0;
        dV_dU[nDim + 2] -= V[H_INDEX] / Density;
        dV_dU[nDim + 3] = (Ru / Ms[jVar] - (Gamma - 1.0) * Cvtr[jVar]) / V[RHOCVTR_INDEX];
      } else if (jVar < nSpecies + nDim) {
        dV_dU[jVar - nSpecies] = 1.0 / Density;
      } else if (jVar == nSpecies + nDim) {
        dV_dU[nDim + 2] += 1.0 / Density;
      }

      /*--- Chain rule ---*/
      su2double dConvVel_dU = 0.0, dPressureFlux_dU[MAXNDIM] = {0.0};
      for (auto iVar = 0ul; iVar < nDim + 4ul; iVar++) {
        dConvVel_dU += dConvVel_dV[iVar] * dV_dU[iVar];
        for (auto iDim = 0ul; iDim < nDim; iDim++) dPressureFlux_dU[iDim] += dPressureFlux_dV[iDim][iVar] * dV_dU[iVar];
      }

      /*--- Jacobian contribution: convective velocity and pressure flux ---*/
      for (auto iVar = 0ul; iVar < nVar; iVar++) Jacobian[iVar][jVar] = Fc_upwind[iVar] * dConvVel_dU;
      for (auto iDim = 0ul; iDim < nDim; iDim++) Jacobian[nSpecies + iDim][jVar] += dPressureFlux_dU[iDim];

      /*--- Jacobian contribution: convected variables, only for the upwind state. They are the
       conservative variables, with the pressure added to the total energy (rho*H = rho*E + P). ---*/
      if (upwind) {
        Jacobian[jVar][jVar] += ConvVel;
        Jacobian[nSpecies + nDim][jVar] += ConvVel * dPdU[jVar];
      }

      /*--- Integrate over dual-face area ---*/
      for (auto iVar = 0ul; iVar < nVar; iVar++) Jacobian[iVar][jVar] *= Area;
    }
  };

  StateJacobian(V_i, dPdU_i, Gamma_i, upwind_i, dConvVel_dVi, dPressureFlux_dVi, val_Jacobian_i);
  StateJacobian(V_j, dPdU_j, Gamma_j, !upwind_i, dConvVel_dVj, dPressureFlux_dVj, val_Jacobian_j);
}

CNumerics::ResidualType<> CUpwAUSM_SLAU_Base_NEMO::ComputeResidual(const CConfig* config) {
  /*--- Compute geometric quantities ---*/
  Area = GeometryToolbox::Norm(nDim, Normal);

  for (auto iDim = 0ul; iDim < nDim; iDim++) UnitNormal[iDim] = Normal[iDim] / Area;

  /*--- Pull stored primitive variables ---*/
  // Primitives: [rho1,...,rhoNs, T, Tve, u, v, w, P, rho, h, a, c]
  for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++) {
    rhos_i[iSpecies] = V_i[RHOS_INDEX + iSpecies];
    rhos_j[iSpecies] = V_j[RHOS_INDEX + iSpecies];
  }
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    Velocity_i[iDim] = V_i[VEL_INDEX + iDim];
    Velocity_j[iDim] = V_j[VEL_INDEX + iDim];
  }

  Pressure_i = V_i[P_INDEX];
  Enthalpy_i = V_i[H_INDEX];
  Density_i = V_i[RHO_INDEX];
  SoundSpeed_i = V_i[A_INDEX];

  Pressure_j = V_j[P_INDEX];
  Enthalpy_j = V_j[H_INDEX];
  Density_j = V_j[RHO_INDEX];
  SoundSpeed_j = V_j[A_INDEX];

  e_ve_i = e_ve_j = 0;
  for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++) {
    e_ve_i += (V_i[RHOS_INDEX + iSpecies] * eve_i[iSpecies]) / Density_i;
    e_ve_j += (V_j[RHOS_INDEX + iSpecies] * eve_j[iSpecies]) / Density_j;
  }

  /*--- Projected velocities ---*/
  ProjVelocity_i = GeometryToolbox::DotProduct(nDim, Velocity_i, UnitNormal);
  ProjVelocity_j = GeometryToolbox::DotProduct(nDim, Velocity_j, UnitNormal);

  /*--- Compute mass and pressure fluxes of specific scheme ---*/
  ComputeInterfaceQuantities(config, PressureFlux, M_F, A_F);

  const su2double MassFlux_i = M_F * A_F[0];
  const su2double MassFlux_j = M_F * A_F[1];

  const su2double DissFlux_i = fabs(MassFlux_i);
  const su2double DissFlux_j = fabs(MassFlux_j);

  /*--- Assign left & right convective flux vectors ---*/
  for (auto iSpecies = 0ul; iSpecies < nSpecies; iSpecies++) {
    Fc_L[iSpecies] = rhos_i[iSpecies];
    Fc_R[iSpecies] = rhos_j[iSpecies];
  }
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    Fc_L[nSpecies + iDim] = Density_i * Velocity_i[iDim];
    Fc_R[nSpecies + iDim] = Density_j * Velocity_j[iDim];
  }
  Fc_L[nSpecies + nDim] = Density_i * Enthalpy_i;
  Fc_R[nSpecies + nDim] = Density_j * Enthalpy_j;
  Fc_L[nSpecies + nDim + 1] = Density_i * e_ve_i;
  Fc_R[nSpecies + nDim + 1] = Density_j * e_ve_j;

  /*--- Compute numerical flux ---*/
  for (auto iVar = 0ul; iVar < nVar; iVar++)
    Flux[iVar] = 0.5 * ((MassFlux_i + DissFlux_i) * Fc_L[iVar] + (MassFlux_j - DissFlux_j) * Fc_R[iVar]) * Area;

  for (auto iDim = 0ul; iDim < nDim; iDim++) Flux[nSpecies + iDim] += PressureFlux[iDim] * Area;

  /*--- If required, compute Jacobians (approximated using AUSM if the scheme does not have its own) ---*/
  if (implicit) {
    if (HasAnalyticalDerivatives)
      AccurateJacobian(Jacobian_i, Jacobian_j);
    else
      ApproximateJacobian(Jacobian_i, Jacobian_j);
  }

  return ResidualType<>(Flux, Jacobian_i, Jacobian_j);
}

CUpwAUSM_NEMO::CUpwAUSM_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar,
                             unsigned short val_nPrimVarGrad, const CConfig* config)
    : CUpwAUSM_SLAU_Base_NEMO(val_nDim, val_nVar, val_nPrimVar, val_nPrimVarGrad, config) {}

void CUpwAUSM_NEMO::ComputeInterfaceQuantities(const CConfig* config, su2double* pressure, su2double& interface_mach,
                                               su2double* interface_soundspeed) {
  /*--- Calculate L/R Mach numbers ---*/
  interface_soundspeed[0] = SoundSpeed_i;
  interface_soundspeed[1] = SoundSpeed_j;
  M_L = ProjVelocity_i / interface_soundspeed[0];
  M_R = ProjVelocity_j / interface_soundspeed[1];

  /*--- Calculate split numerical fluxes ---*/
  su2double M_LP, M_RM, P_LP, P_RM;

  if (fabs(M_L) <= 1.0) {
    M_LP = 0.25 * (M_L + 1.0) * (M_L + 1.0);
    P_LP = 0.25 * Pressure_i * (M_L + 1.0) * (M_L + 1.0) * (2.0 - M_L);
  } else {
    M_LP = 0.5 * (M_L + fabs(M_L));
    P_LP = 0.5 * Pressure_i * (M_L + fabs(M_L)) / M_L;
  }

  if (fabs(M_R) <= 1.0) {
    M_RM = -0.25 * (M_R - 1.0) * (M_R - 1.0);
    P_RM = 0.25 * Pressure_j * (M_R - 1.0) * (M_R - 1.0) * (2.0 + M_R);
  } else {
    M_RM = 0.5 * (M_R - fabs(M_R));
    P_RM = 0.5 * Pressure_j * (M_R - fabs(M_R)) / M_R;
  }

  // M_Lnferface = M(1/2) = (M_LP + M_RM)
  interface_mach = (M_LP + M_RM);

  // Split pressure P(1/2)
  // Note: only a single pressure flux in AUSM
  for (auto iDim = 0ul; iDim < nDim; iDim++) pressure[iDim] = (P_LP + P_RM) * UnitNormal[iDim];
}

CUpwAUSMPLUSUP2_NEMO::CUpwAUSMPLUSUP2_NEMO(unsigned short val_nDim, unsigned short val_nVar,
                                           unsigned short val_nPrimVar, unsigned short val_nPrimVarGrad,
                                           const CConfig* config)
    : CUpwAUSM_SLAU_Base_NEMO(val_nDim, val_nVar, val_nPrimVar, val_nPrimVarGrad, config) {
  Minf = config->GetMach();
  Kp = 0.25;
  sigma = 1.0;
  HasAnalyticalDerivatives = true;

  if (Minf < EPS)
    SU2_MPI::Error("AUSM+-Up2 requires a reference Mach number (\"MACH_NUMBER\") greater than 0.", CURRENT_FUNCTION);
}

void CUpwAUSMPLUSUP2_NEMO::ComputeInterfaceQuantities(const CConfig* config, su2double* pressure,
                                                      su2double& interface_mach, su2double* interface_soundspeed) {
  const su2double sq_veli = GeometryToolbox::SquaredNorm(nDim, Velocity_i);
  const su2double sq_velj = GeometryToolbox::SquaredNorm(nDim, Velocity_j);

  /*--- Compute C*  ---*/
  const su2double CstarL = sqrt(2.0 * (Gamma_i - 1.0) / (Gamma_i + 1.0) * Enthalpy_i);
  const su2double CstarR = sqrt(2.0 * (Gamma_j - 1.0) / (Gamma_j + 1.0) * Enthalpy_j);

  /*--- Compute C^ ---*/
  const su2double ChatL = CstarL * CstarL / max(CstarL, ProjVelocity_i);
  const su2double ChatR = CstarR * CstarR / max(CstarR, -ProjVelocity_j);

  /*--- Interface speed of sound ---*/
  const su2double aF = min(ChatL, ChatR);
  interface_soundspeed[0] = interface_soundspeed[1] = aF;

  M_L = ProjVelocity_i / aF;
  M_R = ProjVelocity_j / aF;

  const su2double rhoF = 0.5 * (Density_i + Density_j);
  const su2double MFsq = 0.5 * (M_L * M_L + M_R * M_R);

  const su2double param1 = max(MFsq, Minf * Minf);
  const su2double Mrefsq = (min(1.0, param1));
  const su2double fa = 2.0 * sqrt(Mrefsq) - Mrefsq;

  const su2double alpha = 3.0 / 16.0 * (-4.0 + 5.0 * fa * fa);
  const su2double beta = 1.0 / 8.0;

  /*--- Pressure diffusion term ---*/
  const su2double Mp = -(Kp / fa) * max((1.0 - sigma * MFsq), 0.0) * (Pressure_j - Pressure_i) / (rhoF * aF * aF);

  su2double M_LP, P_LP, M_RM, P_RM;

  if (fabs(M_L) <= 1.0) {
    M_LP = 0.25 * (M_L + 1.0) * (M_L + 1.0) + beta * (M_L * M_L - 1.0) * (M_L * M_L - 1.0);
    P_LP = (0.25 * (M_L + 1.0) * (M_L + 1.0) * (2.0 - M_L) + alpha * M_L * (M_L * M_L - 1.0) * (M_L * M_L - 1.0));
  } else {
    M_LP = 0.5 * (M_L + fabs(M_L));
    P_LP = 0.5 * (M_L + fabs(M_L)) / M_L;
  }

  if (fabs(M_R) <= 1.0) {
    M_RM = -0.25 * (M_R - 1.0) * (M_R - 1.0) - beta * (M_R * M_R - 1.0) * (M_R * M_R - 1.0);
    P_RM = (0.25 * (M_R - 1.0) * (M_R - 1.0) * (2.0 + M_R) - alpha * M_R * (M_R * M_R - 1.0) * (M_R * M_R - 1.0));
  } else {
    M_RM = 0.5 * (M_R - fabs(M_R));
    P_RM = 0.5 * (M_R - fabs(M_R)) / M_R;
  }

  /*--- Interface Mach number ---*/
  interface_mach = (M_LP + M_RM + Mp);

  /*--- Modified pressure flux ---*/
  const su2double pFi = sqrt(0.5 * (sq_veli + sq_velj)) * (P_LP + P_RM - 1.0) * 0.5 * (Density_j + Density_i) * aF;

  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    pressure[iDim] =
        (0.5 * (Pressure_j + Pressure_i) + 0.5 * (P_LP - P_RM) * (Pressure_i - Pressure_j) + pFi) * UnitNormal[iDim];
  }

  if (!implicit) return;

  /*--- Analytical differentiation of the convective velocity (interface_mach * aF) and of the pressure
   flux with respect to the velocity, pressure, density, enthalpy and gamma of the two states. It is done
   in reverse mode, "x_b" denotes the derivative of the output with respect to x. For min, max and abs
   the branch that is active is differentiated. The equation numbers are the ones of the reference
   (Kitamura and Shima, 2013, Appendix A). ---*/

  su2double dMLP_dML, dPLP_dML, dPLP_dalpha, dMRM_dMR, dPRM_dMR, dPRM_dalpha;
  SplitDerivatives(M_L, alpha, beta, dMLP_dML, dPLP_dML, dPLP_dalpha);
  SplitDerivatives(-M_R, alpha, beta, dMRM_dMR, dPRM_dMR, dPRM_dalpha);
  dPRM_dMR = -dPRM_dMR;

  /*--- Mean velocity magnitude used by pFi ---*/
  const su2double velF = sqrt(0.5 * (sq_veli + sq_velj));

  su2double dPressure_dVi[MAXNVAR_DERIV], dPressure_dVj[MAXNVAR_DERIV];

  for (int outVar = 0; outVar < 2; ++outVar) {
    su2double aF_b = 0.0, mL_b = 0.0, mR_b = 0.0, MFsq_b = 0.0, fa_b = 0.0, rhoF_b = 0.0, velF_b = 0.0, p_i_b = 0.0,
              p_j_b = 0.0;

    if (outVar == 0) {
      /*--- Convective velocity = interface_mach * aF ---*/
      const su2double mF_b = aF;
      aF_b += interface_mach;

      /*--- interface_mach = M_LP + M_RM + Mp, Eqs. (A.4b) and (A.4c) ---*/
      mL_b += dMLP_dML * mF_b;
      mR_b += dMRM_dMR * mF_b;

      /*--- Mp = -(Kp / fa) * max(1 - sigma * MFsq, 0) * (P_j - P_i) / (rhoF * aF^2), Eq. (A.4d) ---*/
      if (sigma * MFsq < 1.0) {
        const su2double tmp = (Kp / fa) / (rhoF * aF * aF) * mF_b;
        p_i_b += tmp * (1.0 - sigma * MFsq);
        p_j_b -= tmp * (1.0 - sigma * MFsq);
        MFsq_b += tmp * sigma * (Pressure_j - Pressure_i);
        fa_b -= Mp / fa * mF_b;
        rhoF_b -= Mp / rhoF * mF_b;
        aF_b -= 2.0 * Mp / aF * mF_b;
      }
    } else {
      /*--- Pressure = 0.5 * (P_i + P_j) + 0.5 * (P_LP - P_RM) * (P_i - P_j) + pFi, Eq. (A.2) ---*/
      p_i_b += 0.5 + 0.5 * (P_LP - P_RM);
      p_j_b += 0.5 - 0.5 * (P_LP - P_RM);

      /*--- pFi = velF * (P_LP + P_RM - 1) * rhoF * aF ---*/
      velF_b += (P_LP + P_RM - 1.0) * rhoF * aF;
      rhoF_b += pFi / rhoF;
      aF_b += pFi / aF;

      /*--- P_LP = f(M_L, alpha), P_RM = f(M_R, alpha) ---*/
      const su2double PLP_b = 0.5 * (Pressure_i - Pressure_j) + velF * rhoF * aF;
      const su2double PRM_b = -0.5 * (Pressure_i - Pressure_j) + velF * rhoF * aF;
      mL_b += dPLP_dML * PLP_b;
      mR_b += dPRM_dMR * PRM_b;

      /*--- alpha = 3/16 * (-4 + 5 * fa^2), Eq. (A.4g) ---*/
      fa_b += 1.875 * fa * (dPLP_dalpha * PLP_b + dPRM_dalpha * PRM_b);
    }

    /*--- Steps shared by the two outputs ---*/

    /*--- fa = 2 * sqrt(Mrefsq) - Mrefsq, with Mrefsq = min(1, max(MFsq, Minf^2)), Eqs. (A.4h) and (A.4i) ---*/
    if ((MFsq > Minf * Minf) && (MFsq < 1.0)) MFsq_b += (1.0 / sqrt(Mrefsq) - 1.0) * fa_b;

    /*--- MFsq = 0.5 * (M_L^2 + M_R^2), Eq. (A.4j) ---*/
    mL_b += M_L * MFsq_b;
    mR_b += M_R * MFsq_b;

    /*--- M_L = ProjVelocity_i / aF, M_R = ProjVelocity_j / aF ---*/
    su2double Vn_i_b = mL_b / aF, Vn_j_b = mR_b / aF;
    aF_b -= (M_L * mL_b + M_R * mR_b) / aF;

    /*--- aF = min(ChatL, ChatR), with Chat = Cstar^2 / max(Cstar, +-ProjVelocity)
     and Cstar = sqrt(2 * (Gamma - 1) / (Gamma + 1) * Enthalpy), Eqs. (A.4e) and (A.4f) ---*/
    su2double Cstar_b = aF_b, H_i_b = 0.0, H_j_b = 0.0, Gamma_i_b = 0.0, Gamma_j_b = 0.0;

    if (ChatL <= ChatR) {
      if (CstarL < ProjVelocity_i) {
        const su2double tmp = CstarL / ProjVelocity_i;
        Cstar_b = 2.0 * tmp * aF_b;
        Vn_i_b -= tmp * tmp * aF_b;
      }
      H_i_b = 0.5 * CstarL / Enthalpy_i * Cstar_b;
      Gamma_i_b = CstarL / ((Gamma_i - 1.0) * (Gamma_i + 1.0)) * Cstar_b;
    } else {
      if (CstarR < -ProjVelocity_j) {
        const su2double tmp = -CstarR / ProjVelocity_j;
        Cstar_b = 2.0 * tmp * aF_b;
        Vn_j_b += tmp * tmp * aF_b;
      }
      H_j_b = 0.5 * CstarR / Enthalpy_j * Cstar_b;
      Gamma_j_b = CstarR / ((Gamma_j - 1.0) * (Gamma_j + 1.0)) * Cstar_b;
    }

    /*--- Store the derivatives ---*/
    su2double* target_i = (outVar == 0) ? dConvVel_dVi : dPressure_dVi;
    su2double* target_j = (outVar == 0) ? dConvVel_dVj : dPressure_dVj;

    /*--- ProjVelocity = Velocity * UnitNormal, velF = sqrt(0.5 * (|Velocity_i|^2 + |Velocity_j|^2)) ---*/
    for (auto iDim = 0ul; iDim < nDim; iDim++) {
      target_i[iDim] = UnitNormal[iDim] * Vn_i_b + 0.5 * Velocity_i[iDim] / max(velF, EPS) * velF_b;
      target_j[iDim] = UnitNormal[iDim] * Vn_j_b + 0.5 * Velocity_j[iDim] / max(velF, EPS) * velF_b;
    }
    target_i[nDim] = p_i_b;
    target_j[nDim] = p_j_b;
    target_i[nDim + 1] = target_j[nDim + 1] = 0.5 * rhoF_b;
    target_i[nDim + 2] = H_i_b;
    target_j[nDim + 2] = H_j_b;
    target_i[nDim + 3] = Gamma_i_b;
    target_j[nDim + 3] = Gamma_j_b;
  }

  /*--- The pressure flux is the pressure times the normal ---*/
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    for (auto iVar = 0ul; iVar < nDim + 4ul; iVar++) {
      dPressureFlux_dVi[iDim][iVar] = dPressure_dVi[iVar] * UnitNormal[iDim];
      dPressureFlux_dVj[iDim][iVar] = dPressure_dVj[iVar] * UnitNormal[iDim];
    }
  }
}

CUpwAUSMPLUSM_NEMO::CUpwAUSMPLUSM_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar,
                                       unsigned short val_nPrimVarGrad, const CConfig* config)
    : CUpwAUSM_SLAU_Base_NEMO(val_nDim, val_nVar, val_nPrimVar, val_nPrimVarGrad, config) {
  Minf = config->GetMach();
  beta = 1.0 / 8.0;
  HasAnalyticalDerivatives = true;

  if (Minf < EPS)
    SU2_MPI::Error("AUSM+M requires a reference Mach number (\"MACH_NUMBER\") greater than 0.", CURRENT_FUNCTION);
}

void CUpwAUSMPLUSM_NEMO::ComputeInterfaceQuantities(const CConfig* config, su2double* pressure,
                                                    su2double& interface_mach, su2double* interface_soundspeed) {
  /*--- Calculate interface numerical gammas and speed of sound. The enthalpy does not
   include the kinetic energy of the tangential velocity, so that the sound speed fulfills
   the Prandtl relation across an oblique shock (Kim et al., 2001, Eq. (31)). ---*/
  const su2double sq_veli_tan = GeometryToolbox::SquaredNorm(nDim, Velocity_i) - ProjVelocity_i * ProjVelocity_i;
  const su2double sq_velj_tan = GeometryToolbox::SquaredNorm(nDim, Velocity_j) - ProjVelocity_j * ProjVelocity_j;

  const su2double Enthalpy_norm = 0.5 * (Enthalpy_i - 0.5 * sq_veli_tan + Enthalpy_j - 0.5 * sq_velj_tan);
  const su2double Gamma_ij = 0.5 * (Gamma_i + Gamma_j);
  const su2double A_s = sqrt(2.0 * Enthalpy_norm * (Gamma_ij - 1.0) / (Gamma_ij + 1.0));

  su2double A_F;
  if (0.5 * (ProjVelocity_i + ProjVelocity_j) >= 0.0)
    A_F = A_s * A_s / max(fabs(ProjVelocity_i), A_s);
  else
    A_F = A_s * A_s / max(fabs(ProjVelocity_j), A_s);

  /*--- Compute L/R Mach numbers ---*/
  interface_soundspeed[0] = interface_soundspeed[1] = A_F;
  M_L = ProjVelocity_i / interface_soundspeed[0];
  M_R = ProjVelocity_j / interface_soundspeed[1];

  /*--- Interface mach number w/ pressure diffusion term (M_p)---*/
  const su2double Density_F = 0.5 * (Density_i + Density_j);

  /*--- The reference uses the constant alpha of AUSM+ in the pressure functions.
   The scaling for low Mach numbers is done by f0 in the pressure flux, Eq. (19). ---*/
  const su2double alpha = 3.0 / 16.0;
  const su2double f = 0.5 * (1 - cos(PI_NUMBER * min(1.0, max(abs(M_L), abs(M_R)))));

  /*--- Pressure sensor terms ---*/
  const su2double h = min(Sensor_i, Sensor_j);
  const su2double g = 0.5 * (1 + cos(PI_NUMBER * h));
  const su2double f0 = min(1.0, max(f, Minf * Minf));

  /*--- Pressure diffusion term ---*/
  const su2double M_p = -0.5 * (1.0 - f) * (Pressure_j - Pressure_i) / (Density_F * A_F * A_F) * (1.0 - g);

  /*--- Compute base split mach and pressure fluxes ---*/
  su2double M_LP, M_RM, P_LP, P_RM;

  if (fabs(M_L) <= 1.0) {
    M_LP = 0.25 * (M_L + 1.0) * (M_L + 1.0) + beta * (M_L * M_L - 1.0) * (M_L * M_L - 1.0);
    P_LP = (0.25 * (M_L + 1.0) * (M_L + 1.0) * (2.0 - M_L) + alpha * M_L * (M_L * M_L - 1.0) * (M_L * M_L - 1.0));
  } else {
    M_LP = 0.5 * (M_L + fabs(M_L));
    P_LP = 0.5 * (M_L + fabs(M_L)) / M_L;
  }

  if (fabs(M_R) <= 1.0) {
    M_RM = -0.25 * (M_R - 1.0) * (M_R - 1.0) - beta * (M_R * M_R - 1.0) * (M_R * M_R - 1.0);
    P_RM = (0.25 * (M_R - 1.0) * (M_R - 1.0) * (2.0 + M_R) - alpha * M_R * (M_R * M_R - 1.0) * (M_R * M_R - 1.0));
  } else {
    M_RM = 0.5 * (M_R - fabs(M_R));
    P_RM = 0.5 * (M_R - fabs(M_R)) / M_R;
  }

  interface_mach = M_LP + M_RM + M_p;

  /*--- Compute and add pressure sensor term to pressure flux ---*/
  const su2double pFi = f0 * (P_LP + P_RM - 1.0) * 0.5 * (Pressure_i + Pressure_j);

  /*--- Velocity diffusion term, Eq. (26) of the reference: it uses the difference of
   the velocity vectors, so it also damps the difference of the transverse velocity. ---*/
  su2double P_un[MAXNDIM] = {0.0};

  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    P_un[iDim] = -g * Gamma_ij * 0.5 * (Pressure_i + Pressure_j) / A_F * P_LP * P_RM *
                 (Velocity_j[iDim] - Velocity_i[iDim]);
  }

  /*--- Pressure flux ---*/
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    pressure[iDim] =
        (0.5 * (Pressure_j + Pressure_i) + 0.5 * (P_LP - P_RM) * (Pressure_i - Pressure_j) + pFi) * UnitNormal[iDim] +
        P_un[iDim];
  }

  if (!implicit) return;

  /*--- Analytical differentiation of the convective velocity (interface_mach * A_F) and of the pressure
   flux with respect to the velocity, pressure, density, enthalpy and gamma of the two states. It is done
   in reverse mode, "x_b" denotes the derivative of the output with respect to x. For min, max and abs
   the branch that is active is differentiated. The equation numbers are the ones of the reference
   (Chen et al., 2020). The pressure sensor g, Eq. (25), is kept constant because it depends on the
   pressure of all the neighbours of the two points. ---*/

  su2double dMLP_dML, dPLP_dML, dPLP_dalpha, dMRM_dMR, dPRM_dMR, dPRM_dalpha;
  SplitDerivatives(M_L, alpha, beta, dMLP_dML, dPLP_dML, dPLP_dalpha);
  SplitDerivatives(-M_R, alpha, beta, dMRM_dMR, dPRM_dMR, dPRM_dalpha);
  dPRM_dMR = -dPRM_dMR;

  const su2double Pressure_F = 0.5 * (Pressure_i + Pressure_j);
  const su2double Mach_max = max(abs(M_L), abs(M_R));
  const bool upwind_i = (0.5 * (ProjVelocity_i + ProjVelocity_j) >= 0.0);
  const su2double ProjVelocity_upw = upwind_i ? ProjVelocity_i : ProjVelocity_j;

  /*--- The pressure flux has two parts: Pressure * UnitNormal + P_un_coeff * (Velocity_j - Velocity_i),
   Eq. (27). The second one is the velocity diffusion term, Eq. (26). ---*/
  const su2double P_un_coeff = -g * Gamma_ij * Pressure_F / A_F * P_LP * P_RM;

  su2double dPressure_dVi[MAXNVAR_DERIV], dPressure_dVj[MAXNVAR_DERIV];
  su2double dP_un_coeff_dVi[MAXNVAR_DERIV], dP_un_coeff_dVj[MAXNVAR_DERIV];
  su2double* const target_i[3] = {dConvVel_dVi, dPressure_dVi, dP_un_coeff_dVi};
  su2double* const target_j[3] = {dConvVel_dVj, dPressure_dVj, dP_un_coeff_dVj};

  for (int outVar = 0; outVar < 3; ++outVar) {
    su2double aF_b = 0.0, mL_b = 0.0, mR_b = 0.0, f_b = 0.0, rhoF_b = 0.0, p_i_b = 0.0, p_j_b = 0.0, Gamma_b = 0.0;

    if (outVar == 0) {
      /*--- Convective velocity = interface_mach * A_F ---*/
      const su2double mF_b = A_F;
      aF_b += interface_mach;

      /*--- interface_mach = M_LP + M_RM + M_p, the split functions are Eq. (6) ---*/
      mL_b += dMLP_dML * mF_b;
      mR_b += dMRM_dMR * mF_b;

      /*--- M_p = -0.5 * (1 - f) * (P_j - P_i) / (Density_F * A_F^2) * (1 - g), Eq. (14) ---*/
      const su2double tmp = 0.5 * (1.0 - g) / (Density_F * A_F * A_F) * mF_b;
      p_i_b += tmp * (1.0 - f);
      p_j_b -= tmp * (1.0 - f);
      f_b += tmp * (Pressure_j - Pressure_i);
      rhoF_b -= M_p / Density_F * mF_b;
      aF_b -= 2.0 * M_p / A_F * mF_b;
    } else if (outVar == 1) {
      /*--- Pressure = 0.5 * (P_i + P_j) + 0.5 * (P_LP - P_RM) * (P_i - P_j) + pFi,
       with pFi = f0 * (P_LP + P_RM - 1) * 0.5 * (P_i + P_j), Eq. (19) ---*/
      p_i_b += 0.5 + 0.5 * (P_LP - P_RM) + 0.5 * f0 * (P_LP + P_RM - 1.0);
      p_j_b += 0.5 - 0.5 * (P_LP - P_RM) + 0.5 * f0 * (P_LP + P_RM - 1.0);
      mL_b += dPLP_dML * (0.5 * (Pressure_i - Pressure_j) + f0 * Pressure_F);
      mR_b += dPRM_dMR * (-0.5 * (Pressure_i - Pressure_j) + f0 * Pressure_F);

      /*--- f0 = min(1, max(f, Minf^2)), Eq. (20) ---*/
      if ((f > Minf * Minf) && (f < 1.0)) f_b += (P_LP + P_RM - 1.0) * Pressure_F;
    } else {
      /*--- P_un_coeff = -g * Gamma_ij * 0.5 * (P_i + P_j) / A_F * P_LP * P_RM, Eq. (26) ---*/
      const su2double tmp = -g * Gamma_ij * Pressure_F / A_F;
      p_i_b += 0.5 * P_un_coeff / Pressure_F;
      p_j_b += 0.5 * P_un_coeff / Pressure_F;
      mL_b += dPLP_dML * tmp * P_RM;
      mR_b += dPRM_dMR * tmp * P_LP;
      Gamma_b += P_un_coeff / Gamma_ij;
      aF_b -= P_un_coeff / A_F;
    }

    /*--- Steps shared by the three outputs ---*/

    /*--- f = 0.5 * (1 - cos(pi * min(1, Mach_max))), with Mach_max = max(|M_L|, |M_R|), Eq. (15) ---*/
    if (Mach_max < 1.0) {
      const su2double Mach_max_b = 0.5 * PI_NUMBER * sin(PI_NUMBER * Mach_max) * f_b;
      if (abs(M_L) >= abs(M_R))
        mL_b += ((M_L >= 0.0) ? 1.0 : -1.0) * Mach_max_b;
      else
        mR_b += ((M_R >= 0.0) ? 1.0 : -1.0) * Mach_max_b;
    }

    /*--- M_L = ProjVelocity_i / A_F, M_R = ProjVelocity_j / A_F ---*/
    su2double Vn_i_b = mL_b / A_F, Vn_j_b = mR_b / A_F;
    aF_b -= (M_L * mL_b + M_R * mR_b) / A_F;

    /*--- A_F = A_s^2 / max(|ProjVelocity_upw|, A_s), Eq. (28) ---*/
    su2double As_b = aF_b;
    if (fabs(ProjVelocity_upw) > A_s) {
      const su2double tmp = A_s / fabs(ProjVelocity_upw);
      As_b = 2.0 * tmp * aF_b;
      const su2double Vn_upw_b = ((ProjVelocity_upw >= 0.0) ? -1.0 : 1.0) * tmp * tmp * aF_b;
      if (upwind_i)
        Vn_i_b += Vn_upw_b;
      else
        Vn_j_b += Vn_upw_b;
    }

    /*--- A_s = sqrt(2 * Enthalpy_norm * (Gamma_ij - 1) / (Gamma_ij + 1)), Eq. (29) ---*/
    const su2double Hnorm_b = 0.5 * A_s / Enthalpy_norm * As_b;
    Gamma_b += A_s / ((Gamma_ij - 1.0) * (Gamma_ij + 1.0)) * As_b;

    /*--- Store the derivatives. ProjVelocity = Velocity * UnitNormal, and Enthalpy_norm is the mean of
     Enthalpy - 0.5 * |tangential velocity|^2 of the two states. ---*/
    for (auto iDim = 0ul; iDim < nDim; iDim++) {
      target_i[outVar][iDim] =
          UnitNormal[iDim] * Vn_i_b - 0.5 * (Velocity_i[iDim] - ProjVelocity_i * UnitNormal[iDim]) * Hnorm_b;
      target_j[outVar][iDim] =
          UnitNormal[iDim] * Vn_j_b - 0.5 * (Velocity_j[iDim] - ProjVelocity_j * UnitNormal[iDim]) * Hnorm_b;
    }
    target_i[outVar][nDim] = p_i_b;
    target_j[outVar][nDim] = p_j_b;
    target_i[outVar][nDim + 1] = target_j[outVar][nDim + 1] = 0.5 * rhoF_b;
    target_i[outVar][nDim + 2] = target_j[outVar][nDim + 2] = 0.5 * Hnorm_b;
    target_i[outVar][nDim + 3] = target_j[outVar][nDim + 3] = 0.5 * Gamma_b;
  }

  /*--- Pressure flux = Pressure * UnitNormal + P_un_coeff * (Velocity_j - Velocity_i) ---*/
  for (auto iDim = 0ul; iDim < nDim; iDim++) {
    for (auto iVar = 0ul; iVar < nDim + 4ul; iVar++) {
      dPressureFlux_dVi[iDim][iVar] =
          dPressure_dVi[iVar] * UnitNormal[iDim] + dP_un_coeff_dVi[iVar] * (Velocity_j[iDim] - Velocity_i[iDim]);
      dPressureFlux_dVj[iDim][iVar] =
          dPressure_dVj[iVar] * UnitNormal[iDim] + dP_un_coeff_dVj[iVar] * (Velocity_j[iDim] - Velocity_i[iDim]);
    }
    dPressureFlux_dVi[iDim][iDim] -= P_un_coeff;
    dPressureFlux_dVj[iDim][iDim] += P_un_coeff;
  }
}
