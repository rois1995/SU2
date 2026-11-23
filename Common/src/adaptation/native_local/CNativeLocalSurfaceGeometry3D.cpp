/*!
 * \file CNativeLocalSurfaceGeometry3D.cpp
 * \brief Float surface walk, exact feature traces and validated surface state rebuild.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalSurface3D.hpp"
#include "../../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../include/adaptation/CNativeCurvedReference.hpp"
#include "../../../include/adaptation/CNativeTimerTree3D.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>

namespace SU2NativeLocal3D {
namespace {
using BoundaryState = SU2Native3D::BoundaryParameterState;
using SU2NativeGeometry::AtlasBudget;
using SU2NativeGeometry::BoundaryCoverage;
using SU2NativeGeometry::EdgeTrace;
using SU2Native3D::PlanarReference;
using SU2Native3D::Patch;
using transfer_memory::Add;
using transfer_memory::Mul;
using Triangle = std::array<BoundaryPoint, 3>;
using Scope = SU2NativeTiming3D::Scope;
using Operation = SU2NativeTiming3D::Operation;

thread_local size_t* query_scratch_maximum = nullptr;

class QueryPeak {
 public:
  explicit QueryPeak(AtlasBudget& budget)
      : budget_(budget), maximum_(query_scratch_maximum), historical_(budget.peak_bytes),
        baseline_(budget.caller_bytes) {
    if (maximum_) budget_.peak_bytes = baseline_;
  }

  ~QueryPeak() {
    if (!maximum_) return;
    *maximum_ = std::max(*maximum_, budget_.peak_bytes - baseline_);
    budget_.peak_bytes = std::max(historical_, budget_.peak_bytes);
  }

 private:
  AtlasBudget& budget_;
  size_t* maximum_;
  size_t historical_, baseline_;
};

template <class Container>
size_t MapBytes(const Container& rows) {
  return Mul(rows.size(), sizeof(typename Container::value_type) + 4 * sizeof(void*));
}

using WalkWeights = std::array<double, 3>;
using WalkNet = std::array<WalkWeights, 10>;

WalkWeights Values(Point p) { return {p.x, p.y, p.z}; }
using SU2NativeGeometry::MappedFacetReference;
using SU2NativeGeometry::SurfaceBinding;

struct WalkStart {
  Id facet = UINT64_MAX;
  double u = 0, v = 0;
};

struct WalkResult {
  Id facet = UINT64_MAX;
  double u = 0, v = 0, distance = std::numeric_limits<double>::infinity();
  bool converged = false;
};

double WalkDistance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z); }

class Walker {
 public:
  explicit Walker(const BoundaryState& state) : state(state), geometry(state.Geometry()) {
    std::map<SurfaceEdge, std::pair<Id, unsigned>> first;
    for (const auto& [id, face] : geometry.Reference().Facets()) {
      auto& row = facets[id];
      row.entity = state.Entity(id);
      for (unsigned k = 0; k < 3; ++k) row.vertices[k] = face.v[k].id;
      const auto net = geometry.Curves().find(id);
      row.curved = net != geometry.Curves().end();
      for (unsigned k = 0; k < 3; ++k) row.net[k] = Values(face.v[k].p);
      if (row.curved)
        for (unsigned k = 0; k < row.net.size(); ++k) row.net[k] = Values(net->second[k]);
      for (unsigned k = 0; k < 3; ++k) {
        SurfaceEdge edge{row.vertices[(k + 1) % 3], row.vertices[(k + 2) % 3]};
        if (edge[0] > edge[1]) std::swap(edge[0], edge[1]);
        if (state.Mobility().features.count(edge)) continue;
        const auto [where, inserted] = first.emplace(edge, std::make_pair(id, k));
        if (inserted) continue;
        const auto [other, opposite] = where->second;
        if (facets.at(other).entity != row.entity) continue;
        row.neighbors[k] = other;
        facets.at(other).neighbors[opposite] = id;
      }
    }
  }

  size_t ResidentBytes() const { return MapBytes(facets); }

  WalkStart Start(const SurfaceFace& face, const Triangle& triangle, Point query, const WalkWeights& weights) const {
    if (std::all_of(triangle.begin(), triangle.end(),
                    [&](const auto& point) { return geometry.Supports(face.facet, point.binding); }))
      return Interpolated(face.facet, triangle, weights);

    const auto nearest = std::min_element(triangle.begin(), triangle.end(), [&](const auto& a, const auto& b) {
      const auto left = WalkDistance(a.node.p, query), right = WalkDistance(b.node.p, query);
      return left == right ? a.node.id < b.node.id : left < right;
    });
    Id oriented = UINT64_MAX, equal = UINT64_MAX;
    const auto entity = facets.at(face.facet).entity;
    const std::array<Point, 3> corners{triangle[0].node.p, triangle[1].node.p, triangle[2].node.p};
    geometry.ForFacets(nearest->binding, [&](Id side) {
      if (facets.at(side).entity != entity) return;
      equal = std::min(equal, side);
      if (geometry.OrientedFace(side, nearest->binding, corners)) oriented = std::min(oriented, side);
    });
    const Id side = oriented == UINT64_MAX ? equal : oriented;
    if (side == UINT64_MAX) return {};
    const auto uv = geometry.SurfaceCoordinates(side, nearest->binding);
    return {side, uv.y, uv.z};
  }

  WalkStart EdgeStart(const SurfaceFace& face, const Triangle& triangle, unsigned first, unsigned second,
                      Point query) const {
    WalkWeights weights{};
    weights.at(first) = weights.at(second) = .5;
    const auto& a = triangle.at(first).binding;
    const auto& b = triangle.at(second).binding;
    if (geometry.Supports(face.facet, a) && geometry.Supports(face.facet, b))
      return Interpolated(face.facet, triangle, weights);
    Id side = UINT64_MAX;
    const auto entity = facets.at(face.facet).entity;
    geometry.ForFacets(a, [&](Id candidate) {
      if (facets.at(candidate).entity == entity && geometry.Supports(candidate, b)) side = std::min(side, candidate);
    });
    return side == UINT64_MAX ? Start(face, triangle, query, weights) : Interpolated(side, triangle, weights);
  }

  WalkResult Run(Point query, WalkStart start) const {
    if (start.facet == UINT64_MAX) return {};
    auto best = Minimize(query, start.facet, start.u, start.v);
    std::vector<Id> visited{start.facet};
    size_t changes = 0;
    constexpr size_t maximum_changes = 64;
    while (changes < maximum_changes) {
      const auto& facet = facets.at(best.facet);
      const WalkWeights weights{1 - best.u - best.v, best.u, best.v};
      WalkResult next;
      next.distance = best.distance;
      auto consider = [&](Id candidate) {
        if (candidate == UINT64_MAX || std::find(visited.begin(), visited.end(), candidate) != visited.end()) return;
        visited.push_back(candidate);
        const auto& vertices = facets.at(candidate).vertices;
        WalkWeights mapped{};
        for (unsigned i = 0; i < 3; ++i)
          for (unsigned j = 0; j < 3; ++j)
            if (facet.vertices[i] == vertices[j]) mapped[j] = weights[i];
        auto hit = Minimize(query, candidate, mapped[1], mapped[2]);
        if (hit.distance < next.distance) next = hit;
      };

      const auto zero = std::count(weights.begin(), weights.end(), 0.);
      if (zero == 0) break;
      if (zero == 1) {
        const auto opposite = std::find(weights.begin(), weights.end(), 0.) - weights.begin();
        consider(facet.neighbors[opposite]);
      } else {
        const Id vertex = facet.vertices[std::max_element(weights.begin(), weights.end()) - weights.begin()];
        for (Id candidate : Fan(best.facet, vertex)) consider(candidate);
      }
      if (next.facet == UINT64_MAX) break;
      best = next;
      ++changes;
    }
    best.converged = best.converged && changes < maximum_changes;
    return best;
  }

  SurfaceBinding Binding(const WalkResult& result) const {
    const auto& f = facets.at(result.facet);
    const auto u = result.u, v = result.v;
    auto edge = [&](unsigned a, unsigned b, double t) {
      return f.vertices[a] < f.vertices[b] ? geometry.EdgePoint({f.vertices[a], f.vertices[b]}, t)
                                           : geometry.EdgePoint({f.vertices[b], f.vertices[a]}, 1 - t);
    };
    if (u == 1) return geometry.Vertex(f.vertices[1]);
    if (v == 1) return geometry.Vertex(f.vertices[2]);
    if (u == 0 && v == 0) return geometry.Vertex(f.vertices[0]);
    if (v == 0) return edge(0, 1, u);
    if (u == 0) return edge(0, 2, v);
    if (1 - u - v == 0) return edge(1, 2, v);
    return geometry.FacetPoint(result.facet, u, v);
  }

  std::array<WalkResult, 4> Samples(const SurfaceFace& face, const Triangle& triangle) const {
    constexpr std::array<WalkWeights, 4> weights{
        {{1. / 3., 1. / 3., 1. / 3.}, {.5, .5, 0}, {0, .5, .5}, {.5, 0, .5}}};
    std::array<WalkResult, 4> results;
    for (size_t i = 0; i < weights.size(); ++i) {
      Point query;
      for (unsigned k = 0; k < 3; ++k) {
        query.x += weights[i][k] * triangle[k].node.p.x;
        query.y += weights[i][k] * triangle[k].node.p.y;
        query.z += weights[i][k] * triangle[k].node.p.z;
      }
      results[i] = Run(query, Start(face, triangle, query, weights[i]));
    }
    return results;
  }

 private:
  struct Stencil {
    WalkNet net{};
    bool curved = false;
    std::array<Id, 3> vertices;
    std::array<Id, 3> neighbors{UINT64_MAX, UINT64_MAX, UINT64_MAX};
    BoundaryState::EntityKey entity;
  };

  WalkStart Interpolated(Id side, const Triangle& triangle, const WalkWeights& weights) const {
    WalkStart result{side};
    for (size_t k = 0; k < weights.size(); ++k) {
      if (weights[k] == 0) continue;
      const auto uv = geometry.SurfaceCoordinates(side, triangle[k].binding);
      result.u += weights[k] * uv.y;
      result.v += weights[k] * uv.z;
    }
    return result;
  }

  WalkResult Minimize(Point query, Id side, double u, double v) const {
    using namespace SU2NativeCurved::detail;
    const auto& row = facets.at(side);
    const auto& original = row.net;
    if (!row.curved) {
      ClosestOnTriangle(Values(query), original[0], original[1], original[2], u, v);
      WalkWeights point{};
      for (unsigned k = 0; k < 3; ++k)
        point[k] = (1 - u - v) * original[0][k] + u * original[1][k] + v * original[2][k];
      return {side, u, v, WalkDistance(query, {point[0], point[1], point[2]}), true};
    }
    auto normalized = original;
    const auto target = NormalizeTriangleProjection(normalized, {query.x, query.y, query.z});
    ClampSimplex(u, v);
    const bool converged = MinimizeTriangle(
        target, u, v, [&](double a, double b) { return TrianglePatch(normalized, 1 - a - b, a, b); },
        [&](double a, double b, WalkWeights& first, WalkWeights& second) {
          TriangleDerivatives(normalized, a, b, first, second);
        });
    constexpr double tiny = 1e-13;
    if (u < tiny) u = 0;
    if (v < tiny) v = 0;
    if (1 - u - v < tiny) {
      const double sum = u + v;
      u /= sum;
      v = 1 - u;
    }
    const auto foot = TrianglePatch(original, 1 - u - v, u, v);
    return {side, u, v, WalkDistance(query, {foot[0], foot[1], foot[2]}), converged};
  }

  std::vector<Id> Fan(Id start, Id vertex) const {
    std::vector<Id> fan{start};
    for (size_t i = 0; i < fan.size(); ++i) {
      const auto& row = facets.at(fan[i]);
      for (unsigned k = 0; k < 3; ++k) {
        const auto next = row.neighbors[k];
        if (row.vertices[k] == vertex || next == UINT64_MAX) continue;
        if (std::find(fan.begin(), fan.end(), next) == fan.end()) fan.push_back(next);
      }
    }
    std::sort(fan.begin(), fan.end());
    return fan;
  }

  const BoundaryState& state;
  const MappedFacetReference& geometry;
  std::map<Id, Stencil> facets;
};

SurfaceEdge EdgeKey(Id a, Id b) { return {std::min(a, b), std::max(a, b)}; }

SurfaceFaceKey FaceKey(SurfaceFaceKey face) {
  std::sort(face.begin(), face.end());
  return face;
}

SurfaceFaceKey FaceOrder(SurfaceFaceKey face) {
  const auto first = std::min_element(face.begin(), face.end());
  std::rotate(face.begin(), first, face.end());
  return face;
}

BoundaryState::Support BindingKey(const BoundaryPoint& point) {
  const auto& b = point.binding;
  return {b.dimension, b.feature, b.u, b.v};
}

bool SamePosition(Point a, Point b) {
  const auto left = Values(a), right = Values(b);
  return std::memcmp(left.data(), right.data(), sizeof(double) * 3) == 0;
}

Point Midpoint(Point a, Point b) {
  return {0.5 * (a.x + b.x), 0.5 * (a.y + b.y), 0.5 * (a.z + b.z)};
}

std::array<SurfaceFace, 4> Children(const std::array<SurfaceFace, 2>& parents, SurfaceEdge edge, Id middle) {
  std::array<SurfaceFace, 4> children;
  size_t i = 0;
  for (const auto& parent : parents)
    for (Id id : edge) {
      auto child = parent;
      *std::find(child.v.begin(), child.v.end(), id) = middle;
      children[i++] = child;
    }
  return children;
}

SurfacePlacement Refused(SurfaceRefusal refusal, std::string reason = {}) {
  SurfacePlacement result;
  result.refusal = refusal;
  result.reason = std::move(reason);
  return result;
}

const BoundaryState& GetState(const std::shared_ptr<const PlanarReference>& reference) {
  if (!reference || !reference->Parameters() || !reference->Parameters()->EntityGeometry() ||
      !reference->Parameters()->Mobility().identity)
    throw std::invalid_argument("Native surface splits require an entity geometry state and mobility policy.");
  return *reference->Parameters();
}

double Clock(bool report) { return report ? SU2_MPI::Wtime() : 0.; }

template <class Action>
void LocalStep(const World& world, Action action) {
  if (world.size == 1) {
    action();
    return;
  }
  CLocalFailure local;
  try { action(); }
  catch (const std::exception& e) { local.Set(2, 0, e.what()); }
  const auto failure = world.elect(local);
  if (failure.any) throw failure;
}

template <class Row>
std::vector<Row> SendEverywhere(World& world, const std::vector<Row>& local, MemoryBudget& budget, size_t caller) {
  using Buckets = std::vector<std::vector<Row>>;
  budget.Require(world, Add(caller, Mul(world.size, Add(sizeof(std::vector<Row>), transfer_memory::Bytes(local)))),
                 "Native local surface send lists exceed working-storage admission.");
  Buckets outgoing;
  LocalStep(world, [&] { outgoing.assign(world.size, local); });
  std::vector<Row> received;
  LocalStep(world, [&] {
    bool admitted = false;
    received = world.exchange(outgoing, &admitted, budget.ceiling, caller);
    budget.maximum_bytes = std::max(budget.maximum_bytes, world.max_exchange_work_bytes);
    if (!admitted) throw std::length_error("Native local surface exchange exceeds working-storage admission.");
  });
  return received;
}

struct CreatedRecord {
  BoundaryPoint point;
  uint64_t source = 0;
  template <class Stream>
  void Fields(Stream& stream) { stream(point, source); }
};

struct RemovedRecord {
  Id id = 0;
  uint64_t source = 0;
  template <class Stream>
  void Fields(Stream& stream) { stream(id, source); }
};

struct ErasedRecord {
  SurfaceEdge edge;
  template <class Stream>
  void Fields(Stream& stream) { stream(edge); }
};

struct AtlasPage {
  uint64_t source = 0, revision = 0, identity = 0, sequence = 0, total = 0, used = 0;
  std::array<uint64_t, 64> words{};
  template <class Stream>
  void Fields(Stream& stream) { stream(source, revision, identity, sequence, total, used, words); }
};

std::vector<AtlasPage> EncodePages(const BoundaryCoverage& coverage, const BoundaryState& state, int rank,
                                  size_t ceiling, size_t caller) {
  if (coverage.traces.empty()) return {};
  const auto bytes = SU2Native3D::EncodeBoundaryAtlas(coverage, {}, ceiling, caller);
  SU2NativeBoundary2D::RecordStream input(bytes);
  constexpr size_t page_words = std::tuple_size<decltype(AtlasPage::words)>::value;
  const auto words = bytes.size() / sizeof(uint64_t);
  const auto count = transfer_memory::CeilDiv(words, page_words);
  std::vector<AtlasPage> pages(count);
  for (size_t i = 0; i < count; ++i) {
    auto& page = pages[i];
    page.source = rank;
    page.revision = state.Revision();
    page.identity = state.Mobility().identity;
    page.sequence = i;
    page.total = count;
    page.used = std::min(page_words, words - i * page_words);
    for (size_t j = 0; j < page.used; ++j) input(page.words[j]);
  }
  if (!input.End()) throw std::invalid_argument("Native local surface trace payload is not word-aligned.");
  return pages;
}

template <class Consumer>
void DecodePages(std::vector<AtlasPage>& pages, const BoundaryState& state, int ranks, size_t ceiling, size_t caller,
                 Consumer consume) {
  std::sort(pages.begin(), pages.end(), [](const auto& a, const auto& b) {
    return std::tie(a.source, a.sequence) < std::tie(b.source, b.sequence);
  });
  for (size_t first = 0; first < pages.size();) {
    const auto source = pages[first].source, total = pages[first].total;
    size_t last = first + 1;
    while (last < pages.size() && pages[last].source == source) ++last;
    if (source >= static_cast<uint64_t>(ranks) || total != last - first)
      throw std::invalid_argument("Native local surface trace payload has missing or duplicate pages.");
    std::vector<char> bytes;
    bytes.reserve(Mul(total, sizeof(AtlasPage::words)));
    SU2NativeBoundary2D::RecordStream output(bytes);
    for (size_t i = first; i < last; ++i) {
      const auto& p = pages[i];
      if (p.sequence != i - first || p.total != total || p.revision != state.Revision() ||
          p.identity != state.Mobility().identity || p.used == 0 || p.used > p.words.size() ||
          (i + 1 < last && p.used != p.words.size()))
        throw std::invalid_argument("Native local surface trace payload has stale or inconsistent pages.");
      for (size_t j = 0; j < p.used; ++j) {
        auto word = p.words[j];
        output(word);
      }
    }
    BoundaryCoverage coverage;
    SU2NativeGeometry::MobilityPolicy policy;
    SU2Native3D::DecodeBoundaryAtlas(bytes, coverage, policy, ceiling, caller);
    if (!coverage.faces.empty() || policy.identity || !policy.roles.empty() || !policy.features.empty() ||
        !policy.corners.empty() || !policy.bodies.empty() || !policy.sheets.empty())
      throw std::invalid_argument("Native local surface trace payload attempts a face or policy edit.");
    consume(source, std::move(coverage));
    first = last;
  }
}

SurfaceFace OwnedFace(const SU2Native3D::OwnedCell& row, unsigned k) {
  const auto order = OrientedFace({0, 1, 2, 3}, k);
  return {{row.cell.v[order[0]].id, row.cell.v[order[1]].id, row.cell.v[order[2]].id}, row.physical[k].facet};
}

std::vector<SurfaceFace> OwnedFaces(const Owned& owned) {
  std::vector<SurfaceFace> faces;
  for (const auto& entry : owned)
    for (unsigned k = 0; k < 4; ++k)
      if (entry.second.physical[k].present) faces.push_back(OwnedFace(entry.second, k));
  return faces;
}

size_t CheckOwnedFaces(const Owned& owned, const BoundaryState& next) {
  size_t faces = 0;
  for (const auto& entry : owned) {
    const auto& row = entry.second;
    for (unsigned k = 0; k < 4; ++k) {
      if (!row.physical[k].present) continue;
      ++faces;
      const auto face = OwnedFace(row, k);
      const auto& checked = next.Surface().at(FaceKey(face.v));
      if (checked.v != FaceOrder(face.v) || checked.facet != next.Geometry().Canonical(face.facet))
        throw std::invalid_argument("Native local surface state has a different owned face order or anchor.");
      for (auto index : OrientedFace({0, 1, 2, 3}, k)) {
        const auto& node = row.cell.v[index];
        const auto* point = next.Find(node.id);
        if (!point || point->node.id != node.id || !SamePosition(node.p, point->node.p))
          throw std::invalid_argument("Native local surface state has a different owned boundary point.");
      }
    }
  }
  return faces;
}
}  // namespace

SurfaceQueryScratch::SurfaceQueryScratch(size_t* maximum) : previous_(query_scratch_maximum) {
  query_scratch_maximum = maximum;
}

SurfaceQueryScratch::~SurfaceQueryScratch() { query_scratch_maximum = previous_; }

struct StateSurfaceGeometry::Impl {
  Impl(std::shared_ptr<const PlanarReference> reference, AtlasBudget input)
      : original(std::move(reference)), state(GetState(original)), walker(state), budget(std::move(input)),
        caller_bytes(budget.caller_bytes) {
    for (const auto& entry : state.Records()) AddKeys(entry.second.point);
  }

  const BoundaryPoint* Find(Id id) const {
    if (removed.count(id)) return nullptr;
    const auto found = created.find(id);
    return found == created.end() ? state.Find(id) : &found->second;
  }

  const BoundaryPoint& At(Id id) const {
    const auto* point = Find(id);
    if (!point) throw std::invalid_argument("Native local surface point has no persistent association.");
    return *point;
  }

  const EdgeTrace* Trace(const SurfaceEdge& edge) const {
    if (erased.count(edge)) return nullptr;
    const auto added = overlay.traces.find(edge);
    if (added != overlay.traces.end()) return &added->second;
    const auto found = state.Coverage().traces.find(edge);
    return found == state.Coverage().traces.end() ? nullptr : &found->second;
  }

  Triangle Corners(const SurfaceFace& face, const BoundaryPoint* middle = nullptr) const {
    Triangle result;
    for (unsigned k = 0; k < 3; ++k) {
      const Id id = face.v[k];
      result[k] = middle && id == middle->node.id ? *middle : At(id);
    }
    return result;
  }

  bool Duplicate(const BoundaryPoint& point) const {
    return supports.count(BindingKey(point)) || coordinates.count(Values(point.node.p));
  }

  void AddKeys(const BoundaryPoint& point) {
    supports.insert(BindingKey(point));
    coordinates.insert(Values(point.node.p));
  }

  size_t ResidentBytes() const {
    return Add(sizeof(*this), walker.ResidentBytes(), MapBytes(created), MapBytes(removed), MapBytes(erased),
               MapBytes(supports), MapBytes(coordinates), SU2Native3D::BoundaryCoverageBytes(overlay),
               transfer_memory::Bytes(pending_points), transfer_memory::Bytes(pending_removed),
               transfer_memory::Bytes(pending_traces), transfer_memory::Bytes(pending_erased));
  }

  void CheckBudget() {
    if (budget.work_exhausted) throw std::length_error("Native local surface work_exhausted.");
    if (budget.time_exhausted) throw std::length_error("Native local surface time_exhausted.");
  }

  void BeginQuery() {
    budget.CheckTime();
    budget.caller_bytes = Add(caller_bytes, ResidentBytes());
    if (budget.caller_bytes == SIZE_MAX || budget.caller_bytes > budget.maximum_bytes)
      throw std::length_error("Native local surface overlay exceeds its working-storage ceiling.");
  }

  bool Propose(const std::array<Node, 2>& ends, const EdgeTrace& trace, Id id, BoundaryPoint& point,
               std::string& reason) {
    const auto a = ends[0].p, b = ends[1].p;
    bool accepted = false;
    try {
      accepted = SU2NativeGeometry::ProposeFeaturePoint(state.Geometry(), state.Mobility(), {trace}, a, b,
                                                       {id, Midpoint(a, b)}, point, budget, reason);
    } catch (const std::exception& e) {
      reason = e.what();
    }
    CheckBudget();
    return accepted;
  }

  SurfacePlacement PlaceOnFeature(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& parents,
                                  Id id, const EdgeTrace& trace) {
    BoundaryPoint middle;
    std::string reason;
    if (!Propose(ends, trace, id, middle, reason)) return Refused(SurfaceRefusal::Feature, reason);
    if (Duplicate(middle)) return Refused(SurfaceRefusal::Duplicate);

    const auto edge = EdgeKey(ends[0].id, ends[1].id);
    Patch old;
    old.surface.assign(parents.begin(), parents.end());
    for (const auto& parent : parents)
      for (Id vertex : parent.v) old.parameters.emplace(vertex, At(vertex));
    old.coverage.traces.emplace(edge, trace);
    auto points = old.parameters;
    points.emplace(id, middle);
    const auto children = Children(parents, edge, id);
    const std::vector<SurfaceFace> faces(children.begin(), children.end());
    std::map<SurfaceEdge, EdgeTrace> traces;
    bool accepted = false;
    try {
      accepted = SU2NativeGeometry::RebuildFeatureTraces(old, faces, points, *original, traces, budget, reason);
    } catch (const std::exception& e) {
      reason = e.what();
    }
    CheckBudget();
    if (!accepted) return Refused(SurfaceRefusal::Trace, reason);

    SurfacePlacement result;
    result.point = middle;
    result.parent = edge;
    for (Id end : edge) {
      auto child = traces.extract(EdgeKey(end, id));
      if (child.empty()) return Refused(SurfaceRefusal::Trace, "Feature replacement omitted a child trace.");
      result.children.insert(std::move(child));
    }
    return result;
  }

  double ProjectorBound(Point query, const SurfaceBinding& binding) const {
    // As in the engine's projector, a model foot within its tol/8 accuracy needs no further
    // optimality proof. Distance plus the realization error bounds the distance to a model point.
    const auto& geometry = state.Geometry();
    const auto point = geometry.Realize(binding);
    const long double distance = std::hypot(static_cast<long double>(query.x) - point.x,
                                            static_cast<long double>(query.y) - point.y,
                                            static_cast<long double>(query.z) - point.z);
    return std::nextafter(double(distance + geometry.ErrorBound(binding, point)), INFINITY);
  }

  SurfacePlacement PlaceOnSheet(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& parents, Id id) {
    const auto& first = FaceKey(parents[0].v) < FaceKey(parents[1].v) ? parents[0] : parents[1];
    const auto corner = [&](Id vertex) { return std::find(first.v.begin(), first.v.end(), vertex) - first.v.begin(); };
    const auto query = Midpoint(ends[0].p, ends[1].p);
    const auto refused = [](SurfaceRefusal reason, SurfaceWalk walk, std::string message = {}) {
      auto result = Refused(reason, std::move(message));
      result.walk = walk;
      return result;
    };
    try {
      const auto hit = walker.Run(query, walker.EdgeStart(first, Corners(first), corner(ends[0].id),
                                                          corner(ends[1].id), query));
      if (hit.facet == UINT64_MAX) return refused(SurfaceRefusal::Walk, SurfaceWalk::NoStart);
      const auto binding = walker.Binding(hit);
      auto walk = SurfaceWalk::None;
      if (!hit.converged) {
        const double upper = ProjectorBound(query, binding);
        if (!std::isfinite(upper)) return refused(SurfaceRefusal::Walk, SurfaceWalk::Exception);
        if (upper > state.GeometryTolerance() / 8) return refused(SurfaceRefusal::Walk, SurfaceWalk::Far);
        walk = SurfaceWalk::Tolerance;
      }
      if (state.Geometry().MobilityDimension(binding, state.Mobility()) != 2)
        return refused(SurfaceRefusal::Protected, walk);
      SurfacePlacement result;
      result.point = {{id, state.Geometry().Realize(binding)}, binding};
      if (Duplicate(result.point)) return refused(SurfaceRefusal::Duplicate, walk);
      result.walk = walk;
      return result;
    } catch (const std::exception& e) {
      return refused(SurfaceRefusal::Walk, SurfaceWalk::Exception, e.what());
    }
  }

  std::shared_ptr<const PlanarReference> original;
  const SU2Native3D::BoundaryParameterState& state;
  Walker walker;
  AtlasBudget budget;
  size_t caller_bytes;
  BoundaryPoints created;
  std::set<Id> removed;
  BoundaryCoverage overlay;
  std::set<SurfaceEdge> erased;
  std::set<BoundaryState::Coordinate> coordinates;
  std::set<BoundaryState::Support> supports;
  size_t local_splits = 0, local_removals = 0;
  std::vector<Id> pending_points, pending_removed;
  std::vector<SurfaceEdge> pending_traces, pending_erased;
};

StateSurfaceGeometry::StateSurfaceGeometry(std::shared_ptr<const PlanarReference> reference, AtlasBudget budget)
    : impl_(new Impl(std::move(reference), std::move(budget))) {
  impl_->BeginQuery();
}

StateSurfaceGeometry::~StateSurfaceGeometry() = default;

double StateSurfaceGeometry::Tolerance() const { return impl_->state.GeometryTolerance(); }
bool StateSurfaceGeometry::Feature(const SurfaceEdge& edge) const { return FindTrace(edge) != nullptr; }
const BoundaryPoint* StateSurfaceGeometry::Find(Id id) const { return impl_->Find(id); }
const EdgeTrace* StateSurfaceGeometry::FindTrace(const SurfaceEdge& edge) const { return impl_->Trace(edge); }
const SU2Native3D::BoundaryParameterState& StateSurfaceGeometry::State() const { return impl_->state; }
size_t StateSurfaceGeometry::ResidentBytes() const { return Add(sizeof(*this), impl_->ResidentBytes()); }
size_t StateSurfaceGeometry::PeakBytes() const { return impl_->budget.peak_bytes; }
size_t StateSurfaceGeometry::Splits() const { return impl_->local_splits; }
size_t StateSurfaceGeometry::Removals() const { return impl_->local_removals; }

void StateSurfaceGeometry::SetCallerBytes(size_t caller) {
  impl_->caller_bytes = caller;
  impl_->BeginQuery();
}

SurfaceSamples StateSurfaceGeometry::Samples(const SurfaceFace& face) {
  Scope timing(Operation::GeometryBound);
  impl_->budget.CheckTime();
  const auto hits = impl_->walker.Samples(face, impl_->Corners(face));
  SurfaceSamples samples;
  for (size_t k = 0; k < hits.size(); ++k) {
    samples.distances[k] = hits[k].distance;
    samples.unconverged += !hits[k].converged;
  }
  return samples;
}

std::optional<double> StateSurfaceGeometry::Chord(const std::array<Node, 2>& ends) {
  Scope timing(Operation::GeometryBound);
  impl_->BeginQuery();
  const QueryPeak query_peak(impl_->budget);
  const auto* trace = impl_->Trace(EdgeKey(ends[0].id, ends[1].id));
  if (!trace) return std::nullopt;
  BoundaryPoint middle;
  std::string reason;
  if (!impl_->Propose(ends, *trace, 0, middle, reason)) return std::nullopt;
  return WalkDistance(Midpoint(ends[0].p, ends[1].p), middle.node.p);
}

SurfacePlacement StateSurfaceGeometry::Place(const std::array<Node, 2>& ends,
                                               const std::array<SurfaceFace, 2>& parents, Id id) {
  Scope timing(Operation::GeometryBound);
  impl_->BeginQuery();
  const QueryPeak query_peak(impl_->budget);
  const auto* trace = impl_->Trace(EdgeKey(ends[0].id, ends[1].id));
  return trace ? impl_->PlaceOnFeature(ends, parents, id, *trace) : impl_->PlaceOnSheet(ends, parents, id);
}

bool StateSurfaceGeometry::OnEntity(const SurfaceFace& face, const BoundaryPoint& middle) {
  return impl_->state.ContainsEntityFace(face.facet, impl_->Corners(face, &middle));
}

void StateSurfaceGeometry::Accept(SurfacePlacement&& placement) {
  auto& geometry = *impl_;
  if (!placement.Accepted() || geometry.removed.count(placement.point.node.id) || geometry.Duplicate(placement.point) ||
      !geometry.created.emplace(placement.point.node.id, placement.point).second)
    throw std::invalid_argument("Native local surface attempted a duplicate or refused publication.");
  geometry.AddKeys(placement.point);
  ++geometry.local_splits;
  geometry.pending_points.push_back(placement.point.node.id);
  if (placement.parent) {
    geometry.erased.insert(*placement.parent);
    geometry.pending_erased.push_back(*placement.parent);
    geometry.overlay.traces.erase(*placement.parent);
    for (const auto& child : placement.children) geometry.pending_traces.push_back(child.first);
    geometry.overlay.traces.merge(placement.children);
    if (!placement.children.empty()) throw std::invalid_argument("Native local surface child trace already exists.");
  }
}

unsigned StateSurfaceGeometry::Dimension(Id id) const {
  return impl_->state.Geometry().MobilityDimension(impl_->At(id).binding, impl_->state.Mobility());
}

void StateSurfaceGeometry::Remove(Id id) {
  auto& g = *impl_;
  if (g.removed.count(id))
    throw std::invalid_argument("Native local surface point was already removed.");
  const auto created = g.created.find(id);
  if (created == g.created.end() && !g.state.Find(id))
    throw std::invalid_argument("Native local surface removed point has no persistent association.");
  const auto pending = std::find(g.pending_points.begin(), g.pending_points.end(), id);
  if (pending != g.pending_points.end()) {
    const auto& point = g.created.at(id);
    g.supports.erase(BindingKey(point));
    g.coordinates.erase(Values(point.node.p));
    g.pending_points.erase(pending);
  } else {
    g.pending_removed.push_back(id);
  }
  if (created != g.created.end()) g.created.erase(created);
  g.removed.insert(id);
  ++g.local_removals;
}

bool StateSurfaceGeometry::OnEntityFace(const SurfaceFace& face) const {
  return impl_->state.ContainsEntityFace(face.facet, impl_->Corners(face));
}

bool StateSurfaceGeometry::WithinTolerance(const SurfaceFace& face) {
  Scope timing(Operation::GeometryBound);
  impl_->BeginQuery();
  const QueryPeak query_peak(impl_->budget);
  auto& state = impl_->state;
  // Support selection depends on the first vertex. Match the rebuilt census inputs.
  // Admission can change its traversal, so the rebuilt proof remains the gate.
  const auto facet = state.Geometry().Canonical(face.facet);
  const auto corners = impl_->Corners({FaceOrder(face.v), facet});
  std::array<SurfaceBinding, 3> bindings;
  std::array<Point, 3> points;
  for (size_t k = 0; k < corners.size(); ++k) {
    bindings[k] = corners[k].binding;
    points[k] = corners[k].node.p;
  }
  const auto bound = state.EntityFaceError(facet, bindings, points, &impl_->budget,
                                         SU2NativeGeometry::SurfaceQueryNeed::Classify());
  impl_->CheckBudget();
  return bound <= state.GeometryTolerance();
}

void StateSurfaceGeometry::Exchange(World& world, MemoryBudget& budget, size_t caller) {
  if (world.size == 1) return;
  auto& g = *impl_;
  const auto baseline = Add(caller, ResidentBytes());
  std::vector<CreatedRecord> local_points;
  std::vector<RemovedRecord> local_removed;
  std::vector<ErasedRecord> local_erased;
  std::vector<AtlasPage> local_pages;
  budget.Require(world, Add(baseline, Mul(2, SU2Native3D::BoundaryCoverageBytes(g.overlay)),
                            Mul(g.pending_points.size(), sizeof(CreatedRecord)),
                            Mul(g.pending_removed.size(), sizeof(RemovedRecord)),
                            Mul(g.pending_erased.size(), sizeof(ErasedRecord))),
                 "Native local surface overlay send preparation exceeds working-storage admission.");
  LocalStep(world, [&] {
    local_points.reserve(g.pending_points.size());
    for (Id id : g.pending_points) local_points.push_back({g.created.at(id), static_cast<uint64_t>(world.rank)});
    local_removed.reserve(g.pending_removed.size());
    for (Id id : g.pending_removed) local_removed.push_back({id, static_cast<uint64_t>(world.rank)});
    local_erased.reserve(g.pending_erased.size());
    for (const auto& edge : g.pending_erased) local_erased.push_back({edge});
    BoundaryCoverage recent;
    // An edge created and split again in one pass is already absent from the outgoing live traces.
    for (const auto& edge : g.pending_traces)
      if (const auto* trace = g.Trace(edge)) recent.traces.emplace(edge, *trace);
    const auto live = Add(baseline, transfer_memory::Bytes(local_points), transfer_memory::Bytes(local_removed),
                          transfer_memory::Bytes(local_erased), SU2Native3D::BoundaryCoverageBytes(recent));
    const auto encoded = recent.traces.empty() ? 0 : SU2Native3D::BoundaryAtlasBytes(recent, {});
    const auto peak = Add(live, Mul(4, encoded));
    if (peak == SIZE_MAX || peak > budget.ceiling)
      throw std::length_error("Native local surface trace packing exceeds working-storage admission.");
    budget.maximum_bytes = std::max(budget.maximum_bytes, peak);
    local_pages = EncodePages(recent, g.state, world.rank, budget.ceiling, live);
  });
  const auto send_live = Add(baseline, transfer_memory::Bytes(local_points), transfer_memory::Bytes(local_removed),
                             transfer_memory::Bytes(local_erased), transfer_memory::Bytes(local_pages));
  auto points = SendEverywhere(world, local_points, budget, send_live);
  const auto points_live = Add(send_live, transfer_memory::Bytes(points));
  auto removed = SendEverywhere(world, local_removed, budget, points_live);
  const auto removals_live = Add(points_live, transfer_memory::Bytes(removed));
  auto erased = SendEverywhere(world, local_erased, budget, removals_live);
  const auto lists = Add(removals_live, transfer_memory::Bytes(erased));
  auto pages = SendEverywhere(world, local_pages, budget, lists);
  const auto decode_base = Add(lists, transfer_memory::Bytes(pages));
  const auto merge_bound = Add(decode_base, Mul(4, transfer_memory::Bytes(pages)),
                               Mul(points.size(), 512), Mul(removed.size(), sizeof(Id) + 4 * sizeof(void*)),
                               Mul(erased.size(), 128));
  budget.Require(world, merge_bound, "Native local surface overlay merge exceeds working-storage admission.");

  LocalStep(world, [&] {
    BoundaryCoverage incoming;
    std::set<SurfaceEdge> own_traces;
    DecodePages(pages, g.state, world.size, budget.ceiling, merge_bound,
                [&](uint64_t source, BoundaryCoverage&& decoded) {
      for (const auto& row : decoded.traces)
        if (source == static_cast<uint64_t>(world.rank)) own_traces.insert(row.first);
      incoming.traces.merge(decoded.traces);
      if (!decoded.traces.empty()) throw std::invalid_argument("Duplicate native local surface child edge from workers.");
    });

    std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.point.node.id < b.point.node.id; });
    for (size_t i = 0; i < points.size(); ++i) {
      const auto& row = points[i];
      const auto id = row.point.node.id;
      if (i && points[i - 1].point.node.id == id)
        throw std::invalid_argument("Duplicate native local surface point id from workers.");
      if (g.removed.count(id))
        throw std::invalid_argument("Removed native local surface point id from workers.");
      if (row.source >= static_cast<uint64_t>(world.size))
        throw std::invalid_argument("Invalid native local surface point source.");
      if (row.source == static_cast<uint64_t>(world.rank)) {
        const auto& previous = g.created.at(id);
        if (!SamePosition(previous.node.p, row.point.node.p) || BindingKey(previous) != BindingKey(row.point) ||
            previous.binding.version != row.point.binding.version)
          throw std::invalid_argument("Native local surface point changed on its round trip.");
      } else {
        if (g.Find(id) || g.Duplicate(row.point) || !g.created.emplace(id, row.point).second)
          throw std::invalid_argument("Duplicate native local surface point association from workers.");
        g.AddKeys(row.point);
      }
    }

    for (const auto& row : removed) {
      if (row.source >= static_cast<uint64_t>(world.size))
        throw std::invalid_argument("Invalid native local surface removal source.");
      if (row.source == static_cast<uint64_t>(world.rank)) continue;
      if (!g.Find(row.id))
        throw std::invalid_argument("Duplicate or unknown native local surface removal from workers.");
      g.created.erase(row.id);
      g.removed.insert(row.id);
    }

    for (const auto& edge : own_traces) incoming.traces.erase(edge);
    g.overlay.traces.merge(incoming.traces);
    if (!incoming.traces.empty()) throw std::invalid_argument("Native local surface child edge was already stored.");
    for (const auto& row : erased) {
      g.erased.insert(row.edge);
      g.overlay.traces.erase(row.edge);
    }
    g.pending_points.clear();
    g.pending_removed.clear();
    g.pending_traces.clear();
    g.pending_erased.clear();
  });
}

SurfaceEdits StateSurfaceGeometry::GatherEdits(const std::vector<SurfaceFace>& faces) const {
  SurfaceEdits edits;
  edits.faces = faces;
  std::set<SurfaceFaceKey> seen;
  for (const auto& face : faces) {
    if (!seen.insert(FaceKey(face.v)).second) throw std::invalid_argument("Duplicate gathered native surface face.");
    for (unsigned k = 0; k < 3; ++k) {
      edits.points.try_emplace(face.v[k], impl_->At(face.v[k]));
      const auto edge = EdgeKey(face.v[k], face.v[(k + 1) % 3]);
      const auto* trace = impl_->Trace(edge);
      if (trace) edits.coverage.traces.try_emplace(edge, *trace);
    }
  }
  for (const auto& entry : impl_->created)
    if (!edits.points.count(entry.first)) throw std::invalid_argument("New native surface point has no gathered face.");
  return edits;
}

size_t SurfaceAtlasIntegerBits() {
  constexpr size_t scratch = size_t{16} << 20;
  size_t low = 0, high = scratch * 8;
  while (low < high) {
    const size_t middle = low + (high - low + 1) / 2;
    if (Mul(2, SU2NativeGeometry::CurvedAtlasWorkspaceBytes(middle)) <= scratch / 2)
      low = middle;
    else
      high = middle - 1;
  }
  return low;
}

SurfaceStateResult BuildSurfaceState(const StateSurfaceGeometry& geometry, const SurfaceEdits& edits, size_t ceiling,
                                     size_t caller, bool report) {
  const auto start = Clock(report);
  const auto& state = geometry.State();
  if (state.Revision() == UINT64_MAX) throw std::invalid_argument("Native surface state exhausted its revision.");
  const auto points = edits.points.size(), faces = edits.faces.size();
  const auto live = Add(caller, geometry.ResidentBytes(), Mul(256, points), Mul(sizeof(SurfaceFace), faces),
                        Mul(4, SU2Native3D::BoundaryCoverageBytes(edits.coverage)));
  const auto bound = Add(live, state.Authority()->ResidentBytes(), Mul(2048, points), Mul(8192, faces),
                         Mul(4, SU2Native3D::BoundaryAtlasBytes({}, state.Mobility())));
  if (bound == SIZE_MAX || bound > ceiling)
    throw std::length_error("Native surface state rebuild exceeds storage admission.");
  auto reservation = SU2Native3D::BoundaryAtlasBudget(SU2Native3D::BoundaryAtlasBytes(edits.coverage, state.Mobility()),
                                                      ceiling, bound);
  SurfaceStateResult result;
  result.state = std::make_shared<SU2Native3D::BoundaryParameterState>(
      state.Authority(), edits.points, edits.faces, state.Revision() + 1, ceiling, live, edits.coverage,
      state.Mobility(), &reservation, state.GeometryTolerance());
  result.splits = geometry.Splits();
  result.removals = geometry.Removals();
  result.modeled_peak = std::max(bound, reservation.peak_bytes);
  result.seconds_build = Clock(report) - start;
  return result;
}

SurfaceStateResult BuildSurfaceState(World& world, const Owned& owned, const StateSurfaceGeometry& geometry,
                                     MemoryBudget& budget, size_t caller, bool report) {
  const auto splits = world.size == 1 ? geometry.Splits()
                                      : world.reduce<uint64_t>(geometry.Splits(), CPassiveComm::Op::SUM);
  const auto removals = world.size == 1 ? geometry.Removals()
                                       : world.reduce<uint64_t>(geometry.Removals(), CPassiveComm::Op::SUM);
  if (!splits && !removals) return {};
  const auto begin = Clock(report);
  const auto baseline = Add(caller, geometry.ResidentBytes());
  std::vector<SurfaceFace> local;
  LocalStep(world, [&] {
    size_t count = 0;
    for (const auto& entry : owned)
      for (const auto& binding : entry.second.physical) count += binding.present;
    const auto bound = Add(baseline, transfer_memory::GrowthBound(count, sizeof(SurfaceFace)));
    if (bound == SIZE_MAX || bound > budget.ceiling)
      throw std::length_error("Native local owned face list exceeds working-storage admission.");
    budget.maximum_bytes = std::max(budget.maximum_bytes, bound);
    local = OwnedFaces(owned);
  });
  auto faces = world.size == 1 ? std::move(local)
                               : SendEverywhere(world, local, budget, Add(baseline, transfer_memory::Bytes(local)));
  const auto lists = Add(transfer_memory::Bytes(local), transfer_memory::Bytes(faces));
  SurfaceEdits edits;
  LocalStep(world, [&] {
    const auto face_scratch = Add(Mul(3, 256), sizeof(SurfaceFace) + sizeof(SurfaceFaceKey) + 4 * sizeof(void*));
    const auto bound = Add(baseline, lists, Mul(faces.size(), face_scratch),
                           SU2Native3D::BoundaryCoverageBytes(geometry.State().Coverage()), geometry.ResidentBytes());
    if (bound == SIZE_MAX || bound > budget.ceiling)
      throw std::length_error("Native local surface edit assembly exceeds working-storage admission.");
    budget.maximum_bytes = std::max(budget.maximum_bytes, bound);
    edits = geometry.GatherEdits(faces);
  });
  const auto gather = Clock(report) - begin;
  SurfaceStateResult result;
  LocalStep(world, [&] { result = BuildSurfaceState(geometry, edits, budget.ceiling, Add(caller, lists), report); });
  const auto check = Clock(report);
  size_t counted = 0;
  LocalStep(world, [&] { counted = CheckOwnedFaces(owned, *result.state); });
  if (world.size > 1) counted = world.reduce<uint64_t>(counted, CPassiveComm::Op::SUM);
  LocalStep(world, [&] {
    if (counted != result.state->Surface().size())
      throw std::invalid_argument("Owned physical face count differs from native surface state.");
  });
  budget.maximum_bytes = std::max(result.modeled_peak, budget.maximum_bytes);
  result.splits = splits;
  result.removals = removals;
  result.modeled_peak = budget.maximum_bytes;
  result.seconds_gather = gather;
  result.seconds_check = Clock(report) - check;
  return result;
}
}  // namespace SU2NativeLocal3D
