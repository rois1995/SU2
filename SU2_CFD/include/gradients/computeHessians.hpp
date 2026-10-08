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
#include "../../../Common/include/linear_algebra/blas_structure.hpp"

namespace detail {
/*!
 * \brief Reciprocal condition of the coordinate-equilibrated LS normal matrix.
 * \note Passive diagnostic only: the solve and its derivatives are unchanged. Zero means a missing,
 *       non-finite or numerically singular direction. This is the condition of A^T A, not of A.
 */
template <class RMatrixType>
passivedouble hessianStencilReciprocalCondition(unsigned short nDim, unsigned long point, const RMatrixType& R) {
  passivedouble scale[3], normal[3][3] = {}, vec[3][3], val[3], work[3];
  for (unsigned short i = 0; i < nDim; ++i) {
    const auto diagonal = SU2_TYPE::GetValue(R(point, i, i));
    if (!(diagonal > 0.0) || !std::isfinite(diagonal)) return 0.0;
    scale[i] = sqrt(diagonal);
  }
  for (unsigned short i = 0; i < nDim; ++i) {
    for (unsigned short j = i; j < nDim; ++j) {
      normal[i][j] = normal[j][i] = SU2_TYPE::GetValue(R(point, i, j)) / scale[i] / scale[j];
      if (!std::isfinite(normal[i][j])) return 0.0;
    }
  }
  CBlasStructure::EigenDecomposition(normal, vec, val, nDim, work);
  const auto largest = *std::max_element(val, val + nDim), smallest = *std::min_element(val, val + nDim);
  if (!(largest > 0.0) || !std::isfinite(largest) || !std::isfinite(smallest)) return 0.0;
  return std::max(passivedouble(0.0), smallest / largest);
}

/*! \brief Reuse the preceding WLS normal matrices; share neighbor weights across all sensors.
 *  \note Same geometry and weighting only. Periodic, symmetry and AD use the existing recovery path.
 */
template<size_t dim, class GradientType, class HessianType>
void hessiansReuseGeometry(CGeometry& geometry, const GradientType& gradient, size_t begin, size_t end,
                           const C3DDoubleMatrix& R, HessianType& hessian) {
  std::vector<su2uint> orderedNeighbors;
  for (size_t point = 0; point < geometry.GetnPointDomain(); ++point) {
    su2double inverse[dim][dim];
    leastSquaresInverse<dim>(point, R, inverse);
    for (size_t v = begin; v < end; ++v)
      for (size_t k = 0; k < dim * (dim + 1) / 2; ++k) hessian(point, v, k) = 0.0;
    orderAdaptationNeighbors(geometry, point, orderedNeighbors);
    for (const auto neighbor : orderedNeighbors) {
      su2double dx[dim], coefficient[dim] = {};
      GeometryToolbox::Distance(dim, geometry.nodes->GetCoord(neighbor), geometry.nodes->GetCoord(point), dx);
      const auto squared = GeometryToolbox::SquaredNorm(dim, dx);
      if (!(squared > 0.0)) continue;
      for (size_t i = 0; i < dim; ++i)
        for (size_t j = 0; j < dim; ++j)
          coefficient[i] += inverse[std::min(i,j)][std::max(i,j)] * dx[j] / squared;
      for (size_t v = begin; v < end; ++v) {
        su2double delta[dim];
        for (size_t i = 0; i < dim; ++i) delta[i] = gradient(neighbor, v, i) - gradient(point, v, i);
        size_t k = 0;
        for (size_t i = 0; i < dim; ++i)
          for (size_t j = i; j < dim; ++j)
            hessian(point, v, k++) += 0.5 * (coefficient[j] * delta[i] + coefficient[i] * delta[j]);
      }
    }
  }
}
}  // namespace detail

/*!
 * \brief Apply the existing symmetry rule to packed Hessians of mirror-even scalars.
 * \note Remove normal-tangential components, retaining normal-normal curvature. Reuse the caller's
 *       gradient workspace and the same modified/original normals as the gradient kernels.
 *       Euler walls are excluded: a slip wall does not imply an even scalar extension.
 */
template <size_t dim, class HessianType>
void correctHessiansSymmetry(CGeometry& geometry, const CConfig& config, size_t begin, size_t end,
                             C3DDoubleMatrix& work, HessianType& hessian) {
  if (config.GetnMarker_SymWall() == 0) return;
  std::vector<unsigned short> markers;
  for (auto marker = 0u; marker < geometry.GetnMarker(); ++marker)
    if (config.GetMarker_All_KindBC(marker) == SYMMETRY_PLANE) markers.push_back(marker);
  for (size_t v = begin; v < end; ++v) {
    for (const auto marker : markers)
      for (auto vertex = 0ul; vertex < geometry.GetnVertex(marker); ++vertex) {
        const auto point = geometry.vertex[marker][vertex]->GetNode();
        size_t k = 0;
        for (size_t i = 0; i < dim; ++i)
          for (size_t j = i; j < dim; ++j)
            work(point, i, j) = work(point, j, i) = hessian(point, v, k++);
      }
    // The gradient of a scalar is a vector: its gradient follows the velocity symmetry rule.
    correctGradientsSymmetry<dim>(geometry, config, 0, dim, 0, work, false);
    for (const auto marker : markers)
      for (auto vertex = 0ul; vertex < geometry.GetnVertex(marker); ++vertex) {
        const auto point = geometry.vertex[marker][vertex]->GetNode();
        size_t k = 0;
        for (size_t i = 0; i < dim; ++i)
          for (size_t j = i; j < dim; ++j)
            hessian(point, v, k++) = 0.5 * (work(point, i, j) + work(point, j, i));
      }
  }
}

/*!
 * \brief Compute Hessians by differentiating the gradients, then symmetrizing the result.
 * \ingroup FvmAlgos
 * \note The gradients must be known on halo points. Hessians are computed on domain points
 *       only, the caller must communicate them.
 * \note On symmetry planes, the gradient of each variable is corrected like a velocity, i.e. the
 *       normal-tangential components of the Hessian are removed. This is exact for a field symmetric
 *       about the plane. Euler walls are not corrected, n.grad(phi) = 0 does not hold on curved walls
 *       (e.g. dp/dn = rho v^2 / R) and imposing it creates large spurious normal second derivatives.
 * \note With a solver and periodic markers, periodic contributions are included (PERIODIC_HESS_GG or
 *       PERIODIC_HESS_LS), the gradient of each variable is rotated like a vector. The work arrays must
 *       then be those of the solver (CVariable::GetHessian_Field, GetHessian_Grad and GetRmatrix).
 *       The kernels also exchange the halo Hessians (MPI_QUANTITIES::HESSIAN) after each variable.
 * \note Not thread-safe, call outside of OpenMP parallel regions.
 * \param[in] solver - Optional, solver used only for periodic communications.
 * \param[in] method - GREEN_GAUSS, LEAST_SQUARES (not periodic) or WEIGHTED_LEAST_SQUARES.
 * \param[in] geometry - Geometric grid properties.
 * \param[in] config - Configuration of the problem, used to identify types of boundaries.
 * \param[in] gradient - Generic object implementing operator (iPoint, iVar, iDim).
 * \param[in] varBegin - Index of first variable for which to compute the Hessian.
 * \param[in] varEnd - Index of last variable for which to compute the Hessian.
 * \param[out] field - Work array (nPoint, nDim), the gradient of one variable.
 * \param[out] gradGrad - Work array (nPoint, nDim, nDim), the gradient of field.
 * \param[out] Rmatrix - Work array (nPoint, nDim, nDim) for least squares.
 * \param[out] hessian - Generic object implementing operator (iPoint, iVar, iMet), with the upper
 *             triangle stored row-wise: (xx, xy, yy) in 2D, (xx, xy, xz, yy, yz, zz) in 3D.
 */
template <class GradientType, class FieldType, class HessianType>
void computeHessians(CSolver* solver, ENUM_FLOW_GRADIENT method, CGeometry& geometry, const CConfig& config,
                     const GradientType& gradient, const size_t varBegin, const size_t varEnd, FieldType& field,
                     C3DDoubleMatrix& gradGrad, C3DDoubleMatrix& Rmatrix, HessianType& hessian, bool reuseWlsGeometry = false) {
  const size_t nDim = geometry.GetnDim();
  const size_t nPoint = geometry.GetnPoint();
  const size_t nPointDomain = geometry.GetnPointDomain();

  /*--- The solver is only used for periodic contributions. ---*/

  if (config.GetnMarker_Periodic() == 0) solver = nullptr;
  if (solver != nullptr && method == LEAST_SQUARES) {
    SU2_MPI::Error("Periodic Hessians require GREEN_GAUSS or WEIGHTED_LEAST_SQUARES.", CURRENT_FUNCTION);
  }

#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
  if (reuseWlsGeometry && method == WEIGHTED_LEAST_SQUARES && config.GetnMarker_Periodic() == 0 &&
      config.GetnMarker_SymWall() == 0) {
    if (nDim == 2) detail::hessiansReuseGeometry<2>(geometry, gradient, varBegin, varEnd, Rmatrix, hessian);
    else detail::hessiansReuseGeometry<3>(geometry, gradient, varBegin, varEnd, Rmatrix, hessian);
    return;
  }
#endif

  for (size_t iVar = varBegin; iVar < varEnd; ++iVar) {
    /*--- Gradient of this variable, including halo points. ---*/

    for (size_t iPoint = 0; iPoint < nPoint; ++iPoint)
      for (size_t iDim = 0; iDim < nDim; ++iDim) field(iPoint, iDim) = gradient(iPoint, iVar, iDim);

    /*--- The gradient is a vector, it is corrected on symmetry planes like the velocity (idxVel = 0). ---*/

    switch (method) {
      case GREEN_GAUSS:
        computeGradientsGreenGauss(solver, MPI_QUANTITIES::HESSIAN, PERIODIC_HESS_GG, geometry, config, field, 0,
                                   nDim, 0, gradGrad, false);
        break;
      case LEAST_SQUARES:
      case WEIGHTED_LEAST_SQUARES:
        computeGradientsLeastSquares(solver, MPI_QUANTITIES::HESSIAN, PERIODIC_HESS_LS, geometry, config,
                                     method == WEIGHTED_LEAST_SQUARES, field, 0, nDim, 0, gradGrad, Rmatrix, false);
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
