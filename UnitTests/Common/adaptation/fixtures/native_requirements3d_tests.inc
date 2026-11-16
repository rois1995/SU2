#include "../../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include <sstream>
TEST_CASE("Native3D requirements borrow existing configured values", "[NativeRequirements3D]") {
  using namespace SU2Native3D;
  const std::array<Node, 3> nodes{{{1, {0, 0, 0}}, {2, {1, 0, 0}}, {3, {0, 1, 0}}}};
  const std::vector<Facet> facets{{1, 7, {nodes[0], nodes[1], nodes[2]}}};
  const PlanarReference geometry(facets);
  EngineOptions unavailable;
  const auto defaults = MakeAdaptationRequirements(geometry, false, unavailable);
  CHECK(std::isnan(defaults.size.controls.hmin));
  CHECK(std::isnan(defaults.size.controls.complexity));
  CHECK(defaults.walls.first_heights->empty());
  EngineOptions options;
  options.requirement_size = {0.01, 0.5, 12.0, 1.3, 4321.0};
  options.surface_tolerance = 1e-4;
  options.wall_heights.emplace(7, 0.02);
  options.wall_heights.tolerance = 0.03;
  options.region_walls.emplace(7, RegionWallPolicy{0.02, 1.2, 0.1});
  options.maximum_work = 123;
  options.maximum_seconds = 4.5;
  options.memory_ceiling = 678;
  options.job_memory_limit = 901;
  const auto record = MakeAdaptationRequirements(geometry, true, options);
  CHECK(record.size.composition_present);
  CHECK(record.size.controls.hmin == 0.01);
  CHECK(record.size.controls.complexity == 4321.0);
  CHECK_FALSE(record.geometry.mapped);
  CHECK_FALSE(record.geometry.entity_geometry);
  CHECK_FALSE(record.geometry.curved);
  CHECK(record.geometry.tolerance == 1e-4);
  CHECK(record.walls.first_heights == &options.wall_heights);
  CHECK(record.walls.region_policies == &options.region_walls);
  CHECK(record.walls.height_tolerance == 0.03);
  CHECK(record.budgets.maximum_work == 123);
  CHECK(record.budgets.maximum_seconds == 4.5);
  CHECK(record.budgets.memory_ceiling == 678);
  CHECK(record.budgets.job_memory_limit == 901);
  std::ostringstream output;
  output.precision(8);
  PrintAdaptationRequirements(record, 2, output);
  CHECK(output.precision() == 8);
  CHECK(output.str().find("NATIVE3D_REQUIREMENTS") == 0);
  CHECK(output.str().find("complexity=4321") != std::string::npos);
  CHECK(output.str().find("7:0.02,1.2,0.1") != std::string::npos);
}
