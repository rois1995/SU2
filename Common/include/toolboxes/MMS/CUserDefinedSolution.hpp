/*!
 * \file CUserDefinedSolution.hpp
 * \brief Header file for the class CUserDefinedSolution.
 *        The implementations are in the <i>CUserDefinedSolution.cpp</i> file.
 * \author T. Economon, E. van der Weide
 * \version 8.1.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2024, SU2 Contributors (cf. AUTHORS.md)
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
#include "CVerificationSolution.hpp"

/*!
 * \class CUserDefinedSolution
 * \brief Class to define the required data for a user defined solution.
 * \author E. van der Weide, T. Economon
 */
class CUserDefinedSolution final : public CVerificationSolution {
  private:
  su2double gamma, Pr, C1, S, R;

  //  su2double r0,r1,r2,r3,r4,
  //            u0,u1,u2,u3,u4,
  //            v0,v1,v2,v3,v4,
  //            w0,w1,w2,w3,w4,
  //            p0,p1,p2,p3,p4;
  
    su2double r0,rx,ry,rz,rxyz,
              u0,ux,uy,uz,uxyz,
              v0,vx,vy,vz,vxyz,
              w0,wx,wy,wz,wxyz,
              p0,px,py,pz,pxyz;
  
    su2double arx,ary,arz,arxyz,
              aux,auy,auz,auxyz,
              avx,avy,avz,avxyz,
              awx,awy,awz,awxyz,
              apx,apy,apz,apxyz;
  
 public:
  /*!
   * \brief Constructor of the class.
   */
  CUserDefinedSolution(void);

  /*!
   * \overload
   * \param[in] val_nDim  - Number of dimensions of the problem.
   * \param[in] val_nvar  - Number of variables of the problem.
   * \param[in] val_iMesh - Multigrid level of the solver.
   * \param[in] config    - Configuration of the particular problem.
   */
  CUserDefinedSolution(unsigned short val_nDim, unsigned short val_nvar, unsigned short val_iMesh, CConfig* config);

  /*!
   * \brief Destructor of the class.
   */
  ~CUserDefinedSolution(void) override;

  /*!
   * \brief Get the exact solution at the current position and time.
   * \param[in] val_coords   - Cartesian coordinates of the current position.
   * \param[in] val_t        - Current physical time.
   * \param[in] val_solution - Array where the exact solution is stored.
   */
  void GetSolution(const su2double* val_coords, const su2double val_t, su2double* val_solution) const override;

  /*!
   * \brief Get the boundary conditions state for an exact solution.
   * \param[in] val_coords   - Cartesian coordinates of the current position.
   * \param[in] val_t        - Current physical time.
   * \param[in] val_solution - Array where the exact solution is stored.
   */
  void GetBCState(const su2double* val_coords, const su2double val_t, su2double* val_solution) const override;

  /*!
   * \brief Get the source term for the manufactured solution (MMS).
   * \param[in] val_coords   - Cartesian coordinates of the current position.
   * \param[in] val_t        - Current physical time.
   * \param[in] val_solution - Array where the exact solution is stored.
   */
  void GetMMSSourceTerm(const su2double* val_coords, const su2double val_t, su2double* val_source) const override;

  /*!
   * \brief Whether or not this verification solution is a manufactured solution.
   * \return  - True if this is a manufactured solution and false otherwise.
   */
  bool IsManufacturedSolution(void) const override;
};
