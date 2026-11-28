// Manages versioned cell and edge certificates, completeness and debt.

// Accounts for every live census and queue allocation admitted by the engine.
size_t Selection::Bytes() const {
  return transfer_memory::Add(
      MapBytes(census.cells), MapBytes(census.edges), MapBytes(queues.queue), MapBytes(queues.surface_queue),
      MapBytes(queues.edge_history), MapBytes(queues.construction_frontier), MapBytes(queues.cell_repairs),
      MapBytes(queues.repair_users), MapBytes(queues.repair_queue), MapBytes(queues.repair_cell_severity),
      MapBytes(queues.repair_severity), MapBytes(queues.repair_severity_queue), MapBytes(queues.repair_zero),
      queues.repair_bytes, MapBytes(queues.geometry_queue), MapBytes(queues.geometry_split_priorities),
      MapBytes(census.vertex_cells), MapBytes(queues.blocked_edges),
      transfer_memory::Mul(transfer_memory::Add(census.vertex_links, queues.blocked_links),
                           sizeof(Id) + 4 * sizeof(void*)));
}

// Applies one cached debt contribution in the same order for entry and retirement.
void Selection::Account(const CellState& row, bool add) {
  const auto count = [&](size_t& total, size_t value) {
    if (add)
      total += value;
    else
      total -= value;
  };
  const auto& d = row.debt;
  const long double sign = add ? 1.L : -1.L;
  count(census.debt.q_measured_cells, d.q_measured);
  count(census.debt.j_measured_cells, d.j_measured);
  count(census.debt.shape_measured_cells, d.q_measured && d.j_measured);
  count(census.debt.height_measured_cells, d.height_measured);
  count(census.debt.geometry_measured_cells, d.geometry_measured);
  count(census.debt.height_unresolved_cells, d.height_unresolved);
  count(census.debt.q_bad_cells, d.q_bad);
  count(census.debt.j_bad_cells, d.j_bad);
  count(census.debt.size_bad_cells, d.size_bad);
  count(census.debt.height_bad_cells, d.height_bad);
  count(census.debt.geometry_bad_cells, row.geometry_invalid);
  census.debt.q_hinge_squared += sign * d.q;
  census.debt.j_hinge_squared += sign * d.j;
  census.debt.size_hinge_squared += sign * d.size;
  census.debt.height_hinge_squared += sign * d.height;
  census.debt.geometry_excess_squared += sign * d.geometry;
}

// Identifies an owned cell carrying a physical face with a requested wall height.
bool Selection::RequestedWall(const OwnedCell& owned) const {
  if (heights.empty()) return false;
  return std::any_of(owned.physical.begin(), owned.physical.end(),
                     [&](const FacetBinding& f) { return f.present && heights.count(geometry.Get(f.facet).marker); });
}

// Counts only required wall cells with current cached measurements.
std::array<size_t, 3> Selection::SurfaceCensus(const Owned& owned) const {
  std::array<size_t, 3> count{};  // required, current measured, current geometry measured
  for (const auto& item : owned)
    if (RequestedWall(item.second)) {
      ++count[0];
      const auto found = census.cells.find(item.first);
      if (found != census.cells.end() && found->second.version == item.second.version) {
        ++count[1];
        count[2] += found->second.debt.geometry_measured;
      }
    }
  return count;
}

// Reports census completeness and queue sizes from the current published state.
StageProgress Selection::Progress(size_t owned) const {
  StageProgress result = census.progress_consistent ? census.debt : StageProgress{};
  result.cache_consistent = census.progress_consistent;
  result.owned_cells = owned;
  result.measured_cells = census.progress_consistent ? census.cells.size() : 0;
  result.invalid_cells = census.progress_consistent ? census.invalid : 0;
  result.construction_invalid_cells = census.progress_consistent ? census.construction_invalid : 0;
  result.quality_invalid_cells = census.progress_consistent ? census.quality_invalid : 0;
  result.remaining_edges = queues.queue.size() + queues.surface_queue.size() + queues.geometry_queue.size();
  result.remaining_repairs = queues.repair_queue.size();
  result.unmeasured_cells = owned - result.measured_cells;
  result.shape_unmeasured_cells = owned - result.shape_measured_cells;
  result.height_requested = !heights.empty();
  result.surface_error_requested = !geometry.ExtensionComponents().empty();
  result.height_unmeasured_cells = result.height_requested ? owned - result.height_measured_cells : 0;
  result.geometry_unmeasured_cells = owned - result.geometry_measured_cells;
  result.census_complete = result.measured_cells == owned && census.progress_consistent && census.census_started;
  result.shape_complete = result.shape_measured_cells == owned && census.progress_consistent && census.census_started;
  result.height_complete = (!result.height_requested || !result.height_unmeasured_cells) && census.progress_consistent;
  result.geometry_complete = !result.geometry_unmeasured_cells && census.progress_consistent;
  result.complete =
      result.census_complete && result.shape_complete && result.height_complete && result.geometry_complete;
  // Incremental subtraction can leave round-off below zero after a large number of updates.
  result.q_hinge_squared = std::max(0.L, result.q_hinge_squared);
  result.j_hinge_squared = std::max(0.L, result.j_hinge_squared);
  result.size_hinge_squared = std::max(0.L, result.size_hinge_squared);
  result.height_hinge_squared = std::max(0.L, result.height_hinge_squared);
  result.geometry_excess_squared = std::max(0.L, result.geometry_excess_squared);
  return result;
}

// Exports the matching cell version, edge measurements and active repair history.
bool Selection::Diagnose(const OwnedCell& owned, CellDiagnostic& out) const {
  out = {};
  const auto found = census.cells.find(owned.cell.id);
  if (!census.progress_consistent || found == census.cells.end() || found->second.version != owned.version)
    return false;
  const auto& row = found->second;
  const auto& d = row.debt;
  out.q_measured = d.q_measured;
  out.j_measured = d.j_measured;
  out.geometry_measured = d.geometry_measured;
  out.q = row.q_value;
  out.J = row.j_value;
  out.geometry_excess_l2 = double(std::sqrt(std::max(0.L, d.geometry)));
  out.q_bad = d.q_bad;
  out.j_bad = d.j_bad;
  out.geometry_bad = row.geometry_invalid;
  out.size_bad = d.size_bad;
  out.height_bad = d.height_bad;
  out.height_unresolved = d.height_unresolved;
  out.surface = row.surface;
  out.near_feature = row.near_feature;
  out.feature_known = row.feature_known;
  out.edges = row.edges;
  for (size_t i = 0; i < 6; ++i) {
    const auto edge = census.edges.find(row.edges[i]);
    if (edge == census.edges.end()) return false;
    out.lengths[i] = edge->second.measure.length;
  }
  const auto repairs = queues.cell_repairs.find(owned.cell.id);
  if (repairs != queues.cell_repairs.end())
    for (const auto& key : repairs->second) {
      if (out.repair_count == out.repairs.size()) return false;
      const auto demand = queues.repair_users.find(key);
      if (demand == queues.repair_users.end()) return false;
      out.repairs[out.repair_count++] = {Feature{key.second, key.first}, demand->second.epoch, demand->second.blocked};
    }
  out.cache_valid = true;
  return true;
}

// Reuses certificates for canonical edges present in the current census.
void Selection::Cached(const Cell& cell, std::map<Edge, EdgeMeasure>& values) const {
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = i + 1; j < 4; ++j) {
      const Edge key{std::min(cell.v[i].id, cell.v[j].id), std::max(cell.v[i].id, cell.v[j].id)};
      const auto found = census.edges.find(key);
      if (found != census.edges.end()) values.emplace(key, found->second.measure);
    }
}

// Removes a published cell and retires its census and queue contributions in order.
void Selection::Drop(Id id) {
  const auto found = census.cells.find(id);
  if (found == census.cells.end()) throw std::logic_error("Missing native 3D cached cell removal.");
  Account(found->second, false);
  census.invalid -= !found->second.valid;
  census.construction_invalid -= !found->second.construction_valid;
  census.quality_invalid -= !found->second.quality_valid;
  census.unmeasured_shape -= !found->second.shape_measured;
  for (size_t i = 0; i < 6; ++i)
    queues.geometry_queue.erase(
        {-GeometryPriority(found->second.geometry_scores[i], found->second.edges[i]), id, found->second.edges[i]});
  for (Id vertex : found->second.vertices) {
    auto& star = census.vertex_cells.at(vertex);
    census.vertex_links -= star.erase(id);
    if (star.empty()) census.vertex_cells.erase(vertex);
  }
  RemoveRepairs(id);
  for (size_t slot = 0; slot < found->second.edges.size(); ++slot) {
    const auto key = found->second.edges[slot];
    auto& state = census.edges.at(key);
    Queue(key, state.measure.length, false);
    state.physical_users -= found->second.physical[slot];
    for (unsigned direction = 0; direction < 2; ++direction)
      state.forbidden[direction] -= bool(found->second.forbidden[slot] & (1u << direction));
    if (--state.users == 0) {
      Forget(key);
      Forget({key[1], key[0]});
      census.edges.erase(key);
    } else
      Queue(key, state.measure.length, true);
  }
  census.cells.erase(found);
}

// Retargets the versioned census and rebuilds queues in the original phase order.
void Selection::ChangePhase(bool improvement, const Owned& working,
                            const std::shared_ptr<const FrozenMetricIdentity>& identity) {
  queues.geometry_split_priorities.clear();
  queues.geometry_priority_mode = false;
  size_repairs = geometry_repairs = false;
  track_repairs = repair_event = improvement;
  prune_collapses = false;
  queues.queue.clear();
  queues.surface_queue.clear();
  queues.geometry_queue.clear();
  queues.construction_frontier.clear();
  for (auto& item : census.cells) {
    auto& row = item.second;
    const auto found = working.find(item.first);
    if (found == working.end() || found->second.version != row.version)
      throw std::logic_error("Phase census lost a versioned owned row.");
    const auto& cell = found->second.cell;
    for (unsigned i = 0; i < 4; ++i)
      if (cell.v[i].id != row.vertices[i])
        throw std::logic_error("Phase census changed owned vertices without a version.");
    for (const auto& edge : row.edges) {
      const auto& cached = census.edges.at(edge).measure;
      const Node *a = nullptr, *b = nullptr;
      for (const auto& node : cell.v) {
        if (node.id == edge[0]) a = &node;
        if (node.id == edge[1]) b = &node;
      }
      if (!a || !b || !cached.Reusable(a->p, b->p, identity))
        throw std::logic_error("Phase census changed edge coordinates or field without a version.");
    }
    const bool before = row.valid;
    census.invalid -= !before;
    row.target_valid =
        improvement && !surface_planning ? (!row.debt.size_bad && row.quality_valid) : !row.debt.size_bad;
    row.valid = row.target_valid && !row.geometry_invalid;
    census.invalid += !row.valid;
    UpdateRepairs(found->second.cell, row);
    UpdateRepairWeight(item.first, row);
  }
  FinishRepairs();
  if (!improvement) {
    SeedFrontier(working);
    for (const auto& row : census.edges) Queue(row.first, row.second.measure.length, true);
    if (!coarsening)
      for (const auto& item : census.cells)
        for (size_t i = 0; i < 6; ++i) {
          const auto edge = item.second.edges[i];
          const auto blocked = queues.blocked_edges.find(edge[1]);
          if (item.second.geometry_scores[i] > 0 && queues.construction_frontier.count(edge) &&
              (blocked == queues.blocked_edges.end() || !blocked->second.count(edge[0])))
            queues.geometry_queue.insert({-GeometryPriority(item.second.geometry_scores[i], edge), item.first, edge});
        }
  }
}

// Records whether a measured cell touches a known lower-dimensional feature.
void Selection::ClassifyFeatures(const Cell& cell, CellState& row) const {
  const auto* parameters = geometry.Parameters();
  const auto* mobility = geometry.Mobility();
  row.near_feature = false;
  row.feature_known = parameters && mobility && mobility->identity;
  if (row.feature_known) try {
      for (const auto& node : cell.v) {
        const auto* point = parameters->Find(node.id);
        if (point && parameters->Geometry().MobilityDimension(point->binding, *mobility) < 2) row.near_feature = true;
      }
    } catch (const std::exception&) {
      row.feature_known = row.near_feature = false;
    }
}

// Replaces shape and height debt without changing the immutable edge certificate.
void Selection::RefreshQuality(const OwnedCell& owned, const PatchMetric& metric, bool valid, bool quality_valid,
                               const FirstHeightReport* height, bool height_valid,
                               const std::shared_ptr<const FrozenMetricIdentity>& identity) {
  auto& row = census.cells.at(owned.cell.id);
  if (row.version != owned.version) throw std::logic_error("Quality refresh found stale owned version.");
  ClassifyFeatures(owned.cell, row);
  if (metric.edges.size() != 6 || !std::isfinite(metric.maximum_length))
    throw std::invalid_argument("Incomplete quality refresh edge measurements.");
  for (const auto& key : row.edges) {
    const auto measured = metric.edges.find(key);
    if (measured == metric.edges.end()) throw std::logic_error("Quality refresh omitted cached edge.");
    const auto& prior = census.edges.at(key).measure;
    const auto& fresh = measured->second;
    if (!prior.Reusable(fresh.a, fresh.b, identity) || !fresh.Reusable(prior.a, prior.b, identity) ||
        prior.length != fresh.length)
      throw std::logic_error("Quality refresh changed cached edge certificate.");
  }
  Account(row, false);
  census.invalid -= !row.valid;
  census.quality_invalid -= !row.quality_valid;
  census.unmeasured_shape -= !row.shape_measured;
  auto& d = row.debt;
  auto hinge = [](long double x) { return std::max(0.L, x); };
  row.shape_measured = metric.shape_measured;
  row.quality_valid = quality_valid;
  row.q_value = metric.shape_measured ? metric.minimum_mean_ratio : 0;
  row.j_value = metric.shape_measured ? metric.minimum_scaled_jacobian : 0;
  d.q_measured = metric.shape_measured && std::isfinite(metric.minimum_mean_ratio);
  d.j_measured = metric.shape_measured && std::isfinite(metric.minimum_scaled_jacobian);
  d.q = d.j = 0;
  d.q_bad = d.j_bad = false;
  if (d.q_measured) {
    const auto r = hinge(1.L - static_cast<long double>(metric.minimum_mean_ratio) / MinimumMeanRatio);
    d.q = r * r;
    d.q_bad = metric.minimum_mean_ratio < MinimumMeanRatio;
  }
  if (d.j_measured) {
    const auto r = hinge(1.L - static_cast<long double>(metric.minimum_scaled_jacobian) / MinimumScaledJacobian);
    d.j = r * r;
    d.j_bad = metric.minimum_scaled_jacobian < MinimumScaledJacobian;
  }
  d.height = 0;
  d.height_measured = d.height_bad = d.height_unresolved = false;
  if (height && !heights.empty()) {
    d.height_measured =
        height->measured && std::isfinite(height->maximum_relative_error) && std::isfinite(height->minimum_barycentric);
    d.height_bad = !height_valid;
    d.height_unresolved = !d.height_measured || height->missing_cells || height->reference_clearance_failures;
    if (d.height_measured) {
      const auto r = hinge(static_cast<long double>(height->maximum_relative_error) / heights.tolerance - 1.L),
                 f = hinge(-3.L * height->minimum_barycentric);
      d.height = r * r + f * f;
    }
  }
  row.target_valid = valid;
  row.valid = valid && !row.geometry_invalid;
  Account(row, true);
  census.invalid += !row.valid;
  census.quality_invalid += !row.quality_valid;
  census.unmeasured_shape += !row.shape_measured;
  UpdateRepairs(owned.cell, row);
  UpdateRepairWeight(owned.cell.id, row);
  FinishRepairs();
}

// Publishes a measured cell into the census and updates its queue contributions.
void Selection::Add(const OwnedCell& owned, const PatchMetric& metric, bool valid, bool quality_valid,
                    const std::string& reason, const FirstHeightReport* height, bool height_valid) {
  D9QueryScope d9_query(SU2NativeD9::Site::Census, &geometry);
  const auto& cell = owned.cell;
  if (metric.edges.size() != 6 || !std::isfinite(metric.maximum_length))
    throw std::runtime_error(reason.empty() ? "Incomplete native 3D initial/updated metric measurement." : reason);
  CellState row;
  row.version = owned.version;
  row.valid = row.target_valid = valid;
  row.shape_measured = metric.shape_measured;
  row.quality_valid = quality_valid;
  row.surface =
      std::any_of(owned.physical.begin(), owned.physical.end(), [](const FacetBinding& face) { return face.present; });
  const auto* parameters = geometry.Parameters();
  if (metric.shape_measured) ClassifyFeatures(cell, row);
  CollectCellDebt(owned, metric, height, height_valid, row);
  std::map<Edge, long double> geometry_scores;
  MeasureGeometryDebt(owned, parameters, row, geometry_scores);
  for (size_t i = 0; i < 4; ++i) {
    row.vertices[i] = cell.v[i].id;
    census.vertex_links += census.vertex_cells[row.vertices[i]].insert(cell.id).second;
  }
  AddCellEdges(owned, metric, geometry_scores, row);
  row.construction_valid = row.construction_valid && !row.geometry_invalid;
  const auto inserted = census.cells.emplace(cell.id, std::move(row));
  if (!inserted.second) throw std::logic_error("Duplicate native 3D cached cell.");
  Account(inserted.first->second, true);
  census.invalid += !inserted.first->second.valid;
  census.construction_invalid += !inserted.first->second.construction_valid;
  census.quality_invalid += !inserted.first->second.quality_valid;
  census.unmeasured_shape += !inserted.first->second.shape_measured;
  UpdateRepairs(cell, inserted.first->second);
}

// Restores a migrated cell certificate and accounts for it without seeding the queue.
void Selection::Restore(const OwnedCell& owned, const CachedRow& record,
                        const std::shared_ptr<const FrozenMetricIdentity>& identity) {
  const auto& row = record.state;
  const auto& cell = owned.cell;
  if (record.id != cell.id || row.version != owned.version)
    throw std::logic_error("Migrated selection version mismatch.");
  for (unsigned i = 0; i < 4; ++i) {
    if (row.vertices[i] != cell.v[i].id) throw std::logic_error("Migrated selection vertex mismatch.");
    census.vertex_links += census.vertex_cells[row.vertices[i]].insert(cell.id).second;
  }
  if (!census.cells.emplace(cell.id, row).second) throw std::logic_error("Duplicate migrated selection row.");
  for (unsigned i = 0; i < 6; ++i) {
    const auto edge = row.edges[i];
    Point a{}, b{};
    bool has_a = false, has_b = false;
    for (auto node : cell.v) {
      if (node.id == edge[0]) {
        a = node.p;
        has_a = true;
      }
      if (node.id == edge[1]) {
        b = node.p;
        has_b = true;
      }
    }
    if (!has_a || !has_b || !(record.lengths[i] > 0)) throw std::logic_error("Invalid migrated edge certificate.");
    auto inserted = census.edges.emplace(edge, EdgeState{EdgeMeasure{a, b, record.lengths[i], identity}, 0});
    auto& e = inserted.first->second;
    if (!e.measure.Reusable(a, b, identity) || e.measure.length != record.lengths[i])
      throw std::logic_error("Migrated edge certificates disagree.");
    e.physical_users += row.physical[i];
    ++e.users;
    for (unsigned d = 0; d < 2; ++d) e.forbidden[d] += bool(row.forbidden[i] & (1u << d));
  }
  Account(row, true);
  census.invalid += !row.valid;
  census.construction_invalid += !row.construction_valid;
  census.quality_invalid += !row.quality_valid;
  census.unmeasured_shape += !row.shape_measured;
}

// Adopts the pending indexes without swapping modes, references or repair epoch.
void Selection::SwapState(Selection& other) noexcept {
  census.cells.swap(other.census.cells);
  census.edges.swap(other.census.edges);
  queues.queue.swap(other.queues.queue);
  queues.surface_queue.swap(other.queues.surface_queue);
  queues.geometry_queue.swap(other.queues.geometry_queue);
  queues.geometry_split_priorities.swap(other.queues.geometry_split_priorities);
  std::swap(queues.geometry_priority_mode, other.queues.geometry_priority_mode);
  queues.construction_frontier.swap(other.queues.construction_frontier);
  queues.edge_history.swap(other.queues.edge_history);
  census.vertex_cells.swap(other.census.vertex_cells);
  queues.blocked_edges.swap(other.queues.blocked_edges);
  queues.cell_repairs.swap(other.queues.cell_repairs);
  queues.repair_users.swap(other.queues.repair_users);
  queues.repair_queue.swap(other.queues.repair_queue);
  queues.repair_cell_severity.swap(other.queues.repair_cell_severity);
  queues.repair_severity.swap(other.queues.repair_severity);
  queues.repair_severity_queue.swap(other.queues.repair_severity_queue);
  queues.repair_zero.swap(other.queues.repair_zero);
  std::swap(census.vertex_links, other.census.vertex_links);
  std::swap(queues.blocked_links, other.queues.blocked_links);
  std::swap(queues.repair_bytes, other.queues.repair_bytes);
  std::swap(census.invalid, other.census.invalid);
  std::swap(census.construction_invalid, other.census.construction_invalid);
  std::swap(census.quality_invalid, other.census.quality_invalid);
  std::swap(census.unmeasured_shape, other.census.unmeasured_shape);
  std::swap(census.debt, other.census.debt);
  std::swap(census.progress_consistent, other.census.progress_consistent);
  std::swap(census.census_started, other.census.census_started);
}

// Drops the cached census and queues while retaining directed rejection history.
void Selection::Reset(bool improvement) {
  queues.geometry_split_priorities.clear();
  queues.geometry_priority_mode = false;
  census.cells.clear();
  census.edges.clear();
  queues.queue.clear();
  queues.surface_queue.clear();
  queues.geometry_queue.clear();
  queues.construction_frontier.clear();
  queues.edge_history.clear();
  census.vertex_cells.clear();
  queues.cell_repairs.clear();
  queues.repair_users.clear();
  queues.repair_queue.clear();
  queues.repair_cell_severity.clear();
  queues.repair_severity.clear();
  queues.repair_severity_queue.clear();
  queues.repair_zero.clear();
  census.invalid = census.construction_invalid = census.quality_invalid = census.unmeasured_shape =
      queues.repair_bytes = census.vertex_links = 0;
  census.debt = {};
  census.progress_consistent = true;
  census.census_started = false;
  track_repairs = repair_event = improvement;
  prune_collapses = false;
}

// Measures shape, size and height debt before publication to the indexes.
void Selection::CollectCellDebt(const OwnedCell& owned, const PatchMetric& metric, const FirstHeightReport* height,
                                bool height_valid, CellState& row) const {
  const auto& cell = owned.cell;
  row.construction_valid = metric.maximum_length <= MaximumMetricEdge;
  auto hinge = [](long double value) { return std::max(0.L, value); };
  auto& d = row.debt;
  row.q_value = metric.shape_measured ? metric.minimum_mean_ratio : 0;
  row.j_value = metric.shape_measured ? metric.minimum_scaled_jacobian : 0;
  d.q_measured = metric.shape_measured && std::isfinite(metric.minimum_mean_ratio);
  d.j_measured = metric.shape_measured && std::isfinite(metric.minimum_scaled_jacobian);
  if (d.q_measured) {
    const auto r = hinge(1.L - static_cast<long double>(metric.minimum_mean_ratio) / MinimumMeanRatio);
    d.q = r * r;
    d.q_bad = metric.minimum_mean_ratio < MinimumMeanRatio;
  }
  if (d.j_measured) {
    const auto r = hinge(1.L - static_cast<long double>(metric.minimum_scaled_jacobian) / MinimumScaledJacobian);
    d.j = r * r;
    d.j_bad = metric.minimum_scaled_jacobian < MinimumScaledJacobian;
  }
  d.size_bad = metric.maximum_length > MaximumMetricEdge;
  for (const auto& edge : metric.edges) {
    if (surface_planning) {
      bool requested = false;
      for (unsigned f = 0; f < 4; ++f)
        if (owned.physical[f].present && heights.count(geometry.Get(owned.physical[f].facet).marker) &&
            cell.v[f].id != edge.first[0] && cell.v[f].id != edge.first[1])
          requested = true;
      if (!requested) continue;
    }
    const auto r = hinge(edge.second.length / MaximumMetricEdge - 1.L);
    d.size += r * r;
  }
  if (height && !heights.empty()) {
    d.height_measured =
        height->measured && std::isfinite(height->maximum_relative_error) && std::isfinite(height->minimum_barycentric);
    d.height_bad = !height_valid;
    d.height_unresolved = !d.height_measured || height->missing_cells || height->reference_clearance_failures;
    if (d.height_measured) {
      const auto r = hinge(static_cast<long double>(height->maximum_relative_error) / heights.tolerance - 1.L);
      const auto f = hinge(-3.L * height->minimum_barycentric);
      d.height = r * r + f * f;
    }
  }
}

// Measures physical faces in slot order before the cell is indexed.
void Selection::MeasureGeometryDebt(const OwnedCell& owned, const BoundaryParameterState* parameters, CellState& row,
                                    std::map<Edge, long double>& geometry_scores) {
  const auto& cell = owned.cell;
  auto& d = row.debt;
  if (!geometry.ExtensionComponents().empty()) {
    const auto faces = Faces(cell);
    for (size_t slot = 0; slot < 4; ++slot)
      if (owned.physical[slot].present) {
        MeasureGeometryFace(owned, faces[slot], slot, parameters, row, geometry_scores);
      }
  }
  if (!d.geometry_measured) d.geometry = 0;
}

// Measures one physical face and records its contribution to geometry debt.
void Selection::MeasureGeometryFace(const OwnedCell& owned, const Face& face, size_t slot,
                                    const BoundaryParameterState* parameters, CellState& row,
                                    std::map<Edge, long double>& geometry_scores) {
  const auto& cell = owned.cell;
  auto& d = row.debt;
  auto hinge = [](long double value) { return std::max(0.L, value); };
  if (surface_planning && !heights.count(geometry.Get(owned.physical[slot].facet).marker)) return;
  std::array<Point, 3> triangle{};
  for (size_t i = 0; i < 3; ++i)
    for (auto n : cell.v)
      if (n.id == face[i]) triangle[i] = n.p;
  const double error =
      geometry.SurfaceDeviation(owned.physical[slot].facet, triangle, SU2NativeGeometry::SurfaceQueryNeed::Magnitude().ForMarker());
  const auto allowed = FaceTolerance(geometry, owned.physical[slot].facet, tolerance);
  if (surface_planning && wall_counters)
    wall_counters->ObserveMeasuredFace(face, owned.physical[slot].facet, error, tolerance);
  if (std::isfinite(error)) {
    const auto excess = hinge(static_cast<long double>(error) - allowed);
    d.geometry += excess * excess;
  } else
    d.geometry_measured = false;
  if (error <= allowed) return;
  unsigned type = 0;
  const Id facet = owned.physical[slot].facet;
  const auto analytic = geometry.Analytic().find(facet);
  if (analytic != geometry.Analytic().end())
    type = unsigned(analytic->second.kind);
  else if (geometry.Affine().count(facet))
    type = 5;
  else if (parameters && parameters->Geometry().Curves().count(facet))
    type = 6;
  row.geometry_types |= static_cast<unsigned char>(1u << type);
  unsigned bin = 5;
  if (std::isfinite(error) && allowed > 0) {
    const auto excess = static_cast<long double>(error) / allowed - 1;
    bin = excess < .01L ? 0 : excess < .1L ? 1 : excess < 1 ? 2 : excess < 10 ? 3 : 4;
  }
  row.geometry_excess_bin = std::max(row.geometry_excess_bin, static_cast<unsigned char>(bin));
  row.valid = false;
  row.geometry_invalid = true;
  WallFaceWeights law_weights;
  if (surface_planning && std::isfinite(error)) {
    std::array<WallEdgeEstimate, 3> estimates{};
    if (parameters) {
      size_t edge = 0;
      for (size_t i = 0; i < 3; ++i)
        for (size_t j = i + 1; j < 3; ++j)
          estimates[edge++] = EstimateWallEdge(*parameters, {face[i], triangle[i]}, {face[j], triangle[j]}, facet);
    }
    law_weights = WallFaceEdgeWeights(estimates);
    if (wall_counters) wall_counters->ObserveLawFace(law_weights.weighted);
  }
  AccumulateGeometryScores(face, triangle, error, allowed, law_weights, geometry_scores);
}

// Adds the face-weighted geometry excess to the same canonical edge keys.
void Selection::AccumulateGeometryScores(const Face& face, const std::array<Point, 3>& triangle, double error,
                                         double allowed, const WallFaceWeights& law_weights,
                                         std::map<Edge, long double>& geometry_scores) const {
  size_t face_edge = 0;
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = i + 1; j < 3; ++j, ++face_edge) {
      const auto a = triangle[i], b = triangle[j];
      const Edge key{std::min(face[i], face[j]), std::max(face[i], face[j])};
      if (!std::isfinite(error)) continue;
      const long double excess = static_cast<long double>(error) - allowed;
      const long double x = static_cast<long double>(b.x) - a.x, y = static_cast<long double>(b.y) - a.y,
                        z = static_cast<long double>(b.z) - a.z;
      // Queue current geometry demand; the selected complete star owns projection and progress validation.
      if (surface_planning)
        geometry_scores[key] += excess * excess * (x * x + y * y + z * z) * law_weights.factors[face_edge];
      else
        geometry_scores[key] += excess * excess * (x * x + y * y + z * z);
    }
}

// Updates the versioned edge census and queue in metric-edge order.
void Selection::AddCellEdges(const OwnedCell& owned, const PatchMetric& metric,
                             std::map<Edge, long double>& geometry_scores, CellState& row) {
  const auto& cell = owned.cell;
  size_t slot = 0;
  for (const auto& measured : metric.edges) {
    const auto key = measured.first;
    row.geometry_scores[slot] = geometry_scores[key];
    const auto blocked = queues.blocked_edges.find(key[1]);
    if (!coarsening && queues.construction_frontier.count(key) && row.geometry_scores[slot] > 0 &&
        (blocked == queues.blocked_edges.end() || !blocked->second.count(key[0])))
      queues.geometry_queue.insert({-GeometryPriority(row.geometry_scores[slot], key), cell.id, key});
    auto inserted = census.edges.emplace(key, EdgeState{measured.second, 0});
    auto& state = inserted.first->second;
    Queue(key, state.measure.length, false);
    if (!Same(state.measure.a, measured.second.a) || !Same(state.measure.b, measured.second.b) ||
        state.measure.length != measured.second.length)
      throw std::logic_error("Inconsistent native 3D frozen edge measurement.");
    row.forbidden[slot] = Candidate(measured.second.length) ? Forbidden(owned, key) : 0;
    for (unsigned direction = 0; direction < 2; ++direction)
      state.forbidden[direction] += bool(row.forbidden[slot] & (1u << direction));
    bool physical = false;
    for (unsigned face = 0; face < 4; ++face)
      if (owned.physical[face].present &&
          (!surface_planning || heights.count(geometry.Get(owned.physical[face].facet).marker)) &&
          cell.v[face].id != key[0] && cell.v[face].id != key[1])
        physical = true;
    state.physical_users += physical;
    row.physical[slot] = physical;
    ++state.users;
    row.edges[slot++] = key;
    Queue(key, state.measure.length, true);
  }
}
