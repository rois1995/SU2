/*!
 * \file computeHessians.hpp
 * \brief Generic computation of Hessians as gradients of gradients (used for mesh adaptation).
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

#include "computeGradientsGreenGauss.hpp"
#include "computeGradientsLeastSquares.hpp"

namespace detail {
/*!
 * \brief Present the gradient of one variable as a field with nDim variables.
 */
template <class GradientType>
struct GradientComponents {
  const GradientType& gradient;
  const size_t iVar;

  decltype(auto) operator()(size_t iPoint, size_t iDim) const { return gradient(iPoint, iVar, iDim); }
};
}  // namespace detail

/*!
 * \brief Compute Hessians by differentiating the gradients, then symmetrizing the result.
 * \ingroup FvmAlgos
 * \note The gradients must be known on halo points. Hessians are computed on domain points
 *       only, no communication is performed.
 * \note On symmetry planes and Euler walls, the gradient of each variable is corrected
 *       like a velocity, i.e. the normal-tangential components of the Hessian are removed.
 * \note Not thread-safe, call outside of OpenMP parallel regions.
 * \param[in] method - GREEN_GAUSS, LEAST_SQUARES or WEIGHTED_LEAST_SQUARES.
 * \param[in] geometry - Geometric grid properties.
 * \param[in] config - Configuration of the problem, used to identify types of boundaries.
 * \param[in] gradient - Generic object implementing operator (iPoint, iVar, iDim).
 * \param[in] varBegin - Index of first variable for which to compute the Hessian.
 * \param[in] varEnd - Index of last variable for which to compute the Hessian.
 * \param[out] hessian - Generic object implementing operator (iPoint, iVar, iMet), with the upper
 *             triangle stored row-wise: (xx, xy, yy) in 2D, (xx, xy, xz, yy, yz, zz) in 3D.
 */
template <class GradientType, class HessianType>
void computeHessians(ENUM_FLOW_GRADIENT method, CGeometry& geometry, const CConfig& config,
                     const GradientType& gradient, const size_t varBegin, const size_t varEnd,
                     HessianType& hessian) {
  const size_t nDim = geometry.GetnDim();
  const size_t nPointDomain = geometry.GetnPointDomain();

  /*--- Gradient of the gradient of one variable. ---*/
  C3DDoubleMatrix gradGrad(geometry.GetnPoint(), nDim, nDim);
  C3DDoubleMatrix Rmatrix;
  if (method != GREEN_GAUSS) Rmatrix.resize(geometry.GetnPoint(), nDim, nDim);

  for (size_t iVar = varBegin; iVar < varEnd; ++iVar) {
    const detail::GradientComponents<GradientType> field{gradient, iVar};

    /*--- The gradient is a vector, it is corrected on symmetries like the velocity (idxVel = 0). ---*/

    switch (method) {
      case GREEN_GAUSS:
        computeGradientsGreenGauss(nullptr, MPI_QUANTITIES::HESSIAN, PERIODIC_NONE, geometry, config, field, 0, nDim,
                                   0, gradGrad);
        break;
      case LEAST_SQUARES:
      case WEIGHTED_LEAST_SQUARES:
        computeGradientsLeastSquares(nullptr, MPI_QUANTITIES::HESSIAN, PERIODIC_NONE, geometry, config,
                                     method == WEIGHTED_LEAST_SQUARES, field, 0, nDim, 0, gradGrad, Rmatrix);
        break;
      default:
        SU2_MPI::Error("Unsupported method for Hessian computation.", CURRENT_FUNCTION);
        break;
    }

    for (size_t iPoint = 0; iPoint < nPointDomain; ++iPoint) {
      size_t iMet = 0;
      for (size_t iDim = 0; iDim < nDim; ++iDim) {
        for (size_t jDim = iDim; jDim < nDim; ++jDim, ++iMet) {
          hessian(iPoint, iVar, iMet) = 0.5 * (gradGrad(iPoint, iDim, jDim) + gradGrad(iPoint, jDim, iDim));
        }
      }
    }
  }
}
