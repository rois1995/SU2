/*!
 * \file CDistributedMemoryMeshReaderFVM.cpp
 * \brief Reads the linear partition of this rank of a simplex mesh from reader slices in memory (FVM).
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

#include "../../../include/geometry/meshreader/CDistributedMemoryMeshReaderFVM.hpp"
#include "../../../include/adaptation/CReaderSlices.hpp"
#include "../../../include/toolboxes/CLinearPartitioner.hpp"

CDistributedMemoryMeshReaderFVM::CDistributedMemoryMeshReaderFVM(const CConfig* val_config,
                                                                 const CReaderSlices& slices,
                                                                 unsigned short val_iZone, unsigned short val_nZone)
    : CMeshReaderBase(val_config, val_iZone, val_nZone) {
  dimension = slices.nDim;
  if (dimension != 2 && dimension != 3) {
    SU2_MPI::Error("The mesh in memory must be 2D or 3D.", CURRENT_FUNCTION);
  }

  /*--- The slices must be those of this rank's linear partition. ---*/
  const CLinearPartitioner pointPartitioner(slices.nPointGlobal, 0);
  if (slices.nPointGlobal == 0 || slices.nElemGlobal == 0 ||
      slices.firstPoint != pointPartitioner.GetFirstIndexOnRank(rank) ||
      slices.nPointLocal != pointPartitioner.GetSizeOnRank(rank) || slices.coord.size() != dimension ||
      slices.elemRows.size() != slices.nElemLocal * SU2_CONN_SIZE ||
      slices.boundaryRows.size() != slices.markerNames.size()) {
    SU2_MPI::Error("The reader slices of this rank are empty or inconsistent.", CURRENT_FUNCTION);
  }
  for (const auto& coord : slices.coord) {
    if (coord.size() != slices.nPointLocal) SU2_MPI::Error("Inconsistent reader slices.", CURRENT_FUNCTION);
  }

  numberOfGlobalPoints = slices.nPointGlobal;
  numberOfLocalPoints = slices.nPointLocal;
  localPointCoordinates = slices.coord;

  numberOfGlobalElements = slices.nElemGlobal;
  numberOfLocalElements = slices.nElemLocal;
  localVolumeElementConnectivity = slices.elemRows;

  numberOfMarkers = slices.markerNames.size();
  markerNames = slices.markerNames;
  surfaceElementConnectivity = slices.boundaryRows;

  /*--- Duplicate some markers if requested. ---*/
  CopyMarkers(val_config->GetMarkerCreateCopy());
}
