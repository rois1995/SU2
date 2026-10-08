/*!
 * \file linelet_tests.cpp
 * \brief Check real LINELET preconditioning when some or all ranks have no lines.
 * \version 8.5.0 "Harrier"
 * SU2 is distributed under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../adaptation/TransferTestCase.hpp"
#include "../../../Common/include/linear_algebra/CSysMatrix.hpp"

TEST_CASE("LINELET handles empty and mixed line distributions", "[LinearAlgebra][Linelet]") {
  for (const bool someLines : {false, true}) {
    CAPTURE(someLines);
    auto config = transfer_test::MakeConfig(2, "SOLVER= EULER\nLINEAR_SOLVER_PREC= LINELET\n");
    transfer_test::MeshSolution domain(config.get(), transfer_test::BoxMesh(2, 2, false), 0);
    auto& geometry = domain.Fine();
    // Create lines only on rank zero, after the common solver/geometry construction.
    if (someLines && SU2_MPI::GetRank() == 0) config->SetMarker_All_KindBC(0, EULER_WALL);
    CSysMatrix<su2mixedfloat> matrix;
    matrix.Initialize(geometry.GetnPoint(), geometry.GetnPointDomain(), 1, 1, true, &geometry, config.get());
    matrix.SetValZero();
    for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) matrix.AddVal2Diag(point, 2.0);
    CSysVector<su2mixedfloat> rhs(geometry.GetnPoint(), geometry.GetnPointDomain(), 1, 2.0), answer(rhs);
    SU2_OMP_PARALLEL {
      for (auto repeat = 0; repeat < 3; ++repeat) {
        matrix.BuildLineletPreconditioner(&geometry, config.get());
        matrix.ComputeLineletPreconditioner(rhs, answer, &geometry, config.get());
      }
    }
    const auto& info = geometry.GetLineletInfo(config.get());
    CHECK(info.computed);
    if (!someLines || SU2_MPI::GetRank() != 0) CHECK(info.linelets.empty());
    else CHECK_FALSE(info.linelets.empty());
    for (auto point = 0ul; point < geometry.GetnPointDomain(); ++point) CHECK(answer(point, 0) == Approx(1.0));
  }
}

TEST_CASE("Empty point partitions participate in LINELET construction", "[LinearAlgebra][Linelet]") {
  auto config = transfer_test::MakeConfig(2, "SOLVER= EULER\n");
  transfer_test::MeshSolution domain(config.get(), transfer_test::BoxMesh(2, 2, false), 0);
  CGeometry empty;
  if (SU2_MPI::GetRank() == 0) config->SetMarker_All_KindBC(0, EULER_WALL);
  auto& geometry = SU2_MPI::GetRank() == 0 ? domain.Fine() : empty;
  for (auto repeat = 0; repeat < 3; ++repeat) {
    const auto& info = geometry.GetLineletInfo(config.get());
    CHECK(info.computed);
    if (SU2_MPI::GetRank() == 0) CHECK_FALSE(info.linelets.empty());
    else {
      CHECK(info.linelets.empty());
      CHECK(info.lineletIdx.empty());
    }
  }
}
