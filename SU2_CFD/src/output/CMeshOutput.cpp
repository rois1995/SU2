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
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../include/output/filewriter/CFVMDataSorter.hpp"
#include "../../include/output/filewriter/CSU2MeshFileWriter.hpp"
#include "../../include/output/filewriter/CSU2MeshBinaryFileWriter.hpp"
#include "../../include/output/filewriter/CCGNSFileWriter.hpp"

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

  /*--- Sort the coordinates and the volume elements by global index, as for the volume output. ---*/

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
  sorter.SortConnectivity(config, geometry, true);

  /*--- The boundaries come from the geometry, named as the markers. ---*/

  std::unique_ptr<CFileWriter> fileWriter;
  string extension;

  switch (config->GetMesh_Out_FileFormat()) {
    case ENUM_GRID::SU2: {
      auto* writer = new CSU2MeshFileWriter(&sorter, config->GetiZone(), config->GetnZone());
      fileWriter.reset(writer);
      writer->SetBoundaryMarkers(config, geometry, &sorter);
      extension = CSU2MeshFileWriter::fileExt;
      break;
    }
    case ENUM_GRID::SU2_BIN: {
      auto* writer = new CSU2MeshBinaryFileWriter(&sorter, config->GetiZone(), config->GetnZone());
      fileWriter.reset(writer);
      writer->SetBoundaryMarkers(config, geometry, &sorter);
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
