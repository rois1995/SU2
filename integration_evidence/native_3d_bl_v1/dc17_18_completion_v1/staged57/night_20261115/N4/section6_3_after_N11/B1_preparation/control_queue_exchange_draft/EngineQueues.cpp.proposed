// Owns only empty optional slots; payload lifetimes remain at their original Adapt local points.
#include "EngineQueues.hpp"
#include "EngineContext.hpp"

EngineQueues::EngineQueues() = default;
EngineQueues::~EngineQueues() = default;

void EngineQueues::LengthDirection(EngineContext& context, bool collapse) {
  auto& selection = *context.census.selection;
  auto& inactive_blocked_edges = *this->inactive_blocked_edges;
  auto& inactive_blocked_links = *this->inactive_blocked_links;
  auto& coarsening = *context.control.coarsening;
  auto& construction_kind = *context.control.construction_kind;
  auto& length_blocks_dirty = *this->length_blocks_dirty;

  if (coarsening != collapse) {
    selection.blocked_edges.swap(inactive_blocked_edges);
    std::swap(selection.blocked_links, inactive_blocked_links);
    coarsening = selection.coarsening = collapse;
    construction_kind = collapse ? MetricChange::Coarsen : MetricChange::Refine;
  }
  if (length_blocks_dirty[collapse]) {
    selection.blocked_edges.clear();
    selection.blocked_links = 0;
    length_blocks_dirty[collapse] = false;
  }
}
