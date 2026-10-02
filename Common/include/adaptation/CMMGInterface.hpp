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

#include "../containers/C2DContainer.hpp"
#include "CSimplexMesh.hpp"
#include "CRemesher.hpp"

class CConfig;
class CGeometry;

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
    bool surface = true;        /*!< \brief MMG may change the boundary mesh (ADAP_SURFACE), else -nosurf,
                                     -nosizreq (the metric is kept at the fixed boundary points) and -hgradreq -1
                                     (no extra gradation from them, ADAP_HGRAD still applies). */
    int verbosity = -1;         /*!< \brief MMG verbosity, -1 silent, 1 MMG default, up to 10. */
  };

  /*!
   * \brief MMG return status (same values as MMG5_SUCCESS, MMG5_LOWFAILURE, MMG5_STRONGFAILURE).
   */
  enum class Status { SUCCESS = 0, LOWFAILURE = 1, STRONGFAILURE = 2 };

  /*!
   * \brief Constructor, takes the parameters from ADAP_HMIN, ADAP_HMAX, ADAP_HGRAD, ADAP_HAUSD, ADAP_ANGLE and
   *        ADAP_SURFACE.
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
   *       FEM solvers, MARKER_CREATE_COPY, paired boundaries (actuator disk, near field, fluid interface), halo
   *       points and send/receive markers. Non-simplex elements are rejected by ExtractMesh.
   * \param[in] config - Definition of the problem.
   * \param[in] geometry - Geometry of the zone (finest grid).
   */
  static void CheckSupport(const CConfig& config, const CGeometry& geometry);

  /*!
   * \brief MMG reference of a marker: 1 + the position of its name in the alphabetical list of the marker names
   *        of the configuration file (MARKER_* tags).
   * \note Depends on the names only, not on the order of the markers in the config or mesh file, nor on the
   *       partition (MPI-local marker indices). Stops with an error if the config does not define the marker.
   * \param[in] config - Definition of the problem.
   * \param[in] name - Marker name.
   * \return Reference (> 0).
   */
  static int GetMarkerReference(const CConfig& config, const std::string& name);

  /*!
   * \brief Check that a metric tensor is finite and positive definite, and not numerically singular.
   * \note Eigenvalues of the tensor scaled to a unit diagonal: the smallest must exceed 1e-14 times the largest.
   *       Metrics with an aspect ratio up to about 1e6 (the ADAP_ARMAX default) are accepted in any orientation.
   * \param[in] nDim - Number of dimensions.
   * \param[in] metric - Upper triangle, (xx,xy,yy) in 2D, (xx,xy,xz,yy,yz,zz) in 3D.
   * \return Whether the metric is valid.
   */
  static bool IsFinitePositiveDefinite(unsigned short nDim, const passivedouble* metric);

  /*!
   * \brief Make the metric compatible with fixed boundary faces (ADAP_SURFACE= NO): at each boundary point, for each
   *        boundary edge e at it that is longer than 1 in the metric, the metric becomes the union of the metric and
   *        the size |e| along e (sizes along e raised to |e|, the sizes across e kept: the size tensors M^-1 and e e^T
   *        combined by simultaneous reduction). MMG cannot split the fixed edges; with a finer metric along them (kept
   *        at the boundary points with nosizreq) it places nodes very close to them. On a curved boundary the faces
   *        are chords inclined to the tangent by half the turn, so a boundary-layer normal size at the points is
   *        raised to about |e| sin(turn / 2).
   * \param[in,out] mesh - Mesh with its metric; only the metric of the boundary points changes.
   */
  static void FloorFixedBoundaryMetric(CSimplexMesh& mesh);

  /*!
   * \brief Copy the mesh and the metric of the local (single-rank) geometry into plain arrays.
   * \note Elements are reoriented to a positive volume if needed (zero volume is an error), the metric must be
   *       finite and positive definite, all elements must be triangles (2D) or tetrahedra (3D).
   *       Marker references are given by GetMarkerReference.
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
   * \brief Check that a mesh has the boundary of a reference mesh: the same boundary points (bitwise equal
   *        coordinates) and, for each marker, the same boundary faces, up to the numbering of the points and faces.
   * \note Used after remeshing with a fixed surface (ADAP_SURFACE= NO); stops with an error otherwise.
   * \param[in] mesh - Mesh to check.
   * \param[in] reference - Mesh whose boundary must be kept.
   * \param[in] what - Description of the mesh used in the error messages.
   */
  static void CheckSameBoundary(const CSimplexMesh& mesh, const CSimplexMesh& reference, const std::string& what);

  /*!
   * \brief Load a mesh and its metric into MMG (MMG2D or MMG3D depending on the dimension).
   * \note Indices are converted to MMG's 1-based numbering. Points shared by two or more markers are set as
   *       required corners so that the extent of each marker is kept. With a fixed surface (Parameters::surface
   *       false) the boundary points get their index + 1 as MMG reference, and GetMesh restores their exact
   *       coordinates (MMG scales the mesh to a unit box and back, which changes them by round-off).
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
   * \brief Status of the last Remesh (STRONGFAILURE before the first one).
   */
  Status GetStatus() const { return status; }

  /*!
   * \brief Copy the MMG mesh (and its metric) back into plain arrays, with SU2's 0-based numbering.
   * \note Boundary elements are grouped by marker name. MMG boundary elements whose reference is not one of the
   *       loaded markers are an error (MMG created a boundary that has no SU2 marker).
   */
  CSimplexMesh GetMesh() const;

  /*!
   * \brief Remesh: SetMesh, Remesh, GetMesh and ValidateMesh against the input; with a fixed surface
   *        (Parameters::surface false) also CheckSameBoundary.
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
  Status status = Status::STRONGFAILURE; /*!< \brief Status of the last Remesh. */
  std::vector<bool> fixedBoundary;     /*!< \brief Fixed surface: boundary points of the loaded mesh. */
  std::vector<passivedouble> fixedCoord; /*!< \brief Fixed surface: coordinates of the loaded mesh. */
};

/*!
 * \class CMMGRemesher
 * \brief Remesher of the adaptation loop with serial MMG: CMMGInterface::ExtractMesh, then CMMGInterface::Adapt with
 *        the remeshing parameters of the config at the time of the call.
 */
class CMMGRemesher final : public CRemesher {
 public:
  /*!
   * \brief Constructor, stops with an error if SU2 was built without MMG.
   */
  CMMGRemesher();

  CSimplexMesh Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) override;
};
