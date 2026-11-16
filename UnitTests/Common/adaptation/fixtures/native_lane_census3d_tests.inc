#include "../../../../Common/include/adaptation/CNativeLaneCensus3D.hpp"

TEST_CASE("Lane census classifies complete dependency and physical-face footprints", "[NativeLaneCensus3D]") {
  using namespace SU2Native3D;
  using SU2NativeLaneCensus3D::Classify;
  Dependencies dependencies;
  dependencies.cells = {{10, 1, 2}, {11, 1, 2}};
  Imported imported;
  imported.cells.resize(2);
  CHECK(Classify(dependencies, imported, 2) == 0);
  dependencies.cells.back().owner = 3;
  CHECK(Classify(dependencies, imported, 2) == 2);
  imported.cells.back().physical[3].present = true;
  CHECK(Classify(dependencies, imported, 2) == 3);
  dependencies.cells.back().owner = 2;
  CHECK(Classify(dependencies, imported, 2) == 1);
  CHECK(Classify(Dependencies{}, Imported{}, 2) == 0);
}

TEST_CASE("Lane census retains attribution across moved proposal inputs", "[NativeLaneCensus3D]") {
  using namespace SU2Native3D;
  using namespace SU2NativeLaneCensus3D;
  Dependencies dependencies;
  dependencies.cells = {{10, 1, 3}};
  Imported imported;
  imported.cells.resize(1);
  imported.cells.front().physical[2].present = true;
  Counters counters(true);
  Slot slot;
  size_t work = 7;
  {
    Trial trial(counters, true, slot, dependencies, imported, 2, work);
    dependencies.cells.clear();
    imported.cells.clear();
    work += 11;
  }
  counters.Approved(true, slot, true);
  counters.Commit(true, slot, true, true, false);
  const auto& row = counters.records[1][3];
  CHECK(row.trials == 1);
  CHECK(row.approved == 1);
  CHECK(row.commits == 1);
  CHECK(row.work == 11);
  CHECK(row.same_skin == 1);
  CHECK(row.no_parameter_edits == 1);
  CHECK(row.no_coverage == 0);
  CHECK(row.seconds >= 0);
  CHECK(counters.records[1][0].trials == 0);
  std::ostringstream output;
  output.precision(9);
  counters.Print(2, output);
  const auto text = output.str();
  CHECK(std::count(text.begin(), text.end(), '\n') == 8);
  CHECK(text.find("stage=quality class=shared_wall trials=1 approved=1 commits=1") != std::string::npos);
  CHECK(output.precision() == 9);
}

TEST_CASE("Disabled lane census leaves no observations or output", "[NativeLaneCensus3D]") {
  using namespace SU2NativeLaneCensus3D;
  Counters counters(false);
  Slot slot;
  size_t work = 0;
  {
    Trial trial(counters, false, slot, {}, {}, 0, work);
    ++work;
  }
  counters.Approved(false, slot, true);
  counters.Commit(false, slot, true, true, true);
  CHECK_FALSE(slot.reached);
  CHECK(counters.records[0][0].trials == 0);
  CHECK(counters.records[0][0].commits == 0);
  std::ostringstream output;
  counters.Print(0, output);
  CHECK(output.str().empty());
}
