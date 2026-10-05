/*!
 * \file CNativeGeometryValidation2D.hpp
 * \brief Validate the global fluid-left physical graph before using it as an immutable 2D reference.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once
#include "CNativeReference2D.hpp"
#include <numeric>

namespace SU2NativeBoundary2D {

// Physical faces are already replicated for the polyline reference. A sorted x-box sweep avoids testing
// separated segments, retains O(B) storage, and uses the strict exact-sign predicates for actual contacts.
// The worst case is O(B^2). This preflight is independent of per-cavity dependency admission.
inline void ValidatePhysicalGraph(const std::vector<PolylineReference::Face>& faces) {
  if (faces.empty()) throw std::invalid_argument("Empty native physical graph.");
  std::map<Id, Node> points;
  std::map<std::array<double, 2>, Id> coordinateIds;
  std::map<Id, Id> successor, predecessor;
  for (const auto& face : faces) {
    for (const auto node : {face.a, face.b}) {
      if (!(std::isfinite(node.p.x) && std::isfinite(node.p.y)))
        throw std::invalid_argument("Nonfinite native physical graph point.");
      const auto stored = points.emplace(node.id, node);
      if (!stored.second && (stored.first->second.p.x != node.p.x || stored.first->second.p.y != node.p.y))
        throw std::invalid_argument("Inconsistent native physical graph point.");
      const auto unique = coordinateIds.emplace(std::array<double, 2>{node.p.x, node.p.y}, node.id);
      if (!unique.second && unique.first->second != node.id)
        throw std::invalid_argument("Distinct native physical nodes have identical coordinates.");
    }
    if (face.a.id == face.b.id || !successor.emplace(face.a.id, face.b.id).second ||
        !predecessor.emplace(face.b.id, face.a.id).second)
      throw std::invalid_argument("Branched or duplicate native physical graph.");
  }
  for (const auto& node : points)
    if (!successor.count(node.first) || !predecessor.count(node.first))
      throw std::invalid_argument("Open native physical graph.");
  std::vector<size_t> order(faces.size());
  std::iota(order.begin(), order.end(), 0);
  auto minX = [&](size_t f) { return std::min(faces[f].a.p.x, faces[f].b.p.x); };
  std::sort(order.begin(), order.end(),
            [&](size_t a, size_t b) { return std::make_pair(minX(a), a) < std::make_pair(minX(b), b); });
  for (size_t i = 0; i < order.size(); ++i) {
    const auto& a = faces[order[i]];
    for (size_t j = i + 1; j < order.size() && minX(order[j]) <= std::max(a.a.p.x, a.b.p.x); ++j) {
      const auto& b = faces[order[j]];
      if (std::min(a.a.p.y, a.b.p.y) > std::max(b.a.p.y, b.b.p.y) ||
          std::min(b.a.p.y, b.b.p.y) > std::max(a.a.p.y, a.b.p.y))
        continue;
      if (!SegmentsIntersect(a.a.p, a.b.p, b.a.p, b.b.p)) continue;
      bool shared = false;
      for (const auto av : {a.a, a.b})
        for (const auto bv : {b.a, b.b})
          if (av.id == bv.id) {
            shared = true;
            const auto otherA = av.id == a.a.id ? a.b : a.a;
            const auto otherB = bv.id == b.a.id ? b.b : b.a;
            if (OnSegment(av.p, otherA.p, otherB.p) || OnSegment(av.p, otherB.p, otherA.p))
              throw std::invalid_argument("Overlapping incident native physical faces.");
          }
      if (!shared) throw std::invalid_argument("Crossing or touching nonincident native physical faces.");
    }
  }
  std::set<Id> remaining;
  for (const auto& node : points) remaining.insert(node.first);
  std::vector<std::vector<Id>> loops;
  while (!remaining.empty()) {
    const Id start = *remaining.begin();
    std::vector<Id> loop;
    Id current = start;
    do {
      if (!remaining.erase(current)) throw std::invalid_argument("Invalid native physical loop closure.");
      loop.push_back(current);
      current = successor.at(current);
    } while (current != start);
    if (loop.size() < 3) throw std::invalid_argument("Degenerate native physical loop.");
    loops.push_back(std::move(loop));
  }
  for (size_t k = 0; k < loops.size(); ++k) {
    const auto& loop = loops[k];
    const auto extreme = std::min_element(loop.begin(), loop.end(), [&](Id a, Id b) {
      return std::tie(points.at(a).p.x, points.at(a).p.y) < std::tie(points.at(b).p.x, points.at(b).p.y);
    });
    const size_t index = extreme - loop.begin();
    const auto point = points.at(*extreme).p;
    const auto direction = orient(points.at(loop[(index + loop.size() - 1) % loop.size()]).p, point,
                                  points.at(loop[(index + 1) % loop.size()]).p);
    if (!direction) throw std::invalid_argument("Degenerate native physical loop orientation.");
    int winding = 0;
    for (size_t other = 0; other < loops.size(); ++other)
      if (other != k) {
        const auto& enclosing = loops[other];
        for (size_t j = 0; j < enclosing.size(); ++j) {
          const auto a = points.at(enclosing[j]).p, b = points.at(enclosing[(j + 1) % enclosing.size()]).p;
          if (a.y <= point.y && b.y > point.y && orient(a, b, point) > 0) ++winding;
          if (a.y > point.y && b.y <= point.y && orient(a, b, point) < 0) --winding;
        }
      }
    if (winding != (direction > 0 ? 0 : 1))
      throw std::invalid_argument("Native physical loops do not bound a single-cover fluid domain.");
  }
}
}  // namespace SU2NativeBoundary2D
