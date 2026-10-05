/*!
 * \file CBoundaryLayerRemesher.hpp
 * \brief Two-pass remesh for boundary-layer walls (ADAP_BL_METHOD= TWO_PASS, 2D).
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

#include <array>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../../../Common/include/adaptation/CRemesher.hpp"
#include "../../../Common/include/adaptation/CSimplexMesh.hpp"
#include "CBarycentricTransfer.hpp"
#include "CReferenceWall.hpp"

class CConfig;

/*!
 * \class CBoundaryLayerRemesher
 * \brief Remesher of ADAP_BL_METHOD= TWO_PASS (2D; SERIAL_BL_FIX_PLAN.md 2.1). The metric it receives is the sensor
 *        metric without the boundary-layer part (CSolver::ComputeMetric with boundaryLayer false); everything else runs
 *        on the master rank on the gathered mesh:
 *  - Reference wall (SERIAL_BL_FIX_PLAN.md 2.2): fitted to the current wall and written to ADAP_BL_REFERENCE on the
 *    first remesh of a fresh run (no RESTART_SOL) or with ADAP_BL_REFERENCE_REBASE= YES; otherwise (restart) read from
 *    it, and a missing, malformed or foreign file, or one farther than 10 x ADAP_BL_GEOM_TOL from the wall, is an error
 *    (never refitted silently). Later remeshes of the same run use the reference kept in memory (the same knots).
 *  - Wall size t_w along the reference: min(smallest tangential extent of the sensor metric at x + d n, d = 0.1 to 1
 *    x the thickness, c sqrt(2 R h0), ADAP_HMAX), at least
 *    max(ADAP_HMIN, 2 h0), graded along the wall and into sharp corners. It does not depend on the current wall edges.
 *  - Pass A (ADAP_SURFACE= YES): near the boundary-layer walls (distance < thickness) the metric is isotropic, of size
 *    t_w + 0.25 d (the sensor is not used there: inside a resolved boundary layer it is fine in all directions and
 *    would keep the band fine), t_w at the wall points; elsewhere the sensor metric. MMG
 *    adapts the boundary-layer walls (the other markers are required), with swaps; then the new wall points are
 *    projected onto their own feature segment of the reference (sweeps of partial moves that keep every cell valid).
 *    Gates (SERIAL_BL_FIX_PLAN.md 2.1.5): G1 the other boundaries unchanged (bijective point map, same edges, required
 *    corners kept, same wall chains between the same corners), G2 every wall line with L sin(turn/2) <=
 *    ADAP_BL_GATE_FACTOR h0, G3 every wall point within ADAP_BL_GEOM_TOL of its segment, in order, and the reference arc
 *    of every wall line within ADAP_BL_GEOM_TOL of the line (guaranteed bound), G4 every triangle positive (robust
 *    predicate, after the last coordinate change), G5 every wall line away from corners at most 2 t_w. A failed G1 falls
 *    back at once to the one-pass remesh of the input mesh; a failed G2-G5 is retried once with t_w x 0.7 near the
 *    failures and half the Hausdorff distance, and a second failure falls back. With ADAP_SURFACE= NO pass A only
 *    resets the near-wall band (fixed boundary) and G2 is reported, not enforced.
 *  - Pass B: the sensor metric interpolated to the pass-A mesh (log-Euclidean, barycentric; boundary points outside
 *    the input mesh from the closest face of their own markers), the bounds ADAP_HMIN,
 *    ADAP_HMAX, ADAP_ARMAX, then the boundary-layer metric of the pass-A walls; MMG with the fixed surface (floor,
 *    nosizreq), edge swaps if ADAP_BL_SWAP= YES.
 */
class CBoundaryLayerRemesher final : public CRemesher {
 public:
  /*!
   * \brief What the two passes did (master rank).
   */
  struct Report {
    bool passA = false;                 /*!< \brief Pass A was accepted (else the one-pass fallback was used). */
    unsigned short attempts = 0;        /*!< \brief Pass-A attempts. */
    std::string failedGate;             /*!< \brief Last failed gate (empty if none). */
    bool immediateFallback = false;     /*!< \brief G1 failed: fallback without retry. */
    unsigned long nWallPointA = 0;      /*!< \brief Boundary-layer wall points after pass A. */
    unsigned long nProjected = 0;       /*!< \brief Wall points moved onto the reference. */
    passivedouble maxResidual = 0.0;    /*!< \brief Largest distance of a wall point from its reference segment. */
    passivedouble maxArcDeviation = 0.0;/*!< \brief Largest G3 bound of the reference arc from a wall line. */
    unsigned long nG2 = 0, nG3 = 0, nG4 = 0, nG5 = 0; /*!< \brief Failures of the gates G2-G5 (last attempt). */
    passivedouble maxSizeRatio = 0.0;   /*!< \brief Largest L / t_w of the wall lines checked by G5. */
    unsigned long nUndersized = 0;      /*!< \brief Wall lines checked by G5 with L / t_w < 0.5 (reported). */
    passivedouble maxExtentRatio = 0.0; /*!< \brief Largest L sin(turn/2) / h0 of the final wall lines. */
    unsigned long nExtentAbove = 0;     /*!< \brief Final wall lines with L sin(turn/2) > 1.1 h0 (h0_eff > 1.1 h0). */
    unsigned long nSizeConflict = 0;    /*!< \brief Wall samples where max(ADAP_HMIN, 2 h0) exceeds the curvature cap. */
    unsigned long nFloorRaised = 0;     /*!< \brief Wall points of pass B whose normal size the floor raised > 1.1x. */
    passivedouble maxFloorRatio = 1.0;  /*!< \brief Largest ratio of the floored to the given wall-normal size. */
    unsigned long nSensorLoss = 0;      /*!< \brief Band points of pass B where the sensor metric is finer than the
                                             boundary-layer normal size in the wall-normal direction (by > 2.5%). */
    unsigned long nMetricChecked = 0, nMetricViolations = 0, nMetricCornerViolations = 0; /*!< \brief MetricCheck of
                                             the last MMG call (fixed boundary points checked, coarser, corners). */
    passivedouble metricWorstRatio = 1.0; /*!< \brief Smallest eigenvalue ratio of MetricCheck. */
    unsigned long nPointA = 0, nPointB = 0; /*!< \brief Points after pass A and pass B. */
    unsigned long nCorner = 0, nConvexCorner = 0; /*!< \brief Sharp corners of the reference, convex ones. */
    unsigned long nCornerUnmatched = 0; /*!< \brief Corners without an input mesh point (no floor). */
    unsigned long nCornerSymmetry = 0;  /*!< \brief Wall size samples lowered by the corner symmetry. */
    unsigned long nCornerFloorRaised = 0; /*!< \brief Wall size samples raised by a convex-corner floor. */
    passivedouble maxCornerFloorRatio = 1.0; /*!< \brief Largest raise factor of the convex-corner floor. */
    unsigned long nCornerFace = 0;      /*!< \brief Final wall faces within 2 t_c of a convex corner. */
    unsigned long nCornerFaceOut = 0;   /*!< \brief Those with a first cell outside [0.5, 2] h0. */
    passivedouble cornerFaceMin = 0.0, cornerFaceMax = 0.0; /*!< \brief Their smallest and largest first cell / h0. */
    unsigned long nCornerEdgeRatio = 0; /*!< \brief Sharp corners whose two final wall edges differ by more than 1.2x. */
    passivedouble maxCornerEdgeRatio = 1.0; /*!< \brief Largest ratio of the two final wall edges at a sharp corner. */
    std::vector<BLWallRule::Corner> corners; /*!< \brief Sharp corners of the reference (classified). */
    std::vector<std::string> nearWallMarker; /*!< \brief Boundary-layer markers of the first-node counts. */
    std::vector<unsigned long> nFirstNode;   /*!< \brief Wall points with a neighbour off the marker (final mesh). */
    std::vector<unsigned long> nFirstNodeBelow, nFirstNodeAbove; /*!< \brief Of those: the nearest such neighbour's
                                                  wall distance below 0.5 h0, above 2 h0 (the window [0.5, 2] h0). */
  };

  /*!
   * \brief Ordered points of a wall chain between two break points (or a closed loop without them: first point
   *        repeated at the end), with the reference segment it belongs to.
   */
  struct WallPiece {
    std::vector<unsigned long> points; /*!< \brief Points in chain order. */
    bool closed = false;               /*!< \brief Closed loop without break points. */
    long segment = -1;                 /*!< \brief Reference segment (-1: none). */
  };

  /*!
   * \brief Test hook of TwoPass: changes the pass-A mesh right after MMG (attempt 0 or 1), to inject gate failures.
   */
  using PassAHook = std::function<void(CSimplexMesh& meshA, unsigned short attempt)>;

  /*!
   * \brief Collective: gather, the two passes on the master rank, then the reader slices of every rank.
   */
  CRemeshResult Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) override;

  /*!
   * \brief The two passes on a complete mesh with its sensor metric (serial).
   * \param[in] config - Remeshing and boundary-layer options.
   * \param[in] mesh - Mesh with the sensor metric (no boundary-layer part).
   * \param[in] reference - Reference wall of the boundary-layer markers.
   * \param[out] report - What was done.
   * \param[in] hook - Test hook (see PassAHook).
   * \return The adapted mesh.
   */
  static CSimplexMesh TwoPass(const CConfig& config, const CSimplexMesh& mesh, const CReferenceWall& reference,
                              Report& report, const PassAHook& hook = nullptr);

  /*!
   * \brief Reference wall of a remesh (SERIAL_BL_FIX_PLAN.md 2.2, master rank).
   * \param[in] config - ADAP_BL_REFERENCE, ADAP_ANGLE, ADAP_BL_GEOM_TOL, boundary-layer markers.
   * \param[in] mesh - Current mesh.
   * \param[in] create - Fit the reference to the current wall and write it (first remesh of a fresh run, or rebase);
   *            otherwise read it and check it against the current wall.
   * \param[out] reference - The reference.
   * \param[out] info - What was done (for the log).
   * \return Empty, or the error message (the caller stops with it).
   */
  static std::string PrepareReference(const CConfig& config, const CSimplexMesh& mesh, bool create,
                                      CReferenceWall& reference, std::string& info);

  /*!
   * \brief Whether the first remesh of this run creates the reference wall (no RESTART_SOL, or
   *        ADAP_BL_REFERENCE_REBASE= YES) instead of reading it.
   */
  static bool CreatesReference(const CConfig& config);

  /*!
   * \brief Check that a reference belongs to the current wall: every boundary-layer marker has segments, the corner
   *        angle is ADAP_ANGLE, the wall points lie within max(10 ADAP_BL_GEOM_TOL, 1e-9 x domain size) of it.
   * \return Empty, or the error message.
   */
  static std::string CheckReference(const CConfig& config, const CSimplexMesh& mesh, const CReferenceWall& reference,
                                    passivedouble* distance = nullptr);

  /*!
   * \brief Gate G1 (SERIAL_BL_FIX_PLAN.md 2.1.5): every marker that is not a boundary-layer wall maps bijectively onto
   *        the input (each output point within tol of exactly one input point, no input point used twice) with the
   *        same edges; every required corner of the input is a point of the output; every boundary-layer marker has the
   *        same chains between the same corners. A match is the nearest point within tol, at least 3 times closer
   *        than the second nearest (an exact match of finely spaced points is always unique). The matched points get
   *        their exact input coordinates.
   * \param[in] in - Input mesh.
   * \param[in,out] out - Pass-A mesh.
   * \param[in] blMarkers - Boundary-layer wall markers.
   * \param[in] required - Required corner points (input indices).
   * \param[in] tol - Matching distance.
   * \param[out] breaks - Output indices of the required corners.
   * \return Empty if the gate holds, else the reason.
   */
  static std::string CheckOtherBoundaries(const CSimplexMesh& in, CSimplexMesh& out,
                                          const std::vector<std::string>& blMarkers,
                                          const std::vector<unsigned long>& required, passivedouble tol,
                                          std::set<unsigned long>& breaks);

  /*!
   * \brief Pieces of the chains of a wall marker, split at break points (and at points of degree other than 2).
   */
  static std::vector<WallPiece> WallPieces(const CSimplexMesh& mesh, const std::string& marker,
                                           const std::set<unsigned long>& breaks);

  /*!
   * \brief Own reference segment of each piece: the segment most edge midpoints of the piece project onto.
   */
  static void AssignSegments(const CSimplexMesh& mesh, const CReferenceWall& reference, const std::string& marker,
                             std::vector<WallPiece>& pieces);

  /*!
   * \brief Gates G3 and G5 on the lines of wall pieces (G3: end points within geomTol of the own segment, parameters in
   *        order along the piece, guaranteed bound of the arc between them from the line <= geomTol, and the pieces of
   *        the marker cover each of its reference segments exactly once; G5: lines farther than one t_w from the ends
   *        of an open piece have L / t_w(midpoint) <= 2).
   * \param[in] sizeAt - t_w at a parameter of a segment (nullptr: G5 not checked).
   * \param[in,out] report - nG3, nG5, maxResidual, maxArcDeviation, maxSizeRatio, nUndersized (accumulated).
   * \param[out] failures - Positions of the failures (for the retry).
   */
  static void CheckPieces(const CSimplexMesh& mesh, const CReferenceWall& reference, const std::string& marker,
                          const std::vector<WallPiece>& pieces, passivedouble geomTol,
                          const std::function<passivedouble(long, passivedouble)>& sizeAt, Report& report,
                          std::vector<std::array<passivedouble, 2>>& failures);

  /*!
   * \brief Gate G4: triangles that are not positively oriented (robust predicate); their centroids in failures.
   */
  static unsigned long CountInverted(const CSimplexMesh& mesh, std::vector<std::array<passivedouble, 2>>* failures);

  /*!
   * \brief Donor stencil of a pass-A point: the element of the input mesh that contains it; a point outside the input
   *        mesh that lies on markers gets the closest face of those markers (same-marker fallback), any other point the
   *        closest boundary face.
   */
  static CBarycentricLocator::Stencil DonorStencil(CBarycentricLocator& locator, const su2double* x,
                                                   const std::vector<std::string>& markers);

  /*!
   * \brief Fluid-side angle (radians) of a mesh point: the sum of the angles at it of its triangles.
   */
  static passivedouble FluidAngle(const CSimplexMesh& mesh, unsigned long point);

  /*!
   * \brief Sharp corners of the reference with their convexity (from the angles of the input mesh at the corner
   *        point), wedge angle, first height and convex-corner floor (SERIAL_BL_FIX_PLAN.md 11.7).
   * \param[in] config - ADAP_BL_* (first heights), ADAP_HMAX.
   * \param[in] mesh - Input mesh (2D).
   * \param[in] reference - Reference wall.
   * \param[out] nUnmatched - Corners without a point of the input mesh at their position (kept concave: no floor).
   */
  static std::vector<BLWallRule::Corner> Corners(const CConfig& config, const CSimplexMesh& mesh,
                                                 const CReferenceWall& reference, unsigned long& nUnmatched);

  /*!
   * \brief Report of the corners on a final mesh: first cells of the boundary-layer wall faces within 2 t_c of each
   *        convex corner, and the ratio of the two wall edges at every sharp corner.
   */
  /*!
   * \brief First off-wall node of every wall point of each boundary-layer marker (final mesh): the smallest distance
   *        to the marker of its edge neighbours that are not on the marker, against the window [0.5, 2] h0.
   */
  static void MeasureNearWall(const CSimplexMesh& mesh, const CConfig& config, Report& report);

  static void MeasureCorners(const CSimplexMesh& mesh, const std::vector<std::string>& blMarkers,
                             const std::vector<BLWallRule::Corner>& corners, Report& report);

  /*!
   * \brief Pass B alone: boundary-layer metric on the mesh (whose metric is the sensor metric), then MMG with the
   *        fixed surface. Also the fallback of TwoPass.
   */
  static CSimplexMesh BoundaryLayerPass(const CConfig& config, const CSimplexMesh& mesh, Report& report);

  /*!
   * \brief Log-Euclidean weighted mean of metrics (upper triangles): exp(sum w log M).
   */
  static void LogEuclideanMean(unsigned short nDim, unsigned short n, const passivedouble* const* metrics,
                               const passivedouble* weights, passivedouble* result);

  /*!
   * \brief Bounds of ComputeMetric applied to one metric: eigenvalues in [1/hmax^2, 1/hmin^2], aspect ratio at most
   *        ARMAX (the smaller eigenvalues raised).
   */
  static void BoundMetric(unsigned short nDim, passivedouble hmin, passivedouble hmax, passivedouble armax,
                          passivedouble* metric);

 private:
  std::unique_ptr<CReferenceWall> reference; /*!< \brief Reference wall of this run (master rank, after the first
                                                  remesh). */
};
