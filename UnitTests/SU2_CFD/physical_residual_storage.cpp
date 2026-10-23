#include "catch.hpp"
#include "../UnitQuadTestCase.hpp"

namespace {
struct ResidualProbe : CEulerSolver {
  using CEulerSolver::CEulerSolver;
  size_t CacheRows() const { return PhysicalResidual.rows(); }
};
}

TEST_CASE("Physical residual storage follows enabled outputs and owns no halos", "[Output][Residual]") {
  for (const auto* files : {"NONE", "RESTART"}) for (const auto* fields : {"SOLUTION", "RESIDUAL", "RES_DENSITY"}) {
    UnitQuadTestCase test;
    test.config_options = "SOLVER=EULER\nINIT_OPTION=TD_CONDITIONS\nMESH_FORMAT=BOX\n"
        "MESH_BOX_SIZE=5,5,5\nMESH_BOX_LENGTH=1,1,1\nMESH_BOX_OFFSET=0,0,0\nMACH_NUMBER=0.2\n"
        "MARKER_EULER=(y_minus,y_plus)\nMARKER_CUSTOM=(x_minus,x_plus,z_minus,z_plus)\n";
    test.AddOption(std::string("OUTPUT_FILES= ")+files);
    test.AddOption(std::string("VOLUME_OUTPUT= ")+fields);
    test.InitConfig();
    test.InitGeometry(true);
    ResidualProbe solver(test.geometry.get(), test.config.get(), MESH_0);
    const bool enabled = std::string(files) != "NONE" && std::string(fields) != "SOLUTION";
    CHECK(solver.CacheRows() == (enabled ? test.geometry->GetnPointDomain() : 0));
    for (auto point=0ul; point<test.geometry->GetnPointDomain(); ++point)
      for (auto var=0u; var<5; ++var) solver.LinSysRes(point,var) = point+var+1;
    solver.SavePhysicalResidual();
    for (auto point=0ul; point<test.geometry->GetnPointDomain(); ++point)
      for (auto var=0u; var<5; ++var)
        CHECK(solver.GetPhysicalResidual(point,var) == (enabled ? su2double(point+var+1) : su2double(0)));
    for (auto point=test.geometry->GetnPointDomain(); point<test.geometry->GetnPoint(); ++point)
      CHECK(solver.GetPhysicalResidual(point,0) == 0);
  }
}
