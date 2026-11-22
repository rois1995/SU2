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
    pending_->edge = Key(ends[0].id, ends[1].id);
    if (distance_ || trace_) {
      for (size_t i = 0; i < faces.size(); ++i) {
        pending_->faces[i] = faces[i].v;
        pending_->longest[i] = IsLongest(ends, faces[i].v, pending_->edge);
      }
    }
    auto placement = geometry_.Place(ends, faces, id);
    pending_->walk = placement.walk;
    if (placement.walk == SurfaceWalk::Tolerance && !placement.Accepted()) {
      if (placement.refusal == SurfaceRefusal::Protected) {
        ++tolerance_protected_;
      } else if (placement.refusal == SurfaceRefusal::Duplicate) {
        ++tolerance_duplicate_;
      } else {
        throw std::logic_error("Unexpected refusal of a tolerance placement.");
      }
      pending_.reset();
    }
    return placement;
  }

  void RefusedAfterPlace(const SurfaceEdge& edge, SurfaceRefusal refusal) {
    if (!pending_ || pending_->walk != SurfaceWalk::Tolerance || pending_->edge != edge) return;
    switch (refusal) {
      case SurfaceRefusal::Outward:
      case SurfaceRefusal::Near:
      case SurfaceRefusal::Size:
      case SurfaceRefusal::Validity:
      case SurfaceRefusal::Topology:
      case SurfaceRefusal::Entity:
        ++tolerance_after_[static_cast<size_t>(refusal)];
        break;
      default:
        throw std::logic_error("Unexpected refusal after a tolerance placement.");
    }
    pending_.reset();
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
    tolerance_accepted_ += pending_->walk == SurfaceWalk::Tolerance;
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

  bool ReportTolerance(std::ostream& out, int rank, unsigned pass, size_t expected) const {
    const auto after = [&](SurfaceRefusal refusal) { return tolerance_after_[static_cast<size_t>(refusal)]; };
    const auto a = tolerance_protected_ + tolerance_duplicate_;
    const auto b = tolerance_accepted_;
    const auto c = after(SurfaceRefusal::Outward) + after(SurfaceRefusal::Near) + after(SurfaceRefusal::Size) +
                   after(SurfaceRefusal::Validity) + after(SurfaceRefusal::Topology) + after(SurfaceRefusal::Entity);
    out << "FP2_B walk_probe rank=" << rank << " pass=" << pass << " a_protected=" << tolerance_protected_
        << " a_duplicate=" << tolerance_duplicate_ << " b_accepted=" << b
        << " c_outward=" << after(SurfaceRefusal::Outward) << " c_near=" << after(SurfaceRefusal::Near)
        << " c_size=" << after(SurfaceRefusal::Size) << " c_validity=" << after(SurfaceRefusal::Validity)
        << " c_topology=" << after(SurfaceRefusal::Topology) << " c_entity=" << after(SurfaceRefusal::Entity)
        << " a=" << a << " b=" << b << " c=" << c << " total=" << a + b + c << '\n';
    return a + b + c == expected && b > 0;
  }

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
    SurfaceWalk walk = SurfaceWalk::None;
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
  size_t tolerance_protected_ = 0, tolerance_duplicate_ = 0, tolerance_accepted_ = 0;
  SurfaceSweepOutcomes tolerance_after_{};
};
}  // namespace SU2NativeLocal3D
