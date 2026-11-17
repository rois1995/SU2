// Borrows the single canonical inputs; does not allocate, vote, or start a state value.
#include "EngineContext.hpp"

EngineContext::EngineContext(EngineSetup& setup, World& world, const Owned& accepted,
                             const std::vector<DonorCell>& original, const PlanarReference& geometry,
                             MetricComposition& composition, EngineOptions& options, MetricChange& intent,
                             bool& bidirectional, BoundaryMetric& surface_target)
    : setup(setup),
      world(world),
      accepted(accepted),
      original(original),
      geometry(geometry),
      composition(composition),
      options(options),
      intent(intent),
      bidirectional(bidirectional),
      surface_target(surface_target) {}
EngineContext::~EngineContext() = default;
