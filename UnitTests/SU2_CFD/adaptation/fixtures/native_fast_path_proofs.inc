// FP0 test-only helpers. Include from the anonymous namespace of the measurement TU.
constexpr size_t Fp0GeometryByteLimit = size_t{12288} << 20;
constexpr size_t Fp0GeometryScratch = size_t{16} << 20;

void Fp0JsonString(const std::string& value) {
  constexpr char digits[] = "0123456789abcdef";
  std::cout << '"';
  for (unsigned char c : value) {
    if (c == '"' || c == '\\') {
      std::cout << '\\' << static_cast<char>(c);
    } else if (c < 0x20) {
      std::cout << "\\u00" << digits[c / 16] << digits[c % 16];
    } else {
      std::cout << static_cast<char>(c);
    }
  }
  std::cout << '"';
}

void Fp0JsonNumber(double value) {
  if (std::isfinite(value))
    std::cout << value;
  else
    std::cout << "null";
}

SU2NativeGeometry::AtlasBudget Fp0AtlasBudget(size_t cells, size_t caller_bytes, Clock::time_point start,
                                              double seconds) {
  using SU2NativeGeometry::CurvedAtlasWorkspaceBytes;
  size_t low = 0, high = Fp0GeometryScratch * 8;
  while (low < high) {
    const size_t middle = low + (high - low + 1) / 2;
    if (2 * CurvedAtlasWorkspaceBytes(middle) <= Fp0GeometryScratch / 2)
      low = middle;
    else
      high = middle - 1;
  }
  SU2NativeGeometry::AtlasBudget budget;
  budget.maximum_work =
      (cells + std::max(size_t{4096}, cells * 64)) * SU2NativeGeometry::MappedFacetReference::ClearanceWork;
  budget.maximum_bytes = Fp0GeometryByteLimit;
  budget.maximum_integer_bits = low;
  budget.caller_bytes = caller_bytes;
  budget.within_time = [start, seconds] { return Seconds(start) < seconds; };
  return budget;
}

struct Fp0Center {
  Node node;
  size_t incident_bad = 0;
};

Fp0Center Fp0BadNeighborhood(const Owned& cells) {
  std::map<Id, Fp0Center> candidates;
  for (const auto& row : cells) {
    const auto& cell = row.second.cell;
    Tensor mean{0, 0, 0, 0, 0, 0};
    Tetrahedron points;
    for (size_t k = 0; k < points.size(); ++k) {
      points[k] = cell.v[k].p;
      const auto& t = row.second.sensor[k];
      mean.xx += t.xx * .25;
      mean.xy += t.xy * .25;
      mean.xz += t.xz * .25;
      mean.yy += t.yy * .25;
      mean.yz += t.yz * .25;
      mean.zz += t.zz * .25;
    }
    const auto shape = Measure(points, mean);
    if (shape.mean_ratio >= .20 && shape.minimum_scaled_jacobian >= .05) continue;
    for (const auto& point : cell.v) {
      auto& candidate = candidates[point.id];
      candidate.node = point;
      ++candidate.incident_bad;
    }
  }
  if (candidates.empty()) return {cells.begin()->second.cell.v[0], 0};
  return std::max_element(candidates.begin(), candidates.end(),
                          [](const auto& a, const auto& b) { return a.second.incident_bad < b.second.incident_bad; })
      ->second;
}

struct Fp0Region {
  Owned mesh;
  std::set<Id> fixed;
  Id next_node = 0, next_cell = 0;
  double radius = 0;
};

Fp0Region Fp0Ball(const Owned& cells, Point center, size_t desired) {
  if (desired == 0 || cells.empty()) throw std::invalid_argument("FP0 optimizer region is empty.");
  Fp0Region region;
  std::vector<std::pair<double, Id>> distance;
  distance.reserve(cells.size());
  for (const auto& row : cells) {
    const auto p = Centroid(row.second.cell);
    const double x = p.x - center.x, y = p.y - center.y, z = p.z - center.z;
    distance.emplace_back(x * x + y * y + z * z, row.first);
    region.next_cell = std::max(region.next_cell, row.first + 1);
    for (const auto& node : row.second.cell.v) region.next_node = std::max(region.next_node, node.id + 1);
  }
  auto limit = distance.begin() + (std::min(desired, distance.size()) - 1);
  std::nth_element(distance.begin(), limit, distance.end());
  const double squared_radius = limit->first;
  region.radius = std::sqrt(squared_radius);
  std::map<Face, size_t> uses;
  for (const auto& row : distance) {
    if (row.first > squared_radius) continue;
    const auto& cell = cells.at(row.second);
    region.mesh.emplace(row.second, cell);
    for (size_t omitted = 0; omitted < 4; ++omitted) {
      Face face;
      size_t slot = 0;
      for (size_t k = 0; k < 4; ++k)
        if (k != omitted) face[slot++] = cell.cell.v[k].id;
      std::sort(face.begin(), face.end());
      ++uses[face];
    }
  }
  for (const auto& face : uses)
    if (face.second == 1) region.fixed.insert(face.first.begin(), face.first.end());
  return region;
}

long double Fp0EndpointLength(FrozenField& field, Point a, Point b) {
  const auto first = MetricNorm(a, b, field.Query(a).target);
  const auto last = MetricNorm(a, b, field.Query(b).target);
  const auto largest = std::max(first, last);
  const auto ratio = std::min(first, last) / largest;
  return largest * (2.L / 3) * (1 + ratio + ratio * ratio) / (1 + ratio);
}

void RunOptimizer(const ImportedGeometry& imported, const PlanarReference& geometry, size_t desired) {
  const auto started = Clock::now();
  const char* place = "choose region";
  size_t before = 0, after = 0;
  double optimizer_seconds = 0;
  OptimizeReport report;
  std::string error;
  try {
    const auto center = Fp0BadNeighborhood(imported.cells);
    auto region = Fp0Ball(imported.cells, center.node.p, desired);
    before = after = region.mesh.size();
    place = "build frozen target index";
    const auto components = AssociatedExtensions(imported.original, geometry);
    FrozenField field(imported.original, {}, false, components, {}, {}, true, imported.original.size());
    RegionMetric target([&](Point p) { return field.Query(p).target; },
                        [&](Point a, Point b) { return Fp0EndpointLength(field, a, b); });
    const size_t caller_bytes = geometry.ResidentBytes() + OwnedStorageBytes(imported.cells) +
                                OwnedStorageBytes(region.mesh) + 2 * imported.original.size() * sizeof(DonorCell);
    auto budget = Fp0AtlasBudget(imported.cells.size(), caller_bytes, started, 900);
    const auto borrowed = geometry.WithAtlasReservation(budget);
    OptimizeOptions options;
    options.maximum_work = 160;
    options.maximum_seconds = 900 - Seconds(started);
    options.cache_entries = 8 * MaximumReplacementCells;
    options.boundary = false;
    options.geometry = borrowed.get();
    options.surface_tolerance = geometry.Parameters()->GeometryTolerance();
    options.frozen_vertices = std::move(region.fixed);
    std::cout << "FP0 A9_setup {\"desired\":" << desired << ",\"cells\":" << before
              << ",\"center_id\":" << center.node.id << ",\"center_incident_bad\":" << center.incident_bad
              << ",\"radius\":" << region.radius << ",\"fixed_vertices\":" << options.frozen_vertices.size()
              << ",\"seconds\":" << Seconds(started) << "}" << std::endl;
    if (options.maximum_seconds <= 0) throw std::length_error("FP0 optimizer setup exhausted the 900 s wall bound.");
    struct RestoreAdmission {
      ShapeAdmission previous = GetShapeAdmission();
      ~RestoreAdmission() { SetShapeAdmission(previous); }
    } restore;
    SetShapeAdmission(ShapeAdmission::ValidityOnly);
    place = "OptimizeMetricMesh";
    const auto measure = Clock::now();
    try {
      OptimizeMetricMesh(region.mesh, target, region.next_node, region.next_cell, options, report);
    } catch (...) {
      optimizer_seconds = Seconds(measure);
      after = region.mesh.size();
      throw;
    }
    optimizer_seconds = Seconds(measure);
    after = region.mesh.size();
  } catch (const std::exception& e) {
    error = e.what();
  }
  const double seconds = Seconds(started);
  const size_t reconnects = report.edge_removals + report.length_removals + report.face_swaps + report.boundary_flips;
  const bool completed = error.empty() && report.stop != "atlas reservation missing";
  std::cout << "FP0 A9 {\"desired\":" << desired << ",\"cells\":" << before << ",\"final_cells\":" << after
            << ",\"seconds\":" << optimizer_seconds << ",\"total_seconds\":" << seconds
            << ",\"ns_per_cell\":" << (before ? optimizer_seconds * 1e9 / before : 0)
            << ",\"ms_per_cell\":" << (before ? optimizer_seconds * 1e3 / before : 0) << ",\"bad_before\":";
  if (completed || report.sweeps)
    std::cout << report.bad_before;
  else
    std::cout << "null";
  std::cout << ",\"bad_after\":";
  if (completed)
    std::cout << report.bad_after;
  else
    std::cout << "null";
  std::cout << ",\"moves\":" << report.moves << ",\"reconnects\":" << reconnects
            << ",\"edge_removals\":" << report.edge_removals << ",\"length_removals\":" << report.length_removals
            << ",\"face_swaps\":" << report.face_swaps << ",\"boundary_flips\":" << report.boundary_flips
            << ",\"collapses\":" << report.collapses << ",\"trials\":" << report.trials
            << ",\"work_limit\":160,\"stop\":";
  Fp0JsonString(report.stop);
  std::cout << ",\"place\":";
  Fp0JsonString(place);
  std::cout << ",\"error\":";
  Fp0JsonString(error);
  std::cout << "}" << std::endl;
}

void Fp0ProofEntity(const BoundaryParameterState::Certificates::value_type& row) {
  const auto& key = row.first;
  const auto& proof = row.second;
  std::cout << "{\"body\":" << std::get<0>(key) << ",\"sheet\":" << std::get<1>(key)
            << ",\"marker\":" << std::get<2>(key)
            << ",\"within_tolerance\":" << (proof.within_tolerance ? "true" : "false")
            << ",\"outside_tolerance\":" << (proof.outside_tolerance ? "true" : "false") << ",\"mesh_to_model\":";
  Fp0JsonNumber(proof.mesh_to_model);
  std::cout << ",\"model_to_mesh\":";
  Fp0JsonNumber(proof.model_to_mesh);
  std::cout << ",\"model_pieces\":" << proof.model_pieces << ",\"physical_pieces\":" << proof.physical_pieces << '}';
}

void RunSurfaceProof(const PlanarReference& reference) {
  const auto& state = *reference.Parameters();
  std::vector<SurfaceFace> surface;
  surface.reserve(state.Surface().size());
  for (const auto& row : state.Surface()) surface.push_back(row.second);
  const auto started = Clock::now();
  // This is the pinned FP0 event-one input mesh, including the interior cells that set the engine allowance.
  auto budget =
      Fp0AtlasBudget(726442, reference.ResidentBytes() + surface.capacity() * sizeof(SurfaceFace), started, 5400);
  const auto* certificates = static_cast<const BoundaryParameterState::Certificates*>(nullptr);
  const char* place = "OwnedFaceReverseBounds";
  double reverse_seconds = 0, full_seconds = 0;
  size_t reverse_work = 0;
  std::vector<double> reverse;
  std::string error;
  const auto stats_before = state.Geometry().GeometryStatsSnapshot();
  auto stats_reverse = stats_before;
  try {
    reverse = state.OwnedFaceReverseBounds(surface, budget);
    reverse_seconds = Seconds(started);
    reverse_work = budget.work;
    stats_reverse = state.Geometry().GeometryStatsSnapshot();
    place = "GeometryCertificates";
    const auto full_start = Clock::now();
    try {
      certificates = &state.GeometryCertificates(budget, true, &reverse);
    } catch (...) {
      full_seconds = Seconds(full_start);
      throw;
    }
    full_seconds = Seconds(full_start);
  } catch (const std::exception& e) {
    error = e.what();
    if (!reverse_seconds) {
      reverse_seconds = Seconds(started);
      reverse_work = budget.work;
      stats_reverse = state.Geometry().GeometryStatsSnapshot();
    }
  }
  const auto stats_final = state.Geometry().GeometryStatsSnapshot();
  const auto time = reverse_seconds + full_seconds;
  std::cout << "FP0 A11 {\"faces\":" << surface.size() << ",\"seconds\":" << time
            << ",\"ns_per_face\":" << (surface.empty() ? 0 : time * 1e9 / surface.size())
            << ",\"reverse_seconds\":" << reverse_seconds << ",\"reverse_work\":" << reverse_work
            << ",\"reverse_direct_calls\":" << stats_reverse.direct_calls - stats_before.direct_calls
            << ",\"reverse_direct_work\":" << stats_reverse.direct_work - stats_before.direct_work
            << ",\"reverse_full_calls\":" << stats_reverse.full_calls - stats_before.full_calls
            << ",\"reverse_full_work\":" << stats_reverse.full_work - stats_before.full_work
            << ",\"certificate_seconds\":" << full_seconds << ",\"certificate_work\":" << budget.work - reverse_work
            << ",\"certificate_full_calls\":" << stats_final.full_calls - stats_reverse.full_calls
            << ",\"certificate_full_work\":" << stats_final.full_work - stats_reverse.full_work
            << ",\"max_work\":" << budget.maximum_work
            << ",\"work_exhausted\":" << (budget.work_exhausted ? "true" : "false")
            << ",\"peak_atlas_bytes\":" << budget.peak_bytes << ",\"place\":";
  Fp0JsonString(place);
  std::cout << ",\"error\":";
  Fp0JsonString(error);
  std::cout << ",\"reverse_bounds\":[";
  for (size_t i = 0; i < reverse.size(); ++i) {
    if (i) std::cout << ',';
    Fp0JsonNumber(reverse[i]);
  }
  std::cout << "],\"entities\":[";
  if (certificates) {
    bool first = true;
    for (const auto& row : *certificates) {
      if (!first) std::cout << ',';
      Fp0ProofEntity(row);
      first = false;
    }
  }
  std::cout << "]}" << std::endl;
}

using Fp0Vec = std::array<double, 3>;
using Fp0Triangle = std::array<Fp0Vec, 3>;
using Fp0Net = std::array<Fp0Vec, 10>;

Fp0Vec Fp0Coordinates(Point point) { return {point.x, point.y, point.z}; }

Fp0Net Fp0Controls(const SU2NativeGeometry::MappedFacetReference::CubicNet& net) {
  Fp0Net result;
  std::transform(net.begin(), net.end(), result.begin(), Fp0Coordinates);
  return result;
}

double Fp0Distance(const Fp0Vec& a, const Fp0Vec& b) { return std::hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]); }

struct Fp0OrderCheck {
  size_t samples = 0, mismatches = 0;
  double greatest_difference = 0, seconds = 0;
};

Fp0OrderCheck Fp0CheckCubicOrder(const SU2NativeGeometry::MappedFacetReference& geometry) {
  const auto started = Clock::now();
  Fp0OrderCheck check;
  auto facet = geometry.Curves().begin();
  const size_t count = geometry.Curves().size();
  if (!count) throw std::invalid_argument("FP0 reference has no cubic controls.");
  for (size_t i = 0, previous = 0; i < 1000; ++i) {
    const size_t offset = i * count / 1000;
    std::advance(facet, offset - previous);
    previous = offset;
    double u = (double(i % 31) + .5) / 31, v = (double(i * 13 % 37) + .25) / 37;
    if (u + v >= 1) {
      u = 1 - u;
      v = 1 - v;
    }
    const auto result = SU2NativeCurved::detail::TrianglePatch(Fp0Controls(facet->second), 1 - u - v, u, v);
    const auto exact =
        Fp0Coordinates(geometry.EvaluateSurface(facet->first, geometry.FacetPoint(facet->first, u, v)).point);
    const double difference = Fp0Distance(result, exact);
    const double scale = std::max(1., std::hypot(exact[0], exact[1], exact[2]));
    ++check.samples;
    check.greatest_difference = std::max(check.greatest_difference, difference);
    check.mismatches += difference > 64 * std::numeric_limits<double>::epsilon() * scale;
  }
  check.seconds = Seconds(started);
  return check;
}

struct Fp0FaceSample {
  Fp0Net controls;
  Fp0Triangle real, parameter, anchor;
  bool mapped = false;
};

Fp0FaceSample Fp0PrepareSurfaceFace(const BoundaryParameterState& state, const SurfaceFace& face) {
  const auto& geometry = state.Geometry();
  Fp0FaceSample result;
  result.controls = Fp0Controls(geometry.Curves().at(face.facet));
  result.mapped = true;
  for (size_t k = 0; k < 3; ++k) {
    const auto& point = *state.Find(face.v[k]);
    result.real[k] = Fp0Coordinates(point.node.p);
    bool incident = false;
    geometry.ForFacets(point.binding, [&](Id side) { incident = incident || side == face.facet; });
    result.mapped = result.mapped && incident;
  }
  if (result.mapped) {
    for (size_t k = 0; k < 3; ++k)
      result.parameter[k] = Fp0Coordinates(geometry.SurfaceCoordinates(face.facet, state.Find(face.v[k])->binding));
  } else {
    const auto& facet = geometry.Reference().Get(face.facet);
    for (size_t k = 0; k < 3; ++k) result.anchor[k] = Fp0Coordinates(facet.v[k].p);
  }
  return result;
}

Fp0Vec Fp0Interpolate(const Fp0Triangle& triangle, const Fp0Vec& weights) {
  Fp0Vec result{};
  for (size_t k = 0; k < 3; ++k)
    for (size_t axis = 0; axis < 3; ++axis) result[axis] += triangle[k][axis] * weights[k];
  return result;
}

double Fp0ThinTriangleDistance(const Fp0Vec& point, const Fp0Triangle& triangle) {
  using SU2NativeCurved::Axpy;
  using SU2NativeCurved::Cross;
  using SU2NativeCurved::Dot;
  using SU2NativeCurved::Sub;
  const auto ab = Sub<3>(triangle[1], triangle[0]), ac = Sub<3>(triangle[2], triangle[0]);
  const auto ap = Sub<3>(point, triangle[0]);
  const auto normal = Cross(ab, ac);
  const double normal_squared = Dot<3>(normal, normal);
  if (normal_squared > 0) {
    const double u = Dot<3>(Cross(ap, ac), normal) / normal_squared;
    const double v = Dot<3>(Cross(ab, ap), normal) / normal_squared;
    if (u >= 0 && v >= 0 && u + v <= 1) return std::abs(Dot<3>(ap, normal)) / std::sqrt(normal_squared);
  }
  double closest = std::numeric_limits<double>::infinity();
  for (size_t i = 0; i < 3; ++i) {
    const auto& a = triangle[i];
    const auto chord = Sub<3>(triangle[(i + 1) % 3], a);
    const double squared = Dot<3>(chord, chord);
    const double along = squared > 0 ? std::clamp(Dot<3>(Sub<3>(point, a), chord) / squared, 0., 1.) : 0.;
    closest = std::min(closest, Fp0Distance(point, Axpy<3>(a, along, chord)));
  }
  return closest;
}

double Fp0CubicDistance(const Fp0FaceSample& face, const Fp0Vec& weight, size_t& projection_retries) {
  using SU2NativeCurved::detail::ClosestOnTriangle;
  double u, v;
  Fp0Vec bary;
  if (face.mapped) {
    bary = Fp0Interpolate(face.parameter, weight);
  } else {
    ClosestOnTriangle(Fp0Interpolate(face.real, weight), face.anchor[0], face.anchor[1], face.anchor[2], u, v);
    bary = {1 - u - v, u, v};
  }
  const auto point = SU2NativeCurved::detail::TrianglePatch(face.controls, bary[0], bary[1], bary[2]);
  if (!std::all_of(point.begin(), point.end(), [](double value) { return std::isfinite(value); }))
    throw std::runtime_error("Nonfinite FP0 cubic point.");
  const double distance = Fp0Distance(point, ClosestOnTriangle(point, face.real[0], face.real[1], face.real[2], u, v));
  if (std::isfinite(distance)) return distance;
  // The closest-point formula can cancel its denominator on a very thin realized triangle.
  ++projection_retries;
  return Fp0ThinTriangleDistance(point, face.real);
}

struct Fp0DistanceStats {
  size_t faces = 0, above_half = 0, above_full = 0;
  double sum = 0, maximum = 0;
  void Add(double peak, double sample_sum) {
    ++faces;
    above_half += peak > .5e-4;
    above_full += peak > 1e-4;
    maximum = std::max(maximum, peak);
    sum += sample_sum;
  }
  void Print() const {
    std::cout << "{\"faces\":" << faces << ",\"max_distance\":" << maximum
              << ",\"mean_distance\":" << (faces ? sum / (4 * faces) : 0) << ",\"above_0_5e_4\":" << above_half
              << ",\"above_1e_4\":" << above_full << '}';
  }
};

void RunFloatSurface(const PlanarReference& reference) {
  const auto& state = *reference.Parameters();
  const auto order = Fp0CheckCubicOrder(state.Geometry());
  const auto setup = Clock::now();
  std::vector<Fp0FaceSample> faces;
  faces.reserve(state.Surface().size());
  for (const auto& row : state.Surface()) faces.push_back(Fp0PrepareSurfaceFace(state, row.second));
  const double setup_seconds = Seconds(setup);
  const auto started = Clock::now();
  Fp0DistanceStats all, bound, projected;
  size_t projection_retries = 0;
  constexpr std::array<Fp0Vec, 4> weights{{{1. / 3, 1. / 3, 1. / 3}, {.5, .5, 0}, {0, .5, .5}, {.5, 0, .5}}};
  for (const auto& face : faces) {
    double peak = 0, sum = 0;
    for (const auto& weight : weights) {
      const double distance = Fp0CubicDistance(face, weight, projection_retries);
      peak = std::max(peak, distance);
      sum += distance;
    }
    all.Add(peak, sum);
    (face.mapped ? bound : projected).Add(peak, sum);
  }
  const double seconds = Seconds(started);
  std::cout << "FP0 A12 {\"faces\":" << faces.size() << ",\"seconds\":" << seconds
            << ",\"ns_per_face\":" << (faces.empty() ? 0 : seconds * 1e9 / faces.size())
            << ",\"setup_seconds\":" << setup_seconds << ",\"order_seconds\":" << order.seconds
            << ",\"order_samples\":" << order.samples << ",\"order_mismatches\":" << order.mismatches
            << ",\"order_max_difference\":" << order.greatest_difference
            << ",\"projection_retries\":" << projection_retries << ",\"all\":";
  all.Print();
  std::cout << ",\"anchor_forward_bindings\":";
  bound.Print();
  std::cout << ",\"flat_anchor_projection\":";
  projected.Print();
  std::cout << "}" << std::endl;
}
