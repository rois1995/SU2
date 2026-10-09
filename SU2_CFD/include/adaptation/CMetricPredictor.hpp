/*!
 * \file CMetricPredictor.hpp
 * \brief Prediction of the adaptation metric over the next time window (ADAP_UNSTEADY_METRIC= PREDICT): motion of the
 *        metric features by optical flow on the mesh, transport of the metric along it, intersection over the window.
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
#include <vector>

#include "../../../Common/include/basic_types/datatype_structure.hpp"
#include "../../../Common/include/adaptation/CSimplexMesh.hpp"

class CGeometry;
class CBarycentricLocator;

/*!
 * \class CMetricPredictor
 * \brief Predicts the metric of the next time window from metric snapshots of the current window, on the current
 *        mesh (simplices, serial: with MPI the driver gathers the mesh and the snapshots on one rank). Mesh-native
 *        counterpart of the user's PredictMetric_Aniso_Numba.py (PIV on a raster):
 *        1. Motion field W (displacement per time step) of the scalar invariant s = 0.5 log10(det M) between the
 *           snapshots j and k (k the later one): optical flow on the mesh (MotionField).
 *        2. For each instant t of the horizon (time steps after snapshot k), the metric M_k is moved along W
 *           (semi-Lagrangian: the backward trajectory of each point) and, where it is anisotropic, reoriented and
 *           stretched by the deformation of the motion, M_t = F^-T M_k F^-1 (congruence); its sizes are limited to
 *           [hmin, hmax]; the instants are intersected (Predict).
 *        Three or more snapshots fit temporally filtered feature velocity and acceleration (MotionHistory).
 *        Accelerating transport traces each future endpoint back independently.
 *        The caller scales the result to the target complexity and applies the bounds (CSolver::ComputeMetric).
 * \note Optical flow (MotionField): brightness constancy s_j(x - D) = s_k(x) for the displacement D = W x separation,
 *       solved as a variational (Horn-Schunck type) problem with a normalized data term,
 *         min sum_i V_i w_i (g_i.dD_i - r_i)^2 + l^2 (D + dD)^T K (D + dD) + eps sum_i V_i |D_i + dD_i|^2,
 *       r = s_j(x - D) - s_k(x), g = (grad s_j(x - D) + grad s_k(x)) / 2, w = 1 / (|g|^2 + mu^2), K the P1 stiffness
 *       matrix, V the lumped mass. The weight w makes the data term a displacement along g (in length units) where the
 *       metric varies, and lets it vanish where |grad s| << mu (no information: the field is filled smoothly from the
 *       features, eps lets it decay to zero far from them). Large motions: Gauss-Newton with warping (D updated, s_j
 *       sampled again at x - D; no point moves by more than Ls per step, points sampled outside the domain carry no
 *       data), coarse to fine (both snapshots smoothed by the heat
 *       kernel of lengths 2, 1, 0.5, 0 times the feature width Ls = range of s / |grad s| at the 99th percentile). l = regularization x Ls. Two
 *       starts are solved, zero and the given guess (the flow velocity: metric features are mostly convected), and the
 *       one with the smaller final mismatch is kept; if it does not match better than no motion, the motion is zero. The two snapshots are on the same mesh; with moving meshes, s_j
 *       must be sampled at the same physical points (a locator on the coordinates of snapshot j), the rest is unchanged.
 * \note Transport (Predict): the trajectory of each point is integrated backward with the midpoint rule, from instant
 *       to instant; the deformation of each sub-step is exp(-dt grad W) at the midpoint, so F^-1 = dy/dx is the product
 *       of these exponentials: always invertible, exactly a rotation for a rigid rotation (the linear F = I + t grad W
 *       of the Python stretches a rotated metric by sqrt(1 + (w t)^2)). Points whose trajectory leaves the domain take
 *       the closest boundary point. The metric is interpolated linearly (convex weights: SPD kept).
 */
class CMetricPredictor {
 public:
  /*!
   * \brief Parameters of the prediction.
   */
  struct Options {
    su2double regularization = 0.5; /*!< \brief Smoothness length of the motion in units of the feature width. */
    su2double anisoThreshold = 1.5; /*!< \brief Congruence where sqrt(lambda_max / lambda_min) exceeds it. */
    su2double hmin = 0.0, hmax = 0.0; /*!< \brief Size limits of each instant (0: none). */
  };

  /*!
   * \brief Diagnostics of the motion field.
   */
  struct MotionReport {
    su2double featureLength = 0.0;  /*!< \brief Ls, width of the steepest features of s. */
    su2double smoothLength = 0.0;   /*!< \brief Regularization length l. */
    su2double offset = 0.0;         /*!< \brief Median of s_k - s_j removed before the flow (global factor). */
    su2double mismatchBefore = 0.0; /*!< \brief RMS of s_j - s_k (volume weighted). */
    su2double mismatchAfter = 0.0;  /*!< \brief RMS of s_j(x - D) - s_k(x) with the final motion. */
    su2double mismatchOther = -1.0; /*!< \brief Final mismatch of the start not kept (-1 if only one). */
    bool guessKept = false;         /*!< \brief The start from the guess was kept. */
    bool noMotion = false;          /*!< \brief No start matched better than zero motion: the motion is zero. */
    su2double meanSpeed = 0.0;      /*!< \brief |W| averaged with the weight V |grad s_k|^2 (the features). */
    su2double maxSpeed = 0.0;       /*!< \brief Largest |W|. */
    unsigned long linearIterations = 0; /*!< \brief CG iterations of all solves. */
    bool converged = true;          /*!< \brief All linear solves reached their tolerance. */
  };

  /*! \brief Diagnostics of the temporally filtered feature motion. */
  struct HistoryReport {
    std::vector<MotionReport> pairs;
    su2double constantMismatch = 0, fittedMismatch = 0, maxAcceleration = 0;
    bool accelerationKept = false;
  };

  /*!
   * \brief Diagnostics of the transport.
   */
  struct PredictionReport {
    unsigned long nInstant = 0;    /*!< \brief Instants of the horizon. */
    unsigned long nCongruence = 0; /*!< \brief Point-instants reoriented by the congruence. */
    unsigned long nOutside = 0;    /*!< \brief Points whose trajectory left the domain (at the last instant). */
  };

  /*!
   * \brief Build the operators of the mesh: lumped mass, P1 stiffness, element gradients, point locator.
   * \param[in] geometry - Triangles (2D) or tetrahedra (3D), one rank (no halo points).
   */
  explicit CMetricPredictor(const CGeometry& geometry);

  /*!
   * \brief Build the operators of a mesh given as arrays (with MPI: the whole mesh gathered on one rank).
   * \param[in] mesh - Points, triangles (2D) or tetrahedra (3D), boundary elements (for the closest boundary point).
   */
  explicit CMetricPredictor(const CSimplexMesh& mesh);
  ~CMetricPredictor();

  /*!
   * \brief Invariant 0.5 log10(det M) of a metric stored as its upper triangle (xx, xy, yy) or (xx, xy, xz, yy, yz, zz).
   * \note Computed from the Cholesky factor of the metric scaled to a unit diagonal (no cancellation for anisotropic,
   *       rotated metrics); -150 (det 1e-300) if the metric is not positive definite.
   */
  static su2double Invariant(unsigned short nDim, const su2double* metric);

  /*!
   * \brief Motion of the features of s between two snapshots (displacement per time step, nPoint x nDim).
   * \param[in] sj - Invariant of the earlier snapshot at the points.
   * \param[in] sk - Invariant of the later snapshot at the points.
   * \param[in] separation - Time steps from j to k (> 0).
   * \param[in] guess - If given: a second start (motion per time step, nPoint x nDim), e.g. the flow velocity x dt.
   * \param[in] regularization - Smoothness length in units of the feature width.
   * \param[out] report - Diagnostics.
   * \param[in] fixed - If given: points where the motion is zero (no-slip walls: the features there are attached to
   *            the wall); the smoothness term then takes the motion from zero at the wall to that of the flow.
   */
  std::vector<su2double> MotionField(const std::vector<su2double>& sj, const std::vector<su2double>& sk,
                                     su2double separation, const std::vector<su2double>* guess,
                                     su2double regularization, MotionReport& report,
                                     const std::vector<bool>* fixed = nullptr);

  /*! \brief Fit final feature velocity and acceleration from earlier-to-final optical flows. Times increase,
   *         all invariants share one mesh. Temporal ridge filtering and a history-mismatch fallback reject
   *         acceleration that fails to improve the saved history; two snapshots preserve MotionField exactly.
   *         This is an in-sample safeguard, not a guarantee of future acceleration. Units are time steps.
   */
  std::vector<su2double> MotionHistory(const std::vector<std::vector<su2double>>& snapshots,
                                     const std::vector<su2double>& times, const std::vector<su2double>* guess,
                                     su2double regularization, su2double temporalFilter,
                                     std::vector<su2double>& acceleration, HistoryReport& report,
                                     const std::vector<bool>* fixed = nullptr);

  /*!
   * \brief Metric of snapshot k moved (and reoriented) along the motion to each instant, limited to the size bounds,
   *        intersected over the instants.
   * \param[in] metricK - Metric of snapshot k (nPoint rows of the upper triangle).
   * \param[in] motion - Displacement per time step (nPoint x nDim).
   * \param[in] instants - Time steps after snapshot k, ascending, >= 0.
   * \param[in] options - Threshold and size bounds.
   * \param[out] report - Diagnostics.
   * \param[out] perInstant - If given: the metric of each instant (before the intersection).
   * \param[in] acceleration - Optional feature acceleration, nPoint x nDim. Nonzero acceleration traces each
   *            future endpoint back independently with W(x,t)=motion(x)+t acceleration(x); this costs more than
   *            autonomous transport. Convex metric interpolation and exponential deformation retain SPD.
   * \return The predicted metric (nPoint rows of the upper triangle).
   */
  std::vector<su2double> Predict(const std::vector<su2double>& metricK, const std::vector<su2double>& motion,
                                 const std::vector<su2double>& instants, const Options& options,
                                 PredictionReport& report, std::vector<std::vector<su2double>>* perInstant = nullptr,
                                 const std::vector<su2double>* acceleration = nullptr);

  /*!
   * \brief Nodal gradient of a P1 field (volume-weighted mean of the element gradients), nPoint x nDim.
   */
  std::vector<su2double> Gradient(const std::vector<su2double>& field, unsigned short stride = 1,
                                  unsigned short component = 0) const;

  /*!
   * \brief Heat-kernel smoothing over the length sigma: (V + sigma^2/2 K) u = V field.
   */
  std::vector<su2double> Smooth(const std::vector<su2double>& field, su2double sigma, unsigned long* iterations = nullptr,
                                bool* converged = nullptr) const;

  unsigned long GetnPoint() const { return nPoint; }
  const std::vector<su2double>& GetMass() const { return mass; }

 private:
  unsigned short nDim = 0;
  unsigned long nPoint = 0, nElem = 0;
  std::vector<su2double> coord;              /*!< \brief Point coordinates (nPoint x nDim). */
  std::vector<unsigned long> elemNode;       /*!< \brief Element points (nElem x (nDim + 1)). */
  std::vector<su2double> elemGrad;           /*!< \brief Gradients of the hat functions (nElem x (nDim+1) x nDim). */
  std::vector<su2double> elemVolume;         /*!< \brief Element volumes. */
  std::vector<su2double> mass;               /*!< \brief Lumped P1 mass (median-dual volumes). */
  std::vector<unsigned long> rowPtr, colInd; /*!< \brief Node graph with the diagonal (CSR). */
  std::vector<su2double> stiffness;          /*!< \brief P1 stiffness matrix on the node graph. */
  std::vector<unsigned long> diagPos;        /*!< \brief Position of the diagonal in each row. */
  std::unique_ptr<CBarycentricLocator> locator;
  std::vector<bool> fixedPoint;              /*!< \brief Points of zero motion (MotionField). */

  struct Sample {
    unsigned short nPoint = 0;
    unsigned long point[4] = {};
    su2double weight[4] = {};
    bool inside = true;
  };
  Sample Locate(const su2double* x);

  /*--- y = K x for nComp interleaved components. ---*/
  void MultiplyStiffness(const std::vector<su2double>& x, unsigned short nComp, std::vector<su2double>& y) const;

  /*--- Snapshots smoothed at one level of the coarse-to-fine optical flow, with their gradients. ---*/
  struct FlowLevel {
    su2double mu2 = 0.0;
    std::vector<su2double> sj, sk, gj, gk;
  };

  /*--- Gauss-Newton iterations of the optical flow on the levels [first, last) from the displacement D (updated). ---*/
  void SolveFlow(const std::vector<FlowLevel>& levels, size_t first, size_t last, su2double featureLength,
                 su2double smoothLength, std::vector<su2double>& D, MotionReport& report);

  /*--- RMS of s_j(x - D) - s_k(x), volume weighted. ---*/
  su2double Mismatch(const std::vector<su2double>& sj, const std::vector<su2double>& sk,
                     const std::vector<su2double>& D);
};
