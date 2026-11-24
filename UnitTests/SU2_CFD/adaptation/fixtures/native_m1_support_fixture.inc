/*!
 * \file native_m1_support_fixture.inc
 * \brief Focused input trace for the FP2-M1 bound support observer.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
namespace {
constexpr size_t M1StripPanels = 24;
constexpr size_t M1FixtureCeiling = size_t{1} << 28;
constexpr double M1PanelWidth = 1. / 64;
constexpr double M1FixtureTolerance = native_fp2_fixture::Tolerance;

Node M1StripNode(size_t x, size_t y, bool bottom = false) {
  return {4 * x + 2 * y + bottom,
          {x * M1PanelWidth, y * M1PanelWidth, bottom ? -M1PanelWidth : 0.}};
}

Face M1SortedFace(Face ids) {
  std::sort(ids.begin(), ids.end());
  return ids;
}

struct M1Strip {
  Owned cells;
  ReferenceState reference;
  std::map<Face, Id> facet_ids;

  M1Strip() {
    // Six tetrahedra around each body diagonal give the same face split
    // on both sides of each panel join.
    constexpr std::array<size_t, 6> ring{1, 2, 3, 7, 4, 5};
    std::map<Face, std::vector<std::pair<Id, unsigned>>> uses;
    for (size_t x = 0; x < M1StripPanels; ++x) {
      const std::array<Node, 8> corners{
          M1StripNode(x, 0, true), M1StripNode(x + 1, 0, true), M1StripNode(x + 1, 1, true),
          M1StripNode(x, 1, true), M1StripNode(x, 0), M1StripNode(x + 1, 0), M1StripNode(x + 1, 1),
          M1StripNode(x, 1)};
      for (size_t k = 0; k < ring.size(); ++k) {
        OwnedCell row;
        row.cell.id = cells.size();
        row.cell.v = {corners[0], corners[6], corners[ring[k]], corners[ring[(k + 1) % ring.size()]]};
        auto& v = row.cell.v;
        if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) < 0) std::swap(v[0], v[1]);
        REQUIRE(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0);
        row.sensor_present = true;
        for (unsigned f = 0; f < 4; ++f) {
          const auto face = Oriented(row.cell, f);
          uses[M1SortedFace({face[0].id, face[1].id, face[2].id})].emplace_back(row.cell.id, f);
        }
        cells.emplace(row.cell.id, row);
      }
    }

    reference.marker_names = {"top", "sides"};
    for (const auto& [key, use] : uses) {
      REQUIRE(use.size() <= 2);
      if (use.size() == 2) continue;
      const auto [cell, side] = use.front();
      auto& row = cells.at(cell);
      const auto nodes = Oriented(row.cell, side);
      const bool top = std::all_of(nodes.begin(), nodes.end(), [](const auto& node) { return node.p.z == 0; });
      const auto facet = reference.facets.size();
      reference.facets.push_back({facet, top ? 0 : 1, nodes});
      row.physical[side] = {true, facet};
      facet_ids.emplace(key, facet);
    }
    SetReferenceGeometry();
  }

 private:
  static std::array<Node, 3> Oriented(const Cell& cell, unsigned side) {
    const auto slots = local::OrientedFace({0, 1, 2, 3}, side);
    return {cell.v[slots[0]], cell.v[slots[1]], cell.v[slots[2]]};
  }

  void SetReferenceGeometry() {
    using SU2NativeGeometry::MappedFacetReference;
    auto definition = std::make_shared<PlanarReference>(reference.facets);
    std::map<Id, double> errors;
    MappedFacetReference::CurvedNets nets;
    constexpr std::array<std::array<unsigned, 3>, 10> powers{{
        {3, 0, 0}, {0, 3, 0}, {0, 0, 3}, {2, 1, 0}, {1, 2, 0},
        {0, 2, 1}, {0, 1, 2}, {1, 0, 2}, {2, 0, 1}, {1, 1, 1}}};
    for (const auto& facet : reference.facets) {
      errors.emplace(facet.id, 1e-12);
      auto& net = nets[facet.id];
      for (size_t i = 0; i < powers.size(); ++i) {
        auto& control = net[i];
        for (size_t k = 0; k < 3; ++k) {
          control.x += powers[i][k] * facet.v[k].p.x;
          control.y += powers[i][k] * facet.v[k].p.y;
          control.z += powers[i][k] * facet.v[k].p.z;
        }
        control = {control.x / 3, control.y / 3, control.z / 3};
      }
      // Only the interior control moves. The two curved top facets in
      // every panel keep matching edge controls with every neighbour.
      if (facet.marker == 0) net[9].z = M1PanelWidth / 65536;
    }
    auto authority = std::make_shared<MappedFacetReference>(definition, errors, 1, M1FixtureCeiling,
                                                             false, nets);
    auto budget = native_fp2_fixture::Budget(cells.size(), 0, Clock::now(), 5400, M1FixtureCeiling);
    SU2NativeGeometry::MobilityPolicy policy;
    std::string reason;
    const bool built = authority->BuildMobilityPolicy(45, {{0, 2}, {1, 2}}, policy, budget, reason);
    INFO(reason);
    REQUIRE(built);
    BoundaryPoints points;
    std::vector<SurfaceFace> surface;
    for (const auto& f : reference.facets) {
      surface.push_back({{f.v[0].id, f.v[1].id, f.v[2].id}, f.id});
      for (const auto& node : f.v) points.emplace(node.id, BoundaryPoint{node, authority->Vertex(node.id)});
    }
    const BoundaryParameterState initial(authority, points, surface, 1, M1FixtureCeiling, 0, {}, policy, &budget);
    reference.original = PlanarReference::WithParameters(definition, initial.WithEntityGeometry(
        M1FixtureTolerance, budget, M1FixtureCeiling));
  }
};

auto M1Point(const Point& point) { return std::tie(point.x, point.y, point.z); }

auto M1Binding(const SU2NativeGeometry::SurfaceBinding& binding) {
  return std::tie(binding.dimension, binding.feature, binding.version, binding.u, binding.v);
}
}  // namespace

TEST_CASE("FP2-M1 curved support copies the exact production bound inputs", "[.][NativeFP2M1Support]") {
  const World world;
  REQUIRE(world.size == 1);
  const char* trace = std::getenv("SU2_NATIVE_GEOMETRY_WORK_TRACE");
  REQUIRE(trace != nullptr);
  REQUIRE(std::string(trace) == "1");
  const auto start = Clock::now();
  M1Strip strip;
  const std::string sidecar = "fp2_m1_curved_strip.native_ref";
  WriteReference(sidecar, strip.reference, M1FixtureCeiling);

  ReferenceState reference;
  bool loaded = false;
  std::string reason;
  size_t load_peak = 0;
  const bool restored = LoadReference(sidecar, reference, loaded, reason, M1FixtureCeiling, 0, load_peak);
  INFO(reason);
  REQUIRE(restored);
  REQUIRE(loaded);
  REQUIRE(reference.original);
  REQUIRE(reference.original->Parameters());
  local::Mesh mesh(strip.cells);
  auto budget = native_fp2_fixture::Budget(strip.cells.size(), reference.original->ResidentBytes(),
                                          start, 5400, M1FixtureCeiling);
  local::StateSurfaceGeometry surface(reference.original, std::move(budget));
  LabM1Shadow shadow;
  REQUIRE(shadow.Load(sidecar, surface.State(), M1FixtureCeiling, M1FixtureCeiling));
  const auto& authority = shadow.Geometry();
  REQUIRE(&authority != &surface.State().Geometry());
  REQUIRE(std::count_if(reference.facets.begin(), reference.facets.end(), [](const auto& facet) {
    return facet.marker == 0;
  }) == 2 * M1StripPanels);

  LabM1Observer observer(mesh, surface, shadow, strip.cells.size(), M1FixtureCeiling, start, world.rank, 1, true);
  bool witnessed_wrong_anchor = false;
  for (size_t x = 2; x < 22; ++x) {
    CAPTURE(x);
    const auto first = M1StripNode(x, 0), second = M1StripNode(x + 2, 0), third = M1StripNode(x + 1, 1);
    const auto next = M1StripNode(x + 1, 0);
    const auto facet = strip.facet_ids.at(M1SortedFace({first.id, next.id, third.id}));
    // Put the interior vertex first in the raw order. Cyclic ordering for
    // production puts the min ID (the left endpoint) first without reflection.
    const SurfaceFace face{{third.id, first.id, second.id}, facet};
    observer.BeforeForward(face);
    const auto& copy = observer.Input();
    REQUIRE_FALSE(copy.direct);
    REQUIRE(copy.facet == facet);
    REQUIRE((copy.ids == Face{first.id, second.id, third.id}));
    REQUIRE(M1Point(copy.anchor) == M1Point(first.p));
    REQUIRE(M1Binding(copy.bindings[0]) == M1Binding(authority.Vertex(first.id)));
    REQUIRE(copy.radius == std::nextafter(2 * M1PanelWidth + M1FixtureTolerance, INFINITY));
    REQUIRE(copy.tol == M1FixtureTolerance);
    REQUIRE_FALSE(copy.small1.empty());
    REQUIRE(copy.small1.size() < copy.full.size());
    if (x == 4) {
      // In panel widths, the unrotated vertex lies sqrt(2) from either
      // endpoint. Its x box drops panels reached by the true radius of two.
      const double wrong = std::nextafter(std::sqrt(2.) * M1PanelWidth + M1FixtureTolerance, INFINITY);
      const Point lo{std::nextafter(first.p.x - wrong, -INFINITY), std::nextafter(-wrong, -INFINITY),
                     std::nextafter(-wrong, -INFINITY)};
      const Point hi{std::nextafter(second.p.x + wrong, INFINITY), std::nextafter(M1PanelWidth + wrong, INFINITY),
                     std::nextafter(wrong, INFINITY)};
      auto wrong_support = authority.NearbySurfaceFacets(facet, {lo, hi});
      const auto entity = surface.State().Entity(facet);
      wrong_support.erase(std::remove_if(wrong_support.begin(), wrong_support.end(), [&](Id candidate) {
        return surface.State().Entity(candidate) != entity;
      }), wrong_support.end());
      REQUIRE_FALSE(wrong_support.empty());
      REQUIRE(wrong_support.size() < copy.full.size());
      REQUIRE(std::includes(copy.full.begin(), copy.full.end(), wrong_support.begin(), wrong_support.end()));
      witnessed_wrong_anchor = true;
    }

    observer.ForwardBegin();
    const auto query_start = Clock::now();
    const bool answer = surface.WithinTolerance(face);
    observer.AfterForward(face, Seconds(query_start), answer);
  }
  REQUIRE(witnessed_wrong_anchor);
  REQUIRE(observer.Proved1() > 0);
  REQUIRE(observer.ReplicaAttempted() > 0);
  std::ostringstream rows;
  observer.Report(rows);
  std::cout << rows.str() << std::flush;
}
