/*!
 * \file CBarycentricTransfer.hpp
 * \brief Transfer of the solution to a new mesh by barycentric (P1) interpolation (mesh adaptation).
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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CSolutionTransfer.hpp"
#include "CDistributedLocator.hpp"
#include "../../../Common/include/adaptation/CSimplexMesh.hpp"
#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/option_structure.hpp"

class CFluidModel;
class CTransferAdmissibility;

/*!
 * \class CBarycentricLocator
 * \brief Locates points in a simplex mesh (triangles or tetrahedra): the element that contains the point and its
 *        barycentric coordinates, or the closest point of the mesh boundary. Uses local ADTs of the mesh (no
 *        communication, each rank searches its own mesh). Serial; the reference (test oracle) of the distributed
 *        location (CDistributedLocator), with the same canonical rules (MPI_TRANSFER_PLAN.md D-B2, D-B3).
 * \note Canonical containing element: of every element the ADT accepts (no early exit), the one with the largest
 *       minimum raw barycentric weight, ties by the smallest element key (sorted point indices; the global indices for
 *       a gathered mesh).
 *       Points outside the mesh (e.g. new boundary points on a curved boundary, which lie outside the faceted boundary
 *       of the mesh) get the closest point of the mesh boundary: the canonical nearest boundary face (CCanonicalBoundary:
 *       smallest (distance, marker position, face key) of the canonical closest-point kernel over a complete candidate
 *       set) and the barycentric coordinates of the closest point in it, which are in [0,1]. Points of a marker can
 *       instead be projected on the faces of markers with the same name (LocateOnBoundary). Marker positions: the
 *       named markers in the order of the mesh (the config order for a gathered mesh), the unnamed ones after them.
 *       Distance limit: a point is accepted at any distance up to GetDistanceLimit(faceSize) = max(faceSize,
 *       absoluteLimit, 1e-3 x the diagonal of the bounding box of the mesh), faceSize the longest edge of the nearest
 *       face. The face size bounds the gap between two discretizations of the same curved boundary that comes from the
 *       faceting (the sagitta of a face is at most half its length, L^2/(8R) for a circle); absoluteLimit is the
 *       tolerance of the remesher's boundary approximation (twice ADAP_HAUSD in the transfer); the domain term covers
 *       round-off and small faces on strongly curved parts. Farther points are flagged (Stencil::beyondLimit): they
 *       mean that the two meshes do not describe the same domain (wrong mesh, units, a moved boundary).
 */
class CBarycentricLocator {
 public:
  /*!
   * \brief Donor points and weights of one located point.
   */
  struct Stencil {
    enum : unsigned short { MAXPOINT = 4 }; /*!< \brief Points of a tetrahedron. */
    unsigned short nPoint = 0;              /*!< \brief Number of donor points (nDim+1 inside, nDim on a boundary face). */
    unsigned long point[MAXPOINT] = {};      /*!< \brief Donor points. */
    su2double weight[MAXPOINT] = {};         /*!< \brief Barycentric weights, in [0,1], sum 1. */
    bool inside = true;                     /*!< \brief Whether the point is inside an element of the mesh. */
    bool onFace = false;                    /*!< \brief The stencil is the closest point of a boundary face. */
    su2double distance = 0.0;               /*!< \brief Distance to the closest point (0 for an element stencil). */
    su2double faceSize = 0.0;               /*!< \brief Longest edge of the boundary face (face stencils only). */
    bool beyondLimit = false;               /*!< \brief The distance exceeds GetDistanceLimit(faceSize). */
    uint32_t marker = 0;                    /*!< \brief Face stencils: position of the face's marker. */
    CSimplexKey key = {};                   /*!< \brief Key of the element or face (sorted point indices). */
  };

  /*!
   * \brief Build the search structures of a mesh.
   * \param[in] geometry - Mesh (triangles in 2D, tetrahedra in 3D; all points of the rank, halos included).
   * \param[in] markerTags - Name of each marker of the geometry (by iMarker); needed for LocateOnBoundary.
   * \param[in] absoluteLimit - Distance accepted outside the mesh at any face size (see the class note).
   */
  explicit CBarycentricLocator(const CGeometry& geometry, const std::vector<std::string>& markerTags = {},
                               su2double absoluteLimit = 0.0);

  /*!
   * \brief Build the search structures of a mesh given as arrays (e.g. the whole mesh gathered on one rank).
   * \param[in] mesh - Points, triangles (2D) or tetrahedra (3D), boundary elements by marker name (markers with an
   *            empty name take part in Locate, not in LocateOnBoundary).
   * \param[in] absoluteLimit - Distance accepted outside the mesh at any face size (see the class note).
   */
  explicit CBarycentricLocator(const CSimplexMesh& mesh, su2double absoluteLimit = 0.0);

  /*!
   * \brief Locate a point: the element that contains it, else the closest point of the nearest boundary face.
   * \param[in] coord - Coordinates of the point.
   * \return Donor points and weights.
   */
  Stencil Locate(const su2double* coord);

  /*!
   * \brief Closest point of the faces of the markers with the given names (names unknown to this mesh are ignored).
   * \note Stencil::inside still tells whether the point is inside an element. If no name is known, same as Locate.
   * \param[in] coord - Coordinates of the point.
   * \param[in] names - Marker names.
   * \param[out] nameFound - If given: the name of the marker of the closest face (empty if no name is known).
   * \return Donor points and weights (a face stencil if a name is known).
   */
  Stencil LocateOnBoundary(const su2double* coord, const std::vector<std::string>& names,
                           std::string* nameFound = nullptr);

  /*!
   * \brief Names of the markers with boundary faces in the mesh.
   */
  std::vector<std::string> GetMarkerNames() const {
    std::vector<std::string> names;
    for (const auto& entry : markerIndex) names.push_back(entry.first);
    return names;
  }

  /*!
   * \brief Canonical element of the mesh (index in the geometry) that contains the point, -1 if none (ADT tolerance).
   */
  long ContainingElement(const su2double* coord);

  /*!
   * \brief Canonical containing element with its raw weights (CElementHit::found false if none).
   */
  CElementHit ContainingHit(const su2double* coord, long* element = nullptr);

  /*!
   * \brief Whether a marker with this name has boundary faces in the mesh.
   */
  bool HasMarker(const std::string& name) const { return markerIndex.count(name) > 0; }

  /*!
   * \brief Elements (indices) whose inflated bounding box intersects the box [bbMin, bbMax].
   */
  void IntersectingElements(const su2double* bbMin, const su2double* bbMax, std::vector<unsigned long>& elements) {
    elemADT->DetermineIntersectingElements(bbMin, bbMax, elements);
  }

  /*!
   * \brief Diagonal of the bounding box of the mesh.
   */
  su2double GetDomainSize() const { return domainSize; }

  /*!
   * \brief Largest distance accepted for a point whose nearest face has the given size (longest edge).
   */
  su2double GetDistanceLimit(su2double faceSize) const;

  static constexpr passivedouble domainFraction = 1e-3; /*!< \brief Fraction of the domain size always accepted. */

 private:
  /*--- Search structures of the mesh. ---*/
  void Build(const CSimplexMesh& mesh);

  /*--- Stencil of a canonical face hit. ---*/
  Stencil FaceStencil(const CFaceHit& hit) const;

  unsigned short nDim = 0;
  su2double domainSize = 0.0;            /*!< \brief Diagonal of the bounding box of the mesh. */
  su2double absoluteLimit = 0.0;         /*!< \brief Distance accepted outside the mesh at any face size. */
  std::vector<su2double> coord;          /*!< \brief Coordinates of the mesh points. */
  std::vector<unsigned long> elemConn;   /*!< \brief Nodes of the elements (nDim+1 per element). */
  std::vector<CSimplexKey> elemKeys;     /*!< \brief Key of each element. */
  std::unique_ptr<CADTElemClass> elemADT;
  std::unique_ptr<CCanonicalBoundary> boundary;     /*!< \brief Faces of all markers. */
  std::vector<uint32_t> allMarkers;                 /*!< \brief Positions of all markers with faces (named, unnamed). */
  std::map<std::string, uint32_t> markerIndex;      /*!< \brief Position of each marker name with faces. */
  std::vector<std::string> markerName;              /*!< \brief Name of each position (empty: unnamed). */
};

/*!
 * \class CBarycentricTransfer
 * \brief Transfer of the solution of the compressible flow (EULER, NAVIER_STOKES, RANS with SA or SST) by barycentric
 *        (P1) interpolation of the donor solution at each point of the new mesh.
 * \note MPI (MPI_TRANSFER_PLAN.md, M1): distributed. The donor values of the owned donor points go to a rendezvous
 *       directory (CPointDirectory); the owned points of the new mesh are located in the donor by CDistributedLocator
 *       (routing to the ranks whose boxes contain them, collect-all location in each rank's owned donor elements,
 *       canonical merge; canonical nearest face on the replicated donor boundary); the stencil values are fetched from
 *       the directory and the point kernel below runs on the rank that owns the new point. The stencil decisions are
 *       the same for every number of ranks and equal to those of the serial locator on the gathered meshes, so the
 *       values are bitwise those of the gathered transfer (expected, reported by the tests; required within 1e-12 of
 *       the field scale). Data-dependent errors are elected over the ranks (CollectiveFailure). Statistics are global
 *       (accurate sums). No rank holds a whole mesh. The gathered transfer (both meshes and the fields on the master
 *       rank, the serial locator there) is kept as the test reference (constructor flag).
 * \note Rules:
 *       - Stencils (CBarycentricLocator): a point on a marker of the new mesh takes the closest point of the donor faces
 *         of the marker(s) with the same name, whether it lies inside or outside the donor mesh: boundary states come
 *         from the same boundary (no-slip walls: the zero velocity of the donor wall, slip walls: the tangential
 *         velocity at the donor wall), and on a curved boundary the new points, which lie off the faceted donor
 *         boundary on both sides, are treated alike. Other points take the donor element that contains them, or the
 *         closest point of the nearest donor boundary face if they lie outside the donor mesh (e.g. near a remeshed
 *         curved boundary). Where a new boundary point coincides with a donor boundary point (fixed surface,
 *         ADAP_SURFACE= NO) it gets the donor values exactly. All weights are in [0,1]: every transferred value is a
 *         convex combination of donor values (no extrapolation). Points farther from the donor than the locator's
 *         distance limit (max(face size, 2 ADAP_HAUSD, 1e-3 x domain size)) stop the transfer with an error.
 *       - Flow: the conservative variables are interpolated; the primitive variables are then computed from them by
 *         the solver with its own fluid model (no separate interpolation or clipping of pressure, temperature).
 *       - Turbulence: nu_tilde for SA (also negative with the negative SA variant); for SST the conservative variables
 *         of the SST solver, rho k and rho omega, are interpolated and divided by the interpolated density (a
 *         density-weighted interpolation of k and omega). Then the values are limited to the bounds the solver applies
 *         after each update (the negative SA variant has no lower bound). With weights in [0,1] the values stay within
 *         the donor ones. Eddy viscosity and wall-distance-dependent quantities are not interpolated, the solver
 *         computes them.
 *       - Admissibility of the complete state of a point (flow and turbulence of the same time level): density,
 *         pressure, temperature and squared speed of sound from the fluid model positive and finite, with the internal
 *         energy of the solver, from which SST subtracts k (CNSVariable::SetPrimVar). With weights in [0,1] and an
 *         ideal gas this holds whenever it holds at the donor points: rho e = rho E - |rho u|^2 / (2 rho) - rho k is
 *         concave in (rho, rho u, rho E, rho k), which is why rho k is interpolated with the density. If not, the point
 *         takes the flow AND turbulence states of its donor point of largest weight whose complete state is admissible
 *         (paired, a donor state, not a clipped one); error if no donor point of the stencil is admissible.
 *       - Then, as after loading a restart file: communication, primitive variables and eddy viscosity, restriction
 *         to the coarse multigrid levels (CSolver::UpdateLoadedSolution), and the old solution equals the new one.
 *       - Time domain (dual time stepping): the transfer is done at the end of a time step (after the dual-time
 *         update of step n), where the solution and Solution_time_n hold the same state U^n (checked, bitwise, for
 *         every transferred solver). U^n is interpolated once and set as the solution and Solution_time_n; U^(n-1)
 *         (Solution_time_n1) is interpolated with the same stencils and rules only for 2nd-order dual time stepping,
 *         otherwise Solution_time_n1 is set to U^n (SU2 allocates it for every time marching, 1st order does not use
 *         it). The history is then restricted to the coarse multigrid levels (as PushSolutionBackInTime after a
 *         restart).
 *         Static meshes only: there are no grid velocities or volumes at n, n-1 to transfer.
 *       Barycentric interpolation does not conserve the integrals of the conservative variables over the domain
 *       (a conservative P1 projection would). The transfer prints the defects and the round-trip difference
 *       (donor -> new -> donor) as a measure of the interpolation error.
 */
class CBarycentricTransfer final : public CSolutionTransfer {
 public:
  /*!
   * \brief Statistics of the last transfer.
   */
  /*!
   * \brief Statistics of the points of one marker of the new mesh (or of the points on no marker).
   * \note Distances are those to the closest point used (the donor boundary of the same marker for marker points,
   *       the nearest donor boundary face for other points outside the donor), relative ones to the longest edge of
   *       that face. Means are over the points at a distance larger than round-off (nOff).
   */
  struct MarkerSummary {
    std::string name;                   /*!< \brief Marker name, "(interior)" for the points on no marker. */
    unsigned long nPoint = 0;           /*!< \brief Points of the new mesh on the marker. */
    unsigned long nOutside = 0;         /*!< \brief Of them, points outside the donor mesh. */
    unsigned long nOff = 0;             /*!< \brief Of them, points off the donor boundary (distance above round-off). */
    unsigned long nBeyondFace = 0;      /*!< \brief Of them, points farther than the size of their donor face. */
    su2double maxDistance = 0.0, sumDistance = 0.0;       /*!< \brief Largest and summed distance. */
    su2double maxRelDistance = 0.0, sumRelDistance = 0.0; /*!< \brief Largest and summed distance / face size. */
  };

  struct Summary {
    unsigned long nPoint = 0;           /*!< \brief Points of the new mesh. */
    unsigned long nOutside = 0;         /*!< \brief Points outside the donor mesh. */
    su2double maxDistance = 0.0;        /*!< \brief Largest distance of a point to its closest donor point (face stencils). */
    su2double maxRelDistance = 0.0;     /*!< \brief Largest distance relative to the size of the donor face. */
    su2double distanceLimit = 0.0;      /*!< \brief Distance always accepted (2 ADAP_HAUSD, 1e-3 x domain size). */
    std::vector<MarkerSummary> markers; /*!< \brief Per marker of the new mesh, then the points on no marker. */
    unsigned long nFlowFixed = 0;       /*!< \brief Points where the interpolated state was not admissible (U^n). */
    unsigned long nHistoryFixed = 0;    /*!< \brief Same for U^(n-1) (Solution_time_n1, 2nd-order dual time stepping). */
    unsigned short nTimeLevels = 0;     /*!< \brief History arrays set (0 steady, 1 or 2). */
    bool interpolateTimeN1 = false;     /*!< \brief U^(n-1) interpolated (2nd order), else Solution_time_n1 = U^n. */
    unsigned long nTurbLimited = 0;     /*!< \brief Turbulence values limited to the bounds of the solver (all time levels). */
    su2double donorVolume = 0.0;        /*!< \brief Volume of the donor domain (sum of control volumes). */
    su2double newVolume = 0.0;          /*!< \brief Volume of the new domain. */
    std::vector<su2double> donorIntegral;  /*!< \brief Integral of each conservative variable on the donor. */
    std::vector<su2double> newIntegral;    /*!< \brief Integral of each conservative variable on the new mesh. */
    std::vector<su2double> roundTripL2;    /*!< \brief RMS of the round-trip difference / donor range, per variable. */
    std::vector<su2double> roundTripLinf;  /*!< \brief Max of the round-trip difference / donor range, per variable. */
    unsigned long nRoundTripBeyond = 0;    /*!< \brief Donor points beyond the distance limit of the new mesh. */
    passivedouble roundTripWorstRatio = 0; /*!< \brief Their largest distance / limit (reported, never an error). */
    passivedouble time = 0.0;              /*!< \brief Wall time of the transfer (seconds). */
  };

  /*!
   * \brief Stencil decision of one point of the new mesh (for the tests): element or face, its key and marker.
   */
  struct StencilRecord {
    uint64_t gid = 0;                  /*!< \brief Global index of the point of the new mesh. */
    bool onFace = false, inside = false, beyondLimit = false;
    uint32_t marker = 0;               /*!< \brief Face stencils: marker position (gathered: in the gathered mesh). */
    CSimplexKey key = {};              /*!< \brief Element or face key (global indices). */
    uint64_t point[4] = {};            /*!< \brief Donor points (global indices) in stencil order. */
    passivedouble weight[4] = {};
  };

  /*!
   * \param[in] roundTripCheck - Measure the round-trip difference (interpolates the new solution back to the donor).
   * \param[in] gathered - Run the MPI-1 transfer on the meshes gathered on the master rank (test reference).
   */
  explicit CBarycentricTransfer(bool roundTripCheck = true, bool gathered = false)
      : roundTripCheck(roundTripCheck), gathered(gathered) {}

  void Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver) override;

  /*!
   * \brief Keep the stencil decisions of the owned points of the new mesh (gathered: all points, on the master rank).
   */
  void KeepStencils(bool keep) { keepStencils = keep; }
  const std::vector<StencilRecord>& GetStencils() const { return stencilRecords; }

  /*!
   * \brief The interpolation of one point and time level (the same code in the distributed and the gathered
   *        transfer): conservative flow variables and turbulence (SST: rho k, rho omega interpolated, divided by the
   *        interpolated density) with the weights, turbulence bounded (finite values clipped, NaN kept), then the
   *        full admissibility predicate; if it fails, the flow and turbulence states of the donor point of largest
   *        weight whose complete (bounded) state is admissible.
   * \param[in] donorLevel - Per stencil point: its fields of the level (flow, then raw turbulence).
   * \param[out] out - Flow, then turbulence.
   * \param[out] fixed - The interpolated state was not admissible.
   * \return False if no donor state of the stencil is admissible.
   */
  static bool PointKernel(const CTransferAdmissibility& admissibility, unsigned short nPoint, const passivedouble* weight,
                          const su2double* const* donorLevel, su2double* out, bool& fixed,
                          unsigned long& nTurbLimited);

  /*!
   * \brief Statistics of the last transfer.
   */
  const Summary& GetSummary() const { return summary; }

  Report GetReport() const override;

  /*!
   * \brief Whether a conservative flow state is admissible: density, pressure, temperature and squared speed of
   *        sound positive and finite, with the internal energy as the solver computes it (CEulerVariable::SetPrimVar,
   *        and CNSVariable::SetPrimVar, which subtracts the turbulent kinetic energy k with SST), and finite values.
   * \param[in] fluidModel - Fluid model of the flow solver (its state is changed).
   * \param[in] nDim - Number of dimensions.
   * \param[in] solution - Conservative variables (density, momentum, total energy per volume).
   * \param[in] turbKineticEnergy - k of the SST model at the point (per unit mass), 0 otherwise.
   */
  static bool AdmissibleState(CFluidModel& fluidModel, unsigned short nDim, const su2double* solution,
                              su2double turbKineticEnergy = 0.0);

 private:
  /*--- The MPI-1 transfer on the gathered meshes (test reference). ---*/
  void TransferGathered(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver);

  /*--- The counts, distances, volumes and integrals of the summary from the master rank to all ranks (the per-marker
   *    statistics and the round trip stay on the master rank; gathered transfer). ---*/
  void BroadcastSummary();

  /*--- The log of the transfer (master rank). ---*/
  void PrintSummary(unsigned short nDim, bool turbulence, const std::string& timing) const;

  bool roundTripCheck;
  bool gathered;
  bool keepStencils = false;
  Summary summary;
  std::vector<StencilRecord> stencilRecords;
};
