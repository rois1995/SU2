/*!
 * \file CMMGInterface.hpp
 * \brief Conversion of the SU2 mesh and adaptation metric to MMG (MMG2D/MMG3D) and remeshing in memory.
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

#include <memory>
#include <string>
#include <vector>

#include "../basic_types/datatype_structure.hpp"
#include "../containers/C2DContainer.hpp"

class CConfig;
class CGeometry;

/*!
 * \brief Simplex mesh (triangles in 2D, tetrahedra in 3D) stored as plain arrays.
 * \note This is the exchange format between SU2 and MMG, and the input to rebuild a CPhysicalGeometry.
 *       Point indices are 0-based, volume elements are positively oriented. Boundary elements are grouped
 *       by physical marker name; the orientation of boundary elements is not prescribed.
 */
struct CSimplexMesh {
  /*!
   * \brief Boundary elements of one physical marker.
   */
  struct Marker {
    std::string name;                /*!< \brief Physical marker name (MARKER_* tag). */
    int ref = 0;                     /*!< \brief MMG reference of the marker (> 0). */
    std::vector<unsigned long> elem; /*!< \brief Boundary connectivity, nDim points per element (lines or triangles). */

    unsigned long GetnElem(unsigned short nDim) const { return elem.size() / nDim; }
  };

  unsigned short nDim = 0;            /*!< \brief Number of dimensions (2 or 3). */
  std::vector<passivedouble> coord;   /*!< \brief Point coordinates, nPoint x nDim. */
  std::vector<passivedouble> metric;  /*!< \brief Metric at the points, nPoint x nMetric, upper triangle (may be empty). */
  std::vector<unsigned long> elem;    /*!< \brief Volume connectivity, nElem x (nDim+1). */
  std::vector<int> elemRef;           /*!< \brief Reference of each volume element. */
  std::vector<Marker> markers;        /*!< \brief Boundary elements per physical marker. */

  /*! \brief Number of metric components, (xx,xy,yy) in 2D, (xx,xy,xz,yy,yz,zz) in 3D. */
  static unsigned short GetnMetric(unsigned short nDim) { return nDim * (nDim + 1) / 2; }

  unsigned long GetnPoint() const { return nDim ? coord.size() / nDim : 0; }
  unsigned long GetnElem() const { return nDim ? elem.size() / (nDim + 1) : 0; }

  /*!
   * \brief Find a marker by its physical name.
   * \return Pointer to the marker, nullptr if it does not exist.
   */
  const Marker* FindMarker(const std::string& name) const {
    for (const auto& marker : markers)
      if (marker.name == name) return &marker;
    return nullptr;
  }
};

/*!
 * \class CMMGInterface
 * \brief Remeshing of a simplex mesh with the MMG library (MMG2D for triangles, MMG3D for tetrahedra), in memory.
 * \note First milestone: one MPI rank only. The extraction works on the local mesh of the rank and is the place
 *       where a gather on rank 0 will be added. Without HAVE_MMG, only the extraction and validation are available.
 *
 * Typical use:
 * \code
 *   CMMGInterface::CheckSupport(config, geometry);
 *   const auto mesh = CMMGInterface::ExtractMesh(config, geometry, solver->GetNodes()->GetMetric());
 *   CMMGInterface mmg(config);
 *   const auto adapted = mmg.Adapt(mesh);
 * \endcode
 */
class CMMGInterface {
 public:
  /*!
   * \brief Remeshing parameters.
   */
  struct Parameters {
    passivedouble hmin = 1e-8;  /*!< \brief Minimum edge size (ADAP_HMIN). */
    passivedouble hmax = 10.0;  /*!< \brief Maximum edge size (ADAP_HMAX). */
    passivedouble hgrad = 1.3;  /*!< \brief Size gradation (ADAP_HGRAD). */
    passivedouble hausd = 0.01; /*!< \brief Hausdorff distance of the boundaries (ADAP_HAUSD). */
    passivedouble angle = 45.0; /*!< \brief Sharp angle detection threshold in degrees (ADAP_ANGLE). */
    int verbosity = -1;         /*!< \brief MMG verbosity, -1 silent, 1 MMG default, up to 10. */
  };

  /*!
   * \brief MMG return status (same values as MMG5_SUCCESS, MMG5_LOWFAILURE, MMG5_STRONGFAILURE).
   */
  enum class Status { SUCCESS = 0, LOWFAILURE = 1, STRONGFAILURE = 2 };

  /*!
   * \brief Constructor, takes the parameters from ADAP_HMIN, ADAP_HMAX, ADAP_HGRAD, ADAP_HAUSD and ADAP_ANGLE.
   * \note Stops with an error if SU2 was built without MMG.
   */
  explicit CMMGInterface(const CConfig& config);

  /*!
   * \brief Destructor, frees the MMG structures.
   */
  ~CMMGInterface();

  CMMGInterface(const CMMGInterface&) = delete;
  CMMGInterface& operator=(const CMMGInterface&) = delete;

  /*!
   * \brief Get the remeshing parameters.
   */
  Parameters& GetParameters() { return params; }
  const Parameters& GetParameters() const { return params; }

  /*!
   * \brief Stop with an error for cases that the adaptation does not support (yet).
   * \note More than one MPI rank, multizone and sliding meshes, periodic boundaries, moving or deforming meshes,
   *       FEM solvers, paired boundaries (actuator disk, near field, fluid interface), halo points and
   *       send/receive markers. Non-simplex elements are rejected by ExtractMesh.
   * \param[in] config - Definition of the problem.
   * \param[in] geometry - Geometry of the zone (finest grid).
   */
  static void CheckSupport(const CConfig& config, const CGeometry& geometry);

  /*!
   * \brief Copy the mesh and the metric of the local (single-rank) geometry into plain arrays.
   * \note Elements are reoriented to a positive volume if needed (zero volume is an error), the metric must be
   *       finite and positive definite, all elements must be triangles (2D) or tetrahedra (3D).
   *       Marker references are 1 + the index of the marker in the configuration file, which does not depend
   *       on the partition (MARKER_* tags, not the MPI-local marker indices).
   * \param[in] config - Definition of the problem.
   * \param[in] geometry - Geometry of the zone (finest grid).
   * \param[in] metric - Metric at the points, nPoint x nMetric, upper triangle (e.g. CVariable::GetMetric()).
   * \return The mesh with its metric.
   */
  static CSimplexMesh ExtractMesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric);

  /*!
   * \brief Check that a mesh is valid for SU2: all points used, positive volumes, every boundary face of the
   *        volume mesh belongs to exactly one marker, every marker element is a face of the mesh.
   * \param[in] mesh - Mesh to check.
   * \param[in] reference - If given, every marker with elements in the reference must have elements in the mesh.
   * \param[in] what - Description of the mesh used in the error messages.
   */
  static void ValidateMesh(const CSimplexMesh& mesh, const CSimplexMesh* reference, const std::string& what);

  /*!
   * \brief Load a mesh and its metric into MMG (MMG2D or MMG3D depending on the dimension).
   * \note Indices are converted to MMG's 1-based numbering. Points shared by two or more markers are set as
   *       required corners so that the extent of each marker is kept.
   */
  void SetMesh(const CSimplexMesh& mesh);

  /*!
   * \brief Run MMG with the current parameters.
   * \note SUCCESS is accepted. LOWFAILURE (MMG kept a valid mesh but could not fully adapt it) is accepted with a
   *       warning, the caller must validate the result (Adapt does). STRONGFAILURE or any other value stops
   *       with an error.
   * \return MMG status.
   */
  Status Remesh();

  /*!
   * \brief Copy the MMG mesh (and its metric) back into plain arrays, with SU2's 0-based numbering.
   * \note Boundary elements are grouped by marker name. MMG boundary elements whose reference is not one of the
   *       loaded markers are an error (MMG created a boundary that has no SU2 marker).
   */
  CSimplexMesh GetMesh() const;

  /*!
   * \brief Remesh: SetMesh, Remesh, GetMesh and ValidateMesh against the input.
   * \param[in] mesh - Input mesh with its metric.
   * \return Adapted mesh with the metric interpolated by MMG.
   */
  CSimplexMesh Adapt(const CSimplexMesh& mesh);

  /*!
   * \brief Save the current MMG mesh (and metric) in Medit format (.mesh, .sol), for inspection.
   */
  void SaveMesh(const std::string& filename) const;

 private:
  struct MMGData;                     /*!< \brief MMG structures (only defined with HAVE_MMG). */
  std::unique_ptr<MMGData> mmg;       /*!< \brief MMG mesh and metric. */
  Parameters params;                  /*!< \brief Remeshing parameters. */
  std::vector<CSimplexMesh::Marker> markerInfo; /*!< \brief Names and references of the loaded markers. */
  unsigned short nDim = 0;            /*!< \brief Dimension of the loaded mesh. */
};
