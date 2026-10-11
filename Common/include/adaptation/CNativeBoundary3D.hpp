/*!
 * \file CNativeBoundary3D.hpp
 * \brief Coupled tetrahedral edge edits on immutable planar physical facets.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeCavity3D.hpp"
#include <map>
namespace SU2Native3D {
struct Facet {
  Id id = 0;
  int marker = 0;
  std::array<Node, 3> v;
  template <class Stream>
  void Fields(Stream& s) {
    s(id, marker, v);
  }
};
struct SurfaceFace {
  Face v;
  Id facet = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(v, facet);
  }
};
struct Patch {
  std::vector<Cell> cells;
  std::vector<SurfaceFace> surface;
};
/*! Bounded immutable imported facet authority. Exact finite planar containment; certified convex coplanar same-marker
 * seams are removable. */
class PlanarReference {
 public:
  explicit PlanarReference(std::vector<Facet> facets);
  const Facet& Get(Id id) const;
  /*! Requested resident map/vector storage; allocator metadata and constructor scratch are separate. */
  size_t ResidentBytes() const;
  bool Contains(Id facet, Point p, KernelStats* stats = nullptr) const;
  Id Canonical(Id facet) const;
  /*! Finite complete triangle containment and exact orientation relative to the original physical facet. */
  bool ContainsFace(Id facet, const std::array<Point, 3>& triangle, KernelStats* stats = nullptr) const;

 private:
  std::map<Id, Facet> facets;
  std::map<Id, Id> canonical;
  std::map<Id, std::vector<Edge>> boundaries;
  std::map<Id, Node> nodes;
};
/*! Geometric private proposals only. Require the globally admitted complete edge/vertex star and authoritative
 * physical skin. Exact on-edge binary64 point; unrepresentable planar proposals reject, never snap silently.
 * first_cell is a caller-reserved ID range. Distributed IDs, metric gates and external collision checks are separate.
 * Both mesh and physical-face output remain unchanged on rejection. */
bool SplitEdge(const Patch& old, Edge edge, Node point, Id first_cell, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats = nullptr);
/*! Inserts a representable point by coning a complete edge-star cavity, without exact on-edge placement.
 * Interior skin is unchanged. A planar surface seam can be rebuilt only when its two physical triangles
 * share one canonical facet; marker junctions and unadmitted interface faces reject. The point must see
 * every retained oriented skin face and lie in the original surface fan. Metric gates are separate.
 * Fresh output is unchanged on rejection; callers reserve identities and admit complete dependencies. */
bool InsertEdgeCavity(const Patch& old, Edge edge, Node point, Id first_cell, const PlanarReference& reference,
                      Patch& fresh, std::string& reason, KernelStats* stats = nullptr);
/*! Inverse edge bisection: all removed-vertex incident cells must form paired children of the supplied endpoints.
 * Preserves original facets/markers and feature edges; this is not general boundary-vertex collapse. */
bool UndoEdgeSplit(const Patch& old, Id point, Edge endpoints, Id first_cell, const PlanarReference& reference,
                   Patch& fresh, std::string& reason, KernelStats* stats = nullptr);
/*! Contracts a vertex to an existing neighbor in its complete admitted vertex star, without split history.
 * Boundary motion must lie on every incident finite reference patch; corners/junctions cannot drift.
 * Positive cells and oriented physical faces preserve the embedded domain through the contraction homotopy.
 * Metric/quality gates and global dependency admission remain caller requirements. Fresh cells follow old
 * cell order excluding cells containing both endpoints. Rejection leaves both output vectors unchanged. */
bool CollapseVertex(const Patch& old, Id point, Id neighbor, Id first_cell, const PlanarReference& reference,
                    Patch& fresh, std::string& reason, KernelStats* stats = nullptr);
/*! Moves one complete vertex star, replacing its vertex by a caller-reserved distinct identity.
 * Vertex/cell counts are unchanged. Exact facet-contained motion and positive endpoint determinants/faces
 * preserve the embedded domain; interface-star omissions, corner drift and inversion reject atomically.
 * New identity permits surface-coordinate changes without weakening fixed-interface coordinate checks. */
bool MoveVertex(const Patch& old, Id vertex, Node point, Id first_cell, const PlanarReference& reference, Patch& fresh,
                std::string& reason, KernelStats* stats = nullptr);
/*! Fixed-skin reconnection to an existing admitted apex (including face2->3 and edge3->2/4->4).
 * Geometry/topology only: frozen metric progress, complete dependencies and global IDs remain caller duties. */
bool Reconnect(const Patch& old, Id vertex, Id first_cell, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats = nullptr);
}  // namespace SU2Native3D
