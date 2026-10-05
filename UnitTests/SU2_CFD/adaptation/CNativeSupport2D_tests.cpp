/*!
 * \file CNativeSupport2D_tests.cpp
 * \brief Default backend and configuration-level native support controls.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include <fstream>
#include "../../../Common/include/adaptation/CNativeRemesher.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
using namespace transfer_test;

TEST_CASE("Native support: default remains MMG and primal triangles are admitted", "[NativeSupport2D]") {
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  CHECK(config->GetKind_Adap_Remesher() == ADAP_REMESHER::MMG);
  MeshSolution mesh(config.get(), BoxMesh(2, 2, true), 0);
  CNativeRemesher::CheckSupport(*config, mesh.Fine());
}

// Run only in a separate MPI process: the expected collective support diagnostic
// terminates the executable. Geometry comes from a supported primal solve; the
// derivative request must be rejected even though this executable uses doubles.
TEST_CASE("Native support failure: continuous adjoint request in a primal build", "[NativeUnsupportedDerivative][.]") {
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution mesh(config.get(), BoxMesh(2, 2, true), 0);
  auto adjoint = MakeConfig(2, "SOLVER= EULER\nMATH_PROBLEM= CONTINUOUS_ADJOINT\n");
  REQUIRE(adjoint->GetContinuous_Adjoint());
  CNativeRemesher::CheckSupport(*adjoint, mesh.Fine());
  FAIL("Unsupported derivative request passed native support check.");
}

// No sidecar exists in the fresh runner directory. Native restart must fail
// explicitly rather than treating an adapted mesh as a new original reference.
TEST_CASE("Native restart failure: missing original reference cannot be silently rebased", "[NativeMissingRestartReference][.]") {
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution mesh(config.get(), BoxMesh(2, 2, true), 0);
  auto restart = MakeConfig(2, "SOLVER= EULER\nRESTART_SOL= YES\n");
  REQUIRE(restart->GetRestart());
  const auto sidecar = restart->GetMesh_FileName() + ".native_ref";
  REQUIRE_FALSE(std::ifstream(sidecar).good());
  auto state = std::make_shared<SU2NativeBoundary2D::ReferenceState>();
  CNativeRemesher remesher(state);
  remesher.PrepareReference(*restart, mesh.Fine());
  FAIL("Native restart silently constructed a new original reference.");
}
