/*!
 * \file CMemoryMeshReaderFVM.hpp
 * \brief Header file for the class CMemoryMeshReaderFVM.
 *        The implementations are in the <i>CMemoryMeshReaderFVM.cpp</i> file.
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

#include "CMeshReaderBase.hpp"

struct CSimplexMesh;

/*!
 * \class CMemoryMeshReaderFVM
 * \brief Mesh reader for the FVM solver that takes a simplex mesh from memory instead of a file.
 * \note The reader gives the same data as the file readers: points and volume elements of the linear partition of
 *       the rank, all markers (by name) on the master rank. The mesh must be the complete (global) mesh and be
 *       available on every rank, as a mesh file would be. Coordinates are used as they are (no unit conversion,
 *       they are already in the units of the solver). Elements are taken with any orientation, the geometry
 *       preprocessing reorients them (REORIENT_ELEMENTS). Markers listed in MARKER_CREATE_COPY are copied as in the
 *       file readers, so the mesh must not contain the copies.
 */
class CMemoryMeshReaderFVM : public CMeshReaderBase {
 private:
  /*!
   * \brief Store the coordinates of the points of the linear partition of this rank.
   */
  void SetPointCoordinates(const CSimplexMesh& mesh);

  /*!
   * \brief Store the volume elements that have at least one point in the linear partition of this rank.
   */
  void SetVolumeElementConnectivity(const CSimplexMesh& mesh);

  /*!
   * \brief Store the markers, the boundary elements are stored on the master rank only.
   */
  void SetSurfaceElementConnectivity(const CSimplexMesh& mesh);

 public:
  /*!
   * \brief Constructor of the CMemoryMeshReaderFVM class.
   * \param[in] val_config - config object for the current zone.
   * \param[in] mesh - Simplex mesh (triangles in 2D, tetrahedra in 3D), with its markers.
   * \param[in] val_iZone - Current zone index.
   * \param[in] val_nZone - Total number of zones.
   */
  CMemoryMeshReaderFVM(const CConfig* val_config, const CSimplexMesh& mesh, unsigned short val_iZone,
                       unsigned short val_nZone);
};
