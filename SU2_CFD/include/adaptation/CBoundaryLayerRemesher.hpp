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

#include <string>
#include <vector>

#include "../../../Common/include/adaptation/CRemesher.hpp"
#include "../../../Common/include/adaptation/CSimplexMesh.hpp"
#include "CReferenceWall.hpp"

class CConfig;

/*!
 * \class CBoundaryLayerRemesher
 * \brief Remesher of ADAP_BL_METHOD= TWO_PASS (2D; SERIAL_BL_FIX_PLAN.md 2.1). The metric it receives is the sensor
 *        metric without the boundary-layer part (CSolver::ComputeMetric with boundaryLayer false); everything else runs
 *        on the master rank on the gathered mesh:
 *  - Reference wall: read from ADAP_BL_REFERENCE (it must match the current wall), or fitted to the current wall and
 *    written there if the file does not exist or ADAP_BL_REFERENCE_REBASE= YES.
 *  - Wall size t_w along the reference: min(smallest tangential extent of the sensor metric at x + d n, d = 0.1 to 1
 *    x the thickness, c sqrt(2 R h0), ADAP_HMAX), at least
 *    max(ADAP_HMIN, 2 h0), graded along the wall and into sharp corners. It does not depend on the current wall edges.
 *  - Pass A (ADAP_SURFACE= YES): near the boundary-layer walls (distance < thickness) the metric is isotropic, of size
 *    t_w + 0.25 d (the sensor is not used there: inside a resolved boundary layer it is fine in all directions and
 *    would keep the band fine), t_w at the wall points; elsewhere the sensor metric. MMG
 *    adapts the boundary-layer walls (the other markers are required), with swaps; then the new wall points are
 *    projected onto the reference (sweeps of partial moves that keep every cell valid). Gates: the other boundaries
 *    unchanged, every wall line with L sin(turn/2) <= ADAP_BL_GATE_FACTOR h0, every wall point within ADAP_BL_GEOM_TOL
 *    of the reference. A failed gate is retried once with t_w x 0.7 near the failures; a second failure falls back to
 *    the one-pass remesh of the input mesh. With ADAP_SURFACE= NO pass A only resets the near-wall band (fixed
 *    boundary) and the wall gates are reported, not enforced.
 *  - Pass B: the sensor metric interpolated to the pass-A mesh (log-Euclidean, barycentric), the bounds ADAP_HMIN,
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
    unsigned long nWallPointA = 0;      /*!< \brief Boundary-layer wall points after pass A. */
    unsigned long nProjected = 0;       /*!< \brief Wall points moved onto the reference. */
    passivedouble maxResidual = 0.0;    /*!< \brief Largest distance of a wall point from the reference. */
    passivedouble maxMidpointDeviation = 0.0; /*!< \brief Largest distance of a wall line midpoint from it. */
    passivedouble maxExtentRatio = 0.0; /*!< \brief Largest L sin(turn/2) / h0 of the final wall lines. */
    unsigned long nExtentAbove = 0;     /*!< \brief Final wall lines with L sin(turn/2) > 1.1 h0 (h0_eff > 1.1 h0). */
    unsigned long nSizeConflict = 0;    /*!< \brief Wall samples where max(ADAP_HMIN, 2 h0) exceeds the curvature cap. */
    unsigned long nFloorRaised = 0;     /*!< \brief Wall points of pass B whose normal size the floor raised > 1.1x. */
    unsigned long nPointA = 0, nPointB = 0; /*!< \brief Points after pass A and pass B. */
  };

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
   * \return The adapted mesh.
   */
  static CSimplexMesh TwoPass(const CConfig& config, const CSimplexMesh& mesh, const CReferenceWall& reference,
                              Report& report);

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
};
