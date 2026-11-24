/*!
 * \file native_m2_model.inc
 * \brief FP2-M2 forward differential geometry and seam pins of the immutable reference.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

namespace {
struct M2PatchSample {
  Point point, normal;
};

M2PatchSample M2AtEdge(const Facet& facet, const SU2NativeGeometry::MappedFacetReference::CubicNet& controls,
                      Edge edge, double t) {
  using SU2NativeCurved::Vec;
  std::array<Vec<3>, 10> net;
  for (size_t i = 0; i < net.size(); ++i) net[i] = {controls[i].x, controls[i].y, controls[i].z};
  std::array<double, 3> weight{};
  for (size_t i = 0; i < 3; ++i) {
    if (facet.v[i].id == edge[0]) weight[i] = 1 - t;
    if (facet.v[i].id == edge[1]) weight[i] = t;
  }
  const auto point = SU2NativeCurved::detail::TrianglePatch(net, weight[0], weight[1], weight[2]);
  Vec<3> du{}, dv{};
  SU2NativeCurved::detail::TriangleDerivatives(net, weight[1], weight[2], du, dv);
  return {{point[0], point[1], point[2]}, M2Cross({du[0], du[1], du[2]}, {dv[0], dv[1], dv[2]})};
}

Point M2FacetVertex(const Facet& facet, Id id) {
  for (const auto& vertex : facet.v)
    if (vertex.id == id) return vertex.p;
  throw std::logic_error("FP2-M2 reference edge absent from its incident facet.");
}

struct M2Incidence {
  std::array<const Facet*, 2> facets{};
  size_t count = 0;
};

struct M2ModelCounts {
  size_t edges = 0, other_entity_edges = 0, open_edges = 0, nonmanifold_edges = 0;
  size_t missing_angle_edges = 0, no_finite_angle_edges = 0, eval_exceptions = 0;
  size_t seam_checks = 0, endpoint_checks = 0;
  double seam_max_error = 0, endpoint_max_error = 0;
  bool pins_ok = true;
};

void M2SeamPin(double error, double limit, bool endpoint, M2ModelCounts& counts) {
  auto& max_error = endpoint ? counts.endpoint_max_error : counts.seam_max_error;
  auto& checks = endpoint ? counts.endpoint_checks : counts.seam_checks;
  max_error = std::isfinite(error) ? std::max(max_error, error) : std::numeric_limits<double>::infinity();
  ++checks;
  counts.pins_ok = counts.pins_ok && error <= limit;
}

void M2ModelEdge(std::ostream& out, const BoundaryParameterState& state, Edge edge, const Facet& first,
                 const Facet& second, double pin, M2ModelCounts& counts) {
  const auto& curves = state.Geometry().Curves();
  const auto& first_net = curves.at(first.id);
  const auto& second_net = curves.at(second.id);
  const Point a = M2FacetVertex(first, edge[0]), b = M2FacetVertex(first, edge[1]);
  const auto entity = state.Entity(first.id);
  const auto midpoint = M2Midpoint(a, b);
  out << edge[0] << ',' << edge[1] << ',' << first.id << ',' << second.id;
  M2CsvEntity(out, entity);
  M2CsvPoint(out, midpoint);
  out << ',' << M2Distance(a, b) << ',' << M2Bin(std::get<2>(entity), midpoint);
  std::optional<double> kink_max;
  size_t finite = 0;
  for (size_t i = 0; i < 5; ++i) {
    const auto lhs = M2AtEdge(first, first_net, edge, .25 * i);
    const auto rhs = M2AtEdge(second, second_net, edge, .25 * i);
    M2SeamPin(M2Distance(lhs.point, rhs.point), pin, false, counts);
    if (i == 0 || i == 4) {
      const auto named = i == 0 ? a : b;
      M2SeamPin(M2Distance(lhs.point, named), pin, true, counts);
      M2SeamPin(M2Distance(rhs.point, named), pin, true, counts);
    }
    const auto angle = M2Angle(lhs.normal, rhs.normal);
    M2CsvValue(out, angle);
    if (angle) {
      kink_max = kink_max ? std::max(*kink_max, *angle) : *angle;
      ++finite;
    }
  }
  M2CsvValue(out, kink_max);
  out << '\n';
  ++counts.edges;
  counts.missing_angle_edges += finite < 5;
  counts.no_finite_angle_edges += finite == 0;
}

void M2ModelFacet(std::ostream& out, const BoundaryParameterState& state, const Facet& facet,
                  Clock::time_point start, M2ModelCounts& counts) {
  const auto& geometry = state.Geometry();
  const auto entity = state.Entity(facet.id);
  const std::array<Point, 3> p{facet.v[0].p, facet.v[1].p, facet.v[2].p};
  const double longest = std::max({M2Distance(p[0], p[1]), M2Distance(p[1], p[2]), M2Distance(p[2], p[0])});
  out << facet.id;
  M2CsvEntity(out, entity);
  out << ',' << M2Bin(std::get<2>(entity), M2Centroid(p)) << ',' << longest
      << ',' << geometry.StructuralEstimate(facet.id).control_deviation;
  for (size_t i = 0; i < 4; ++i) {
    M2CheckDeadline(start);
    std::optional<SU2NativeGeometry::MappedFacetReference::SurfaceEvaluation> evaluation;
    try {
      const auto binding = i == 0 ? geometry.FacetPoint(facet.id, 1. / 3, 1. / 3) : geometry.Vertex(facet.v[i - 1].id);
      evaluation = geometry.EvaluateSurface(facet.id, binding);
    } catch (const std::bad_alloc&) {
      throw;
    } catch (const std::exception&) {
      ++counts.eval_exceptions;
    }
    M2CheckDeadline(start);
    if (evaluation) out << ',' << evaluation->curvature1 << ',' << evaluation->curvature2;
    else out << ",,";
  }
  out << '\n';
}
}  // namespace

TEST_CASE("FP2-M2 immutable reference model", "[.][NativeFP2M2Model]") {
  const auto start = Clock::now();
  World world;
  REQUIRE(world.size == 1);
  const auto* path = std::getenv("SU2_NATIVE_FP2_M2_REFERENCE");
  const auto* ceiling_env = std::getenv("SU2_NATIVE_FP2_M2_CEILING");
  REQUIRE(path != nullptr);
  REQUIRE(ceiling_env != nullptr);
  const size_t ceiling = size_t{12288} * 1024 * 1024;
  REQUIRE(std::to_string(ceiling) == ceiling_env);
  ReferenceState reference;
  bool loaded = false;
  size_t load_peak = 0;
  std::string reason;
  const bool admitted = LoadReference(path, reference, loaded, reason, ceiling, 0, load_peak);
  INFO(reason);
  REQUIRE(admitted);
  REQUIRE(loaded);
  native_fp2_fixture::PrepareIN1(reference, ceiling);
  const auto& state = *reference.original->Parameters();
  const auto& facets = state.Geometry().Reference().Facets();
  REQUIRE_FALSE(facets.empty());
  REQUIRE(state.Geometry().Curves().size() == facets.size());

  std::map<Edge, M2Incidence> incidences;
  M2CheckStorage(ceiling, transfer_memory::Add(ReferenceStorageBytes(reference),
      M2MapBytes<decltype(incidences)>(transfer_memory::Mul(3, facets.size())), size_t{1} << 20));
  Point lo = facets.begin()->second.v.front().p, hi = lo;
  for (const auto& entry : facets) {
    M2CheckDeadline(start);
    const auto& facet = entry.second;
    for (size_t i = 0; i < 3; ++i) {
      const auto& v = facet.v[i];
      lo = {std::min(lo.x, v.p.x), std::min(lo.y, v.p.y), std::min(lo.z, v.p.z)};
      hi = {std::max(hi.x, v.p.x), std::max(hi.y, v.p.y), std::max(hi.z, v.p.z)};
      auto& adjacent = incidences[M2Edge(v.id, facet.v[(i + 1) % 3].id)];
      if (adjacent.count < adjacent.facets.size()) adjacent.facets[adjacent.count] = &facet;
      ++adjacent.count;
    }
  }
  const double pin = 1e-12 * M2Distance(lo, hi);

  std::ofstream edge_file, facet_file;
  M2Output(edge_file, "model_edges.csv");
  M2Output(facet_file, "model_facets.csv");
  edge_file << "v0,v1,facet0,facet1,body,sheet,marker,midpoint_x,midpoint_y,midpoint_z,length,bin,"
               "kink_t0,kink_t25,kink_t50,kink_t75,kink_t100,kink_max\n";
  facet_file << "facet,body,sheet,marker,bin,longest_edge,control_deviation,curvature1_centroid,curvature2_centroid,"
                "curvature1_0,curvature2_0,curvature1_1,curvature2_1,curvature1_2,curvature2_2\n";
  M2ModelCounts counts;
  for (const auto& [edge, adjacent] : incidences) {
    M2CheckDeadline(start);
    if (adjacent.count == 1) {
      ++counts.open_edges;
    } else if (adjacent.count != 2) {
      ++counts.nonmanifold_edges;
    } else if (state.Entity(adjacent.facets[0]->id) != state.Entity(adjacent.facets[1]->id)) {
      ++counts.other_entity_edges;
    } else {
      M2ModelEdge(edge_file, state, edge, *adjacent.facets[0], *adjacent.facets[1], pin, counts);
    }
  }
  for (const auto& entry : facets) M2ModelFacet(facet_file, state, entry.second, start, counts);
  edge_file.close();
  facet_file.close();
  std::ostringstream report;
  report << std::setprecision(17) << "FP2_M2 MODEL rank=0 status=" << (counts.pins_ok ? "complete" : "failed")
         << " facets=" << facets.size() << " edges=" << counts.edges << " other_entity_edges=" << counts.other_entity_edges
         << " open_edges=" << counts.open_edges << " nonmanifold_edges=" << counts.nonmanifold_edges
         << " missing_angle_edges=" << counts.missing_angle_edges << " no_finite_angle_edges=" << counts.no_finite_angle_edges
         << " eval_exceptions=" << counts.eval_exceptions << " seam_checks=" << counts.seam_checks
         << " endpoint_checks=" << counts.endpoint_checks << " seam_max_error=" << counts.seam_max_error
         << " endpoint_max_error=" << counts.endpoint_max_error << " pin_tolerance=" << pin
         << " seconds=" << Seconds(start) << '\n';
  std::cout << report.str() << std::flush;
  INFO(report.str());
  REQUIRE(counts.edges > 0);
  REQUIRE(counts.pins_ok);
}
