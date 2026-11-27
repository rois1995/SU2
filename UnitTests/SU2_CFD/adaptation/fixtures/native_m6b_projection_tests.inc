struct M6bSignCounts {
  uint64_t positive = 0, zero = 0, negative = 0;
  void Add(long double value) {
    if (value > 0)
      ++positive;
    else if (value < 0)
      ++negative;
    else
      ++zero;
  }
};

struct M6bShellCounts {
  M6bSignCounts wing, other;
  uint64_t wing_faces = 0, other_faces = 0;
};

using M6bClock = std::chrono::steady_clock;

std::map<Edge, std::vector<Id>> M6bIndexCells(const Owned& cells, const D9LocalityIndex& index,
                                              size_t& incidence_count) {
  std::map<Edge, std::vector<Id>> result;
  incidence_count = 0;
  for (const auto& row : cells) {
    const auto& v = row.second.cell.v;
    for (unsigned a = 0; a < 4; ++a) {
      for (unsigned b = a + 1; b < 4; ++b) {
        const auto edge = D9SortedEdge(v[a].id, v[b].id);
        if (!index.wall_edges.count(edge)) continue;
        result[edge].push_back(row.first);
        ++incidence_count;
      }
    }
  }
  return result;
}

Owned M6bEdgeCells(const Owned& cells, const std::vector<Id>& ids) {
  Owned subset;
  for (Id id : ids) subset.emplace(id, cells.at(id));
  return subset;
}

bool M6bHasEdge(const Face& face, Edge edge) {
  return std::find(face.begin(), face.end(), edge[0]) != face.end() &&
         std::find(face.begin(), face.end(), edge[1]) != face.end();
}

Point M6bCavityPoint(const Patch& patch, Id id) {
  for (const auto& cell : patch.cells)
    for (const auto& node : cell.v)
      if (node.id == id) return node.p;
  throw std::invalid_argument("Physical shell face has a vertex outside its edge cavity.");
}

M6bShellCounts M6bMeasureShell(const Patch& old, const ReferenceState& reference, int wing_marker, Point projected) {
  M6bShellCounts result;
  for (const auto& face : old.surface) {
    const auto& facet = reference.original->Get(face.facet);
    auto& marker_counts = facet.marker == wing_marker ? result.wing : result.other;
    if (facet.marker == wing_marker)
      ++result.wing_faces;
    else
      ++result.other_faces;
    marker_counts.Add(Orientation(M6bCavityPoint(old, face.v[0]), M6bCavityPoint(old, face.v[1]),
                                  M6bCavityPoint(old, face.v[2]), projected));
  }
  return result;
}

std::set<Id> M6bIncidentFacets(const Patch& old, Edge edge) {
  std::set<Id> result;
  for (const auto& face : old.surface) {
    if (M6bHasEdge(face.v, edge)) result.insert(face.facet);
  }
  return result;
}

Node M6bProjectedMidpoint(const PlanarReference& accepted, const Patch& old, Edge edge, Id id,
                          SU2NativeGeometry::AtlasBudget& budget) {
  const auto incident = M6bIncidentFacets(old, edge);
  if (incident.empty()) throw std::invalid_argument("Wall edge has no incident physical projection facet.");
  auto reserved = accepted.WithAtlasReservation(budget);
  if (!reserved) throw std::invalid_argument("Accepted reference has no private projection authority.");
  auto scoped = reserved->PrivateParameters(old, &budget);
  if (!scoped) throw std::invalid_argument("Projection overlay was not created.");
  return scoped->BindNewPoint(D9Midpoint(old, edge, id), incident, &edge);
}

void M6bCsvText(std::ostream& out, const std::string& value) {
  out << '"';
  for (char ch : value) {
    if (ch == '"') out << '"';
    if (ch == '\n')
      out << "\\n";
    else if (ch != '\r')
      out << ch;
  }
  out << '"';
}

void M6bWriteCsvRow(std::ostream& out, Edge edge, const M6bShellCounts& shell, bool shell_measured, bool projected,
                    double seconds, const std::string& reason, size_t work, size_t peak_bytes) {
  out << edge[0] << ',' << edge[1] << ',' << shell_measured << ",true,";
  if (shell_measured)
    out << shell.wing_faces << ',' << shell.wing.positive << ',' << shell.wing.zero << ',' << shell.wing.negative << ','
        << shell.other_faces << ',' << shell.other.positive << ',' << shell.other.zero << ',' << shell.other.negative;
  else
    out << ",,,,,,,";
  out << ',' << projected << ',' << std::setprecision(17) << seconds << ',' << work << ',' << peak_bytes << ',';
  M6bCsvText(out, reason);
  out << '\n';
}

struct M6bRun {
  size_t attempted = 0, succeeded = 0, refused = 0, shell_faces = 0, unvisited = 0;
  M6bShellCounts totals;
  double projection_seconds = 0;
  size_t atlas_work = 0, atlas_peak = 0;
};

struct M6bInput {
  ReferenceState reference;
  ImportedGeometry imported;
  D9LocalityIndex locality;
  std::map<Edge, std::vector<Id>> edge_cells;
  size_t model_ceiling = size_t(12) << 30, reference_peak = 0, edge_index_bytes = 0;
  size_t edge_incidence_count = 0, baseline = 0, import_caller = 0;
  Id maximum_node = 0;
  int wing_marker = -1;
  double import_seconds = 0;
};

M6bInput M6bLoadAcceptedEuler(const char* mesh_path, CConfig& config, MeshSolution& mesh) {
  M6bInput input;
  input.import_caller = D9ImportCallerBytes(mesh.Fine());
  bool loaded = false;
  std::string reason;
  if (!LoadReference(std::string(mesh_path) + ".native_ref", input.reference, loaded, reason, input.model_ceiling,
                     input.import_caller, input.reference_peak) ||
      !loaded)
    throw std::runtime_error("Accepted M6b reference load failed: " + reason);
  BridgeStats import_stats;
  if (!ImportGeometry(config, mesh.Fine(), nullptr, input.reference, input.imported, reason, import_stats,
                      input.model_ceiling, input.import_caller))
    throw std::runtime_error("Accepted M6b geometry import failed: " + reason);
  if (!input.reference.original || !input.reference.original->Parameters())
    throw std::runtime_error("Accepted M6b reference lacks boundary authority.");
  for (const auto& row : input.imported.cells)
    for (const auto& node : row.second.cell.v) input.maximum_node = std::max(input.maximum_node, node.id);
  if (input.maximum_node == UINT64_MAX) throw std::runtime_error("M6b new node id cannot be represented.");
  const auto marker = std::find(input.reference.marker_names.begin(), input.reference.marker_names.end(), "WING3D");
  if (marker == input.reference.marker_names.end()) throw std::runtime_error("Accepted M6b input lacks WING3D.");
  input.wing_marker = int(marker - input.reference.marker_names.begin());
  D9BuildLocalityIndex(input.imported.cells, input.reference, input.wing_marker, input.locality);
  if (input.locality.wall_edges.size() != 82559) throw std::runtime_error("M6b WING3D edge population mismatch.");
  input.edge_cells = M6bIndexCells(input.imported.cells, input.locality, input.edge_incidence_count);
  if (input.edge_cells.size() != input.locality.wall_edges.size())
    throw std::runtime_error("M6b edge index is incomplete.");
  input.edge_index_bytes = transfer_memory::Add(
      transfer_memory::GrowthBound(input.edge_cells.size(),
                                   sizeof(decltype(input.edge_cells)::value_type) + 4 * sizeof(void*)),
      transfer_memory::GrowthBound(input.edge_incidence_count, sizeof(Id)));
  input.baseline = transfer_memory::Add(
      input.import_caller,
      D9OperatorBaseline(input.imported.cells, input.reference, input.imported.original, input.locality),
      input.edge_index_bytes);
  input.import_seconds = import_stats.total_seconds;
  return input;
}

void M6bAddTotals(M6bRun& run, const M6bShellCounts& shell) {
  run.shell_faces += shell.wing_faces + shell.other_faces;
  run.totals.wing_faces += shell.wing_faces;
  run.totals.other_faces += shell.other_faces;
  run.totals.wing.positive += shell.wing.positive;
  run.totals.wing.zero += shell.wing.zero;
  run.totals.wing.negative += shell.wing.negative;
  run.totals.other.positive += shell.other.positive;
  run.totals.other.zero += shell.other.zero;
  run.totals.other.negative += shell.other.negative;
}

M6bRun M6bRunCensus(const M6bInput& input, M6bClock::time_point deadline, std::ofstream& csv) {
  M6bRun run;
  for (const auto& edge : input.locality.wall_edges) {
    if (M6bClock::now() >= deadline) {
      run.unvisited = input.locality.wall_edges.size() - run.attempted;
      break;
    }
    ++run.attempted;
    const auto subset = M6bEdgeCells(input.imported.cells, input.edge_cells.at(edge));
    const Patch old = D9EdgeCavity(subset, edge);
    if (old.cells.empty()) throw std::runtime_error("Indexed M6b edge has an empty cavity.");
    const size_t caller = D9OperatorCallerBytes(input.baseline, old, {});
    auto budget = D9ActualOperatorBudget(caller, deadline);
    Node projected;
    std::string reason;
    const auto before = M6bClock::now();
    bool success = false;
    try {
      projected = M6bProjectedMidpoint(*input.reference.original, old, edge, input.maximum_node + 1, budget);
      success = std::isfinite(projected.p.x) && std::isfinite(projected.p.y) && std::isfinite(projected.p.z);
      if (!success) reason = "BindNewPoint returned nonfinite coordinates.";
    } catch (const std::bad_alloc&) {
      throw;
    } catch (const std::exception& error) {
      reason = error.what();
    }
    const double seconds = std::chrono::duration<double>(M6bClock::now() - before).count();
    run.projection_seconds += seconds;
    M6bShellCounts shell;
    if (success) {
      shell = M6bMeasureShell(old, input.reference, input.wing_marker, projected.p);
      M6bAddTotals(run, shell);
    } else
      ++run.refused;
    run.succeeded += success;
    run.atlas_work = transfer_memory::Add(run.atlas_work, budget.work);
    run.atlas_peak = std::max(run.atlas_peak, budget.peak_bytes);
    M6bWriteCsvRow(csv, edge, shell, success, success, seconds, reason, budget.work, budget.peak_bytes);
  }
  if (run.attempted == input.locality.wall_edges.size()) run.unvisited = 0;
  return run;
}

void M6bWriteSummary(const char* path, const M6bInput& input, const M6bRun& run, double fixture_seconds) {
  std::ofstream json("native_m6b_projection.json");
  if (!json) throw std::runtime_error("Cannot open M6b projection summary.");
  const auto& total = run.totals;
  json << std::setprecision(17) << "{\"mesh\":" << std::quoted(path) << ",\"marker\":\"WING3D\""
       << ",\"marker_id\":" << input.wing_marker << ",\"wall_edges\":" << input.locality.wall_edges.size()
       << ",\"attempted\":" << run.attempted << ",\"projection_succeeded\":" << run.succeeded
       << ",\"projection_refusals\":" << run.refused << ",\"unvisited\":" << run.unvisited
       << ",\"all_edges_attempted\":" << (run.unvisited == 0 ? "true" : "false")
       << ",\"all_attempted_shells_measured\":" << (run.refused == 0 && run.unvisited == 0 ? "true" : "false")
       << ",\"shell_faces\":" << run.shell_faces << ",\"wing_faces\":" << total.wing_faces
       << ",\"wing_positive\":" << total.wing.positive << ",\"wing_zero\":" << total.wing.zero
       << ",\"wing_negative\":" << total.wing.negative << ",\"other_faces\":" << total.other_faces
       << ",\"other_positive\":" << total.other.positive << ",\"other_zero\":" << total.other.zero
       << ",\"other_negative\":" << total.other.negative << ",\"projection_seconds\":" << run.projection_seconds
       << ",\"atlas_work\":" << run.atlas_work << ",\"atlas_peak_bytes\":" << run.atlas_peak
       << ",\"model_ceiling_bytes\":" << input.model_ceiling << ",\"reference_peak_bytes\":" << input.reference_peak
       << ",\"job_memory_limit_mib\":19073" << ",\"import_seconds\":" << input.import_seconds
       << ",\"edge_index_bytes\":" << input.edge_index_bytes
       << ",\"edge_incidence_count\":" << input.edge_incidence_count
       << ",\"fixture_elapsed_seconds\":" << fixture_seconds
       << ",\"mesh_modified\":false,\"split_performed\":false,\"geometry_only\":true}\n";
  if (!json) throw std::runtime_error("Cannot finish M6b projection summary.");
}

TEST_CASE("Accepted Euler wall projection reports M6b shell orientations", "[.][NativeM6bProjection]") {
  using Clock = std::chrono::steady_clock;
  World world;
  REQUIRE(world.size == 1);
  const auto start = Clock::now(), deadline = start + std::chrono::minutes(90);
  const auto* path = std::getenv("SU2_NATIVE_D9_MESH");
  REQUIRE(path != nullptr);
  REQUIRE(std::string(path).size() > 0);
  std::stringstream options;
  options << "SOLVER= EULER\nADAP_NORM= 1.0\nADAP_HGRAD= 1.3\nMESH_FORMAT= SU2\nMESH_FILENAME= " << path << "\n"
          << "MARKER_FAR= (FARFIELD)\nMARKER_EULER= (WING3D)\nMARKER_SYM= (SYMMETRY)\nMGLEVEL= 0\nADAP_HAUSD= 1e-4\n";
  Mute mute;
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  CSU2ASCIIMeshReaderFVM reader(&config, 0, 1);
  MeshSolution mesh(&config, reader, 0);
  const M6bInput input = M6bLoadAcceptedEuler(path, config, mesh);
  std::ofstream csv("native_m6b_projection.csv");
  REQUIRE(csv.good());
  csv << "edge_a,edge_b,shell_measured,projection_attempted,wing_shell_faces,wing_positive,wing_zero,wing_negative,"
         "other_shell_faces,"
         "other_positive,other_zero,other_negative,projection_succeeded,projection_seconds,atlas_work,"
         "atlas_peak_bytes,projection_reason\n";
  M6bRun run = M6bRunCensus(input, deadline, csv);
  csv.close();
  REQUIRE(csv.good());
  M6bWriteSummary(path, input, run, std::chrono::duration<double>(Clock::now() - start).count());
}
