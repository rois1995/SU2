/*!
 * \file CBarycentricTransfer_tests.cpp
 * \brief Unit tests for the barycentric solution transfer between meshes (mesh adaptation).
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

#include "catch.hpp"

#include <cmath>
#include <functional>
#include <memory>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"
#include "../../../SU2_CFD/include/fluid/CIdealGas.hpp"
#include "../../../SU2_CFD/include/solvers/CSolver.hpp"
#include "../../../SU2_CFD/include/solvers/CSolverFactory.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

namespace {

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
std::unique_ptr<CConfig> MakeConfig(unsigned short nDim, const string& solverOptions) {
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
CSimplexMesh BoxMesh(unsigned short nDim, unsigned long n, bool perturb) {
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
Field AffineFlow(unsigned short nDim) {
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
passivedouble RelDiff(su2double a, su2double b, passivedouble floor = 1e-8) {
  return SU2_TYPE::GetValue(fabs(a - b) / max(max(fabs(a), fabs(b)), su2double(floor)));
}

/*--- Pressure of an ideal gas from the conservative variables. ---*/
su2double IdealGasPressure(unsigned short nDim, su2double gamma, const su2double* U) {
  su2double momentum2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) momentum2 += U[iDim + 1] * U[iDim + 1];
  return (gamma - 1.0) * (U[nDim + 1] - 0.5 * momentum2 / U[0]);
}

/*--- The coarse levels hold the restriction of the fine one (as after a restart), the old solution equals it. ---*/
void CheckCoarseLevels(const MeshSolution& target, unsigned short iSol) {
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

}  // namespace

TEST_CASE("Barycentric locator", "[Adaptation]") {
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeConfig(nDim, "SOLVER= EULER\n");
      MeshSolution mesh(config.get(), BoxMesh(nDim, 4, true), 0);
      CBarycentricLocator locator(mesh.Fine());
      const auto& nodes = *mesh.Fine().nodes;

      auto interpolate = [&](const CBarycentricLocator::Stencil& stencil, su2double* x) {
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          x[iDim] = 0.0;
          for (unsigned short k = 0; k < stencil.nPoint; ++k)
            x[iDim] += stencil.weight[k] * nodes.GetCoord(stencil.point[k], iDim);
        }
      };

      /*--- Inside: weights in [0,1] (ADT tolerance), sum 1, reproduce the point. ---*/
      for (int i = 0; i < 200; ++i) {
        su2double point[3] = {}, x[3] = {};
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          point[iDim] = (iDim == 0 && nDim == 2 ? 2.0 : 1.0) * (0.5 + 0.5 * sin(3.1 * i + 1.7 * iDim + 0.3));
        const auto stencil = locator.Locate(point);
        REQUIRE(stencil.inside);
        REQUIRE(stencil.nPoint == nDim + 1);
        su2double sum = 0.0;
        for (unsigned short k = 0; k < stencil.nPoint; ++k) {
          CHECK(stencil.weight[k] >= -1e-10);
          sum += stencil.weight[k];
        }
        CHECK(fabs(sum - 1.0) < 1e-14);
        interpolate(stencil, x);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) CHECK(fabs(x[iDim] - point[iDim]) < 1e-14);
      }

      /*--- Outside: closest point of the boundary (face interior, edge, corner), distance. ---*/
      const passivedouble d = 1e-3;
      struct Outside {
        su2double point[3], closest[3];
      };
      std::vector<Outside> cases;
      if (nDim == 2) {
        cases = {{{0.7, 1.0 + d, 0.0}, {0.7, 1.0, 0.0}},
                 {{2.0 + d, 1.0 + d, 0.0}, {2.0, 1.0, 0.0}},
                 {{-d, 0.3, 0.0}, {0.0, 0.3, 0.0}}};
      } else {
        cases = {{{0.35, 0.6, 1.0 + d}, {0.35, 0.6, 1.0}},
                 {{1.0 + d, 0.4, 1.0 + d}, {1.0, 0.4, 1.0}},
                 {{1.0 + d, 1.0 + d, 1.0 + d}, {1.0, 1.0, 1.0}},
                 {{0.2, -d, 0.7}, {0.2, 0.0, 0.7}}};
      }
      for (const auto& test : cases) {
        const auto stencil = locator.Locate(test.point);
        CHECK_FALSE(stencil.inside);
        REQUIRE(stencil.nPoint == nDim);
        su2double x[3] = {}, distance = 0.0;
        interpolate(stencil, x);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          CHECK(stencil.weight[iDim] >= 0.0);
          CHECK(stencil.weight[iDim] <= 1.0);
          CHECK(fabs(x[iDim] - test.closest[iDim]) < 1e-14);
          distance += pow(test.point[iDim] - test.closest[iDim], 2);
        }
        CHECK(fabs(stencil.distance - sqrt(distance)) < 1e-14);
        CHECK(stencil.faceSize > 0.2);
      }
    }
  }
}

TEST_CASE("Barycentric transfer: admissible states", "[Adaptation]") {
  CIdealGas gas(1.4, 287.0);
  const su2double good[] = {1.2, 100.0, -20.0, 2.5e5};
  CHECK(CBarycentricTransfer::AdmissibleState(gas, 2, good));

  const su2double negativeDensity[] = {-1.2, 100.0, -20.0, 2.5e5};
  CHECK_FALSE(CBarycentricTransfer::AdmissibleState(gas, 2, negativeDensity));

  /*--- Kinetic energy above the total energy: negative pressure and temperature. ---*/
  const su2double negativePressure[] = {1.2, 800.0, 0.0, 2.5e5};
  CHECK_FALSE(CBarycentricTransfer::AdmissibleState(gas, 2, negativePressure));

  const su2double notFinite[] = {1.2, 100.0, NAN, 2.5e5};
  CHECK_FALSE(CBarycentricTransfer::AdmissibleState(gas, 2, notFinite));
}

TEST_CASE("Barycentric transfer: affine and constant flow fields", "[Adaptation]") {
  for (const unsigned short nDim : {2, 3}) {
    for (const bool constant : {false, true}) {
      SECTION("nDim " + std::to_string(nDim) + (constant ? ", constant" : ", affine")) {
        auto config = MakeConfig(nDim, "SOLVER= EULER\n");
        MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 2);
        const Field field = constant ? Field([nDim](const su2double*, su2double* U) {
          const su2double x0[3] = {0.3, 0.4, 0.5};
          AffineFlow(nDim)(x0, U);
        })
                                     : AffineFlow(nDim);
        donor.SetField(FLOW_SOL, field);

        MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 2);
        CBarycentricTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        const auto& summary = transfer.GetSummary();
        CHECK(summary.nPoint == target.Fine().GetnPoint());
        CHECK(summary.nOutside == 0);
        CHECK(summary.nFlowFixed == 0);

        /*--- Exact at every point (interior and boundary), primitives from the transferred state. ---*/
        const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
        const auto tol = constant ? 1e-14 : 1e-12;
        passivedouble maxDiff = 0.0, maxPressureDiff = 0.0;
        for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
          su2double exact[MAXVAR] = {}, U[MAXVAR] = {};
          field(target.Fine().nodes->GetCoord(iPoint), exact);
          for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
            U[iVar] = nodes->GetSolution(iPoint, iVar);
            maxDiff = max(maxDiff, RelDiff(U[iVar], exact[iVar]));
          }
          maxPressureDiff =
              max(maxPressureDiff, RelDiff(nodes->GetPressure(iPoint), IdealGasPressure(nDim, config->GetGamma(), U)));
        }
        CHECK(maxDiff < tol);
        CHECK(maxPressureDiff < 1e-12);

        CheckCoarseLevels(target, FLOW_SOL);

        /*--- Same box: same volume; constant field: same integrals; affine: exact round trip. ---*/
        CHECK(RelDiff(summary.newVolume, summary.donorVolume) < 1e-12);
        for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
          if (constant) CHECK(RelDiff(summary.newIntegral[iVar], summary.donorIntegral[iVar]) < 1e-12);
          CHECK(summary.roundTripLinf[iVar] < 1e-10);
        }
      }
    }
  }
}

TEST_CASE("Barycentric transfer: point outside the donor mesh", "[Adaptation]") {
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeConfig(nDim, "SOLVER= EULER\n");
      MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 0);
      donor.SetField(FLOW_SOL, AffineFlow(nDim));

      /*--- Move the boundary point nearest to (0.7, 1) / (0.35, 0.6, 1) outwards by d. ---*/
      const passivedouble d = 1e-3;
      auto mesh = BoxMesh(nDim, 6, true);
      const passivedouble target2D[] = {0.7, 1.0}, target3D[] = {0.35, 0.6, 1.0};
      const auto* goal = (nDim == 2) ? target2D : target3D;
      unsigned long moved = 0;
      passivedouble best = 1e300;
      for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint) {
        passivedouble dist = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) dist += pow(mesh.coord[iPoint * nDim + iDim] - goal[iDim], 2);
        if (dist < best) {
          best = dist;
          moved = iPoint;
        }
      }
      su2double projection[3] = {};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) projection[iDim] = mesh.coord[moved * nDim + iDim];
      mesh.coord[moved * nDim + nDim - 1] += d;

      MeshSolution target(config.get(), mesh, 0);
      CBarycentricTransfer transfer;
      {
        Mute mute;
        transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }
      const auto& summary = transfer.GetSummary();
      CHECK(summary.nOutside == 1);
      CHECK(fabs(summary.maxDistance - d) < 1e-12);
      CHECK(summary.nFlowFixed == 0);

      /*--- The moved point gets the value at its projection on the donor boundary, the others the exact one. ---*/
      const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
      for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
        const auto* x = target.Fine().nodes->GetCoord(iPoint);
        su2double dist = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) dist += pow(x[iDim] - projection[iDim], 2);
        const bool isMoved = sqrt(dist) < 2 * d;
        su2double exact[MAXVAR] = {};
        AffineFlow(nDim)(isMoved ? projection : x, exact);
        passivedouble maxDiff = 0.0;
        for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar)
          maxDiff = max(maxDiff, RelDiff(nodes->GetSolution(iPoint, iVar), exact[iVar]));
        CHECK(maxDiff < 1e-12);
      }
    }
  }
}

TEST_CASE("Barycentric transfer: inadmissible interpolated state", "[Adaptation]") {
  const unsigned short nDim = 2;
  auto config = MakeConfig(nDim, "SOLVER= EULER\n");

  /*--- Donor: affine field, but zero total energy (negative pressure) at the point nearest to (1, 0.5). The new mesh
   *    contains the donor points (twice finer), so one new point has that state as its interpolated state. ---*/
  MeshSolution donor(config.get(), BoxMesh(nDim, 4, false), 0);
  donor.SetField(FLOW_SOL, AffineFlow(nDim));
  unsigned long bad = 0;
  su2double badCoord[2] = {};
  passivedouble best = 1e300;
  for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
    const auto* x = donor.Fine().nodes->GetCoord(iPoint);
    const passivedouble dist = SU2_TYPE::GetValue(pow(x[0] - 1.0, 2) + pow(x[1] - 0.5, 2));
    if (dist < best) {
      best = dist;
      bad = iPoint;
      badCoord[0] = x[0];
      badCoord[1] = x[1];
    }
  }
  auto* donorNodes = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
  donorNodes->SetSolution(bad, nDim + 1, 0.0);

  MeshSolution target(config.get(), BoxMesh(nDim, 8, false), 0);
  CBarycentricTransfer transfer;
  {
    Mute mute;
    transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
  }
  CHECK(transfer.GetSummary().nFlowFixed >= 1);

  auto* fluidModel = target.solver[MESH_0][FLOW_SOL]->GetFluidModel();
  const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
  const passivedouble hDonor = 0.25;
  for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
    su2double U[MAXVAR] = {};
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) U[iVar] = nodes->GetSolution(iPoint, iVar);
    CHECK(CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, U));

    const auto* x = target.Fine().nodes->GetCoord(iPoint);
    const passivedouble dist = SU2_TYPE::GetValue(sqrt(pow(x[0] - badCoord[0], 2) + pow(x[1] - badCoord[1], 2)));

    if (dist < 1e-12) {
      /*--- At the bad donor point: the state of another donor point. ---*/
      bool found = false;
      for (auto jPoint = 0ul; jPoint < donor.Fine().GetnPoint() && !found; ++jPoint) {
        if (jPoint == bad) continue;
        bool same = true;
        for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar)
          same &= (U[iVar] == donorNodes->GetSolution(jPoint, iVar));
        found = same;
      }
      CHECK(found);
    } else if (dist > 1.5 * hDonor) {
      /*--- Away from it: the exact affine value. ---*/
      su2double exact[MAXVAR] = {};
      AffineFlow(nDim)(x, exact);
      passivedouble maxDiff = 0.0;
      for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) maxDiff = max(maxDiff, RelDiff(U[iVar], exact[iVar]));
      CHECK(maxDiff < 1e-12);
    }
  }
}

TEST_CASE("Barycentric transfer: turbulence variables", "[Adaptation]") {
  const unsigned short nDim = 2;
  struct TurbCase {
    string name, options;
    Field field;
    bool clippedAtEps;
  };
  /*--- SA: positive field; negative SA variant: field that changes sign (kept); standard SA with the same field:
   *    values below the solver's lower bound are limited to it. SST: positive k and omega. ---*/
  auto changesSign = [](const su2double* x, su2double* v) { v[0] = -5e-5 + 1e-4 * x[0] - 2e-5 * x[1]; };
  const std::vector<TurbCase> cases = {
      {"SA", "KIND_TURB_MODEL= SA\n", [](const su2double* x, su2double* v) { v[0] = 1e-4 + 2e-5 * x[0] - 1e-5 * x[1]; },
       false},
      {"SA negative", "KIND_TURB_MODEL= SA\nSA_OPTIONS= (NEGATIVE, WITHFT2)\n", changesSign, false},
      {"SA limited", "KIND_TURB_MODEL= SA\n", changesSign, true},
      {"SST", "KIND_TURB_MODEL= SST\n",
       [](const su2double* x, su2double* v) {
         v[0] = 1.0 + 0.1 * x[0] - 0.2 * x[1];
         v[1] = 1e3 + 100.0 * x[0] + 50.0 * x[1];
       },
       false},
  };

  for (const auto& test : cases) {
    SECTION(test.name) {
      auto config = MakeConfig(nDim, "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\n" + test.options);
      MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 2);
      donor.SetField(FLOW_SOL, AffineFlow(nDim));
      donor.SetField(TURB_SOL, test.field);

      MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 2);
      CBarycentricTransfer transfer;
      {
        Mute mute;
        transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }

      const auto* turbNodes = target.solver[MESH_0][TURB_SOL]->GetNodes();
      const auto nVar = target.solver[MESH_0][TURB_SOL]->GetnVar();
      unsigned long nBelow = 0;
      passivedouble maxDiff = 0.0;
      bool negative = false;
      for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
        su2double exact[MAXVAR] = {};
        test.field(target.Fine().nodes->GetCoord(iPoint), exact);
        for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
          if (test.clippedAtEps && exact[iVar] < EPS) {
            nBelow++;
            exact[iVar] = EPS;
          }
          const su2double value = turbNodes->GetSolution(iPoint, iVar);
          negative |= (value < 0.0);
          maxDiff = max(maxDiff, RelDiff(value, exact[iVar], 1e-6));
        }
        /*--- The eddy viscosity is computed by the solver. ---*/
        CHECK(std::isfinite(SU2_TYPE::GetValue(turbNodes->GetmuT(iPoint))));
        CHECK(turbNodes->GetmuT(iPoint) >= 0.0);
      }
      CHECK(maxDiff < 1e-12);
      CHECK(transfer.GetSummary().nTurbLimited == nBelow);
      CHECK(negative == (test.name == "SA negative"));
      if (test.clippedAtEps) CHECK(nBelow > 0);

      CheckCoarseLevels(target, FLOW_SOL);
      CheckCoarseLevels(target, TURB_SOL);
    }
  }
}
