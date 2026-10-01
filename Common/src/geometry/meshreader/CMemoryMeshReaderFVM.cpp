/*!
 * \file CMemoryMeshReaderFVM.cpp
 * \brief Reads a simplex mesh from memory into linear partitions for the finite volume solver (FVM).
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

#include "../../../include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../include/adaptation/CSimplexMesh.hpp"
#include "../../../include/toolboxes/CLinearPartitioner.hpp"

CMemoryMeshReaderFVM::CMemoryMeshReaderFVM(const CConfig* val_config, const CSimplexMesh& mesh,
                                           unsigned short val_iZone, unsigned short val_nZone)
    : CMeshReaderBase(val_config, val_iZone, val_nZone) {
  dimension = mesh.nDim;
  if (dimension != 2 && dimension != 3) {
    SU2_MPI::Error("The mesh in memory must be 2D or 3D.", CURRENT_FUNCTION);
  }
  const unsigned long nPoint = mesh.GetnPoint();
  if (nPoint == 0 || mesh.GetnElem() == 0 || mesh.coord.size() != nPoint * dimension ||
      mesh.elem.size() != mesh.GetnElem() * (dimension + 1)) {
    SU2_MPI::Error("The mesh in memory is empty or its arrays have inconsistent sizes.", CURRENT_FUNCTION);
  }

  SetPointCoordinates(mesh);
  SetVolumeElementConnectivity(mesh);
  SetSurfaceElementConnectivity(mesh);

  /*--- Duplicate some markers if requested. ---*/
  CopyMarkers(val_config->GetMarkerCreateCopy());
}

void CMemoryMeshReaderFVM::SetPointCoordinates(const CSimplexMesh& mesh) {
  numberOfGlobalPoints = mesh.GetnPoint();

  /*--- Linear partitioning of the points, as for a mesh file. ---*/
  CLinearPartitioner pointPartitioner(numberOfGlobalPoints, 0);
  numberOfLocalPoints = pointPartitioner.GetSizeOnRank(rank);
  const unsigned long firstIndex = pointPartitioner.GetFirstIndexOnRank(rank);

  localPointCoordinates.resize(dimension);
  for (unsigned short iDim = 0; iDim < dimension; iDim++) {
    localPointCoordinates[iDim].resize(numberOfLocalPoints);
    for (unsigned long iPoint = 0; iPoint < numberOfLocalPoints; iPoint++) {
      localPointCoordinates[iDim][iPoint] = mesh.coord[(firstIndex + iPoint) * dimension + iDim];
    }
  }
}

void CMemoryMeshReaderFVM::SetVolumeElementConnectivity(const CSimplexMesh& mesh) {
  numberOfGlobalElements = mesh.GetnElem();
  const unsigned short nNode = dimension + 1;
  const unsigned short vtkType = (dimension == 2) ? TRIANGLE : TETRAHEDRON;

  CLinearPartitioner pointPartitioner(numberOfGlobalPoints, 0);

  /*--- Store every element with at least one point in our linear partition (elements on the boundaries of the
   * partitions are stored by more than one rank). ---*/
  numberOfLocalElements = 0;
  for (unsigned long iElem = 0; iElem < numberOfGlobalElements; iElem++) {
    const auto* nodes = &mesh.elem[iElem * nNode];

    bool isOwned = false;
    for (unsigned short iNode = 0; iNode < nNode; iNode++) {
      if (nodes[iNode] >= numberOfGlobalPoints) {
        SU2_MPI::Error("Element " + std::to_string(iElem) + " of the mesh in memory has a point out of range.",
                       CURRENT_FUNCTION);
      }
      if (pointPartitioner.IndexBelongsToRank(nodes[iNode], rank)) isOwned = true;
    }
    if (!isOwned) continue;

    localVolumeElementConnectivity.push_back(iElem);
    localVolumeElementConnectivity.push_back(vtkType);
    for (unsigned short iNode = 0; iNode < N_POINTS_HEXAHEDRON; iNode++) {
      localVolumeElementConnectivity.push_back(iNode < nNode ? nodes[iNode] : 0);
    }
    numberOfLocalElements++;
  }
}

void CMemoryMeshReaderFVM::SetSurfaceElementConnectivity(const CSimplexMesh& mesh) {
  const unsigned short nNode = dimension;
  const unsigned short vtkType = (dimension == 2) ? LINE : TRIANGLE;

  numberOfMarkers = mesh.markers.size();
  markerNames.resize(numberOfMarkers);
  surfaceElementConnectivity.resize(numberOfMarkers);

  for (unsigned long iMarker = 0; iMarker < numberOfMarkers; iMarker++) {
    const auto& marker = mesh.markers[iMarker];
    markerNames[iMarker] = marker.name;
    if (marker.elem.size() % nNode != 0) {
      SU2_MPI::Error("Marker " + marker.name + " of the mesh in memory has an inconsistent size.", CURRENT_FUNCTION);
    }

    /*--- As in the file readers, only the master rank stores the boundary elements. ---*/
    if (rank != MASTER_NODE) continue;

    auto& connectivity = surfaceElementConnectivity[iMarker];
    connectivity.reserve(marker.GetnElem(nNode) * SU2_CONN_SIZE);
    for (unsigned long iElem = 0; iElem < marker.GetnElem(nNode); iElem++) {
      connectivity.push_back(0);
      connectivity.push_back(vtkType);
      for (unsigned short iNode = 0; iNode < N_POINTS_HEXAHEDRON; iNode++) {
        const unsigned long iPoint = iNode < nNode ? marker.elem[iElem * nNode + iNode] : 0;
        if (iPoint >= numberOfGlobalPoints) {
          SU2_MPI::Error("Marker " + marker.name + " of the mesh in memory has a point out of range.",
                         CURRENT_FUNCTION);
        }
        connectivity.push_back(iPoint);
      }
    }
  }
}
