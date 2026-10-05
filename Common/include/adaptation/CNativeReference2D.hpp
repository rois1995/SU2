/*!
 * \file CNativeReference2D.hpp
 * \brief Immutable oriented marker polylines and component/feature identity for native 2D adaptation.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 * See the SU2 license for redistribution terms and warranty limitations.
 */

#pragma once

#include "CNativeBoundary2D.hpp"

namespace SU2NativeBoundary2D {

/*--- Construct once from original physical faces oriented with fluid on the left. MPI import and persistence
 *    belong to the adapter. This class stores geometry, not a changing target or live CFD geometry pointer. ---*/
class PolylineReference {
 public:
  struct Face {
    Node a, b;
    int marker = 0;
    template <class S>
    void Fields(S& s) {
      s(a, b, marker);
    }
  };
  struct Component {
    int marker = 0;
    bool closed = false;
    std::vector<Node> nodes;  // Closing point is repeated only at the end of closed components.
    std::vector<double> arc;
  };

  explicit PolylineReference(const std::vector<Face>& faces, double featureAngleDegrees) {
    if (!(std::isfinite(featureAngleDegrees) && featureAngleDegrees > 0 && featureAngleDegrees <= 180))
      throw std::invalid_argument("Invalid native reference feature angle.");
    std::map<int, std::map<Id, Node>> successor;
    std::map<int, std::map<Id, Id>> predecessor;
    std::map<Id, Node> points;
    std::map<Id, std::set<int>> markers;
    for (const auto& face : faces) {
      if (face.marker < 0 || face.a.id == face.b.id || !(norm(face.b.p - face.a.p) > 0))
        throw std::invalid_argument("Invalid native reference face.");
      for (const auto node : {face.a, face.b}) {
        if (!(std::isfinite(node.p.x) && std::isfinite(node.p.y)))
          throw std::invalid_argument("Nonfinite native reference point.");
        const auto inserted = points.emplace(node.id, node);
        if (!inserted.second && (inserted.first->second.p.x != node.p.x || inserted.first->second.p.y != node.p.y))
          throw std::invalid_argument("Inconsistent native reference point.");
        markers[node.id].insert(face.marker);
      }
      if (!successor[face.marker].emplace(face.a.id, face.b).second ||
          !predecessor[face.marker].emplace(face.b.id, face.a.id).second)
        throw std::invalid_argument("Branched, duplicate or inconsistently oriented native reference.");
    }
    if (faces.empty()) throw std::invalid_argument("Empty native geometry reference.");
    for (auto& marker : successor) {
      auto& remaining = marker.second;
      const auto& previous = predecessor.at(marker.first);
      while (!remaining.empty()) {
        Id start = remaining.begin()->first;
        bool closed = true;
        for (const auto& entry : remaining)
          if (!previous.count(entry.first)) {
            start = entry.first;
            closed = false;
            break;
          }
        Component component;
        component.marker = marker.first;
        component.closed = closed;
        component.nodes.push_back(points.at(start));
        component.arc.push_back(0);
        Id current = start;
        while (remaining.count(current)) {
          const auto next = remaining.at(current);
          const auto length = norm(next.p - component.nodes.back().p);
          const auto arc = component.arc.back() + length;
          if (!(std::isfinite(arc) && arc > component.arc.back()))
            throw std::invalid_argument("Unrepresentable native reference arclength.");
          component.nodes.push_back(next);
          component.arc.push_back(arc);
          remaining.erase(current);
          current = next.id;
          if (current == start) break;
        }
        if ((closed && current != start) || (!closed && current == start) || component.nodes.size() < 2)
          throw std::invalid_argument("Invalid native reference component closure.");
        components.push_back(std::move(component));
      }
    }
    const double radians = featureAngleDegrees * std::acos(-1.) / 180;
    for (size_t c = 0; c < components.size(); ++c) {
      const auto& component = components[c];
      const size_t count = component.nodes.size() - (component.closed ? 1 : 0);
      for (size_t i = 0; i < count; ++i) {
        const auto& node = component.nodes[i];
        const bool endpoint = !component.closed && (i == 0 || i + 1 == count);
        bool feature = endpoint || markers.at(node.id).size() > 1 || (node.fixed & FEATURE);
        if (!endpoint) {
          const auto a = node.p - component.nodes[(i + count - 1) % count].p;
          const auto b = component.nodes[(i + 1) % count].p - node.p;
          const double turning = std::atan2(std::abs(a.x * b.y - a.y * b.x), dot(a, b));
          feature |= turning >= radians;
        }
        if (feature) {
          features.insert(node.id);
          featurePoints.insert({node.p.x, node.p.y});
        }
      }
      for (size_t i = 1; i < component.nodes.size(); ++i)
        if (!faceComponent.emplace(edge(component.nodes[i - 1].id, component.nodes[i].id), c + 1).second)
          throw std::invalid_argument("Physical face belongs to multiple reference components.");
    }
  }

  const std::vector<Component>& Components() const { return components; }
  bool IsFeature(Id originalPoint) const { return features.count(originalPoint) != 0; }
  bool IsFeature(Point point) const { return featurePoints.count({point.x, point.y}) != 0; }
  int ComponentOfOriginalFace(Id a, Id b) const { return faceComponent.at(edge(a, b)); }
  int Marker(int component) const { return Get(component).marker; }

  Point At(int component, double arc) const {
    const auto& line = Get(component);
    const double total = line.arc.back();
    if (!std::isfinite(arc)) throw std::invalid_argument("Nonfinite reference parameter.");
    if (line.closed) {
      arc = std::fmod(arc, total);
      if (arc < 0) arc += total;
    } else
      arc = std::clamp(arc, 0., total);
    auto upper = std::upper_bound(line.arc.begin(), line.arc.end(), arc);
    const size_t i = std::min(line.arc.size() - 2, static_cast<size_t>(upper - line.arc.begin() - 1));
    const double fraction = (arc - line.arc[i]) / (line.arc[i + 1] - line.arc[i]);
    return line.nodes[i].p + (line.nodes[i + 1].p - line.nodes[i].p) * fraction;
  }

  double Parameter(int component, Point point) const {
    const auto& line = Get(component);
    if (!(std::isfinite(point.x) && std::isfinite(point.y)))
      throw std::invalid_argument("Nonfinite native projection query.");
    double distance = std::numeric_limits<double>::infinity(), best = 0;
    for (size_t i = 1; i < line.nodes.size(); ++i) {
      const auto a = line.nodes[i - 1].p, d = line.nodes[i].p - a;
      const auto fraction = std::clamp(dot(point - a, d) / dot(d, d), 0., 1.);
      const auto error = norm(point - (a + d * fraction));
      const auto parameter = line.arc[i - 1] + fraction * (line.arc[i] - line.arc[i - 1]);
      if (error < distance || (error == distance && parameter < best)) {
        distance = error;
        best = parameter;
      }
    }
    return best;
  }

  std::pair<double, double> Interval(int component, Point a, Point b) const {
    const auto& line = Get(component);
    const double first = Parameter(component, a);
    double last = Parameter(component, b);
    if (line.closed && last <= first) last += line.arc.back();
    if (!(last > first)) throw std::invalid_argument("Reference interval reverses component orientation.");
    return {first, last};
  }

  double Deviation(Physical face) const {
    const auto bounds = Interval(face.marker, face.a.p, face.b.p);
    const auto& line = Get(face.marker);
    double maximum = 0;
    auto sample = [&](double arc) {
      const auto fraction = (arc - bounds.first) / (bounds.second - bounds.first);
      maximum = std::max(maximum, norm(At(face.marker, arc) - (face.a.p + (face.b.p - face.a.p) * fraction)));
    };
    sample(bounds.first);
    sample(bounds.second);
    /*--- Between reference knots, both paths are linear in arc: the norm of their difference is convex.
     *    Checking their endpoints bounds this corresponding-parameter deviation without a sampling grid. ---*/
    for (const auto arc : line.arc)
      for (int wrap = 0; wrap <= (line.closed ? 1 : 0); ++wrap) {
        const double query = arc + wrap * line.arc.back();
        if (query > bounds.first && query < bounds.second) sample(query);
      }
    return maximum;
  }

  Reference Policy(const std::map<int, double>& heightByMarker) const {
    /*--- The returned callbacks reference immutable geometry; the adapter must retain this object.
     *    Heights are copied per call so a later request can rebuild cells without rebasing geometry. ---*/
    for (const auto& entry : heightByMarker)
      if (!(std::isfinite(entry.second) && entry.second > 0))
        throw std::invalid_argument("Invalid native marker first-height request.");
    Reference policy;
    policy.point = [this](int c, double u) { return At(c, u); };
    policy.interval = [this](int c, Point a, Point b) { return Interval(c, a, b); };
    policy.deviation = [this](Physical f) { return Deviation(f); };
    policy.height = [this, heightByMarker](int c) {
      const auto found = heightByMarker.find(Marker(c));
      return found == heightByMarker.end() ? 0. : found->second;
    };
    return policy;
  }

 private:
  std::vector<Component> components;
  std::set<Id> features;
  std::set<std::array<double, 2>> featurePoints;
  std::map<Edge, int> faceComponent;
  const Component& Get(int component) const {
    if (component <= 0 || static_cast<size_t>(component) > components.size())
      throw std::out_of_range("Unknown native reference component.");
    return components[component - 1];
  }
};

}  // namespace SU2NativeBoundary2D
