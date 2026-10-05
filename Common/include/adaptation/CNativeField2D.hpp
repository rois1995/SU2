/*!
 * \file CNativeField2D.hpp
 * \brief Bounded imports and immutable P1 nodal targets for native cavity reconstruction.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once

#include "CNativeDistributed2D.hpp"
#include <memory>
#include <tuple>

namespace SU2NativeBoundary2D {

constexpr size_t FIELD_LIMIT = 256, QUERY_CACHE_LIMIT = 2048;
struct DonorCell {
  Triangle triangle;
  std::array<int, 3> marker{};
  std::array<Tensor, 3> metric{};
  std::array<Id, 3> Key() const {
    std::array<Id, 3> key{triangle.v[0].id, triangle.v[1].id, triangle.v[2].id};
    std::sort(key.begin(), key.end());
    return key;
  }
  template <class S>
  void Fields(S& s) {
    s(triangle, marker, metric);
  }
};
struct FieldRegion {
  std::array<double, 2> lo{}, hi{};
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(lo, hi, caller);
  }
};
struct DonorId {
  Id id = 0;
  int owner = 0;
  template <class S>
  void Fields(S& s) {
    s(id, owner);
  }
};
struct DonorRequest {
  Id id = 0;
  int caller = 0;
  template <class S>
  void Fields(S& s) {
    s(id, caller);
  }
};

/*--- The original donor never changes during a remesh call. Canonical sorted original node keys resolve
 *    containment ties independently of cell IDs, ownership and delivery order. Extension is allowed only to
 *    an explicitly associated physical component and a declared distance; global tensor axes never rotate. ---*/
struct FieldPatch {
  std::vector<DonorCell> cells;
  std::set<int> extension_components;
  double extension_limit = 0;
  mutable size_t samples = 0, extensions = 0, roundoff_queries = 0;
  mutable double maximum_extension = 0;
  mutable std::map<std::array<double, 2>, Tensor> cache;

  static Tensor Blend(const DonorCell& cell, const std::array<long double, 3>& weights) {
    long double xx = 0, xy = 0, yy = 0;
    for (int k = 0; k < 3; ++k) {
      xx += weights[k] * cell.metric[k].xx;
      xy += weights[k] * cell.metric[k].xy;
      yy += weights[k] * cell.metric[k].yy;
    }
    const Tensor result{static_cast<double>(xx), static_cast<double>(xy), static_cast<double>(yy)};
    if (!(NormalizedDeterminant(result.xx, result.xy, result.yy) > 1e-14L))
      throw std::runtime_error("Invalid interpolated frozen nodal target.");
    return result;
  }
  Tensor evaluate(Point p) const {
    if (!(std::isfinite(p.x) && std::isfinite(p.y))) throw std::runtime_error("Nonfinite frozen-target query.");
    const auto known = cache.find({p.x, p.y});
    if (known != cache.end()) return known->second;
    ++samples;
    auto retain = [&](Tensor value) {
      if (cache.size() < QUERY_CACHE_LIMIT) cache.emplace(std::array<double, 2>{p.x, p.y}, value);
      return value;
    };
    for (const auto& cell : cells) {
      const auto& t = cell.triangle;
      const std::array<long double, 3> signs{orient(p, t.v[1].p, t.v[2].p), orient(t.v[0].p, p, t.v[2].p),
                                             orient(t.v[0].p, t.v[1].p, p)};
      if (*std::min_element(signs.begin(), signs.end()) < 0) continue;
      const auto denominator = signs[0] + signs[1] + signs[2];
      if (!(denominator > 0 && std::isfinite(denominator))) throw std::runtime_error("Unusable frozen donor simplex.");
      auto weights = signs;
      for (auto& w : weights) w /= denominator;
      return retain(Blend(cell, weights));
    }
    // A rounded centroid/midpoint can be less than one ulp outside its source simplex. Prefer any exact
    // containing donor above; only then admit a four-epsilon coordinate-distance envelope. Negative weights
    // are clamped and renormalized on the same frozen donor. A physical edge must still be associated with
    // this cavity. This is numerical containment, counted separately from the declared domain extension.
    long double nearest = std::numeric_limits<long double>::infinity();
    Tensor rounded{};
    for (const auto& cell : cells) {
      const auto& t = cell.triangle;
      std::array<long double, 3> weights{orient(p, t.v[1].p, t.v[2].p), orient(t.v[0].p, p, t.v[2].p),
                                         orient(t.v[0].p, t.v[1].p, p)};
      long double scale = std::max(std::abs(p.x), std::abs(p.y)), outside = 0;
      bool associated = true;
      for (int k = 0; k < 3; ++k) {
        scale = std::max(scale, static_cast<long double>(std::max(std::abs(t.v[k].p.x), std::abs(t.v[k].p.y))));
        if (weights[k] >= 0) continue;
        const int edgeSlot = (k + 1) % 3;
        if (cell.marker[edgeSlot] && !extension_components.count(cell.marker[edgeSlot])) associated = false;
        const auto a = t.v[(k + 1) % 3].p, b = t.v[(k + 2) % 3].p;
        const auto distance =
            -weights[k] / std::hypot(static_cast<long double>(b.x) - a.x, static_cast<long double>(b.y) - a.y);
        outside = std::max(outside, distance);
      }
      if (!associated || outside > 4 * std::numeric_limits<double>::epsilon() * scale || !(outside < nearest)) continue;
      long double sum = 0;
      for (auto& w : weights) {
        w = std::max(0.L, w);
        sum += w;
      }
      if (!(sum > 0 && std::isfinite(sum))) continue;
      for (auto& w : weights) w /= sum;
      rounded = Blend(cell, weights);
      nearest = outside;
    }
    if (std::isfinite(nearest)) {
      ++roundoff_queries;
      return retain(rounded);
    }
    double best = std::numeric_limits<double>::infinity();
    std::tuple<int, Id, Id> bestKey{INT_MAX, UINT64_MAX, UINT64_MAX};
    Tensor result{};
    if (extension_limit > 0)
      for (const auto& cell : cells)
        for (int k = 0; k < 3; ++k) {
          if (!extension_components.count(cell.marker[k])) continue;
          const int j = (k + 1) % 3;
          const auto a = cell.triangle.v[k], b = cell.triangle.v[j];
          const long double x = static_cast<long double>(b.p.x) - a.p.x, y = static_cast<long double>(b.p.y) - a.p.y;
          const long double px = static_cast<long double>(p.x) - a.p.x, py = static_cast<long double>(p.y) - a.p.y;
          const long double fraction = std::clamp((px * x + py * y) / (x * x + y * y), 0.L, 1.L);
          const double distance = static_cast<double>(std::hypot(px - fraction * x, py - fraction * y));
          const auto key = std::make_tuple(cell.marker[k], std::min(a.id, b.id), std::max(a.id, b.id));
          if (distance < best || (distance == best && key < bestKey)) {
            best = distance;
            bestKey = key;
            std::array<long double, 3> weights{};
            weights[k] = 1 - fraction;
            weights[j] = fraction;
            result = Blend(cell, weights);
          }
        }
    if (!(best <= extension_limit && std::isfinite(best)))
      throw std::runtime_error("Query outside frozen donor and associated extension band.");
    ++extensions;
    maximum_extension = std::max(maximum_extension, best);
    return retain(result);
  }
  void Sort() {
    std::sort(cells.begin(), cells.end(), [](const auto& a, const auto& b) { return a.Key() < b.Key(); });
  }
};

class DonorField {
 public:
  World& world;
  const std::map<Id, DonorCell> owned;
  DonorField(World& w, const std::map<Id, Cell>& original) : world(w), owned(Snapshot(original)) {
    CLocalFailure failure;
    for (const auto& cell : owned) {
      if (!(area(cell.second.triangle) > 0)) failure.Set(1, cell.first, "Nonpositive native donor simplex.");
      for (const auto& m : cell.second.metric)
        if (!(NormalizedDeterminant(m.xx, m.xy, m.yy) > 1e-14L))
          failure.Set(1, cell.first, "Nonfinite or numerically singular original nodal target.");
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
  }
  std::shared_ptr<FieldPatch> import(const std::vector<Cell>& old, bool& active, size_t budget, double extensionLimit,
                                     size_t& maxDiscovered, int& rejected) {
    CLocalFailure failure;
    if (!(std::isfinite(extensionLimit) && extensionLimit >= 0))
      failure.Set(1, 0, "Invalid native target extension distance.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    std::vector<std::vector<FieldRegion>> to(world.size);
    if (active && !old.empty()) {
      FieldRegion region;
      region.caller = world.rank;
      region.lo.fill(std::numeric_limits<double>::infinity());
      region.hi.fill(-std::numeric_limits<double>::infinity());
      for (const auto& cell : old)
        for (const auto& node : cell.t.v) {
          region.lo[0] = std::min(region.lo[0], node.p.x);
          region.lo[1] = std::min(region.lo[1], node.p.y);
          region.hi[0] = std::max(region.hi[0], node.p.x);
          region.hi[1] = std::max(region.hi[1], node.p.y);
        }
      for (int k = 0; k < 2; ++k) {
        region.lo[k] = std::nextafter(region.lo[k] - extensionLimit, -std::numeric_limits<double>::infinity());
        region.hi[k] = std::nextafter(region.hi[k] + extensionLimit, std::numeric_limits<double>::infinity());
      }
      // ponytail: bounded ROI metadata scans local immutable donors. Rank-box routing/ADT comes after profiling.
      for (auto& bucket : to) bucket.push_back(region);
    }
    const auto regions = world.exchange(to);
    std::vector<std::vector<DonorId>> ids(world.size);
    for (const auto& region : regions)
      for (const auto& entry : owned) {
        std::array<double, 2> lo{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
        std::array<double, 2> hi{-lo[0], -lo[1]};
        for (const auto& v : entry.second.triangle.v) {
          lo[0] = std::min(lo[0], v.p.x);
          lo[1] = std::min(lo[1], v.p.y);
          hi[0] = std::max(hi[0], v.p.x);
          hi[1] = std::max(hi[1], v.p.y);
        }
        if (lo[0] <= region.hi[0] && hi[0] >= region.lo[0] && lo[1] <= region.hi[1] && hi[1] >= region.lo[1])
          ids[region.caller].push_back({entry.first, world.rank});
        if (ids[region.caller].size() > FIELD_LIMIT) {
          ids[region.caller].clear();
          ids[region.caller].push_back({0, -1});
          break;
        }
      }
    const auto found = world.exchange(ids);
    maxDiscovered = std::max(maxDiscovered, found.size());
    const size_t required = transfer_memory::Add(transfer_memory::Mul(old.size(), 4 * sizeof(Cell)),
                                                 transfer_memory::Mul(found.size(), 4 * sizeof(DonorCell)),
                                                 transfer_memory::Mul(QUERY_CACHE_LIMIT, 96));
    if (active && (old.empty() || found.empty() || found.size() > FIELD_LIMIT || required > budget ||
                   std::any_of(found.begin(), found.end(), [](auto r) { return r.owner < 0; }))) {
      active = false;
      ++rejected;
    }
    std::vector<std::vector<DonorRequest>> requests(world.size);
    if (active)
      for (const auto& id : found) requests[id.owner].push_back({id.id, world.rank});
    const auto queries = world.exchange(requests);
    std::vector<std::vector<DonorCell>> reply(world.size);
    for (const auto& query : queries) reply[query.caller].push_back(owned.at(query.id));
    bool payloadAdmitted = false;
    auto cells = world.exchange(
        reply, &payloadAdmitted, budget,
        transfer_memory::Add(transfer_memory::Bytes(old), transfer_memory::Bytes(found),
                             transfer_memory::Bytes(queries), transfer_memory::Bytes(requests),
                             transfer_memory::Bytes(regions), transfer_memory::Bytes(to), transfer_memory::Bytes(ids)));
    if (active && !payloadAdmitted) {
      active = false;
      ++rejected;
    }
    auto patch = std::make_shared<FieldPatch>();
    patch->cells = std::move(cells);
    patch->extension_limit = extensionLimit;
    for (const auto& cell : old)
      for (const auto marker : cell.marker)
        if (marker) patch->extension_components.insert(marker);
    patch->Sort();
    return patch;
  }

 private:
  static std::map<Id, DonorCell> Snapshot(const std::map<Id, Cell>& original) {
    std::map<Id, DonorCell> result;
    for (const auto& cell : original)
      result.emplace(cell.first, DonorCell{cell.second.t, cell.second.marker, cell.second.nodal_target});
    return result;
  }
};

/*--- Priority/reader caches sample the actual frozen field. They are not an evolving-mesh target: every
 *    geometrically changed incident cell is reconstructed and resampled before common publication. ---*/
inline void CacheTarget(Cell& cell, const Metric& target, uint64_t epoch) {
  cell.target_cache[0] = quality(cell.t, target);
  for (int k = 0; k < 3; ++k) {
    cell.nodal_target[k] = target(cell.t.v[k].p);
    cell.target_cache[k + 1] = length(cell.t.v[k], cell.t.v[(k + 1) % 3], target);
  }
  if (!(cell.target_cache[0] > 0)) throw std::runtime_error("Unusable native target quality.");
  for (const auto value : cell.target_cache)
    if (!std::isfinite(value)) throw std::runtime_error("Unrepresentable native target cache.");
  cell.target_epoch = epoch;
}

}  // namespace SU2NativeBoundary2D
