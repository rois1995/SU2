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
/*! Bounded immutable imported facet authority. Exact planar containment; no unassociated projection/tolerance. */
class PlanarReference {
 public:
  explicit PlanarReference(std::vector<Facet> facets);
  const Facet& Get(Id id) const;
  bool Contains(Id facet, Point p, KernelStats* stats = nullptr) const;

 private:
  std::map<Id, Facet> facets;
};
/*! Geometric private proposals only. Require the globally admitted complete edge/vertex star and authoritative
 * physical skin. Exact on-edge binary64 point; unrepresentable planar proposals reject, never snap silently.
 * first_cell is a caller-reserved ID range. Distributed IDs, metric gates and external collision checks are separate.
 * Both mesh and physical-face output remain unchanged on rejection. */
bool SplitEdge(const Patch& old, Edge edge, Node point, Id first_cell, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats = nullptr);
/*! Inverse edge bisection: all removed-vertex incident cells must form paired children of the supplied endpoints.
 * Preserves original facets/markers and feature edges; this is not general boundary-vertex collapse. */
bool UndoEdgeSplit(const Patch& old, Id point, Edge endpoints, Id first_cell, const PlanarReference& reference,
                   Patch& fresh, std::string& reason, KernelStats* stats = nullptr);
}  // namespace SU2Native3D
