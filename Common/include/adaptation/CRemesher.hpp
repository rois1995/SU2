/*!
 * \file CRemesher.hpp
 * \brief Interface of the remesher of the mesh adaptation loop.
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

#include <string>
#include <vector>
#include <functional>

#include "../containers/C2DContainer.hpp"
#include "CReaderSlices.hpp"
#include "CSimplexMesh.hpp"

class CConfig;
class CGeometry;

/*!
 * \struct CRemeshResult
 * \brief What a remesher returns on each rank: the rank's reader slices of the new mesh and the markers of the mesh.
 */
struct CRemeshResult {
  /*! \brief Status of the remesh: a complete remesh, or (distributed remeshers) a valid mesh with regions left
   *         unadapted. The serial remesher always returns COMPLETE. */
  enum class Status { COMPLETE, INCOMPLETE_COVERAGE, INCOMPLETE_QUALITY };

  CReaderSlices slices;              /*!< \brief This rank's part of the new mesh (with the metric of its points). */
  std::vector<std::string> markers;  /*!< \brief Markers of the new mesh with boundary elements (all ranks). */
  Status status = Status::COMPLETE;  /*!< \brief Status of the remesh. */
  std::function<void()> onAccepted;  /*!< \brief Publish prepared reference bindings after successful replacement/transfer. */
};

/*!
 * \class CRemesher
 * \brief Makes the new mesh of an adaptation cycle from the current geometry and its metric.
 * \note One of the three parts of an adaptation cycle, with the metric (CSinglezoneDriver::ComputeMetric) and the
 *       solution transfer (CSolutionTransfer). Serial MMG now (CMMGRemesher); a distributed remesher works on the
 *       distributed geometry behind the same call. No rank needs the complete new mesh: each gets the part it reads.
 */
class CRemesher {
 public:
  virtual ~CRemesher() = default;

  /*!
   * \brief Collective: remesh.
   * \param[in] config - Definition of the problem (remeshing parameters, marker names).
   * \param[in] geometry - Current geometry of the zone (finest grid).
   * \param[in] metric - Metric at the points of the geometry, upper triangle (CVariable::GetMetric()).
   * \return This rank's reader slices of the new mesh (validated), the markers with elements, the status.
   */
  virtual CRemeshResult Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) = 0;
};
