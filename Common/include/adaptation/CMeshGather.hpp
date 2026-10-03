/*!
 * \file CMeshGather.hpp
 * \brief Gather of the partitioned mesh and of point values on one rank in the global numbering (mesh adaptation with
 *        MPI), the scatter back, the broadcast of a mesh and the exchange of halo values.
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

class CConfig;
class CGeometry;

/*!
 * \class CMeshGather
 * \brief The partitioned simplex mesh of the finest grid gathered on one rank (the root) in the global numbering of
 *        the mesh reader, the gather of point values to the root and their scatter back to the ranks that own the
 *        points. The serial parts of the mesh adaptation (MMG, the solution transfers, the metric prediction) run on
 *        the root with these complete meshes.
 * \note All methods except the static LocalMesh are collective: every rank of SU2_MPI::GetComm() calls them in the
 *       same order. The result does not depend on the number of ranks or on the partition:
 *       - Points: the domain (owned) points of all ranks in the order of their global index, which must be 0 to
 *         N-1 (CPoint::GetGlobalIndex, the numbering of the mesh file or of the mesh in memory).
 *       - Volume elements: triangles (2D) or tetrahedra (3D), each element once (sent by the rank that owns its node
 *         of smallest global index; that rank holds every element of its points), sorted by their sorted global nodes
 *         (not by their global index, the order of the mesh file: the adapted mesh is exported in an order that
 *         depends on the ranks, and a run restarted from it must give the remesher the same input), nodes in the order
 *         of the geometry.
 *       - Boundary elements (lines or triangles): grouped by marker name, send/receive markers excluded, markers in
 *         the order of the config file (as the mesh output writes them); each element once (same rule), sorted by
 *         their sorted global nodes (the partitioned geometry has no global index for them), nodes in the order of
 *         the geometry. The marker names of the geometry are given by the caller (the config describes the newest
 *         mesh only).
 *       Errors stop all ranks (SU2_MPI::Error aborts the communicator with the message of the failing rank); a rank
 *       that fails while the others wait in a collective call of this class stops them as well.
 */
class CMeshGather {
 public:
  /*!
   * \brief Collective: the global index of every domain point of every rank, on the root.
   * \param[in] geometry - Finest grid, partitioned (domain points first, then halo points).
   * \param[in] root - Rank that receives the mesh and the values.
   */
  explicit CMeshGather(const CGeometry& geometry, int root = MASTER_NODE);

  bool IsRoot() const { return SU2_MPI::GetRank() == root; }
  int GetRoot() const { return root; }

  /*! \brief Number of points of the whole mesh. */
  unsigned long GetnPointGlobal() const { return nPointGlobal; }

  /*!
   * \brief Collective: the whole mesh on the root (points, volume elements, boundary elements by marker name with
   *        their MMG reference, CMMGInterface::GetMarkerReference); the other ranks get nDim and the marker names only.
   * \param[in] config - Definition of the problem (marker references and order).
   * \param[in] markerTags - Name of each marker of the geometry (by iMarker); SEND_RECEIVE markers are skipped. An
   *            empty name is an unnamed marker (one marker of the mesh with reference 0, after the named ones).
   * \param[in] controlVolumes - Also gather the control volume of each point (CSimplexMesh::volume).
   */
  CSimplexMesh GatherMesh(const CConfig& config, const std::vector<std::string>& markerTags,
                          bool controlVolumes) const;

  /*!
   * \brief Collective: values of the domain points (n per point, local[iPoint * n + k] for iPoint < nPointDomain) on
   *        the root, in the global numbering (nPointGlobal x n). Empty on the other ranks.
   */
  std::vector<su2double> Gather(const su2double* local, unsigned short n) const;

  /*!
   * \brief Collective: the reverse of Gather, values of the root (nPointGlobal x n, global numbering) to the ranks that
   *        own the points: local[iPoint * n + k] for the domain points (halo points are not set).
   */
  void Scatter(const std::vector<su2double>& global, unsigned short n, su2double* local) const;

  /*!
   * \brief Collective: copy a mesh from the root to all ranks.
   */
  static void Broadcast(CSimplexMesh& mesh, int root = MASTER_NODE);

  /*!
   * \brief Collective: names of the physical markers (not SEND_RECEIVE) with boundary elements on any rank, in the
   *        order of the config file. Every rank holds only the markers with elements in its partition, in an order of
   *        its own; the names are the key that is the same on all ranks. An empty name (unnamed marker) comes last.
   * \param[in] config - Definition of the problem (order of the markers).
   * \param[in] markerTags - Name of each marker of the geometry (by iMarker).
   * \param[in] geometry - Partitioned geometry.
   */
  static std::vector<std::string> GatherMarkerNames(const CConfig& config, const std::vector<std::string>& markerTags,
                                                    const CGeometry& geometry);

  /*!
   * \brief Collective: the values of the halo points from the ranks that own them (n values per point,
   *        values[iPoint * n + k] for all local points).
   */
  static void ExchangeHalo(const CGeometry& geometry, su2double* values, unsigned short n);

  /*!
   * \brief Not collective: the mesh of this rank in its local numbering (all points, halos included; triangles or
   *        tetrahedra, error otherwise; boundary elements by marker name, markers of the same name merged in the order
   *        of iMarker, send/receive markers and vertex elements skipped), for the serial parts used on one rank.
   * \param[in] controlVolumes - Also copy the control volume of each point.
   */
  static CSimplexMesh LocalMesh(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                bool controlVolumes);

 private:
  /*--- Values of the domain points of all ranks to the root and back (n per point, in rank order). ---*/
  std::vector<su2double> GatherRankOrder(const su2double* local, unsigned short n) const;

  const CGeometry& geometry;
  int root;
  unsigned long nPointDomain = 0;      /*!< \brief Domain points of this rank. */
  unsigned long nPointGlobal = 0;      /*!< \brief Points of the whole mesh. */
  std::vector<int> rankPoints;         /*!< \brief Root: domain points of each rank. */
  std::vector<int> rankOffset;         /*!< \brief Root: offset of each rank in the rank order. */
  std::vector<unsigned long> globalId; /*!< \brief Root: global index of each point in the rank order. */
};
