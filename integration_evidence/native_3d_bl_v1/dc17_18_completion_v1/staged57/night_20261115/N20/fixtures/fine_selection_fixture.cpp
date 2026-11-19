// Exercises the real translation-unit-private selection without exporting a test API.
#include "Common/src/adaptation/CNativeEngine3D.cpp"
#include <cassert>

int main() {
  using namespace SU2Native3D;
  using Fine = Selection::FineRepairState;

  const PlanarReference geometry(std::vector<Facet>{});
  EngineStats stats;
  const WallHeights heights;
  Selection selection(false, true, geometry, stats, false, 0, heights, true);
  const Cell cell{1, {{{11, {0, 0, 0}}, {12, {1, 0, 0}}, {13, {0, 1, 0}}, {14, {0, 0, 1}}}}};
  Selection::CellState row;
  row.vertices = {11, 12, 13, 14};
  size_t edge = 0;
  for (size_t first = 0; first < row.vertices.size(); ++first)
    for (size_t second = first + 1; second < row.vertices.size(); ++second)
      row.edges[edge++] = {row.vertices[first], row.vertices[second]};
  selection.census.cells.emplace(cell.id, row);

  const auto good_reopened = selection.RequestFineMoves();
  assert(good_reopened == 0);
  selection.census.cells.at(cell.id).debt.q_bad = true;
  selection.AddRepairs(cell);
  auto& demands = selection.queues.repair_users;
  for (const auto& [feature, demand] : demands) selection.Service({feature.second, feature.first}, false);

  const auto reopened = selection.RequestFineMoves();
  assert(reopened == 4);
  size_t done = 0;
  for (const auto& [feature, demand] : demands) {
    if (feature.first != 1) {
      assert(demand.blocked && demand.fine == Fine::None);
      continue;
    }
    assert(!demand.blocked && demand.fine == Fine::Requested);
    selection.Service({feature.second, feature.first}, false);
    assert(demand.blocked && demand.fine == Fine::Done);
    ++done;
  }
  const auto repeated = selection.RequestFineMoves();
  assert(repeated == 0);
  selection.TouchRepairSupport(row.vertices);
  size_t reset = 0;
  for (const auto& [feature, demand] : demands) {
    assert(demand.fine == Fine::None && !demand.blocked);
    if (feature.first == 1) ++reset;
  }
  std::cout << "NATIVE3D_FINE_SELECTION good_reopened=" << good_reopened << " reopened=" << reopened << " done=" << done
            << " repeated=" << repeated << " reset=" << reset << '\n';
}
