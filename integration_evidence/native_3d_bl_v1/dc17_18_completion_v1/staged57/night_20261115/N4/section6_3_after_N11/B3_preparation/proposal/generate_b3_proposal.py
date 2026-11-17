from pathlib import Path
import hashlib
import json
import re
import subprocess

ROOT = Path.cwd()
PREP = ROOT / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/B3_preparation"
OUT = PREP / "proposal/generated"
OUT.mkdir(parents=True, exist_ok=True)
MAIN = ROOT / "Common/src/adaptation/CNativeEngine3D.cpp"
EXPECTED_MAIN = "7545edb4cea518fffc7e0e5cd4f763891fbe2a002b2d26b177b04b76e0d3b848"


def sha(data):
    return hashlib.sha256(data).hexdigest()


main = MAIN.read_bytes()
assert sha(main) == EXPECTED_MAIN, "Frozen build552 source changed; regenerate after final N14 disposition."
source = main.decode()
lex = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)


def mask(text):
    return lex.sub(lambda m: "".join("\n" if c == "\n" else " " for c in m[0]), text)


def close_brace(text, opening):
    masked = mask(text)
    depth = 0
    for i in range(opening, len(text)):
        if masked[i] == "{":
            depth += 1
        elif masked[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    raise AssertionError("unclosed block")


def adapt_span(text):
    start = text.index("EngineResult Adapt(")
    setup = text.index("  EngineSetup setup;", start)
    opening = text.rfind("{", start, setup)
    return opening, close_brace(text, opening)


def lambda_span(text, start, end, name):
    match = re.search(r"^  auto " + re.escape(name) + r"=\[&\].*?\{", text[start:end], re.M | re.S)
    assert match, name
    begin = start + match.start()
    opening = start + match.end() - 1
    finish = close_brace(text, opening)
    assert text[finish:finish + 3] == "};\n", name
    return {"name": name, "start": begin, "end": finish + 3, "old": text[begin:finish + 3],
            "body": text[opening + 1:finish]}


NAMES = {
    "scope_census_complete": "ScopeCensusComplete",
    "refresh_surface_census": "RefreshSurfaceCensus",
    "generation_census": "GenerationCensus",
    "generation_debt": "CurrentGenerationDebt",
    "measure": "Measure",
    "seed_next": "SeedNext",
}
start, end = adapt_span(source)
removed = [lambda_span(source, start, end, name) for name in NAMES]
without = source
for item in sorted(removed, key=lambda row: row["start"], reverse=True):
    without = without[:item["start"]] + without[item["end"]:]

# Keep original local argument lists. Route only call symbols; no replacement wrappers.
route_exact = {
    "scope_census_complete()": "context.census.ScopeCensusComplete(context)",
    "refresh_surface_census()": "context.census.RefreshSurfaceCensus(context)",
    "generation_census()": "context.census.GenerationCensus(context)",
    "generation_debt()": "context.census.CurrentGenerationDebt(context)",
}
calls = []
for old, new in route_exact.items():
    count = without.count(old)
    assert count == {"scope_census_complete()": 4, "refresh_surface_census()": 1,
                     "generation_census()": 3, "generation_debt()": 1}[old], (old, count)
    calls.append({"old": old, "new": new, "count": count})
    without = without.replace(old, new)
for old, new, count in (
    ("measure(", "context.census.Measure(context, ", 3),
    ("seed_next()", "context.census.SeedNext(context)", 2),
    ("seed_next(!choice.active)", "context.census.SeedNext(context, !choice.active)", 1),
):
    assert without.count(old) == count, (old, without.count(old))
    calls.append({"old": old, "new": new, "count": count})
    without = without.replace(old, new)

# The three long-lambda partitions preserve the original split order: wrapper, import retry, fill.
METHODS = r'''// Census workers borrow the live Adapt state; none stores a copied payload.
bool EngineCensus::ScopeCensusComplete(EngineContext& context) const {
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  if (!surface_target) return selection.Progress(working.size()).census_complete;
  if (!selection.progress_consistent || !selection.census_started) return false;
  const auto count = selection.SurfaceCensus(working);
  return count[0] == count[1];
}

void EngineCensus::RefreshSurfaceCensus(EngineContext& context) const {
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  if (!surface_target) return;
  const auto count = selection.SurfaceCensus(working);
  stats.surface_demand_required_cells = count[0];
  stats.surface_demand_measured_cells = selection.progress_consistent ? count[1] : 0;
  stats.surface_demand_census_complete = selection.progress_consistent && selection.census_started && count[0] == count[1];
  stats.surface_demand_geometry_complete = stats.surface_demand_census_complete && count[0] == count[2];
}

bool EngineCensus::GenerationCensus(EngineContext& context) {
  auto& selection = *context.census.selection;
  auto& surface_target = context.surface_target;
  auto& edge_progress = *context.census.edge_progress;
  auto& edge_census_round = *context.census.edge_census_round;
  auto& world = context.world;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& options = context.options;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  const auto extra = transfer_memory::Mul(selection.edges.size(), sizeof(EdgeProgressSample) +
                                          sizeof(Selection::History) + sizeof(Edge) + 4 * sizeof(void*));
  if (!context.exchange.Admit(context, extra)) return false;
  CLocalFailure failure;
  std::vector<EdgeProgressSample> samples;
  try {
    selection.RefreshHistory(false);
    samples.reserve(selection.edges.size());
    for (const auto& row : selection.edges) {
      if (surface_target && !row.second.physical_users) continue;
      const auto h = selection.edge_history.find(row.first);
      const auto length = row.second.measure.length;
      double value = double(length);
      if (value < length) value = std::nextafter(value, INFINITY);
      samples.push_back({row.first, value, row.second.physical_users != 0,
          h == selection.edge_history.end() ? 0 : h->second.age,
          h == selection.edge_history.end() ? 0 : h->second.disposition,
          h == selection.edge_history.end() ? 0 : h->second.round});
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  if (!MeasureGenerationProgress(world, samples, edge_progress, result.reason, options.memory_ceiling,
      transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes(),
                           transfer_memory::Bytes(edge_progress.top)))) return false;
  stats.unique_long_surface = edge_progress.long_surface;
  stats.unique_long_interior = edge_progress.long_interior;
  stats.unique_edge_debt = edge_progress.hinge_squared;
  stats.maximum_edge = edge_progress.max_length;
  stats.edge_histogram = edge_progress.histogram;
  edge_census_round = stats.rounds;
  return true;
}

GenerationDebt EngineCensus::CurrentGenerationDebt(EngineContext& context) const {
  auto& edge_progress = *context.census.edge_progress;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& surface_target = context.surface_target;
  auto& world = context.world;
  GenerationDebt d;
  d.edges = edge_progress.hinge_squared;
  d.maximum = edge_progress.max_length;
  const auto p = selection.Progress(working.size());
  d.geometry = TimedAllreduce(double(p.geometry_excess_squared), CPassiveComm::Op::SUM, world.comm);
  bool geometry_complete = p.geometry_complete;
  if (surface_target) {
    const auto count = selection.SurfaceCensus(working);
    geometry_complete = selection.progress_consistent && selection.census_started && count[0] == count[1] && count[0] == count[2];
  }
  d.geometry_complete = !world.maximum(!geometry_complete);
  return d;
}

bool EngineCensus::ImportMeasurementBatch(EngineContext& context, const std::vector<Id>& ids, size_t slot,
                                           size_t& batch, size_t plan_scratch, double* enclosing_seconds,
                                           CLocalFailure& failure, std::map<Edge, EdgeMeasure>& reuse,
                                           DonorPatch& imported) {
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& retired_edges = *context.census.retired_edges;
  auto& field_identity = *context.census.field_identity;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& options = context.options;
  for (;;) {
    std::vector<Cell> regions;
    try {
      reuse.clear();
      for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) {
        const auto& cell = working.at(ids[i]).cell;
        regions.push_back(cell);
        selection.Cached(cell, reuse);
        for (auto a : cell.v) for (auto b : cell.v) if (a.id < b.id) {
          const auto prior = retired_edges.find({a.id, b.id});
          if (prior != retired_edges.end()) reuse.emplace(prior->first, prior->second);
        }
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
    std::vector<unsigned char> requests;
    try {
      requests = MetricEdgeRequests(regions, reuse, field_identity, &campaign_edges);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
    {
      Timer timer(stats.donor_seconds, options.timings, enclosing_seconds, SU2NativeTiming3D::Operation::Import);
      imported = context.exchange.ImportSource(context, regions,
          transfer_memory::Add(MaximumFrozenFieldBytes, plan_scratch), false, &requests);
    }
    if (imported.valid) return true;
    if (batch == 1) {
      result.reason = imported.reason;
      return false;
    }
    batch = (batch + 1) / 2;
    ++stats.batch_retries;
  }
}

bool EngineCensus::MeasureBatchCells(EngineContext& context, const std::vector<Id>& ids, size_t slot,
                                     size_t batch, bool seed, size_t plan_scratch, CLocalFailure& failure,
                                     DonorPatch& imported,
                                     std::map<Edge, EdgeMeasure>& reuse) {
  using Phase = EnginePhase;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& composition = context.composition;
  auto& field_identity = *context.census.field_identity;
  auto& phase = *context.setup.control_prefix.phase;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  if (!context.exchange.Admit(context, transfer_memory::Add(transfer_memory::Bytes(imported.cells),
      MaximumFrozenFieldBytes, plan_scratch,
      transfer_memory::Mul(working.size(), context.control.SelectionBound(context, selection.track_repairs))))) return false;
  if (slot < ids.size()) try {
    CpuTimer private_cpu(stats.measurement_cpu_seconds, options.timings);
    const auto components = AssociatedExtensions(imported.cells, geometry);
    FrozenField field(std::move(imported.cells), composition, options.timings, components, options.edge_breakpoints,
                      field_identity, true, MaximumDonors, true);
    FieldAccounting accounting{stats, field};
    PatchMetric planned;
    if (surface_target && phase == Phase::Construction) {
      Imported published;
      published.cells.reserve(batch);
      for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) published.cells.push_back(working.at(ids[i]));
      MetricStats counted;
      // Published coordinates/associations and the field are unchanged throughout this measurement batch.
      planned = MeasureSurfacePlanning(GeometryPatch(published, &geometry), field, geometry, options.wall_heights,
                                       surface_target, &counted);
      stats.edge_evaluations += counted.edge_evaluations;
      stats.edge_reused += counted.edge_reused;
    }
    for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) {
      PatchMetric metric;
      std::string reason;
      MetricStats counted;
      const auto& cell = working.at(ids[i]).cell;
      bool strict = false, quality_valid = false, height_valid = true;
      FirstHeightReport height;
      if (phase == Phase::Construction) {
        if (surface_target) {
          const auto& owned = working.at(ids[i]);
          for (unsigned a = 0; a < 4; ++a) for (unsigned b = a + 1; b < 4; ++b) {
            Edge key{cell.v[a].id, cell.v[b].id};
            std::sort(key.begin(), key.end());
            metric.edges.emplace(key, planned.edges.at(key));
          }
          for (const auto& edge : metric.edges) for (unsigned face = 0; face < 4; ++face)
            if (cell.v[face].id != edge.first[0] && cell.v[face].id != edge.first[1] && owned.physical[face].present &&
                options.wall_heights.count(geometry.Get(owned.physical[face].facet).marker) &&
                edge.second.length > metric.maximum_length) {
              metric.maximum_length = edge.second.length;
              metric.longest_edge = edge.first;
            }
        } else metric = MeasureMetricEdges({cell}, field, &counted, &reuse, &campaign_edges);
        strict = metric.maximum_length <= MaximumMetricEdge;
      } else {
        const bool metric_valid = ValidateMetricPatch({cell}, field, metric, reason, &counted, &reuse);
        std::string height_reason;
        height_valid = EngineTrial::HeightCheck(context,
            GeometryPatch(Imported{{working.at(ids[i])}, true, {}}, &geometry), height_reason, false, &height);
        if (!height_valid) reason = height_reason;
        quality_valid = metric.shape_measured && std::isfinite(metric.minimum_mean_ratio) &&
            std::isfinite(metric.minimum_scaled_jacobian) && metric.minimum_mean_ratio >= MinimumMeanRatio &&
            metric.minimum_scaled_jacobian >= MinimumScaledJacobian && height_valid;
        strict = metric_valid && height_valid;
      }
      if (phase == Phase::Improvement && selection.cells.count(ids[i]))
        selection.RefreshQuality(working.at(ids[i]), metric, strict, quality_valid,
            !options.wall_heights.empty() ? &height : nullptr, height_valid, field_identity);
      else
        selection.Add(working.at(ids[i]), metric, strict, quality_valid, reason,
            phase == Phase::Improvement && !options.wall_heights.empty() ? &height : nullptr, height_valid);
      for (const auto& edge : metric.edges) reuse.insert_or_assign(edge.first, edge.second);
      stats.edge_evaluations += counted.edge_evaluations;
      stats.edge_reused += counted.edge_reused;
      if (seed) ++stats.initial_cells;
      else ++stats.updated_cells;
      const auto& owned = working.at(ids[i]);
      const unsigned category = selection.RequestedWall(owned) ? 2 : selection.cells.at(ids[i]).surface ? 1 : 0;
      ++(seed ? stats.seed_cell_classes : stats.updated_cell_classes)[category];
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  return context.exchange.Vote(context, failure);
}

bool EngineCensus::Measure(EngineContext& context, const std::vector<Id>& requested_ids, bool seed,
                           double* enclosing_seconds) {
  auto& options = context.options;
  auto& world = context.world;
  const auto& started = *context.setup.control_prefix.started;
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  constexpr size_t MeasurementBatch = 16;
  std::optional<SU2NativeTiming3D::Scope> census_scope;
  if (options.timings) census_scope.emplace(SU2NativeTiming3D::Operation::Census);
  std::vector<Id> demand;
  const auto demand_bytes = surface_target ? transfer_memory::GrowthBound(requested_ids.size(), sizeof(Id)) : 0;
  const auto plan_scratch = transfer_memory::Add(demand_bytes,
      transfer_memory::Mul(3 * MaximumCampaignEdges, sizeof(std::pair<const Edge, EdgeMeasure>) + 6 * sizeof(void*)),
      surface_target ? transfer_memory::Add(SurfacePlanningScratchBytes(MeasurementBatch),
          transfer_memory::Mul(MeasurementBatch, sizeof(OwnedCell))) : 0);
  if (!context.exchange.Admit(context, transfer_memory::Add(plan_scratch,
      transfer_memory::Mul(MeasurementBatch, sizeof(Cell) + sizeof(unsigned char))))) return false;
  if (surface_target) {
    CLocalFailure failure;
    try {
      demand.reserve(requested_ids.size());
      for (Id id : requested_ids) if (selection.RequestedWall(working.at(id))) demand.push_back(id);
      stats.surface_measurement_skipped += requested_ids.size() - demand.size();
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
  }
  const auto& ids = surface_target ? demand : requested_ids;
  selection.census_started = true;
  const size_t count = TimedAllreduce(ids.size(), CPassiveComm::Op::MAX, world.comm);
  for (size_t slot = 0; slot < count;) {
    if (options.maximum_seconds && world.maximum(
        std::chrono::duration<double>(Clock::now() - started).count() >=
        options.maximum_seconds)) {
      stats.stop = EngineStop::TimeBudget;
      result.reason = seed ? "Native3D metric seeding exhausts elapsed-time budget."
                           : "Native3D metric update exhausts elapsed-time budget.";
      return false;
    }
    size_t batch = std::min({MeasurementBatch, MaximumCavityCells / 6, count - slot});
    DonorPatch imported;
    std::map<Edge, EdgeMeasure> reuse;
    CLocalFailure failure;
    if (!ImportMeasurementBatch(context, ids, slot, batch, plan_scratch, enclosing_seconds, failure, reuse, imported))
      return false;
    if (!MeasureBatchCells(context, ids, slot, batch, seed, plan_scratch, failure, imported, reuse)) return false;
    slot += batch;
    if (seed) ++stats.seed_batches;
  }
  return true;
}

bool EngineCensus::SeedNext(EngineContext& context, bool local_active) {
  using Phase = EnginePhase;
  auto& options = context.options;
  auto& world = context.world;
  auto& composition = context.composition;
  auto& surface_target = context.surface_target;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& phase = *context.setup.control_prefix.phase;
  auto& initial = *context.census.initial;
  auto& seed_slot = *context.census.seed_slot;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  const auto& started = *context.setup.control_prefix.started;
  constexpr size_t MeasurementBatch = 16;
  Timer timer(stats.seed_seconds, options.timings, SU2NativeTiming3D::Operation::Census);
  // Composed or sided planning is expensive; a complete cavity may remove still-unmeasured topology.
  if ((!composition && !surface_target) || phase == Phase::Improvement) {
    const bool measured = Measure(context, initial, true, &stats.seed_seconds);
    if (measured) seed_slot = initial.size();
    return measured;
  }
  if (options.maximum_seconds && world.maximum(
      std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)) {
    stats.stop = EngineStop::TimeBudget;
    result.reason = "Native3D metric seeding exhausts elapsed-time budget.";
    return false;
  }
  if (!context.exchange.Admit(context, transfer_memory::Mul(MeasurementBatch, sizeof(Id)))) return false;
  CLocalFailure failure;
  std::vector<Id> next;
  try {
    size_t scanned = 0;
    next.reserve(MeasurementBatch);
    while (local_active && seed_slot < initial.size() && next.size() < MeasurementBatch &&
           scanned++ < MaximumDirectoryScan) {
      const auto id = initial[seed_slot++];
      if (working.count(id) && !selection.cells.count(id)) next.push_back(id);
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  return context.exchange.Vote(context, failure) && Measure(context, next, true, &stats.seed_seconds);
}
'''

# Keep field construction, planning, timers, catch, and final vote in the batch method.
# Move only its pre-existing per-cell loop body to a leaf worker.
cell_loop = "    for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) {\n      PatchMetric metric;"
assert METHODS.count(cell_loop) == 1
loop_start = METHODS.index(cell_loop)
loop_open = METHODS.index("{", loop_start)
loop_end = close_brace(METHODS, loop_open)
original_cell_loop = METHODS[loop_start:loop_end + 1]
batch_start = METHODS.index("bool EngineCensus::MeasureBatchCells(")
batch_open = METHODS.index("{", batch_start)
original_batch = METHODS[batch_start:close_brace(METHODS, batch_open) + 1]
original_cell_body = METHODS[loop_open + 1:loop_end]
cell_body = original_cell_body.replace("ids[i]", "id")
assert "ids[i]" not in cell_body
replacement = "    for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) {\n      MeasureCell(context, ids[i], seed, field, planned, reuse);\n    }"
METHODS = METHODS[:loop_start] + replacement + METHODS[loop_end + 1:]
cell_method = r'''void EngineCensus::MeasureCell(EngineContext& context, Id id, bool seed, FrozenField& field,
                               PatchMetric& planned, std::map<Edge, EdgeMeasure>& reuse) {
  using Phase = EnginePhase;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& field_identity = *context.census.field_identity;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& phase = *context.setup.control_prefix.phase;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
''' + cell_body + r'''
}

'''
METHODS = METHODS.replace("bool EngineCensus::MeasureBatchCells(", cell_method + "bool EngineCensus::MeasureBatchCells(", 1)

capture_audit = {
    "ScopeCensusComplete": ["surface_target", "selection", "working"],
    "RefreshSurfaceCensus": ["surface_target", "selection", "working", "stats"],
    "GenerationCensus": ["selection", "surface_target", "edge_progress", "edge_census_round", "world", "result",
                         "stats", "options", "source", "directory"],
    "CurrentGenerationDebt": ["edge_progress", "selection", "working", "surface_target", "world"],
    "Measure": ["options", "world", "started", "surface_target", "selection", "working", "stats", "result"],
    "SeedNext": ["options", "world", "composition", "stats", "result", "phase", "initial", "seed_slot",
                 "working", "selection", "started"],
    "ImportMeasurementBatch": ["working", "selection", "retired_edges", "field_identity", "campaign_edges",
                                "stats", "result", "options"],
    "MeasureBatchCells": ["selection", "working", "geometry", "surface_target", "composition", "field_identity", "phase", "options", "stats"],
    "MeasureCell": ["selection", "working", "geometry", "surface_target", "field_identity", "campaign_edges",
                    "phase", "options", "stats"],
}

header_path = ROOT / "Common/src/adaptation/native_engine3d/EngineCensus.hpp"
header_bytes = header_path.read_bytes()
base_census_path = ROOT / "Common/src/adaptation/native_engine3d/EngineCensus.cpp"
base_census_bytes = base_census_path.read_bytes()
assert sha(header_bytes) == "bfe3181885c22e13bc1104c405e3adbf383bced1fff5861baed92a04ce02d739"
assert sha(base_census_bytes) == "314fbe6483953704d96d0ad5582e9bbcd507d740c65295b8a2f49d676979a326"
header = header_bytes.decode()
header = header.replace(header.splitlines()[0], "// Owns Census slots and measures live Context state; Adapt retains activation and payload lifetimes.", 1)
decls = '''
  bool ScopeCensusComplete(EngineContext&) const;
  void RefreshSurfaceCensus(EngineContext&) const;
  bool GenerationCensus(EngineContext&);
  GenerationDebt CurrentGenerationDebt(EngineContext&) const;
  bool Measure(EngineContext&, const std::vector<Id>&, bool seed, double* enclosing_seconds);
  bool ImportMeasurementBatch(EngineContext&, const std::vector<Id>& ids, size_t slot, size_t& batch,
                              size_t plan_scratch, double* enclosing_seconds, CLocalFailure&,
                              std::map<Edge, EdgeMeasure>& reuse, DonorPatch& imported);
  bool MeasureBatchCells(EngineContext&, const std::vector<Id>& ids, size_t slot, size_t batch, bool seed,
                         size_t plan_scratch, CLocalFailure&, DonorPatch& imported,
                         std::map<Edge, EdgeMeasure>& reuse);
  void MeasureCell(EngineContext&, Id id, bool seed, FrozenField& field, PatchMetric& planned,
                   std::map<Edge, EdgeMeasure>& reuse);
  bool SeedNext(EngineContext&, bool local_active = true);
'''
assert "struct EngineCensus {" in header and "struct EngineContext;" not in header
header = header.replace("struct EngineCensus {", "struct EngineContext;\n\nstruct EngineCensus {", 1)
header = header.replace("  EngineCensus();", decls + "\n  EngineCensus();", 1)
cpp = METHODS
formatted = subprocess.run(["clang-format-18"], input=cpp, text=True, capture_output=True, check=True).stdout
base_census = base_census_bytes.decode()
base_census = base_census.replace(base_census.splitlines()[0], "// Measures the canonical Selection; owns no extra state and preserves the original protocol.", 1)
census_cpp = base_census.replace('#include "EngineCensus.hpp"',
    '#include "EngineCensus.hpp"\n#include "EngineContext.hpp"\n#include "EngineTrial.hpp"', 1) + "\n" + formatted

header = subprocess.run(["clang-format-18"], input=header, text=True, capture_output=True, check=True).stdout.rstrip() + "\n"
census_cpp = census_cpp.rstrip() + "\n"
OUT.joinpath("EngineCensus.hpp.proposed").write_text(header)
OUT.joinpath("EngineCensus.cpp.proposed").write_text(census_cpp)
OUT.joinpath("CNativeEngine3D.cpp.proposed").write_text(without)

method_names = [NAMES[k] for k in NAMES] + ["ImportMeasurementBatch", "MeasureBatchCells", "MeasureCell"]
masked_cpp = mask(census_cpp)
call_loop = "for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) {\n        MeasureCell(context, ids[i], seed, field, planned, reuse);\n      }"
methods = []
for name in method_names:
    pattern = re.compile(r"(?:bool|void|GenerationDebt) EngineCensus::" + name + r"\s*\([^;]*?\)\s*(?:const\s*)?\{", re.S)
    match = pattern.search(masked_cpp)
    assert match, name
    opening = match.end() - 1
    closing = close_brace(census_cpp, opening)
    lines = census_cpp[match.start():closing + 1].count("\n") + 1
    assert lines <= 80, (name, lines)
    methods.append({"name": name, "formatted_lines": lines,
                    "sha256": sha(census_cpp[match.start():closing + 1].encode())})

def cpp_tokens(text):
    text = re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)
    return re.findall(r'"(?:\\.|[^"\\])*"|\w+|[^\s]', text)


def member_body(text, masked_text, name):
    match = re.search(r"(?:bool|void|GenerationDebt) EngineCensus::" + name + r"\s*\([^;]*?\)\s*(?:const\s*)?\{",
                      masked_text, re.S)
    assert match, name
    opening = match.end() - 1
    return text[opening + 1:close_brace(text, opening)]


alias_audit = {}
local_type_alias_audit = {}
method_params = {
    "ScopeCensusComplete": ["context"], "RefreshSurfaceCensus": ["context"],
    "GenerationCensus": ["context"], "CurrentGenerationDebt": ["context"],
    "Measure": ["context", "requested_ids", "seed", "enclosing_seconds"],
    "SeedNext": ["context", "local_active"],
    "ImportMeasurementBatch": ["context", "ids", "slot", "batch", "plan_scratch", "enclosing_seconds", "failure", "reuse", "imported"],
    "MeasureBatchCells": ["context", "ids", "slot", "batch", "seed", "plan_scratch", "failure", "imported", "reuse"],
    "MeasureCell": ["context", "id", "seed", "field", "planned", "reuse"],
}
outer_capture_names = set().union(*(set(names) for names in capture_audit.values()))
for method in method_names:
    body = member_body(census_cpp, masked_cpp, method)
    lines = body.splitlines(keepends=True)
    prefix = []
    type_aliases = []
    remainder = []
    aliases_open = True
    for line in lines:
        alias = re.match(r"\s*(?:const\s+)?auto\s*&\s*(\w+)\s*=.*;\s*$", line)
        type_alias = re.match(r"\s*using\s+(\w+)\s*=\s*([\w:]+)\s*;\s*$", line)
        if aliases_open and (not line.strip() or alias or type_alias):
            if alias:
                prefix.append((alias.group(1), line))
            elif type_alias:
                type_aliases.append((type_alias.group(1), type_alias.group(2)))
            continue
        aliases_open = False
        remainder.append(line)
    names = [name for name, _ in prefix]
    assert len(names) == len(set(names)), f"duplicate borrowed alias in {method}: {names}"
    rest = "".join(remainder)
    unused = [name for name in names if len(re.findall(r"\b" + re.escape(name) + r"\b", rest)) == 0]
    params = set(method_params[method])
    free_reference_text = re.sub(r"(?:\.|->)\s*\w+", " .", rest)
    referenced = set(cpp_tokens(free_reference_text)) & outer_capture_names
    free = sorted(referenced - set(names) - params)
    assert not free, f"unbound Adapt capture(s) in {method}: {free}"
    alias_audit[method] = {"aliases": names, "bindings_and_types": [line.strip() for _, line in prefix],
                           "parameters": method_params[method], "unused_aliases": unused,
                           "free_capture_names": free}
    local_type_alias_audit[method] = type_aliases


def without_context_aliases(body):
    lines = body.splitlines(keepends=True)
    prefix = []
    remainder = []
    aliases = True
    for line in lines:
        if aliases and (not line.strip() or
                        re.match(r"\s*(?:const\s+)?auto\s*&\s*\w+\s*=.*;\s*$", line) or
                        re.match(r"\s*using\s+\w+\s*=\s*[\w:]+\s*;\s*$", line)):
            if line.strip():
                prefix.append(line)
            continue
        aliases = False
        remainder.append(line)
    return "".join(remainder)


def restore_local_calls(body):
    body = body.replace("Measure(context, ", "measure(")
    body = re.sub(r"(?m)^\s*constexpr\s+size_t\s+MeasurementBatch\s*=\s*16\s*;\s*", "", body)
    return body


old_lambda = {row["name"]: row["body"] for row in removed}
actual_body_proofs = []
for name, member in (("scope_census_complete", "ScopeCensusComplete"),
                     ("refresh_surface_census", "RefreshSurfaceCensus"),
                     ("generation_census", "GenerationCensus"),
                     ("generation_debt", "CurrentGenerationDebt"),
                     ("seed_next", "SeedNext")):
    normalized = restore_local_calls(without_context_aliases(member_body(census_cpp, masked_cpp, member)))
    expected = old_lambda[name]
    if cpp_tokens(normalized) != cpp_tokens(expected):
        actual, wanted = cpp_tokens(normalized), cpp_tokens(expected)
        at = next((i for i, pair in enumerate(zip(actual, wanted)) if pair[0] != pair[1]), min(len(actual), len(wanted)))
        raise AssertionError(("actual old lambda body mismatch", name, at, actual[at:at + 15], wanted[at:at + 15],
                              len(actual), len(wanted)))
    actual_body_proofs.append({"old_lambda": name, "member": member, "actual_lambda_token_equal": True})

measure_body = without_context_aliases(member_body(census_cpp, masked_cpp, "Measure"))
measure_body = re.sub(r"(?m)^\s*constexpr\s+size_t\s+MeasurementBatch\s*=\s*16\s*;\s*", "", measure_body)
import_body = without_context_aliases(member_body(census_cpp, masked_cpp, "ImportMeasurementBatch"))
assert import_body.count("if (imported.valid) return true;") == 1
import_inline = import_body.replace("if (imported.valid) return true;", "if (imported.valid) break;", 1)
measure_body, import_count = re.subn(
    r"if\s*\(\s*!ImportMeasurementBatch\(context,\s*ids,\s*slot,\s*batch,\s*plan_scratch,\s*enclosing_seconds,\s*failure,\s*reuse,\s*imported\)\s*\)\s*return false;",
    lambda _: import_inline, measure_body)
assert import_count == 1, "Measure must call its import helper once per original batch"
batch_body = without_context_aliases(member_body(census_cpp, masked_cpp, "MeasureBatchCells"))
assert batch_body.count("return context.exchange.Vote(context, failure);") == 1
batch_body = batch_body.replace("return context.exchange.Vote(context, failure);",
                                "if (!context.exchange.Vote(context, failure)) return false;", 1)
batch_body = batch_body.replace(call_loop, original_cell_loop.strip(), 1)
measure_body, batch_count = re.subn(
    r"if\s*\(!MeasureBatchCells\(context,\s*ids,\s*slot,\s*batch,\s*seed,\s*plan_scratch,\s*failure,\s*imported,\s*reuse\)\)\s*return false;",
    lambda _: batch_body, measure_body)
assert batch_count == 1, "Measure must call its fill helper once per original batch"
if cpp_tokens(measure_body) != cpp_tokens(old_lambda["measure"]):
    actual, wanted = cpp_tokens(measure_body), cpp_tokens(old_lambda["measure"])
    at = next((i for i, pair in enumerate(zip(actual, wanted)) if pair[0] != pair[1]), min(len(actual), len(wanted)))
    raise AssertionError(("inlined measurement helper tokens differ", at, actual[at:at + 15], wanted[at:at + 15],
                          len(actual), len(wanted)))
actual_body_proofs.append({"old_lambda": "measure", "member": "Measure with ImportMeasurementBatch and MeasureBatchCells inlined",
                           "actual_lambda_token_equal": True,
                           "import_success_return_true_inlined_as_break": True,
                           "batch_final_vote_return_inlined_as_original_false_branch": True})

batch_match = re.search(r"bool EngineCensus::MeasureBatchCells\s*\([^;]*?\)\s*\{", masked_cpp, re.S)
batch_open = batch_match.end() - 1
batch_close = close_brace(census_cpp, batch_open)
formatted_batch = census_cpp[batch_match.start():batch_close + 1]
assert formatted_batch.count(call_loop) == 1, "formatted MeasureCell loop binding changed"
inlined_batch = formatted_batch.replace(call_loop, original_cell_loop.strip(), 1)
assert cpp_tokens(inlined_batch) == cpp_tokens(original_batch), "inlining MeasureCell must recover batch tokens"
cell_match = re.search(r"void EngineCensus::MeasureCell\s*\([^;]*?\)\s*\{", masked_cpp, re.S)
cell_open = cell_match.end() - 1
cell_close = close_brace(census_cpp, cell_open)
formatted_cell_body = census_cpp[cell_open + 1:cell_close]
formatted_cell_body = formatted_cell_body[formatted_cell_body.index("PatchMetric metric;"):]
if cpp_tokens(formatted_cell_body) != cpp_tokens(cell_body):
    actual, expected = cpp_tokens(formatted_cell_body), cpp_tokens(cell_body)
    mismatch = next((i for i, pair in enumerate(zip(actual, expected)) if pair[0] != pair[1]), min(len(actual), len(expected)))
    raise AssertionError(("MeasureCell extraction tokens changed", mismatch, actual[mismatch:mismatch + 12],
                          expected[mismatch:mismatch + 12], len(actual), len(expected)))

# Byte reversal for the main translation unit: reverse routed calls, then restore exact lambda spans.
reversed_main = without
for call in reversed(calls):
    reversed_main = reversed_main.replace(call["new"], call["old"])
for item in sorted(removed, key=lambda row: row["start"]):
    position = item["start"]
    reversed_main = reversed_main[:position] + item["old"] + reversed_main[position:]
if reversed_main != source:
    mismatch = next(i for i, pair in enumerate(zip(reversed_main, source)) if pair[0] != pair[1])
    raise AssertionError(("Adapt call/lambda reversal mismatch", mismatch,
                          repr(reversed_main[mismatch - 80:mismatch + 120]),
                          repr(source[mismatch - 80:mismatch + 120])))

files = {
    "CNativeEngine3D.cpp": without,
    "EngineCensus.hpp": header,
    "EngineCensus.cpp": census_cpp,
}
manifest = {
    "unapplied": True,
    "base_build": "552; regenerate against post-N14-disposition source before applying",
    "base_sha256": {"CNativeEngine3D.cpp": EXPECTED_MAIN,
                    "EngineCensus.hpp": sha(header_bytes),
                    "EngineCensus.cpp": sha(base_census_bytes)},
    "proposed_sha256": {name: sha(text.encode()) for name, text in files.items()},
    "main_cpp_byte_reversal_after_call_routing": True,
    "removed_lambda_names": list(NAMES),
    "call_routing": calls,
    "formatted_member_methods": methods,
    "method_capture_binding_audit": {
        "ScopeCensusComplete": {"aliases": capture_audit["ScopeCensusComplete"], "parameters": ["context"]},
        "RefreshSurfaceCensus": {"aliases": capture_audit["RefreshSurfaceCensus"], "parameters": ["context"]},
        "GenerationCensus": {"aliases": capture_audit["GenerationCensus"], "parameters": ["context"]},
        "CurrentGenerationDebt": {"aliases": capture_audit["CurrentGenerationDebt"], "parameters": ["context"]},
        "Measure": {"aliases": capture_audit["Measure"], "parameters": ["context", "requested_ids", "seed", "enclosing_seconds"]},
        "SeedNext": {"aliases": capture_audit["SeedNext"], "parameters": ["context", "local_active"]},
        "ImportMeasurementBatch": {"aliases": capture_audit["ImportMeasurementBatch"],
                                   "parameters": ["context", "ids", "slot", "batch", "plan_scratch", "enclosing_seconds", "failure", "reuse", "imported"]},
        "MeasureBatchCells": {"aliases": capture_audit["MeasureBatchCells"],
                              "parameters": ["context", "ids", "slot", "batch", "seed", "plan_scratch", "failure", "imported", "reuse"]},
        "MeasureCell": {"aliases": capture_audit["MeasureCell"],
                        "parameters": ["context", "id", "seed", "field", "planned", "reuse"]},
        "reference_constness": "started remains const auto& in Measure and SeedNext; other aliases are mutable auto& matching Adapt's reference captures",
    },
    "borrowed_alias_duplicate_unused_audit": alias_audit,
    "method_local_type_alias_audit": local_type_alias_audit,
    "actual_removed_lambda_body_token_proofs": actual_body_proofs,
    "body_token_proofs": {
        "MeasureBatchCells_with_MeasureCell_inlined_equals_pre_split_batch": True,
        "MeasureCell_body_equals_original_per_cell_body_after_ids_i_to_id_parameter_binding": True,
        "Measure_with_both_helpers_inlined_equals_actual_removed_measure_lambda": True,
        "comparison": "C++ lexical tokens after formatting; comments/whitespace excluded; actual lambda bodies were extracted from the pinned main source"
    },
    "measure_split_map": {
        "wrapper": "admission, surface demand, census start/count, batch deadline/advance",
        "ImportMeasurementBatch": "region/cache collection, votes, requests, import timer and retry",
        "MeasureBatchCells": "post-import admission, field/planning/per-cell work, catch/final vote",
        "failure_lifetime": "one CLocalFailure per original batch, passed by reference across both helpers",
        "temporaries": "regions, requests and demand remain at their old scopes; imported/reuse/batch/slot stay local to Measure"
    },
    "reversal_manifest": [
        "restore the six removed Adapt lambdas and original call spellings; main source then byte-equals base",
        "remove only the B3 declarations and method definitions from EngineCensus files",
        "do not revert any other B2/N14/N15 or phase-B source changes"
    ],
    "no_live_file_written": True,
    "compiled_or_run": False,
}
OUT.joinpath("binding_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
