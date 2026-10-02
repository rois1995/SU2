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
#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/option_structure.hpp"

class CFluidModel;

/*!
 * \class CBarycentricLocator
 * \brief Locates points in a simplex mesh (triangles or tetrahedra): the element that contains the point and its
 *        barycentric coordinates, or the closest point of the mesh boundary. Uses local ADTs of the mesh (no
 *        communication, each rank searches its own mesh).
 * \note Points outside the mesh (e.g. new boundary points on a curved boundary, which lie outside the faceted boundary
 *       of the mesh) get the closest point of the mesh boundary: the nearest boundary face (exact nearest-element search
 *       of the ADT) and the barycentric coordinates of the closest point in it, which are in [0,1]. Points of a marker
 *       can instead be projected on the faces of markers with the same name (LocateOnBoundary).
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
   * \return Donor points and weights (a face stencil if a name is known).
   */
  Stencil LocateOnBoundary(const su2double* coord, const std::vector<std::string>& names);

  /*!
   * \brief Whether a marker with this name has boundary faces in the mesh.
   */
  bool HasMarker(const std::string& name) const { return markerIndex.count(name) > 0; }

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
  /*--- Closest point of the face nearest to a point, in one face ADT. ---*/
  Stencil ClosestFace(const su2double* coord, CADTElemClass& adt, const std::vector<unsigned long>& conn) const;

  unsigned short nDim = 0;
  su2double domainSize = 0.0;            /*!< \brief Diagonal of the bounding box of the mesh. */
  su2double absoluteLimit = 0.0;         /*!< \brief Distance accepted outside the mesh at any face size. */
  std::vector<su2double> coord;          /*!< \brief Coordinates of the mesh points. */
  std::vector<unsigned long> elemConn;   /*!< \brief Nodes of the elements (nDim+1 per element). */
  std::vector<unsigned long> faceConn;   /*!< \brief Nodes of the boundary faces of all markers (nDim per face). */
  std::unique_ptr<CADTElemClass> elemADT, faceADT;
  std::map<std::string, unsigned short> markerIndex;      /*!< \brief Position of a marker name in the vectors below. */
  std::vector<std::vector<unsigned long>> markerFaceConn; /*!< \brief Nodes of the faces of each marker name. */
  std::vector<std::unique_ptr<CADTElemClass>> markerADT;  /*!< \brief ADT of the faces of each marker name. */
};

/*!
 * \class CBarycentricTransfer
 * \brief Transfer of the solution of the compressible flow (EULER, NAVIER_STOKES, RANS with SA or SST) by barycentric
 *        (P1) interpolation of the donor solution at each point of the new mesh. Single rank for now.
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
 *         Admissibility after the interpolation: density, pressure, temperature and squared speed of sound from the
 *         fluid model must be positive and finite. With weights in [0,1] (as here, up to the 1e-10 tolerance of
 *         the search) and an ideal gas this holds whenever it holds at the donor points (the internal energy per
 *         volume is concave in the conservative variables). If not, the point takes the conservative state of its donor point of largest weight that is
 *         admissible (a donor state, not a clipped one); error if no donor point of the stencil is admissible.
 *       - Turbulence: the solution variables of the solver are interpolated (nu_tilde for SA, also negative with the
 *         negative SA variant; k and omega for SST), then limited to the bounds the solver applies after each update
 *         (the negative SA variant has no lower bound). With weights in [0,1] the values stay within the donor ones.
 *         Eddy viscosity and wall-distance-dependent quantities are not interpolated, the solver computes them.
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
    unsigned long nFlowFixed = 0;       /*!< \brief Points where the interpolated flow state was not admissible. */
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
  };

  /*!
   * \param[in] roundTripCheck - Measure the round-trip difference (interpolates the new solution back to the donor).
   */
  explicit CBarycentricTransfer(bool roundTripCheck = true) : roundTripCheck(roundTripCheck) {}

  void Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver) override;

  /*!
   * \brief Statistics of the last transfer.
   */
  const Summary& GetSummary() const { return summary; }

  /*!
   * \brief Whether a conservative flow state is admissible: density, pressure, temperature and squared speed of
   *        sound positive and finite (the checks of CEulerVariable::SetPrimVar, and finite values).
   * \param[in] fluidModel - Fluid model of the flow solver (its state is changed).
   * \param[in] nDim - Number of dimensions.
   * \param[in] solution - Conservative variables (density, momentum, total energy per volume).
   */
  static bool AdmissibleState(CFluidModel& fluidModel, unsigned short nDim, const su2double* solution);

 private:
  bool roundTripCheck;
  Summary summary;
};
