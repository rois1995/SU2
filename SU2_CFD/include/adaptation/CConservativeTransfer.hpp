/*!
 * \file CConservativeTransfer.hpp
 * \brief Conservative P1 transfer of the solution to a new mesh (supermesh of the two meshes, mesh adaptation).
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

#include "CBarycentricTransfer.hpp"
#include "CSolutionTransfer.hpp"

class CFluidModel;

/*!
 * \class CConservativeProjection
 * \brief Conservative P1 projection of nodal fields from a simplex mesh (donor) to another one (target), with the
 *        median-dual control volumes of SU2 as the conserved cells. Port of the user's Python ConservativeP1_Fun.py
 *        (InterpMethod "Conservative", simplices), adapted to SU2. Single rank, triangles (2D) or tetrahedra (3D).
 * \note Method. The donor nodal values u^A define the continuous P1 field u_h^A. The target nodal values u^B solve,
 *       for every target control volume C_j (the median-dual cell of node j),
 *         integral over C_j of u_h^B = integral over C_j of u_h^A,   i.e.   M u^B = S,
 *       M[j][k] = integral over C_j of the hat function of node k (sparse, symmetric, strictly diagonally dominant on
 *       simplices: per element |e| 11/54 on the diagonal and |e| 7/108 off it in 2D, |e| 25/192 and |e| 23/576 in 3D).
 *       Column k of M sums to |C_k|, so sum_k u^B_k |C_k| = sum_j S_j: the lumped integral that the finite-volume
 * solver conserves is the integral of the donor field. On the same mesh the projection is the identity; an affine field
 *       is reproduced exactly where the two domains coincide (u_h^A = u, and the nodal values of u solve M u = S).
 *       - S (supermesh): for every target element, the donor elements that overlap it are found by an advancing front
 *         through the donor neighbours from a seed (ADT: donor element of the target centroid or of points near its
 *         vertices, else the donor element at the nearest donor boundary face); every overlap is the donor simplex
 *         clipped by the target element (convex polytope clipping, no tolerances, ConvexClipping.hpp), split into the
 *         dual pieces of the target vertices ({lambda_i >= lambda_j} in the barycentric coordinates of the target
 *         element, exactly the median-dual piece) and integrated exactly (measure times the donor field at the
 *         centroid).
 *       - New domain outside the donor (S_n, e.g. between a new far-field circle and the donor polygon): every dual
 *         piece not fully covered gets its missing measure times the donor field at the centroid of the missing part,
 *         located as in the barycentric transfer (D1b rule: a piece of a node on a marker takes the closest point of
 *         the donor faces of the same marker, other pieces the containing donor element or the closest donor boundary
 *         point): a convex combination of donor values, no extrapolation.
 *       - Donor domain outside the new one (S_d, donor fluid that is no longer fluid): the content of the uncovered
 *         part of every donor element is computed exactly (measure times the donor field at its centroid).
 *       - Totals (SliverRule): the new total of each field is the target of the limiter and of a uniform shift of
 *         the solution (the difference to sum_j S_j spread over the new domain by volume, M 1 = |C|):
 *         CLOSED (default): at walls and symmetry planes the S_d content is kept and the S_n content taken from the
 *         domain (both spread), at open boundaries (far field, inlet, outlet, ...: Options::openMarkers) the total
 *         follows the change of the domain (S_n added, S_d dropped). With a closed domain the totals are the donor's.
 *         GLOBAL: the totals are always the donor's. NONE: the totals change by the S_n minus the S_d content (the
 *         Python reference). BOUNDARY (the first design, D1b note): the S_d content is added to the nearest new
 *         boundary points of the same marker, the totals are the donor's; it puts a local excess at those points
 *         (density spikes of up to 19% of the range on the adapted NACA0012 far field), not recommended.
 *         The part of the shift that is not caused by the slivers (the supermesh defect) is round-off. With the same
 *         boundary on both meshes (ADAP_SURFACE= NO) there are no slivers and all rules are the same.
 *       - Solve: Jacobi-preconditioned conjugate gradients (M is symmetric positive definite), from the control-volume
 *         means S_j/|C_j|.
 *       - Limiter (maximum principle with conservative redistribution, the rule of the Python reference, enabled by
 *         default): bounds of node j = min/max of the donor nodal values of every donor element that overlaps C_j
 *         (and of the stencils of the S_n fill and of the S_d content added to node j), widened by limiterTolerance
 *         times the range of the field over the donor (2e-3, the GalerkinLimitTol default: smooth extrema pass at
 *         second order). Values are clipped to their bounds, then the integral defect is redistributed over the nodes
 *         with room left, in proportion to the room (iterated), so the total is restored exactly. If the bounds cannot
 *         hold the total (only with slivers), the rest is spread over all nodes by volume and the field is flagged.
 */
class CConservativeProjection {
 public:
  /*! \brief Where the donor content outside the new domain goes (see the class note). */
  enum class SliverRule { CLOSED, GLOBAL, NONE, BOUNDARY };

  struct Options {
    bool limiter = true;                   /*!< \brief Maximum-principle limiter with redistribution. */
    passivedouble limiterTolerance = 2e-3; /*!< \brief Widening of the bounds, relative to the donor range. */
    SliverRule sliverRule = SliverRule::CLOSED;
    std::vector<std::string> openMarkers;  /*!< \brief Names of the open boundaries (far field, inlet, outlet). */
    passivedouble absoluteLimit = 0.0;     /*!< \brief Distance always accepted outside the donor (2 ADAP_HAUSD). */
    passivedouble solverTolerance = 1e-13; /*!< \brief Relative residual of the conjugate gradients. */
    unsigned long maxSolverIter = 2000;
  };

  struct Summary {
    unsigned short nField = 0;
    unsigned long nTargetElem = 0, nDonorElem = 0;
    unsigned long nPairs = 0;          /*!< \brief Overlapping (target, donor) element pairs. */
    unsigned long nTested = 0;         /*!< \brief Pairs clipped (front and seed search). */
    unsigned long nSeedFallback = 0;   /*!< \brief Target elements whose seed came from the nearest boundary face. */
    unsigned long nElemOutside = 0;    /*!< \brief Target elements that overlap no donor element. */
    passivedouble overlapVolume = 0.0; /*!< \brief Measure of the supermesh (both domains). */
    passivedouble targetVolume = 0.0, donorVolume = 0.0; /*!< \brief Sums of the element measures. */
    passivedouble targetCV = 0.0, donorCV = 0.0;         /*!< \brief Sums of the SU2 control volumes. */
    unsigned long nFillPieces = 0, nFillNodes = 0; /*!< \brief Dual pieces / nodes with a part outside the donor. */
    unsigned long nInteriorFill = 0;               /*!< \brief Of the pieces, those of nodes on no marker. */
    passivedouble fillVolume = 0.0;                /*!< \brief Measure of S_n (new domain outside the donor). */
    passivedouble maxFillDistance = 0.0, maxFillRelDistance = 0.0; /*!< \brief Of the fill stencils (face ones). */
    unsigned long nSliverElems = 0;   /*!< \brief Donor elements with a part outside the new domain. */
    passivedouble sliverVolume = 0.0; /*!< \brief Measure of S_d. */
    passivedouble maxSliverDistance = 0.0, maxSliverRelDistance = 0.0; /*!< \brief To the new boundary (BOUNDARY). */
    passivedouble maxUncovered = 0.0; /*!< \brief Largest uncovered fraction of a dual piece. */
    unsigned long nDegenerate = 0;    /*!< \brief Target elements of zero measure (skipped). */
    std::vector<passivedouble> scale; /*!< \brief Per field: sum |u^A| |C^A| (scale of the relative numbers). */
    std::vector<passivedouble> donorTotal, targetTotal, fill, sliver, fillOpen, sliverOpen, correction, supermeshDefect,
        newTotal;
    std::vector<unsigned long> iterations; /*!< \brief Conjugate-gradient iterations per field. */
    std::vector<passivedouble> residual;   /*!< \brief Final relative residual per field. */
    std::vector<unsigned long> nLimited;   /*!< \brief Values clipped by the limiter per field. */
    std::vector<bool> infeasible;          /*!< \brief Bounds could not hold the total (rest spread by volume). */
    passivedouble timeSetup = 0.0, timeSupermesh = 0.0, timeSlivers = 0.0, timeSolve = 0.0, timeLimiter = 0.0;
  };

  /*!
   * \param[in] donor - Donor mesh (triangles or tetrahedra, one rank).
   * \param[in] donorTags - Marker names of the donor by iMarker.
   * \param[in] target - New mesh.
   * \param[in] targetTags - Marker names of the new mesh by iMarker.
   * \param[in] options - Options.
   */
  CConservativeProjection(const CGeometry& donor, const std::vector<std::string>& donorTags, const CGeometry& target,
                          const std::vector<std::string>& targetTags, const Options& options);

  ~CConservativeProjection();

  /*!
   * \brief Project fields.
   * \param[in] nField - Number of fields.
   * \param[in] donorValues - Donor values, point-major (nPointDonor x nField).
   * \param[out] newValues - New values (nPointTarget x nField), limited, with exact totals.
   * \note The field arithmetic is active (su2double: forward-mode derivatives of the values, DIRECT_DIFF, are
   *       projected with them), the geometry and the mass matrix are passive.
   */
  void Project(unsigned short nField, const std::vector<su2double>& donorValues, std::vector<su2double>& newValues);

  /*!
   * \brief Restore the donor total of a field after values were changed, within the limiter bounds, on the nodes that
   *        are not frozen (redistribution as in the limiter).
   * \return False if the bounds cannot hold the total (the rest is then spread over the free nodes by volume).
   */
  bool Redistribute(unsigned short iField, std::vector<su2double>& newValues, const std::vector<bool>& frozen) const;

  /*!
   * \brief Total of a field on the new mesh, sum of value x SU2 control volume.
   */
  su2double NewTotal(unsigned short iField, const std::vector<su2double>& newValues) const;

  const Summary& GetSummary() const { return summary; }

  /*!
   * \brief Mass matrix of the target (CSR) for the tests.
   */
  void GetMassMatrix(std::vector<unsigned long>& rowPtr, std::vector<unsigned long>& col,
                     std::vector<passivedouble>& value) const {
    rowPtr = massRowPtr;
    col = massCol;
    value = massValue;
  }

 private:
  struct Frame;
  class Poly;

  /*--- Overlap of target element iElem (frame) with donor element jElem; accumulates its pieces if requested. ---*/
  bool Overlap(const Frame& frame, unsigned long jElem, bool accumulate);
  long FindSeed(const Frame& frame, std::vector<unsigned long>& stamp, std::vector<unsigned long>& queue);
  void SetFrame(unsigned long iElem, Frame& frame) const;
  void UpdateBounds(unsigned long iPoint, const unsigned long* donorPoints, unsigned short nDonor);
  void SolveMass(const std::vector<passivedouble>& b, std::vector<passivedouble>& x, unsigned long& iterations,
                 passivedouble& residual) const;
  void SolveMassActive(const std::vector<su2double>& b, std::vector<su2double>& x, unsigned long& iterations,
                       passivedouble& residual) const;
  void SetBounds(unsigned short iField, std::vector<su2double>& lo, std::vector<su2double>& hi) const;
  bool BoundedRedistribute(std::vector<su2double>& v, unsigned short iField, su2double total,
                           const std::vector<su2double>& lower, const std::vector<su2double>& upper,
                           const std::vector<bool>* frozen, unsigned long* nClipped) const;

  Options options;
  unsigned short nDim = 0, nNode = 0;
  unsigned short nField = 0;

  /*--- Donor. ---*/
  unsigned long nPointD = 0, nElemD = 0;
  std::vector<passivedouble> coordD, volElemD, cvD;
  std::vector<unsigned long> elemD;
  std::vector<long> nbrD; /*!< \brief Neighbour across the face opposite vertex k. */
  std::vector<std::pair<std::vector<unsigned long>, unsigned long>> freeFaces; /*!< \brief Sorted nodes -> element. */
  std::unique_ptr<CBarycentricLocator> donorLocator;
  std::vector<std::string> donorNames;

  /*--- Target. ---*/
  const CGeometry& targetGeometry;
  unsigned long nPointT = 0, nElemT = 0;
  std::vector<passivedouble> coordT, volElemT, cvT, rowVolume;
  std::vector<unsigned long> elemT;
  std::vector<std::vector<std::string>> pointMarkers; /*!< \brief Marker names of each target point. */
  std::unique_ptr<CBarycentricLocator> targetLocator;
  std::vector<unsigned long> massRowPtr, massCol;
  std::vector<passivedouble> massValue, massDiag;

  /*--- State of a projection. ---*/
  const std::vector<su2double>* donorField = nullptr;
  std::vector<su2double> rhs, lower, upper, range;                           /*!< \brief Active (field values). */
  std::vector<su2double> fillA, sliverA, fillOpenA, sliverOpenA, targetTotalA; /*!< \brief Active totals. */
  std::vector<passivedouble> covT, momT, covD, momD;

  Summary summary;
};

/*!
 * \class CConservativeTransfer
 * \brief Conservative P1 transfer of the solution of the compressible flow (EULER, NAVIER_STOKES, RANS with SA or
 *        SST) to a new mesh (CConservativeProjection). Default transfer of the time-domain adaptation loop.
 * \note - Flow: the conservative variables (density, momentum, total energy per volume) are projected, so their
 *         integrals over the domain (sum of value x control volume, what the finite-volume solver conserves) are
 *         those of the donor to round-off. Primitive variables follow from the solver's fluid model.
 *       - Turbulence: the variable that the solver transports in conservation form: rho k and rho omega for SST
 *         (CTurbSSTSolver is "Conservative": its dual-time term and fluxes act on rho k, rho omega), divided by the
 *         projected density afterwards; nu_tilde itself for SA (not conservative in SU2: the dual-time term is
 *         d(nu_tilde)/dt and the convection uses the velocity), so its integral is kept. The Python reference
 *         multiplies every turbulence variable by rho (MultiplyByRho); for SA this transfer follows the solver
 *         instead.
 *       - No-slip walls: the projected momentum of a wall point is not zero (its control volume reaches into the
 *         moving fluid). It is set to zero, rho E kept, as the solver imposes it in its first iteration, and the
 *         removed momentum is redistributed over the other points within the limiter bounds (momentum totals exact;
 *         if the bounds cannot hold it, the rest is spread by volume and the field is counted).
 *       - Turbulence limited to the bounds the solver applies after each update (SST: k = (rho k) / rho, rho k set
 *         back), counted; the only step that can change a turbulence integral.
 *       - Admissibility (as in the barycentric transfer) of the complete state of a node, flow and turbulence of the
 *         same time level: density, pressure, temperature and squared speed of sound from the fluid model must be
 *         positive and finite, with the internal energy of the solver (SST subtracts k). The limiter bounds every
 *         variable separately, so high-speed states with a small internal energy can come out with the kinetic energy
 *         above the total energy. Recovery (coupled and conservative, CAdmissibilityRecovery in the .cpp): the patch
 *         of the node and its neighbours is blended towards its volume-weighted mean state, all fields of the level
 *         together, with the largest factor that makes every state of the patch admissible; the patch grows (graph
 *         rings) until its mean is admissible. The integrals of the patch and the zero wall momentum are kept exactly
 *         and no new extrema appear. The transfer stops with an error if a state cannot be recovered (not even the
 *         mean of the mesh is admissible), and, after the recovery, if any state of any time level is not admissible,
 *         a no-slip wall momentum is not zero, or a flow total differs from the projection's by more than 1e-12 of
 *         sum |u| V. Counted (points, patches, largest patch radius).
 *       - Time domain: as the barycentric transfer: U^n once into the solution and Solution_time_n; U^(n-1) projected
 *         for 2nd-order dual time stepping only (the same supermesh, one pass for all arrays), else
 *         Solution_time_n1 = U^n.
 *       - Then, as after loading a restart: primitive variables, eddy viscosity, coarse multigrid levels.
 *       - Derivatives: the field values stay active (su2double) through the projection, the limiter, the wall fix and
 *         the recovery; the geometry, the supermesh and the mass matrix are passive, and the linear solve gives
 *         x' = M^-1 b' for the derivatives. So forward-mode derivatives (DIRECT_DIFF) of the solution and of its time
 *         history are transferred like the values. Nothing is recorded on a reverse-mode tape (the adaptation is
 *         rejected with adjoint problems).
 */
class CConservativeTransfer final : public CSolutionTransfer {
 public:
  struct Summary {
    CConservativeProjection::Summary projection;
    unsigned long nPoint = 0;
    unsigned short nVarFlow = 0;         /*!< \brief Flow variables (the first fields of each time level). */
    unsigned long nFlowFixed = 0;        /*!< \brief Points where the projected flow state was not admissible (U^n). */
    unsigned long nHistoryFixed = 0;     /*!< \brief Same for U^(n-1). */
    unsigned long nTurbLimited = 0;      /*!< \brief Turbulence values limited to the bounds of the solver. */
    unsigned long nRecoveryPatches = 0;  /*!< \brief Patches blended by the admissibility recovery (all levels). */
    unsigned long maxRecoveryRing = 0;   /*!< \brief Largest patch radius (graph rings) of the recovery. */
    unsigned long nWallBoundsExceeded = 0; /*!< \brief Momentum fields whose wall redistribution exceeded the bounds. */
    unsigned long nWallPoints = 0;       /*!< \brief Points on no-slip walls (momentum set to zero). */
    passivedouble maxWallMomentum = 0.0; /*!< \brief Largest projected momentum component there before. */
    unsigned short nTimeLevels = 0;
    bool interpolateTimeN1 = false;
    std::vector<std::string> names;            /*!< \brief Names of the projected fields. */
    std::vector<passivedouble> donorIntegral;  /*!< \brief Per field: integral on the donor. */
    std::vector<passivedouble> newIntegral;    /*!< \brief Per field: integral on the new mesh (final state). */
    std::vector<passivedouble> relativeDefect; /*!< \brief Per field: relative change (momentum: to its norm). */
    passivedouble time = 0.0;
  };

  explicit CConservativeTransfer(CConservativeProjection::Options options = CConservativeProjection::Options())
      : options(options) {}

  void Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver) override;

  Report GetReport() const override;

  const Summary& GetSummary() const { return summary; }

 private:
  CConservativeProjection::Options options;
  Summary summary;
};
