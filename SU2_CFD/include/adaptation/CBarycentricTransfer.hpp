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

#include <memory>
#include <vector>

#include "CSolutionTransfer.hpp"
#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/option_structure.hpp"

class CFluidModel;

/*!
 * \class CBarycentricLocator
 * \brief Locates points in a simplex mesh (triangles or tetrahedra): the element that contains the point and its
 *        barycentric coordinates. Uses local ADTs of the mesh (no communication, each rank searches its own mesh).
 * \note A point outside the mesh (e.g. a new boundary point on a curved boundary) gets the closest point of the mesh
 *       boundary instead: the nearest boundary face and the barycentric coordinates of the closest point in it, which
 *       are in [0,1] (the interpolated value is the one at that closest point). This is an error if the distance is
 *       larger than the longest edge of that face (the point is not near the mesh).
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
    su2double weight[MAXPOINT] = {};         /*!< \brief Barycentric weights (sum 1). */
    bool inside = true;                     /*!< \brief Whether the point is inside an element (else boundary face). */
    su2double distance = 0.0;               /*!< \brief Distance to the closest mesh point, 0 inside. */
    su2double faceSize = 0.0;               /*!< \brief Longest edge of the closest boundary face (outside only). */
  };

  /*!
   * \brief Build the search structures of a mesh.
   * \param[in] geometry - Mesh (triangles in 2D, tetrahedra in 3D; all points of the rank, halos included).
   */
  explicit CBarycentricLocator(const CGeometry& geometry);

  /*!
   * \brief Locate a point.
   * \param[in] coord - Coordinates of the point.
   * \return Donor points and weights.
   */
  Stencil Locate(const su2double* coord);

 private:
  unsigned short nDim = 0;
  std::vector<su2double> coord;          /*!< \brief Coordinates of the mesh points. */
  std::vector<unsigned long> elemConn;   /*!< \brief Nodes of the elements (nDim+1 per element). */
  std::vector<unsigned long> faceConn;   /*!< \brief Nodes of the boundary faces (nDim per face). */
  std::unique_ptr<CADTElemClass> elemADT, faceADT;
};

/*!
 * \class CBarycentricTransfer
 * \brief Transfer of the solution of the compressible flow (EULER, NAVIER_STOKES, RANS with SA or SST) by barycentric
 *        (P1) interpolation of the donor solution at each point of the new mesh. Single rank for now.
 * \note Rules:
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
 *       - Time domain (dual time stepping): the time history of every transferred solver (Solution_time_n, and
 *         Solution_time_n1 where it is allocated) is interpolated with the same stencils and the same rules as the
 *         solution, then restricted to the coarse multigrid levels (as PushSolutionBackInTime after a restart). At the
 *         adaptation point of the time-domain loop (after the dual-time update of step n) the solution and
 *         Solution_time_n are both U^n, so they are transferred to the same values; Solution_time_n1 is U^(n-1).
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
  struct Summary {
    unsigned long nPoint = 0;           /*!< \brief Points of the new mesh. */
    unsigned long nOutside = 0;         /*!< \brief Points outside the donor mesh (closest boundary point used). */
    su2double maxDistance = 0.0;        /*!< \brief Largest distance of those points to the donor mesh. */
    su2double maxRelDistance = 0.0;     /*!< \brief Largest distance relative to the size of the donor face. */
    unsigned long nFlowFixed = 0;       /*!< \brief Points where the interpolated flow state was not admissible. */
    unsigned long nHistoryFixed = 0;    /*!< \brief Same for the time history (Solution_time_n, Solution_time_n1). */
    unsigned short nTimeLevels = 0;     /*!< \brief Time history arrays transferred (0 steady, 1 or 2). */
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
