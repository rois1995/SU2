/*!
 * \file CReaderSlices.hpp
 * \brief The part of a new mesh that one rank reads (the arrays of a mesh reader on that rank), built without
 *        giving every rank the complete mesh.
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

#include "CSimplexMesh.hpp"
#include "../option_structure.hpp"
#include "../parallelization/mpi_structure.hpp"

class CGeometry;

/*!
 * \struct CReaderSlices
 * \brief The arrays a mesh reader (CMeshReaderBase) holds on one rank for a simplex mesh, plus the metric of the points
 *        of the rank: the points of the rank's linear slice of the global point numbering (CLinearPartitioner(N, 0)),
 *        every volume element with a point in that slice (row [element index, VTK type, 8 node slots], the element
 *        index being its position in the mesh, rows in mesh order), the names of all markers, and the boundary
 *        elements (rows [0, VTK type, 8 node slots], per marker in mesh order) on the master rank only.
 * \note Read by CDistributedMemoryMeshReaderFVM. FromComplete builds exactly the rows CMemoryMeshReaderFVM builds on
 *       each rank from the complete mesh (same point and element indices, same orders), so the geometry built from
 *       the slices is bitwise the geometry built from the complete mesh, while no rank but the root holds the
 *       complete mesh.
 */
struct CReaderSlices {
  unsigned short nDim = 0;                         /*!< \brief Number of dimensions (2 or 3). */
  unsigned long nPointGlobal = 0;                  /*!< \brief Points of the whole mesh. */
  unsigned long nElemGlobal = 0;                   /*!< \brief Volume elements of the whole mesh. */
  unsigned long firstPoint = 0;                    /*!< \brief Global index of the first point of this rank's slice. */
  unsigned long nPointLocal = 0;                   /*!< \brief Points of this rank's slice. */
  std::vector<std::vector<passivedouble>> coord;   /*!< \brief Coordinates, coord[iDim][iPoint] (reader layout). */
  unsigned short nMetric = 0;                      /*!< \brief Metric components per point (0: no metric). */
  std::vector<passivedouble> metric;               /*!< \brief Metric of the slice's points, nPointLocal x nMetric. */
  unsigned long nElemLocal = 0;                    /*!< \brief Volume elements touching the slice. */
  std::vector<unsigned long> elemRows;             /*!< \brief Their rows, nElemLocal x SU2_CONN_SIZE. */
  std::vector<std::string> markerNames;            /*!< \brief Names of all markers of the mesh, in mesh order. */
  std::vector<std::vector<unsigned long>> boundaryRows; /*!< \brief Per marker: rows of its boundary elements
                                                            (SU2_CONN_SIZE each) on the master rank, empty elsewhere. */
  std::vector<std::string> markersWithElements;    /*!< \brief Names of the markers with boundary elements (all ranks). */

  /*!
   * \brief Collective: the slices of a complete mesh given on the root rank; the mesh of the other ranks is not used.
   * \note The root builds the rows of each rank in turn and sends them (CPassiveComm, in chunks); it holds the mesh
   *       and the rows of one rank at a time. The checks of CMemoryMeshReaderFVM (sizes, point indices) are done on
   *       the root with the same messages. The metric is copied if the mesh has one (nPoint x nMetric).
   * \param[in] mesh - Complete mesh (root only).
   * \param[in] root - Rank that holds the mesh.
   */
  static CReaderSlices FromComplete(const CSimplexMesh& mesh, int root = MASTER_NODE);

  /*!
   * \brief Collective: the metric at the local points (domain and halo) of a geometry built from these slices, fetched
   *        by global point index from the ranks whose slice holds the point.
   * \return nPoint x nMetric values in the local point order of the geometry; empty when the slices have no metric.
   */
  std::vector<passivedouble> FetchPointMetric(const CGeometry& geometry) const;
};
