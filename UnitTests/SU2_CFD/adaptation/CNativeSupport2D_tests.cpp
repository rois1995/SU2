/*!
 * \file CNativeSupport2D_tests.cpp
 * \brief Default backend and configuration-level native support controls.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher.hpp"
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
