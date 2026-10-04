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
 * \note MPI: the extraction gathers the partitioned mesh and metric on the master rank (CMeshGather), MMG runs there
 *       (serial), and CMMGRemesher sends each rank its reader slices of the adapted mesh (CReaderSlices). Without HAVE_MMG, only the extraction and
 *       validation are available.
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
    bool boundaryLayer = false; /*!< \brief The metric has a boundary-layer part (ADAP_BL_MARKER): no edge swaps in 2D
                                     (MMG2D's swaps leave degenerate cells in a re-adapted boundary-layer mesh;
                                     in 3D the swaps are kept, without them there are more slivers). */
    int verbosity = -1;         /*!< \brief MMG verbosity, -1 silent, 1 MMG default, up to 10. */
    std::vector<std::string> boundaryLayerMarkers; /*!< \brief Wall markers of the boundary-layer metric
                                                        (ADAP_BL_MARKER). */
    bool localWallHmax = true;  /*!< \brief Local MMG parameters on the boundary-layer wall markers
                                     (ADAP_BL_LOCAL_HMAX): hmax = 2 x the longest face edge of the marker, which bounds
                                     MMG's own geometric metric at their points (see MetricCheck). */
    int swap = -1;              /*!< \brief Edge swaps in 2D: -1 automatic (none with a boundary-layer metric), 0 none,
                                     1 swaps. */
    std::vector<std::string> requiredMarkers; /*!< \brief With a surface (surface true): markers whose boundary edges
                                                   (2D) or triangles (3D) are required, i.e. kept as they are. */
    std::vector<unsigned long> requiredPoints; /*!< \brief Extra required corner points (indices of the input mesh). */
  };

  /*!
   * \brief Local MMG size parameters of one boundary reference (MMG2D/3D_Set_localParameter on its edges/triangles).
   */
  struct LocalParameter {
    int ref = 0;                /*!< \brief Marker reference. */
    passivedouble hmin = 0.0;   /*!< \brief Local minimum size. */
    passivedouble hmax = 0.0;   /*!< \brief Local maximum size. */
    passivedouble hausd = 0.0;  /*!< \brief Local Hausdorff distance. */
  };

  /*!
   * \brief Check that MMG kept the metric at the fixed boundary points (ADAP_SURFACE= NO).
   * \details MMG intersects the given metric at boundary points with its own geometric boundary metric. The result may
   *          legitimately be finer than the given metric (intersection, gradation), never coarser. With MMG 5.6 the
   *          intersection can fail silently when the two metrics differ by a ratio of about 1e5 in size (MMG issue
   *          #331, fixed by PR #332); the point then keeps MMG's geometric metric, e.g. an isotropic hmax on a straight
   *          wall, and the boundary-layer target there is lost. A point violates the check when
   *          lambda_min(M_in^-1/2 M_out M_in^-1/2) < 1 - tolerance (some direction became coarser).
   */
  struct MetricCheck {
    unsigned long nChecked = 0;       /*!< \brief Fixed boundary points compared. */
    unsigned long nViolations = 0;    /*!< \brief Points where MMG's metric is coarser than the given one. */
    unsigned long nCornerViolations = 0; /*!< \brief Of which corners (points of two or more markers). */
    passivedouble worstRatio = 1.0;   /*!< \brief Smallest lambda_min found (1: nothing coarser). */
    std::vector<unsigned long> points;/*!< \brief Input indices of the violating points. */
  };
  static constexpr passivedouble metricCheckTolerance = 0.05; /*!< \brief Tolerance of MetricCheck (eigenvalue
                                                                 ratio; about 2.5% in size). */

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
   * \note Multizone and sliding meshes, periodic boundaries, moving or deforming meshes, FEM solvers,
   *       MARKER_CREATE_COPY, paired boundaries (actuator disk, near field, fluid interface). Any number of MPI ranks
   *       (the halo points and send/receive markers of the partition are left out by the gather). Non-simplex elements
   *       are rejected by ExtractMesh.
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
   * \brief Smallest eigenvalue of M_in^-1/2 M_out M_in^-1/2: below 1, M_out is coarser than M_in in some direction.
   * \param[in] nDim - Number of dimensions.
   * \param[in] metricIn - Reference metric (upper triangle).
   * \param[in] metricOut - Metric to compare (upper triangle).
   */
  static passivedouble CoarseningRatio(unsigned short nDim, const passivedouble* metricIn,
                                       const passivedouble* metricOut);

  /*!
   * \brief Local parameters of the boundary-layer wall markers of a mesh (Parameters::localWallHmax): for each marker
   *        of Parameters::boundaryLayerMarkers with faces, hmin and hausd of the parameters, hmax = 2 x its longest
   *        face edge.
   */
  std::vector<LocalParameter> WallLocalParameters(const CSimplexMesh& mesh) const;

  /*!
   * \brief Local parameters to use instead of WallLocalParameters for the next SetMesh (empty: WallLocalParameters).
   */
  void SetLocalParameters(std::vector<LocalParameter> local) { localOverride = std::move(local); }

  /*!
   * \brief Result of the metric check of the last Adapt with a fixed surface.
   */
  const MetricCheck& GetMetricCheck() const { return metricCheck; }

  /*!
   * \brief Collective: the whole mesh and metric of the (partitioned) geometry as plain arrays on the master rank, in
   *        the global numbering of the mesh (CMeshGather: the same arrays for any number of ranks).
   * \note Elements are reoriented to a positive volume if needed (zero volume is an error), the metric must be
   *       finite and positive definite (checked by the rank of each point), all elements must be triangles (2D) or
   *       tetrahedra (3D). Markers in the order of the config file, references given by GetMarkerReference. The other
   *       ranks get the dimension and the marker names only.
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
  std::vector<LocalParameter> localParams; /*!< \brief Local parameters of the loaded mesh, set by Remesh. */
  std::vector<LocalParameter> localOverride; /*!< \brief Local parameters given by SetLocalParameters. */
  mutable std::vector<long> fixedSource; /*!< \brief Fixed surface: input index of each point of the last GetMesh
                                              (-1 for new points). */
  MetricCheck metricCheck;               /*!< \brief Metric check of the last Adapt. */
};

/*!
 * \class CMMGRemesher
 * \brief Remesher of the adaptation loop with serial MMG: CMMGInterface::ExtractMesh (gather on the master rank), then
 *        CMMGInterface::Adapt on the master rank with the remeshing parameters of the config at the time of the call,
 *        then each rank receives its reader slices of the adapted mesh (CReaderSlices::FromComplete: the points of its
 *        linear partition with MMG's metric, the elements touching them, the boundary elements on the master rank),
 *        exactly the arrays a reader of the complete mesh would hold on that rank. Memory: the master rank holds the
 *        gathered mesh, MMG's structures and the adapted mesh; the other ranks hold only their slices.
 */
class CMMGRemesher final : public CRemesher {
 public:
  /*!
   * \brief Constructor, stops with an error if SU2 was built without MMG.
   */
  CMMGRemesher();

  CRemeshResult Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) override;
};
