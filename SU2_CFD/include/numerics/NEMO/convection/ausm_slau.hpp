/*!
 * \file ausm_slau.hpp
 * \brief Declaration of numerics classes for the AUSM and SLAU family of schemes in NEMO.
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

#pragma once

#include "../CNEMONumerics.hpp"

/*!
 * \class CUpwAUSM_SLAU_Base_NEMO
 * \brief Class for solving an approximate Riemann AUSM.
 * \ingroup ConvDiscr
 * \author F. Palacios, S.R. Copeland, W. Maier, C. Garbacz
 */
class CUpwAUSM_SLAU_Base_NEMO : public CNEMONumerics {
 protected:
  su2double A_F[2] = {0.0}, PressureFlux[MAXNDIM] = {0.0};
  su2double M_L, M_R, M_F;

  su2double* Fc_L = nullptr;
  su2double* Fc_R = nullptr;
  su2double* Fc_LR = nullptr;
  su2double* dM_LP = nullptr;
  su2double* dM_RM = nullptr;
  su2double* dP_LP = nullptr;
  su2double* dP_RM = nullptr;
  su2double* da_L = nullptr;
  su2double* da_R = nullptr;

  su2double* Flux = nullptr;        /*!< \brief The flux accross the face. */
  su2double** Jacobian_i = nullptr; /*!< \brief The Jacobian w.r.t. point i after computation. */
  su2double** Jacobian_j = nullptr; /*!< \brief The Jacobian w.r.t. point j after computation. */

  /*--- Derivatives of the interface quantities with respect to the variables [u, v, w, P, rho, H, gamma] of
   the states i and j (without w in 2D). A scheme that sets HasAnalyticalDerivatives must compute them in
   ComputeInterfaceQuantities when the time integration is implicit. ---*/
  static constexpr size_t MAXNVAR_DERIV = MAXNDIM + 4; /*!< \brief Max number of those variables. */
  bool HasAnalyticalDerivatives = false;               /*!< \brief The scheme provides the derivatives below. */
  su2double dConvVel_dVi[MAXNVAR_DERIV] = {0.0};       /*!< \brief Of the convective velocity M_F*A_F, state i. */
  su2double dConvVel_dVj[MAXNVAR_DERIV] = {0.0};       /*!< \brief Of the convective velocity M_F*A_F, state j. */
  su2double dPressureFlux_dVi[MAXNDIM][MAXNVAR_DERIV] = {{0.0}}; /*!< \brief Of the pressure flux, state i. */
  su2double dPressureFlux_dVj[MAXNDIM][MAXNVAR_DERIV] = {{0.0}}; /*!< \brief Of the pressure flux, state j. */

  /*!
   * \brief Compute the interface Mach number, soundspeeds and pressure based on Primitives_i/j..
   * \param[in] config - Definition of the particular problem.
   * \param[out] pressure - The pressure at the control volume face.
   * \param[out] interface_mach - The interface Mach number M_(1/2).
   * \param[out] interface_soundspeed - The interface soundspeed (vector for i and j faces if necessary).
   */
  virtual void ComputeInterfaceQuantities(const CConfig* config, su2double* pressure, su2double& interface_mach,
                                          su2double* interface_soundspeed) = 0;

  /*!
   * \brief Derivatives of the split Mach number (M+) and of the split pressure function (P+) of AUSM+. For the
   *        functions of the right state use the symmetry: M-(M) = -M+(-M) and P-(M) = P+(-M).
   * \param[in] mach - Mach number of the state.
   * \param[in] alpha - Parameter of the split pressure function.
   * \param[in] beta - Parameter of the split Mach number.
   * \param[out] dMP_dM - Derivative of M+ with respect to the Mach number.
   * \param[out] dPP_dM - Derivative of P+ with respect to the Mach number.
   * \param[out] dPP_dalpha - Derivative of P+ with respect to alpha.
   */
  static void SplitDerivatives(su2double mach, su2double alpha, su2double beta, su2double& dMP_dM, su2double& dPP_dM,
                               su2double& dPP_dalpha);

 private:
  /*!
   * \brief Compute the flux Jacobians of the AUSM scheme to use as an approximation.
   * \param[out] val_Jacobian_i - Jacobian of the numerical method at node i (implicit computation).
   * \param[out] val_Jacobian_j - Jacobian of the numerical method at node j (implicit computation).
   */
  void ApproximateJacobian(su2double** val_Jacobian_i, su2double** val_Jacobian_j);

  /*!
   * \brief Compute the flux Jacobians of the scheme, from the derivatives of its interface quantities.
   * \param[out] val_Jacobian_i - Jacobian of the numerical method at node i (implicit computation).
   * \param[out] val_Jacobian_j - Jacobian of the numerical method at node j (implicit computation).
   */
  void AccurateJacobian(su2double** val_Jacobian_i, su2double** val_Jacobian_j);

 public:
  /*!
   * \brief Constructor of the class.
   * \param[in] val_nDim - Number of dimension of the problem.
   * \param[in] val_nVar - Number of variables of the problem.
   * \param[in] val_nPrimVar - Number of primitive variables of the problem.
   * \param[in] val_nPrimVarGrad - Number of primitive gradient variables of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  CUpwAUSM_SLAU_Base_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar,
                          unsigned short val_nPrimVarGrad, const CConfig* config);

  /*!
   * \brief Destructor of the class.
   */
  ~CUpwAUSM_SLAU_Base_NEMO(void) override;

  /*!
   * \brief Compute the AUSM and SLAU family of schemes.
   * \param[in] config - Definition of the particular problem.
   * \return A lightweight const-view (read-only) of the residual/flux and Jacobians.
   */
  ResidualType<> ComputeResidual(const CConfig* config) final;
};

/*!
 * \class CUpwAUSM_NEMO
 * \brief Class for solving an approximate Riemann AUSM.
 * \ingroup ConvDiscr
 * \author F. Palacios, S.R. Copeland, W. Maier, C. Garbacz
 */
class CUpwAUSM_NEMO final : public CUpwAUSM_SLAU_Base_NEMO {
 private:
  /*!
   * \brief Compute the interface Mach number, soundspeeds and pressure for AUSM scheme.
   * \param[in] config - Definition of the particular problem.
   * \param[out] pressure - The pressure at the control volume face.
   * \param[out] interface_mach - The interface Mach number M_(1/2).
   * \param[out] interface_soundspeed - The interface soundspeed (vector for i and j faces if necessary).
   */
  virtual void ComputeInterfaceQuantities(const CConfig* config, su2double* pressure, su2double& interface_mach,
                                          su2double* interface_soundspeed) override;

 public:
  /*!
   * \brief Constructor of the class.
   * \param[in] val_nDim - Number of dimensions of the problem.
   * \param[in] val_nVar - Number of variables of the problem.
   * \param[in] val_nPrimVar - Number of primitive variables of the problem
   * \param[in] val_nPrimVarGrad - Number of grad primitive variables of the problem
   * \param[in] config - Definition of the particular problem.
   */
  CUpwAUSM_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar,
                unsigned short val_nPrimVarGrad, const CConfig* config);
};

/*!
 * \class CUpwAUSMPLUSM_NEMO
 * \brief Class for solving an approximate Riemann AUSM+ M, Two-Temperature Model.
 * https://doi.org/10.1016/j.apm.2019.09.005 \ingroup ConvDiscr \author F. Morgado
 */
class CUpwAUSMPLUSM_NEMO final : public CUpwAUSM_SLAU_Base_NEMO {
 private:
  su2double beta;

  /*!
   * \brief Compute the interface Mach number, soundspeeds and pressure for AUSM+M scheme.
   * \param[in] config - Definition of the particular problem.
   * \param[out] pressure - The pressure at the control volume face.
   * \param[out] interface_mach - The interface Mach number M_(1/2).
   * \param[out] interface_soundspeed - The interface soundspeed (vector for i and j faces if necessary).
   */
  virtual void ComputeInterfaceQuantities(const CConfig* config, su2double* pressure, su2double& interface_mach,
                                          su2double* interface_soundspeed) override;

 public:
  /*!
   * \brief Constructor of the class.
   * \param[in] val_nDim - Number of dimension of the problem.
   * \param[in] val_nVar - Number of variables of the problem.
   * \param[in] val_nPrimVar - Number of primitive variables of the problem.
   * \param[in] val_nPrimVarGrad - Number of primitive gradient variables of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  CUpwAUSMPLUSM_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar,
                     unsigned short val_nPrimVarGrad, const CConfig* config);
};

/*!
 * \class CUpwAUSMPLUSUP2_NEMO
 * \brief Class for solving an approximate Riemann AUSM+-up2, Two-Temperature Model.
 * https://doi.org/10.1016/j.jcp.2013.02.046 \ingroup ConvDiscr \author W. Maier, A. Sachedeva, C. Garbacz
 */
class CUpwAUSMPLUSUP2_NEMO final : public CUpwAUSM_SLAU_Base_NEMO {
 private:
  su2double Kp, Ku, sigma;

  /*!
   * \brief Compute the interface Mach number, soundspeeds and pressure for AUSM+-Up2 scheme.
   * \param[in] config - Definition of the particular problem.
   * \param[out] pressure - The pressure at the control volume face.
   * \param[out] interface_mach - The interface Mach number M_(1/2).
   * \param[out] interface_soundspeed - The interface soundspeed (vector for i and j faces if necessary).
   */
  virtual void ComputeInterfaceQuantities(const CConfig* config, su2double* pressure, su2double& interface_mach,
                                          su2double* interface_soundspeed) override;

 public:
  /*!
   * \brief Constructor of the class.
   * \param[in] val_nDim - Number of dimensions of the problem.
   * \param[in] val_nVar - Number of variables of the problem.
   * \param[in] val_nPrimVar - Number of primitive variables of the problem
   * \param[in] val_nPrimVarGrad - Number of grad primitive variables of the problem
   * \param[in] config - Definition of the particular problem.
   */
  CUpwAUSMPLUSUP2_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar,
                       unsigned short val_nPrimVarGrad, const CConfig* config);
};

/*!
 * \class CUpwAUSMPWplus_NEMO
 * \brief Class for solving an approximate Riemann AUSM.
 * \ingroup ConvDiscr
 * \author F. Palacios, W.Maier, C. Garbacz
 */
class CUpwAUSMPWplus_NEMO : public CUpwAUSM_SLAU_Base_NEMO {
 private:
  su2double alpha;

  /*!
   * \brief Compute the interface Mach number, soundspeeds and pressure for AUSMpw+ scheme.
   * \param[in] config - Definition of the particular problem.
   * \param[out] pressure - The pressure at the control volume face.
   * \param[out] interface_mach - The interface Mach number M_(1/2).
   * \param[out] interface_soundspeed - The interface soundspeed (vector for i and j faces if necessary).
   */
  virtual void ComputeInterfaceQuantities(const CConfig* config, su2double* pressure, su2double& interface_mach,
                                          su2double* interface_soundspeed) override;

 public:
  /*!
   * \brief Constructor of the class.
   * \param[in] val_nDim - Number of dimensions of the problem.
   * \param[in] val_nVar - Number of variables of the problem.
   * \param[in] val_nPrimVar - Number of primitive variables of the problem
   * \param[in] val_nPrimVarGrad - Number of grad primitive variables of the problem
   * \param[in] config - Definition of the particular problem.
   */
  CUpwAUSMPWplus_NEMO(unsigned short val_nDim, unsigned short val_nVar, unsigned short val_nPrimVar,
                      unsigned short val_nPrimVarGrad, const CConfig* config);
};
