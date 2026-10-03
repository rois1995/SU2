/*!
 * \file CDistributedMemoryMeshReaderFVM.hpp
 * \brief Header file for the class CDistributedMemoryMeshReaderFVM.
 *        The implementations are in the <i>CDistributedMemoryMeshReaderFVM.cpp</i> file.
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

struct CReaderSlices;

/*!
 * \class CDistributedMemoryMeshReaderFVM
 * \brief Mesh reader for the FVM solver that takes the arrays of this rank from reader slices in memory
 *        (CReaderSlices): the points of the rank's linear partition, the volume elements touching them, all marker
 *        names, the boundary elements on the master rank. No rank needs the complete mesh.
 * \note The arrays are taken as they are (CReaderSlices::FromComplete builds exactly those of CMemoryMeshReaderFVM, so
 *       the geometry is the same as with that reader). Coordinates in the units of the solver; markers listed in
 *       MARKER_CREATE_COPY are copied as in the file readers.
 */
class CDistributedMemoryMeshReaderFVM : public CMeshReaderBase {
 public:
  /*!
   * \brief Constructor (not collective).
   * \param[in] val_config - config object for the current zone.
   * \param[in] slices - This rank's slices of the mesh.
   * \param[in] val_iZone - Current zone index.
   * \param[in] val_nZone - Total number of zones.
   */
  CDistributedMemoryMeshReaderFVM(const CConfig* val_config, const CReaderSlices& slices, unsigned short val_iZone,
                                  unsigned short val_nZone);
};
