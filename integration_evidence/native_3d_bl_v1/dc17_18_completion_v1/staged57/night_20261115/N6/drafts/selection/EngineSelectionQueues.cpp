// Maintains the ordered construction, geometry and repair frontiers.

// Uses the predicted improvement when available, otherwise keeps the raw ordering rule.
long double Selection::GeometryPriority(long double raw, Edge edge) const {
  if (!queues.geometry_priority_mode) return raw;
  const auto found = queues.geometry_split_priorities.find(edge);
  return found != queues.geometry_split_priorities.end() && found->second > 1 ? found->second : raw / (1 + raw);
}

// Accounts for one priority-map entry per eligible geometry edge.
size_t Selection::GeometryPriorityBytes() const {
  return transfer_memory::Mul(queues.geometry_queue.size(),
                              sizeof(std::pair<const Edge, long double>) + 4 * sizeof(void*));
}

// Refreshes geometry priorities in queue order within the existing time limit.
void Selection::PrioritizeGeometrySplits(Clock::time_point started, double maximum_seconds) {
  queues.geometry_split_priorities.clear();
  queues.geometry_priority_mode = true;
  auto pending = std::move(queues.geometry_queue);
  queues.geometry_queue.clear();
  while (!pending.empty()) {
    auto entry = pending.extract(pending.begin());
    const auto edge = std::get<2>(entry.value());
    if (!queues.geometry_split_priorities.count(edge) &&
        (!maximum_seconds || std::chrono::duration<double>(Clock::now() - started).count() < maximum_seconds)) {
      const auto& measured = census.edges.at(edge).measure;
      Timer timer(stats.geometry_guidance_seconds, timings, SU2NativeTiming3D::Operation::GeometryBound);
      ++stats.geometry_guidance_trials;
      const auto prediction = PredictGeometrySplit(edge, {measured.a, measured.b}, geometry, tolerance);
      stats.geometry_guidance_faces += prediction.evaluated_faces;
      const bool better =
          prediction.complete && prediction.before > 0 && prediction.after < prediction.before * (1 - 1e-12L);
      queues.geometry_split_priorities.emplace(
          edge, better ? 1 + (prediction.before - prediction.after) / prediction.before : -1.L);
    }
    const auto id = std::get<1>(entry.value());
    const auto& row = census.cells.at(id);
    const auto found = std::find(row.edges.begin(), row.edges.end(), edge);
    if (found == row.edges.end()) throw std::logic_error("Geometry priority lost its versioned edge.");
    std::get<0>(entry.value()) = -GeometryPriority(row.geometry_scores[found - row.edges.begin()], edge);
    queues.geometry_queue.insert(std::move(entry));
  }
}

// Records the latest disposition for an edge already tracked in the generation.
void Selection::Note(Edge edge, unsigned disposition) {
  std::sort(edge.begin(), edge.end());
  const auto found = queues.edge_history.find(edge);
  if (found != queues.edge_history.end()) {
    found->second.round = stats.rounds;
    found->second.disposition = disposition;
  }
}

// Retires resolved lengths and ages each remaining demand once per generation.
void Selection::RefreshHistory(bool generation) {
  for (auto it = queues.edge_history.begin(); it != queues.edge_history.end();) {
    const auto edge = census.edges.find(it->first);
    if (edge == census.edges.end() || edge->second.measure.length <= MaximumMetricEdge)
      it = queues.edge_history.erase(it);
    else
      ++it;
  }
  for (const auto& edge : census.edges)
    if (edge.second.measure.length > MaximumMetricEdge) {
      auto& history = queues.edge_history[edge.first];
      if (generation) ++history.age;
    }
}

// Orders the length queue by short or long edges for the current direction.
long double Selection::Priority(long double length) const { return coarsening ? length : -length; }

// Applies the current length gate to one cached edge measurement.
bool Selection::Candidate(long double length) const {
  return coarsening ? length < CoarseningMetricEdge : length > MaximumMetricEdge;
}

// Refreshes both directed queue entries without reopening a rejected edge.
void Selection::Queue(Edge edge, long double length, bool insert) {
  if (repair_event || !Candidate(length)) return;
  for (unsigned i = 0; i < (coarsening ? 2u : 1u); ++i) {
    Edge key = edge;
    std::sort(key.begin(), key.end());
    const auto value = std::make_pair(Priority(length), edge);
    auto& destination =
        queues.split_lanes && !coarsening && census.edges.at(key).physical_users ? queues.surface_queue : queues.queue;
    const auto erased = queues.queue.erase(value) + queues.surface_queue.erase(value);
    if (surface_planning && insert && !census.edges.at(key).physical_users) {
      std::swap(edge[0], edge[1]);
      continue;
    }
    const auto blocked = queues.blocked_edges.find(edge[1]);
    if (insert && blocked != queues.blocked_edges.end() && blocked->second.count(edge[0])) {
      // Measuring another unchanged cell cannot reopen a rejected direction; Touch owns reactivation.
      queues.queue.erase({Priority(length), edge});
      ++stats.blocked_queue_skips;
    } else if (insert && !queues.construction_frontier.count(edge))
      queues.queue.erase({Priority(length), edge});
    else if (insert && prune_collapses && census.edges.at(key).forbidden[i])
      stats.collapse_filtered += erased;
    else if (insert)
      destination.insert(value);
    std::swap(edge[0], edge[1]);
  }
}

// Ranks local required debt before the complete distributed trial certifies it.
long double Selection::RepairWeight(const CellState& row) const {
  // Cached local required debt steers search; only the complete distributed transaction certifies the feature.
  const auto& d = row.debt;
  long double weight = 0;
  if (surface_planning) return d.size + d.geometry + (row.geometry_invalid || d.size_bad ? 1.L : 0.L);
  if (d.q_measured)
    weight += d.q;
  else if (!row.quality_valid)
    weight += 1;
  if (d.j_measured)
    weight += d.j;
  else if (!row.quality_valid)
    weight += 1;
  if (!heights.empty()) weight += d.height + (d.height_unresolved || d.height_bad ? 1.L : 0.L);
  if (size_repairs) weight += d.size + (d.size_bad ? 1.L : 0.L);
  if (geometry_repairs) weight += d.geometry + (!d.geometry_measured || row.geometry_invalid ? 1.L : 0.L);
  if (!std::isfinite(weight) || weight < 0) throw std::logic_error("Nonfinite native repair severity.");
  return weight > 0 ? weight : 1.L;
}

// Adds one cell contribution and reinserts the live feature priority.
void Selection::AddRepairSeverity(const RepairFeature& key, long double value) {
  if (!std::isfinite(value) || value < 0) throw std::logic_error("Invalid native repair severity contribution.");
  auto& total = queues.repair_severity[key];
  if (queues.repair_users.at(key).users) queues.repair_severity_queue.erase({-total, key});
  total += value;
  if (!std::isfinite(total)) throw std::logic_error("Nonfinite native repair severity sum.");
  const auto& demand = queues.repair_users.at(key);
  if (demand.users && !demand.blocked) queues.repair_severity_queue.insert({-total, key});
}

// Releases one cell contribution after removing its old priority key.
void Selection::RemoveRepairSeverity(const RepairFeature& key, long double value) {
  const auto found = queues.repair_severity.find(key);
  if (found == queues.repair_severity.end()) throw std::logic_error("Missing native repair severity.");
  auto& total = found->second;
  queues.repair_severity_queue.erase({-total, key});
  total = std::max(0.L, total - value);
}

// Reweights the repair features of an already cached cell.
void Selection::UpdateRepairWeight(Id id, const CellState& row) {
  const auto found = queues.cell_repairs.find(id);
  if (found == queues.cell_repairs.end()) return;
  const long double prior = queues.repair_cell_severity.at(id), fresh = RepairWeight(row);
  if (prior == fresh) return;
  for (const auto& key : found->second) RemoveRepairSeverity(key, prior);
  for (const auto& key : found->second) AddRepairSeverity(key, fresh);
  queues.repair_cell_severity[id] = fresh;
}

// Selects the current first repair, geometry or length candidate.
Choice Selection::Best(bool repairing) const {
  if (repairing) {
    if (queues.repair_queue.empty()) return {};
    const auto& oldest = *queues.repair_queue.begin();
    const auto& key = oldest.second;
    Choice choice;
    choice.active = true;
    choice.feature = {key.second, key.first};
    choice.repair_epoch = oldest.first;
    choice.fair_repair = true;
    return choice;
  }
  if ((!queues.split_lanes || coarsening || queues.lane == Lane::Surface) && !queues.geometry_queue.empty()) {
    const auto key = std::get<2>(*queues.geometry_queue.begin());
    return {key,
            census.edges.at(key).measure,
            true,
            {},
            true,
            std::get<0>(*queues.geometry_queue.begin()),
            std::get<1>(*queues.geometry_queue.begin())};
  }
  const auto& active =
      queues.split_lanes && !coarsening && queues.lane == Lane::Surface ? queues.surface_queue : queues.queue;
  if (active.empty()) return {};
  const auto key = active.begin()->second;
  Edge canonical = key;
  std::sort(canonical.begin(), canonical.end());
  return {
      key, census.edges.at(canonical).measure, true, {}, false, Priority(census.edges.at(canonical).measure.length)};
}

// Selects the oldest repair, then severe candidates with disjoint local stars.
std::vector<Choice> Selection::RepairBatch(const Owned& working, size_t limit, bool& oversized) const {
  oversized = false;
  std::vector<Choice> result;
  if (!limit || queues.repair_queue.empty()) return result;
  auto make_choice = [&](const RepairFeature& key, uint64_t epoch, bool fair) {
    Choice choice;
    choice.active = true;
    choice.feature = {key.second, key.first};
    choice.repair_epoch = epoch;
    choice.fair_repair = fair;
    if (!fair) choice.priority_score = -queues.repair_severity.at(key);
    return choice;
  };
  const auto oldest = *queues.repair_queue.begin();
  const auto first = make_choice(oldest.second, oldest.first, true);
  std::vector<Id> first_cells;
  std::set<Id> used;
  bool first_oversized = false;
  CollectRepairStar(working, oldest.second, first_cells, used, first_oversized);
  if (first_oversized) {
    oversized = true;
    result.push_back(first);
    return result;
  }
  result.push_back(first);
  std::set<RepairFeature> selected{oldest.second};
  size_t scanned = 0;
  for (const auto& priority : queues.repair_severity_queue) {
    if (result.size() >= limit || scanned++ >= MaximumDirectoryScan) break;
    const auto& key = priority.second;
    if (selected.count(key)) continue;
    const auto demand = queues.repair_users.find(key);
    if (demand == queues.repair_users.end() || !demand->second.users || demand->second.blocked) continue;
    Choice choice = make_choice(key, demand->second.epoch, false);
    std::vector<Id> star_cells;
    std::set<Id> star_vertices;
    bool star_oversized = false;
    CollectRepairStar(working, key, star_cells, star_vertices, star_oversized);
    if (star_oversized) continue;
    bool disjoint = true;
    for (Id vertex : star_vertices) disjoint = disjoint && !used.count(vertex);
    if (!disjoint) continue;
    result.push_back(choice);
    selected.insert(key);
    used.insert(star_vertices.begin(), star_vertices.end());
  }
  return result;
}

// Selects local-disjoint length candidates without replacing complete-star reservation.
std::vector<Choice> Selection::Batch(const Owned& working, size_t limit) {
  std::vector<Choice> result;
  std::set<Id> used;
  size_t scanned = 0;
  const auto& active =
      queues.split_lanes && !coarsening && queues.lane == Lane::Surface ? queues.surface_queue : queues.queue;
  for (auto it = active.begin(); it != active.end() && result.size() < limit && scanned++ < MaximumDirectoryScan;) {
    const auto edge = (it++)->second;
    Edge canonical = edge;
    std::sort(canonical.begin(), canonical.end());
    const Choice choice{edge, census.edges.at(canonical).measure, true, {}};
    if (FilterCollapse(choice, working)) continue;
    const auto& star = census.vertex_cells.at(coarsening ? edge[1] : edge[0]);
    std::set<Id> vertices;
    bool disjoint = true, oversized = false;
    size_t star_cell_count = 0, scanned_star = 0;
    for (Id id : star) {
      if (++scanned_star > MaximumDirectoryScan) {
        oversized = true;
        break;
      }
      const auto& cell = working.at(id).cell;
      if (!coarsening && std::none_of(cell.v.begin(), cell.v.end(), [&](Node node) { return node.id == edge[1]; }))
        continue;
      if (++star_cell_count > MaximumCavityCells) {
        oversized = true;
        break;
      }
      for (auto node : cell.v) {
        vertices.insert(node.id);
        disjoint = disjoint && !used.count(node.id);
      }
    }
    if (oversized) {
      result.push_back(choice);
      break;
    }  // Complete-star admission reports the cap.
    // Local disjointness only reduces redundant proposals; reservation admits the complete remote stars.
    if (disjoint) {
      result.push_back(choice);
      used.insert(vertices.begin(), vertices.end());
    }
  }
  return result;
}

// Retires a rejected feature or records its directed edge for later reactivation.
void Selection::Block(Edge edge, Feature feature, bool repairing) {
  if (repairing) {
    Service(feature, false);
    return;
  }
  Note(edge, 4);
  ServiceEdge(edge);
  Edge canonical = edge;
  std::sort(canonical.begin(), canonical.end());
  const auto it = census.edges.find(canonical);
  if (it != census.edges.end()) {
    queues.queue.erase({Priority(it->second.measure.length), edge});
    queues.surface_queue.erase({Priority(it->second.measure.length), edge});
  }
  // Rejection can reach an owner before progressive measurement discovers its edge.
  if (coarsening || track_splits) {
    const bool inserted = queues.blocked_edges[edge[1]].insert(edge[0]).second;
    queues.blocked_links += inserted;
    stats.blocked_unmeasured += inserted && it == census.edges.end();
  }
  if (track_splits && !coarsening) queues.blocked_links += queues.blocked_edges[edge[0]].insert(edge[1]).second;
}

// Removes one retired edge direction and its blocked-link accounting.
void Selection::Forget(Edge edge) {
  const auto found = queues.blocked_edges.find(edge[1]);
  if (found == queues.blocked_edges.end()) return;
  queues.blocked_links -= found->second.erase(edge[0]);
  if (found->second.empty()) queues.blocked_edges.erase(found);
}

// Reopens a rejected feature when its complete support may have changed.
void Selection::TouchRepairSupport(const std::array<Id, 4>& vertices) {
  // Only a tetrahedron containing the whole feature changes its complete imported star.
  for (unsigned mask = 1; mask < 15; ++mask) {
    RepairFeature key{};
    for (unsigned i = 0; i < 4; ++i)
      if (mask & (1u << i)) key.second[key.first++] = vertices[i];
    const auto found = queues.repair_users.find(key);
    if (found == queues.repair_users.end() || !found->second.users || !found->second.blocked) continue;
    auto& demand = found->second;
    demand.blocked = false;
    demand.epoch = queues.repair_epoch;
    queues.repair_queue.insert({demand.epoch, key});
    queues.repair_severity_queue.insert({-queues.repair_severity.at(key), key});
  }
}

// Reactivates blocked edges incident to a changed vertex when still eligible.
void Selection::Touch(Id vertex) {
  const auto found = queues.blocked_edges.find(vertex);
  if (found == queues.blocked_edges.end()) return;
  auto reactivated = std::move(found->second);
  queues.blocked_links -= reactivated.size();
  queues.blocked_edges.erase(found);
  for (Id retained : reactivated) {
    Edge edge{retained, vertex}, key = edge;
    std::sort(key.begin(), key.end());
    const auto measured = census.edges.find(key);
    if (measured == census.edges.end()) continue;
    if (!coarsening && !geometry.ExtensionComponents().empty()) {
      const auto star = census.vertex_cells.find(vertex);
      if (star != census.vertex_cells.end())
        for (Id id : star->second) {
          const auto& row = census.cells.at(id);
          for (size_t k = 0; k < row.edges.size(); ++k)
            if (row.edges[k] == key && row.geometry_scores[k] > 0)
              if (queues.construction_frontier.count(key))
                stats.geometry_reactivated +=
                    queues.geometry_queue.insert({-GeometryPriority(row.geometry_scores[k], key), id, key}).second;
        }
    }
    if (!Candidate(measured->second.measure.length)) continue;
    const auto direction = retained == key[0] ? 0u : 1u;
    if (prune_collapses && measured->second.forbidden[direction]) continue;
    if (!coarsening) edge = key;
    if (!queues.construction_frontier.count(edge)) continue;
    const auto count = queues.queue.size() + queues.surface_queue.size();
    Queue(key, measured->second.measure.length, true);
    const bool inserted = queues.queue.size() + queues.surface_queue.size() > count;
    if (coarsening) stats.collapse_reactivated += inserted;
  }
}

// Applies existing local geometry and orientation screens before a collapse trial.
bool Selection::FilterCollapse(const Choice& choice, const Owned& working) {
  if (!coarsening || !prune_collapses || !choice.active) return false;
  if (!geometry.ContractionAllowed(choice.edge[1], choice.edge[0])) {
    Block(choice.edge, {}, false);
    ++stats.collapse_filtered;
    ++stats.collapse_constraint_filtered;
    return true;
  }
  const auto star = census.vertex_cells.find(choice.edge[1]);
  if (star == census.vertex_cells.end()) throw std::logic_error("Missing native 3D local collapse star.");
  const Point apex = choice.edge[0] < choice.edge[1] ? choice.measure.a : choice.measure.b;
  {
    Timer timer(stats.collapse_geometry_seconds, timings, SU2NativeTiming3D::Operation::GeometryBound);
    size_t scanned = 0;
    for (Id id : star->second) {
      if (++scanned > MaximumDirectoryScan) break;
      const auto& row = working.at(id);
      if (std::any_of(row.cell.v.begin(), row.cell.v.end(), [&](Node n) { return n.id == choice.edge[0]; })) continue;
      if (LeavesFacet(row, choice.edge[1], apex)) {
        Block(choice.edge, {}, false);
        ++stats.collapse_filtered;
        return true;
      }
      if (CollapseConstraintViolation(row, choice.edge[1], {choice.edge[0], apex}, geometry, tolerance, heights)) {
        Block(choice.edge, {}, false);
        ++stats.collapse_filtered;
        ++stats.collapse_constraint_filtered;
        return true;
      }
    }
  }
  Timer timer(stats.collapse_orientation_seconds, timings, SU2NativeTiming3D::Operation::Topology);
  size_t scanned = 0;
  for (Id id : star->second) {
    if (++scanned > MaximumDirectoryScan) break;
    auto v = working.at(id).cell.v;
    if (std::any_of(v.begin(), v.end(), [&](Node n) { return n.id == choice.edge[0]; })) continue;
    for (auto& node : v)
      if (node.id == choice.edge[1]) node = {choice.edge[0], apex};
    ++stats.collapse_orientation_checks;
    if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0)) {
      // Surface planning may repair connectivity before contracting; all final validity checks remain.
      if (surface_planning && !geometry.ExtensionComponents().empty()) return false;
      Block(choice.edge, {}, false);
      ++stats.collapse_orientation_filtered;
      return true;
    }
  }
  return false;
}

// Drops one cell's demands while retaining zero-user history through publication.
void Selection::RemoveRepairs(Id id) {
  const auto it = queues.cell_repairs.find(id);
  if (it == queues.cell_repairs.end()) return;
  queues.repair_bytes -= transfer_memory::Bytes(it->second);
  // Preserve waiting age and rejection state through transient zero-user counts until the publication completes.
  const long double weight = queues.repair_cell_severity.at(id);
  for (const auto& key : it->second) {
    RemoveRepairSeverity(key, weight);
    auto& demand = queues.repair_users.at(key);
    if (--demand.users == 0)
      queues.repair_zero.insert(key);
    else if (!demand.blocked)
      queues.repair_severity_queue.insert({-queues.repair_severity.at(key), key});
  }
  queues.repair_cell_severity.erase(id);
  queues.cell_repairs.erase(it);
}

// Brings one cached cell's demand into line with the active repair requirements.
void Selection::UpdateRepairs(const Cell& cell, const CellState& row) {
  if (!track_repairs) return;
  const bool needed = surface_planning ? !row.construction_valid
                      : quality_only   ? (!row.quality_valid || (geometry_repairs && row.geometry_invalid) ||
                                        (size_repairs && row.debt.size_bad))
                                       : !row.valid;
  if (needed && !queues.cell_repairs.count(cell.id))
    AddRepairs(cell, quality_only && size_repairs && row.quality_valid && !geometry_repairs);
  else if (!needed && queues.cell_repairs.count(cell.id))
    RemoveRepairs(cell.id);
}

// Updates existing cells for the residual size repair stage.
void Selection::EnableSizeRepairs(const Owned& working) {
  size_repairs = true;
  for (const auto& row : working) {
    UpdateRepairs(row.second.cell, census.cells.at(row.first));
    UpdateRepairWeight(row.first, census.cells.at(row.first));
  }
  FinishRepairs();
}

// Updates existing cells for the residual geometry repair stage.
void Selection::EnableGeometryRepairs(const Owned& working) {
  size_repairs = false;
  geometry_repairs = true;
  for (const auto& row : working) {
    UpdateRepairs(row.second.cell, census.cells.at(row.first));
    UpdateRepairWeight(row.first, census.cells.at(row.first));
  }
  FinishRepairs();
}

// Requeues measured lengths under the active collapse screen.
void Selection::EnableCollapseFilter() {
  prune_collapses = true;
  for (const auto& row : census.edges) Queue(row.first, row.second.measure.length, true);
}

// Checks whether a candidate leaves a fixed source facet.
bool Selection::LeavesFacet(const OwnedCell& owned, Id removed, Point retained) {
  if (geometry.Mobility())
    return false;  // Original chart seams are not physical constraints; full-star remapping proves replacement.
  for (size_t face = 0; face < 4; ++face) {
    if (!owned.physical[face].present || owned.cell.v[face].id == removed) continue;
    ++stats.collapse_geometry_checks;
    if (!geometry.Contains(owned.physical[face].facet, retained)) return true;
  }
  return false;
}

// Records both locally forbidden collapse directions of an admitted edge.
unsigned char Selection::Forbidden(const OwnedCell& owned, Edge edge) {
  if (!coarsening) return 0;
  Timer timer(stats.collapse_geometry_seconds, timings, SU2NativeTiming3D::Operation::GeometryBound);
  unsigned char blocked = 0;
  for (unsigned direction = 0; direction < 2; ++direction) {
    const Id retained = edge[direction], removed = edge[1 - direction];
    Point point{};
    for (auto node : owned.cell.v)
      if (node.id == retained) point = node.p;
    if (LeavesFacet(owned, removed, point)) blocked |= 1u << direction;
  }
  return blocked;
}

// Seeds one cell's required repair features and accounts for the retained capacity.
void Selection::AddRepairs(const Cell& cell, bool length_only) {
  auto& repairs = queues.cell_repairs[cell.id];
  if (!repairs.empty()) throw std::logic_error("Duplicate native 3D repair queue seed.");
  if (!length_only)
    for (auto node : cell.v) repairs.push_back({1, {node.id, 0, 0}});
  for (auto edge : census.cells.at(cell.id).edges)
    if (!length_only || census.edges.at(edge).measure.length > MaximumMetricEdge)
      repairs.push_back({2, {edge[0], edge[1], 0}});
  if (!length_only)
    for (auto face : Faces(cell)) repairs.push_back({3, Key(face)});
  for (auto key : repairs) {
    const auto inserted = queues.repair_users.emplace(key, RepairDemand{0, queues.repair_epoch, false});
    auto& demand = inserted.first->second;
    if (demand.users++ == 0 && !demand.blocked) queues.repair_queue.insert({demand.epoch, key});
  }
  const long double weight = RepairWeight(census.cells.at(cell.id));
  queues.repair_cell_severity[cell.id] = weight;
  for (const auto& key : repairs) AddRepairSeverity(key, weight);
  queues.repair_bytes = transfer_memory::Add(queues.repair_bytes, transfer_memory::Bytes(repairs));
}

// Updates the age and availability of a serviced repair feature.
void Selection::Service(Feature feature, bool accepted) {
  const RepairFeature key{feature.count, feature.vertices};
  const auto found = queues.repair_users.find(key);
  if (found == queues.repair_users.end()) return;
  auto& demand = found->second;
  queues.repair_queue.erase({demand.epoch, key});
  queues.repair_severity_queue.erase({-queues.repair_severity.at(key), key});
  demand.epoch = queues.repair_epoch;
  demand.blocked = !accepted;
  if (accepted && demand.users) {
    queues.repair_queue.insert({demand.epoch, key});
    queues.repair_severity_queue.insert({-queues.repair_severity.at(key), key});
  }
}

// Releases zero-user demands only after publication finishes.
void Selection::FinishRepairs() {
  for (const auto& key : queues.repair_zero) {
    const auto found = queues.repair_users.find(key);
    if (found != queues.repair_users.end() && !found->second.users) {
      queues.repair_queue.erase({found->second.epoch, key});
      queues.repair_severity_queue.erase({-queues.repair_severity.at(key), key});
      queues.repair_severity.erase(key);
      queues.repair_users.erase(found);
    }
  }
  queues.repair_zero.clear();
}

// Searches measured short edges for at least one unblocked direction.
bool Selection::HasCoarseningDemand() const {
  for (const auto& edge : census.edges)
    if (Candidate(edge.second.measure.length))
      for (unsigned direction = 0; direction < 2; ++direction) {
        const auto retained = edge.first[direction], removed = edge.first[1 - direction];
        const auto blocked = queues.blocked_edges.find(removed);
        if (!edge.second.forbidden[direction] &&
            (blocked == queues.blocked_edges.end() || !blocked->second.count(retained)))
          return true;
      }
  return false;
}

// Snapshots original length directions for the next construction generation.
void Selection::SeedFrontier(const Owned& working) {
  for (const auto& row : working)
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j) {
        Edge edge{row.second.cell.v[i].id, row.second.cell.v[j].id};
        std::sort(edge.begin(), edge.end());
        queues.construction_frontier.insert(edge);
        if (coarsening) {
          std::swap(edge[0], edge[1]);
          queues.construction_frontier.insert(edge);
        }
      }
}

// Removes a served direction and all corresponding live queue entries.
bool Selection::ServiceEdge(Edge edge) {
  const bool serviced = queues.construction_frontier.erase(edge);
  const auto direction = edge;
  std::sort(edge.begin(), edge.end());
  const auto measured = census.edges.find(edge);
  if (measured != census.edges.end()) {
    queues.queue.erase({Priority(measured->second.measure.length), direction});
    queues.surface_queue.erase({Priority(measured->second.measure.length), direction});
  }
  const auto star = census.vertex_cells.find(edge[0]);
  if (star != census.vertex_cells.end())
    for (Id id : star->second) {
      const auto& row = census.cells.at(id);
      for (size_t slot = 0; slot < row.edges.size(); ++slot)
        if (row.edges[slot] == edge)
          queues.geometry_queue.erase({-GeometryPriority(row.geometry_scores[slot], edge), id, edge});
    }
  return serviced;
}

// Retires frontier edges whose support may have changed after publication.
size_t Selection::ServiceClaimedVertex(Id vertex) {
  // Full old/new vertex claims retire every edge whose paired-face prediction could have changed.
  const auto star = census.vertex_cells.find(vertex);
  if (star == census.vertex_cells.end()) return 0;
  size_t serviced = 0;
  for (Id id : star->second) {
    const auto& row = census.cells.at(id);
    if (!coarsening) {
      for (const auto& edge : row.edges)
        if (queues.construction_frontier.count(edge) && (!queues.split_lanes || census.edges.at(edge).physical_users)) {
          Note(edge, 3);
          serviced += ServiceEdge(edge);
        }
    } else {
      // Every collapse of this neighboring point imports a star containing the committed vertex.
      for (Id removed : row.vertices) {
        const auto& neighbors = census.vertex_cells.at(removed);
        for (Id neighbor : neighbors)
          for (Id retained : census.cells.at(neighbor).vertices)
            if (retained != removed && queues.construction_frontier.count({retained, removed}))
              serviced += ServiceEdge({retained, removed});
      }
    }
  }
  return serviced;
}

// Recreates the fixed vertex, edge and face order of a transported repair mask.
std::array<RepairFeature, 14> Selection::RepairKeys(const CellState& row) {
  std::array<RepairFeature, 14> keys{};
  unsigned at = 0;
  for (Id vertex : row.vertices) keys[at++] = {1, {vertex, 0, 0}};
  for (const auto& edge : row.edges) keys[at++] = {2, {edge[0], edge[1], 0}};
  for (unsigned opposite = 0; opposite < 4; ++opposite) {
    Face face{};
    unsigned k = 0;
    for (unsigned i = 0; i < 4; ++i)
      if (i != opposite) face[k++] = row.vertices[i];
    keys[at++] = {3, Key(face)};
  }
  return keys;
}

// Stores live repair epochs and block bits in the existing migration row.
void Selection::SaveRepairState(CachedRow& record) const {
  const auto found = queues.cell_repairs.find(record.id);
  if (found == queues.cell_repairs.end()) return;
  const auto keys = RepairKeys(record.state);
  size_t saved = 0;
  for (unsigned i = 0; i < keys.size(); ++i) {
    if (std::find(found->second.begin(), found->second.end(), keys[i]) == found->second.end()) continue;
    const auto& demand = queues.repair_users.at(keys[i]);
    if (!demand.users || demand.epoch > queues.repair_epoch) throw std::logic_error("Invalid phase repair history.");
    record.repair_active |= uint16_t(1u << i);
    record.repair_epochs[i] = demand.epoch;
    if (demand.blocked) record.repair_blocked |= uint16_t(1u << i);
    ++saved;
  }
  if (saved != found->second.size()) throw std::logic_error("Incomplete phase repair history.");
}

// Rebuilds feature demand and merges age and refusal from the restored census.
void Selection::RestoreRepairState(const std::vector<CachedRow>& records) {
  if (!queues.cell_repairs.empty() || !queues.repair_users.empty() || !queues.repair_zero.empty())
    throw std::logic_error("Phase repair history must restore into an empty queue.");
  for (const auto& record : records) {
    if ((record.repair_active & ~uint16_t(0x3fff)) || (record.repair_blocked & ~record.repair_active))
      throw std::logic_error("Invalid migrated repair masks.");
    const auto& row = census.cells.at(record.id);
    const bool needed =
        track_repairs && (surface_planning ? !row.construction_valid
                          : quality_only   ? (!row.quality_valid || (geometry_repairs && row.geometry_invalid) ||
                                            (size_repairs && row.debt.size_bad))
                                           : !row.valid);
    if (bool(record.repair_active) != needed) throw std::logic_error("Migration changed required repair support.");
    if (!needed) continue;
    const auto keys = RepairKeys(row);
    auto& repairs = queues.cell_repairs[record.id];
    repairs.reserve(keys.size());
    const auto weight = RepairWeight(row);
    queues.repair_cell_severity.emplace(record.id, weight);
    for (unsigned i = 0; i < keys.size(); ++i) {
      if (!(record.repair_active & (1u << i))) continue;
      if (record.repair_epochs[i] > queues.repair_epoch)
        throw std::logic_error("Migrated repair epoch is in the future.");
      repairs.push_back(keys[i]);
      const bool blocked = record.repair_blocked & (1u << i);
      auto& demand =
          queues.repair_users.emplace(keys[i], RepairDemand{0, record.repair_epochs[i], blocked}).first->second;
      ++demand.users;
      demand.epoch = std::min(demand.epoch, record.repair_epochs[i]);
      demand.blocked = demand.blocked && blocked;
      auto& severity = queues.repair_severity[keys[i]];
      severity += weight;
      if (!std::isfinite(severity)) throw std::logic_error("Nonfinite migrated repair severity.");
    }
    queues.repair_bytes = transfer_memory::Add(queues.repair_bytes, transfer_memory::Bytes(repairs));
  }
  // A merged feature stays unavailable only if every prior contributor rejected its unchanged support.
  for (const auto& row : queues.repair_users) {
    const auto& demand = row.second;
    if (!demand.users) throw std::logic_error("Missing migrated repair history.");
    if (!demand.blocked) {
      queues.repair_queue.insert({demand.epoch, row.first});
      queues.repair_severity_queue.insert({-queues.repair_severity.at(row.first), row.first});
    }
  }
}

// Collects the owned cells and vertices of a feature under the existing scan bounds.
void Selection::CollectRepairStar(const Owned& working, const RepairFeature& key, std::vector<Id>& ids,
                                  std::set<Id>& vertices, bool& too_large) const {
  const auto count = key.first;
  const auto& feature = key.second;
  if (count < 1 || count > 3) throw std::logic_error("Invalid native repair feature.");
  const std::set<Id>* seed = nullptr;
  for (unsigned i = 0; i < count; ++i) {
    const auto found = census.vertex_cells.find(feature[i]);
    if (found == census.vertex_cells.end()) return;
    if (!seed || found->second.size() < seed->size()) seed = &found->second;
  }
  size_t scanned = 0;
  for (Id id : *seed) {
    if (++scanned > MaximumDirectoryScan) {
      too_large = true;
      return;
    }
    const auto found = working.find(id);
    if (found == working.end()) throw std::logic_error("Repair star lost an owned cell.");
    const auto& cell = found->second.cell;
    bool contains = true;
    for (unsigned i = 0; i < count; ++i)
      contains =
          contains && std::any_of(cell.v.begin(), cell.v.end(), [&](Node node) { return node.id == feature[i]; });
    if (!contains) continue;
    ids.push_back(id);
    for (const auto& node : cell.v) vertices.insert(node.id);
    if (ids.size() > MaximumCavityCells) {
      too_large = true;
      return;
    }
  }
}
