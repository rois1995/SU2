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

TEST_CASE("Barycentric transfer: time history (dual time stepping)", "[Adaptation]") {
  /*--- Three different affine fields for U^(n+1) (solution), U^n and U^(n-1), flow and SA: each array is exact on
   *    the new mesh, and the coarse levels of the history hold its restriction (as after a restart). ---*/
  using Getter = su2activematrix& (*)(CVariable*);
  const Getter getters[] = {[](CVariable* n) -> su2activematrix& { return n->GetSolution(); },
                            [](CVariable* n) -> su2activematrix& { return n->GetSolution_time_n(); },
                            [](CVariable* n) -> su2activematrix& { return n->GetSolution_time_n1(); }};
  const char* names[] = {"solution", "time n", "time n-1"};

  auto flowField = [](unsigned short nDim, int level) {
    return Field([nDim, level](const su2double* x, su2double* U) {
      AffineFlow(nDim)(x, U);
      U[0] *= 1.0 + 0.05 * level;
      U[1] += 7.0 * level * x[1];
      U[2] -= 3.0 * level * x[0];
      U[nDim + 1] += 1e3 * level * x[0];
    });
  };
  auto turbField = [](int level) {
    return Field([level](const su2double* x, su2double* v) { v[0] = 1e-4 + 2e-5 * x[0] - (1e-5 + 1e-6 * level) * x[1]; });
  };

  struct TimeCase {
    unsigned short nDim;
    string options;
  };
  const std::vector<TimeCase> cases = {
      {2, "SOLVER= EULER\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n"},
      {3, "SOLVER= EULER\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n"},
      {2, "SOLVER= EULER\nTIME_MARCHING= DUAL_TIME_STEPPING-1ST_ORDER\n"},
      {2, "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SA\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n"},
  };

  for (const auto& test : cases) {
    SECTION("nDim " + std::to_string(test.nDim) + ", " + test.options) {
      const auto nDim = test.nDim;
      auto config = MakeConfig(nDim, test.options + "TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\n");
      const bool rans = config->GetKind_Solver() == MAIN_SOLVER::RANS;
      std::vector<unsigned short> solvers = {FLOW_SOL};
      if (rans) solvers.push_back(TURB_SOL);

      MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 2);
      for (int level = 0; level < 3; ++level) {
        for (const auto iSol : solvers) {
          auto& array = getters[level](donor.solver[MESH_0][iSol]->GetNodes());
          REQUIRE(array.rows() == donor.Fine().GetnPoint());
          const auto field = (iSol == FLOW_SOL) ? flowField(nDim, level) : turbField(level);
          su2double values[MAXVAR] = {};
          for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
            field(donor.Fine().nodes->GetCoord(iPoint), values);
            for (unsigned short iVar = 0; iVar < array.cols(); ++iVar) array(iPoint, iVar) = values[iVar];
          }
        }
      }

      MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 2);
      CBarycentricTransfer transfer;
      {
        Mute mute;
        transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }
      const auto& summary = transfer.GetSummary();
      CHECK(summary.nTimeLevels == 2);
      CHECK(summary.nOutside == 0);
      CHECK(summary.nFlowFixed == 0);
      CHECK(summary.nHistoryFixed == 0);
      CHECK(summary.nTurbLimited == 0);

      for (int level = 0; level < 3; ++level) {
        for (const auto iSol : solvers) {
          INFO(names[level] << ", solver " << iSol);
          const auto field = (iSol == FLOW_SOL) ? flowField(nDim, level) : turbField(level);

          /*--- Exact at every point of the fine level. ---*/
          const auto& array = getters[level](target.solver[MESH_0][iSol]->GetNodes());
          passivedouble maxDiff = 0.0;
          for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
            su2double exact[MAXVAR] = {};
            field(target.Fine().nodes->GetCoord(iPoint), exact);
            for (unsigned short iVar = 0; iVar < array.cols(); ++iVar)
              maxDiff = max(maxDiff, RelDiff(array(iPoint, iVar), exact[iVar], 1e-6));
          }
          CHECK(maxDiff < 1e-12);

          /*--- History on the coarse levels: restriction of the finer level. ---*/
          if (level == 0) continue;
          for (unsigned short iMesh = 1; iMesh <= target.nMGLevels; ++iMesh) {
            const auto& fine = getters[level](target.solver[iMesh - 1][iSol]->GetNodes());
            const auto& coarse = getters[level](target.solver[iMesh][iSol]->GetNodes());
            su2activematrix restricted = coarse;
            CSolver::MultigridRestriction(*target.geometry[iMesh - 1], fine, *target.geometry[iMesh], restricted);
            passivedouble maxCoarse = 0.0;
            for (auto iPoint = 0ul; iPoint < target.geometry[iMesh]->GetnPointDomain(); ++iPoint)
              for (unsigned short iVar = 0; iVar < coarse.cols(); ++iVar)
                maxCoarse = max(maxCoarse, RelDiff(coarse(iPoint, iVar), restricted(iPoint, iVar), 1e-6));
            CHECK(maxCoarse < 1e-14);
          }
        }
      }
      CheckCoarseLevels(target, FLOW_SOL);
    }
  }
}

namespace {

/*--- Independent distance of a point to a segment / triangle (brute-force reference for the nearest-face search). ---*/
passivedouble DistanceSegment(unsigned short nDim, const su2double* a, const su2double* b, const su2double* p) {
  passivedouble ab2 = 0.0, t = 0.0;
  for (unsigned short i = 0; i < nDim; ++i) {
    ab2 += SU2_TYPE::GetValue((b[i] - a[i]) * (b[i] - a[i]));
    t += SU2_TYPE::GetValue((p[i] - a[i]) * (b[i] - a[i]));
  }
  t = std::min(1.0, std::max(0.0, t / ab2));
  passivedouble d2 = 0.0;
  for (unsigned short i = 0; i < nDim; ++i) d2 += pow(SU2_TYPE::GetValue(p[i] - a[i] - t * (b[i] - a[i])), 2);
  return sqrt(d2);
}

passivedouble DistanceTriangle(const su2double* a, const su2double* b, const su2double* c, const su2double* p) {
  /*--- Projection on the plane by the normal equations of the two edge vectors; inside: distance to the plane, else
   *    the nearest edge. ---*/
  passivedouble u[3], v[3], w[3];
  for (int i = 0; i < 3; ++i) {
    u[i] = SU2_TYPE::GetValue(b[i] - a[i]);
    v[i] = SU2_TYPE::GetValue(c[i] - a[i]);
    w[i] = SU2_TYPE::GetValue(p[i] - a[i]);
  }
  auto dot = [](const passivedouble* x, const passivedouble* y) { return x[0] * y[0] + x[1] * y[1] + x[2] * y[2]; };
  const passivedouble uu = dot(u, u), uv = dot(u, v), vv = dot(v, v), wu = dot(w, u), wv = dot(w, v);
  const passivedouble det = uu * vv - uv * uv;
  const passivedouble s = (wu * vv - wv * uv) / det, t = (wv * uu - wu * uv) / det;
  if (s >= 0.0 && t >= 0.0 && s + t <= 1.0) {
    passivedouble d2 = 0.0;
    for (int i = 0; i < 3; ++i) d2 += pow(w[i] - s * u[i] - t * v[i], 2);
    return sqrt(d2);
  }
  return std::min({DistanceSegment(3, a, b, p), DistanceSegment(3, b, c, p), DistanceSegment(3, c, a, p)});
}

/*--- Smallest distance of a point to the boundary faces of a geometry (all markers). ---*/
passivedouble BruteForceDistance(const CGeometry& geometry, const su2double* p) {
  const auto nDim = geometry.GetnDim();
  passivedouble best = std::numeric_limits<passivedouble>::max();
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker)
    for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
      const auto* face = geometry.bound[iMarker][iElem];
      const auto* a = geometry.nodes->GetCoord(face->GetNode(0));
      const auto* b = geometry.nodes->GetCoord(face->GetNode(1));
      best = std::min(best, nDim == 2 ? DistanceSegment(2, a, b, p)
                                      : DistanceTriangle(a, b, geometry.nodes->GetCoord(face->GetNode(2)), p));
    }
  return best;
}

/*--- Config of the disk/ball of SimplexMeshTestCase.hpp, the boundary markers given by markerOptions. ---*/
std::unique_ptr<CConfig> MakeRoundConfig(const string& solverOptions, const string& markerOptions) {
  stringstream options(solverOptions + "MACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\n" +
                       markerOptions + "MGLEVEL= 0\n");
  Mute mute;
  return std::unique_ptr<CConfig>(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
}

/*--- A smooth admissible flow field (not affine). ---*/
void SmoothFlow(unsigned short nDim, const su2double* x, su2double* U) {
  const su2double z = (nDim == 3) ? x[2] : su2double(0.0);
  U[0] = 1.2 + 0.1 * sin(3.0 * x[0]) * cos(2.0 * x[1]) + 0.05 * z * z;
  U[1] = 400.0 + 50.0 * cos(2.0 * x[0] + x[1]) - 20.0 * z;
  U[2] = -30.0 + 40.0 * sin(x[0] - 2.0 * x[1]);
  if (nDim == 3) U[3] = 12.0 + 30.0 * cos(3.0 * z) * x[0];
  U[nDim + 1] = 2.5e5 + 2e4 * x[0] * x[1] - 1e4 * cos(z);
}

}  // namespace

TEST_CASE("Barycentric locator: nearest boundary face", "[Adaptation]") {
  /*--- Points placed outside (and inside) convex and concave boundaries at distances from 1e-6 to 10 times the domain
   *    size: the nearest face of the ADT search is the true nearest face (brute force over all faces). Concave cases:
   *    a U-shaped rectangle (2D), a cube without one corner octant (3D, reentrant edges and corner). ---*/
  struct Case {
    string name;
    unsigned short nDim;
    std::function<CSimplexMesh()> mesh;
  };
  const std::vector<Case> cases = {
      {"disk", 2, [] { return simplex_test::MakeRoundMesh(2, 6, 1.0); }},
      {"U shape", 2,
       [] {
         return simplex_test::MakeSimplexMesh(2, 4, [](const passivedouble*) { return string("wall"); },
                                              [](const passivedouble* x) { return !(x[0] > 0.5 && x[0] < 1.5 && x[1] > 0.5); });
       }},
      {"ball", 3, [] { return simplex_test::MakeRoundMesh(3, 4, 1.0); }},
      {"cube without a corner", 3,
       [] {
         return simplex_test::MakeSimplexMesh(3, 4, [](const passivedouble*) { return string("wall"); },
                                              [](const passivedouble* x) { return !(x[0] > 0.5 && x[1] > 0.5 && x[2] > 0.5); });
       }},
  };
  for (const auto& test : cases) {
    SECTION(test.name) {
      const auto nDim = test.nDim;
      const bool round = test.name == "disk" || test.name == "ball";
      auto config = MakeRoundConfig("SOLVER= EULER\n", round ? "MARKER_EULER= (round_a, round_b)\n" : "MARKER_EULER= (wall)\n");
      MeshSolution mesh(config.get(), test.mesh(), 0);
      CBarycentricLocator locator(mesh.Fine(), mesh.markerTags);
      const std::vector<string> all = mesh.markerTags;

      unsigned long nOutside = 0;
      passivedouble maxError = 0.0;
      for (int i = 0; i < 400; ++i) {
        /*--- Around a boundary point: random direction, distance from 1e-6 to 10 x the domain size. ---*/
        const auto iVertex = (37ul * i) % mesh.Fine().GetnVertex(i % mesh.Fine().GetnMarker());
        const auto* x0 = mesh.Fine().nodes->GetCoord(mesh.Fine().vertex[i % mesh.Fine().GetnMarker()][iVertex]->GetNode());
        const passivedouble distance = pow(10.0, -6.0 + 7.3 * (0.5 + 0.5 * sin(2.3 * i)));
        su2double dir[3] = {}, x[3] = {}, norm = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          dir[iDim] = sin(1.7 * i + 2.9 * iDim + 0.4);
          norm += dir[iDim] * dir[iDim];
        }
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = x0[iDim] + distance * dir[iDim] / sqrt(norm);

        const passivedouble reference = BruteForceDistance(mesh.Fine(), x);
        const auto onBoundary = locator.LocateOnBoundary(x, all);
        REQUIRE(onBoundary.onFace);
        maxError = std::max(maxError, SU2_TYPE::GetValue(fabs(onBoundary.distance - reference)) / std::max(1.0, reference));
        su2double sum = 0.0;
        for (unsigned short k = 0; k < onBoundary.nPoint; ++k) {
          CHECK(onBoundary.weight[k] >= 0.0);
          sum += onBoundary.weight[k];
        }
        CHECK(fabs(sum - 1.0) < 1e-14);

        const auto stencil = locator.Locate(x);
        if (!stencil.inside) {
          nOutside++;
          CHECK(SU2_TYPE::GetValue(fabs(stencil.distance - reference)) < 1e-12 * std::max(1.0, reference));
        }
      }
      CHECK(maxError < 1e-12);
      CHECK(nOutside > 100);
    }
  }
}

TEST_CASE("Barycentric transfer: curved boundary outside the donor", "[Adaptation]") {
  /*--- Donor: disk/ball whose boundary is a polygon/polyhedron inscribed in the circle/sphere; new mesh: finer, its
   *    boundary points on the circle/sphere, so they lie outside the donor (as after remeshing a curved boundary). ---*/
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeRoundConfig("SOLVER= EULER\n", "MARKER_EULER= (round_a, round_b)\n");
      MeshSolution donor(config.get(), simplex_test::MakeRoundMesh(nDim, 4, 1.0), 0);
      MeshSolution target(config.get(), simplex_test::MakeRoundMesh(nDim, 6, 1.0), 0);
      const auto nPoint = target.Fine().GetnPoint();
      const auto nVar = nDim + 2;

      /*--- Stencils as the transfer computes them (same rule), for the checks below. ---*/
      CBarycentricLocator locator(donor.Fine(), donor.markerTags, 2.0 * config->GetAdap_Hausd());
      std::vector<CBarycentricLocator::Stencil> stencils(nPoint);
      std::vector<bool> onMarker(nPoint, false);
      for (unsigned short iMarker = 0; iMarker < target.Fine().GetnMarker(); ++iMarker)
        for (auto iVertex = 0ul; iVertex < target.Fine().GetnVertex(iMarker); ++iVertex)
          onMarker[target.Fine().vertex[iMarker][iVertex]->GetNode()] = true;
      unsigned long nOutside = 0, nOff = 0;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto* x = target.Fine().nodes->GetCoord(iPoint);
        stencils[iPoint] = onMarker[iPoint] ? locator.LocateOnBoundary(x, target.markerTags) : locator.Locate(x);
        const auto& stencil = stencils[iPoint];
        CHECK_FALSE(stencil.beyondLimit);
        nOutside += !stencil.inside;
        nOff += stencil.distance > 1e-12;
        su2double sum = 0.0;
        for (unsigned short k = 0; k < stencil.nPoint; ++k) {
          CHECK(stencil.weight[k] >= 0.0);
          CHECK(stencil.weight[k] <= 1.0);
          sum += stencil.weight[k];
        }
        CHECK(fabs(sum - 1.0) < 1e-14);
        if (onMarker[iPoint]) CHECK(stencil.onFace);
      }
      CHECK(nOutside > 0);
      CHECK(nOff > 0);

      SECTION("constant field: exact") {
        donor.SetField(FLOW_SOL, [nDim](const su2double*, su2double* U) {
          const su2double x0[3] = {0.1, 0.2, 0.3};
          SmoothFlow(nDim, x0, U);
        });
        CBarycentricTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        CHECK(transfer.GetSummary().nOutside == nOutside);
        su2double U0[MAXVAR] = {};
        const su2double x0[3] = {0.1, 0.2, 0.3};
        SmoothFlow(nDim, x0, U0);
        passivedouble maxDiff = 0.0;
        for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
          for (unsigned short iVar = 0; iVar < nVar; ++iVar)
            maxDiff = std::max(maxDiff, RelDiff(target.solver[MESH_0][FLOW_SOL]->GetNodes()->GetSolution(iPoint, iVar), U0[iVar]));
        CHECK(maxDiff < 1e-14);
      }

      SECTION("smooth field: convex combination, error bounded by the distance") {
        donor.SetField(FLOW_SOL, [nDim](const su2double* x, su2double* U) { SmoothFlow(nDim, x, U); });
        CBarycentricTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        const auto& summary = transfer.GetSummary();
        CHECK(summary.nFlowFixed == 0);
        CHECK(summary.nOutside == nOutside);
        REQUIRE(summary.markers.size() == 3);
        CHECK(summary.markers.back().name == "(interior)");
        unsigned long nMarkerPoints = 0;
        for (unsigned short i = 0; i < 2; ++i) {
          CHECK(summary.markers[i].nOff > 0);
          CHECK(summary.markers[i].nBeyondFace == 0);
          CHECK(summary.markers[i].maxRelDistance < 0.25);
          nMarkerPoints += summary.markers[i].nPoint;
        }
        CHECK(nMarkerPoints >= target.Fine().GetnVertex(0));

        const auto* donorNodes = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
        const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
        for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
          const auto& stencil = stencils[iPoint];
          for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
            /*--- Value within the values of its donor points (no overshoot), and equal to the stencil value. ---*/
            su2double low = 1e300, high = -1e300, value = 0.0;
            for (unsigned short k = 0; k < stencil.nPoint; ++k) {
              const su2double donorValue = donorNodes->GetSolution(stencil.point[k], iVar);
              low = min(low, donorValue);
              high = max(high, donorValue);
              value += stencil.weight[k] * donorValue;
            }
            const su2double transferred = nodes->GetSolution(iPoint, iVar);
            const su2double tol = 1e-13 * max(fabs(low), fabs(high));
            CHECK(transferred >= low - tol);
            CHECK(transferred <= high + tol);
            CHECK(RelDiff(transferred, value) < 1e-14);
          }
        }
      }

      SECTION("affine field: error at most gradient times distance; continuity along the curved boundary") {
        donor.SetField(FLOW_SOL, AffineFlow(nDim));
        CBarycentricTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        /*--- Gradient norm of each variable of AffineFlow. ---*/
        const passivedouble grad2D[] = {sqrt(0.01 + 0.0025), sqrt(400.0 + 100.0), sqrt(25.0 + 64.0), sqrt(1e6 + 4e6)};
        const passivedouble grad3D[] = {sqrt(0.01 + 0.0025 + 0.0004), sqrt(400.0 + 100.0 + 25.0), sqrt(25.0 + 64.0 + 9.0),
                                        sqrt(16.0 + 4.0 + 36.0), sqrt(1e6 + 4e6 + 2.5e5)};
        const auto* grad = (nDim == 2) ? grad2D : grad3D;
        const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
        for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
          su2double exact[MAXVAR] = {};
          AffineFlow(nDim)(target.Fine().nodes->GetCoord(iPoint), exact);
          for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
            const su2double error = fabs(nodes->GetSolution(iPoint, iVar) - exact[iVar]);
            CHECK(error <= grad[iVar] * stencils[iPoint].distance * (1.0 + 1e-9) + 1e-12 * fabs(exact[iVar]));
          }
        }

        /*--- Dense samples of the circle / of a spiral on the sphere. 2D: the donor polygon is convex, the closest
         *    point is a contraction, so the value changes by at most |grad| times the distance between samples
         *    (Lipschitz, no jumps). 3D: the inscribed polyhedron has reflex edges (the fixed diagonals of the
         *    structured faces), where the closest point jumps between the two faces; the jump is bounded by |grad|
         *    times the distances of the two samples to the donor boundary. ---*/
        const int nSample = 4000;
        su2double previous[MAXVAR] = {}, xPrevious[3] = {}, previousDistance = 0.0;
        passivedouble worst = 0.0;
        for (int i = 0; i <= nSample; ++i) {
          const passivedouble t = static_cast<passivedouble>(i) / nSample;
          su2double x[3] = {};
          if (nDim == 2) {
            x[0] = cos(2.0 * PI_NUMBER * t);
            x[1] = sin(2.0 * PI_NUMBER * t);
          } else {
            const passivedouble polar = PI_NUMBER * (0.02 + 0.96 * t), azimuth = 12.0 * 2.0 * PI_NUMBER * t;
            x[0] = sin(polar) * cos(azimuth);
            x[1] = sin(polar) * sin(azimuth);
            x[2] = cos(polar);
          }
          const auto stencil = locator.LocateOnBoundary(x, donor.markerTags);
          CHECK_FALSE(stencil.beyondLimit);
          su2double value[MAXVAR] = {};
          for (unsigned short k = 0; k < stencil.nPoint; ++k) {
            const auto* xk = donor.Fine().nodes->GetCoord(stencil.point[k]);
            su2double Uk[MAXVAR] = {};
            AffineFlow(nDim)(xk, Uk);
            for (unsigned short iVar = 0; iVar < nVar; ++iVar) value[iVar] += stencil.weight[k] * Uk[iVar];
          }
          if (i > 0) {
            su2double step = 0.0;
            for (unsigned short iDim = 0; iDim < nDim; ++iDim) step += pow(x[iDim] - xPrevious[iDim], 2);
            step = sqrt(step);
            if (nDim == 3) step += previousDistance + stencil.distance;
            for (unsigned short iVar = 0; iVar < nVar; ++iVar)
              worst = std::max(worst, SU2_TYPE::GetValue(fabs(value[iVar] - previous[iVar]) / (grad[iVar] * step)));
          }
          for (unsigned short iVar = 0; iVar < nVar; ++iVar) previous[iVar] = value[iVar];
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) xPrevious[iDim] = x[iDim];
          previousDistance = stencil.distance;
        }
        CHECK(worst <= 1.0 + 1e-6);
      }

      SECTION("distance limit") {
        /*--- Limit: the face size, at least 2 ADAP_HAUSD (0.02) or 1e-3 x the domain size. ---*/
        const su2double domain = locator.GetDomainSize();
        CHECK(fabs(domain - 2.0 * sqrt(static_cast<passivedouble>(nDim))) < 1e-12);
        CHECK(locator.GetDistanceLimit(0.0) == Approx(0.02));
        CHECK(locator.GetDistanceLimit(0.3) == Approx(0.3));

        /*--- Outside along a direction from the centre: accepted while the distance is below the size of the nearest
         *    face (sagitta-type gaps), flagged far outside (here 0.5 x the domain size: another domain). ---*/
        const su2double dir[3] = {0.48, 0.6, nDim == 3 ? 0.64 : 0.0};
        su2double norm = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) norm += dir[iDim] * dir[iDim];
        for (const passivedouble radius : {1.0 + 1e-3, 1.1, 1.0 + domain * 0.5}) {
          su2double x[3] = {};
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = radius * dir[iDim] / sqrt(norm);
          const auto stencil = locator.Locate(x);
          CHECK_FALSE(stencil.inside);
          CHECK(stencil.beyondLimit == (stencil.distance > max(stencil.faceSize, su2double(0.02))));
          CHECK(stencil.beyondLimit == (radius > 2.0));
        }
      }
    }
  }
}

TEST_CASE("Barycentric transfer: wall points take the donor wall state", "[Adaptation]") {
  /*--- No-slip walls on a disk/ball (donor boundary inscribed in the circle/sphere); the new mesh is the donor shape
   *    scaled by 0.97, so its wall points lie inside the donor fluid, where the containing donor element would give
   *    them the velocity of the first layer. They take the closest point of the donor wall instead: zero momentum
   *    exactly, density and energy of the donor wall. ---*/
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeRoundConfig("SOLVER= NAVIER_STOKES\nREYNOLDS_NUMBER= 1e6\n", "MARKER_HEATFLUX= (round_a, 0.0, round_b, 0.0)\n");
      MeshSolution donor(config.get(), simplex_test::MakeRoundMesh(nDim, 8, 1.0), 0);
      MeshSolution target(config.get(), simplex_test::MakeRoundMesh(nDim, 6, 0.97), 0);

      /*--- Donor: smooth field, momentum zero on the wall (as the no-slip condition leaves it). ---*/
      donor.SetField(FLOW_SOL, [nDim](const su2double* x, su2double* U) { SmoothFlow(nDim, x, U); });
      auto* donorNodes = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
      for (unsigned short iMarker = 0; iMarker < donor.Fine().GetnMarker(); ++iMarker)
        for (auto iVertex = 0ul; iVertex < donor.Fine().GetnVertex(iMarker); ++iVertex)
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            donorNodes->SetSolution(donor.Fine().vertex[iMarker][iVertex]->GetNode(), iDim + 1, 0.0);

      CBarycentricTransfer transfer;
      {
        Mute mute;
        transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }
      const auto& summary = transfer.GetSummary();
      CHECK(summary.nOutside == 0);
      CHECK(summary.nFlowFixed == 0);

      CBarycentricLocator locator(donor.Fine());
      const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
      passivedouble maxElementMomentum = 0.0, maxMomentum = 0.0;
      unsigned long nWall = 0;
      for (unsigned short iMarker = 0; iMarker < target.Fine().GetnMarker(); ++iMarker)
        for (auto iVertex = 0ul; iVertex < target.Fine().GetnVertex(iMarker); ++iVertex) {
          const auto iPoint = target.Fine().vertex[iMarker][iVertex]->GetNode();
          nWall++;
          for (unsigned short iDim = 0; iDim < nDim; ++iDim)
            maxMomentum = std::max(maxMomentum, SU2_TYPE::GetValue(fabs(nodes->GetSolution(iPoint, iDim + 1))));

          /*--- The containing element (the old rule) gives a non-zero velocity there. ---*/
          const auto element = locator.Locate(target.Fine().nodes->GetCoord(iPoint));
          REQUIRE(element.inside);
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
            su2double momentum = 0.0;
            for (unsigned short k = 0; k < element.nPoint; ++k)
              momentum += element.weight[k] * donorNodes->GetSolution(element.point[k], iDim + 1);
            maxElementMomentum = std::max(maxElementMomentum, SU2_TYPE::GetValue(fabs(momentum)));
          }
        }
      CHECK(nWall > 0);
      CHECK(maxMomentum == 0.0);
      CHECK(maxElementMomentum > 1.0);
      for (unsigned short i = 0; i < 2; ++i) {
        CHECK(summary.markers[i].nOutside == 0);
        CHECK(summary.markers[i].nOff == summary.markers[i].nPoint);
        CHECK(summary.markers[i].maxDistance < 0.04);
      }
    }
  }
}
