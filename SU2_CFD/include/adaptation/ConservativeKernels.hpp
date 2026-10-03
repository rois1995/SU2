/*!
 * \file ConservativeKernels.hpp
 * \brief Kernels of the conservative P1 projection shared by the serial projection (test oracle, gathered transfer)
 *        and the distributed one: simplex planes, overlap of two simplices split into dual pieces, mass matrix,
 *        guarded Jacobi conjugate gradients, bounded redistribution with exact totals.
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

#include "../../../Common/include/basic_types/datatype_structure.hpp"

class CGeometry;

namespace conservative {

constexpr passivedouble kOverlapFraction = 1e-13; /*!< \brief Overlaps below this fraction of the smaller element are
                                                       ignored (round-off contacts at shared faces/edges/points). */
constexpr passivedouble kGapFraction = 1e-10;     /*!< \brief Uncovered parts above this fraction are filled/moved. */
constexpr passivedouble kCentroidFraction = 1e-8; /*!< \brief Below it, the centroid of a missing part is not computed
                                                       from the moments (cancellation): the whole cell's centroid. */

/*!
 * \brief Barycentric functions of the simplex y[0..nDim] (local frame): lambda_k(x) = a[k] + G[k].x. Returns the
 *        determinant of the edge matrix (nDim! x the signed measure), 0 for a degenerate simplex.
 */
passivedouble SimplexPlanes(unsigned short nDim, const passivedouble (*y)[3], passivedouble (*G)[3], passivedouble* a);

/*! \brief nDim! (2 or 6). */
inline passivedouble Factorial(unsigned short nDim) { return nDim == 2 ? 2.0 : 6.0; }

/*!
 * \brief Measure of a simplex given by its nodes (frame at node 0), as the serial projection computes it.
 */
passivedouble SimplexMeasure(unsigned short nDim, const passivedouble* const* nodes);

/*!
 * \brief A target element in a frame local to its first vertex: vertices, barycentric functions, bounding box.
 */
struct Frame {
  passivedouble origin[3] = {};
  passivedouble y[4][3] = {};
  passivedouble G[4][3] = {}, a[4] = {};
  passivedouble volume = 0.0;
  passivedouble bbMin[3] = {}, bbMax[3] = {}; /*!< \brief Bounding box in the frame. */
};

/*! \brief Frame of the simplex with the given node coordinates (element node order). */
void SetFrame(unsigned short nDim, const passivedouble* const* nodes, Frame& frame);

/*!
 * \brief Called for each dual piece of an overlap with positive measure: vertex i of the target element, measure,
 *        centroid (in the frame) and the donor's barycentric coordinates mu (nDim+1) at the centroid.
 */
using PieceFunction = void (*)(void* context, unsigned short i, passivedouble volume, const passivedouble* centroid,
                               const passivedouble* mu);

/*!
 * \brief Overlap of a target element (frame) with a donor simplex: the donor simplex clipped by the target's
 *        lambda_k >= 0 (convex polytope clipping, no tolerances), ignored below kOverlapFraction of the smaller element,
 *        split into the dual pieces {lambda_i >= lambda_j} of the target vertices, each integrated exactly (piece()).
 * \param[in] donorNodes - Coordinates of the donor nodes (global), element order.
 * \param[in] donorVolume - Measure of the donor element.
 * \param[out] clipped - True if the bounding boxes intersect (the pair reached the clipping).
 * \return True if the overlap is above the cutoff.
 */
bool Overlap(unsigned short nDim, const Frame& frame, const passivedouble* const* donorNodes, passivedouble donorVolume,
             PieceFunction piece, void* context, bool* clipped);

/*!
 * \brief The dual piece of vertex i of the target element (the element clipped by the piece planes): measure and
 *        centroid in the frame.
 */
void PieceMoments(unsigned short nDim, const Frame& frame, unsigned short i, passivedouble& volume,
                  passivedouble* centroid);

/*!
 * \brief Centroid of the part of a donor element outside the new domain (missing measure, moments of the covered
 *        part relative to node 0), and the donor's barycentric coordinates there.
 * \param[out] x - Global coordinates of the centroid.
 * \param[out] mu - Barycentric coordinates (nDim+1).
 */
void SliverCentroid(unsigned short nDim, const passivedouble* const* donorNodes, passivedouble volume,
                    passivedouble missing, const passivedouble* moment, passivedouble* x, passivedouble* mu);

/*!
 * \brief Mass matrix M[j][k] = integral over the median-dual cell C_j of the hat function of k, for the rows
 *        0..nRow-1 of a local numbering (columns any local point): exact for simplices (per element |e| 11/54 on the
 *        diagonal and |e| 7/108 off it in 2D, |e| 25/192 and |e| 23/576 in 3D). Rows are complete if every element of
 *        a row point is given (the partition property of SU2 for the domain points).
 */
struct MassMatrix {
  unsigned long nRow = 0, nLocal = 0;
  std::vector<unsigned long> rowPtr, col;
  std::vector<passivedouble> value, diag, rowVolume;

  /*!
   * \param[in] elements - Local node indices, nDim+1 per element.
   * \param[in] volumes - Measure of each element.
   */
  void Assemble(unsigned short nDim, unsigned long nRow, unsigned long nLocal, const std::vector<unsigned long>& elements,
                const std::vector<passivedouble>& volumes);

  /*! \brief out = M v for the rows (v indexed by local point). */
  void Multiply(const std::vector<passivedouble>& v, std::vector<passivedouble>& out) const;
};

/*!
 * \brief Result of a guarded conjugate-gradient solve.
 */
struct SolveResult {
  unsigned long iterations = 0;
  passivedouble trueResidual = 0.0; /*!< \brief sqrt(sum (b - M x)^2 / sum b^2) at the end (scaled RHS). */
  bool nonfiniteRHS = false;        /*!< \brief A right-hand side entry is not finite (error). */
  bool breakdown = false;           /*!< \brief Nonfinite or nonpositive initial curvature (error). */
  bool failed = false;              /*!< \brief Nonfinite or too large (> 1e-10) true residual (error). */
  bool warning = false;             /*!< \brief True residual between the tolerance and 1e-10. */
  bool derivativeSolved = false;    /*!< \brief Forward mode: the derivative system was solved. */
};

/*!
 * \brief Jacobi-preconditioned conjugate gradients for M x = b with the guards of MPI_TRANSFER_PLAN.md 5.8: a reduced
 *        nonfinite-RHS flag before any shortcut, the zero-RHS shortcut, an exact power-of-two scaling of the RHS,
 *        finite and positive curvature checks, an explicit iteration counter, the true residual at the end. Every
 *        decision uses reduced values, so every rank runs the same iterations. Serial (haloGeometry null, no
 *        communication) or distributed (rows = the domain points of the geometry, halo exchange of the search
 *        directions, reductions over the ranks).
 */
class MassSolver {
 public:
  MassSolver(const MassMatrix& matrix, const CGeometry* haloGeometry, passivedouble tolerance,
             unsigned long maxIterations)
      : matrix(matrix), haloGeometry(haloGeometry), tolerance(tolerance), maxIterations(maxIterations) {}

  /*! \brief Passive solve; b and x of size nRow. */
  SolveResult Solve(const std::vector<passivedouble>& b, std::vector<passivedouble>& x) const;

  /*!
   * \brief Active solve: the values as a passive solve and, in forward mode, the derivatives x' = M^-1 b' if any rank
   *        has a nonzero derivative (reduced), with the same guards.
   */
  SolveResult SolveActive(const std::vector<su2double>& b, std::vector<su2double>& x) const;

 private:
  passivedouble Sum(passivedouble value) const;
  void Sum2(passivedouble* values) const;
  passivedouble Max(passivedouble value) const;
  bool AnyOf(bool value) const;
  void Exchange(std::vector<passivedouble>& v) const;

  const MassMatrix& matrix;
  const CGeometry* haloGeometry;
  passivedouble tolerance;
  unsigned long maxIterations;
};

/*!
 * \brief Result of a bounded redistribution.
 */
struct RedistributeResult {
  bool totalExact = true;          /*!< \brief Residual <= 1e-15 scale after the final spread. */
  bool relaxed = false;            /*!< \brief The bounds could not hold the total: the rest spread by volume. */
  bool error = false;              /*!< \brief Nonzero required correction without free volume, or a recomputed residual
                                        above 1e-12 scale. */
  std::string reason;
  unsigned long nClipped = 0;      /*!< \brief Values clipped to their bounds (beyond the counting tolerance). */
  unsigned long nViolations = 0;   /*!< \brief Free values outside their bounds after the spread (recomputed). */
  passivedouble maxViolation = 0;  /*!< \brief Largest violation relative to the field range. */
  passivedouble residual = 0;      /*!< \brief Recomputed |total - sum| / scale. */
};

/*!
 * \brief Clip the free values to their bounds, then restore the total within the bounds (iterated proportional to the
 *        room, at most 100 times; unbounded values first), and if the bounds cannot hold it spread the rest over the
 *        free values by volume (bounds relaxed, reported). Precedence: the total over the bounds; the residual is
 *        tested first (an acceptable residual is success whatever the capacity); a nonzero required correction with
 *        no free volume is an error, as is a recomputed residual above 1e-12 scale. Sums are accurate (CAccurateSum),
 *        over the ranks if distributed (every rank must call it, the decisions are the same on all ranks).
 * \param[in,out] v - Values of the rows.
 * \param[in] cv - Control volumes of the rows.
 * \param[in] frozen - If not null: rows that keep their value.
 * \param[in] countTolerance - Clipped values are counted beyond this distance to the bound.
 * \param[in] range - Range of the field (relative violations).
 */
RedistributeResult BoundedRedistribute(std::vector<su2double>& v, const std::vector<passivedouble>& cv, su2double total,
                                       const std::vector<su2double>& lo, const std::vector<su2double>& hi,
                                       const std::vector<bool>* frozen, passivedouble countTolerance,
                                       passivedouble range, bool distributed);

}  // namespace conservative
