/*!
 * \file native_local_surface_probe.hpp
 * \brief Observe the accepted surface splits of a production local-core pass.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once

#include "../../../Common/include/adaptation/CNativeLocalSurface3D.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <unordered_map>

namespace SU2NativeLocal3D {
class SurfaceLongestEdgeProbe final : public SurfaceGeometry {
 public:
  SurfaceLongestEdgeProbe(const Mesh& mesh, SurfaceGeometry& geometry, bool trace = false)
      : mesh_(mesh), geometry_(geometry), trace_(trace) {
    for (Slot slot = 0; slot < mesh_.Points().size(); ++slot) {
      const auto& point = mesh_.Points()[slot];
      if (!point.removed) slots_.emplace(point.node.id, slot);
    }
  }

  double Tolerance() const override { return geometry_.Tolerance(); }
  bool Feature(const SurfaceEdge& edge) const override { return geometry_.Feature(edge); }
  SurfaceSamples Samples(const SurfaceFace& face) override {
    distance_ = true;
    return geometry_.Samples(face);
  }
  std::optional<double> Chord(const std::array<Node, 2>& ends) override { return geometry_.Chord(ends); }

  SurfacePlacement Place(const std::array<Node, 2>& ends, const std::array<SurfaceFace, 2>& faces, Id id) override {
    pending_ = Split{};
    pending_->distance = distance_;
    if (distance_ || trace_) {
      pending_->edge = Key(ends[0].id, ends[1].id);
      for (size_t i = 0; i < faces.size(); ++i) {
        pending_->faces[i] = faces[i].v;
        pending_->longest[i] = IsLongest(ends, faces[i].v, pending_->edge);
      }
    }
    return geometry_.Place(ends, faces, id);
  }

  bool OnEntity(const SurfaceFace& face, const BoundaryPoint& middle) override {
    return geometry_.OnEntity(face, middle);
  }

  void Accept(SurfacePlacement&& placement) override {
    if (!pending_) throw std::logic_error("Accepted a surface split without a placement probe.");
    const auto id = placement.point.node.id;
    geometry_.Accept(std::move(placement));
    const auto& points = mesh_.Points();
    // Surface work appends points. Look up a recycled slot if the mesh contains one.
    const auto point = points.back().node.id == id ? std::prev(points.end()) :
        std::find_if(points.begin(), points.end(), [id](const auto& row) { return !row.removed && row.node.id == id; });
    if (point == points.end()) throw std::logic_error("Accepted surface point missing from the mesh.");
    slots_.insert_or_assign(id, static_cast<Slot>(point - points.begin()));
    if (pending_->distance) {
      ++checked_;
      violations_ += !pending_->longest[0] || !pending_->longest[1];
    }
    if (trace_) {
      pending_->point = id;
      accepted_.push_back(*pending_);
    }
    pending_.reset();
  }

  size_t Checked() const { return checked_; }
  size_t Violations() const { return violations_; }

  void ReportSweep(std::ostream& out, const SweepRow& sweep, int rank, unsigned pass) {
    const bool distance = std::strcmp(sweep.kind, "surface_distance") == 0;
    if (!distance && std::strcmp(sweep.kind, "surface_length") != 0) return;
    if (accepted_.size() != sweep.counters.splits) throw std::logic_error("Surface probe and sweep counts differ.");
    for (const auto& split : accepted_) {
      if (split.distance != distance) throw std::logic_error("Surface probe and sweep steps differ.");
      out << "FP2_B split rank=" << rank << " pass=" << pass << " step=" << (distance ? 'D' : 'L')
          << " sweep=" << sweep.sweep << " a=" << split.edge[0] << " b=" << split.edge[1] << " new=" << split.point;
      for (size_t i = 0; i < split.faces.size(); ++i) {
        const auto& face = split.faces[i];
        out << " f" << i << '=' << face[0] << ',' << face[1] << ',' << face[2];
      }
      out << " longest0=" << split.longest[0] << " longest1=" << split.longest[1] << '\n';
    }
    accepted_.clear();
  }

 private:
  struct Split {
    SurfaceEdge edge{};
    std::array<SurfaceFaceKey, 2> faces{};
    std::array<bool, 2> longest{};
    Id point = 0;
    bool distance = false;
  };

  static SurfaceEdge Key(Id a, Id b) { return {std::min(a, b), std::max(a, b)}; }

  static double Length(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z); }

  bool IsLongest(const std::array<Node, 2>& ends, const SurfaceFaceKey& face, const SurfaceEdge& edge) const {
    const auto third = std::find_if(face.begin(), face.end(), [&](Id id) { return id != edge[0] && id != edge[1]; });
    if (third == face.end()) throw std::logic_error("Physical face missing its third vertex.");
    const auto& point = mesh_.Points().at(slots_.at(*third));
    if (point.node.id != *third || point.removed) throw std::logic_error("Physical face has a stale vertex slot.");
    const auto length = Length(ends[0].p, ends[1].p);
    for (const auto& end : ends) {
      const auto other_length = Length(end.p, point.node.p);
      if (other_length > length || (other_length == length && Key(end.id, *third) < edge)) return false;
    }
    return true;
  }

  const Mesh& mesh_;
  SurfaceGeometry& geometry_;
  bool trace_, distance_ = false;
  std::unordered_map<Id, Slot> slots_;
  std::optional<Split> pending_;
  std::vector<Split> accepted_;
  size_t checked_ = 0, violations_ = 0;
};
}  // namespace SU2NativeLocal3D
