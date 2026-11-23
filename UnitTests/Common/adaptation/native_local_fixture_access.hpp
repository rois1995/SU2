/*!
 * \file native_local_fixture_access.hpp
 * \brief Private core access shared by native local test fixtures.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalSurface3D.hpp"

#include <algorithm>
#include <iosfwd>
#include <map>
#include <stdexcept>

namespace SU2NativeLocal3D {
struct SurfaceFixtureAccess {
  static const auto& Frozen(const Mesh& mesh) { return mesh.frozen_; }
  static uint32_t SweepStamp(const Mesh& mesh) { return mesh.sweep_; }

  static double SmallestAltitude(Point a, Point b, Point c) { return Mesh::SmallestAltitude(a, b, c); }

  static void SurfaceCollapseWork(Mesh& mesh, SurfaceGeometry& geometry, PassStats& stats) {
    mesh.SurfaceCollapseWork(geometry, stats);
  }

  static bool SplitInside(Mesh& mesh, Slot seed, IdSequence& ids) {
    ++mesh.sweep_;
    return mesh.SplitInside(seed, ids, SIZE_MAX);
  }
  static bool CollapseNear(Mesh& mesh, Slot seed) {
    ++mesh.sweep_;
    mesh.MarkWallNear();
    return mesh.CollapseNear(seed, SIZE_MAX);
  }

  static Refusal PrepareSplit(Mesh mesh, Edge edge, Slot seed, SurfaceGeometry& geometry, bool both) {
    const auto ring = mesh.EdgeRing(edge, seed);
    const auto parents = mesh.SurfaceParents(edge, ring);
    REQUIRE(parents.size() == 2);
    const std::array<Node, 2> ends{mesh.points_[edge[0]].node, mesh.points_[edge[1]].node};
    const std::array<SU2Native3D::SurfaceFace, 2> faces{mesh.SurfaceFaceAt(parents[0]), mesh.SurfaceFaceAt(parents[1])};
    const auto placement = geometry.Place(ends, faces, 20);
    REQUIRE(placement.refusal == SurfaceRefusal::None);
    const auto middle = static_cast<Slot>(mesh.points_.size());
    mesh.points_.push_back({placement.point.node, mesh.points_[edge[0]].tensor});
    mesh.points_.back().surface = true;
    std::vector<Vertices> fresh;
    for (const auto cell : ring.cells) {
      for (const auto end : edge) {
        auto child = mesh.cells_[cell].vertices;
        *std::find(child.begin(), child.end(), end) = middle;
        fresh.push_back(child);
      }
    }
    std::vector<Mesh::ReplacedFace> replaced;
    for (size_t i = 0; i < (both ? 2U : 1U); ++i)
      replaced.push_back({parents[i].cell, parents[i].face, edge[0], edge[1], middle});
    const auto plan = mesh.Prepare(ring.cells, fresh, replaced);
    if (plan.Accepted()) CHECK(mesh.ExactlyPositive(plan));
    return plan.refusal;
  }

  static void LengthStep(Mesh& m, SurfaceGeometry& g, IdSequence& ids, PassStats& r) {
    m.SurfaceLength(g, ids, SIZE_MAX, r);
  }

  static void DistanceWork(Mesh& mesh, SurfaceGeometry& geometry, IdSequence& ids, PassStats& report) {
    mesh.SurfaceDistance(geometry, ids, SIZE_MAX, report);
  }

  static auto DistanceChoice(const Mesh& mesh, const SurfaceGeometry& geometry, SU2Native3D::Face face,
                             const std::set<SU2Native3D::Edge>& refused) {
    const auto parents = mesh.PhysicalFaces();
    const auto parent = std::find_if(parents.begin(), parents.end(), [&](auto candidate) {
      auto vertices = mesh.SurfaceFaceAt(candidate).v;
      std::sort(vertices.begin(), vertices.end());
      return vertices == face;
    });
    REQUIRE(parent != parents.end());
    const auto choice = mesh.ChooseDistanceEdge(*parent, geometry, geometry.Limit(), {}, refused, {}, parents.size());
    std::optional<SU2Native3D::Edge> mark;
    if (!choice.reason) {
      const auto edge = mesh.ById(choice.edge);
      mark = SU2Native3D::Edge{mesh.points_[edge[0]].node.id, mesh.points_[edge[1]].node.id};
    }
    return std::make_pair(mark, choice.reason);
  }

  static std::map<SU2Native3D::Face, bool> LongestEdgeBorders(const Mesh& mesh) {
    std::map<SU2Native3D::Face, bool> borders;
    for (const auto face : mesh.PhysicalFaces()) {
      auto vertices = mesh.SurfaceFaceAt(face).v;
      std::sort(vertices.begin(), vertices.end());
      const auto edge = mesh.LongestSurfaceEdge(face);
      if (!borders.emplace(vertices, mesh.EdgeRing(edge, face.cell).border).second)
        throw std::runtime_error("FP2-B duplicate physical face in the shifted mesh.");
    }
    return borders;
  }

  static unsigned BadClass(const Mesh&, const Vertices&, Id point_base);
  static bool HandoverBlocked(const Mesh& mesh, Vertices v) { return mesh.Blocked(v); }
  static std::vector<Slot> AcceptedSplitStar(const Mesh&, const Node&);
  static PassStats HandoverPass(std::ostream&, Mesh&, IdSequence&, size_t cell_limit, CoreOptions,
                                StateSurfaceGeometry*, Id point_base, int rank, unsigned pass,
                                unsigned pass_count, const std::string& extra);
  static void ReportQualityOptions(std::ostream&, Mesh&, size_t cell_limit, Id point_base, unsigned pass,
                                   const char* checkpoint, int rank);
  static void VolumeRounds(std::ostream&, Mesh&, IdSequence&, size_t cell_limit, PassStats&, Id point_base,
                           int rank, unsigned pass, const std::string& mode);
};
}  // namespace SU2NativeLocal3D
