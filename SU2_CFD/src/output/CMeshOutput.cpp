/*!
 * \file CMeshOutput.cpp
 * \brief Main subroutines for the heat solver output
 * \author R. Sanchez
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


#include "../../include/output/CMeshOutput.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/output/filewriter/CFVMDataSorter.hpp"
#include "../../include/output/filewriter/CSU2MeshFileWriter.hpp"
#include "../../include/output/filewriter/CSU2MeshBinaryFileWriter.hpp"
#include "../../include/output/filewriter/CCGNSFileWriter.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <fstream>
#include <memory>

CMeshOutput::CMeshOutput(CConfig *config, unsigned short nDim) : COutput(config, nDim, false) {

  /*--- Set the default history fields if nothing is set in the config file ---*/

  requestedVolumeFields.emplace_back("COORDINATES");
  nRequestedVolumeFields = requestedVolumeFields.size();

  /*--- Set the volume filename --- */

  volumeFilename = config->GetMesh_Out_FileName();

  /*--- Set the surface filename ---*/

  surfaceFilename = "surface_mesh";

}

CMeshOutput::~CMeshOutput() = default;

void CMeshOutput::SetVolumeOutputFields(CConfig *config){

  // Grid coordinates
  AddVolumeOutput("COORD-X", "x", "COORDINATES", "x-component of the coordinate vector");
  AddVolumeOutput("COORD-Y", "y", "COORDINATES", "y-component of the coordinate vector");
  if (nDim == 3)
    AddVolumeOutput("COORD-Z", "z", "COORDINATES", "z-component of the coordinate vector");

  // Mesh quality metrics, computed in CPhysicalGeometry::ComputeMeshQualityStatistics.
  AddVolumeOutput("ORTHOGONALITY", "Orthogonality", "MESH_QUALITY", "Orthogonality Angle (deg.)");
  AddVolumeOutput("ASPECT_RATIO",  "Aspect_Ratio",  "MESH_QUALITY", "CV Face Area Aspect Ratio");
  AddVolumeOutput("VOLUME_RATIO",  "Volume_Ratio",  "MESH_QUALITY", "CV Sub-Volume Ratio");

}

void CMeshOutput::LoadVolumeData(CConfig *config, CGeometry *geometry, CSolver **solver, unsigned long iPoint){

  CPoint*    Node_Geo  = geometry->nodes;

  SetVolumeOutputValue("COORD-X", iPoint,  Node_Geo->GetCoord(iPoint, 0));
  SetVolumeOutputValue("COORD-Y", iPoint,  Node_Geo->GetCoord(iPoint, 1));
  if (nDim == 3)
    SetVolumeOutputValue("COORD-Z", iPoint, Node_Geo->GetCoord(iPoint, 2));

  // Mesh quality metrics
  if (config->GetWrt_MeshQuality()) {
    SetVolumeOutputValue("ORTHOGONALITY", iPoint, geometry->Orthogonality[iPoint]);
    SetVolumeOutputValue("ASPECT_RATIO",  iPoint, geometry->Aspect_Ratio[iPoint]);
    SetVolumeOutputValue("VOLUME_RATIO",  iPoint, geometry->Volume_Ratio[iPoint]);
  }

}

void CMeshOutput::WriteMesh(CConfig *config, CGeometry *geometry, const string& fileName) {

  /*--- Coordinates use the usual point sorter; mesh connectivity uses element IDs, not the lowest node ID. ---*/

  const auto nDim = geometry->GetnDim();
  vector<string> fieldNames = {"x", "y"};
  if (nDim == 3) fieldNames.emplace_back("z");

  CFVMDataSorter sorter(config, geometry, fieldNames);
  for (unsigned long iPoint = 0; iPoint < geometry->GetnPointDomain(); iPoint++) {
    for (unsigned short iDim = 0; iDim < nDim; iDim++) {
      sorter.SetUnsortedData(iPoint, iDim, geometry->nodes->GetCoord(iPoint, iDim));
    }
  }
  sorter.SortOutputData();

  vector<unsigned long> volumeConn;
  vector<CFVMDataSorter::BoundaryMarker> boundaryMarkers;
  if (config->GetMesh_Out_FileFormat() == ENUM_GRID::CGNS_GRID) {
    sorter.SortConnectivity(config, geometry, true);
  } else {
    /*--- Send all copies (including halos) to the linear owner of the element ID, then sort and remove duplicates.
     *    This also handles ranks with no elements and mixed element types without changing the volume sorter. ---*/
    using Element = std::array<unsigned long, SU2_CONN_SIZE>;  // Global ID, VTK type, up to eight global node IDs.
    const CLinearPartitioner elemPartition(geometry->GetGlobal_nElemDomain(), 0);
    const auto size = SU2_MPI::GetSize();
    vector<size_t> sendCount(size, 0), recvCount;
    for (auto iElem = 0ul; iElem < geometry->GetnElem(); ++iElem)
      ++sendCount[elemPartition.GetRankContainingIndex(geometry->elem[iElem]->GetGlobalIndex())];
    vector<size_t> offset(size, 0);
    std::partial_sum(sendCount.begin(), sendCount.end() - 1, offset.begin() + 1);
    vector<Element> send(geometry->GetnElem());
    for (auto iElem = 0ul; iElem < geometry->GetnElem(); ++iElem) {
      const auto* elem = geometry->elem[iElem];
      auto& record = send[offset[elemPartition.GetRankContainingIndex(elem->GetGlobalIndex())]++];
      record[0] = elem->GetGlobalIndex();
      record[1] = elem->GetVTK_Type();
      for (unsigned short iNode = 0; iNode < elem->GetnNodes(); ++iNode)
        record[iNode + 2] = geometry->nodes->GetGlobalIndex(elem->GetNode(iNode));
    }
    auto elements = CPassiveComm::Alltoallv(send, sendCount, recvCount);
    std::sort(elements.begin(), elements.end(), [](const Element& a, const Element& b) { return a[0] < b[0]; });
    elements.erase(std::unique(elements.begin(), elements.end(),
                              [](const Element& a, const Element& b) { return a[0] == b[0]; }), elements.end());
    const auto rank = SU2_MPI::GetRank();
    if (elements.size() != elemPartition.GetSizeOnRank(rank))
      SU2_MPI::Error("Mesh output is missing global volume element IDs.", CURRENT_FUNCTION);
    for (size_t i = 0; i < elements.size(); ++i)
      if (elements[i][0] != elemPartition.GetFirstIndexOnRank(rank) + i)
        SU2_MPI::Error("Mesh output volume element IDs are not dense.", CURRENT_FUNCTION);

    /*--- Retain the full order for the SU2 writers: grouping by type would reorder a mixed mesh even in serial. ---*/
    for (const auto& elem : elements) {
      const auto type = static_cast<GEO_TYPE>(elem[1]);
      volumeConn.push_back(type);
      for (unsigned short iNode = 0; iNode < nPointsOfElementType(type); ++iNode)
        volumeConn.push_back(elem[iNode + 2]);
      volumeConn.push_back(elem[0]);
      ++sorter.nElemPerType[sorter.TypeMap.at(type)];
    }
    sorter.SetTotalElements();
    sorter.connectivitySorted = true;

    /*--- GetGlobalIndex/GetDomainElement of a boundary object stores its adjacent volume, not its face ID.
     *    Geometry preserves the original face IDs separately before freeing the partition buffers. ---*/
    const auto* physical = dynamic_cast<const CPhysicalGeometry*>(geometry);
    if (!physical) SU2_MPI::Error("Mesh output needs a physical geometry for boundary IDs.", CURRENT_FUNCTION);

    for (unsigned short iCfg = 0; iCfg < config->GetnMarker_CfgFile(); ++iCfg) {
      const auto tag = config->GetMarker_CfgFile_TagBound(iCfg);
      if (config->GetMarker_CfgFile_KindBC(tag) == SEND_RECEIVE) continue;
      vector<Element> local;
      for (unsigned short iMarker = 0; iMarker < geometry->GetnMarker(); ++iMarker) {
        if (config->GetMarker_All_TagBound(iMarker) != tag) continue;
        for (auto iElem = 0ul; iElem < geometry->GetnElem_Bound(iMarker); ++iElem) {
          const auto* elem = geometry->bound[iMarker][iElem];
          bool halo = false, owned = false;
          Element record{};
          record[1] = elem->GetVTK_Type();
          for (unsigned short iNode = 0; iNode < elem->GetnNodes(); ++iNode) {
            const auto node = elem->GetNode(iNode);
            halo |= sorter.GetHalo(node);
            owned |= geometry->nodes->GetDomain(node);
            record[iNode + 2] = geometry->nodes->GetGlobalIndex(node);
          }
          if (halo || !owned) continue;
          const auto id = physical->BoundaryGlobalIndex.find(elem);
          if (id == physical->BoundaryGlobalIndex.end())
            SU2_MPI::Error("Mesh output is missing a boundary element ID.", CURRENT_FUNCTION);
          record[0] = id->second;
          local.push_back(record);
        }
      }
      auto faces = CPassiveComm::Allgatherv(local, nullptr);
      if (faces.empty()) continue;
      std::sort(faces.begin(), faces.end(), [](const Element& a, const Element& b) { return a[0] < b[0]; });
      CFVMDataSorter::BoundaryMarker marker;
      marker.name = tag;
      marker.nElem = faces.size();
      for (const auto& face : faces) {
        marker.conn.push_back(face[1]);
        for (unsigned short iNode = 0; iNode < nPointsOfElementType(face[1]); ++iNode)
          marker.conn.push_back(face[iNode + 2]);
      }
      boundaryMarkers.push_back(std::move(marker));
    }
  }

  /*--- The boundaries come from the geometry, named as the markers. ---*/

  std::unique_ptr<CFileWriter> fileWriter;
  string extension;

  switch (config->GetMesh_Out_FileFormat()) {
    case ENUM_GRID::SU2: {
      auto* writer = new CSU2MeshFileWriter(&sorter, config->GetiZone(), config->GetnZone());
      fileWriter.reset(writer);
      writer->SetBoundaryMarkers(boundaryMarkers);
      writer->SetVolumeConnectivity(&volumeConn);
      extension = CSU2MeshFileWriter::fileExt;
      break;
    }
    case ENUM_GRID::SU2_BIN: {
      auto* writer = new CSU2MeshBinaryFileWriter(&sorter, config->GetiZone(), config->GetnZone());
      fileWriter.reset(writer);
      writer->SetBoundaryMarkers(boundaryMarkers);
      writer->SetVolumeConnectivity(&volumeConn);
      extension = CSU2MeshBinaryFileWriter::fileExt;
      break;
    }
    case ENUM_GRID::CGNS_GRID: {
#ifndef HAVE_CGNS
      SU2_MPI::Error("MESH_OUT_FORMAT= CGNS needs CGNS support: SU2 was built without it.", CURRENT_FUNCTION);
#endif
      auto* writer = new CCGNSFileWriter(&sorter);
      fileWriter.reset(writer);
      writer->SetBoundaryMarkers(config, geometry, &sorter);
      extension = CCGNSFileWriter::fileExt;
      break;
    }
    default:
      SU2_MPI::Error("Unrecognized mesh_out format specified!", CURRENT_FUNCTION);
      break;
  }

  fileWriter->WriteData(fileName);

  /*--- The writers do not all check their output streams, check that the file is there. ---*/

  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::ifstream file(fileName + extension, std::ios::binary | std::ios::ate);
    if (!file.is_open() || file.tellg() <= 0) {
      SU2_MPI::Error("The mesh file " + fileName + extension + " could not be written.", CURRENT_FUNCTION);
    }
  }
}
