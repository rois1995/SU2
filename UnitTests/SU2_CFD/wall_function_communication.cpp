/* Wall-function owner/halo state checks; LGPL-2.1-or-later, SU2 Contributors. */
#include "catch.hpp"
#include "../UnitQuadTestCase.hpp"

TEST_CASE("Wall-function halos receive each owner's marker state", "[WallFunction]") {
  UnitQuadTestCase test;
  test.config_options =
      "SOLVER=RANS\nKIND_TURB_MODEL=SA\nINIT_OPTION=TD_CONDITIONS\nMESH_FORMAT=BOX\nMESH_BOX_SIZE=7,7,7\n"
      "MESH_BOX_LENGTH=1,1,1\nMESH_BOX_OFFSET=0,0,0\nMACH_NUMBER=0.2\n"
      "MARKER_HEATFLUX=(y_minus,0,y_plus,0,x_minus,0)\n"
      "MARKER_CUSTOM=(x_plus,z_minus,z_plus)\n"
      "MARKER_WALL_FUNCTIONS=(y_minus,STANDARD_WALL_FUNCTION,y_plus,STANDARD_WALL_FUNCTION,x_minus,STANDARD_WALL_FUNCTION)\n";
  test.InitConfig();
  test.InitGeometry(true);
  test.InitSolver();
  auto flow = test.solver[FLOW_SOL];
  auto geometry = test.geometry.get();
  auto config = test.config.get();
  unsigned short packet = 0;
  COMM_TYPE datatype{};
  flow->GetCommCountAndType(config, MPI_QUANTITIES::WALL_FUNCTION, packet, datatype);
  CHECK(packet == 10); // Three modelled walls; custom and communication markers occupy no slots.
  CHECK(packet < 1 + 3*config->GetnMarker_CfgFile());

  if (SU2_MPI::GetSize() > 1) CHECK(geometry->GetnPoint() > geometry->GetnPointDomain());
  for (int pass = 0; pass < 2; ++pass) {
    for (auto point = 0ul; point < geometry->GetnPoint(); ++point) {
      const auto id = geometry->nodes->GetGlobalIndex(point);
      const bool owner = geometry->nodes->GetDomain(point);
      const auto tau = pass == 0 ? su2double(id+1) : 0.0;
      flow->GetNodes()->SetTau_Wall(point, owner ? tau : -99.0);
      for (auto marker = 0u; marker < config->GetnMarker_All(); ++marker) {
        const auto tag = config->GetMarker_All_TagBound(marker);
        if (!config->GetViscous_Wall(marker)) continue;
        const auto vertex = geometry->nodes->GetVertex(point, marker);
        if (vertex < 0) continue;
        const auto key = su2double(1000*config->GetMarker_CfgFile_TagBound(tag)+id+1);
        const su2double state[3] = {owner && pass == 0 ? key : 0.0,
                                    owner && pass == 0 ? 2*key : 0.0,
                                    owner && pass == 0 ? 3*key : 0.0};
        flow->SetWallFunctionState(marker, vertex, state);
      }
    }
    flow->InitiateComms(geometry, config, MPI_QUANTITIES::WALL_FUNCTION);
    flow->CompleteComms(geometry, config, MPI_QUANTITIES::WALL_FUNCTION);
    for (auto point = 0ul; point < geometry->GetnPoint(); ++point) {
      const auto id = geometry->nodes->GetGlobalIndex(point);
      CHECK(flow->GetNodes()->GetTau_Wall(point) == (pass == 0 ? su2double(id+1) : 0.0));
      for (auto marker = 0u; marker < config->GetnMarker_All(); ++marker) {
        if (!config->GetViscous_Wall(marker)) continue;
        const auto vertex = geometry->nodes->GetVertex(point, marker);
        if (vertex < 0) continue;
        const auto tag = config->GetMarker_All_TagBound(marker);
        const auto key = pass == 0 ? su2double(1000*config->GetMarker_CfgFile_TagBound(tag)+id+1) : 0.0;
        CHECK(flow->GetYPlus(marker, vertex) == key);
        CHECK(flow->GetUTau(marker, vertex) == 2*key);
        CHECK(flow->GetEddyViscWall(marker, vertex) == 3*key);
      }
    }
  }
  delete test.solver[TURB_SOL];
  test.solver[TURB_SOL] = nullptr;
}
