/*!
 * \file computeGradientsGreenGauss.hpp
 * \brief Generic implementation of Green-Gauss gradient computation.
 * \note This allows the same implementation to be used for conservative
 *       and primitive variables of any solver.
 * \author P. Gomes
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

#include <vector>
#include <algorithm>
#include <limits>

#include "../../../Common/include/parallelization/omp_structure.hpp"
#include "../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "correctGradientsSymmetry.hpp"

namespace detail {

/*!
 * \brief Recover gradients by a volume-weighted P1 simplex projection (adaptation opt-in).
 * \note Adaptation opt-in only, before symmetry and halo exchange. Complete owned incident stars are
 *       available after geometry partitioning. Preserve GG for periodic, mixed or degenerate stars.
 *       This restores affine consistency, not quadratic boundary exactness (Clément has truncation bias).
 *       One small reusable star buffer; no full-mesh geometry cache or additional MPI exchange.
 *       ponytail: Serial star traversal; use per-thread buffers if hybrid scaling becomes limiting.
 */
template <size_t dim, class FieldType, class GradientType>
void correctGradientsSimplex(CGeometry& geometry, const FieldType& field, size_t begin, size_t end,
                             GradientType& gradient) {
  constexpr size_t maxVariables = 20;
  if (end - begin > maxVariables) SU2_MPI::Error("Too many variables for GG boundary recovery.", CURRENT_FUNCTION);
  std::vector<unsigned long> star;
  for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) {
    star.assign(geometry.nodes->GetElems(point).begin(), geometry.nodes->GetElems(point).end());
    std::sort(star.begin(), star.end(), [&](auto a, auto b) {
      return geometry.elem[a]->GetGlobalIndex() < geometry.elem[b]->GetGlobalIndex();
    });
    su2double sum[maxVariables][dim] = {}, volume = 0;
    bool valid = !star.empty();
    for (const auto element : star) {
      const auto* cell = geometry.elem[element];
      if (cell->GetVTK_Type() != (dim == 2 ? TRIANGLE : TETRAHEDRON)) { valid = false; break; }
      su2double edge[dim][dim], cofactor[dim][dim], determinant;
      passivedouble scale[dim] = {}, product = 1;
      for (size_t j = 0; j < dim; ++j)
        for (size_t i = 0; i < dim; ++i) {
          edge[j][i] = geometry.nodes->GetCoord(cell->GetNode(j + 1), i) -
                       geometry.nodes->GetCoord(cell->GetNode(0), i);
          scale[i] = std::max(scale[i], fabs(SU2_TYPE::GetValue(edge[j][i])));
        }
      for (size_t i = 0; i < dim; ++i) {
        if (!(scale[i] > 0) || !std::isfinite(scale[i])) { valid = false; break; }
        product *= scale[i];
        for (size_t j = 0; j < dim; ++j) edge[j][i] /= scale[i];
      }
      if (!valid) break;
      if constexpr (dim == 2) {
        cofactor[0][0] = edge[1][1]; cofactor[0][1] = -edge[0][1];
        cofactor[1][0] = -edge[1][0]; cofactor[1][1] = edge[0][0];
        determinant = edge[0][0] * edge[1][1] - edge[0][1] * edge[1][0];
      } else {
        su2double cross[3];
        for (size_t j = 0; j < dim; ++j) {
          GeometryToolbox::CrossProduct(edge[(j + 1) % dim], edge[(j + 2) % dim], cross);
          for (size_t i = 0; i < dim; ++i) cofactor[i][j] = cross[i];
        }
        determinant = 0;
        for (size_t i = 0; i < dim; ++i) determinant += edge[0][i] * cofactor[i][0];
      }
      const auto det = SU2_TYPE::GetValue(determinant);
      if (!std::isfinite(det) || fabs(det) <= 64 * std::numeric_limits<passivedouble>::epsilon() ||
          !(product > 0) || !std::isfinite(product)) { valid = false; break; }
      volume += fabs(determinant) * product;  // The constant dim! cancels in the star average.
      for (size_t v = begin; v < end; ++v)
        for (size_t j = 0; j < dim; ++j) {
          const auto delta = field(cell->GetNode(j + 1), v) - field(cell->GetNode(0), v);
          for (size_t i = 0; i < dim; ++i)
            sum[v - begin][i] += (det > 0 ? 1 : -1) * product / scale[i] * cofactor[i][j] * delta;
        }
    }
    if (!valid || !(SU2_TYPE::GetValue(volume) > 0) || !std::isfinite(SU2_TYPE::GetValue(volume))) continue;
    for (size_t v = begin; v < end; ++v)
      for (size_t i = 0; i < dim; ++i)
        valid = valid && std::isfinite(SU2_TYPE::GetValue(sum[v - begin][i] / volume));
    if (valid)
      for (size_t v = begin; v < end; ++v)
        for (size_t i = 0; i < dim; ++i) gradient(point, v, i) = sum[v - begin][i] / volume;
  }
}

/*!
 * \brief Compute the gradient of a field using the Green-Gauss theorem.
 * \ingroup FvmAlgos
 * \note Template nDim to allow efficient unrolling of inner loops.
 * \note Gradients can be computed only for a contiguous range of variables, defined
 *       by [varBegin, varEnd[ (e.g. 0,1 computes the gradient of the 1st variable).
 *       This can be used, for example, to compute only velocity gradients.
 * \note The function uses an optional solver object to perform communications, if
 *       none (nullptr) is provided the function does not fail (the objective of
 *       this is to improve test-ability).
 * \param[in] solver - Optional, solver associated with the field (used only for MPI).
 * \param[in] kindMpiComm - Type of MPI communication required.
 * \param[in] kindPeriodicComm - Type of periodic communication required.
 * \param[in] geometry - Geometric grid properties.
 * \param[in] config - Configuration of the problem, used to identify types of boundaries.
 * \param[in] field - Generic object implementing operator (iPoint, iVar).
 * \param[in] varBegin - Index of first variable for which to compute the gradient.
 * \param[in] varEnd - Index of last variable for which to compute the gradient.
 * \param[in] idxVel - Index of velocity, or -1 if no velocity present.
 * \param[out] gradient - Generic object implementing operator (iPoint, iVar, iDim).
 * \param[in] eulerWalls - Apply the symmetry corrections on Euler walls too, otherwise only on symmetry planes.
 * \param[in] symmetryPlanes - Apply the symmetry corrections on symmetry planes (false: no correction there).
 * \param[in] simplexRecovery - Adaptation-only P1 simplex recovery; call outside OpenMP regions.
 */
template <size_t nDim, class FieldType, class GradientType>
void computeGradientsGreenGauss(CSolver* solver, MPI_QUANTITIES kindMpiComm, PERIODIC_QUANTITIES kindPeriodicComm,
                                CGeometry& geometry, const CConfig& config, const FieldType& field,
                                const size_t varBegin, const size_t varEnd, const int idxVel, GradientType& gradient,
                                const bool eulerWalls, const bool symmetryPlanes, const bool simplexRecovery) {
  const size_t nPointDomain = geometry.GetnPointDomain();
  const bool stableAdaptation = simplexRecovery && config.GetnMarker_Periodic() == 0;

#ifdef HAVE_OMP
  constexpr size_t OMP_MAX_CHUNK = 512;

  const auto chunkSize = computeStaticChunkSize(nPointDomain, omp_get_max_threads(), OMP_MAX_CHUNK);
#endif

  static constexpr size_t MAXNVAR = 20;

  /*--- For each (non-halo) volume integrate over its faces (edges). ---*/

  SU2_OMP_FOR_DYN(chunkSize)
  for (size_t iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    auto nodes = geometry.nodes;

    /*--- Cannot preaccumulate if hybrid parallel due to shared reading. ---*/
    if (omp_get_num_threads() == 1) AD::StartPreacc();
    AD::SetPreaccIn(nodes->GetVolume(iPoint));
    AD::SetPreaccIn(nodes->GetPeriodicVolume(iPoint));

    for (size_t iVar = varBegin; iVar < varEnd; ++iVar) AD::SetPreaccIn(field(iPoint, iVar));

    /*--- Clear the gradient. --*/

    for (size_t iVar = varBegin; iVar < varEnd; ++iVar)
      for (size_t iDim = 0; iDim < nDim; ++iDim) gradient(iPoint, iVar, iDim) = 0.0;

    /*--- Handle averaging and division by volume in one constant. ---*/

    su2double halfOnVol = 0.5 / (nodes->GetVolume(iPoint) + nodes->GetPeriodicVolume(iPoint));

    /*--- Add a contribution due to each neighbor. ---*/

    for (size_t iNeigh = 0; iNeigh < nodes->GetnPoint(iPoint); ++iNeigh) {
      size_t iEdge = nodes->GetEdge(iPoint, iNeigh);
      size_t jPoint = nodes->GetPoint(iPoint, iNeigh);

      /*--- Determine if edge points inwards or outwards of iPoint.
       *    If inwards we need to flip the area vector. ---*/

      su2double dir = (iPoint < jPoint) ? 1.0 : -1.0;
      su2double weight = dir * halfOnVol;

      const auto area = geometry.edges->GetNormal(iEdge);
      AD::SetPreaccIn(area, nDim);

      for (size_t iVar = varBegin; iVar < varEnd; ++iVar) {
        AD::SetPreaccIn(field(jPoint, iVar));
        su2double flux = weight * (field(iPoint, iVar) + field(jPoint, iVar));

        for (size_t iDim = 0; iDim < nDim; ++iDim) gradient(iPoint, iVar, iDim) += flux * area[iDim];
      }
    }

    for (size_t iVar = varBegin; iVar < varEnd; ++iVar)
      for (size_t iDim = 0; iDim < nDim; ++iDim) AD::SetPreaccOut(gradient(iPoint, iVar, iDim));

    AD::EndPreacc();
  }
  END_SU2_OMP_FOR

  su2double flux[MAXNVAR] = {0.0};

  /*--- Add edges of markers that contribute to the gradients ---*/
  for (size_t iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    if ((config.GetMarker_All_KindBC(iMarker) != INTERNAL_BOUNDARY) &&
        (config.GetMarker_All_KindBC(iMarker) != NEARFIELD_BOUNDARY) &&
        (config.GetMarker_All_KindBC(iMarker) != PERIODIC_BOUNDARY)) {
      /*--- Work is shared in inner loop as two markers
       *    may try to update the same point. ---*/

      SU2_OMP_FOR_STAT(32)
      for (size_t iVertex = 0; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
        size_t iPoint = geometry.vertex[iMarker][iVertex]->GetNode();
        auto nodes = geometry.nodes;

        /*--- Halo points do not need to be considered. ---*/

        if (!nodes->GetDomain(iPoint)) continue;

        su2double volume = nodes->GetVolume(iPoint) + nodes->GetPeriodicVolume(iPoint);
        const auto area = geometry.vertex[iMarker][iVertex]->GetNormal();

        for (size_t iVar = varBegin; iVar < varEnd; iVar++)
          flux[iVar] = field(iPoint,iVar) / volume;

        for (size_t iVar = varBegin; iVar < varEnd; iVar++) {
          for (size_t iDim = 0; iDim < nDim; iDim++) {
            gradient(iPoint, iVar, iDim) -= flux[iVar] * area[iDim];
          }
        } // loop over variables
      } // vertices
      END_SU2_OMP_FOR
    } //found right marker
  } // iMarkers


  if (stableAdaptation)
    correctGradientsSimplex<nDim>(geometry, field, varBegin, varEnd, gradient);

  /*--- Compute the corrections for symmetry planes and Euler walls. ---*/

  correctGradientsSymmetry<nDim>(geometry, config, varBegin, varEnd, idxVel, gradient, eulerWalls, symmetryPlanes);

  /*--- If no solver was provided we do not communicate ---*/

  if (solver == nullptr) return;

  /*--- Account for periodic contributions. ---*/

  for (size_t iPeriodic = 1; iPeriodic <= config.GetnMarker_Periodic() / 2; ++iPeriodic) {
    solver->InitiatePeriodicComms(&geometry, &config, iPeriodic, kindPeriodicComm);
    solver->CompletePeriodicComms(&geometry, &config, iPeriodic, kindPeriodicComm);
  }

  /*--- Obtain the gradients at halo points from the MPI ranks that own them. ---*/

  solver->InitiateComms(&geometry, &config, kindMpiComm);
  solver->CompleteComms(&geometry, &config, kindMpiComm);
}
}  // namespace detail



/*!
 * \brief Instantiations for 2D and 3D.
 * \ingroup FvmAlgos
 */
template <class FieldType, class GradientType>
void computeGradientsGreenGauss(CSolver* solver, MPI_QUANTITIES kindMpiComm, PERIODIC_QUANTITIES kindPeriodicComm,
                                CGeometry& geometry, const CConfig& config, const FieldType& field,
                                const size_t varBegin, const size_t varEnd, const int idxVel, GradientType& gradient,
                                const bool eulerWalls = true, const bool symmetryPlanes = true,
                                const bool simplexRecovery = false) {
  switch (geometry.GetnDim()) {
    case 2:
      detail::computeGradientsGreenGauss<2>(solver, kindMpiComm, kindPeriodicComm, geometry, config, field, varBegin,
                                            varEnd, idxVel, gradient, eulerWalls, symmetryPlanes, simplexRecovery);
      break;
    case 3:
      detail::computeGradientsGreenGauss<3>(solver, kindMpiComm, kindPeriodicComm, geometry, config, field, varBegin,
                                            varEnd, idxVel, gradient, eulerWalls, symmetryPlanes, simplexRecovery);
      break;
    default:
      SU2_MPI::Error("Too many dimensions to compute gradients.", CURRENT_FUNCTION);
      break;
  }
}
