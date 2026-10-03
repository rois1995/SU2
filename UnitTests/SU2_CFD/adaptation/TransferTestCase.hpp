/*!
 * \file TransferTestCase.hpp
 * \brief Meshes, configs, solvers and fields shared by the unit tests of the solution transfers (mesh adaptation).
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

#include "catch.hpp"

#include <cmath>
#include <functional>
#include <memory>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../SU2_CFD/include/adaptation/CSolutionTransfer.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"
#include "../../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../../SU2_CFD/include/solvers/CSolverFactory.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

namespace transfer_test {

using Field = std::function<void(const su2double* x, su2double* values)>;

constexpr unsigned short MAXVAR = 8; /*!< \brief Enough for the flow and turbulence variables of the tests. */

/*--- Silence the console in a scope. ---*/
struct Mute {
  std::streambuf* buffer = cout.rdbuf();
  Mute() { cout.rdbuf(nullptr); }
  ~Mute() { cout.rdbuf(buffer); }
};

/*!
 * \brief Config of the test cases (rectangle or cube of SimplexMeshTestCase.hpp): far field everywhere, or heat flux
 *        walls on the lower/z-minus markers for RANS; two multigrid levels requested.
 */
inline std::unique_ptr<CConfig> MakeConfig(unsigned short nDim, const string& solverOptions) {
  const bool rans = solverOptions.find("RANS") != string::npos;
  string markers;
  if (nDim == 2) {
    markers = rans ? "MARKER_FAR= (left, right, upper)\nMARKER_HEATFLUX= (lower_b, 0.0, lower_a, 0.0)\n"
                   : "MARKER_FAR= (left, right, upper, lower_a, lower_b)\n";
  } else {
    markers = rans ? "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n"
                     "MARKER_HEATFLUX= (z_minus_b, 0.0, z_minus_a, 0.0)\n"
                   : "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n";
  }
  stringstream options(solverOptions + "MACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\n" + markers +
                       "MGLEVEL= 2\nMG_MIN_MESHSIZE= 4\n");
  Mute mute;
  return std::unique_ptr<CConfig>(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
}

/*!
 * \brief Geometry (all multigrid levels) and solvers of a mesh in memory, built as the driver does in ReplaceMesh.
 */
struct MeshSolution {
  CGeometry** geometry = nullptr;
  CSolver*** solver = nullptr;
  unsigned short nMGLevels = 0;
  std::vector<std::string> markerTags; /*!< \brief Marker names of the geometry (the config changes with each mesh). */

  MeshSolution(CConfig* config, const CSimplexMesh& mesh, unsigned short requestedMGLevels) {
    Mute mute;
    config->SetMGLevels(requestedMGLevels);
    CMemoryMeshReaderFVM reader(config, mesh, 0, 1);
    CDriver::BuildGeometryFVM(config, new CPhysicalGeometry(config, reader, 1), geometry, true);
    geometry[MESH_0]->SetPositive_ZArea(config);
    CGeometry** instances[] = {geometry};
    CGeometry*** zones[] = {instances};
    const CConfig* configs[] = {config};
    CGeometry::ComputeWallDistance(configs, zones);

    for (unsigned short iMarker = 0; iMarker < geometry[MESH_0]->GetnMarker(); ++iMarker)
      markerTags.push_back(config->GetMarker_All_TagBound(iMarker));

    nMGLevels = config->GetnMGLevels();
    solver = new CSolver**[nMGLevels + 1];
    for (unsigned short iMesh = 0; iMesh <= nMGLevels; ++iMesh)
      solver[iMesh] = CSolverFactory::CreateSolverContainer(config->GetKind_Solver(), config, geometry[iMesh], iMesh);
  }

  ~MeshSolution() {
    for (unsigned short iMesh = 0; iMesh <= nMGLevels; ++iMesh) {
      for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
        CSolverFactory::ClearSolverMeta(solver[iMesh][iSol]);
        delete solver[iMesh][iSol];
      }
      delete[] solver[iMesh];
      delete geometry[iMesh];
    }
    delete[] solver;
    delete[] geometry;
  }

  CGeometry& Fine() const { return *geometry[MESH_0]; }

  CMeshDonor Donor() const {
    CMeshDonor donor;
    donor.geometry = geometry;
    donor.solver = solver;
    donor.nMGLevels = nMGLevels;
    donor.markerTags = markerTags;
    return donor;
  }

  /*--- Set the solution of the fine level of a solver from a field. ---*/
  void SetField(unsigned short iSol, const Field& field) {
    auto* nodes = solver[MESH_0][iSol]->GetNodes();
    su2double values[MAXVAR] = {};
    for (auto iPoint = 0ul; iPoint < Fine().GetnPoint(); ++iPoint) {
      field(Fine().nodes->GetCoord(iPoint), values);
      for (unsigned short iVar = 0; iVar < solver[MESH_0][iSol]->GetnVar(); ++iVar)
        nodes->SetSolution(iPoint, iVar, values[iVar]);
    }
  }
};

/*--- Rectangle [0,2]x[0,1] or unit cube with n cells per unit length; interior points moved (deterministic, up to 0.1
 *    of the cell size in each direction) so that two meshes share no edges. ---*/
inline CSimplexMesh BoxMesh(unsigned short nDim, unsigned long n, bool perturb) {
  auto mesh = simplex_test::MakeSimplexMesh(nDim, n, nDim == 2 ? simplex_test::Marker2D : simplex_test::Marker3D);
  if (!perturb) return mesh;
  const passivedouble h = 1.0 / n, upper[] = {nDim == 2 ? 2.0 : 1.0, 1.0, 1.0};
  for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint) {
    auto* x = &mesh.coord[iPoint * nDim];
    bool interior = true;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) interior &= (x[iDim] > 1e-12 && x[iDim] < upper[iDim] - 1e-12);
    if (!interior) continue;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] += 0.1 * h * sin(12.9898 * (iPoint + 1) + 78.233 * iDim);
  }
  return mesh;
}

/*--- Conservative flow variables, affine in the coordinates (admissible on the boxes). ---*/
inline Field AffineFlow(unsigned short nDim) {
  return [nDim](const su2double* x, su2double* U) {
    const su2double z = (nDim == 3) ? x[2] : su2double(0.0);
    U[0] = 1.2 + 0.1 * x[0] - 0.05 * x[1] + 0.02 * z;
    U[1] = 400.0 + 20.0 * x[0] - 10.0 * x[1] + 5.0 * z;
    U[2] = -30.0 + 5.0 * x[0] + 8.0 * x[1] - 3.0 * z;
    if (nDim == 3) U[3] = 12.0 - 4.0 * x[0] + 2.0 * x[1] + 6.0 * z;
    U[nDim + 1] = 2.5e5 + 1e3 * x[0] - 2e3 * x[1] + 500.0 * z;
  };
}

/*--- Relative difference, with a floor for values near zero. ---*/
inline passivedouble RelDiff(su2double a, su2double b, passivedouble floor = 1e-8) {
  return SU2_TYPE::GetValue(fabs(a - b) / max(max(fabs(a), fabs(b)), su2double(floor)));
}

/*--- Pressure of an ideal gas from the conservative variables. ---*/
inline su2double IdealGasPressure(unsigned short nDim, su2double gamma, const su2double* U) {
  su2double momentum2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) momentum2 += U[iDim + 1] * U[iDim + 1];
  return (gamma - 1.0) * (U[nDim + 1] - 0.5 * momentum2 / U[0]);
}

/*--- The coarse levels hold the restriction of the fine one (as after a restart), the old solution equals it. ---*/
inline void CheckCoarseLevels(const MeshSolution& target, unsigned short iSol) {
  REQUIRE(target.nMGLevels >= 1);
  for (unsigned short iMesh = 1; iMesh <= target.nMGLevels; ++iMesh) {
    const auto* fineNodes = target.solver[iMesh - 1][iSol]->GetNodes();
    auto* nodes = target.solver[iMesh][iSol]->GetNodes();
    su2activematrix restricted = nodes->GetSolution();
    CSolver::MultigridRestriction(*target.geometry[iMesh - 1], fineNodes->GetSolution(), *target.geometry[iMesh],
                                  restricted);
    passivedouble maxDiff = 0.0, maxOld = 0.0;
    for (auto iPoint = 0ul; iPoint < target.geometry[iMesh]->GetnPointDomain(); ++iPoint) {
      for (unsigned short iVar = 0; iVar < target.solver[iMesh][iSol]->GetnVar(); ++iVar) {
        maxDiff = max(maxDiff, RelDiff(nodes->GetSolution(iPoint, iVar), restricted(iPoint, iVar)));
        maxOld = max(maxOld, RelDiff(nodes->GetSolution(iPoint, iVar), nodes->GetSolution_Old(iPoint, iVar)));
      }
    }
    CHECK(maxDiff < 1e-14);
    CHECK(maxOld == 0.0);
  }
}

/*--- Config of the disk/ball of SimplexMeshTestCase.hpp, the boundary markers given by markerOptions. ---*/
inline std::unique_ptr<CConfig> MakeRoundConfig(const string& solverOptions, const string& markerOptions) {
  stringstream options(solverOptions + "MACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\n" +
                       markerOptions + "MGLEVEL= 0\n");
  Mute mute;
  return std::unique_ptr<CConfig>(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
}

/*--- A smooth admissible flow field (not affine). ---*/
inline void SmoothFlow(unsigned short nDim, const su2double* x, su2double* U) {
  const su2double z = (nDim == 3) ? x[2] : su2double(0.0);
  U[0] = 1.2 + 0.1 * sin(3.0 * x[0]) * cos(2.0 * x[1]) + 0.05 * z * z;
  U[1] = 400.0 + 50.0 * cos(2.0 * x[0] + x[1]) - 20.0 * z;
  U[2] = -30.0 + 40.0 * sin(x[0] - 2.0 * x[1]);
  if (nDim == 3) U[3] = 12.0 + 30.0 * cos(3.0 * z) * x[0];
  U[nDim + 1] = 2.5e5 + 2e4 * x[0] * x[1] - 1e4 * cos(z);
}

/*--- Internal energy per unit mass as the solver computes it (CNSVariable::SetPrimVar): total energy minus kinetic
 *    energy, minus the turbulent kinetic energy k with SST. ---*/
inline su2double InternalEnergy(unsigned short nDim, const su2double* U, su2double k) {
  su2double momentum2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) momentum2 += U[iDim + 1] * U[iDim + 1];
  return U[nDim + 1] / U[0] - 0.5 * momentum2 / (U[0] * U[0]) - k;
}

/*!
 * \brief The two SST states of the review (zero momentum, internal energy 0.1 for both): A = (rho 2, rho E 2.2, k 1),
 *        B = (rho 1, rho E 2.1, k 2), omega 1e3. A donor point of the box mesh with n cells per unit length gets A
 *        when the sum of its grid indices plus level is even, so every element mixes them. With k interpolated by
 *        itself the mixed states have rho E / rho > 0 but e = rho E / rho - k < 0 (-0.067 at equal weights).
 */
inline bool TwoStateA(unsigned short nDim, unsigned long n, int level, const su2double* x) {
  long sum = level;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) sum += lround(SU2_TYPE::GetValue(x[iDim]) * n);
  return sum % 2 == 0;
}
inline void TwoStateFlow(unsigned short nDim, bool a, su2double* U) {
  for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) U[iVar] = 0.0;
  U[0] = a ? 2.0 : 1.0;
  U[nDim + 1] = a ? 2.2 : 2.1;
}
inline void TwoStateTurb(bool a, su2double* v) {
  v[0] = a ? 1.0 : 2.0;
  v[1] = 1e3;
}

/*--- Set the solution, Solution_time_n (level 0) and Solution_time_n1 (level 1) of the flow and SST solvers of the
 *    fine level to the two-state pattern. ---*/
inline void SetTwoStates(MeshSolution& mesh, unsigned short nDim, unsigned long n) {
  auto* flow = mesh.solver[MESH_0][FLOW_SOL]->GetNodes();
  auto* turb = mesh.solver[MESH_0][TURB_SOL]->GetNodes();
  for (auto iPoint = 0ul; iPoint < mesh.Fine().GetnPoint(); ++iPoint) {
    for (int level = 0; level < 2; ++level) {
      const bool a = TwoStateA(nDim, n, level, mesh.Fine().nodes->GetCoord(iPoint));
      su2double U[MAXVAR], v[MAXVAR];
      TwoStateFlow(nDim, a, U);
      TwoStateTurb(a, v);
      for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
        if (level == 0) {
          flow->GetSolution()(iPoint, iVar) = U[iVar];
          flow->GetSolution_time_n()(iPoint, iVar) = U[iVar];
        } else {
          flow->GetSolution_time_n1()(iPoint, iVar) = U[iVar];
        }
      }
      for (unsigned short iVar = 0; iVar < 2; ++iVar) {
        if (level == 0) {
          turb->GetSolution()(iPoint, iVar) = v[iVar];
          turb->GetSolution_time_n()(iPoint, iVar) = v[iVar];
        } else {
          turb->GetSolution_time_n1()(iPoint, iVar) = v[iVar];
        }
      }
    }
  }
}

/*--- Smallest internal energy (the solver's, k subtracted) over the fine level of the new mesh, for the solution
 *    (level 0) or Solution_time_n1 (level 1); also the largest deviation from 0.1. ---*/
inline void JointEnergyRange(const MeshSolution& mesh, unsigned short nDim, int level, passivedouble& minEnergy,
                             passivedouble& maxDeviation) {
  auto* flow = mesh.solver[MESH_0][FLOW_SOL]->GetNodes();
  auto* turb = mesh.solver[MESH_0][TURB_SOL]->GetNodes();
  minEnergy = 1e300;
  maxDeviation = 0.0;
  for (auto iPoint = 0ul; iPoint < mesh.Fine().GetnPoint(); ++iPoint) {
    su2double U[MAXVAR];
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar)
      U[iVar] = level == 0 ? flow->GetSolution(iPoint, iVar) : flow->GetSolution_time_n1()(iPoint, iVar);
    const su2double k = level == 0 ? turb->GetSolution(iPoint, 0) : turb->GetSolution_time_n1()(iPoint, 0);
    const passivedouble e = SU2_TYPE::GetValue(InternalEnergy(nDim, U, k));
    minEnergy = std::min(minEnergy, e);
    maxDeviation = std::max(maxDeviation, fabs(e - 0.1));
  }
}

}  // namespace transfer_test
