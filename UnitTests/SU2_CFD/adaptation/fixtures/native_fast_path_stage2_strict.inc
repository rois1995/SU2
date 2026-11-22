// Measurements through the production boundary-state and proof entry points.
using StrictBudget = SU2NativeGeometry::AtlasBudget;
using StrictGeometry = SU2NativeGeometry::MappedFacetReference;
using StrictBinding = SU2NativeGeometry::SurfaceBinding;
using StrictNeed = SU2NativeGeometry::SurfaceQueryNeed;
using StrictCoverage = SU2NativeGeometry::BoundaryCoverage;

struct FaceMeasurement {
  SurfaceFace face;
  double sample;
  bool original;
  std::optional<double> classified;
  std::optional<bool> contains;
};

std::pair<size_t, StrictBudget> BulkReservation(const State& state, size_t points, size_t faces,
                                                const StrictCoverage& coverage) {
  using transfer_memory::Add;
  using transfer_memory::Mul;
  const auto live = Add(Mul(points, 256), Mul(faces, sizeof(SurfaceFace)),
                        Mul(BoundaryCoverageBytes(coverage), 4));
  const auto bound = Add(live, state.Geometry().ResidentBytes(), Mul(points, 2048), Mul(faces, 8192),
                         Mul(BoundaryAtlasBytes({}, state.Mobility()), 4));
  if (bound == SIZE_MAX || bound > Ceiling) throw std::length_error("Rebind storage admission exceeded.");
  return {live, BoundaryAtlasBudget(BoundaryAtlasBytes(coverage, state.Mobility()), Ceiling, bound)};
}

std::unique_ptr<State> BuildSurface(const State& state, const BoundaryPoints& points,
                                    const std::vector<SurfaceFace>& faces, const StrictCoverage& coverage,
                                    size_t live, StrictBudget& budget) {
  return std::make_unique<State>(state.Authority(), points, faces, state.Revision(), Ceiling, live, coverage,
                                 state.Mobility(), &budget, state.GeometryTolerance());
}

std::vector<SurfaceFace> AcceptedFaces(const State& state) {
  std::vector<SurfaceFace> faces;
  faces.reserve(state.Surface().size());
  for (const auto& row : state.Surface()) faces.push_back(row.second);
  return faces;
}

size_t PolicyMemoEntryBytes(const State& state) {
  // Match the private memo entry layout without exposing test hooks in the authority.
  struct EntryLayout {
    const StrictGeometry* object;
    double angle;
    std::map<int, int> roles;
    int rounding;
    unsigned sse_control;
    std::vector<char> encoded;
    size_t work;
  };
  using Role = decltype(EntryLayout::roles)::value_type;
  const auto& policy = state.Mobility();
  return BoundaryAtlasBytes({}, policy) + sizeof(EntryLayout) + policy.roles.size() * (sizeof(Role) + 4 * sizeof(void*));
}

void MeasureBulk(const State& state) {
  BoundaryPoints points;
  for (const auto& row : state.Records()) points.emplace(row.first, row.second.point);
  const auto faces = AcceptedFaces(state);
  auto [live, reservation] = BulkReservation(state, points.size(), faces.size(), state.Coverage());
  auto policy_budget = reservation;
  const auto rss_before = CurrentRSS3D();
  std::unique_ptr<State> rebuilt;
  std::string error, snapshot_error, policy_error;
  const char* exception = "none";
  double constructor;
  {
    namespace timing = SU2NativeTiming3D;
    timing::Session session(true, "FX2_M6_B1", 0, 1, timing::Phase::Construction);
    const auto start = Clock::now();
    try {
      rebuilt = BuildSurface(state, points, faces, state.Coverage(), live, reservation);
    } catch (const std::exception& e) {
      exception = typeid(e).name();
      error = e.what();
    }
    constructor = Seconds(start);
  }
  const auto rss_after = CurrentRSS3D();
  std::cout << "FX2_MEMO case=F5 "
            << NativeFX2Fixture::CallRow(&reservation, exception, error, rebuilt.get()) << '\n';
  std::optional<bool> equal;
  if (rebuilt) {
    try {
      equal = state.Snapshot() == rebuilt->Snapshot();
    } catch (const std::exception& e) {
      snapshot_error = e.what();
    }
    rebuilt.reset();
  }
  SU2NativeGeometry::MobilityPolicy policy;
  const auto start = Clock::now();
  try {
    if (!state.Geometry().BuildMobilityPolicy(state.Mobility().angle, state.Mobility().roles, policy,
                                               policy_budget, policy_error) && policy_error.empty())
      policy_error = "BuildMobilityPolicy returned false.";
  } catch (const std::exception& e) {
    policy_error = e.what();
  }
  const auto policy_seconds = Seconds(start);
  Output("B1");
  std::cout << "\"points\":" << points.size() << ",\"faces\":" << faces.size()
            << ",\"constructor_seconds\":" << constructor << ",\"policy_seconds\":" << policy_seconds
            << ",\"constructor_minus_policy_seconds\":";
  JsonNumber(error.empty() && policy_error.empty() ? constructor - policy_seconds
                                                   : std::numeric_limits<double>::quiet_NaN());
  std::cout << ",\"modeled_peak_bytes\":" << reservation.peak_bytes
            << ",\"reservation_work\":" << reservation.work
            << ",\"memo_entry_bytes\":" << PolicyMemoEntryBytes(state)
            << ",\"policy_modeled_peak_bytes\":" << policy_budget.peak_bytes
            << ",\"rss_before_bytes\":" << rss_before << ",\"rss_after_bytes\":" << rss_after
            << ",\"rss_change_bytes\":" << int64_t(rss_after) - int64_t(rss_before)
            << ",\"snapshot_equal\":" << (equal ? (*equal ? "true" : "false") : "null")
            << ",\"traces\":" << state.Coverage().traces.size();
  if (!error.empty()) { std::cout << ",\"constructor_error\":"; JsonString(error); }
  if (!snapshot_error.empty()) { std::cout << ",\"snapshot_error\":"; JsonString(snapshot_error); }
  if (!policy_error.empty()) { std::cout << ",\"policy_error\":"; JsonString(policy_error); }
  std::cout << "}" << std::endl;
}

struct LocalChecks {
  size_t admissible = 0, realize = 0, contains = 0;
  size_t inadmissible = 0, realize_errors = 0, bit_differences = 0, outside = 0;
  double admissible_seconds = 0, realize_seconds = 0, contains_seconds = 0, max_difference = 0;
};

void WriteLocal(const LocalChecks& result) {
  std::cout << "{\"admissible_calls\":" << result.admissible << ",\"admissible_failures\":" << result.inadmissible
            << ",\"admissible_seconds\":" << result.admissible_seconds
            << ",\"admissible_us_per_call\":" << 1e6 * result.admissible_seconds / std::max(size_t{1}, result.admissible)
            << ",\"realize_calls\":" << result.realize << ",\"realize_failures\":" << result.realize_errors
            << ",\"realize_seconds\":" << result.realize_seconds
            << ",\"realize_us_per_call\":" << 1e6 * result.realize_seconds / std::max(size_t{1}, result.realize)
            << ",\"realize_not_bit_equal\":" << result.bit_differences
            << ",\"realize_max_difference\":" << result.max_difference
            << ",\"contains_calls\":" << result.contains << ",\"contains_failures\":" << result.outside
            << ",\"contains_seconds\":" << result.contains_seconds
            << ",\"contains_us_per_call\":" << 1e6 * result.contains_seconds / std::max(size_t{1}, result.contains)
            << '}';
}

bool SamePointBits(Point a, Point b) {
  return std::memcmp(&a.x, &b.x, sizeof(double)) == 0 && std::memcmp(&a.y, &b.y, sizeof(double)) == 0 &&
         std::memcmp(&a.z, &b.z, sizeof(double)) == 0;
}

void MeasureLocal(const State& state) {
  LocalChecks all;
  std::map<State::EntityKey, LocalChecks> entities;
  std::map<Id, std::set<State::EntityKey>> incidence;
  for (const auto& row : state.Surface()) {
    const auto entity = state.Entity(row.second.facet);
    for (const auto id : row.second.v) incidence[id].insert(entity);
  }
  std::string first_error;
  const auto begin = Clock::now();
  for (const auto& row : state.Records()) {
    const auto& point = row.second.point;
    auto start = Clock::now();
    bool admissible = false;
    try { admissible = state.Geometry().Admissible(point.binding, point.node.p); }
    catch (const std::exception& e) { if (first_error.empty()) first_error = e.what(); }
    const auto admissible_seconds = Seconds(start);
    start = Clock::now();
    Point realized{};
    bool realized_ok = false;
    try {
      realized = state.Geometry().Realize(point.binding);
      realized_ok = true;
    } catch (const std::exception& e) { if (first_error.empty()) first_error = e.what(); }
    const auto realize_seconds = Seconds(start);
    const bool differs = realized_ok && !SamePointBits(point.node.p, realized);
    const auto& p = point.node.p;
    const double distance = differs ? std::hypot(p.x - realized.x, p.y - realized.y, p.z - realized.z) : 0;
    const auto accumulate = [&](LocalChecks& result) {
      ++result.admissible;
      ++result.realize;
      result.inadmissible += !admissible;
      result.realize_errors += !realized_ok;
      result.bit_differences += differs;
      result.admissible_seconds += admissible_seconds;
      result.realize_seconds += realize_seconds;
      result.max_difference = std::max(result.max_difference, distance);
    };
    accumulate(all);
    for (const auto& entity : incidence.at(row.first)) accumulate(entities[entity]);
  }
  for (const auto& row : state.Surface()) {
    const auto& face = row.second;
    const Triangle points{state.Records().at(face.v[0]).point, state.Records().at(face.v[1]).point,
                          state.Records().at(face.v[2]).point};
    const auto start = Clock::now();
    bool contains = false;
    try { contains = state.ContainsEntityFace(face.facet, points); }
    catch (const std::exception& e) { if (first_error.empty()) first_error = e.what(); }
    const auto seconds = Seconds(start);
    auto& entity = entities[state.Entity(face.facet)];
    for (auto* result : {&entity, &all}) {
      ++result->contains;
      result->outside += !contains;
      result->contains_seconds += seconds;
    }
  }
  Output("B2");
  std::cout << "\"seconds\":" << Seconds(begin) << ",\"all\":";
  WriteLocal(all);
  std::cout << ",\"entities\":[";
  bool comma = false;
  for (const auto& row : entities) {
    if (comma) std::cout << ',';
    comma = true;
    std::cout << "{\"entity\":";
    EntityJson(row.first);
    std::cout << ",\"checks\":";
    WriteLocal(row.second);
    std::cout << '}';
  }
  std::cout << ']';
  if (!first_error.empty()) { std::cout << ",\"first_exception\":"; JsonString(first_error); }
  std::cout << "}" << std::endl;
}

void MeasureBulkAndLocal(const State& state) {
  auto start = Clock::now();
  try { MeasureBulk(state); }
  catch (const std::exception& e) { ReportError("B1", start, e); }
  start = Clock::now();
  try { MeasureLocal(state); }
  catch (const std::exception& e) { ReportError("B2", start, e); }
}

std::pair<std::array<StrictBinding, 3>, std::array<Point, 3>> FaceData(const Triangle& triangle) {
  return {{triangle[0].binding, triangle[1].binding, triangle[2].binding},
          {triangle[0].node.p, triangle[1].node.p, triangle[2].node.p}};
}

double StrictFaceError(const State& state, const SurfaceFace& face, const BoundaryPoints& points, StrictBudget& budget,
                       StrictNeed need) {
  const auto [binding, coords] = FaceData(GetTriangle(points, face));
  return state.EntityFaceError(face.facet, binding, coords, &budget, need);
}

using StrictFaceKey = std::pair<Id, Face>;

StrictFaceKey FaceIdentity(const SurfaceFace& face) {
  auto vertices = face.v;
  std::sort(vertices.begin(), vertices.end());
  return {face.facet, vertices};
}

void WriteFaceIds(std::ostream& out, const SurfaceFace& face) {
  out << "\"anchor\":" << face.facet << ",\"vertices\":[" << face.v[0] << ',' << face.v[1] << ',' << face.v[2] << ']';
}

void WritePosition(std::ostream& out, Point point) {
  out << '[' << point.x << ',' << point.y << ',' << point.z << ']';
}

void WriteNullable(std::ostream& out, double value) {
  if (std::isfinite(value)) out << value;
  else out << "null";
}

void WriteFaceValue(std::ostream& out, const State& state, const char* dataset, const FaceMeasurement& row) {
  out << "{\"kind\":\"face\",\"surface\":\"" << dataset << "\",";
  WriteFaceIds(out, row.face);
  const auto [body, sheet, marker] = state.Entity(row.face.facet);
  out << ",\"entity\":[" << body << ',' << sheet << ',' << marker << "],\"original\":"
      << (row.original ? "true" : "false") << ",\"sample\":";
  WriteNullable(out, row.sample);
  out << ",\"classify\":";
  if (row.classified) WriteNullable(out, *row.classified);
  else out << "null";
  out << ",\"classify_observed\":" << (row.classified ? "true" : "false")
      << ",\"classify_above_tol\":" << (row.classified && *row.classified > Tol ? "true" : "false")
      << ",\"contains\":" << (row.contains ? (*row.contains ? "true" : "false") : "null") << "}\n";
}

struct PairedCounts {
  size_t both_above = 0, float_only_above = 0, classify_only_above = 0, both_within = 0, unpaired = 0;
  void Add(bool has_sample, bool float_above, const std::optional<double>& classified) {
    if (!has_sample || !classified) {
      ++unpaired;
      return;
    }
    const bool strict_above = *classified > Tol;
    both_above += float_above && strict_above;
    float_only_above += float_above && !strict_above;
    classify_only_above += !float_above && strict_above;
    both_within += !float_above && !strict_above;
  }
};

struct StrictCounts {
  size_t faces = 0, contains_measured = 0, outside = 0;
  size_t sampled = 0, sampled_above_half = 0, sampled_above_seven = 0, sampled_above_eight = 0;
  size_t sampled_above_tol = 0;
  size_t classified = 0, classified_above_tol = 0;
  PairedCounts at_tol, at_float_limit;
  void Add(const FaceMeasurement& row) {
    ++faces;
    contains_measured += bool(row.contains);
    outside += row.contains && !*row.contains;
    const bool has_sample = std::isfinite(row.sample);
    const bool float_above = row.sample > Tol;
    const bool strict_above = row.classified && *row.classified > Tol;
    if (has_sample) {
      ++sampled;
      sampled_above_half += row.sample > 0.5 * Tol;
      sampled_above_seven += row.sample > 0.7 * Tol;
      sampled_above_eight += row.sample > 0.8 * Tol;
      sampled_above_tol += float_above;
    }
    classified += bool(row.classified);
    classified_above_tol += strict_above;
    at_tol.Add(has_sample, float_above, row.classified);
    at_float_limit.Add(has_sample, row.sample > 0.7 * Tol, row.classified);
  }
};

void WritePairedCounts(const PairedCounts& row) {
  std::cout << "{\"both_above\":" << row.both_above << ",\"float_only_above\":" << row.float_only_above
            << ",\"classify_only_above\":" << row.classify_only_above << ",\"both_within\":" << row.both_within
            << ",\"unpaired\":" << row.unpaired << '}';
}

void WriteStrictCounts(const StrictCounts& row) {
  std::cout << "{\"faces\":" << row.faces << ",\"contains_measured\":" << row.contains_measured
            << ",\"contains_failed\":" << row.outside << ",\"float_sampled\":" << row.sampled
            << ",\"float_above_half_tol\":" << row.sampled_above_half
            << ",\"float_above_seven_tenths_tol\":" << row.sampled_above_seven
            << ",\"float_above_eight_tenths_tol\":" << row.sampled_above_eight
            << ",\"float_above_tol\":" << row.sampled_above_tol << ",\"classified\":" << row.classified
            << ",\"unclassified\":" << row.faces - row.classified
            << ",\"classify_above_tol\":" << row.classified_above_tol << ",\"paired_at_tol\":";
  WritePairedCounts(row.at_tol);
  std::cout << ",\"paired_at_float_limit\":";
  WritePairedCounts(row.at_float_limit);
  std::cout << '}';
}

void WriteStrictDataset(const State& state, const std::vector<FaceMeasurement>& faces, bool new_only) {
  StrictCounts total;
  std::map<State::EntityKey, StrictCounts> entities;
  for (const auto& row : faces) {
    if (new_only && row.original) continue;
    total.Add(row);
    entities[state.Entity(row.face.facet)].Add(row);
  }
  std::cout << "{\"all\":";
  WriteStrictCounts(total);
  std::cout << ",\"entities\":[";
  bool comma = false;
  for (const auto& row : entities) {
    if (comma) std::cout << ',';
    comma = true;
    std::cout << "{\"entity\":";
    EntityJson(row.first);
    std::cout << ",\"counts\":";
    WriteStrictCounts(row.second);
    std::cout << '}';
  }
  std::cout << "]}";
}

void WriteEntityFailure(const State& state, const BoundaryPoints& points, const FaceMeasurement& row);

void MeasureStrict(const State& state, const BoundaryPoints& in_points, std::vector<FaceMeasurement>& in_faces,
                   const BoundaryPoints& final_points, std::vector<FaceMeasurement>& final_faces, bool diagnose) {
  std::map<StrictFaceKey, const FaceMeasurement*> unchanged;
  const auto caller = state.ResidentBytes() + state.Geometry().ResidentBytes();
  std::string contains_error, in_error, pd_error;
  const auto contains_start = Clock::now();
  const auto check_entity = [&](const BoundaryPoints& points, FaceMeasurement& row) {
    row.classified.reset();
    row.contains.reset();
    try { row.contains = state.ContainsEntityFace(row.face.facet, GetTriangle(points, row.face)); }
    catch (const std::exception& e) { if (contains_error.empty()) contains_error = e.what(); }
  };
  for (auto& row : in_faces) {
    check_entity(in_points, row);
    unchanged.emplace(FaceIdentity(row.face), &row);
  }
  std::vector<FaceMeasurement*> fresh;
  for (auto& row : final_faces) {
    const auto found = unchanged.find(FaceIdentity(row.face));
    row.original = found != unchanged.end();
    if (row.original) row.contains = found->second->contains;
    else {
      check_entity(final_points, row);
      fresh.push_back(&row);
    }
  }
  const double contains_seconds = Seconds(contains_start);

  auto start = Clock::now();
  auto budget = Fp2Budget(726442, caller, start, 5400);
  const auto classify = [&](const BoundaryPoints& points, FaceMeasurement& row, std::string& first_error) {
    try { row.classified = StrictFaceError(state, row.face, points, budget, StrictNeed::Classify()); }
    catch (const std::exception& e) { if (first_error.empty()) first_error = e.what(); }
  };
  size_t in_attempted = 0;
  for (auto& row : in_faces) {
    if (budget.time_exhausted || budget.work_exhausted || Seconds(start) >= 5400) break;
    ++in_attempted;
    classify(in_points, row, in_error);
  }
  const auto in_seconds = Seconds(start);
  for (auto& row : final_faces)
    if (row.original) row.classified = unchanged.at(FaceIdentity(row.face))->classified;

  constexpr double pd_limit = 1200;
  start = Clock::now();
  budget = Fp2Budget(726442, caller, start, pd_limit);
  const auto pilot_limit = std::min(fresh.size(), size_t{2000});
  size_t pilot = 0, pd_attempted = 0;
  const auto pd_open = [&] {
    return !budget.time_exhausted && !budget.work_exhausted && Seconds(start) < pd_limit;
  };
  for (; pilot < pilot_limit && pd_open(); ++pilot)
    classify(final_points, *fresh[pilot], pd_error);
  pd_attempted += pilot;
  const auto pilot_seconds = Seconds(start);
  const double estimate = pilot != 0 ? pilot_seconds * fresh.size() / pilot : 0;
  size_t stride = 1;
  if (estimate > pd_limit && pilot < fresh.size()) {
    const double affordable = (pd_limit - pilot_seconds) * pilot / pilot_seconds;
    const auto remaining = fresh.size() - pilot;
    const auto slots = static_cast<size_t>(std::max(1.0, std::floor(affordable)));
    stride = (remaining + slots - 1) / slots;
  }
  for (size_t i = pilot; i < fresh.size() && pd_open(); i += stride) {
    ++pd_attempted;
    classify(final_points, *fresh[i], pd_error);
  }
  const auto pd_seconds = Seconds(start);
  Output("P_STRICT");
  std::cout << "\"seconds\":" << contains_seconds + in_seconds + pd_seconds
            << ",\"contains_seconds\":" << contains_seconds << ",\"in_classify_seconds\":" << in_seconds
            << ",\"in_attempted\":" << in_attempted << ",\"in_classify_source\":\"recomputed\""
            << ",\"pd_classify_seconds\":" << pd_seconds << ",\"pd_new_total\":" << fresh.size()
            << ",\"pd_new_attempted\":" << pd_attempted << ",\"pd_pilot_attempted\":" << pilot
            << ",\"pd_pilot_seconds\":" << pilot_seconds << ",\"pd_full_estimate_seconds\":" << estimate
            << ",\"pd_remaining_stride\":" << stride << ",\"pd_limit_seconds\":" << pd_limit;
  if (!contains_error.empty()) { std::cout << ",\"contains_first_exception\":"; JsonString(contains_error); }
  if (!in_error.empty()) { std::cout << ",\"in_first_exception\":"; JsonString(in_error); }
  if (!pd_error.empty()) { std::cout << ",\"pd_first_exception\":"; JsonString(pd_error); }
  std::cout << ",\"IN1\":";
  WriteStrictDataset(state, in_faces, false);
  std::cout << ",\"PD_NEW\":";
  WriteStrictDataset(state, final_faces, true);
  std::cout << ",\"PD\":";
  WriteStrictDataset(state, final_faces, false);
  std::cout << "}" << std::endl;

  const char* values_path = std::getenv("SU2_FP2_VALUES");
  if (!values_path) throw std::runtime_error("SU2_FP2_VALUES must name the per-face JSONL output.");
  std::ofstream values(values_path);
  values.exceptions(std::ios::badbit | std::ios::failbit);
  values << std::setprecision(17);
  for (const auto& row : in_faces) WriteFaceValue(values, state, "IN1", row);
  for (const auto& row : final_faces) WriteFaceValue(values, state, "PD", row);
  values.flush();
  if (diagnose) {
    for (const auto& row : final_faces) {
      if (row.contains && !*row.contains) WriteEntityFailure(state, final_points, row);
    }
  }
}

void WritePointWitness(const char* predicate, const BoundaryPoint& point, std::optional<Id> other = {}) {
  const auto& binding = point.binding;
  std::cout << "{\"predicate\":";
  JsonString(predicate);
  std::cout << ",\"id\":" << point.node.id;
  if (other) std::cout << ",\"other_id\":" << *other;
  std::cout << ",\"coordinates\":";
  WritePosition(std::cout, point.node.p);
  std::cout << ",\"binding\":{\"dimension\":" << binding.dimension << ",\"feature\":" << binding.feature
            << ",\"version\":" << binding.version << ",\"u\":" << binding.u << ",\"v\":" << binding.v << "}}";
}

void WriteFaceWitness(const State& state, const BoundaryPoints& points, const FaceMeasurement& row) {
  std::cout << '{';
  WriteFaceIds(std::cout, row.face);
  std::cout << ",\"entity\":";
  EntityJson(state.Entity(row.face.facet));
  std::cout << ",\"float_sample\":";
  JsonNumber(row.sample);
  std::cout << ",\"classify\":";
  if (row.classified) JsonNumber(*row.classified);
  else std::cout << "null";
  std::cout << ",\"coordinates\":[";
  for (size_t i = 0; i < row.face.v.size(); ++i) {
    if (i) std::cout << ',';
    WritePosition(std::cout, points.at(row.face.v[i]).node.p);
  }
  std::cout << "]}";
}

void WriteEntityFailure(const State& state, const BoundaryPoints& points, const FaceMeasurement& row) {
  const auto triangle = GetTriangle(points, row.face);
  const auto coords = FaceData(triangle).second;
  const auto normal = FlatNormal(triangle);
  const auto entity = state.Entity(row.face.facet);
  const auto& geometry = state.Geometry();
  Output("P_ENTITY_FAILURE");
  std::cout << "\"face\":";
  WriteFaceWitness(state, points, row);
  std::cout << ",\"corners\":[";
  for (size_t i = 0; i < triangle.size(); ++i) {
    if (i) std::cout << ',';
    const auto& point = triangle[i];
    std::cout << "{\"id\":" << point.node.id << ",\"admissible\":";
    try {
      std::cout << (geometry.Admissible(point.binding, point.node.p) ? "true" : "false");
    } catch (const std::exception& e) {
      std::cout << "null,\"admissible_error\":";
      JsonString(e.what());
    }
    std::cout << ",\"sides\":[";
    bool comma = false;
    std::string facets_error;
    try {
      geometry.ForFacets(point.binding, [&](Id side) {
        const auto side_entity = state.Entity(side);
        if (comma) std::cout << ',';
        comma = true;
        std::cout << "{\"side\":" << side << ",\"entity\":";
        EntityJson(side_entity);
        std::cout << ",\"entity_matches\":" << (side_entity == entity ? "true" : "false")
                  << ",\"oriented_face\":";
        try {
          std::cout << (geometry.OrientedFace(side, point.binding, coords) ? "true" : "false");
        } catch (const std::exception& e) {
          std::cout << "null,\"oriented_face_error\":";
          JsonString(e.what());
        }
        std::cout << ",\"float_dot\":";
        try {
          JsonNumber(ChildNormalDot(state, side, point, normal));
        } catch (const std::exception& e) {
          std::cout << "null,\"float_dot_error\":";
          JsonString(e.what());
        }
        std::cout << '}';
      });
    } catch (const std::exception& e) {
      facets_error = e.what();
    }
    std::cout << ']';
    if (!facets_error.empty()) {
      std::cout << ",\"facets_error\":";
      JsonString(facets_error);
    }
    std::cout << '}';
  }
  std::cout << "]}" << std::endl;
}

std::string FindBulkWitness(const State& state, const BoundaryPoints& points,
                            const std::vector<FaceMeasurement>& faces) {
  std::map<State::Coordinate, Id> coordinates;
  std::map<State::Support, Id> supports;
  for (const auto& row : points) {
    const auto& point = row.second;
    const auto& p = point.node.p;
    const auto& b = point.binding;
    if (row.first != point.node.id || !state.Geometry().Admissible(b, p)) {
      WritePointWitness("point identity or Admissible", point);
      return "point";
    }
    const auto existing = coordinates.emplace(State::Coordinate{p.x, p.y, p.z}, row.first);
    if (!existing.second) {
      WritePointWitness("duplicate coordinates", point, existing.first->second);
      return "point";
    }
    const auto support = supports.emplace(State::Support{b.dimension, b.feature, b.u, b.v}, row.first);
    if (!support.second) {
      WritePointWitness("duplicate binding", point, support.first->second);
      return "point";
    }
  }
  for (const auto corner : state.Mobility().corners) {
    if (supports.count(State::Support{0, corner, 0, 0})) continue;
    std::cout << "{\"predicate\":\"omitted physical corner\",\"corner\":" << corner << '}';
    return "corner";
  }
  std::set<Id> used;
  for (const auto& row : faces) {
    used.insert(row.face.v.begin(), row.face.v.end());
    if (row.contains ? *row.contains : state.ContainsEntityFace(row.face.facet, GetTriangle(points, row.face))) continue;
    WriteFaceWitness(state, points, row);
    return "face";
  }
  for (const auto& row : points) {
    if (used.count(row.first)) continue;
    WritePointWitness("unused point", row.second);
    return "point";
  }
  return {};
}

struct TraceAudit {
  std::optional<Edge> edge;
  std::string error;
  std::map<Id, size_t> chain_entries;
};

std::array<Edge, 3> FaceEdges(const SurfaceFace& face);

std::string FindTopologyWitness(const State& state, const BoundaryPoints& points,
                                const std::vector<FaceMeasurement>& faces) {
  std::set<Face> keys;
  std::map<Edge, int> directions;
  std::vector<Face> skin;
  skin.reserve(faces.size());
  const auto write_edge = [](const Edge& edge) {
    std::cout << "{\"edge\":[" << edge[0] << ',' << edge[1] << "]}";
    return "topology_edge";
  };
  for (const auto& row : faces) {
    const auto& face = row.face.v;
    auto key = face;
    std::sort(key.begin(), key.end());
    if (std::adjacent_find(key.begin(), key.end()) != key.end() || !keys.insert(key).second) {
      WriteFaceWitness(state, points, row);
      return "topology_face";
    }
    const auto edges = FaceEdges(row.face);
    for (size_t k = 0; k < edges.size(); ++k) {
      const int direction = face[k] < face[(k + 1) % 3] ? 1 : -1;
      const auto [entry, inserted] = directions.emplace(edges[k], direction);
      if (!inserted) {
        if (entry->second != -direction) return write_edge(edges[k]);
        entry->second = 0;
      }
    }
    skin.push_back(face);
  }
  for (const auto& [edge, direction] : directions)
    if (direction != 0) return write_edge(edge);
  Feature closure;
  try { ValidateSurfaceTopology(skin, true, skin.size(), &closure); }
  catch (const std::invalid_argument&) {
    if (closure.count == 1) {
      std::cout << "{\"vertex\":" << closure.vertices[0] << '}';
      return "topology_vertex";
    }
    if (closure.count == 2) return write_edge({closure.vertices[0], closure.vertices[1]});
  }
  return {};
}

void AuditTraces(const State& state, const BoundaryPoints& points, const std::vector<SurfaceFace>& faces,
                 const StrictCoverage& coverage, TraceAudit& result) {
  const auto& geometry = state.Geometry();
  auto budget = BulkReservation(state, points.size(), faces.size(), coverage).second;
  std::set<Edge> edges;
  for (const auto& face : faces)
    for (const auto& edge : FaceEdges(face)) edges.insert(edge);
  for (const auto& row : state.Mobility().features) result.chain_entries.emplace(row.second, 0);
  for (const auto& [edge, trace] : coverage.traces) {
    result.edge = edge;
    if (edge[0] >= edge[1] || !edges.count(edge) || trace.pieces.empty()) {
      result.error = "Entity curve has no physical edge.";
      return;
    }
    auto normalized = trace;
    std::optional<Id> chain;
    std::string reason;
    if (!SU2NativeGeometry::NormalizeFeatureTrace(geometry, state.Mobility(), normalized, chain, budget, reason) ||
        !chain) {
      result.error = "Invalid entity feature trace: " + reason;
      return;
    }
    bool equal = false;
    if (trace.pieces.size() != normalized.pieces.size() ||
        !geometry.CompareTraces(trace, normalized, equal, budget, reason) || !equal) {
      result.error = "Entity feature trace is not canonical.";
      return;
    }
    for (size_t endpoint = 0; endpoint < edge.size(); ++endpoint) {
      const auto& piece = endpoint ? trace.pieces.back() : trace.pieces.front();
      const auto& parameter = endpoint ? piece.last : piece.first;
      SU2NativeGeometry::ExactBary binding;
      std::vector<uint64_t> actual, expected;
      if (!geometry.BindingParameters(piece.facet, points.at(edge[endpoint]).binding, binding, budget, reason) ||
          !geometry.ReferencePointKey(piece.facet, binding, actual, budget, reason) ||
          !geometry.ReferencePointKey(piece.facet, parameter, expected, budget, reason) || actual != expected) {
        result.error = "Entity feature trace endpoints differ from persistent associations.";
        return;
      }
    }
    ++result.chain_entries[*chain];
  }
  result.edge.reset();
}

void MeasureDirectedProof(const State& rebuilt, const BoundaryPoints& points, const std::vector<FaceMeasurement>& faces) {
  const auto accepted = AcceptedFaces(rebuilt);
  std::set<State::EntityKey> keys;
  for (const auto& row : rebuilt.Geometry().Reference().Facets()) keys.insert(rebuilt.Entity(row.first));
  const auto begin = Clock::now();
  auto budget = Fp2Budget(726442, rebuilt.ResidentBytes() + rebuilt.Geometry().ResidentBytes(), begin, 5400);
  std::vector<double> reverse;
  const State::Certificates* forward = nullptr;
  std::string reverse_error, forward_error;
  try {
    reverse = rebuilt.OwnedFaceReverseBounds(accepted, budget);
    if (reverse.size() != keys.size()) {
      reverse_error = "OwnedFaceReverseBounds returned a different number of canonical entity bounds.";
      reverse.clear();
    }
  } catch (const std::exception& e) {
    reverse_error = e.what();
  }
  const double reverse_seconds = Seconds(begin);
  const auto start_forward = Clock::now();
  try { forward = &rebuilt.GeometryCertificates(budget, false); }
  catch (const std::exception& e) { forward_error = e.what(); }
  const double forward_seconds = Seconds(start_forward);

  const auto magnitude_start = Clock::now();
  std::map<State::EntityKey, size_t> total, observed;
  std::map<State::EntityKey, std::vector<const FaceMeasurement*>> above;
  for (const auto& row : faces) {
    const auto entity = rebuilt.Entity(row.face.facet);
    ++total[entity];
    if (!row.classified) continue;
    ++observed[entity];
    if (*row.classified > Tol) above[entity].push_back(&row);
  }
  Output("P4");
  std::cout << "\"status\":" << (reverse_error.empty() && forward_error.empty() ? "\"measured\"" : "\"partial\"")
            << ",\"seconds\":" << reverse_seconds + forward_seconds
            << ",\"mesh_to_model_seconds\":" << reverse_seconds << ",\"model_to_mesh_seconds\":" << forward_seconds;
  if (!reverse_error.empty()) { std::cout << ",\"OwnedFaceReverseBounds_error\":"; JsonString(reverse_error); }
  if (!forward_error.empty()) { std::cout << ",\"GeometryCertificates_error\":"; JsonString(forward_error); }
  std::cout << ",\"entities\":[";
  size_t index = 0;
  const auto missing = std::numeric_limits<double>::quiet_NaN();
  for (const auto& entity : keys) {
    if (index) std::cout << ',';
    const double a = index < reverse.size() ? reverse[index] : missing;
    ++index;
    const StrictGeometry::SurfaceUnionBounds* certificate = nullptr;
    if (forward) {
      const auto found = forward->find(entity);
      if (found != forward->end()) certificate = &found->second;
    }
    const double b = certificate ? certificate->model_to_mesh : missing;
    const bool decided = !std::isnan(a) && certificate;
    const bool accepted_entity = decided && !certificate->outside_tolerance && std::isfinite(a) && a >= 0 && a <= Tol &&
                                 std::isfinite(b) && b >= 0 && b <= Tol;
    std::cout << "{\"entity\":";
    EntityJson(entity);
    std::cout << ",\"mesh_to_model\":";
    JsonNumber(a);
    std::cout << ",\"model_to_mesh\":";
    JsonNumber(b);
    std::cout << ",\"accepted\":" << (decided ? (accepted_entity ? "true" : "false") : "null")
              << ",\"seconds\":null,\"classify_above_tol\":" << above[entity].size()
              << ",\"classified\":" << observed[entity] << ",\"total_faces\":" << total[entity]
              << ",\"witnesses\":[";
    const auto& bad = above[entity];
    for (size_t i = 0; i < std::min(size_t{20}, bad.size()); ++i) {
      if (i) std::cout << ',';
      const auto& row = *bad[i];
      std::cout << "{\"face\":";
      WriteFaceWitness(rebuilt, points, row);
      double bound = missing;
      std::string error;
      try { bound = StrictFaceError(rebuilt, row.face, points, budget, StrictNeed::Magnitude()); }
      catch (const std::exception& e) { error = e.what(); }
      std::cout << ",\"magnitude\":";
      JsonNumber(bound);
      if (!error.empty()) { std::cout << ",\"magnitude_error\":"; JsonString(error); }
      std::cout << '}';
    }
    std::cout << "]}";
  }
  std::cout << "],\"witness_seconds\":" << Seconds(magnitude_start)
            << ",\"work\":" << budget.work << ",\"modeled_peak_bytes\":" << budget.peak_bytes
            << ",\"work_exhausted\":" << (budget.work_exhausted ? "true" : "false")
            << ",\"time_exhausted\":" << (budget.time_exhausted ? "true" : "false") << "}" << std::endl;
}

bool MeasureProof(const State& state, const BoundaryPoints& final_points,
                  const std::vector<FaceMeasurement>& final_faces, const StrictCoverage& coverage) {
  std::vector<SurfaceFace> faces;
  faces.reserve(final_faces.size());
  for (const auto& row : final_faces) faces.push_back(row.face);
  auto start = Clock::now();
  std::unique_ptr<State> rebuilt;
  size_t modeled_peak = 0;
  std::string error;
  try {
    auto [live, budget] = BulkReservation(state, final_points.size(), faces.size(), coverage);
    try { rebuilt = BuildSurface(state, final_points, faces, coverage, live, budget); }
    catch (const std::exception& e) { error = e.what(); }
    modeled_peak = budget.peak_bytes;
  } catch (const std::exception& e) {
    error = e.what();
  }
  const auto seconds = Seconds(start);
  Output("P3");
  std::cout << "\"accepted\":" << (rebuilt ? "true" : "false") << ",\"seconds\":" << seconds
            << ",\"modeled_peak_bytes\":" << modeled_peak << ",\"points\":" << final_points.size()
            << ",\"faces\":" << faces.size() << ",\"traces\":" << coverage.traces.size();
  if (!rebuilt) {
    std::cout << ",\"constructor_error\":";
    JsonString(error);
    std::cout << ",\"first_local_witness\":";
    std::string witness;
    std::string witness_error;
    const bool topology = error.find("Native 3D domain skin") == 0 || error.find("Native 3D cavity") == 0;
    try {
      witness = topology ? FindTopologyWitness(state, final_points, final_faces)
                         : FindBulkWitness(state, final_points, final_faces);
    }
    catch (const std::exception& e) {
      witness_error = e.what();
    }
    if (witness.empty()) {
      TraceAudit traces;
      if (witness_error.empty() && !topology) {
        try { AuditTraces(state, final_points, faces, coverage, traces); }
        catch (const std::exception& e) { traces.error = e.what(); }
      }
      if (traces.edge) {
        witness = "trace";
        std::cout << "{\"edge\":[" << (*traces.edge)[0] << ',' << (*traces.edge)[1] << "],\"predicate\":";
        JsonString(traces.error);
        std::cout << '}';
      } else {
        std::cout << "null";
        if (witness_error.empty() && traces.error.empty() && !topology) {
          std::cout << ",\"trace_chain_entries\":[";
          bool comma = false;
          for (const auto& [chain, entries] : traces.chain_entries) {
            if (comma) std::cout << ',';
            comma = true;
            std::cout << "{\"chain\":" << chain << ",\"entries\":" << entries << '}';
          }
          std::cout << ']';
        }
      }
      if (!traces.edge && witness_error.empty()) witness_error = traces.error;
    }
    if (!witness_error.empty()) {
      std::cout << ",\"witness_error\":";
      JsonString(witness_error);
    }
    if (!witness.empty()) {
      std::cout << ",\"witness_kind\":";
      JsonString(witness);
    }
  }
  std::cout << "}" << std::endl;
  if (!rebuilt) {
    Output("P4");
    std::cout << "\"status\":\"not_applicable\",\"reason\":\"P3 constructor declined the prototype surface.\","
                 "\"seconds\":0}" << std::endl;
    return false;
  }
  start = Clock::now();
  try { MeasureDirectedProof(*rebuilt, final_points, final_faces); }
  catch (const std::exception& e) { ReportError("P4", start, e); }
  return true;
}
