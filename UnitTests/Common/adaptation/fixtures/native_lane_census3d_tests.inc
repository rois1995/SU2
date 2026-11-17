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
  CHECK(std::count(text.begin(), text.end(), '\n') == 9);
  CHECK(text.find("stage=quality class=shared_wall trials=1 approved=1 commits=1") != std::string::npos);
  CHECK(text.find("NATIVE3D_WALL_LANE_CENSUS rank=2 commits=0 class_s=0") != std::string::npos);
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

namespace {
SU2Native3D::BoundaryPoint LanePoint(SU2Native3D::Id id, SU2Native3D::Point point) {
  SU2Native3D::BoundaryPoint result;
  result.node = {id, point};
  return result;
}

SU2Native3D::Patch LanePatch(std::initializer_list<SU2Native3D::Face> faces) {
  SU2Native3D::Patch result;
  for (const auto& face : faces) result.surface.push_back({face, 7});
  return result;
}

void AddLanePoints(SU2Native3D::Patch& patch, std::initializer_list<SU2Native3D::Id> ids) {
  for (const auto id : ids) patch.parameters.emplace(id, LanePoint(id, {double(id), double(id % 2), 0}));
}
}  // namespace

TEST_CASE("N15 classifies wall commits and computes first proof from old face order", "[NativeLaneCensus3D]") {
  using namespace SU2Native3D;
  using namespace SU2NativeLaneCensus3D;
  const Face old_face{{1, 2, 3}};
  auto old_patch = LanePatch({old_face});
  AddLanePoints(old_patch, {1, 2, 3});
  auto same_patch = LanePatch({old_face});
  AddLanePoints(same_patch, {1, 2, 3});
  auto w1_patch = LanePatch({Face{{1, 2, 4}}});
  AddLanePoints(w1_patch, {1, 2, 4});
  w1_patch.parameters.at(1).node.p = {0, 0, 0};
  w1_patch.parameters.at(2).node.p = {1, 0, 0};
  w1_patch.parameters.at(4).node.p = {0.2, 0.2, 0};
  old_patch.parameters.at(1).node.p = {0, 0, 0};
  old_patch.parameters.at(2).node.p = {1, 0, 0};
  old_patch.parameters.at(3).node.p = {0, 1, 0};
  const std::vector<BoundaryParameterEdit> w1_edits{{LanePoint(3, {0, 1, 0}), 1, false},
                                                    {LanePoint(4, {0.2, 0.2, 0}), 1, true}};
  auto w2m_old = LanePatch({Face{{1, 2, 3}}, Face{{1, 3, 4}}});
  auto w2m_fresh = LanePatch({Face{{1, 5, 6}}, Face{{1, 6, 4}}});
  AddLanePoints(w2m_old, {1, 2, 3, 4});
  AddLanePoints(w2m_fresh, {1, 4, 5, 6});
  const std::vector<BoundaryParameterEdit> w2m_edits{{LanePoint(2, {}), 1, false},
                                                     {LanePoint(3, {}), 1, false},
                                                     {LanePoint(5, {}), 1, true},
                                                     {LanePoint(6, {}), 1, true}};

  Counters counters(true);
  Slot s, w1, w2m, w2o;
  s.reached = w1.reached = w2m.reached = w2o.reached = true;
  s.category = w1.category = w2m.category = w2o.category = 1;
  DescribeWall(s, old_patch, same_patch, {}, true, true, false, 0);
  DescribeWall(w1, old_patch, w1_patch, w1_edits, false, false, true, 1);
  DescribeWall(w2m, w2m_old, w2m_fresh, w2m_edits, false, false, true, 1);
  DescribeWall(w2o, old_patch, w1_patch, w2m_edits, false, false, false, 2);
  CHECK(s.wall.classification == WallClass::S);
  CHECK(w1.wall.classification == WallClass::W1);
  CHECK(w1.wall.first_proof);
  CHECK(w1.wall.link_local);
  auto proof_edits = w1_edits;
  proof_edits.back().point.node.p = {0.2, 0.2, 1};
  const bool above = FirstProof(old_patch, w1_patch, &proof_edits.back());
  proof_edits.back().point.node.p.z = -1;
  const bool below = FirstProof(old_patch, w1_patch, &proof_edits.back());
  CHECK(above != below);
  std::swap(old_patch.surface[0].v[0], old_patch.surface[0].v[1]);
  CHECK(FirstProof(old_patch, w1_patch, &proof_edits.back()) == above);
  std::swap(old_patch.surface[0].v[0], old_patch.surface[0].v[1]);
  CHECK(w2m.wall.classification == WallClass::W2M);
  CHECK(w2m.wall.erased == 2);
  CHECK(w2m.wall.inserted == 2);
  CHECK(w2o.wall.classification == WallClass::W2O);
  s.seconds = 0.25;
  w1.seconds = 0.5;
  w2m.seconds = 0.75;
  counters.CommitWall(true, s);
  counters.CommitWall(true, w1);
  counters.CommitWall(true, w2m);
  CHECK(counters.wall_record.commits == 3);
  CHECK(counters.wall_record.class_s + counters.wall_record.class_w1 + counters.wall_record.class_w2m +
            counters.wall_record.class_w2o ==
        3);
  CHECK(counters.wall_record.w1_kind1 == 1);
  CHECK(counters.wall_record.w1_link_local_first_proof == 1);
  CHECK(counters.wall_record.seconds_s == 0.25);
  CHECK(counters.wall_record.seconds_w1 == 0.5);
  CHECK(counters.wall_record.seconds_w2m == 0.75);
  CHECK(counters.wall_record.seconds_w1_link_local == 0.5);
  CHECK(counters.wall_record.seconds_w1_link_local_first_proof == 0.5);
  CHECK(counters.wall_record.seconds_w2m_link_local == 0.75);
  std::ostringstream output;
  counters.Print(4, output);
  CHECK(output.str().find("NATIVE3D_WALL_LANE_CENSUS rank=4 commits=3 class_s=1 class_w1=1 class_w2m=1 class_w2o=0") !=
        std::string::npos);
}

TEST_CASE("N15 seconds reuse the lane trial interval and diagnostics stay disabled", "[NativeLaneCensus3D]") {
  using namespace SU2Native3D;
  using namespace SU2NativeLaneCensus3D;
  Dependencies dependencies;
  dependencies.cells = {{10, 1, 2}};
  Imported imported;
  imported.cells.resize(1);
  imported.cells[0].physical[0].present = true;
  Counters counters(true);
  Slot slot;
  size_t work = 0;
  {
    Trial trial(counters, true, slot, dependencies, imported, 2, work);
    ++work;
  }
  CHECK(counters.records[1][1].seconds == slot.seconds);
  auto old_patch = LanePatch({Face{{1, 2, 3}}});
  AddLanePoints(old_patch, {1, 2, 3});
  DescribeWall(slot, old_patch, old_patch, {}, true, true, false, 0);
  counters.CommitWall(true, slot);
  CHECK(counters.wall_record.commits == 1);

  Counters disabled(false);
  Slot empty;
  {
    Trial trial(disabled, true, empty, dependencies, imported, 2, work);
    ++work;
  }
  DescribeWall(empty, old_patch, old_patch, {}, true, true, false, 0);
  disabled.CommitWall(true, empty);
  std::ostringstream output;
  disabled.Print(2, output);
  CHECK(output.str().empty());
  CHECK_FALSE(empty.reached);
  CHECK(disabled.wall_record.commits == 0);
}
